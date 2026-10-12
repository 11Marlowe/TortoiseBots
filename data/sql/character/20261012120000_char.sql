-- Pre-pull Sap switch (`.bot action sap on|off`): rogues get the new "sap"
-- non-combat strategy by default (AiFactory). A bot with a stored strategy
-- snapshot replays that snapshot verbatim on load, so every rogue saved before
-- this change would silently come back with Sap OFF. Add "+sap" to existing
-- rogue "nc" snapshots once; afterwards `.bot action sap off` removes it and
-- the next save persists that choice.
--
-- Idempotent: rows that already list +sap are left alone.

UPDATE `ai_playerbot_db_store` AS `store`
JOIN `characters` AS `chars` ON `chars`.`guid` = `store`.`guid`
SET `store`.`value` = IF(COALESCE(`store`.`value`, '') = '', '+sap', CONCAT(`store`.`value`, ',+sap'))
WHERE `store`.`key` = 'nc'
  AND `chars`.`class` = 4
  AND FIND_IN_SET('+sap', COALESCE(`store`.`value`, '')) = 0;
