-- Dire Maul North (map 429): the first plan of the wing, rebuilt from the map-429 spawn census of 2026-10-24 (dungeons/dire_maul_north.json,
-- status 'draft (not mapped, not live-checked)'), exported with the options of the header of 20261023140000:
--   turtle-agent plan-export --mmaps <map 429 mmaps> --allow-draft --gm-cross-direct --approach-hops --rev 107dead \
--     --stamp 20261024150900 --out <dir> dire_maul_north
-- NO ROW IS HAND-SET. Sources: tools/dungeongen/testdata/map429 (creatures, gameobjects, entrances, portals) and ~/tortoise-wow/sql/base
-- (creature_template, creature_ai_scripts, creature_spells, creature_groups, pool_*, creature_loot_template, gameobject*) plus the core's
-- src/scripts/dungeons/dire_maul (instance_dire_maul.cpp, boss_gordok_king.cpp). No live DB, no vmaps, no live run.
-- What the census found and the plan does:
--   * The generated skeleton carried only 13036 mastiffs, 13160 Carrion Swarmers and 11446 Gordok Spirit: the generator drops every spawn
--     with npc_flags != 0, and the Gordok ogres carry npc_flags 1 (gossip). They are hostile (faction 45) -- 11441 Brute, 11444 Mage-Lord,
--     11445 Captain, 11448 Warlock, 11450 Reaver -- and are 99 of the 225 hostile trash spawns (114 export rows: 15 are unused variants
--     of the First Pack pool). The plan pulls them: 70 pull groups.
--   * 11446 Gordok Spirit (13 spawns in the King's ring) has unit_flags 514 (non-attackable) and 1 hp: no pull can land on it. The 13
--     spirit pulls of the skeleton are gone. 14338 Knot Thimblejack (faction 35) and 14322 Stomper Kreeg (vendor) are not pulled.
--   * The courtyard's First Pack is pool-of-pools 4294 (one of six variants 4295-4300 live, max_limit 1): 4 ogres at the four points
--     293.43 22.68, 290 43.94, 278.29 32.93 and 299.92 40.06, entries Brute/Mage-Lord plus a mastiff in variants 2 and 5. The pull lists all
--     three entries at those four points, so whichever variant is live is pulled; the other five are not spawned.
--   * Leg order follows the two locked doors: Mol'dar (courtyard) -> great hall + Fengus -> Gordok Courtyard Door -> Slip'kik (lower ring)
--     -> Gordok Inner Door -> Kromcrush tier -> King Gordok. The skeleton had Slip'kik before Fengus, which is a walk back through the
--     courtyard door (its navmesh path goes through the hall).
--   * Keys: Gordok Courtyard Key 18266 (Fengus's Chest 179516, 100 %; Guard Fengus 1.2 %) and Gordok Inner Door Key 18268 (Guard Mol'dar,
--     100 %). The bots' mesh walks through closed doors; a player commander needs `.additem 18266` and `.additem 18268`.
--   * cc_priority 11448,11444,11445 (Warlock's Summon Doomguard 22865 / Curse of Tongues, Mage-Lord's Polymorph 13323 / Flamestrike 16102,
--     Captain's Fear 12096); no_cc 13036,13160 (beasts: a Polymorph does not land, and the pack CC must not pick them). No fleer in the
--     wing (no EventAI flee, command 47), no trash healer, so kill_first is empty; the guards enrage at 50 % and call for help 50 yd,
--     Kromcrush summons two Gordok Reavers 11450 at 50 %.
--   * Two packs stand inside a boss's 20 yd detection: leg 1 group 8 (19.5 yd from Fengus) and leg 2 group 9 (18.2 yd from Slip'kik); the
--     boss joins that fight and is dead for his own boss step. The 32 patrol blocks are the spawns' own creature_movement paths (estimated
--     periods); the Wandering Eye of Kilrogg 14386 (2 patrols) is only ever a patrol block, and the solo patrolling Brute 134805 is a pull
--     of its own (leg 0 group 1, 22 points, 379 yd).
-- Fix 20261024060600 (r28c leg 0): leg 0 pull group 2 carries four approach stands down the g1-g2 corridor on the mesh floor
-- (319.73 10.93 -8.64, 363.73 10.93 -25.28, 411.73 10.93 -24.85, 456.27 10.93 -3.82). The exporter's 50 yd cut of the g1-g2 hop
-- joined the straight corners 311.73/456.27 at z -3.8 while the corridor floor drops to z -25, so 361.73/411.73 floated ~21 yd
-- above the mesh and the r28c run stopped at step 6 with no walkable path from its island at 311.7 10.9. Every hop plans
-- complete at 47-49 yd with its point on the mesh; the last stands 48 yd from the pack (run leader sees 60). No walk through
-- unpulled packs by centre and radius (nearest g16 17.3, g18 13.5 yd); the final stand keeps 38+ yd ground clearance to later spawns.
-- Plan check (turtle-agent plan-check, map 429 mmaps): ok, 242 points on the navmesh, 34 warnings (33 estimated patrol periods and the
-- leg-1 group 9 detour: a gallery pack 37 yd away but 105 yd by way), 56 run-leader hop notices.
-- Fix 20261024150900 (r31 leg 2): Slip'kik (14323) and his guard pack (2 x 11448 at 577-585 545-549) are pulled right after
-- the swarmers, before the north-loop walk: the old step-115 move (552.8 563.5) stood 15-17 yd off Slip'kik's spawn on his 465 yd
-- patrol loop, so the tank engaged him on arrival and then picked 11448/11450 off the healer (28 and 36 yd) and wiped. Stand
-- 545 560 for Slip'kik (17.7 yd off him, 34+ yd off the guard pack) and stand 575 565 for the guard pack (16-22 yd off it,
-- 22+ yd off the boss); both stands on the mesh floor (z -25.3) with complete paths from the swarmers and on to group 6.
-- The old group 8 at the leg's end is gone; the boss step takes Slip'kik already dead. No other leg-2 move walks into an
-- unpulled pack (every other move keeps 25+ yd centre distance on its floor); legs 0, 1, 3, 4 unchanged.
-- 5 legs, 149 steps (19 + 28 + 10 + 10 + 4 pulls, 74 moves, the 5 bosses), 155 rows.

