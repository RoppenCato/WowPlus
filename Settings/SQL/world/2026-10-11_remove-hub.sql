-- The hub on GM Island is gone (decided 2026-10-11).
--
-- It had vendors for crafting supplies, materials, glyphs, gems and enchant scrolls, a weapon
-- master, and menus onto the game's own tier and PvP vendors. Robin: with an auction house that
-- always has the materials, glyphs, gems and enchants (mod-ah-bot and mod-wowplus), and the
-- gear vendors standing in the cities anyway, the island is not needed.
--
-- This file takes out what the hub file (2026-10-10_hub.sql, now deleted) had put in. It did
-- not change any of the game's own vendors. One thing stays: that file had also raised the buy
-- price of the items it sold where the game had none; those prices are what the auction house
-- staples are sold for, so they are left as they are.
-- Safe to run again.
DELETE FROM creature WHERE guid BETWEEN 9000001 AND 9000099;
DELETE FROM creature_template_model WHERE CreatureID BETWEEN 9000001 AND 9000099;
DELETE FROM creature_template WHERE entry BETWEEN 9000001 AND 9000099;
DELETE FROM creature_default_trainer WHERE CreatureId BETWEEN 9000001 AND 9000099;
DELETE FROM trainer_spell WHERE TrainerId = 900001;
DELETE FROM trainer WHERE Id = 900001;
DELETE FROM npc_vendor WHERE entry BETWEEN 9001000 AND 9001999;
DROP TABLE IF EXISTS wowplus_hub_menu;