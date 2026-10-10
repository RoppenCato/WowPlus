/*
 * WowPlus travel: getting around faster without shrinking the world.
 *
 * So far: the hearthstone has no cooldown. The approach (clearing the cooldown at the moment
 * the hearthstone teleports) follows mod-no-hearthstone-cooldown by BytesGalore.
 */

#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
    constexpr uint32 SPELL_HEARTHSTONE = 8690;
    constexpr uint32 SPELL_NO_PLACE_LIKE_HOME = 39937;      // Ruby Slippers, shares the hearthstone cooldown

    bool hearthstoneNoCooldown = true;

    void ClearHearthstoneCooldown(Player* player)
    {
        for (uint32 spellId : {SPELL_HEARTHSTONE, SPELL_NO_PLACE_LIKE_HOME})
        {
            player->RemoveSpellCooldown(spellId, true);
            player->SendClearCooldown(spellId, player);
        }
    }
}

class WowPlusTravelWorld : public WorldScript
{
public:
    WowPlusTravelWorld() : WorldScript("WowPlusTravelWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        hearthstoneNoCooldown = sConfigMgr->GetOption<bool>("WowPlus.Travel.Hearthstone.NoCooldown", true);
    }
};

class WowPlusTravelPlayer : public PlayerScript
{
public:
    WowPlusTravelPlayer() : PlayerScript("WowPlusTravelPlayer") { }

    // The hearthstone is still being cast at the moment it teleports
    bool OnPlayerBeforeTeleport(Player* player, uint32 /*mapid*/, float /*x*/, float /*y*/, float /*z*/, float /*orientation*/,
        uint32 /*options*/, Unit* /*target*/) override
    {
        if (hearthstoneNoCooldown && player->FindCurrentSpellBySpellId(SPELL_HEARTHSTONE))
            ClearHearthstoneCooldown(player);

        return true;
    }

    // A cooldown left over from before the setting was switched on
    void OnPlayerLogin(Player* player) override
    {
        if (hearthstoneNoCooldown)
            ClearHearthstoneCooldown(player);
    }
};

void AddSC_WowPlusTravel()
{
    new WowPlusTravelWorld();
    new WowPlusTravelPlayer();
}
