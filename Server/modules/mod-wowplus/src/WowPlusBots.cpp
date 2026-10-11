/*
 * WowPlus bots: small things that make the random bots better company. See Settings/Docs/Bots.md.
 *
 * - Passing buffs: now and then a bot standing or walking near a real player casts its class
 *   buff on them, the way a friendly stranger would.
 * - Being alone: ".bots away" sends every free bot near the player somewhere else, and
 *   ".bots alone on" keeps doing that, for levelling in peace. Bots in the player's party
 *   stay; so do bots that belong to someone.
 * - Recruiting: most bots are in guilds of their own. A bot you invite with /ginvite leaves its
 *   bot guild first, so the invitation works on anyone.
 * - Guild mates keep up: a random bot in the player's guild is brought to the player's level,
 *   with gear to match, whenever it has fallen behind or is far ahead. Someone invited today
 *   has "caught up" by the next login.
 * - Companions: up to two guild mates the player has chosen. ".bots call" brings them into
 *   the party, ".bots dismiss" lets them go. While in the party they do small things by
 *   themselves: light a fire and sit down when the player rests, a gesture or a remark on
 *   the road.
 *
 * Bot chat through Claude is in WowPlusBotChat.cpp. How well a bot is dressed is a small change
 * in the bot module itself (PlayerbotFactory.cpp, look for "WowPlus").
 */

#include "CharacterCache.h"
#include "Chat.h"
#include "CommandScript.h"
#include "Config.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotFactory.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WowPlusBotChat.h"

#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

using namespace Acore::ChatCommands;

namespace
{
    // First rank of each buff. The bot casts the highest rank it knows that the player's level allows.
    constexpr uint32 SPELL_POWER_WORD_FORTITUDE = 1243;
    constexpr uint32 SPELL_ARCANE_INTELLECT = 1459;
    constexpr uint32 SPELL_MARK_OF_THE_WILD = 1126;
    constexpr uint32 SPELL_BLESSING_OF_MIGHT = 19740;
    constexpr uint32 SPELL_BLESSING_OF_WISDOM = 19742;
    constexpr uint32 SPELL_BLESSING_OF_KINGS = 20217;
    constexpr uint32 SPELL_BASIC_CAMPFIRE = 818;

    constexpr uint32 CHECK_INTERVAL = 5000;                 // ms between looks at the real players
    constexpr uint32 GUILD_CHECK_INTERVAL = 30000;          // ms between looks at the guild mates' levels
    constexpr uint8 DEATH_KNIGHT_FIRST_LEVEL = 55;
    constexpr uint32 MAX_COMPANIONS = 2;
    constexpr float COMPANION_RANGE = 40.0f;

    std::string const SETTING_SOURCE = "mod-wowplus-bots";
    constexpr uint32 SETTING_ALONE = 0;
    constexpr uint32 SETTING_COMPANION_FIRST = 1;           // and the one after it: the companions' GUIDs

    bool buffsEnabled = true;
    uint32 buffMinSeconds = 90;
    uint32 buffMaxSeconds = 240;
    float buffRadius = 20.0f;
    float awayRadius = 200.0f;
    bool guildRecruits = true;
    bool guildKeepsUp = true;
    uint32 guildLevelGap = 2;
    bool companionsEnabled = true;
    uint32 companionIdleSeconds = 60;
    bool companionCampfire = true;
    uint32 companionRoadMinMinutes = 3;
    uint32 companionRoadMaxMinutes = 7;

    // What a real player's companions are up to
    struct CompanionState
    {
        Position lastPosition;
        uint32 mapId = 0;
        uint32 idle = 0;                                    // ms the player has stood still
        bool fireLit = false;
        uint32 untilIdleAct = 0;                            // ms
        uint32 untilRoadAct = 0;                            // ms
        std::vector<ObjectGuid> sitting;
    };

    std::unordered_map<ObjectGuid, uint32> nextBuffAt;      // real player -> ms until the next passing buff
    std::unordered_map<ObjectGuid, CompanionState> companionStates;
    uint32 sinceCheck = 0;
    uint32 sinceGuildCheck = 0;

    bool IsBot(Player const* player)
    {
        return player->GetSession()->IsHeadless();
    }

    std::vector<Player*> PlayersNear(Player* center, float radius, bool bots)
    {
        std::vector<Player*> found;
        std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
            if (other != center && other->IsInWorld() && IsBot(other) == bots && other->GetMap() == center->GetMap() &&
                other->IsWithinDist(center, radius))
                found.push_back(other);

        return found;
    }

