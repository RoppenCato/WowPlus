/*
 * WowPlus travel: getting around faster without shrinking the world.
 *
 * - The hearthstone has no cooldown. (The approach, clearing the cooldown at the moment the
 *   hearthstone teleports, follows mod-no-hearthstone-cooldown by BytesGalore.)
 * - Using the hearthstone opens a menu: home as always, plus the cities, hub towns, dungeons
 *   and raids this character has been to. A place unlocks the first time the character
 *   gets there. Everything is free.
 *
 * What a character has unlocked is kept in the core's per-character settings
 * (table character_settings), which needs EnablePlayerSettings = 1 in worldserver.conf.
 */

#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

#include <array>
#include <map>
#include <vector>

namespace
{
    constexpr uint32 SPELL_HEARTHSTONE = 8690;
    constexpr uint32 SPELL_NO_PLACE_LIKE_HOME = 39937;      // Ruby Slippers, shares the hearthstone cooldown

    std::string const SETTING_SOURCE = "mod-wowplus-travel";
    constexpr uint32 SETTING_PLACES = 0;                    // one bit per entry of PLACES
    constexpr uint32 SETTING_INSTANCES = 1;                 // from here: one bit per map id, 32 per setting

    bool hearthstoneNoCooldown = true;
    bool menuEnabled = true;

    struct Place
    {
        char const* name;
        bool city;                                          // listed under Cities, otherwise under Towns
        uint32 zone;                                        // unlocks on entering this zone...
        uint32 area;                                        // ...or this area, when it is only part of a zone
        uint32 map;
        float x, y, z, o;
    };

    // The order is what is stored per character, so only ever add at the end (at most 32).
    std::array<Place, 16> const PLACES =
    {{
        {"Stormwind",       true,  1519, 0,    0,   -8833.4f,   628.6f,   94.0f, 1.07f},
        {"Ironforge",       true,  1537, 0,    0,   -4918.9f,  -940.4f,  501.6f, 5.42f},
        {"Darnassus",       true,  1657, 0,    1,    9949.6f,  2284.2f, 1341.4f, 1.60f},
        {"The Exodar",      true,  3557, 0,    530, -3965.7f, -11653.6f, -138.8f, 0.85f},
        {"Orgrimmar",       true,  1637, 0,    1,    1629.8f, -4373.6f,   31.6f, 3.70f},
        {"Thunder Bluff",   true,  1638, 0,    1,   -1277.4f,   124.8f,  131.3f, 5.22f},
        {"Undercity",       true,  1497, 0,    0,    1584.1f,   240.3f,  -52.2f, 0.04f},
        {"Silvermoon City", true,  3487, 0,    530,  9487.7f, -7279.2f,   14.3f, 6.16f},
        {"Shattrath",       true,  3703, 0,    530, -1838.2f,  5301.8f,  -12.4f, 5.95f},
        {"Dalaran",         true,  4395, 0,    571,  5808.0f,   588.5f,  660.9f, 1.67f},
        {"Booty Bay",       false, 0,    35,   0,  -14297.2f,   531.0f,    8.8f, 3.99f},
        {"Gadgetzan",       false, 0,    976,  1,   -7177.1f, -3785.3f,    8.4f, 6.10f},
        {"Everlook",        false, 0,    2255, 1,    6725.7f, -4619.4f,  720.9f, 4.67f},
        {"Ratchet",         false, 0,    392,  1,    -956.7f, -3754.7f,    5.3f, 1.00f},
        {"Area 52",         false, 0,    3712, 530,  3043.3f,  3681.3f,  143.1f, 5.07f},
        {"The Dark Portal", false, 0,    72,   0,  -11840.1f, -3196.6f,  -29.6f, 3.34f}
    }};

    struct Instance
    {
        uint32 map;
        std::string name;
        bool raid;
        uint32 expansion;                                   // 0 classic, 1 The Burning Crusade, 2 Wrath of the Lich King
        float x, y, z, o;                                   // where the entrance portal puts a character
    };

    std::vector<Instance> instances;                        // built at startup, only read afterwards

    // Menu pages. The gossip "sender" carries the page, the "action" the entry on it.
    enum Page : uint32
    {
        PAGE_MAIN = 1,
        PAGE_CITIES,
        PAGE_TOWNS,
        PAGE_DUNGEONS,                                      // + expansion
        PAGE_RAIDS = PAGE_DUNGEONS + 3,                     // + expansion
        PAGE_GO_PLACE = 20,
        PAGE_GO_INSTANCE
    };

    constexpr uint32 ACTION_HOME = 1000;
    constexpr uint32 ACTION_BACK = 1001;

    char const* const EXPANSION_NAMES[3] = {"Classic", "The Burning Crusade", "Wrath of the Lich King"};

