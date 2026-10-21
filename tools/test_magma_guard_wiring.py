"""The magma guard's glue (RFC r3-19z20, MagmaHazardPolicy.h).

The lead tank's idle formation station walk took it into Ragefire Chasm's
lava basin and the party followed (5 deaths). The pure rules are tested in
tools/test_magma_hazard_policy.cpp; this pins where the movement layer and the
run leader call them, so the glue cannot drift away silently:
- MoveTo2 checks the refusal memory first, then the end point, then the
  route, each refusal returns false with MOVE_FAIL_MAGMA, and a route into
  magma is refused whole (nothing dispatched, no hop to its dry part);
- the formation station trigger does not fire for a slot in magma;
- the run leader holds the lead on "stay" while resting, unless it stands in
  magma, before the rest order goes out.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
MOVES = (ROOT / "ai/playerbot/strategy/actions/MovementActions.cpp").read_text()
TRIGGERS = (ROOT / "ai/playerbot/strategy/triggers/RangeTriggers.h").read_text()
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
WORLDPOS = (ROOT / "ai/playerbot/WorldPosition.cpp").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


def at(text, needle):
    i = text.find(needle)
    if i < 0:
        raise AssertionError("missing: " + needle)
    return i


class MagmaGuardWiringTest(unittest.TestCase):
    def setUp(self):
        self.move2 = body(MOVES, "bool MovementAction::MoveTo2(", "\n}\n")

    def test_order_in_move_to2(self):
        f = self.move2
        guard = at(f, "bool const magmaGuard = ai::MagmaGuardApplies(bot->GetMap()->IsDungeon());")
        memory = at(f, "ai::MagmaRefusalActive(lastMove.magmaRefusal, endPos.GetMapId()")
        end = at(f, "if (endPos.isInMagma())")
        clip = at(f, "movePath.ClipPath(ai, mover, ignoreEnemyTargets, packHold);")
        route = at(f, "RouteEntersMagma(bot, startPos, movePath, magmaAt)")
        dispatch = at(f, "DispatchMovement(movePath, generatePath, masterWalking)")
        teleport = at(f, "bot->TeleportTo(teleportPosition.GetMapId(),")
        self.assertTrue(guard < memory < end < clip < route < teleport < dispatch)

    def test_each_refusal_returns_false_with_magma(self):
        f = self.move2
        memory = f[at(f, "ai::MagmaRefusalActive("):]
        self.assertRegex(memory, r"^[^}]*lastMove\.moveFailReason = MOVE_FAIL_MAGMA;\s*return false;")
        end = f[at(f, "if (endPos.isInMagma())"):]
        self.assertRegex(end, r"^[^}]*RefuseMagmaMove\(bot, lastMove, endPos, endPos, \"end point\"\);\s*return false;")
        route = f[at(f, "RouteEntersMagma(bot, startPos, movePath, magmaAt)"):]
        self.assertRegex(route, r"^[^}]*RefuseMagmaMove\(bot, lastMove, endPos, magmaAt, \"route\"\);\s*return false;")

    def test_refusal_sets_memory_and_reason(self):
        f = body(MOVES, "static void RefuseMagmaMove(", "\n}\n")
        self.assertIn("lastMove.magmaRefusal = ai::MagmaRefusalFor(endPos.GetMapId()", f)
        self.assertIn("lastMove.moveFailReason = MOVE_FAIL_MAGMA;", f)

    def test_route_check_never_edits_the_route(self):
        f = body(MOVES, "static bool RouteEntersMagma(", "\n}\n")
        self.assertIn("ai::CutPathBeforeMagma(start, route,", f)
        self.assertIn(".isInMagma(terrain)", f)
        for edit in ("erase(", "addPoint(", "push_back(PathNodePoint", "points.clear("):
            self.assertNotIn(edit, f)

    def test_magma_query_is_the_cores(self):
        f = body(WORLDPOS, "bool WorldPosition::isInMagma(", "\n}\n")
        self.assertIn("getLiquidStatus(x, y, z + 0.01f, MAP_ALL_LIQUIDS, &liquid)", f)
        self.assertIn("ai::IsMagmaHazard(liquid.type_flags", f)

    def test_formation_station_skips_a_slot_in_magma(self):
        f = body(TRIGGERS, "class FormationStationTrigger", "\n    };\n")
        wanted = at(f, "if (!FormationStationWanted(distance))")
        magma = at(f, "!(MagmaGuardApplies(bot->GetMap()->IsDungeon()) && WorldPosition(loc).isInMagma())")
        self.assertLess(wanted, magma)

    def test_run_leader_holds_the_lead_while_resting(self):
        f = body(COMMANDS, "void TickDungeonRun(uint64 commanderRaw, uint32 dt)", "\n}\n")
        tick = at(f, "TortoiseBots::DungeonRun::Tick(slot.run, facts, dt);")
        due = at(f, "TortoiseBots::DungeonRun::LeadStayDueWhileResting(slot.run, lead->IsAlive(), lead->IsInCombat(),")
        issue = at(f, "IssueRunOrder(commander, order, answer,")
        self.assertTrue(tick < due < issue)
        hold = f[due:issue]
        self.assertIn('leadAi->HasStrategy("stay", BotState::BOT_STATE_NON_COMBAT), lead->IsInMagma()))', hold)
        self.assertIn("FormUpOnLead(commander, lead, std::vector<Player*>{ lead }, true);", hold)
        self.assertIn("leadAi->StopMoving();", hold)

    def test_run_moves_are_logged_not_refused(self):
        f = body(COMMANDS, "static bool CommanderMove(ChatHandler* handler, CommanderOrder const& order, std::string const& args,",
                 "\n}\n")
        note = at(f, "NoteRunMagma(order.lead, request.point.x, request.point.y, request.point.z,")
        direct = at(f, "return CommanderMoveDirect(handler, order, request.point, intent);")
        self.assertLess(note, direct)
        helper = body(COMMANDS, "static void NoteRunMagma(", "\n}\n")
        self.assertNotIn("return true", helper)
        self.assertNotIn("SendActionError", helper)


if __name__ == "__main__":
    unittest.main()
