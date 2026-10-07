#include "../runtime/ClaimedBotPolicy.h"

#include <cassert>
#include <iostream>

using namespace TortoiseBots;

static void TestEligibility()
{
    // Normal wandering bot eligible
    assert(MayClaimBot(true, true, false, false));

    // Already in a guild -> rejected
    assert(!MayClaimBot(false, true, false, false));

    // Not a pool bot (e.g. real player or already personal alt) -> rejected
    assert(!MayClaimBot(true, false, false, false));

    // Already claimed -> rejected
    assert(!MayClaimBot(true, true, true, false));

    // Opposing faction -> rejected
    assert(!MayClaimBot(true, true, false, true));
}

static void TestSelfEquip()
{
    // Unclaimed bots can self-equip at any level
    assert(MayClaimedBotSelfEquip(10, false));
    assert(MayClaimedBotSelfEquip(59, false));
    assert(MayClaimedBotSelfEquip(60, false));

    // Claimed bots: leveling can self-equip
    assert(MayClaimedBotSelfEquip(1, true));
    assert(MayClaimedBotSelfEquip(25, true));
    assert(MayClaimedBotSelfEquip(59, true));

    // Claimed bots at max level (60+): HARD LOCK
    assert(!MayClaimedBotSelfEquip(60, true));
    assert(!MayClaimedBotSelfEquip(61, true));
}

static void TestAutoVendor()
{
    // Unclaimed bots can auto-vendor any quality
    assert(MayClaimedBotAutoVendor(30, kQualityPoor, false));
    assert(MayClaimedBotAutoVendor(30, kQualityRare, false));
    assert(MayClaimedBotAutoVendor(60, kQualityEpic, false));

    // Claimed bots below 60: can vendor grey/white/green leveling junk
    assert(MayClaimedBotAutoVendor(30, kQualityPoor, true));
    assert(MayClaimedBotAutoVendor(30, kQualityCommon, true));
    assert(MayClaimedBotAutoVendor(30, kQualityUncommon, true));

    // Claimed bots below 60: Rare and Epic are preserved in bags!
    assert(!MayClaimedBotAutoVendor(30, kQualityRare, true));
    assert(!MayClaimedBotAutoVendor(30, kQualityEpic, true));
    assert(!MayClaimedBotAutoVendor(30, kQualityLegendary, true));

    // Claimed bots at level 60: never auto-vendor anything!
    assert(!MayClaimedBotAutoVendor(60, kQualityPoor, true));
    assert(!MayClaimedBotAutoVendor(60, kQualityCommon, true));
    assert(!MayClaimedBotAutoVendor(60, kQualityUncommon, true));
    assert(!MayClaimedBotAutoVendor(60, kQualityRare, true));
    assert(!MayClaimedBotAutoVendor(60, kQualityEpic, true));
}

static void TestResetExemption()
{
    assert(IsClaimedBotExemptFromReset(true));
    assert(!IsClaimedBotExemptFromReset(false));
}

int main()
{
    TestEligibility();
    TestSelfEquip();
    TestAutoVendor();
    TestResetExemption();

    std::cout << "claimed bot policy: all checks passed\n";
    return 0;
}
