/*
 * WowPlus classes: small helps so that no class is missing its core while levelling alone.
 *
 * The rule (Settings/Docs/Class-Changes.md): keep the friction, remove the misery.
 * Abilities that used to come from class quests are taught by the class trainers instead
 * (data: Settings/SQL/world/..._class-quest-abilities.sql). This file covers what a trainer
 * cannot do:
 *
 * - A hunter has a pet from level 1. It is a plain starter animal; Tame Beast still comes at
 *   level 10, and that is the day the hunter chooses a pet of their own.
 * - A shaman gets the Totem of the Earthen Ring, which stands in for the four totem items the
 *   totem quests used to hand out. Without it the totem spells cannot be cast.
 */

#include "Chat.h"
#include "Config.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <array>

namespace
{
    constexpr uint32 ITEM_TOTEM_OF_THE_EARTHEN_RING = 46978;
    constexpr uint8 SHAMAN_TOTEM_LEVEL = 4;                 // the first totem spell

    // What a hunter needs to keep a pet: Call Pet, Dismiss Pet, Revive Pet, Feed Pet
    constexpr std::array<uint32, 4> HUNTER_PET_CARE_SPELLS = {883, 2641, 982, 6991};
    constexpr uint8 HUNTER_TAME_LEVEL = 10;

    std::string const SETTING_SOURCE = "mod-wowplus-classes";
    constexpr uint32 SETTING_STARTER_PET_GIVEN = 0;

    bool hunterStarterPet = true;
    bool shamanTotemRelic = true;

    // An animal from the hunter's own starting zone
    uint32 StarterPetFor(uint8 race)
    {
        switch (race)
        {
            case RACE_NIGHTELF:
                return 2031;                                // Young Nightsaber
            case RACE_DWARF:
                return 708;                                 // Small Crag Boar
            case RACE_TAUREN:
                return 2955;                                // Plainstrider
            case RACE_BLOODELF:
                return 15366;                               // Springpaw Cub
            case RACE_DRAENEI:
                return 16520;                               // Vale Moth
            default:
                return 3098;                                // Mottled Boar (orcs and trolls)
        }
    }

    void HelpHunter(Player* player)
    {
        if (!hunterStarterPet || player->getClass() != CLASS_HUNTER)
            return;

        for (uint32 spellId : HUNTER_PET_CARE_SPELLS)
            if (!player->HasSpell(spellId))
                player->learnSpell(spellId, false);

        // Only before the hunter can tame, and only once: a pet lost or abandoned is not replaced
        if (player->GetLevel() >= HUNTER_TAME_LEVEL || player->GetPlayerSetting(SETTING_SOURCE, SETTING_STARTER_PET_GIVEN).value)
            return;

        if (player->IsExistPet())
        {
            player->UpdatePlayerSetting(SETTING_SOURCE, SETTING_STARTER_PET_GIVEN, 1);
            return;
        }

        if (player->CreatePet(StarterPetFor(player->getRace())))
        {
            player->UpdatePlayerSetting(SETTING_SOURCE, SETTING_STARTER_PET_GIVEN, 1);
            ChatHandler(player->GetSession()).SendSysMessage("An animal from these lands has taken to you. At level 10 your trainer "
                "will teach you to tame a companion of your own choosing.");
        }
    }

    void HelpShaman(Player* player)
    {
        if (!shamanTotemRelic || player->getClass() != CLASS_SHAMAN || player->GetLevel() < SHAMAN_TOTEM_LEVEL)
            return;

        if (player->HasItemCount(ITEM_TOTEM_OF_THE_EARTHEN_RING, 1, true))
            return;

        if (player->AddItem(ITEM_TOTEM_OF_THE_EARTHEN_RING, 1))
            ChatHandler(player->GetSession()).SendSysMessage("The elements answer you: you receive the Totem of the Earthen Ring. "
                "Keep it with you to call your totems.");
    }
}

class WowPlusClassesWorld : public WorldScript
{
public:
    WowPlusClassesWorld() : WorldScript("WowPlusClassesWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        hunterStarterPet = sConfigMgr->GetOption<bool>("WowPlus.Classes.HunterStarterPet", true);
        shamanTotemRelic = sConfigMgr->GetOption<bool>("WowPlus.Classes.ShamanTotemRelic", true);
    }
};

class WowPlusClassesPlayer : public PlayerScript
{
public:
    WowPlusClassesPlayer() : PlayerScript("WowPlusClassesPlayer") { }

    void OnPlayerLogin(Player* player) override
    {
        if (player->GetSession()->IsHeadless())
            return;                                         // bots are equipped by the bot module

        HelpHunter(player);
        HelpShaman(player);
    }

    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (player->GetSession()->IsHeadless())
            return;

        HelpShaman(player);
    }
};

void AddSC_WowPlusClasses()
{
    new WowPlusClassesWorld();
    new WowPlusClassesPlayer();
}
