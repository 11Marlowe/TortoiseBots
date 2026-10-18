-- Ragefire Chasm (map 389), leg 2 (trogg_cave): two approach points before pull groups 1 and 2, from the observe run of
-- 2026-10-17 13:21 (rfc-observe-1, server 19z13, the rows of 20261017130000_world.sql). Replaces the 11 step rows of
-- leg 2 with 13; the other legs and the plan and leg rows are untouched (that file stays as applied). Idempotent
-- (delete the leg's rows, insert them again). Dated after every open whole-plan re-export (20261017180000, 200000,
-- 210000, 220000), none of which holds a ragefire_chasm row.
--
-- The stall: the run paused twice at step 13 (pull group 2, the five-pack at -263.9 -34.6) with "8 approach hops did not
-- bring the party within 60.0 yd of the pack" (24 events, 13:27:16 to 13:33:00 and again to 13:33:53). The party stood
-- on the cave ramp at -169.1 -34.9 -41.4 (botwatch, Bwon and Redhammer, 13:27:19 to 13:29:49 and 13:31:01 to 13:33:19),
-- 95 yd from the pack. Each approach hop walked the lead bot down the ramp (to -206 -35 -50 at 13:26:55 and 13:30:43),
-- and the next tick's "gathering back at -169.1 -34.9 -40.9 before a pull (the fight left the lead bot off the way)"
-- walked it back up (15 such notes in events.json): the last point the run had walked to was 9 yd above the stand the
-- fight of pull group 1 had left it on. The manual `cmd move -205 -34 -52` plus resume at 13:30 went the same way. The
-- skip at 13:33:20 pulled the next pack into the five-pack and the priest died (516 -> 72 hp in 16 s at -244 -33 -58).
-- The hop logic stays to be fixed in runtime/DungeonRunPolicy.h (CheckAnchor, SendGather); this gives it a gather point
-- on the stand.
--
-- Two rows are new, move rows before the pulls (turtle-agent dungeons/ragefire_chasm.json, `approach` of pull groups 1
-- and 2; `turtle-agent plan-export --mmaps <map 389 mmaps> --approach-hops` of that plan; the other eight legs export
-- row for row as in 20261017130000_world.sql; no row is hand-set):
--   * leg 2 seq 2, before pull group 1 (the Trogg pair, -238.3 -36.0): -205 -34 -49.9, the point the manual move reached
--     (botwatch: the lead bot stood at -205 -34 -50 at 13:30:01). Map 389 navmesh floor -49.86 (a magma floor at -65.4
--     lies below it, under the avoid box); 24.3 yd from pull group 0 (-181.5 -32.1), 34.0 yd to the pair.
--   * leg 2 seq 4, before pull group 2 (the five-pack, -263.9 -34.6): -221.2 -34.7 -54.7, the waypoint of the patrolling
--     Trogg 48648 (creature_movement; navmesh floor -54.68). 16.9 yd from the first point, 33.4 to 49.4 yd to the five
--     members, 33 yd from the next pair (-240.7 -8.0). The navmesh paths are complete with the lava box applied.
-- The five-pack is not split: its Shamans (-253.9 -38.9 and -266.7 -28.0) stand 17 yd apart and the second has a Trogg
-- 5 yd away. The rows after the new ones move up (seq 3..10 become 3, 5..12), so the whole leg is written again.
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'ragefire_chasm' AND `leg` = 2;
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('ragefire_chasm', 2, 0, 'move', 0, -129.13, -35.16, -33.26, 0, 0, 0, 0, 0, '', 'to'),
('ragefire_chasm', 2, 1, 'pull', 0, -181.51, -32.07, -44.43, 20, 0, 0, 0, 0, '11318', 'pull group 0'),
('ragefire_chasm', 2, 2, 'move', 0, -205, -34, -49.9, 0, 0, 0, 0, 0, '', 'pull group 1 approach'),
('ragefire_chasm', 2, 3, 'pull', 0, -238.33, -35.95, -56.44, 12, 0, 0, 0, 0, '11318', 'pull group 1'),
('ragefire_chasm', 2, 4, 'move', 0, -221.2, -34.7, -54.7, 0, 0, 0, 0, 0, '', 'pull group 2 approach'),
('ragefire_chasm', 2, 5, 'pull', 0, -263.92, -34.62, -60.63, 12, 0, 0, 0, 0, '11319,11318', 'pull group 2'),
('ragefire_chasm', 2, 6, 'pull', 0, -263.11, -14.45, -60.97, 12, 0, 0, 0, 0, '11318', 'pull group 3'),
('ragefire_chasm', 2, 7, 'pull', 0, -289.53, -21.1, -61.52, 12, 0, 0, 0, 0, '11318', 'pull group 4'),
('ragefire_chasm', 2, 8, 'move', 0, -270.93338, -11.466794, -63.16366, 0, 0, 0, 0, 0, '', 'approach path'),
('ragefire_chasm', 2, 9, 'move', 0, -263.46674, -9.333462, -63.86366, 0, 0, 0, 0, 0, '', 'approach path'),
('ragefire_chasm', 2, 10, 'pull', 0, -215.45, -15.93, -61.8, 6, 0, 0, 0, 0, '11318', 'pull group 5'),
('ragefire_chasm', 2, 11, 'pull', 0, -211.15, -4.54, -61.76, 6, 0, 0, 0, 0, '11318,11319', 'pull group 6'),
('ragefire_chasm', 2, 12, 'pull', 0, -183.39, 2.8, -56.81, 12, 0, 0, 0, 0, '11318', 'pull group 7');
