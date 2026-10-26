// Standalone check for runtime/RunItemNeedPolicy.h (spec item-leg-skip C1/F1-6+F6b+F6c):
//   g++ -std=c++17 -Wall -Wextra tools/test_run_item_need_policy.cpp -o /tmp/test_run_item_need_policy && /tmp/test_run_item_need_policy
#include "../runtime/RunItemNeedPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace TortoiseBots::RunItemNeed;
using ai::DungeonPlan;
using ai::PlanStep;
using ai::PlanStepKind;

static PlanStep Move(std::uint8_t leg, float x = 0)
{
    PlanStep s;
    s.leg = leg;
    s.kind = PlanStepKind::Move;
    s.x = x;
    ai::NormalizePlanStep(s);
    return s;
}

static PlanStep LootFor(std::uint8_t leg, std::uint32_t entry, std::uint32_t item)
{
    PlanStep s;
    s.leg = leg;
    s.kind = PlanStepKind::Loot;
    s.entry = entry;
    s.item = item;
    ai::NormalizePlanStep(s);
    return s;
}

static PlanStep OpenStep(std::uint8_t leg, std::uint32_t entry, std::uint32_t item)
{
    PlanStep s;
    s.leg = leg;
    s.kind = PlanStepKind::Open;
    s.entry = entry;
    s.item = item;
    s.x = 1;
    s.y = 1;
    s.z = 1;
    ai::NormalizePlanStep(s);
    return s;
}

static PlanStep UseItem(std::uint8_t leg, std::uint32_t entry, std::uint32_t made)
{
    PlanStep s;
    s.leg = leg;
    s.kind = PlanStepKind::UseItem;
    s.entry = entry;
    s.item = made;
    ai::NormalizePlanStep(s);
    return s;
}

static Member Bot(const std::string& name, std::uint32_t carries = 0)
{
    Member m;
    m.name = name;
    m.bot = true;
    if (carries)
        m.carries.push_back(carries);
    return m;
}

static Member Player(const std::string& name, std::uint32_t carries = 0, std::uint32_t lockpick = 0)
{
    Member m;
    m.name = name;
    m.bot = false;
    m.lockpick = lockpick;
    if (carries)
        m.carries.push_back(carries);
    return m;
}

// A plan with a door row (leg 1), an optional row (leg 2) and an `open` with
// item 8444 made by a `loot` two steps earlier -> 2 needs, 8444 not among
// them; started after the loot -> 8444 is a Blocking need.
static void TestNeedsFromRowsAndDerived()
{
    DungeonPlan plan;
    plan.name = "t";
    PlanStep loot = LootFor(0, 9000, 8444);
    PlanStep openMade = OpenStep(0, 1234, 8444);
    plan.steps = {loot, Move(0), openMade, Move(1), Move(2)};
    ai::PlanLegNeed door;
    door.leg = 1;
    door.item = 11000;
    door.mode = ai::LegNeedMode::Door;
    door.object = 170560;
    door.pick = 250;
    door.name = "Shadowforge Key";
    door.how = "Dark Iron Legacy quest (Searing Gorge/BRD)";
    plan.needs.push_back(door);
    ai::PlanLegNeed opt;
    opt.leg = 2;
    opt.item = 9240;
    opt.mode = ai::LegNeedMode::Optional;
    opt.name = "Mallet of Zul'Farrak";
    plan.needs.push_back(opt);

    std::vector<NeedItem> from0 = NeedsFrom(plan, 0);
    CHECK(from0.size() == 2);
    CHECK(from0[0].item == 11000 && from0[0].mode == ai::LegNeedMode::Door);
    CHECK(from0[0].firstStep == 3 && from0[0].where == "leg 1");
    CHECK(from0[1].item == 9240 && from0[1].mode == ai::LegNeedMode::Optional);

    // Started at the open step: the loot no longer counts as made, so 8444 is a derived Blocking need.
    std::vector<NeedItem> fromMade = NeedsFrom(plan, 2);
    CHECK(fromMade.size() == 3);
    bool has8444 = false;
    for (const NeedItem& n : fromMade)
        if (n.item == 8444)
        {
            has8444 = true;
            CHECK(n.mode == ai::LegNeedMode::Blocking && n.where == "step 3" && n.firstStep == 2);
        }
    CHECK(has8444);

    // The same item in several steps is kept apart.
    DungeonPlan twice;
    twice.name = "w";
    twice.steps = {OpenStep(0, 1111, 8444), OpenStep(0, 2222, 8444)};
    std::vector<NeedItem> both = NeedsFrom(twice, 0);
    CHECK(both.size() == 2);
    CHECK(both[0].where == "step 1" && both[1].where == "step 2");

    // A loot maker counts as made too (the open then needs nothing).
    DungeonPlan made;
    made.name = "m";
    made.steps = {LootFor(0, 9000, 8444), OpenStep(0, 3333, 8444)};
    CHECK(NeedsFrom(made, 0).empty());
    // ... and so does a use_item maker: its product 8444 is no need, while its
    // own entry 7777 is (kept apart: it names the first step).
    DungeonPlan umade;
    umade.name = "u";
    umade.steps = {UseItem(0, 7777, 8444), OpenStep(0, 3333, 8444)};
    std::vector<NeedItem> uneeds = NeedsFrom(umade, 0);
    CHECK(uneeds.size() == 1 && uneeds[0].item == 7777 && uneeds[0].where == "step 1");
}

