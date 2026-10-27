"""A hired mercenary accepts the LFT offer made to its hirer's party (#1284).

`LftBotFillService::AcceptPartyBotOffers` must route the random-pool skip
through `LftPartyBotMayAccept`: a random-pool bot that is not the human's is
still skipped, but a hire (a pool character that is its hirer's companion)
is accepted like an alt. The same-offer anchor (master or group leader, a
real player in the same offer) keeps a hire out of offers its hirer is not
in, and fill-owned bots still go through `AcceptPendingOffers`.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP = (ROOT / "runtime/LftBotFillService.cpp").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class LftPartyAcceptWiringTest(unittest.TestCase):
    def test_party_acceptor_delegates_to_policy(self):
        self.assertIn('#include "runtime/policy/hire/LftPartyAcceptPolicy.h"', CPP)
        self.assertIn('#include "HireLifecycle.h"', CPP)
        fn = strip_comments(body(CPP, "void LftBotFillService::AcceptPartyBotOffers()", "\n}\n"))
        self.assertIn("LftPartyBotMayAccept(", fn)
        self.assertIn("HireLifecycle::Instance().IsHired(guid)", fn)
        self.assertNotRegex(
            fn,
            r"if\s*\(BotManager::Instance\(\)\.IsRandomBot\(guid\)\)\s*\n\s*continue;",
        )

    def test_party_acceptor_keeps_fill_and_same_offer_guards(self):
        fn = strip_comments(body(CPP, "void LftBotFillService::AcceptPartyBotOffers()", "\n}\n"))
        self.assertLess(
            fn.index("m_pending.find(guidLow) != m_pending.end()"),
            fn.index("LftPartyBotMayAccept("),
        )
        self.assertIn("IsInOffer(master->GetObjectGuid())", fn)
        self.assertIn("IsInOffer(leader->GetObjectGuid())", fn)


if __name__ == "__main__":
    unittest.main()
