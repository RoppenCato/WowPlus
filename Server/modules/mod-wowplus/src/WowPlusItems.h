/*
 * WowPlus item pools: the lists of items our features hand out, built once at startup.
 * Shared by the extra loot (WowPlusLoot.cpp) and the surprise stock of vendors (WowPlusVendors.cpp).
 */

#ifndef WOWPLUS_ITEMS_H
#define WOWPLUS_ITEMS_H

#include "Define.h"

#include <string>
#include <vector>

class Item;
class Player;
struct ItemTemplate;

namespace WowPlus
{
    constexpr uint8 MAX_LEVEL_INDEX = 84;                   // required levels 0..83
    constexpr uint8 QUALITY_COUNT = 4;                      // 0 uncommon, 1 rare, 2 epic, 3 legendary

    using ItemList = std::vector<ItemTemplate const*>;

    // Builds the pools. Safe to call more than once; only the first call does the work.
    // Call it from OnStartup, never later: the pools are read without locks afterwards.
    void BuildItemPools();

    // Weapons and armour that need exactly this level, of one rarity.
    ItemList const& Equipment(uint32 requiredLevel, uint8 quality);

    // Potions, elixirs and flasks that need exactly this level.
    ItemList const& Consumables(uint32 requiredLevel);

    // Profession recipes, by the character level at which their skill requirement is usually reached.
    ItemList const& Recipes(uint32 level);

    ItemList const& Mounts();
    ItemList const& Pets();

    // The level from which a mount can be ridden. Some mounts carry no level of their own.
    uint32 MountLevel(ItemTemplate const* proto);

    // Is this item still in the player's bags, bank or on them? An item handed to a script hook can already have
    // been destroyed by another module's hook that ran first (mod-junk-to-gold sells grey loot that way), and
    // touching it then crashes the server. This looks for the pointer without touching the item.
    bool StillCarried(Player* player, Item const* item);

    // A true sentence about one vendor's surprise stock this week, for bots to pass on (WowPlusVendors.cpp).
    // Empty when there is none. World thread only.
    std::string VendorRumour();
}

#endif
