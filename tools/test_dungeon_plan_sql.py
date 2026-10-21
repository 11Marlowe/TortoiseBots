"""Guards the dungeon plan tables (ai_dungeon_plan, ai_dungeon_leg, ai_dungeon_step).

The loader (ai/playerbot/DungeonPlanMgr.cpp) names its columns in SELECTs; a column that a
migration does not create would make the query fail at startup and leave every plan out. This
checks that each column the loader reads is created by a migration, and that any step rows a
later (exporter) migration inserts use a kind the loader knows.
"""
import glob
import math
import os
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KINDS = {"move", "move_direct", "move_hop_exact", "pull", "boss", "loot", "open", "use_item", "wait_object",
         "wait_clear", "rest", "ritual", "hold", "gossip"}
TABLES = ("ai_dungeon_plan", "ai_dungeon_leg", "ai_dungeon_step", "ai_dungeon_boss")
# The plans whose bosses ai_dungeon_boss lists, and their maps.
BOSS_PLANS = {"ragefire_chasm": 389, "wailing_caverns": 43, "uldaman": 70, "zulfarrak": 209, "deadmines": 36}
# Every ai_dungeon_boss row, by (map, entry): the creature_template name and rank in the core's data (Tortoise V2
# sql/base tw_world_creature_template.sql, read 2026-10-21). All rank 1 (elite) and no instance bind, hence the table.
SEEDED_BOSSES = {
    (389, 11517): "Oggleflint", (389, 11520): "Taragaman the Hungerer", (389, 11518): "Jergosh the Invoker",
    (389, 11519): "Bazzalan",
    (43, 3653): "Kresh", (43, 3671): "Lady Anacondra", (43, 3669): "Lord Cobrahn", (43, 3670): "Lord Pythas",
    (43, 3674): "Skum", (43, 3673): "Lord Serpentis", (43, 5775): "Verdan the Everliving",
    (43, 3654): "Mutanus the Devourer", (43, 61965): "Vangros", (43, 61968): "Zandara Windhoof",
    (70, 6910): "Revelosh", (70, 6906): "Baelog", (70, 7228): "Ironaya", (70, 7023): "Obsidian Sentinel",
    (70, 7206): "Ancient Stone Keeper", (70, 7291): "Galgann Firehammer", (70, 4854): "Grimlok", (70, 2748): "Archaedas",
    (209, 8127): "Antu'sul", (209, 7272): "Theka the Martyr", (209, 7795): "Hydromancer Velratha",
    (209, 7271): "Witch Doctor Zum'rah", (209, 7274): "Sandfury Executioner", (209, 7275): "Shadowpriest Sezz'ziz",
    (209, 7267): "Chief Ukorz Sandscalp", (209, 7273): "Gahz'rilla",
    (36, 644): "Rhahk'Zor", (36, 642): "Sneed's Shredder", (36, 643): "Sneed", (36, 1763): "Gilnid",
    (36, 61963): "Masterpiece Harvester", (36, 646): "Mr. Smite", (36, 647): "Captain Greenskin",
    (36, 639): "Edwin VanCleef", (36, 645): "Cookie",
}
# Not bosses here: adds of a boss step (Eric 6907, Olaf 6908, Nekrum 7796, Ruuzlu 7797), the friendly Naralex 3679 and his
# Disciple 3678, the Sergeant Bly event 7604, Miner Johnson 3586 (a rare in the original game), rares (Deviate Faerie Dragon 5912, Zerillis 10082, Sandarr Dunereaver 10080).
NOT_BOSSES = {6907, 6908, 7796, 7797, 3679, 3678, 7604, 5912, 10082, 10080, 3586}
# Zum'rah's 31 Shallow Graves (128403), in the order the leg opens them: the nearest-neighbour walk of the plan from 1859 1058
# (the exporter writes each as a stand move, a loot and a sweep pull, in this order). A reorder is a deliberate change.
ZUMRAH_GRAVES = (
    (1858.70, 1053.75), (1862.12, 1050.76), (1871.08, 1046.76), (1875.85, 1044.26),
    (1881.46, 1041.43), (1881.99, 1034.68), (1888.36, 1037.00), (1867.74, 1041.84),
    (1845.96, 1047.25), (1841.08, 1045.54), (1843.19, 1041.16), (1839.18, 1036.88),
    (1832.72, 1032.39), (1841.82, 1027.76), (1839.42, 1023.49), (1845.54, 1024.12),
    (1852.41, 1024.78), (1861.12, 1019.39), (1865.03, 1012.49), (1866.24, 1008.40),
    (1871.30, 1008.32), (1871.86, 994.26), (1890.12, 989.76), (1894.39, 977.01),
    (1887.46, 967.14), (1882.70, 971.83), (1879.89, 974.65), (1876.44, 972.21),
    (1892.57, 1006.37), (1891.60, 1023.06), (1887.94, 1029.08),
)

def migrations():
    for path in sorted(glob.glob(os.path.join(ROOT, "data/sql/world/*_world.sql"))):
        with open(path, encoding="utf-8") as handle:
            yield os.path.basename(path), handle.read()


def newest_zulfarrak_rows():
    """The step rows of the newest migration that inserts the zulfarrak steps (its name, its text, its zulfarrak rows)."""
    newest = None
    for name, text in migrations():
        if re.search(r"INSERT INTO `ai_dungeon_step`.*?\('zulfarrak',", text, re.S):
            newest = (name, text)
    assert newest is not None, "no migration inserts the zulfarrak steps"
    name, text = newest
    return name, text, [r for r in DungeonPlanSql.step_rows(None, text) if r["plan"] == "zulfarrak"]


