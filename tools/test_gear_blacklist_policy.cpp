// Standalone regression test for the gear exclusion list: a blocked item id
// is never handed out by any auto-gear path. Guards
//  - the blacklist lookup (empty list blocks nothing),
//  - blacklist-over-whitelist precedence,
//  - worn blocked gear counting as empty for pool/hired bots only; real
//    players and owned bots keep whatever they wear.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_gear_blacklist_policy.cpp -o /tmp/test_gear_blacklist
//   /tmp/test_gear_blacklist

#include "../runtime/policy/provision/GearBlacklistPolicy.h"

#include <cstdio>
#include <cstdlib>
#include <list>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static void TestLookup()
{
    std::list<uint32_t> empty;
    CHECK(!GearBlacklistBlocks(empty, 12345));

    std::list<uint32_t> blacklist = { 12345, 23456 };
    CHECK(GearBlacklistBlocks(blacklist, 12345));
    CHECK(GearBlacklistBlocks(blacklist, 23456));
    CHECK(!GearBlacklistBlocks(blacklist, 99999));
}

static void TestWhitelistPrecedence()
{
    std::list<uint32_t> empty;
    CHECK(GearWhitelistApplies(empty, true, 12345));
    CHECK(!GearWhitelistApplies(empty, false, 12345));

    // On both lists: the blacklist wins.
    std::list<uint32_t> blacklist = { 12345 };
    CHECK(!GearWhitelistApplies(blacklist, true, 12345));
    CHECK(GearWhitelistApplies(blacklist, true, 23456));
    CHECK(!GearWhitelistApplies(blacklist, false, 12345));
}

static void TestWornCountsAsEmpty()
{
    std::list<uint32_t> blacklist = { 12345 };

    // Pool and hired bots re-gear a blocked worn piece.
    CHECK(GearWornCountsAsEmpty(false, blacklist, 12345));
    CHECK(!GearWornCountsAsEmpty(false, blacklist, 23456));

    // Real players and owned bots keep whatever they wear.
    CHECK(!GearWornCountsAsEmpty(true, blacklist, 12345));
    CHECK(!GearWornCountsAsEmpty(true, blacklist, 23456));
}

int main()
{
    TestLookup();
    TestWhitelistPrecedence();
    TestWornCountsAsEmpty();
    std::printf("gear blacklist policy OK (%d checks)\n", checks);
    return 0;
}
