/*
 * WowPlus experience: levelling alone with bots should not be slow, and going somewhere new
 * should pay better than grinding.
 *
 * The plain rates (kills, quests, exploring, rested) are settings of the server itself, in
 * Settings/worldserver.conf. This file covers what a rate cannot do:
 *
 * - A party of bots does not eat the experience. The game divides a kill between the party
 *   members, so with four bots the player would get about a quarter. Here the player's share
 *   in a party of only bots is set by WowPlus.XP.BotPartyShare: less than alone, because the
 *   party also kills much faster, but far more than the game's own split.
 * - A bonus the first time a character kills each rare, and each dungeon or raid boss.
 *   Given as a share of the level the character is working on, so it is worth about as much
 *   at level 15 as at level 75. Remembered per character (table wowplus_first_kills).
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DataMap.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "QueryResult.h"
#include "ScriptMgr.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "WorldSession.h"

#include <unordered_set>
#include <vector>

namespace
{
    std::string const DATA_KEY = "wowplus-first-kills";

    std::vector<float> botPartyShare = {1.0f, 0.75f, 0.6f, 0.5f, 0.45f};   // by party size 1..5
    float firstRarePercent = 3.0f;
    float firstBossPercent = 4.0f;

    struct FirstKills : public DataMap::Base
    {
        std::unordered_set<uint32> entries;
    };

    bool IsBot(Player const* player)
    {
        return player->GetSession()->IsHeadless();
    }

    // The size of the party if the player is its only real member, otherwise 0
    uint32 BotPartySize(Player* player)
    {
        Group* group = player->GetGroup();
        if (!group || group->isRaidGroup())
            return 0;

        uint32 size = 0;
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            if (Player* member = itr->GetSource())
            {
                if (member != player && !IsBot(member))
                    return 0;
                ++size;
            }

        return size;
    }
}

class WowPlusXPWorld : public WorldScript
{
public:
    WowPlusXPWorld() : WorldScript("WowPlusXPWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        std::vector<float> shares;
        std::string const setting = sConfigMgr->GetOption<std::string>("WowPlus.XP.BotPartyShare", "1 0.75 0.6 0.5 0.45");
        for (std::string_view token : Acore::Tokenize(setting, ' ', false))
            if (Optional<float> share = Acore::StringTo<float>(token))
                shares.push_back(*share);

        if (shares.size() == 5)
            botPartyShare = shares;
        else
            LOG_ERROR("module", "WowPlus.XP.BotPartyShare needs five numbers (party of 1, 2, 3, 4 and 5). Using the defaults.");

        firstRarePercent = sConfigMgr->GetOption<float>("WowPlus.XP.FirstKill.RarePercent", 3.0f);
        firstBossPercent = sConfigMgr->GetOption<float>("WowPlus.XP.FirstKill.BossPercent", 4.0f);
    }
};

class WowPlusXPPlayer : public PlayerScript
{
public:
    WowPlusXPPlayer() : PlayerScript("WowPlusXPPlayer") { }

    void OnPlayerLogin(Player* player) override
    {
        if (IsBot(player))
            return;

        FirstKills* kills = player->CustomData.GetDefault<FirstKills>(DATA_KEY);
        if (QueryResult result = CharacterDatabase.Query("SELECT entry FROM wowplus_first_kills WHERE guid = {}", player->GetGUID().GetCounter()))
            do
                kills->entries.insert((*result)[0].Get<uint32>());
            while (result->NextRow());
    }

    // The share of a kill's experience this player gets. The game has just worked out its own split.
    void OnPlayerRewardKillRewarder(Player* player, KillRewarder* /*rewarder*/, bool /*isDungeon*/, float& rate) override
    {
        if (IsBot(player))
            return;

        uint32 const size = BotPartySize(player);
        if (size >= 2 && size <= botPartyShare.size())
            rate = botPartyShare[size - 1];
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        // Only kills that are worth experience: a grey rare gives no bonus either
        if (xpSource != XPSOURCE_KILL || !amount || !victim || IsBot(player))
            return;

        Creature* creature = victim->ToCreature();
        if (!creature || creature->IsPet() || creature->IsSummon())
            return;

        uint32 const rank = creature->GetCreatureTemplate()->rank;
        bool const rare = rank == CREATURE_ELITE_RARE || rank == CREATURE_ELITE_RAREELITE;
        bool const boss = creature->IsDungeonBoss() || creature->isWorldBoss();
        float const percent = boss ? firstBossPercent : rare ? firstRarePercent : 0.0f;
        if (percent <= 0.0f)
            return;

        FirstKills* kills = player->CustomData.GetDefault<FirstKills>(DATA_KEY);
        if (!kills->entries.insert(creature->GetEntry()).second)
            return;

        CharacterDatabase.Execute("INSERT IGNORE INTO wowplus_first_kills (guid, entry) VALUES ({}, {})",
            player->GetGUID().GetCounter(), creature->GetEntry());

        uint32 const bonus = uint32(float(player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP)) * percent / 100.0f);
        if (!bonus)
            return;

        ChatHandler(player->GetSession()).PSendSysMessage("First kill: {}. Bonus experience: {}.", creature->GetName(), bonus);
        player->GiveXP(bonus, nullptr);
    }
};

void AddSC_WowPlusXP()
{
    new WowPlusXPWorld();
    new WowPlusXPPlayer();
}
