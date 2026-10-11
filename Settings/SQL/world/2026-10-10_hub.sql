-- Phase 8: the hub on GM Island (decided 2026-10-10).
--
-- A lean hub for the solo player: the things one cannot reasonably get or do alone.
-- Eight NPCs stand in a row in front of the manor, facing the spot where the hearthstone
-- menu puts you. Each vendor opens a small menu of lists (the menu and the lists are data in
-- this file; the menu itself is drawn by mod-wowplus, script npc_wowplus_hub_vendor).
--
-- Our own ids:  NPCs 9000001-9000099, their spawns 9000001-9000099,
--               vendor lists 9001000-9001999, trainer 900001.
--
-- Safe to run again: everything it creates is deleted first and rebuilt.
-- To remove the hub: delete this file and run the DELETE statements below by hand,
-- plus: DROP TABLE wowplus_hub_menu;

-- ---------------------------------------------------------------------------------------
-- 1. The NPCs. Each is a copy of an existing NPC (for its looks), made friendly to everyone.
-- ---------------------------------------------------------------------------------------
DELETE FROM creature WHERE guid BETWEEN 9000001 AND 9000099;
DELETE FROM creature_template_model WHERE CreatureID BETWEEN 9000001 AND 9000099;
DELETE FROM creature_template WHERE entry BETWEEN 9000001 AND 9000099;

DROP TEMPORARY TABLE IF EXISTS wp_npc;
CREATE TEMPORARY TABLE wp_npc (entry INT UNSIGNED PRIMARY KEY, source INT UNSIGNED, name VARCHAR(100), subname VARCHAR(100),
    npcflag INT UNSIGNED, script VARCHAR(64), x FLOAT, z FLOAT);
-- npcflag: 129 = talks and sells, 16 = trainer, 2097152 = auctioneer
-- Bram Coalhand sold raw materials at first. Since 2026-10-11 he is the auctioneer instead: materials come
-- from the auction house (mod-ah-bot), which Robin preferred to a vendor with everything.
INSERT INTO wp_npc VALUES
    (9000001,    66, 'Tilda Packwell',   'Crafting Supplies', 129, 'npc_wowplus_hub_vendor', 16205, 13.2),
    (9000002,   514, 'Bram Coalhand',    'Auctioneer',    2097152, '',                       16211, 13.7),
    (9000003, 25263, 'Elowen Quill',     'Glyphs',            129, 'npc_wowplus_hub_vendor', 16217, 13.3),
    (9000004, 28701, 'Marn Facet',       'Cut Gems',          129, 'npc_wowplus_hub_vendor', 16223, 13.2),
    (9000005, 28693, 'Sel Runeweaver',   'Enchant Scrolls',   129, 'npc_wowplus_hub_vendor', 16229, 13.3),
    (9000006, 11867, 'Hanzo Steelwind',  'Weapon Master',      16, '',                       16235, 13.8),
    (9000007, 28997, 'Ysolde Greaves',   'Tier Armor',        129, 'npc_wowplus_hub_vendor', 16241, 14.3),
    (9000008, 12785, 'Corporal Dane',    'PvP Quartermaster', 129, 'npc_wowplus_hub_vendor', 16247, 14.9);

-- one row per hub NPC, even if two would share a source
DROP TEMPORARY TABLE IF EXISTS wp_ct2;
CREATE TEMPORARY TABLE wp_ct2 AS
    SELECT ct.*, n.entry AS wp_entry, n.name AS wp_name, n.subname AS wp_subname, n.npcflag AS wp_npcflag, n.script AS wp_script
    FROM creature_template ct JOIN wp_npc n ON n.source = ct.entry;
UPDATE wp_ct2 SET entry = wp_entry, name = wp_name, subname = wp_subname, npcflag = wp_npcflag, ScriptName = wp_script,
    gossip_menu_id = 0, minlevel = 80, maxlevel = 80, faction = 35, `rank` = 0,
    difficulty_entry_1 = 0, difficulty_entry_2 = 0, difficulty_entry_3 = 0, KillCredit1 = 0, KillCredit2 = 0,
    lootid = 0, pickpocketloot = 0, skinloot = 0, mingold = 0, maxgold = 0, AIName = '', MovementType = 0,
    unit_flags = unit_flags | 2, VerifiedBuild = 0;
