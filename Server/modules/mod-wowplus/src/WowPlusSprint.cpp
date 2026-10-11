/*
 * WowPlus sprint: hold Shift to run or swim faster for a short burst, for as long as stamina lasts.
 * Short bursts are rewarded: a bar that is run empty leaves the character winded, and refills slower.
 *
 * The game client never tells the server which keys are held. A small addon
 * (Client/AddOns/WowPlusSprint) does: it sends a hidden addon message when Shift goes down or
 * up, and draws the stamina bar. The server owns the stamina and decides everything.
 *
 * The speed comes from a hidden passive aura (spell 100200, Settings/SQL/world/..._sprint.sql).
 * Sprint is for travelling on foot and swimming: it does nothing while mounted, in a travelling
 * form, in stealth, or while any other speed boost is active (Aspect of the Cheetah, rogue
 * Sprint, Dash, Ghost Wolf and so on). It must never be the better choice than those.
 *
 * Messages, prefix WPS:
 *   addon to server: "H" hello, "1" Shift held, "0" Shift released
 *   server to addon: "S:<stamina 0..1000>:<change per second>:<exhausted 0 or 1>", or "X" when
 *   sprint is switched off
 */

#include "Chat.h"
#include "Config.h"
#include "DataMap.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>

namespace
{
    constexpr uint32 SPELL_WOWPLUS_SPRINT = 100200;
    constexpr float FULL = 1000.0f;                         // stamina scale, as sent to the addon
    constexpr uint32 SYNC_INTERVAL = 2000;                  // ms between corrections while stamina changes
    std::string const ADDON_PREFIX = "WPS\t";
    std::string const DATA_KEY = "wowplus-sprint";

    bool enabled = true;
    int32 speedPercent = 30;
    float drainPerSecond = FULL / 6.0f;
    float refillPerSecond = FULL / 8.0f;
    uint32 refillDelay = 1000;
    uint32 windedRefillDelay = 2000;                        // after the bar was run empty
    float windedRefillSpeed = 0.75f;
    uint32 visualKit = 10440;                               // a small puff of dust at the feet
    uint32 visualInterval = 700;
    float combatDrain = 2.0f;
    float restartAt = FULL * 0.25f;                         // an emptied bar must refill this far first

    struct SprintState : public DataMap::Base
    {
        bool wants = false;                                 // Shift is held
        bool running = false;                               // the aura is on
        bool exhausted = false;
        float stamina = FULL;
        uint32 idle = 0;                                    // ms since the last sprinting moment
        int32 sentRate = 0;
        bool sentExhausted = false;
        uint32 sinceSync = 0;
        uint32 sinceVisual = 0;
    };

    void Send(Player* player, std::string const& text)
    {
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, ADDON_PREFIX + text);
        player->SendDirectMessage(&data);
    }

    void SendState(Player* player, SprintState& state, int32 rate)
    {
        state.sentRate = rate;
        state.sentExhausted = state.exhausted;
        state.sinceSync = 0;
        Send(player, "S:" + std::to_string(int32(state.stamina)) + ":" + std::to_string(rate) + ":" + (state.exhausted ? "1" : "0"));
    }

    bool HasOtherSpeedBoost(Player* player)
    {
        for (AuraType type : {SPELL_AURA_MOD_INCREASE_SPEED, SPELL_AURA_MOD_INCREASE_SWIM_SPEED})
            for (AuraEffect const* effect : player->GetAuraEffectsByType(type))
                if (effect->GetId() != SPELL_WOWPLUS_SPRINT && effect->GetAmount() > 0)
                    return true;

        return false;
    }

    // Sprint is for going on foot or swimming, with no other speed boost.
    bool CanSprintNow(Player* player)
    {
        if (!player->IsAlive() || player->IsMounted() || player->IsInFlight() || player->GetVehicle())
            return false;

        if (player->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED))
            return false;

        switch (player->GetShapeshiftForm())
        {
            case FORM_CAT:
            case FORM_TRAVEL:
            case FORM_AQUA:
            case FORM_GHOSTWOLF:
            case FORM_FLIGHT:
            case FORM_FLIGHT_EPIC:
                return false;
            default:
                break;
        }

        return !player->HasStealthAura() && !HasOtherSpeedBoost(player);
    }

    void SetRunning(Player* player, SprintState& state, bool run)
    {
        if (state.running == run)
            return;

        state.running = run;
        if (!run)
        {
            player->RemoveAurasDueToSpell(SPELL_WOWPLUS_SPRINT);
            return;
        }

        // Effect 0 is run speed, effect 1 swim speed
        if (Aura* aura = player->AddAura(SPELL_WOWPLUS_SPRINT, player))
            for (uint8 index : {EFFECT_0, EFFECT_1})
                if (AuraEffect* effect = aura->GetEffect(index))
                    effect->ChangeAmount(speedPercent);
    }

    void Update(Player* player, SprintState& state, uint32 diff)
    {
        float const seconds = diff / 1000.0f;
        int32 rate = 0;

        bool const run = state.wants && !state.exhausted && state.stamina > 0.0f && player->isMoving() && CanSprintNow(player);
        if (run)
        {
            float const drain = drainPerSecond * (player->IsInCombat() ? combatDrain : 1.0f);
            state.stamina -= drain * seconds;
            state.idle = 0;
            rate = -int32(drain);

            if (state.stamina <= 0.0f)
            {
                state.stamina = 0.0f;
                state.exhausted = true;
            }
        }
        else if (state.stamina < FULL)
        {
            // Winded (the bar was run empty): a longer wait and a slower refill until sprint is back
            float const refill = refillPerSecond * (state.exhausted ? windedRefillSpeed : 1.0f);
            state.idle += diff;
            if (state.idle >= (state.exhausted ? windedRefillDelay : refillDelay))
            {
                state.stamina = std::min(FULL, state.stamina + refill * seconds);
                rate = int32(refill);

                if (state.exhausted && state.stamina >= restartAt)
                    state.exhausted = false;
            }
        }

        SetRunning(player, state, run && !state.exhausted);

        // Dust at the feet while sprinting on land
        state.sinceVisual += diff;
        if (state.running && visualKit && state.sinceVisual >= visualInterval && !player->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
        {
            state.sinceVisual = 0;
            player->SendPlaySpellVisual(visualKit);
        }

        if (state.stamina >= FULL)
            rate = 0;

        // The addon moves the bar by itself from the last rate; tell it when the rate changes,
        // and correct it now and then while the bar is moving.
        state.sinceSync += diff;
        if (rate != state.sentRate || state.exhausted != state.sentExhausted || (rate != 0 && state.sinceSync >= SYNC_INTERVAL))
            SendState(player, state, rate);
    }
}

