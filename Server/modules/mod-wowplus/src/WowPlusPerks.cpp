/*
 * WowPlus: lasting perks that some of our quests give when they are handed in.
 *
 * Which quest gives which perk is in the table wowplus_quest_perks, made by Settings/Scripts/Make-Quests.py from the quest
 * files ("perk": "..."). A player has a perk for as long as the quest counts as done, so nothing more is stored.
 *
 *   pet_fed      a hunter's pet loses happiness half as fast
 *   stable_slot  one stable slot, free, when the quest is handed in
 *   skinning     skinning a beast sometimes gives one more of what it gave
 */

#include "Chat.h"
#include "Creature.h"
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

    std::unordered_map<std::string, std::vector<uint32>> questsWithPerk;    // perk -> the quests that give it
    std::unordered_map<uint32, std::string> perkOfQuest;
    uint32 sinceHappiness = 0;

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

    void OnUpdate(uint32 diff) override
    {
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
    }

    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid lootguid) override
    {
        if (!item || !lootguid.IsCreature() || !HasPerk(player, "skinning"))
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
