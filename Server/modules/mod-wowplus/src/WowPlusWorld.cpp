/*
 * WowPlus world: the clock, the weather, and word of the champions.
 *
 * - The day. A full day and night passes in WowPlus.World.DayMinutes real minutes (60 by
 *   default), so an evening of play sees dawn, noon, dusk and night. Only the sky and the
 *   minimap clock follow it: the game client is told the hour and how fast it runs. Nothing
 *   the server does by the clock (daily resets, events) changes.
 * - The weather. Every WowPlus.Weather.SpellMinutes the sky of each zone is decided anew, from
 *   the world seed and the time, so it is the same for everyone and can be known in advance by
 *   nobody. More often and heavier than the game's own weather. Which kind (rain, snow, sand)
 *   comes from the game's weather table where the zone has a line there, and from where the
 *   zone lies otherwise.
 * - The champions (Settings/SQL/world/..._champions.sql) are read at startup so that bots can
 *   pass on where one has been seen.
 */

#include "WowPlusJournal.h"

#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QueryResult.h"
#include "ScriptMgr.h"
#include "StringFormat.h"
#include "Timer.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <array>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    constexpr uint32 CHECK_INTERVAL = 10 * IN_MILLISECONDS;
    constexpr uint32 CLOCK_RESEND = 5 * MINUTE * IN_MILLISECONDS;   // keeps the client's clock from drifting

    enum WeatherKind : uint8
    {
        KIND_NONE,
        KIND_RAIN,
        KIND_SNOW,
        KIND_SAND
    };

    struct Champion
    {
        std::string name;
        std::string kind;
        uint32 zone;
        uint8 level;
    };

    uint32 dayMinutes = 60;
    bool weatherEnabled = true;
    uint32 weatherChance = 40;
    uint32 weatherHeavyChance = 40;
    uint32 weatherSpellMinutes = 20;
    bool weatherEverywhere = true;
    uint64 worldSeed = 1;

    std::unordered_map<uint32, std::array<uint32, 3>> weatherTable;     // zone -> chance of rain, snow, sand this season
    std::unordered_map<uint64, uint32> appliedWeather;                 // map and zone -> what we last set there
    std::vector<Champion> champions;
    uint32 sinceCheck = 0;
    uint32 sinceClock = 0;

    // Zones the game's weather table leaves out, by what falls there
    std::unordered_set<uint32> const SNOW_ZONES = {1, 36, 618, 65, 67, 210, 4197};      // Dun Morogh, Alterac, Winterspring, Dragonblight, Storm Peaks, Icecrown, Wintergrasp
    std::unordered_set<uint32> const SAND_ZONES = {440, 1377, 400};                     // Tanaris, Silithus, Thousand Needles
    std::unordered_set<uint32> const DRY_ZONES = {1519, 1537, 1657, 3557, 1637, 1638, 1497, 3487, 4395, 3703};  // the capitals

    // ---------------------------------------------------------------------------------------
    // The clock
    // ---------------------------------------------------------------------------------------

    void SendClock(Player* player)
    {
        if (!dayMinutes || player->GetSession()->IsHeadless())
            return;

        // How many game days pass in one real day, and where in the game day we are
        uint32 const factor = std::max<uint32>(1, 1440 / dayMinutes);
        time_t const now = time(nullptr);
        uint32 const gameSecondOfDay = uint32((uint64(now) * factor) % DAY);

        // Today's date with the game's hour: the packet wants a real point in time
        tm local = Acore::Time::TimeBreakdown(now);
        time_t const shown = now - (local.tm_hour * HOUR + local.tm_min * MINUTE + local.tm_sec) + gameSecondOfDay;

        WorldPacket data(SMSG_LOGIN_SETTIMESPEED, 4 + 4 + 4);
        data.AppendPackedTime(shown);
        data << float(factor) / 60.0f;                      // game minutes per real second
        data << uint32(0);
        player->SendDirectMessage(&data);
    }

    // ---------------------------------------------------------------------------------------
    // The weather
    // ---------------------------------------------------------------------------------------

    uint64 Mix(uint64 a, uint64 b, uint64 c)
    {
        uint64 x = a * 0x9E3779B97F4A7C15ULL ^ (b + 0x7F4A7C15ULL) * 0xBF58476D1CE4E5B9ULL ^ (c + 0x1CE4E5B9ULL) * 0x94D049BB133111EBULL;
        x ^= x >> 31;
        x *= 0xD6E8FEB86659FD93ULL;
        return x ^ (x >> 29);
    }

    WeatherKind KindFor(Player* player, uint32 zone, uint64 roll)
    {
        auto listed = weatherTable.find(zone);
        if (listed != weatherTable.end())
        {
            std::array<uint32, 3> const& chances = listed->second;
            uint32 const total = chances[0] + chances[1] + chances[2];
            if (!total)
                return KIND_NONE;

            uint32 const pick = uint32(roll % total);
            return pick < chances[0] ? KIND_RAIN : pick < chances[0] + chances[1] ? KIND_SNOW : KIND_SAND;
        }

        // Not in the game's table: only the old world and Northrend, and only under the open sky
        if (!weatherEverywhere || DRY_ZONES.count(zone) || (player->GetMapId() != 0 && player->GetMapId() != 1 && player->GetMapId() != 571))
            return KIND_NONE;

        return SNOW_ZONES.count(zone) ? KIND_SNOW : SAND_ZONES.count(zone) ? KIND_SAND : KIND_RAIN;
    }

    void UpdateWeather(Player* player)
    {
        if (!player->GetMap() || player->GetMap()->Instanceable())
            return;

        uint32 const zone = player->GetZoneId();
        uint64 const spell = uint64(time(nullptr)) / (std::max<uint32>(1, weatherSpellMinutes) * MINUTE);
        uint64 const roll = Mix(worldSeed, zone, spell);

        WeatherState state = WEATHER_STATE_FINE;
        float grade = 0.0f;
        if (roll % 100 < weatherChance)
        {
            // How hard: light, medium or heavy
            uint32 const strength = (roll >> 8) % 100 < weatherHeavyChance ? 2 : (roll >> 16) % 2;
            static float const grades[] = {0.3f, 0.6f, 0.95f};
            grade = grades[strength];

            switch (KindFor(player, zone, roll >> 24))
            {
                case KIND_RAIN:
                    state = strength == 2 ? WEATHER_STATE_HEAVY_RAIN : strength ? WEATHER_STATE_MEDIUM_RAIN : WEATHER_STATE_LIGHT_RAIN;
                    break;
                case KIND_SNOW:
                    state = strength == 2 ? WEATHER_STATE_HEAVY_SNOW : strength ? WEATHER_STATE_MEDIUM_SNOW : WEATHER_STATE_LIGHT_SNOW;
                    break;
                case KIND_SAND:
                    state = strength == 2 ? WEATHER_STATE_HEAVY_SANDSTORM : strength ? WEATHER_STATE_MEDIUM_SANDSTORM : WEATHER_STATE_LIGHT_SANDSTORM;
                    break;
                default:
                    grade = 0.0f;
                    break;
            }
        }

        uint64 const place = uint64(player->GetMapId()) << 32 | zone;
        auto applied = appliedWeather.find(place);
        if (applied != appliedWeather.end() && applied->second == uint32(state))
            return;

        appliedWeather[place] = uint32(state);
        player->GetMap()->SetZoneWeather(zone, state, grade);
    }
}

