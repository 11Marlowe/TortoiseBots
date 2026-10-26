-- Stormwind Stockade (map 34): the plan for the run leader (.bot run), exported from turtle-agent feat/stockade-plan-fixes 1bb93ed
-- (dungeons/stormwind_stockade.json, status 'draft (not mapped, not live-checked)') with the options of the header of 20261017130000:
--   turtle-agent plan-export --mmaps <map 34 mmaps> --allow-draft --gm-cross-direct --approach-hops --rev 1bb93ed \
--     --stamp 20261022210100 --out <dir> stormwind_stockade
-- Stacks on 20261022210000. This file replaces the first export of the same name (the draft as the world export gave it), which was never merged or
-- applied anywhere. NO ROW IS HAND-SET. The fixes it carries over that draft are in ~/TortoiseBots-audit/PREP-STOCKADE-2026-10-22.md ("Fixes applied"):
--   * the plan row's lists: cc_priority 1711,1715,1706 (the Convict and the Insurgent stun, the Prisoner kicks), cc_always 1711, kill_first 1717
--     (Hamhock, the only caster of the map);
--   * leg 0 pull group 10 includes the rare Bruegal Ironknuckle 1720 (5.7 yd from a member of the pack; he was a skipped unit the pull dragged in), the
--     pack east of him is two pairs instead of one of four, the two single packs at Targorr (leg 0 old groups 13 and 14) are the boss row's entries;
--   * Dextren's guards, Kam's Insurgent and two Inmates, and Bazil's Insurgent and Convict are the boss rows' entries (no pull of their own); Hamhock's
--     guards come with the boss (a creature_group), so his leg has no pull for them and its two other packs are pulled before him; Dextren's leg pulls
--     the west pack, the east pack and then the three Inmates in front of him; Kam's leg pulls the Convict and the block of four before him; leg 4
--     begins on Hamhock's spot, 35 yd from Bazil, not on the Insurgent's own spawn;
--   * pull radii cut to the group's own members where a neighbour, Bruegal or a boss stood in them (legs 0, 1 and 2).
-- 5 legs, 95 steps, 101 rows (14 + 8 + 9 + 3 + 0 pulls; bosses 1696 Targorr the Dread, 1663 Dextren Ward, 1666 Kam Deepfury, 1717 Hamhock, 1716 Bazil
-- Thredd, in leg order). Plan check (turtle-agent plan-check, map 34 mmaps): ok, 87 points on the navmesh, 2 detour warnings (leg 0 pull 9 and leg 2 pull 4:
-- the pack is 29-39 yd away but the way to it is 113-124 yd) and 15 run-leader-hop warnings (a pull or boss 50-100 yd from the move row before it; the run
-- leader walks there in navmesh hops of its own).
--
-- WHAT THE RUN LEADER CANNOT DO (issue #984: nothing is dropped silently). Not in these rows:
--   * skip (creature entries a leg leaves alone): legs 1-4 [1720] (Bruegal Ironknuckle, now fought in leg 0; the entry is no longer skipped there). There is no
--     skip column: the run leader pulls a hostile within 15 yd of a hop point that no later step owns, a skipped unit too.
--   * fleers 1663,1666,1706,1707,1708,1711,1715,1716 (every one of them flees at 15 % health) and the boss tactic of Hamhock (interrupt Chain Lightning):
--     the plan has them, the rows have no place for them (an ai_boss_tactic row (34, 1717, 'interrupt', 0, 421) would be a later migration).
--   * doors, levers and keys: the draft has none (the map has no instance script, and the world export of map 34 lists only chests and one crate as
--     objects). The first live run is the mapping walk the plan notes ask for.
--   * the entrance (trigger 101, the portal at -8761.85 848.56 87.81 on map 0), level_band 23-28 and hire_composition: the run leader starts inside the
--     instance and leads the bound party as it is.
--
-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- stormwind_stockade: map 34, 5 legs, 95 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'stormwind_stockade';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'stormwind_stockade';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'stormwind_stockade';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('stormwind_stockade', 34, '1711,1715,1706', '1711', '1717', 'turtle-agent 1bb93ed dungeons/stormwind_stockade.json', 'plan v1, draft (not mapped, not live-checked)');
INSERT INTO `ai_dungeon_leg` (`plan`, `leg`, `name`, `comment`) VALUES
('stormwind_stockade', 0, 'targorr_the_dread', ''),
('stormwind_stockade', 1, 'dextren_ward', ''),
('stormwind_stockade', 2, 'kam_deepfury', ''),
('stormwind_stockade', 3, 'hamhock', ''),
('stormwind_stockade', 4, 'bazil_thredd', '');
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('stormwind_stockade', 0, 0, 'move', 0, 76.2288, 0.302129, -25.5229, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_stockade', 0, 1, 'pull', 0, 82.16, -6.95, -25.98, 13, 0, 0, 0, 0, '1708,1706,1707', 'pull group 0'),
('stormwind_stockade', 0, 2, 'pull', 0, 93.47, 3.03, -25.52, 10, 0, 0, 0, 0, '1706', 'pull group 1'),
('stormwind_stockade', 0, 3, 'pull', 0, 110.66, -0.8, -25.52, 10, 0, 0, 0, 0, '1707', 'pull group 2'),
('stormwind_stockade', 0, 4, 'pull', 0, 110.51, -14.29, -26.44, 10, 0, 0, 0, 0, '1706', 'pull group 3'),
('stormwind_stockade', 0, 5, 'pull', 0, 132.88, 1.67, -25.52, 17.6, 0, 0, 0, 0, '1706,1707', 'pull group 4'),
('stormwind_stockade', 0, 6, 'pull', 0, 129.41, 21.69, -29.22, 10, 0, 0, 0, 0, '1711', 'pull group 5'),
('stormwind_stockade', 0, 7, 'pull', 0, 107.46, 21.53, -26.47, 19.6, 0, 0, 0, 0, '1707', 'pull group 6'),
('stormwind_stockade', 0, 8, 'pull', 0, 83.82, 22.81, -26.44, 17.8, 0, 0, 0, 0, '1707,1706', 'pull group 7'),
('stormwind_stockade', 0, 9, 'pull', 0, 106.75, 46.8, -34.77, 14, 0, 0, 0, 0, '1708,1707', 'pull group 8'),
('stormwind_stockade', 0, 10, 'pull', 0, 129.89, 44.98, -33.86, 14.3, 0, 0, 0, 0, '1708,1707', 'pull group 9'),
('stormwind_stockade', 0, 11, 'pull', 0, 154.12, 42.04, -34.79, 11, 0, 0, 0, 0, '1708,1706,1720', 'pull group 10'),
('stormwind_stockade', 0, 12, 'pull', 0, 161.93, 56.6, -34.86, 9, 0, 0, 0, 0, '1708,1706', 'pull group 11'),
('stormwind_stockade', 0, 13, 'pull', 0, 150, 62.59, -34.77, 9, 0, 0, 0, 0, '1708,1707', 'pull group 12'),
('stormwind_stockade', 0, 14, 'pull', 0, 105.4, 70.1, -34.77, 10, 0, 0, 0, 0, '1708', 'pull group 13'),
('stormwind_stockade', 0, 15, 'move', 0, 109.59997, 72.79998, -34.851204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 0, 16, 'move', 0, 110.66663, 72.79998, -34.851204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 0, 17, 'move', 0, 127.99996, 67.99998, -34.851204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 0, 18, 'move', 0, 130.1333, 67.19998, -34.451202, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 0, 19, 'move', 0, 132.07635, 17.760067, -27.24571, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 0, 20, 'move', 0, 132.5333, 6.133332, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 0, 21, 'move', 0, 132.79996, 4.7999988, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 0, 22, 'move', 0, 134.39996, 4.2666655, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 0, 23, 'boss', 1696, 159.582, 1.25311, -25.6062, 0, 0, 0, 0, 0, '1706,1707', 'boss'),
('stormwind_stockade', 1, 0, 'move', 0, 145.59996, 1.333333, -25.551203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 1, 1, 'move', 0, 134.39996, 4.2666655, -25.551203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 1, 2, 'move', 0, 132.79996, 4.7999988, -25.551203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 1, 3, 'move', 0, 132.5333, 6.133332, -25.551203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 1, 4, 'move', 0, 132.5333, 7.199998, -25.551203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 1, 5, 'move', 0, 133.06664, 39.19999, -33.851204, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 1, 6, 'move', 0, 133.3333, 48.53332, -33.851204, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 1, 7, 'move', 0, 134.054, 70.9823, -33.8562, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_stockade', 1, 8, 'pull', 0, 134.05, 70.98, -33.86, 10, 0, 0, 0, 0, '1707', 'pull group 0'),
('stormwind_stockade', 1, 9, 'pull', 0, 121.62, 74.72, -34.77, 10, 0, 0, 0, 0, '1708', 'pull group 1'),
('stormwind_stockade', 1, 10, 'pull', 0, 108.45, 82.4, -34.86, 10, 0, 0, 0, 0, '1708', 'pull group 2'),
('stormwind_stockade', 1, 11, 'pull', 0, 141.99, 84.11, -33.94, 10, 0, 0, 0, 0, '1708', 'pull group 3'),
('stormwind_stockade', 1, 12, 'pull', 0, 151.99, 106.04, -35.19, 17.4, 0, 0, 0, 0, '1707,1708', 'pull group 4'),
('stormwind_stockade', 1, 13, 'pull', 0, 124.56, 123.57, -33.88, 17.1, 0, 0, 0, 0, '1708', 'pull group 5'),
('stormwind_stockade', 1, 14, 'pull', 0, 184.42, 90.84, -33.86, 23.6, 0, 0, 0, 0, '1708,1706,1707', 'pull group 6'),
('stormwind_stockade', 1, 15, 'move', 0, 167.19995, 100.79997, -33.851204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 1, 16, 'move', 0, 165.86662, 103.73331, -34.451202, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 1, 17, 'move', 0, 161.33328, 117.3333, -35.151203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 1, 18, 'move', 0, 161.06662, 120.799965, -33.851204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 1, 19, 'pull', 0, 172.04, 144.21, -33.91, 7, 0, 0, 0, 0, '1708', 'pull group 7'),
('stormwind_stockade', 1, 20, 'boss', 1663, 166.789, 134.787, -33.9396, 0, 0, 0, 0, 0, '1706,1708', 'boss'),
('stormwind_stockade', 2, 0, 'move', 0, 145.59996, 92.26664, -33.851204, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 1, 'move', 0, 145.06662, 90.66664, -33.851204, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 2, 'move', 0, 138.39996, 69.33331, -33.851204, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 3, 'move', 0, 136.26663, 60.799984, -33.851204, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 4, 'move', 0, 133.3333, 48.53332, -33.851204, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 5, 'move', 0, 127.99996, 21.333328, -29.051203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 6, 'move', 0, 126.66663, 6.399998, -25.551203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 7, 'move', 0, 124.799965, 4.2666655, -25.551203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 8, 'move', 0, 107.99997, -3.4667964, -26.151203, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 9, 'move', 0, 107.7333, -5.066796, -26.451202, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 10, 'move', 0, 107.99997, -6.6667953, -26.451202, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 11, 'move', 0, 112.26663, -23.466791, -26.451202, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_stockade', 2, 12, 'move', 0, 110.986, -29.1003, -26.4396, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_stockade', 2, 13, 'pull', 0, 106.84, -28.99, -26.44, 10, 0, 0, 0, 0, '1706,1707', 'pull group 0'),
('stormwind_stockade', 2, 14, 'pull', 0, 99.39, -37.66, -34.77, 7, 0, 0, 0, 0, '1715', 'pull group 1'),
('stormwind_stockade', 2, 15, 'pull', 0, 114.71, -43.58, -34.77, 13.8, 0, 0, 0, 0, '1711,1708', 'pull group 2'),
('stormwind_stockade', 2, 16, 'pull', 0, 98.46, -56.92, -34.77, 12, 0, 0, 0, 0, '1711,1708', 'pull group 3'),
('stormwind_stockade', 2, 17, 'pull', 0, 110.8, -66, -34.77, 10, 0, 0, 0, 0, '1715', 'pull group 4'),
('stormwind_stockade', 2, 18, 'pull', 0, 124.04, -66.3, -33.9, 11, 0, 0, 0, 0, '1708,1711', 'pull group 5'),
('stormwind_stockade', 2, 19, 'pull', 0, 155.15, -52.04, -34.86, 10, 0, 0, 0, 0, '1711', 'pull group 6'),
('stormwind_stockade', 2, 20, 'pull', 0, 135.89, -43.64, -34.36, 17, 0, 0, 0, 0, '1715,1711', 'pull group 7'),
('stormwind_stockade', 2, 21, 'move', 0, 135.19997, -43.73345, -34.851204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 22, 'move', 0, 134.1333, -43.73345, -34.851204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 23, 'move', 0, 132.5333, -43.466785, -34.451202, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 24, 'move', 0, 132.26663, -42.666786, -33.851204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 25, 'move', 0, 127.99996, -21.333458, -29.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 26, 'move', 0, 125.59997, -3.7334628, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 27, 'move', 0, 124.799965, -2.6667967, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 28, 'move', 0, 106.66663, 0, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 29, 'move', 0, 90.66664, -2.6667967, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 30, 'move', 0, 85.33331, -3.7334628, -26.251204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 31, 'pull', 0, 87.74, -28.97, -26.44, 10, 0, 0, 0, 0, '1707', 'pull group 8'),
('stormwind_stockade', 2, 32, 'move', 0, 90.13331, -24.26679, -26.051203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 33, 'move', 0, 90.13331, -23.466791, -26.451202, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 34, 'move', 0, 89.06664, -19.733458, -26.451202, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 35, 'move', 0, 85.33331, -6.400129, -26.451202, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 36, 'move', 0, 85.33331, -3.7334628, -26.251204, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 37, 'move', 0, 90.66664, -2.6667967, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 38, 'move', 0, 106.66663, 0, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 39, 'move', 0, 124.799965, -2.6667967, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 40, 'move', 0, 125.59997, -3.7334628, -25.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 41, 'move', 0, 127.99996, -21.333458, -29.551203, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_stockade', 2, 42, 'boss', 1666, 142.644, -71.8536, -34.8562, 0, 0, 0, 0, 0, '1715,1708', 'boss'),
('stormwind_stockade', 3, 0, 'move', 0, 114.613, -86.3965, -33.9396, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_stockade', 3, 1, 'pull', 0, 114.61, -86.4, -33.94, 10, 0, 0, 0, 0, '1708', 'pull group 0'),
('stormwind_stockade', 3, 2, 'pull', 0, 74.5, -89.46, -33.86, 19.8, 0, 0, 0, 0, '1708,1715,1711', 'pull group 1'),
('stormwind_stockade', 3, 3, 'pull', 0, 134.91, -122.39, -33.92, 20.3, 0, 0, 0, 0, '1711,1715,1708', 'pull group 2'),
('stormwind_stockade', 3, 4, 'boss', 1717, 105.523, -105.795, -35.1062, 0, 0, 0, 0, 0, '', 'boss'),
('stormwind_stockade', 4, 0, 'move', 0, 105.523, -105.795, -35.1062, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_stockade', 4, 1, 'boss', 1716, 89.5518, -136.922, -33.9396, 0, 0, 0, 0, 0, '1715,1711', 'boss');

-- Dungeon bosses of map 34, for the commander telemetry's boss rank (see 20261021114931_world.sql): the five boss steps' entries,
-- read from Turtle's own world data (tortoise-wow sql/base creature_template: all five are rank 1 there, so the telemetry would call
-- them 'e'). Bruegal Ironknuckle 1720 (a rare, rank 2) is left out. A DELETE of this one map, then REPLACE.
DELETE FROM `ai_dungeon_boss` WHERE `map` IN (34);
REPLACE INTO `ai_dungeon_boss` (`map`, `entry`, `name`) VALUES
(34, 1696, 'Targorr the Dread'),
(34, 1663, 'Dextren Ward'),
(34, 1666, 'Kam Deepfury'),
(34, 1717, 'Hamhock'),
(34, 1716, 'Bazil Thredd');
