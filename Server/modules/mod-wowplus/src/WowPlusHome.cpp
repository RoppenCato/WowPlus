/*
 * WowPlus home: a room of your own. See Settings/Docs/Housing.md.
 *
 * The data is in Settings/SQL/world/..._home.sql: a key for each city, sold by its innkeepers,
 * and pieces of furniture. This file is the rules.
 *
 * - The key. Used in its city for the first time, it makes the spot where you stand your room
 *   there. After that it opens a small menu: go to your room, move the room to where you stand,
 *   pick up the nearest piece of furniture, pick up everything.
 * - Furniture. A piece used inside your room is put down where you stand, facing the way you
 *   face, and stays there. It is in the world only while you are near your room: put there when
 *   you come within reach, taken away when you leave, so the room is furnished for you alone.
 * - Some pieces do something: the storage chest opens your bank, the bed leaves you rested.
 *   The mailbox, the guild vault, the fire, the anvil and the forge work by themselves, being
 *   the game's own objects.
 *
 * Kept per character in the characters database: wowplus_home (the spot in each city) and
 * wowplus_home_objects (what stands where).
 */

#include "WowPlusHome.h"

#include "Chat.h"
#include "Config.h"
#include "DataMap.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "GameObject.h"
#include "Item.h"
#include "Log.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QueryResult.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "WorldSession.h"

