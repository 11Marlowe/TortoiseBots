-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- scarlet_cathedral: map 189, 2 legs, 78 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'scarlet_cathedral';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'scarlet_cathedral';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'scarlet_cathedral';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('scarlet_cathedral', 189, '', '', '4303,4300,4299', 'turtle-agent 39dd2ca dungeons/scarlet_cathedral.json', 'plan v1, draft (not mapped, not live-checked)');
INSERT INTO `ai_dungeon_leg` (`plan`, `leg`, `name`, `comment`) VALUES
('scarlet_cathedral', 0, 'high_inquisitor_fairbanks', ''),
('scarlet_cathedral', 1, 'scarlet_commander_mograine', '');
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('scarlet_cathedral', 0, 0, 'move', 0, 876.701, 1322.55, 18.0893, 0, 0, 0, 0, 0, '', 'to'),
('scarlet_cathedral', 0, 1, 'pull', 0, 876.7, 1322.55, 18.09, 10, 0, 0, 0, 0, '4295', 'pull group 0'),
('scarlet_cathedral', 0, 2, 'pull', 0, 870.88, 1359.36, 18.09, 15.7, 0, 0, 0, 0, '4298', 'pull group 1'),
('scarlet_cathedral', 0, 3, 'pull', 0, 870.85, 1406.03, 18.09, 12.5, 0, 0, 0, 0, '4298,4295', 'pull group 2'),
('scarlet_cathedral', 0, 4, 'pull', 0, 889.97, 1376.13, 18.76, 11.9, 0, 0, 0, 0, '4294,4295', 'pull group 3'),
('scarlet_cathedral', 0, 5, 'pull', 0, 890.28, 1360.67, 18.76, 10, 0, 0, 0, 0, '4298', 'pull group 4'),
('scarlet_cathedral', 0, 6, 'move', 0, 940.19037, 1363.4357, 19.845392, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 7, 'move', 0, 959.9999, 1364.5333, 20.300001, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 8, 'move', 0, 961.3332, 1365.3334, 20, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 9, 'move', 0, 967.46655, 1375.2, 20.300001, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 10, 'move', 0, 974.39984, 1375.4667, 23.4, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 11, 'move', 0, 981.3332, 1375.4667, 25.800001, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 12, 'pull', 0, 987.71, 1343.82, 29.19, 11.7, 0, 0, 0, 0, '4295,4301', 'pull group 5'),
('scarlet_cathedral', 0, 13, 'pull', 0, 1015.45, 1343.83, 29.15, 11.7, 0, 0, 0, 0, '4295', 'pull group 6'),
('scarlet_cathedral', 0, 14, 'pull', 0, 1044.22, 1343.42, 29.15, 12.8, 0, 0, 0, 0, '4295', 'pull group 7'),
('scarlet_cathedral', 0, 15, 'move', 0, 1045.3331, 1344, 29.2, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 16, 'move', 0, 1051.7332, 1384.8, 29.1, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 17, 'move', 0, 1067.7334, 1393.3334, 30.800001, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 18, 'move', 0, 1070.1334, 1394.4, 30.800001, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 19, 'move', 0, 1071.7334, 1394.6666, 30.800001, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 20, 'move', 0, 1089.8667, 1383.7333, 31.1, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 21, 'move', 0, 1127.2001, 1373.3334, 31.300001, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 22, 'move', 0, 1130.6667, 1372.2666, 31.2, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_cathedral', 0, 23, 'pull', 0, 1140.18, 1340.5, 30.39, 10, 0, 0, 0, 0, '4300,4301,4302', 'pull group 8'),
('scarlet_cathedral', 0, 24, 'boss', 4542, 1158.86, 1354.27, 30.389, 0, 0, 0, 0, 0, '4540', 'boss'),
('scarlet_cathedral', 1, 0, 'move', 0, 1131.25, 1390.61, 30.3824, 0, 0, 0, 0, 0, '', 'to'),
('scarlet_cathedral', 1, 1, 'move', 0, 1095, 1420, 30.5578, 0, 0, 0, 0, 0, '', 'pull group 0 approach'),
('scarlet_cathedral', 1, 2, 'pull', 0, 1131.18, 1391.82, 30.38, 11.3, 0, 0, 0, 0, '4540,4302', 'pull group 0'),
('scarlet_cathedral', 1, 3, 'move', 0, 1100, 1414, 30.6191, 0, 0, 0, 0, 0, '', 'pull group 1 approach'),
('scarlet_cathedral', 1, 4, 'pull', 0, 1131.07, 1406.46, 30.39, 9, 0, 0, 0, 0, '4303,4540', 'pull group 1'),
('scarlet_cathedral', 1, 5, 'pull', 0, 1114, 1400.55, 30.39, 10, 0, 0, 0, 0, '4540,4302,4303', 'pull group 2'),
('scarlet_cathedral', 1, 6, 'pull', 0, 1093.91, 1406.32, 30.38, 10, 0, 0, 0, 0, '4303,4540', 'pull group 3'),
('scarlet_cathedral', 1, 7, 'pull', 0, 1093.98, 1392.05, 30.38, 11.1, 0, 0, 0, 0, '4302,4540', 'pull group 4'),
('scarlet_cathedral', 1, 8, 'pull', 0, 1079.82, 1376.72, 30.31, 10, 0, 0, 0, 0, '4299', 'pull group 5'),
('scarlet_cathedral', 1, 9, 'pull', 0, 1080.24, 1375.09, 30.39, 13.1, 0, 0, 0, 0, '4299,4301', 'pull group 6'),
('scarlet_cathedral', 1, 10, 'pull', 0, 1063.73, 1384.76, 30.85, 12.7, 0, 0, 0, 0, '4301', 'pull group 7'),
('scarlet_cathedral', 1, 11, 'pull', 0, 1041.41, 1400.29, 27.39, 10, 0, 0, 0, 0, '4295', 'pull group 8'),
('scarlet_cathedral', 1, 12, 'pull', 0, 1035.56, 1418.87, 27.39, 10, 0, 0, 0, 0, '4298', 'pull group 9'),
('scarlet_cathedral', 1, 13, 'pull', 0, 1042.55, 1436.11, 27.38, 10, 0, 0, 0, 0, '4301', 'pull group 10'),
('scarlet_cathedral', 1, 14, 'pull', 0, 1044.34, 1455.02, 29.23, 12.1, 0, 0, 0, 0, '4294,4301', 'pull group 11'),
('scarlet_cathedral', 1, 15, 'pull', 0, 1016.33, 1454.71, 29.19, 11.5, 0, 0, 0, 0, '4300,4301', 'pull group 12'),
('scarlet_cathedral', 1, 16, 'pull', 0, 1016.04, 1436.88, 27.39, 10, 0, 0, 0, 0, '4294', 'pull group 13'),
('scarlet_cathedral', 1, 17, 'pull', 0, 991.13, 1437.42, 27.38, 10, 0, 0, 0, 0, '4298', 'pull group 14'),
('scarlet_cathedral', 1, 18, 'pull', 0, 986.83, 1455.31, 29.19, 11.3, 0, 0, 0, 0, '4298,4294', 'pull group 15'),
('scarlet_cathedral', 1, 19, 'pull', 0, 959.86, 1437.52, 18.76, 12.2, 0, 0, 0, 0, '4298,4295', 'pull group 16'),
('scarlet_cathedral', 1, 20, 'pull', 0, 948.8, 1423.6, 18.11, 10, 0, 0, 0, 0, '4300', 'pull group 17'),
('scarlet_cathedral', 1, 21, 'pull', 0, 964.3, 1414.41, 18.76, 10, 0, 0, 0, 0, '4301', 'pull group 18'),
('scarlet_cathedral', 1, 22, 'pull', 0, 931.61, 1437.55, 18.76, 10, 0, 0, 0, 0, '4295', 'pull group 19'),
('scarlet_cathedral', 1, 23, 'pull', 0, 903.93, 1437.55, 18.68, 10, 0, 0, 0, 0, '4300', 'pull group 20'),
('scarlet_cathedral', 1, 24, 'pull', 0, 892.58, 1436.19, 18.76, 12.7, 0, 0, 0, 0, '4295,4294', 'pull group 21'),
('scarlet_cathedral', 1, 25, 'pull', 0, 889.99, 1420.7, 18.76, 10, 0, 0, 0, 0, '4295', 'pull group 22'),
('scarlet_cathedral', 1, 26, 'pull', 0, 908.5, 1421.89, 18.08, 10, 0, 0, 0, 0, '4298', 'pull group 23'),
('scarlet_cathedral', 1, 27, 'pull', 0, 909.23, 1399.03, 18.11, 10, 0, 0, 0, 0, '4301', 'pull group 24'),
('scarlet_cathedral', 1, 28, 'pull', 0, 909.25, 1378.57, 18.08, 10, 0, 0, 0, 0, '4301', 'pull group 25'),
('scarlet_cathedral', 1, 29, 'pull', 0, 925.63, 1377.55, 18.02, 10, 0, 0, 0, 0, '4301', 'pull group 26'),
('scarlet_cathedral', 1, 30, 'pull', 0, 929.25, 1360.17, 18.76, 12, 0, 0, 0, 0, '4294', 'pull group 27'),
('scarlet_cathedral', 1, 31, 'pull', 0, 947.38, 1375.67, 18.11, 10, 0, 0, 0, 0, '4300', 'pull group 28'),
('scarlet_cathedral', 1, 32, 'pull', 0, 958.62, 1360.57, 18.76, 12.1, 0, 0, 0, 0, '4295,4294', 'pull group 29'),
('scarlet_cathedral', 1, 33, 'pull', 0, 992.71, 1362.11, 27.38, 10, 0, 0, 0, 0, '4301', 'pull group 30'),
('scarlet_cathedral', 1, 34, 'pull', 0, 1021.64, 1362.29, 27.39, 10, 0, 0, 0, 0, '4300', 'pull group 31'),
('scarlet_cathedral', 1, 35, 'pull', 0, 1042.64, 1363.84, 27.38, 10, 0, 0, 0, 0, '4298', 'pull group 32'),
('scarlet_cathedral', 1, 36, 'pull', 0, 1035.74, 1378.37, 27.39, 10, 0, 0, 0, 0, '4301', 'pull group 33'),
('scarlet_cathedral', 1, 37, 'pull', 0, 1063.99, 1413.39, 30.85, 12.7, 0, 0, 0, 0, '4301', 'pull group 34'),
('scarlet_cathedral', 1, 38, 'pull', 0, 1080.32, 1421.53, 30.39, 14.5, 0, 0, 0, 0, '4299,4301', 'pull group 35'),
('scarlet_cathedral', 1, 39, 'pull', 0, 1082.92, 1417.33, 30.3, 10, 0, 0, 0, 0, '4300', 'pull group 36'),
('scarlet_cathedral', 1, 40, 'pull', 0, 1101.94, 1422.63, 30.39, 10, 0, 0, 0, 0, '4301,4302', 'pull group 37'),
('scarlet_cathedral', 1, 41, 'pull', 0, 1121.14, 1422.52, 30.39, 14.2, 0, 0, 0, 0, '4301,4302', 'pull group 38'),
('scarlet_cathedral', 1, 42, 'move', 0, 1134.5, 1431.5, 30.5, 0, 0, 0, 0, 0, '', 'pull group 39 approach'),
('scarlet_cathedral', 1, 43, 'pull', 0, 1138.47, 1456.49, 30.38, 16.9, 0, 0, 0, 0, '4300,4299,4301', 'pull group 39'),
('scarlet_cathedral', 1, 44, 'move', 0, 1130, 1436, 30.937, 0, 0, 0, 0, 0, '', 'pull group 40 approach'),
('scarlet_cathedral', 1, 45, 'pull', 0, 1135.38, 1459.44, 30.3, 10, 0, 0, 0, 0, '4300', 'pull group 40'),
('scarlet_cathedral', 1, 46, 'move', 0, 1146, 1462, 30.4573, 0, 0, 0, 0, 0, '', 'pull group 41 approach'),
('scarlet_cathedral', 1, 47, 'pull', 0, 1162.71, 1456.77, 31.01, 13.2, 0, 0, 0, 0, '4299,4540', 'pull group 41'),
('scarlet_cathedral', 1, 48, 'move', 0, 1150, 1450, 30.8036, 0, 0, 0, 0, 0, '', 'pull group 42 approach'),
('scarlet_cathedral', 1, 49, 'pull', 0, 1164.05, 1438.33, 31.22, 13.9, 0, 0, 0, 0, '4540,4302', 'pull group 42'),
('scarlet_cathedral', 1, 50, 'move', 0, 1118, 1415, 30.487, 0, 0, 0, 0, 0, '', 'pull group 43 approach'),
('scarlet_cathedral', 1, 51, 'pull', 0, 1112.35, 1376.42, 30.35, 14, 0, 0, 0, 0, '4300,4301,4302', 'pull group 43'),
('scarlet_cathedral', 1, 52, 'hold', 1, 1140, 1398, 31.0314, 35, 0, 4, 8, 600, '3976,3977', 'boss hold: the event''s mobs are fought on the spot until instance data 1 reads 4 (a script value, not an encounter state)');

-- The dashboard's boss table: rank and name for the map's bosses (the step rows cannot carry the creature_template's
-- data). Same pattern as 20261023030100_world.sql. Only this wing's three bosses are replaced: map 189 holds four plans.
DELETE FROM `ai_dungeon_boss` WHERE `map` IN (189) AND `entry` IN (4542, 3976, 3977);
REPLACE INTO `ai_dungeon_boss` (`map`, `entry`, `name`) VALUES
(189, 4542, 'High Inquisitor Fairbanks'),
(189, 3976, 'Scarlet Commander Mograine'),
(189, 3977, 'High Inquisitor Whitemane');
