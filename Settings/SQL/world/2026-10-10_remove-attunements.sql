-- Phase 1.2: remove all attunements (decided 2026-10-10).
-- Every dungeon and raid is open as soon as the character has the level.
--
-- dungeon_access_requirements held 35 rows: the quests, heroic keys and achievements needed to
-- enter (Caverns of Time quests, the Outland heroic keys, Magisters' Terrace heroic, the frozen
-- halls quest chain, and the achievements for heroic Trial of the Crusader and Icecrown Citadel).
-- Level requirements are in dungeon_access_template and stay.
--
-- To get them back: the original rows are in Server/data/sql/base/db_world/dungeon_access_requirements.sql
DELETE FROM dungeon_access_requirements;
