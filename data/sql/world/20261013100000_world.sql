-- Alterac Valley named points for the AV battleground objective tactics.
--
-- Source: T-imothy/tortoise-wow, branch mantech-turtle at 0fb4d7c (read 2026-10-13),
--   modules/mod-playerbots/sql/world/classic/ai_playerbot_named_location.sql
--   blob 7497bbaf676de591ba53b401d8d1454e69e7a1e7, last changed by 8415f1b.
--   The 17 AV_* rows of that dump, all on map 30 (Alterac Valley).
-- Data-only port: the DROP and CREATE header is omitted. The schema stays owned by
--   20260824090000_world.sql. The dump has no licence header. The owner approved
--   shipping it on 2026-10-13 (docs/LICENSE_AUDIT.md).
--
-- Why it ships: BGTactics::SelectAvObjectiveAlliance and SelectAvObjectiveHorde
--   (ai/playerbot/strategy/actions/BattleGroundTacticsAV.cpp) look these 17 names up
--   with getNamedLocation. The names match that file exactly. A fresh server has no
--   AV_* rows, so those lookups return nothing.
--
-- Idempotency: the DELETE removes only names starting with AV_ (the underscore is
--   escaped, so AVxxx names are not matched), then the 17 rows go back in, so a
--   re-run ends in the same state. FISH_LOCATION% and every other named location
--   are not touched. The core runs each migration file inside one transaction, so
--   this file has no START TRANSACTION. The INSERT is plain, not IGNORE: a conflict
--   fails the migration loudly instead of skipping rows.

DELETE FROM `ai_playerbot_named_location` WHERE `name` LIKE 'AV\_%';

INSERT INTO `ai_playerbot_named_location` (`name`, `map_id`, `position_x`, `position_y`, `position_z`, `orientation`, `description`) VALUES
('AV_DUNBALDAR_NORTH', 30, 674.00061035156250000000, -143.12506103515625000000, 63.66151428222656250000, 0.99483770132064819300, 'AV - Dunbaldar North'),
('AV_DUNBALDAR_SOUTH', 30, 553.77941894531250000000, -78.65657806396484370000, 51.93778610229492187000, -1.22173047065734863000, 'AV - Dunbaldar South'),
('AV_EAST_FROSTWOLF_TOWER', 30, -1302.86816406250000000000, -316.96838378906250000000, 113.86710357666015620000, 2.00712871551513671800, 'AV - East Frostwolf Tower'),
('AV_FROSTWOLF_GRAVEYARD', 30, -1082.52941894531250000000, -346.56701660156250000000, 54.97712326049804687000, -1.55334317684173583000, 'AV - Frostwolf Graveyard'),
('AV_FROSTWOLF_RELIEF_HUT', 30, -1402.19189453125000000000, -307.46707153320312500000, 89.44189453125000000000, 0.19198621809482574400, 'AV - Frostwolf Relief Hut'),
('AV_ICEBLOOD_GARRISON_WAITING_ALLIANCE', 30, -492.17000000000000000000, -187.07700000000000000000, 57.13420000000000000000, 2.77000000000000000000, 'AV - Iceblood Garrison - Waiting Alliance'),
('AV_ICEBLOOD_GRAVEYARD', 30, -612.67163085937500000000, -396.69326782226562500000, 60.85839080810546875000, 3.08923268318176269500, 'AV - Iceblood Graveyard'),
('AV_ICEBLOOD_TOWER', 30, -571.87963867187500000000, -262.77676391601562500000, 75.00867462158203125000, -0.80285149812698364000, 'AV - Iceblood Tower'),
('AV_ICEWING_BUNKER', 30, 203.23849487304687500000, -360.26422119140625000000, 56.38615036010742187000, -0.87266474962234497000, 'AV - Icewing Bunker'),
('AV_SNOWFALL_GRAVEYARD', 30, -202.27778625488281200000, -113.20844268798828100000, 78.47940826416015625000, -1.25663685798645019000, 'AV - Snowfall Graveyard'),
('AV_STONEHEART_BUNKER', 30, -152.43391418457031200000, -441.61480712890625000000, 40.39709472656250000000, -1.93731522560119628000, 'AV - Stonehearth Bunker'),
('AV_STONEHEART_GRAVEYARD', 30, 77.50444793701171875000, -404.58734130859375000000, 46.78254318237304687000, 2.28638124465942382800, 'AV - Stoneheart Graveyard'),
('AV_STONEHEART_OUTPOST_WAITING_HORDE', 30, 28.12640000000000000000, -302.59300000000000000000, 15.07600000000000000000, 2.96000000000000000000, 'AV - Stoneheart Outpost - Waiting Horde'),
('AV_STORMPIKE_AID_STATION', 30, 638.59210205078125000000, -32.42203903198242180000, 46.06084442138671875000, -1.62315630912780761000, 'AV - Stormpike Aid Station'),
('AV_STORMPIKE_GRAVEYARD', 30, 669.00726318359375000000, -294.07827758789062500000, 30.29091072082519531000, 2.77507352828979492100, 'AV - Stormpike Graveyard'),
('AV_TOWERPOINT', 30, -768.08728027343750000000, -362.66607666015625000000, 90.89485931396484375000, 1.11701071262359619100, 'AV - Towerpoint'),
('AV_WEST_FROSTWOLF_TOWER', 30, -1297.71545410156250000000, -266.74148559570312500000, 114.15122222900390620000, -2.96705961227416992000, 'AV - West Frostwolf Tower');
