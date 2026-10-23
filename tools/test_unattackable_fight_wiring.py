"""The run leader's glue reads which units the group can attack.

Gnomeregan r21-19z40e step 180: the Alarm-a-bomb 2600 (7897) kept the party in
combat, every `attack` on it was refused invalid-target ("The group cannot
attack that target."), and "a fight lasted more than 180 s" paused the run four
times. runtime/DungeonRunPolicy.h treats a fight with only units the group
cannot attack as no fight (UnattackableOnlyFight) and never pulls, picks up,
engages or focuses such a unit (UnitFact::attackable). This test pins that
commands/BotCommands.cpp feeds those rules: the same IsValidAttackTarget test
`cmd attack` refuses on, judged by the same bot, set on every unit fact the
run picks from, and the engaged units read on every tick in combat and for the
ceiling pause's self-resume.
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


class UnattackableFightWiringTest(unittest.TestCase):
    def test_the_check_is_the_attack_orders_own(self):
        # `cmd attack` refuses NotAttackable on the judge's IsValidAttackTarget.
        self.assertIn("facts.attackable = order.judge->IsValidAttackTarget(unit);", CODE)
        can = body(CODE, "bool GroupCanAttack(Player* judge, Unit* unit)")
        self.assertIn("judge->IsValidAttackTarget(unit)", can)
        # The judge is picked as CommanderOrder::judge is: the lead bot, else a living bot on its map; a charmed
        # bot or one not friendly to the lead bot never judges (test_boss_pair_wiring pins RunJudgeFit).
        judge = body(CODE, "Player* RunJudge(Player* lead, std::vector<Player*> const& bots)")
        self.assertIn("if (RunJudgeFit(lead, lead))\n        return lead;", judge)
        self.assertIn("if (RunJudgeFit(lead, bot))\n            return bot;", judge)
        fit = body(CODE, "bool RunJudgeFit(Player* lead, Player* bot)")
        self.assertIn("!bot->IsAlive() || !bot->IsInWorld() || bot->GetMap() != lead->GetMap()", fit)

    def test_engaged_units_feed_the_stale_test_and_the_fight_rule(self):
        read = body(CODE, "void ReadEngagedUnits(Player* lead, std::vector<Player*> const& bots,")
        self.assertIn("fact.attackable = GroupCanAttack(judge, other);", read)
        self.assertIn("f.engagedUnits.push_back(fact);", read)
        self.assertIn("f.engagedRead = true;", read)
        # Both a bot's and its pet's attackers and victims, as the stale test read them.
        self.assertIn("for (Unit* attacker : unit->GetAttackers())", read)
        self.assertIn("add(unit->GetVictim());", read)
        self.assertIn("read(pet);", read)
        self.assertIn("sServerFacade.IsFriendlyTo(other, lead)", read)
        facts = body(CODE, "void ReadRunFacts(DungeonRunSlot const& slot, Player* lead,")
        self.assertIn("ReadEngagedUnits(lead, bots, f);\n        f.staleCombat = f.engagedUnits.empty();", facts)

    def test_every_picked_unit_carries_attackable(self):
        facts = body(CODE, "void ReadRunFacts(DungeonRunSlot const& slot, Player* lead,")
        self.assertIn("Player* const judge = RunJudge(lead, bots);", facts)
        # The step's units (pull/boss, and a hold's), and the hop units.
        self.assertEqual(facts.count("unit.attackable = GroupCanAttack(judge, creature);"), 2)
        self.assertIn("fact.attackable = GroupCanAttack(judge, unit);\n                if (f.pullStand)", facts)
        guard = body(CODE, "void ReadGuardFacts(Player* lead, std::vector<Player*> const& bots,")
        self.assertIn("fact.attackable = GroupCanAttack(lead, nearestHit);", guard)
        self.assertIn("fact.attackable = GroupCanAttack(lead, attacker);", guard)
        kill = body(CODE, "void ReadKillFirst(ai::DungeonPlan const& plan, Player* lead,")
        self.assertIn("fact.attackable = GroupCanAttack(judge, unit);", kill)

    def test_ceiling_self_resume_counts_unattackable_combat_as_quiet(self):
        resume = body(CODE, "bool partyInCombat = false;", "CeilingFightOverWhilePaused(slot.run, partyInCombat, dt)")
        self.assertIn("ReadEngagedUnits(lead, pausedBots, engaged);", resume)
        self.assertIn("partyInCombat = TortoiseBots::DungeonRun::FightOn(slot.run, engaged, dt);", resume)


if __name__ == "__main__":
    unittest.main()