    // ---------------------------------------------------------------------------------------
    // Passing buffs
    // ---------------------------------------------------------------------------------------

    // The buff this bot would give this player, as the first rank of the spell. 0: none.
    uint32 BuffFor(Player* bot, Player* player)
    {
        bool const usesMana = player->getPowerType() == POWER_MANA;
        switch (bot->getClass())
        {
            case CLASS_PRIEST:
                return SPELL_POWER_WORD_FORTITUDE;
            case CLASS_MAGE:
                return usesMana ? SPELL_ARCANE_INTELLECT : 0;
            case CLASS_DRUID:
                return SPELL_MARK_OF_THE_WILD;
            case CLASS_PALADIN:
                if (bot->HasSpell(SPELL_BLESSING_OF_KINGS))
                    return SPELL_BLESSING_OF_KINGS;
                return usesMana && player->getClass() != CLASS_HUNTER ? SPELL_BLESSING_OF_WISDOM : SPELL_BLESSING_OF_MIGHT;
            default:
                return 0;
        }
    }

    // The rank to cast, or nullptr when the bot cannot or the player already has the buff
    SpellInfo const* RankToCast(Player* bot, Player* player, uint32 firstRank)
    {
        SpellInfo const* best = nullptr;
        for (SpellInfo const* rank = sSpellMgr->GetSpellInfo(firstRank); rank; rank = rank->GetNextRankSpell())
        {
            if (player->HasAura(rank->Id))
                return nullptr;
            if (bot->HasSpell(rank->Id))
                best = rank;
        }

        return best ? best->GetAuraRankForLevel(player->GetLevel()) : nullptr;
    }

    void TryPassingBuff(Player* player)
    {
        if (!player->IsAlive() || player->IsInCombat() || player->IsInFlight() || player->HasStealthAura())
            return;

        for (Player* bot : PlayersNear(player, buffRadius, true))
        {
            if (!bot->IsAlive() || bot->IsInCombat() || bot->IsMounted() || bot->GetTeamId() != player->GetTeamId() ||
                bot->IsNonMeleeSpellCast(false) || (player->GetGroup() && player->GetGroup() == bot->GetGroup()))
                continue;                                   // bots in the party buff by themselves

            uint32 const buff = BuffFor(bot, player);
            SpellInfo const* rank = buff ? RankToCast(bot, player, buff) : nullptr;
            if (!rank || !bot->IsWithinLOSInMap(player))
                continue;

            bot->SetFacingToObject(player);
            bot->CastSpell(player, rank->Id, true);
            return;
        }
    }

    // ---------------------------------------------------------------------------------------
    // Being alone
    // ---------------------------------------------------------------------------------------

    // A free bot: a random bot that is not in this player's party and serves no real player
    bool IsFreeBot(Player* bot, Player* player)
    {
        if (!sRandomPlayerbotMgr.IsRandomBot(bot) || (player->GetGroup() && player->GetGroup() == bot->GetGroup()))
            return false;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        return botAI && !botAI->HasGameClientMaster();
    }

    uint32 SendBotsAway(Player* player)
    {
        uint32 sent = 0;
        for (Player* bot : PlayersNear(player, awayRadius, true))
        {
            if (!bot->IsAlive() || bot->IsInCombat() || bot->IsBeingTeleported() || bot->IsInFlight() || !IsFreeBot(bot, player))
                continue;

            sRandomPlayerbotMgr.RandomTeleportForLevel(bot);
            ++sent;
        }

        return sent;
    }

    bool WantsToBeAlone(Player* player)
    {
        return player->GetPlayerSetting(SETTING_SOURCE, SETTING_ALONE).value != 0;
    }

    // ---------------------------------------------------------------------------------------
    // Guild mates keep up
    // ---------------------------------------------------------------------------------------

    bool SafeToChange(Player* bot)
    {
        return bot->IsInWorld() && bot->IsAlive() && !bot->IsInCombat() && !bot->IsInFlight() && !bot->IsBeingTeleported() &&
            !bot->InBattleground() && !bot->GetMap()->IsDungeon() && !bot->GetSession()->IsLoggingOut();
    }

    void GuildMatesKeepUp(Player* player)
    {
        if (!player->GetGuildId())
            return;

        std::vector<Player*> mates;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
                if (other != player && other->IsInWorld() && IsBot(other) && other->GetGuildId() == player->GetGuildId())
                    mates.push_back(other);
        }

