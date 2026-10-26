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


ACTIONS = ROOT / "ai" / "playerbot" / "strategy" / "actions"
CHAR_MIGRATION = (ROOT / "data" / "sql" / "character" / "20261026120000_char.sql").read_text()


class GivenItemProvenanceWiringTest(unittest.TestCase):
    """Codex fix 1: the ledger holds the exact created item instance, written before the bot holds it; the take-back
    destroys that instance only; a failed ledger write refuses the give; the given item cannot leave the bot."""

    def give(self):
        return body(COMMANDS, "void GiveRunItems(DungeonRunSlot& slot,")

    def take(self):
        return body(COMMANDS, "void TakeRunItems(DungeonRunSlot& slot,")

    def cleanup(self):
        return body(COMMANDS, "static void CleanupRunGivenItemsImpl(Player* player)")

    def test_ledger_row_is_written_before_the_bot_holds_the_item(self):
        give = self.give()
        created = give.index("Item::CreateItem(order.item, 1, bot)")
        bound = give.index("given->SetBinding(true)")
        written = give.index("RunGivenRowInsert(")
        stored = give.index("bot->StoreItem(dest, given, true)")
        self.assertLess(created, bound)
        self.assertLess(bound, written)
        self.assertLess(written, stored)
        self.assertNotIn("bot->StoreNewItem(", give)

    def test_failed_ledger_write_refuses_the_give(self):
        give = self.give()
        refuse = body(give, "if (!RunGivenRowInsert(", "continue;")
        self.assertIn("delete given;", refuse)
        insert = body(COMMANDS, "static bool RunGivenRowInsert(")
        self.assertIn("CharacterDatabase.DirectPExecute(", insert)
        self.assertIn("return ", insert)

    def test_give_records_the_instance(self):
        give = self.give()
        self.assertIn("RunGivenItems::Add(itemGuidLow)", give)
        self.assertIn("slot.given[raw].push_back({order.item, itemGuidLow})", give)

    def test_take_back_destroys_the_given_instance_only(self):
        take = self.take()
        self.assertIn("bot->GetItemByGuid(ObjectGuid(HIGHGUID_ITEM, row.itemGuid))", take)
        self.assertIn("bot->DestroyItem(item->GetBagSlot(), item->GetSlot(), true)", take)
        self.assertNotIn("DestroyItemCount", take)
        self.assertNotIn("GetItemCount", take)
        self.assertIn("RunGivenRowDelete(row.itemGuid)", take)

    def test_login_cleanup_destroys_the_ledger_instances_in_world_only(self):
        cleanup = self.cleanup()
        self.assertNotIn("DestroyItemCount", cleanup)
        self.assertIn("GetItemByGuid(ObjectGuid(HIGHGUID_ITEM,", cleanup)
        # Not in the world yet: the rows stay (queued for the tick), nothing deleted.
        guard = cleanup.index("if (!player->IsInWorld())")
        self.assertLess(guard, cleanup.index("PQuery("))
        self.assertIn("PendingRunGivenCleanup().insert(", body(cleanup, "if (!player->IsInWorld())", "}"))

    def test_ledger_table_keys_the_item_instance(self):
        self.assertIn("`item_guid` int unsigned NOT NULL", CHAR_MIGRATION)
        self.assertIn("PRIMARY KEY (`item_guid`)", CHAR_MIGRATION)
        self.assertNotIn("`count`", CHAR_MIGRATION)

    def test_bot_item_paths_refuse_a_given_item(self):
        give = (ACTIONS / "GiveItemAction.cpp").read_text()
        share = (ACTIONS / "GuildShareItemAction.cpp").read_text()
        sell = (ACTIONS / "SellAction.cpp").read_text()
        for name, text in (("GiveItemAction", give), ("GuildShareItemAction", share)):
            self.assertIn("RunGivenItemRegistry.h", text, name)
            moved = text.index("bot->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);")
            guard = text.index("TortoiseBots::RunGivenItems::Has(item->GetGUIDLow())")
            self.assertLess(guard, moved, name)
        sell_one = body(sell + "\n", "bool SellAction::Sell(Player* requester, Item* item)")
        self.assertIn("TortoiseBots::RunGivenItems::Has(item->GetGUIDLow())", sell_one)


if __name__ == "__main__":
    unittest.main()
