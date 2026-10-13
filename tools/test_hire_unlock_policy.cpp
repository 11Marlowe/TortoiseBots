// Standalone regression test for issue #490 PR 2: hire-gear tier unlock
// quests. Guards
//  - the quest id <-> tier mapping (90100..90106),
//  - the counter catalog (unique credit entries, at most 4 slots per tier
//    quest, every tier 1-7 covered, unique boss entries),
//  - kill lookup incl. the Four Horsemen grouped encounter,
//  - highest unlocked tier (start tier vs rewarded quests),
//  - the tier cost multiplier parse and price math.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_unlock_policy.cpp -o /tmp/test_hire_unlock
//   /tmp/test_hire_unlock

#include "../runtime/HireUnlockPolicy.h"

#include <cstdio>
#include <cstdlib>
#include <set>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static void TestQuestIds()
{
    CHECK(HireUnlockQuestForTier(0) == 0);
    CHECK(HireUnlockQuestForTier(1) == 90100);
    CHECK(HireUnlockQuestForTier(7) == 90106);
    CHECK(HireUnlockQuestForTier(8) == 0);
    for (uint8_t tier = 1; tier <= 7; ++tier)
        CHECK(HireUnlockTierForQuest(HireUnlockQuestForTier(tier)) == tier);
    CHECK(HireUnlockTierForQuest(90099) == 0);
    CHECK(HireUnlockTierForQuest(90107) == 0);
    CHECK(HireUnlockTierForQuest(90000) == 0);
}

static void TestCatalog()
{
    std::set<uint32_t> credits;
    std::set<uint32_t> bosses;
    std::set<uint32_t> slots; // tier * 10 + slot
    int perTier[8] = {};
    size_t totalBosses = 0;
    for (HireUnlockCounter const& counter : kHireUnlockCounters)
    {
        CHECK(counter.tier >= 1 && counter.tier <= 7);
        CHECK(counter.slot < 4);
        CHECK(counter.creditEntry >= 95110 && counter.creditEntry <= 95121);
        CHECK(credits.insert(counter.creditEntry).second);
        CHECK(slots.insert(counter.tier * 10u + counter.slot).second);
        CHECK(counter.bossCount >= 1);
        ++perTier[counter.tier];
        for (uint8_t i = 0; i < counter.bossCount; ++i)
        {
            CHECK(bosses.insert(counter.bosses[i].entry).second);
            CHECK(counter.bosses[i].mapId != 0);
            HireUnlockBoss const* boss = nullptr;
            CHECK(HireUnlockCounterForKill(counter.bosses[i].entry, &boss) == &counter);
            CHECK(boss == &counter.bosses[i]);
        }
        totalBosses += counter.bossCount;
    }
    for (int tier = 1; tier <= 7; ++tier)
        CHECK(perTier[tier] >= 1 && perTier[tier] <= 4);
    // Spec counts: 10 | 8+6+5 | 14+1+7 | 8+2 | 7 | 15 | 8.
    CHECK(totalBosses == 10 + 19 + 22 + 10 + 7 + 15 + 8);
    // Optional bosses never count.
    for (uint32_t optional : { 15114u, 15511u, 15543u, 15544u, 15299u, 15517u, 59968u, 59970u, 59971u })
        CHECK(HireUnlockCounterForKill(optional) == nullptr);
}