-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- dire_maul_north: map 429, 5 legs, 149 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'dire_maul_north';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'dire_maul_north';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'dire_maul_north';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('dire_maul_north', 429, '11448,11444,11445', '', '', 'turtle-agent 107dead dungeons/dire_maul_north.json', 'plan v1, draft (not mapped, not live-checked)');
INSERT INTO `ai_dungeon_leg` (`plan`, `leg`, `name`, `comment`) VALUES
('dire_maul_north', 0, 'guard_mol_dar', ''),
('dire_maul_north', 1, 'guard_fengus', ''),
('dire_maul_north', 2, 'guard_slip_kik', ''),
('dire_maul_north', 3, 'captain_kromcrush', ''),
('dire_maul_north', 4, 'king_gordok', '');
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('dire_maul_north', 0, 0, 'move', 0, 293.43, 22.677, -3.833, 0, 0, 0, 0, 0, '', 'to'),
('dire_maul_north', 0, 1, 'pull', 0, 290.41, 34.9, -3.5, 13.6, 0, 0, 0, 0, '11441,11444,13036', 'pull group 0'),
('dire_maul_north', 0, 2, 'pull', 0, 301.62, 48.94, -3.89, 5, 0, 0, 0, 0, '11441', 'pull group 1'),
('dire_maul_north', 0, 3, 'move', 0, 319.73, 10.93, -8.64, 0, 0, 0, 0, 0, '', 'pull group 2 approach'),
('dire_maul_north', 0, 4, 'move', 0, 363.73, 10.93, -25.28, 0, 0, 0, 0, 0, '', 'pull group 2 approach'),
('dire_maul_north', 0, 5, 'move', 0, 411.73, 10.93, -24.85, 0, 0, 0, 0, 0, '', 'pull group 2 approach'),
('dire_maul_north', 0, 6, 'move', 0, 456.27, 10.93, -3.82, 0, 0, 0, 0, 0, '', 'pull group 2 approach'),
('dire_maul_north', 0, 7, 'pull', 0, 470.5, 56.98, -3.83, 11.54, 0, 0, 0, 0, '11441,11444', 'pull group 2'),
('dire_maul_north', 0, 8, 'pull', 0, 415.75, -45.06, -23.32, 5.68, 0, 0, 0, 0, '13036', 'pull group 3'),
('dire_maul_north', 0, 9, 'pull', 0, 289.29, -75.34, -3.8, 5, 0, 0, 0, 0, '11441', 'pull group 4'),
('dire_maul_north', 0, 10, 'pull', 0, 297.37, -66.74, -3.8, 5, 0, 0, 0, 0, '11441', 'pull group 5'),
('dire_maul_north', 0, 11, 'move', 0, 296.53326, -55.46678, -3.818124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 12, 'pull', 0, 310.69, -12.72, -3.8, 5, 0, 0, 0, 0, '11441', 'pull group 6'),
('dire_maul_north', 0, 13, 'move', 0, 308.48886, 37.23149, -3.876716, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 14, 'move', 0, 306.93326, 72.53331, -3.9181242, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 15, 'move', 0, 302.93326, 77.06664, -3.9181242, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 16, 'move', 0, 297.5999, 82.66664, -3.9181242, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 17, 'pull', 0, 256.46, 96.37, -2.48, 7.14, 0, 0, 0, 0, '11441,11444', 'pull group 7'),
('dire_maul_north', 0, 18, 'move', 0, 297.5999, 82.66664, -3.9181242, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 19, 'move', 0, 302.93326, 77.06664, -3.9181242, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 20, 'move', 0, 306.93326, 72.53331, -3.9181242, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 21, 'move', 0, 310.717, 22.676754, -3.8346589, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 22, 'move', 0, 311.46658, 12.799996, -3.818124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 23, 'move', 0, 311.73325, 10.933331, -3.818124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 24, 'move', 0, 340.7999, -4.0001297, -20.818125, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 25, 'move', 0, 343.73325, -6.400129, -24.418125, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 26, 'pull', 0, 346.99, -39.71, -25.53, 9.57, 0, 0, 0, 0, '13036', 'pull group 8'),
('dire_maul_north', 0, 27, 'pull', 0, 341.01, -67.3, -25.53, 12.45, 0, 0, 0, 0, '13036', 'pull group 9'),
('dire_maul_north', 0, 28, 'pull', 0, 372.34, -39.18, -24.9, 5, 0, 0, 0, 0, '13036', 'pull group 10'),
('dire_maul_north', 0, 29, 'pull', 0, 382.62, -56.59, -23.61, 9.26, 0, 0, 0, 0, '13036', 'pull group 11'),
('dire_maul_north', 0, 30, 'move', 0, 421.33322, -54.13345, -23.918125, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 31, 'move', 0, 431.99988, -52.533447, -14.018125, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 32, 'move', 0, 433.06656, -52.800117, -13.018125, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 33, 'move', 0, 445.33322, -58.400116, -6.618124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 34, 'pull', 0, 469.47, -65.13, -3.89, 20.94, 0, 0, 0, 0, '13160', 'pull group 12'),
('dire_maul_north', 0, 35, 'pull', 0, 461.95, -107.5, -3.89, 20.27, 0, 0, 0, 0, '13160', 'pull group 13'),
('dire_maul_north', 0, 36, 'pull', 0, 411.5, -106.95, -3.89, 21.87, 0, 0, 0, 0, '13160', 'pull group 14'),
('dire_maul_north', 0, 37, 'pull', 0, 353.16, -107.77, -3.8, 5.69, 0, 0, 0, 0, '11441,11444', 'pull group 15'),
('dire_maul_north', 0, 38, 'move', 0, 315.19992, -91.466774, -3.818124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 39, 'move', 0, 314.93326, -90.93344, -3.818124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 40, 'move', 0, 312.83832, -40.97735, -3.818124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 41, 'move', 0, 311.46658, -8.266795, -3.818124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 42, 'move', 0, 311.46658, -3.7334628, -3.818124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 43, 'pull', 0, 365.8, 27.11, -25.25, 10.86, 0, 0, 0, 0, '11441,11444', 'pull group 16'),
('dire_maul_north', 0, 44, 'move', 0, 366.93323, 21.333328, -24.518124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 45, 'move', 0, 368.26657, 21.333328, -25.718124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 46, 'move', 0, 395.99988, 47.466652, -24.018124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 47, 'pull', 0, 421.59, 74.18, -23.81, 13.84, 0, 0, 0, 0, '11441,13036', 'pull group 17'),
('dire_maul_north', 0, 48, 'move', 0, 417.86655, 55.466652, -23.318125, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 0, 49, 'pull', 0, 416.3, 22.83, -25, 10.74, 0, 0, 0, 0, '13036', 'pull group 18'),
('dire_maul_north', 0, 50, 'boss', 14326, 396.463, -8.599, -24.932, 0, 0, 0, 0, 0, '', 'boss'),
('dire_maul_north', 1, 0, 'move', 0, 426.66653, -0.00013065338, -22.818125, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 1, 1, 'move', 0, 456.26654, 10.933331, -3.818124, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 1, 2, 'move', 0, 466.1184, 59.952385, -3.5477984, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 1, 3, 'move', 0, 467.19986, 65.33331, -3.518124, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 1, 4, 'move', 0, 467.99988, 69.866646, -3.818124, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 1, 5, 'move', 0, 447.93137, 115.65071, -2.7812047, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 1, 6, 'move', 0, 439.642, 134.562, -2.499, 0, 0, 0, 0, 0, '', 'to'),
('dire_maul_north', 1, 7, 'pull', 0, 444.86, 134.3, -2.5, 6.22, 0, 0, 0, 0, '11441', 'pull group 0'),
('dire_maul_north', 1, 8, 'move', 0, 458.1332, 152.79996, 3.881876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 9, 'move', 0, 460.79987, 155.99995, 3.881876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 10, 'pull', 0, 483.84, 175.7, 2.94, 6.31, 0, 0, 0, 0, '11441,11444', 'pull group 1'),
('dire_maul_north', 1, 11, 'pull', 0, 478.28, 192.7, 2.94, 7.51, 0, 0, 0, 0, '11441,11444', 'pull group 2'),
('dire_maul_north', 1, 12, 'pull', 0, 476.12, 262.29, 4.32, 19.52, 0, 0, 0, 0, '13160', 'pull group 3'),
('dire_maul_north', 1, 13, 'pull', 0, 476.88, 299.92, 2.85, 18.36, 0, 0, 0, 0, '13160', 'pull group 4'),
('dire_maul_north', 1, 14, 'pull', 0, 477.83, 340.6, 2.85, 8.84, 0, 0, 0, 0, '13036,11444', 'pull group 5'),
('dire_maul_north', 1, 15, 'pull', 0, 444.62, 353.23, 3.38, 13.21, 0, 0, 0, 0, '11441,13036', 'pull group 6'),
('dire_maul_north', 1, 16, 'pull', 0, 405.93, 333.93, 2.87, 8.86, 0, 0, 0, 0, '11441,13036', 'pull group 7'),
('dire_maul_north', 1, 17, 'pull', 0, 442.33, 303.07, 11.2, 16.38, 0, 0, 0, 0, '11441,13036', 'pull group 8'),
('dire_maul_north', 1, 18, 'move', 0, 422.1332, 281.0666, 11.081876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 19, 'move', 0, 420.5332, 277.33325, 11.081876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 20, 'move', 0, 419.46655, 273.86658, 11.081876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 21, 'pull', 0, 412, 245.51, 11.21, 6, 0, 0, 0, 0, '11441', 'pull group 9'),
('dire_maul_north', 1, 22, 'move', 0, 383.99988, 255.99992, 11.481876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 23, 'pull', 0, 357.22, 267.02, 11.2, 6, 0, 0, 0, 0, '11441', 'pull group 10'),
('dire_maul_north', 1, 24, 'pull', 0, 350.72, 306.47, 11.21, 5, 0, 0, 0, 0, '11441', 'pull group 11'),
('dire_maul_north', 1, 25, 'pull', 0, 327.38, 299.49, 11.26, 12.1, 0, 0, 0, 0, '11444,11441', 'pull group 12'),
('dire_maul_north', 1, 26, 'move', 0, 336.2614, 250.2916, 12.081324, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 27, 'pull', 0, 345.73, 212.58, 11.29, 11.61, 0, 0, 0, 0, '11441,11444', 'pull group 13'),
('dire_maul_north', 1, 28, 'move', 0, 343.46658, 211.46661, 13.181876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 29, 'move', 0, 343.46658, 209.0666, 11.681876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 30, 'move', 0, 346.3999, 202.93327, 11.281877, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 31, 'move', 0, 348.7999, 202.66661, 11.281877, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 32, 'pull', 0, 419.91, 205.62, 11.27, 9.37, 0, 0, 0, 0, '11441,11444', 'pull group 14'),
('dire_maul_north', 1, 33, 'pull', 0, 406.28, 176.53, 2.87, 8.6, 0, 0, 0, 0, '11441,13036', 'pull group 15'),
('dire_maul_north', 1, 34, 'pull', 0, 383.35, 138.78, -1.45, 7.28, 0, 0, 0, 0, '11441,11444', 'pull group 16'),
('dire_maul_north', 1, 35, 'pull', 0, 392.97, 105.68, -3.87, 5.14, 0, 0, 0, 0, '11441,13036', 'pull group 17'),
('dire_maul_north', 1, 36, 'pull', 0, 350.82, 119.49, -3.89, 5, 0, 0, 0, 0, '11441,13036', 'pull group 18'),
('dire_maul_north', 1, 37, 'pull', 0, 327.26, 130.01, -2.93, 8.18, 0, 0, 0, 0, '11441,11444', 'pull group 19'),
('dire_maul_north', 1, 38, 'pull', 0, 310.75, 180.33, 2.87, 11.97, 0, 0, 0, 0, '11441,13036', 'pull group 20'),
('dire_maul_north', 1, 39, 'pull', 0, 273.01, 195.12, 2.94, 5, 0, 0, 0, 0, '11444', 'pull group 21'),
('dire_maul_north', 1, 40, 'pull', 0, 280.53, 204.35, 2.94, 5, 0, 0, 0, 0, '11444', 'pull group 22'),
('dire_maul_north', 1, 41, 'move', 0, 282.39993, 211.46661, 2.881876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 42, 'move', 0, 282.6666, 213.33327, 2.881876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 43, 'move', 0, 282.6666, 228.79994, 2.881876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 1, 44, 'pull', 0, 281.88, 257.99, 2.94, 5, 0, 0, 0, 0, '11441', 'pull group 23'),
('dire_maul_north', 1, 45, 'pull', 0, 298.7, 273.23, 2.87, 11.27, 0, 0, 0, 0, '11441,13036', 'pull group 24'),
('dire_maul_north', 1, 46, 'pull', 0, 288.66, 317.85, 2.94, 5, 0, 0, 0, 0, '11444', 'pull group 25'),
('dire_maul_north', 1, 47, 'pull', 0, 279.31, 325.27, 2.94, 5, 0, 0, 0, 0, '11444', 'pull group 26'),
('dire_maul_north', 1, 48, 'pull', 0, 326.77, 355.72, 2.87, 7.57, 0, 0, 0, 0, '11441,13036', 'pull group 27'),
('dire_maul_north', 1, 49, 'boss', 14321, 384.67, 341.272, 2.864, 0, 0, 0, 0, 0, '', 'boss'),
('dire_maul_north', 2, 0, 'move', 0, 384.2788, 391.1218, -0.97093177, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 2, 1, 'move', 0, 383.99988, 426.66653, -3.7181242, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 2, 2, 'move', 0, 383.1999, 454.9332, -7.218124, 0, 0, 0, 0, 0, '', 'path'),
('dire_maul_north', 2, 3, 'move', 0, 379.625, 466.833, -7.149, 0, 0, 0, 0, 0, '', 'to'),
('dire_maul_north', 2, 4, 'pull', 0, 383.22, 465.33, -7.19, 5, 0, 0, 0, 0, '11448,11450', 'pull group 0'),
('dire_maul_north', 2, 5, 'move', 0, 383.99988, 469.3332, -7.218124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 2, 6, 'move', 0, 399.73322, 501.86652, -12.718124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 2, 7, 'pull', 0, 417.99, 533.63, -18.34, 5, 0, 0, 0, 0, '11450', 'pull group 1'),
('dire_maul_north', 2, 8, 'pull', 0, 425.41, 543.03, -18.26, 5, 0, 0, 0, 0, '11448', 'pull group 2'),
('dire_maul_north', 2, 9, 'pull', 0, 491.95, 543.17, -25.39, 19.1, 0, 0, 0, 0, '13160', 'pull group 3'),
('dire_maul_north', 2, 10, 'move', 0, 545, 560, -25.3, 0, 0, 0, 0, 0, '', 'pull group 4 approach'),
('dire_maul_north', 2, 11, 'pull', 0, 559.22, 549.43, -25.4, 5, 0, 0, 0, 0, '14323', 'pull group 4'),
('dire_maul_north', 2, 12, 'move', 0, 575, 565, -25.3, 0, 0, 0, 0, 0, '', 'pull group 5 approach'),
('dire_maul_north', 2, 13, 'pull', 0, 581.14, 546.86, -25.32, 5.35, 0, 0, 0, 0, '11448', 'pull group 5'),
('dire_maul_north', 2, 14, 'pull', 0, 579.37, 590.73, -25.32, 5, 0, 0, 0, 0, '11448', 'pull group 6'),
('dire_maul_north', 2, 15, 'pull', 0, 572.4, 599.51, -25.32, 5, 0, 0, 0, 0, '11448', 'pull group 7'),
('dire_maul_north', 2, 16, 'pull', 0, 526.2, 591.53, -25.4, 13.09, 0, 0, 0, 0, '11450', 'pull group 8'),
('dire_maul_north', 2, 17, 'pull', 0, 482.4, 603.94, -25.32, 5.14, 0, 0, 0, 0, '11448', 'pull group 9'),
('dire_maul_north', 2, 18, 'move', 0, 531.98456, 597.50824, -25.315992, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 2, 19, 'move', 0, 533.3333, 597.3333, -25.318125, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 2, 20, 'boss', 14323, 559.22, 549.432, -25.402, 0, 0, 0, 0, 0, '', 'boss'),
('dire_maul_north', 3, 0, 'move', 0, 548.933, 575.927, -4.671, 0, 0, 0, 0, 0, '', 'to'),
('dire_maul_north', 3, 1, 'pull', 0, 544.78, 578.2, -4.71, 5.73, 0, 0, 0, 0, '11448,11450', 'pull group 0'),
('dire_maul_north', 3, 2, 'pull', 0, 581.5, 596.28, -4.67, 5, 0, 0, 0, 0, '11448', 'pull group 1'),
('dire_maul_north', 3, 3, 'pull', 0, 593.01, 596.73, -4.67, 5, 0, 0, 0, 0, '11448', 'pull group 2'),
('dire_maul_north', 3, 4, 'pull', 0, 592.99, 565.4, -4.76, 5, 0, 0, 0, 0, '11450', 'pull group 3'),
('dire_maul_north', 3, 5, 'pull', 0, 580.36, 565.47, -4.67, 5, 0, 0, 0, 0, '11448', 'pull group 4'),
('dire_maul_north', 3, 6, 'move', 0, 581.3333, 560, -2.318124, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 3, 7, 'move', 0, 581.3333, 543.19995, 7.5818763, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 3, 8, 'move', 0, 580.8, 541.06665, 8.481876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 3, 9, 'move', 0, 557.3333, 539.73334, 19.481876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 3, 10, 'move', 0, 541.06665, 540.8, 29.481876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 3, 11, 'pull', 0, 520.66, 558.96, 27.96, 5.18, 0, 0, 0, 0, '11448,11450', 'pull group 5'),
('dire_maul_north', 3, 12, 'pull', 0, 485.35, 559.46, 27.91, 5.67, 0, 0, 0, 0, '11450', 'pull group 6'),
('dire_maul_north', 3, 13, 'pull', 0, 477, 565.55, 28, 5, 0, 0, 0, 0, '11448', 'pull group 7'),
('dire_maul_north', 3, 14, 'pull', 0, 491.5, 524.79, 27.92, 7.4, 0, 0, 0, 0, '11450', 'pull group 8'),
('dire_maul_north', 3, 15, 'move', 0, 495.19986, 487.46652, 30.281876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 3, 16, 'move', 0, 499.99985, 485.86652, 29.481876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 3, 17, 'pull', 0, 563.23, 480.03, 29.49, 10.21, 0, 0, 0, 0, '11448,11450', 'pull group 9'),
('dire_maul_north', 3, 18, 'boss', 14325, 627.59, 481.721, 29.463, 0, 0, 0, 0, 0, '', 'boss'),
('dire_maul_north', 4, 0, 'move', 0, 667.484, 467.305, 29.539, 0, 0, 0, 0, 0, '', 'to'),
('dire_maul_north', 4, 1, 'pull', 0, 679.89, 458.83, 29.49, 19.02, 0, 0, 0, 0, '11445,13036', 'pull group 0'),
('dire_maul_north', 4, 2, 'pull', 0, 687.23, 507.2, 29.12, 7.87, 0, 0, 0, 0, '11445,11444', 'pull group 1'),
('dire_maul_north', 4, 3, 'pull', 0, 711.22, 480.7, 28.18, 20.54, 0, 0, 0, 0, '13036', 'pull group 2'),
('dire_maul_north', 4, 4, 'pull', 0, 736.71, 430.79, 28.24, 9.55, 0, 0, 0, 0, '11444,11445', 'pull group 3'),
('dire_maul_north', 4, 5, 'move', 0, 746.66656, 447.99988, 28.281876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 4, 6, 'move', 0, 761.8666, 475.19986, 28.281876, 0, 0, 0, 0, 0, '', 'approach path'),
('dire_maul_north', 4, 7, 'boss', 11501, 829.697, 481.328, 37.401, 0, 0, 0, 0, 0, '14324', 'boss');

-- Dungeon bosses of map 429 for this plan's boss steps (the commander telemetry's boss rank; see 20261021114931_world.sql): read from
-- creature_template (all five rank 1, faction 45, no instance bind, hence the table). Guard Fengus and Guard Slip'kik patrol (852 and 465 yd);
-- Cho'Rush the Observer 14324 is King Gordok's first entry in the boss step, not a boss of the table. The DELETE is scoped to these five
-- entries, not the map, so another plan's map-429 rows survive.
DELETE FROM `ai_dungeon_boss` WHERE `map` IN (429) AND `entry` IN (14326, 14321, 14323, 14325, 11501);
REPLACE INTO `ai_dungeon_boss` (`map`, `entry`, `name`) VALUES
(429, 14326, 'Guard Mol''dar'),
(429, 14321, 'Guard Fengus'),
(429, 14323, 'Guard Slip''kik'),
(429, 14325, 'Captain Kromcrush'),
(429, 11501, 'King Gordok');
