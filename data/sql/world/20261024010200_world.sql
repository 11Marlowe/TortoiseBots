-- Stormwind Vault (map 35): the plan for the run leader (.bot run), exported from turtle-agent 7c80a94
-- (dungeons/stormwind_vault.json, status 'draft (not mapped, not live-checked)') with the options of the header of 20261017130000:
--   turtle-agent plan-export --mmaps <map 35 mmaps> --allow-draft --gm-cross-direct --approach-hops --rev 7c80a94 \
--     --stamp 20261024010200 --out <dir> stormwind_vault
-- Stacks on 20261023220000. The generator draft this replaces was never merged or applied anywhere; this file is the first export of the plan,
-- and it already carries the census fixes (the plan was rebuilt before this one export, not after it). NO ROW IS HAND-SET: the plan file is
-- the source; the fixes over the draft are in ~/TortoiseBots-audit/PREP-STORMWIND-VAULT-2026-10-24.md ("Fixes applied"). In short:
--   * every pull rebuilt from the map-35 spawn census (137 spawns): the two giant draft groups (33 Runic Constructs/Grellkin in the mid-west
--     hall, 16 in the west hall) are 39 pulls of 1-6, the duplicate draft groups (0.1/0.2, 0.4/0.5, 5.2/5.3) are one pull each, and every
--     radius is the farthest own member + 0.5 yd (5.0-8.3), so no row reaches the next pack (0 joiners in the census).
--   * the Aszosh guard (60597 at -5.71 181.43 and 3.57 192.76, 10.6 and 5.2 yd from him) and the Black Bride guard (60604 at 65.03 116.12,
--     8.0 yd from her) are the bosses' first: the loader refuses a pull of a boss.first entry, so their draft pulls are gone.
--   * the plan row's kill_first 60602,60596,60601,60603: the Blast Wave Scorcher, the Arcane Explosion Construct, the Frost Nova Sorcerer and
--     the Drain Mana Manacrazed Grell (all cast through walls, CF_FORCE_CAST). cc_priority and cc_always stay empty: the only humanoid is the
--     60597 guard, whose spells are physical, so no pull depends on a Polymorph (the r21 lesson).
--   * the west-wing trash moved to the Tham'Grarr leg and Arc'tiras is the LAST leg (Tham'Grarr -> walk back through the cleared west hall ->
--     Arc'tiras), instead of the draft's Arc'tiras -> Tham'Grarr, which walked 450 yd back east through the bride hall and past the live west
--     hall packs. The route oracle's live length per leg is 332 / 109 / 484 / 533 / 824 / 388 yd (the draft: 760 / 756 / 246 / 444 / 547 / 455).
-- 6 legs, 100 steps (7 + 2 + 10 + 10 + 24 + 0 pulls, 41 moves, the 6 bosses), 107 rows. Boss order: Volkan Cruelblade 80851, Damian 80854,
-- Aszosh Grimflame 80853 (first 60597), Black Bride 80850 (first 60604), Tham'Grarr 80852, Arc'tiras 93107. Plan check (turtle-agent
-- plan-check, map 35 mmaps): ok, 140 points on the navmesh, 0 detour and 0 estimated-patrol warnings, 22 run-leader hop warnings (a pull or
-- boss 50-104 yd from the move row before it; the mapping walk's first items). No needs_walk point.
--
-- WHAT THE RUN LEADER CANNOT DO (issue #984: nothing is dropped silently). Not in these rows:
--   * no_cc 60596 60598 60599 60601 60602 60603 60604 93105 (elementals, undead, demons and beasts: no Polymorph lands) and the fleer list
--     (empty: no EventAI flee row exists for any map-35 spawn): there is no column. The plan row has cc_priority and kill_first.
--   * the rat trap GO 3000273 at -1.697 116.335 -40.0507 (stormwind_vault_rat_trap): a player within 5 yd is teleported to it, the two
--     Portcullis Small doors 3000275/3000276 close and 15 Vault Rats 93106 spawn and attack; the doors reopen when the rats are dead. No plan
--     point is within 5 yd of it, but the corridor is the lower floor's east-west link at x -6.8..4.1, y 116: a move routed through it fires
--     the trap. The Trap's rats are not plan rows either.
--   * the vault door 4000509 (opens when Black Bride/Tham'Grarr/Aszosh/Damian are dead; the instance script sets all four flags on the first
--     of them) and the ice-block obstruction 5001265/5001289 Arc'tiras removes on death: no door step is needed and none is exported.
--   * boss adds with no spawn row: Risen Lackey 10482 (Aszosh, spell 17618), Frigid Guardian 60612 (Arc'tiras at 60 %), Vault Rat 93106;
--     Arc'tiras' Cause Insanity on a random non-tank and Black Bride's Possess 17244 are the run leader's fight.
--   * the floor seams: the map joins the upper hall (z -27.5) to the lower hall (z -39..-40) with a 12-yd drop the mesh walks down without a
--     climb (plan-check's detour warning of the draft; the rebuilt order keeps one drop per leg), and the plan's stands are spawn rows. The
--     mapping walk measures the transitions before the first run.
--   * the entrance (trigger 107, portal map 0 -8667.56 623.563 85.4054, level 58), level_band 60-60 (spawns are 61, bosses 62) and
--     hire_composition: the run leader starts inside the instance and leads the bound party as it is.

-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- stormwind_vault: map 35, 6 legs, 100 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'stormwind_vault';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'stormwind_vault';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'stormwind_vault';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('stormwind_vault', 35, '', '', '60602,60596,60601,60603', 'turtle-agent 7c80a94 dungeons/stormwind_vault.json', 'plan v1, draft (not mapped, not live-checked)');
INSERT INTO `ai_dungeon_leg` (`plan`, `leg`, `name`, `comment`) VALUES
('stormwind_vault', 0, 'volkan_cruelblade', ''),
('stormwind_vault', 1, 'damian', ''),
('stormwind_vault', 2, 'aszosh_grimflame', ''),
('stormwind_vault', 3, 'black_bride', ''),
('stormwind_vault', 4, 'tham''grarr', ''),
('stormwind_vault', 5, 'arc''tiras', '');
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('stormwind_vault', 0, 0, 'move', 0, 1.8631, 58.6976, -27.5507, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_vault', 0, 1, 'pull', 0, -1.28, 61.74, -27.55, 6.1, 0, 0, 0, 0, '60597,60596', 'pull group 0'),
('stormwind_vault', 0, 2, 'pull', 0, -16.6, 71.72, -29.12, 5.4, 0, 0, 0, 0, '60597', 'pull group 1'),
('stormwind_vault', 0, 3, 'pull', 0, -5.61, 82, -27.55, 5, 0, 0, 0, 0, '60597', 'pull group 2'),
('stormwind_vault', 0, 4, 'pull', 0, 6.36, 79.12, -27.55, 5, 0, 0, 0, 0, '60597', 'pull group 3'),
('stormwind_vault', 0, 5, 'pull', 0, -1.31, 99.38, -27.55, 5, 0, 0, 0, 0, '60597', 'pull group 4'),
('stormwind_vault', 0, 6, 'pull', 0, -1.77, 94.39, -39.82, 7.6, 0, 0, 0, 0, '60599,60597', 'pull group 5'),
('stormwind_vault', 0, 7, 'pull', 0, -12.62, 81.41, -39.13, 5, 0, 0, 0, 0, '60597,60596', 'pull group 6'),
('stormwind_vault', 0, 8, 'boss', 80851, -12.6354, 52.1519, -39.134, 0, 0, 0, 0, 0, '', 'boss'),
('stormwind_vault', 1, 0, 'move', 0, -21.1887, 94.05, -40.0507, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_vault', 1, 1, 'pull', 0, -19.16, 95.18, -39.59, 5, 0, 0, 0, 0, '93105,60597', 'pull group 0'),
('stormwind_vault', 1, 2, 'pull', 0, -20.84, 116.07, -40.05, 5, 0, 0, 0, 0, '60602', 'pull group 1'),
('stormwind_vault', 1, 3, 'boss', 80854, -12.7104, 150.171, -40.384, 0, 0, 0, 0, 0, '', 'boss'),
('stormwind_vault', 2, 0, 'move', 0, 1.7437, 128.581, -27.5507, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_vault', 2, 1, 'pull', 0, -0.97, 133.3, -28.06, 7.4, 0, 0, 0, 0, '60598,60597,60599', 'pull group 0'),
('stormwind_vault', 2, 2, 'pull', 0, -1.32, 114.87, -27.55, 5, 0, 0, 0, 0, '60597', 'pull group 1'),
('stormwind_vault', 2, 3, 'pull', 0, 8.3, 158.33, -28.8, 5, 0, 0, 0, 0, '60599,60598', 'pull group 2'),
('stormwind_vault', 2, 4, 'pull', 0, -9.75, 159.12, -28.8, 5, 0, 0, 0, 0, '60599,60598', 'pull group 3'),
('stormwind_vault', 2, 5, 'pull', 0, -25.16, 146.53, -27.55, 6, 0, 0, 0, 0, '60599,60598', 'pull group 4'),
('stormwind_vault', 2, 6, 'pull', 0, -40.51, 151.03, -27.55, 7.7, 0, 0, 0, 0, '60599,60598,60597', 'pull group 5'),
('stormwind_vault', 2, 7, 'move', 0, -37.60012, 150.66663, -27.54564, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_vault', 2, 8, 'pull', 0, 25.64, 150.98, -27.55, 6.7, 0, 0, 0, 0, '60599,60598', 'pull group 6'),
('stormwind_vault', 2, 9, 'pull', 0, 39.93, 154.74, -27.55, 5, 0, 0, 0, 0, '60598', 'pull group 7'),
('stormwind_vault', 2, 10, 'pull', 0, 39.7, 144.4, -27.55, 5, 0, 0, 0, 0, '60599', 'pull group 8'),
('stormwind_vault', 2, 11, 'pull', 0, 3.63, 170.91, -27.55, 5, 0, 0, 0, 0, '60597', 'pull group 9'),
('stormwind_vault', 2, 12, 'boss', 80853, -1.361, 191.111, -27.5507, 0, 0, 0, 0, 0, '60597', 'boss'),
('stormwind_vault', 3, 0, 'move', 0, -0.65148354, 141.11603, -27.541494, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 1, 'move', 0, -0.53346384, 132.79996, -27.54564, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 2, 'move', 0, 1.333333, 95.46664, -27.445642, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 3, 'move', 0, 2.1333327, 75.99998, -26.445642, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 4, 'move', 0, 3.4666657, 74.93331, -27.54564, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 5, 'move', 0, 24.799994, 74.93331, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 6, 'move', 0, 27.999992, 75.466644, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 7, 'move', 0, 28.533325, 77.06664, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 8, 'move', 0, 28.533325, 99.46664, -37.845642, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 9, 'move', 0, 27.999992, 112.5333, -40.045643, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 10, 'move', 0, 26.666658, 112.799965, -40.045643, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 3, 11, 'move', 0, 9.0205, 113.749, -40.0507, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_vault', 3, 12, 'pull', 0, 7.94, 117, -39.83, 5, 0, 0, 0, 0, '93105', 'pull group 0'),
('stormwind_vault', 3, 13, 'pull', 0, 32.28, 129.15, -40.05, 5, 0, 0, 0, 0, '60604,93105', 'pull group 1'),
('stormwind_vault', 3, 14, 'pull', 0, 49.03, 116.55, -40.05, 7.3, 0, 0, 0, 0, '93105,60604', 'pull group 2'),
('stormwind_vault', 3, 15, 'pull', 0, 32.15, 104.83, -39.93, 5, 0, 0, 0, 0, '93105,60604', 'pull group 3'),
('stormwind_vault', 3, 16, 'pull', 0, 32.23, 90.59, -34.65, 5, 0, 0, 0, 0, '93105,60604', 'pull group 4'),
('stormwind_vault', 3, 17, 'pull', 0, 22.83, 70.98, -31.72, 5, 0, 0, 0, 0, '93105', 'pull group 5'),
('stormwind_vault', 3, 18, 'pull', 0, 28.73, 57.94, -31.72, 5, 0, 0, 0, 0, '60604', 'pull group 6'),
('stormwind_vault', 3, 19, 'pull', 0, 47.49, 69.82, -31.72, 6, 0, 0, 0, 0, '93105,60604', 'pull group 7'),
('stormwind_vault', 3, 20, 'pull', 0, 56.25, 75.73, -31.72, 5, 0, 0, 0, 0, '60604', 'pull group 8'),
('stormwind_vault', 3, 21, 'pull', 0, 70.82, 70.86, -31.72, 7.8, 0, 0, 0, 0, '93105,60604', 'pull group 9'),
('stormwind_vault', 3, 22, 'boss', 80850, 73.0208, 116.193, -40.0507, 0, 0, 0, 0, 0, '60604', 'boss'),
('stormwind_vault', 4, 0, 'move', 0, 48.799988, 115.46664, -40.045643, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 1, 'move', 0, 47.199986, 115.19997, -40.045643, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 2, 'move', 0, 37.333324, 112.799965, -40.045643, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 3, 'move', 0, 35.466656, 111.46664, -40.045643, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 4, 'move', 0, 28.533325, 77.06664, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 5, 'move', 0, 27.999992, 75.466644, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 6, 'move', 0, 24.799994, 74.93331, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 7, 'move', 0, -25.199335, 74.674255, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 8, 'move', 0, -26.66679, 74.66665, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 9, 'move', 0, -33.60012, 75.733315, -32.14564, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 10, 'move', 0, -33.866787, 77.33331, -32.545643, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 11, 'move', 0, -31.46679, 96.5333, -32.245644, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 4, 12, 'move', 0, -31.2861, 100.789, -32.634, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_vault', 4, 13, 'pull', 0, -30.33, 101.25, -32.63, 5, 0, 0, 0, 0, '60596,60603', 'pull group 0'),
('stormwind_vault', 4, 14, 'pull', 0, -38.22, 93.82, -32.63, 5, 0, 0, 0, 0, '60603', 'pull group 1'),
('stormwind_vault', 4, 15, 'pull', 0, -30.61, 86.22, -32.63, 5, 0, 0, 0, 0, '60596,60603', 'pull group 2'),
('stormwind_vault', 4, 16, 'pull', 0, -39.87, 81.34, -32.63, 5, 0, 0, 0, 0, '60603,60596', 'pull group 3'),
('stormwind_vault', 4, 17, 'pull', 0, -30.36, 73.67, -31.72, 5, 0, 0, 0, 0, '60603,60596', 'pull group 4'),
('stormwind_vault', 4, 18, 'pull', 0, -34.91, 62.53, -32.33, 6.6, 0, 0, 0, 0, '60601', 'pull group 5'),
('stormwind_vault', 4, 19, 'pull', 0, -35.87, 44.91, -32.63, 7.3, 0, 0, 0, 0, '60601', 'pull group 6'),
('stormwind_vault', 4, 20, 'pull', 0, -51.48, 40.22, -32.63, 5, 0, 0, 0, 0, '60602', 'pull group 7'),
('stormwind_vault', 4, 21, 'pull', 0, -62.02, 45.31, -32.63, 5, 0, 0, 0, 0, '60602', 'pull group 8'),
('stormwind_vault', 4, 22, 'pull', 0, -54.6, 56.59, -32.63, 7, 0, 0, 0, 0, '60602', 'pull group 9'),
('stormwind_vault', 4, 23, 'pull', 0, -48.53, 70.46, -31.72, 5.9, 0, 0, 0, 0, '60603,60596,60601', 'pull group 10'),
('stormwind_vault', 4, 24, 'pull', 0, -70.91, 67.63, -31.72, 5, 0, 0, 0, 0, '60602', 'pull group 11'),
('stormwind_vault', 4, 25, 'pull', 0, -82.2, 61.97, -32.97, 5, 0, 0, 0, 0, '60601,60602', 'pull group 12'),
('stormwind_vault', 4, 26, 'pull', 0, -84.85, 50.8, -31.72, 5, 0, 0, 0, 0, '60601', 'pull group 13'),
('stormwind_vault', 4, 27, 'pull', 0, -91.18, 32.45, -31.72, 8.2, 0, 0, 0, 0, '60603,60601,60596,60602', 'pull group 14'),
('stormwind_vault', 4, 28, 'pull', 0, -95.14, 50.13, -31.72, 5, 0, 0, 0, 0, '60602', 'pull group 15'),
('stormwind_vault', 4, 29, 'pull', 0, -100.09, 62.68, -32.97, 5, 0, 0, 0, 0, '60601,60603,60602', 'pull group 16'),
('stormwind_vault', 4, 30, 'pull', 0, -99.8, 80.34, -32.97, 5, 0, 0, 0, 0, '60602,60603,60601', 'pull group 17'),
('stormwind_vault', 4, 31, 'pull', 0, -89.89, 91.24, -31.72, 5, 0, 0, 0, 0, '60603,60602', 'pull group 18'),
('stormwind_vault', 4, 32, 'pull', 0, -77.47, 78.88, -32.55, 8.1, 0, 0, 0, 0, '60601,60602', 'pull group 19'),
('stormwind_vault', 4, 33, 'pull', 0, -56.88, 87.57, -32.63, 5, 0, 0, 0, 0, '60602,60601', 'pull group 20'),
('stormwind_vault', 4, 34, 'pull', 0, -56.72, 101.96, -32.63, 5.7, 0, 0, 0, 0, '60596,60603', 'pull group 21'),
('stormwind_vault', 4, 35, 'pull', 0, -85.19, 112.16, -31.72, 5, 0, 0, 0, 0, '60603', 'pull group 22'),
('stormwind_vault', 4, 36, 'pull', 0, -95.01, 108.79, -31.72, 5.2, 0, 0, 0, 0, '60602,60603', 'pull group 23'),
('stormwind_vault', 4, 37, 'move', 0, -96.00011, 108.26664, -31.245642, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_vault', 4, 38, 'move', 0, -91.466774, 90.39997, -31.645641, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_vault', 4, 39, 'move', 0, -91.2001, 88.79997, -31.645641, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_vault', 4, 40, 'move', 0, -91.2001, 87.199974, -31.645641, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_vault', 4, 41, 'move', 0, -93.333435, 84.53331, -32.94564, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_vault', 4, 42, 'move', 0, -103.46677, 74.39998, -32.94564, 0, 0, 0, 0, 0, '', 'approach path'),
('stormwind_vault', 4, 43, 'boss', 80852, -131.321, 71.3321, -31.6499, 0, 0, 0, 0, 0, '', 'boss'),
('stormwind_vault', 5, 0, 'move', 0, -106.66676, 72.26665, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 5, 1, 'move', 0, -103.46677, 74.39998, -32.94564, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 5, 2, 'move', 0, -93.333435, 84.53331, -32.94564, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 5, 3, 'move', 0, -91.2001, 87.199974, -31.645641, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 5, 4, 'move', 0, -85.600105, 107.99997, -30.445642, 0, 0, 0, 0, 0, '', 'path'),
('stormwind_vault', 5, 5, 'move', 0, -85.1901, 112.164, -31.7173, 0, 0, 0, 0, 0, '', 'to'),
('stormwind_vault', 5, 6, 'boss', 93107, -100.115, 150.499, -40.3813, 0, 0, 0, 0, 0, '', 'boss');

-- Dungeon bosses of map 35 for this plan's boss steps only (the commander telemetry's boss rank; see 20261021114931_world.sql): the six boss
-- steps' entries, read from Turtle's own world data (tortoise-wow sql/base creature_template: all six are rank 1 there, so the telemetry would
-- call them 'e'). The DELETE is scoped to these entries, not the map, so another plan's map-35 rows survive.
DELETE FROM `ai_dungeon_boss` WHERE `map` IN (35) AND `entry` IN (80851, 80854, 80853, 80850, 80852, 93107);
REPLACE INTO `ai_dungeon_boss` (`map`, `entry`, `name`) VALUES
(35, 80851, 'Volkan Cruelblade'),
(35, 80854, 'Damian'),
(35, 80853, 'Aszosh Grimflame'),
(35, 80850, 'Black Bride'),
(35, 80852, 'Tham''Grarr'),
(35, 93107, 'Arc''tiras');