std::string WowPlus::ChampionRumour(uint8 level)
{
    std::vector<Champion const*> fitting;
    for (Champion const& champion : champions)
        if (champion.level + 3 >= level && champion.level <= level + 5)
            fitting.push_back(&champion);

    if (fitting.empty())
        return "";

    Champion const* champion = fitting[urand(0, fitting.size() - 1)];
    AreaTableEntry const* zone = sAreaTableStore.LookupEntry(champion->zone);
    if (!zone)
        return "";

    return Acore::StringFormat("A champion has been seen in {}: {}, a {} bigger and stronger than the rest of its kind (about level {}). "
        "The innkeeper there pays a bounty for it.", zone->area_name[0], champion->name, champion->kind, champion->level);
}

class WowPlusWorldWorld : public WorldScript
{
public:
    WowPlusWorldWorld() : WorldScript("WowPlusWorldWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        dayMinutes = sConfigMgr->GetOption<uint32>("WowPlus.World.DayMinutes", 60);
        if (dayMinutes > 1440)
            dayMinutes = 0;                                 // longer than a real day makes no sense: the real clock then

        weatherEnabled = sConfigMgr->GetOption<bool>("WowPlus.Weather.Enable", true);
        weatherChance = sConfigMgr->GetOption<uint32>("WowPlus.Weather.Chance", 40);
        weatherHeavyChance = sConfigMgr->GetOption<uint32>("WowPlus.Weather.HeavyChance", 40);
        weatherSpellMinutes = sConfigMgr->GetOption<uint32>("WowPlus.Weather.SpellMinutes", 20);
        weatherEverywhere = sConfigMgr->GetOption<bool>("WowPlus.Weather.Everywhere", true);
        worldSeed = sConfigMgr->GetOption<uint64>("WowPlus.World.Seed", 1);
        appliedWeather.clear();
    }

