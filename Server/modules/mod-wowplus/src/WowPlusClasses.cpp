/*
 * WowPlus classes: small helps so that no class is missing its core while levelling alone.
 *
 * The rule (Settings/Docs/Class-Changes.md): keep the friction, remove the misery.
 * Most class changes are data: which level a trainer teaches what
 * (Settings/SQL/world/..._class-quest-abilities.sql and ..._early-levels.sql).
 * This file covers what a trainer cannot do:
 *
 * - A hunter has a pet from level 1. It is a plain starter animal; Tame Beast still comes at
 *   level 10, and that is the day the hunter chooses a pet of their own.
 * - A shaman gets the Totem of the Earthen Ring, which stands in for the four totem items the
 *   totem quests used to hand out. Without it the totem spells cannot be cast.
 * - A rogue can dual wield from the start.
 * - Warlocks keep an enslaved demon for as long as they like (it is gone when they log out), and an
 *   Infernal or a Doomguard stays instead of leaving after a minute. Either takes the place of the
 *   warlock's ordinary demon: only one of them is out at a time. Ritual of Doom works alone.
 * - Rogues move at full speed in stealth. Shamans can take Ghost Wolf form indoors.
 * - Everyone gets the second talent set (dual specialisation) for free at level 10, and can
 *   reset talents for free from the talent window (a button added by the addon WowPlusTalents,
 *   which sends the request here).
 * - Optional, off by default: a second starting dagger for rogues and a first wand for casters
 *   at level 5. Since 2026-10-11 the weaponsmiths of the starting areas sell those instead.
 */