// 5 bots, one already carries 11000 -> 4 GiveOrders for 11000 plus 5 for 9240; never refuses.
static void TestBotOnlyGetsEveryItemOnEveryBot()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {Move(1), Move(2)};
    ai::PlanLegNeed door;
    door.leg = 1;
    door.item = 11000;
    door.mode = ai::LegNeedMode::Door;
    door.name = "Shadowforge Key";
    plan.needs.push_back(door);
    ai::PlanLegNeed opt;
    opt.leg = 2;
    opt.item = 9240;
    opt.mode = ai::LegNeedMode::Optional;
    opt.name = "Mallet of Zul'Farrak";
    plan.needs.push_back(opt);
    std::vector<Member> party = {Bot("A", 11000), Bot("B"), Bot("C"), Bot("D"), Bot("E")};
    StartVerdict v = CheckStart(plan, 0, party, true);
    CHECK(!v.refuse && v.warn.empty());
    CHECK(v.give.size() == 9);
    std::size_t for9240 = 0, for11000 = 0;
    for (const GiveOrder& g : v.give)
    {
        if (g.item == 9240)
            ++for9240;
        if (g.item == 11000)
        {
            ++for11000;
            CHECK(g.bot != "A");
        }
    }
    CHECK(for9240 == 5 && for11000 == 4);
    // Ordered by item, then party order.
    CHECK(v.give.front().item == 9240 && v.give.front().bot == "A");
}

// giveOn false: a door need with no carrier -> no refusal; a derived blocking need -> refusal.
static void TestBotOnlyGiveOffRefusesOnlyBlocking()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {Move(0), OpenStep(1, 170560, 11000)};
    ai::PlanLegNeed door;
    door.leg = 1;
    door.item = 11000;
    door.mode = ai::LegNeedMode::Door;
    door.name = "Shadowforge Key";
    plan.needs.push_back(door);
    std::vector<Member> party = {Bot("A"), Bot("B")};
    StartVerdict doorOnly = CheckStart(plan, 0, party, false);
    CHECK(!doorOnly.refuse && doorOnly.give.empty());

    DungeonPlan bplan;
    bplan.name = "b";
    bplan.steps = {Move(0), OpenStep(0, 1234, 8444)};
    StartVerdict blocking = CheckStart(bplan, 0, party, false);
    CHECK(blocking.refuse);
    CHECK(blocking.warn.empty());
    CHECK(blocking.give.empty());
    CHECK(blocking.text.find("8444") != std::string::npos);
    CHECK(blocking.text.find("AiPlayerbot.DungeonRunGiveItems is 0") != std::string::npos);
}

