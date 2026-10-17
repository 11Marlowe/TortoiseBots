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

static void TestProtections()
{
    // B9: autonomous economy is off; companions keep today's behaviour.
    assert(AltBotMayUseEconomy(true) == false);
    assert(AltBotMayUseEconomy(false) == true);
    // B10: greys and whites sell; soulbound/equipped never do.
    assert(AltBotMayVendorItem(0, false, false) == true);
    assert(AltBotMayVendorItem(1, false, false) == true);
    assert(AltBotMayVendorItem(2, false, false) == false);
    assert(AltBotMayVendorItem(0, true, false) == false);
    assert(AltBotMayVendorItem(1, false, true) == false);
    // B10: repair is paid without the gold cheat.
    assert(AltBotRepairIsPaid(false) == true);
    assert(AltBotRepairIsPaid(true) == false);
    // B11: spells, repair, flight only.
    assert(AltBotMaySpendGoldOn(AltBotBuyNeed::TrainerSpell) == true);
    assert(AltBotMaySpendGoldOn(AltBotBuyNeed::Repair) == true);
    assert(AltBotMaySpendGoldOn(AltBotBuyNeed::FlightPath) == true);
    assert(AltBotMaySpendGoldOn(AltBotBuyNeed::Mount) == false);
    assert(AltBotMaySpendGoldOn(AltBotBuyNeed::Bag) == false);
    assert(AltBotMaySpendGoldOn(AltBotBuyNeed::Respec) == false);
    assert(AltBotMaySpendGoldOn(AltBotBuyNeed::OtherVendorBuy) == false);
    // B12: autonomous alts never auto-spend talents.
    assert(AltBotMayAutoSpendTalents(true) == false);
    assert(AltBotMayAutoSpendTalents(false) == true);
    // B13: best score wins, earliest on ties.
    assert(AltBotQuestRewardChoice({{0, 10}, {1, 30}, {2, 20}}) == 1);
    assert(AltBotQuestRewardChoice({{0, 30}, {1, 30}}) == 0);
    assert(AltBotQuestRewardChoice({}) == 0);
    // B14: no room for the old piece means no equip.
    assert(AltBotMayAutoEquip(false) == false);
    assert(AltBotMayAutoEquip(true) == true);
    // B16: drop boost is a pool property.
    assert(AltBotMayGetDropBoost(true) == true);
    assert(AltBotMayGetDropBoost(false) == false);
    // C: Need demotes to Greed for alts; others pass through.
    assert(AltBotGroupLootVote(2, true) == 1);
    assert(AltBotGroupLootVote(1, true) == 1);
    assert(AltBotGroupLootVote(0, true) == 0);
    assert(AltBotGroupLootVote(2, false) == 2);
    // B15: item cheat must be on for the alt.
    assert(AltBotItemCheatCovered(true) == true);
    assert(AltBotItemCheatCovered(false) == false);
    // Footprint: no hearth rebind while autonomous.
    assert(AltBotMayRebindHearth(true) == false);
    assert(AltBotMayRebindHearth(false) == true);
    // Footprint: 10 free quest slots; only abandon own quests.
    assert(AltBotMayAcceptQuest(14) == true);
    assert(AltBotMayAcceptQuest(15) == false);
    assert(AltBotMayAcceptQuest(24) == false);
    assert(AltBotMayAbandonQuest(true, true) == true);
    assert(AltBotMayAbandonQuest(true, false) == false);
    assert(AltBotMayAbandonQuest(false, false) == true);
    // Footprint: rep-lowering kills are never picked solo.
    assert(AltBotMayPickTarget(true) == false);
    assert(AltBotMayPickTarget(false) == true);
}

int main()
{
    TestStart();
    TestDisplace();
    TestTarget();
    TestSummary();
    TestProtections();
    std::cout << "test_alt_bot_policy: OK" << std::endl;
    return 0;
}
