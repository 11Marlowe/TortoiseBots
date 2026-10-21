"""Wiring pins for the hunter and pet manners fix (audit F5, F6, F13).

The pure rules live in runtime/PetAttackGatePolicy.h and
ai/playerbot/HunterSwitchPolicy.h (unit-tested in tools/test_pet_attack_gate_policy.cpp
and tools/test_hunter_switch_policy.cpp). These pins prove the live rows call them:
a rule nobody calls changes nothing in the game.

 - F5: the pet is not sent in before the tank holds the mob (the "pet attack"
   row and the pet send inside AttackAction::Attack; explicit orders still go at
   once), and the pet Cowers on its first threat lead over the tank.
 - F13: two or more mobs on the hunter -> no melee trade, ranged kit back, Wing
   Clip row above the kite step-out; Hunter's Mark only on the tank's mob.
 - F6: Multi-Shot needs two or more usable targets.
 - The live hunter strategy is HunterStrategy.cpp; GenericHunterStrategy.cpp is
   a donor file with no creator, so none of its rows count.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
STRAT = ROOT / "ai/playerbot/strategy"
ATTACK = (STRAT / "actions/AttackAction.cpp").read_text()
GENERIC_TRIGGERS = (STRAT / "triggers/GenericTriggers.cpp").read_text()
H_TRIG_H = (STRAT / "hunter/HunterTriggers.h").read_text()
H_TRIG_CPP = (STRAT / "hunter/HunterTriggers.cpp").read_text()
H_STRAT = (STRAT / "hunter/HunterStrategy.cpp").read_text()
H_CTX = (STRAT / "hunter/HunterAiObjectContext.cpp").read_text()
USE_ITEM = (STRAT / "actions/UseItemAction.h").read_text()


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class PetManners(unittest.TestCase):
    def test_pet_send_in_attack_waits_for_the_tank_unless_ordered(self):
        fn = body(ATTACK, "bool AttackAction::Attack(Player* requester, Unit* target, float maxDistance, bool acceptCurrentVictim)")
        self.assertIn("(acceptCurrentVictim || !PetHeldForTank(ai, target))", fn)
        self.assertIn("creatureAI->AttackStart(target)", fn)

    def test_pet_attack_row_waits_for_the_tank(self):
        fn = body(GENERIC_TRIGGERS, "bool PetAttackTrigger::IsActive()")
        self.assertIn("AttackAction::PetHeldForTank(ai, target)", fn)

    def test_explicit_pet_attack_order_is_not_held(self):
        generic = (STRAT / "actions/GenericActions.cpp").read_text()
        for fn in (body(generic, "bool PetAttackAction::isUseful()"), body(generic, "bool PetAttackAction::Execute(Event& event)")):
            self.assertNotIn("PetHeldForTank", fn)

    def test_hold_reads_live_tank_victim_and_tank_threat(self):
        fn = body(ATTACK, "bool AttackAction::PetHeldForTank(PlayerbotAI* ai, Unit* target)")
        self.assertIn("!IsPetTauntAllowed(ai, bot)", fn)
        self.assertIn("ThreatValue::GetTankThreat(ai, target)", fn)
        self.assertIn("PlayerbotAI::IsTank(victimPlayer)", fn)
        self.assertIn("TortoiseBots::PetAttackHeldForTank(in)", fn)

    def test_cower_fires_on_the_first_threat_lead(self):
        fn = body(H_TRIG_CPP, "bool PetHasAggroTrigger::IsActive()")
        self.assertIn("TortoiseBots::PetThreatLeadsTank(", fn)
        self.assertIn("IsPetTauntAllowed", fn)
        self.assertIn("target->GetVictim() == pet", fn)

    def test_pet_rows_still_wired(self):
        pet = body(H_STRAT, "void HunterPetStrategy::InitCombatTriggers", "\n}\n")
        self.assertIn('"pet attack",', pet)
        self.assertRegex(pet, r'"pet has aggro",\s*NextAction::array\(0, new NextAction\("cower"')


class HunterManners(unittest.TestCase):
    def test_swarmed_hunter_does_not_trade_into_melee_and_gets_ranged_back(self):
        melee = body(H_TRIG_H, "class SwitchToMeleeTrigger", "\n    };")
        ranged = body(H_TRIG_H, "class SwitchToRangedTrigger", "\n    };")
        self.assertIn('AI_VALUE(uint8, "my attacker count")', melee)
        self.assertIn('AI_VALUE(uint8, "my attacker count")', ranged)
        self.assertIn("ai::ShouldSwitchToMelee(", melee)
        self.assertIn("ai::ShouldSwitchToRanged(", ranged)

    def test_wing_clip_row_sits_above_the_kite_step_out(self):
        self.assertRegex(H_STRAT, r'"hunter swarmed",\s*NextAction::array\(0, new NextAction\("wing clip", ACTION_MOVE\)')
        # The kite flee row it must outrank stays one below ACTION_MOVE.
        self.assertRegex(H_STRAT, r'"enemy too close for auto shot",\s*NextAction::array\(0, new NextAction\("flee", static_cast<float>\(ACTION_MOVE\) - 1\.0f\)')
        self.assertIn('creators["hunter swarmed"]', H_CTX)

    def test_swarmed_trigger_uses_the_policy(self):
        fn = body(H_TRIG_CPP, "bool HunterSwarmedTrigger::IsActive()")
        self.assertIn("HunterSwarmed(attackers)", fn)
        self.assertIn("HunterSwarmClipWanted(", fn)

    def test_hunters_mark_only_on_the_tanks_mob(self):
        fn = body(H_TRIG_CPP, "bool HuntersMarkTrigger::IsActive()")
        self.assertIn("HuntersMarkAllowed(true, holder)", fn)
        self.assertIn("DebuffTrigger::IsActive()", fn)
        self.assertIn("bool IsActive() override;", body(H_TRIG_H, "class HuntersMarkTrigger", "\n    };"))

    def test_multi_shot_needs_two_usable_targets(self):
        fn = body(H_TRIG_CPP, "bool MultishotCanCastTrigger::IsActive()")
        self.assertIn("    return MultiShotWorthIt(usable);", fn)
        self.assertIn("HoldsBreakableCc", fn)
        self.assertIn('creators["multi-shot"] = [](PlayerbotAI* ai) { return new MultishotCanCastTrigger(ai); };', H_CTX)
        self.assertRegex(H_STRAT, r'"multi-shot",\s*NextAction::array\(0, new NextAction\("multi-shot", ACTION_HIGH\)')

    def test_bandage_refuses_while_anything_attacks_the_bot(self):
        fn = body(USE_ITEM, "class UseBandageAction", "\n    };")
        self.assertRegex(fn, r'"my attacker count"\) > 0')

    def test_donor_generic_hunter_strategy_has_no_creator(self):
        for path in list((STRAT / "hunter").glob("*.cpp")) + list((STRAT / "hunter").glob("*.h")):
            if path.name.startswith("GenericHunterStrategy"):
                continue
            self.assertNotRegex(path.read_text(), r"new GenericHunterStrategy\b", path.name)


if __name__ == "__main__":
    unittest.main()
