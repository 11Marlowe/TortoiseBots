// Standalone regression test for issue #490 PR 2 (HIRE part): the hire-tier
// request policy (runtime/HireTierRequestPolicy.h). Guards
//  - tier display/kit names,
//  - `.bot hire` tier-word parsing,
//  - the equipment-step visibility rule,
//  - the gossip sender encode/decode,
//  - the unlock-chain PrevQuestId.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_tier_request_policy.cpp -o /tmp/test_hire_tier_request
//   /tmp/test_hire_tier_request

#include "../runtime/HireTierRequestPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static void TestNames()
{
    CHECK(std::string(HireTierName(0)) == "Standard");
    CHECK(std::string(HireTierName(1)) == "Pre-raid");
    CHECK(std::string(HireTierName(2)) == "Zul'Gurub");
    CHECK(std::string(HireTierName(3)) == "Molten Core");
    CHECK(std::string(HireTierName(4)) == "Blackwing Lair");
    CHECK(std::string(HireTierName(5)) == "Ahn'Qiraj");
    CHECK(std::string(HireTierName(6)) == "Naxxramas");
    CHECK(std::string(HireTierName(7)) == "Karazhan");
    CHECK(HireTierName(8) == nullptr);
    CHECK(HireTierKitName(0) == "Standard kit");
    CHECK(HireTierKitName(3) == "Molten Core kit");
    CHECK(HireTierKitName(8).empty());
}

static void TestTierWords()
{
    uint8_t tier = 255;
    CHECK(ParseHireTierWord("t0", tier) && tier == 0);
    CHECK(ParseHireTierWord("t3", tier) && tier == 3);
    CHECK(ParseHireTierWord("t7", tier) && tier == 7);
    CHECK(ParseHireTierWord("tier3", tier) && tier == 3);
    CHECK(ParseHireTierWord("tier0", tier) && tier == 0);
    CHECK(ParseHireTierWord("mc", tier) && tier == 3);
    CHECK(ParseHireTierWord("bwl", tier) && tier == 4);
    CHECK(ParseHireTierWord("zg", tier) && tier == 2);
    CHECK(ParseHireTierWord("aq", tier) && tier == 5);
    CHECK(ParseHireTierWord("aq40", tier) && tier == 5);
    CHECK(ParseHireTierWord("naxx", tier) && tier == 6);
    CHECK(ParseHireTierWord("naxxramas", tier) && tier == 6);
    CHECK(ParseHireTierWord("kara", tier) && tier == 7);
    CHECK(ParseHireTierWord("karazhan", tier) && tier == 7);
    CHECK(ParseHireTierWord("pre-raid", tier) && tier == 1);
    CHECK(ParseHireTierWord("standard", tier) && tier == 0);
    CHECK(ParseHireTierWord("molten", tier) && tier == 3);
    CHECK(ParseHireTierWord("qiraj", tier) && tier == 5);
    CHECK(ParseHireTierWord("blackwing", tier) && tier == 4);
    // Non-tier words stay untouched: bare numbers (class ids), roles, specs.
    CHECK(!ParseHireTierWord("3", tier));
    CHECK(!ParseHireTierWord("t8", tier));
    CHECK(!ParseHireTierWord("tier8", tier));
    CHECK(!ParseHireTierWord("tank", tier));
    CHECK(!ParseHireTierWord("frost", tier));
    CHECK(!ParseHireTierWord("mage", tier));
    CHECK(!ParseHireTierWord("", tier));
    CHECK(!ParseHireTierWord("t", tier));
    // Bare "pre" is a Protection spec word, never a tier (needs pre-raid).
    CHECK(!ParseHireTierWord("pre", tier));
}

static void TestVisibility()
{
    CHECK(HireTierStepVisible(true, 60, 1));
    CHECK(HireTierStepVisible(true, 60, 7));
    CHECK(!HireTierStepVisible(false, 60, 7));
    CHECK(!HireTierStepVisible(true, 59, 7));
    CHECK(!HireTierStepVisible(true, 1, 7));
    CHECK(!HireTierStepVisible(true, 60, 0));
    CHECK(!HireTierStepVisible(true, 60, 8));
}

static void TestSenders()
{
    // Tier senders stay clear of the wizard steps and the world-buff branch.
    CHECK(kHireTierStepSenderBase == 520);
    CHECK(kHireTierConfirmSenderBase == 530);
    CHECK(kHireTierBackSender == 540);
    for (uint8_t tier = 0; tier <= 7; ++tier)
    {
        uint8_t decoded = 255;
        CHECK(DecodeHireTierStepSender(HireTierStepSender(tier), decoded) && decoded == tier);
        CHECK(DecodeHireTierConfirmSender(HireTierConfirmSender(tier), decoded) && decoded == tier);
    }
    uint8_t decoded = 255;
    CHECK(!DecodeHireTierStepSender(504, decoded));
    CHECK(!DecodeHireTierStepSender(505, decoded));
    CHECK(!DecodeHireTierStepSender(529, decoded));
    CHECK(!DecodeHireTierConfirmSender(520, decoded));
    CHECK(!DecodeHireTierConfirmSender(540, decoded));
}

static void TestChainPrev()
{
    CHECK(HireTierChainPrevQuest(1, false) == 0);
    CHECK(HireTierChainPrevQuest(2, false) == 90100);
    CHECK(HireTierChainPrevQuest(7, false) == 90105);
    for (uint8_t tier = 1; tier <= 7; ++tier)
        CHECK(HireTierChainPrevQuest(tier, true) == 0);
    CHECK(HireTierChainPrevQuest(0, false) == 0);
    CHECK(HireTierChainPrevQuest(8, false) == 0);
}

int main()
{
    TestNames();
    TestTierWords();
    TestVisibility();
    TestSenders();
    TestChainPrev();
    std::printf("hire_tier_request_policy: %d checks passed\n", checks);
    return 0;
}