#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr uint32 CHECK_INTERVAL = 2000;                 // ms between looks at who is near their room
    constexpr float FURNISH_RANGE = 70.0f;                  // the furniture is put out when the owner comes this near
    constexpr float CLEAR_RANGE = 100.0f;                   // ...and taken away when they are this far
    constexpr float PICK_UP_RANGE = 6.0f;
    constexpr uint32 BED_REST_SECONDS = 30 * MINUTE;        // between two rests in the bed
    std::string const DATA_KEY = "wowplus-home";

    enum KeyAction : uint32
    {
        ACTION_CLAIM = 1,
        ACTION_GO,
        ACTION_MOVE,
        ACTION_PICK_UP_NEAREST,
        ACTION_PICK_UP_ALL,
        ACTION_CLOSE
    };

    struct City
    {
        uint32 zone;
        std::string name;
    };

    struct Room
    {
        uint32 map;
        float x, y, z, o;
    };

    struct Piece
    {
        uint64 id;                                          // wowplus_home_objects.id
        uint32 city;
        uint32 item;
        float x, y, z, o;
        ObjectGuid spawned;                                 // the object in the world, while the owner is near
    };

    struct Home : public DataMap::Base
    {
        std::unordered_map<uint32, Room> rooms;             // by city zone
        std::vector<Piece> pieces;
        uint32 furnishedCity = 0;                           // the city whose furniture is out right now
        uint32 furnishedMap = 0;                            // ...and the map it stands on
        time_t restedAt = 0;
    };

    constexpr uint32 ZONE_SHATTRATH = 3703;
    constexpr uint32 ZONE_DALARAN = 4395;

    bool enabled = true;
    uint32 firstKeyGold = 10;
    uint32 shattrathKeyGold = 1000;
    uint32 dalaranKeyGold = 5000;
    float roomRadius = 30.0f;                               // furniture can be put down this far from the room's spot
    uint32 maxPieces = 80;

    std::unordered_map<uint32, City> cityByKey;             // key item -> city
    std::unordered_map<uint32, uint32> objectByItem;        // furniture item -> game object
    uint32 sinceCheck = 0;
    uint64 nextPieceId = 1;

    void Tell(Player* player, std::string const& text)
    {
        ChatHandler(player->GetSession()).SendSysMessage(text);
    }

    Home* HomeOf(Player* player)
    {
        return player->CustomData.GetDefault<Home>(DATA_KEY);
    }

    void PutOut(Player* player, Piece& piece)
    {
        auto object = objectByItem.find(piece.item);
        if (object == objectByItem.end() || piece.spawned)
            return;

        // Put into the world the way the game's own objects are: belonging to nobody, turned to face the way
        // the owner faced. (Objects summoned as the player's own vanished again unless they were of a kind
        // that is used, such as the mailbox.)
        Map* map = player->GetMap();
        G3D::Quat const facing(0.0f, 0.0f, std::sin(piece.o / 2.0f), std::cos(piece.o / 2.0f));
        GameObject* go = new GameObject();
        if (!go->Create(map->GenerateLowGuid<HighGuid::GameObject>(), object->second, map, player->GetPhaseMask(), piece.x, piece.y, piece.z,
            piece.o, facing, 255, GO_STATE_READY))
        {
            delete go;
            return;
        }

        go->SetRespawnTime(0);
        if (!map->AddToMap(go))
        {
            delete go;
            return;
        }

        piece.spawned = go->GetGUID();
    }

    // mapId: the map the piece was put out on, which is not always where the player is by now
    void TakeAway(uint32 mapId, Piece& piece)
    {
        if (!piece.spawned)
            return;

        Map* map = sMapMgr->FindMap(mapId, 0);
        if (GameObject* go = map ? map->GetGameObject(piece.spawned) : nullptr)
        {
            go->SetRespawnTime(0);
            go->Delete();
        }
        piece.spawned.Clear();
    }

    void ClearRoom(Home* home)
    {
        for (Piece& piece : home->pieces)
            TakeAway(home->furnishedMap, piece);
        home->furnishedCity = 0;
    }

    // The room the player stands in or near, if any: its city zone
    uint32 RoomNear(Player* player, Home* home, float range)
    {
        for (auto const& [city, room] : home->rooms)
            if (room.map == player->GetMapId() && player->GetExactDist(room.x, room.y, room.z) <= range)
                return city;

        return 0;
    }

    void UpdateFurnishing(Player* player)
    {
        Home* home = player->CustomData.Get<Home>(DATA_KEY);
        if (!home || home->rooms.empty())
            return;

        if (home->furnishedCity)
        {
            auto room = home->rooms.find(home->furnishedCity);
            if (room == home->rooms.end() || room->second.map != player->GetMapId() ||
                player->GetExactDist(room->second.x, room->second.y, room->second.z) > CLEAR_RANGE)
                ClearRoom(home);
            return;
        }

        if (uint32 const city = RoomNear(player, home, FURNISH_RANGE))
        {
            home->furnishedCity = city;
            home->furnishedMap = player->GetMapId();
            for (Piece& piece : home->pieces)
                if (piece.city == city)
                    PutOut(player, piece);
        }
    }

    // Hands a piece back as the item it was placed from. False if the bags are full.
    bool PickUp(Player* player, Home* home, std::size_t index)
    {
        Piece& piece = home->pieces[index];
        if (!player->AddItem(piece.item, 1))
        {
            Tell(player, "Your bags are full.");
            return false;
        }

        TakeAway(home->furnishedMap, piece);
        CharacterDatabase.Execute("DELETE FROM wowplus_home_objects WHERE id = {}", piece.id);
        home->pieces.erase(home->pieces.begin() + index);
        return true;
    }

    uint32 PickUpAll(Player* player, Home* home, uint32 city)
    {
        uint32 taken = 0;
        for (std::size_t index = home->pieces.size(); index-- > 0;)
            if (home->pieces[index].city == city)
            {
                if (!PickUp(player, home, index))
                    break;
                ++taken;
            }

        return taken;
    }

    void LoadData()
    {
        if (WorldDatabase.Query("SHOW TABLES LIKE 'wowplus_home_cities'"))
            if (QueryResult result = WorldDatabase.Query("SELECT item, zone, name FROM wowplus_home_cities"))
                do
                {
                    Field* fields = result->Fetch();
                    cityByKey[fields[0].Get<uint32>()] = {fields[1].Get<uint32>(), fields[2].Get<std::string>()};
                } while (result->NextRow());

        if (WorldDatabase.Query("SHOW TABLES LIKE 'wowplus_furniture'"))
            if (QueryResult result = WorldDatabase.Query("SELECT item, object FROM wowplus_furniture"))
                do
                {
                    Field* fields = result->Fetch();
                    if (sObjectMgr->GetGameObjectTemplate(fields[1].Get<uint32>()))
                        objectByItem[fields[0].Get<uint32>()] = fields[1].Get<uint32>();
                } while (result->NextRow());

        if (QueryResult result = CharacterDatabase.Query("SELECT IFNULL(MAX(id), 0) FROM wowplus_home_objects"))
            nextPieceId = (*result)[0].Get<uint64>() + 1;

        LOG_INFO("server.loading", ">> WowPlus home: rooms in {} cities, {} kinds of furniture", cityByKey.size(), objectByItem.size());
    }
}

