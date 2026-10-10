/*
 * WowPlus professions: one player has to supply every material alone, so gathering gives more.
 *
 * Herbs, ore, skinning and fishing yield a multiple of the normal amount. The number of
 * different items stays the same; only the stack sizes grow.
 */

#include "Config.h"
#include "DBCStores.h"
#include "GameObject.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
    float gatherMultiplier = 3.0f;

    bool IsGatheringNode(GameObject const* go)
    {
        GameObjectTemplate const* info = go->GetGOInfo();
        if (info->type != GAMEOBJECT_TYPE_CHEST)
            return false;

        if (LockEntry const* lock = sLockStore.LookupEntry(info->GetLockId()))
            for (uint8 i = 0; i < MAX_LOCK_CASE; ++i)
                if (lock->Type[i] == LOCK_KEY_SKILL && (lock->Index[i] == LOCKTYPE_HERBALISM || lock->Index[i] == LOCKTYPE_MINING))
                    return true;

        return false;
    }
}

class WowPlusProfessionsWorld : public WorldScript
{
public:
    WowPlusProfessionsWorld() : WorldScript("WowPlusProfessionsWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        gatherMultiplier = sConfigMgr->GetOption<float>("WowPlus.Professions.GatherMultiplier", 3.0f);
    }
};

class WowPlusProfessionsMisc : public MiscScript
{
public:
    WowPlusProfessionsMisc() : MiscScript("WowPlusProfessionsMisc", {MISCHOOK_ON_AFTER_LOOT_TEMPLATE_PROCESS}) { }

    void OnAfterLootTemplateProcess(Loot* loot, LootTemplate const* /*tab*/, LootStore const& store, Player* /*lootOwner*/,
        bool /*personal*/, bool /*noEmptyError*/, uint16 /*lootMode*/) override
    {
        if (gatherMultiplier <= 1.0f)
            return;

        bool const gathering = &store == &LootTemplates_Skinning || &store == &LootTemplates_Fishing ||
            (&store == &LootTemplates_Gameobject && loot->sourceGameObject && IsGatheringNode(loot->sourceGameObject));
        if (!gathering)
            return;

        for (LootItem& item : loot->items)
        {
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(item.itemid);
            if (!proto || proto->GetMaxStackSize() <= 1)
                continue;                                   // a single item that cannot stack, such as a caught weapon

            uint32 const count = uint32(float(item.count) * gatherMultiplier + 0.5f);
            item.count = std::min<uint32>(count, proto->GetMaxStackSize());
        }
    }
};

void AddSC_WowPlusProfessions()
{
    new WowPlusProfessionsWorld();
    new WowPlusProfessionsMisc();
}
