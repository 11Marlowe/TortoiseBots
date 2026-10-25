-- Additive compatibility for installations that applied the original native
-- migration before RandomItemMgr's full weight-scale writer was reconciled.

SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_1` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_1');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_2` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_2');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_3` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_3');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_4` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_4');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_5` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_5');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_6` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_6');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_7` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_7');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_8` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_8');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_9` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_9');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_10` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_10');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_11` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_11');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_12` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_12');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_13` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_13');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_14` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_14');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_15` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_15');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_16` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_16');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_17` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_17');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_18` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_18');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_19` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_19');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_20` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_20');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_21` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_21');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_22` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_22');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_23` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_23');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_24` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_24');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_25` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_25');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_26` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_26');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_27` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_27');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_28` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_28');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_29` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_29');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_30` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_30');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_31` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_31');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @s := (SELECT IF(COUNT(*) = 0,
  'ALTER TABLE `ai_playerbot_item_info_cache` ADD COLUMN `scale_32` mediumint NOT NULL DEFAULT 0',
  'DO 0')
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_playerbot_item_info_cache' AND COLUMN_NAME = 'scale_32');
PREPARE stmt FROM @s;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
