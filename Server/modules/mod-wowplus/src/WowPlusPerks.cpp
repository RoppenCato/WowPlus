/*
 * WowPlus: lasting perks that some of our quests give when they are handed in.
 *
 * Which quest gives which perk is in the table wowplus_quest_perks, made by Settings/Scripts/Make-Quests.py from the quest
 * files ("perk": "..."). A player has a perk for as long as the quest counts as done, so nothing more is stored.
 *
 *   pet_fed      a hunter's pet loses happiness half as fast
 *   stable_slot  one stable slot, free, when the quest is handed in
 *   skinning     skinning a beast sometimes gives one more of what it gave
 *   rage_hold    rage drains half as fast out of combat
 *   second_wind  health comes back faster out of combat
 *   repairs      repairs cost a quarter less
 *   pockets      picking a pocket brings in twice the coin
 *   fence        a tenth more coin from everything looted
 *   mana_rest    mana comes back faster out of combat
 *   polymorph_pig   teaches the pig shape of Polymorph when the quest is handed in
 *   double_conjure  conjuring food or water makes twice as much
 *   wild_rest    health and mana both come back faster out of combat
 *   herbs        gathering a herb sometimes gives one more
 *   shard_luck   a kill worth experience sometimes leaves a soul shard without draining for it
 *   demon_mend   a warlock's demon mends itself out of combat
 */

#include "Chat.h"
#include "Creature.h"
#include "DBCStores.h"
#include "Formulas.h"
#include "GameObject.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "LootMgr.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "PetDefines.h"
#include "Player.h"
#include "QuestDef.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr uint32 HAPPINESS_TICK = 7500;                 // ms, the same beat at which a pet loses happiness
    constexpr int32 HAPPINESS_BACK = 335;                   // half of what a resting pet loses on each beat
    constexpr uint32 EXTRA_SKIN_CHANCE = 40;                // percent
    constexpr uint32 REST_TICK = 2000;                      // ms, the beat at which rage drains out of combat
    constexpr int32 RAGE_BACK = 10;                         // half of the 2 rage (20 in the game's units) lost on each beat
    constexpr float WIND_SHARE = 0.01f;                     // of full health, back on each beat out of combat
    constexpr float REPAIR_SHARE = 0.75f;                   // of the usual price
    constexpr float MANA_SHARE = 0.01f;                     // of full mana, back on each beat out of combat
    constexpr uint32 SPELL_POLYMORPH_PIG = 28272;
    constexpr uint32 ITEM_SOUL_SHARD = 6265;
    constexpr uint32 SHARD_CHANCE = 20;                     // percent
    constexpr float MEND_SHARE = 0.02f;                     // of the demon's full health, back on each beat out of combat

    std::unordered_map<std::string, std::vector<uint32>> questsWithPerk;    // perk -> the quests that give it
    std::unordered_map<uint32, std::string> perkOfQuest;
    uint32 sinceHappiness = 0;
    uint32 sinceRest = 0;

    uint32 LoadPerks()
    {
        questsWithPerk.clear();
        perkOfQuest.clear();
        if (!WorldDatabase.Query("SHOW TABLES LIKE 'wowplus_quest_perks'"))
            return 0;

        uint32 count = 0;
        if (QueryResult result = WorldDatabase.Query("SELECT quest, perk FROM wowplus_quest_perks"))
            do
            {
                Field* fields = result->Fetch();
                questsWithPerk[fields[1].Get<std::string>()].push_back(fields[0].Get<uint32>());
                perkOfQuest[fields[0].Get<uint32>()] = fields[1].Get<std::string>();
                ++count;
            } while (result->NextRow());

        return count;
    }

    bool HasPerk(Player* player, std::string const& perk)
    {
        auto found = questsWithPerk.find(perk);
        if (found == questsWithPerk.end())
            return false;

        for (uint32 quest : found->second)
            if (player->GetQuestRewardStatus(quest))
                return true;

        return false;
    }
}

class WowPlusPerksWorld : public WorldScript
{
public:
    WowPlusPerksWorld() : WorldScript("WowPlusPerksWorld") { }

    void OnStartup() override
    {
        LOG_INFO("server.loading", ">> WowPlus perks: {} quests give a lasting perk", LoadPerks());
    }

    void Rest(uint32 diff);