#include "Chat.h"
#include "Config.h"
#include "Pet.h"
#include "Player.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "DBCStores.h"
#include "TemporarySummon.h"
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

    constexpr uint32 SPELL_DUAL_WIELD = 674;
    constexpr uint32 SPELL_TEACH_SPEC_SWITCHING = 63680;    // teaches "activate primary / secondary spec"
    constexpr uint32 SPELL_LEARN_SECOND_SPEC = 63624;
    std::string const TALENT_ADDON_PREFIX = "WPT\t";
    constexpr uint32 ITEM_WORN_DAGGER = 2092;               // what a new rogue starts with
    constexpr uint8 ROGUE_OLD_DUAL_WIELD_LEVEL = 10;

    constexpr uint32 ITEM_LESSER_MAGIC_WAND = 11287;        // the classic first wand, usable from level 5
    constexpr uint8 WAND_LEVEL = 5;
    constexpr uint8 WAND_LAST_LEVEL = 14;                   // a character above this has long outgrown it

    // What has been handed out is remembered per character (table character_settings)
    std::string const SETTING_SOURCE = "mod-wowplus-classes";
    constexpr uint32 SETTING_STARTER_PET_GIVEN = 0;
    constexpr uint32 SETTING_ITEMS_GIVEN = 1;               // one bit per item below
    constexpr uint32 GIVEN_WAND = 0x1;
    constexpr uint32 GIVEN_SECOND_DAGGER = 0x2;

    bool hunterStarterPet = true;
    bool shamanTotemRelic = true;
    bool rogueDualWield = true;
    bool rogueSecondDagger = false;                         // off: the starting weaponsmiths sell one
    bool casterWand = false;                                // off: the starting weaponsmiths sell one
    bool warlockDemons = true;
    bool rogueStealthSpeed = true;
    bool shamanWolfIndoors = true;

    // Warlock
    constexpr uint32 SPELL_INFERNO = 1122;
    constexpr uint32 SPELL_RITUAL_OF_DOOM = 18540;
    constexpr uint32 SPELL_SUMMON_DOOMGUARD = 60478;        // what the finished ritual casts
    constexpr uint32 NPC_INFERNAL = 89;
    constexpr uint32 NPC_DOOMGUARD = 11859;
    constexpr uint32 DURATION_UNTIL_CANCELLED = 21;         // SpellDuration.dbc
    constexpr std::array<uint32, 4> ENSLAVE_DEMON_RANKS = {1098, 11725, 11726, 61191};
    constexpr std::array<uint32, 5> ORDINARY_DEMON_SUMMONS = {688, 697, 712, 691, 30146};

    constexpr uint32 SPELL_STEALTH = 1784;
    constexpr uint32 SPELL_GHOST_WOLF = 2645;

    template <std::size_t N>
    bool OneOf(std::array<uint32, N> const& list, uint32 id)
    {
        return std::find(list.begin(), list.end(), id) != list.end();
    }

    // Sends away a warlock's Infernal or Doomguard
    void DismissGreatDemons(Player* player)
    {
        std::vector<Unit*> gone;
        for (Unit* controlled : player->m_Controlled)
            if (controlled->GetEntry() == NPC_INFERNAL || controlled->GetEntry() == NPC_DOOMGUARD)
                gone.push_back(controlled);

        for (Unit* demon : gone)
            if (Creature* creature = demon->ToCreature())
                creature->DespawnOrUnsummon();
    }
    uint32 dualSpecLevel = 10;                              // 0: not given
    bool talentResetButton = true;

    void Tell(Player* player, char const* text)
    {
        ChatHandler(player->GetSession()).SendSysMessage(text);
    }

    // Hands an item over once per character. False if it was given before or the bags are full.
    bool GiveOnce(Player* player, uint32 flag, uint32 itemId)
    {
        uint32 const given = player->GetPlayerSetting(SETTING_SOURCE, SETTING_ITEMS_GIVEN).value;
        if (given & flag)
            return false;

        if (!player->AddItem(itemId, 1))
            return false;                                   // tried again at the next login or level

        player->UpdatePlayerSetting(SETTING_SOURCE, SETTING_ITEMS_GIVEN, given | flag);
        return true;
    }

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
            Tell(player, "An animal from these lands has taken to you. At level 10 your trainer will teach you to tame a "
                "companion of your own choosing.");
        }
    }

    void HelpShaman(Player* player)
    {
        if (!shamanTotemRelic || player->getClass() != CLASS_SHAMAN || player->GetLevel() < SHAMAN_TOTEM_LEVEL)
            return;

        if (player->HasItemCount(ITEM_TOTEM_OF_THE_EARTHEN_RING, 1, true))
            return;

        if (player->AddItem(ITEM_TOTEM_OF_THE_EARTHEN_RING, 1))
            Tell(player, "The elements answer you: you receive the Totem of the Earthen Ring. Keep it with you to call your totems.");
    }

    void HelpRogue(Player* player)
    {
        if (!rogueDualWield || player->getClass() != CLASS_ROGUE)
            return;

        if (!player->HasSpell(SPELL_DUAL_WIELD))
            player->learnSpell(SPELL_DUAL_WIELD, false);

        // A new rogue owns one dagger; one that is past the old Dual Wield level has found weapons already
        if (rogueSecondDagger && player->GetLevel() < ROGUE_OLD_DUAL_WIELD_LEVEL && GiveOnce(player, GIVEN_SECOND_DAGGER, ITEM_WORN_DAGGER))
            Tell(player, "You find a second dagger in your pack. A rogue fights with both hands.");
    }

    void HelpCaster(Player* player)
    {
        if (!casterWand)
            return;

        uint8 const playerClass = player->getClass();
        if (playerClass != CLASS_PRIEST && playerClass != CLASS_MAGE && playerClass != CLASS_WARLOCK)
            return;

        if (player->GetLevel() < WAND_LEVEL || player->GetLevel() > WAND_LAST_LEVEL)
            return;

        if (GiveOnce(player, GIVEN_WAND, ITEM_LESSER_MAGIC_WAND))
            Tell(player, "Your studies have earned you a wand of your own: you receive a Lesser Magic Wand.");
    }

    // The second talent set, as the trainers sell it at level 40 for 1000 gold
    void GiveDualSpec(Player* player)
    {
        if (!dualSpecLevel || player->GetLevel() < dualSpecLevel || player->GetSpecsCount() >= 2)
            return;

        player->CastSpell(player, SPELL_TEACH_SPEC_SWITCHING, true, nullptr, nullptr, player->GetGUID());
        player->CastSpell(player, SPELL_LEARN_SECOND_SPEC, true, nullptr, nullptr, player->GetGUID());
        Tell(player, "You can now keep two sets of talents and switch between them: see the second tab of the talent window.");
    }

    void HelpAll(Player* player)
    {
        if (player->GetSession()->IsHeadless())
            return;                                         // bots are equipped by the bot module

        GiveDualSpec(player);
        HelpHunter(player);
        HelpShaman(player);
        HelpRogue(player);
        HelpCaster(player);
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
        rogueDualWield = sConfigMgr->GetOption<bool>("WowPlus.Classes.RogueDualWield", true);
        rogueSecondDagger = sConfigMgr->GetOption<bool>("WowPlus.Classes.RogueSecondDagger", false);
        casterWand = sConfigMgr->GetOption<bool>("WowPlus.Classes.CasterWand", false);
        dualSpecLevel = sConfigMgr->GetOption<uint32>("WowPlus.Classes.DualSpecLevel", 10);
        talentResetButton = sConfigMgr->GetOption<bool>("WowPlus.Classes.TalentResetButton", true);
        warlockDemons = sConfigMgr->GetOption<bool>("WowPlus.Classes.WarlockDemons", true);
        rogueStealthSpeed = sConfigMgr->GetOption<bool>("WowPlus.Classes.RogueStealthSpeed", true);
        shamanWolfIndoors = sConfigMgr->GetOption<bool>("WowPlus.Classes.ShamanWolfIndoors", true);
    }
};

