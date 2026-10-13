// Standalone regression test for issue #490 PR 2 (kills): hire-unlock
// counter sync math. Guards
//  - the counter target (recorded + optional-map bosses, never lowers),
//  - the missing-boss roster (recorded/optional bosses excluded),
//  - the one-line gossip fit flag,
//  - the contracts-branch visibility gate.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_unlock_sync.cpp -o /tmp/test_hire_unlock_sync
//   /tmp/test_hire_unlock_sync

#include "../runtime/HireUnlockSync.h"

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

static void TestCounterTarget()
{
    HireUnlockCounter const* dungeons = nullptr;
    for (HireUnlockCounter const& counter : kHireUnlockCounters)
        if (counter.creditEntry == 95110)
            dungeons = &counter;
    CHECK(dungeons && dungeons->bossCount == 10);

    std::set<uint32_t> none;
    CHECK(HireUnlockCounterTarget(*dungeons, none, none, 0) == 0);
    // Never lowers the client's current value.
    CHECK(HireUnlockCounterTarget(*dungeons, none, none, 4) == 4);

    std::set<uint32_t> recorded = { 9019u, 1853u }; // Thaurissan + Gandling
    CHECK(HireUnlockCounterTarget(*dungeons, recorded, none, 0) == 2);

    // Optional maps cover the rest automatically.
    std::set<uint32_t> optional = { kMapBlackrockDepths, kMapScholomance,
        kMapStratholme, kMapBlackrockSpire, kMapDireMaul, kMapKarazhanCrypt };
    CHECK(HireUnlockCounterTarget(*dungeons, recorded, optional, 0) == dungeons->bossCount);
    // One optional map only covers its own bosses.
    std::set<uint32_t> depths = { kMapBlackrockDepths };
    CHECK(HireUnlockCounterTarget(*dungeons, recorded, depths, 0) == 2);
    std::set<uint32_t> scholo = { kMapScholomance };
    CHECK(HireUnlockCounterTarget(*dungeons, none, scholo, 0) == 1);
}

static void TestMissingBosses()
{
    HireUnlockCounter const* onyxia = nullptr;
    for (HireUnlockCounter const& counter : kHireUnlockCounters)
        if (counter.creditEntry == 95115)
            onyxia = &counter;
    CHECK(onyxia && onyxia->bossCount == 1);

    std::vector<HireUnlockBoss const*> missing;
    std::set<uint32_t> none;
    MissingHireUnlockBosses(*onyxia, none, none, missing);
    CHECK(missing.size() == 1 && missing[0]->entry == 10184);

    std::set<uint32_t> recorded = { 10184u };
    MissingHireUnlockBosses(*onyxia, recorded, none, missing);
    CHECK(missing.empty());

    std::set<uint32_t> optional = { kMapOnyxiasLair };
    MissingHireUnlockBosses(*onyxia, none, optional, missing);
    CHECK(missing.empty());
}

static void TestCounterLine()
{
    HireUnlockCounter const* onyxia = nullptr;
    HireUnlockCounter const* naxx = nullptr;
    for (HireUnlockCounter const& counter : kHireUnlockCounters)
    {
        if (counter.creditEntry == 95115)
            onyxia = &counter;
        if (counter.creditEntry == 95120)
            naxx = &counter;
    }
    CHECK(onyxia && naxx);

    std::string line;
    std::set<uint32_t> none;
    CHECK(HireUnlockCounterLine(*onyxia, 0, none, none, line));
    CHECK(line == "Onyxia slain: 0/1, missing: Onyxia");

    std::set<uint32_t> recorded = { 10184u };
    CHECK(HireUnlockCounterLine(*onyxia, 1, recorded, none, line));
    CHECK(line == "Onyxia slain: 1/1");

    // The 15-boss Naxxramas roster never fits one gossip line: caller opens
    // a sub-page instead of truncating names.
    CHECK(!HireUnlockCounterLine(*naxx, 0, none, none, line));
}

static void TestVisibility()
{
    CHECK(ShouldShowHireContracts(true, true, false, 60));
    CHECK(!ShouldShowHireContracts(false, true, false, 60));
    CHECK(!ShouldShowHireContracts(true, false, false, 60));
    CHECK(!ShouldShowHireContracts(true, true, true, 60));
    CHECK(!ShouldShowHireContracts(true, true, false, 59));
    CHECK(!ShouldShowHireContracts(true, true, false, 1));
}

int main()
{
    TestCounterTarget();
    TestMissingBosses();
    TestCounterLine();
    TestVisibility();
    std::printf("hire-unlock-sync: %d checks passed\n", checks);
    return 0;
}