    void OnUpdate(uint32 diff) override
    {
        Rest(diff);
        sinceHappiness += diff;
        if (sinceHappiness < HAPPINESS_TICK || questsWithPerk.find("pet_fed") == questsWithPerk.end())
            return;
        sinceHappiness = 0;

        std::vector<Player*> hunters;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
                if (player->IsInWorld() && !player->GetSession()->IsHeadless() && player->getClass() == CLASS_HUNTER)
                    hunters.push_back(player);
        }

        for (Player* player : hunters)
            if (Pet* pet = player->GetPet())
                if (pet->getPetType() == HUNTER_PET && pet->IsAlive() && HasPerk(player, "pet_fed"))
                    pet->ModifyPower(POWER_HAPPINESS, HAPPINESS_BACK);
    }
};

void WowPlusPerksWorld::Rest(uint32 diff)
{
    sinceRest += diff;
    if (sinceRest < REST_TICK)
        return;
    sinceRest = 0;

    bool const rage = questsWithPerk.find("rage_hold") != questsWithPerk.end();
    bool const wind = questsWithPerk.find("second_wind") != questsWithPerk.end();
    bool const mana = questsWithPerk.find("mana_rest") != questsWithPerk.end();
    bool const wild = questsWithPerk.find("wild_rest") != questsWithPerk.end();
    bool const mend = questsWithPerk.find("demon_mend") != questsWithPerk.end();
    if (!rage && !wind && !mana && !wild && !mend)
        return;

    std::vector<Player*> resting;
    {
        std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
            if (player->IsInWorld() && !player->GetSession()->IsHeadless() && player->IsAlive() && !player->IsInCombat())
                resting.push_back(player);
    }

    for (Player* player : resting)
    {
        if (rage && player->getPowerType() == POWER_RAGE && player->GetPower(POWER_RAGE) > 0 && HasPerk(player, "rage_hold"))
            player->ModifyPower(POWER_RAGE, RAGE_BACK);
        if (mend && player->getClass() == CLASS_WARLOCK)
            if (Pet* demon = player->GetPet())
                if (demon->IsAlive() && !demon->IsInCombat() && demon->GetHealth() < demon->GetMaxHealth() && HasPerk(player, "demon_mend"))
                    demon->ModifyHealth(int32(demon->GetMaxHealth() * MEND_SHARE));
        bool const rests = wild && HasPerk(player, "wild_rest");
        if (rests && player->GetHealth() < player->GetMaxHealth())
            player->ModifyHealth(int32(player->GetMaxHealth() * WIND_SHARE));
        if (rests && player->GetPower(POWER_MANA) < player->GetMaxPower(POWER_MANA))
            player->ModifyPower(POWER_MANA, int32(player->GetMaxPower(POWER_MANA) * MANA_SHARE));
        if (wind && player->GetHealth() < player->GetMaxHealth() && HasPerk(player, "second_wind"))
            player->ModifyHealth(int32(player->GetMaxHealth() * WIND_SHARE));
        if (mana && player->getPowerType() == POWER_MANA && player->GetPower(POWER_MANA) < player->GetMaxPower(POWER_MANA) && HasPerk(player, "mana_rest"))
            player->ModifyPower(POWER_MANA, int32(player->GetMaxPower(POWER_MANA) * MANA_SHARE));
    }
}

class WowPlusPerksPlayer : public PlayerScript
{
public:
    WowPlusPerksPlayer() : PlayerScript("WowPlusPerksPlayer") { }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        auto found = perkOfQuest.find(quest->GetQuestId());
        if (found == perkOfQuest.end())
            return;

