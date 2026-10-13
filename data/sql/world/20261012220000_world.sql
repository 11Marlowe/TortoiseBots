-- Recruiter Kerra (spawn guid 4000033, entry 95032, Mercenary Hire beside
-- innkeeper 7744) was spawned with movement_type 2 (waypoint movement) but
-- has no waypoints, so it has no path to walk. Every other recruiter
-- stands still (0) or wanders (1). Give it 0 like the rest.
-- 20260928100000_world.sql stays as released: applied migrations must never
-- change. Idempotent: touches only that spawn, and only while it is still 2.
UPDATE `creature` SET `movement_type` = 0
WHERE `guid` = 4000033 AND `id` = 95032 AND `movement_type` = 2;
