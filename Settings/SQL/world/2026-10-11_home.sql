-- A room of your own: the keys, the furniture, and who sells them (decided 2026-10-11).
-- See Settings/Docs/Housing.md. The rules are in mod-wowplus, src/WowPlusHome.cpp; this file is the data.
--
-- - A key for each city (items 9200201 and up), sold by that city's innkeepers. Using it lets
--   you claim any spot in that city as your room, and later brings you back there.
-- - Furniture (items 9200101 and up), sold by every city innkeeper. Using a piece inside your
--   room puts it down where you stand. Each piece is an object of the game world; a few are
--   new objects made here from models the game already has (a bed, a bookshelf, a storage chest).
-- Prices are in copper. Change them here and restart. Safe to run again.

-- The cities ---------------------------------------------------------------------------------
DROP TABLE IF EXISTS wowplus_home_cities;
CREATE TABLE wowplus_home_cities (
    item  INT UNSIGNED NOT NULL PRIMARY KEY COMMENT 'the key',
    zone  INT UNSIGNED NOT NULL,
    name  VARCHAR(40) NOT NULL,
    price INT UNSIGNED NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
INSERT INTO wowplus_home_cities VALUES
    (9200201, 1637, 'Orgrimmar',        30000),
    (9200202, 1638, 'Thunder Bluff',    20000),
    (9200203, 1497, 'the Undercity',    30000),
    (9200204, 3487, 'Silvermoon City',  30000),
    (9200205, 1519, 'Stormwind',        30000),
    (9200206, 1537, 'Ironforge',        30000),
    (9200207, 1657, 'Darnassus',        20000),
    (9200208, 3557, 'the Exodar',       20000),
    (9200209, 3703, 'Shattrath',       500000),
    (9200210, 4395, 'Dalaran',        1500000);

-- The furniture ------------------------------------------------------------------------------
-- object: the game object that is put down. 9200001 and up are made below.
DROP TABLE IF EXISTS wowplus_furniture;
CREATE TABLE wowplus_furniture (
    item   INT UNSIGNED NOT NULL PRIMARY KEY,
    object INT UNSIGNED NOT NULL,
    name   VARCHAR(60) NOT NULL,
    price  INT UNSIGNED NOT NULL,
    note   VARCHAR(120) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
INSERT INTO wowplus_furniture VALUES
    (9200101, 9200001, 'Storage Chest',        5000, 'Opens your bank.'),
    (9200102,  187289, 'Guild Vault',         20000, 'Opens the vault of your guild.'),
    (9200103,   32349, 'Mailbox',              5000, 'Your mail, at home.'),
    (9200104, 9200002, 'Bed',                 10000, 'Lie down for a moment and wake up rested.'),
    (9200105,    3797, 'Cozy Fire',            3000, 'A fire to sit by, and to cook on.'),
    (9200106,    1744, 'Anvil',               10000, 'For smithing at home.'),
    (9200107,    1685, 'Forge',               20000, 'For smithing at home.'),
    (9200108,    2413, 'Wooden Chair',          500, 'To sit on.'),
    (9200109,    2489, 'High Back Chair',      2000, 'To sit on, grandly.'),
    (9200110,   24538, 'Wooden Bench',         1000, 'To sit on, with company.'),
    (9200111,  182593, 'Stool',                 300, 'To sit on, briefly.'),
    (9200112,  181075, 'Table',                1500, ''),
    (9200113,  181077, 'Rug',                  2000, ''),
    (9200114,  188346, 'Tauren Rug',           4000, ''),
    (9200115,  180473, 'Brazier',              2500, ''),
    (9200116,  180765, 'Lantern',              1500, ''),
    (9200117,  188436, 'Candles',              1000, ''),
    (9200118, 9200005, 'Candle',                300, ''),
    (9200119, 9200003, 'Bookshelf',            6000, ''),
    (9200120, 9200004, 'Wardrobe',             8000, ''),
    (9200121,  183269, 'Weapon Rack',          5000, ''),
    (9200122,  186281, 'Barrel',                500, ''),
    (9200123, 9200010, 'Beer Keg',             3000, ''),
    (9200124, 9200006, 'Pillow',                500, ''),
    (9200125, 9200007, 'Globe',               10000, ''),
    (9200126, 9200008, 'Painting',             7500, ''),
    (9200127, 9200009, 'Flower Planter',       2000, ''),
    (9200128,  152079, 'Horde Banner',         3000, ''),
    (9200129,  194278, 'Orgrimmar Banner',     4000, ''),
    (9200130,  194283, 'Thunder Bluff Banner', 4000, ''),
    (9200131,  194276, 'Undercity Banner',     4000, ''),
    (9200132,  194275, 'Silvermoon Banner',    4000, ''),
    (9200133,  192252, 'Alliance Banner',      3000, '');

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
DROP TEMPORARY TABLE IF EXISTS wp_item;
CREATE TEMPORARY TABLE wp_item AS
    SELECT i.*, f.item AS wp_entry, f.name AS wp_name, f.price AS wp_price,
           CONCAT('Furniture. Use it inside your room to put it down where you stand. ', f.note) AS wp_text,
           'item_wowplus_furniture' AS wp_script, 20 AS wp_stack, 0 AS wp_max,
           (SELECT displayid FROM item_template WHERE entry = IF(f.item = 9200101, 6353, 4470)) AS wp_icon
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

-- Who sells them -------------------------------------------------------------------------------
-- The innkeepers standing in each city: that city's key, and all the furniture.
DELETE FROM npc_vendor WHERE item BETWEEN 9200000 AND 9299999;
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