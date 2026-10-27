"""Bot LFG chat advertising wiring (part 3, Task 4).

Pins that a waiting bot LFG group posts in its faction's LookingForGroup
channel, that every exit from Waiting goes through StopAdvertising, and
that the leader set is published to the whisper inbox.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP = (ROOT / "runtime/BotLfgService.cpp").read_text()
CMD = (ROOT / "commands/Commander.cpp").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


SVC = strip_comments(CPP)


class BotLfgChatWiringTest(unittest.TestCase):
    def test_advertising_only_while_waiting(self):
        fn = body(SVC, "bool BotLfgService::Advertising(", "\n}\n")
        self.assertIn("BotLfgLifecycle::State::Waiting", fn)
        self.assertIn("sPlayerbotAIConfig.botLfgChat", fn)

    def test_advertise_groups_pins(self):
        fn = body(SVC, "void BotLfgService::AdvertiseGroups()", "\n}\n")
        for snippet in ("BotLfgAdvert::PostDue(", "m_channelPostMs[", "RealPlayerOfTeamOnline(",
                        "PostInChannel("):
            self.assertIn(snippet, fn)
        self.assertLess(fn.index("RealPlayerOfTeamOnline("), fn.index("PostInChannel("))

    def test_post_in_channel_pins(self):
        fn = body(SVC, "void BotLfgService::PostInChannel(", "\n}\n")
        for snippet in ("BotLfgAdvert::PostText(", "BotLfgAdvert::DungeonName(",
                        "GetOrCreateChannel(BotLfgAdvert::kChannel)", "AsyncSay(", "++group.posts",
                        "group.lastPostMs = m_nowMs", "m_channelPosted["):
            self.assertIn(snippet, fn)

    def test_transition_and_end_stop_advertising(self):
        transition = body(SVC, "void BotLfgService::Transition(", "\n}\n")
        self.assertIn("StopAdvertising(group)", transition)
        self.assertLess(transition.index("StopAdvertising(group)"), transition.index("group.state = to"))
        end = body(SVC, "void BotLfgService::End(", "\n}\n")
        self.assertIn("StopAdvertising(group)", end)
        self.assertLess(end.index("StopAdvertising(group)"), end.index("RemoveMember("))

    def test_stop_advertising_and_publish_leaders_pins(self):
        stop = body(SVC, "void BotLfgService::StopAdvertising(", "\n}\n")
        self.assertIn("group.invitee = ObjectGuid()", stop)
        self.assertIn("PublishLeaders()", stop)
        publish = body(SVC, "void BotLfgService::PublishLeaders() const", "\n}\n")
        self.assertIn("BotLfgWhisperInbox::Instance().PublishLeaders(", publish)
        self.assertIn("Advertising(", publish)

    def test_update_advertises_after_tick_groups(self):
        fn = body(SVC, "void BotLfgService::Update(uint32 diff)", "\n}\n")
        self.assertLess(fn.index("TickGroups("), fn.index("AdvertiseGroups()"))
        self.assertLess(fn.index("m_groups.erase(", fn.index("AdvertiseGroups()")), fn.index("AdvertiseGroups()"))
        self.assertLess(fn.index("AdvertiseGroups()"), fn.index("if (!enabled)"))
        self.assertIn("PublishLeaders()", fn)

    def test_shutdown_clears_leaders(self):
        fn = body(SVC, "void BotLfgService::Shutdown()", "\n}\n")
        self.assertIn("BotLfgWhisperInbox::Instance().PublishLeaders({})", fn)

    def test_commander_list_shows_posts_and_invite(self):
        self.assertIn("posts:%u invite:%s", CMD)
    def test_update_advertises_after_tick_groups(self):
        fn = body(SVC, "void BotLfgService::Update(uint32 diff)", "\n}\n")
        self.assertLess(fn.index("TickGroups("), fn.index("AdvertiseGroups()"))
        self.assertLess(fn.index("m_groups.erase("), fn.index("AdvertiseGroups()"))
        self.assertLess(fn.index("AdvertiseGroups()"), fn.index("if (!enabled)"))
        self.assertIn("PublishLeaders()", fn)