    void OnStartup() override
    {
        // The game's own weather table, for the season it is now (the same reckoning as the game's)
        static char const* const seasons[] = {"spring", "summer", "fall", "winter"};
        uint32 const season = ((Acore::Time::GetDayInYear() - 78 + 365) / 91) % 4;
        if (QueryResult result = WorldDatabase.Query("SELECT zone, {0}_rain_chance, {0}_snow_chance, {0}_storm_chance FROM game_weather", seasons[season]))
            do
            {
                Field* fields = result->Fetch();
                weatherTable[fields[0].Get<uint32>()] = {fields[1].Get<uint32>(), fields[2].Get<uint32>(), fields[3].Get<uint32>()};
            } while (result->NextRow());

        if (WorldDatabase.Query("SHOW TABLES LIKE 'wowplus_champions'"))
            if (QueryResult result = WorldDatabase.Query("SELECT name, kind, zone, level FROM wowplus_champions"))
                do
                {
                    Field* fields = result->Fetch();
                    champions.push_back({fields[0].Get<std::string>(), fields[1].Get<std::string>(), fields[2].Get<uint32>(), fields[3].Get<uint8>()});
                } while (result->NextRow());

        LOG_INFO("server.loading", ">> WowPlus world: a day lasts {} minutes, {} champions are abroad", dayMinutes ? dayMinutes : 1440, champions.size());
    }

    void OnUpdate(uint32 diff) override
    {
        sinceCheck += diff;
        sinceClock += diff;
        if (sinceCheck < CHECK_INTERVAL)
            return;

        sinceCheck = 0;
        bool const clock = sinceClock >= CLOCK_RESEND;
        if (clock)
            sinceClock = 0;

        std::vector<Player*> realPlayers;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
                if (player->IsInWorld() && !player->GetSession()->IsHeadless() && !player->IsBeingTeleported())
                    realPlayers.push_back(player);
        }

        for (Player* player : realPlayers)
        {
            if (clock)
                SendClock(player);
            if (weatherEnabled)
                UpdateWeather(player);
        }
    }
};

class WowPlusWorldPlayer : public PlayerScript
{
public:
    WowPlusWorldPlayer() : PlayerScript("WowPlusWorldPlayer") { }

    void OnPlayerLogin(Player* player) override
    {
        SendClock(player);
    }

    // The game tells the client the real time again on every change of map
    void OnPlayerMapChanged(Player* player) override
    {
        SendClock(player);
    }
};

void AddSC_WowPlusWorld()
{
    new WowPlusWorldWorld();
    new WowPlusWorldPlayer();
}
