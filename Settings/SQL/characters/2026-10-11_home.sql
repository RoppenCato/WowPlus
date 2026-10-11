-- A room of your own (mod-wowplus, src/WowPlusHome.cpp; Settings/Docs/Housing.md).
-- Safe to run again.

-- The spot a character has claimed in each city
CREATE TABLE IF NOT EXISTS `wowplus_home` (
    `guid` INT UNSIGNED NOT NULL COMMENT 'the character',
    `city` INT UNSIGNED NOT NULL COMMENT 'the zone of the city',
    `map`  INT UNSIGNED NOT NULL,
    `x`    FLOAT NOT NULL,
    `y`    FLOAT NOT NULL,
    `z`    FLOAT NOT NULL,
    `o`    FLOAT NOT NULL,
    PRIMARY KEY (`guid`, `city`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The furniture standing in it
CREATE TABLE IF NOT EXISTS `wowplus_home_objects` (
    `id`    BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `guid`  INT UNSIGNED NOT NULL COMMENT 'the character',
    `city`  INT UNSIGNED NOT NULL,
    `item`  INT UNSIGNED NOT NULL COMMENT 'the furniture item it was placed from, and is handed back as',
    `x`     FLOAT NOT NULL,
    `y`     FLOAT NOT NULL,
    `z`     FLOAT NOT NULL,
    `o`     FLOAT NOT NULL,
    PRIMARY KEY (`id`),
    KEY `by_room` (`guid`, `city`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;