ALTER TABLE wp_ct2 DROP COLUMN wp_entry, DROP COLUMN wp_name, DROP COLUMN wp_subname, DROP COLUMN wp_npcflag, DROP COLUMN wp_script;
INSERT INTO creature_template SELECT * FROM wp_ct2;

INSERT INTO creature_template_model (CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild)
    SELECT n.entry, m.Idx, m.CreatureDisplayID, m.DisplayScale, m.Probability, 0
    FROM creature_template_model m JOIN wp_npc n ON n.source = m.CreatureID;

-- The row: 6 yards apart along x, at y = 16268, facing south (towards the arrival point).
INSERT INTO creature (guid, id, map, zoneId, areaId, spawnMask, phaseMask, equipment_id, position_x, position_y, position_z, orientation,
        spawntimesecs, wander_distance, currentwaypoint, curhealth, curmana, MovementType, npcflag, unit_flags, dynamicflags, ScriptName,
        VerifiedBuild, CreateObject, Comment)
    SELECT entry, entry, 1, 0, 0, 1, 1, 0, x, 16268, z, 4.712, 300, 0, 0, 1, 0, 0, 0, 0, 0, '', 0, 0, 'WowPlus hub'
    FROM wp_npc;

-- ---------------------------------------------------------------------------------------
-- 2. The weapon master: one trainer with every weapon skill the game's weapon masters teach.
-- ---------------------------------------------------------------------------------------
DELETE FROM creature_default_trainer WHERE CreatureId BETWEEN 9000001 AND 9000099;
DELETE FROM trainer_spell WHERE TrainerId = 900001;
DELETE FROM trainer WHERE Id = 900001;
INSERT INTO trainer (Id, Type, Requirement, Greeting, VerifiedBuild) VALUES (900001, 2, 0, 'Name the weapon. I teach them all.', 0);
INSERT INTO trainer_spell (TrainerId, SpellId, MoneyCost, ReqSkillLine, ReqSkillRank, ReqAbility1, ReqAbility2, ReqAbility3, ReqLevel, VerifiedBuild)
    SELECT 900001, SpellId, MIN(MoneyCost), 0, 0, 0, 0, 0, MIN(ReqLevel), 0
    FROM trainer_spell
    WHERE TrainerId IN (SELECT TrainerId FROM (SELECT DISTINCT TrainerId FROM trainer_spell WHERE SpellId IN (201, 196, 198, 227, 264, 5011, 1180, 266, 2567, 15590, 200, 197, 199, 202)) t)
    GROUP BY SpellId;
INSERT INTO creature_default_trainer (CreatureId, TrainerId) VALUES (9000006, 900001);

-- ---------------------------------------------------------------------------------------
-- 3. The vendor lists and the menu that leads to them.
-- ---------------------------------------------------------------------------------------
DROP TABLE IF EXISTS wowplus_hub_menu;
CREATE TABLE wowplus_hub_menu (
    npc_entry INT UNSIGNED NOT NULL,
    sort INT UNSIGNED NOT NULL,
    label VARCHAR(100) NOT NULL,
    vendor_entry INT UNSIGNED NOT NULL COMMENT 'a list in npc_vendor: one of ours (9001000+) or the entry of one of the game''s own vendors',
    team CHAR(1) NOT NULL DEFAULT '' COMMENT 'A = only shown to Alliance characters, H = only to Horde, empty = to everyone',
    class_mask INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'only shown to these classes (1 warrior, 2 paladin, 4 hunter, 8 rogue, 16 priest, 32 death knight, 64 shaman, 128 mage, 256 warlock, 1024 druid; add them up); 0 = everyone',
    PRIMARY KEY (npc_entry, sort)
) COMMENT = 'WowPlus: the menu of each hub vendor; each line opens one vendor list';
DELETE FROM npc_vendor WHERE entry BETWEEN 9001000 AND 9001999;

