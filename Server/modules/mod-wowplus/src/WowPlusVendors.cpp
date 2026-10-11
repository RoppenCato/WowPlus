/*
 * WowPlus vendors: the vendors the game already has carry a little surprise stock.
 *
 * - An armour or weapon vendor will often have a green of the kind it sells, and now and then a
 *   blue. Other speciality vendors now and then have a potion, an elixir or a recipe. Any of
 *   them may, very rarely, have a mount or a companion pet.
 * - The merchants who walk a route get a theme (beasts, mounts, potions, armour, weapons,
 *   recipes, oddities) and a real stock of it, on top of what they sold before.
 * - Stock is for the level of the zone the vendor stands in, not for the player who walks in.
 * - Innkeepers, food and drink sellers and stable masters are left alone, and so are the
 *   vendors of the starting areas.
 *
 * Everything is worked out from the world seed and the week: the same week gives the same
 * stock however often the server restarts, and a new week gives new stock. Which theme a
 * wanderer has depends on the seed alone, so it holds until the seed is changed.
 *
 * Nothing is written to the database. A vendor's stock is worked out the first time the
 * vendor comes into view, and added to its list in memory.
 */

#include "Config.h"
#include "Creature.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "StringFormat.h"
#include "WowPlusItems.h"

#include <array>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    using namespace WowPlus;

    constexpr uint32 MAX_VENDOR_LIST = 145;                 // the game shows at most 150 items; leave room
    constexpr uint32 STARTING_AREA_LEVEL = 5;               // vendors in areas up to this level get nothing
    constexpr uint32 OUR_NPC_ENTRIES_FROM = 9000000;        // the hub and other NPCs of our own

    enum Theme : uint8
    {
        THEME_BEASTMASTER,
        THEME_STABLEHAND,
        THEME_ALCHEMIST,
        THEME_ARMOURER,
        THEME_WEAPONSMITH,
        THEME_RECIPE_PEDLAR,
        THEME_SMUGGLER,
        THEME_COUNT
    };

    struct WowPlusVendorConfig
    {
        bool enabled = true;
        uint32 seed = 1;
        uint32 periodDays = 7;
        float gearGreenChance = 60.0f;
        float gearBlueChance = 12.0f;
        float gearEpicChance = 1.0f;
        float otherConsumableChance = 15.0f;
        float otherRecipeChance = 4.0f;
        float otherGearChance = 8.0f;
        float mountChance = 0.25f;
        float petChance = 0.5f;
        bool wanderers = true;
        float priceFactor = 1.0f;
        uint32 mountPriceGold = 800;                        // times the riding tier: 20 -> 1x, 40 -> 2x, 60 -> 5x, 70 -> 10x
        uint32 petPriceGold = 40;
        bool debug = false;
    };

    WowPlusVendorConfig config;

    // A small generator of its own, so that the same seed always gives the same stock.
    // (The core's random helpers cannot be seeded.)
    class Dice
    {
    public:
        Dice(uint64 a, uint64 b, uint64 c) : _state(a * 0x9E3779B97F4A7C15ull ^ (b + 0x632BE59BD9B4E019ull) * 0xBF58476D1CE4E5B9ull ^ c * 0x94D049BB133111EBull)
        {
            Next();
        }

        uint64 Next()
        {
            uint64 z = (_state += 0x9E3779B97F4A7C15ull);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            return z ^ (z >> 31);
        }

        // A whole number from low to high, both included
        uint32 Range(uint32 low, uint32 high)
        {
            return high <= low ? low : low + uint32(Next() % (uint64(high) - low + 1));
        }

        bool Chance(float percent)
        {
            return float(Next() % 1000000) < percent * 10000.0f;
        }

        template <typename T>
        T const& Pick(std::vector<T> const& list)
        {
            return list[Range(0, uint32(list.size()) - 1)];
        }

    private:
        uint64 _state;
    };

    // A vendor that came into view and waits for the world thread to stock it
    struct Sighting
    {
        uint32 entry;
        uint32 areaLevel;                                   // 0: no level, such as a city
        uint32 npcFlags;
        bool wanderer;
    };

    std::mutex sightingMutex;
    std::vector<Sighting> sightings;                        // filled from the map threads
    std::unordered_set<uint32> sighted;                     // entries already queued or stocked this period

    // World thread only
    uint64 stockedPeriod = 0;
    std::unordered_map<uint32, std::vector<uint32>> ourItems;   // vendor entry -> the items we added

    uint64 CurrentPeriod()
    {
        return uint64(GameTime::GetGameTime().count()) / (uint64(std::max<uint32>(1, config.periodDays)) * DAY) + 1;
    }

    void LoadConfig()
    {
        config.enabled = sConfigMgr->GetOption<bool>("WowPlus.Vendors.Enable", true);
        config.seed = sConfigMgr->GetOption<uint32>("WowPlus.World.Seed", 1);
        config.periodDays = sConfigMgr->GetOption<uint32>("WowPlus.Vendors.PeriodDays", 7);
        config.gearGreenChance = sConfigMgr->GetOption<float>("WowPlus.Vendors.Gear.GreenChance", 60.0f);
        config.gearBlueChance = sConfigMgr->GetOption<float>("WowPlus.Vendors.Gear.BlueChance", 12.0f);
        config.gearEpicChance = sConfigMgr->GetOption<float>("WowPlus.Vendors.Gear.EpicChance", 1.0f);
        config.otherConsumableChance = sConfigMgr->GetOption<float>("WowPlus.Vendors.Other.ConsumableChance", 15.0f);
        config.otherRecipeChance = sConfigMgr->GetOption<float>("WowPlus.Vendors.Other.RecipeChance", 4.0f);
        config.otherGearChance = sConfigMgr->GetOption<float>("WowPlus.Vendors.Other.GearChance", 8.0f);
        config.mountChance = sConfigMgr->GetOption<float>("WowPlus.Vendors.MountChance", 0.25f);
        config.petChance = sConfigMgr->GetOption<float>("WowPlus.Vendors.PetChance", 0.5f);
        config.wanderers = sConfigMgr->GetOption<bool>("WowPlus.Vendors.Wanderers", true);
        config.priceFactor = sConfigMgr->GetOption<float>("WowPlus.Vendors.PriceFactor", 1.0f);
        config.mountPriceGold = sConfigMgr->GetOption<uint32>("WowPlus.Vendors.MountPriceGold", 800);
        config.petPriceGold = sConfigMgr->GetOption<uint32>("WowPlus.Vendors.PetPriceGold", 40);
        config.debug = sConfigMgr->GetOption<bool>("WowPlus.Vendors.Debug", false);
    }

    // A vendor charges an item's own buy price. For dropped gear the game's price is a token
    // one, far too low for something you get to choose. Raised once at startup, in memory:
    // item level squared, times a factor for the rarity (the rule the hub uses). The sell
    // price is not touched, so buying and selling again always loses money.
    // Side effect: the few vendors in the game that already sold such items for gold charge more too.
    void RaisePrice(ItemTemplate const* proto, uint32 atLeastCopper)
    {
        if (uint32(proto->BuyPrice) < atLeastCopper)
            const_cast<ItemTemplate*>(proto)->BuyPrice = int32(atLeastCopper);
    }

    void SetPrices()
    {
        static std::array<float, QUALITY_COUNT> const factorByRarity = {40.0f, 150.0f, 600.0f, 2000.0f};

        for (uint32 level = 1; level < MAX_LEVEL_INDEX; ++level)
        {
            for (uint8 quality = 0; quality < QUALITY_COUNT; ++quality)
                for (ItemTemplate const* proto : Equipment(level, quality))
                    RaisePrice(proto, uint32(float(proto->ItemLevel) * float(proto->ItemLevel) * factorByRarity[quality] * config.priceFactor));

            for (ItemTemplate const* proto : Recipes(level))
                RaisePrice(proto, uint32(float(level) * float(level) * 40.0f * config.priceFactor));
        }

        for (ItemTemplate const* proto : Mounts())
        {
            uint32 const level = MountLevel(proto);
            uint32 const tier = level >= 70 ? 10 : level >= 60 ? 5 : level >= 40 ? 2 : 1;
            RaisePrice(proto, uint32(float(config.mountPriceGold * tier * GOLD) * config.priceFactor));
        }

        for (ItemTemplate const* proto : Pets())
            RaisePrice(proto, uint32(float(config.petPriceGold * GOLD) * config.priceFactor));
    }

    // What a vendor already sells decides what kind of surprise fits it
    struct Trade
    {
        uint32 weapons = 0;
        uint32 armor = 0;
        uint32 foodAndDrink = 0;
        uint32 forCurrency = 0;
        uint32 total = 0;
        std::unordered_set<uint32> armorSubclasses;
        std::unordered_set<uint32> items;
    };

    Trade ReadTrade(VendorItemData const* list)
    {
        Trade trade;
        for (VendorItem const* vendorItem : list->m_items)
        {
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(vendorItem->item);
            if (!proto)
                continue;

            ++trade.total;
            trade.items.insert(vendorItem->item);
            if (vendorItem->ExtendedCost)
                ++trade.forCurrency;

            if (proto->Class == ITEM_CLASS_WEAPON)
                ++trade.weapons;
            else if (proto->Class == ITEM_CLASS_ARMOR)
            {
                ++trade.armor;
                trade.armorSubclasses.insert(proto->SubClass);
            }
            else if (proto->Class == ITEM_CLASS_CONSUMABLE && proto->SubClass == ITEM_SUBCLASS_FOOD)
                ++trade.foodAndDrink;
        }

        return trade;
    }

    // The level a zone-less vendor (a city) stocks for: anything
    uint32 StockLevel(Dice& dice, uint32 areaLevel)
    {
        return areaLevel ? areaLevel : dice.Range(10, 78);
    }

    // classFilter: ITEM_CLASS_WEAPON, ITEM_CLASS_ARMOR, or MAX_ITEM_CLASS for either
    ItemTemplate const* PickGear(Dice& dice, uint32 level, uint8 quality, uint32 classFilter, Trade const* trade, uint32 levelsAbove = 5)
    {
        std::vector<ItemTemplate const*> candidates;
        for (uint32 requiredLevel = level; requiredLevel <= std::min<uint32>(level + levelsAbove, MAX_LEVEL_INDEX - 1); ++requiredLevel)
            for (ItemTemplate const* proto : Equipment(requiredLevel, quality))
            {
                if (classFilter != MAX_ITEM_CLASS && proto->Class != classFilter)
                    continue;

                // An armourer who sells mail and shields stocks mail and shields
                if (trade && proto->Class == ITEM_CLASS_ARMOR && !trade->armorSubclasses.empty() &&
                    !trade->armorSubclasses.count(proto->SubClass) && proto->SubClass != ITEM_SUBCLASS_ARMOR_MISC)
                    continue;

                candidates.push_back(proto);
            }

        return candidates.empty() ? nullptr : dice.Pick(candidates);
    }

    ItemTemplate const* PickFromLevels(Dice& dice, ItemList const& (*pool)(uint32), uint32 fromLevel, uint32 toLevel)
    {
        std::vector<ItemTemplate const*> candidates;
        for (uint32 level = std::max<uint32>(1, fromLevel); level <= std::min<uint32>(toLevel, MAX_LEVEL_INDEX - 1); ++level)
            for (ItemTemplate const* proto : pool(level))
                candidates.push_back(proto);

        return candidates.empty() ? nullptr : dice.Pick(candidates);
    }

    ItemTemplate const* PickMount(Dice& dice, uint32 level)
    {
        std::vector<ItemTemplate const*> candidates;
        for (ItemTemplate const* proto : Mounts())
            if (MountLevel(proto) <= level + 5)
                candidates.push_back(proto);

        return candidates.empty() ? nullptr : dice.Pick(candidates);
    }

    uint32 LevelBelow(uint32 level, uint32 by)
    {
        return level > by ? level - by : 1;
    }

    class Shelf
    {
    public:
        Shelf(uint32 entry, Trade const& trade) : _entry(entry), _trade(trade) { }

        void Put(ItemTemplate const* proto)
        {
            if (!proto || _trade.items.count(proto->ItemId) || _trade.total + _added.size() >= MAX_VENDOR_LIST)
                return;

            for (uint32 itemId : _added)
                if (itemId == proto->ItemId)
                    return;

            // One in stock; it does not come back before the stock changes
            sObjectMgr->AddVendorItem(_entry, proto->ItemId, 1, config.periodDays * DAY, 0, false);
            _added.push_back(proto->ItemId);
        }

        std::vector<uint32> const& Added() const { return _added; }

    private:
        uint32 _entry;
        Trade const& _trade;
        std::vector<uint32> _added;
    };

    void StockWanderer(Dice& stock, Theme theme, uint32 areaLevel, Shelf& shelf)
    {
        uint32 const level = StockLevel(stock, areaLevel);

        switch (theme)
        {
            case THEME_BEASTMASTER:
                for (uint32 i = stock.Range(4, 6); i > 0 && !Pets().empty(); --i)
                    shelf.Put(stock.Pick(Pets()));
                break;
            case THEME_STABLEHAND:
                for (uint32 i = stock.Range(1, 2); i > 0; --i)
                    shelf.Put(PickMount(stock, std::max<uint32>(level, 20)));
                break;
            case THEME_ALCHEMIST:
                for (uint32 i = stock.Range(5, 8); i > 0; --i)
                    shelf.Put(PickFromLevels(stock, &Consumables, LevelBelow(level, 10), level + 3));
                break;
            case THEME_ARMOURER:
                for (uint32 i = stock.Range(3, 5); i > 0; --i)
                    shelf.Put(PickGear(stock, level, 1, ITEM_CLASS_ARMOR, nullptr));
                break;
            case THEME_WEAPONSMITH:
                for (uint32 i = stock.Range(3, 5); i > 0; --i)
                    shelf.Put(PickGear(stock, level, 1, ITEM_CLASS_WEAPON, nullptr));
                break;
            case THEME_RECIPE_PEDLAR:
                for (uint32 i = stock.Range(4, 6); i > 0; --i)
                    shelf.Put(PickFromLevels(stock, &Recipes, LevelBelow(level, 10), level + 5));
                break;
            case THEME_SMUGGLER:
                // Oddities: things from further up the road, at the price that implies
                shelf.Put(PickGear(stock, level + 5, 2, MAX_ITEM_CLASS, nullptr, 8));
                for (uint32 i = stock.Range(1, 2); i > 0; --i)
                    shelf.Put(PickGear(stock, level + 5, 1, MAX_ITEM_CLASS, nullptr, 8));
                break;
            default:
                break;
        }
    }

    void StockVendor(Sighting const& sighting, uint64 period)
    {
        VendorItemData const* list = sObjectMgr->GetNpcVendorItemList(sighting.entry);
        if (!list || list->Empty())
            return;

        Trade const trade = ReadTrade(list);
        Shelf shelf(sighting.entry, trade);
        Dice stock(config.seed, period, sighting.entry);

        if (sighting.wanderer)
        {
            // The theme belongs to the world: it depends on the seed, not on the week
            Dice world(config.seed, 0, sighting.entry);
            StockWanderer(stock, Theme(world.Range(0, THEME_COUNT - 1)), sighting.areaLevel, shelf);
        }
        else
        {
            // "Normal" vendors: inns, food and drink, stables, and those who sell for honor, tokens or badges
            if (sighting.npcFlags & (UNIT_NPC_FLAG_INNKEEPER | UNIT_NPC_FLAG_VENDOR_FOOD | UNIT_NPC_FLAG_STABLEMASTER))
                return;

            if (trade.foodAndDrink * 2 > trade.total || trade.forCurrency * 2 > trade.total)
                return;

            if (trade.weapons + trade.armor >= 3)
            {
                uint32 const kind = !trade.armor ? ITEM_CLASS_WEAPON : !trade.weapons ? ITEM_CLASS_ARMOR : MAX_ITEM_CLASS;

                if (stock.Chance(config.gearGreenChance))
                    for (uint32 i = stock.Range(1, 2); i > 0; --i)
                        shelf.Put(PickGear(stock, StockLevel(stock, sighting.areaLevel), 0, kind, &trade));

                if (stock.Chance(config.gearBlueChance))
                    shelf.Put(PickGear(stock, StockLevel(stock, sighting.areaLevel), 1, kind, &trade));

                if (stock.Chance(config.gearEpicChance))
                    shelf.Put(PickGear(stock, StockLevel(stock, sighting.areaLevel), 2, kind, &trade));
            }
            else
            {
                if (stock.Chance(config.otherConsumableChance))
                    for (uint32 i = stock.Range(1, 2); i > 0; --i)
                    {
                        uint32 const level = StockLevel(stock, sighting.areaLevel);
                        shelf.Put(PickFromLevels(stock, &Consumables, LevelBelow(level, 10), level + 3));
                    }

                if (stock.Chance(config.otherRecipeChance))
                {
                    uint32 const level = StockLevel(stock, sighting.areaLevel);
                    shelf.Put(PickFromLevels(stock, &Recipes, LevelBelow(level, 10), level + 5));
                }

                if (stock.Chance(config.otherGearChance))
                    shelf.Put(PickGear(stock, StockLevel(stock, sighting.areaLevel), 0, MAX_ITEM_CLASS, nullptr));
            }

            if (stock.Chance(config.mountChance))
                shelf.Put(PickMount(stock, std::max<uint32>(StockLevel(stock, sighting.areaLevel), 20)));

            if (stock.Chance(config.petChance) && !Pets().empty())
                shelf.Put(stock.Pick(Pets()));
        }

        if (!shelf.Added().empty())
            ourItems[sighting.entry] = shelf.Added();

        if (config.debug)
        {
            std::string names;
            for (uint32 itemId : shelf.Added())
                if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId))
                    names += Acore::StringFormat("{}{} ({}g)", names.empty() ? "" : ", ", proto->Name1, proto->BuyPrice / GOLD);

            CreatureTemplate const* vendor = sObjectMgr->GetCreatureTemplate(sighting.entry);
            LOG_INFO("module", "WowPlus vendors: {} <{}>, zone level {}{}: {}", vendor ? vendor->Name : "?", vendor ? vendor->SubName : "",
                sighting.areaLevel, sighting.wanderer ? ", wanderer" : "", names.empty() ? "nothing" : names);
        }
    }

    // A new week: take back what we added, so the vendors can be stocked afresh
    void ClearStock()
    {
        for (auto const& [entry, items] : ourItems)
            for (uint32 itemId : items)
                sObjectMgr->RemoveVendorItem(entry, itemId, false);

        ourItems.clear();

        std::lock_guard<std::mutex> lock(sightingMutex);
        sighted.clear();
        sightings.clear();
    }
}

