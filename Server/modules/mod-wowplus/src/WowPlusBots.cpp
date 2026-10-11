/*
 * WowPlus bots: small things that make the random bots better company. See Settings/Docs/Bots.md.
 *
 * - Passing buffs: now and then a bot standing or walking near a real player casts its class
 *   buff on them, the way a friendly stranger would.
 * - Being alone: ".bots away" sends every free bot near the player somewhere else, and
 *   ".bots alone on" keeps doing that, for levelling in peace. Bots in the player's party
 *   stay; so do bots that belong to someone.
 *
 * Bot chat through Claude is in WowPlusBotChat.cpp. How well a bot is dressed is a small change
 * in the bot module itself (PlayerbotFactory.cpp, look for "WowPlus").
 */

#include "Chat.h"
#include "CommandScript.h"
#include "Config.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "WorldSession.h"

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

    constexpr uint32 CHECK_INTERVAL = 5000;                 // ms between looks at the real players

    std::string const SETTING_SOURCE = "mod-wowplus-bots";
    constexpr uint32 SETTING_ALONE = 0;

    bool buffsEnabled = true;
    uint32 buffMinSeconds = 90;
    uint32 buffMaxSeconds = 240;
    float buffRadius = 20.0f;
    float awayRadius = 200.0f;

    std::unordered_map<ObjectGuid, uint32> nextBuffAt;      // real player -> ms until the next passing buff
    uint32 sinceCheck = 0;

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
    }

    // The world thread, after the maps have been updated: safe to look at and move players
    void OnUpdate(uint32 diff) override
    {
        sinceCheck += diff;
        if (sinceCheck < CHECK_INTERVAL)
            return;

        uint32 const elapsed = sinceCheck;
        sinceCheck = 0;

        std::vector<Player*> realPlayers;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
                if (player->IsInWorld() && !IsBot(player) && !player->IsBeingTeleported())
                    realPlayers.push_back(player);
        }

        for (Player* player : realPlayers)
        {
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
    }
};

class WowPlusBotsCommands : public CommandScript
{
public:
    WowPlusBotsCommands() : CommandScript("WowPlusBotsCommands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable botsTable =
        {
            { "away",  HandleAway,  SEC_PLAYER, Console::No },
            { "alone", HandleAlone, SEC_PLAYER, Console::No },
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
};

void AddSC_WowPlusBots()
{
    new WowPlusBotsWorld();
    new WowPlusBotsPlayer();
    new WowPlusBotsCommands();
}
