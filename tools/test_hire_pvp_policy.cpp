// Standalone regression test for the hire PvE-only battleground rules:
// queueing warns once, entry dismisses, hiring is refused while queued or
// inside, and HireAllowPvP keeps today's behaviour. Pure header math, no
// core or hiring service.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_pvp_policy.cpp -o /tmp/test_hire_pvp
//   /tmp/test_hire_pvp

#include "../runtime/HirePvpPolicy.h"

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

static void TestEntryDismisses()
{
    CHECK(ShouldDismissHireOnBattleground(false, true, false));
    CHECK(ShouldDismissHireOnBattleground(false, false, true));
    CHECK(!ShouldDismissHireOnBattleground(false, false, false));
}

static void TestAllowKeepsToday()
{
    CHECK(!ShouldDismissHireOnBattleground(true, true, true));
    CHECK(!ShouldWarnMasterOnBgQueue(true, true, false));
    CHECK(!ShouldDequeueHireFromBgQueue(true, true, false));
    CHECK(MayHireForPvpState(true, true, true));
}

static void TestQueueWarnsOnce()
{
    CHECK(ShouldWarnMasterOnBgQueue(false, true, false));
    CHECK(!ShouldWarnMasterOnBgQueue(false, false, false));
    CHECK(!ShouldWarnMasterOnBgQueue(false, true, true));
}

static void TestQueueDequeuesHire()
{
    CHECK(ShouldDequeueHireFromBgQueue(false, true, false));
    CHECK(!ShouldDequeueHireFromBgQueue(false, false, false));
    CHECK(!ShouldDequeueHireFromBgQueue(false, true, true));
}

static void TestHiringRefused()
{
    CHECK(MayHireForPvpState(false, false, false));
    CHECK(!MayHireForPvpState(false, true, false));
    CHECK(!MayHireForPvpState(false, false, true));
}

int main()
{
    TestEntryDismisses();
    TestAllowKeepsToday();
    TestQueueWarnsOnce();
    TestQueueDequeuesHire();
    TestHiringRefused();
    std::printf("PASS hire pvp policy (%d checks)\n", checks);
    return 0;
}
