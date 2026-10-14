// Blessing coverage: one blessing per class bucket, unique source covers all,
// visit members missing anything known (not everything known).
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_blessing_coverage_policy.cpp -o /tmp/test_blessing_coverage
//   /tmp/test_blessing_coverage

#include "../ai/playerbot/BlessingCoveragePolicy.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace ai;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static void Set(bool* arr, BlessingFamily f) { arr[static_cast<std::size_t>(f)] = true; }

// Raid scene (Kings on the ret, Wisdom/Might/Light on the holy paladins):
// a healer holds Greater Wisdom and lacks Kings.
static void TestHealerKeepsWisdomGainsKings()
{
    bool retKnows[7] = {};
    Set(retKnows, BlessingFamily::Kings);

    bool present[7] = {};
    Set(present, BlessingFamily::Wisdom); // holy paladin's greater wisdom

    // The visit predicate fires: the healer misses Kings, not everything.
    CHECK(MemberNeedsBlessingVisit(retKnows, present));

    BlessingMember healer;
    healer.id = 42;
    healer.classId = 5; // priest
    healer.healer = true;
    CHECK(BlessingForMember(healer, retKnows, present) == BlessingFamily::Kings);

    // Once Kings has landed, nothing is left for the ret to do.
    Set(present, BlessingFamily::Kings);
    CHECK(BlessingForMember(healer, retKnows, present) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(retKnows, present));
}
static void TestHealerPriorityOrder()
{
    BlessingMember healer;
    healer.classId = 5;
    healer.healer = true;

    bool none[7] = {};
    bool knowsWisdom[7] = {};
    Set(knowsWisdom, BlessingFamily::Wisdom);
    Set(knowsWisdom, BlessingFamily::Kings);
    CHECK(BlessingForMember(healer, knowsWisdom, none) == BlessingFamily::Wisdom);

    bool knowsKingsOnly[7] = {};
    Set(knowsKingsOnly, BlessingFamily::Kings);
    CHECK(BlessingForMember(healer, knowsKingsOnly, none) == BlessingFamily::Kings);
}

// Same family present blocks; other families do not.
static void TestSameFamilyBlocks()
{
    BlessingMember dps;
    dps.classId = 4; // rogue
    bool knows[7] = {};
    Set(knows, BlessingFamily::Might);
    Set(knows, BlessingFamily::Kings);
    bool present[7] = {};
    Set(present, BlessingFamily::Might);
    // Might held: the bot's next known pick (Kings) still applies.
    CHECK(BlessingForMember(dps, knows, present) == BlessingFamily::Kings);
    // Both held: nothing left.
    Set(present, BlessingFamily::Kings);
    CHECK(BlessingForMember(dps, knows, present) == BlessingFamily::None);
}

// Visit predicate: full cover needs no visit; any gap does (self included).
static void TestVisitPredicate()
{
    bool knows[7] = {};
    Set(knows, BlessingFamily::Kings);
    bool present[7] = {};
    Set(present, BlessingFamily::Kings);
    CHECK(!MemberNeedsBlessingVisit(knows, present));
    bool missing[7] = {};
    CHECK(MemberNeedsBlessingVisit(knows, missing));
    // A member holding only an unknown blessing is still a candidate.
    bool strange[7] = {};
    Set(strange, BlessingFamily::Salvation);
    CHECK(MemberNeedsBlessingVisit(knows, strange));
}

// Greater buckets: one cast per class; role splits only where it matters.
static void TestBuckets()
{
    BlessingMember warriorA, warriorB, priest;
    warriorA.classId = 1;
    warriorB.classId = 1;
    priest.classId = 5;
    priest.healer = true;
    CHECK(SameBlessingBucket(warriorA, warriorB));
    CHECK(!SameBlessingBucket(warriorA, priest));

    BlessingMember tankW, dpsW;
    tankW.classId = 1;
    tankW.tank = true;
    dpsW.classId = 1;
    CHECK(!SameBlessingBucket(tankW, dpsW)); // warriors split by role

    BlessingMember mageA, mageB;
    mageA.classId = 8;
    mageB.classId = 8;
    mageB.healer = true; // mages never heal: flag ignored, same bucket
    CHECK(SameBlessingBucket(mageA, mageB));
}

int main()
{
    TestHealerKeepsWisdomGainsKings();
    TestHealerPriorityOrder();
    TestSameFamilyBlocks();
    TestVisitPredicate();
    TestBuckets();
    std::printf("test_blessing_coverage_policy: %d checks passed\n", checks);
    return 0;
}
