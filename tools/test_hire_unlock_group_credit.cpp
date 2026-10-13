// Standalone regression test for issue #490 PR 3 (credit): the kill-credit
// fan-out filter. Guards that the killer always counts, bots never do,
// out-of-range members are skipped, and the killer is never double-listed.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_unlock_group_credit.cpp -o /tmp/test_hire_unlock_group_credit
//   /tmp/test_hire_unlock_group_credit

#include "../runtime/HireUnlockGroupCredit.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static void TestSoloKiller()
{
    std::vector<uint32_t> targets;
    HireUnlockCreditTargets(7, nullptr, 0, targets);
    CHECK(targets.size() == 1);
    CHECK(targets[0] == 7);
}

static void TestPartyFanOut()
{
    HireUnlockCreditCandidate members[] = {
        { 7, false, true },   // killer (also listed as member)
        { 8, false, true },   // real player in range: credit
        { 9, true, true },    // bot in range: never credit
        { 10, false, false }, // real player out of range: skip
    };
    std::vector<uint32_t> targets;
    HireUnlockCreditTargets(7, members, 4, targets);
    CHECK(targets.size() == 2);
    CHECK(targets[0] == 7);
    CHECK(targets[1] == 8);
}

static void TestBotKillerStillCreditsParty()
{
    HireUnlockCreditCandidate members[] = {
        { 9, true, true },   // bot landed the blow
        { 7, false, true },  // owner in range: credit
        { 8, false, false }, // other member out of range: skip
    };
    std::vector<uint32_t> targets;
    // The killer slot holds the bot guid; the adapter swaps it for the
    // owner before calling, so pass the owner here: the bot itself must
    // never appear in the target list.
    HireUnlockCreditTargets(7, members, 3, targets);
    CHECK(targets.size() == 1);
    CHECK(targets[0] == 7);
}

static void TestKillerNotDuplicated()
{
    HireUnlockCreditCandidate members[] = {
        { 7, false, true },
        { 7, false, true },
    };
    std::vector<uint32_t> targets;
    HireUnlockCreditTargets(7, members, 2, targets);
    CHECK(targets.size() == 1);
    CHECK(targets[0] == 7);
}

int main()
{
    TestSoloKiller();
    TestPartyFanOut();
    TestBotKillerStillCreditsParty();
    TestKillerNotDuplicated();
    std::printf("hire_unlock_group_credit: %d checks passed\n", checks);
    return 0;
}
