/*
 * WowPlus group care: the small things that make a group of bots behave like a group.
 *
 * 1. An honest ready check. The bot module answers every ready check with "ready". Here a ready check
 *    by a player whose group is otherwise bots is answered by this file instead: a bot that is dead,
 *    far away, low on health or low on mana answers "not ready", sits down to eat or drink, and one of
 *    them says why. The game's own ready check window shows the ticks and crosses.
 *
 * 2. A healer that speaks up. Out of combat, a healer below a share of mana says so once and drinks,
 *    so that the player knows to wait before the next pull.
 *
 * 3. Running back. The bot module leaves a dead bot lying in a dungeon until someone resurrects it;
 *    after a wipe that is nobody. Here a bot that has been dead for a while, once the player is alive
 *    and out of combat, is back on its feet next to the player, as if it had run back too. When
 *    someone alive in the group can resurrect, they get more time to do it first.
 *
 * Marks need nothing here: the bot module's damage dealers and tanks already go for the skull, and
 * mages, hunters and druids keep the moon out of the fight. The bot bar has buttons for both.
 *
 * Settings: WowPlus.Group.*
 */

#include "Chat.h"
#include "Config.h"
#include "Group.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WowPlusBotChat.h"

#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr uint32 TICK = 2000;
    constexpr float TOO_FAR = 80.0f;
    constexpr uint32 MANA_LINE_PAUSE = 3 * MINUTE * IN_MILLISECONDS;    // between two "need to drink" lines in one group
    constexpr uint32 BACK_LINE_PAUSE = 2 * MINUTE * IN_MILLISECONDS;    // between two "made it back" lines

    bool readyCheck = true;
    uint32 readyHealth = 70;
    uint32 readyMana = 40;
    uint32 readyHealerMana = 60;
    uint32 healerAsksBelow = 30;
    uint32 runBackSeconds = 20;
    uint32 runBackWithHealerSeconds = 60;

    struct Answer
    {
        ObjectGuid bot;
        bool ready;
        uint32 delay;                                       // ms until the tick or cross shows
    };

    struct Care
    {
        std::vector<Answer> answers;                        // a ready check that is being answered
        uint32 untilManaLine = 0;
        uint32 untilBackLine = 0;
        uint32 untilNextBack = 0;                           // they get up one after another
        std::unordered_map<ObjectGuid, uint32> deadFor;     // bot -> ms it has been dead
    };

    std::unordered_map<ObjectGuid, Care> cares;
    uint32 sinceTick = 0;

    bool IsBot(Player const* player)
    {
        return player->GetSession()->IsHeadless();
    }

    std::vector<Player*> BotsOf(Player* player)
    {
        std::vector<Player*> bots;
        if (Group* group = player->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                if (Player* member = itr->GetSource())
                    if (member != player && member->IsInWorld() && IsBot(member))
                        bots.push_back(member);
        return bots;
    }

    bool OnlyBotsWith(Player* player)
    {
        Group* group = player->GetGroup();
        if (!group)
            return false;

        uint32 bots = 0;
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            if (Player* member = itr->GetSource())
            {
                if (member != player && !IsBot(member))
                    return false;                           // another person: the game's own ready check
                bots += member != player;
            }
        return bots > 0;
    }

    bool Heals(Player* bot)
    {
        return bot->GetLevel() >= 10 ? PlayerbotAI::IsHeal(bot, true) : bot->getClass() == CLASS_PRIEST;
    }

    bool CanResurrect(Player* bot)
    {
        switch (bot->getClass())
        {
            case CLASS_PRIEST: case CLASS_PALADIN: case CLASS_SHAMAN: case CLASS_DRUID:
                return true;
            default:
                return false;
        }
    }

    void Rest(Player* bot)
    {
        PlayerbotAI* ai = GET_PLAYERBOT_AI(bot);
        if (!ai || bot->IsInCombat() || !bot->IsAlive())
            return;

        if (bot->getPowerType() == POWER_MANA && bot->GetPowerPct(POWER_MANA) < 90.0f)
            ai->DoSpecificAction("drink", Event(), true);
        else if (bot->GetHealthPct() < 90.0f)
            ai->DoSpecificAction("food", Event(), true);
    }

    // Why this bot is not ready, in words for the bot itself. Empty: ready.
    std::string NotReady(Player* bot, Player* player)
    {
        if (!bot->IsAlive())
            return "you are dead";
        if (bot->GetMapId() != player->GetMapId() || !bot->IsWithinDist(player, TOO_FAR))
            return "you are not with the group yet";
        if (bot->GetHealthPct() < float(readyHealth))
            return "you are at " + std::to_string(uint32(bot->GetHealthPct())) + " percent health and need to eat";
        if (bot->getPowerType() == POWER_MANA && bot->GetPowerPct(POWER_MANA) < float(Heals(bot) ? readyHealerMana : readyMana))
            return "you are at " + std::to_string(uint32(bot->GetPowerPct(POWER_MANA))) + " percent mana and need to drink";
        return "";
    }

    // The player's ready check, answered here. The bots never see it.
    void AnswerReadyCheck(Player* player)
    {
        Care& care = cares[player->GetGUID()];
        care.answers.clear();

        WorldPacket started(MSG_RAID_READY_CHECK, 8);
        started << player->GetGUID();
        player->SendDirectMessage(&started);

        Player* speaker = nullptr;
        std::string speakerReason, summary;
        uint32 delay = 0;
        for (Player* bot : BotsOf(player))
        {
            std::string const reason = NotReady(bot, player);
            delay += urand(400, 1500);
            care.answers.push_back({bot->GetGUID(), reason.empty(), delay});
            if (reason.empty())
                continue;

            Rest(bot);
            summary += (summary.empty() ? "" : ", ") + bot->GetName();
            if (bot->IsAlive() && (!speaker || (Heals(bot) && !Heals(speaker))))
            {
                speaker = bot;
                speakerReason = reason;
            }
        }

        if (!summary.empty())
            ChatHandler(player->GetSession()).PSendSysMessage("Not ready: {}.", summary);
        if (speaker)
            WowPlus::BotReactsInParty(speaker, player, std::string(player->GetName()) + " asked whether everyone is ready. You are not: " +
                speakerReason + ". Say so in a few words.");
    }

    void UpdateAnswers(Player* player, Care& care)
    {
        if (care.answers.empty())
            return;

        for (auto answer = care.answers.begin(); answer != care.answers.end();)
        {
            if (answer->delay > TICK)
            {
                answer->delay -= TICK;
                ++answer;
                continue;
            }

            WorldPacket confirm(MSG_RAID_READY_CHECK_CONFIRM, 9);
            confirm << answer->bot;
            confirm << uint8(answer->ready ? 1 : 0);
            player->SendDirectMessage(&confirm);
            answer = care.answers.erase(answer);
        }

        if (care.answers.empty())
        {
            WorldPacket finished(MSG_RAID_READY_CHECK_FINISHED, 0);
            player->SendDirectMessage(&finished);
        }
    }

    void Lower(uint32& timer)
    {
        timer = timer > TICK ? timer - TICK : 0;
    }

    void UpdateGroup(Player* player, Care& care)
    {
        Lower(care.untilManaLine);
        Lower(care.untilBackLine);
        Lower(care.untilNextBack);

        std::vector<Player*> const bots = BotsOf(player);
        bool const calm = player->IsAlive() && !player->IsInCombat();

        bool someoneCanResurrect = false;
        for (Player* bot : bots)
            someoneCanResurrect |= bot->IsAlive() && CanResurrect(bot) && bot->GetMapId() == player->GetMapId();

        for (Player* bot : bots)
        {
            PlayerbotAI* ai = GET_PLAYERBOT_AI(bot);
            if (!ai || ai->GetMaster() != player)
                continue;

            if (bot->IsAlive())
            {
                care.deadFor.erase(bot->GetGUID());

                // A healer low on mana says so, once, and drinks
                if (healerAsksBelow && calm && !care.untilManaLine && !bot->IsInCombat() && Heals(bot) &&
                    bot->getPowerType() == POWER_MANA && bot->GetPowerPct(POWER_MANA) < float(healerAsksBelow) &&
                    bot->GetMapId() == player->GetMapId() && bot->IsWithinDist(player, TOO_FAR))
                {
                    care.untilManaLine = MANA_LINE_PAUSE;
                    Rest(bot);
                    WowPlus::BotReactsInParty(bot, player, "You are the healer and down to " + std::to_string(uint32(bot->GetPowerPct(POWER_MANA))) +
                        " percent mana. You are sitting down to drink; ask the group to wait a moment.");
                }
                continue;
            }

            // Dead: after a while, and once the player is alive and out of a fight, it has "run back"
            uint32& deadFor = care.deadFor[bot->GetGUID()];
            deadFor += TICK;
            uint32 const wait = (someoneCanResurrect ? runBackWithHealerSeconds : runBackSeconds) * IN_MILLISECONDS;
            if (!wait || !calm || deadFor < wait || care.untilNextBack || bot->IsBeingTeleported() || bot->InBattleground())
                continue;

            if (bot->GetMapId() != player->GetMapId() || !bot->IsWithinDist(player, 30.0f))
                bot->TeleportTo(player->GetMapId(), player->GetPositionX() + frand(-2.0f, 2.0f), player->GetPositionY() + frand(-2.0f, 2.0f),
                    player->GetPositionZ() + 0.5f, player->GetOrientation());
            bot->ResurrectPlayer(0.5f);
            bot->SpawnCorpseBones();
            care.deadFor.erase(bot->GetGUID());
            care.untilNextBack = urand(3, 7) * IN_MILLISECONDS;

            if (!care.untilBackLine)
            {
                care.untilBackLine = BACK_LINE_PAUSE;
                WowPlus::BotReactsInParty(bot, player, "You died in that fight and have just made it back to the group on foot. Say one short thing about it.");
            }
            break;                                          // one at a time
        }
    }
}