-- Every list is first collected here, then numbered into npc_vendor.
DROP TEMPORARY TABLE IF EXISTS wp_list;
CREATE TEMPORARY TABLE wp_list (list INT UNSIGNED NOT NULL, item INT UNSIGNED NOT NULL, PRIMARY KEY (list, item));

-- Crafting supplies ---------------------------------------------------------------------
INSERT INTO wowplus_hub_menu (npc_entry, sort, label, vendor_entry) VALUES
    (9000001, 1, 'Enchanting rods and vellums', 9001001),
    (9000001, 2, 'Trade supplies: vials, thread, flux, dyes and the rest', 9001002);
-- the rods a blacksmith makes for enchanters, and the vellums of inscription
INSERT IGNORE INTO wp_list SELECT 9001001, entry FROM item_template
    WHERE entry IN (6217, 6338, 11128, 11144, 16206, 25843, 25844, 25845, 41745, 38682, 37602, 43145, 39349, 39350, 43146);
-- every trade good that some vendor in the world sells for gold
INSERT IGNORE INTO wp_list SELECT DISTINCT 9001002, v.item FROM npc_vendor v JOIN item_template i ON i.entry = v.item
    WHERE v.entry < 9000000 AND v.ExtendedCost = 0 AND i.class = 7;

-- Materials -----------------------------------------------------------------------------
-- No longer offered (see the note at the top): the lists are still built below but nobody sells them.
INSERT IGNORE INTO wp_list SELECT 9001011, entry FROM item_template
    WHERE class = 7 AND subclass = 7 AND name LIKE '% Bar' AND NOT REGEXP_LIKE(name, 'Test|Deprecated|OLD|Unused', 'c');
INSERT IGNORE INTO wp_list SELECT 9001012, entry FROM item_template
    WHERE class = 7 AND subclass = 5 AND (name LIKE 'Bolt of %' OR entry IN (14342, 24271, 24272, 21845, 41594, 41593, 41595))
      AND NOT REGEXP_LIKE(name, 'Test|Deprecated|OLD|Unused', 'c');
INSERT IGNORE INTO wp_list SELECT 9001013, entry FROM item_template
    WHERE class = 7 AND subclass = 6 AND (name LIKE 'Cured %' OR entry IN (23793, 38425))
      AND NOT REGEXP_LIKE(name, 'Test|Deprecated|OLD|Unused', 'c');
INSERT IGNORE INTO wp_list SELECT 9001014, entry FROM item_template
    WHERE class = 7 AND subclass = 1 AND NOT REGEXP_LIKE(name, 'Test|Deprecated|OLD|Unused', 'c');
INSERT IGNORE INTO wp_list SELECT 9001015, entry FROM item_template
    WHERE class = 7 AND subclass = 10 AND NOT REGEXP_LIKE(name, 'Test|Deprecated|OLD|Unused', 'c');
-- Black Lotus, Arcane Crystal, Righteous Orb, Dark Iron Ore, Fel Lotus, Frost Lotus, Titanium Ore
INSERT IGNORE INTO wp_list SELECT 9001016, entry FROM item_template
    WHERE entry IN (13468, 12363, 12811, 11370, 22794, 36908, 36910);

-- Glyphs, one list per class (the item subclass of a glyph is the class id) -----------------
INSERT INTO wowplus_hub_menu (npc_entry, sort, label, vendor_entry) VALUES
    (9000003, 1, 'Warrior', 9001101), (9000003, 2, 'Paladin', 9001102), (9000003, 3, 'Hunter', 9001103),
    (9000003, 4, 'Rogue', 9001104), (9000003, 5, 'Priest', 9001105), (9000003, 6, 'Death Knight', 9001106),
    (9000003, 7, 'Shaman', 9001107), (9000003, 8, 'Mage', 9001108), (9000003, 9, 'Warlock', 9001109),
    (9000003, 10, 'Druid', 9001111);
INSERT IGNORE INTO wp_list SELECT 9001100 + subclass, entry FROM item_template
    WHERE class = 16 AND subclass BETWEEN 1 AND 11 AND NOT REGEXP_LIKE(name, 'Test|Deprecated|OLD|Unused', 'c');