// 1 player + 4 bots, giveOn true, flag off: give is empty for every need mix.
static void TestPlayerPartyNeverGetsItems()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {Move(1), Move(2)};
    ai::PlanLegNeed door;
    door.leg = 1;
    door.item = 11000;
    door.mode = ai::LegNeedMode::Door;
    door.pick = 250;
    door.object = 170560;
    door.name = "Shadowforge Key";
    door.how = "Dark Iron Legacy quest (Searing Gorge/BRD)";
    plan.needs.push_back(door);
    ai::PlanLegNeed opt;
    opt.leg = 2;
    opt.item = 9240;
    opt.mode = ai::LegNeedMode::Optional;
    opt.name = "Mallet of Zul'Farrak";
    plan.needs.push_back(opt);
    std::vector<Member> party = {Player("Hypeer"), Bot("A"), Bot("B"), Bot("C"), Bot("D")};
    StartVerdict v = CheckStart(plan, 0, party, true, false);
    CHECK(!v.refuse && v.give.empty());
}

// No carrier, player lockpick 0: no refusal, warn names the key, pick and how; player carries -> no warn;
// a BOT carries -> still warns; rogue 250 -> no warn, 249 -> warns.
static void TestPlayerPartyDoorWarnsNamingKey()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {Move(1), Move(2)};
    ai::PlanLegNeed door;
    door.leg = 1;
    door.item = 11000;
    door.mode = ai::LegNeedMode::Door;
    door.pick = 250;
    door.object = 170560;
    door.name = "Shadowforge Key";
    door.how = "Dark Iron Legacy quest (Searing Gorge/BRD)";
    plan.needs.push_back(door);
    std::vector<Member> party = {Player("Hypeer"), Bot("A")};
    StartVerdict v = CheckStart(plan, 0, party, true, false);
    CHECK(!v.refuse && v.give.empty());
    CHECK(v.warn.find("Shadowforge Key (item 11000)") != std::string::npos);
    CHECK(v.warn.find("Lockpicking 250") != std::string::npos);
    CHECK(v.warn.find("Dark Iron Legacy quest") != std::string::npos);

    std::vector<Member> playerCarries = {Player("Hypeer", 11000), Bot("A")};
    CHECK(CheckStart(plan, 0, playerCarries, true, false).warn.empty());

    std::vector<Member> botCarries = {Player("Hypeer"), Bot("A", 11000)};
    CHECK(!CheckStart(plan, 0, botCarries, true, false).warn.empty());

    std::vector<Member> rogue250 = {Player("Roguey", 0, 250), Bot("A")};
    CHECK(CheckStart(plan, 0, rogue250, true, false).warn.empty());

    std::vector<Member> rogue249 = {Player("Roguey", 0, 249), Bot("A")};
    CHECK(!CheckStart(plan, 0, rogue249, true, false).warn.empty());
}

// Only an optional need, nobody carries -> ok, no warn, no give.
static void TestPlayerPartyOptionalNeverWarns()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {Move(9)};
    ai::PlanLegNeed opt;
    opt.leg = 9;
    opt.item = 9240;
    opt.mode = ai::LegNeedMode::Optional;
    opt.name = "Mallet of Zul'Farrak";
    plan.needs.push_back(opt);
    std::vector<Member> party = {Player("Hypeer"), Bot("A")};
    StartVerdict v = CheckStart(plan, 0, party, true, false);
    CHECK(!v.refuse && v.warn.empty() && v.give.empty());
}

// A derived blocking need with a player, nobody carries -> no refusal, warn names the item and the stop line.
static void TestBlockingWarnsWithPlayer()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {Move(0), OpenStep(0, 1234, 8444)};
    std::vector<Member> party = {Player("Hypeer"), Bot("A")};
    StartVerdict v = CheckStart(plan, 0, party, true, false);
    CHECK(!v.refuse && v.give.empty());
    CHECK(v.warn.find("8444") != std::string::npos);
    CHECK(v.warn.find("stop at the step") != std::string::npos);
}