class WowPlusClassesPlayer : public PlayerScript
{
public:
    WowPlusClassesPlayer() : PlayerScript("WowPlusClassesPlayer") { }

    void OnPlayerLogin(Player* player) override
    {
        HelpAll(player);
    }

    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        HelpAll(player);
    }

    // Warlock: one servant at a time. An Infernal or a Doomguard takes the ordinary demon's place, and the other way round.
    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!warlockDemons || player->getClass() != CLASS_WARLOCK)
            return;

        uint32 const id = spell->GetSpellInfo()->Id;
        if (id == SPELL_INFERNO || id == SPELL_RITUAL_OF_DOOM)
        {
            DismissGreatDemons(player);
            if (Pet* pet = player->GetPet())
                player->RemovePet(pet, PET_SAVE_NOT_IN_SLOT);

            // The ritual asks for five people at a portal. Alone, the Doomguard simply comes.
            if (id == SPELL_RITUAL_OF_DOOM)
                player->CastSpell(player, SPELL_SUMMON_DOOMGUARD, true);
        }
        else if (OneOf(ORDINARY_DEMON_SUMMONS, id))
            DismissGreatDemons(player);
    }

    using PlayerScript::OnPlayerCanUseChat;                 // keep the other overloads visible

    // The "Reset talents" button of the addon WowPlusTalents: it whispers its own character. Nothing is passed on.
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* /*receiver*/) override
    {
        if (language != LANG_ADDON || type != CHAT_MSG_WHISPER || msg.compare(0, TALENT_ADDON_PREFIX.size(), TALENT_ADDON_PREFIX) != 0)
            return true;

        if (!talentResetButton || msg.substr(TALENT_ADDON_PREFIX.size()) != "reset")
            return false;

        if (player->IsInCombat() || player->InBattleground() || !player->IsAlive())
        {
            Tell(player, "You cannot reset your talents right now.");
            return false;
        }

        player->resetTalents(true);
        player->SendTalentsInfoData(false);
        Tell(player, "Your talents have been reset.");
        return false;
    }
};

// Changes to what spells do, made once when the server has read them. The game client still shows its own
// description of each.
class WowPlusClassesSpells : public GlobalScript
{
public:
    WowPlusClassesSpells() : GlobalScript("WowPlusClassesSpells") { }

    void OnLoadSpellCustomAttr(SpellInfo* spell) override
    {
        // This runs before the settings of this module are read, so it reads its own
        if (sConfigMgr->GetOption<bool>("WowPlus.Classes.WarlockDemons", true) &&
            (OneOf(ENSLAVE_DEMON_RANKS, spell->Id) || spell->Id == SPELL_INFERNO || spell->Id == SPELL_SUMMON_DOOMGUARD))
            spell->DurationEntry = sSpellDurationStore.LookupEntry(DURATION_UNTIL_CANCELLED);

        // Stealth: the third effect slows the rogue by 30 percent. With base points -1 it comes to nothing.
        if (spell->Id == SPELL_STEALTH && sConfigMgr->GetOption<bool>("WowPlus.Classes.RogueStealthSpeed", true))
            spell->Effects[EFFECT_2].BasePoints = -1;

        if (spell->Id == SPELL_GHOST_WOLF && sConfigMgr->GetOption<bool>("WowPlus.Classes.ShamanWolfIndoors", true))
            spell->Attributes &= ~SPELL_ATTR0_ONLY_OUTDOORS;
    }
};

void AddSC_WowPlusClasses()
{
    new WowPlusClassesSpells();
    new WowPlusClassesWorld();
    new WowPlusClassesPlayer();
}