        uint8 const maxLevel = uint8(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
        for (Player* bot : mates)
        {
            int32 const gap = int32(bot->GetLevel()) - int32(player->GetLevel());
            if (uint32(std::abs(gap)) <= guildLevelGap || !sRandomPlayerbotMgr.IsRandomBot(bot) || !SafeToChange(bot))
                continue;

            uint8 const level = uint8(std::clamp<int32>(int32(player->GetLevel()) + irand(-1, 1), 1, maxLevel));
            if (bot->getClass() == CLASS_DEATH_KNIGHT && level < DEATH_KNIGHT_FIRST_LEVEL)
                continue;                                   // a death knight cannot be lower than its starting level

            if (bot->IsMounted())
                bot->Dismount();

            uint8 const oldLevel = bot->GetLevel();
            PlayerbotFactory factory(bot, level);
            factory.Randomize(false);
            LOG_INFO("module", "WowPlus bots: guild mate {} went from level {} to {} to keep up with {}.",
                bot->GetName(), oldLevel, level, player->GetName());

            // Somewhere that suits the new level, unless they are travelling with the player
            if (!player->GetGroup() || player->GetGroup() != bot->GetGroup())
                sRandomPlayerbotMgr.RandomTeleportForLevel(bot);
        }
    }

    // ---------------------------------------------------------------------------------------
    // Companions
    // ---------------------------------------------------------------------------------------

    std::vector<ObjectGuid::LowType> CompanionIds(Player* player)
    {
        std::vector<ObjectGuid::LowType> ids;
        for (uint32 i = 0; i < MAX_COMPANIONS; ++i)
            if (uint32 const id = player->GetPlayerSetting(SETTING_SOURCE, SETTING_COMPANION_FIRST + i).value)
                ids.push_back(id);

        return ids;
    }

    void StoreCompanionIds(Player* player, std::vector<ObjectGuid::LowType> const& ids)
    {
        for (uint32 i = 0; i < MAX_COMPANIONS; ++i)
            player->UpdatePlayerSetting(SETTING_SOURCE, SETTING_COMPANION_FIRST + i, i < ids.size() ? uint32(ids[i]) : 0);
    }

    // The companions that are online right now
    std::vector<Player*> Companions(Player* player)
    {
        std::vector<Player*> found;
        for (ObjectGuid::LowType id : CompanionIds(player))
            if (Player* bot = ObjectAccessor::FindPlayerByLowGUID(id))
                if (bot->IsInWorld() && IsBot(bot))
                    found.push_back(bot);

        return found;
    }

    void StandUp(CompanionState& state)
    {
        for (ObjectGuid guid : state.sitting)
            if (Player* bot = ObjectAccessor::FindConnectedPlayer(guid))
                if (bot->IsInWorld() && bot->IsSitState())
                    bot->SetStandState(UNIT_STAND_STATE_STAND);

        state.sitting.clear();
    }