static void TestHorsemen()
{
    HireUnlockCounter const* naxx = HireUnlockCounterForKill(16028);
    CHECK(naxx && naxx->tier == 6);
    for (uint32_t member : kHireUnlockFourHorsemen)
    {
        CHECK(HireUnlockEncounterKey(member) == 16062);
        CHECK(HireUnlockCounterForKill(member) == naxx);
    }
    // Thekal's real death fires as entry 14509 (P1 is a fake death with no
    // loot); both entries resolve to the same encounter and counter.
    CHECK(HireUnlockEncounterKey(14599) == 14599);
    CHECK(HireUnlockEncounterKey(kHireUnlockThekalPhaseTwo) == 14599);
    HireUnlockCounter const* zg = HireUnlockCounterForKill(14599);
    CHECK(zg && zg->tier == 2);
    CHECK(HireUnlockCounterForKill(kHireUnlockThekalPhaseTwo) == zg);
    CHECK(!IsHireUnlockGroupedEncounter(14599));
    // Linked credit: Ragnaros also credits Majordomo, Solnius also credits
    // Erennius; anything else (and the linked bosses themselves) has none.
    CHECK(HireUnlockLinkedCreditFor(11502) == 12018);
    CHECK(HireUnlockLinkedCreditFor(60748) == 60747);
    CHECK(HireUnlockLinkedCreditFor(12018) == 0);
    CHECK(HireUnlockLinkedCreditFor(60747) == 0);
    CHECK(HireUnlockLinkedCreditFor(14599) == 0);
    CHECK(HireUnlockLinkedCreditFor(16062) == 0);
    // Linked encounters resolve to their counters (same tier quests).
    HireUnlockBoss const* linkedBoss = nullptr;
    HireUnlockCounter const* linkedCounter =
        HireUnlockCounterForKill(HireUnlockLinkedCreditFor(11502), &linkedBoss);
    CHECK(linkedCounter && linkedCounter->tier == 3);
    CHECK(linkedBoss && linkedBoss->entry == 12018);
    linkedBoss = nullptr;
    linkedCounter = HireUnlockCounterForKill(HireUnlockLinkedCreditFor(60748), &linkedBoss);
    CHECK(linkedCounter && linkedCounter->tier == 4);
    CHECK(linkedBoss && linkedBoss->entry == 60747);
    CHECK(IsHireUnlockGroupedEncounter(16062));
    CHECK(!IsHireUnlockGroupedEncounter(16063));
    CHECK(!IsHireUnlockGroupedEncounter(15990));
    CHECK(HireUnlockEncounterKey(15990) == 15990);
}

static void TestHighestTier()
{
    CHECK(HighestUnlockedHireTier(0, 0) == 0);
    CHECK(HighestUnlockedHireTier(3, 0) == 3);
    CHECK(HighestUnlockedHireTier(9, 0) == 7);
    CHECK(HighestUnlockedHireTier(0, 0x1) == 1);
    CHECK(HighestUnlockedHireTier(0, 0x7) == 3);
    CHECK(HighestUnlockedHireTier(0, 0x40) == 7); // all-at-once: tier 7 alone
    CHECK(HighestUnlockedHireTier(5, 0x3) == 5);
    CHECK(HighestUnlockedHireTier(2, 0x8) == 4);
}

static void TestCostMultipliers()
{
    float mults[8];
    CHECK(ParseHireTierCostMultipliers("1,1.5,2,3,4,5,6,6", mults));
    CHECK(mults[1] == 1.5f && mults[7] == 6.0f);
    CHECK(ParseHireTierCostMultipliers(" 1, 1, 1, 1, 1, 1, 1, 2 ", mults));
    CHECK(mults[0] == 1.0f && mults[7] == 2.0f);
    for (char const* bad : { "", "1,2,3", "1,1.5,2,3,4,5,6,6,7", "1,x,2,3,4,5,6,6", "1,-1,2,3,4,5,6,6", "1,,2,3,4,5,6,6", "1,1.5,2,3,4,5,6," })
    {
        mults[3] = 99.0f;
        CHECK(!ParseHireTierCostMultipliers(bad, mults));
        CHECK(mults[3] == kHireTierCostMultDefault[3]);
    }
    float defaults[8];
    ParseHireTierCostMultipliers("", defaults);
    CHECK(HireCostAtTier(150000, 0, defaults) == 150000);
    CHECK(HireCostAtTier(150000, 1, defaults) == 225000);
    CHECK(HireCostAtTier(150000, 7, defaults) == 900000);
    CHECK(HireCostAtTier(150000, 12, defaults) == 900000);
    CHECK(HireCostAtTier(3, 1, defaults) == 5); // 4.5 rounds up
    CHECK(HireCostAtTier(4000000000u, 7, defaults) == 4294967295u);
}

int main()
{
    TestQuestIds();
    TestCatalog();
    TestHorsemen();
    TestHighestTier();
    TestCostMultipliers();
    std::printf("test_hire_unlock_policy: %d checks passed\n", checks);
    return 0;
}
