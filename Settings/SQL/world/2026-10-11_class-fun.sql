-- Class changes of 2026-10-11, the part that is data (see Settings/Docs/Class-Changes.md).
-- The rest is in mod-wowplus: src/WowPlusClasses.cpp and src/WowPlusLoot.cpp.
-- Safe to run again.

-- The level at which the class trainers teach a spell, with a price that fits the new level
DROP TEMPORARY TABLE IF EXISTS wp_move;
CREATE TEMPORARY TABLE wp_move (spell INT UNSIGNED PRIMARY KEY, level INT UNSIGNED, cost INT UNSIGNED, what VARCHAR(100));
INSERT INTO wp_move VALUES
    (66842, 20, 2000, 'Shaman: Call of the Elements, all four totems at once (was 30)'),
    (  783, 14,  900, 'Druid: Travel Form (was 16)');
UPDATE trainer_spell ts
    JOIN trainer t ON t.Id = ts.TrainerId AND t.Type = 0
    JOIN wp_move m ON m.spell = ts.SpellId
    SET ts.ReqLevel = m.level, ts.MoneyCost = m.cost;
DROP TEMPORARY TABLE IF EXISTS wp_move;

-- Mage: the polymorph tomes. Two of them were left unfinished by the game (no spell in them);
-- they now teach their polymorph like the others. mod-wowplus lets all six drop for mages.
--   483 = "Learning" (the use), and the second spell is the one that is learned
UPDATE item_template SET spellid_1 = 483, spelltrigger_1 = 0, spellcharges_1 = -1, spellcooldown_1 = -1, spellcategorycooldown_1 = -1,
        spellid_2 = 28272, spelltrigger_2 = 6, spellcharges_2 = 0, spellcooldown_2 = -1, spellcategorycooldown_2 = -1,
        AllowableClass = 128, RequiredLevel = 20
    WHERE entry = 4142;     -- Tome of Polymorph: Pig
UPDATE item_template SET spellid_1 = 483, spelltrigger_1 = 0, spellcharges_1 = -1, spellcooldown_1 = -1, spellcategorycooldown_1 = -1,
        spellid_2 = 61025, spelltrigger_2 = 6, spellcharges_2 = 0, spellcooldown_2 = -1, spellcategorycooldown_2 = -1,
        AllowableClass = 128, RequiredLevel = 20
    WHERE entry = 44600;    -- Tome of Polymorph: Serpent
-- The others asked for level 60; a collection is for the whole journey
UPDATE item_template SET RequiredLevel = 20 WHERE entry IN (22739, 44709, 44793, 44811);