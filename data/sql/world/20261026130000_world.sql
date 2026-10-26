-- Items and keys a run needs (spec item-leg-skip A): one row per (plan, leg, item).
--
-- mode: 1 optional (the leg is skipped when nobody in the party carries the item;
-- Zul'Farrak's gahzrilla leg: the Mallet of Zul'Farrak), 2 door (a key door into
-- the leg: with a real player in the party the start warns and the run holds at
-- the door until the player opens it; bot-only runs walk through), 3 blocking (an
-- item the run must use: with a player the start warns and the run asks at the
-- use step). pick: the Lockpicking skill that opens a door lock instead (Lock.dbc,
-- 0 none, at most 300), door rows only. object: the key door's gameobject entry
-- (the hold watches it open), door rows only. name: the item's name, for the
-- notes. how: door rows only: where a player gets the key (short, for the notes).
--
-- Read once at startup and by `.bot run reloadplans` (ai/playerbot/DungeonPlanMgr.cpp).
-- Never edit this file once applied; add rows with a new migration.

CREATE TABLE IF NOT EXISTS `ai_dungeon_leg_need` (
  `plan` varchar(32) NOT NULL,
  `leg` tinyint unsigned NOT NULL,
  `item` int unsigned NOT NULL,
  `mode` tinyint unsigned NOT NULL,
  `pick` smallint unsigned NOT NULL DEFAULT 0,
  `object` int unsigned NOT NULL DEFAULT 0,
  `name` varchar(64) NOT NULL DEFAULT '',
  `how` varchar(255) NOT NULL DEFAULT '',
  `comment` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`plan`, `leg`, `item`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'zulfarrak';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('zulfarrak', 9, 9240, 1, 0, 0, 'Mallet of Zul''Farrak', '', 'optional: gahzrilla');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'blackrock_depths';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('blackrock_depths', 1, 11000, 2, 250, 170560, 'Shadowforge Key', 'Dark Iron Legacy quest (Searing Gorge/BRD)', 'Shadowforge Gate 170560'),
('blackrock_depths', 2, 11000, 2, 250, 170560, 'Shadowforge Key', 'Dark Iron Legacy quest (Searing Gorge/BRD)', 'Shadowforge Gate 170560'),
('blackrock_depths', 3, 11000, 2, 250, 170560, 'Shadowforge Key', 'Dark Iron Legacy quest (Searing Gorge/BRD)', 'Shadowforge Gate 170560'),
('blackrock_depths', 4, 11000, 2, 250, 170560, 'Shadowforge Key', 'Dark Iron Legacy quest (Searing Gorge/BRD)', 'Shadowforge Gate 170560'),
('blackrock_depths', 5, 11000, 2, 250, 170560, 'Shadowforge Key', 'Dark Iron Legacy quest (Searing Gorge/BRD)', 'Shadowforge Gate 170560'),
('blackrock_depths', 6, 11000, 2, 250, 170560, 'Shadowforge Key', 'Dark Iron Legacy quest (Searing Gorge/BRD)', 'Shadowforge Gate 170560'),
('blackrock_depths', 7, 11000, 2, 250, 170560, 'Shadowforge Key', 'Dark Iron Legacy quest (Searing Gorge/BRD)', 'Shadowforge Gate 170560'),
('blackrock_depths', 8, 11000, 2, 250, 170560, 'Shadowforge Key', 'Dark Iron Legacy quest (Searing Gorge/BRD)', 'Shadowforge Gate 170560');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'scarlet_cathedral';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('scarlet_cathedral', 0, 7146, 2, 175, 104591, 'The Scarlet Key', 'SM Library chest (Arcanist Doan)', 'Chapel Door 104591');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'scarlet_armory';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('scarlet_armory', 1, 7146, 2, 175, 101854, 'The Scarlet Key', 'SM Library chest (Arcanist Doan)', 'Herod''s Door 101854');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'stratholme_undead';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('stratholme_undead', 0, 12382, 2, 300, 175357, 'Key to the City', 'Stratholme: Magistrate Barthilas (100 %)', 'Gauntlet Gate 175357'),
('stratholme_undead', 1, 12382, 2, 300, 175357, 'Key to the City', 'Stratholme: Magistrate Barthilas (100 %)', 'Gauntlet Gate 175357');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'scholomance';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('scholomance', 1, 13873, 2, 0, 175167, 'Viewing Room Key', 'Scholomance: Rattlegore (100 %), quest chain', 'Viewing Room Door 175167'),
('scholomance', 2, 13873, 2, 0, 175167, 'Viewing Room Key', 'Scholomance: Rattlegore (100 %), quest chain', 'Viewing Room Door 175167'),
('scholomance', 3, 13873, 2, 0, 175167, 'Viewing Room Key', 'Scholomance: Rattlegore (100 %), quest chain', 'Viewing Room Door 175167'),
('scholomance', 4, 13873, 2, 0, 175167, 'Viewing Room Key', 'Scholomance: Rattlegore (100 %), quest chain', 'Viewing Room Door 175167'),
('scholomance', 8, 13873, 2, 0, 175167, 'Viewing Room Key', 'Scholomance: Rattlegore (100 %), quest chain', 'Viewing Room Door 175167');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'dire_maul_west';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('dire_maul_west', 2, 18249, 2, 300, 300601, 'Crescent Key', 'Dire Maul: Pusillin (East), quest', 'DM West Door 300601'),
('dire_maul_west', 3, 18249, 2, 300, 300601, 'Crescent Key', 'Dire Maul: Pusillin (East), quest', 'DM West Door 300601'),
('dire_maul_west', 4, 18249, 2, 300, 300601, 'Crescent Key', 'Dire Maul: Pusillin (East), quest', 'DM West Door 300601'),
('dire_maul_west', 5, 18249, 2, 300, 300601, 'Crescent Key', 'Dire Maul: Pusillin (East), quest', 'DM West Door 300601');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'dire_maul_north';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('dire_maul_north', 2, 18266, 2, 0, 177219, 'Gordok Courtyard Key', 'Dire Maul: Fengus''s Chest (100 %)', 'Courtyard 177219'),
('dire_maul_north', 2, 18268, 2, 200, 177217, 'Gordok Inner Door Key', 'Dire Maul: Guard Mol''dar (100 %)', 'Inner 177217'),
('dire_maul_north', 3, 18268, 2, 200, 177217, 'Gordok Inner Door Key', 'Dire Maul: Guard Mol''dar (100 %)', 'Inner 177217');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'gnomeregan';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('gnomeregan', 2, 6893, 2, 150, 90858, 'Workshop Key', 'Gnomeregan drop', 'Workshop 90858');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'stormwrought_ruins';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('stormwrought_ruins', 4, 41913, 2, 185, 2020233, 'Key to Stormwrought Castle', 'quest 41837', 'Castle Gate 2020233'),
('stormwrought_ruins', 5, 41913, 2, 185, 2020233, 'Key to Stormwrought Castle', 'quest 41837', 'Castle Gate 2020233');

DELETE FROM `ai_dungeon_leg_need` WHERE `plan` = 'dragonmaw_retreat';
INSERT INTO `ai_dungeon_leg_need` (`plan`, `leg`, `item`, `mode`, `pick`, `object`, `name`, `how`, `comment`) VALUES
('dragonmaw_retreat', 3, 41876, 2, 160, 2020232, 'Lower Reserve Key', 'quest 41774', 'Lower Reserve Gate 2020232');
