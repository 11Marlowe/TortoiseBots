-- Which given run items a bot still holds (spec item-leg-skip D7): the crash
-- ledger for the bot-only giving (AiPlayerbot.DungeonRunGiveItems). A given
-- item never stays in a bot's bags in normal play: the take-back at done, stop,
-- erase, bot leaves or a real player joining deletes these rows; a server crash
-- or restart mid-run leaves them, and the login cleanup destroys up to `count`
-- of each item and deletes the rows.
-- Never edit this file once applied.

CREATE TABLE IF NOT EXISTS `ai_run_given_item` (
  `guid` int unsigned NOT NULL,
  `item` int unsigned NOT NULL,
  `count` int unsigned NOT NULL DEFAULT 1,
  PRIMARY KEY (`guid`, `item`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