    void LoadInstances()
    {
        instances.clear();

        // The entrance of every dungeon and raid: where its portal puts you
        QueryResult result = WorldDatabase.Query("SELECT target_map, target_position_x, target_position_y, target_position_z, "
            "target_orientation FROM areatrigger_teleport ORDER BY ID");
        if (!result)
            return;

        std::map<uint32, Instance> byMap;
        do
        {
            Field* fields = result->Fetch();
            uint32 const mapId = fields[0].Get<uint32>();
            MapEntry const* entry = sMapStore.LookupEntry(mapId);
            if (!entry || !entry->IsDungeon() || byMap.count(mapId))
                continue;

            Instance instance;
            instance.map = mapId;
            instance.name = entry->name[0] ? entry->name[0] : "Unknown";
            instance.raid = entry->IsRaid();
            instance.expansion = std::min<uint32>(entry->Expansion(), 2);
            instance.x = fields[1].Get<float>();
            instance.y = fields[2].Get<float>();
            instance.z = fields[3].Get<float>();
            instance.o = fields[4].Get<float>();
            byMap[mapId] = instance;
        } while (result->NextRow());

        for (auto const& [mapId, instance] : byMap)
            instances.push_back(instance);

        std::sort(instances.begin(), instances.end(), [](Instance const& a, Instance const& b) { return a.name < b.name; });

        LOG_INFO("server.loading", ">> WowPlus travel: {} cities and towns, {} dungeons and raids for the hearthstone menu",
            PLACES.size(), instances.size());
    }

    bool HasPlace(Player* player, uint32 index)
    {
        return player->GetPlayerSetting(SETTING_SOURCE, SETTING_PLACES).value & (1u << index);
    }

    bool HasInstance(Player* player, uint32 mapId)
    {
        return player->GetPlayerSetting(SETTING_SOURCE, SETTING_INSTANCES + mapId / 32).value & (1u << (mapId % 32));
    }

    void Announce(Player* player, std::string const& name)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("|cff00ccff{}|r can now be reached with your hearthstone.", name);
    }

    void UnlockPlaces(Player* player, uint32 zone, uint32 area)
    {
        if (!menuEnabled)
            return;

        for (uint32 i = 0; i < PLACES.size(); ++i)
        {
            Place const& place = PLACES[i];
            if (!(place.zone && place.zone == zone) && !(place.area && place.area == area))
                continue;

            uint32 const known = player->GetPlayerSetting(SETTING_SOURCE, SETTING_PLACES).value;
            if (known & (1u << i))
                continue;

            player->UpdatePlayerSetting(SETTING_SOURCE, SETTING_PLACES, known | (1u << i));
            Announce(player, place.name);
        }
    }

    void UnlockInstance(Player* player)
    {
        if (!menuEnabled || !player->GetMap()->IsDungeon())
            return;

        uint32 const mapId = player->GetMapId();
        for (Instance const& instance : instances)
        {
            if (instance.map != mapId)
                continue;

            if (!HasInstance(player, mapId))
            {
                uint32 const index = SETTING_INSTANCES + mapId / 32;
                uint32 const known = player->GetPlayerSetting(SETTING_SOURCE, index).value;
                player->UpdatePlayerSetting(SETTING_SOURCE, index, known | (1u << (mapId % 32)));
                Announce(player, instance.name);
            }
            break;
        }
    }

    bool HasAnything(Player* player)
    {
        if (player->GetPlayerSetting(SETTING_SOURCE, SETTING_PLACES).value)
            return true;

        for (Instance const& instance : instances)
            if (HasInstance(player, instance.map))
                return true;

        return false;
    }

    uint32 CountInstances(Player* player, bool raid, uint32 expansion)
    {
        uint32 count = 0;
        for (Instance const& instance : instances)
            if (instance.raid == raid && instance.expansion == expansion && HasInstance(player, instance.map))
                ++count;

        return count;
    }

    void ShowPage(Player* player, Item* item, uint32 page)
    {
        ClearGossipMenuFor(player);

        if (page == PAGE_MAIN)
        {
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Home", PAGE_MAIN, ACTION_HOME);

            bool cities = false;
            bool towns = false;
            for (uint32 i = 0; i < PLACES.size(); ++i)
                if (HasPlace(player, i))
                    (PLACES[i].city ? cities : towns) = true;

            if (cities)
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Cities", PAGE_CITIES, ACTION_BACK);
            if (towns)
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Towns and landmarks", PAGE_TOWNS, ACTION_BACK);

            for (uint32 raid = 0; raid < 2; ++raid)
                for (uint32 expansion = 0; expansion < 3; ++expansion)
                    if (uint32 count = CountInstances(player, raid != 0, expansion))
                        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, Acore::StringFormat("{}: {} ({})", raid ? "Raids" : "Dungeons",
                            EXPANSION_NAMES[expansion], count), (raid ? PAGE_RAIDS : PAGE_DUNGEONS) + expansion, ACTION_BACK);
        }
        else if (page == PAGE_CITIES || page == PAGE_TOWNS)
        {
            for (uint32 i = 0; i < PLACES.size(); ++i)
                if (PLACES[i].city == (page == PAGE_CITIES) && HasPlace(player, i))
                    AddGossipItemFor(player, GOSSIP_ICON_TAXI, PLACES[i].name, PAGE_GO_PLACE, i);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", PAGE_MAIN, ACTION_BACK);
        }
        else if (page >= PAGE_DUNGEONS && page < PAGE_RAIDS + 3)
        {
            bool const raid = page >= PAGE_RAIDS;
            uint32 const expansion = page - (raid ? PAGE_RAIDS : PAGE_DUNGEONS);
            uint32 shown = 0;

            for (uint32 i = 0; i < instances.size() && shown < GOSSIP_MAX_MENU_ITEMS - 1; ++i)
                if (instances[i].raid == raid && instances[i].expansion == expansion && HasInstance(player, instances[i].map))
                {
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, instances[i].name, PAGE_GO_INSTANCE, i);
                    ++shown;
                }

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", PAGE_MAIN, ACTION_BACK);
        }

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
    }

    bool CanTravel(Player* player)
    {
        if (player->IsInCombat())
        {
            ChatHandler(player->GetSession()).SendSysMessage("You cannot do that while in combat.");
            return false;
        }

        return player->IsAlive() && !player->InBattleground() && !player->IsInFlight();
    }

    void ClearHearthstoneCooldown(Player* player)
    {
        for (uint32 spellId : {SPELL_HEARTHSTONE, SPELL_NO_PLACE_LIKE_HOME})
        {
            player->RemoveSpellCooldown(spellId, true);
            player->SendClearCooldown(spellId, player);
        }
    }
}