    void UpdateCompanions(Player* player, uint32 elapsed)
    {
        CompanionState& state = companionStates[player->GetGUID()];

        bool const moved = state.mapId != player->GetMapId() || player->GetExactDist(&state.lastPosition) > 2.0f;
        if (moved)
        {
            state.lastPosition = player->GetPosition();
            state.mapId = player->GetMapId();
            state.idle = 0;
            state.fireLit = false;
            StandUp(state);
        }
        else
            state.idle += elapsed;

        if (!player->IsAlive() || player->IsInCombat() || player->IsInFlight() || player->InBattleground() || !player->GetGroup())
            return;

        // Those travelling with the player and free to do something
        std::vector<Player*> present;
        for (Player* bot : Companions(player))
            if (bot->GetGroup() == player->GetGroup() && bot->GetMap() == player->GetMap() && bot->IsAlive() && !bot->IsInCombat() &&
                !bot->IsMounted() && bot->IsWithinDist(player, COMPANION_RANGE))
                present.push_back(bot);

        if (present.empty())
            return;

        Player* bot = present[urand(0, present.size() - 1)];

        if (state.idle >= companionIdleSeconds * IN_MILLISECONDS)
        {
            // Resting: a fire first, then small things now and then
            if (companionCampfire && !state.fireLit && !player->GetMap()->IsDungeon())
            {
                state.fireLit = true;
                bot->CastSpell(bot, SPELL_BASIC_CAMPFIRE, true);
                state.untilIdleAct = urand(8, 15) * IN_MILLISECONDS;
                return;
            }

            if (state.untilIdleAct > elapsed)
            {
                state.untilIdleAct -= elapsed;
                return;
            }

            state.untilIdleAct = urand(25, 60) * IN_MILLISECONDS;
            switch (urand(0, 5))
            {
                case 0:
                case 1:
                    if (!bot->IsSitState())
                    {
                        bot->SetStandState(UNIT_STAND_STATE_SIT);
                        state.sitting.push_back(bot->GetGUID());
                    }
                    break;
                case 2:
                    bot->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
                    break;
                case 3:
                    bot->HandleEmoteCommand(bot->IsSitState() ? EMOTE_ONESHOT_LAUGH : EMOTE_ONESHOT_DANCE);
                    break;
                default:
                    WowPlus::BotRemarksInParty(bot, player);
                    break;
            }
            return;
        }

        // On the road: a gesture or a remark every few minutes
        if (!state.untilRoadAct)
            state.untilRoadAct = urand(companionRoadMinMinutes, companionRoadMaxMinutes) * MINUTE * IN_MILLISECONDS;

        if (state.untilRoadAct > elapsed)
        {
            state.untilRoadAct -= elapsed;
            return;
        }

        state.untilRoadAct = urand(companionRoadMinMinutes, companionRoadMaxMinutes) * MINUTE * IN_MILLISECONDS;
        if (roll_chance_i(50))
            WowPlus::BotRemarksInParty(bot, player);
        else
        {
            static uint32 const gestures[] = {EMOTE_ONESHOT_POINT, EMOTE_ONESHOT_CHEER, EMOTE_ONESHOT_TALK, EMOTE_ONESHOT_WAVE};
            bot->HandleEmoteCommand(gestures[urand(0, 3)]);
        }
    }
}

class WowPlusBotsWorld : public WorldScript
{
public:
    WowPlusBotsWorld() : WorldScript("WowPlusBotsWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        buffsEnabled = sConfigMgr->GetOption<bool>("WowPlus.Bots.Buffs.Enable", true);
        buffMinSeconds = sConfigMgr->GetOption<uint32>("WowPlus.Bots.Buffs.MinSeconds", 90);
        buffMaxSeconds = std::max(buffMinSeconds, sConfigMgr->GetOption<uint32>("WowPlus.Bots.Buffs.MaxSeconds", 240));
        buffRadius = sConfigMgr->GetOption<float>("WowPlus.Bots.Buffs.Radius", 20.0f);
        awayRadius = sConfigMgr->GetOption<float>("WowPlus.Bots.Away.Radius", 200.0f);
        guildRecruits = sConfigMgr->GetOption<bool>("WowPlus.Bots.Guild.Recruit", true);
        guildKeepsUp = sConfigMgr->GetOption<bool>("WowPlus.Bots.Guild.KeepUp", true);
        guildLevelGap = sConfigMgr->GetOption<uint32>("WowPlus.Bots.Guild.KeepUp.LevelGap", 2);
        companionsEnabled = sConfigMgr->GetOption<bool>("WowPlus.Bots.Companions.Enable", true);
        companionIdleSeconds = sConfigMgr->GetOption<uint32>("WowPlus.Bots.Companions.IdleSeconds", 60);
        companionCampfire = sConfigMgr->GetOption<bool>("WowPlus.Bots.Companions.Campfire", true);
        companionRoadMinMinutes = std::max<uint32>(1, sConfigMgr->GetOption<uint32>("WowPlus.Bots.Companions.Road.MinMinutes", 3));
        companionRoadMaxMinutes = std::max(companionRoadMinMinutes, sConfigMgr->GetOption<uint32>("WowPlus.Bots.Companions.Road.MaxMinutes", 7));
    }

    // The world thread, after the maps have been updated: safe to look at and move players
    void OnUpdate(uint32 diff) override
    {
        sinceCheck += diff;
        sinceGuildCheck += diff;
        if (sinceCheck < CHECK_INTERVAL)
            return;

        uint32 const elapsed = sinceCheck;
        sinceCheck = 0;

        bool const guildCheck = guildKeepsUp && sinceGuildCheck >= GUILD_CHECK_INTERVAL;
        if (guildCheck)
            sinceGuildCheck = 0;

        std::vector<Player*> realPlayers;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
                if (player->IsInWorld() && !IsBot(player) && !player->IsBeingTeleported())
                    realPlayers.push_back(player);
        }

