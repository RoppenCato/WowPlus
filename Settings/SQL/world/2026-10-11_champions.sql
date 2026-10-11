-- Champions (decided 2026-10-11): one or two ordinary enemies in every zone are something more.
--
-- A champion is an enemy of the zone with a name of its own, the silver dragon of a rare, more
-- health, a bigger body, and a level above its kind. Being a rare, it always drops an extra item
-- and gives the first-kill experience bonus (mod-wowplus). Which enemies they are comes from
-- the world seed (WowPlus.World.Seed in Settings/mod_wowplus.conf, handed to this file as
-- @wowplus_seed by Deploy-Settings.ps1): the same seed always gives the same champions, a new
-- seed gives new ones somewhere else. Robin: "a bit limited, 1-2 per zone".
--
-- How: each champion gets a copy of its kind's creature (entries 9100001 and up) with the
-- changes, and that one spawn is switched to the copy. The table wowplus_champions remembers
-- what each spawn was, and this file first puts everything back and then chooses again, so it
-- is safe to run again and nothing piles up.
--
-- Needs the zone of every spawn in the database (Calculate.Creature.Zone.Area.Data in
-- Settings/worldserver.conf, which the server fills in while starting). On a new installation
-- there are therefore no champions until the second start.

SET @seed = IFNULL(@wowplus_seed, 1);

