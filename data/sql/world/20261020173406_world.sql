-- Issue #1125 (shaman tank, experimental): `shamantank` weight scale.
--
-- New id 16 (class 7): first-version weights for the shaman tank:
-- Stamina = Armor > Agility, hit to 8% valued, some Strength, low Defense,
-- zero spell power. Id 16 is the first DK-gap id: the seed migration
-- (20260916090001_world.sql) documents 16-19 as deliberately unused (the
-- target core has no death knight class), the item cache table carries
-- scale_1..scale_32 columns, and MAX_STAT_SCALES is 32 — so 16 scores,
-- caches, and persists with no engine or schema change. (Id 33+ would need
-- a new cache column, a broader INSERT column list, and a MAX_STAT_SCALES
-- bump.) The r3 "16-19 collides with operator 32-spec dumps" caveat does
-- not apply here: the equipment cache is generated per install from the
-- installed scales, so a foreign id 16 row can only exist where the same
-- scale already scores.
--
-- Idempotency: DELETEs the touched (id, field) rows before re-inserting
-- (ai_playerbot_weightscale_data has no unique key and a duplicate stat row
-- would double-count a weight). The core runs each migration file inside one
-- transaction, so this file has no START TRANSACTION.

DELETE FROM `ai_playerbot_weightscales` WHERE `id` = 16;
DELETE FROM `ai_playerbot_weightscale_data` WHERE `id` = 16;

INSERT INTO `ai_playerbot_weightscales` (`id`, `name`, `class`) VALUES
	(16, 'shamantank', 7);

INSERT INTO `ai_playerbot_weightscale_data` (`id`, `field`, `val`) VALUES
	(16, 'sta', 20),
	(16, 'armor', 2),
	(16, 'agi', 8),
	(16, 'str', 6),
	(16, 'defrtng', 6),
	(16, 'hitrtng', 60),
	(16, 'critstrkrtng', 40),
	(16, 'atkpwr', 4),
	(16, 'mledps', 28),
	(16, 'int', 2),
	(16, 'block', 2),
	(16, 'blockrtng', 8),
	(16, 'dodgertng', 8),
	(16, 'exprtng', 1);
