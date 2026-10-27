-- The plans bot LFG groups may pick (runtime/BotLfgService.cpp). A row means the plan has a full
-- bot-only clear.
--
-- lft_code is the plan's key in the LFT addon range table (runtime/policy/lfg/LftDungeonRangePolicy.h),
-- which sets its level band. teams is 1 Alliance, 2 Horde, 3 both.
--
-- Left out: scarlet_graveyard (its newest run paused at its last step on the event-only Scorn), and
-- dragonmaw_retreat, frostmane_hollow, stormwrought_ruins and windhorn_canyon (no LFT range).
--
-- Its own table, so a plan re-export (DELETE and INSERT of the ai_dungeon_plan row) never clears it.
--
-- Read at startup and by `.bot run reloadplans`. Idempotent. Never edit this file once applied; add a
-- newer migration.

CREATE TABLE IF NOT EXISTS `ai_bot_lfg_plan` (
  `plan` varchar(32) NOT NULL,
  `lft_code` varchar(32) NOT NULL,
  `teams` tinyint unsigned NOT NULL DEFAULT 3,
  `comment` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`plan`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;

REPLACE INTO `ai_bot_lfg_plan` (`plan`, `lft_code`, `teams`, `comment`) VALUES
('ragefire_chasm', 'rfc', 2, 'full clear; Horde only (Orgrimmar)'),
('wailing_caverns', 'wc', 3, 'full clear'),
('deadmines', 'deadmines', 3, 'full clear'),
('shadowfang_keep', 'sfk', 3, 'full clear'),
('stormwind_stockade', 'stockade', 1, 'full clear; Alliance only (Stormwind)'),
('blackfathom_deeps', 'bfd', 3, 'full clear'),
('gnomeregan', 'gnomer', 3, 'full clear'),
('razorfen_kraul', 'rfk', 3, 'full clear'),
('crescent_grove', 'tcg', 3, 'full clear'),
('scarlet_library', 'smlib', 3, 'full clear'),
('scarlet_armory', 'smarmory', 3, 'full clear'),
('scarlet_cathedral', 'smcath', 3, 'full clear'),
('razorfen_downs', 'rfd', 3, 'full clear'),
('uldaman', 'ulda', 3, 'full clear'),
('gilneas_city', 'gilneas', 3, 'full clear'),
('zulfarrak', 'zf', 3, 'full clear'),
('maraudon_orange', 'maraorange', 3, 'full clear'),
('maraudon_purple', 'marapurple', 3, 'full clear'),
('sunken_temple', 'st', 3, 'full clear'),
('hateforge_quarry', 'hfq', 3, 'full clear'),
('blackrock_depths', 'brd', 3, 'full clear'),
('lower_blackrock_spire', 'lbrs', 3, 'full clear'),
('dire_maul_east', 'dme', 3, 'full clear'),
('dire_maul_north', 'dmn', 3, 'full clear'),
('dire_maul_west', 'dmw', 3, 'full clear'),
('scholomance', 'scholo', 3, 'full clear'),
('stratholme_live', 'stratlive', 3, 'full clear'),
('stratholme_undead', 'stratud', 3, 'full clear'),
('karazhan_crypt', 'kc', 3, 'full clear'),
('black_morass', 'cotbm', 3, 'full clear'),
('stormwind_vault', 'swv', 1, 'full clear; Alliance only (Stormwind)');