class WowPlusSprintWorld : public WorldScript
{
public:
    WowPlusSprintWorld() : WorldScript("WowPlusSprintWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("WowPlus.Sprint.Enable", true);
        speedPercent = sConfigMgr->GetOption<int32>("WowPlus.Sprint.SpeedPercent", 30);
        drainPerSecond = FULL / std::max(1.0f, sConfigMgr->GetOption<float>("WowPlus.Sprint.Seconds", 6.0f));
        refillPerSecond = FULL / std::max(1.0f, sConfigMgr->GetOption<float>("WowPlus.Sprint.RefillSeconds", 8.0f));
        refillDelay = uint32(std::max(0.0f, sConfigMgr->GetOption<float>("WowPlus.Sprint.RefillDelay", 1.0f)) * 1000.0f);
        windedRefillDelay = uint32(std::max(0.0f, sConfigMgr->GetOption<float>("WowPlus.Sprint.Winded.RefillDelay", 2.0f)) * 1000.0f);
        windedRefillSpeed = std::clamp(sConfigMgr->GetOption<float>("WowPlus.Sprint.Winded.RefillSpeed", 0.75f), 0.05f, 1.0f);
        visualKit = sConfigMgr->GetOption<uint32>("WowPlus.Sprint.VisualKit", 10440);
        visualInterval = uint32(std::max(0.2f, sConfigMgr->GetOption<float>("WowPlus.Sprint.VisualInterval", 0.7f)) * 1000.0f);
        combatDrain = std::max(0.0f, sConfigMgr->GetOption<float>("WowPlus.Sprint.CombatDrain", 2.0f));
        restartAt = FULL * std::clamp(sConfigMgr->GetOption<float>("WowPlus.Sprint.RestartPercent", 25.0f), 0.0f, 100.0f) / 100.0f;
    }

    void OnStartup() override
    {
        if (!enabled)
            return;

        if (!sSpellMgr->GetSpellInfo(SPELL_WOWPLUS_SPRINT))
        {
            LOG_ERROR("server.loading", ">> WowPlus sprint: spell {} is missing (Settings/SQL/world/2026-10-11_sprint.sql), sprint is off",
                SPELL_WOWPLUS_SPRINT);
            enabled = false;
            return;
        }

        LOG_INFO("server.loading", ">> WowPlus sprint: +{} percent on foot and swimming while Shift is held", speedPercent);
    }
};

class WowPlusSprintPlayer : public PlayerScript
{
public:
    WowPlusSprintPlayer() : PlayerScript("WowPlusSprintPlayer") { }

    // The addon talks to the server by whispering its own character. Nothing is passed on.
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* /*receiver*/) override
    {
        if (language != LANG_ADDON || type != CHAT_MSG_WHISPER || msg.compare(0, ADDON_PREFIX.size(), ADDON_PREFIX) != 0)
            return true;

        std::string const text = msg.substr(ADDON_PREFIX.size());
        if (!enabled)
        {
            if (text == "H")
                Send(player, "X");
            return false;
        }

        SprintState* state = player->CustomData.GetDefault<SprintState>(DATA_KEY);
        if (text == "1")
            state->wants = true;
        else if (text == "0")
            state->wants = false;
        else if (text == "H")
        {
            state->wants = false;
            SendState(player, *state, state->sentRate);
        }

        return false;
    }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!enabled)
            return;

        // Only characters whose addon has said hello have a state; bots never do
        if (SprintState* state = player->CustomData.Get<SprintState>(DATA_KEY))
            Update(player, *state, diff);
    }
};

void AddSC_WowPlusSprint()
{
    new WowPlusSprintWorld();
    new WowPlusSprintPlayer();
}
