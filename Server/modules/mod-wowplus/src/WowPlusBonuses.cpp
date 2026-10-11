/*
 * WowPlus bonuses: looted, crafted and quest-reward equipment can come with random bonuses.
 *
 * The list of possible bonuses is the table item_enchantment_random_tiers that comes with
 * mod-random-enchants (five tiers, weakest to strongest). That module picks the tier from
 * the item's colour alone and lets any item roll three bonuses, which made low-level greens
 * far too strong. Here the tier follows the item's level, the colour only shifts it, and the
 * colour limits how many bonuses an item can have. Crafted equipment always gets one.
 *
 * mod-random-enchants itself is switched off in Settings/random_enchants.conf; it stays
 * installed for its table.
 */

#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "WowPlusItems.h"
#include "Tokenize.h"
#include "StringConvert.h"

#include <array>
#include <vector>

namespace
{
    constexpr uint8 TIER_COUNT = 5;
    constexpr uint8 MAX_BONUSES = 3;

    struct Bonus
    {
        uint32 enchantId;
        uint32 itemClass;                                   // ITEM_CLASS_WEAPON, ITEM_CLASS_ARMOR, or MAX_ITEM_CLASS for any
        int32 subClass;                                     // -1: any
    };

    struct WowPlusBonusConfig
    {
        bool enabled = true;
        bool onLoot = true;
        bool onCraft = true;
        bool onQuest = true;
        bool craftedGuaranteed = true;
        bool announce = true;
        std::array<float, MAX_BONUSES> chance = {35.0f, 40.0f, 40.0f};
        std::array<uint32, MAX_ITEM_QUALITY> maxCount{};
        std::array<int32, MAX_ITEM_QUALITY> tierShift{};
        std::vector<uint32> tierItemLevels;                 // item level at which the next tier starts
        std::array<uint32, MAX_BONUSES> slots = {0, 1, 5};
    };

    WowPlusBonusConfig config;
    std::array<std::vector<Bonus>, TIER_COUNT> tiers;       // built at startup, only read afterwards

    std::vector<uint32> ParseNumbers(std::string const& text)
    {
        std::vector<uint32> numbers;
        for (std::string_view token : Acore::Tokenize(text, ' ', false))
            if (Optional<uint32> number = Acore::StringTo<uint32>(token))
                numbers.push_back(*number);

        return numbers;
    }

    void LoadConfig()
    {
        config.enabled = sConfigMgr->GetOption<bool>("WowPlus.Bonus.Enable", true);
        config.onLoot = sConfigMgr->GetOption<bool>("WowPlus.Bonus.OnLoot", true);
        config.onCraft = sConfigMgr->GetOption<bool>("WowPlus.Bonus.OnCraft", true);
        config.onQuest = sConfigMgr->GetOption<bool>("WowPlus.Bonus.OnQuestReward", true);
        config.craftedGuaranteed = sConfigMgr->GetOption<bool>("WowPlus.Bonus.CraftedGuaranteed", true);
        config.announce = sConfigMgr->GetOption<bool>("WowPlus.Bonus.Announce", true);

        config.chance[0] = sConfigMgr->GetOption<float>("WowPlus.Bonus.Chance1", 35.0f);
        config.chance[1] = sConfigMgr->GetOption<float>("WowPlus.Bonus.Chance2", 40.0f);
        config.chance[2] = sConfigMgr->GetOption<float>("WowPlus.Bonus.Chance3", 40.0f);

        config.maxCount.fill(0);
        config.maxCount[ITEM_QUALITY_UNCOMMON] = sConfigMgr->GetOption<uint32>("WowPlus.Bonus.MaxCount.Uncommon", 1);
        config.maxCount[ITEM_QUALITY_RARE] = sConfigMgr->GetOption<uint32>("WowPlus.Bonus.MaxCount.Rare", 2);
        config.maxCount[ITEM_QUALITY_EPIC] = sConfigMgr->GetOption<uint32>("WowPlus.Bonus.MaxCount.Epic", 3);
        config.maxCount[ITEM_QUALITY_LEGENDARY] = sConfigMgr->GetOption<uint32>("WowPlus.Bonus.MaxCount.Legendary", 3);

        config.tierShift.fill(0);
        config.tierShift[ITEM_QUALITY_UNCOMMON] = sConfigMgr->GetOption<int32>("WowPlus.Bonus.TierShift.Uncommon", -1);
        config.tierShift[ITEM_QUALITY_RARE] = sConfigMgr->GetOption<int32>("WowPlus.Bonus.TierShift.Rare", 0);
        config.tierShift[ITEM_QUALITY_EPIC] = sConfigMgr->GetOption<int32>("WowPlus.Bonus.TierShift.Epic", 0);
        config.tierShift[ITEM_QUALITY_LEGENDARY] = sConfigMgr->GetOption<int32>("WowPlus.Bonus.TierShift.Legendary", 1);

        config.tierItemLevels = ParseNumbers(sConfigMgr->GetOption<std::string>("WowPlus.Bonus.TierItemLevels", "30 70 115 200"));

        std::vector<uint32> const slots = ParseNumbers(sConfigMgr->GetOption<std::string>("WowPlus.Bonus.Slots", "0 1 5"));
        for (uint8 i = 0; i < MAX_BONUSES; ++i)
            config.slots[i] = i < slots.size() && slots[i] < MAX_ENCHANTMENT_SLOT ? slots[i] : std::array<uint32, MAX_BONUSES>{0, 1, 5}[i];
    }

