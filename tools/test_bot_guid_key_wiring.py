"""The bot registry is looked up by a player guid only.

BotManager::m_bots is keyed by a guid's counter. A creature's or a charmed
summon's pet guid has a small counter too, and Deadmines r12-19z29 (the
Remote-Controlled Golem summons, high part 0xF140) answered "a bot" for one:
ResolveOrderTarget (commands/BotCommands.cpp) refused the tank's pick-up order
with "That GUID is a bot or a member of the group" three times. Every lookup by
an ObjectGuid argument goes through BotRegistryKey (runtime/BotGuidKey.h,
tested by tools/test_bot_guid_key_policy.cpp), which refuses any guid whose
high part is not HIGHGUID_PLAYER. Lookups by a Player's own guid are players
by construction and stay as they are.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP = (ROOT / "runtime/BotManager.cpp").read_text()
HEADER = (ROOT / "runtime/BotManager.h").read_text()
KEY = (ROOT / "runtime/BotGuidKey.h").read_text()
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()


def body(text, start):
    i = text.index(start)
    return text[i:text.index("\n}\n", i)]


class BotGuidKeyWiringTest(unittest.TestCase):
    def test_key_refuses_every_high_part_but_the_players(self):
        self.assertIn("constexpr std::uint16_t kBotGuidHighPlayer = 0x0000;", KEY)
        fn = body(KEY, "inline bool BotRegistryKey(")
        self.assertIn("if (raw == 0 || BotGuidHigh(raw) != kBotGuidHighPlayer)", fn)
        self.assertIn("return false;", fn)

    def test_lookups_by_a_guid_argument_use_the_key(self):
        self.assertIn('#include "BotGuidKey.h"', CPP)
        for start in (
            "BotRecord* BotManager::FindBot(::ObjectGuid guid)",
            "bool BotManager::IsBot(::ObjectGuid guid) const",
            "bool BotManager::IsRandomBot(::ObjectGuid guid) const",
            "bool BotManager::AddBotWithMaster(",
            "bool BotManager::RemoveBot(::ObjectGuid guid, bool save)",
        ):
            fn = body(CPP, start)
            self.assertIn("BotRegistryKey(guid.GetRawValue(), key)", fn, start)
            self.assertNotIn("GetCounter()", fn.split("BotRegistryKey", 1)[0], start)
        for start in (
            "bool BotManager::BindBotMaster(",
            "bool BotManager::ClearBotMaster(",
            "bool BotManager::SetBotFollow(",
        ):
            fn = body(CPP, start)
            self.assertIn("FindByGuid(m_bots, botGuid)", fn, start)
        self.assertIn("FindByGuid(m_bots, masterGuid)", body(CPP, "bool BotManager::BindBotMaster("))
        helper = body(CPP, "auto FindByGuid(Map& bots, ::ObjectGuid const& guid)")
        self.assertIn("if (!BotRegistryKey(guid.GetRawValue(), key))", helper)
        self.assertIn("return bots.end();", helper)

    def test_no_registry_find_by_a_bare_guid_counter(self):
        # m_bots.find(<x>.GetCounter()) is allowed only for a Player's own guid.
        for m in re.finditer(r"m_bots\.find\(([^)]*GetCounter\(\))\)", CPP):
            self.assertIn("player->GetObjectGuid()", m.group(1), m.group(0))

    def test_the_analyzer_stub_has_the_raw_value(self):
        stub = HEADER[HEADER.index("#ifndef MANGOS_OBJECT_GUID_H"):HEADER.index("#endif")]
        self.assertIn("uint64_t GetRawValue() const", stub)

    def test_order_target_asks_the_registry_by_the_units_full_guid(self):
        fn = body(COMMANDS, "static Unit* ResolveOrderTarget(")
        self.assertIn("BotManager::Instance().IsBot(unit->GetObjectGuid())", fn)


if __name__ == "__main__":
    unittest.main()
