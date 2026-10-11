-- The journal (mod-wowplus, src/WowPlusJournal.cpp): one line per thing worth remembering.
-- Safe to run again.

CREATE TABLE IF NOT EXISTS `wowplus_journal` (
    `id`   BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    `guid` INT UNSIGNED NOT NULL COMMENT 'the character',
    `book` VARCHAR(10) NOT NULL COMMENT 'world or people',
    `at`   INT UNSIGNED NOT NULL COMMENT 'unix time',
    `text` VARCHAR(255) NOT NULL,
    PRIMARY KEY (`id`),
    KEY `by_character` (`guid`, `book`, `id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;