    void LoadBonuses()
    {
        for (auto& tier : tiers)
            tier.clear();

        uint32 count = 0;
        QueryResult result = WorldDatabase.Query("SELECT enchantID, tier, class, exclusiveSubClass FROM item_enchantment_random_tiers");
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 const enchantId = fields[0].Get<uint32>();
                uint32 const tier = fields[1].Get<uint32>();
                std::string const itemClass = fields[2].Get<std::string>();

                if (tier < 1 || tier > TIER_COUNT || !sSpellItemEnchantmentStore.LookupEntry(enchantId))
                    continue;

                Bonus bonus;
                bonus.enchantId = enchantId;
                bonus.subClass = fields[3].IsNull() ? -1 : fields[3].Get<int32>();
                if (itemClass == "WEAPON")
                    bonus.itemClass = ITEM_CLASS_WEAPON;
                else if (itemClass == "ARMOR")
                    bonus.itemClass = ITEM_CLASS_ARMOR;
                else if (itemClass == "ANY")
                    bonus.itemClass = MAX_ITEM_CLASS;
                else
                    continue;

                tiers[tier - 1].push_back(bonus);
                ++count;
            } while (result->NextRow());
        }

        if (!count)
            LOG_WARN("server.loading", ">> WowPlus bonuses: the table item_enchantment_random_tiers is empty or missing; no bonuses will be given");
        else
            LOG_INFO("server.loading", ">> WowPlus bonuses: {} possible bonuses in {} tiers", count, TIER_COUNT);
    }

    // Tier 0..4 for an item: its item level decides, its colour shifts it.
    uint8 TierFor(ItemTemplate const* proto)
    {
        int32 tier = 0;
        for (uint32 itemLevel : config.tierItemLevels)
            if (proto->ItemLevel >= itemLevel)
                ++tier;

        tier += config.tierShift[proto->Quality];
        return uint8(std::clamp<int32>(tier, 0, TIER_COUNT - 1));
    }

    uint32 PickBonus(ItemTemplate const* proto)
    {
        std::vector<uint32> candidates;
        for (Bonus const& bonus : tiers[TierFor(proto)])
        {
            if (bonus.itemClass != MAX_ITEM_CLASS && bonus.itemClass != proto->Class)
                continue;

            if (bonus.subClass >= 0 && uint32(bonus.subClass) != proto->SubClass)
                continue;

            candidates.push_back(bonus.enchantId);
        }

        return candidates.empty() ? 0 : candidates[urand(0, candidates.size() - 1)];
    }

    void RollBonuses(Player* player, Item* item, bool crafted)
    {
        if (!config.enabled || !item || !item->IsInWorld())
            return;

        ItemTemplate const* proto = item->GetTemplate();
        if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR)
            return;

        if (proto->Quality >= MAX_ITEM_QUALITY)
            return;

        uint32 const maxCount = std::min<uint32>(config.maxCount[proto->Quality], MAX_BONUSES);
        uint32 applied = 0;

        for (uint32 i = 0; i < maxCount; ++i)
        {
            // Each further bonus is only rolled when the one before succeeded
            bool const guaranteed = crafted && config.craftedGuaranteed && i == 0;
            if (!guaranteed && !roll_chance_f(config.chance[i]))
                break;

            EnchantmentSlot const slot = EnchantmentSlot(config.slots[i]);
            if (item->GetEnchantmentId(slot))
                break;

            uint32 const enchantId = PickBonus(proto);
            if (!enchantId)
                break;

            player->ApplyEnchantment(item, slot, false);
            item->SetEnchantment(slot, enchantId, 0, 0);
            player->ApplyEnchantment(item, slot, true);
            ++applied;
        }

        if (applied && config.announce)
            ChatHandler(player->GetSession()).PSendSysMessage("|cff1eff00{}|r came with {} random {}.", proto->Name1, applied,
                applied == 1 ? "bonus" : "bonuses");
    }
}

class WowPlusBonusWorld : public WorldScript
{
public:
    WowPlusBonusWorld() : WorldScript("WowPlusBonusWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadConfig();
    }

    void OnStartup() override
    {
        LoadBonuses();
    }
};

class WowPlusBonusPlayer : public PlayerScript
{
public:
    WowPlusBonusPlayer() : PlayerScript("WowPlusBonusPlayer") { }

    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override
    {
        // Another module may have sold the item already (grey loot)
        if (config.onLoot && WowPlus::StillCarried(player, item))
            RollBonuses(player, item, false);
    }

    void OnPlayerGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/) override
    {
        if (config.onLoot)
            RollBonuses(player, item, false);
    }

    void OnPlayerCreateItem(Player* player, Item* item, uint32 /*count*/) override
    {
        if (config.onCraft)
            RollBonuses(player, item, true);
    }

    void OnPlayerQuestRewardItem(Player* player, Item* item, uint32 /*count*/) override
    {
        if (config.onQuest)
            RollBonuses(player, item, false);
    }
};

void AddSC_WowPlusBonuses()
{
    new WowPlusBonusWorld();
    new WowPlusBonusPlayer();
}
