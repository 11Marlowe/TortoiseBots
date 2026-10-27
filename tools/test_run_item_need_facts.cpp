// Standalone check for runtime/RunItemNeedFacts.h (item-leg-skip review round 2, Opus 1/2/6): the party's bags as the
// glue reads them (commands/Commander.cpp ReadRunNeedFacts -> FillLegNeedFacts, pinned by
// tools/test_run_item_need_wiring.py) drive the run's door hold and optional-leg gate through DungeonRun::Tick.
//   g++ -std=c++17 -Wall -Wextra tools/test_run_item_need_facts.cpp -o /tmp/t && /tmp/t
#include "../runtime/RunItemNeedFacts.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace TortoiseBots;
using namespace TortoiseBots::DungeonRun;
using RunItemNeed::Member;
using ai::DungeonPlan;
using ai::PlanStep;
using ai::PlanStepKind;

static PlanStep Move(std::uint8_t leg, float x)
{
    PlanStep s;
    s.leg = leg;
    s.kind = PlanStepKind::Move;
    s.x = x;
    ai::NormalizePlanStep(s);
    return s;
}

// Leg 1 = [move 50] behind the Shadowforge Gate at x 55 (11000, pick 250); leg 2 = [move 100] optional 9240.
static DungeonPlan Plan()
{
    DungeonPlan plan;
    plan.name = "brd";
    ai::PlanLeg leg;
    leg.leg = 1;
    leg.name = "arena";
    plan.legs.push_back(leg);
    plan.steps = {Move(1, 50), Move(2, 100)};
    ai::PlanLegNeed door;
    door.leg = 1;
    door.item = 11000;
    door.mode = ai::LegNeedMode::Door;
    door.pick = 250;
    door.object = 170560;
    door.name = "Shadowforge Key";
    door.how = "Dark Iron Legacy quest";
    plan.needs.push_back(door);
    ai::PlanLegNeed opt;
    opt.leg = 2;
    opt.item = 9240;
    opt.mode = ai::LegNeedMode::Optional;
    plan.needs.push_back(opt);
    return plan;
}

static Member M(const std::string& name, bool bot, std::uint32_t carries = 0, std::uint32_t lockpick = 0)
{
    Member m;
    m.name = name;
    m.bot = bot;
    m.lockpick = lockpick;
    if (carries)
        m.carries.push_back(carries);
    return m;
}

// The glue's facts for one tick: the party (lead bot first), Hypeer the real player, the door found closed at x 55.
static Facts Read(const Run& r, const std::vector<Member>& party)
{
    Facts f;
    f.total = 5;
    f.alive = 5;
    f.carrier = "Tanky";
    f.seen = true;
    PlayerFact p;
    p.name = "Hypeer";
    f.players.push_back(p);
    f.doorNeedRead = true;
    f.doorNeedState = 0;
    f.doorNeedX = 55.0f;
    RunItemNeed::FillLegNeedFacts(r, party, f);
    return f;
}

// Opus 1: a real player carrying the door's key is a qualified holder even when a bot (read first) carries it too:
// no hold, the leg is decided, no "Get it" text anywhere.
static void TestPlayerCarryingTheKeyIsNotHeld()
{
    DungeonPlan plan = Plan();
    Run r = Start(&plan);
    std::vector<Member> party = {M("Tanky", true, 11000), M("Heals", true), M("Hypeer", false, 11000)};
    Facts f = Read(r, party);
    CHECK(f.legNeedRead);
    CHECK(f.legNeedHolders.size() == 1 && f.legNeedHolders[0].first == 11000 && f.legNeedHolders[0].second == "Hypeer");
    CHECK(Tick(r, f) == "move 50.0 0.0 0.0");
    CHECK(r.state == State::Running && r.legNeedDecided.count(1) == 1);
    CHECK(r.note.find("Get it") == std::string::npos);
}

// ... only a bot carries it: held (a bot's key opens nothing for the player), and the note has no "Get it" (the
// key is in the party).
static void TestOnlyABotCarryingTheKeyHoldsWithoutGetIt()
{
    DungeonPlan plan = Plan();
    Run r = Start(&plan);
    std::vector<Member> party = {M("Tanky", true, 11000), M("Hypeer", false)};
    Facts f = Read(r, party);
    CHECK(f.legNeedHolders[0].second == "Tanky");
    CHECK(f.doorNeedCarrier == "Tanky");
    CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Paused && r.doorPaused);
    CHECK(r.note.find("Get it") == std::string::npos);
    // Nobody carries it: the note says where to get it.
    Run n = Start(&plan);
    Facts g = Read(n, {M("Tanky", true), M("Hypeer", false)});
    CHECK(g.legNeedHolders[0].second.empty() && g.doorNeedCarrier.empty());
    Tick(n, g);
    CHECK(n.state == State::Paused && n.note.find("Get it: Dark Iron Legacy quest.") != std::string::npos);
}

// Opus 2: doorNeedLockpick is the best real player's Lockpicking (bots never pick): 250 opens the 250 lock, 249 holds.
static void TestBestPlayerLockpickOpens()
{
    DungeonPlan plan = Plan();
    Run r = Start(&plan);
    Facts f = Read(r, {M("Tanky", true, 0, 300), M("Hypeer", false, 0, 120), M("Roguey", false, 0, 250)});
    CHECK(f.doorNeedLockpick == 250);
    CHECK(Tick(r, f) == "move 50.0 0.0 0.0");
    CHECK(r.legNeedDecided.count(1) == 1);
    Run s = Start(&plan);
    Facts g = Read(s, {M("Tanky", true, 0, 300), M("Roguey", false, 0, 249)});
    CHECK(g.doorNeedLockpick == 249);
    Tick(s, g);
    CHECK(s.state == State::Paused && s.doorPaused);
}

// Opus 6: with DungeonRunBotKeysForPlayers = 1 the bot the glue reads as the key's carrier gets the open order.
static void TestFlagCarrierBotOpens()
{
    DungeonPlan plan = Plan();
    Run r = Start(&plan);
    Facts f = Read(r, {M("Tanky", true), M("Heals", true, 11000), M("Hypeer", false)});
    f.botKeysForPlayers = true;
    CHECK(Tick(r, f) == "open entry:170560@55.0,0.0,0.0 Heals");
}

// The optional gate reads its holder from the same fill: anyone (bots first).
static void TestOptionalHolderIsAnyone()
{
    DungeonPlan plan = Plan();
    Run r = Start(&plan, 1);
    Facts f = Read(r, {M("Tanky", true), M("Hypeer", false, 9240)});
    CHECK(f.legNeedHolders.size() == 1 && f.legNeedHolders[0].second == "Hypeer");
    CHECK(f.doorNeedCarrier.empty());
    Facts g = Read(r, {M("Tanky", true, 9240), M("Hypeer", false, 9240)});
    CHECK(g.legNeedHolders[0].second == "Tanky");
    CHECK(RunItemNeed::LegNeedItems(r) == std::vector<std::uint32_t>{9240});
}

int main()
{
    TestPlayerCarryingTheKeyIsNotHeld();
    TestOnlyABotCarryingTheKeyHoldsWithoutGetIt();
    TestBestPlayerLockpickOpens();
    TestFlagCarrierBotOpens();
    TestOptionalHolderIsAnyone();
    std::cout << "ok\n";
    return 0;
}
