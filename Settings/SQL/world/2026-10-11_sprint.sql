-- Sprint on Shift (mod-wowplus, src/WowPlusSprint.cpp).
-- A hidden passive aura that raises run speed and swim speed. The game client does not know this spell and
-- is never told about it; the server only uses it to carry the speed bonus, so that it follows
-- the game's own rule for speed boosts (the highest one counts, they do not add up).
-- The amount here (30 percent) is replaced by the setting WowPlus.Sprint.SpeedPercent.
-- Safe to run again.

DELETE FROM `spell_dbc` WHERE `ID` = 100200;
INSERT INTO `spell_dbc`
    (`ID`, `Attributes`, `CastingTimeIndex`, `ProcChance`, `DurationIndex`, `RangeIndex`, `EquippedItemClass`,
     `Effect_1`, `EffectDieSides_1`, `EffectBasePoints_1`, `ImplicitTargetA_1`, `EffectAura_1`,
     `Effect_2`, `EffectDieSides_2`, `EffectBasePoints_2`, `ImplicitTargetA_2`, `EffectAura_2`, `SchoolMask`, `Name_Lang_enUS`)
VALUES
    -- Attributes 208: passive, not shown. Duration 21: until removed. Effect 6: aura.
    -- Aura 31: run speed. Aura 58: swim speed.
    (100200, 208, 1, 101, 21, 1, -1,
     6, 1, 29, 1, 31,
     6, 1, 29, 1, 58, 1, 'WowPlus Sprint');
