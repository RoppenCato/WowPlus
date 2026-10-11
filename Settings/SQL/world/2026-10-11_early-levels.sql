-- Phase 7: early-level class changes (decided 2026-10-11, see Settings/Docs/Class-Changes.md).
--
-- Each row moves the level at which the class trainers teach one spell, and sets a price that
-- fits the new level (the old price was sized for the old level: Shadowfiend cost almost 9 gold).
-- Nothing about what the spells do is changed.
--
-- Safe to run again. To undo: the original levels and prices are in
-- Server/data/sql/base/db_world/trainer_spell.sql and item_template.sql.

DROP TEMPORARY TABLE IF EXISTS wp_move;
CREATE TEMPORARY TABLE wp_move (spell INT UNSIGNED PRIMARY KEY, level INT UNSIGNED, cost INT UNSIGNED, what VARCHAR(100));
INSERT INTO wp_move VALUES
    -- Druid: the cat arrives at 8 and gets a finishing move early. Bear Form stays at 10.
    (  768,  8,   200, 'Cat Form (was 20)'),
    ( 1082,  8,   200, 'Claw rank 1 (was 20)'),
    ( 1822, 10,   400, 'Rake rank 1 (was 24)'),
    (22568, 12,   600, 'Ferocious Bite rank 1 (was 32): the first finishing move'),
    ( 1079, 16,  1500, 'Rip rank 1 (was 20): the second finishing move, after Ferocious Bite'),
    ( 6785, 20,  2000, 'Ravage rank 1 (was 32): needs stealth, and Prowl is learned at 20'),
    -- Warrior
    ( 2687,  8,   200, 'Bloodrage (was 10)'),
    -- Paladin
    (26573, 14,   900, 'Consecration rank 1 (was 20)'),
    (  879, 16,  1500, 'Exorcism rank 1 (was 20)'),
    -- Shaman
    (52127, 14,   900, 'Water Shield rank 1 (was 20)'),
    -- Priest: something exciting at 10
    (34433, 10,   400, 'Shadowfiend (was 66)'),
    -- Mage: conjured water and food one step ahead. Each rank is learned where the rank before
    -- it used to be, so the mage always makes the next tier instead of the last one.
    ( 5505,  6,   100, 'Conjure Water rank 2 (was 10)'),
    ( 5506, 10,   400, 'Conjure Water rank 3 (was 20)'),
    ( 6127, 20,  2000, 'Conjure Water rank 4 (was 30)'),
    (10138, 30,  8000, 'Conjure Water rank 5 (was 40)'),
    (10139, 40, 15000, 'Conjure Water rank 6 (was 50)'),
    (10140, 50, 32000, 'Conjure Water rank 7 (was 60)'),
    (37420, 60, 42000, 'Conjure Water rank 8 (was 65)'),
    (27090, 65, 70000, 'Conjure Water rank 9 (was 70)'),
    (  597,  8,   200, 'Conjure Food rank 2 (was 12)'),
    (  990, 12,   600, 'Conjure Food rank 3 (was 22)'),
    ( 6129, 22,  3000, 'Conjure Food rank 4 (was 32)'),
    (10144, 32, 10000, 'Conjure Food rank 5 (was 42)'),
    (10145, 42, 18000, 'Conjure Food rank 6 (was 52)'),
    (28612, 52, 35000, 'Conjure Food rank 7 (was 60)'),
    (33717, 60, 42000, 'Conjure Food rank 8 (was 70)');

-- Only class trainers (type 0): Dual Wield and the like are not touched here.
UPDATE trainer_spell ts
    JOIN trainer t ON t.Id = ts.TrainerId AND t.Type = 0
    JOIN wp_move m ON m.spell = ts.SpellId
    SET ts.ReqLevel = m.level, ts.MoneyCost = m.cost;

DROP TEMPORARY TABLE IF EXISTS wp_move;

-- Rogue: Dual Wield from the start. The rogue trainers (requirement 4) offer it at level 1 for
-- free; mod-wowplus also teaches it at login and adds a second dagger. Warriors and hunters
-- keep theirs at 20.
UPDATE trainer_spell ts JOIN trainer t ON t.Id = ts.TrainerId AND t.Type = 0 AND t.Requirement = 4
    SET ts.ReqLevel = 1, ts.MoneyCost = 0 WHERE ts.SpellId = 674;

-- The conjured food and water carry a level requirement of their own. Lowered to the level
-- at which the mage now learns to make them, or a mage could conjure what it cannot consume.
UPDATE item_template SET RequiredLevel = 10 WHERE entry = 2136;   -- Conjured Purified Water (was 15)
UPDATE item_template SET RequiredLevel = 20 WHERE entry = 3772;   -- Conjured Spring Water (was 25)
UPDATE item_template SET RequiredLevel = 30 WHERE entry = 8077;   -- Conjured Mineral Water (was 35)
UPDATE item_template SET RequiredLevel = 40 WHERE entry = 8078;   -- Conjured Sparkling Water (was 45)
UPDATE item_template SET RequiredLevel = 50 WHERE entry = 8079;   -- Conjured Crystal Water (was 55)
UPDATE item_template SET RequiredLevel = 12 WHERE entry = 1114;   -- Conjured Rye (was 15)
UPDATE item_template SET RequiredLevel = 22 WHERE entry = 1487;   -- Conjured Pumpernickel (was 25)
UPDATE item_template SET RequiredLevel = 32 WHERE entry = 8075;   -- Conjured Sourdough (was 35)
UPDATE item_template SET RequiredLevel = 42 WHERE entry = 8076;   -- Conjured Sweet Roll (was 45)
UPDATE item_template SET RequiredLevel = 52 WHERE entry = 22895;  -- Conjured Cinnamon Roll (was 55)
UPDATE item_template SET RequiredLevel = 60 WHERE entry = 22019;  -- Conjured Croissant (was 65)
