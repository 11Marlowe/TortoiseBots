-- Karazhan Crypt (map 800): the Hivaxxis leg reordered and given approach stands, exported from turtle-agent
-- prep/karazhan_crypt 0f9f0d2 (dungeons/karazhan_crypt.json) with feat/offmesh-links 2756955. This file replaces the rows of
-- 20261025001400: the plan row, its six legs and all 265 steps are deleted and inserted again, and it carries NO
-- ai_dungeon_boss row - the six bosses of map 800 were seeded in 20261024021100 and are unchanged.
--   turtle-agent plan-export --dir dungeons --mmaps <map 800 mmaps with the rebuilt 8005334> --allow-draft --approach-hops \
--     --rev 0f9f0d2 --stamp 20261025001500 karazhan_crypt
--
-- WHY THE LEG. r40 slot5b (RUN-LESSONS "r40 karazhan_crypt slot5b") ran the reordered plan to the end: 0 deaths, 0 wipes,
-- 70 min, Archlich Enkhraz, Guard Captain Gort, Commander Andreon and Marrowspike down, the dive working for all five. The
-- Hivaxxis leg then stopped, and Hivaxxis was never fought:
--   step 256  pull group 3 (the Unseen Stalker 91923 at -11173.3 -1741.94 -15.44): "pack deferred: not reachable from this
--             floor" (8 approach hops, nearest 72.6 yd, which is the straight distance from where the lead stood)
--   step 257  move -11222.385 -1751.4604 -15.287915: "no dry stand-in within 25 yd", skipped
--   step 258  move -11230.666 -1753.0667 -15.273479: "refused: unreachable" (nothing within 25 yd)
-- The lead sat at -11237.9 -1746.7 -13.5, 17 yd from Hivaxxis (-11255 -1744.53 -15.69).
--
-- TWO CAUSES, BOTH IN THE ROWS THE EXPORTER CUT.
--   1. The two approach moves are the offline cutter's 50 yd cut of the hop from the stalker to the boss (cutWith over
--      navmesh.Planner's straight path; its corner is the -11230.666 -1753.0667 point itself). Their z is -15.27/-15.29,
--      0.4-0.5 yd UNDER the corridor floor at those x/y (-14.78..-15.17 on the server's own polyline), because the offline
--      path sags between the polyline corners the server's path follows. The core answers a path to them 0x8 (no path) from
--      the west; only from the east, where r40 slot5 (old order) happened to stand, did they answer 0x1. That is why the
--      same two rows worked in the earlier run and failed here.
--   2. The leg pulled its room's packs in the wrong order. Group 0 (the Tomb Creepers 91930 pair at -11244.2/-11241.1) came
--      first: the walk that the pull starts (295 yd, around the lower crypt and up the ramp) brings the party in at the
--      corridor's EAST end, and the group then drags it WEST into the pair. The stand the run leader then searched for its
--      next pull (group 1, no approach point) was the off-mesh pocket at -11245.7 -1747.3 -15.0: pathdump answers 0x4 to the
--      boss (9.3 yd short) and the only walkable neighbour is the z -13.5 ledge at -11245.7 -1749.2. From there the stalker
--      (72.6 yd, no progress) and every corridor point (0x8) were out of reach, and the run's own stand search then put the
--      party on the ledge (-11237.9 -1746.7 -13.5).
--
-- THE FIX (leg 5 only; the other five legs and every other row are the 20261025001400 rows, unchanged):
--   * the four pull groups are in the order the party physically walks them, east to west: the Unseen Stalker 91923 behind
--     the Tomb of the Unrepentant door first, then the four Tomb Creepers 91930 at -11206.6 -1743.9, the creeper at -11229.1,
--     the pair at -11244.2/-11241.1, and Hivaxxis (91929) last. Same groups, entries, positions, radii, flags and patrol
--     blocks as before; only the order is new.
--   * EVERY pull group and the boss has an `approach`: the stand is a point of the server's own corridor path
--     (-14.78..-15.08), 10.1-17.0 yd from its pack, so the run leader never searches a stand of its own (the pocket above was
--     one of those searches). The stalker's approach is the 226 yd walk itself in four hops (43.6, 42.7, 44.0, 46.6 yd, the
--     server's own polyline points), each under the cutter's 60 yd, so the exporter adds no points of its own in this leg.
--   * the boss's stand is -11243.99 -1751.02 -15.17, 12.9 yd of path from Hivaxxis (0x1).
--
-- THE ORDER AND COUNTS: 6 legs, 265 steps (141 pulls + 119 moves + 5 boss steps), 272 rows (1 plan + 6 legs + 265 steps).
-- Leg 5 gains six rows (four approach hops, two stands) and is steps 252-265:
--   252 move -11249 -1769.26 -28.4651 (the leg's `to`, unchanged)
--   253-256 the four approach hops of the stalker
--   257 pull 91923 (-11173.3 -1741.94 -15.44)
--   258 move -11196.86 -1746.51 -15.08, 259 pull 91930 x4 (-11206.6 -1743.9 -15.57)
--   260 move -11212.57 -1749.56 -14.91, 261 pull 91930 (-11229.1 -1746.86 -15.39)
--   262 move -11230.24 -1752.98 -14.78, 263 pull 91930 x2 (-11242.65 -1758.72 -15.7)
--   264 move -11243.99 -1751.02 -15.17, 265 boss 91929 (-11255 -1744.53 -15.6889)
--
-- CHECKS (server-exact, the rebuilt tile 8005334 in ~/turtle-agent-mmaps).
--   * pathdump, every one of the leg's 14 rows from the row before it, and from the party's real starts - the plan's leg
--     entry, r40 slot5b's real leg-5 start (-11166.1 -1802.5 -28.5) and r40 slot5's (-11209 -1748.9 -31.4): all type 0x1,
--     ending within 1 yd; the boss 0x1 from its stand. The two old cut points answer 0x8 from the west (-11237.9 -1746.7
--     -13.5 and -11245.7 -1747.3 -15.0) and 0x1 only from the east: the failure reproduced, then the fix checked.
--   * plan-check (feat/offmesh-links 2756955, the rebuilt tile): ok, 193 points, 0 failing; 111 warnings (106 estimated
--     patrol periods, 4 detours - the leg's two are gone - and 1 off-mesh link, the dive's) and 100 run-leader notices (the
--     leg's two, "51 yd"/"82 yd from the last move row", are gone).
--   * plan-export: 6 legs, 265 steps (141 pulls, 119 moves, 5 bosses), 272 rows; reads back as the rows it wrote.
--   * tests/test_dungeon_plan_sql.py and test_boss_tactics_sql.py: see the commit.
--
-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- karazhan_crypt: map 800, 6 legs, 265 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'karazhan_crypt';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'karazhan_crypt';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'karazhan_crypt';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('karazhan_crypt', 800, '', '', '91918,91913,91911,91925', 'turtle-agent 0f9f0d2 dungeons/karazhan_crypt.json', 'plan v1, draft (reordered 2026-10-25: the upper floors first, the shaft dive last, the lower crypt at the end; Corpsemuncher out: its wing has no dry contact; not live-checked; hivaxxis leg reordered with a stand for every pack after r40 slot5b)');
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
('karazhan_crypt', 5, 1, 'move', 0, -11215.79, -1743.7, -29.24, 0, 0, 0, 0, 0, '', 'pull group 0 approach'),
('karazhan_crypt', 5, 2, 'move', 0, -11199.96, -1704.06, -28.24, 0, 0, 0, 0, 0, '', 'pull group 0 approach'),
('karazhan_crypt', 5, 3, 'move', 0, -11191.09, -1668.78, -27.97, 0, 0, 0, 0, 0, '', 'pull group 0 approach'),
('karazhan_crypt', 5, 4, 'move', 0, -11164.57, -1698.66, -27.97, 0, 0, 0, 0, 0, '', 'pull group 0 approach'),
('karazhan_crypt', 5, 5, 'pull', 0, -11173.3, -1741.94, -15.44, 5, 0, 0, 1, 0, '91923', 'pull group 0'),
('karazhan_crypt', 5, 6, 'move', 0, -11196.86, -1746.51, -15.08, 0, 0, 0, 0, 0, '', 'pull group 1 approach'),
('karazhan_crypt', 5, 7, 'pull', 0, -11206.6, -1743.9, -15.57, 12.7, 0, 0, 0, 0, '91930', 'pull group 1'),
('karazhan_crypt', 5, 8, 'move', 0, -11212.57, -1749.56, -14.91, 0, 0, 0, 0, 0, '', 'pull group 2 approach'),
('karazhan_crypt', 5, 9, 'pull', 0, -11229.1, -1746.86, -15.39, 5, 0, 0, 0, 0, '91930', 'pull group 2'),
('karazhan_crypt', 5, 10, 'move', 0, -11230.24, -1752.98, -14.78, 0, 0, 0, 0, 0, '', 'pull group 3 approach'),
('karazhan_crypt', 5, 11, 'pull', 0, -11242.65, -1758.72, -15.7, 7.5, 0, 0, 0, 0, '91930', 'pull group 3'),
('karazhan_crypt', 5, 12, 'move', 0, -11243.99, -1751.02, -15.17, 0, 0, 0, 0, 0, '', 'boss approach'),
('karazhan_crypt', 5, 13, 'boss', 91929, -11255, -1744.53, -15.6889, 0, 0, 0, 0, 0, '', 'boss');
