/*
 * WowPlus PvP rewards: honor and arena points from ordinary kills.
 *
 * The game's PvP vendors in the cities price their gear in honor and arena points, which a solo
 * player barely earns. So enemies at the player's level give a little of both, and no arena
 * rating is asked for. (This used to belong to a hub on GM Island with vendors for everything;
 * the hub was removed on 2026-10-11 in favour of the auction house and the cities.)
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "Formulas.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <array>

namespace
{
    bool honorEnabled = true;
    std::array<uint32, 4> honorByRank = {10, 30, 60, 300};       // normal, elite, rare, boss
    std::array<float, 4> arenaPointsByRank = {0.2f, 1.0f, 3.0f, 15.0f};
    bool noRatingRequired = true;

    Player* HumanFor(Player* killer, Creature const* killed)
    {
        if (!killer->GetSession()->IsHeadless())
            return killer;

        if (Group* group = killer->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                if (Player* member = itr->GetSource())
                    if (!member->GetSession()->IsHeadless() && member->IsAtLootRewardDistance(killed))
                        return member;

        return nullptr;
    }

    void RewardKillHonor(Player* killer, Creature* killed)
    {
        if (!honorEnabled || !killer || !killed || killed->IsPet() || killed->IsCritter())
            return;

        Player* player = HumanFor(killer, killed);
        if (!player || killed->GetLevel() <= Acore::XP::GetGrayLevel(player->GetLevel()))
            return;

        uint8 rank = 0;
        if (killed->IsDungeonBoss() || killed->isWorldBoss())
            rank = 3;
        else if (killed->GetCreatureTemplate()->rank == CREATURE_ELITE_RARE || killed->GetCreatureTemplate()->rank == CREATURE_ELITE_RAREELITE)
            rank = 2;
        else if (killed->isElite())
            rank = 1;

        if (honorByRank[rank])
            player->RewardHonor(nullptr, 1, int32(honorByRank[rank]), false);

        // Arena points come in whole numbers: 0.2 means one point on every fifth kill or so
        uint32 arenaPoints = uint32(arenaPointsByRank[rank]);
        if (roll_chance_f((arenaPointsByRank[rank] - float(arenaPoints)) * 100.0f))
            ++arenaPoints;

        if (arenaPoints)
        {
            player->ModifyArenaPoints(int32(arenaPoints));
            if (arenaPoints >= 5)
                ChatHandler(player->GetSession()).PSendSysMessage("You receive {} arena points.", arenaPoints);
        }
    }
}

class WowPlusPvPWorld : public WorldScript
{
public:
    WowPlusPvPWorld() : WorldScript("WowPlusPvPWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        honorEnabled = sConfigMgr->GetOption<bool>("WowPlus.Honor.FromKills", true);
        honorByRank[0] = sConfigMgr->GetOption<uint32>("WowPlus.Honor.Normal", 10);
        honorByRank[1] = sConfigMgr->GetOption<uint32>("WowPlus.Honor.Elite", 30);
        honorByRank[2] = sConfigMgr->GetOption<uint32>("WowPlus.Honor.Rare", 60);
        honorByRank[3] = sConfigMgr->GetOption<uint32>("WowPlus.Honor.Boss", 300);
        arenaPointsByRank[0] = sConfigMgr->GetOption<float>("WowPlus.ArenaPoints.Normal", 0.2f);
        arenaPointsByRank[1] = sConfigMgr->GetOption<float>("WowPlus.ArenaPoints.Elite", 1.0f);
        arenaPointsByRank[2] = sConfigMgr->GetOption<float>("WowPlus.ArenaPoints.Rare", 3.0f);
        arenaPointsByRank[3] = sConfigMgr->GetOption<float>("WowPlus.ArenaPoints.Boss", 15.0f);
        noRatingRequired = sConfigMgr->GetOption<bool>("WowPlus.ArenaPoints.NoRatingRequired", true);
    }
};

class WowPlusPvPPlayer : public PlayerScript
{
public:
    WowPlusPvPPlayer() : PlayerScript("WowPlusPvPPlayer") { }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        RewardKillHonor(killer, killed);
    }

    void OnPlayerCreatureKilledByPet(Player* petOwner, Creature* killed) override
    {
        RewardKillHonor(petOwner, killed);
    }

    // Arena gear that asks for a personal rating: with points earned from kills there is no rating to have
    void OnPlayerGetMaxPersonalArenaRatingRequirement(Player const* /*player*/, uint32 /*minSlot*/, uint32& maxArenaRating) const override
    {
        if (noRatingRequired)
            maxArenaRating = 5000;
    }
};

void AddSC_WowPlusPvP()
{
    new WowPlusPvPWorld();
    new WowPlusPvPPlayer();
}
