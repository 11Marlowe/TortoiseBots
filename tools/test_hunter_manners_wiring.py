"""Wiring pins for the hunter and pet manners.

The pure rules live in runtime/PetEngagePolicy.h, ai/playerbot/HunterAoePolicy.h,
ai/playerbot/HunterKitePolicy.h and ai/playerbot/HunterSwitchPolicy.h (unit-tested in
tools/test_pet_engage_policy.cpp, tools/test_hunter_aoe_policy.cpp,
tools/test_hunter_kite_policy.cpp and tools/test_hunter_switch_policy.cpp). These pins
prove the live rows call them: a rule nobody calls changes nothing in the game.

 - The pet waits out the tank's lead window in a bot-led party (CanPetAttack, used by
   the "pet attack" row, the pet send inside AttackAction::Attack and the explicit pet
   attack order), and Cowers on its first threat lead over the tank.
 - Two or more mobs on the hunter: the kite owns the fight (Wing Clip the nearest,
   step out; the melee rows stand down).
 - Multi-Shot flies only when its splash hits a second fighting enemy.
 - Hunter's Mark with a living tank goes only on the tank's mob.
 - The live hunter strategy is HunterStrategy.cpp; GenericHunterStrategy.cpp is
   a donor file with no creator, so none of its rows count.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
STRAT = ROOT / "ai/playerbot/strategy"
ATTACK = (STRAT / "actions/AttackAction.cpp").read_text()
H_TRIG_H = (STRAT / "hunter/HunterTriggers.h").read_text()
H_TRIG_CPP = (STRAT / "hunter/HunterTriggers.cpp").read_text()
H_STRAT = (STRAT / "hunter/HunterStrategy.cpp").read_text()
H_CTX = (STRAT / "hunter/HunterAiObjectContext.cpp").read_text()
USE_ITEM = (STRAT / "actions/UseItemAction.h").read_text()


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class PetManners(unittest.TestCase):
    def test_pet_send_holds_for_the_tank_lead_in_bot_led_parties(self):
        fn = body(ATTACK, "bool AttackAction::CanPetAttack(PlayerbotAI* ai, Pet* pet, Unit* target)")
        self.assertIn("TortoiseBots::PetEngageInputs engage;", fn)
        self.assertIn("TortoiseBots::ShouldHoldPetForTankLead(engage)", fn)
        attack = body(ATTACK, "bool AttackAction::Attack(Player* requester, Unit* target, float maxDistance, bool acceptCurrentVictim)")
        self.assertIn("CanPetAttack(ai, pet, target)", attack)
        self.assertIn("creatureAI->AttackStart(target)", attack)

    def test_cower_fires_on_the_first_threat_lead(self):
        fn = body(H_TRIG_CPP, "bool PetHasAggroTrigger::IsActive()")
        self.assertIn("TortoiseBots::PetThreatLeadInputs cower;", fn)
        self.assertIn("return TortoiseBots::ShouldPetCower(cower);", fn)
        self.assertIn("IsPetTauntAllowed", fn)
        self.assertIn("target->GetVictim() == pet", fn)

    def test_pet_rows_still_wired(self):
        pet = body(H_STRAT, "void HunterPetStrategy::InitCombatTriggers", "\n}\n")
        self.assertIn('"pet attack",', pet)
        self.assertRegex(pet, r'"pet has aggro",\s*NextAction::array\(0, new NextAction\("cower"')


class HunterManners(unittest.TestCase):
    def test_kite_row_clips_and_steps_out_above_the_melee_chain(self):
        self.assertRegex(H_STRAT,
            r'"hunter multiple attackers",\s*NextAction::array\(0, new NextAction\("wing clip on attacker", static_cast<float>\(ACTION_MOVE\) \+ 1\.0f\),\s*'
            r'new NextAction\("hunter kite step out", static_cast<float>\(ACTION_MOVE\) \+ 0\.5f\), NULL\)')
        self.assertIn('creators["hunter multiple attackers"]', H_CTX)

    def test_kite_trigger_uses_the_policy(self):
        trig = body(H_TRIG_H, "class HunterMultipleAttackersTrigger", "\n    };")
        self.assertIn("MyAttackerCountTrigger(ai, kHunterKiteMinAttackers)", trig)
        self.assertIn('HunterShouldKite(ai, AI_VALUE(uint8, "my attacker count"))', trig)

    def test_melee_rows_stand_down_while_the_kite_owns_the_fight(self):
        melee = body(H_TRIG_H, "class HunterEnemyIsCloseTrigger", "\n    };")
        self.assertIn('if (HunterShouldKite(ai, AI_VALUE(uint8, "my attacker count")))', melee)
        switch = body(H_TRIG_H, "class SwitchToMeleeTrigger", "\n    };")
        self.assertIn('if (HunterShouldKite(ai, AI_VALUE(uint8, "my attacker count")))', switch)

    def test_hunters_mark_only_on_the_tanks_mob(self):
        fn = body(H_TRIG_CPP, "bool HuntersMarkTrigger::IsActive()")
        self.assertIn("ShouldHuntersMarkTarget(groupHasTank, targetIsTankTarget)", fn)
        self.assertIn("PlayerbotAI::IsTank(member)", fn)
        self.assertIn("DebuffTrigger::IsActive()", fn)

    def test_multi_shot_needs_a_second_enemy_in_the_splash(self):
        fn = body(H_TRIG_CPP, "bool HunterMultiShotTrigger::IsActive()")
        self.assertIn("ShouldHunterMultiShot(splashTargets, breakableCcInSplash)", fn)
        self.assertIn("HoldsBreakableCc", fn)
        self.assertIn('creators["multi-shot"] = [](PlayerbotAI* ai) { return new HunterMultiShotTrigger(ai); };', H_CTX)
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
