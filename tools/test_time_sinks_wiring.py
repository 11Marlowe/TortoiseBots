"""The run leader's time-sink fixes of 2026-10-23 reach the game (tools/test_run_time_sinks_policy.cpp tests the
policy itself; this file checks the glue in commands/BotCommands.cpp, the config plumbing and the docs that carry it).

(A) the walk back: the glue sets Run::startStep next to Start (a placed run never walks back before it), passes the
    pre-start steps as uncleared to the straight path's pack check (UnclearedForPath), and reports a partial approach
    path (Facts::approachIncomplete) for a straight walk back; the policy rescues a stuck walk back instead of pausing,
    and the retreat at two dead is read from AiPlayerbot.DungeonRunRetreatAt2Dead on every tick.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
POLICY = (ROOT / "runtime/DungeonRunPolicy.h").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class TimeSinksWiringTest(unittest.TestCase):
    def test_start_step_is_set_next_to_start(self):
        self.assertIn("        slot.run = Start(plan, startStep);\n"
                      "        slot.run.startStep = startStep;", COMMANDS)
        floor = body(POLICY, "inline std::size_t ReplayFloor(const Run& r)", "\n}\n")
        self.assertIn("r.startStep", floor)

    def test_straight_path_check_counts_the_pre_start_steps(self):
        self.assertIn("f.directPathCrossesPack = PathCrossesUnclearedPack(*slot.run.plan,\n"
                      "                            TortoiseBots::DungeonRun::LiveFrom(slot.run, true), whole, f.team,\n"
                      "                            TortoiseBots::DungeonRun::UnclearedForPath(slot.run));", COMMANDS)

    def test_partial_approach_path_is_reported(self):
        approach = body(COMMANDS, "bool ApproachOnPath(Player* lead,", "\n}\n")
        # Round 1 of 0d9957be: a bare PATHFIND_INCOMPLETE (the polygon cap, a point just off the mesh) is not short.
        self.assertIn("*incomplete = PathEndsShort((type & PATHFIND_NOPATH) != 0, (type & PATHFIND_INCOMPLETE) != 0, end.x, end.y,", approach)
        # Before the no-path return, so a no path leaves it read too.
        self.assertLess(approach.index("*incomplete ="), approach.index("if (path.getPathType() & PATHFIND_NOPATH)"))
        self.assertIn("direct ? &f.approachIncomplete : nullptr);", COMMANDS)
        walk_back = body(POLICY, "inline bool WalkBack(Run& r, const Facts& f, std::uint32_t dt, std::string& order)",
                         "\n}\n")
        self.assertIn("if (r.walkBackDirect && f.approachKnown && f.approachIncomplete)", walk_back)

    def test_stuck_walk_back_is_rescued_before_the_pause(self):
        walk = body(POLICY, "inline bool WalkTo(Run& r, const Facts& f, float x, float y, float z, bool direct,", "\n}\n")
        self.assertLess(walk.index("if (WalkBackStuckRescue(r, f, dist, x, y, z))"),
                        walk.index('Pause(r, "the lead bot is stuck "'))

    def test_retreat_key_is_read_with_the_config(self):
        config = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
        header = (ROOT / "ai/playerbot/PlayerbotAIConfig.h").read_text()
        template = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
        keys = body(config, "void PlayerbotAIConfig::LoadDungeonRunKeys()\n{", "\n}\n")
        self.assertIn('dungeonRunRetreatAt2Dead = config.GetBoolDefault("AiPlayerbot.DungeonRunRetreatAt2Dead", false);',
                      keys)
        self.assertIn("bool dungeonRunRetreatAt2Dead;", header)
        self.assertIn("\nAiPlayerbot.DungeonRunRetreatAt2Dead = 0\n", template)
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        self.assertIn("f.retreatAt2Dead = sPlayerbotAIConfig.dungeonRunRetreatAt2Dead;", facts)
        self.assertIn("bool retreatAt2Dead = false;", POLICY)
        for doc in ("docs/server/settings/dungeons-raids.md", "docs/play/dungeon-run-leader.md"):
            self.assertIn("AiPlayerbot.DungeonRunRetreatAt2Dead", (ROOT / doc).read_text())

    def test_round_one_keys_are_read_with_the_config(self):
        config = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
        header = (ROOT / "ai/playerbot/PlayerbotAIConfig.h").read_text()
        template = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
        keys = body(config, "void PlayerbotAIConfig::LoadDungeonRunKeys()\n{", "\n}\n")
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        for key, member, fact in (("WalkBackRescue", "dungeonRunWalkBackRescue", "walkBackRescue"),
                                  ("FightForward", "dungeonRunFightForward", "fightForward"),
                                  ("LightRest", "dungeonRunLightRest", "lightRest")):
            self.assertIn('%s = config.GetBoolDefault("AiPlayerbot.DungeonRun%s", true);' % (member, key), keys)
            self.assertIn("bool %s;" % member, header)
            self.assertIn("\nAiPlayerbot.DungeonRun%s = 1\n" % key, template)
            self.assertIn("f.%s = sPlayerbotAIConfig.%s;" % (fact, member), facts)
            self.assertIn("bool %s = true;" % fact, POLICY)
            for doc in ("docs/server/settings/dungeons-raids.md", "docs/play/dungeon-run-leader.md"):
                self.assertIn("AiPlayerbot.DungeonRun%s" % key, (ROOT / doc).read_text())
        self.assertIn("ReadTimeSinkWays(slot, lead, f);", facts)

    def test_fight_forward_and_shortcut_reads(self):
        reads = body(COMMANDS, "void ReadTimeSinkWays(", "\n}\n")
        self.assertIn("f.forwardPathOk = DirectNavPath(lead, pull.x, pull.y, pull.z, way);", reads)
        self.assertIn("f.forwardLos = lead->IsWithinLOS(pull.x, pull.y, pull.z + 1.0f, true, 0.0f);", reads)
        self.assertIn("GetAttackDistance(lead) + kForwardAggroMarginYd;", reads)
        self.assertIn("<= kHopPullYd;", reads)
        self.assertIn("f.shortcutOk = DirectNavPath(lead, p.x, p.y, p.z, way);", reads)
        direct = body(COMMANDS, "bool DirectNavPath(", "\n}\n")
        self.assertIn("PathEndsShort(", direct)
        self.assertIn("PathFitsStraight(length, DistanceTo(lead, x, y, z))", direct)

    def test_retreat_runs_before_the_guard(self):
        tick = body(POLICY, "inline std::string TickBody(Run& r, const Facts& f, std::uint32_t dt, const RestLines& lines)\n{",
                    "\n}\n")
        self.assertLess(tick.index("if (RetreatTick(r, f, *s, dt, order))"), tick.index("if (CheckGuard(r, f, dt, order))"))
        self.assertIn("EndRetreat(r);", body(POLICY, "inline std::string Tick(Run& r, const Facts& in,", "\n}\n"))


if __name__ == "__main__":
    unittest.main()
