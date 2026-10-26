-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- scarlet_library: map 189, 3 legs, 88 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'scarlet_library';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'scarlet_library';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'scarlet_library';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('scarlet_library', 189, '4299,4296', '4299', '4291', 'turtle-agent c670d6e dungeons/scarlet_library.json', 'plan v1, draft (not mapped, not live-checked)');
INSERT INTO `ai_dungeon_leg` (`plan`, `leg`, `name`, `comment`) VALUES
('scarlet_library', 0, 'houndmaster_loksey', ''),
('scarlet_library', 1, 'brother_wystan', ''),
('scarlet_library', 2, 'arcanist_doan', '');
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('scarlet_library', 0, 0, 'move', 0, 240.84, -229.627, 18.6141, 0, 0, 0, 0, 0, '', 'to'),
('scarlet_library', 0, 1, 'pull', 0, 240.88, -231.54, 18.61, 12, 0, 0, 0, 0, '4287', 'pull group 0'),
('scarlet_library', 0, 2, 'pull', 0, 218.38, -226.36, 18.61, 12, 0, 0, 0, 0, '4287,4296', 'pull group 1'),
('scarlet_library', 0, 3, 'pull', 0, 201.14, -231.66, 18.53, 11.3, 0, 0, 0, 0, '4304,4288', 'pull group 2'),
('scarlet_library', 0, 4, 'pull', 0, 201, -231.96, 18.53, 10, 0, 0, 0, 0, '4296', 'pull group 3'),
('scarlet_library', 0, 5, 'pull', 0, 193.48, -228.07, 18.61, 11.5, 0, 0, 0, 0, '4287,4296', 'pull group 4'),
('scarlet_library', 0, 6, 'pull', 0, 188.11, -258.54, 18.75, 11.3, 0, 0, 0, 0, '4304,4288', 'pull group 5'),
('scarlet_library', 0, 7, 'pull', 0, 182.41, -274.64, 18.79, 11.5, 0, 0, 0, 0, '4288,4304', 'pull group 6'),
('scarlet_library', 0, 8, 'pull', 0, 182.79, -289.65, 19.28, 10, 0, 0, 0, 0, '4287', 'pull group 7'),
('scarlet_library', 0, 9, 'pull', 0, 199.24, -287.59, 19.2, 10, 0, 0, 0, 0, '4287', 'pull group 8'),
('scarlet_library', 0, 10, 'pull', 0, 196.28, -306.49, 18.61, 11.6, 0, 0, 0, 0, '4287', 'pull group 9'),
('scarlet_library', 0, 11, 'pull', 0, 174.66, -312.68, 18.61, 10, 0, 0, 0, 0, '4287', 'pull group 10'),
('scarlet_library', 0, 12, 'pull', 0, 155.75, -285.67, 19.2, 10, 0, 0, 0, 0, '4287', 'pull group 11'),
('scarlet_library', 0, 13, 'pull', 0, 153.94, -268.83, 19.28, 10, 0, 0, 0, 0, '4287', 'pull group 12'),
('scarlet_library', 0, 14, 'pull', 0, 167.61, -268.1, 18.66, 10.9, 0, 0, 0, 0, '4288,4304', 'pull group 13'),
('scarlet_library', 0, 15, 'pull', 0, 203.7, -268.73, 19.28, 10, 0, 0, 0, 0, '4287', 'pull group 14'),
('scarlet_library', 0, 16, 'pull', 0, 220.76, -286.88, 19.28, 12.9, 0, 0, 0, 0, '4287,4296', 'pull group 15'),
('scarlet_library', 0, 17, 'move', 0, 218.13327, -298.66672, 19.205809, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 18, 'move', 0, 217.59995, -300.80005, 18.905807, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 19, 'move', 0, 213.33327, -301.60004, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 20, 'move', 0, 197.86661, -302.66672, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 21, 'move', 0, 181.59995, -302.1334, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 22, 'move', 0, 177.59995, -300.80005, 18.905807, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 23, 'move', 0, 177.33328, -298.66672, 19.205809, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 24, 'move', 0, 177.59995, -293.60004, 19.205809, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 25, 'move', 0, 177.86662, -291.4667, 19.205809, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 26, 'move', 0, 187.46661, -283.7334, 20.505808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 27, 'move', 0, 196.53328, -274.1334, 20.505808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 28, 'move', 0, 197.86661, -272.5334, 19.205809, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 29, 'move', 0, 203.99994, -234.66673, 18.905807, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 30, 'move', 0, 204.2666, -234.13339, 18.905807, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 31, 'move', 0, 208.2666, -233.3334, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 32, 'move', 0, 223.99994, -232.26674, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 33, 'pull', 0, 269.63, -233.8, 18.61, 11.8, 0, 0, 0, 0, '4287', 'pull group 16'),
('scarlet_library', 0, 34, 'move', 0, 251.19994, -232.26674, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 35, 'move', 0, 223.99994, -232.26674, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 36, 'move', 0, 208.2666, -233.3334, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 37, 'move', 0, 204.2666, -234.13339, 18.905807, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 38, 'move', 0, 197.86661, -238.13339, 19.205809, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 39, 'move', 0, 162.39995, -245.86673, 20.505808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 40, 'move', 0, 160.5333, -247.7334, 20.505808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 41, 'move', 0, 151.99995, -258.93338, 19.205809, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 42, 'move', 0, 149.3333, -259.20004, 19.205809, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 0, 43, 'boss', 3974, 119.938, -260.869, 18.6299, 0, 0, 0, 0, 0, '4304', 'boss'),
('scarlet_library', 1, 0, 'move', 0, 151.73329, -262.66672, 19.205809, 0, 0, 0, 0, 0, '', 'path'),
('scarlet_library', 1, 1, 'move', 0, 166.66663, -273.60007, 19.005808, 0, 0, 0, 0, 0, '', 'path'),
('scarlet_library', 1, 2, 'move', 0, 167.73329, -274.40005, 18.905807, 0, 0, 0, 0, 0, '', 'path'),
('scarlet_library', 1, 3, 'move', 0, 171.73329, -281.0667, 20.505808, 0, 0, 0, 0, 0, '', 'path'),
('scarlet_library', 1, 4, 'move', 0, 172.26662, -282.40005, 20.805807, 0, 0, 0, 0, 0, '', 'path'),
('scarlet_library', 1, 5, 'move', 0, 177.59995, -300.80005, 18.905807, 0, 0, 0, 0, 0, '', 'path'),
('scarlet_library', 1, 6, 'move', 0, 211.489, -312.366, 18.6126, 0, 0, 0, 0, 0, '', 'to'),
('scarlet_library', 1, 7, 'pull', 0, 211.49, -312.37, 18.61, 10, 0, 0, 0, 0, '4287', 'pull group 0'),
('scarlet_library', 1, 8, 'pull', 0, 228.85, -307.32, 18.61, 12.8, 0, 0, 0, 0, '4287', 'pull group 1'),
('scarlet_library', 1, 9, 'pull', 0, 243.61, -303.31, 20.16, 13.6, 0, 0, 0, 0, '4299,4540', 'pull group 2'),
('scarlet_library', 1, 10, 'pull', 0, 265.46, -306.14, 20.13, 16.3, 0, 0, 0, 0, '4540,4291', 'pull group 3'),
('scarlet_library', 1, 11, 'pull', 0, 272.49, -328.86, 18.62, 12.3, 0, 0, 0, 0, '4540,4299', 'pull group 4'),
('scarlet_library', 1, 12, 'pull', 0, 250.93, -333.9, 18.62, 13.3, 0, 0, 0, 0, '4291', 'pull group 5'),
('scarlet_library', 1, 13, 'pull', 0, 232.17, -325.76, 18.62, 10, 0, 0, 0, 0, '4291', 'pull group 6'),
('scarlet_library', 1, 14, 'pull', 0, 216.41, -334.12, 18.53, 18.6, 0, 0, 0, 0, '4540,4291,4299', 'pull group 7'),
('scarlet_library', 1, 15, 'pull', 0, 232.34, -377.61, 23.87, 13.1, 0, 0, 0, 0, '4540', 'pull group 8'),
('scarlet_library', 1, 16, 'pull', 0, 220.82, -386.67, 23.84, 10, 0, 0, 0, 0, '4540', 'pull group 9'),
('scarlet_library', 1, 17, 'pull', 0, 215.89, -420.26, 18.62, 11.9, 0, 0, 0, 0, '4291,4540', 'pull group 10'),
('scarlet_library', 1, 18, 'pull', 0, 231.85, -429.12, 18.62, 13.7, 0, 0, 0, 0, '4291', 'pull group 11'),
('scarlet_library', 1, 19, 'pull', 0, 253.05, -432.9, 18.62, 10, 0, 0, 0, 0, '4291', 'pull group 12'),
('scarlet_library', 1, 20, 'pull', 0, 265.02, -428.94, 18.53, 10, 0, 0, 0, 0, '4540', 'pull group 13'),
('scarlet_library', 1, 21, 'pull', 0, 270.07, -435.69, 18.62, 10, 0, 0, 0, 0, '4291', 'pull group 14'),
('scarlet_library', 1, 22, 'pull', 0, 278.46, -408, 20.49, 12.4, 0, 0, 0, 0, '4291,4540', 'pull group 15'),
('scarlet_library', 1, 23, 'pull', 0, 254.05, -407.71, 20.49, 13.2, 0, 0, 0, 0, '4540,4299', 'pull group 16'),
('scarlet_library', 1, 24, 'pull', 0, 264.57, -386.73, 20.16, 11.4, 0, 0, 0, 0, '4291,4540', 'pull group 17'),
('scarlet_library', 1, 25, 'pull', 0, 265.03, -379.05, 20.08, 10, 0, 0, 0, 0, '4299', 'pull group 18'),
('scarlet_library', 1, 26, 'pull', 0, 278.76, -371.62, 20.49, 12.2, 0, 0, 0, 0, '4299,4291', 'pull group 19'),
('scarlet_library', 1, 27, 'pull', 0, 252.6, -365.34, 20.48, 11.9, 0, 0, 0, 0, '4291,4540', 'pull group 20'),
('scarlet_library', 1, 28, 'move', 0, 261.0666, -373.06668, 20.105808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 29, 'move', 0, 262.6666, -374.6667, 21.305807, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 30, 'move', 0, 263.99994, -376.80002, 20.105808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 31, 'move', 0, 263.99994, -415.2, 20.105808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 32, 'move', 0, 263.73325, -418.40002, 19.505808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 33, 'move', 0, 257.33325, -424.26666, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 34, 'move', 0, 209.86661, -428.26666, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 35, 'move', 0, 207.46661, -428.26666, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 36, 'move', 0, 204.79994, -428, 18.605808, 0, 0, 0, 0, 0, '', 'approach path'),
('scarlet_library', 1, 37, 'pull', 0, 182.83, -414.78, 18.94, 12, 0, 0, 0, 0, '4540,4291', 'pull group 21'),
('scarlet_library', 1, 38, 'pull', 0, 197.23, -432.25, 18.53, 10, 0, 0, 0, 0, '4540', 'pull group 22'),
('scarlet_library', 1, 39, 'boss', 61983, 209.491, -401.61, 23.8661, 0, 0, 0, 0, 0, '4540', 'boss'),
('scarlet_library', 2, 0, 'move', 0, 173.965, -427.5, 18.6164, 0, 0, 0, 0, 0, '', 'to'),
('scarlet_library', 2, 1, 'pull', 0, 173.97, -429.18, 18.62, 9, 0, 0, 0, 0, '4540', 'pull group 0'),
('scarlet_library', 2, 2, 'pull', 0, 181.14, -443.63, 18.94, 9, 0, 0, 0, 0, '4291,4540,4299', 'pull group 1'),
('scarlet_library', 2, 3, 'boss', 6487, 148.32, -428.692, 18.4864, 0, 0, 0, 0, 0, '', 'boss');


-- The dashboard's boss table: rank and name for the map's bosses (the step rows cannot carry the creature_template's
-- data). Same pattern as 20261021114931_world.sql. Only the library's three are on the map of this plan.
DELETE FROM `ai_dungeon_boss` WHERE `map` IN (189);
REPLACE INTO `ai_dungeon_boss` (`map`, `entry`, `name`) VALUES
(189, 3974, 'Houndmaster Loksey'),
(189, 61983, 'Brother Wystan'),
(189, 6487, 'Arcanist Doan');
