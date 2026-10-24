"""The lead bot in another part of the instance: the glue (Scarlet Monastery r20-19z39 slot 7).

The pure rules (LeadInOtherPart, PauseOtherPart, HoldStandsNow, the near-wipe
hold, no summon or island hop toward such a lead bot) are tested in
tools/test_dungeon_run_policy.cpp; this pins where commands/BotCommands.cpp
feeds and obeys them, since that file only builds against the core:
- ReadRunFacts gives the policy the instance's doors (Facts::doors): the
  destination of every area trigger onto the plan's map, from a per-map cache;
- SyncRunHold keeps the held bots on stay while HoldStandsNow (running, or the
  pause for a lead bot in another part), and the paused tick does not release
  them then (their follow would walk them out through the instance's exits).
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
POLICY = (ROOT / "runtime/DungeonRunPolicy.h").read_text()


def body(text, start, end):
    i = text.index(start)
    j = text.index(end, i + len(start))
    return text[i:j]


class LeadOtherPartWiringTest(unittest.TestCase):
    def test_doors_are_the_trigger_destinations_onto_the_map(self):
        doors = body(COMMANDS, "InstanceDoors(uint32 mapId)", "void ReadRunFacts(")
        self.assertIn("at->destination.mapId != mapId", doors)
        for axis in "xyz":
            self.assertIn(f"door.{axis} = at->destination.{axis};", doors)
        self.assertIn("static std::unordered_map<uint32, std::vector<TortoiseBots::DungeonRun::DoorFact>> cache;", doors)

    def test_read_run_facts_fills_the_doors_for_a_living_lead(self):
        facts = body(COMMANDS, "void ReadRunFacts(", "f.leadX = lead->GetPositionX();")
        self.assertIn("f.doors = InstanceDoors(slot.run.plan->map);", facts)
        self.assertIn("if (f.leadAlive)", facts)

    def test_the_hold_stands_while_paused_for_it(self):
        sync = body(COMMANDS, "void SyncRunHold(", "void SyncHoldStands(")
        self.assertIn("TortoiseBots::DungeonRun::HoldStandsNow(slot.run)", sync)
        self.assertNotIn("slot.run.state == TortoiseBots::DungeonRun::State::Running &&", sync)
        tick = body(COMMANDS, "void TickDungeonRun(", "Player* lead = FindCharacter(binding->leadGuid);\n    Group* group")
        paused = body(tick, "if (slot.run.state != TortoiseBots::DungeonRun::State::Running || !commander)", "return;")
        self.assertIn("if (!(commander && TortoiseBots::DungeonRun::HoldStandsNow(slot.run)))", paused)
        self.assertIn("ReleaseRunHold(commander, binding->leadGuid, slot);", paused)

    def test_policy_pause_clears_the_flag(self):
        pause = body(POLICY, "inline void Pause(Run& r, const std::string& why)", "inline bool HoldStandsNow(")
        self.assertIn("r.holdWhilePaused = false;", pause)


    def test_resume_is_refused_while_the_lead_is_still_away(self):
        # SM-REVIVE-PATH-2026-10-23 fix 5: `.bot run resume` (the agent's dungeon_resume) asks
        # ResumeChecked with the lead bot as it stands, and a refusal answers ACTION_ERR and keeps the pause.
        resume = body(COMMANDS, 'else if (verb == "resume")', "else // skip")
        self.assertNotIn("done = Resume(slot.run);", resume)
        self.assertIn("done = ResumeChecked(slot.run, spot, refusal);", resume)
        self.assertIn("Player* lead = FindCharacter(binding->leadGuid)", resume)
        self.assertIn("if (lead->IsInWorld())", resume)
        self.assertIn("spot.read = true;", resume)
        self.assertIn("spot.alive = lead->IsAlive();", resume)
        self.assertIn("spot.onPlanMap = lead->GetMapId() == slot.run.plan->map;", resume)
        for axis in "XYZ":
            self.assertIn(f"spot.{axis.lower()} = lead->GetPosition{axis}();", resume)
        refused = body(resume, "if (!refusal.empty())", "}")
        self.assertIn('SendActionError(handler, intent, "lead-other-part", refusal.c_str());', refused)
        self.assertIn("return true;", refused)

    def test_policy_pause_marks_and_resume_clears_the_other_part_pause(self):
        other = body(POLICY, "inline void PauseOtherPart(", "\n}\n")
        self.assertIn("r.otherPartPaused = true;", other)
        self.assertGreater(other.index("r.otherPartPaused = true;"), other.index("Pause(r, "))
        self.assertIn("r.otherPartPaused = false;", body(POLICY, "inline void Pause(Run& r, const std::string& why)", "\n}\n"))
        self.assertIn("r.otherPartPaused = false;", body(POLICY, "inline bool Resume(Run& r)", "\n}\n"))

    def test_revived_in_another_part_is_rescued(self):
        # Dire Maul West r36 slot D: the rescue teleport before the pause, at both places the pause was the only answer.
        target = body(POLICY, "inline std::size_t OtherPartRescueTarget(const Run& r, const Facts& f)", "\n}\n")
        self.assertIn("if (!f.players.empty() || f.dead > 0 || !f.leadAlive || !RescueAllowed(r, f))", target)
        self.assertIn("std::size_t const last = LastReachedBefore(r, r.step);", target)
        self.assertIn("return SafePointAtOrBefore(r, f.team, last);", target)
        rescue = body(POLICY, "inline bool OtherPartRescue(Run& r, const Facts& f)", "\n}\n")
        self.assertLess(rescue.index("RescueDue(r, f, target);"), rescue.index("r.strandBotOnly = true;"))
        revived = body(POLICY, "inline bool LeadRevivedAway(Run& r, const Facts& f)", "\n}\n")
        other = body(revived, "if (LeadInOtherPart(r, f))", "if (r.autoRegroups >= kAutoRegroupsPerStep)")
        self.assertLess(other.index("RearmRescueAfterDeath(r);"), other.index("OtherPartRescueTarget(r, f)"))
        self.assertIn("if (r.autoRegroups < kAutoRegroupsPerStep && OtherPartRescueTarget(r, f) != "
                      "static_cast<std::size_t>(-1))", other)
        self.assertLess(other.index("++r.autoRegroups;"), other.index("OtherPartRescue(r, f);"))
        self.assertLess(other.index("ResetStep(r);"), other.index("OtherPartRescue(r, f);"))
        self.assertLess(other.index("OtherPartRescue(r, f);"), other.index("PauseOtherPart(r, f);"))
        regroup = body(POLICY, "if (r.regroup && !runBackWait && !f.anyInCombat)", "if (r.holdDue && f.leadAlive)")
        site = body(regroup, "if (LeadInOtherPart(r, f))", "PauseOtherPart(r, f);")
        self.assertIn("if (OtherPartRescue(r, f))", site)
        for line in ("r.regroup = false;", "r.waitFor.clear();", "r.walkBackDue = false;", "r.hold.clear();"):
            self.assertIn(line, site)

if __name__ == "__main__":
    unittest.main()
