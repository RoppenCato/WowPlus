/*
 * WowPlus hub: a small place on GM Island with what a solo player cannot reasonably get alone.
 *
 * The NPCs, their vendor lists and prices are data: Settings/SQL/world/..._hub.sql.
 * This file adds the two things data cannot do:
 *
 * - One vendor with several lists. A hub vendor opens a menu (table wowplus_hub_menu) and each
 *   line opens a different vendor list, so the hub needs six NPCs instead of forty.
 * - Honor and arena points from ordinary kills. The hub's PvP lists are the game's own, priced
 *   in honor and arena points, which a solo player barely earns, so enemies at the player's
 *   level give a little of both. No arena rating is asked for either.
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Formulas.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

#include <array>
#include <unordered_map>
#include <vector>

namespace
{
    struct MenuLine
    {
        std::string label;
        uint32 vendorEntry;
        char team;                                          // 'A', 'H' or 0 for everyone
        uint32 classMask;                                   // 0 for everyone
    };

    std::unordered_map<uint32, std::vector<MenuLine>> menus;    // by NPC entry; built at startup, only read afterwards

    bool honorEnabled = true;
    std::array<uint32, 4> honorByRank = {10, 30, 60, 300};       // normal, elite, rare, boss
    std::array<float, 4> arenaPointsByRank = {0.2f, 1.0f, 3.0f, 15.0f};
    bool noRatingRequired = true;

    bool Shown(MenuLine const& line, Player const* player)
    {
        if (line.team == 'A' && player->GetTeamId() != TEAM_ALLIANCE)
            return false;

        if (line.team == 'H' && player->GetTeamId() != TEAM_HORDE)
            return false;

        return !line.classMask || (line.classMask & player->getClassMask());
    }

    void LoadMenus()
    {
        menus.clear();

        // The table belongs to the hub SQL file; without it there is simply no hub.
        if (!WorldDatabase.Query("SHOW TABLES LIKE 'wowplus_hub_menu'"))
        {
            LOG_INFO("server.loading", ">> WowPlus hub: table wowplus_hub_menu not found, the hub vendors have no lists");
            return;
        }

        uint32 count = 0;
        if (QueryResult result = WorldDatabase.Query("SELECT npc_entry, label, vendor_entry, team, class_mask FROM wowplus_hub_menu ORDER BY npc_entry, sort"))
        {
            do
            {
                Field* fields = result->Fetch();
                std::string const team = fields[3].Get<std::string>();
                menus[fields[0].Get<uint32>()].push_back({fields[1].Get<std::string>(), fields[2].Get<uint32>(), team.empty() ? char(0) : team[0],
                    fields[4].Get<uint32>()});
                ++count;
            } while (result->NextRow());
        }

        LOG_INFO("server.loading", ">> WowPlus hub: {} vendor lists on {} vendors", count, menus.size());
    }

    Player* HumanFor(Player* killer, Creature const* killed)
    {
        if (!killer->GetSession()->IsHeadless())
            return killer;

        if (Group* group = killer->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                if (Player* member = itr->GetSource())
                    if (!member->GetSession()->IsHeadless() && member->IsAtLootRewardDistance(killed))
                        return member;

        return nullptr;
    }

    void RewardKillHonor(Player* killer, Creature* killed)
    {
        if (!honorEnabled || !killer || !killed || killed->IsPet() || killed->IsCritter())
            return;

        Player* player = HumanFor(killer, killed);
        if (!player || killed->GetLevel() <= Acore::XP::GetGrayLevel(player->GetLevel()))
            return;

        uint8 rank = 0;
        if (killed->IsDungeonBoss() || killed->isWorldBoss())
            rank = 3;
        else if (killed->GetCreatureTemplate()->rank == CREATURE_ELITE_RARE || killed->GetCreatureTemplate()->rank == CREATURE_ELITE_RAREELITE)
            rank = 2;
        else if (killed->isElite())
            rank = 1;

        if (honorByRank[rank])
            player->RewardHonor(nullptr, 1, int32(honorByRank[rank]), false);

        // Arena points come in whole numbers: 0.2 means one point on every fifth kill or so
        uint32 arenaPoints = uint32(arenaPointsByRank[rank]);
        if (roll_chance_f((arenaPointsByRank[rank] - float(arenaPoints)) * 100.0f))
            ++arenaPoints;

        if (arenaPoints)
        {
            player->ModifyArenaPoints(int32(arenaPoints));
            if (arenaPoints >= 5)
                ChatHandler(player->GetSession()).PSendSysMessage("You receive {} arena points.", arenaPoints);
        }
    }
}

class WowPlusHubWorld : public WorldScript
{
public:
    WowPlusHubWorld() : WorldScript("WowPlusHubWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        honorEnabled = sConfigMgr->GetOption<bool>("WowPlus.Honor.FromKills", true);
        honorByRank[0] = sConfigMgr->GetOption<uint32>("WowPlus.Honor.Normal", 10);
        honorByRank[1] = sConfigMgr->GetOption<uint32>("WowPlus.Honor.Elite", 30);
        honorByRank[2] = sConfigMgr->GetOption<uint32>("WowPlus.Honor.Rare", 60);
        honorByRank[3] = sConfigMgr->GetOption<uint32>("WowPlus.Honor.Boss", 300);
        arenaPointsByRank[0] = sConfigMgr->GetOption<float>("WowPlus.ArenaPoints.Normal", 0.2f);
        arenaPointsByRank[1] = sConfigMgr->GetOption<float>("WowPlus.ArenaPoints.Elite", 1.0f);
        arenaPointsByRank[2] = sConfigMgr->GetOption<float>("WowPlus.ArenaPoints.Rare", 3.0f);
        arenaPointsByRank[3] = sConfigMgr->GetOption<float>("WowPlus.ArenaPoints.Boss", 15.0f);
        noRatingRequired = sConfigMgr->GetOption<bool>("WowPlus.Hub.NoRatingRequired", true);
    }

    void OnStartup() override
    {
        LoadMenus();
    }
};

class WowPlusHubVendor : public CreatureScript
{
public:
    WowPlusHubVendor() : CreatureScript("npc_wowplus_hub_vendor") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        auto const itr = menus.find(creature->GetEntry());
        if (itr == menus.end() || itr->second.empty())
            return false;

        // Only the lines meant for this character's faction and class
        std::vector<uint32> shown;
        for (uint32 i = 0; i < itr->second.size(); ++i)
            if (Shown(itr->second[i], player))
                shown.push_back(i);

        if (shown.empty())
            return false;

        // A single list needs no menu
        if (shown.size() == 1)
        {
            player->GetSession()->SendListInventory(creature->GetGUID(), itr->second[shown.front()].vendorEntry);
            return true;
        }

        ClearGossipMenuFor(player);
        for (uint32 i = 0; i < shown.size() && i < GOSSIP_MAX_MENU_ITEMS; ++i)
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, itr->second[shown[i]].label, GOSSIP_SENDER_MAIN, shown[i]);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        CloseGossipMenuFor(player);

        auto const itr = menus.find(creature->GetEntry());
        if (itr != menus.end() && action < itr->second.size() && Shown(itr->second[action], player))
            player->GetSession()->SendListInventory(creature->GetGUID(), itr->second[action].vendorEntry);

        return true;
    }
};

class WowPlusHubPlayer : public PlayerScript
{
public:
    WowPlusHubPlayer() : PlayerScript("WowPlusHubPlayer") { }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        RewardKillHonor(killer, killed);
    }

    void OnPlayerCreatureKilledByPet(Player* petOwner, Creature* killed) override
    {
        RewardKillHonor(petOwner, killed);
    }

    // Arena gear that asks for a personal rating: with points earned from kills there is no rating to have
    void OnPlayerGetMaxPersonalArenaRatingRequirement(Player const* /*player*/, uint32 /*minSlot*/, uint32& maxArenaRating) const override
    {
        if (noRatingRequired)
            maxArenaRating = 5000;
    }
};

void AddSC_WowPlusHub()
{
    new WowPlusHubWorld();
    new WowPlusHubVendor();
    new WowPlusHubPlayer();
}
