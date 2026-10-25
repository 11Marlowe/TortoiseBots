"""New server intents stay on the documented surface.

Pins the additive `.bot action` intents (end pull, stance, rebuff, guard,
free), the `status` query on the aoe/autocc/loot toggles, the accepted
`.bot cc` forms, the `sap=` BOTSTATE field, the `rm` alias, the bare
`strategy reset`, and the class-spec role ids: parsing accepts them and the
usage/help lines advertise them.

Run: python3 -m unittest tools/test_tbm_server_intents.py
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
ACTION = (ROOT / "commands/Action.cpp").read_text()
PARTY = (ROOT / "commands/Party.cpp").read_text()
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()
ADDON = (ROOT / "docs/play/commands/addon.md").read_text()


class TbmServerIntentsTest(unittest.TestCase):
    def test_parser_accepts_new_intents(self):
        allowlist = ACTION[ACTION.index("first != \"attack\""):ACTION.index("return false;", ACTION.index("first != \"attack\""))]
        for word in ["stance", "rebuff", "guard", "free", "end"]:
            self.assertIn('first != "%s"' % word, allowlist)
        self.assertIn('first == "stance"', ACTION)
        self.assertIn('first == "guard" || first == "free" || first == "rebuff"', ACTION)
        self.assertIn('intent = "end pull";', ACTION)
        self.assertIn('first == "aoe" || first == "loot" || first == "autocc"', ACTION)
        self.assertIn('intent = first == "autocc" ? "auto cc" : first;', ACTION)

    def test_new_intents_dispatch_and_ack(self):
        for intent in ["end pull", "stance", "rebuff", "guard", "free"]:
            self.assertIn('intent == "%s"' % intent, ACTION)
        self.assertIn('ExecuteOrderedAction(ai, "end pull",', ACTION)
        self.assertIn('ExecuteOrderedAction(ai, "stance",', ACTION)
        self.assertIn('ExecuteOrderedAction(ai, "force rebuff",', ACTION)
        self.assertIn('ExecuteOrderedAction(ai, "guard chat shortcut",', ACTION)
        self.assertIn('ExecuteOrderedAction(ai, "free chat shortcut",', ACTION)
        self.assertIn('intent == "stance" ? option : ""', ACTION)
        self.assertIn('option == "status"', ACTION)

    def test_usage_advertises_new_forms(self):
        usage = ACTION[ACTION.index("Usage: .bot action attack"):]
        usage = usage[:usage.index('");')]
        for form in ["end pull", "guard/free", "stance <near|tank|turnback|behind>",
                     "rebuff", "autocc [on/off/status]", "aoe [on/off/status]",
                     "loot [on/off/status]"]:
            self.assertIn(form, usage)

    def test_cc_verb_registered_and_shared(self):
        self.assertIn('{"cc", HandleCc,', COMMANDS)
        self.assertIn("bool HandleCc(ChatHandler* handler, char const* args);",
                      (ROOT / "commands/BotCommandHandlers.h").read_text())
        self.assertIn("bool HandleCc(ChatHandler* handler, char const* args)", PARTY)
        self.assertIn("RunCcOrder(handler, context, requester,", PARTY)

    def test_rm_alias_and_strategy_reset(self):
        self.assertIn('{"rm"}', COMMANDS)
        self.assertIn('if (lowered == "reset")', PARTY)
        self.assertIn("Strategy change 'reset' applied to %u bot(s).", PARTY)
        self.assertIn("strategy reset", COMMANDS)

    def test_spec_role_ids(self):
        for spec in ["bm", "mm", "survival", "combat", "assa", "subtlety",
                     "hybrid", "ele", "enh", "frost", "fire", "arcane",
                     "affliction", "demonology", "destruction", "cat", "boomkin"]:
            self.assertIn('{ "%s",' % spec, PARTY)
        self.assertIn("SpecRoleChange(roleToken, role)", PARTY)

    def test_sap_in_botstate(self):
        send = PARTY[PARTY.index("void SendBotStateLine("):PARTY.index("bool HandleBehavior(")]
        self.assertIn('",sap="', send)
        self.assertIn("no sap", send)
        self.assertIn("sap=<on|off>", ADDON)

    def test_human_raid_line_next_to_ack(self):
        self.assertIn('"Custom raid tactics are %s"', ACTION)
        self.assertIn('"TBM:ACTION_ACK|raid custom status|bot:%s|1|%s"', ACTION)


if __name__ == "__main__":
    unittest.main()
