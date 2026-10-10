/*
 * WowPlus loot: every kill and every treasure chest has an extra chance at equipment
 * for the player's level, and separate small chances at a mount and at a companion pet.
 *
 * The game's own loot tables are not touched. After the server has built the normal loot
 * for a creature or a chest, this adds its own rolls on top. All numbers are settings in
 * mod_wowplus.conf; the design is described in Settings/Docs/Plan.md, section 2.
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Formulas.h"
#include "GameObject.h"
#include "Group.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "LootMgr.h"
#include "Mail.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "WorldSession.h"

#include <array>
#include <unordered_set>
#include <vector>

namespace
{
    constexpr uint8 MAX_LEVEL_INDEX = 84;                   // required levels 0..83
    constexpr uint8 QUALITY_COUNT = 4;                      // uncommon, rare, epic, legendary
    constexpr std::array<uint8, 3> LEVEL_CAPS = {60, 70, 80};

    enum EnemyRank : uint8
    {
        RANK_NORMAL,
        RANK_ELITE,
        RANK_RARE,
        RANK_BOSS,
        RANK_CHEST,
        RANK_COUNT
    };

    struct WowPlusLootConfig
    {
        bool enabled = false;
        std::array<float, RANK_COUNT> chance{};             // percent, per kill or chest
        std::array<uint32, RANK_COUNT> rarityRolls{};       // rarity is rolled this often, best one wins
        std::array<float, QUALITY_COUNT> rarityWeight{};
        uint32 levelWindow = 3;                             // items requiring up to this many levels above the player
        float usableBias = 70.0f;                           // percent of drops picked from what the class can use
        float lookAheadMaxChance = 25.0f;                   // percent of drops taken from the next cap, at one level below it
        uint32 lookAheadLevels = 10;
        std::array<uint32, 3> capFloor{};                   // lowest item level of the gear at 60, 70, 80
        uint32 capStep = 12;                                // how far above the player's gear a cap drop may be
        uint32 capBelow = 10;                               // how far below
        float capJumpChance = 5.0f;                         // percent of cap drops that may reach much higher
        float capJumpFactor = 2.5f;
        float mountChance = 0.1f;                           // percent, flat
        float petChance = 0.2f;                             // percent, flat
        bool skipKnownCollectibles = true;
    };

    WowPlusLootConfig config;

    // Equipment by required level and rarity. Built once at startup, only read afterwards.
    std::array<std::array<std::vector<ItemTemplate const*>, QUALITY_COUNT>, MAX_LEVEL_INDEX> equipment;
    std::vector<ItemTemplate const*> mounts;
    std::vector<ItemTemplate const*> pets;

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
    // Items outside this set are leftovers that were never obtainable.
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

    void BuildPools()
    {
        for (auto& byQuality : equipment)
            for (auto& pool : byQuality)
                pool.clear();
        mounts.clear();
        pets.clear();

        std::unordered_set<uint32> const obtainable = LoadObtainableItems();
        uint32 equipmentCount = 0;

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

            if (proto.Class != ITEM_CLASS_WEAPON && proto.Class != ITEM_CLASS_ARMOR)
                continue;

            if (proto.InventoryType == INVTYPE_NON_EQUIP || proto.InventoryType == INVTYPE_BAG ||
                proto.InventoryType == INVTYPE_TABARD || proto.InventoryType == INVTYPE_BODY)
                continue;

            if (proto.Quality < ITEM_QUALITY_UNCOMMON || proto.Quality > ITEM_QUALITY_LEGENDARY)
                continue;

            if (!proto.RequiredLevel || proto.RequiredLevel >= MAX_LEVEL_INDEX)
                continue;

            if (proto.StartQuest || NameLooksPvP(proto.Name1) || !obtainable.count(entry))
                continue;

            equipment[proto.RequiredLevel][proto.Quality - ITEM_QUALITY_UNCOMMON].push_back(&proto);
            ++equipmentCount;
        }

        LOG_INFO("server.loading", ">> WowPlus loot: {} equipment items, {} mounts, {} companion pets in the pools",
            equipmentCount, mounts.size(), pets.size());
    }

    void LoadConfig()
    {
        config.enabled = sConfigMgr->GetOption<bool>("WowPlus.Loot.Enable", true);

        config.chance[RANK_NORMAL] = sConfigMgr->GetOption<float>("WowPlus.Loot.Chance.Normal", 6.0f);
        config.chance[RANK_ELITE] = sConfigMgr->GetOption<float>("WowPlus.Loot.Chance.Elite", 20.0f);
        config.chance[RANK_RARE] = sConfigMgr->GetOption<float>("WowPlus.Loot.Chance.Rare", 60.0f);
        config.chance[RANK_BOSS] = sConfigMgr->GetOption<float>("WowPlus.Loot.Chance.Boss", 100.0f);
        config.chance[RANK_CHEST] = sConfigMgr->GetOption<float>("WowPlus.Loot.Chance.Chest", 35.0f);

        config.rarityRolls[RANK_NORMAL] = sConfigMgr->GetOption<uint32>("WowPlus.Loot.RarityRolls.Normal", 1);
        config.rarityRolls[RANK_ELITE] = sConfigMgr->GetOption<uint32>("WowPlus.Loot.RarityRolls.Elite", 2);
        config.rarityRolls[RANK_RARE] = sConfigMgr->GetOption<uint32>("WowPlus.Loot.RarityRolls.Rare", 3);
        config.rarityRolls[RANK_BOSS] = sConfigMgr->GetOption<uint32>("WowPlus.Loot.RarityRolls.Boss", 4);
        config.rarityRolls[RANK_CHEST] = sConfigMgr->GetOption<uint32>("WowPlus.Loot.RarityRolls.Chest", 2);

        config.rarityWeight[0] = sConfigMgr->GetOption<float>("WowPlus.Loot.Weight.Uncommon", 92.0f);
        config.rarityWeight[1] = sConfigMgr->GetOption<float>("WowPlus.Loot.Weight.Rare", 6.0f);
        config.rarityWeight[2] = sConfigMgr->GetOption<float>("WowPlus.Loot.Weight.Epic", 1.9f);
        config.rarityWeight[3] = sConfigMgr->GetOption<float>("WowPlus.Loot.Weight.Legendary", 0.1f);

        config.levelWindow = sConfigMgr->GetOption<uint32>("WowPlus.Loot.LevelWindow", 3);
        config.usableBias = sConfigMgr->GetOption<float>("WowPlus.Loot.UsableBias", 70.0f);

        config.lookAheadMaxChance = sConfigMgr->GetOption<float>("WowPlus.Loot.LookAhead.MaxChance", 25.0f);
        config.lookAheadLevels = std::max<uint32>(1, sConfigMgr->GetOption<uint32>("WowPlus.Loot.LookAhead.Levels", 10));

        config.capFloor[0] = sConfigMgr->GetOption<uint32>("WowPlus.Loot.Cap.Floor.60", 58);
        config.capFloor[1] = sConfigMgr->GetOption<uint32>("WowPlus.Loot.Cap.Floor.70", 105);
        config.capFloor[2] = sConfigMgr->GetOption<uint32>("WowPlus.Loot.Cap.Floor.80", 174);
        config.capStep = sConfigMgr->GetOption<uint32>("WowPlus.Loot.Cap.Step", 12);
        config.capBelow = sConfigMgr->GetOption<uint32>("WowPlus.Loot.Cap.Below", 10);
        config.capJumpChance = sConfigMgr->GetOption<float>("WowPlus.Loot.Cap.JumpChance", 5.0f);
        config.capJumpFactor = sConfigMgr->GetOption<float>("WowPlus.Loot.Cap.JumpFactor", 2.5f);

        config.mountChance = sConfigMgr->GetOption<float>("WowPlus.Loot.Mount.Chance", 0.1f);
        config.petChance = sConfigMgr->GetOption<float>("WowPlus.Loot.Pet.Chance", 0.2f);
        config.skipKnownCollectibles = sConfigMgr->GetOption<bool>("WowPlus.Loot.Collectibles.SkipKnown", true);
    }

    bool IsBot(Player const* player)
    {
        return player->GetSession()->IsHeadless();
    }

    // The human the extra drops belong to: the looter, or a human in the looter's group nearby.
    Player* FindRealPlayer(Player* lootOwner, WorldObject const* source)
    {
        if (!IsBot(lootOwner))
            return lootOwner;

        if (Group* group = lootOwner->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                if (Player* member = itr->GetSource())
                    if (!IsBot(member) && member->IsAtLootRewardDistance(source))
                        return member;

        return nullptr;
    }

    int8 CapIndex(uint32 level)
    {
        for (uint8 i = 0; i < LEVEL_CAPS.size(); ++i)
            if (LEVEL_CAPS[i] == level)
                return int8(i);

        return -1;
    }

    // The armour a class is meant to wear at this level.
    uint32 BestArmorSubclass(uint8 playerClass, uint8 level)
    {
        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
                return level >= 40 ? ITEM_SUBCLASS_ARMOR_PLATE : ITEM_SUBCLASS_ARMOR_MAIL;
            case CLASS_DEATH_KNIGHT:
                return ITEM_SUBCLASS_ARMOR_PLATE;
            case CLASS_HUNTER:
            case CLASS_SHAMAN:
                return level >= 40 ? ITEM_SUBCLASS_ARMOR_MAIL : ITEM_SUBCLASS_ARMOR_LEATHER;
            case CLASS_ROGUE:
            case CLASS_DRUID:
                return ITEM_SUBCLASS_ARMOR_LEATHER;
            default:
                return ITEM_SUBCLASS_ARMOR_CLOTH;
        }
    }

    bool AllowedByClassAndRace(Player const* player, ItemTemplate const* proto)
    {
        if (!(proto->AllowableClass & player->getClassMask()) || !(proto->AllowableRace & player->getRaceMask()))
            return false;

        if (proto->HasFlag2(ITEM_FLAG2_FACTION_HORDE) && player->GetTeamId() != TEAM_HORDE)
            return false;

        if (proto->HasFlag2(ITEM_FLAG2_FACTION_ALLIANCE) && player->GetTeamId() != TEAM_ALLIANCE)
            return false;

        return true;
    }

    bool SuitsClass(Player const* player, ItemTemplate const* proto, uint32 level)
    {
        if (!AllowedByClassAndRace(player, proto))
            return false;

        if (uint32 skill = proto->GetSkill())
            if (!player->GetSkillValue(skill))
                return false;

        if (proto->Class != ITEM_CLASS_ARMOR || proto->InventoryType == INVTYPE_CLOAK)
            return true;

        switch (proto->SubClass)
        {
            case ITEM_SUBCLASS_ARMOR_CLOTH:
            case ITEM_SUBCLASS_ARMOR_LEATHER:
            case ITEM_SUBCLASS_ARMOR_MAIL:
            case ITEM_SUBCLASS_ARMOR_PLATE:
                return proto->SubClass == BestArmorSubclass(player->getClass(), uint8(level));
            default:
                return true;                                // rings, necks, trinkets, shields, relics
        }
    }

    uint8 RollRarity(EnemyRank rank)
    {
        float total = 0.0f;
        for (float weight : config.rarityWeight)
            total += weight;

        uint8 best = 0;
        if (total <= 0.0f)
            return best;

        for (uint32 roll = 0; roll < std::max<uint32>(1, config.rarityRolls[rank]); ++roll)
        {
            float pick = frand(0.0f, total);
            uint8 rolled = 0;
            for (uint8 i = 0; i < QUALITY_COUNT; ++i)
            {
                if (pick < config.rarityWeight[i])
                {
                    rolled = i;
                    break;
                }
                pick -= config.rarityWeight[i];
            }
            best = std::max(best, rolled);
        }

        return best;
    }

    // The equipment that may drop for this player, for one rarity.
    void CollectEquipment(Player* player, uint8 rarity, bool lookAhead, std::vector<ItemTemplate const*>& out)
    {
        uint32 const level = player->GetLevel();
        uint32 fromLevel = level;
        uint32 toLevel = std::min<uint32>(level + config.levelWindow, MAX_LEVEL_INDEX - 1);

        if (lookAhead)
        {
            for (uint8 cap : LEVEL_CAPS)
                if (cap > level)
                {
                    fromLevel = toLevel = cap;
                    break;
                }
        }

        bool const jump = roll_chance_f(config.capJumpChance);

        for (uint32 requiredLevel = fromLevel; requiredLevel <= toLevel; ++requiredLevel)
        {
            int8 const cap = CapIndex(requiredLevel);

            // Gear for 60, 70 and 80 all has the same required level and differs by item level,
            // so there the drop follows what the player is wearing.
            uint32 minItemLevel = 0;
            uint32 maxItemLevel = std::numeric_limits<uint32>::max();
            if (cap >= 0)
            {
                uint32 base = config.capFloor[cap];
                if (level >= requiredLevel)
                {
                    base = std::max<uint32>(base, uint32(player->GetAverageItemLevel()));
                    minItemLevel = base > config.capBelow ? base - config.capBelow : 0;
                }
                maxItemLevel = base + uint32(float(config.capStep) * (jump ? config.capJumpFactor : 1.0f));
            }

            for (ItemTemplate const* proto : equipment[requiredLevel][rarity])
            {
                // Legendaries are too few to band by item level
                if (rarity < 3 && (proto->ItemLevel < minItemLevel || proto->ItemLevel > maxItemLevel))
                    continue;

                out.push_back(proto);
            }
        }
    }

    ItemTemplate const* PickEquipment(Player* player, EnemyRank rank)
    {
        uint32 const level = player->GetLevel();

        // Rising chance to see the next cap's gear early
        bool lookAhead = false;
        for (uint8 cap : LEVEL_CAPS)
            if (level < cap && level + config.lookAheadLevels >= cap)
            {
                float const progress = float(level + config.lookAheadLevels - cap + 1) / float(config.lookAheadLevels);
                lookAhead = roll_chance_f(config.lookAheadMaxChance * progress * progress);
                break;
            }

        std::vector<ItemTemplate const*> candidates;
        for (int8 rarity = int8(RollRarity(rank)); rarity >= 0 && candidates.empty(); --rarity)
            CollectEquipment(player, uint8(rarity), lookAhead, candidates);

        if (candidates.empty())
            return nullptr;

        if (roll_chance_f(config.usableBias))
        {
            std::vector<ItemTemplate const*> usable;
            for (ItemTemplate const* proto : candidates)
                if (SuitsClass(player, proto, std::max<uint32>(level, proto->RequiredLevel)))
                    usable.push_back(proto);

            if (!usable.empty())
                return usable[urand(0, usable.size() - 1)];
        }

        return candidates[urand(0, candidates.size() - 1)];
    }

    bool AlreadyKnown(Player const* player, ItemTemplate const* proto)
    {
        for (_Spell const& spell : proto->Spells)
            if (spell.SpellId > 0 && player->HasSpell(uint32(spell.SpellId)))
                return true;

        return false;
    }

    // Mounts teach riding ranks at 20, 40, 60 and 70; some have no level of their own.
    uint32 MountLevel(ItemTemplate const* proto)
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

    ItemTemplate const* PickCollectible(Player* player, std::vector<ItemTemplate const*> const& pool, bool isMount)
    {
        std::vector<ItemTemplate const*> candidates;
        for (ItemTemplate const* proto : pool)
        {
            if (!AllowedByClassAndRace(player, proto))
                continue;

            if (isMount ? MountLevel(proto) > player->GetLevel() : proto->RequiredLevel > player->GetLevel())
                continue;

            if (config.skipKnownCollectibles && AlreadyKnown(player, proto))
                continue;

            candidates.push_back(proto);
        }

        return candidates.empty() ? nullptr : candidates[urand(0, candidates.size() - 1)];
    }

    // Alone, the item lies in the loot like any other. In a group it goes straight to the
    // human player, so bots have nothing to roll on.
    void GiveItem(Player* player, Loot* loot, ItemTemplate const* proto)
    {
        if (!player->GetGroup())
        {
            loot->AddItem(LootStoreItem(proto->ItemId, 0, 100.0f, false, LOOT_MODE_DEFAULT, 0, 1, 1));
            return;
        }

        ItemPosCountVec dest;
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, proto->ItemId, 1) == EQUIP_ERR_OK)
        {
            if (Item* item = player->StoreNewItem(dest, proto->ItemId, true, Item::GenerateItemRandomPropertyId(proto->ItemId)))
            {
                player->SendNewItem(item, 1, true, false);
                // Tell other modules it was looted, so that for example random bonuses apply to it too
                sScriptMgr->OnPlayerLootItem(player, item, 1, loot->sourceWorldObjectGUID);
            }
            return;
        }

        // Bags are full: send it by mail
        if (Item* item = Item::CreateItem(proto->ItemId, 1, player))
        {
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            item->SaveToDB(trans);
            MailDraft draft("Your bags were full", "You found this, but had no room to carry it.");
            draft.AddItem(item);
            draft.SendMailTo(trans, MailReceiver(player), MailSender(MAIL_NORMAL, player->GetGUID().GetCounter(), MAIL_STATIONERY_GM));
            CharacterDatabase.CommitTransaction(trans);
            ChatHandler(player->GetSession()).PSendSysMessage("Your bags are full. {} was sent to your mailbox.", proto->Name1);
        }
    }

    bool IsGrey(uint32 playerLevel, uint32 sourceLevel)
    {
        return sourceLevel <= Acore::XP::GetGrayLevel(uint8(playerLevel));
    }

    bool IsTreasureChest(GameObject const* go, Loot const* loot)
    {
        GameObjectTemplate const* info = go->GetGOInfo();
        if (info->type != GAMEOBJECT_TYPE_CHEST || loot->items.empty())
            return false;

        // Herbs, ore veins and fishing pools are "chests" to the game as well
        if (LockEntry const* lock = sLockStore.LookupEntry(info->GetLockId()))
            for (uint8 i = 0; i < MAX_LOCK_CASE; ++i)
                if (lock->Type[i] == LOCK_KEY_SKILL &&
                    (lock->Index[i] == LOCKTYPE_HERBALISM || lock->Index[i] == LOCKTYPE_MINING || lock->Index[i] == LOCKTYPE_FISHING))
                    return false;

        return true;
    }
}

class WowPlusLootWorld : public WorldScript
{
public:
    WowPlusLootWorld() : WorldScript("WowPlusLootWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadConfig();
    }

    void OnStartup() override
    {
        BuildPools();
    }
};

class WowPlusLootMisc : public MiscScript
{
public:
    WowPlusLootMisc() : MiscScript("WowPlusLootMisc", {MISCHOOK_ON_AFTER_LOOT_TEMPLATE_PROCESS}) { }

    void OnAfterLootTemplateProcess(Loot* loot, LootTemplate const* /*tab*/, LootStore const& store, Player* lootOwner,
        bool /*personal*/, bool /*noEmptyError*/, uint16 /*lootMode*/) override
    {
        if (!config.enabled || !lootOwner)
            return;

        EnemyRank rank = RANK_NORMAL;
        WorldObject const* source = nullptr;
        uint32 sourceLevel = 0;                             // 0: no level to compare with

        if (&store == &LootTemplates_Creature)
        {
            Creature* creature = ObjectAccessor::GetCreature(*lootOwner, loot->sourceWorldObjectGUID);
            if (!creature || creature->IsPet() || creature->IsCritter())
                return;

            source = creature;
            sourceLevel = creature->GetLevel();

            if (creature->IsDungeonBoss() || creature->isWorldBoss())
                rank = RANK_BOSS;
            else if (creature->GetCreatureTemplate()->rank == CREATURE_ELITE_RARE ||
                creature->GetCreatureTemplate()->rank == CREATURE_ELITE_RAREELITE)
                rank = RANK_RARE;
            else if (creature->isElite())
                rank = RANK_ELITE;
        }
        else if (&store == &LootTemplates_Gameobject)
        {
            GameObject const* go = loot->sourceGameObject;
            if (!go || !IsTreasureChest(go, loot))
                return;

            source = go;
            rank = RANK_CHEST;

            // A chest has no level, so the area's level stands in. Dungeons have none and always count.
            if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(go->GetAreaId()))
                if (area->area_level > 0)
                    sourceLevel = uint32(area->area_level);
        }
        else
            return;

        Player* player = FindRealPlayer(lootOwner, source);
        if (!player)
            return;

        // Nothing from enemies and areas that are far below the player
        if (sourceLevel && IsGrey(player->GetLevel(), sourceLevel))
            return;

        if (roll_chance_f(config.chance[rank]))
            if (ItemTemplate const* proto = PickEquipment(player, rank))
                GiveItem(player, loot, proto);

        if (roll_chance_f(config.mountChance))
            if (ItemTemplate const* proto = PickCollectible(player, mounts, true))
                GiveItem(player, loot, proto);

        if (roll_chance_f(config.petChance))
            if (ItemTemplate const* proto = PickCollectible(player, pets, false))
                GiveItem(player, loot, proto);
    }
};

void AddSC_WowPlusLoot()
{
    new WowPlusLootWorld();
    new WowPlusLootMisc();
}
