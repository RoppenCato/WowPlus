/*
 * WowPlus hub: a small place on GM Island with what a solo player cannot reasonably get alone.
 *
 * The NPCs, their vendor lists and prices are data: Settings/SQL/world/..._hub.sql.
 * This file adds the two things data cannot do:
 *
 * - One vendor with several lists. A hub vendor opens a menu (table wowplus_hub_menu) and each
 *   line opens a different vendor list, so the hub needs six NPCs instead of forty.
 * - Honor from ordinary kills. The hub's PvP vendors charge honor, which a solo player barely
 *   earns, so enemies at the player's level give a little.
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Formulas.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
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
    };

    std::unordered_map<uint32, std::vector<MenuLine>> menus;    // by NPC entry; built at startup, only read afterwards

    bool honorEnabled = true;
    std::array<uint32, 4> honorByRank = {10, 30, 60, 300};       // normal, elite, rare, boss

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
        if (QueryResult result = WorldDatabase.Query("SELECT npc_entry, label, vendor_entry FROM wowplus_hub_menu ORDER BY npc_entry, sort"))
        {
            do
            {
                Field* fields = result->Fetch();
                menus[fields[0].Get<uint32>()].push_back({fields[1].Get<std::string>(), fields[2].Get<uint32>()});
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

        // A single list needs no menu
        if (itr->second.size() == 1)
        {
            player->GetSession()->SendListInventory(creature->GetGUID(), itr->second.front().vendorEntry);
            return true;
        }

        ClearGossipMenuFor(player);
        for (uint32 i = 0; i < itr->second.size() && i < GOSSIP_MAX_MENU_ITEMS; ++i)
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, itr->second[i].label, GOSSIP_SENDER_MAIN, i);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        CloseGossipMenuFor(player);

        auto const itr = menus.find(creature->GetEntry());
        if (itr != menus.end() && action < itr->second.size())
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
};

void AddSC_WowPlusHub()
{
    new WowPlusHubWorld();
    new WowPlusHubVendor();
    new WowPlusHubPlayer();
}