def newest_plan_text(plan):
    """The name and text of the newest migration that inserts the plan's steps."""
    newest = None
    for name, text in migrations():
        if re.search(r"INSERT INTO `ai_dungeon_step`.*?\('%s'," % plan, text, re.S):
            newest = (name, text)
    assert newest is not None, "no migration inserts the %s steps" % plan
    return newest


def newest_uldaman_rows():
    """The step rows of the newest migration that inserts the uldaman steps (its name, its text, its uldaman rows)."""
    name, text = newest_plan_text("uldaman")
    return name, text, [r for r in DungeonPlanSql.step_rows(None, text) if r["plan"] == "uldaman"]


def leg_rows(text, plan):
    """The ai_dungeon_leg rows of a migration for the plan as {leg: (name, comment)}."""
    return {int(m.group(1)): (m.group(2), m.group(3).replace("''", "'")) for m in
            re.finditer(r"^\('%s', (\d+), '(\w+)', '((?:[^']|'')*)'\)[,;]$" % plan, text, re.M)}


def dist(row, x, y):
    return math.hypot(float(row["x"]) - x, float(row["y"]) - y)


def created_columns():
    columns = {}
    for _name, text in migrations():
        for table in TABLES:
            match = re.search(r"CREATE TABLE IF NOT EXISTS `%s` \((.*?)\n\) ENGINE" % table, text, re.S)
            if match:
                columns.setdefault(table, set()).update(re.findall(r"^\s+`(\w+)`", match.group(1), re.M))
            for alter in re.findall(r"ALTER TABLE `%s`(.*?);" % table, text, re.S):
                columns.setdefault(table, set()).update(re.findall(r"ADD COLUMN IF NOT EXISTS `(\w+)`", alter))
    return columns


def loader_columns():
    with open(os.path.join(ROOT, "ai/playerbot/DungeonPlanMgr.cpp"), encoding="utf-8") as handle:
        text = handle.read().replace('"\n        "', "").replace('" "', "")
    found = {}
    for select in re.findall(r'"SELECT (.*?) FROM `(\w+)`"', text, re.S):
        found[select[1]] = set(re.findall(r"`(\w+)`", select[0]))
    return found


