-- Quest bosses come back after two minutes (decided 2026-10-15). Robin: "I think the quest bosses
-- should respawn after like 2 min, in general".
--
-- A quest boss here is a creature that a quest asks you to kill and that stands in the open world
-- in at most three places (so a named one, not "kill 10 boars"), with no world bosses among them.
-- The few that stay away for two hours or more are left alone: those are timed events, not bosses.
-- Our own quests' enemies (Settings/Scripts/Make-Quests.py) and the champions of the bounties
-- (2026-10-11_champions.sql) get the same two minutes in their own files.
--
-- The table wowplus_quest_boss_respawn remembers each spawn's own time, so this can be undone:
--   UPDATE creature c JOIN wowplus_quest_boss_respawn w ON w.guid = c.guid SET c.spawntimesecs = w.original_spawntime;

SET @quest_boss_respawn = 120;

CREATE TABLE IF NOT EXISTS wowplus_quest_boss_respawn (
    guid               INT UNSIGNED NOT NULL PRIMARY KEY COMMENT 'the spawn (creature.guid)',
    original_spawntime INT UNSIGNED NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TEMPORARY TABLE IF EXISTS wp_quest_targets;
CREATE TEMPORARY TABLE wp_quest_targets (entry INT UNSIGNED NOT NULL PRIMARY KEY)
    SELECT DISTINCT target AS entry FROM (
        SELECT RequiredNpcOrGo1 AS target FROM quest_template WHERE ID < 9000000
        UNION SELECT RequiredNpcOrGo2 FROM quest_template WHERE ID < 9000000
        UNION SELECT RequiredNpcOrGo3 FROM quest_template WHERE ID < 9000000
        UNION SELECT RequiredNpcOrGo4 FROM quest_template WHERE ID < 9000000) t
    WHERE target > 0;

DROP TEMPORARY TABLE IF EXISTS wp_quest_bosses;
CREATE TEMPORARY TABLE wp_quest_bosses (entry INT UNSIGNED NOT NULL PRIMARY KEY)
    SELECT c.id AS entry FROM creature c JOIN wp_quest_targets t ON t.entry = c.id
    JOIN creature_template ct ON ct.entry = c.id
    WHERE ct.`rank` <> 3
    GROUP BY c.id HAVING COUNT(*) <= 3 AND SUM(c.map NOT IN (0, 1, 530, 571)) = 0;

INSERT IGNORE INTO wowplus_quest_boss_respawn (guid, original_spawntime)
    SELECT c.guid, c.spawntimesecs FROM creature c JOIN wp_quest_bosses b ON b.entry = c.id
    WHERE c.spawntimesecs > @quest_boss_respawn AND c.spawntimesecs < 7200;

UPDATE creature c JOIN wowplus_quest_boss_respawn w ON w.guid = c.guid JOIN wp_quest_bosses b ON b.entry = c.id
    SET c.spawntimesecs = @quest_boss_respawn;

DROP TEMPORARY TABLE wp_quest_bosses;
DROP TEMPORARY TABLE wp_quest_targets;
