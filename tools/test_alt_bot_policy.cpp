#include "../runtime/AltBotPolicy.h"

#include <cassert>
#include <iostream>

using namespace TortoiseBots;

static void TestStart()
{
    assert(CheckAltBotStart(true, true, false, true) == AltBotStartRefusal::None);
    // Config off: a kicked alt stays a companion (G48 as decided on #872).
    assert(CheckAltBotStart(false, true, false, true) == AltBotStartRefusal::FeatureOff);
    // Pool and hired characters are never alt-bots.
    assert(CheckAltBotStart(true, false, false, true) == AltBotStartRefusal::NotOwnedAlt);
    assert(CheckAltBotStart(true, true, false, false) == AltBotStartRefusal::NotInWorld);
    // Hardcore is refused.
    assert(CheckAltBotStart(true, true, true, true) == AltBotStartRefusal::Hardcore);
}

static void TestDisplace()
{
    std::vector<PoolDisplaceCandidate> none;
    assert(PickPoolBotToDisplace(none, 7) == 0);

    std::vector<PoolDisplaceCandidate> c(5);
    c[0] = {10, 5, true, false, false, false};   // grouped
    c[1] = {11, 3, false, true, false, false};   // in combat
    c[2] = {12, 2, false, false, true, false};   // in an instance
    c[3] = {13, 1, false, false, false, true};   // has a master
    c[4] = {14, 30, false, false, false, false}; // the only free one
    assert(PickPoolBotToDisplace(c, 0) == 14);
    assert(PickPoolBotToDisplace(c, 99) == 14);

    // Lowest level wins; ties are picked by the roll.
    c.push_back({15, 8, false, false, false, false});
    c.push_back({16, 8, false, false, false, false});
    assert(PickPoolBotToDisplace(c, 0) == 15);
    assert(PickPoolBotToDisplace(c, 1) == 16);
    assert(PickPoolBotToDisplace(c, 2) == 15);

    for (PoolDisplaceCandidate& x : c)
        x.grouped = true;
    assert(PickPoolBotToDisplace(c, 0) == 0);
}

static void TestTarget()
{
    assert(PoolTargetWithAlts(500, 0) == 500);
    assert(PoolTargetWithAlts(500, 3) == 497);
    assert(PoolTargetWithAlts(2, 5) == 0);
}

static void TestSummary()
{
    AltBotSessionCounters c;
    c.startLevel = 12;
    c.endLevel = 14;
    c.quests = 1;
    c.deaths = 2;
    c.startMoney = 10000;
    c.endMoney = 35050;
    assert(FormatAltBotSummary("Alt", c) ==
        "Alt played on its own: level 12 -> 14, 1 quest, 2 deaths, gold +2g 50s 50c.");

    c.endLevel = 12;
    c.quests = 0;
    c.deaths = 1;
    c.endMoney = 9990;
    assert(FormatAltBotSummary("Alt", c) ==
        "Alt played on its own: level 12, 0 quests, 1 death, gold -10c.");
    assert(FormatAltBotMoney(0) == "+0c");
    assert(FormatAltBotMoney(100) == "+1s 0c");
}

int main()
{
    TestStart();
    TestDisplace();
    TestTarget();
    TestSummary();
    std::cout << "test_alt_bot_policy: OK" << std::endl;
    return 0;
}