class WowPlusVendorWorld : public WorldScript
{
public:
    WowPlusVendorWorld() : WorldScript("WowPlusVendorWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadConfig();
    }

    void OnStartup() override
    {
        BuildItemPools();
        if (config.enabled)
            SetPrices();

        stockedPeriod = CurrentPeriod();
    }

    // The world thread, between map updates: the only safe place to change vendor lists
    void OnUpdate(uint32 /*diff*/) override
    {
        if (!config.enabled)
            return;

        uint64 const period = CurrentPeriod();
        if (period != stockedPeriod)
        {
            ClearStock();
            stockedPeriod = period;
        }

        std::vector<Sighting> batch;
        {
            std::lock_guard<std::mutex> lock(sightingMutex);
            batch.swap(sightings);
        }

        for (Sighting const& sighting : batch)
            StockVendor(sighting, period);
    }
};

class WowPlusVendorCreatures : public AllCreatureScript
{
public:
    WowPlusVendorCreatures() : AllCreatureScript("WowPlusVendorCreatures") { }

    // Runs on the map threads whenever a creature comes into view, so it only takes notes
    void OnCreatureAddWorld(Creature* creature) override
    {
        if (!config.enabled || !creature->IsVendor() || !creature->GetOwnerGUID().IsEmpty() || creature->GetEntry() >= OUR_NPC_ENTRIES_FROM)
            return;

        uint32 areaLevel = 0;
        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(creature->GetAreaId()))
        {
            areaLevel = area->area_level > 0 ? uint32(area->area_level) : 0;
            if (!areaLevel && area->zone)
                if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(area->zone))
                    areaLevel = zone->area_level > 0 ? uint32(zone->area_level) : 0;
        }

        if (areaLevel && areaLevel <= STARTING_AREA_LEVEL)
            return;

        bool const wanderer = config.wanderers && creature->GetDefaultMovementType() == WAYPOINT_MOTION_TYPE;

        std::lock_guard<std::mutex> lock(sightingMutex);
        if (sighted.insert(creature->GetEntry()).second)
            sightings.push_back({creature->GetEntry(), areaLevel, uint32(creature->GetNpcFlags()), wanderer});
    }
};

void AddSC_WowPlusVendors()
{
    new WowPlusVendorWorld();
    new WowPlusVendorCreatures();
}
