-- A room of your own: the keys, the furniture, and who sells them (decided 2026-10-11).
-- See Settings/Docs/Housing.md. The rules are in mod-wowplus, src/WowPlusHome.cpp; this file is the data.
--
-- The game shows an icon only for items it already knows, so the keys and the furniture are not new item
-- numbers: they take over items the game has but never hands out (old test lockboxes and deprecated keys).
-- New numbers showed a question mark instead of an icon.
--
-- - A key for each city, sold by that city's innkeepers. Using it lets
--   you claim any spot in that city as your room, and later brings you back there.
-- - Furniture, sold by every city innkeeper. Using a piece inside your
--   room puts it down where you stand. Each piece is an object of the game world; a few are
--   new objects made here from models the game already has (a bed, a bookshelf, a storage chest).
-- Prices are in copper. Change them here and restart. Safe to run again.

-- The cities ---------------------------------------------------------------------------------
DROP TABLE IF EXISTS wowplus_home_cities;
CREATE TABLE wowplus_home_cities (
    seq   INT UNSIGNED NOT NULL PRIMARY KEY,
    item  INT UNSIGNED NOT NULL COMMENT 'the key',
    zone  INT UNSIGNED NOT NULL,
    name  VARCHAR(40) NOT NULL,
    price INT UNSIGNED NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
INSERT INTO wowplus_home_cities VALUES
    ( 1,  2363, 1637, 'Orgrimmar',        30000),
    ( 2,  3580, 1638, 'Thunder Bluff',    20000),
    ( 3,  1918, 1497, 'the Undercity',    30000),
    ( 4, 11085, 3487, 'Silvermoon City',  30000),
    ( 5,  2789, 1519, 'Stormwind',        30000),
    ( 6,  2790, 1537, 'Ironforge',        30000),
    ( 7,  2791, 1657, 'Darnassus',        20000),
    ( 8,  2792, 3557, 'the Exodar',       20000),
    ( 9,  1253, 3703, 'Shattrath',       500000),
    (10,  5916, 4395, 'Dalaran',        1500000);

-- The furniture ------------------------------------------------------------------------------
-- object: the game object that is put down. 9200001 and up are made below.
DROP TABLE IF EXISTS wowplus_furniture;
CREATE TABLE wowplus_furniture (
    seq    INT UNSIGNED NOT NULL PRIMARY KEY,
    item   INT UNSIGNED NOT NULL,
    object INT UNSIGNED NOT NULL,
    name   VARCHAR(60) NOT NULL,
    price  INT UNSIGNED NOT NULL,
    note   VARCHAR(120) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
INSERT INTO wowplus_furniture VALUES
    ( 1, 16068, 9200001, 'Storage Chest',        5000, 'Opens your bank.'),
    ( 2, 16069,  187289, 'Guild Vault',         20000, 'Opens the vault of your guild.'),
    ( 3, 16070,   32349, 'Mailbox',              5000, 'Your mail, at home.'),
    ( 4, 16071, 9200002, 'Bed',                 10000, 'Lie down for a moment and wake up rested.'),
    ( 5, 16074,    3797, 'Cozy Fire',            3000, 'A fire to sit by, and to cook on.'),
    ( 6, 16075,    1744, 'Anvil',               10000, 'For smithing at home.'),
    ( 7, 16076,    1685, 'Forge',               20000, 'For smithing at home.'),
    ( 8, 16077,    2413, 'Wooden Chair',          500, 'To sit on.'),
    ( 9, 16078,    2489, 'High Back Chair',      2000, 'To sit on, grandly.'),
    (10, 16079,   24538, 'Wooden Bench',         1000, 'To sit on, with company.'),
    (11, 16080,  182593, 'Stool',                 300, 'To sit on, briefly.'),
    (12, 16081,  181075, 'Table',                1500, ''),
    (13, 16109,  181077, 'Rug',                  2000, ''),
    (14, 16173,  188346, 'Tauren Rug',           4000, ''),
    (15, 16174,  180473, 'Brazier',              2500, ''),
    (16, 16175,  180765, 'Lantern',              1500, ''),
    (17, 16176,  188436, 'Candles',              1000, ''),
    (18, 16177, 9200005, 'Candle',                300, ''),
    (19, 16178, 9200003, 'Bookshelf',            6000, ''),
    (20, 16179, 9200004, 'Wardrobe',             8000, ''),
    (21, 16180,  183269, 'Weapon Rack',          5000, ''),
    (22, 16181,  186281, 'Barrel',                500, ''),
    (23, 16182, 9200010, 'Beer Keg',             3000, ''),
    (24, 16183, 9200006, 'Pillow',                500, ''),
    (25, 16184, 9200007, 'Globe',               10000, ''),
    (26, 16185, 9200008, 'Painting',             7500, ''),
    (27, 16186, 9200009, 'Flower Planter',       2000, ''),
    (28, 16187,  152079, 'Horde Banner',         3000, ''),
    (29, 16188,  194278, 'Orgrimmar Banner',     4000, ''),
    (30, 17910,  194283, 'Thunder Bluff Banner', 4000, ''),
    (31, 17911,  194276, 'Undercity Banner',     4000, ''),
    (32, 20364,  194275, 'Silvermoon Banner',    4000, ''),
    (33, 22045,  192252, 'Alliance Banner',      3000, '');

-- New objects from models the game has. Type 10 answers to being used (by a script), type 5 just stands there.
DELETE FROM gameobject_template WHERE entry BETWEEN 9200000 AND 9299999;
INSERT INTO gameobject_template (entry, type, displayId, name, IconName, castBarCaption, unk1, size, ScriptName) VALUES
    (9200001, 10,  259, 'Storage Chest',  '', '', '', 1, 'go_wowplus_home_bank'),
    (9200002, 10,  392, 'Bed',            '', '', '', 1, 'go_wowplus_home_bed'),
    (9200003,  5,  137, 'Bookshelf',      '', '', '', 1, ''),
    (9200004,  5,    4, 'Wardrobe',       '', '', '', 1, ''),
    (9200005,  5,  100, 'Candle',         '', '', '', 1, ''),
    (9200006,  5, 5911, 'Pillow',         '', '', '', 1, ''),
    (9200007,  5, 5265, 'Globe',          '', '', '', 1, ''),
    (9200008,  5, 4731, 'Painting',       '', '', '', 1, ''),
    (9200009,  5, 8559, 'Flower Planter', '', '', '', 1, ''),
    (9200010,  5, 1727, 'Beer Keg',       '', '', '', 1, '');

-- The items ------------------------------------------------------------------------------------
-- Copies of a plain item, changed into something you use. The "use" is a spell that does
-- nothing (13567); the script takes over before it is cast.
DELETE FROM item_template WHERE entry BETWEEN 9200000 AND 9299999;
DELETE FROM item_template WHERE entry IN (SELECT item FROM wowplus_furniture) OR entry IN (SELECT item FROM wowplus_home_cities);
DROP TEMPORARY TABLE IF EXISTS wp_item;
CREATE TEMPORARY TABLE wp_item AS
    SELECT i.*, f.item AS wp_entry, f.name AS wp_name, f.price AS wp_price,
           CONCAT('Furniture. Use it inside your room to put it down where you stand. ', f.note) AS wp_text,
           'item_wowplus_furniture' AS wp_script, 20 AS wp_stack, 0 AS wp_max,
           (SELECT displayid FROM item_template WHERE entry = IF(f.seq = 1, 6353, 4470)) AS wp_icon
    FROM item_template i JOIN wowplus_furniture f WHERE i.entry = 4470;
INSERT INTO wp_item
    SELECT i.*, c.item, CONCAT('Key to a Room in ', REPLACE(c.name, 'the ', '')), c.price,
           CONCAT('Use it in ', c.name, ' to make the place where you stand your own room. After that it brings you back there, and helps you move your furniture.'),
           'item_wowplus_room_key', 1, 1, (SELECT displayid FROM item_template WHERE entry = 13704)
    FROM item_template i JOIN wowplus_home_cities c WHERE i.entry = 4470;
UPDATE wp_item SET entry = wp_entry, class = 15, subclass = 0, name = wp_name, displayid = wp_icon, Quality = 1,
    BuyCount = 1, BuyPrice = wp_price, SellPrice = wp_price DIV 4, stackable = wp_stack, maxcount = wp_max, bonding = 0,
    spellid_1 = 13567, spelltrigger_1 = 0, spellcharges_1 = 0, spellcooldown_1 = 1500, spellcategory_1 = 0, spellcategorycooldown_1 = -1,
    description = wp_text, ScriptName = wp_script, ItemLevel = 1, RequiredLevel = 0;
ALTER TABLE wp_item DROP COLUMN wp_entry, DROP COLUMN wp_name, DROP COLUMN wp_price, DROP COLUMN wp_text,
    DROP COLUMN wp_script, DROP COLUMN wp_stack, DROP COLUMN wp_max, DROP COLUMN wp_icon;
INSERT INTO item_template SELECT * FROM wp_item;
DROP TEMPORARY TABLE wp_item;

-- The game client decides the kind, material and icon of an item it knows, and the server checks ours
-- against it at every start. Give each the values the client has, so the log stays clean.
UPDATE item_template SET Material = 1, displayid = 9632 WHERE entry IN (SELECT item FROM wowplus_furniture);
UPDATE item_template SET class = 13,
    subclass  = IF(entry IN (1253, 1918, 2789, 2790, 2791, 2792), 1, 0),
    Material  = CASE entry WHEN 1253 THEN 1 WHEN 2363 THEN 0 WHEN 2789 THEN 4 WHEN 2790 THEN 4 WHEN 2791 THEN 4 WHEN 2792 THEN 4 ELSE Material END,
    displayid = CASE entry WHEN 1253 THEN 6707 WHEN 1918 THEN 6706 WHEN 2363 THEN 6708 WHEN 2789 THEN 6707 WHEN 2790 THEN 6707
                           WHEN 2791 THEN 6707 WHEN 2792 THEN 6707 WHEN 3580 THEN 6713 WHEN 5916 THEN 7737 WHEN 11085 THEN 4287 ELSE displayid END
    WHERE entry IN (SELECT item FROM wowplus_home_cities);

-- Who sells them -------------------------------------------------------------------------------
-- The innkeepers standing in each city: that city's key, and all the furniture.
DELETE FROM npc_vendor WHERE item BETWEEN 9200000 AND 9299999;
DELETE FROM npc_vendor WHERE item IN (SELECT item FROM wowplus_furniture) OR item IN (SELECT item FROM wowplus_home_cities);
DROP TEMPORARY TABLE IF EXISTS wp_inns;
CREATE TEMPORARY TABLE wp_inns AS
    SELECT DISTINCT c.id AS npc, c.zoneId AS zone
    FROM creature c JOIN creature_template ct ON ct.entry = c.id
    WHERE (ct.npcflag & 65536) AND c.zoneId IN (SELECT zone FROM wowplus_home_cities) AND ct.entry < 9000000;
INSERT INTO npc_vendor (entry, slot, item, maxcount, incrtime, ExtendedCost, VerifiedBuild)
    SELECT i.npc, 0, c.item, 0, 0, 0, 0 FROM wp_inns i JOIN wowplus_home_cities c ON c.zone = i.zone;
INSERT INTO npc_vendor (entry, slot, item, maxcount, incrtime, ExtendedCost, VerifiedBuild)
    SELECT i.npc, 0, f.item, 0, 0, 0, 0 FROM wp_inns i JOIN wowplus_furniture f;
-- An innkeeper who sold nothing so far becomes a vendor (flag 128). This stays set.
UPDATE creature_template ct JOIN wp_inns i ON i.npc = ct.entry SET ct.npcflag = ct.npcflag | 128 WHERE (ct.npcflag & 128) = 0;
DROP TEMPORARY TABLE wp_inns;

-- Keys and furniture bought on the first day had item numbers of their own (9200101 and up). Turn those
-- into the numbers used now, in bags and in rooms. Does nothing once done.
UPDATE acore_characters.item_instance ii JOIN wowplus_furniture f ON ii.itemEntry = 9200100 + f.seq SET ii.itemEntry = f.item;
UPDATE acore_characters.item_instance ii JOIN wowplus_home_cities c ON ii.itemEntry = 9200200 + c.seq SET ii.itemEntry = c.item;
UPDATE acore_characters.wowplus_home_objects o JOIN wowplus_furniture f ON o.item = 9200100 + f.seq SET o.item = f.item;
