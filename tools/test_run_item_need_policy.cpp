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

// The per-tick given-item glue (spec D5, review round 2 Opus 4/5), as TickDungeonRun applies it tick after tick:
// the glue's records (bot -> given) change by the plan's takes and gives.
struct GlueLedger
{
    std::vector<std::uint64_t> given; // the bots holding given items
    int pauses = 0;
    int gives = 0;
    void Apply(const GiveTickPlan& plan, const std::vector<std::uint64_t>& groupBots)
    {
        for (std::uint64_t bot : plan.takeBots)
            given.erase(std::remove(given.begin(), given.end(), bot), given.end());
        if (plan.takeAll)
            given.clear();
        if (plan.pauseForPlayer)
            ++pauses;
        if (plan.giveAgain)
        {
            ++gives;
            given = groupBots;
        }
    }
};

static GiveTickIn In(const GlueLedger& l, const std::vector<std::uint64_t>& group, bool realPlayer, bool system = false,
    bool botKeys = false)
{
    GiveTickIn in;
    in.giveRun = true;
    in.realPlayer = realPlayer;
    in.botKeys = botKeys;
    in.system = system;
    in.running = true;
    in.givenBots = l.given;
    in.groupBots = group;
    return in;
}

// Opus 5: every tick, a given bot no longer in the lead bot's group gives its items back; the others keep theirs.
static void TestGiveTickTakesFromBotsThatLeft()
{
    GlueLedger l;
    l.given = {1, 2, 3};
    std::vector<std::uint64_t> const group = {1, 3, 9};
    GiveTickPlan const plan = DecideGiveTick(In(l, group, false));
    CHECK(plan.takeBots == std::vector<std::uint64_t>{2});
    CHECK(!plan.takeAll && !plan.pauseForPlayer && !plan.giveAgain);
    l.Apply(plan, group);
    CHECK((l.given == std::vector<std::uint64_t>{1, 3}));
    // Not running, done: still taken from a bot that left; done takes the rest.
    GiveTickIn done = In(l, {1}, false);
    done.running = false;
    done.done = true;
    GiveTickPlan const end = DecideGiveTick(done);
    CHECK(end.takeBots == std::vector<std::uint64_t>{3} && end.takeAll && !end.giveAgain);
}

// Opus 4: a real player joins a non-system run: the items go at once and the run pauses ONCE (the tick they were
// taken), not every tick the player stays.
static void TestGiveTickPlayerJoinPausesOnlyOnTheTakeTick()
{
    GlueLedger l;
    std::vector<std::uint64_t> const group = {1, 2, 7};
    l.given = {1, 2};
    for (int tick = 0; tick < 5; ++tick)
        l.Apply(DecideGiveTick(In(l, group, true)), group);
    CHECK(l.given.empty());
    CHECK(l.pauses == 1);
    CHECK(l.gives == 0);
    // A system run: taken at once, its own L6 pause (no pause here).
    GlueLedger s;
    s.given = {1, 2};
    for (int tick = 0; tick < 3; ++tick)
        s.Apply(DecideGiveTick(In(s, group, true, true)), group);
    CHECK(s.given.empty() && s.pauses == 0);
    // The flag on: the bots keep carrying for the party.
    GlueLedger k;
    k.given = {1, 2};
    GiveTickPlan const keep = DecideGiveTick(In(k, group, true, false, true));
    CHECK(!keep.takeAll && !keep.pauseForPlayer && keep.takeBots.empty() && !keep.giveAgain);
}
// Review round 3 Opus fix 1: the lead bot alone (no group) still holds its
// given items. The glue counts the lead itself as in the group, so with
// groupBots = just the lead nothing is taken and nothing is given again; an
// empty group instead takes the lead's items back and gives them again every
// tick (the churn this replaces).
static void TestGiveTickLeadAloneKeepsItsItems()
{
    GlueLedger l;
    l.given = {7};
    std::vector<std::uint64_t> const alone = {7};
    GiveTickPlan const plan = DecideGiveTick(In(l, alone, false));
    CHECK(plan.takeBots.empty());
    CHECK(!plan.takeAll && !plan.pauseForPlayer && !plan.giveAgain);
    GiveTickPlan const churn = DecideGiveTick(In(l, {}, false));
    CHECK(churn.takeBots == std::vector<std::uint64_t>{7} && churn.giveAgain);
}


