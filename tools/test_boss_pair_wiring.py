"""The run leader's glue for the Scarlet Cathedral Mograine / Whitemane fixes (SM-CATHEDRAL-MOGRAINE-2026-10-23).

r23 slot 5: Whitemane's Dominate Mind made the judge bot Hele (the lead and two more were dead), so the run read
Whitemane as "cannot be attacked" and listed Hele as an engaged hostile; the boss step for Mograine paused "not
found" three times while his corpse lay at the door 290 yd away; and Whitemane, held at 1 hp by her script, was
fought by the same resumed fight four times. runtime/DungeonRunPolicy.h decides on the facts (EngagedPlanBossOrHold,
the boss search in FightStep, HpHeldTick, ResumeChecked); this test pins that commands/BotCommands.cpp feeds them.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


CODE = strip_comments(COMMANDS)


class BossPairWiringTest(unittest.TestCase):
    def test_a_charmed_bot_never_judges(self):
        fit = body(CODE, "bool RunJudgeFit(Player* lead, Player* bot)")
        self.assertIn("bot->IsCharmed()", fit)
        self.assertIn("return bot == lead || lead->IsCharmed() || sServerFacade.IsFriendlyTo(bot, lead);", fit)
        judge = body(CODE, "Player* RunJudge(Player* lead, std::vector<Player*> const& bots)")
        self.assertNotIn("lead->IsAlive()", judge)

    def test_party_members_and_pets_are_never_engaged_units(self):
        read = body(CODE, "void ReadEngagedUnits(Player* lead, std::vector<Player*> const& bots,")
        self.assertIn("party.insert(bot->GetObjectGuid().GetRawValue());", read)
        self.assertIn("party.insert(pet->GetObjectGuid().GetRawValue());", read)
        self.assertIn("party.count(other->GetObjectGuid().GetRawValue())", read)
        # The party set is filled before any unit is added.
        self.assertLess(read.index("party.insert(bot->"), read.index("for (Unit* attacker : unit->GetAttackers())"))

    def test_engaged_units_carry_hp_and_party_hits(self):
        read = body(CODE, "void ReadEngagedUnits(Player* lead, std::vector<Player*> const& bots,")
        self.assertIn("fact.health = other->GetHealth();", read)
        self.assertIn("fact.partyHits = victims.count(fact.guid) != 0;", read)
        self.assertIn("victims.insert(victim->GetObjectGuid().GetRawValue());", read)
        self.assertIn("if (Unit* victim = pet->GetVictim())", read)
        self.assertIn("fact.evading = other->IsCreature() && static_cast<Creature*>(other)->IsInEvadeMode();", read)

    def test_boss_search_reads_the_instance_when_asked(self):
        facts = body(CODE, "void ReadRunFacts(DungeonRunSlot const& slot, Player* lead,")
        self.assertIn("if (step->kind == ai::PlanStepKind::Boss && slot.run.bossSearchWanted && "
                      "slot.run.step < slot.run.plan->steps.size() &&\n                step == &slot.run.plan->steps[slot.run.step])\n"
                      "                ReadBossInInstance(*step, lead, f);", facts)
        search = body(CODE, "void ReadBossInInstance(ai::PlanStep const& step, Player* lead,")
        self.assertIn("f.bossSearchRead = true;", search)
        self.assertIn("for (uint32 low : BossSpawnLowGuids(lead->GetMapId(), step.entry))", search)
        self.assertIn("map->GetCreature(data->GetObjectGuid(low))", search)
        self.assertIn("f.bossSearchAlive = true;", search)
        self.assertIn("f.bossSearchDead = true;", search)
        self.assertNotIn("GetCreatureListWithEntryInGrid", search)
        spawns = body(CODE, "std::vector<uint32> const& BossSpawnLowGuids(uint32 mapId, uint32 entry)")
        self.assertIn("std::lock_guard<std::mutex> guard(lock);", spawns)
        self.assertIn("if (it != cache.end())\n        return it->second;", spawns)
        self.assertIn("sObjectMgr.DoCreatureData(worker);", spawns)

    def test_resume_of_a_unit_that_cannot_die_is_refused(self):
        resume = body(CODE, "    else if (verb == \"resume\")", "        done = Skip(slot.run);")
        self.assertIn("bool const hpHeld = slot.run.hpHeldPaused;\n        done = ResumeChecked(slot.run, spot, refusal);", resume)
        self.assertIn('SendActionError(handler, intent, "unit-cannot-die", refusal.c_str());', resume)


if __name__ == "__main__":
    unittest.main()
