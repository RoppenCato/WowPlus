-- Phase 7: abilities that came from class quests are taught by the class trainers (decided 2026-10-11).
--
-- Why: the class quests are group-era chores that are miserable alone, and several of them
-- hold back something the class needs to work at all (a hunter's pet, a warrior's stances,
-- a warlock's demons). The abilities now come from the trainer at the level the quest was
-- offered. The quests are left in the game for whoever wants the adventure.
--
-- How: each row below is the very spell the quest cast on the player as its reward, so the
-- trainer teaches exactly what the quest taught, bundles included (for example "Path of
-- Defense" teaches Defensive Stance, Sunder Armor and Taunt). In the trainer window the
-- line carries the name of that lesson spell, which is sometimes a little odd.
--
-- Not here because the trainers already teach them: Sense Undead, the paladin and warlock
-- mounts, Summon Imp, Aquatic Form, Swift Flight Form.
-- Not here because a trainer cannot give items: the shaman's totems (see mod-wowplus,
-- setting WowPlus.Classes.ShamanTotemRelic) .
--
-- Safe to run again. To undo: run the DELETE below on its own.

DROP TEMPORARY TABLE IF EXISTS wp_lesson;
CREATE TEMPORARY TABLE wp_lesson (class INT UNSIGNED, spell INT UNSIGNED, level INT UNSIGNED, cost INT UNSIGNED, what VARCHAR(100));
INSERT INTO wp_lesson VALUES
    -- class 1 warrior
    (1,   8121, 10,   300, 'Defensive Stance, Sunder Armor, Taunt'),
    (1,   8616, 30,  8000, 'Berserker Stance, Intercept'),
    -- class 2 paladin
    (2,   7329, 12,   500, 'Redemption'),
    -- class 3 hunter
    (3,   1579, 10,   300, 'Tame Beast and the pet commands'),
    (3,   5300, 10,   300, 'Feed Pet, Revive Pet'),
    -- class 4 rogue
    (4,   2995, 20,  2000, 'Poisons'),
    -- class 7 shaman
    (7,   8073,  4,   100, 'Stoneskin Totem'),
    (7,   2075, 10,   300, 'Searing Totem'),
    (7,   5396, 20,  2000, 'Healing Stream Totem'),
    -- class 9 warlock
    (9,  11520, 10,   300, 'Summon Voidwalker'),
    (9,  11519, 20,  2000, 'Summon Succubus'),
    (9,   1373, 30,  8000, 'Summon Felhunter'),
    (9,   1413, 50, 30000, 'Inferno'),
    (9,  20700, 60, 50000, 'Ritual of Doom'),
    -- class 11 druid
    (11, 19179, 10,   300, 'Bear Form, Growl, Maul'),
    (11,  8947, 14,   900, 'Cure Poison');

-- Every class trainer of that class (type 0 = class trainer, requirement = the class)
DELETE ts FROM trainer_spell ts
    JOIN trainer t ON t.Id = ts.TrainerId AND t.Type = 0
    JOIN wp_lesson l ON l.class = t.Requirement AND l.spell = ts.SpellId;

INSERT INTO trainer_spell (TrainerId, SpellId, MoneyCost, ReqSkillLine, ReqSkillRank, ReqAbility1, ReqAbility2, ReqAbility3, ReqLevel, VerifiedBuild)
    SELECT t.Id, l.spell, l.cost, 0, 0, 0, 0, 0, l.level, 0
    FROM trainer t JOIN wp_lesson l ON l.class = t.Requirement
    WHERE t.Type = 0;

DROP TEMPORARY TABLE IF EXISTS wp_lesson;
