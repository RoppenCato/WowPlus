/*
 * WowPlus item pools. See WowPlusItems.h.
 *
 * Left out of every pool: items whose name marks them as unused, temporary and conjured
 * items, and items that nothing in the game hands out (never dropped, sold, crafted or given
 * by a quest), which removes leftovers and test items. Equipment additionally leaves out
 * quest starters and PvP sets.
 */

#include "WowPlusItems.h"

#include "DatabaseEnv.h"
#include "Field.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "QueryResult.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <array>
#include <unordered_set>

namespace
{
    using namespace WowPlus;

    bool built = false;

    std::array<std::array<ItemList, QUALITY_COUNT>, MAX_LEVEL_INDEX> equipment;
    std::array<ItemList, MAX_LEVEL_INDEX> consumables;
    std::array<ItemList, MAX_LEVEL_INDEX> recipes;
    ItemList mounts;
    ItemList pets;
    ItemList const empty;

    bool NameLooksUnused(std::string const& name)
    {
        static std::array<char const*, 14> const markers =
        {
            "Test", "TEST", "Deprecated", "DEPRECATED", "DEP ", "OLD", "(old", "Monster -", "[PH]", "PH ",
            "Unused", "UNUSED", "QA", "NPC Equip"
        };

        for (char const* marker : markers)
            if (name.find(marker) != std::string::npos)
                return true;

        return false;
    }

    bool NameLooksPvP(std::string const& name)
    {
        static std::array<char const*, 20> const markers =
        {
            "Gladiator's", "Grand Marshal's", "High Warlord's", "Field Marshal's", "Warlord's", "Marshal's",
            "General's", "Lieutenant Commander's", "Champion's", "Knight-", "Legionnaire's", "Blood Guard's",
            "Sergeant", "Vindicator's", "Guardian's", "Veteran's", "Medallion of the", "Insignia of the",
            "Battlemaster's", "Titan-Forged"
        };

        for (char const* marker : markers)
            if (name.find(marker) != std::string::npos)
                return true;

        return false;
    }

    // Every item the game can really hand out: dropped, sold, a quest reward, or crafted.
    std::unordered_set<uint32> LoadObtainableItems()
    {
        std::unordered_set<uint32> obtainable;

        static std::array<char const*, 7> const queries =
        {
            "SELECT DISTINCT Item FROM creature_loot_template WHERE Reference = 0",
            "SELECT DISTINCT Item FROM reference_loot_template WHERE Reference = 0",
            "SELECT DISTINCT Item FROM gameobject_loot_template WHERE Reference = 0",
            "SELECT DISTINCT Item FROM item_loot_template WHERE Reference = 0",
            "SELECT DISTINCT item FROM npc_vendor WHERE item > 0",
            "SELECT RewardItem1 FROM quest_template UNION SELECT RewardItem2 FROM quest_template "
            "UNION SELECT RewardItem3 FROM quest_template UNION SELECT RewardItem4 FROM quest_template",
            "SELECT RewardChoiceItemID1 FROM quest_template UNION SELECT RewardChoiceItemID2 FROM quest_template "
            "UNION SELECT RewardChoiceItemID3 FROM quest_template UNION SELECT RewardChoiceItemID4 FROM quest_template "
            "UNION SELECT RewardChoiceItemID5 FROM quest_template UNION SELECT RewardChoiceItemID6 FROM quest_template"
        };

        for (char const* query : queries)
            if (QueryResult result = WorldDatabase.Query(query))
                do
                    obtainable.insert((*result)[0].Get<uint32>());
                while (result->NextRow());

        // Crafted items: anything a spell creates
        for (uint32 spellId = 0; spellId < sSpellMgr->GetSpellInfoStoreSize(); ++spellId)
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
                for (SpellEffectInfo const& effect : spellInfo->GetEffects())
                    if (effect.Effect == SPELL_EFFECT_CREATE_ITEM && effect.ItemType)
                        obtainable.insert(effect.ItemType);

        obtainable.erase(0);
        return obtainable;
    }

    // A recipe asks for a profession skill, not a level. Skill 300 is reached around level 60,
    // 375 around 70, 450 at 80.
    uint32 RecipeLevelFor(ItemTemplate const& proto)
    {
        uint32 const rank = proto.RequiredSkillRank;
        if (rank <= 300)
            return std::max<uint32>(5, rank / 5);
        if (rank <= 375)
            return 60 + (rank - 300) * 10 / 75;

        return std::min<uint32>(80, 70 + (rank - 375) * 10 / 75);
    }
}