// What a key costs this player. Shattrath and Dalaran have a price of their own; among the other cities
// the first key costs FirstKeyGold, the second twice that, the third three times, and so on.
bool WowPlus::RoomOffer(Player* player, uint32 zone, uint32& keyItem, std::string& cityName, uint32& price)
{
    if (!enabled)
        return false;

    Home* home = HomeOf(player);
    uint32 owned = 0;
    keyItem = 0;
    for (auto const& [key, city] : cityByKey)
    {
        bool const has = player->HasItemCount(key, 1, true) || home->rooms.count(city.zone);
        if (city.zone == zone)
        {
            if (has)
                return false;
            keyItem = key;
            cityName = city.name;
        }
        else if (has && city.zone != ZONE_SHATTRATH && city.zone != ZONE_DALARAN)
            ++owned;
    }

    if (!keyItem)
        return false;

    price = (zone == ZONE_SHATTRATH ? shattrathKeyGold : zone == ZONE_DALARAN ? dalaranKeyGold : firstKeyGold * (owned + 1)) * GOLD;
    return true;
}

void WowPlus::BuyRoomKey(Player* player, uint32 keyItem)
{
    auto city = cityByKey.find(keyItem);
    if (city == cityByKey.end())
        return;

    uint32 offered = 0;
    uint32 price = 0;
    std::string name;
    if (!RoomOffer(player, city->second.zone, offered, name, price) || offered != keyItem)
        return;

    if (!player->HasEnoughMoney(price))
    {
        Tell(player, "You cannot afford it.");
        return;
    }

    if (!player->AddItem(keyItem, 1))
    {
        Tell(player, "You have no room for the key.");
        return;
    }

    player->ModifyMoney(-int32(price));
    Tell(player, "You have the key to a room in " + name + ". Stand where you want your room to be, and use the key.");
}

class WowPlusHomeWorld : public WorldScript
{
public:
    WowPlusHomeWorld() : WorldScript("WowPlusHomeWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("WowPlus.Home.Enable", true);
        roomRadius = sConfigMgr->GetOption<float>("WowPlus.Home.RoomRadius", 30.0f);
        maxPieces = sConfigMgr->GetOption<uint32>("WowPlus.Home.MaxPieces", 80);
        firstKeyGold = sConfigMgr->GetOption<uint32>("WowPlus.Home.FirstKeyGold", 10);
        shattrathKeyGold = sConfigMgr->GetOption<uint32>("WowPlus.Home.ShattrathKeyGold", 1000);
        dalaranKeyGold = sConfigMgr->GetOption<uint32>("WowPlus.Home.DalaranKeyGold", 5000);
    }

    void OnStartup() override
    {
        if (enabled)
            LoadData();
    }

    void OnUpdate(uint32 diff) override
    {
        sinceCheck += diff;
        if (!enabled || sinceCheck < CHECK_INTERVAL)
            return;

        sinceCheck = 0;

        std::vector<Player*> realPlayers;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
                if (player->IsInWorld() && !player->GetSession()->IsHeadless() && !player->IsBeingTeleported())
                    realPlayers.push_back(player);
        }

        for (Player* player : realPlayers)
            UpdateFurnishing(player);
    }
};

class WowPlusHomePlayer : public PlayerScript
{
public:
    WowPlusHomePlayer() : PlayerScript("WowPlusHomePlayer") { }