class WowPlusGroupCareWorld : public WorldScript
{
public:
    WowPlusGroupCareWorld() : WorldScript("WowPlusGroupCareWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        readyCheck = sConfigMgr->GetOption<bool>("WowPlus.Group.ReadyCheck", true);
        readyHealth = sConfigMgr->GetOption<uint32>("WowPlus.Group.ReadyCheck.Health", 70);
        readyMana = sConfigMgr->GetOption<uint32>("WowPlus.Group.ReadyCheck.Mana", 40);
        readyHealerMana = sConfigMgr->GetOption<uint32>("WowPlus.Group.ReadyCheck.HealerMana", 60);
        healerAsksBelow = sConfigMgr->GetOption<uint32>("WowPlus.Group.HealerAsksBelowMana", 30);
        runBackSeconds = sConfigMgr->GetOption<uint32>("WowPlus.Group.RunBackSeconds", 20);
        runBackWithHealerSeconds = sConfigMgr->GetOption<uint32>("WowPlus.Group.RunBackWithHealerSeconds", 60);
    }

    void OnUpdate(uint32 diff) override
    {
        sinceTick += diff;
        if (sinceTick < TICK)
            return;
        sinceTick = 0;

        std::vector<Player*> players;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
                if (player->IsInWorld() && !IsBot(player) && player->GetGroup())
                    players.push_back(player);
        }

