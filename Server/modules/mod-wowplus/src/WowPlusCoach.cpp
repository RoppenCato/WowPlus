/*
 * WowPlus coach: every innkeeper can send you to another inn, for a fare.
 *
 * Decided 2026-10-11, when the hearthstone stopped teleporting everywhere: talk to an innkeeper,
 * say you need a coach, pick another inn on the same continent, pay, and the screen fades to
 * black and you arrive there. No need to have been there before. No coach is seen and no time
 * passes; crossing the sea stays with boats and zeppelins.
 *
 * The fare is like a flight's: small where a low-level character travels, more in the lands of
 * higher levels, and growing with the distance.
 *
 * This file also adds the innkeepers' other new line: in a city, the key to a room of your own
 * (WowPlusHome.cpp), since both have to live in the same conversation.
 *
 * The fade is done by the addon WowPlusCoach, told by a hidden message ("WPC" and the text to
 * show). Without the addon the journey is an ordinary loading screen.
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QueryResult.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WowPlusHome.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    // Senders of our own lines in an innkeeper's conversation; far from anything the game uses
    constexpr uint32 SENDER_COACH_LIST = 9200001;           // action: the page to show
    constexpr uint32 SENDER_COACH_GO = 9200002;             // action: the inn to go to
    constexpr uint32 SENDER_ROOM = 9200003;                 // action: the key to buy

    constexpr uint32 PAGE_SIZE = 10;
    constexpr float SAME_INN_RANGE = 100.0f;                // an inn this near is the one you are standing in
    constexpr uint32 FADE_TIME = 1600;                      // ms between paying and arriving, for the fade to black
    constexpr uint32 LOWEST_FARE = 30;                      // copper
    constexpr uint32 UNKNOWN_ZONE_LEVEL = 10;

    struct Inn
    {
        uint32 map;
        float x, y, z, o;
        uint32 zone;
        uint32 area;
        uint32 faction;                                     // the innkeeper's
    };

    struct Journey
    {
        ObjectGuid player;
        uint32 inn;
        uint32 wait;                                        // ms
    };

    bool enabled = true;
    float priceFactor = 1.0f;

    std::vector<Inn> inns;
    std::unordered_map<uint32, uint32> zoneLevel;           // the usual level of a zone, from its quests
    std::vector<Journey> journeys;

    std::string Money(uint32 copper)
    {
        std::string text;
        if (copper >= GOLD)
            text += std::to_string(copper / GOLD) + "g ";
        if (copper % GOLD >= SILVER)
            text += std::to_string(copper % GOLD / SILVER) + "s ";
        if (copper % SILVER || text.empty())
            text += std::to_string(copper % SILVER) + "c";
        if (text.back() == ' ')
            text.pop_back();
        return text;
    }

    std::string NameOf(Inn const& inn)
    {
        AreaTableEntry const* zone = sAreaTableStore.LookupEntry(inn.zone);
        AreaTableEntry const* area = sAreaTableStore.LookupEntry(inn.area);
        std::string name = area ? area->area_name[0] : "";
        if (zone && name != zone->area_name[0])
            name += (name.empty() ? "" : ", ") + std::string(zone->area_name[0]);
        return name.empty() ? "an inn" : name;
    }

    uint32 LevelOf(uint32 zone, uint32 fallback)
    {
        auto found = zoneLevel.find(zone);
        return found != zoneLevel.end() ? found->second : fallback;
    }

    // Like a flight: a rate per thousand yards that rises with the level of the land travelled to
    uint32 Fare(Creature* innkeeper, Inn const& to)
    {
        uint32 const level = LevelOf(to.zone, LevelOf(innkeeper->GetZoneId(), UNKNOWN_ZONE_LEVEL));
        float const rate = 20.0f + 0.6f * float(level) * float(level);
        float const distance = innkeeper->GetExactDist(to.x, to.y, to.z);
        uint32 const fare = uint32(distance / 1000.0f * rate * priceFactor);
        return std::max(LOWEST_FARE, fare / 5 * 5);
    }

    bool Welcomes(Inn const& inn, Player* player)
    {
        FactionTemplateEntry const* theirs = sFactionTemplateStore.LookupEntry(inn.faction);
        FactionTemplateEntry const* mine = player->GetFactionTemplateEntry();
        return theirs && mine && !theirs->IsHostileTo(*mine) && (theirs->IsFriendlyTo(*mine) || theirs->IsNeutralToAll());
    }

    // The inns this player can ride to from here, nearest first
    std::vector<uint32> Destinations(Player* player, Creature* innkeeper)
    {
        std::vector<uint32> found;
        for (uint32 index = 0; index < inns.size(); ++index)
        {
            Inn const& inn = inns[index];
            if (inn.map == innkeeper->GetMapId() && innkeeper->GetExactDist(inn.x, inn.y, inn.z) > SAME_INN_RANGE && Welcomes(inn, player))
                found.push_back(index);
        }

        std::sort(found.begin(), found.end(), [innkeeper](uint32 a, uint32 b)
        {
            return innkeeper->GetExactDist(inns[a].x, inns[a].y, inns[a].z) < innkeeper->GetExactDist(inns[b].x, inns[b].y, inns[b].z);
        });
        return found;
    }

    void ShowCoachList(Player* player, Creature* innkeeper, uint32 page)
    {
        std::vector<uint32> const destinations = Destinations(player, innkeeper);
        ClearGossipMenuFor(player);

        if (destinations.empty())
            ChatHandler(player->GetSession()).SendSysMessage("No coach leaves from here.");

        for (uint32 shown = page * PAGE_SIZE; shown < destinations.size() && shown < (page + 1) * PAGE_SIZE; ++shown)
        {
            Inn const& inn = inns[destinations[shown]];
            uint32 const fare = Fare(innkeeper, inn);
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, NameOf(inn) + "  (" + Money(fare) + ")", SENDER_COACH_GO, destinations[shown],
                "Take the coach to " + NameOf(inn) + "?", fare, false);
        }

        if ((page + 1) * PAGE_SIZE < destinations.size())
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Further away...", SENDER_COACH_LIST, page + 1);
        if (page)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Nearer...", SENDER_COACH_LIST, page - 1);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, innkeeper->GetGUID());
    }

    void TakeCoach(Player* player, Creature* innkeeper, uint32 index)
    {
        CloseGossipMenuFor(player);
        if (index >= inns.size() || player->IsInCombat() || !player->IsAlive())
            return;

        Inn const& inn = inns[index];
        if (inn.map != innkeeper->GetMapId() || !Welcomes(inn, player))
            return;

        uint32 const fare = Fare(innkeeper, inn);
        if (!player->HasEnoughMoney(fare))
        {
            ChatHandler(player->GetSession()).SendSysMessage("You cannot afford the fare.");
            return;
        }

        player->ModifyMoney(-int32(fare));
        if (player->IsMounted())
            player->Dismount();

        // The addon fades the screen to black with this line; then the journey is the loading screen
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, "WPC\tThe coach leaves for " + NameOf(inn) + "...");
        player->SendDirectMessage(&data);

        journeys.push_back({player->GetGUID(), index, FADE_TIME});
    }

    void LoadInns()
    {
        // The usual level of each zone: the average level of its quests
        std::unordered_map<uint32, std::pair<uint32, uint32>> sums;
        for (auto const& [id, quest] : sObjectMgr->GetQuestTemplates())
            if (quest->GetZoneOrSort() > 0 && quest->GetQuestLevel() > 0)
            {
                auto& sum = sums[uint32(quest->GetZoneOrSort())];
                sum.first += uint32(quest->GetQuestLevel());
                ++sum.second;
            }
        for (auto const& [zone, sum] : sums)
            if (sum.second >= 5)
                zoneLevel[zone] = sum.first / sum.second;

        // Every innkeeper standing out in the world, one per area and faction
        std::unordered_set<uint64> seen;
        if (QueryResult result = WorldDatabase.Query(
            "SELECT c.map, c.position_x, c.position_y, c.position_z, c.orientation, c.zoneId, c.areaId, ct.faction "
            "FROM creature c JOIN creature_template ct ON ct.entry = c.id "
            "LEFT JOIN game_event_creature e ON e.guid = c.guid "
            "WHERE (ct.npcflag & 65536) AND c.map IN (0, 1, 530, 571) AND c.zoneId > 0 AND c.phaseMask = 1 AND e.guid IS NULL AND ct.entry < 9000000 "
            "ORDER BY c.guid"))
            do
            {
                Field* fields = result->Fetch();
                if (!seen.insert(uint64(fields[6].Get<uint32>()) << 32 | fields[7].Get<uint32>()).second)
                    continue;

                Inn inn{fields[0].Get<uint32>(), fields[1].Get<float>(), fields[2].Get<float>(), fields[3].Get<float>(), fields[4].Get<float>(),
                    fields[5].Get<uint32>(), fields[6].Get<uint32>(), fields[7].Get<uint32>()};
                // Arrive a few steps in front of the innkeeper
                inn.x += 3.0f * std::cos(inn.o);
                inn.y += 3.0f * std::sin(inn.o);
                inn.o = Position::NormalizeOrientation(inn.o + float(M_PI));
                inns.push_back(inn);
            } while (result->NextRow());

        LOG_INFO("server.loading", ">> WowPlus coach: {} inns to travel between", inns.size());
    }
}

class WowPlusCoachWorld : public WorldScript
{
public:
    WowPlusCoachWorld() : WorldScript("WowPlusCoachWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("WowPlus.Coach.Enable", true);
        priceFactor = sConfigMgr->GetOption<float>("WowPlus.Coach.PriceFactor", 1.0f);
    }

    void OnStartup() override
    {
        LoadInns();
    }

    void OnUpdate(uint32 diff) override
    {
        for (auto journey = journeys.begin(); journey != journeys.end();)
        {
            if (journey->wait > diff)
            {
                journey->wait -= diff;
                ++journey;
                continue;
            }

            if (Player* player = ObjectAccessor::FindConnectedPlayer(journey->player))
                if (player->IsInWorld() && journey->inn < inns.size())
                {
                    Inn const& inn = inns[journey->inn];
                    player->TeleportTo(inn.map, inn.x, inn.y, inn.z, inn.o);
                }

            journey = journeys.erase(journey);
        }
    }
};

// The innkeepers' conversation: what they always say, plus our lines
class WowPlusInnkeepers : public AllCreatureScript
{
public:
    WowPlusInnkeepers() : AllCreatureScript("WowPlusInnkeepers") { }

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        if (!creature->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER) || player->GetSession()->IsHeadless() || creature->GetEntry() >= 9000000)
            return false;

        uint32 keyItem = 0;
        uint32 price = 0;
        std::string city;
        bool const room = WowPlus::RoomOffer(player, creature->GetZoneId(), keyItem, city, price);
        bool const coach = enabled && !inns.empty();
        if (!room && !coach)
            return false;                                   // nothing of ours to add: the game's own conversation

        // The game's own lines first (make this my home, show me your goods, quests...)
        player->PrepareGossipMenu(creature, creature->GetCreatureTemplate()->GossipMenuId, true);

        if (coach)
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, "I need a coach.", SENDER_COACH_LIST, 0);
        if (room)
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "I would like a room of my own in " + city + ".  (" + Money(price) + ")", SENDER_ROOM, keyItem,
                "Buy the key to a room in " + city + "?", price, false);

        player->SendPreparedGossip(creature);
        return true;
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        switch (sender)
        {
            case SENDER_COACH_LIST:
                ShowCoachList(player, creature, action);
                return true;
            case SENDER_COACH_GO:
                TakeCoach(player, creature, action);
                return true;
            case SENDER_ROOM:
                CloseGossipMenuFor(player);
                WowPlus::BuyRoomKey(player, action);
                return true;
            default:
                return false;                               // one of the game's own lines
        }
    }
};

void AddSC_WowPlusCoach()
{
    new WowPlusCoachWorld();
    new WowPlusInnkeepers();
}
