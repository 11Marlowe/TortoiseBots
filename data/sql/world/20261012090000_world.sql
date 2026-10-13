-- Issue #490 PR 1: static best-in-slot sets for level-60 hires.
--
-- Schema only: the rows come from the offline generator
-- (tools/hire_gear/, committed separately). An empty table is a valid state
-- and simply provisions nothing. Idempotent: safe to re-apply.

CREATE TABLE IF NOT EXISTS `tortoise_bots_hire_gear` (
  `class` tinyint unsigned NOT NULL,
  `spec` varchar(16) NOT NULL,
  `race` tinyint unsigned NOT NULL DEFAULT 0,
  `tier` tinyint unsigned NOT NULL,
  `slot` tinyint unsigned NOT NULL,
  `item_id` int unsigned NOT NULL DEFAULT 0,
  `enchant_spell` int unsigned NOT NULL DEFAULT 0,
  `flags` tinyint unsigned NOT NULL DEFAULT 0,
  `source` varchar(24) NOT NULL DEFAULT 'manual',
  PRIMARY KEY (`class`, `spec`, `race`, `tier`, `slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;