        for (Player* player : realPlayers)
        {
            if (guildCheck)
                GuildMatesKeepUp(player);

            if (companionsEnabled)
                UpdateCompanions(player, elapsed);

            if (WantsToBeAlone(player))
            {
                SendBotsAway(player);
                continue;                                   // no passing buffs from bots that are being sent off
            }

            if (!buffsEnabled)
                continue;

            auto [timer, isNew] = nextBuffAt.try_emplace(player->GetGUID(), urand(buffMinSeconds, buffMaxSeconds) * IN_MILLISECONDS);
            if (timer->second > elapsed)
            {
                timer->second -= elapsed;
                continue;
            }

            TryPassingBuff(player);
            timer->second = urand(buffMinSeconds, buffMaxSeconds) * IN_MILLISECONDS;
        }
    }
};

class WowPlusBotsPlayer : public PlayerScript
{
public:
    WowPlusBotsPlayer() : PlayerScript("WowPlusBotsPlayer") { }

    void OnPlayerLogout(Player* player) override
    {
        nextBuffAt.erase(player->GetGUID());
        companionStates.erase(player->GetGUID());
    }
};

class WowPlusBotsCommands : public CommandScript
{
public:
    WowPlusBotsCommands() : CommandScript("WowPlusBotsCommands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable companionTable =
        {
            { "add",    HandleCompanionAdd,    SEC_PLAYER, Console::No },
            { "remove", HandleCompanionRemove, SEC_PLAYER, Console::No },
            { "list",   HandleCompanionList,   SEC_PLAYER, Console::No },
        };

        static ChatCommandTable botsTable =
        {
            { "away",      HandleAway,    SEC_PLAYER, Console::No },
            { "alone",     HandleAlone,   SEC_PLAYER, Console::No },
            { "call",      HandleCall,    SEC_PLAYER, Console::No },
            { "dismiss",   HandleDismiss, SEC_PLAYER, Console::No },
            { "companion", companionTable },
        };

        static ChatCommandTable commandTable =
        {
            { "bots", botsTable },
        };

        return commandTable;
    }

    // .bots away: every free bot nearby goes somewhere else, once
    static bool HandleAway(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        uint32 const sent = SendBotsAway(player);
        handler->PSendSysMessage(sent ? "{} bots nearby have gone somewhere else." : "There are no free bots nearby.", sent);
        return true;
    }

    // .bots alone on|off: keep free bots out of the area for as long as it is on. Without a word: tells which it is.
    static bool HandleAlone(ChatHandler* handler, Optional<bool> on)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        if (on)
        {
            player->UpdatePlayerSetting(SETTING_SOURCE, SETTING_ALONE, *on ? 1 : 0);
            if (*on)
                SendBotsAway(player);
        }

