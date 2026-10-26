-- Karazhan Crypt (map 800): the plan reordered after r40 (RUN-LESSONS "r40 karazhan_crypt slot5"), exported from turtle-agent
-- prep/karazhan_crypt b5ac639 (dungeons/karazhan_crypt.json) with feat/offmesh-links 2756955. This file replaces the rows of
-- 20261025001100: the plan row, its six legs and all 259 steps are deleted and inserted again, and it carries NO
-- ai_dungeon_boss row - the six bosses of map 800 were seeded in 20261024021100 and are unchanged.
--   turtle-agent plan-export --dir dungeons --mmaps <map 800 mmaps with the rebuilt 8005334> --allow-draft --approach-hops \
--     --rev b5ac639 --stamp 20261025001400 karazhan_crypt
--
-- WHY THE REORDER. r40 ran the 2026-10-25 export live from the door: 0 deaths, 113 of 275 steps in 45 min, Archlich Enkhraz,
-- Marrowspike and Hivaxxis down; the dive worked for all five bots in 12 s (06:29:25 -> 06:29:37), the four Drowned Sinners
-- 91932 at the shaft floor and the Shadowblade Spectre 91918 pull worked (tests/r40/slot5/). It then stopped at step 113 (old
-- leg 3 guard_captain_gort, first move after Hivaxxis) by the stop rule, 4 pauses with the same note: the plan sent the party
-- from the lower crypt (z -15.7) back up to the walkway (-11223.6 -1728.8 4.3), and the only way up is the dive link in
-- reverse, from the water. The run leader walked the lead to the shaft floor (-11221.3 -1560 -45.9) and it made no way toward
-- (-11221.3 -1626.7 8.6) ("ends where the last one did ... the lead bot made no way toward it"); a water-to-water move is what
-- the bots' mover refuses (BFD C2/C3). So the link is effectively one-way DOWN, and the plan now never asks to come back up.
--
-- THE ORDER (6 legs, 259 steps = 141 pulls + 113 moves + 5 boss steps, 266 rows):
--   0 archlich_enkhraz    entrance hall (z 52) down to the z 4.3 walkway, boss 91916 (first 91927)
--   1 guard_captain_gort  the walkway z 4.3, boss 92935
--   2 commander_andreon   the upper hall z 28, boss 91919
--   3 marrowspike_dive    the dive and the shaft floor: the dry ramp point -11219.96 -1586.82 4.13 (water surface 3.046), one
--                         56.8 yd move through the link to -11221.3 -1560 -45.9, the four Drowned Sinners at the floor, the
--                         slope pack (2 Risen Crypt Guards + Spectre 91918) from -11213.8 -1632 -34.3
--   4 marrowspike         the lower crypt z -28.5, boss 91920
--   5 hivaxxis            the lower crypt z -15.7, boss 91929; the run ends here, down there
-- The dive leg's five pull groups, the Sinners' stands and the Spectre stand are the rows r40 ran, unchanged.
--
-- THE ROWS r40 ASKED FOR.
--   * step 67 (old leg 1 seq 15) was "cmd move -11200.0 -1749.3 -29.4 refused (unreachable)": the exporter's cut approach row
--     to pull group 5. The run walked the 6.1 yd to the nearest reachable point -11194.8 -1752.3 -28.3 and went on. That point
--     is now a VIA row of the marrowspike leg, and the leg's `to` is r40's next row -11199.199 -1770.6667 -29.27348 (the lead
--     stood there at 06:33:59 and the pull from it worked). The via chain before it is r40's own walk: -11211 -1689 -28.5
--     (step 65) and -11202.032 -1738.1836 -29.204737 (step 66). The leg is split (marrowspike_dive / marrowspike) because a
--     via row is walked before a leg's `to`, not between pull groups; pull group 5's old approach (-11211 -1689) is dropped so
--     the walk does not go back.
--   * commander_andreon's `to` was the terrace (-11118.3 -1630.84 5.87) - a gort-leg pull point - while its first upper-hall
--     pack stands 23.7 yd straight ABOVE it and 297.6 yd away on the navmesh (the hall's own way up). The run leader's `seen`
--     is a 3D distance (kSeeYd 60), so from the terrace it would have seen the hall pack and pulled it through the ceiling. The
--     `to` is now a hall-floor point 21.0 yd along that path from pull group 0 (-11096.95 -1642.28 28.8).
--   * the dive leg keeps one more via, -11200.09 -1621.23 4.82: the exporter's 50 yd cut off the andreon boss had put a point
--     at -11200 -1621.3334 4.3265 that the core's own query answers NOPATH (0x8) although the path's polyline runs through
--     it; the point on the polyline is fine.
--   * CORPSEMUNCHER 91917 IS OUT. His wing (y -1400..-1480) is a navmesh island (79 polygons, KARAZHAN-CRYPT-NAVMESH.md)
--     whose only contact is the swim across the shaft (-11213.1 -1572.3 -2.8 -> -11207.3 -1510.4 -3.8): water at both ends,
--     so the core answers BUILD_UNDERWATER_PATH (0x81) and the mover refuses that type (BFD-STUCK-POINTS C2/C3). r40 shows the
--     same one-way behaviour from the water. His 11 south-wing pulls stay in this file's history (20261025001100 leg 5) for
--     when the swim works. No row of this plan stands in his wing.
--
-- CHECKS (turtle-agent feat/offmesh-links 2756955, the rebuilt 8005334, md5 6a8168b1d173551393bbeb75e4bb79d, on
-- ~/turtle-agent-mmaps): plan-check = ok, 185 points on the map 800 navmesh, 0 failing; 113 warnings (106 estimated patrol
-- periods, 6 detours, 1 off-mesh link - the dive) and 102 run-leader hop notices. server-exact replay of all 259 rows from the
-- party's real stands (ServerPath + the move rules, each pull checked from the stand the party fought at): 0 refused moves,
-- 0 packs unreachable; the only hop over 60 yd is the very first (entrance -> the leg-0 `to`, 61 yd), which r40 walked live.
-- Legs 0-2 hold no point below z -1.5 and legs 4-5 none above z -15.3; no move row starts below z 0 and ends above z 3
-- (the climb r40 could not make). The 91932 pulls are 5 (4 at the floor + 1 on the south ramp in leg 1), the 91930 Tomb
-- Creepers 3; no 14881, no 91927 pull, no 91922, no 91917.

-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- karazhan_crypt: map 800, 6 legs, 259 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'karazhan_crypt';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'karazhan_crypt';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'karazhan_crypt';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('karazhan_crypt', 800, '', '', '91918,91913,91911,91925', 'turtle-agent b5ac639 dungeons/karazhan_crypt.json', 'plan v1, draft (reordered 2026-10-25: the upper floors first, the shaft dive last, the lower crypt at the end; Corpsemuncher out: its wing has no dry contact; not live-checked)');
INSERT INTO `ai_dungeon_leg` (`plan`, `leg`, `name`, `comment`) VALUES
('karazhan_crypt', 0, 'archlich_enkhraz', ''),
('karazhan_crypt', 1, 'guard_captain_gort', ''),
('karazhan_crypt', 2, 'commander_andreon', ''),
('karazhan_crypt', 3, 'marrowspike_dive', ''),
('karazhan_crypt', 4, 'marrowspike', ''),
('karazhan_crypt', 5, 'hivaxxis', '');
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('karazhan_crypt', 0, 0, 'move', 0, -11052.7, -1751.09, 32.7358, 0, 0, 0, 0, 0, '', 'to'),
('karazhan_crypt', 0, 1, 'pull', 0, -11052.7, -1751.09, 32.74, 5, 0, 0, 0, 0, '91924', 'pull group 0'),
('karazhan_crypt', 0, 2, 'pull', 0, -11042.7, -1739.46, 31.39, 5, 0, 0, 0, 0, '91924', 'pull group 1'),
('karazhan_crypt', 0, 3, 'pull', 0, -11031.9, -1751.49, 31.06, 5, 0, 0, 0, 0, '91924', 'pull group 2'),
('karazhan_crypt', 0, 4, 'pull', 0, -11025.6, -1730.81, 29.57, 5, 0, 0, 0, 0, '91924', 'pull group 3'),
('karazhan_crypt', 0, 5, 'pull', 0, -11059.8, -1740.64, 19.3, 5, 0, 0, 1, 0, '91923', 'pull group 4'),
('karazhan_crypt', 0, 6, 'pull', 0, -11069.85, -1740.01, 34.26, 5, 0, 0, 0, 0, '91924,91925', 'pull group 5'),
('karazhan_crypt', 0, 7, 'pull', 0, -11074, -1720.03, 36.11, 5, 0, 0, 0, 0, '91924', 'pull group 6'),
('karazhan_crypt', 0, 8, 'pull', 0, -11092.4, -1728.71, 36.9, 5, 0, 0, 0, 0, '91924', 'pull group 7'),
('karazhan_crypt', 0, 9, 'pull', 0, -11099.8, -1721.12, 36.72, 5, 0, 0, 0, 0, '91925', 'pull group 8'),
('karazhan_crypt', 0, 10, 'pull', 0, -11110.05, -1717.54, 36.36, 5, 0, 0, 0, 0, '91925,91924', 'pull group 9'),
('karazhan_crypt', 0, 11, 'pull', 0, -11107.4, -1727.11, 8.26, 5, 0, 0, 1, 0, '91923', 'pull group 10'),
('karazhan_crypt', 0, 12, 'pull', 0, -11117, -1740.01, 9.02, 5, 0, 0, 0, 0, '91925', 'pull group 11'),
('karazhan_crypt', 0, 13, 'pull', 0, -11089.6, -1749.34, 15.86, 5, 0, 0, 0, 0, '91925', 'pull group 12'),
('karazhan_crypt', 0, 14, 'pull', 0, -11087.9, -1736.98, 16.11, 5, 0, 0, 0, 0, '91925', 'pull group 13'),
('karazhan_crypt', 0, 15, 'pull', 0, -11091.4, -1746.98, 43.59, 5, 0, 0, 0, 0, '91925', 'pull group 14'),
('karazhan_crypt', 0, 16, 'pull', 0, -11103, -1751.17, 44.46, 5, 0, 0, 0, 0, '91925', 'pull group 15'),
('karazhan_crypt', 0, 17, 'move', 0, -11095.732, -1728.8, 37.326523, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 18, 'move', 0, -11093.332, -1728.5333, 37.126522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 19, 'move', 0, -11089.065, -1729.3334, 36.626522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 20, 'move', 0, -11043.732, -1738.9333, 31.62652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 21, 'move', 0, -11037.599, -1738.9333, 31.026522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 22, 'move', 0, -11034.666, -1738.4, 31.32652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 23, 'move', 0, -11033.599, -1738.1334, 31.32652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 24, 'move', 0, -11028.532, -1734.6667, 30.426521, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 25, 'move', 0, -11026.933, -1732.5333, 30.026522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 26, 'move', 0, -11026.133, -1728, 29.026522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 27, 'move', 0, -11027.466, -1720.2667, 27.12652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 28, 'move', 0, -11029.332, -1716.8, 26.12652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 29, 'move', 0, -11035.732, -1713.8667, 24.22652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 30, 'move', 0, -11037.599, -1714.1334, 23.82652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 31, 'move', 0, -11074.933, -1733.6, 18.526522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 32, 'move', 0, -11085.065, -1737.0667, 16.72652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 33, 'move', 0, -11086.666, -1737.0667, 16.426521, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 34, 'move', 0, -11097.599, -1734.1334, 12.826521, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 35, 'move', 0, -11114.666, -1728, 7.2265205, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 36, 'move', 0, -11131.999, -1715.7334, 4.8265204, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 37, 'move', 0, -11132.532, -1703.4667, 3.6265206, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 38, 'move', 0, -11182.503, -1704.7968, 4.6952825, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 39, 'move', 0, -11202.666, -1705.3334, 5.1265206, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 40, 'move', 0, -11202.934, -1706.6667, 5.0265207, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 41, 'move', 0, -11202.666, -1735.4667, 4.3265204, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 42, 'pull', 0, -11198.9, -1776.42, 4.26, 5, 0, 0, 0, 0, '91925', 'pull group 16'),
('karazhan_crypt', 0, 43, 'pull', 0, -11209.7, -1758.61, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 17'),
('karazhan_crypt', 0, 44, 'pull', 0, -11222.7, -1759.02, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 18'),
('karazhan_crypt', 0, 45, 'pull', 0, -11232.2, -1777.35, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 19'),
('karazhan_crypt', 0, 46, 'pull', 0, -11244.55, -1791.93, 4.27, 5, 0, 0, 0, 0, '91924', 'pull group 20'),
('karazhan_crypt', 0, 47, 'pull', 0, -11242.65, -1811.01, 4.3, 5, 0, 0, 0, 0, '91924', 'pull group 21'),
('karazhan_crypt', 0, 48, 'pull', 0, -11228.2, -1822.99, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 22'),
('karazhan_crypt', 0, 49, 'move', 0, -11215.733, -1809.0667, 4.3265204, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 0, 50, 'boss', 91916, -11188.2, -1791.57, 4.29357, 0, 0, 0, 0, 0, '91927', 'boss'),
('karazhan_crypt', 1, 0, 'move', 0, -11220.267, -1775.4667, 4.3265204, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 1, 1, 'move', 0, -11221.066, -1770.6667, 4.3265204, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 1, 2, 'move', 0, -11222.133, -1755.7334, 4.3265204, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 1, 3, 'move', 0, -11223.6, -1728.79, 4.25172, 0, 0, 0, 0, 0, '', 'to'),
('karazhan_crypt', 1, 4, 'pull', 0, -11223.6, -1728.79, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 0'),
('karazhan_crypt', 1, 5, 'pull', 0, -11210, -1727.89, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 1'),
('karazhan_crypt', 1, 6, 'pull', 0, -11195.7, -1701.24, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 2'),
('karazhan_crypt', 1, 7, 'pull', 0, -11205.8, -1685.06, 4.36, 5, 0, 0, 0, 0, '91924', 'pull group 3'),
('karazhan_crypt', 1, 8, 'pull', 0, -11196.05, -1671.83, 12.18, 11.3, 0, 0, 0, 0, '91926,91924', 'pull group 4'),
('karazhan_crypt', 1, 9, 'pull', 0, -11182.4, -1685.52, 4.31, 5, 0, 0, 0, 0, '91924', 'pull group 5'),
('karazhan_crypt', 1, 10, 'pull', 0, -11168.2, -1674.21, 8.11, 5, 0, 0, 0, 0, '91924', 'pull group 6'),
('karazhan_crypt', 1, 11, 'pull', 0, -11151, -1679.11, 6.23, 5, 0, 0, 0, 0, '91924', 'pull group 7'),
('karazhan_crypt', 1, 12, 'pull', 0, -11129.9, -1668.99, 7.69, 5, 0, 0, 0, 0, '91924', 'pull group 8'),
('karazhan_crypt', 1, 13, 'pull', 0, -11117.7, -1677.26, 6.8, 10, 0, 0, 0, 0, '91926', 'pull group 9'),
('karazhan_crypt', 1, 14, 'pull', 0, -11101.7, -1673.15, 3.74, 5, 0, 0, 0, 0, '91924', 'pull group 10'),
('karazhan_crypt', 1, 15, 'pull', 0, -11088.7, -1668.43, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 11'),
('karazhan_crypt', 1, 16, 'pull', 0, -11088.5, -1698.2, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 12'),
('karazhan_crypt', 1, 17, 'pull', 0, -11113, -1699.29, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 13'),
('karazhan_crypt', 1, 18, 'pull', 0, -11137.4, -1700.44, 3.59, 5, 0, 0, 0, 0, '91925', 'pull group 14'),
('karazhan_crypt', 1, 19, 'pull', 0, -11132.2, -1714.73, 4.6, 5, 0, 0, 1, 0, '91923', 'pull group 15'),
('karazhan_crypt', 1, 20, 'move', 0, -11131.199, -1709.6001, 4.1265206, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 21, 'pull', 0, -11118.8, -1649.47, 1.34, 5, 0, 0, 0, 0, '91924', 'pull group 16'),
('karazhan_crypt', 1, 22, 'pull', 0, -11104.6, -1637.46, 3.73, 5, 0, 0, 0, 0, '91924', 'pull group 17'),
('karazhan_crypt', 1, 23, 'pull', 0, -11118.3, -1630.84, 5.87, 10, 0, 0, 0, 0, '91926', 'pull group 18'),
('karazhan_crypt', 1, 24, 'pull', 0, -11126.5, -1618.54, 4.03, 5, 0, 0, 0, 0, '91924', 'pull group 19'),
('karazhan_crypt', 1, 25, 'pull', 0, -11141.1, -1633.23, 8.9, 5, 0, 0, 0, 0, '91924', 'pull group 20'),
('karazhan_crypt', 1, 26, 'pull', 0, -11154.6, -1619.3, 4.11, 5, 0, 0, 0, 0, '91924', 'pull group 21'),
('karazhan_crypt', 1, 27, 'pull', 0, -11173.9, -1632.72, 7.31, 5, 0, 0, 0, 0, '91924', 'pull group 22'),
('karazhan_crypt', 1, 28, 'pull', 0, -11187.2, -1623.88, 5.76, 5, 0, 0, 0, 0, '91924,91926', 'pull group 23'),
('karazhan_crypt', 1, 29, 'pull', 0, -11199, -1631.78, 6.11, 5, 0, 0, 0, 0, '91924', 'pull group 24'),
('karazhan_crypt', 1, 30, 'pull', 0, -11213.6, -1630.37, 9.35, 5, 0, 0, 0, 0, '91924', 'pull group 25'),
('karazhan_crypt', 1, 31, 'pull', 0, -11225.6, -1634, 10.74, 10, 0, 0, 0, 0, '91926', 'pull group 26'),
('karazhan_crypt', 1, 32, 'pull', 0, -11234.6, -1656.04, 3.08, 5, 0, 0, 0, 0, '91924', 'pull group 27'),
('karazhan_crypt', 1, 33, 'pull', 0, -11225.1, -1672.5, 6.16, 5, 0, 0, 0, 0, '91924', 'pull group 28'),
('karazhan_crypt', 1, 34, 'pull', 0, -11211.5, -1653.36, 1.53, 10, 0, 0, 0, 0, '91926', 'pull group 29'),
('karazhan_crypt', 1, 35, 'pull', 0, -11191.5, -1652.67, 0.68, 5, 0, 0, 0, 0, '91924', 'pull group 30'),
('karazhan_crypt', 1, 36, 'pull', 0, -11167.03, -1651.31, 1.29, 12.8, 0, 0, 0, 0, '91926,91924', 'pull group 31'),
('karazhan_crypt', 1, 37, 'pull', 0, -11199, -1604.67, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 32'),
('karazhan_crypt', 1, 38, 'pull', 0, -11219.7, -1575.75, -1.46, 10, 0, 0, 1, 0, '91932', 'pull group 33'),
('karazhan_crypt', 1, 39, 'pull', 0, -11241.5, -1604.71, 4.26, 5, 0, 0, 0, 0, '91925', 'pull group 34'),
('karazhan_crypt', 1, 40, 'move', 0, -11242.666, -1621.3334, 3.8265204, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 41, 'pull', 0, -11244.7, -1689.73, 3.8, 5, 0, 0, 0, 0, '91924', 'pull group 35'),
('karazhan_crypt', 1, 42, 'pull', 0, -11239.7, -1702.43, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 36'),
('karazhan_crypt', 1, 43, 'pull', 0, -11256.6, -1702.92, 4.25, 5, 0, 0, 0, 0, '91925', 'pull group 37'),
('karazhan_crypt', 1, 44, 'move', 0, -11218.306, -1670.7714, 4.5930963, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 45, 'move', 0, -11202.934, -1657.8667, 4.6265206, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 46, 'move', 0, -11163.912, -1626.6049, 4.7120914, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 47, 'move', 0, -11157.332, -1621.3334, 4.7265205, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 48, 'pull', 0, -11102.6, -1615.94, 3.16, 5, 0, 0, 0, 0, '91924', 'pull group 38'),
('karazhan_crypt', 1, 49, 'pull', 0, -11104.2, -1600.69, 4.24, 5, 0, 0, 0, 0, '91925', 'pull group 39'),
('karazhan_crypt', 1, 50, 'pull', 0, -11090.4, -1629.9, 4.23, 5, 0, 0, 0, 0, '91925', 'pull group 40'),
('karazhan_crypt', 1, 51, 'move', 0, -11130.427, -1659.8326, 5.732204, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 52, 'move', 0, -11135.999, -1664, 5.9265203, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 53, 'move', 0, -11178.666, -1685.3334, 4.6265206, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 1, 54, 'boss', 92935, -11217, -1696.79, 4.89425, 0, 0, 0, 0, 0, '', 'boss'),
('karazhan_crypt', 2, 0, 'move', 0, -11178.666, -1685.3334, 4.6265206, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 1, 'move', 0, -11135.999, -1685.3334, 3.8265204, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 2, 'move', 0, -11114.133, -1724, 6.8265204, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 3, 'move', 0, -11110.933, -1729.0667, 8.026521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 4, 'move', 0, -11097.599, -1734.1334, 12.826521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 5, 'move', 0, -11086.666, -1737.0667, 16.426521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 6, 'move', 0, -11085.065, -1737.0667, 16.72652, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 7, 'move', 0, -11074.933, -1733.6, 18.526522, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 8, 'move', 0, -11066.133, -1727.4667, 19.926521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 9, 'move', 0, -11053.332, -1717.8667, 22.426521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 10, 'move', 0, -11050.666, -1713.3334, 23.026522, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 11, 'move', 0, -11047.199, -1706.6667, 23.82652, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 12, 'move', 0, -11046.933, -1703.7334, 24.12652, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 13, 'move', 0, -11048.266, -1672.8, 28.026522, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 14, 'move', 0, -11048.532, -1670.6667, 28.026522, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 15, 'move', 0, -11091.665, -1645.3785, 28.265648, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 2, 16, 'move', 0, -11096.95, -1642.28, 28.8001, 0, 0, 0, 0, 0, '', 'to'),
('karazhan_crypt', 2, 17, 'pull', 0, -11109.9, -1625.67, 27.96, 5, 0, 0, 1, 0, '91923', 'pull group 0'),
('karazhan_crypt', 2, 18, 'pull', 0, -11094.2, -1618.62, 28.35, 5.8, 0, 0, 0, 0, '91910,91911', 'pull group 1'),
('karazhan_crypt', 2, 19, 'pull', 0, -11078.2, -1625.93, 27.98, 5, 0, 0, 1, 0, '91923', 'pull group 2'),
('karazhan_crypt', 2, 20, 'pull', 0, -11056.97, -1620.66, 28.01, 12.7, 0, 0, 0, 0, '91918,91911,91923', 'pull group 3'),
('karazhan_crypt', 2, 21, 'pull', 0, -11056.1, -1597.72, 27.98, 5, 0, 0, 0, 0, '91925', 'pull group 4'),
('karazhan_crypt', 2, 22, 'pull', 0, -11058.1, -1579.51, 27.84, 5, 0, 0, 0, 0, '91924', 'pull group 5'),
('karazhan_crypt', 2, 23, 'pull', 0, -11076.7, -1586.38, 29.35, 5, 0, 0, 1, 0, '91923', 'pull group 6'),
('karazhan_crypt', 2, 24, 'pull', 0, -11083, -1578.31, 29.58, 5, 0, 0, 0, 0, '91924', 'pull group 7'),
('karazhan_crypt', 2, 25, 'pull', 0, -11077.2, -1560.11, 27.15, 5, 0, 0, 0, 0, '91924', 'pull group 8'),
('karazhan_crypt', 2, 26, 'pull', 0, -11090, -1543.72, 27.81, 5, 0, 0, 0, 0, '91912', 'pull group 9'),
('karazhan_crypt', 2, 27, 'pull', 0, -11102.8, -1544.55, 27.65, 5, 0, 0, 0, 0, '91912', 'pull group 10'),
('karazhan_crypt', 2, 28, 'pull', 0, -11111, -1558.84, 25.63, 5, 0, 0, 0, 0, '91924', 'pull group 11'),
('karazhan_crypt', 2, 29, 'pull', 0, -11109.5, -1573.39, 27.41, 5, 0, 0, 0, 0, '91924', 'pull group 12'),
('karazhan_crypt', 2, 30, 'pull', 0, -11088.1, -1598.84, 28.19, 5, 0, 0, 0, 0, '91925', 'pull group 13'),
('karazhan_crypt', 2, 31, 'pull', 0, -11031.1, -1581.15, 28.38, 5, 0, 0, 0, 0, '91924', 'pull group 14'),
('karazhan_crypt', 2, 32, 'pull', 0, -11015.5, -1577.93, 27.95, 5, 0, 0, 0, 0, '91924', 'pull group 15'),
('karazhan_crypt', 2, 33, 'pull', 0, -11017.2, -1558.79, 28.07, 5, 0, 0, 0, 0, '91924', 'pull group 16'),
('karazhan_crypt', 2, 34, 'pull', 0, -11039.6, -1560.82, 27.31, 5, 0, 0, 0, 0, '91924', 'pull group 17'),
('karazhan_crypt', 2, 35, 'pull', 0, -11033.15, -1529.42, 33.07, 8.1, 0, 0, 0, 0, '91924,91910', 'pull group 18'),
('karazhan_crypt', 2, 36, 'pull', 0, -11049.6, -1516.14, 32.13, 5, 0, 0, 1, 0, '91923', 'pull group 19'),
('karazhan_crypt', 2, 37, 'pull', 0, -11064.7, -1514.06, 29.63, 5, 0, 0, 0, 0, '91918', 'pull group 20'),
('karazhan_crypt', 2, 38, 'pull', 0, -11064.6, -1529.04, 28.42, 5, 0, 0, 0, 0, '91910', 'pull group 21'),
('karazhan_crypt', 2, 39, 'pull', 0, -11080.8, -1518.44, 27.98, 5, 0, 0, 1, 0, '91923', 'pull group 22'),
('karazhan_crypt', 2, 40, 'pull', 0, -11096.65, -1523.47, 28.35, 6.1, 0, 0, 0, 0, '91910,91913', 'pull group 23'),
('karazhan_crypt', 2, 41, 'pull', 0, -11112.7, -1522.41, 27.58, 5, 0, 0, 1, 0, '91923', 'pull group 24'),
('karazhan_crypt', 2, 42, 'pull', 0, -11095.2, -1495.35, 28.14, 5, 0, 0, 0, 0, '91924', 'pull group 25'),
('karazhan_crypt', 2, 43, 'pull', 0, -11072.8, -1495.3, 29.09, 5, 0, 0, 0, 0, '91924', 'pull group 26'),
('karazhan_crypt', 2, 44, 'pull', 0, -11070.7, -1483.47, 31.05, 5, 0, 0, 0, 0, '91924', 'pull group 27'),
('karazhan_crypt', 2, 45, 'pull', 0, -11055.2, -1488.26, 29.12, 5, 0, 0, 0, 0, '91924', 'pull group 28'),
('karazhan_crypt', 2, 46, 'pull', 0, -11023.3, -1510.55, 30.07, 6.4, 0, 0, 1, 0, '91923', 'pull group 29'),
('karazhan_crypt', 2, 47, 'move', 0, -11022.666, -1504.5333, 28.72652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 48, 'move', 0, -11022.933, -1502.6667, 28.426521, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 49, 'move', 0, -11068.635, -1482.4664, 30.234911, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 50, 'pull', 0, -11093.4, -1471.52, 31.13, 5, 0, 0, 0, 0, '91924', 'pull group 30'),
('karazhan_crypt', 2, 51, 'move', 0, -11054.839, -1503.3245, 32.421127, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 52, 'move', 0, -11054.666, -1503.4667, 32.42652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 53, 'move', 0, -11050.666, -1514.6667, 32.326523, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 54, 'move', 0, -11043.732, -1541.0667, 28.526522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 55, 'pull', 0, -11024.4, -1595.94, 28.04, 5, 0, 0, 0, 0, '91925', 'pull group 31'),
('karazhan_crypt', 2, 56, 'pull', 0, -11030.65, -1617.14, 28.35, 5.9, 0, 0, 0, 0, '91911,91913', 'pull group 32'),
('karazhan_crypt', 2, 57, 'pull', 0, -11014.3, -1623.52, 27.97, 10, 0, 0, 1, 0, '91923', 'pull group 33'),
('karazhan_crypt', 2, 58, 'pull', 0, -11029, -1669.78, 27.76, 5, 0, 0, 0, 0, '91925', 'pull group 34'),
('karazhan_crypt', 2, 59, 'pull', 0, -11040.7, -1680.31, 27.48, 5, 0, 0, 0, 0, '91924', 'pull group 35'),
('karazhan_crypt', 2, 60, 'pull', 0, -11050.8, -1687.98, 27.25, 5, 0, 0, 0, 0, '91924', 'pull group 36'),
('karazhan_crypt', 2, 61, 'pull', 0, -11061.3, -1670.56, 27.35, 5, 0, 0, 0, 0, '91925', 'pull group 37'),
('karazhan_crypt', 2, 62, 'pull', 0, -11090.6, -1677.26, 28.39, 5, 0, 0, 0, 0, '91924', 'pull group 38'),
('karazhan_crypt', 2, 63, 'pull', 0, -11103.5, -1670.63, 28.21, 5, 0, 0, 0, 0, '91925', 'pull group 39'),
('karazhan_crypt', 2, 64, 'pull', 0, -11103.1, -1694.39, 31.32, 5, 0, 0, 0, 0, '91924', 'pull group 40'),
('karazhan_crypt', 2, 65, 'pull', 0, -11088.8, -1708.03, 34.93, 5, 0, 0, 0, 0, '91924', 'pull group 41'),
('karazhan_crypt', 2, 66, 'move', 0, -11091.999, -1711.2001, 36.126522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 67, 'move', 0, -11093.065, -1716.8, 37.326523, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 68, 'move', 0, -11092.266, -1718.4, 37.326523, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 69, 'move', 0, -11050.666, -1736, 33.126522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 70, 'move', 0, -11043.732, -1738.9333, 31.62652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 71, 'move', 0, -11037.599, -1738.9333, 31.026522, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 72, 'pull', 0, -11013.6, -1721.67, 28.02, 5, 0, 0, 0, 0, '91924', 'pull group 42'),
('karazhan_crypt', 2, 73, 'move', 0, -11037.599, -1697.0667, 24.926521, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 74, 'move', 0, -11042.933, -1690.4, 26.62652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 75, 'move', 0, -11047.193, -1640.5887, 27.453033, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 76, 'move', 0, -11050.666, -1600, 28.12652, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 2, 77, 'boss', 91919, -11064.3, -1541.61, 27.9231, 0, 0, 0, 0, 0, '', 'boss'),
('karazhan_crypt', 3, 0, 'move', 0, -11053.593, -1590.4497, 28.196922, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 1, 'move', 0, -11051.732, -1598.9333, 28.22652, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 2, 'move', 0, -11049.389, -1648.8782, 28.091291, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 3, 'move', 0, -11048.266, -1672.8, 28.026522, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 4, 'move', 0, -11046.933, -1703.7334, 24.12652, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 5, 'move', 0, -11047.199, -1706.6667, 23.82652, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 6, 'move', 0, -11050.666, -1713.3334, 23.026522, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 7, 'move', 0, -11053.332, -1717.8667, 22.426521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 8, 'move', 0, -11066.133, -1727.4667, 19.926521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 9, 'move', 0, -11074.933, -1733.6, 18.526522, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 10, 'move', 0, -11085.065, -1737.0667, 16.72652, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 11, 'move', 0, -11086.666, -1737.0667, 16.426521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 12, 'move', 0, -11097.599, -1734.1334, 12.826521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 13, 'move', 0, -11105.065, -1729.8667, 9.026521, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 14, 'move', 0, -11114.133, -1724, 6.8265204, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 15, 'move', 0, -11146.206, -1685.6536, 5.8921566, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 16, 'move', 0, -11178.279, -1647.3071, 4.9577928, 0, 0, 0, 0, 0, '', 'path'),
('karazhan_crypt', 3, 17, 'move', 0, -11200.09, -1621.23, 4.82, 0, 0, 0, 0, 0, '', 'via'),
('karazhan_crypt', 3, 18, 'move', 0, -11219.96, -1586.82, 4.13, 0, 0, 0, 0, 0, '', 'to'),
('karazhan_crypt', 3, 19, 'move', 0, -11221.3, -1560, -45.9, 0, 0, 0, 0, 0, '', 'pull group 0 approach'),
('karazhan_crypt', 3, 20, 'pull', 0, -11207.9, -1553.22, -44.73, 8, 0, 0, 1, 0, '91932', 'pull group 0'),
('karazhan_crypt', 3, 21, 'pull', 0, -11237.8, -1554.05, -41.22, 8, 0, 0, 1, 0, '91932', 'pull group 1'),
('karazhan_crypt', 3, 22, 'move', 0, -11221.333, -1578.6666, -45.87348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 3, 23, 'pull', 0, -11210.3, -1604.6, -42.72, 8, 0, 0, 1, 0, '91932', 'pull group 2'),
('karazhan_crypt', 3, 24, 'pull', 0, -11233, -1606.03, -42.8, 8, 0, 0, 1, 0, '91932', 'pull group 3'),
('karazhan_crypt', 3, 25, 'move', 0, -11213.8, -1632, -34.3, 0, 0, 0, 0, 0, '', 'pull group 4 approach'),
('karazhan_crypt', 3, 26, 'pull', 0, -11219.43, -1638.69, -31.34, 11.1, 0, 0, 0, 0, '91925,91918', 'pull group 4'),
('karazhan_crypt', 4, 0, 'move', 0, -11211, -1689, -28.5, 0, 0, 0, 0, 0, '', 'via'),
('karazhan_crypt', 4, 1, 'move', 0, -11202.032, -1738.1836, -29.204737, 0, 0, 0, 0, 0, '', 'via'),
('karazhan_crypt', 4, 2, 'move', 0, -11194.8, -1752.3, -28.3, 0, 0, 0, 0, 0, '', 'via'),
('karazhan_crypt', 4, 3, 'move', 0, -11199.199, -1770.6667, -29.27348, 0, 0, 0, 0, 0, '', 'to'),
('karazhan_crypt', 4, 4, 'pull', 0, -11194.2, -1796.43, -28.46, 5, 0, 0, 0, 0, '91915', 'pull group 0'),
('karazhan_crypt', 4, 5, 'pull', 0, -11215.1, -1778.02, -29.92, 5, 0, 0, 1, 0, '91923', 'pull group 1'),
('karazhan_crypt', 4, 6, 'pull', 0, -11238.1, -1797.69, -28.46, 5, 0, 0, 0, 0, '91915', 'pull group 2'),
('karazhan_crypt', 4, 7, 'pull', 0, -11249, -1769.26, -28.47, 5, 0, 0, 0, 0, '91912', 'pull group 3'),
('karazhan_crypt', 4, 8, 'pull', 0, -11250.3, -1738.71, -28.47, 5, 0, 0, 0, 0, '91915', 'pull group 4'),
('karazhan_crypt', 4, 9, 'pull', 0, -11250.4, -1708.87, -28.46, 5, 0, 0, 0, 0, '91912', 'pull group 5'),
('karazhan_crypt', 4, 10, 'pull', 0, -11230.5, -1686.84, -28.51, 5, 0, 0, 0, 0, '91912', 'pull group 6'),
('karazhan_crypt', 4, 11, 'pull', 0, -11204.4, -1685.89, -28.51, 5, 0, 0, 0, 0, '91912', 'pull group 7'),
('karazhan_crypt', 4, 12, 'pull', 0, -11183.4, -1706.76, -28.49, 5, 0, 0, 0, 0, '91912', 'pull group 8'),
('karazhan_crypt', 4, 13, 'pull', 0, -11159.5, -1696.19, -28.49, 5, 0, 0, 1, 0, '91923', 'pull group 9'),
('karazhan_crypt', 4, 14, 'pull', 0, -11143.1, -1695.08, -28.5, 5, 0, 0, 1, 0, '91923', 'pull group 10'),
('karazhan_crypt', 4, 15, 'pull', 0, -11129.1, -1720, -29.74, 5, 0, 0, 1, 0, '91923', 'pull group 11'),
('karazhan_crypt', 4, 16, 'pull', 0, -11107.3, -1742.45, -28.52, 5, 0, 0, 1, 0, '91923', 'pull group 12'),
('karazhan_crypt', 4, 17, 'pull', 0, -11120.4, -1754.29, -28.51, 5, 0, 0, 0, 0, '91924', 'pull group 13'),
('karazhan_crypt', 4, 18, 'pull', 0, -11115.85, -1771.05, -28.51, 5, 0, 0, 0, 0, '91924', 'pull group 14'),
('karazhan_crypt', 4, 19, 'pull', 0, -11137.5, -1763.75, -29.74, 5, 0, 0, 0, 0, '91924', 'pull group 15'),
('karazhan_crypt', 4, 20, 'pull', 0, -11160.3, -1753.96, -28.5, 5, 0, 0, 1, 0, '91923', 'pull group 16'),
('karazhan_crypt', 4, 21, 'pull', 0, -11181.3, -1767.49, -28.51, 5, 0, 0, 0, 0, '91912', 'pull group 17'),
('karazhan_crypt', 4, 22, 'pull', 0, -11181.7, -1737.11, -28.52, 5, 0, 0, 0, 0, '91915', 'pull group 18'),
('karazhan_crypt', 4, 23, 'pull', 0, -11161.5, -1727.53, -28.5, 5, 0, 0, 1, 0, '91923', 'pull group 19'),
('karazhan_crypt', 4, 24, 'move', 0, -11164.532, -1692.5333, -28.47348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 25, 'move', 0, -11177.865, -1670.1334, -28.47348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 26, 'move', 0, -11178.666, -1669.8667, -28.47348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 27, 'move', 0, -11194.933, -1669.3334, -28.47348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 28, 'move', 0, -11199.199, -1669.3334, -28.47348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 29, 'pull', 0, -11217, -1718.23, -29.88, 5, 0, 0, 1, 0, '91923', 'pull group 20'),
('karazhan_crypt', 4, 30, 'move', 0, -11221.333, -1707.4667, -29.27348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 31, 'pull', 0, -11241.9, -1655.8, -28.47, 5, 0, 0, 1, 0, '91923', 'pull group 21'),
('karazhan_crypt', 4, 32, 'move', 0, -11191.901, -1655.5763, -28.435532, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 33, 'pull', 0, -11130.2, -1655.3, -28.49, 5, 0, 0, 0, 0, '91925', 'pull group 22'),
('karazhan_crypt', 4, 34, 'move', 0, -11131.999, -1666.4, -28.47348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 35, 'move', 0, -11138.399, -1691.7334, -28.47348, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 36, 'move', 0, -11138.399, -1741.7299, -29.067106, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 4, 37, 'pull', 0, -11137.75, -1803.52, -28.73, 5.2, 0, 0, 0, 0, '91924', 'pull group 23'),
('karazhan_crypt', 4, 38, 'pull', 0, -11166.53, -1803.5, -28.5, 8.7, 0, 0, 0, 0, '91924,91923', 'pull group 24'),
('karazhan_crypt', 4, 39, 'boss', 91920, -11214.6, -1800.15, -28.3992, 0, 0, 0, 0, 0, '', 'boss'),
('karazhan_crypt', 5, 0, 'move', 0, -11249, -1769.26, -28.4651, 0, 0, 0, 0, 0, '', 'to'),
('karazhan_crypt', 5, 1, 'pull', 0, -11242.65, -1758.72, -15.7, 7.5, 0, 0, 0, 0, '91930', 'pull group 0'),
('karazhan_crypt', 5, 2, 'pull', 0, -11229.1, -1746.86, -15.39, 5, 0, 0, 0, 0, '91930', 'pull group 1'),
('karazhan_crypt', 5, 3, 'pull', 0, -11206.6, -1743.9, -15.57, 12.7, 0, 0, 0, 0, '91930', 'pull group 2'),
('karazhan_crypt', 5, 4, 'pull', 0, -11173.3, -1741.94, -15.44, 5, 0, 0, 1, 0, '91923', 'pull group 3'),
('karazhan_crypt', 5, 5, 'move', 0, -11222.385, -1751.4604, -15.287915, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 5, 6, 'move', 0, -11230.666, -1753.0667, -15.273479, 0, 0, 0, 0, 0, '', 'approach path'),
('karazhan_crypt', 5, 7, 'boss', 91929, -11255, -1744.53, -15.6889, 0, 0, 0, 0, 0, '', 'boss');
