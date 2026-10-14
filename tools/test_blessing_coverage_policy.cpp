// Blessing coverage: one blessing slot per member per paladin (core: one
// SPELL_BLESSING per target per caster). The on-party action/trigger blesses
// only members holding NO blessing at all; anything already blessed is left
// alone — casting over it evicts the live aura (same caster) or churns
// against the other paladin's (observed Light/Kings alternation, round 2).
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

// Live scene (round 2, 5-man: ret + holy paladin): a member holding ANY
// blessing is left alone — no top-up, no overwrite, no eviction churn.
static void TestBlessedMemberLeftAlone()
{
    BlessingMember priest;
    priest.id = 7;
    priest.classId = 5; // priest
    priest.healer = true;

    // Ret knows Kings only; priest holds the holy paladin's Wisdom.
    bool retKnows[7] = {};
    Set(retKnows, BlessingFamily::Kings);
    bool holdsWisdom[7] = {};
    Set(holdsWisdom, BlessingFamily::Wisdom);
    CHECK(BlessingForMember(priest, retKnows, holdsWisdom) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(retKnows, holdsWisdom));

    // Holy paladin knows Wisdom+Might; priest holds its own Might.
    // (Live: holy paladin cast Might then Wisdom on the same priest,
    // evicting its own cast each time.)
    bool holyKnows[7] = {};
    Set(holyKnows, BlessingFamily::Wisdom);
    Set(holyKnows, BlessingFamily::Might);
    bool holdsMight[7] = {};
    Set(holdsMight, BlessingFamily::Might);
    CHECK(BlessingForMember(priest, holyKnows, holdsMight) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(holyKnows, holdsMight));
}

// Bare member gets the role priority pick.
static void TestBareMemberGetsPriorityPick()
{
    BlessingMember healer;
    healer.classId = 5;
    healer.healer = true;

    bool none[7] = {};
    bool knowsWisdomKings[7] = {};
    Set(knowsWisdomKings, BlessingFamily::Wisdom);
    Set(knowsWisdomKings, BlessingFamily::Kings);
    CHECK(BlessingForMember(healer, knowsWisdomKings, none) == BlessingFamily::Wisdom);
    CHECK(MemberNeedsBlessingVisit(knowsWisdomKings, none));

    bool knowsKingsOnly[7] = {};
    Set(knowsKingsOnly, BlessingFamily::Kings);
    CHECK(BlessingForMember(healer, knowsKingsOnly, none) == BlessingFamily::Kings);
    CHECK(MemberNeedsBlessingVisit(knowsKingsOnly, none));

    // Bot knowing nothing visits nobody.
    bool knowsNone[7] = {};
    CHECK(BlessingForMember(healer, knowsNone, none) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(knowsNone, none));
}

// Once the single slot is filled, nothing is left to do.
static void TestFilledSlotNeedsNoVisit()
{
    bool knows[7] = {};
    Set(knows, BlessingFamily::Kings);
    bool present[7] = {};
    Set(present, BlessingFamily::Kings);
    BlessingMember dps;
    dps.classId = 4; // rogue
    CHECK(BlessingForMember(dps, knows, present) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(knows, present));
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
    TestBlessedMemberLeftAlone();
    TestBareMemberGetsPriorityPick();
    TestFilledSlotNeedsNoVisit();
    TestBuckets();
    std::printf("test_blessing_coverage_policy: %d checks passed\n", checks);
    return 0;
}
