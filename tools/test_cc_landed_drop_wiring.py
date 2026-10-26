"""The run's crowd control landing drops every bot's order on that unit (issue #1160).

The sheep the run ordered holds its mark: InvalidTargetValue takes the held
mark off every bot's target only once no bot keeps an attack or explicit
attack order on it. When the run's CC lands, the glue drops those orders so
the held-mark rule takes the mob off every bot's target:
- runtime/DungeonRunPolicy.h queues the landed guids (Run::ccLandedDrop,
  filled by CheckCc and SapWait, cleared with the rest of the cc state);
- commands/Commander.cpp's TickDungeonRun calls DropOrdersOnHeldCc for each
  queued guid and then clears the queue;
- DropOrdersOnHeldCc resets "attack target" and "explicit attack target" to
  the landed guid and resets "invalid target", "current target".
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands" / "Commander.cpp").read_text()
POLICY = (ROOT / "runtime" / "DungeonRunPolicy.h").read_text()


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class CcLandedDropWiringTest(unittest.TestCase):
    def test_tick_drains_the_landed_drop(self):
        tick = body(COMMANDS, "void TickDungeonRun(uint64 commanderRaw, uint32 dt)", "\n}\n")
        self.assertIn("ccLandedDrop", tick)
        self.assertIn("DropOrdersOnHeldCc(", tick)
        self.assertIn("ccLandedDrop.clear()", tick)

    def test_drop_resets_orders_and_invalid_target(self):
        fn = body(COMMANDS, "void DropOrdersOnHeldCc(", "\n}\n")
        self.assertIn('"attack target"', fn)
        self.assertIn('"explicit attack target"', fn)
        self.assertIn('"invalid target", "current target"', fn)
        self.assertIn("->Reset()", fn)


if __name__ == "__main__":
    unittest.main()
