-- First kills (mod-wowplus, src/WowPlusXP.cpp): which rares and bosses a character has
-- already killed once, so that the first-kill experience bonus is given only once each.
-- Safe to run again.

CREATE TABLE IF NOT EXISTS `wowplus_first_kills` (
    `guid`  INT UNSIGNED NOT NULL COMMENT 'the character',
    `entry` INT UNSIGNED NOT NULL COMMENT 'the creature (creature_template.entry)',
    `at`    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`guid`, `entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;