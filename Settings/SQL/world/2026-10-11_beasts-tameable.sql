-- Hunters can tame more (decided 2026-10-11, see Settings/Docs/Class-Changes.md).
--
-- Named so that it runs before the champions file: a champion is a copy of its kind, and takes
-- over whether its kind can be tamed.
--
-- 1. New kinds of beast. In this version of the game many beasts belong to no pet family and so
--    cannot be tamed: basilisks, hydras, stags, kodos, mammoths, foxes and more. Each kind is
--    given the family it resembles most, and with that the family's abilities and diet:
--      basilisks and crocodiles     Crocolisk (6)
--      hydras                       Wind Serpent (27)
--      stags, rams, kodos, mammoths, giraffes, zhevras, talbuks, clefthoofs, horses
--                                   Boar (5): hoofed animals that charge
--      foxes, worgs, hounds, jackals, coyotes, mastiffs
--                                   Wolf (1)
--      beetles and scarabs          Crab (8)
--    Only wild ones: they must have loot or a hide, and offer no service.
-- 2. Beasts that have a family but were marked "cannot be tamed", rares among them, can be tamed.
--    Bosses are left out. Exotic families still need the Beast Mastery talent.
--
-- type_flags 1 = can be tamed. Safe to run again. This cannot be undone by a file: the original
-- values are in Server/data/sql/base/db_world/creature_template.sql.

UPDATE creature_template SET family = CASE
        WHEN name REGEXP 'Basilisk|Crocolisk|Crocodile' THEN 6
        WHEN name REGEXP 'Hydra' THEN 27
        WHEN name REGEXP 'Fox|Worg|Hound|Jackal|Coyote|Mastiff' THEN 1
        WHEN name REGEXP 'Beetle|Scarab' THEN 8
        ELSE 5 END
    WHERE type = 1 AND family = 0 AND entry < 9000000 AND npcflag = 0 AND (lootid > 0 OR skinloot > 0)
      AND `rank` IN (0, 1, 2, 4) AND (unit_flags & 33554690) = 0
      AND name REGEXP 'Basilisk|Crocolisk|Crocodile|Hydra|Fox|Worg|Hound|Jackal|Coyote|Mastiff|Beetle|Scarab|Stag|Deer|Gazelle|Elk|Moose|Caribou|Ram$|Ram |Goat|Kodo|Mammoth|Yak|Giraffe|Zhevra|Talbuk|Clefthoof|Horse|Stallion';

UPDATE creature_template SET type_flags = type_flags | 1
    WHERE type = 1 AND family > 0 AND (type_flags & 1) = 0 AND entry < 9000000 AND npcflag = 0
      AND `rank` IN (0, 1, 2, 4) AND (unit_flags & 33554690) = 0 AND (lootid > 0 OR skinloot > 0);