-- Cut gems, one list per colour (the item subclass of a gem is its colour) ------------------
INSERT INTO wowplus_hub_menu (npc_entry, sort, label, vendor_entry) VALUES
    (9000004, 1, 'Red', 9001200), (9000004, 2, 'Blue', 9001201), (9000004, 3, 'Yellow', 9001202),
    (9000004, 4, 'Purple', 9001203), (9000004, 5, 'Green', 9001204), (9000004, 6, 'Orange', 9001205),
    (9000004, 7, 'Meta', 9001206), (9000004, 8, 'Prismatic', 9001208);
INSERT IGNORE INTO wp_list SELECT 9001200 + subclass, entry FROM item_template
    WHERE class = 3 AND subclass IN (0, 1, 2, 3, 4, 5, 6, 8) AND GemProperties > 0 AND Quality BETWEEN 2 AND 4
      AND NOT REGEXP_LIKE(name, 'Test|Deprecated|OLD|Unused', 'c');

-- Enchant scrolls, one list per slot (taken from the name: "Scroll of Enchant <Slot> - ...") --
DROP TEMPORARY TABLE IF EXISTS wp_scroll;
CREATE TEMPORARY TABLE wp_scroll AS
    SELECT entry, TRIM(SUBSTRING_INDEX(SUBSTRING(name, 19), ' - ', 1)) AS slot FROM item_template
    WHERE name LIKE 'Scroll of Enchant % - %' AND NOT REGEXP_LIKE(name, 'Test|Deprecated|OLD|Unused', 'c');
DROP TEMPORARY TABLE IF EXISTS wp_slot;
CREATE TEMPORARY TABLE wp_slot AS
    SELECT slot, ROW_NUMBER() OVER (ORDER BY slot) AS n FROM (SELECT DISTINCT slot FROM wp_scroll) s;
INSERT INTO wowplus_hub_menu (npc_entry, sort, label, vendor_entry) SELECT 9000005, n, slot, 9001300 + n FROM wp_slot;
INSERT IGNORE INTO wp_list SELECT 9001300 + s.n, sc.entry FROM wp_scroll sc JOIN wp_slot s ON s.slot = sc.slot;

-- Tier armor ----------------------------------------------------------------------------
-- These lines open the lists of the game's own token vendors, with their own prices
-- (tokens from raid bosses, sometimes emblems). Nothing is copied, so they stay correct
-- if the game data changes. A line is only shown to the classes that can use it.
--   armour types as class masks: cloth 400, leather 1032, mail 68, plate 35
INSERT INTO wowplus_hub_menu (npc_entry, sort, label, vendor_entry, team, class_mask) VALUES
    (9000007,  1, 'Tier 4: Karazhan, Gruul, Magtheridon', 20616, '', 0),
    (9000007,  2, 'Tier 5: Serpentshrine Cavern, Tempest Keep', 21906, '', 0),
    (9000007,  3, 'Tier 6: Mount Hyjal, Black Temple', 23381, '', 0),
    (9000007,  4, 'Tier 6: Sunwell Plateau', 25976, '', 0),
    (9000007,  5, 'Tier 7 and 8: Naxxramas, Ulduar', 28995, '', 400),
    (9000007,  6, 'Tier 7 and 8: Naxxramas, Ulduar', 28992, '', 1032),
    (9000007,  7, 'Tier 7 and 8: Naxxramas, Ulduar', 29523, '', 68),
    (9000007,  8, 'Tier 7 and 8: Naxxramas, Ulduar', 28997, '', 35),
    (9000007,  9, 'Tier 9: Trial of the Crusader', 35577, 'A', 0),
    (9000007, 10, 'Tier 9: Trial of the Crusader', 35578, 'H', 0),
    (9000007, 11, 'Tier 9: Trial of the Grand Crusader', 35575, 'A', 0),
    (9000007, 12, 'Tier 9: Trial of the Grand Crusader', 35576, 'H', 0),
    (9000007, 13, 'Tier 9 and 10: emblem pieces', 35496, '', 400),
    (9000007, 14, 'Tier 9 and 10: emblem pieces', 35497, '', 1032),
    (9000007, 15, 'Tier 9 and 10: emblem pieces', 35500, '', 68),
    (9000007, 16, 'Tier 9 and 10: emblem pieces', 35498, '', 35),
    (9000007, 17, 'Tier 10: Icecrown Citadel', 37688, '', 1),
    (9000007, 18, 'Tier 10: Icecrown Citadel', 37696, '', 2),
    (9000007, 19, 'Tier 10: Icecrown Citadel', 37993, '', 4),
    (9000007, 20, 'Tier 10: Icecrown Citadel', 37991, '', 8),
    (9000007, 21, 'Tier 10: Icecrown Citadel', 38054, '', 16),
    (9000007, 22, 'Tier 10: Icecrown Citadel', 38316, '', 32),
    (9000007, 23, 'Tier 10: Icecrown Citadel', 38841, '', 64),
    (9000007, 24, 'Tier 10: Icecrown Citadel', 38284, '', 128),
    (9000007, 25, 'Tier 10: Icecrown Citadel', 38181, '', 256),
    (9000007, 26, 'Tier 10: Icecrown Citadel', 37992, '', 1024);

