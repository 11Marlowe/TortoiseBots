-- The item instances a dungeon run gave to its bots (spec item-leg-skip D7):
-- the crash ledger for AiPlayerbot.DungeonRunGiveItems. One row per given item
-- instance: `item_guid` is the exact item the run created (written before the
-- bot holds it; a failed write refuses the give), `guid` the bot, `item` the
-- entry. The take-back at done, stop, erase, a bot leaving or a real player
-- joining destroys exactly that instance (never the bot's own copy) and deletes
-- the row; a server crash or restart mid-run leaves the row, and the bot's
-- login cleanup destroys the instance and deletes the row.
-- Not released before review round 2 (2026-10-26): the columns changed then.
-- Never edit this file once applied.

CREATE TABLE IF NOT EXISTS `ai_run_given_item` (
  `item_guid` int unsigned NOT NULL,
  `guid` int unsigned NOT NULL,
  `item` int unsigned NOT NULL,
  PRIMARY KEY (`item_guid`),
  KEY `idx_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