class WowPlusTravelWorld : public WorldScript
{
public:
    WowPlusTravelWorld() : WorldScript("WowPlusTravelWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        hearthstoneNoCooldown = sConfigMgr->GetOption<bool>("WowPlus.Travel.Hearthstone.NoCooldown", true);
        menuEnabled = sConfigMgr->GetOption<bool>("WowPlus.Travel.Hearthstone.Menu", true);
    }

    void OnStartup() override
    {
        LoadInstances();
    }
};

class WowPlusTravelPlayer : public PlayerScript
{
public:
    WowPlusTravelPlayer() : PlayerScript("WowPlusTravelPlayer") { }

    // The hearthstone is still being cast at the moment it teleports
    bool OnPlayerBeforeTeleport(Player* player, uint32 /*mapid*/, float /*x*/, float /*y*/, float /*z*/, float /*orientation*/,
        uint32 /*options*/, Unit* /*target*/) override
    {
        if (hearthstoneNoCooldown && player->FindCurrentSpellBySpellId(SPELL_HEARTHSTONE))
            ClearHearthstoneCooldown(player);

        return true;
    }

    void OnPlayerLogin(Player* player) override
    {
        // A cooldown left over from before the setting was switched on
        if (hearthstoneNoCooldown)
            ClearHearthstoneCooldown(player);

        // Wherever the character already stands counts as visited
        UnlockPlaces(player, player->GetZoneId(), player->GetAreaId());
        UnlockInstance(player);
    }

    void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea) override
    {
        UnlockPlaces(player, newZone, newArea);
    }

    void OnPlayerUpdateArea(Player* player, uint32 /*oldArea*/, uint32 newArea) override
    {
        UnlockPlaces(player, 0, newArea);
    }

    void OnPlayerMapChanged(Player* player) override
    {
        UnlockInstance(player);
    }
};

// Bound to the hearthstone item (6948) by Settings/SQL/world/..._hearthstone-menu.sql
class WowPlusHearthstone : public ItemScript
{
public:
    WowPlusHearthstone() : ItemScript("item_wowplus_hearthstone") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        // Nothing unlocked yet, or switched off: the hearthstone works as it always did
        if (!menuEnabled || !HasAnything(player))
            return false;

        ShowPage(player, item, PAGE_MAIN);
        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        if (action == ACTION_BACK)
        {
            ShowPage(player, item, sender);
            return;
        }

        CloseGossipMenuFor(player);

        if (action == ACTION_HOME)
        {
            player->CastSpell(player, SPELL_HEARTHSTONE, false);
            return;
        }

        if (!CanTravel(player))
            return;

        if (sender == PAGE_GO_PLACE && action < PLACES.size() && HasPlace(player, action))
        {
            Place const& place = PLACES[action];
            player->TeleportTo(place.map, place.x, place.y, place.z, place.o);
        }
        else if (sender == PAGE_GO_INSTANCE && action < instances.size() && HasInstance(player, instances[action].map))
        {
            Instance const& instance = instances[action];
            player->TeleportTo(instance.map, instance.x, instance.y, instance.z, instance.o);
        }
    }
};

void AddSC_WowPlusTravel()
{
    new WowPlusTravelWorld();
    new WowPlusTravelPlayer();
    new WowPlusHearthstone();
}
