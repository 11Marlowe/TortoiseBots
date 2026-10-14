// Blessing coverage: one blessing slot per member PER PALADIN (core: one
// SPELL_BLESSING per target per caster). A paladin never overwrites its own
// live blessing (that cast evicts it — the round-2 churn); where it holds
// nothing, it picks the highest role-priority blessing no caster provides
// (same family from anyone blocks that family only — other casters' other
// families stack, so each member ends with one blessing from EACH paladin).
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

// Live scene (5-man: ret knows Kings, holy knows Wisdom/Might): a member
// holding the OTHER paladin's blessing still gets this bot's pick —
// different casters stack. Own live blessing always stands.
static void TestCrossCasterStacks()
{
    BlessingMember priest;
    priest.id = 7;
    priest.classId = 5; // priest
    priest.healer = true;

    // Ret (Kings only) visits a priest holding the holy paladin's Wisdom:
    // Kings lands alongside it.
    bool retKnows[7] = {};
    Set(retKnows, BlessingFamily::Kings);
    bool holdsWisdom[7] = {};
    Set(holdsWisdom, BlessingFamily::Wisdom);
    bool mineNone[7] = {};
    CHECK(BlessingForMember(priest, retKnows, holdsWisdom, mineNone) == BlessingFamily::Kings);
    CHECK(MemberNeedsBlessingVisit(retKnows, holdsWisdom, mineNone));

    // Holy (Wisdom first) visits a priest holding the ret's Kings: Wisdom
    // lands alongside it (healer priority: Wisdom before Kings).
    bool holyKnows[7] = {};
    Set(holyKnows, BlessingFamily::Wisdom);
    Set(holyKnows, BlessingFamily::Might);
    bool holdsKings[7] = {};
    Set(holdsKings, BlessingFamily::Kings);
    CHECK(BlessingForMember(priest, holyKnows, holdsKings, mineNone) == BlessingFamily::Wisdom);
    CHECK(MemberNeedsBlessingVisit(holyKnows, holdsKings, mineNone));
}

// Own live blessing stands: never a second, different pick on top of it
// (the eviction churn — each such cast destroyed the bot's own aura).
static void TestOwnBlessingStands()
{
    BlessingMember pal;
    pal.classId = 2; // paladin

    bool retKnows[7] = {};
    Set(retKnows, BlessingFamily::Kings);
    Set(retKnows, BlessingFamily::Light);
    bool holdsNothing[7] = {};
    bool mineLight[7] = {};
    Set(mineLight, BlessingFamily::Light);
    // Ret's own Light on the member blocks Kings: no overwrite.
    CHECK(BlessingForMember(pal, retKnows, holdsNothing, mineLight) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(retKnows, holdsNothing, mineLight));

    BlessingMember priest;
    priest.classId = 5;
    priest.healer = true;
    bool holyKnows[7] = {};
    Set(holyKnows, BlessingFamily::Wisdom);
    Set(holyKnows, BlessingFamily::Might);
    bool mineMight[7] = {};
    Set(mineMight, BlessingFamily::Might);
    // Holy's own Might blocks Wisdom: no overwrite.
    CHECK(BlessingForMember(priest, holyKnows, holdsNothing, mineMight) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(holyKnows, holdsNothing, mineMight));
}

// Same family from ANY caster covers the member: skip it, pick the next
// uncovered priority entry.
static void TestSameFamilyCoverSkipsFamily()
{
    BlessingMember dps;
    dps.classId = 4; // rogue
    bool knows[7] = {};
    Set(knows, BlessingFamily::Might);
    Set(knows, BlessingFamily::Kings);
    // Another caster's Might covers Might: bot picks Kings.
    bool holdsMight[7] = {};
    Set(holdsMight, BlessingFamily::Might);
    bool mineNone[7] = {};
    CHECK(BlessingForMember(dps, knows, holdsMight, mineNone) == BlessingFamily::Kings);
    CHECK(MemberNeedsBlessingVisit(knows, holdsMight, mineNone));
    // Both covered: nothing left.
    Set(holdsMight, BlessingFamily::Kings);
    CHECK(BlessingForMember(dps, knows, holdsMight, mineNone) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(knows, holdsMight, mineNone));
}

// Bare member gets the role priority pick; a bot knowing nothing visits nobody.
static void TestBareMemberGetsPriorityPick()
{
    BlessingMember healer;
    healer.classId = 5;
    healer.healer = true;

    bool none[7] = {};
    bool knowsWisdomKings[7] = {};
    Set(knowsWisdomKings, BlessingFamily::Wisdom);
    Set(knowsWisdomKings, BlessingFamily::Kings);
    CHECK(BlessingForMember(healer, knowsWisdomKings, none, none) == BlessingFamily::Wisdom);
    CHECK(MemberNeedsBlessingVisit(knowsWisdomKings, none, none));

    bool knowsKingsOnly[7] = {};
    Set(knowsKingsOnly, BlessingFamily::Kings);
    CHECK(BlessingForMember(healer, knowsKingsOnly, none, none) == BlessingFamily::Kings);

    bool knowsNone[7] = {};
    CHECK(BlessingForMember(healer, knowsNone, none, none) == BlessingFamily::None);
    CHECK(!MemberNeedsBlessingVisit(knowsNone, none, none));
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
    TestCrossCasterStacks();
    TestOwnBlessingStands();
    TestSameFamilyCoverSkipsFamily();
    TestBareMemberGetsPriorityPick();
    TestBuckets();
    std::printf("test_blessing_coverage_policy: %d checks passed\n", checks);
    return 0;
}