// The player leaves again (bots only, running, nothing given): given again once; then nothing more.
static void TestGiveTickGivesAgainWhenBotsOnly()
{
    GlueLedger l;
    std::vector<std::uint64_t> const group = {1, 2};
    l.Apply(DecideGiveTick(In(l, group, false)), group);
    CHECK(l.gives == 1 && l.given == group);
    l.Apply(DecideGiveTick(In(l, group, false)), group);
    CHECK(l.gives == 1);
    GiveTickIn off = In(GlueLedger(), group, false);
    off.giveRun = false;
    CHECK(!DecideGiveTick(off).giveAgain);
    GiveTickIn paused = In(GlueLedger(), group, false);
    paused.running = false;
    CHECK(!DecideGiveTick(paused).giveAgain);
}

// Codex fix 3: the start's glue plan. A bot-only refusal (giveOn 0, a blocking
// need nobody carries) carries the refusal LINE (the verdict's text), never the
// empty warning; the glue sends startPlan.refusal (tools/test_run_item_need_wiring.py).
static void TestStartPlanRefusalCarriesText()
{
    DungeonPlan bplan;
    bplan.name = "b";
    bplan.steps = {Move(0), OpenStep(0, 1234, 8444)};
    std::vector<Member> party = {Bot("A"), Bot("B")};
    StartPlan p = PlanStart(bplan, 0, party, false, false);
    CHECK(p.refuse);
    CHECK(!p.refusal.empty());
    CHECK(p.refusal == CheckStart(bplan, 0, party, false).text);
    CHECK(p.refusal.find("8444") != std::string::npos);
    CHECK(p.refusal.find("AiPlayerbot.DungeonRunGiveItems is 0") != std::string::npos);
    CHECK(!p.giveRun);
    // giveOn: never refuses, gives, giveRun set; with a player and the flag off: giveRun false.
    StartPlan g = PlanStart(bplan, 0, party, true, false);
    CHECK(!g.refuse && g.refusal.empty() && g.giveRun && g.give.size() == 2);
    std::vector<Member> mixed = {Player("Hypeer"), Bot("A")};
    StartPlan m = PlanStart(bplan, 0, mixed, true, false);
    CHECK(!m.refuse && !m.giveRun && m.give.empty() && m.warn.find("item 8444") != std::string::npos);
    CHECK(PlanStart(bplan, 0, mixed, true, true).giveRun);
}

// Codex fix 3: the items the glue reads the party's bags for are every need of
// NeedsFrom (derived blocking items too), so a bot carrying a derived item
// counts as its carrier and the start is not refused for it.
static void TestPlanNeedItemsIncludeDerived()
{
    DungeonPlan bplan;
    bplan.name = "b";
    bplan.steps = {Move(0), OpenStep(1, 1234, 8444)};
    ai::PlanLegNeed door;
    door.leg = 1;
    door.item = 11000;
    door.mode = ai::LegNeedMode::Door;
    bplan.needs.push_back(door);
    std::vector<std::uint32_t> const items = PlanNeedItems(bplan, 0);
    CHECK(items.size() == 2 && items[0] == 8444 && items[1] == 11000);
    std::vector<Member> party = {Bot("A", 8444), Bot("B")};
    CHECK(!PlanStart(bplan, 0, party, false, false).refuse);
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
    TestGiveTickLeadAloneKeepsItsItems();
    TestBotKeysFlagGivesWithPlayer();
    TestGiveTickTakesFromBotsThatLeft();
    TestGiveTickPlayerJoinPausesOnlyOnTheTakeTick();
    TestGiveTickGivesAgainWhenBotsOnly();
    TestStartPlanRefusalCarriesText();
    TestPlanNeedItemsIncludeDerived();
    std::cout << "ok\n";
    return 0;
}
