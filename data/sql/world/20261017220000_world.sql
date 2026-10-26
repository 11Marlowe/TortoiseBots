-- Uldaman (map 70): the scorpid pit's first crossing step becomes a plain move. Rows of 20261017180000_world.sql
-- (and of the open whole-plan re-exports 20261017200000/20261017210000, whichever applies last), changed in place;
-- idempotent, UPDATE only. Mirrored in turtle-agent dungeons/uldaman.json (pjerra/turtle-agent#226).
--
-- Step 40 (leg 5 sentinel, seq 1): -161.1 284.5 -52.1 on the pit floor, 'move_direct' -> 'move'. The point is on the
-- map 70 navmesh (0.0 yd off) and the navmesh path from the lip (-163.5 298.7 -52.5, step 39) to it is complete,
-- 17.0 yd, so it needs no direct step: a plain `cmd move` walks the server's path there and the party follows. The one
-- hole of the mesh is only the ramp after it (-161.1 284.5 -52.1 -> -158.4 283.7 -48.9, 4.2 yd, path partial), which
-- stays the one direct step (seq 2, unchanged). Checked on the live map 70 vmap (lostool, yulon-ubuntu2
-- ~/tb-ci/vmaps70): sight lines on that step clear at 1.5, 2 and 2.5 yd both ways; floor under the line -52.32 at the
-- pit point rising to -49.03 at the east point in steps of at most 0.8 yd; floor under the lip -52.85.
--
-- Why: both pauses of the run of 2026-10-17 (uldaman-events-4, 14:03 and 14:39) were on the walk back after the lead
-- bot died: "the point of step 40 refused 4 times (too-far: <bot> is not with Bonky ...)". The bots named were the
-- ones held alive where the lead bot fell (Bearcaller, Quickseeker 320 yd away at the descent), not the ones beside
-- it on the lip (their navmesh paths to the lead bot's spot: 2.5, 4.0 and 8.7 yd, complete). The run leader's fix is
-- in runtime/DungeonRunPolicy.h (a summoned held bot leaves the hold so the summon is waited for); with one direct
-- step instead of two, the walk back meets that check once.
UPDATE `ai_dungeon_step` SET `kind` = 'move'
WHERE `plan` = 'uldaman' AND `leg` = 5 AND `seq` = 1 AND `kind` = 'move_direct'
  AND ABS(`x` - -161.1) < 0.05 AND ABS(`y` - 284.5) < 0.05;