        for (Player* player : players)
        {
            Care& care = cares[player->GetGUID()];
            UpdateAnswers(player, care);
            UpdateGroup(player, care);
        }

        // Forget who is gone
        std::erase_if(cares, [](auto const& entry) { return !ObjectAccessor::FindConnectedPlayer(entry.first); });
    }
};

class WowPlusGroupCareServer : public ServerScript
{
public:
    WowPlusGroupCareServer() : ServerScript("WowPlusGroupCareServer", {SERVERHOOK_CAN_PACKET_RECEIVE}) { }

    // Runs on the world thread, just before the game handles the packet
    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        if (!readyCheck || packet.GetOpcode() != MSG_RAID_READY_CHECK || !packet.empty())
            return true;

        Player* player = session->GetPlayer();
        if (!player || IsBot(player) || !player->GetGroup() || !OnlyBotsWith(player) || player->InBattleground())
            return true;

        Group* group = player->GetGroup();
        if (!group->IsLeader(player->GetGUID()) && !group->IsAssistant(player->GetGUID()))
            return true;

        AnswerReadyCheck(player);
        return false;                                       // the bots answer through this file, not by themselves
    }
};

void AddSC_WowPlusGroupCare()
{
    new WowPlusGroupCareWorld();
    new WowPlusGroupCareServer();
}
