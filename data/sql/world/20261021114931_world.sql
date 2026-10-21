-- Dungeon bosses: the bosses of the planned dungeons, for the commander telemetry's boss rank.
--
-- A five-man boss carries creature rank 1 (elite) and no instance-bind flag in Turtle's world data, so the telemetry
-- (runtime/CommanderTelemetry.h RankKind) reported every one of them as 'e' and the dashboard map crowned none. The rows
-- below are read once at startup with the dungeon plans (ai/playerbot/DungeonPlanMgr.cpp) into the same (map, entry) set
-- as the plans' `boss` step entries (DungeonPlanTable::IsBossEntry); nothing reads the table per tick. The table also
-- lists the bosses no boss step names: Kresh 3653 (the plan skips him), Lady Anacondra 3671 (fought in pull groups),
-- Mutanus the Devourer 3654 (the Naralex event), Archaedas 2748 (a hold step) and Gahz'rilla 7273 (summoned at the gong,
-- not in the plan).
--
-- First content: the four planned dungeons, Ragefire Chasm (389), Wailing Caverns (43), Uldaman (70) and Zul'Farrak
-- (209). Every entry, name and map was read from Turtle's own world data (tortoise-wow sql/base creature_template and
-- creature); all are rank 1 there. Left out: the adds of a boss step (Eric "The Swift" 6907 and Olaf 6908 beside Baelog,
-- Nekrum Gutchewer 7796 beside Sezz'ziz, Ruuzlu 7797 beside Ukorz), the friendly Naralex 3679 and his Disciple 3678, the
-- Sergeant Bly event 7604 and rares (Deviate Faerie Dragon 5912, Zerillis 10082, Sandarr Dunereaver 10080). Other
-- dungeons get their rows when their plans arrive.
--
-- Idempotent: CREATE TABLE IF NOT EXISTS, a DELETE of these four maps only, then REPLACE. Never edit this file once
-- applied; add a newer migration.

CREATE TABLE IF NOT EXISTS `ai_dungeon_boss` (
  `map` int unsigned NOT NULL,
  `entry` int unsigned NOT NULL,
  `name` varchar(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`map`, `entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;

DELETE FROM `ai_dungeon_boss` WHERE `map` IN (389, 43, 70, 209);
REPLACE INTO `ai_dungeon_boss` (`map`, `entry`, `name`) VALUES
(389, 11517, 'Oggleflint'),
(389, 11520, 'Taragaman the Hungerer'),
(389, 11518, 'Jergosh the Invoker'),
(389, 11519, 'Bazzalan'),
(43, 3653, 'Kresh'),
(43, 3671, 'Lady Anacondra'),
(43, 3669, 'Lord Cobrahn'),
(43, 3670, 'Lord Pythas'),
(43, 3674, 'Skum'),
(43, 3673, 'Lord Serpentis'),
(43, 5775, 'Verdan the Everliving'),
(43, 3654, 'Mutanus the Devourer'),
(70, 6910, 'Revelosh'),
(70, 6906, 'Baelog'),
(70, 7228, 'Ironaya'),
(70, 7023, 'Obsidian Sentinel'),
(70, 7206, 'Ancient Stone Keeper'),
(70, 7291, 'Galgann Firehammer'),
(70, 4854, 'Grimlok'),
(70, 2748, 'Archaedas'),
(209, 8127, 'Antu''sul'),
(209, 7272, 'Theka the Martyr'),
(209, 7795, 'Hydromancer Velratha'),
(209, 7271, 'Witch Doctor Zum''rah'),
(209, 7274, 'Sandfury Executioner'),
(209, 7275, 'Shadowpriest Sezz''ziz'),
(209, 7267, 'Chief Ukorz Sandscalp'),
(209, 7273, 'Gahz''rilla');
