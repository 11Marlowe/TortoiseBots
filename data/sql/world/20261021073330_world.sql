-- Zul'Farrak (map 209), the run leader's plan extended past the stairs event: the courtyard, Witch Doctor Zum'rah, the pyramid stairs
-- up to Nekrum Gutchewer and Shadowpriest Sezz'ziz, then Weegli's door (hold, gossip, End Door wait) and Chief Ukorz Sandscalp
-- (Gahz'rilla and Bly left out, see below). Replaces the rows of 20261021005900_world.sql (that file and the older ones stay as
-- applied; legs 0-13 here are byte-identical to its 122 steps). Exported from turtle-agent db9f044 (origin/dev, with the gossip step
-- of turtle-agent #279) with
--   turtle-agent plan-export --drop-companions-loot --skip-legs bly --mmaps <map 209 mmaps of 2026-10-20> --rev db9f044 --stamp 20261021073330 zulfarrak
-- 16 legs, 133 steps (20261021005900: 14 legs, 122 steps, ending at the stairs foot).
--
-- NEW IN THIS MIGRATION (none of it has run under the run leader yet):
--   * leg 14 'weegli_door' (4 steps): a move to the stairs foot, a hold on 1877.5 1199.6 8.9 until instance data 1 reads 8
--     (PYRAMID_KILLED_ALL_TROLLS, a script value: flag 8 without 32), 600 s, units 7787 7788 7789 8876 8877; then the gossip row
--     (Weegli Blastfuse 7607, menu option 1 via the order `cmd gossip`, the lead bot talks), then wait_object End Door 146084
--     active, 60 s.
--   * leg 15 'ukorz' (7 steps): five packs behind the End Door, then the boss Chief Ukorz Sandscalp 7267 (Ruuzlu 7797 as his add).
--   * Legs 0-13 are unchanged. The first run tests legs 0-8 of a changed route (71 steps, not the applied 86) as well as the later
--     legs; the earlier notes on them follow.
-- NEW IN THE ROWS (none of it has run under the run leader yet; the agent-led rounds 3-5 of 2026-10-15/17 ran these legs):
--   * legs 0-8: the plan's current route to Velratha (the applied antusul_a4 leg is gone; the velratha leg is split into
--     'velratha' and 'velratha_b', Velratha met on her strip by the gong, stand 1633.6 1130.7), with the pool-stairs approach
--     moves before the last two packs of 'velratha' (zf-full-19z16 STUCK #2). Their last points, where the party stands to
--     pull, are 1603 1043 9.8 (group 4: 44.0 yd or more from every member's spawn, 29.3 yd or more from the Sandcrawler
--     81587's patrol waypoints less its followers' 5.3 yd) and 1584 1088 9.1 (group 5: 24.1 yd or more from the four), so the
--     walk there does not pull the pack before the Polymorph (cold review 2026-10-21; the first export had 1582 1070, 10.8 yd
--     from the Witch Doctor, and 1593.6 1089.9, 16.6 yd from a Shadowhunter). Legs 0-8 are 71 steps, not the applied 86: the
--     route itself changed, so a first run of these rows tests legs 0-8 again as well as the new legs.
--   * leg 9 'courtyard' (7 packs) and leg 10 'zumrah': the walk into area trigger 962 is a move to its stand (1903 1014 11.8);
--     the server must list it: AiPlayerbot.BotAreaTriggers = 962 (TortoiseBots #1002), or Zum'rah (7271) stays friendly and the
--     boss step pauses after 60 s. The 31 Shallow Grave steps of the plan are NOT in the rows (companions_loot false cannot be
--     carried, so --drop-companions-loot leaves them out): Zum'rah raises the unopened graves (spell 10731), so his fight has the
--     round-2 adds (81-93 % of the hits on the party in zf-zum2). kill_first now begins with his Ward 7785.
--   * leg 11 'stairs': the walk up, the Sandfury Executioner 7274 (boss), his Executioner's Key 8444 (loot row, 100 % drop), the
--     Troll Cage 141073 (open, lock 420 needs 8444), then a hold on the stairs top 1887 1274 42 until instance data 1 (the
--     pyramid phase, tortoise-wow zulfarrak.h EVENT_PYRAMID) reads 3 (WAVE_1: Weegli arrived, wave 1 spawned), 90 s.
--   * leg 12 'stairs_top': a hold on the same spot until data 1 reads 7 (WAVE_3: waves 1 and 2 dead, wave 3 up, the crew walks
--     down), 600 s, units 7787 7788 7789 8876 8877. Both holds have flags 8 without 32: the phase is a script value, not an
--     encounter state, so the run leader reads no FAIL or reset into it and ends the hold when the data equals the param.
--   * leg 13 'stairs_base': Nekrum 7796 then Sezz'ziz 7275 at the foot, then wait_clear 35 yd (the rest of wave 3).
-- NOT IN THE ROWS: the plan's leg 'bly' (--skip-legs bly): Bly's gossip turns the crew hostile later, and the run leader has no boss
--   that turns hostile after a gossip. The optional 'gahzrilla' leg (the Gong 141832 rung with the Mallet of Zul'Farrak 9240) is not
--   in the rows either: the run leader's use_item has no object target.
-- SETUP FOR A RUN: AiPlayerbot.BotGossipNpcs must list 7607 (Weegli; the live conf has 7607,7604), or the gossip order is refused;
--   AiPlayerbot.BotAreaTriggers lists 962 (Zum'rah's trigger); a level 46 party of five (warrior tank as lead bot, priest, mage for
--   the Polymorph, two damage dealers) on a fresh instance; give the Executioner's Key 8444 to the lead bot before `.bot run start`
--   as a fallback (the loot row then reads 'already carries').
-- NOT CARRIED (the exporter lists them): skip lists, per-leg rest (stairs_top's 10 s gate), patrols, boss tactics, leftover
--   sweeps, companions_loot, the graves, the unit waits' own conditions and fail texts (the holds replace them).
-- HAND-SET VALUE: the UPDATE at the end puts back cc_priority 7246,7247,5648 (the owner's list of 2026-10-15, live in the
--   86-step clear of 2026-10-20). The plan file has 7246,5648 since turtle-agent 98d7b4a (its loader refuses a kill_first
--   entry in a cc list, and 7247 is kill_first); the exporter writes the file's list, so this row keeps the proven one until the
--   lead settles it in the plan file. cc_always 7246 and kill_first 7785,8179,5650,7247 are the exporter's (the owner's
--   kill_first with the Ward of Zum'rah first). tools/test_dungeon_plan_sql.py pins all three.
--
-- Everything below up to the UPDATE is the exporter's output, unchanged:
-- Dungeon plans for the run leader (.bot run), exported by turtle-agent (plan-export). Plan format v1; the mapping is
-- docs/guides/dungeon-run-leader.md ("From plan v1 to rows"). The plan file in turtle-agent is the single source: edit it
-- there and export again, never these rows by hand. The plan row carries the unit lists cc_priority, cc_always and kill_first
-- (columns from 20261015200000_world.sql). Never edit this file once applied; export a new one.
--
-- Idempotent: each plan's rows are deleted, then inserted. The tables come from 20261014150000_world.sql.

-- zulfarrak: map 209, 16 legs, 133 steps
DELETE FROM `ai_dungeon_step` WHERE `plan` = 'zulfarrak';
DELETE FROM `ai_dungeon_leg` WHERE `plan` = 'zulfarrak';
DELETE FROM `ai_dungeon_plan` WHERE `plan` = 'zulfarrak';
INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES
('zulfarrak', 209, '7246,5648', '7246', '7785,8179,5650,7247', 'turtle-agent db9f044 dungeons/zulfarrak.json', 'plan v1, live-checked');
INSERT INTO `ai_dungeon_leg` (`plan`, `leg`, `name`, `comment`) VALUES
('zulfarrak', 0, 'antusul_a', ''),
('zulfarrak', 1, 'antusul_a2', ''),
('zulfarrak', 2, 'antusul_a3', ''),
('zulfarrak', 3, 'antusul', ''),
('zulfarrak', 4, 'antusul_c', ''),
('zulfarrak', 5, 'antusul_b', ''),
('zulfarrak', 6, 'theka', ''),
('zulfarrak', 7, 'velratha', ''),
('zulfarrak', 8, 'velratha_b', ''),
('zulfarrak', 9, 'courtyard', ''),
('zulfarrak', 10, 'zumrah', ''),
('zulfarrak', 11, 'stairs', ''),
('zulfarrak', 12, 'stairs_top', ''),
('zulfarrak', 13, 'stairs_base', ''),
('zulfarrak', 14, 'weegli_door', ''),
('zulfarrak', 15, 'ukorz', '');
INSERT INTO `ai_dungeon_step` (`plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, `timeout_s`, `entries`, `comment`) VALUES
('zulfarrak', 0, 0, 'move', 0, 1252.3, 836.4, 9.1, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 0, 1, 'pull', 0, 1263.13, 842.53, 9.13, 15, 0, 0, 0, 0, '5649,5648', 'pull group 0'),
('zulfarrak', 0, 2, 'pull', 0, 1295.37, 810.53, 11.3, 30, 0, 0, 0, 0, '5649,5648,5650', 'pull group 1'),
('zulfarrak', 1, 0, 'move', 0, 1289.9, 816.1, 9.4, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 1, 1, 'move', 0, 1297.1, 827.9, 9.3, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 1, 2, 'pull', 0, 1324.33, 839.5, 10, 30, 0, 0, 0, 0, '5649,5648,5650', 'pull group 0'),
('zulfarrak', 1, 3, 'pull', 0, 1361.35, 818.6, 9, 15, 0, 0, 0, 0, '5649', 'pull group 1'),
('zulfarrak', 1, 4, 'pull', 0, 1406.5, 813.05, 8.9, 60, 0, 0, 0, 0, '5649', 'pull group 2'),
('zulfarrak', 1, 5, 'pull', 0, 1428.1, 796.63, 11.47, 15, 0, 0, 0, 0, '5648', 'pull group 3'),
('zulfarrak', 2, 0, 'move', 0, 1420.7, 801.1, 11, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 2, 1, 'move', 0, 1433.9, 804.7, 10.3, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 2, 2, 'move', 0, 1442.5, 804.7, 12.7, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 2, 3, 'move', 0, 1450.1, 805.9, 12.9, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 2, 4, 'move', 0, 1460.5, 808.5, 10.8, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 2, 5, 'pull', 0, 1465.17, 833.57, 10.06, 15, 0, 0, 0, 0, '5648', 'pull group 0'),
('zulfarrak', 2, 6, 'pull', 0, 1491.13, 824.87, 9.07, 30, 0, 0, 0, 0, '5650,5649', 'pull group 1'),
('zulfarrak', 2, 7, 'pull', 0, 1506.75, 838.5, 9.45, 15, 0, 0, 0, 0, '5648', 'pull group 2'),
('zulfarrak', 3, 0, 'move', 0, 1506, 844.9, 10.2, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 3, 1, 'move', 0, 1520, 840, 9.5, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 3, 2, 'move', 0, 1535, 834, 9, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 3, 3, 'pull', 0, 1530.05, 823.93, 9.07, 30, 0, 0, 0, 0, '5649,5648', 'pull group 0'),
('zulfarrak', 3, 4, 'pull', 0, 1545.6, 837.05, 8.9, 30, 0, 0, 0, 0, '5649,5648', 'pull group 1'),
('zulfarrak', 3, 5, 'pull', 0, 1583.83, 850.53, 10.3, 15, 0, 0, 0, 0, '5650,5648', 'pull group 2'),
('zulfarrak', 3, 6, 'pull', 0, 1615.6, 894.19, 9.38, 12, 0, 0, 0, 0, '5650,5648,5649', 'pull group 3'),
('zulfarrak', 3, 7, 'pull', 0, 1633.3, 868.3, 10.5, 15, 0, 0, 0, 0, '5650', 'pull group 4'),
('zulfarrak', 3, 8, 'pull', 0, 1666.77, 903.85, 8.97, 15, 0, 0, 0, 0, '5648,5650', 'pull group 5'),
('zulfarrak', 3, 9, 'pull', 0, 1660.45, 937.2, 9.15, 15, 0, 0, 0, 0, '7246,5648,5649', 'pull group 6'),
('zulfarrak', 3, 10, 'pull', 0, 1643.65, 904, 9.4, 10, 0, 0, 0, 0, '5648,7247', 'pull group 7'),
('zulfarrak', 4, 0, 'move', 0, 1633, 868, 10.5, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 4, 1, 'pull', 0, 1685.03, 876.3, 9, 20, 0, 0, 0, 0, '5649,5648', 'pull group 0'),
('zulfarrak', 4, 2, 'pull', 0, 1713.7, 784.1, 17.3, 15, 0, 0, 0, 0, '5649', 'pull group 1'),
('zulfarrak', 4, 3, 'pull', 0, 1776.9, 813.7, 23.2, 15, 0, 0, 0, 0, '5650', 'pull group 2'),
('zulfarrak', 5, 0, 'move', 0, 1769.03, 802.98, 24.04, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 5, 1, 'move', 0, 1787.8, 790.6, 17.7, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 5, 2, 'pull', 0, 1793.5, 753.4, 14.3, 50, 0, 0, 0, 0, '5649', 'pull group 0'),
('zulfarrak', 5, 3, 'pull', 0, 1756.28, 724.41, 19.44, 10, 0, 0, 0, 0, '5650,7246', 'pull group 1'),
('zulfarrak', 5, 4, 'pull', 0, 1802.2, 738.7, 14.5, 30, 0, 0, 0, 0, '8095', 'pull group 2'),
('zulfarrak', 5, 5, 'boss', 8127, 1815.8, 670.4, 15, 0, 0, 0, 0, 0, '', 'boss'),
('zulfarrak', 6, 0, 'move', 0, 1812, 700, 15, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 6, 1, 'move', 0, 1812, 716, 15, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 6, 2, 'move', 0, 1804, 753.8666, 17.225752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 6, 3, 'move', 0, 1804, 762.66656, 17.625753, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 6, 4, 'move', 0, 1807.2, 798.5, 13.9, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 6, 5, 'pull', 0, 1767.15, 853.55, 8.9, 30, 0, 0, 0, 0, '5649,7247', 'pull group 0'),
('zulfarrak', 6, 6, 'boss', 7272, 1778, 859.7, 8.9, 0, 0, 0, 0, 0, '', 'boss'),
('zulfarrak', 7, 0, 'move', 0, 1742.3999, 834.66656, 9.125752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 7, 1, 'move', 0, 1704, 813.5999, 8.925753, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 7, 2, 'move', 0, 1698.9333, 810.9332, 8.925753, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 7, 3, 'move', 0, 1691.4667, 809.33325, 9.425753, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 7, 4, 'move', 0, 1682.4, 807.9999, 8.925753, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 7, 5, 'move', 0, 1676.5333, 818.13324, 8.925753, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 7, 6, 'move', 0, 1674.9333, 821.5999, 9.025752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 7, 7, 'move', 0, 1676.1, 833.5, 9.5, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 7, 8, 'move', 0, 1678.48, 856.47, 8.95, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 7, 9, 'move', 0, 1668.92, 872.47, 11.57, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 7, 10, 'move', 0, 1658.35, 878.91, 13.06, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 7, 11, 'pull', 0, 1643.65, 904, 9.4, 15, 0, 0, 0, 0, '5648,7247', 'pull group 0'),
('zulfarrak', 7, 12, 'pull', 0, 1605.77, 973.07, 9.13, 15, 0, 0, 0, 0, '5649', 'pull group 1'),
('zulfarrak', 7, 13, 'pull', 0, 1589.4, 985.4, 9.2, 30, 0, 0, 0, 0, '5648', 'pull group 2'),
('zulfarrak', 7, 14, 'pull', 0, 1585.1, 1035.5, 9, 30, 0, 0, 0, 0, '5649', 'pull group 3'),
('zulfarrak', 7, 15, 'move', 0, 1584, 1050, 9, 0, 0, 0, 0, 0, '', 'pull group 4 approach'),
('zulfarrak', 7, 16, 'move', 0, 1603, 1043, 9.8, 0, 0, 0, 0, 0, '', 'pull group 4 approach'),
('zulfarrak', 7, 17, 'pull', 0, 1583, 1086.76, 8.98, 30, 0, 0, 0, 0, '5650,7246,8095,5649', 'pull group 4'),
('zulfarrak', 7, 18, 'move', 0, 1588, 1080, 9.1, 0, 0, 0, 0, 0, '', 'pull group 5 approach'),
('zulfarrak', 7, 19, 'move', 0, 1584, 1088, 9.1, 0, 0, 0, 0, 0, '', 'pull group 5 approach'),
('zulfarrak', 7, 20, 'pull', 0, 1606.7, 1107.4, 9, 15, 0, 0, 0, 0, '7246,5650', 'pull group 5'),
('zulfarrak', 8, 0, 'move', 0, 1633.6, 1130.7, 9.9, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 8, 1, 'pull', 0, 1639.44, 1106.82, 9.04, 15, 0, 0, 0, 0, '5649,5650,7246', 'pull group 0'),
('zulfarrak', 8, 2, 'pull', 0, 1649.33, 1157, 9.03, 30, 0, 0, 0, 0, '5649', 'pull group 1'),
('zulfarrak', 8, 3, 'pull', 0, 1715.48, 1186.18, 10.02, 15, 0, 0, 0, 0, '5649,5650,7246', 'pull group 2'),
('zulfarrak', 8, 4, 'boss', 7795, 1650.4, 1161.4, 9.9, 0, 0, 0, 0, 0, '', 'boss'),
('zulfarrak', 9, 0, 'move', 0, 1660.2666, 1161.0668, 10.833365, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 1, 'move', 0, 1661.3334, 1161.0668, 9.633366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 2, 'move', 0, 1693.0667, 1165.8667, 8.933366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 3, 'move', 0, 1696.5333, 1164.8, 8.933366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 4, 'move', 0, 1724.2666, 1149.3334, 9.033365, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 5, 'move', 0, 1728, 1148.2667, 9.233365, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 6, 'move', 0, 1732.2666, 1147.4667, 9.033365, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 7, 'move', 0, 1780, 1146.9333, 9.233365, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 8, 'move', 0, 1786.6666, 1149.8667, 9.033365, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 9, 9, 'move', 0, 1822, 1177, 9.1, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 9, 10, 'pull', 0, 1835.45, 1180.78, 9, 30, 0, 0, 0, 0, '7247,5649', 'pull group 0'),
('zulfarrak', 9, 11, 'pull', 0, 1863.11, 1199.95, 9, 15, 0, 0, 0, 0, '5650', 'pull group 1'),
('zulfarrak', 9, 12, 'pull', 0, 1878.71, 1159.65, 9.6, 15, 0, 0, 0, 0, '7246', 'pull group 2'),
('zulfarrak', 9, 13, 'pull', 0, 1903.9, 1147.69, 9.18, 15, 0, 0, 0, 0, '5649,5650', 'pull group 3'),
('zulfarrak', 9, 14, 'pull', 0, 1870.34, 1116.77, 10.05, 30, 0, 0, 0, 0, '7246,8095,5650', 'pull group 4'),
('zulfarrak', 9, 15, 'pull', 0, 1916.44, 1197.43, 9, 15, 0, 0, 0, 0, '5650,7246', 'pull group 5'),
('zulfarrak', 9, 16, 'pull', 0, 1894.79, 1216.11, 9.07, 15, 0, 0, 0, 0, '7247,7246,5649', 'pull group 6'),
('zulfarrak', 10, 0, 'move', 0, 1892.3, 1198.4, 8.9, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 10, 1, 'move', 0, 1892.2666, 1196.5333, 8.933366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 10, 2, 'move', 0, 1891.4666, 1192.2667, 8.933366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 10, 3, 'move', 0, 1880.292, 1143.5625, 10.672804, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 10, 4, 'move', 0, 1877.3333, 1130.6667, 11.133366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 10, 5, 'move', 0, 1864.2609, 1082.4279, 9.6781845, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 10, 6, 'move', 0, 1858.3999, 1060.7998, 9.025752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 10, 7, 'move', 0, 1859, 1058, 9.3, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 10, 8, 'move', 0, 1893.987, 1022.3433, 11.422803, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 10, 9, 'move', 0, 1897.3333, 1018.93317, 11.625752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 10, 10, 'move', 0, 1903, 1014, 11.8, 0, 0, 0, 0, 0, '', 'area trigger 962: the bot walks into the zone; needs AiPlayerbot.BotAreaTriggers = 962 (TortoiseBots #1002); expect unit 7271 hostile'),
('zulfarrak', 10, 11, 'boss', 7271, 1912.2, 1016.1, 11.6, 0, 0, 0, 0, 0, '', 'boss'),
('zulfarrak', 11, 0, 'move', 0, 1898.6666, 1018.6665, 12.125752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 1, 'move', 0, 1897.3333, 1018.93317, 11.625752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 2, 'move', 0, 1862.621, 1054.8591, 9.532091, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 3, 'move', 0, 1859.2, 1058.3998, 9.325752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 4, 'move', 0, 1858.3999, 1060.7998, 9.025752, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 5, 'move', 0, 1871.4723, 1109.0387, 10.480933, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 6, 'move', 0, 1877.3333, 1130.6667, 11.133366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 7, 'move', 0, 1888.5078, 1179.371, 9.393928, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 8, 'move', 0, 1891.4666, 1192.2667, 8.933366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 9, 'move', 0, 1892.2666, 1196.5333, 8.933366, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 10, 'move', 0, 1892.3, 1198.4, 8.9, 0, 0, 0, 0, 0, '', 'via'),
('zulfarrak', 11, 11, 'move', 0, 1889.0942, 1244.127, 28.90261, 0, 0, 0, 0, 0, '', 'path'),
('zulfarrak', 11, 12, 'move', 0, 1887, 1274, 42, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 11, 13, 'boss', 7274, 1886.8, 1286.8, 44.2, 0, 0, 0, 0, 0, '', 'boss'),
('zulfarrak', 11, 14, 'loot', 7274, 1886.8, 1286.8, 44.2, 0, 8444, 0, 0, 0, '', 'boss loot: Executioner''s Key (item 8444)'),
('zulfarrak', 11, 15, 'move', 0, 1886.9, 1295.4, 48.3, 0, 0, 0, 0, 0, '', 'stand'),
('zulfarrak', 11, 16, 'open', 141073, 1886.9, 1298.9, 48.2, 0, 8444, 0, 0, 0, '', 'open (Executioner''s Key (item 8444))'),
('zulfarrak', 11, 17, 'hold', 1, 1887, 1274, 42, 30, 0, 3, 8, 90, '7787,7788,7789,8876,8877', 'wait hold: the event''s mobs are fought on the spot until instance data 1 reads 3 (a script value, not an encounter state)'),
('zulfarrak', 12, 0, 'hold', 1, 1887, 1274, 42, 30, 0, 7, 8, 600, '7787,7788,7789,8876,8877', 'wait hold: the event''s mobs are fought on the spot until instance data 1 reads 7 (a script value, not an encounter state)'),
('zulfarrak', 13, 0, 'move', 0, 1885, 1232, 12.6, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 13, 1, 'boss', 7275, 1889.23, 1207.72, 8.87, 0, 0, 0, 0, 0, '7796', 'boss'),
('zulfarrak', 13, 2, 'wait_clear', 0, 1886, 1215, 9, 35, 0, 0, 0, 180, '', 'wait until no hostile: Nekrum and Sezz''ziz are dead but a Sandfury troll of wave 3 still lives at the foot of the stairs: the event (Bly''s menu, Weegli''s door) ends only when every unit of wave 3 is dead.'),
('zulfarrak', 14, 0, 'move', 0, 1884, 1212, 9, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 14, 1, 'hold', 1, 1877.5, 1199.6, 8.9, 30, 0, 8, 8, 600, '7787,7788,7789,8876,8877', 'wait hold: the event''s mobs are fought on the spot until instance data 1 reads 8 (a script value, not an encounter state)'),
('zulfarrak', 14, 2, 'gossip', 7607, 1877.52, 1199.63, 8.87, 0, 0, 1, 0, 0, '', 'talk to 7607, menu index 0 (cmd gossip option 1): Weegli''s menu has no option to blow up the door: the event is not over (PYRAMID_KILLED_ALL_TROLLS), or he is dead.'),
('zulfarrak', 14, 3, 'wait_object', 146084, 1854.5, 1142.9, 16.1, 0, 0, 1, 0, 60, '', 'wait for object 146084 active: The End Door (146084) is not open 60 s after Weegli Blastfuse''s gossip: he opens it only if he reaches it alive and unhurt.'),
('zulfarrak', 15, 0, 'move', 0, 1856.4, 1152.6, 14.4, 0, 0, 0, 0, 0, '', 'to'),
('zulfarrak', 15, 1, 'pull', 0, 1824.05, 1118.7, 27.25, 15, 0, 0, 0, 0, '7246', 'pull group 0'),
('zulfarrak', 15, 2, 'pull', 0, 1769, 1043.2, 50.6, 15, 0, 0, 0, 0, '8120', 'pull group 1'),
('zulfarrak', 15, 3, 'pull', 0, 1765.4, 1019.1, 53.65, 15, 0, 0, 0, 0, '7268', 'pull group 2'),
('zulfarrak', 15, 4, 'pull', 0, 1744.9, 1066.6, 51.2, 15, 0, 0, 0, 0, '8120', 'pull group 3'),
('zulfarrak', 15, 5, 'pull', 0, 1721.8, 1057.5, 53.8, 15, 0, 0, 0, 0, '7268', 'pull group 4'),
('zulfarrak', 15, 6, 'boss', 7267, 1727.5, 1017.4, 54.9, 0, 0, 0, 0, 0, '7797', 'boss');

-- Hand-set (see the header): the owner's cc_priority of 2026-10-15, as applied by 20261015200200_world.sql.
UPDATE `ai_dungeon_plan` SET `cc_priority` = '7246,7247,5648' WHERE `plan` = 'zulfarrak';