class DungeonPlanSql(unittest.TestCase):
    def test_tables_exist(self):
        columns = created_columns()
        for table in TABLES:
            self.assertIn(table, columns, "no migration creates %s" % table)

    def test_loader_reads_created_columns(self):
        created = created_columns()
        loaded = loader_columns()
        self.assertEqual(set(loaded), set(TABLES))
        for table, cols in loaded.items():
            missing = cols - created.get(table, set())
            self.assertFalse(missing, "%s: the loader reads columns no migration creates: %s" % (table, missing))

    def test_zulfarrak_keeps_its_crowd_control(self):
        # The Zul'Farrak plan row after all migrations (its newest INSERT, then the UPDATEs after it)
        # carries hand-set cc_always and kill_first (pending pjerra/turtle-agent#190); a re-export from
        # an exporter that does not write them must not land silently.
        row, name = None, None
        for name_, text in migrations():
            # A statement ends with the ';' that ends its line: a comment value may hold one ('live-checked; cut before ...').
            for statement in re.findall(r"(INSERT INTO `ai_dungeon_plan`.*?;$|UPDATE `ai_dungeon_plan`.*?;$)", text, re.S | re.M):
                if statement.startswith("INSERT"):
                    cols, values = re.match(r"INSERT INTO `ai_dungeon_plan` \(([^)]*)\) VALUES\s*(.*);", statement, re.S).groups()
                    columns = re.findall(r"`(\w+)`", cols)
                    for found in re.findall(r"\(('zulfarrak'.*?)\)\s*(?:,|;|$)", values + ";", re.S):
                        fields = [f.strip().strip("'") for f in re.findall(r"'(?:[^']|'')*'|[^,\s][^,]*", found)]
                        row, name = dict(zip(columns, fields)), name_
                elif row is not None and re.search(r"WHERE\s+`plan`\s*=\s*'zulfarrak'", statement):
                    for column, value in re.findall(r"`(\w+)`\s*=\s*'((?:[^']|'')*)'", statement.split("WHERE")[0]):
                        row[column] = value
                    name = name_
        self.assertIsNotNone(row, "no migration inserts the zulfarrak plan row")
        for column in ("cc_always", "kill_first"):
            self.assertTrue(row.get(column), "%s: the zulfarrak plan row has no %s" % (name, column))
        self.assertEqual(row.get("cc_always"), "7246")
        # 20261021005900 (the plan through the stairs event): the exporter writes the lists now; kill_first is the owner's
        # 8179,5650,7247 with the Ward of Zum'rah 7785 first (his leg is in the rows), and cc_priority stays the owner's list
        # (hand-set there: the plan file dropped 7247).
        self.assertEqual(row.get("kill_first"), "7785,8179,5650,7247")
        self.assertEqual(row.get("cc_priority"), "7246,7247,5648")

    def test_uldaman_keeps_its_event_steps(self):
        # The newest migration that inserts Uldaman's steps carries the two scripted events as hold rows
        # (#984; set by hand in 20261016120000, exported since 20261016220000): a re-export from an
        # exporter that has no `hold` kind must not drop them silently.
        newest = None
        for name, text in migrations():
            if re.search(r"INSERT INTO `ai_dungeon_step`.*?\('uldaman',", text, re.S):
                newest = (name, text)
        self.assertIsNotNone(newest, "no migration inserts the uldaman steps")
        name, text = newest
        for data, units in (("1", "'4857'"), ("2", "'2748,7309,7077,7076,10120'")):
            self.assertRegex(text, r"\('uldaman', \d+, \d+, 'hold', %s, [^)]*, 3, 40, 600, %s," % (data, units),
                             "%s: the uldaman hold on instance data %s is missing" % (name, data))
        self.assertEqual(len(re.findall(r"\('uldaman', \d+, \d+, 'ritual', \d+, [^)]*?, 0, 0, 3, 0, 0,", text)), 2,
                         "%s: the uldaman rituals need param 3" % name)

    def test_ragefire_wailing_deadmines_are_whole(self):
        # 20261017130000 exports the three plans (turtle-agent plan-export): each is deleted, then inserted, with
        # its bosses in leg order; the Wailing Caverns gm_cross is the only move_direct (a straight walk down to the
        # Serpentis ramp), and the Deadmines doors, gunpowder and cannon are plain steps (#984: nothing dropped silently).
        text = dict(migrations()).get("20261017130000_world.sql")
        self.assertIsNotNone(text, "20261017130000_world.sql is missing")
        bosses = {"ragefire_chasm": ["11517", "11520", "11518", "11519"],
                  "wailing_caverns": ["3669", "3670", "3674", "3673", "5775"],
                  "deadmines": ["644", "642", "61963", "1763", "646", "647", "639"]}
        for plan, entries in bosses.items():
            for table in ("ai_dungeon_step", "ai_dungeon_leg", "ai_dungeon_plan"):
                self.assertIn("DELETE FROM `%s` WHERE `plan` = '%s';" % (table, plan), text)
            self.assertLess(text.index("DELETE FROM `ai_dungeon_step` WHERE `plan` = '%s'" % plan),
                            text.index("INSERT INTO `ai_dungeon_plan` (`plan`, `map`, `cc_priority`, `cc_always`, `kill_first`, `source`, `comment`) VALUES\n('%s'" % plan))
            self.assertEqual(re.findall(r"^\('%s', \d+, \d+, 'boss', (\d+)," % plan, text, re.M), entries)
        self.assertEqual(re.findall(r"^\('(\w+)', \d+, \d+, 'move_direct'", text, re.M), ["wailing_caverns"])
        for kind, count in (("open", 3), ("loot", 1), ("wait_object", 6), ("wait_clear", 2)):
            self.assertEqual(len(re.findall(r"^\('deadmines', \d+, \d+, '%s'," % kind, text, re.M)), count, kind)
        self.assertRegex(text, r"\('deadmines', 9, \d+, 'open', 16398, [^)]*, 0, 5397, 0, 0, 0,")
        self.assertIn("gm_cross", text)  # the header names every step the run leader cannot do

    def step_rows(self, text):
        # Each ai_dungeon_step row of a migration as a dict of its columns, in file order.
        for block in re.findall(r"INSERT INTO `ai_dungeon_step` \(([^)]*)\) VALUES(.*?);\n", text, re.S):
            cols = [c.strip(" `") for c in block[0].split(",")]
            for row in re.findall(r"^\((.*)\)[,]?$", block[1], re.M):
                # A quoted value may hold commas (`entries`); the space after each comma is skipped before the match.
                values = [v.strip() for v in re.findall(r"\s*('(?:[^']|'')*'|[^,]+)", row)]
                yield dict(zip(cols, [v.strip("'") for v in values]))

    def test_exact_flag_only_on_a_move_before_its_direct_step(self):
        # flags 64 (kPlanFlagExact, DungeonPlanPolicy.h): the loader refuses it on any kind but move, and it marks the launch
        # point of the move_direct right after it in the same leg (the run leader sends that hop only after the exact proof).
        for name, text in migrations():
            rows = list(self.step_rows(text))
            for i, row in enumerate(rows):
                if int(row["flags"]) & 64 == 0:
                    continue
                self.assertEqual(row["kind"], "move", "%s: the exact flag on a %s row" % (name, row["kind"]))
                self.assertLess(i + 1, len(rows), "%s: an exact move is the last row" % name)
                after = rows[i + 1]
                self.assertEqual((after["plan"], after["leg"], after["kind"]), (row["plan"], row["leg"], "move_hop_exact"),
                                 "%s: the exact move %s/%s/%s is not followed by its move_hop_exact" % (name, row["plan"], row["leg"], row["seq"]))

    def test_hop_exact_follows_its_exact_move(self):
        # move_hop_exact (DungeonPlanPolicy.h kPlanKindMoveHopExact) is the hop launched from the flag 64 move right before it in
        # its leg; a module without exact waypoints skips the unknown kind (no direct order for the hop).
        for name, text in migrations():
            rows = list(self.step_rows(text))
            for i, row in enumerate(rows):
                if row["kind"] != "move_hop_exact":
                    continue
                self.assertGreater(i, 0, "%s: a move_hop_exact is the first row" % name)
                before = rows[i - 1]
                self.assertEqual((before["plan"], before["leg"], before["kind"]), (row["plan"], row["leg"], "move"), name)
                self.assertTrue(int(before["flags"]) & 64, "%s: the row before the move_hop_exact %s/%s/%s is not an exact move" %
                                (name, row["plan"], row["leg"], row["seq"]))

    def test_inserted_kinds_fit_the_column(self):
        # ai_dungeon_step.kind is a varchar(16) (20261014150000): a longer kind fails the INSERT in strict mode, or is cut.
        for name, text in migrations():
            for row in self.step_rows(text):
                self.assertLessEqual(len(row["kind"]), 16, "%s: kind %r is longer than the column" % (name, row["kind"]))
        with open(os.path.join(ROOT, "ai/playerbot/DungeonPlanPolicy.h"), encoding="utf-8") as handle:
            policy = handle.read()
        for kind in re.findall(r'\{"(\w+)", PlanStepKind::', policy) + re.findall(r'kPlanKind\w+ = "(\w+)"', policy):
            self.assertLessEqual(len(kind), 16, "kind %r is longer than ai_dungeon_step.kind" % kind)

    def test_wailing_caverns_serpentis_foot_is_exact(self):
        # The newest migration that inserts the wailing_caverns steps carries the live-proven hop (tests/wc-hop-probe-2): the foot
        # -293.3 0.5 -58 as an exact move (flags 64), then the move_direct to the landing -287.5 5 -63.6 (pjerra/TortoiseBots
        # feat/exact-waypoint). A re-export from an exporter that writes no exact flag would launch the hop from 3-6 yd short.
        newest = None
        for name, text in migrations():
            if re.search(r"INSERT INTO `ai_dungeon_step`.*?\('wailing_caverns',", text, re.S):
                newest = (name, text)
        self.assertIsNotNone(newest)
        name, text = newest
        rows = [r for r in self.step_rows(text) if r["plan"] == "wailing_caverns"]
        exact = [i for i, r in enumerate(rows) if int(r["flags"]) & 64]
        self.assertEqual(len(exact), 1, "%s: the wailing_caverns rows need exactly one exact move" % name)
        foot, hop = rows[exact[0]], rows[exact[0] + 1]
        self.assertEqual((foot["kind"], foot["x"], foot["y"], foot["z"]), ("move", "-293.3", "0.5", "-58"), name)
        self.assertEqual((hop["kind"], hop["x"], hop["y"], hop["z"]), ("move_hop_exact", "-287.5", "5", "-63.6"), name)

    def test_gossip_rows_fit_the_loader(self):
        # NormalizePlanStep (DungeonPlanPolicy.h): a gossip row needs its NPC entry, an option of 1-15 (the number `.bot cmd gossip` takes,
        # counted from 1: it refuses 0) and no flag; the timeout is at most 600. A row the loader refuses is skipped with a log line and the
        # run then pauses at the step before it, so a migration must never carry one.
        seen = 0
        for name, text in migrations():
            for row in self.step_rows(text):
                if row["kind"] != "gossip":
                    continue
                seen += 1
                where = "%s: gossip %s/%s/%s" % (name, row["plan"], row["leg"], row["seq"])
                self.assertGreater(int(row["entry"]), 0, where + ": no NPC entry")
                self.assertTrue(1 <= int(row["param"]) <= 15, where + ": the option is outside 1-15 (counted from 1)")
                self.assertEqual(int(row["flags"]), 0, where + ": a gossip carries no flag")
                self.assertLessEqual(int(row["timeout_s"]), 600, where)
        self.assertGreaterEqual(seen, 1)

    def test_zulfarrak_end_door_is_a_hold_a_gossip_and_a_door_wait(self):
        # Weegli Blastfuse (7607) blows the End Door (146084) only once the pyramid phase (instance data 1) reads 8 (PYRAMID_KILLED_ALL_TROLLS,
        # tortoise-wow zulfarrak.h:40) and his menu then offers its one line (zulfarrak.cpp:497: index 0, so `cmd gossip` option 1).
        name, _text, zf = newest_zulfarrak_rows()
        rows = [r for r in zf if r["leg"] == "14"]
        self.assertEqual([r["kind"] for r in rows], ["move", "hold", "gossip", "wait_object"])
        hold, gossip, door = rows[1], rows[2], rows[3]
        # flag 8 alone: the phase is a script value, not an encounter state (no 32), and param 8 is above the encounter states 0-4
        self.assertEqual((hold["entry"], hold["param"], hold["flags"], hold["timeout_s"]), ("1", "8", "8", "600"))
        self.assertEqual((gossip["entry"], gossip["param"], gossip["flags"], gossip["timeout_s"]), ("7607", "1", "0", "0"))
        self.assertEqual((gossip["x"], gossip["y"], gossip["z"]), ("1877.52", "1199.63", "8.87"))
        # the door wait: object 146084 in state 1 (active); the order of the rows is the point
        self.assertEqual((door["entry"], door["param"]), ("146084", "1"))
        # the Ukorz leg follows with the plain kinds (Bly's leg is not in the rows)
        ukorz = [r for r in zf if r["leg"] == "15"]
        self.assertEqual(max(int(r["leg"]) for r in zf), 15, name + ": Ukorz is the last leg")
        self.assertEqual([r["kind"] for r in ukorz], ["move", "pull", "pull", "pull", "pull", "pull", "boss"])
        self.assertEqual((ukorz[-1]["entry"], ukorz[-1]["entries"]), ("7267", "7797"))

    def test_gossip_kind_is_in_the_policy_tables(self):
        # The kind text the exporter writes is the one the loader parses and the status line prints.
        with open(os.path.join(ROOT, "ai/playerbot/DungeonPlanPolicy.h"), encoding="utf-8") as handle:
            policy = handle.read()
        self.assertIn('{"gossip", PlanStepKind::Gossip}', policy)
        self.assertIn('case PlanStepKind::Gossip: return "gossip";', policy)

    def test_hold_rows_follow_the_loader_rules(self):
        # NormalizePlanStep (DungeonPlanPolicy.h) for a hold: at most one done flag (8 data, 16 object); the encounter flag 32 only
        # with 8 and then a state 0-4; by object a state 0-2 and an entry; with neither flag, entries. A plain script value (flag 8
        # without 32: Zul'Farrak's pyramid phase) may be any number.
        for name, text in migrations():
            for row in self.step_rows(text):
                if row["kind"] != "hold":
                    continue
                flags, param, where = int(row["flags"]), int(row["param"]), "%s: hold %s/%s/%s" % (name, row["plan"], row["leg"], row["seq"])
                self.assertFalse(flags & 8 and flags & 16, where + " has both done flags")
                if flags & 32:
                    self.assertTrue(flags & 8, where + ": the encounter flag without the data flag")
                    self.assertLessEqual(param, 4, where + ": no encounter state")
                if flags & 16:
                    self.assertNotEqual(row["entry"], "0", where)
                    self.assertLessEqual(param, 2, where)
                if not flags & 24:
                    self.assertTrue(row["entries"], where + ": done by its units without entries")

    def test_zulfarrak_runs_the_stairs_event(self):
        # The newest migration that inserts the zulfarrak steps (20261021174713, the graves on 20261021115557, 20261021095914 before; turtle-agent plan-export of the plan with the stairs
        # holds, Weegli's door and Ukorz) carries Zum'rah, the Executioner's key to the cage, the three holds on the pyramid phase
        # (instance data 1: 3 = WAVE_1, 7 = WAVE_3 on the stairs top, 8 = PYRAMID_KILLED_ALL_TROLLS at the foot, script values with
        # flag 8 alone), Nekrum before Sezz'ziz at the end of leg 13 and Ukorz as the last row; a re-export from an exporter without hold values
        # would cut the plan at the stairs again (or write them as encounter states, which the loader refuses above 4).
        name, text, rows = newest_zulfarrak_rows()
        self.assertEqual([r["entry"] for r in rows if r["kind"] == "boss"], ["8127", "7272", "7795", "7271", "7274", "7275", "7267"], name)
        cage = [i for i, r in enumerate(rows) if r["kind"] == "open"]
        self.assertEqual(len(cage), 1, name)
        i = cage[0]
        self.assertEqual((rows[i]["entry"], rows[i]["item"]), ("141073", "8444"), name)
        loot = [r for r in rows[:i] if r["kind"] == "loot"]
        self.assertEqual((loot[-1]["entry"], loot[-1]["item"]), ("7274", "8444"), name + ": the key is looted before the cage")
        holds = [r for r in rows if r["kind"] == "hold"]
        self.assertEqual([(r["entry"], r["param"], r["flags"], r["timeout_s"]) for r in holds], [("1", "3", "8", "90"), ("1", "7", "8", "600"), ("1", "8", "8", "600")], name)
        self.assertEqual(rows[i + 1]["kind"], "hold", name + ": the first hold right after the cage (the event's start)")
        for r in holds[:2]:
            self.assertEqual((r["x"], r["y"], r["z"], r["entries"]), ("1887", "1274", "42", "7787,7788,7789,8876,8877"), name)
        self.assertEqual((holds[2]["leg"], holds[2]["x"], holds[2]["y"], holds[2]["z"], holds[2]["entries"]),
                         ("14", "1877.5", "1199.6", "8.9", "7787,7788,7789,8876,8877"), name)
        leg13 = [r for r in rows if r["leg"] == "13"]
        self.assertEqual(leg13[-2]["entries"], "7796", name + ": Nekrum before Sezz'ziz")
        self.assertEqual(leg13[-1]["kind"], "wait_clear", name)
        self.assertEqual((rows[-1]["kind"], rows[-1]["leg"]), ("boss", "15"), name + ": Ukorz ends the plan")
        self.assertIn("BotAreaTriggers = 962", text, name + ": the header names the trigger setting Zum'rah needs")

    def test_zulfarrak_antusul_pulls_the_pair_before_the_camp_from_stands(self):
        # r2-19z19 step 26: with the camp before the pair and no stand, the run leader stopped 30 yd short of the camp centre, 15.4 yd from the
        # pair's Shadowcaster, and the walk began the fight. The pair (5648,7247) is pulled first, from the stand 1620 880; the camp
        # (5648,5650) after it, from 1630 878; the north pack last. Each pull is right behind its stand move.
        name, _text, rows = newest_zulfarrak_rows()
        leg = [r for r in rows if r["leg"] == "3"]
        pulls = [(i, r) for i, r in enumerate(leg) if r["kind"] == "pull"]
        order = [r["entries"] for _i, r in pulls]
        self.assertEqual(order[-3:], ["5648,7247", "5648,5650", "7246,5648,5649"], name + ": the pair, the camp, the north pack")
        for (i, r), stand in ((pulls[-3], ("1620", "880")), (pulls[-2], ("1630", "878"))):
            move = leg[i - 1]
            self.assertEqual((move["kind"], move["x"], move["y"]), ("move", stand[0], stand[1]), name + ": a stand before " + r["entries"])
        # the stands keep 20 yd from every later pack and 15 yd from the patrol path (the plan check of turtle-agent measures it); here the
        # pair's stand is west of 1625 and the camp's stand west of 1635, so neither is near the Shadowcaster camp at 1660 901
        for stand, limit in ((leg[pulls[-3][0] - 1], 1625), (leg[pulls[-2][0] - 1], 1635)):
            self.assertLess(float(stand["x"]), limit, name)
        self.assertEqual(max(int(r["leg"]) for r in rows), 15, name)

    def test_zulfarrak_graves_before_the_trigger(self):
        # Zum'rah's 31 Shallow Graves (128403) are opened one at a time before the trigger 962 (turtle-agent plan-export
        # --carrier-chests): each is a move to its stand (4 yd or less from the grave), the loot (gone-is-done flag 256, item 0)
        # and the sweep pull of 7286,7276 at the grave's point (radius 15). The walk keeps ZUMRAH_GRAVES' order. 128308 (the
        # script graves, not Zum'rah's) is never a row. The graves are read from the newest zulfarrak migration.
        name, _text, rows = newest_zulfarrak_rows()
        leg = next(r["leg"] for r in rows if r["kind"] == "boss" and r["entry"] == "7271")
        steps = [r for r in rows if r["leg"] == leg]
        self.assertEqual([int(r["seq"]) for r in steps], list(range(len(steps))), name + ": the zumrah seq is not 0..n-1")
        self.assertEqual([r for r in rows if r["entry"] == "128308"], [], name + ": a 128308 row (script graves, not Zum'rah's)")
        loots = [i for i, r in enumerate(steps) if r["kind"] == "loot"]
        self.assertEqual(len(loots), len(ZUMRAH_GRAVES), name + ": the number of grave loot rows")
        for i, (gx, gy) in zip(loots, ZUMRAH_GRAVES):
            where = "%s: grave at seq %s" % (name, steps[i]["seq"])
            self.assertEqual((steps[i]["entry"], steps[i]["flags"], steps[i]["item"]), ("128403", "256", "0"), where)
            self.assertLessEqual(math.hypot(float(steps[i]["x"]) - gx, float(steps[i]["y"]) - gy), 0.1, where + ": not the plan's point")
            move, pull = steps[i - 1], steps[i + 1]
            self.assertEqual((move["kind"], move["comment"]), ("move", "stand"), where + ": not preceded by its stand")
            self.assertLessEqual(math.hypot(float(move["x"]) - gx, float(move["y"]) - gy), 4, where + ": the stand is more than 4 yd off")
            self.assertEqual((pull["kind"], pull["entries"], pull["radius"]), ("pull", "7286,7276", "15"), where + ": not followed by the sweep")
            self.assertLessEqual(math.hypot(float(pull["x"]) - float(steps[i]["x"]), float(pull["y"]) - float(steps[i]["y"])), 0.1,
                                 where + ": the sweep is not at the grave")
        trigger = [i for i, r in enumerate(steps) if (r["kind"], r["x"], r["y"], r["z"]) == ("move", "1903", "1014", "11.8")]
        boss = [i for i, r in enumerate(steps) if r["kind"] == "boss"]
        self.assertEqual((len(trigger), len(boss)), (1, 1), name + ": the trigger 962 and the boss 7271 are each one row")
        self.assertLess(loots[-1] + 1, trigger[0], name + ": the last grave's sweep is not before the trigger")
        self.assertLess(trigger[0], boss[0], name + ": the trigger is not before the boss")
        self.assertEqual(boss[0], len(steps) - 1, name + ": the boss does not end the zumrah leg")

    def test_zulfarrak_velratha_approach_stays_off_the_island_shadowhunter(self):
        # turtle-agent#286: the lone Shadowhunter 81610 stands on a navmesh island at 1591 1047.7 (no path leaves it; a fight there
        # takes the tank onto the island, zf-r6-19z24). Pull group 4's approach points were 1584 1050 and 1603 1043, 7.4 and 12.9 yd
        # from it; they are 1594 1011, 1598 1028 and 1606 1040 now, and the leg's comment carries the plan's `leave` list as text
        # (the run leader does not read it yet).
        name, text, rows = newest_zulfarrak_rows()
        legs = leg_rows(text, "zulfarrak")
        leg = next(k for k, (leg_name, _c) in legs.items() if leg_name == "velratha")
        steps = [r for r in rows if r["leg"] == str(leg)]
        approach = [(float(r["x"]), float(r["y"])) for r in steps if r["kind"] == "move" and r["comment"] == "pull group 4 approach"]
        self.assertEqual(approach, [(1594.0, 1011.0), (1598.0, 1028.0), (1606.0, 1040.0)], name)
        for old in ((1584.0, 1050.0), (1603.0, 1043.0)):
            self.assertEqual([r for r in steps if (float(r["x"]), float(r["y"])) == old], [], "%s: the old approach point %s is back" % (name, old))
        for r in steps:
            if r["kind"] == "move":
                self.assertGreaterEqual(dist(r, 1591.0, 1047.7), 15, "%s: velratha move %s is within 15 yd of the island Shadowhunter" % (name, r["seq"]))
        # the approach is right before its pull, which still fetches the Shadowhunters and the Witch Doctor of the group
        after = steps[[r["comment"] for r in steps].index("pull group 4")]
        self.assertEqual((after["kind"], after["x"], after["y"]), ("pull", "1583", "1086.76"), name)
        self.assertEqual(int(after["seq"]) - 1, int(next(r for r in steps if r["x"] == "1606")["seq"]), name + ": the last approach is not right before its pull")
        comment = legs[leg][1]
        self.assertIn("leave (not read by the run leader yet)", comment, name)
        self.assertIn("81610", comment, name)
        self.assertIn("1591 1047.7", comment, name)
        # the graves of 20261021115557 stay: 31 loots of 128403 with the gone-is-done flag, none of 128308
        graves = [r for r in rows if r["kind"] == "loot" and r["entry"] == "128403"]
        self.assertEqual(len(graves), 31, name)
        self.assertEqual({r["flags"] for r in graves}, {"256"}, name)

    def test_uldaman_pulls_the_keepers_neighbours_before_the_boss(self):
        # PLAN-ULDAMAN-GRIMLOK #2 (turtle-agent#287): the boss 7206 was pulled with the Stone Steward 4860 (a patrol that walks to 13.6 yd
        # from it), the Sculptors and the Rocksmasher of the galgann leg alive 20-35 yd north, and 5-6 mobs joined (r4 slot1, r6 slot5).
        # They are pulled first, each from an approach move, in the Keeper's own leg; nothing of them is left in the galgann leg.
        name, text, rows = newest_uldaman_rows()
        legs = leg_rows(text, "uldaman")
        keeper = next(k for k, (leg_name, _c) in legs.items() if leg_name == "ancient_stone_keeper")
        galgann = next(k for k, (leg_name, _c) in legs.items() if leg_name == "galgann")
        steps = [r for r in rows if r["leg"] == str(keeper)]
        boss = [i for i, r in enumerate(steps) if r["kind"] == "boss" and r["entry"] == "7206"]
        self.assertEqual(boss, [len(steps) - 1], name + ": the Stone Keeper is the one boss row and ends its leg")
        for entries, x, y in (("4860", -39.6, 272.8), ("7012,7011", -43.03, 263.93), ("7011", -46.4, 202.9)):
            pulls = [i for i, r in enumerate(steps) if r["kind"] == "pull" and r["entries"] == entries and dist(r, x, y) < 0.1]
            self.assertEqual(len(pulls), 1, "%s: the pull of %s at %s %s is missing or doubled" % (name, entries, x, y))
            self.assertLess(pulls[0], boss[0], "%s: the pull of %s comes after the Stone Keeper" % (name, entries))
            self.assertEqual(steps[pulls[0] - 1]["kind"], "move", name + ": the pull of %s has no approach move" % entries)
            self.assertIn("approach", steps[pulls[0] - 1]["comment"], name)
        # they are the last three pulls of the leg, the Steward (the patrol's ground) first; the boss approach (-73.3 240.2) is the row before the boss
        self.assertEqual([r["entries"] for r in steps if r["kind"] == "pull"][-3:], ["4860", "7012,7011", "7011"], name)
        self.assertEqual((steps[-2]["kind"], steps[-2]["x"], steps[-2]["y"]), ("move", "-73.3", "240.2"), name)
        for r in rows:
            if r["leg"] == str(galgann) and r["kind"] == "pull":
                self.assertNotEqual(r["entries"], "4860", name + ": the Steward is pulled in the galgann leg again")
                self.assertGreater(dist(r, -43.03, 263.93), 1, name + ": the Sculptor pull is in the galgann leg again")

    def test_uldaman_grimlok_corridor_is_pulled_one_group_at_a_time(self):
        # PLAN-ULDAMAN-GRIMLOK #2: Brawler 30105 (40.6 359.9) joined pull group 0 in 8 of 8 runs, so group 0 lists it (radius 16 at
        # 35.77 370.57: 11.7 yd to it, the Brawler at 55.4 379 stays out). The first groups of the keepers_door leg are pulled next
        # from stands at 28 360 and 33 354, the patrol and its trio are one group (radius 18), and the pair at 38-40 439-445 is pulled from 48 419.
        name, text, rows = newest_uldaman_rows()
        legs = leg_rows(text, "uldaman")
        by_name = {leg_name: k for k, (leg_name, _c) in legs.items()}
        grimlok = [r for r in rows if r["leg"] == str(by_name["grimlok"])]
        pulls = [r for r in grimlok if r["kind"] == "pull"]
        first = pulls[0]
        self.assertEqual((first["comment"], first["radius"], first["entries"]), ("pull group 0", "16", "4853,4855"), name)
        self.assertLessEqual(dist(first, 40.6, 359.9), float(first["radius"]), name + ": the Brawler 30105 at 40.6 359.9 is outside group 0")
        self.assertGreater(dist(first, 55.4, 379.0), float(first["radius"]), name + ": the patrol-side Brawler at 55.4 379 is inside group 0")
        # the keepers_door packs now pulled from the grimlok leg, before the patrol and the east groups
        order = [(float(r["x"]), float(r["y"])) for r in pulls]
        def at(x, y):
            found = [i for i, (px, py) in enumerate(order) if math.hypot(px - x, py - y) < 0.1]
            self.assertEqual(len(found), 1, "%s: the grimlok leg has %d pulls at %s %s" % (name, len(found), x, y))
            return found[0]
        pair, brawlers, merged = at(49.1, 343.45), at(39.55, 327.5), at(58.63, 374.4)
        self.assertLess(0, pair, name)
        self.assertLess(pair, brawlers, name)
        self.assertLess(brawlers, merged, name + ": the keepers_door packs are not pulled before the patrol")
        for index, stand in ((pair, (28.0, 360.0)), (brawlers, (33.0, 354.0))):
            move = grimlok[grimlok.index(pulls[index]) - 1]
            self.assertEqual((move["kind"], float(move["x"]), float(move["y"])), ("move", stand[0], stand[1]), name)
        # the patrol and the trio are one group of radius 18 (pull groups 1 and 2 of the old order were never alone)
        self.assertEqual((pulls[merged]["radius"], pulls[merged]["entries"]), ("18", "4855,4853"), name)
        self.assertEqual([i for i, (px, py) in enumerate(order) if math.hypot(px - 57.9, py - 374.0) < 3 and i != merged], [], name + ": a second pull of the patrol")
        # the southern pair from 48 419, right before its pull, then Grimlok
        south = at(38.85, 442.0)
        move = grimlok[grimlok.index(pulls[south]) - 1]
        self.assertEqual((move["kind"], float(move["x"]), float(move["y"])), ("move", 48.0, 419.0), name)
        self.assertEqual(grimlok[-1]["kind"], "boss", name)
        self.assertEqual(grimlok[-1]["entry"], "4854", name)
        # none of the three packs is left to the keepers_door leg
        for r in rows:
            if r["leg"] == str(by_name["keepers_door"]) and r["kind"] == "pull":
                for x, y in ((49.1, 343.45), (40.6, 359.9), (39.55, 327.5)):
                    self.assertGreater(dist(r, x, y), 1, "%s: keepers_door pulls the pack at %s %s again" % (name, x, y))

    def test_zulfarrak_and_uldaman_rows_pass_the_loader_checks(self):
        # Every step row of the newest zulfarrak and uldaman migrations goes through the loader's own code (DungeonPlanPolicy.h:
        # ParsePlanStepRowKind, ParsePlanEntries, DungeonPlanTable::AddStep with NormalizePlanStep and the (plan, leg, seq) key),
        # compiled from tools/test_dungeon_plan_rows.cpp: a row the loader refuses is skipped at startup and the run pauses before it.
        compiler = shutil.which("g++")
        if compiler is None:
            self.skipTest("g++ is not installed")
        lines, counts = [], {}
        for plan, rows in (("zulfarrak", newest_zulfarrak_rows()[2]), ("uldaman", newest_uldaman_rows()[2])):
            lines.append("P\t%s\t%d" % (plan, BOSS_PLANS[plan]))
            counts[plan] = len(rows)
            for r in rows:
                lines.append("\t".join(["S", r["plan"], r["leg"], r["seq"], r["kind"], r["entry"], r["x"], r["y"], r["z"], r["radius"], r["item"],
                                        r["param"], r["flags"], r["timeout_s"], r["entries"]]))
        self.assertGreater(counts["zulfarrak"], 200)
        self.assertGreater(counts["uldaman"], 130)
        with tempfile.TemporaryDirectory() as tmp:
            binary = os.path.join(tmp, "rows")
            build = subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", os.path.join(ROOT, "tools/test_dungeon_plan_rows.cpp"), "-o", binary],
                                   capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            run = subprocess.run([binary], input="\n".join(lines) + "\n", capture_output=True, text=True)
        self.assertEqual(run.stdout.strip().splitlines()[-1], "rows %d refused 0" % sum(counts.values()), run.stdout)
        self.assertEqual(run.returncode, 0, run.stdout)

    def test_inserted_step_kinds_are_known(self):
        for name, text in migrations():
            for block in re.findall(r"INSERT INTO `ai_dungeon_step` \(([^)]*)\) VALUES(.*?);", text, re.S):
                cols = [c.strip(" `") for c in block[0].split(",")]
                kind_at = cols.index("kind")
                for row in re.findall(r"^\((.*)\)[,]?$", block[1], re.M):
                    values = [v.strip() for v in re.findall(r"'[^']*'|[^,]+", row)]
                    self.assertIn(values[kind_at].strip("'"), KINDS, "%s: unknown step kind in %s" % (name, row))

    def boss_rows(self):
        # Each ai_dungeon_boss row of every migration as (map, entry, name), in file order.
        for _name, text in migrations():
            for block in re.findall(r"(?:INSERT|REPLACE) INTO `ai_dungeon_boss` \(`map`, `entry`, `name`\) VALUES(.*?);\n", text, re.S):
                for row in re.findall(r"^\((\d+), (\d+), '((?:[^']|'')*)'\)[,]?$", block, re.M):
                    yield int(row[0]), int(row[1]), row[2].replace("''", "'")

    def test_boss_table_is_the_seed(self):
        rows = list(self.boss_rows())
        keys = [(m, e) for m, e, _n in rows]
        self.assertEqual(len(keys), len(set(keys)), "a (map, entry) is seeded twice")
        self.assertEqual({(m, e): n for m, e, n in rows}, SEEDED_BOSSES)
        self.assertFalse({e for _m, e in keys} & NOT_BOSSES, "an add, an event unit or a rare is seeded as a boss")

    def test_boss_table_lists_every_plan_boss(self):
        # The newest migration that inserts a plan's boss steps holds its bosses (later ones replace single legs).
        seeded = set(SEEDED_BOSSES)
        for plan, map_id in BOSS_PLANS.items():
            newest = None
            for name, text in migrations():
                if re.search(r"^\('%s', \d+, \d+, 'boss'," % plan, text, re.M):
                    newest = text
            self.assertIsNotNone(newest, plan)
            entries = [int(r["entry"]) for r in self.step_rows(newest) if r["plan"] == plan and r["kind"] == "boss"]
            self.assertTrue(entries, plan)
            for entry in entries:
                self.assertIn((map_id, entry), seeded, "%s: boss step entry %d is not in ai_dungeon_boss" % (plan, entry))

    def test_loader_adds_the_boss_rows_before_the_seal(self):
        # AddBoss is refused after the seal, so a row read after it would be dropped without a word.
        with open(os.path.join(ROOT, "ai/playerbot/DungeonPlanMgr.cpp"), encoding="utf-8") as handle:
            loader = handle.read()
        add = loader.find("table.AddBoss(fields[0].GetUInt32(), fields[1].GetUInt32())")
        self.assertNotEqual(add, -1, "the loader does not add the ai_dungeon_boss rows")
        self.assertLess(add, loader.index("table.Seal();"))

    def test_boss_table_is_created_and_seeded_idempotently(self):
        for name, text in migrations():
            if "`ai_dungeon_boss`" not in text:
                continue
            self.assertNotRegex(text, r"CREATE TABLE `ai_dungeon_boss`", name)  # only IF NOT EXISTS
            self.assertNotRegex(text, r"(?m)^INSERT INTO `ai_dungeon_boss`", name)  # REPLACE, so a re-run keeps one row
            # The DELETE before the rows is scoped to exactly the maps the file seeds: a seeded map it leaves out keeps a
            # row a later file drops; a map it names beyond them loses rows it does not put back.
            seeded = {int(m) for m in re.findall(r"(?m)^\((\d+), \d+, '", text)}
            scoped = re.findall(r"DELETE FROM `ai_dungeon_boss` WHERE `map` IN \(([\d, ]+)\);", text)
            self.assertEqual(len(scoped), 1, name)
            self.assertEqual({int(m) for m in scoped[0].split(",")}, seeded, name)


if __name__ == "__main__":
    unittest.main()
