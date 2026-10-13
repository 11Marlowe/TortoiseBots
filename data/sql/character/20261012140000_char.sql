-- Per-character hire-unlock boss kills (Issue #490 PR 2, kills worker).
--
-- Each completed encounter counts ONCE per character, ever: kills add up
-- across runs and lockouts. The encounter key is HireUnlockEncounterKey
-- (Four Horsemen keyed 16062; every other boss its own entry).
-- Optional-map bosses (HireGearOptionalMaps) count automatically and are
-- never stored here.
--
-- Rows are loaded into a memory cache at login and inserted on credit;
-- a deleted character's rows are removed by the module's OnDelete hook
-- (core Player::DeleteFromDB knows nothing about this table).

CREATE TABLE IF NOT EXISTS `tortoise_bots_hire_unlock_kills` (
  `guid` INT(10) UNSIGNED NOT NULL,
  `boss_entry` INT(10) UNSIGNED NOT NULL,
  `killed_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`, `boss_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
