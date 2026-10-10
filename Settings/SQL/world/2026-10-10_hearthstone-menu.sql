-- Phase 6: the hearthstone opens our travel menu.
-- Hands the hearthstone item (6948) to the script item_wowplus_hearthstone in mod-wowplus.
-- While nothing is unlocked, or with WowPlus.Travel.Hearthstone.Menu = 0, it works as always.
--
-- To undo: UPDATE item_template SET ScriptName = '' WHERE entry = 6948;
UPDATE item_template SET ScriptName = 'item_wowplus_hearthstone' WHERE entry = 6948;
