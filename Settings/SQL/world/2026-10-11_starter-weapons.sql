-- The weaponsmiths of the starting areas sell every basic weapon (decided 2026-10-11).
--
-- Robin: "Add each starter weapon to the weapon vendors in the starting zones, so that you
-- don't actually start with a wand or an extra dagger, but have to purchase them. Update so
-- every class can buy what grey/white weapon they need."
--
-- So a rogue buys the second dagger for Dual Wield, a caster buys the first wand (usable from
-- level 5), and anyone can try another weapon type for a few copper.
--
-- Vendors: Janos Hammerknuckle (Northshire), Rybrad Coldbank (Coldridge Valley),
-- Harold Raims (Deathknell), Marjak Keenblade (Camp Narache), Kzan Thornslash (Valley of
-- Trials), Khardan Proudblade (Shadowglen), Raelis Dawnstar (Sunstrider Isle), Mura (Ammen Vale).
--
-- Safe to run again: rows that exist are left alone.
-- To undo: DELETE FROM npc_vendor WHERE entry IN (78, 945, 2117, 3073, 3159, 3588, 15289, 16919)
--          AND item IN (<the list below>)  -- but check first what each sold originally.
INSERT IGNORE INTO npc_vendor (entry, slot, item, maxcount, incrtime, ExtendedCost, VerifiedBuild)
    SELECT v.entry, 0, i.entry, 0, 0, 0, 0
    FROM (SELECT 78 AS entry UNION SELECT 945 UNION SELECT 2117 UNION SELECT 3073 UNION SELECT 3159
          UNION SELECT 3588 UNION SELECT 15289 UNION SELECT 16919) v
    JOIN item_template i ON i.entry IN (
        25,     -- Worn Shortsword
        36,     -- Worn Mace
        37,     -- Worn Axe
        2092,   -- Worn Dagger
        35,     -- Bent Staff
        49778,  -- Worn Greatsword
        12282,  -- Worn Battleaxe
        2361,   -- Battleworn Hammer
        2504,   -- Worn Shortbow
        2508,   -- Old Blunderbuss
        2512,   -- Rough Arrow
        2516,   -- Light Shot
        25861,  -- Crude Throwing Axe
        28979,  -- Light Throwing Knife
        2362,   -- Worn Wooden Shield
        11287   -- Lesser Magic Wand (needs level 5)
    )
    JOIN creature_template ct ON ct.entry = v.entry;