// botKeysForPlayers true, 1 player + 4 bots -> same give as the bot-only branch, no warn, no refusal;
// flag false -> empty (test 4).
static void TestBotKeysFlagGivesWithPlayer()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {Move(1), Move(2)};
    ai::PlanLegNeed door;
    door.leg = 1;
    door.item = 11000;
    door.mode = ai::LegNeedMode::Door;
    door.name = "Shadowforge Key";
    plan.needs.push_back(door);
    ai::PlanLegNeed opt;
    opt.leg = 2;
    opt.item = 9240;
    opt.mode = ai::LegNeedMode::Optional;
    opt.name = "Mallet of Zul'Farrak";
    plan.needs.push_back(opt);
    std::vector<Member> bots = {Bot("A"), Bot("B"), Bot("C"), Bot("D")};
    StartVerdict botOnly = CheckStart(plan, 0, bots, true, false);
    CHECK(botOnly.give.size() == 8); // 2 items x 4 bots
    std::vector<Member> party = {Player("Hypeer"), Bot("A"), Bot("B"), Bot("C"), Bot("D")};
    StartVerdict v = CheckStart(plan, 0, party, true, true);
    CHECK(!v.refuse && v.warn.empty());
    CHECK(v.give.size() == botOnly.give.size());
    StartVerdict off = CheckStart(plan, 0, party, true, false);
    CHECK(off.give.empty());

    // Empty name falls back to "item <id>".
    DungeonPlan noname;
    noname.name = "n";
    noname.steps = {Move(0), OpenStep(0, 1234, 8444)};
    std::vector<Member> solo = {Player("Hypeer"), Bot("A")};
    StartVerdict w = CheckStart(noname, 0, solo, true, false);
    CHECK(w.warn.find("item 8444") != std::string::npos);
}

// GivenLedger + DecideGiveTick glue (spec F 15-21): TakeAll / TakeAllAndRecheck / GiveAgain / None.
static void TestLedgerTakeAll()
{
    GivenLedger ledger;
    ledger.add("A", 9240, 1);
    ledger.add("B", 9240, 1);
    CHECK(!ledger.empty());
    GiveTick t = DecideGiveTick(true, true, false, true, false, false);
    CHECK(t == GiveTick::TakeAll);
    ledger.takeAll();
    CHECK(ledger.empty());
}

static void TestLedgerTakeAllAndRecheck()
{
    GiveTick t = DecideGiveTick(true, true, false, true, false, true);
    CHECK(t == GiveTick::TakeAllAndRecheck);
}

static void TestLedgerGiveAgain()
{
    GiveTick t = DecideGiveTick(true, false, true, true, false, false);
    CHECK(t == GiveTick::GiveAgain);
}

static void TestLedgerNone()
{
    CHECK(DecideGiveTick(false, true, false, true, false, false) == GiveTick::None);
    CHECK(DecideGiveTick(false, false, false, true, false, false) == GiveTick::None);
    CHECK(DecideGiveTick(true, false, true, false, false, false) == GiveTick::None);
    CHECK(DecideGiveTick(true, false, true, true, true, false) == GiveTick::None);
    // takeBot drops only that bot's record.
    GivenLedger ledger;
    ledger.add("A", 9240, 1);
    ledger.add("B", 11000, 2);
    ledger.takeBot("A");
    CHECK(ledger.takeAllFor("A").empty());
    std::vector<std::pair<std::uint32_t, std::uint32_t>> took = ledger.takeAllFor("B");
    CHECK(took.size() == 1 && took.front().first == 11000 && took.front().second == 2);
    CHECK(ledger.empty());
}

int main()
{
    TestNeedsFromRowsAndDerived();
    TestBotOnlyGetsEveryItemOnEveryBot();
    TestBotOnlyGiveOffRefusesOnlyBlocking();
    TestPlayerPartyNeverGetsItems();
    TestPlayerPartyDoorWarnsNamingKey();
    TestPlayerPartyOptionalNeverWarns();
    TestBlockingWarnsWithPlayer();
    TestBotKeysFlagGivesWithPlayer();
    TestLedgerTakeAll();
    TestLedgerTakeAllAndRecheck();
    TestLedgerGiveAgain();
    TestLedgerNone();
    std::cout << "ok\n";
    return 0;
}
