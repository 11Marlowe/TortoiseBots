"""The magma guard's glue (RFC r3-19z20, MagmaHazardPolicy.h).

The lead tank's idle formation station walk took it into Ragefire Chasm's
lava basin and the party followed (5 deaths). The pure rules are tested in
tools/test_magma_hazard_policy.cpp; this pins where the movement layer and the
run leader call them, so the glue cannot drift away silently:
- MoveTo2 checks the refusal memory first, then the end point, then the
  route, each refusal returns false with MOVE_FAIL_MAGMA, and a route into
  magma is refused whole (nothing dispatched, no hop to its dry part);
- a route refusal is remembered from the bot's origin cell only, and logged
  to the module log;
- a commander task's walk nudge (a plain core MovePoint) waits after a magma
  refusal and never walks onto a stand in magma;
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
        self.assertRegex(end, r"^[^}]*RefuseMagmaMove\(bot, lastMove, endPos, endPos, nullptr, \"end point\"\);\s*return false;")
        route = f[at(f, "RouteEntersMagma(bot, startPos, movePath, magmaAt)"):]
        self.assertRegex(route, r"^[^}]*RefuseMagmaMove\(bot, lastMove, endPos, magmaAt, &startPos, \"route\"\);\s*return false;")

    def test_refusal_sets_memory_and_reason(self):
        f = body(MOVES, "static void RefuseMagmaMove(", "\n}\n")
        self.assertIn("lastMove.magmaRefusal = origin", f)
        self.assertIn("? ai::MagmaRouteRefusalFor(endPos.GetMapId()", f)
        self.assertIn("origin->getX(), origin->getY(), origin->getZ(), now)", f)
        self.assertIn(": ai::MagmaRefusalFor(endPos.GetMapId()", f)
        self.assertIn("lastMove.moveFailReason = MOVE_FAIL_MAGMA;", f)

    def test_memory_is_keyed_on_the_bots_origin(self):
        f = self.move2
        self.assertIn("bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), static_cast<std::int64_t>(time(nullptr))))",
                      f[at(f, "ai::MagmaRefusalActive("):])

    def test_refusal_logs_to_the_module_log(self):
        f = body(MOVES, "static void LogMagmaRefusal(", "\n}\n")
        self.assertIn("TB_LOG_DETAIL(", f)
        self.assertIn('#include "host/ModuleLog.h"', MOVES)

    def test_task_walk_nudge_waits_after_a_magma_refusal(self):
        f = body(COMMANDS, "void NudgeTaskWalk(CommanderTask& task, Player* bot, uint32 diff)", "\n}\n")
        blocked = at(f, "if (ai::MagmaNudgeBlocked(guard, lastMove.moveFailReason == ai::MOVE_FAIL_MAGMA, lastMove.magmaRefusal,")
        self.assertIn("bool const guard = ai::MagmaGuardApplies(bot->GetMap()->IsDungeon());", f)
        self.assertIn("guard && ai::WorldPosition(bot->GetMapId(), task.stand.x, task.stand.y, task.stand.z).isInMagma(", f)
        walk = at(f, "bot->GetMotionMaster()->MovePoint(0, task.stand.x, task.stand.y, task.stand.z,")
        self.assertLess(blocked, walk)
        self.assertEqual(f.count("MovePoint("), 1)  # the guarded walk is the only one
        self.assertRegex(f[blocked:walk], r"isInMagma\(\s*bot->GetMap\(\)->GetTerrain\(\)\)\)\)\s*return;")

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
        self.assertIn("request.direct ? RunMagmaNote::DirectMovePoint : RunMagmaNote::MovePoint,", f)
        direct = at(f, "return CommanderMoveDirect(handler, order, request.point, intent);")
        self.assertLess(note, direct)
        helper = body(COMMANDS, "static void NoteRunMagma(", "\n}\n")
        self.assertIn("time_t& last = lastLog[static_cast<size_t>(kind)];", helper)
        self.assertNotIn("return true", helper)
        self.assertNotIn("SendActionError", helper)


if __name__ == "__main__":
    unittest.main()