        handler->SendSysMessage(WantsToBeAlone(player) ?
            "Alone: on. Free bots keep away from where you are. Your party stays. \".bots alone off\" lets them back." :
            "Alone: off. Bots come and go as they please. \".bots alone on\" keeps them away, \".bots away\" sends them off once.");
        return true;
    }

    // .bots companion add <name>: a guild mate becomes a companion
    static bool HandleCompanionAdd(ChatHandler* handler, std::string name)
    {
        Player* player = handler->GetPlayer();
        if (!player || !normalizePlayerName(name))
            return false;

        Player* bot = ObjectAccessor::FindPlayerByName(name);
        if (!bot || !IsBot(bot) || !player->GetGuildId() || bot->GetGuildId() != player->GetGuildId())
        {
            handler->PSendSysMessage("{} must be a bot in your guild, and online. Invite them with /ginvite first.", name);
            return true;
        }

        std::vector<ObjectGuid::LowType> ids = CompanionIds(player);
        if (std::find(ids.begin(), ids.end(), bot->GetGUID().GetCounter()) != ids.end())
        {
            handler->PSendSysMessage("{} is your companion already.", name);
            return true;
        }

        if (ids.size() >= MAX_COMPANIONS)
        {
            handler->PSendSysMessage("You have {} companions already. Remove one first: .bots companion remove <name>", MAX_COMPANIONS);
            return true;
        }

        ids.push_back(bot->GetGUID().GetCounter());
        StoreCompanionIds(player, ids);
        handler->PSendSysMessage("{} is now your companion. \".bots call\" brings your companions to you, \".bots dismiss\" lets them go.", name);
        return true;
    }

    static bool HandleCompanionRemove(ChatHandler* handler, std::string name)
    {
        Player* player = handler->GetPlayer();
        if (!player || !normalizePlayerName(name))
            return false;

        std::vector<ObjectGuid::LowType> ids = CompanionIds(player);
        ObjectGuid const guid = sCharacterCache->GetCharacterGuidByName(name);
        auto found = std::find(ids.begin(), ids.end(), guid.GetCounter());
        if (!guid || found == ids.end())
        {
            handler->PSendSysMessage("{} is not one of your companions.", name);
            return true;
        }

        ids.erase(found);
        StoreCompanionIds(player, ids);
        handler->PSendSysMessage("{} is no longer your companion. They stay in the guild.", name);
        return true;
    }

    static bool HandleCompanionList(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        std::vector<ObjectGuid::LowType> const ids = CompanionIds(player);
        if (ids.empty())
        {
            handler->SendSysMessage("You have no companions. Choose a guild mate: .bots companion add <name>");
            return true;
        }

        for (ObjectGuid::LowType id : ids)
        {
            std::string name;
            sCharacterCache->GetCharacterNameByGuid(ObjectGuid::Create<HighGuid::Player>(id), name);
            Player* bot = ObjectAccessor::FindPlayerByLowGUID(id);
            handler->PSendSysMessage("Companion: {} ({})", name, bot ? "online, level " + std::to_string(bot->GetLevel()) : "not online");
        }
        return true;
    }

    // .bots call: the companions join the party and come to the player
    static bool HandleCall(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        std::vector<Player*> const companions = Companions(player);
        if (companions.empty())
        {
            handler->SendSysMessage("None of your companions is online. Choose one with: .bots companion add <name>");
            return true;
        }

        for (Player* bot : companions)
        {
            if (player->GetGroup() && player->GetGroup() == bot->GetGroup())
            {
                // Already travelling together: just come here
                if (!bot->IsInCombat() && !bot->IsWithinDist(player, COMPANION_RANGE))
                    bot->TeleportTo(player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
                continue;
            }

            // The ordinary invitation; the bot accepts and the bot module brings it over
            WorldPacket invite(CMSG_GROUP_INVITE, bot->GetName().size() + 5);
            invite << bot->GetName();
            invite << uint32(0);
            player->GetSession()->HandleGroupInviteOpcode(invite);
        }

        handler->SendSysMessage("Your companions are on their way.");
        return true;
    }

    // .bots dismiss: the companions leave the party and go about their own business
    static bool HandleDismiss(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        uint32 gone = 0;
        if (Group* group = player->GetGroup())
            for (Player* bot : Companions(player))
                if (bot->GetGroup() == group)
                {
                    Player::RemoveFromGroup(group, bot->GetGUID());
                    ++gone;
                }

        handler->SendSysMessage(gone ? "Your companions have gone their own way." : "No companion is in your party.");
        return true;
    }
};

// A real player invites someone to their guild. If that someone is a bot in a guild of bots, it leaves
// that guild first; the game would otherwise refuse with "already in a guild". The invitation itself
// then goes its normal way, and the bot accepts.
class WowPlusBotsServer : public ServerScript
{
public:
    WowPlusBotsServer() : ServerScript("WowPlusBotsServer", {SERVERHOOK_CAN_PACKET_RECEIVE}) { }

    // Runs on the world thread, just before the game handles the packet
    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        if (packet.GetOpcode() != CMSG_GUILD_INVITE || !guildRecruits)
            return true;

        Player* inviter = session->GetPlayer();
        if (!inviter || IsBot(inviter) || !inviter->GetGuildId())
            return true;

        WorldPacket copy(packet);
        std::string name;
        copy >> name;
        if (!normalizePlayerName(name))
            return true;

        Player* bot = ObjectAccessor::FindPlayerByName(name);
        if (!bot || !IsBot(bot) || !bot->GetGuildId() || bot->GetGuildId() == inviter->GetGuildId() || bot->GetTeamId() != inviter->GetTeamId())
            return true;

        Guild* old = sGuildMgr->GetGuildById(bot->GetGuildId());
        if (!old)
            return true;

        // Only out of a guild of bots: one led by a real player keeps its members
        uint32 const leaderAccount = sCharacterCache->GetCharacterAccountIdByGuid(old->GetLeaderGUID());
        if (!sPlayerbotAIConfig.IsInRandomAccountList(leaderAccount))
            return true;

        old->DeleteMember(bot->GetGUID(), false, false, false);
        return true;
    }
};

void AddSC_WowPlusBots()
{
    new WowPlusBotsServer();
    new WowPlusBotsWorld();
    new WowPlusBotsPlayer();
    new WowPlusBotsCommands();
}