    void OnPlayerLogin(Player* player) override
    {
        if (!enabled || player->GetSession()->IsHeadless())
            return;

        Home* home = HomeOf(player);
        uint32 const guid = player->GetGUID().GetCounter();

        if (QueryResult result = CharacterDatabase.Query("SELECT city, map, x, y, z, o FROM wowplus_home WHERE guid = {}", guid))
            do
            {
                Field* fields = result->Fetch();
                home->rooms[fields[0].Get<uint32>()] = {fields[1].Get<uint32>(), fields[2].Get<float>(), fields[3].Get<float>(),
                    fields[4].Get<float>(), fields[5].Get<float>()};
            } while (result->NextRow());

        if (QueryResult result = CharacterDatabase.Query("SELECT id, city, item, x, y, z, o FROM wowplus_home_objects WHERE guid = {}", guid))
            do
            {
                Field* fields = result->Fetch();
                home->pieces.push_back({fields[0].Get<uint64>(), fields[1].Get<uint32>(), fields[2].Get<uint32>(), fields[3].Get<float>(),
                    fields[4].Get<float>(), fields[5].Get<float>(), fields[6].Get<float>(), ObjectGuid::Empty});
            } while (result->NextRow());
    }

    // The furniture is only out while its owner is near: gone when they leave the map or the game
    void OnPlayerMapChanged(Player* player) override
    {
        if (Home* home = player->CustomData.Get<Home>(DATA_KEY))
            ClearRoom(home);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (Home* home = player->CustomData.Get<Home>(DATA_KEY))
            ClearRoom(home);
    }
};

// The key to a room
class WowPlusRoomKey : public ItemScript
{
public:
    WowPlusRoomKey() : ItemScript("item_wowplus_room_key") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        auto city = cityByKey.find(item->GetEntry());
        if (!enabled || city == cityByKey.end())
            return true;

        Home* home = HomeOf(player);
        bool const hasRoom = home->rooms.count(city->second.zone);
        bool const inCity = player->GetZoneId() == city->second.zone;

        ClearGossipMenuFor(player);
        if (!hasRoom)
        {
            if (!inCity)
            {
                Tell(player, "This is the key to a room in " + city->second.name + ". Go there, stand where you want your room to be, and use the key.");
                return true;
            }
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Make this spot my room in " + city->second.name + ".", item->GetEntry(), ACTION_CLAIM);
        }
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, "Go to my room in " + city->second.name + ".", item->GetEntry(), ACTION_GO);
            if (RoomNear(player, home, roomRadius) == city->second.zone)
            {
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Pick up the nearest piece of furniture.", item->GetEntry(), ACTION_PICK_UP_NEAREST);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Pick up all the furniture.", item->GetEntry(), ACTION_PICK_UP_ALL);
            }
            if (inCity)
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Move my room to this spot. (The furniture goes back into my bags.)", item->GetEntry(), ACTION_MOVE);
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Never mind.", item->GetEntry(), ACTION_CLOSE);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 /*sender*/, uint32 action) override
    {
        CloseGossipMenuFor(player);

        auto city = cityByKey.find(item->GetEntry());
        if (!enabled || city == cityByKey.end() || action == ACTION_CLOSE)
            return;

        Home* home = HomeOf(player);
        uint32 const zone = city->second.zone;
        uint32 const guid = player->GetGUID().GetCounter();
        auto room = home->rooms.find(zone);

        switch (action)
        {
            case ACTION_CLAIM:
            case ACTION_MOVE:
            {
                if (player->GetZoneId() != zone || player->IsInCombat() || player->IsMounted() || player->IsInFlight())
                {
                    Tell(player, "Stand, on foot and at peace, where you want your room in " + city->second.name + " to be.");
                    return;
                }

                // A room that moves takes its furniture along in the bags
                if (room != home->rooms.end())
                {
                    PickUpAll(player, home, zone);
                    for (Piece const& piece : home->pieces)
                        if (piece.city == zone)
                        {
                            Tell(player, "Your bags are full. Make room for the rest of the furniture, then move the room.");
                            return;
                        }
                }

                home->rooms[zone] = {player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation()};
                CharacterDatabase.Execute("REPLACE INTO wowplus_home (guid, city, map, x, y, z, o) VALUES ({}, {}, {}, {}, {}, {}, {})",
                    guid, zone, player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
                Tell(player, "This is now your room in " + city->second.name + ". The key brings you back here. Furniture is sold by the innkeepers of the cities.");
                break;
            }
            case ACTION_GO:
                if (room == home->rooms.end())
                    return;
                if (player->IsInCombat() || player->IsInFlight())
                {
                    Tell(player, "Not right now.");
                    return;
                }
                player->TeleportTo(room->second.map, room->second.x, room->second.y, room->second.z, room->second.o);
                break;
            case ACTION_PICK_UP_NEAREST:
            {
                std::size_t nearest = home->pieces.size();
                float best = PICK_UP_RANGE;
                for (std::size_t index = 0; index < home->pieces.size(); ++index)
                {
                    Piece const& piece = home->pieces[index];
                    float const distance = player->GetExactDist(piece.x, piece.y, piece.z);
                    if (piece.city == zone && distance < best)
                    {
                        best = distance;
                        nearest = index;
                    }
                }

                if (nearest == home->pieces.size())
                    Tell(player, "Stand next to the piece you want to pick up.");
                else
                    PickUp(player, home, nearest);
                break;
            }
            case ACTION_PICK_UP_ALL:
                if (uint32 const taken = PickUpAll(player, home, zone))
                    Tell(player, std::to_string(taken) + " pieces of furniture are back in your bags.");
                break;
            default:
                break;
        }
    }
};