        ChatHandler chat(player->GetSession());
        if (found->second == "stable_slot")
        {
            PetStable& stable = player->GetOrInitPetStable();
            if (stable.MaxStabledPets < MAX_PET_STABLES)
            {
                ++stable.MaxStabledPets;
                chat.SendSysMessage("You have earned a stable slot: stable masters will keep one more of your pets, free of charge.");
            }
            else
                chat.SendSysMessage("You already have every stable slot there is. The stable masters salute you instead.");
        }
        else if (found->second == "pet_fed")
            chat.SendSysMessage("You have learned to keep a beast content: your pets lose happiness half as fast from now on.");
        else if (found->second == "skinning")
            chat.SendSysMessage("You have learned to waste nothing: skinning a beast will sometimes give you one more of what it gave.");
        else if (found->second == "rage_hold")
            chat.SendSysMessage("You have learned to hold your anger: your rage drains half as fast out of combat from now on.");
        else if (found->second == "second_wind")
            chat.SendSysMessage("You have learned to catch your breath: your health comes back faster between fights from now on.");
        else if (found->second == "mana_rest")
            chat.SendSysMessage("You have learned to draw on what is around you: your mana comes back faster out of combat from now on.");
        else if (found->second == "polymorph_pig")
        {
            if (!player->HasSpell(SPELL_POLYMORPH_PIG))
                player->learnSpell(SPELL_POLYMORPH_PIG);
            chat.SendSysMessage("You have learned a new shape for Polymorph: the pig.");
        }
        else if (found->second == "double_conjure")
            chat.SendSysMessage("You have learned to waste nothing of a spell: conjuring food or water makes twice as much from now on.");
        else if (found->second == "shard_luck")
            chat.SendSysMessage("You have learned that the dead owe you: now and then a kill will leave a soul shard in your bag without your draining for it.");
        else if (found->second == "demon_mend")
            chat.SendSysMessage("You have learned to bind with a looser hand: your demon mends itself out of combat from now on.");
        else if (found->second == "wild_rest")
            chat.SendSysMessage("You have learned to rest as the wild rests: your health and mana both come back faster out of combat from now on.");
        else if (found->second == "herbs")
            chat.SendSysMessage("You have learned to take without harming the root: gathering a herb will sometimes give you one more.");
        else if (found->second == "pockets")
            chat.SendSysMessage("You have learned where people keep what matters: picking a pocket brings you twice the coin from now on.");
        else if (found->second == "fence")
            chat.SendSysMessage("You have learned what to look for: you find a tenth more coin on everything you loot from now on.");
        else if (found->second == "repairs")
            chat.SendSysMessage("You have learned to care for your arms: repairs cost you a quarter less from now on.");
    }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        if (!killer || !killed || killer->getClass() != CLASS_WARLOCK || killed->GetLevel() <= Acore::XP::GetGrayLevel(killer->GetLevel()))
            return;

        if (HasPerk(killer, "shard_luck") && roll_chance_i(SHARD_CHANCE))
            killer->AddItem(ITEM_SOUL_SHARD, 1);
    }

    void OnPlayerCreateItem(Player* player, Item* item, uint32 count) override
    {
        if (!item || !count || !HasPerk(player, "double_conjure"))
            return;

        ItemTemplate const* made = item->GetTemplate();
        if (made->Class == ITEM_CLASS_CONSUMABLE && made->SubClass == ITEM_SUBCLASS_FOOD && made->HasFlag(ITEM_FLAG_CONJURED))
            player->AddItem(made->ItemId, count);
    }

    void OnPlayerBeforeLootMoney(Player* player, Loot* loot) override
    {
        if (!loot || !loot->gold)
            return;

        if (loot->loot_type == LOOT_PICKPOCKETING && HasPerk(player, "pockets"))
            loot->gold *= 2;
        if (HasPerk(player, "fence"))
            loot->gold += loot->gold / 10;
    }

    void OnPlayerBeforeDurabilityRepair(Player* player, ObjectGuid /*npcGUID*/, ObjectGuid /*itemGUID*/, float& discountMod, uint8 /*guildBank*/) override
    {
        if (HasPerk(player, "repairs"))
            discountMod *= REPAIR_SHARE;
    }

    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid lootguid) override
    {
        if (!item)
            return;

        // A herb picked from a plant in the ground
        if (lootguid.IsGameObject())
        {
            if (!HasPerk(player, "herbs"))
                return;

            GameObject* plant = ObjectAccessor::GetGameObject(*player, lootguid);
            LockEntry const* lock = plant ? sLockStore.LookupEntry(plant->GetGOInfo()->GetLockId()) : nullptr;
            if (!lock || !roll_chance_i(EXTRA_SKIN_CHANCE))
                return;

            for (uint8 i = 0; i < MAX_LOCK_CASE; ++i)
                if (lock->Type[i] == LOCK_KEY_SKILL && lock->Index[i] == LOCKTYPE_HERBALISM)
                {
                    player->AddItem(item->GetEntry(), 1);
                    return;
                }
            return;
        }

        if (!lootguid.IsCreature() || !HasPerk(player, "skinning"))
            return;

        Creature* creature = ObjectAccessor::GetCreature(*player, lootguid);
        if (!creature || creature->loot.loot_type != LOOT_SKINNING || !roll_chance_i(EXTRA_SKIN_CHANCE))
            return;

        player->AddItem(item->GetEntry(), 1);
    }
};

void WowPlusPerksReload()
{
    LoadPerks();
}

void AddSC_WowPlusPerks()
{
    new WowPlusPerksWorld();
    new WowPlusPerksPlayer();
}
