-- Deadmines (map 36): the whole plan again, replacing the rows of 20261017130000_world.sql (that file stays as applied; it also holds
-- Ragefire Chasm and Wailing Caverns, which this file does not touch). Exported from turtle-agent fix/deadmines-packs 16efe83
-- (dungeons/deadmines.json: origin/dev abd37a3 plus the pack-size fixes) with the options of the header of 20261017130000:
--   turtle-agent plan-export --mmaps <map 36 mmaps> --allow-draft --gm-cross-direct --approach-hops --rev 16efe83 \
--     --stamp 20261022200000 deadmines
-- NO ROW IS HAND-SET. The plan row's cc_priority, cc_always and kill_first are exported by the plan-export (columns from
-- 20261015200000_world.sql), so no UPDATE follows. 16 legs, 210 steps, 227 rows (20261017130000: 202 steps, 219 rows; the 8 new rows are
-- pull approach moves). Plan check (turtle-agent plan-check, map 36 mmaps): ok, 221 points on the navmesh.
-- This file replaces a first draft (stamp 20261022190000) that was never applied anywhere; the cold review of 2026-10-22 found four defects in it.
--
-- WHY (Deadmines r12-19z29 and r14-19z32, 2026-10-22: 202/202 steps, 14 and 10 deaths; r14 killed 5 at step 90 and 5 at step 118, the tank
-- dead early in both, the mana users at 1-55 % at death after entering at 75-100 %): the packs were 2-10 times the plan's group math.
-- Patrolling units the pull rows did not count where they walk (the Engineer 622 spawn 79213 walks the Gilnid floor, the golem 2582923 from
-- the hall to the Harvester room's door, the Woodcarver 79207 to the Sneed door), Summon Remote-Controlled Golem adds, fleeing runners that drag a fight into the
-- next camp, and walks over uncleared ground (step 118's walk down the foundry spiral to the Craftsman 79247 engaged the terrace Engineers
-- at the spiral's foot).
--
-- WHAT CHANGES IN THE ROWS (the other 12 legs are row for row the same):
--   * the plan row: cc_priority '622,1731,4418,1729,1732' (was '4418,1729,1732'), cc_always '622' (was ''), kill_first '636,622' (same). 622 is
--     kill_first too: the run leader sheeps one of a pack of 622 (never the pull target) and kills the others first. CHECK LIVE: orders
--     `cmd cc moon <guid> <mage>` on 622 and 1731 (they were 0 in r12 and r14).
--   * leg 3 'sneed': the group that owns the Woodcarver 79207's beat (pull radius 19) is the first pull, the pair (radius 12) the second
--     (r12 step 50, a move at the door, was engaged 2 s in by the patroller and the pair: 5 Woodcarvers + the Shredder + Sneed, 1 dead).
--   * leg 6 'harvester' (10 -> 11 steps): the Taskmaster 4417 pull has radius 12 (was 6) round its west end: the disc stops at x -83.9, 17.0 yd
--     from the Masterpiece Harvester (a radius 22 reached 7 yd; the boss assists within 10 yd), and a Taskmaster on the east half of its beat is
--     waited for as before; the south-middle pack of step 90 (1731 + 622 + golem 61962, radius 9 now, was 5) stays ONE pull of three, so the joiner
--     estimate counts the golem (it stands 6.5 yd from the Engineer, same faction: it assists), behind a move row to the west stand (-112 -573
--     z 52.5): 20.2 yd from the Craftsman, 23.0 yd (3-D) from the beat of the ledge Overseer 2583051 (reach 15-16 yd) and 18 yd or more from the
--     boss; the first pull of the room, 622 at -152.7 -570.5, also names 61962 (the golem 2582923, which is in hall group 2 at its spawn and walks
--     to this room's door; the point -147.8 -567.8 of its beat, 5 yd away), so the pack counts it a second time.
--   * leg 7 'gilnid' (33 -> 41 steps): pull 3 is new, the two terrace Engineers 79212 and 79209 as one pull of two 622 from the terrace stand
--     (-195 -596 z 36.5, over the open pit: 19.3 yd 3-D from the Craftsman 79247, the nearest spawn pulled later); pull 4 (the Craftsman 79247
--     under the spiral, whose surface at z 32-34 lies right above him) is shot from the floor (-215 -590 z 21.1: 21.2 yd from him, 22.5 yd from the
--     nearest spawn pulled after it), after the Engineers are dead; then the floor ring one camp at a time, each behind a move row to a stand 17 yd
--     or more (3-D) from every spawn pulled later: the west Craftsman 79218 with the patrolling Engineer 79213 (-221 -596), the west-wall pair
--     79205/79208 (-225 -584), the terrace Craftsman 79203 (-221 -584), the northern pair 79197/79198 (in no group before; -217 -576), the
--     northern pair 79196/79201 (moved up from leg 8; -207 -568) and the Craftsman 79204 beside Gilnid (-203 -568). The old pulls 5, 6 and 9
--     (the Engineers 79212 and 79209 alone, the Engineer 79213 at radius 3 on its spawn point) are inside those groups. The boss row (Gilnid
--     1763, entries 1731) is unchanged: no boss approach moved. (A stand on the spiral at -195 -598 z 34.8 for 79247 was the first draft and is wrong:
--     no sight line, so the run leader would walk the tank down the 57 yd spiral to the Engineers' reach, the r14 step-118 wipe.)
--   * leg 8 'cannon_a' (9 -> 8 steps): its first pull (622 + 1731 at -187.05 -551.48, spawns 79196/79201) is leg 7's pull 9 now.
-- NOT IN THE ROWS: a rest row before steps 90 and 118 (the plan format has no rest step the exporter can write; the run leader's own rest
-- gate, under 80 % health / 50 % healer mana / 35 % mana, already ran at 1473 s, 1575 s and 2124 s of r12, and every ordered pull in r12 and r14
-- started at 53-100 % mana). The Summon Remote-Controlled Golem adds are not modelled, and the stands were checked only with a navmesh sight proxy
-- (no walkable surface crosses the eye-height line; no vmaps here), not a line-of-sight test: the live run shows both.
--
-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- deadmines: map 36, 16 legs, 210 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'deadmines';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'deadmines';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'deadmines';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('deadmines', 36, '622,1731,4418,1729,1732', '622', '636,622', 'turtle-agent 16efe83 dungeons/deadmines.json', 'plan v1, draft (not mapped, not live-checked)');
INSERT INTO `ai_dungeon_leg` (`plan`, `leg`, `name`, `comment`) VALUES
('deadmines', 0, 'mine_a', ''),
('deadmines', 1, 'rhahk_zor', ''),
('deadmines', 2, 'factory', ''),
('deadmines', 3, 'sneed', ''),
('deadmines', 4, 'west_area', ''),
('deadmines', 5, 'hall', ''),
('deadmines', 6, 'harvester', ''),
('deadmines', 7, 'gilnid', ''),
('deadmines', 8, 'cannon_a', ''),
('deadmines', 9, 'cannon', ''),
('deadmines', 10, 'dock', ''),
('deadmines', 11, 'mr_smite', ''),
('deadmines', 12, 'deck_a', ''),
('deadmines', 13, 'deck_b', ''),
('deadmines', 14, 'captain_greenskin', ''),
('deadmines', 15, 'edwin_vancleef', '');
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('deadmines', 0, 0, 'move', 0, -17.3, -383.7, 62.4, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 0, 1, 'pull', 0, -36.2, -371.06, 58.1, 13, 0, 0, 0, 0, '598', 'pull group 0'),
('deadmines', 0, 2, 'pull', 0, -49.18, -392.16, 55.91, 6, 0, 0, 0, 0, '1729', 'pull group 1'),
('deadmines', 0, 3, 'pull', 0, -53.66, -403.97, 55.41, 6, 0, 0, 0, 0, '598', 'pull group 2'),
('deadmines', 0, 4, 'pull', 0, -52.65, -368.03, 55.59, 20, 0, 0, 0, 0, '598', 'pull group 3'),
('deadmines', 0, 5, 'pull', 0, -58.95, -390.73, 53.95, 7, 0, 0, 0, 0, '1729', 'pull group 4'),
('deadmines', 0, 6, 'pull', 0, -69.7, -367.15, 55.24, 10, 0, 0, 0, 0, '598', 'pull group 5'),
('deadmines', 0, 7, 'pull', 0, -70.89, -395.71, 55.8, 8, 0, 0, 0, 0, '598', 'pull group 6'),
('deadmines', 0, 8, 'pull', 0, -72.6, -409.2, 55.01, 8, 0, 0, 0, 0, '634,598', 'pull group 7'),
('deadmines', 0, 9, 'pull', 0, -79.07, -382.14, 55.39, 10, 0, 0, 0, 0, '598', 'pull group 8'),
('deadmines', 0, 10, 'pull', 0, -83.01, -396.35, 59.09, 5, 0, 0, 0, 0, '598', 'pull group 9'),
('deadmines', 0, 11, 'pull', 0, -91.77, -376.14, 57.83, 6, 0, 0, 0, 0, '1729', 'pull group 10'),
('deadmines', 0, 12, 'pull', 0, -91.35, -401.85, 58.26, 5, 0, 0, 0, 0, '4416,634,1729', 'pull group 11'),
('deadmines', 0, 13, 'pull', 0, -100.51, -373, 59.22, 7, 0, 0, 0, 0, '598', 'pull group 12'),
('deadmines', 0, 14, 'pull', 0, -98.28, -383.23, 59.3, 7, 0, 0, 0, 0, '598', 'pull group 13'),
('deadmines', 1, 0, 'move', 0, -113.3, -386.7, 57.2, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 1, 1, 'pull', 0, -108.02, -403.98, 60.02, 8, 0, 0, 0, 0, '598', 'pull group 0'),
('deadmines', 1, 2, 'pull', 0, -116.61, -409.11, 59.19, 5, 0, 0, 0, 0, '634,598', 'pull group 1'),
('deadmines', 1, 3, 'pull', 0, -128.3, -384.09, 59.19, 7, 0, 0, 0, 0, '598', 'pull group 2'),
('deadmines', 1, 4, 'pull', 0, -130.81, -394.41, 59.42, 7, 0, 0, 0, 0, '598', 'pull group 3'),
('deadmines', 1, 5, 'pull', 0, -139.8, -399.8, 58.44, 7, 0, 0, 0, 0, '598', 'pull group 4'),
('deadmines', 1, 6, 'pull', 0, -139.34, -412.57, 58.93, 7, 0, 0, 0, 0, '598', 'pull group 5'),
('deadmines', 1, 7, 'pull', 0, -147.79, -407.3, 57.78, 7, 0, 0, 0, 0, '598', 'pull group 6'),
('deadmines', 1, 8, 'pull', 0, -153.94, -395.62, 57, 10, 0, 0, 0, 0, '598', 'pull group 7'),
('deadmines', 1, 9, 'pull', 0, -170.79, -403.69, 57.36, 8, 0, 0, 0, 0, '598,634,1729', 'pull group 8'),
('deadmines', 1, 10, 'pull', 0, -182.3, -410.3, 55.62, 8, 0, 0, 0, 0, '598', 'pull group 9'),
('deadmines', 1, 11, 'boss', 644, -192.915, -448.211, 54.4339, 0, 0, 0, 0, 0, '1725', 'boss'),
('deadmines', 1, 12, 'wait_object', 13965, -191.414, -457.446, 54.4391, 0, 0, 1, 0, 60, '', 'wait for object 13965 active: Rhahk''Zor''s death opens the Factory Door (13965; instance_deadmines OnCreatureDeath NPC_RHAHKZOR).'),
('deadmines', 2, 0, 'move', 0, -192.4, -458.9, 54.3, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 2, 1, 'pull', 0, -189.88, -483.08, 54.04, 6, 0, 0, 0, 0, '1729,634', 'pull group 0'),
('deadmines', 2, 2, 'pull', 0, -176.81, -487.95, 54.25, 8, 0, 0, 0, 0, '598', 'pull group 1'),
('deadmines', 2, 3, 'pull', 0, -197.44, -490.74, 54.02, 6, 0, 0, 0, 0, '598', 'pull group 2'),
('deadmines', 2, 4, 'pull', 0, -176.35, -499.36, 54.23, 8, 0, 0, 0, 0, '598', 'pull group 3'),
('deadmines', 2, 5, 'pull', 0, -158.29, -510.28, 53.1, 7, 0, 0, 0, 0, '598', 'pull group 4'),
('deadmines', 2, 6, 'pull', 0, -170.74, -509.42, 53.72, 8, 0, 0, 0, 0, '598', 'pull group 5'),
('deadmines', 2, 7, 'pull', 0, -186.47, -506.31, 53.4, 9, 0, 0, 0, 0, '598', 'pull group 6'),
('deadmines', 2, 8, 'pull', 0, -152.81, -530.05, 51.35, 6, 0, 0, 0, 0, '598', 'pull group 7'),
('deadmines', 2, 9, 'pull', 0, -149.5, -519.48, 53.09, 6, 0, 0, 0, 0, '598', 'pull group 8'),
('deadmines', 2, 10, 'move', 0, -162.13342, -510.4, 53.311405, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 2, 11, 'move', 0, -180.00008, -504.8, 54.111404, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 2, 12, 'pull', 0, -206.48, -512.78, 52.18, 9, 0, 0, 0, 0, '598', 'pull group 9'),
('deadmines', 2, 13, 'pull', 0, -207.56, -499.16, 51.04, 9, 0, 0, 0, 0, '598', 'pull group 10'),
('deadmines', 2, 14, 'pull', 0, -222.04, -500.22, 49.61, 11, 0, 0, 0, 0, '598', 'pull group 11'),
('deadmines', 2, 15, 'pull', 0, -219.44, -485.82, 49.5, 11, 0, 0, 0, 0, '598', 'pull group 12'),
('deadmines', 2, 16, 'pull', 0, -233.46, -490.31, 49.6, 10, 0, 0, 0, 0, '598', 'pull group 13'),
('deadmines', 2, 17, 'pull', 0, -231.31, -476.99, 49.34, 10, 0, 0, 0, 0, '598', 'pull group 14'),
('deadmines', 2, 18, 'move', 0, -261.1, -485.6, 49.5, 0, 0, 0, 0, 0, '', 'stand'),
('deadmines', 2, 19, 'open', 17153, -262.714, -482.361, 49.4353, 0, 0, 0, 0, 0, '', 'open'),
('deadmines', 2, 20, 'wait_object', 17153, -262.714, -482.361, 49.4353, 0, 0, 1, 0, 60, '', 'wait for object 17153 active: The Heavy Door at -262.7 -482.4 (spawn 26183) has no instance script and no lever: it is a plain door (lock 86, no key) that a player clicks open. If it does not open, the mapping walk must find what opens it.'),
('deadmines', 3, 0, 'move', 0, -261.1, -485.6, 49.5, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 3, 1, 'pull', 0, -277.26, -494.08, 49.59, 19, 0, 0, 0, 0, '641', 'pull group 0'),
('deadmines', 3, 2, 'pull', 0, -288.33, -479.33, 49.28, 12, 0, 0, 0, 0, '641', 'pull group 1'),
('deadmines', 3, 3, 'pull', 0, -281.28, -502.01, 49.38, 5, 0, 0, 0, 0, '641', 'pull group 2'),
('deadmines', 3, 4, 'pull', 0, -308.87, -485.28, 49.8, 15, 0, 0, 0, 0, '641', 'pull group 3'),
('deadmines', 3, 5, 'pull', 0, -294.26, -506.22, 49.27, 5, 0, 0, 0, 0, '641', 'pull group 4'),
('deadmines', 3, 6, 'pull', 0, -273.85, -518.12, 49.68, 13, 0, 0, 0, 0, '641', 'pull group 5'),
('deadmines', 3, 7, 'pull', 0, -295.53, -530.02, 49.39, 15, 0, 0, 0, 0, '641', 'pull group 6'),
('deadmines', 3, 8, 'boss', 642, -289.453, -513.009, 49.6785, 0, 0, 0, 0, 0, '', 'boss'),
('deadmines', 3, 9, 'wait_clear', 0, -289.453, -513.009, 49.6785, 30, 0, 0, 0, 90, '', 'wait until no hostile: Killing Sneed''s Shredder (642) ejects Sneed (643; spell 5141 ''Eject Sneed'', no spawn row): he must be killed too, his death opens the Heavy Door at -290.3 -537.0 (OnCreatureDeath NPC_SNEED).'),
('deadmines', 3, 10, 'wait_object', 17153, -290.294, -536.96, 49.4353, 0, 0, 1, 0, 60, '', 'wait for object 17153 active: Sneed''s death opens the Heavy Door at -290.3 -537.0 (spawn 26185); the lever 101832 at -287.3 -539.9 beyond it opens it for 15 s as well.'),
('deadmines', 4, 0, 'move', 0, -292.5, -538.8, 49.3, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 4, 1, 'pull', 0, -283.23, -560, 49.51, 4, 0, 0, 0, 0, '641', 'pull group 0'),
('deadmines', 4, 2, 'pull', 0, -290.57, -561.45, 48.85, 4, 0, 0, 0, 0, '4417', 'pull group 1'),
('deadmines', 4, 3, 'pull', 0, -297.79, -567.18, 50.07, 6, 0, 0, 0, 0, '4416', 'pull group 2'),
('deadmines', 4, 4, 'pull', 0, -287.62, -576.59, 49.28, 7, 0, 0, 0, 0, '4416', 'pull group 3'),
('deadmines', 4, 5, 'pull', 0, -305.66, -579.79, 48.84, 9, 0, 0, 0, 0, '4416', 'pull group 4'),
('deadmines', 4, 6, 'pull', 0, -294.31, -584.52, 49.76, 7, 0, 0, 0, 0, '4416', 'pull group 5'),
('deadmines', 4, 7, 'pull', 0, -313.12, -592.62, 48.57, 11, 0, 0, 0, 0, '4416', 'pull group 6'),
('deadmines', 4, 8, 'pull', 0, -292.77, -608.24, 47.7, 10, 0, 0, 0, 0, '641', 'pull group 7'),
('deadmines', 4, 9, 'pull', 0, -281.69, -601.38, 50.84, 10, 0, 0, 0, 0, '4416', 'pull group 8'),
('deadmines', 4, 10, 'pull', 0, -276.87, -588.67, 50.97, 3, 0, 0, 0, 0, '4416', 'pull group 9'),
('deadmines', 4, 11, 'pull', 0, -275.49, -574.47, 50.52, 6, 0, 0, 0, 0, '4416', 'pull group 10'),
('deadmines', 4, 12, 'pull', 0, -259.33, -587.67, 51.03, 7, 0, 0, 0, 0, '4416', 'pull group 11'),
('deadmines', 4, 13, 'pull', 0, -263.8, -578.19, 50.56, 3, 0, 0, 0, 0, '4417', 'pull group 12'),
('deadmines', 4, 14, 'pull', 0, -263.51, -571.29, 51.05, 9, 0, 0, 0, 0, '641', 'pull group 13'),
('deadmines', 4, 15, 'move', 0, -245.5, -575.8, 51.3, 0, 0, 0, 0, 0, '', 'stand'),
('deadmines', 4, 16, 'open', 17154, -242.965, -578.561, 51.1366, 0, 0, 0, 0, 0, '', 'open'),
('deadmines', 4, 17, 'wait_object', 17154, -242.965, -578.561, 51.1366, 0, 0, 1, 0, 60, '', 'wait for object 17154 active: The Heavy Door 17154 at -243.0 -578.6 (spawn 26184) has no instance script and no lever: a plain door (lock 86) that a player clicks open. If it does not open, the mapping walk must find what opens it.'),
('deadmines', 5, 0, 'move', 0, -245.5, -575.8, 51.2, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 5, 1, 'pull', 0, -205.31, -548.82, 51.23, 15, 0, 0, 0, 0, '634', 'pull group 0'),
('deadmines', 5, 2, 'pull', 0, -205.67, -531.17, 51.23, 14, 0, 0, 0, 0, '1731', 'pull group 1'),
('deadmines', 5, 3, 'pull', 0, -185.99, -530.84, 51.27, 12, 0, 0, 0, 0, '61962', 'pull group 2'),
('deadmines', 5, 4, 'pull', 0, -170.37, -535.16, 51.23, 9, 0, 0, 0, 0, '622', 'pull group 3'),
('deadmines', 5, 5, 'pull', 0, -162.19, -555.34, 51.23, 11, 0, 0, 0, 0, '622,1731', 'pull group 4'),
('deadmines', 6, 0, 'move', 0, -151.5, -560, 51.3, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 6, 1, 'pull', 0, -150.27, -569.13, 51.21, 11, 0, 0, 0, 0, '622,61962', 'pull group 0'),
('deadmines', 6, 2, 'pull', 0, -118.07, -560.75, 51.23, 7, 0, 0, 0, 0, '1731,61962', 'pull group 1'),
('deadmines', 6, 3, 'pull', 0, -113.65, -580.87, 51.23, 14, 0, 0, 0, 0, '61962,622', 'pull group 2'),
('deadmines', 6, 4, 'pull', 0, -95.93, -569.47, 51.23, 12, 0, 0, 0, 0, '4417', 'pull group 3'),
('deadmines', 6, 5, 'move', 0, -112, -573, 52.5, 0, 0, 0, 0, 0, '', 'pull group 4 approach'),
('deadmines', 6, 6, 'pull', 0, -87.49, -580.41, 51.23, 9, 0, 0, 0, 0, '1731,622,61962', 'pull group 4'),
('deadmines', 6, 7, 'pull', 0, -92.23, -559.99, 51.23, 9, 0, 0, 0, 0, '622', 'pull group 5'),
('deadmines', 6, 8, 'pull', 0, -81.02, -555.55, 51.23, 9, 0, 0, 0, 0, '61962', 'pull group 6'),
('deadmines', 6, 9, 'pull', 0, -63.79, -559.28, 51.23, 5, 0, 0, 0, 0, '622', 'pull group 7'),
('deadmines', 6, 10, 'boss', 61963, -66.8989, -570.032, 51.2299, 0, 0, 0, 0, 0, '', 'boss'),
('deadmines', 7, 0, 'move', 0, -71.733444, -573.0668, 52.45861, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 1, 'move', 0, -73.06678, -574.4001, 52.45861, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 2, 'move', 0, -85.33344, -576.0001, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 3, 'move', 0, -100.53343, -574.9334, 52.45861, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 4, 'move', 0, -101.333435, -574.9334, 52.45861, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 5, 'move', 0, -106.66676, -576.0001, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 6, 'move', 0, -115.200096, -574.66675, 52.45861, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 7, 'move', 0, -162.23412, -557.70734, 52.034626, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 8, 'move', 0, -170.66675, -554.66675, 51.95861, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 9, 'move', 0, -172.53342, -553.33344, 51.95861, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 10, 'move', 0, -178.1334, -543.4668, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 11, 'move', 0, -181.33342, -538.1334, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 12, 'move', 0, -184.26674, -534.9335, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 13, 'move', 0, -187.46675, -533.6001, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 14, 'move', 0, -192.00008, -533.86676, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 15, 'move', 0, -196.5334, -536.53345, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 16, 'move', 0, -201.86674, -542.66675, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 17, 'move', 0, -202.1334, -543.73346, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 18, 'move', 0, -201.86674, -548.2668, 51.258614, 0, 0, 0, 0, 0, '', 'path'),
('deadmines', 7, 19, 'move', 0, -199.7, -551.4, 51.3, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 7, 20, 'pull', 0, -182.8, -565.8, 51.13, 20, 0, 0, 0, 0, '1731', 'pull group 0'),
('deadmines', 7, 21, 'pull', 0, -188.73, -600.47, 36.58, 12, 0, 0, 0, 0, '1731', 'pull group 1'),
('deadmines', 7, 22, 'pull', 0, -205.8, -604.77, 29.36, 8, 0, 0, 0, 0, '1731', 'pull group 2'),
('deadmines', 7, 23, 'move', 0, -195, -596, 36.5, 0, 0, 0, 0, 0, '', 'pull group 3 approach'),
('deadmines', 7, 24, 'pull', 0, -202.74, -584.35, 20.98, 10, 0, 0, 0, 0, '622', 'pull group 3'),
('deadmines', 7, 25, 'move', 0, -215, -590, 21.1, 0, 0, 0, 0, 0, '', 'pull group 4 approach'),
('deadmines', 7, 26, 'pull', 0, -198.93, -603.78, 19.3, 8, 0, 0, 0, 0, '1731', 'pull group 4'),
('deadmines', 7, 27, 'move', 0, -221, -596, 21.8, 0, 0, 0, 0, 0, '', 'pull group 5 approach'),
('deadmines', 7, 28, 'pull', 0, -233.75, -572.8, 19.31, 9, 0, 0, 0, 0, '1731,622', 'pull group 5'),
('deadmines', 7, 29, 'move', 0, -225, -584, 20.9, 0, 0, 0, 0, 0, '', 'pull group 6 approach'),
('deadmines', 7, 30, 'pull', 0, -226.07, -559.82, 19.31, 5, 0, 0, 0, 0, '1731,622', 'pull group 6'),
('deadmines', 7, 31, 'move', 0, -221, -584, 21, 0, 0, 0, 0, 0, '', 'pull group 7 approach'),
('deadmines', 7, 32, 'pull', 0, -209.57, -568.15, 20.98, 12, 0, 0, 0, 0, '1731', 'pull group 7'),
('deadmines', 7, 33, 'move', 0, -217, -576, 21.1, 0, 0, 0, 0, 0, '', 'pull group 8 approach'),
('deadmines', 7, 34, 'pull', 0, -208.61, -552, 19.31, 8, 0, 0, 0, 0, '1731,622', 'pull group 8'),
('deadmines', 7, 35, 'move', 0, -207, -568, 21.1, 0, 0, 0, 0, 0, '', 'pull group 9 approach'),
('deadmines', 7, 36, 'pull', 0, -187.05, -551.48, 19.3, 11, 0, 0, 0, 0, '622,1731', 'pull group 9'),
('deadmines', 7, 37, 'move', 0, -203, -568, 21.1, 0, 0, 0, 0, 0, '', 'pull group 10 approach'),
('deadmines', 7, 38, 'pull', 0, -178.4, -565.43, 19.31, 6, 0, 0, 0, 0, '1731', 'pull group 10'),
('deadmines', 7, 39, 'boss', 1763, -177.362, -574.458, 19.3113, 0, 0, 0, 0, 0, '1731', 'boss'),
('deadmines', 7, 40, 'wait_object', 17153, -168.514, -579.861, 19.3159, 0, 0, 1, 0, 60, '', 'wait for object 17153 active: Gilnid''s death opens the Heavy Door at -168.5 -579.9 (spawn 26182, OnCreatureDeath NPC_GILDNID); the lever 101834 at -165.4 -576.9 beyond it opens it for 15 s as well.'),
('deadmines', 8, 0, 'move', 0, -177.4, -574.5, 19.4, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 8, 1, 'pull', 0, -153, -590.71, 19.05, 7, 0, 0, 0, 0, '4416', 'pull group 0'),
('deadmines', 8, 2, 'pull', 0, -149.15, -580.59, 19.31, 7, 0, 0, 0, 0, '4417', 'pull group 1'),
('deadmines', 8, 3, 'pull', 0, -139.83, -569.44, 19.79, 9, 0, 0, 0, 0, '4416', 'pull group 2'),
('deadmines', 8, 4, 'pull', 0, -131.69, -578.84, 18.65, 9, 0, 0, 0, 0, '4416', 'pull group 3'),
('deadmines', 8, 5, 'pull', 0, -135.13, -593.67, 17.9, 6, 0, 0, 0, 0, '4416', 'pull group 4'),
('deadmines', 8, 6, 'pull', 0, -138, -602.4, 16.11, 6, 0, 0, 0, 0, '4416', 'pull group 5'),
('deadmines', 8, 7, 'pull', 0, -126.25, -596.36, 17.81, 6, 0, 0, 0, 0, '4416', 'pull group 6'),
('deadmines', 9, 0, 'move', 0, -128, -597.3, 17.2, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 9, 1, 'pull', 0, -127.41, -607.54, 15.17, 8, 0, 0, 0, 0, '4416', 'pull group 0'),
('deadmines', 9, 2, 'pull', 0, -138.05, -613.23, 15.3, 6, 0, 0, 0, 0, '4416', 'pull group 1'),
('deadmines', 9, 3, 'pull', 0, -105.27, -617.09, 14.45, 7, 0, 0, 0, 0, '4416', 'pull group 2'),
('deadmines', 9, 4, 'pull', 0, -115.19, -621.2, 14.11, 7, 0, 0, 0, 0, '4416', 'pull group 3'),
('deadmines', 9, 5, 'pull', 0, -137.65, -622.89, 15.04, 6, 0, 0, 0, 0, '4416', 'pull group 4'),
('deadmines', 9, 6, 'pull', 0, -132.5, -637.78, 14.33, 10, 0, 0, 0, 0, '4416', 'pull group 5'),
('deadmines', 9, 7, 'pull', 0, -118.95, -637.26, 11.82, 10, 0, 0, 0, 0, '4416,4418', 'pull group 6'),
('deadmines', 9, 8, 'pull', 0, -104.38, -640.79, 8.77, 12, 0, 0, 0, 0, '4416', 'pull group 7'),
('deadmines', 9, 9, 'pull', 0, -115.42, -651.49, 8.16, 10, 0, 0, 0, 0, '4416', 'pull group 8'),
('deadmines', 9, 10, 'loot', 17155, -106.409, -617.284, 13.8495, 0, 5397, 0, 0, 0, '', 'loot (Defias Gunpowder (item 5397))'),
('deadmines', 9, 11, 'wait_clear', 0, -106.409, -617.284, 13.8495, 45, 0, 0, 0, 90, '', 'wait until no hostile: Looting the Defias Gunpowder (17155) summons one Defias Overseer (634) that runs to -115.3 -617.4: kill him (and anything else within 45 yd) before the cannon.'),
('deadmines', 9, 12, 'move', 0, -109.7, -655.5, 8.2, 0, 0, 0, 0, 0, '', 'stand'),
('deadmines', 9, 13, 'open', 16398, -107.562, -659.674, 7.21211, 0, 5397, 0, 0, 0, '', 'open (Defias Gunpowder (item 5397))'),
('deadmines', 9, 14, 'wait_object', 16397, -100.502, -668.771, 7.41049, 0, 0, 2, 0, 60, '', 'wait for object 16397 active_alt: The Defias Cannon (16398) needs the Defias Gunpowder (item 5397, lock 83); its use sets the Iron Clad Door (16397) to the alternative open state (instance_deadmines SetData TYPE_DEFIAS_ENDDOOR).'),
('deadmines', 10, 0, 'move', 0, -109.2, -656.5, 7.1, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 10, 1, 'pull', 0, -123.16, -681.32, 10.56, 14, 0, 0, 0, 0, '3947', 'pull group 0'),
('deadmines', 10, 2, 'pull', 0, -121.64, -698.16, 8.69, 14, 0, 0, 0, 0, '3947', 'pull group 1'),
('deadmines', 10, 3, 'pull', 0, -89.7, -691.33, 8.25, 11, 0, 0, 0, 0, '657', 'pull group 2'),
('deadmines', 10, 4, 'pull', 0, -102.52, -697.94, 8.84, 11, 0, 0, 0, 0, '657', 'pull group 3'),
('deadmines', 10, 5, 'pull', 0, -93.94, -722.4, 8.48, 11, 0, 0, 0, 0, '657,3450', 'pull group 4'),
('deadmines', 10, 6, 'pull', 0, -79.97, -729.44, 8.95, 16, 0, 0, 0, 0, '657', 'pull group 5'),
('deadmines', 10, 7, 'pull', 0, -60.24, -731.37, 8.82, 9, 0, 0, 0, 0, '1732', 'pull group 6'),
('deadmines', 10, 8, 'pull', 0, -50.14, -723.34, 9.08, 8, 0, 0, 0, 0, '1732', 'pull group 7'),
('deadmines', 10, 9, 'pull', 0, -42.24, -732.09, 9.04, 8, 0, 0, 0, 0, '1732', 'pull group 8'),
('deadmines', 10, 10, 'pull', 0, -31.38, -727.91, 8.47, 13, 0, 0, 0, 0, '657', 'pull group 9'),
('deadmines', 11, 0, 'move', 0, -28.8, -733.1, 8.3, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 11, 1, 'pull', 0, -15.01, -724.66, 7.91, 10, 0, 0, 0, 0, '657', 'pull group 0'),
('deadmines', 11, 2, 'pull', 0, -22.57, -736.14, 8.63, 9, 0, 0, 0, 0, '1732', 'pull group 1'),
('deadmines', 11, 3, 'pull', 0, -8.14, -740.62, 8.88, 11, 0, 0, 0, 0, '657,3450', 'pull group 2'),
('deadmines', 11, 4, 'move', 0, -4.5334625, -780.5334, 9.9586115, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 11, 5, 'move', 0, -5.3334627, -781.60004, 10.258611, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 11, 6, 'move', 0, -5.8667955, -782.13336, 10.658611, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 11, 7, 'pull', 0, -43.45, -787.8, 18.62, 17, 0, 0, 0, 0, '657,3947', 'pull group 3'),
('deadmines', 11, 8, 'pull', 0, -25.35, -799.31, 19.58, 2, 0, 0, 0, 0, '657,1732', 'pull group 4'),
('deadmines', 11, 9, 'pull', 0, -57.05, -782.48, 18.01, 9, 0, 0, 0, 0, '1732', 'pull group 5'),
('deadmines', 11, 10, 'move', 0, -47.466785, -786.6667, 18.45861, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 11, 11, 'move', 0, -25.86679, -796.2667, 19.45861, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 11, 12, 'move', 0, -24.533457, -796.00006, 20.45861, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 11, 13, 'move', 0, -23.733458, -795.4667, 20.358612, 0, 0, 0, 0, 0, '', 'approach path'),
('deadmines', 11, 14, 'pull', 0, -1.95, -779.34, 9.92, 26, 0, 0, 1, 0, '636', 'pull group 6'),
('deadmines', 11, 15, 'boss', 646, -22.8471, -797.283, 20.3745, 0, 0, 0, 0, 0, '', 'boss'),
('deadmines', 12, 0, 'move', 0, -59.4, -786.3, 18.2, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 12, 1, 'pull', 0, -79.54, -781.29, 17.7, 6, 0, 0, 0, 0, '657,3450', 'pull group 0'),
('deadmines', 12, 2, 'pull', 0, -94.17, -787.37, 17.11, 7, 0, 0, 0, 0, '3947', 'pull group 1'),
('deadmines', 12, 3, 'pull', 0, -99.94, -778.27, 22.34, 8, 0, 0, 0, 0, '1732', 'pull group 2'),
('deadmines', 13, 0, 'move', 0, -106.7, -787.5, 17.3, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 13, 1, 'pull', 0, -127.42, -788.93, 17.36, 14, 0, 0, 0, 0, '3947,657', 'pull group 0'),
('deadmines', 13, 2, 'pull', 0, -122.84, -806.02, 16.89, 14, 0, 0, 0, 0, '3947', 'pull group 1'),
('deadmines', 13, 3, 'pull', 0, -112.98, -796.37, 16.89, 9, 0, 0, 0, 0, '657,3450', 'pull group 2'),
('deadmines', 13, 4, 'pull', 0, -83.51, -776.18, 26.8, 8, 0, 0, 0, 0, '657', 'pull group 3'),
('deadmines', 13, 5, 'pull', 0, -75.83, -784.08, 26.31, 6, 0, 0, 0, 0, '657,3450', 'pull group 4'),
('deadmines', 13, 6, 'pull', 0, -83.5, -791.17, 25.85, 8, 0, 0, 0, 0, '3947', 'pull group 5'),
('deadmines', 13, 7, 'pull', 0, -93.31, -785.65, 27.21, 7, 0, 0, 0, 0, '657', 'pull group 6'),
('deadmines', 13, 8, 'pull', 0, -122.59, -823.97, 16.94, 9, 0, 0, 0, 0, '3947', 'pull group 7'),
('deadmines', 14, 0, 'move', 0, -96.8, -800, 32.1, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 14, 1, 'pull', 0, -97.65, -805.56, 30.68, 12, 0, 0, 0, 0, '3947', 'pull group 0'),
('deadmines', 14, 2, 'pull', 0, -106.43, -793.29, 28.16, 11, 0, 0, 0, 0, '657', 'pull group 1'),
('deadmines', 14, 3, 'pull', 0, -78.98, -792.49, 38.6, 9, 0, 0, 0, 0, '3947,1732', 'pull group 2'),
('deadmines', 14, 4, 'pull', 0, -65.92, -794.4, 39.4, 9, 0, 0, 0, 0, '657', 'pull group 3'),
('deadmines', 14, 5, 'pull', 0, -55.31, -790.77, 39.12, 6, 0, 0, 0, 0, '657,3450', 'pull group 4'),
('deadmines', 14, 6, 'pull', 0, -40.43, -796.79, 39.37, 10, 0, 0, 0, 0, '657,3450', 'pull group 5'),
('deadmines', 14, 7, 'boss', 647, -68.9846, -807.998, 40.7529, 0, 0, 0, 0, 0, '3450,657', 'boss'),
('deadmines', 15, 0, 'move', 0, -87.4, -819.9, 39.4, 0, 0, 0, 0, 0, '', 'to'),
('deadmines', 15, 1, 'pull', 0, -80.27, -820.09, 39.75, 4, 0, 0, 1, 0, '636', 'pull group 0'),
('deadmines', 15, 2, 'boss', 639, -87.369, -819.895, 39.3004, 0, 0, 0, 0, 0, '', 'boss');
