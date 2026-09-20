-- Issue #219: stamina undervaluation in random-bot gear scoring.
--
-- Warlock specs (26, 27, 28) had sta = 0, spi = 0, int = 1 with shasplpwr = 5:
-- a green "...of Shadow Wrath" item outscored blue stamina/intellect gear 10:1,
-- so warlocks spawned with 0 stamina. Healer specs (holy paladin 4, disc 13,
-- holy priest 14, resto shaman 22, resto druid 31) had sta = 0 with massive
-- splheal weights, and arms/fury warriors (1, 2) had sta = 0, so the same
-- pure-throughput bias applied. Warlock int = 1 further devalued primary stats
-- against single-school spell damage.
--
-- Fix: add sta weights (warlocks 6, healers 10-12, warriors 15) and raise
-- warlock int 1 -> 4 so stamina/intellect gear scores competitively without
-- drowning throughput stats. Idempotent: DELETEs the touched (id, field) rows
-- before re-inserting (ai_playerbot_weightscale_data has no unique key and a
-- duplicate stat row would double-count a weight).

DELETE FROM `ai_playerbot_weightscale_data`
WHERE (`id` IN (26,27,28) AND `field` IN ('sta','int'))
   OR (`id` IN (1,2) AND `field` = 'sta')
   OR (`id` IN (4,13,14,22,31) AND `field` = 'sta');

INSERT INTO `ai_playerbot_weightscale_data` (`id`, `field`, `val`) VALUES
	(1, 'sta', 15),
	(2, 'sta', 15),
	(4, 'sta', 12),
	(13, 'sta', 10),
	(14, 'sta', 12),
	(22, 'sta', 10),
	(31, 'sta', 10),
	(26, 'sta', 6),
	(27, 'sta', 6),
	(28, 'sta', 6),
	(26, 'int', 4),
	(27, 'int', 4),
	(28, 'int', 4);