-- PvP gear ------------------------------------------------------------------------------
-- Also the game's own vendors and prices: honor, and for the arena seasons arena points.
-- Both are earned from ordinary kills (mod-wowplus, settings WowPlus.Honor.* and
-- WowPlus.ArenaPoints.*), and no arena rating is asked for (WowPlus.Hub.NoRatingRequired).
INSERT INTO wowplus_hub_menu (npc_entry, sort, label, vendor_entry, team, class_mask) VALUES
    (9000008,  1, 'Level 60: armor', 12785, 'A', 0),
    (9000008,  2, 'Level 60: armor', 12795, 'H', 0),
    (9000008,  3, 'Level 60: weapons', 12784, 'A', 0),
    (9000008,  4, 'Level 60: weapons', 12794, 'H', 0),
    (9000008,  5, 'Trinkets, insignia and accessories', 12781, 'A', 0),
    (9000008,  6, 'Trinkets, insignia and accessories', 12793, 'H', 0),
    (9000008,  7, 'War mounts', 12783, 'A', 0),
    (9000008,  8, 'War mounts', 12796, 'H', 0),
    (9000008,  9, 'Level 70: honor gear', 12778, 'A', 0),
    (9000008, 10, 'Level 70: honor gear', 12788, 'H', 0),
    (9000008, 11, 'Level 70: Vengeful Gladiator (arena)', 19857, '', 0),
    (9000008, 12, 'Level 70: Brutal Gladiator (arena)', 20278, '', 0),
    (9000008, 13, 'Level 80: Savage Gladiator', 32834, 'A', 0),
    (9000008, 14, 'Level 80: Savage Gladiator', 32832, 'H', 0),
    (9000008, 15, 'Level 80: Hateful Gladiator', 32381, 'A', 0),
    (9000008, 16, 'Level 80: Hateful Gladiator', 32383, 'H', 0),
    (9000008, 17, 'Level 80: Deadly Gladiator (honor)', 32380, 'A', 0),
    (9000008, 18, 'Level 80: Deadly Gladiator (honor)', 32385, 'H', 0),
    (9000008, 19, 'Level 80: Deadly Gladiator (arena)', 31863, '', 0),
    (9000008, 20, 'Level 80: Furious Gladiator (honor)', 34076, 'A', 0),
    (9000008, 21, 'Level 80: Furious Gladiator (honor)', 34058, 'H', 0),
    (9000008, 22, 'Level 80: Furious Gladiator (arena)', 33921, '', 0),
    (9000008, 23, 'Level 80: Furious weapons (arena)', 34087, '', 0),
    (9000008, 24, 'Level 80: Relentless Gladiator (honor)', 34075, 'A', 0),
    (9000008, 25, 'Level 80: Relentless Gladiator (honor)', 34038, 'H', 0),
    (9000008, 26, 'Level 80: Relentless Gladiator (arena)', 33927, '', 0),
    (9000008, 27, 'Level 80: Relentless weapons (arena)', 34090, '', 0),
    (9000008, 28, 'Level 80: Wrathful Gladiator (honor)', 34078, 'A', 0),
    (9000008, 29, 'Level 80: Wrathful Gladiator (honor)', 34060, 'H', 0),
    (9000008, 30, 'Level 80: Wrathful Gladiator (arena)', 33936, '', 0),
    (9000008, 31, 'Level 80: Wrathful weapons (arena)', 34093, '', 0);

