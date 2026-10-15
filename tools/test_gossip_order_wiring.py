"""`.bot cmd gossip` goes through the core's own gossip path (issue #1012).

The order hands the bot's session the client's CMSG_GOSSIP_HELLO and then
CMSG_GOSSIP_SELECT_OPTION, so script NPCs (Weegli 7607, Sergeant Bly 7604) get
sScriptMgr.OnGossipHello/OnGossipSelect exactly as for a player. The option is
picked from the menu the core built (runtime/GossipOrderPolicy.h), the walk is
the commander task walk of `open`, and the order is owner-or-GM.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMMANDS = (ROOT / "commands/BotCommands.cpp").read_text()


def body(text, start, end="\n}\n"):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class GossipOrderWiringTest(unittest.TestCase):
    def test_act_uses_the_clients_packets_in_order(self):
        fn = body(COMMANDS, "ActStart BeginGossipAct(CommanderTask& task, Player* bot, TaskTarget const& target)")
        clear = fn.index("bot->PlayerTalkClass->ClearMenus();")
        hello = fn.index("bot->GetSession()->HandleGossipHelloOpcode(hello);")
        menu = fn.index("bot->PlayerTalkClass->GetGossipMenu()")
        match = fn.index("GossipOrder::MatchOption(options, task.gossip)")
        select = fn.index("bot->GetSession()->HandleGossipSelectOptionOpcode(select);")
        self.assertTrue(clear < hello < menu < match < select)
        self.assertIn("select << uint32(match.listIndex);", fn)
        self.assertIn("bot->CanInteractWithNPC(npc, UNIT_NPC_FLAG_NONE)", fn)
        # No shortcut around the core's handlers.
        for shortcut in ("OnGossipHello(", "OnGossipSelect(", "PrepareGossipMenu("):
            self.assertNotIn(shortcut, fn)

    def test_order_is_dispatched_and_walks_like_open(self):
        handler = body(COMMANDS, "static bool HandleCommanderOrder(ChatHandler* handler, char const* args)")
        self.assertIn("parsed = GossipOrder::ParseRequest(rest, gossipRequest);", handler)
        self.assertIn("return CommanderGossip(handler, order, bot, gossipRequest, intent);", handler)
        fn = body(COMMANDS, "static bool CommanderGossip(")
        self.assertIn("CommanderTaskGuard(handler, intent, bot, ai, true)", fn)
        self.assertIn("FindGossipNpc(bot, request.entry)", fn)
        self.assertIn("RemoteCommander::TaskKind::Gossip", fn)
        self.assertIn("return LaunchCommanderTask(handler, order, bot, ai, task);", fn)
        act = body(COMMANDS, "ActStart BeginTaskAct(CommanderTask& task, Player* commander")
        self.assertIn("start = BeginGossipAct(task, bot, target);", act)

    def test_npc_search_is_bounded_and_living(self):
        fn = body(COMMANDS, "uint64 FindGossipNpc(Player* bot, uint32 entry)")
        self.assertIn("Cell::VisitGridObjects(reference.x, reference.y, map, searcher, GossipOrder::kSearchYards);", fn)
        check = body(COMMANDS, "class EntryLivingCreatureCheck", "\n};\n")
        self.assertIn("creature->IsAlive()", check)


if __name__ == "__main__":
    unittest.main()