CREATE TABLE IF NOT EXISTS wowplus_champions (
    guid               INT UNSIGNED NOT NULL PRIMARY KEY COMMENT 'the spawn (creature.guid)',
    original_id        INT UNSIGNED NOT NULL COMMENT 'what the spawn was before',
    original_spawntime INT UNSIGNED NOT NULL,
    entry              INT UNSIGNED NOT NULL COMMENT 'the champion''s own creature_template entry',
    zone               INT UNSIGNED NOT NULL,
    name               VARCHAR(100) NOT NULL,
    kind               VARCHAR(100) NOT NULL COMMENT 'the name of the enemy it is one of',
    level              TINYINT UNSIGNED NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Put everything back --------------------------------------------------------------------
UPDATE creature c JOIN wowplus_champions w ON w.guid = c.guid SET c.id = w.original_id, c.spawntimesecs = w.original_spawntime;
DELETE FROM creature_template WHERE entry BETWEEN 9100000 AND 9199999;
DELETE FROM creature_template_model WHERE CreatureID BETWEEN 9100000 AND 9199999;
DELETE FROM creature_template_addon WHERE entry BETWEEN 9100000 AND 9199999;
DELETE FROM creature_template_spell WHERE CreatureID BETWEEN 9100000 AND 9199999;
DELETE FROM creature_equip_template WHERE CreatureID BETWEEN 9100000 AND 9199999;
DELETE FROM creature_text WHERE CreatureID BETWEEN 9100000 AND 9199999;
DELETE FROM smart_scripts WHERE source_type = 0 AND entryorguid BETWEEN 9100000 AND 9199999;
DELETE FROM creature_queststarter WHERE quest BETWEEN 9100000 AND 9199999;
DELETE FROM creature_questender WHERE quest BETWEEN 9100000 AND 9199999;
DELETE FROM quest_offer_reward WHERE ID BETWEEN 9100000 AND 9199999;
DELETE FROM quest_request_items WHERE ID BETWEEN 9100000 AND 9199999;
DELETE FROM quest_template WHERE ID BETWEEN 9100000 AND 9199999;
DELETE FROM wowplus_champions;

-- Choose -----------------------------------------------------------------------------------
-- Candidates: ordinary enemies out in the world that are meant to be fought: they have loot,
-- offer no service, can be attacked, are not part of a seasonal event, and are past the
-- starting areas. Not in the capitals.
-- The first by the seed's order in each zone is a champion; in about half the zones the second too.
INSERT INTO wowplus_champions (guid, original_id, original_spawntime, entry, zone, name, kind, level)
SELECT x.guid, x.id, x.spawntimesecs, 9100000 + ROW_NUMBER() OVER (ORDER BY x.zoneId, x.rn), x.zoneId,
    IF(x.type = 1,
        CONCAT(ELT(1 + x.h % 14, 'Grey', 'Black', 'Scar', 'Blood', 'Pale', 'Old ', 'Iron', 'Bone', 'Red', 'Long', 'Split', 'Night', 'Storm', 'Ash'),
               ELT(1 + (x.h DIV 14) % 12, 'fang', 'maw', 'claw', 'hide', 'tusk', 'mane', 'eye', 'jaw', 'back', 'tail', 'tooth', 'pelt')),
        CONCAT(ELT(1 + x.h % 30, 'Grakkar', 'Morven', 'Ulric', 'Sethra', 'Vorn', 'Halga', 'Krom', 'Ysmir', 'Dreth', 'Zalika', 'Brugan', 'Nerys',
                   'Tholin', 'Marrow', 'Skarn', 'Velra', 'Orrin', 'Gashnak', 'Lune', 'Harrow', 'Kezra', 'Bolgar', 'Ivra', 'Thane', 'Rukh',
                   'Sable', 'Morga', 'Eldrin', 'Vask', 'Urzul'),
               ' the ',
               ELT(1 + (x.h DIV 30) % 30, 'Unyielding', 'Patient', 'Cruel', 'Scarred', 'Silent', 'Hungry', 'Elder', 'Returned', 'Bold', 'Watchful',
                   'Restless', 'Crooked', 'Proud', 'Lost', 'Grim', 'Tireless', 'Cunning', 'Forsworn', 'Stubborn', 'Bitter', 'Unbroken',
                   'Wanderer', 'Last', 'Pale', 'Relentless', 'Sly', 'Ancient', 'Fearless', 'Hollow', 'Wary'))),
    x.kind, x.maxlevel
FROM (
    SELECT c.guid, c.id, c.spawntimesecs, c.zoneId, ct.name AS kind, ct.maxlevel, ct.type,
        ROW_NUMBER() OVER (PARTITION BY c.zoneId ORDER BY CRC32(CONCAT(@seed, ':', c.guid)), c.guid) AS rn,
        CRC32(CONCAT(@seed, ':name:', c.guid)) AS h
    FROM creature c
    JOIN creature_template ct ON ct.entry = c.id
    LEFT JOIN game_event_creature e ON e.guid = c.guid
    WHERE c.map IN (0, 1, 530, 571) AND c.zoneId > 0 AND c.phaseMask = 1 AND e.guid IS NULL
      AND c.zoneId NOT IN (1519, 1537, 1657, 3557, 1637, 1638, 1497, 3487, 3703, 4395)
      AND ct.`rank` = 0 AND ct.type IN (1, 2, 3, 4, 5, 6, 7, 9) AND ct.npcflag = 0 AND ct.lootid > 0 AND ct.minlevel >= 6
      AND (ct.flags_extra & 130) = 0 AND (ct.unit_flags & 33554690) = 0
      AND ct.entry < 9000000
) x
WHERE x.rn = 1 OR (x.rn = 2 AND CRC32(CONCAT(@seed, ':two:', x.zoneId)) % 2 = 0);

-- Make them ----------------------------------------------------------------------------------
-- The champion's creature: a copy of its kind, with a name (its kind shows as the title), the
-- rank of a rare (4), half as much health again on top of what a rare gets, one level higher.
-- Kills still count for quests that ask for its kind.
DROP TEMPORARY TABLE IF EXISTS wp_ch;
CREATE TEMPORARY TABLE wp_ch AS
    SELECT ct.*, w.entry AS wp_entry, w.name AS wp_name, w.original_id AS wp_original
    FROM creature_template ct JOIN wowplus_champions w ON w.original_id = ct.entry;
UPDATE wp_ch SET entry = wp_entry, subname = LEFT(name, 100), name = wp_name, `rank` = 4,
    HealthModifier = HealthModifier * 1.5, minlevel = LEAST(maxlevel + 1, 83), maxlevel = LEAST(maxlevel + 1, 83),
    KillCredit1 = IF(KillCredit1 = 0, wp_original, KillCredit1);
ALTER TABLE wp_ch DROP COLUMN wp_entry, DROP COLUMN wp_name, DROP COLUMN wp_original;
INSERT INTO creature_template SELECT * FROM wp_ch;
DROP TEMPORARY TABLE wp_ch;

-- Its looks (a quarter bigger), weapons, spells, lines and behaviour: those of its kind
INSERT INTO creature_template_model (CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild)
    SELECT w.entry, m.Idx, m.CreatureDisplayID, m.DisplayScale * 1.25, m.Probability, m.VerifiedBuild
    FROM creature_template_model m JOIN wowplus_champions w ON w.original_id = m.CreatureID;
INSERT INTO creature_equip_template (CreatureID, ID, ItemID1, ItemID2, ItemID3, VerifiedBuild)
    SELECT w.entry, q.ID, q.ItemID1, q.ItemID2, q.ItemID3, q.VerifiedBuild
    FROM creature_equip_template q JOIN wowplus_champions w ON w.original_id = q.CreatureID;
INSERT INTO creature_template_spell (CreatureID, `Index`, Spell, VerifiedBuild)
    SELECT w.entry, s.`Index`, s.Spell, s.VerifiedBuild
    FROM creature_template_spell s JOIN wowplus_champions w ON w.original_id = s.CreatureID;

DROP TEMPORARY TABLE IF EXISTS wp_copy;
CREATE TEMPORARY TABLE wp_copy AS
    SELECT a.*, w.entry AS wp_entry FROM creature_template_addon a JOIN wowplus_champions w ON w.original_id = a.entry;
UPDATE wp_copy SET entry = wp_entry;
ALTER TABLE wp_copy DROP COLUMN wp_entry;
INSERT INTO creature_template_addon SELECT * FROM wp_copy;
DROP TEMPORARY TABLE wp_copy;

CREATE TEMPORARY TABLE wp_copy AS
    SELECT t.*, w.entry AS wp_entry FROM creature_text t JOIN wowplus_champions w ON w.original_id = t.CreatureID;
UPDATE wp_copy SET CreatureID = wp_entry;
ALTER TABLE wp_copy DROP COLUMN wp_entry;
INSERT INTO creature_text SELECT * FROM wp_copy;
DROP TEMPORARY TABLE wp_copy;

CREATE TEMPORARY TABLE wp_copy AS
    SELECT s.*, w.entry AS wp_entry FROM smart_scripts s JOIN wowplus_champions w ON w.original_id = s.entryorguid
    WHERE s.source_type = 0;
UPDATE wp_copy SET entryorguid = wp_entry;
ALTER TABLE wp_copy DROP COLUMN wp_entry;
INSERT INTO smart_scripts SELECT * FROM wp_copy;
DROP TEMPORARY TABLE wp_copy;

-- Put them in the world: that one spawn becomes the champion, and comes back after 4 to 8 hours
UPDATE creature c JOIN wowplus_champions w ON w.guid = c.guid
    SET c.id = w.entry, c.spawntimesecs = 14400 + CRC32(CONCAT(@seed, ':time:', c.guid)) % 14400;

-- Bounties -------------------------------------------------------------------------------------
-- Every champion has a price on its head: a real quest, offered and paid by the innkeepers of
-- its zone (where a zone has no inn, by its flight masters). Robin, 2026-10-11: "bounty boards
-- with quests to kill the champions". A board would need a place chosen by hand in every town;
-- the innkeeper is already there.
-- The quest's number depends on the seed, so a new world's bounties are new quests even for a
-- character that collected the old ones.
SET @quest_base = 9100000 + (@seed % 90) * 1000 - 9100000;

INSERT INTO quest_template (ID, QuestType, QuestLevel, MinLevel, QuestSortID, RewardXPDifficulty, RewardMoney, Flags,
        RequiredNpcOrGo1, RequiredNpcOrGoCount1, LogTitle, LogDescription, QuestDescription, QuestCompletionLog, ObjectiveText1)
    SELECT w.entry + @quest_base, 2, w.level + 1, GREATEST(1, w.level - 5), w.zone, 6, (w.level + 1) * (w.level + 1) * 6, 0,
        w.entry, 1,
        CONCAT('Bounty: ', w.name),
        CONCAT('Kill ', w.name, ', the champion among the ', w.kind, 's of this land, and return for the bounty.'),
        CONCAT('You look like you can handle yourself, $N.$B$BThere is one out there that folk have started to call ', w.name,
               '. A ', w.kind, ', but bigger and meaner than any of its kind, and it has cost travellers dearly.$B$B',
               'There is a purse put together for whoever ends it. Bring it down and the money is yours.'),
        CONCAT('Return to the one who offered the bounty on ', w.name, '.'),
        CONCAT(w.name, ' slain')
    FROM wowplus_champions w;

INSERT INTO quest_request_items (ID, EmoteOnComplete, EmoteOnIncomplete, CompletionText, VerifiedBuild)
    SELECT w.entry + @quest_base, 0, 0, CONCAT('Is it done? Is ', w.name, ' dead?'), 0 FROM wowplus_champions w;
INSERT INTO quest_offer_reward (ID, RewardText, VerifiedBuild)
    SELECT w.entry + @quest_base, CONCAT('So ', w.name, ' is no more. The roads will be quieter for it, $N.$B$BHere is the purse, every coin of it. You have earned it.'), 0
    FROM wowplus_champions w;

-- Who offers them: the innkeepers standing in the champion's zone...
DROP TEMPORARY TABLE IF EXISTS wp_givers;
CREATE TEMPORARY TABLE wp_givers AS
    SELECT DISTINCT c.id AS npc, c.zoneId AS zone, 1 AS inn
    FROM creature c JOIN creature_template ct ON ct.entry = c.id
    WHERE c.map IN (0, 1, 530, 571) AND c.zoneId > 0 AND (ct.npcflag & 65536) AND ct.entry < 9000000;
-- ...and the flight masters of zones that have no inn
DROP TEMPORARY TABLE IF EXISTS wp_inn_zones;
CREATE TEMPORARY TABLE wp_inn_zones AS SELECT DISTINCT zone FROM wp_givers;
INSERT INTO wp_givers
    SELECT DISTINCT c.id, c.zoneId, 0
    FROM creature c JOIN creature_template ct ON ct.entry = c.id
    LEFT JOIN wp_inn_zones z ON z.zone = c.zoneId
    WHERE c.map IN (0, 1, 530, 571) AND c.zoneId > 0 AND (ct.npcflag & 8192) AND ct.entry < 9000000 AND z.zone IS NULL;
DROP TEMPORARY TABLE wp_inn_zones;

INSERT IGNORE INTO creature_queststarter (id, quest)
    SELECT g.npc, w.entry + @quest_base FROM wowplus_champions w JOIN wp_givers g ON g.zone = w.zone;
INSERT IGNORE INTO creature_questender (id, quest)
    SELECT g.npc, w.entry + @quest_base FROM wowplus_champions w JOIN wp_givers g ON g.zone = w.zone;
-- They must be able to give quests (flag 2). This stays set even when the champions move on.
UPDATE creature_template ct JOIN wp_givers g ON g.npc = ct.entry SET ct.npcflag = ct.npcflag | 2 WHERE (ct.npcflag & 2) = 0;
DROP TEMPORARY TABLE wp_givers;
