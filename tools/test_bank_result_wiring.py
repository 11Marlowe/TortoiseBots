"""Bank/guild-bank moves report success when an item moves.

`BankAction::ExecuteCommand` accumulated per-item results with `&=` from a
`false` seed, so it always returned false even when items moved. Same for
`GuildBankAction::Execute(text, bank, requester)` (seeded true, but `&=`
fails the whole batch when any single move fails). Both now use `|=`.

Run: python3 -m unittest tools/test_bank_result_wiring.py
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class BankResultWiringTest(unittest.TestCase):
    def test_bank_accumulates_success(self):
        text = (ROOT / "ai/playerbot/strategy/actions/BankAction.cpp").read_text()
        block = text[text.index("bool BankAction::ExecuteCommand("):text.index("bool BankAction::Withdraw(")]
        self.assertIn("bool result = false;", block)
        self.assertIn("result |= Withdraw(", block)
        self.assertIn("result |= Deposit(", block)
        self.assertNotIn("result &=", block)

    def test_guild_bank_accumulates_success(self):
        text = (ROOT / "ai/playerbot/strategy/actions/GuildBankAction.cpp").read_text()
        block = text[text.index("bool GuildBankAction::Execute(std::string"):text.index("bool GuildBankAction::MoveFromCharToBank(")]
        self.assertIn("result |= MoveFromCharToBank(", block)
        self.assertNotIn("result &=", block)


if __name__ == "__main__":
    unittest.main()
