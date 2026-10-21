"""The island escape's glue (Zul'Farrak r3-19z20, DungeonRunPolicy.h IslandEscape).

The tank stood 13 minutes on a 3 x 3 yd navmesh island while the walk back
passed every point. The pure rules (the island reading and its second sign,
the ring spot, the straight line's check) are tested in
tools/test_dungeon_run_policy.cpp; this pins where commands/BotCommands.cpp
calls them, so the glue cannot drift away silently:
- the run's "escape ..." order never goes out as a `cmd`: IssueRunOrder hands
  it to RunIslandEscape, whose sscanf formats match the order the policy writes;
- RunIslandEscape refuses with "wait: ..." (not a try) when the lead bot moved
  since the read or a party bot is in combat, the combat check right before the
  first change to the lead bot;
- every spot passes the straight line's check (floor, liquid, hostiles:
  CheckEscapeSegment) before the lead bot is put on stay, reset or moved;
- the island's facts come from the approach hop's own path, and the second
  sign (a) is read only after a reading said island.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
POLICY = (ROOT / "runtime/DungeonRunPolicy.h").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


def at(text, needle):
    i = text.find(needle)
    if i < 0:
        raise AssertionError("missing: " + needle)
    return i


class IslandEscapeWiringTest(unittest.TestCase):
    def setUp(self):
        self.escape = body(COMMANDS, "bool RunIslandEscape(Player* commander, std::string const& order, std::string& answer)\n{",
                           "\n}\n")

    def test_issue_run_order_hands_the_escape_to_the_glue(self):
        f = body(COMMANDS, "bool IssueRunOrder(", "\n}\n")
        route = at(f, 'if (text.compare(0, 7, "escape ") == 0)\n        return RunIslandEscape(commander, text, answer);')
        cmd = at(f, "HandleCommanderOrder(&handler, text.c_str());")
        self.assertLess(route, cmd)

    def test_order_text_matches_the_policy(self):
        f = self.escape
        at(f, '"escape from %f %f %f to %f %f %f toward %f %f %f"')
        at(f, '"escape from %f %f %f toward %f %f %f"')
        p = body(POLICY, "inline void IslandEscape(", "\n}\n")
        at(p, 'order = "escape from " + Num(f.leadX) + " " + Num(f.leadY) + " " + Num(f.leadZ) + " " +')
        at(p, '(good ? "to " + Num(r.goodLeadX)')
        at(p, '"toward " + Num(tx) + " " + Num(ty) + " " + Num(tz);')

    def test_checks_come_before_the_first_change_to_the_lead_bot(self):
        f = self.escape
        moved = at(f, "if (ai::PlanDistance(lx, ly, lz, fx, fy, fz) > kEscapeMovedYd)")
        good = at(f, "bool found = good && GoodSpotFits(lx, ly, lz, gx, gy, gz) && fits(gx, gy, ground) && lineFits(gx, gy, ground);")
        ring = at(f, "if (!lineFits(px, py, pz))")
        none = at(f, 'answer = "no-spot: no spot within "')
        fight = at(f, "bool fight = lead->IsInCombat();")
        supersede = at(f, "SupersedeCommanderTasks(commander->GetObjectGuid().GetRawValue(), 0, RemoteCommander::LaterOrder::Move);")
        drop = at(f, "leadAi->DropQueuedMovementOrders();")
        reset = at(f, "leadAi->Reset();")
        stay = at(f, 'leadAi->SetMovementStrategy("stay");')
        anchor = at(f, "SetStayAnchor(leadAi, x, y, ground, lead->GetMapId());")
        clear = at(f, "lead->GetMotionMaster()->Clear();")
        move = at(f, "lead->GetMotionMaster()->MovePoint(0, x, y, ground, MOVE_RUN_MODE);")
        self.assertTrue(moved < good < ring < none < fight < supersede < drop < reset < stay < anchor < clear < move)
        # Nothing touches the lead bot's movement before the checks.
        self.assertEqual(f.count("GetMotionMaster()"), 2)
        self.assertEqual(f.count("leadAi->Reset()"), 1)

    def test_wait_refusals(self):
        f = self.escape
        self.assertRegex(f, r"> kEscapeMovedYd\)\s*\{\s*answer = \"wait: ")
        self.assertRegex(f, r"for \(Player\* bot : CommandedBots\(group, lead\)\)\s*fight = fight \|\| \(bot && bot->IsInWorld\(\) && bot->IsAlive\(\) && bot->IsInCombat\(\)\);")
        self.assertRegex(f, r"if \(fight\)\s*\{\s*answer = \"wait: ")
        p = body(POLICY, "inline void OrderRefused(", "\n}\n")
        at(p, 'bool const wait = answer.compare(0, 4, "wait") == 0;')

    def test_the_line_check_reads_floor_liquid_and_hostiles(self):
        f = body(self.escape, "auto lineFits = ", "\n    };\n")
        samples = at(f, "EscapeSegmentSamples(lx, ly, lz, px, py, pz)")
        height = at(f, "map->GetHeight(sample.x, sample.y, sample.z + kSegmentGroundYd, true,")
        liquid = at(f, "getLiquidStatus(sample.x, sample.y, floor + 0.5f, MAP_ALL_LIQUIDS,")
        at(f, "!= LIQUID_MAP_NO_WATER;")
        hostiles = at(f, "HostileCreaturesAround(lead, GroundDistance(lx, ly, px, py) + kEscapeHostileYd, hostiles);")
        verdict = at(f, "return CheckEscapeSegment(lz, samples, hostileNear) == SegmentVerdict::Ok;")
        self.assertTrue(samples < height < liquid < hostiles < verdict)

    def test_ring_spots_lead_on(self):
        at(self.escape, "if (!RingSpotLeadsOn((probeType & PATHFIND_NOPATH) != 0, (probeType & PATHFIND_INCOMPLETE) != 0,")

    def test_island_facts(self):
        f = body(COMMANDS, "bool ApproachOnPath(", "\n}\n")
        note = at(f, "NoteLeadWay(path, lead, x, y, z, *way);")
        nopath = at(f, "if (path.getPathType() & PATHFIND_NOPATH)")
        self.assertLess(note, nopath)
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        at(facts, "if (f.leadIsland && lead->IsAlive())\n        f.goodSpotCut = GoodSpotCutOff(lead, slot.run);")

    def test_a_move_point_in_sight_is_read_forward_too(self):
        # r6-19z24: the forward run's step 64/65 points were in sight (6-9 yd)
        # and nothing read the lead bot's path to them; the reading is not tied
        # to a walk back any more, and MoveStep asks IslandConfirmed before its walk.
        facts = body(COMMANDS, "void ReadRunFacts(", "\n}\n")
        read = at(facts, "else if (step->kind == ai::PlanStepKind::Move && slot.run.phase != Phase::Walk &&\n"
                         "                !slot.run.regroup && lead->IsAlive() && lead->GetMapId() == slot.run.plan->map && toPoint > kArriveYd)")
        note = at(facts, "NoteLeadWay(path, lead, step->x, step->y, step->z, f);")
        self.assertLess(read, note)
        self.assertNotIn("slot.run.walkBack && step->kind == ai::PlanStepKind::Move && slot.run.phase", facts)
        move = body(POLICY, "inline void MoveStep(", "\n}\n")
        island = at(move, "IslandConfirmed(r, f, s.x, s.y, s.z)")
        self.assertLess(island, at(move, "WalkTo(r, f, s.x, s.y, s.z, s.kind == PlanStepKind::MoveDirect, dt, order)"))
        self.assertLess(island, at(move, "if (r.detour)"))

    def test_the_detour_candidate_leads_on(self):
        # r6-19z24: NearestReachablePoint took ring points whose partial paths
        # ended on the lead bot's island; every candidate goes through the pure rule.
        f = body(COMMANDS, "bool NearestReachablePoint(", "\n}\n")
        check = at(f, "if (!TortoiseBots::DungeonRun::DetourPointReachable((type & PATHFIND_NOPATH) != 0,\n"
                      "                (type & PATHFIND_INCOMPLETE) != 0, ai::PlanDistance(end.x, end.y, end.z, px, py, ground),\n"
                      "                DistanceTo(lead, end.x, end.y, end.z)))\n            return false;")
        self.assertLess(check, at(f, "ox = px;"))
        self.assertNotIn("> 5.0f)", f)
        p = body(POLICY, "inline bool DetourPointReachable(", "\n}\n")
        at(p, "RingSpotLeadsOn(pathNoPath, pathIncomplete, endFromLeadYd, endFromLeadYd)")


if __name__ == "__main__":
    unittest.main()
