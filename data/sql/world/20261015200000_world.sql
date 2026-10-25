-- Dungeon plans for the run leader (.bot run): plan-wide unit lists on `ai_dungeon_plan` (issue #999).
--
--   cc_priority  creature entries the run leader crowd-controls in a pack, best first (`cc moon <guid> <mage>`
--                before the pull); plan v1's cc_priority
--   cc_always    entries crowd-controlled in any pack they are in (ranked after cc_priority); plan v1's cc_always
--   kill_first   entries the party kills first in a fight (`focus <guid>`; a summoned healing ward too); plan v1's
--                kill_first
-- Each is a comma list as in `ai_dungeon_step`.`entries` ('' none). Read once at startup
-- (ai/playerbot/DungeonPlanMgr.cpp).
--
-- Idempotent per column (information_schema + PREPARE runs on MySQL 8 and MariaDB).
-- Never edit this file once applied; add a newer migration.

SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_dungeon_plan` ADD COLUMN `cc_priority` varchar(255) NOT NULL DEFAULT '''' AFTER `map`',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_dungeon_plan' AND COLUMN_NAME = 'cc_priority');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_dungeon_plan` ADD COLUMN `cc_always` varchar(255) NOT NULL DEFAULT '''' AFTER `cc_priority`',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_dungeon_plan' AND COLUMN_NAME = 'cc_always');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_dungeon_plan` ADD COLUMN `kill_first` varchar(255) NOT NULL DEFAULT '''' AFTER `cc_always`',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_dungeon_plan' AND COLUMN_NAME = 'kill_first');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
