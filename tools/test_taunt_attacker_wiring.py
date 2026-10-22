"""Tank taunts the attacker of a non-tank (PLAN-SPELL-AUDIT F1, 2026-10-21).

33 dungeon runs: 31 of 53 priest deaths had a mob on the priest in the 5 s
before, only 8 saw a Taunt. "lose aggro" reads only the tank's current target
and "taunt on snare target" only a mob still running. The live protection
strategy gained two rows on a new value; the pure rule is
ai/playerbot/strategy/values/TauntAttackerPolicy.h (tools/test_taunt_attacker_policy.cpp).

Run: python3 -m unittest tools/test_taunt_attacker_wiring.py
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
STRAT = ROOT / "ai/playerbot/strategy"
WARRIOR = STRAT / "warrior"
VALUES = STRAT / "values"


def body(text, signature):
    """The body of the function or method whose definition starts with `signature`."""
    start = text.index(signature)
    end = text.index("\n}\n", start)
    return text[start:end]


def combat_rows(path, function):
    """(trigger, action, relevance) for every single-action row of one InitCombatTriggers."""
    text = body((WARRIOR / path).read_text(), function)
    pat = re.compile(r'TriggerNode\(\s*"([^"]+)",\s*NextAction::array\(0, new NextAction\("([^"]+)", ([^)]+)\), NULL\)\)')
    return [(m.group(1), m.group(2), m.group(3).strip()) for m in pat.finditer(text)]


class TauntAttackerWiringTest(unittest.TestCase):
    def test_rows_in_the_live_protection_strategy(self):
        rows = combat_rows("ProtectionWarriorStrategy.cpp", "void ProtectionWarriorStrategy::InitCombatTriggers(")
        self.assertIn(("party member attacked", "taunt on attacker", "ACTION_INTERRUPT + 1"), rows)
        self.assertIn(("party member attacked aoe", "challenging shout", "ACTION_INTERRUPT + 2"), rows)
        # The two older taunt rows stay as they were.
        self.assertIn(("lose aggro", "taunt", "ACTION_INTERRUPT + 1"), rows)
        self.assertIn(("taunt on snare target", "taunt on snare target", "ACTION_MOVE"), rows)

    def test_strategy_is_live_and_compiled(self):
        # The protection kit is created from WarriorAiObjectContext (TankWarriorStrategy.cpp is a
        # dead donor file), and the module build globs the warrior and values directories.
        ctx = (WARRIOR / "WarriorAiObjectContext.cpp").read_text()
        self.assertIn('creators["protection pve"] = [](PlayerbotAI* ai) { return new ProtectionWarriorPveStrategy(ai); };', ctx)
        self.assertNotIn("party member attacked", (WARRIOR / "TankWarriorStrategy.cpp").read_text())
        cmake = (ROOT / "TortoiseBots.cmake").read_text()
        self.assertIn('"${TORTOISEBOTS_ROOT}/ai/playerbot/strategy/values/*.cpp"', cmake)
        self.assertRegex(cmake, r"TORTOISEBOTS_CLASS_DIRS\s+druid hunter mage paladin priest rogue shaman warlock warrior\)")

    def test_creators(self):
        ctx = (WARRIOR / "WarriorAiObjectContext.cpp").read_text()
        self.assertIn('creators["party member attacked"] = [](PlayerbotAI* ai) { return new TauntAttackerOfNonTankTrigger(ai); };', ctx)
        self.assertIn('creators["party member attacked aoe"] = [](PlayerbotAI* ai) { return new ChallengingShoutOnAttackersTrigger(ai); };', ctx)
        self.assertIn('creators["taunt on attacker"] = [](PlayerbotAI* ai) { return new CastTauntOnAttackerAction(ai); };', ctx)
        values = (VALUES / "ValueContext.h").read_text()
        self.assertIn('#include "AttackerOfNonTankValue.h"', values)
        self.assertIn('creators["attacker of non-tank"] = [](PlayerbotAI* ai) { return new AttackerOfNonTankValue(ai); };', values)

    def test_action_targets_the_value_and_never_moves(self):
        actions = (WARRIOR / "WarriorActions.h").read_text()
        cls = actions[actions.index("class CastTauntOnAttackerAction"):]
        cls = cls[:cls.index("};")]
        self.assertIn('CastSpellAction(ai, "taunt")', cls)
        self.assertIn('GetTargetName() override { return "attacker of non-tank"; }', cls)
        self.assertIn("GetTargetQualifier() override { return GetSpellName(); }", cls)
        self.assertIn('GetReachActionName() override { return ""; }', cls)
        execute = body((WARRIOR / "WarriorActions.cpp").read_text(), "bool CastTauntOnAttackerAction::Execute(")
        self.assertIn("CastSpellAction::Execute(event)", execute)
        self.assertIn("value->NoteTaunt(target);", execute)

    def test_triggers_gate_on_tank_group_and_cast(self):
        trig = (WARRIOR / "WarriorTriggers.cpp").read_text()
        target = body(trig, "Value<Unit*>* TauntAttackerOfNonTankTrigger::GetTargetValue(")
        self.assertIn('context->GetValue<Unit*>("attacker of non-tank", spell)', target)
        active = body(trig, "bool TauntAttackerOfNonTankTrigger::IsActive(")
        self.assertIn("TauntAttackerGate(ai->IsTank(bot), bot->GetGroup() != nullptr,", active)
        self.assertIn("ai->CanCastSpell(spell, target, true, nullptr, true)", active)
        shout = body(trig, "bool ChallengingShoutOnAttackersTrigger::IsActive(")
        self.assertIn("bot->CanReachWithMeleeAutoAttack(target)", shout)
        self.assertIn('ai->CanCastSpell("challenging shout", target, true, nullptr, true)', shout)
        self.assertIn('CollectNonTankAttackers(ai, "taunt", nullptr, attackers);', shout)
        self.assertIn("ChallengingShoutOnAttackers(facts, ChallengingShoutRadiusYd)", shout)

    def test_value_fills_every_policy_fact(self):
        src = (VALUES / "AttackerOfNonTankValue.cpp").read_text()
        collect = body(src, "void ai::CollectNonTankAttackers(")
        for needle in (
            'GetValue<std::list<ObjectGuid>>("attackers")',
            "victimPlayer->GetGroup() != group",
            "PlayerbotAI::IsTank(victimPlayer)",
            "PlayerbotAI::IsHeal(victimPlayer)",
            "FindNonCcTargetStrategy::IsCrowdControlled(ai, unit)",
            "IsInEvadeMode()",
            "ccGuard.IsHeldCcMark(unit)",
            "CcHeldMarkTauntAllowed(tankTaunt, c.crowdControlled,",
            "bot->IsWithinLOSInMap(unit)",
            "GetThreatManager().getThreat(bot)",
            "GetThreatManager().getThreat(victimPlayer)",
            "TauntedRecently(true, it->second, now)",
            "bot->GetCombatDistance(unit) <= maxRange",
        ):
            self.assertIn(needle, collect)
        calc = body(src, "Unit* AttackerOfNonTankValue::Calculate(")
        self.assertIn("CollectNonTankAttackers(ai, qualifier, &lastTaunt, attackers);", calc)
        self.assertIn("TauntAttackerPick(facts)", calc)

    def test_snare_target_semantics_unchanged(self):
        snare = (VALUES / "SnareTargetValue.cpp").read_text()
        self.assertIn("if (!sServerFacade.isMoving(unit))", snare)
        self.assertIn("case CHASE_MOTION_TYPE:", snare)

    def test_gate_runs_both_tests(self):
        gate = (ROOT / "tools/verify_all.sh").read_text()
        self.assertIn("python3 -m unittest tools/test_taunt_attacker_wiring.py", gate)
        self.assertRegex(gate, r"\n\s+test_taunt_attacker_policy\n")


if __name__ == "__main__":
    unittest.main()