// A piece of furniture, as an item: used inside a room it is put down
class WowPlusFurniture : public ItemScript
{
public:
    WowPlusFurniture() : ItemScript("item_wowplus_furniture") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!enabled || !objectByItem.count(item->GetEntry()))
            return true;

        Home* home = HomeOf(player);
        uint32 const city = RoomNear(player, home, roomRadius);
        if (!city)
        {
            Tell(player, home->rooms.empty() ?
                "You have no room yet. The innkeepers of the cities sell the key to one." :
                "Furniture can only be put down in your own room. Stand within reach of the spot you claimed.");
            return true;
        }

        uint32 inRoom = 0;
        for (Piece const& piece : home->pieces)
            inRoom += piece.city == city;

        if (inRoom >= maxPieces)
        {
            Tell(player, "This room is full: " + std::to_string(maxPieces) + " pieces.");
            return true;
        }

        if (player->IsInCombat() || player->IsMounted())
            return true;

        Piece piece{nextPieceId++, city, item->GetEntry(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),
            player->GetOrientation(), ObjectGuid::Empty};
        CharacterDatabase.Execute("INSERT INTO wowplus_home_objects (id, guid, city, item, x, y, z, o) VALUES ({}, {}, {}, {}, {}, {}, {}, {})",
            piece.id, player->GetGUID().GetCounter(), city, piece.item, piece.x, piece.y, piece.z, piece.o);

        uint32 one = 1;
        player->DestroyItemCount(item, one, true);          // the item may be gone after this line

        home->pieces.push_back(piece);
        if (home->furnishedCity == city)
            PutOut(player, home->pieces.back());
        return true;
    }
};

// The storage chest: your bank
class WowPlusHomeBank : public GameObjectScript
{
public:
    WowPlusHomeBank() : GameObjectScript("go_wowplus_home_bank") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        player->GetSession()->SendShowBank(player->GetGUID());
        return true;
    }
};

// The bed: rested, as if you had slept in an inn for a long while
class WowPlusHomeBed : public GameObjectScript
{
public:
    WowPlusHomeBed() : GameObjectScript("go_wowplus_home_bed") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        Home* home = HomeOf(player);
        time_t const now = time(nullptr);
        player->SetStandState(UNIT_STAND_STATE_SLEEP);

        if (now - home->restedAt < time_t(BED_REST_SECONDS))
        {
            Tell(player, "You lie down for a while. You are not tired enough to sleep again yet.");
            return true;
        }

        home->restedAt = now;
        // Half of the most rested experience a character can have (one and a half levels' worth)
        float const most = float(player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP)) * 1.5f / 2.0f;
        player->SetRestBonus(std::max(player->GetRestBonus(), most * 0.5f));
        Tell(player, "You sleep in your own bed, and wake up rested.");
        return true;
    }
};

void AddSC_WowPlusHome()
{
    new WowPlusHomeWorld();
    new WowPlusHomePlayer();
    new WowPlusRoomKey();
    new WowPlusFurniture();
    new WowPlusHomeBank();
    new WowPlusHomeBed();
}