void WowPlus::BuildItemPools()
{
    if (built)
        return;

    built = true;

    std::unordered_set<uint32> const obtainable = LoadObtainableItems();
    uint32 equipmentCount = 0;
    uint32 consumableCount = 0;
    uint32 recipeCount = 0;

    for (auto const& [entry, proto] : *sObjectMgr->GetItemTemplateStore())
    {
        if (NameLooksUnused(proto.Name1) || proto.Duration || proto.HasFlag(ITEM_FLAG_CONJURED))
            continue;

        if (proto.Class == ITEM_CLASS_MISC && proto.SubClass == ITEM_SUBCLASS_JUNK_MOUNT)
        {
            mounts.push_back(&proto);
            continue;
        }

        if (proto.Class == ITEM_CLASS_MISC && proto.SubClass == ITEM_SUBCLASS_JUNK_PET)
        {
            pets.push_back(&proto);
            continue;
        }

        if (!obtainable.count(entry))
            continue;

        if (proto.Class == ITEM_CLASS_CONSUMABLE)
        {
            if ((proto.SubClass == ITEM_SUBCLASS_POTION || proto.SubClass == ITEM_SUBCLASS_ELIXIR || proto.SubClass == ITEM_SUBCLASS_FLASK) &&
                proto.Quality >= ITEM_QUALITY_NORMAL && proto.RequiredLevel && proto.RequiredLevel < MAX_LEVEL_INDEX)
            {
                consumables[proto.RequiredLevel].push_back(&proto);
                ++consumableCount;
            }
            continue;
        }

        if (proto.Class == ITEM_CLASS_RECIPE)
        {
            if (proto.Quality >= ITEM_QUALITY_NORMAL && proto.RequiredSkill && proto.RequiredSkillRank)
            {
                recipes[RecipeLevelFor(proto)].push_back(&proto);
                ++recipeCount;
            }
            continue;
        }

        if (proto.Class != ITEM_CLASS_WEAPON && proto.Class != ITEM_CLASS_ARMOR)
            continue;

        if (proto.InventoryType == INVTYPE_NON_EQUIP || proto.InventoryType == INVTYPE_BAG ||
            proto.InventoryType == INVTYPE_TABARD || proto.InventoryType == INVTYPE_BODY)
            continue;

        if (proto.Quality < ITEM_QUALITY_UNCOMMON || proto.Quality > ITEM_QUALITY_LEGENDARY)
            continue;

        if (!proto.RequiredLevel || proto.RequiredLevel >= MAX_LEVEL_INDEX)
            continue;

        if (proto.StartQuest || NameLooksPvP(proto.Name1))
            continue;

        equipment[proto.RequiredLevel][proto.Quality - ITEM_QUALITY_UNCOMMON].push_back(&proto);
        ++equipmentCount;
    }

    LOG_INFO("server.loading", ">> WowPlus items: {} equipment, {} potions and elixirs, {} recipes, {} mounts, {} companion pets in the pools",
        equipmentCount, consumableCount, recipeCount, mounts.size(), pets.size());
}

WowPlus::ItemList const& WowPlus::Equipment(uint32 requiredLevel, uint8 quality)
{
    return requiredLevel < MAX_LEVEL_INDEX && quality < QUALITY_COUNT ? equipment[requiredLevel][quality] : empty;
}

WowPlus::ItemList const& WowPlus::Consumables(uint32 requiredLevel)
{
    return requiredLevel < MAX_LEVEL_INDEX ? consumables[requiredLevel] : empty;
}

WowPlus::ItemList const& WowPlus::Recipes(uint32 level)
{
    return level < MAX_LEVEL_INDEX ? recipes[level] : empty;
}

WowPlus::ItemList const& WowPlus::Mounts()
{
    return mounts;
}

WowPlus::ItemList const& WowPlus::Pets()
{
    return pets;
}

// Mounts teach riding ranks at 20, 40, 60 and 70.
uint32 WowPlus::MountLevel(ItemTemplate const* proto)
{
    if (proto->RequiredLevel)
        return proto->RequiredLevel;

    if (proto->RequiredSkillRank >= 300)
        return 70;
    if (proto->RequiredSkillRank >= 225)
        return 60;
    if (proto->RequiredSkillRank >= 150)
        return 40;

    return 20;
}