-- Clean-up over all lists ----------------------------------------------------------------
-- The game's "parts" category also holds the inks of inscription: they get a line of their own.
UPDATE wp_list l JOIN item_template i ON i.entry = l.item SET l.list = 9001017
    WHERE l.list = 9001014 AND (i.name LIKE '% Ink' OR i.name LIKE 'Ink of %');
-- Leftovers the game never hands out: nameless "NPC Equip" props, grey junk, broken test items.
-- (Found in the hub on 2026-10-10: the engineering parts started with eight "NPC Equip" entries.)
DELETE l FROM wp_list l JOIN item_template i ON i.entry = l.item
    WHERE i.Quality = 0 OR i.name LIKE 'NPC Equip%' OR i.name LIKE 'Broken % Elixir'
       OR (i.ItemLevel = 0 AND l.list <> 9001002);

-- Number the lists into npc_vendor: cheapest content first, 150 items at most per list
-- (the game cannot show more in one vendor window).
INSERT INTO npc_vendor (entry, slot, item, maxcount, incrtime, ExtendedCost, VerifiedBuild)
    SELECT list, n, item, 0, 0, 0, 0 FROM (
        SELECT l.list, l.item, ROW_NUMBER() OVER (PARTITION BY l.list ORDER BY i.RequiredLevel, i.ItemLevel, i.name) AS n
        FROM wp_list l JOIN item_template i ON i.entry = l.item) numbered
    WHERE n <= 150;

-- ---------------------------------------------------------------------------------------
-- 4. Prices. A vendor charges the item's own "buy price". Most of what is sold here is
--    normally made or found by players and has only a token price, or none at all.
--
--    Price = (item level squared) x a factor for the rarity, in copper:
--        common 10, uncommon 40, rare 150, epic 600
--    and never less than four times what a vendor pays for the item, so that buying
--    here and selling again always loses money.
--        Examples: Titansteel Bar 25g 60s, Arcanite Bar 12g, Mooncloth 3g, Primal Might 63g,
--                  an epic level 80 cut gem 384g, a rare one 96g, a glyph 2g to 6g.
--    Enchant scrolls count four times (the materials in them cost more than the paper),
--    the "rare finds" three times.
--    The trade supplies keep the game's own prices.
--
--    These numbers are first guesses for the "comfortable" gold level, to be tuned.
-- ---------------------------------------------------------------------------------------
UPDATE item_template i
    JOIN (SELECT item, MIN(list) AS list FROM wp_list WHERE list <> 9001002 GROUP BY item) l ON l.item = i.entry
    SET i.BuyPrice = GREATEST(
        50,
        i.SellPrice * 4,
        ROUND(POW(GREATEST(i.ItemLevel, 1), 2)
            * CASE i.Quality WHEN 0 THEN 5 WHEN 1 THEN 10 WHEN 2 THEN 40 WHEN 3 THEN 150 ELSE 600 END
            * CASE WHEN l.list BETWEEN 9001300 AND 9001399 THEN 4 WHEN l.list = 9001016 THEN 3 ELSE 1 END));

DROP TEMPORARY TABLE IF EXISTS wp_npc;
DROP TEMPORARY TABLE IF EXISTS wp_ct2;
DROP TEMPORARY TABLE IF EXISTS wp_list;
DROP TEMPORARY TABLE IF EXISTS wp_scroll;
DROP TEMPORARY TABLE IF EXISTS wp_slot;
