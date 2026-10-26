"""The item-leg-skip glue (commands/Commander.cpp) calls the pure decisions of runtime/RunItemNeedPolicy.h and
runtime/DungeonRunPolicy.h that tools/test_run_item_need_policy.cpp and tools/test_dungeon_run_policy.cpp test, so
those tests drive the code the server runs. Review round 2 (Codex + Opus, 2026-10-26).
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands" / "Commander.cpp").read_text()


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class StartWiringTest(unittest.TestCase):
    def start_block(self):
        return body(COMMANDS, "bool HandleDungeonRun(ChatHandler* handler, char const* args)")

    def test_refusal_sends_the_refusal_line(self):
        # Codex fix 3: the bot-only refusal sends the verdict's text (PlanStart's refusal), never the empty warning.
        block = self.start_block()
        self.assertIn("RunItemNeed::PlanStart(", block)
        self.assertIn('SendActionError(handler, intent, "requirement", startPlan.refusal.c_str());', block)
        self.assertNotIn("needVerdict.warn.c_str()", block)

    def test_party_is_read_for_every_needed_item(self):
        # The bags are read for NeedsFrom's items (derived blocking ones too), not only the need rows.
        block = self.start_block()
        self.assertIn("RunItemNeed::PlanNeedItems(*plan, startStep)", block)


if __name__ == "__main__":
    unittest.main()
