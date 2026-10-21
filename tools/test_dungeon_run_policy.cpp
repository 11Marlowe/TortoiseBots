// Standalone check for runtime/DungeonRunPolicy.h (the run leader's step machine):
//   g++ -std=c++17 -Wall -Wextra tools/test_dungeon_run_policy.cpp -o /tmp/test_dungeon_run_policy && /tmp/test_dungeon_run_policy
#include "../runtime/DungeonRunPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace TortoiseBots::DungeonRun;
using ai::DungeonPlan;
using ai::PlanStep;
using ai::PlanStepKind;
namespace RemoteCommander = TortoiseBots::RemoteCommander;

static PlanStep Step(PlanStepKind kind, float x = 0, float y = 0, float z = 0)
{
    PlanStep s;
    s.kind = kind;
    s.x = x;
    s.y = y;
    s.z = z;
    ai::NormalizePlanStep(s);
    return s;
}

static PlanStep PullStep(float x, std::vector<std::uint32_t> entries, std::uint32_t flags = 0)
{
    PlanStep s;
    s.kind = PlanStepKind::Pull;
    s.x = x;
    s.entries = entries;
    s.flags = flags;
    ai::NormalizePlanStep(s);
    return s;
}

static Facts Party(float x = 0)
{
    Facts f;
    f.total = 5;
    f.alive = 5;
    f.leadX = x;
    f.carrier = "Tanky";
    f.seen = true;  // the step's point within kSeeYd; tests of far steps clear it
    return f;
}

// A living member `yd` from the lead bot, standing on the x axis at that
// distance (the stand-still watch reads its position).
static MemberFact Mem(const std::string& name, float yd, float dz = 0.0f)
{
    MemberFact m;
    m.name = name;
    m.yd = yd;
    m.dz = dz;
    m.x = yd;
    return m;
}

static UnitFact Unit(std::uint64_t guid, std::uint32_t entry, float dist)
{
    UnitFact u;
    u.guid = guid;
    u.entry = entry;
    u.distLead = dist;
    return u;
}

static void TestMove()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {Step(PlanStepKind::Move, 50, 0, 0), Step(PlanStepKind::MoveDirect, 52, 0, 0)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f) == "move 50.0 0.0 0.0");
    CHECK(r.phase == Phase::Walk);
    // Progress: no new order.
    f.leadX = 20;
    CHECK(Tick(r, f).empty());
    // Arrived: next step, the direct move goes out at once.
    f.leadX = 48;
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 1);
    CHECK(Tick(r, f) == "move 52.0 0.0 0.0 direct");
    f.leadX = 52;
    Tick(r, f);
    CHECK(r.state == State::Done);
    CHECK(StatusLine(r) == "TBM:RUN|t|done|2/2|0|move_direct|begin|plan done");
}

static void TestWalkStuckIsBounded()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 50, 0, 0)};
    Run r = Start(&plan);
    Facts f = Party(0);
    int orders = 0;
    for (int i = 0; i < 200 && r.state == State::Running; ++i)
        if (!Tick(r, f).empty())
            ++orders;
    CHECK(r.state == State::Paused);
    CHECK(orders == static_cast<int>(kWalkTries));
    CHECK(r.note.find("stuck") != std::string::npos);
}

// Live (WC run B, rows 28/30/31 and 20 more): a plan row 3-6 yd from the tank
// costs ~13 s -- `cmd move` to a point that close does not move the lead bot at
// all, and the walk waited 12 s before it counted it as arrived. 275 s of a
// 101 min run. A plain walk that gains nothing is done after 3 s now.
static void TestShortHopArrivesWithoutTheRetryWait()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 5, 0, 0), Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f) == "move 5.0 0.0 0.0");
    CHECK(r.step == 0);
    for (int i = 0; i < 4 && r.step == 0 && r.state == State::Running; ++i)
        CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Running && r.step == 1 && r.note == "arrived");
    // The anchor is where the lead bot stands, not the point it could not reach.
    CHECK(r.anchorKnown && r.anchorX == 0.0f);
}

// A direct step is the plan's own line over a navmesh hole: it keeps the 12 s
// rule (and its checkpoint), so a hop it cannot finish is not cut short.
static void TestShortDirectHopKeepsTheLongWait()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::MoveDirect, 5, 0, 0), Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f) == "move 5.0 0.0 0.0 direct");
    for (int i = 0; i < 11; ++i)
    {
        CHECK(Tick(r, f).empty());
        CHECK(r.step == 0);
    }
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 1 && r.note == "arrived");
}

// ... and a hop the lead bot does walk is not cut short: the walk goes on while
// it gains ground (more than 2 yd a look), and it arrives at its point.
static void TestShortHopThatMovesIsNotCut()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 20, 0, 0), Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(26); // 6 yd out: inside kStalledArriveYd, not at the point
    CHECK(Tick(r, f) == "move 20.0 0.0 0.0");
    f.leadX = 23.5f;
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 0 && !r.anchorKnown);
    f.leadX = 21.0f;
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 1 && r.note == "arrived");
    CHECK(r.anchorKnown && r.anchorX == 20.0f); // arrived at the plan's point, not on its own spot
}

// Live bug: a move point just off the mesh; the lead bot stopped 4.6 yd short.
static void TestStalledNearPointCountsAsArrived()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 50, 0, 0), Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(!Tick(r, f).empty());
    f.leadX = 45.4f;
    for (int i = 0; i < 30 && r.step == 0 && r.state == State::Running; ++i)
        CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Running && r.step == 1 && r.note == "arrived");
    // Still farther than kStalledArriveYd: retries, then the stuck pause.
    Run r2 = Start(&plan);
    f.leadX = 0;
    Tick(r2, f);
    f.leadX = 40;
    for (int i = 0; i < 200 && r2.state == State::Running; ++i)
        Tick(r2, f);
    CHECK(r2.state == State::Paused && r2.note.find("stuck") != std::string::npos);
}

// Live bug: a scorpid in the Sentinel pit was pulled, fought and leashed home
// over and over for minutes.
static void TestPullLoopOnOneTargetPauses()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {7078}), Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = true;
    f.units = {Unit(42, 7078, 10)};
    int pulls = 0;
    for (int i = 0; i < 2000 && r.state == State::Running && r.step == 0; ++i)
    {
        f.anyInCombat = false;
        if (Tick(r, f) == "pullback 42")
        {
            ++pulls;
            // Every pull starts a short fight; the unit survives it.
            f.anyInCombat = true;
            for (int k = 0; k < 5; ++k)
                Tick(r, f);
        }
    }
    CHECK(pulls == static_cast<int>(kMaxPullsPerTarget));
    // Left as out of reach: the step ends "skipped, not cleared", the run goes on.
    CHECK(r.state == State::Running && r.step == 1 && r.note.find("skipped, not cleared") != std::string::npos);
    CHECK(r.unreachable.count(42) == 1);
}

// Live: ten Cleft Scorpids in a pit, each pull fought one that leashed home.
static void TestOutOfReachPackIsSkippedNotCleared()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {7078}), Step(PlanStepKind::Move, 30)};
    Run r = Start(&plan);
    Facts f = Party(0);
    for (std::uint64_t g = 1; g <= 10; ++g)
        f.units.push_back(Unit(g, 7078, 10 + g));
    int pulls = 0;
    for (int i = 0; i < 3000 && r.state == State::Running && r.step == 0; ++i)
    {
        f.anyInCombat = false;
        if (Tick(r, f).compare(0, 9, "pullback ") == 0)
        {
            ++pulls;
            f.anyInCombat = true;
            for (int k = 0; k < 5; ++k)
                Tick(r, f);
        }
    }
    CHECK(pulls == static_cast<int>(kFruitlessPulls));
    CHECK(r.state == State::Running && r.step == 1);
    CHECK(r.note.find("skipped, not cleared: 10 units") != std::string::npos);
    // Never pulled again on the way (hop pulls skip them too).
    f.units.clear();
    f.hopUnits = {Unit(3, 7078, 13)};
    CHECK(Tick(r, f) == "move 30.0 0.0 0.0");
}

// Live cascade: no Shaft -> no Staff -> Keystone -> Seal wait, each failing.
static void TestMissingItemPausesOnce()
{
    DungeonPlan plan;
    PlanStep loot = Step(PlanStepKind::Loot, 1, 2, 3);
    loot.entry = 6910;
    loot.item = 7741;
    PlanStep use = Step(PlanStepKind::UseItem);
    use.entry = 7740;
    use.item = 7733;
    PlanStep open = Step(PlanStepKind::Open, 4, 5, 6);
    open.entry = 124371;
    open.item = 7733;
    PlanStep wait = Step(PlanStepKind::WaitObject, 7, 8, 9);
    wait.entry = 124372;
    wait.param = 1;
    plan.steps = {loot, use, open, wait, Step(PlanStepKind::Move, 1)};
    Run r = Start(&plan);
    Facts f = Party(0);
    std::string order = Tick(r, f);
    CHECK(order.compare(0, 5, "loot ") == 0);
    // The corpse is gone: the order is refused. One pause, naming the item.
    OrderRefused(r, order, "not-found: No spawned game object or corpse of that entry is near the point");
    CHECK(r.state == State::Paused && r.note.find("item 7741 missing") != std::string::npos);
    CHECK(Skip(r));
    CHECK(r.missing.count(7741) == 1 && r.state == State::Running && r.step == 1);
    // The medallion is there, but its use needs the Shaft: refused, no pause.
    f.carrierHasEntryItem = true;
    order = Tick(r, f);
    CHECK(order == "use 7740 Tanky");
    OrderRefused(r, order, "busy: The cast failed (spell check 40).");
    CHECK(r.state == State::Running && r.step == 2 && r.missing.count(7733) == 1);
    // No staff: the Keystone and the Seal wait are left out with notes.
    Tick(r, f);
    CHECK(r.state == State::Running && r.step == 3 && r.note.find("left out") != std::string::npos);
    Tick(r, f);
    CHECK(r.state == State::Running && r.step == 4 && r.note.find("open before it") != std::string::npos);
}

// ai::kPlanFlagGoneIsDone (Zul'Farrak's graves): a loot or open row with the flag counts the object being gone as
// done; the same row without the flag pauses as before.
static PlanStep GraveStep(PlanStepKind kind, std::uint32_t flags)
{
    PlanStep s = Step(kind, 1858.7f, 1053.8f, 8.9f);
    s.entry = 128403;
    s.flags = flags;
    ai::NormalizePlanStep(s);
    return s;
}

static void TestGoneChestWithFlagIsDone()
{
    for (PlanStepKind kind : {PlanStepKind::Loot, PlanStepKind::Open})
    {
        // The search answers `not-found` (nothing spawned of that entry near the point): done, one note, no pause.
        DungeonPlan plan;
        plan.steps = {GraveStep(kind, ai::kPlanFlagGoneIsDone), Step(PlanStepKind::Move, 9)};
        Run r = Start(&plan);
        Facts f = Party(0);
        std::string order = Tick(r, f);
        CHECK(order.compare(0, 5, kind == PlanStepKind::Loot ? "loot " : "open ") == 0);
        OrderRefused(r, order, "not-found: No spawned game object of that entry is near the point (or the bot).");
        CHECK(r.state == State::Running && r.step == 1);
        CHECK(r.note.find("object gone: counted as done") != std::string::npos);

        // The object despawned while the bot walked to it: the task fails `invalid-target`: done too.
        Run r2 = Start(&plan);
        order = Tick(r2, f);
        f.carrierBusy = true;
        f.taskReply = 2;
        f.taskText = "invalid-target: The object is gone.";
        Tick(r2, f);
        CHECK(r2.state == State::Running && r2.step == 1);
        CHECK(r2.note.find("object gone: counted as done") != std::string::npos);
    }
}

static void TestGoneChestWithoutFlagPauses()
{
    for (PlanStepKind kind : {PlanStepKind::Loot, PlanStepKind::Open})
    {
        DungeonPlan plan;
        plan.steps = {GraveStep(kind, 0), Step(PlanStepKind::Move, 9)};
        Run r = Start(&plan);
        Facts f = Party(0);
        std::string order = Tick(r, f);
        OrderRefused(r, order, "not-found: No spawned game object of that entry is near the point (or the bot).");
        CHECK(r.state == State::Paused && r.step == 0);
        Run r2 = Start(&plan);
        order = Tick(r2, f);
        f.carrierBusy = true;
        f.taskReply = 2;
        f.taskText = "invalid-target: The object is gone.";
        Tick(r2, f);
        CHECK(r2.state == State::Paused && r2.step == 0);
    }
}

// The flag settles a gone object only: any other refusal (in combat, too far, busy) still pauses, and a row that
// names an item stays a missing-item case.
static void TestGoneIsDoneOnlyForAGoneObject()
{
    DungeonPlan plan;
    plan.steps = {GraveStep(PlanStepKind::Loot, ai::kPlanFlagGoneIsDone), Step(PlanStepKind::Move, 9)};
    Run r = Start(&plan);
    Facts f = Party(0);
    std::string order = Tick(r, f);
    OrderRefused(r, order, "in-combat: The bot is in combat.");
    CHECK(r.state == State::Paused && r.step == 0);

    DungeonPlan withItem;
    PlanStep s = GraveStep(PlanStepKind::Loot, ai::kPlanFlagGoneIsDone);
    s.item = 7741;
    withItem.steps = {s, Step(PlanStepKind::Move, 9)};
    Run r2 = Start(&withItem);
    order = Tick(r2, f);
    OrderRefused(r2, order, "not-found: No spawned game object or corpse of that entry is near the point");
    CHECK(r2.state == State::Paused && r2.step == 0);
}

// After a wipe: resume revives the party and gathers it before the step.
static void TestResumeRegroups()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40)};
    Run r = Start(&plan);
    r.autoRegroups = kAutoRegroupsPerStep; // the regroups by itself are used up: it pauses
    Facts f = Party(0);
    f.dead = 3;
    f.alive = 2;
    Tick(r, f);
    CHECK(r.state == State::Paused);
    CHECK(Resume(r));
    CHECK(r.autoRegroups == 0);
    CHECK(Tick(r, f) == "revive party");
    CHECK(r.state == State::Running && r.note.find("regroup") == 0);
    CHECK(Tick(r, f).empty());
    // Up again but spread out: gather on the lead bot, then the step.
    f.dead = 0;
    f.alive = 5;
    f.maxMemberYd = 80;
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    f.maxMemberYd = 5;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    // Revive that never works is bounded.
    Run r2 = Start(&plan);
    r2.autoRegroups = kAutoRegroupsPerStep;
    Facts g = Party(0);
    g.dead = 3;
    g.alive = 2;
    Tick(r2, g);
    Resume(r2);
    for (int i = 0; i < 400 && r2.state == State::Running; ++i)
        Tick(r2, g);
    CHECK(r2.state == State::Paused && r2.note.find("still dead") != std::string::npos);
}

// A far move point (the lead came back from the entrance): navmesh hops.
static void TestFarMoveWalksBackInHops()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 200)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = false;
    f.approachKnown = true;
    f.approachX = 40;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    f.leadX = 40;
    CHECK(Tick(r, f).empty());
    f.seen = true;
    CHECK(Tick(r, f) == "move 200.0 0.0 0.0");
}

// The Sentinel ramp: a direct step gathers the party first.
static void TestDirectStepGathersFirst()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::MoveDirect, 5)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 30;
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    CHECK(Tick(r, f).empty());
    f.maxMemberYd = 4;
    CHECK(Tick(r, f) == "move 5.0 0.0 0.0 direct");
}

// `.bot run skip` while running goes on running.
static void TestSkipWhileRunning()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), Step(PlanStepKind::Move, 10)};
    Run r = Start(&plan);
    Facts f = Party(0);
    Tick(r, f);
    CHECK(Skip(r));
    CHECK(r.state == State::Running && r.step == 1 && !r.regroup);
    CHECK(Tick(r, f) == "move 10.0 0.0 0.0");
}

// Live: `cmd move -159.5 281.6 -49.1` refused unreachable (a point up a
// ledge). The glue finds a reachable point near it; the step walks there.
static void TestUnreachableMoveDetours()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 50, 0, 0), Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    std::string order = Tick(r, f);
    CHECK(order == "move 50.0 0.0 0.0");
    // Too far from the plan's point: not a stand-in.
    CHECK(!UseDetour(r, 30, 0, 0));
    CHECK(UseDetour(r, 44, 2, 0));
    CHECK(r.state == State::Running && r.detour);
    // Only once per step.
    CHECK(!UseDetour(r, 45, 0, 0));
    CHECK(Tick(r, f) == "move 44.0 2.0 0.0");
    f.leadX = 44;
    f.leadY = 2;
    Tick(r, f);
    CHECK(r.step == 1 && r.note.find("nearest reachable point") != std::string::npos);
    // The detour is per step.
    CHECK(!r.detour);
    // Not for a step that is not a move.
    CHECK(!UseDetour(r, 1, 1, 1));
}

static void TestPullGatherAndFight()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851, 4852})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = true;
    f.units = {Unit(11, 4851, 45), Unit(12, 4852, 38)};
    // Scattered party: gather first (the lead holds where it stands).
    f.maxMemberYd = 35;
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    CHECK(r.phase == Phase::Gather);
    CHECK(Tick(r, f).empty());
    f.maxMemberYd = 5;
    // Nearest first.
    CHECK(Tick(r, f) == "pullback 12");
    CHECK(r.phase == Phase::AwaitPull);
    // The fight: no orders while it lasts.
    f.anyInCombat = true;
    for (int i = 0; i < 30; ++i)
        CHECK(Tick(r, f).empty());
    CHECK(r.note == "fighting");
    // Both died; low mana afterwards: rest, then the step is done.
    f.anyInCombat = false;
    f.units.clear();
    f.minHealerManaPct = 20;
    CHECK(Tick(r, f) == "rest");
    CHECK(r.phase == Phase::Rest);
    f.minHealerManaPct = 85;
    Tick(r, f);
    CHECK(r.state == State::Done || r.step == 1);
}

// RFC r3-19z20 step 22: the lead bot holds "stay" while the run rests (an
// attack order had ended its stay, and its idle formation walk took it into
// the lava basin while the healer drank). Only in the rest phase of a running
// run, only for a living lead out of combat that is off "stay".
static void TestLeadHoldsStayWhileResting()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(11, 4851, 38)};
    f.maxMemberYd = 5;
    // Before the pull and during the fight: no hold from here.
    CHECK(Tick(r, f) == "pullback 11");
    CHECK(!LeadStayDueWhileResting(r, true, false, false, false));
    f.anyInCombat = true;
    Tick(r, f);
    CHECK(!LeadStayDueWhileResting(r, true, true, false, false));
    // The fight is over and the healer is dry: the run rests, the lead holds.
    f.anyInCombat = false;
    f.units.clear();
    f.minHealerManaPct = 20;
    CHECK(Tick(r, f) == "rest");
    CHECK(r.phase == Phase::Rest);
    CHECK(LeadStayDueWhileResting(r, true, false, false, false));
    // Sent once: a lead already on "stay" is left alone.
    CHECK(!LeadStayDueWhileResting(r, true, false, true, false));
    // A lead standing in magma is left free to walk out of it.
    CHECK(!LeadStayDueWhileResting(r, true, false, false, true));
    // A dead lead or one pulled into a fight again is not put on "stay".
    CHECK(!LeadStayDueWhileResting(r, false, false, false, false));
    CHECK(!LeadStayDueWhileResting(r, true, true, false, false));
    // Still resting a tick later (the formation walk would start any time).
    Tick(r, f);
    CHECK(r.phase == Phase::Rest);
    CHECK(LeadStayDueWhileResting(r, true, false, false, false));
    // A paused run gives no orders.
    Run paused = r;
    paused.state = State::Paused;
    CHECK(!LeadStayDueWhileResting(paused, true, false, false, false));
    // Rested: the step goes on and the hold is the next order's business.
    f.minHealerManaPct = 85;
    Tick(r, f);
    CHECK(r.phase != Phase::Rest);
    CHECK(!LeadStayDueWhileResting(r, true, false, false, false));
}

static void TestUnseenPackIsApproachedNotCleared()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {4851})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = false;
    // The glue gives the next hop on the navmesh path; one move per hop.
    f.approachKnown = true;
    f.approachX = 35;
    CHECK(Tick(r, f) == "move 35.0 0.0 0.0");
    f.leadX = 10;
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 0 && r.state == State::Running);
    // At the hop and still not in sight: the next hop.
    f.leadX = 34;
    CHECK(Tick(r, f).empty());
    f.approachX = 70;
    CHECK(Tick(r, f) == "move 70.0 0.0 0.0");
    f.leadX = 70;
    f.seen = true;
    Tick(r, f);
    CHECK(r.state == State::Done);
    // No path toward it at all: pause, never a straight-line guess.
    Run r2 = Start(&plan);
    Facts g = Party(0);
    g.seen = false;
    Tick(r2, g);
    CHECK(r2.state == State::Paused && r2.note.find("no walkable path") == 0);
    // A hop within kArriveYd of the lead bot goes nowhere: no path at once
    // (it used to "arrive" 8 times first; r3-19z20 B-FIX-2). The hop cap's
    // own test is TestApproachHopCapCountsHopsWithoutProgress.
    Run r3 = Start(&plan);
    g.approachKnown = true;
    g.approachX = 1;
    Tick(r3, g);
    CHECK(r3.state == State::Paused && r3.note.find("no walkable path") == 0);
}

// Ragefire 19z13, trogg cave pull 13: the pack is 70 yd down a ramp. The hop
// toward it was broken off by the pull's gather as soon as the pack came in
// sight, while the anchor was still the previous stand 11 yd higher (more than
// kStragglerDzYd): the gather "went back" to it, the next hop came forward
// again, eight times, then the hop cap. The hop is walked to its end first, and
// its point becomes the anchor.
static void TestFarPackHopIsNotUndoneByTheGatherBack()
{
    DungeonPlan plan;
    plan.steps = {PullStep(-238.0f, {11318})};
    plan.steps[0].y = -36.0f;
    plan.steps[0].z = -56.4f;
    Run r = Start(&plan);
    r.restDue = false;
    r.anchorKnown = true; // the previous step's stand, at the top of the ramp
    r.anchorX = -169.1f; r.anchorY = -34.9f; r.anchorZ = -40.9f;
    Facts f = Party(-169.1f);
    f.leadY = -34.9f; f.leadZ = -40.9f;
    f.seen = false;
    f.approachKnown = true;
    f.approachX = -205; f.approachY = -34; f.approachZ = -52;
    CHECK(Tick(r, f) == "move -205.0 -34.0 -52.0");
    // Half way down the pack comes in sight (60 yd): the hop goes on.
    f.leadX = -200.7f; f.leadY = -35.6f; f.leadZ = -48.5f;
    f.seen = true;
    f.units = {Unit(1, 11318, 38)};
    CHECK(Tick(r, f).empty());
    CHECK(r.phase == Phase::Walk && r.note == "approaching the pack");
    // At the hop's point: it is the anchor and the pull goes out, no gather back.
    f.leadX = -205; f.leadY = -34; f.leadZ = -52;
    f.units = {Unit(1, 11318, 34)};
    CHECK(Tick(r, f) == "pullback 1");
    CHECK(r.anchorX == -205.0f && r.anchorZ == -52.0f);
    CHECK(r.note.find("gathering back") == std::string::npos);
    // A unit of the pack within kHopClearYd, the followers far behind: the hop
    // is over and the pull goes out from here. Neither a gather beside the pack
    // nor a walk back to the old anchor, however many ticks it takes.
    Run q = Start(&plan);
    q.restDue = false;
    q.anchorKnown = true;
    q.anchorX = -169.1f; q.anchorY = -34.9f; q.anchorZ = -40.9f;
    Facts g = Party(-169.1f);
    g.leadY = -34.9f; g.leadZ = -40.9f;
    g.seen = false;
    g.approachKnown = true;
    g.approachX = -205; g.approachY = -34; g.approachZ = -52;
    Tick(q, g);
    g.leadX = -222; g.leadY = -34; g.leadZ = -54;
    g.seen = true;
    g.maxMemberYd = 40;
    g.units = {Unit(1, 11318, 17)};
    bool pulled = false;
    for (int i = 0; i < 40 && q.state == State::Running; ++i)
    {
        std::string const order = Tick(q, g);
        CHECK(order.compare(0, 5, "move ") != 0);
        CHECK(q.note.find("gathering") == std::string::npos);
        if (order == "pullback 1")
        {
            pulled = true;
            break;
        }
    }
    CHECK(pulled && q.state == State::Running && q.pulls == 1 && q.anchorX == -169.1f);
    // The fight starts and ends: the next pull is an ordinary one again.
    g.anyInCombat = true;
    Tick(q, g);
    CHECK(q.state == State::Running && q.fighting);
    // A player's move next to the visible pack, the party behind: the anchor stays too.
    Run m = Start(&plan);
    m.restDue = false;
    m.anchorKnown = true;
    m.anchorX = -169.1f; m.anchorY = -34.9f; m.anchorZ = -40.9f;
    Facts h = g;
    h.units = {Unit(1, 11318, 15)};
    Tick(m, h);
    CHECK(m.anchorX == -169.1f && m.anchorZ == -40.9f);
}

// A hop loop that really gets nowhere still pauses after kMaxApproachHops, with
// how near it got; a long way walked in hops that do get nearer is not capped.
static void TestApproachHopCapCountsHopsWithoutProgress()
{
    DungeonPlan plan;
    plan.steps = {PullStep(1000, {1})};
    Run r = Start(&plan);
    r.restDue = false;
    Facts f = Party(0);
    f.seen = false;
    f.approachKnown = true;
    for (int hop = 1; hop <= 3 * static_cast<int>(kMaxApproachHops); ++hop)
    {
        f.approachX = f.leadX + 40;
        CHECK(Tick(r, f) == "move " + Num(f.approachX) + " 0.0 0.0");
        f.leadX = f.approachX; // the hop is walked, 40 yd nearer
        CHECK(Tick(r, f).empty());
        CHECK(r.state == State::Running);
    }
    // The lead bot is pushed back each time: no progress, 8 hops, the pause.
    Run s = Start(&plan);
    s.restDue = false;
    Facts g = Party(0);
    g.seen = false;
    g.approachKnown = true;
    for (int i = 0; i < 100 && s.state == State::Running; ++i)
    {
        g.approachX = 40;
        Tick(s, g);
        g.leadX = 40;
        Tick(s, g);
        g.leadX = 0; // dragged back
    }
    CHECK(s.state == State::Paused);
    CHECK(s.note.find("8 approach hops did not bring the party within 60.0 yd of the pack") == 0);
    CHECK(s.note.find("nearest it got: 1000.0 yd") != std::string::npos);
}

// Uldaman 19z13, Revelosh loot step 20: the lead bot may not loot the corpse
// (quest item 7741, another member's turn). The step is done once any party bot
// carries the item; else the bot whose turn it is, then each living bot, is
// tried before the run pauses.
static void TestLootStepIsDoneWhenAnyPartyBotHasTheItem()
{
    DungeonPlan plan;
    PlanStep loot = Step(PlanStepKind::Loot, 1, 2, 3);
    loot.entry = 6910;
    loot.item = 7741;
    plan.steps = {loot, Step(PlanStepKind::Move, 9)};
    // Before the order.
    Run r = Start(&plan);
    Facts f = Party(0);
    f.members = {Mem("Tanky", 0), Mem("Healy", 5)};
    f.partyHasItem = true;
    f.itemHolder = "Healy";
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 1 && r.note.find("Healy already carries item 7741") != std::string::npos);
    // After a refusal (Healy took the item while the order was out).
    Run q = Start(&plan);
    Facts g = Party(0);
    g.members = f.members;
    std::string const order = Tick(q, g);
    CHECK(order.compare(0, 5, "loot ") == 0);
    OrderRefused(q, order, "failed: This corpse has nothing this bot may loot under the group's loot rules");
    CHECK(q.state == State::Running && q.step == 0);
    g.partyHasItem = true;
    g.itemHolder = "Healy";
    CHECK(Tick(q, g).empty());
    CHECK(q.step == 1 && q.state == State::Running);
}

static void TestLootRefusalTriesTheTurnBotThenEachBot()
{
    DungeonPlan plan;
    PlanStep loot = Step(PlanStepKind::Loot, 1, 2, 3);
    loot.entry = 6910;
    loot.item = 7741;
    plan.steps = {loot, Step(PlanStepKind::Move, 9)};
    char const* rights = "failed: This corpse has nothing this bot may loot under the group's loot rules (another "
        "member's turn, already looted, or not tapped by the group)";
    Run r = Start(&plan);
    Facts f = Party(0);
    f.members = {Mem("Tanky", 0), Mem("Healy", 5), Mem("Stabby", 6)};
    f.corpseChecked = true;
    f.corpseHasItem = true; // still on the corpse
    std::string order = Tick(r, f);
    CHECK(order.find(" Tanky 7741") != std::string::npos);
    OrderRefused(r, order, rights);
    CHECK(r.state == State::Running);
    // The corpse's round-robin turn is Stabby's: he is tried first.
    f.lootTurn = "Stabby";
    order = Tick(r, f);
    CHECK(order.compare(0, 5, "loot ") == 0 && order.find(" Stabby 7741") != std::string::npos);
    CHECK(r.state == State::Running && r.taskBot == "Stabby");
    OrderRefused(r, order, rights);
    // Then each bot not tried yet.
    order = Tick(r, f);
    CHECK(order.find(" Healy 7741") != std::string::npos);
    // His task is accepted and the item lands in his bags: done.
    f.taskReply = 1;
    f.partyHasItem = true;
    f.itemHolder = "Healy";
    Tick(r, f);
    CHECK(r.state == State::Running && r.step == 1);

    // Everybody refused: one pause naming who was tried and the skip hint.
    Run p = Start(&plan);
    Facts g = f;
    g.partyHasItem = false;
    g.taskReply = 0;
    g.lootTurn.clear();
    for (int i = 0; i < 3; ++i)
    {
        order = Tick(p, g);
        CHECK(order.compare(0, 5, "loot ") == 0);
        OrderRefused(p, order, rights);
    }
    Tick(p, g);
    CHECK(p.state == State::Paused);
    CHECK(p.note.find("(tried Tanky Healy Stabby)") != std::string::npos);
    CHECK(p.note.find("item 7741 missing") != std::string::npos);

    // The item is gone from the corpse and in no bag: no point in trying the
    // rest; the pause says so plainly.
    Run e = Start(&plan);
    Facts h = f;
    h.partyHasItem = false;
    h.taskReply = 0;
    h.corpseHasItem = false;
    order = Tick(e, h);
    OrderRefused(e, order, rights);
    Tick(e, h);
    CHECK(e.state == State::Paused);
    CHECK(e.note.find("item 7741 not on corpse and not in any bag") != std::string::npos);
    CHECK(e.note.find("item 7741 missing") != std::string::npos);
    // A refusal that is not about the loot rules still pauses at once.
    Run n = Start(&plan);
    order = Tick(n, f);
    OrderRefused(n, order, "not-found: No corpse of that entry is near the point");
    CHECK(n.state == State::Paused && n.note.find("not-found") != std::string::npos);
    // A bot off the lead bot's map (yd 1000) is never tried, not even on its round-robin turn.
    Run m = Start(&plan);
    Facts k = f;
    k.taskReply = 0;
    k.partyHasItem = false;
    k.members = {Mem("Tanky", 0), Mem("Healy", 5), Mem("Stabby", 1000)};
    k.lootTurn = "Stabby";
    order = Tick(m, k);
    OrderRefused(m, order, rights);
    order = Tick(m, k);
    CHECK(order.find(" Healy 7741") != std::string::npos && m.taskBot == "Healy");
}

// ---- loot step: free-for-all detour (AiPlayerbot.DungeonRunLootFfa) ----

static char const* const kFfaRights = "failed: This corpse has nothing this bot may loot under the group's loot rules "
    "(another member's turn, already looted, or not tapped by the group)";

static DungeonPlan FfaPlan()
{
    DungeonPlan plan;
    PlanStep loot = Step(PlanStepKind::Loot, 1, 2, 3);
    loot.entry = 6910;
    loot.item = 7741;
    plan.steps = {loot, Step(PlanStepKind::Move, 9)};
    return plan;
}

// The facts of a party under group loot (3) whose lead bot leads it, the key on.
static Facts FfaParty()
{
    Facts f = Party(0);
    f.members = {Mem("Tanky", 0), Mem("Healy", 5), Mem("Stabby", 6)};
    f.lootFfaOn = true;
    f.lootMethod = RemoteCommander::kLootGroup;
    f.leadLeadsGroup = true;
    f.corpseChecked = true;
    f.corpseHasItem = true;
    return f;
}

// Order sequence: loot refused on the rules -> lootmethod ffa -> (its ACK) loot by the carrier -> the task's
// answer -> lootmethod group -> the step ends.
static void TestLootFfaOrderSequence()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    f.lootMethod = RemoteCommander::kLootGroup;
    std::string order = Tick(r, f);
    CHECK(order.compare(0, 5, "loot ") == 0 && order.find(" Tanky 7741") != std::string::npos);
    OrderRefused(r, order, kFfaRights);
    // Before NextLootBot: the method is changed first, once.
    CHECK(Tick(r, f) == "lootmethod ffa");
    CHECK(r.lootFfaLive && r.lootRestore == RemoteCommander::kLootGroup && r.lootFfa == LootFfa::Ordered);
    // The order was accepted: the next tick reads ffa and sends the loot to the carrier (not another bot).
    f.lootMethod = RemoteCommander::kLootFfa;
    order = Tick(r, f);
    CHECK(order.compare(0, 5, "loot ") == 0 && order.find(" Tanky 7741") != std::string::npos);
    CHECK(r.orderSent && r.lootFfa == LootFfa::Looting && r.step == 0);
    // The task runs: nothing is sent, the method is not touched.
    f.carrierBusy = true;
    for (int i = 0; i < 3; ++i)
        CHECK(Tick(r, f).empty() && r.lootFfa == LootFfa::Looting);
    // The answer: the method goes back first, then the answer is read.
    f.taskReply = 1;
    CHECK(Tick(r, f) == "lootmethod group");
    CHECK(r.lootFfa == LootFfa::Restoring && r.step == 0);
    f.lootMethod = RemoteCommander::kLootGroup;
    f.carrierBusy = false;
    f.carrierHasItem = true;
    f.partyHasItem = true;
    f.itemHolder = "Tanky";
    Tick(r, f);
    CHECK(r.step == 1 && r.state == State::Running && r.lootFfa == LootFfa::Idle);
    CHECK(r.note.find("loot done") != std::string::npos);
    // The next running tick reads the method back and lets the flag go, with no second order.
    CHECK(Tick(r, f).compare(0, 11, "lootmethod ") != 0);
    CHECK(!r.lootFfaLive && r.lootRestoreTries == 0);
}

// An ERR answer restores too, then the old path goes on (the next bot), without a second ffa.
static void TestLootFfaRestoresOnAnErrAnswerAndNeverTwice()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    f.lootMethod = RemoteCommander::kLootRoundRobin;
    OrderRefused(r, Tick(r, f), kFfaRights);
    CHECK(Tick(r, f) == "lootmethod ffa");
    f.lootMethod = RemoteCommander::kLootFfa;
    CHECK(Tick(r, f).compare(0, 5, "loot ") == 0);
    f.carrierBusy = true;
    f.taskReply = 2;
    f.taskText = kFfaRights;
    CHECK(Tick(r, f) == "lootmethod rr"); // the saved method, by the name the order parses
    f.lootMethod = RemoteCommander::kLootRoundRobin;
    f.carrierBusy = false;
    std::string const order = Tick(r, f); // the answer is read: refused on the rules -> the next bot
    CHECK(r.lootFfa == LootFfa::Idle);
    CHECK(order.empty() || order.compare(0, 11, "lootmethod ") != 0);
    for (int i = 0; i < 3; ++i)
    {
        std::string const next = Tick(r, f);
        CHECK(next.compare(0, 11, "lootmethod ") != 0);
        if (next.compare(0, 5, "loot ") == 0)
            OrderRefused(r, next, kFfaRights);
    }
    CHECK(r.lootFfaUsed);
}

// The restore is what the glue sends when the run pauses or stops while the method is ffa: the flag and the
// saved method stay set through a pause (fail-if-removed: lootFfaLive / lootRestore set at the ffa order).
static void TestLootFfaStaysLiveThroughAPause()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    f.lootMethod = RemoteCommander::kLootNeedBeforeGreed;
    OrderRefused(r, Tick(r, f), kFfaRights);
    CHECK(Tick(r, f) == "lootmethod ffa");
    f.lootMethod = RemoteCommander::kLootFfa;
    CHECK(Tick(r, f).compare(0, 5, "loot ") == 0);
    // The run pauses (the cap, a player's order) with the loot order out and the method on ffa.
    Pause(r, "something stopped the run");
    CHECK(r.state == State::Paused && r.lootFfaLive && r.lootRestore == RemoteCommander::kLootNeedBeforeGreed);
    CHECK(std::string(RemoteCommander::LootMethodName(r.lootRestore)) == "nbg");
    // A resume starts the step afresh but keeps the flag until the glue has put the method back
    // (a resume can come before its next tick).
    CHECK(Resume(r));
    CHECK(r.lootFfa == LootFfa::Idle && !r.lootFfaUsed && r.lootFfaLive);
}

// The loot task ends with no ACK or ERR line seen (not busy any more, reply 0): the method goes back all the same.
static void TestLootFfaRestoresWhenTheTaskEndsUnanswered()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    OrderRefused(r, Tick(r, f), kFfaRights);
    CHECK(Tick(r, f) == "lootmethod ffa");
    f.lootMethod = RemoteCommander::kLootFfa;
    CHECK(Tick(r, f).compare(0, 5, "loot ") == 0);
    f.carrierBusy = true;
    CHECK(Tick(r, f).empty());
    f.carrierBusy = false;
    CHECK(Tick(r, f) == "lootmethod group");
}

// A refused ffa order: the old path (NextLootBot) goes on, the method was never changed.
static void TestLootFfaRefusedFallsBackToTheOldPath()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    OrderRefused(r, Tick(r, f), kFfaRights);
    CHECK(Tick(r, f) == "lootmethod ffa");
    OrderRefused(r, "lootmethod ffa", "lead-unavailable: The lead bot does not lead the group.");
    CHECK(r.state == State::Running && r.lootFfa == LootFfa::Idle && !r.lootFfaLive);
    f.lootTurn = "Stabby";
    std::string const order = Tick(r, f);
    CHECK(order.compare(0, 5, "loot ") == 0 && order.find(" Stabby 7741") != std::string::npos);
    CHECK(r.taskBot == "Stabby");
    // An ACK that did not take (the method still reads group): the same.
    Run q = Start(&plan);
    Facts g = FfaParty();
    OrderRefused(q, Tick(q, g), kFfaRights);
    CHECK(Tick(q, g) == "lootmethod ffa");
    std::string const next = Tick(q, g); // g.lootMethod is still group
    CHECK(next.compare(0, 11, "lootmethod ") != 0 && q.lootFfa == LootFfa::Idle);
    CHECK(q.state == State::Running);
}

// A regroup (a wipe, a lead death, a fight cap) resets the step between the ffa ACK and the restore and keeps the
// run Running: the very next tick orders the method back, whatever the step; the tick after reads it and is done.
static void TestLootFfaRegroupBetweenAckAndRestoreRestores()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    OrderRefused(r, Tick(r, f), kFfaRights);
    CHECK(Tick(r, f) == "lootmethod ffa");
    f.lootMethod = RemoteCommander::kLootFfa;
    CHECK(Tick(r, f).compare(0, 5, "loot ") == 0);
    f.carrierBusy = true;
    CHECK(Tick(r, f).empty());
    ResetStep(r); // the regroup redoes the step
    CHECK(r.lootFfa == LootFfa::Idle && r.lootFfaLive && r.state == State::Running);
    CHECK(Tick(r, f) == "lootmethod group");
    CHECK(r.lootRestoreTries == 1);
    // It took: the facts read the saved method; no second order, the flag is gone.
    f.lootMethod = RemoteCommander::kLootGroup;
    CHECK(Tick(r, f).compare(0, 11, "lootmethod ") != 0);
    CHECK(!r.lootFfaLive && r.lootRestoreTries == 0 && r.state == State::Running);
    // The redone step may go through the detour once more: the method is not ffa any more.
    CHECK(r.lootFfaUsed == false);
}

// The restore is refused or does not take: one order a tick, three in all, then the run pauses naming the key.
static void TestLootFfaRestoreRefusedRetriesThenPauses()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    f.lootMethod = RemoteCommander::kLootFfa;
    r.lootFfaLive = true;
    r.lootRestore = RemoteCommander::kLootNeedBeforeGreed;
    for (int i = 1; i <= 3; ++i)
    {
        std::string const order = Tick(r, f);
        CHECK(order == "lootmethod nbg");
        OrderRefused(r, order, "lead-unavailable: The lead bot does not lead the group.");
        CHECK(r.state == State::Running && r.lootRestoreTries == static_cast<std::uint32_t>(i));
    }
    CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Paused && r.lootFfaLive); // the glue tries once more as the run pauses, and clears the flag
    CHECK(r.note.find("still free-for-all") != std::string::npos);
    CHECK(r.note.find("the lead bot's group") != std::string::npos);
    CHECK(r.note.find("AiPlayerbot.DungeonRunLootFfa") != std::string::npos);
    CHECK(r.note.find("lootmethod nbg") != std::string::npos);
}

// Only "no group" or a method that is not ffa ends the flag; the key switched off meanwhile does not.
static void TestLootFfaFlagEndsOnlyWhenTheMethodIsNotFfa()
{
    DungeonPlan plan = FfaPlan();
    // The key was switched off by a reload while the detour was live: the restore still goes out.
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        f.lootFfaOn = false;
        f.lootMethod = RemoteCommander::kLootFfa;
        r.lootFfaLive = true;
        r.lootRestore = RemoteCommander::kLootRoundRobin;
        CHECK(Tick(r, f) == "lootmethod rr" && r.lootFfaLive);
    }
    // ... and in the Ordered tick (the key off there, the method ffa): the detour goes on, the flag stays.
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        OrderRefused(r, Tick(r, f), kFfaRights);
        CHECK(Tick(r, f) == "lootmethod ffa");
        f.lootFfaOn = false;
        f.lootMethod = RemoteCommander::kLootFfa;
        CHECK(Tick(r, f).compare(0, 5, "loot ") == 0 && r.lootFfaLive);
    }
    // No group (-1): nothing to put back.
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        f.lootMethod = -1;
        r.lootFfaLive = true;
        r.lootRestore = RemoteCommander::kLootGroup;
        CHECK(Tick(r, f).compare(0, 11, "lootmethod ") != 0 && !r.lootFfaLive);
    }
    // A player set another method meanwhile (Ordered tick reads nbg, not ffa, not the saved one): no order, flag gone.
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        OrderRefused(r, Tick(r, f), kFfaRights);
        CHECK(Tick(r, f) == "lootmethod ffa");
        f.lootMethod = RemoteCommander::kLootNeedBeforeGreed;
        CHECK(Tick(r, f).compare(0, 11, "lootmethod ") != 0);
        CHECK(!r.lootFfaLive && r.lootFfa == LootFfa::Idle);
    }
}

// A bot dies while the detour waits for a busy carrier; the WaitForFallen regroup does not reset the step: the
// detour ends there and the method is ordered back on the next tick (not after the whole regroup).
static void TestLootFfaOrderedThenFallenRegroupRestores()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    OrderRefused(r, Tick(r, f), kFfaRights);
    CHECK(Tick(r, f) == "lootmethod ffa");
    f.lootMethod = RemoteCommander::kLootFfa;
    f.carrierBusy = true;
    CHECK(Tick(r, f).empty() && r.lootFfa == LootFfa::Ordered);
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Stabby"};
    f.members = {Mem("Tanky", 0), Mem("Healy", 5)};
    Tick(r, f);
    CHECK(r.regroup && r.state == State::Running && r.lootFfa == LootFfa::Idle && r.lootFfaLive);
    // The regroup's ticks: the restore is the first order out.
    CHECK(Tick(r, f) == "lootmethod group");
}

// A carrier that stays busy: the ffa wait is bounded; on the cap the method goes back and the step goes the old way.
static void TestLootFfaOrderedWaitIsBounded()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = FfaParty();
    OrderRefused(r, Tick(r, f), kFfaRights);
    CHECK(Tick(r, f) == "lootmethod ffa");
    f.lootMethod = RemoteCommander::kLootFfa;
    f.carrierBusy = true;
    int ticks = 0;
    while (r.lootFfa == LootFfa::Ordered && ticks < 60)
    {
        CHECK(Tick(r, f).empty());
        ++ticks;
    }
    CHECK(ticks >= 19 && ticks <= 22 && r.lootFfa == LootFfa::Idle && r.state == State::Running);
    CHECK(r.lootFfaLive);
    CHECK(Tick(r, f) == "lootmethod group");
}

// No ffa without the item, without the key, without the lead bot leading the group, for a method the order cannot
// restore, or when the item is gone from the corpse; the shortcuts come first.
static void TestLootFfaConditions()
{
    DungeonPlan plan = FfaPlan();
    // No item on the step: the old path (the next bot).
    DungeonPlan bare = FfaPlan();
    bare.steps[0].item = 0;
    {
        Run r = Start(&bare);
        Facts f = FfaParty();
        OrderRefused(r, Tick(r, f), kFfaRights);
        std::string const order = Tick(r, f);
        CHECK(order.compare(0, 11, "lootmethod ") != 0 && r.lootFfa == LootFfa::Idle);
    }
    // Key off (the facts' default), not the leader, already ffa, no group: the old path.
    for (int variant = 0; variant < 4; ++variant)
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        if (variant == 0)
            f.lootFfaOn = false;
        else if (variant == 1)
            f.leadLeadsGroup = false;
        else if (variant == 2)
            f.lootMethod = RemoteCommander::kLootFfa;
        else
            f.lootMethod = -1;
        OrderRefused(r, Tick(r, f), kFfaRights);
        for (int i = 0; i < 2; ++i)
            CHECK(Tick(r, f).compare(0, 11, "lootmethod ") != 0);
        CHECK(!r.lootFfaLive && r.lootFfa == LootFfa::Idle);
    }
    // The item is not on the corpse and in no bag: no point (the usual pause).
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        OrderRefused(r, Tick(r, f), kFfaRights);
        f.corpseHasItem = false;
        Tick(r, f);
        CHECK(r.state == State::Paused && r.lootFfa == LootFfa::Idle);
    }
    // 'Already carries' and partyHasItem come first: no order at all.
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        OrderRefused(r, Tick(r, f), kFfaRights);
        f.partyHasItem = true;
        f.itemHolder = "Healy";
        CHECK(Tick(r, f).empty());
        CHECK(r.step == 1 && r.lootFfa == LootFfa::Idle && !r.lootFfaLive);
    }
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        f.carrierHasItem = true;
        CHECK(Tick(r, f).empty() && r.step == 1 && !r.lootFfaLive);
    }
    // A refusal that is not about the loot rules pauses at once, no ffa.
    {
        Run r = Start(&plan);
        Facts f = FfaParty();
        OrderRefused(r, Tick(r, f), "not-found: No corpse of that entry is near the point");
        CHECK(r.state == State::Paused && !r.lootFfaLive);
    }
}

// The key off: the facts' defaults (what the glue leaves when the key is 0) give byte-identical orders to the old path.
static void TestLootFfaKeyOffIsTheOldPath()
{
    DungeonPlan plan = FfaPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.members = {Mem("Tanky", 0), Mem("Healy", 5), Mem("Stabby", 6)};
    f.corpseChecked = true;
    f.corpseHasItem = true;
    CHECK(!f.lootFfaOn && f.lootMethod == -1 && !f.leadLeadsGroup);
    std::string order = Tick(r, f);
    CHECK(order == "loot entry:6910@1.0,2.0,3.0 Tanky 7741");
    OrderRefused(r, order, kFfaRights);
    f.lootTurn = "Stabby";
    CHECK(Tick(r, f) == "loot entry:6910@1.0,2.0,3.0 Stabby 7741");
    CHECK(!r.lootFfaUsed && !r.lootFfaLive);
}

// open / use_item go to the bot that carries every input of the step: its item
// (or the item to use) and `param`. Loot another bot keeps for other reasons
// does not count.
static void TestItemCarrierIsPickedFromTheStepsOwnInputs()
{
    std::vector<BagFact> bags(3);
    bags[0].name = "Tanky";
    bags[1].name = "Healy";
    bags[2].name = "Stabby";
    std::string split;
    // The Medallion use: 7740, and the Shaft 7741 in `param`.
    std::vector<std::uint32_t> inputs = {7740, 7741};
    bags[0].held = {7740};
    bags[1].held = {7740, 7741};
    CHECK(PickItemCarrier(bags, inputs, "Tanky", &split) == "Healy");
    // The lead bot carries all: it keeps the step.
    bags[0].held = {7740, 7741};
    CHECK(PickItemCarrier(bags, inputs, "Tanky", &split) == "Tanky");
    // The Keystone: only its key matters; two bots carry unrelated loot (the
    // glue lists only the step's own inputs, so those do not show up at all).
    std::vector<std::uint32_t> key = {7733};
    bags[0].held.clear();
    bags[1].held.clear();
    bags[2].held = {7733};
    CHECK(PickItemCarrier(bags, key, "Tanky", &split) == "Stabby");
    CHECK(split.empty());
    // Split: the lead bot stays the carrier and the note says who holds what.
    bags[0].held = {7740};
    bags[1].held = {7741};
    bags[2].held.clear();
    CHECK(PickItemCarrier(bags, inputs, "Tanky", &split) == "Tanky");
    CHECK(split == "Tanky holds 7740; Healy holds 7741");
    // The only carrier is dead or off the map: never chosen; the note says where the item is.
    bags[0].held = {7740};
    bags[1].held = {7740, 7741};
    bags[1].away = "dead";
    split.clear();
    CHECK(PickItemCarrier(bags, inputs, "Tanky", &split) == "Tanky");
    CHECK(split == "Tanky holds 7740; item 7740, 7741 is with Healy, who is dead");
    bags[1].away = "not in the instance";
    split.clear();
    CHECK(PickItemCarrier(bags, inputs, "Tanky", &split) == "Tanky");
    CHECK(split.find("who is not in the instance") != std::string::npos);
    // ... and another available bot that carries everything is preferred over the unavailable one.
    bags[2].held = {7740, 7741};
    CHECK(PickItemCarrier(bags, inputs, "Tanky", &split) == "Stabby");
    bags[1].away.clear();
    bags[1].held = {7741};
    bags[2].held.clear();
    // Nobody has the first input: no split note (the missing-item rules apply).
    split.clear();
    bags[0].held.clear();
    CHECK(PickItemCarrier(bags, inputs, "Tanky", &split) == "Tanky" && split.empty());
}

// While an open / use_item order is out, its bot stays the carrier. Uldaman:
// Healy (not the lead bot) used the Medallion 7740 with the Shaft 7741; both
// are consumed and the Staff 7733 is in Healy's bags, so no bot carries the
// inputs and PickItemCarrier alone would fall back to the lead bot.
static void TestOutstandingCarrierKeepsTheOrdersBot()
{
    std::vector<BagFact> bags(3);
    bags[0].name = "Tanky";
    bags[0].guid = 11;
    bags[1].name = "Healy";
    bags[1].guid = 12;
    bags[2].name = "Stabby";
    bags[2].guid = 13;
    std::vector<std::uint32_t> inputs = {7740, 7741};
    CHECK(PickItemCarrier(bags, inputs, "Tanky") == "Tanky"); // what the bags alone say
    // Still present: pinned, the order's bot keeps it.
    std::string name;
    CHECK(OutstandingCarrierVerdict(true, 12, bags, &name) == CarrierVerdict::Pinned && name == "Healy");
    // No order out yet (or a retried step: ResetStep cleared orderSent), or a new order (carrierRaw 0).
    CHECK(OutstandingCarrierVerdict(false, 12, bags) == CarrierVerdict::NoOrder);
    CHECK(OutstandingCarrierVerdict(true, 0, bags) == CarrierVerdict::NoOrder);
    // That bot left the party, died or went off the map: the order is retired, never re-picked.
    CHECK(OutstandingCarrierVerdict(true, 14, bags) == CarrierVerdict::Retire);
    bags[1].away = "dead";
    CHECK(OutstandingCarrierVerdict(true, 12, bags) == CarrierVerdict::Retire);
    bags[1].away = "not in the instance";
    CHECK(OutstandingCarrierVerdict(true, 12, bags) == CarrierVerdict::Retire);
}

// A retired order (its bot gone) goes out again to the fresh pick; the old
// task is not read as finished from the replacement's bags.
static void TestRetiredUseItemOrderGoesOutAgain()
{
    DungeonPlan plan;
    PlanStep use = Step(PlanStepKind::UseItem);
    use.entry = 7740;
    use.item = 7733;
    use.param = 7741;
    plan.steps = {use, Step(PlanStepKind::Move, 9)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.carrier = "Healy";
    f.carrierHasEntryItem = true;
    CHECK(Tick(r, f) == "use 7740 Healy");
    CHECK(r.orderSent && r.phase == Phase::AwaitTask);
    // Healy died with the order out; Tanky now carries both inputs.
    RetireOrder(r);
    CHECK(!r.orderSent && r.phase == Phase::Begin && r.state == State::Running);
    Facts g = Party(0);
    g.carrierHasEntryItem = true;
    CHECK(Tick(r, g) == "use 7740 Tanky");
    CHECK(r.state == State::Running && r.step == 0);
    // Without the retire the replacement's bags would end the task: the pause the fix prevents.
    Run q = Start(&plan);
    CHECK(Tick(q, f) == "use 7740 Healy");
    Facts h = Party(0);
    CHECK(Tick(q, h).empty());
    CHECK(q.state == State::Paused && q.note.find("is not in Tanky's bags") != std::string::npos);
}

// The use_item pauses, naming who holds what, when the inputs are split.
static void TestUseItemPausesOnSplitInputs()
{
    DungeonPlan plan;
    PlanStep use = Step(PlanStepKind::UseItem);
    use.entry = 7740;
    use.item = 7733;
    use.param = 7741;
    plan.steps = {use};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.carrierHasEntryItem = true;
    f.itemsSplit = "Tanky holds 7740; Healy holds 7741";
    Tick(r, f);
    CHECK(r.state == State::Paused);
    CHECK(r.note.find("no bot carries every item it needs: Tanky holds 7740; Healy holds 7741") != std::string::npos);
    // Healy carries both: the glue names her the carrier and the use goes to her.
    Run q = Start(&plan);
    Facts g = Party(0);
    g.carrier = "Healy";
    g.carrierHasEntryItem = true;
    CHECK(Tick(q, g) == "use 7740 Healy");
}

// Uldaman run 4, archaedas wait_object 135/135: Archaedas was dead and the
// vault door 124369 active, yet "a fight lasted more than 180 s" paused the run.
// The hold's long fight had left fightMs over the plain 180 s ceiling, and the
// combat flag stood a few seconds after the last kill.
static void TestWaitObjectDoneWinsOverAStaleFightTimer()
{
    DungeonPlan plan;
    PlanStep wait = Step(PlanStepKind::WaitObject, 1, 2, 3);
    wait.entry = 124369;
    wait.param = 1;
    plan.steps = {wait, Step(PlanStepKind::Move, 9)};
    // The door is in the wanted state and only the combat flag stands: done.
    Run r = Start(&plan);
    r.fighting = true;
    r.fightMs = kFightCapMs + 5000;
    Facts f = Party(0);
    f.anyInCombat = true;
    f.staleCombat = true;
    f.objectState = 1;
    // One stale snapshot proves nothing: the wait step is finished once the
    // flag has stood unbroken for kStaleConfirmMs.
    Tick(r, f);
    CHECK(r.step == 0 && r.state == State::Running);
    for (std::uint32_t i = 0; i < kStaleConfirmMs / kTickMs && r.step == 0; ++i)
        Tick(r, f);
    CHECK(r.state == State::Running && r.step == 1);
    // A break (one engaged tick) starts the confirmation over.
    Run b = Start(&plan);
    b.fighting = true;
    Facts e = f;
    Tick(b, e);
    Tick(b, e);
    e.staleCombat = false;
    Tick(b, e);
    e.staleCombat = true;
    Tick(b, e);
    CHECK(b.step == 0 && b.staleRunMs == kTickMs);
    // A real fight (a hostile engaged) with the door open keeps its handling
    // and its ceiling: the step does not advance.
    Run q = Start(&plan);
    q.fighting = true;
    q.fightMs = kFightCapMs + 5000;
    Facts g = Party(0);
    g.anyInCombat = true;
    g.objectState = 1;
    Tick(q, g);
    CHECK(q.state == State::Paused && q.step == 0 &&
        q.note.find("a fight lasted more than 180 s") != std::string::npos);
    // A stale flag with the door shut is waited out for the grace period (the
    // timer does not run), then the ceiling runs again: it ends in a pause.
    Run p = Start(&plan);
    p.fighting = true;
    p.fightMs = kFightCapMs + 5000;
    Facts h = g;
    h.objectState = 0;
    h.staleCombat = true;
    Tick(p, h);
    CHECK(p.state == State::Running && p.fightMs == kFightCapMs + 5000);
    for (std::uint32_t i = 0; i < kStaleCombatGraceMs / kTickMs + 2 && p.state == State::Running; ++i)
        Tick(p, h);
    CHECK(p.state == State::Paused && p.note.find("a fight lasted more than 180 s") != std::string::npos);
    // The grace is spent once per fight: alternating 19 stale ticks with one
    // engaged tick does not renew it, the ceiling still fires.
    Run a = Start(&plan);
    a.fighting = true;
    a.fightMs = kFightCapMs + 5000;
    Facts k = h;
    for (int i = 0; i < 200 && a.state == State::Running; ++i)
    {
        k.staleCombat = (i % 20) != 19;
        Tick(a, k);
    }
    CHECK(a.state == State::Paused && a.note.find("a fight lasted more than 180 s") != std::string::npos);
    // Two satisfied wait_object steps over one uninterrupted stale flag: the
    // second does not get a fresh grace.
    DungeonPlan two;
    two.steps = {plan.steps[0], plan.steps[0]};
    Run w = Start(&two);
    w.fighting = true;
    Facts z = f;
    for (int i = 0; i < 10 && w.step == 0; ++i)
        Tick(w, z);
    CHECK(w.step == 1 && w.staleMs >= kStaleConfirmMs);
    // The flag has stood this long already; the next step's confirmation is
    // unbroken too, so it finishes at once, the grace is not renewed.
    std::uint32_t const spent = w.staleMs;
    Tick(w, z);
    CHECK(w.staleMs > spent);
    // Stale combat, then a wipe regroup by itself when it clears: the next
    // combat gets a fresh grace and a full confirmation.
    Run g2 = Start(&plan);
    g2.fighting = true;
    g2.staleMs = kStaleCombatGraceMs;
    g2.staleRunMs = kStaleConfirmMs;
    Facts wipe = Party(0);
    wipe.dead = kWipeDead;
    wipe.alive = 2;
    wipe.anyInCombat = false;
    Tick(g2, wipe);
    CHECK(g2.regroup && g2.staleMs == 0 && g2.staleRunMs == 0);
    // The flag clearing resets the grace.
    Run c = Start(&plan);
    c.fighting = true;
    Tick(c, h);
    CHECK(c.staleMs > 0);
    h.anyInCombat = false;
    Tick(c, h);
    CHECK(c.staleMs == 0);
    // A fight that goes on into the next step starts that step's ceiling over.
    Run n = Start(&plan);
    n.fighting = true;
    n.fightMs = kFightCapMs + 5000;
    NextStep(n, "hold done");
    CHECK(n.fightMs == 0 && n.fighting);
}

static void TestPullThatStartsNoFightIsPassed()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {4851})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = true;
    f.units = {Unit(7, 4851, 10)};
    int pulls = 0;
    for (int i = 0; i < 120 && r.state == State::Running && r.step == 0; ++i)
    {
        std::string order = Tick(r, f);
        if (order == "pullback 7")
            ++pulls;
    }
    CHECK(pulls == static_cast<int>(kPullTriesPerUnit));
    // The unit is passed, so the step ends (the unit is alive: it is not "cleared", it is passed).
    CHECK(r.state == State::Done);
}

static void TestAmbushWalksThrough()
{
    DungeonPlan plan;
    plan.steps = {PullStep(20, {4850}, ai::kPlanFlagAmbush)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = true;
    CHECK(Tick(r, f) == "move 20.0 0.0 0.0");
    f.leadX = 19;
    Tick(r, f);
    CHECK(r.state == State::Done);
}

static void TestBossKillOrderAndTeamGate()
{
    DungeonPlan plan;
    PlanStep boss = Step(PlanStepKind::Boss, 10);
    boss.entry = 6906;
    boss.entries = {6907, 6908};
    ai::NormalizePlanStep(boss);
    plan.steps = {boss, Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = true;
    f.units = {Unit(1, 6906, 5), Unit(2, 6908, 9), Unit(3, 6907, 12)};
    // First listed add (6907) before the nearer boss.
    CHECK(Tick(r, f) == "pullback 3");
    // Not hostile (an Alliance party and the Lost Dwarves): left alone, step done.
    Run r2 = Start(&plan);
    for (UnitFact& u : f.units)
        u.hostile = false;
    Tick(r2, f);
    CHECK(r2.step == 1 && r2.note.find("friendly") != std::string::npos);
}

static void TestHopPullsNearUnitsFirst()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.hopUnits = {Unit(9, 4852, 35)};
    CHECK(Tick(r, f) == "pullback 9");
    f.hopUnits.clear();
    f.anyInCombat = true;
    Tick(r, f);
    f.anyInCombat = false;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
}

// Uldaman rerun pull 58 (build 19z11): the fight ended with the priest and the
// mage dead (fewer than kWipeDead) and the next pullback went out at once. Now
// the dead are revived and the party gathers before the pull.
static void TestFewDeadReviveBeforePull()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(21, 7011, 30)};
    f.dead = 2;
    f.alive = 3;
    // No pull while two are dead: the regroup's revive goes out instead.
    CHECK(Tick(r, f).empty());
    CHECK(r.regroup && r.note.find("waiting for 2 dead: reviving and gathering") == 0);
    CHECK(Tick(r, f) == "revive party");
    CHECK(Tick(r, f).empty());
    // Up again and gathered: the pull follows.
    f.dead = 0;
    f.alive = 5;
    std::string order;
    for (int i = 0; i < 5 && order.empty(); ++i)
        order = Tick(r, f);
    CHECK(order == "pullback 21");
    // In a fight with one dead nothing changes: the fight is the bots' own.
    Run r2 = Start(&plan);
    f.dead = 1;
    f.alive = 4;
    f.anyInCombat = true;
    CHECK(Tick(r2, f).empty());
    CHECK(!r2.regroup && r2.note.find("fighting") == 0);
}

// Review of #1055: the 1-2 dead rule stays out of scripted events (a quiet
// gap between waves is not the end of the fight).
static void TestFewDeadRuleSkipsEvents()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Run r = Start(&plan);
    r.eventOn = true;
    Facts f = Party(0);
    f.units = {Unit(21, 7011, 30)};
    f.dead = 1;
    f.alive = 4;
    Tick(r, f);
    CHECK(!r.regroup);
    // A move leading into a hold counts as the event too.
    DungeonPlan holdPlan;
    holdPlan.steps = {Step(PlanStepKind::Move, 40), Step(PlanStepKind::Hold, 40)};
    Run h = Start(&holdPlan);
    Tick(h, f);
    CHECK(!h.regroup);
}

// Review of #1055: a fight during a regroup (a pack met on the corpse run)
// gets the combat block: fight note, fight ceiling, and a third death reaches
// the regroup by itself.
static void TestRegroupFightsFirst()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.dead = 1;
    f.alive = 4;
    BeginRegroup(r);
    f.anyInCombat = true;
    CHECK(Tick(r, f).empty());
    CHECK(r.note == "fighting" && r.fighting);
    // The fight ceiling still holds inside the regroup.
    for (std::uint32_t ms = 0; ms <= kFightCapMs && r.state == State::Running; ms += kTickMs)
        Tick(r, f);
    CHECK(r.state == State::Paused && r.note.find("a fight lasted") == 0);
    // Three dead in the regroup's fight: the regroup by itself follows the fight.
    Run w = Start(&plan);
    BeginRegroup(w);
    f.dead = 1;
    f.alive = 4;
    CHECK(Tick(w, f).empty());
    f.dead = 3;
    f.alive = 2;
    CHECK(Tick(w, f).empty());
    CHECK(w.note.find("3 dead: fighting on") == 0);
    f.anyInCombat = false;
    Tick(w, f);
    CHECK(w.regroup && w.autoRegroups == 1);
}

// Review of #1055: a living rezzer gets kPartyRezWaitMs to raise the dead
// before `revive party` releases them; without one the revive goes out at once.
static void TestRegroupWaitsForPartyRez()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(21, 7011, 30)};
    f.dead = 1;
    f.alive = 4;
    f.rezzer = true;
    CHECK(Tick(r, f).empty()); // the regroup starts
    std::uint32_t waited = 0;
    std::string order;
    while (order.empty() && waited < 60000)
    {
        order = Tick(r, f);
        waited += kTickMs;
        if (order.empty())
            CHECK(r.note.find("a resurrection in the party first") != std::string::npos);
    }
    CHECK(order == "revive party" && waited >= kPartyRezWaitMs && waited <= kPartyRezWaitMs + 2 * kTickMs);
    // Raised in time: no revive at all, the pull follows.
    Run q = Start(&plan);
    Tick(q, f);
    Tick(q, f);
    f.dead = 0;
    f.alive = 5;
    order.clear();
    for (int i = 0; i < 5 && order.empty(); ++i)
        order = Tick(q, f);
    CHECK(order == "pullback 21");
    // No rezzer: the revive goes out on the regroup's first tick.
    Run n = Start(&plan);
    f.dead = 1;
    f.alive = 4;
    f.rezzer = false;
    Tick(n, f);
    CHECK(Tick(n, f) == "revive party");
}

// The rez hold (AiPlayerbot.DungeonRunRezHold, RezHoldPolicy.h). The roster: a tank, a priest
// healer and three more; the mage (index 2) is dead when `dead` is set.
static std::vector<TortoiseBots::RezHold::Member> RezRoster(bool mageDead, std::uint32_t priestMana, float corpseYd = -1.0f)
{
    using namespace TortoiseBots::RezHold;
    std::vector<Member> m(5);
    m[0].tank = true;
    m[1].rez = RezClass::Priest;
    m[1].healer = true;
    m[1].mana = priestMana;
    m[1].rezCost = 160;
    if (mageDead)
    {
        m[2].dead = true;
        m[2].corpseYd = corpseYd;
    }
    return m;
}

// Ticks a regroup with one dead until a revive goes out; the seconds it took (0: none in 200 s).
static std::uint32_t SecondsToRevive(const Facts& f, std::string* note = nullptr)
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Run r = Start(&plan);
    BeginRegroup(r);
    for (std::uint32_t ms = kTickMs; ms <= 200000; ms += kTickMs)
    {
        std::string const order = Tick(r, f);
        if (note)
            *note = r.note;
        if (order == "revive party")
            return ms / 1000;
    }
    return 0;
}

// Key off (the default): the rez hold's facts are never looked at. A run fed a roster that would
// change the verdict (rezHold false) ticks exactly as one fed none, order and note, tick by tick,
// and the old fixed wait is still the one that decides.
static void TestRezHoldOffIsUnchanged()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Facts plain = Party(0);
    plain.units = {Unit(21, 7011, 30)};
    plain.dead = 1;
    plain.alive = 4;
    plain.rezzer = true;
    Facts withRoster = plain;
    withRoster.rezHold = false;
    withRoster.rezMembers = RezRoster(true, 0, 500.0f); // a priest with no mana and a far body: would send at once
    Run a = Start(&plan);
    Run b = Start(&plan);
    std::uint32_t firstOrder = 0;
    for (std::uint32_t ms = kTickMs; ms <= 300000; ms += kTickMs)
    {
        std::string const oa = Tick(a, plain);
        std::string const ob = Tick(b, withRoster);
        CHECK(oa == ob);
        CHECK(a.note == b.note);
        CHECK(a.regroupMs == b.regroupMs && a.reviveSent == b.reviveSent && a.state == b.state);
        if (!firstOrder && oa == "revive party")
            firstOrder = ms;
    }
    CHECK(firstOrder >= kPartyRezWaitMs && firstOrder <= kPartyRezWaitMs + 3 * kTickMs);
    // No rezzer in the plain facts: the old rule revives at once, with or without the roster.
    plain.rezzer = false;
    withRoster.rezzer = false;
    CHECK(SecondsToRevive(plain) == SecondsToRevive(withRoster));
    CHECK(SecondsToRevive(plain) <= 3);
}

// Key on: the hold waits for the priest well past the old 15 s, waits for its mana, and sends the
// revive when a rezzer is missing, the body is far or the budget is spent.
static void TestRezHoldOn()
{
    using namespace TortoiseBots::RezHold;
    Facts f = Party(0);
    f.dead = 1;
    f.alive = 4;
    f.rezHold = true;
    f.rezzer = true; // the old flag is not what decides any more
    std::string note;
    // A priest with mana: held for kHoldMs (not 15 s), then the revive.
    f.rezMembers = RezRoster(true, 500);
    std::uint32_t s = SecondsToRevive(f, &note);
    CHECK(s >= kHoldMs / 1000 && s <= kHoldMs / 1000 + 3);
    f.rezMembers = RezRoster(true, 500);
    {
        DungeonPlan plan;
        plan.steps = {PullStep(40, {7011})};
        Run r = Start(&plan);
        BeginRegroup(r);
        for (int i = 0; i < 30; ++i)
            CHECK(Tick(r, f).empty());
        CHECK(r.note.find("a resurrection in the party first") != std::string::npos);
        CHECK(r.note.find("waiting for a resurrection") != std::string::npos);
    }
    // The priest is short of mana: waited for up to kManaWaitMs.
    f.rezMembers = RezRoster(true, 20);
    s = SecondsToRevive(f, &note);
    CHECK(s >= kManaWaitMs / 1000 && s <= kManaWaitMs / 1000 + 3);
    {
        DungeonPlan plan;
        plan.steps = {PullStep(40, {7011})};
        Run r = Start(&plan);
        BeginRegroup(r);
        for (int i = 0; i < 20; ++i)
            CHECK(Tick(r, f).empty());
        CHECK(r.note.find("the resurrector drinks first") != std::string::npos);
    }
    // No resurrector at all: the revive goes out on the first ticks.
    f.rezMembers = RezRoster(true, 500);
    f.rezMembers[1].rez = RezClass::None;
    CHECK(SecondsToRevive(f) <= 3);
    // The body lies too far: no wait either.
    f.rezMembers = RezRoster(true, 500, kCorpseMaxYd + 10.0f);
    CHECK(SecondsToRevive(f) <= 3);
    // Raised in time: no revive, the pull follows.
    {
        DungeonPlan plan;
        plan.steps = {PullStep(40, {7011})};
        Run q = Start(&plan);
        f.units = {Unit(21, 7011, 30)};
        f.rezMembers = RezRoster(true, 500);
        BeginRegroup(q);
        for (int i = 0; i < 20; ++i)
            CHECK(Tick(q, f).empty());
        f.dead = 0;
        f.alive = 5;
        f.rezMembers = RezRoster(false, 500);
        std::string order;
        for (int i = 0; i < 6 && order.empty(); ++i)
            order = Tick(q, f);
        CHECK(order == "pullback 21");
    }
    // A revive that went out is not taken back by a resurrector turning up after it (the priest
    // revived at the spirit healer is one): the regroup goes on reviving, it does not hold again.
    {
        DungeonPlan plan;
        plan.steps = {PullStep(40, {7011})};
        Run r = Start(&plan);
        BeginRegroup(r);
        f.units.clear();
        f.dead = 2;
        f.alive = 3;
        f.rezMembers = RezRoster(true, 500);
        f.rezMembers[1].rez = RezClass::None;
        f.rezMembers[3].dead = true;
        CHECK(Tick(r, f) == "revive party");
        f.rezMembers[1].rez = RezClass::Priest;
        Tick(r, f);
        CHECK(r.reviveSent && r.note.find("reviving") != std::string::npos);
        CHECK(r.note.find("a resurrection in the party first") == std::string::npos);
        f.dead = 1;
        f.alive = 4;
    }
    // Once the revive went out it is not held back again: the old resend every 60 s still runs.
    {
        DungeonPlan plan;
        plan.steps = {PullStep(40, {7011})};
        Run r = Start(&plan);
        BeginRegroup(r);
        f.units.clear();
        f.dead = 1;
        f.alive = 4;
        f.rezMembers = RezRoster(true, 500);
        int revives = 0;
        for (std::uint32_t ms = 0; ms < 200000; ms += kTickMs)
            if (Tick(r, f) == "revive party")
                ++revives;
        CHECK(revives == 3); // at ~60 s, then every 60 s: ~120 s, ~180 s
    }
}

// The rez hold's nudge: a hold that went kRezNudgeMs with a rezzer able and no cast under way sends the lead
// bot onto the body, once; the note gives the rezzer's distance to the body and its sight of it.
// The roster: the priest (1) stands `rezzerYd` from the body of the dead mage (2), which lies `bodyYd` from
// the lead bot (at x 0) on the x axis.
static std::vector<TortoiseBots::RezHold::Member> NudgeRoster(float bodyYd, float rezzerYd, bool sees)
{
    std::vector<TortoiseBots::RezHold::Member> m = RezRoster(true, 500);
    m[1].placed = true;
    m[1].x = bodyYd + rezzerYd;
    m[1].seesBody = sees;
    m[2].placed = true;
    m[2].bodyX = bodyYd;
    m[2].bodyY = 0.0f;
    m[2].bodyZ = 0.0f;
    return m;
}

// Ticks a regroup for `ms`; every non-empty order, in order.
static std::vector<std::string> NudgeOrders(Run& r, const Facts& f, std::uint32_t ms)
{
    std::vector<std::string> orders;
    for (std::uint32_t t = kTickMs; t <= ms; t += kTickMs)
    {
        std::string const o = Tick(r, f);
        if (!o.empty())
            orders.push_back(o);
    }
    return orders;
}

static void TestRezHoldNudge()
{
    using namespace TortoiseBots::RezHold;
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Facts f = Party(0);
    f.dead = 1;
    f.alive = 4;
    f.rezHold = true;
    f.leadAlive = true;
    std::string const move = MoveOrder(20.0f, 0.0f, 0.0f, false);
    // A body 20 yd away: nothing before kRezNudgeMs, then one move onto it, and no second one.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        CHECK(NudgeOrders(r, f, kRezNudgeMs - 2 * kTickMs).empty());
        CHECK(!r.rezNudged);
        std::vector<std::string> const orders = NudgeOrders(r, f, 3 * kTickMs);
        CHECK(orders.size() == 1 && orders[0] == move);
        CHECK(r.rezNudged);
        CHECK(r.note.find("waiting for a resurrection") != std::string::npos);
        CHECK(NudgeOrders(r, f, 30000).empty()); // none a second time
        // The next regroup may nudge again.
        BeginRegroup(r);
        CHECK(!r.rezNudged && r.rezHoldMs == 0);
    }
    // The note names the rezzer's distance to the body and its sight of it.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        f.rezMembers[1].name = "Lyrieluna";
        f.rezMembers[2].name = "Beardkinz";
        Tick(r, f);
        CHECK(r.note.find("Lyrieluna 18.0 yd from the body, sight no") != std::string::npos);
        f.rezMembers[1].seesBody = true;
        Tick(r, f);
        CHECK(r.note.find("Lyrieluna 18.0 yd from the body, sight yes") != std::string::npos);
    }
    // A body 5 yd from the lead bot: no move at all.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = NudgeRoster(5.0f, 3.0f, true);
        CHECK(NudgeOrders(r, f, 40000).empty());
        CHECK(!r.rezNudged);
    }
    // A resurrection under way (a cast, or a request not yet accepted): no move.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        f.rezInProgress = true;
        CHECK(NudgeOrders(r, f, 40000).empty());
        CHECK(!r.rezNudged);
        // The cast ends without a rez: the 12 s run again from there.
        f.rezInProgress = false;
        CHECK(NudgeOrders(r, f, kRezNudgeMs - 2 * kTickMs).empty());
        CHECK(NudgeOrders(r, f, 3 * kTickMs).size() == 1);
    }
    // The resurrector drinks (short of mana): no move, and the clock starts over when it is back.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        f.rezMembers[1].mana = 20;
        CHECK(NudgeOrders(r, f, 30000).empty());
        f.rezMembers[1].mana = 500;
        CHECK(NudgeOrders(r, f, kRezNudgeMs - 2 * kTickMs).empty());
        CHECK(NudgeOrders(r, f, 3 * kTickMs).size() == 1);
    }
    // Able 8 s, drinking, able again: the 12 s start over from the drink (the same restart as the hold budget's).
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        CHECK(NudgeOrders(r, f, 8000).empty());
        f.rezMembers[1].mana = 20;
        CHECK(NudgeOrders(r, f, 4 * kTickMs).empty());
        f.rezMembers[1].mana = 500;
        CHECK(NudgeOrders(r, f, kRezNudgeMs - 2 * kTickMs).empty());
        CHECK(NudgeOrders(r, f, 3 * kTickMs).size() == 1);
    }
    // A revived straggler whose summon falls due on the very tick the 12 s are reached: the summon goes out
    // (the nudge never replaces an order SummonRevived set), and the nudge follows on a later tick, once.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        r.waitFor.insert("Quelinator"); // died earlier in the run, revived far away
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        f.members = {Mem("Tanky", 0), Mem("Quelinator", 270)};
        f.members[1].inCombat = true; // a summon is not sent into a fight: not due yet
        CHECK(NudgeOrders(r, f, kRezNudgeMs - kTickMs).empty());
        CHECK(r.rezHoldMs == kRezNudgeMs - kTickMs && !r.rezNudged);
        f.members[1].inCombat = false; // due now: this tick reaches 12 s
        CHECK(Tick(r, f) == "summon Quelinator");
        CHECK(r.rezHoldMs >= kRezNudgeMs && !r.rezNudged);
        CHECK(Tick(r, f) == move); // the next tick: the nudge, once
        CHECK(r.rezNudged);
        CHECK(NudgeOrders(r, f, 5000).empty());
        f.members.clear();
    }
    // A body the glue did not place, or a dead lead bot: no move.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        f.rezMembers[2].placed = false;
        CHECK(NudgeOrders(r, f, 40000).empty());
        Run q = Start(&plan);
        BeginRegroup(q);
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        f.leadAlive = false;
        CHECK(NudgeOrders(q, f, 40000).empty());
        f.leadAlive = true;
    }
    // The key off: the same facts send no move, ever, and the old wait is unchanged.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezHold = false;
        f.rezzer = true;
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        for (std::string const& o : NudgeOrders(r, f, 60000))
            CHECK(o == "revive party");
        CHECK(!r.rezNudged && r.rezHoldMs == 0);
        f.rezHold = true;
        f.rezzer = false;
    }
    // The revive going out ends the nudge for good.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
        std::vector<std::string> const orders = NudgeOrders(r, f, 70000);
        CHECK(orders.size() == 2 && orders[0] == move && orders[1] == "revive party");
        CHECK(r.rezHoldMs == 0 && !r.rezNudged);
    }
}

// T4 (F3): the rez nudge's move refused as unreachable (the Uldaman scorpid pit: the ledge the body lies on is
// not on the navmesh) ends the hold at once and goes to the revive; any other refusal keeps the hold.
static void TestRezNudgeUnreachable()
{
    using namespace TortoiseBots::RezHold;
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Facts f = Party(0);
    f.dead = 1;
    f.alive = 4;
    f.rezHold = true;
    f.leadAlive = true;
    f.rezMembers = NudgeRoster(20.0f, 18.0f, false);
    std::string const move = MoveOrder(20.0f, 0.0f, 0.0f, false);
    std::string const noPath = "unreachable: The lead bot has no walkable path to that point.";
    // Reaches the nudge and checks the move order went out.
    auto nudged = [&](Run& r)
    {
        BeginRegroup(r);
        // Ticks up to the nudge itself (the 15 s party-rezzer wait has not run out then).
        std::string o;
        for (std::uint32_t t = 0; t < 2 * kRezNudgeMs && o.empty(); t += kTickMs)
            o = Tick(r, f);
        CHECK(o == move);
        CHECK(r.rezNudged && r.rezNudgeOrder == move && !r.rezBodyUnreachable);
    };
    // No walkable path: not paused, the very next tick revives.
    {
        Run r = Start(&plan);
        nudged(r);
        OrderRefused(r, move, noPath);
        CHECK(r.state != State::Paused);
        CHECK(r.rezBodyUnreachable && r.rezNudgeOrder.empty());
        CHECK(r.note.find("cannot walk to the body") != std::string::npos);
        CHECK(r.note.find("reviving instead") != std::string::npos);
        CHECK(Tick(r, f) == "revive party");
        CHECK(r.state != State::Paused);
        CHECK(r.note.find("reviving") != std::string::npos);
        CHECK(!r.rezBodyUnreachable); // the revive going out resets it
    }
    // Every path refusal code counts, and a living party rezzer does not bring the 15 s wait back.
    for (char const* code : {"off-mesh: x", "no-ground: x", "no-los: x"})
    {
        Run r = Start(&plan);
        nudged(r);
        f.rezzer = true;
        OrderRefused(r, move, code);
        CHECK(r.state != State::Paused && r.rezBodyUnreachable);
        CHECK(Tick(r, f) == "revive party");
        f.rezzer = false;
    }
    // A refusal for the lead bot's state of the moment: not paused, no revive, the hold kept, no second nudge.
    {
        Run r = Start(&plan);
        nudged(r);
        OrderRefused(r, move, "busy: The lead bot is casting.");
        CHECK(r.state != State::Paused);
        CHECK(!r.rezBodyUnreachable && r.rezNudgeOrder.empty());
        CHECK(r.note.find("refused") != std::string::npos);
        for (std::string const& o : NudgeOrders(r, f, 20000))
            CHECK(o != "revive party" && o != move);
        CHECK(r.state != State::Paused);
    }
    // A refusal of some other order is not taken for the nudge's.
    {
        Run r = Start(&plan);
        nudged(r);
        OrderRefused(r, MoveOrder(99.0f, 0.0f, 0.0f, false), noPath);
        CHECK(!r.rezBodyUnreachable);
    }
    // The next regroup starts clean.
    {
        Run r = Start(&plan);
        nudged(r);
        OrderRefused(r, move, noPath);
        BeginRegroup(r);
        CHECK(!r.rezBodyUnreachable && r.rezNudgeOrder.empty() && !r.rezNudged);
    }
}

// The rez hold's clocks (review of b228d8e8). A priest short of mana for 70 s and then back up keeps
// the whole hold budget from there (before, one shared clock released it the same tick); a cast under
// way finishes past the budget; the 240 s corpse-run budget starts when the hold gave up; the status
// note names who is waited for.
static void TestRezHoldClocks()
{
    using namespace TortoiseBots::RezHold;
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Facts f = Party(0);
    f.dead = 1;
    f.alive = 4;
    f.rezHold = true;
    // Mana back at 70 s: held until 70 s + kHoldMs, then the revive.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        std::uint32_t revivedAt = 0;
        for (std::uint32_t ms = kTickMs; ms <= 300000 && !revivedAt; ms += kTickMs)
        {
            f.rezMembers = RezRoster(true, ms < 70000 ? 20 : 500);
            if (Tick(r, f) == "revive party")
                revivedAt = ms;
        }
        CHECK(revivedAt >= 70000 + kHoldMs && revivedAt <= 70000 + kHoldMs + 3 * kTickMs);
    }
    // A cast under way when the budget is spent: kCastGraceMs more, once.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = RezRoster(true, 500);
        std::uint32_t revivedAt = 0;
        for (std::uint32_t ms = kTickMs; ms <= 200000 && !revivedAt; ms += kTickMs)
        {
            f.rezInProgress = ms >= 50000;
            if (Tick(r, f) == "revive party")
                revivedAt = ms;
        }
        CHECK(revivedAt >= kHoldMs + kCastGraceMs && revivedAt <= kHoldMs + kCastGraceMs + 3 * kTickMs);
        f.rezInProgress = false;
    }
    // The corpse run keeps its 240 s: the pause comes kRegroupReviveMs after the revive, not after the regroup began.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = RezRoster(true, 500);
        std::uint32_t revivedAt = 0, pausedAt = 0;
        for (std::uint32_t ms = kTickMs; ms <= 400000 && !pausedAt; ms += kTickMs)
        {
            if (Tick(r, f) == "revive party" && !revivedAt)
                revivedAt = ms;
            if (r.state == State::Paused)
                pausedAt = ms;
        }
        CHECK(revivedAt >= kHoldMs && pausedAt >= revivedAt + kRegroupReviveMs &&
              pausedAt <= revivedAt + kRegroupReviveMs + 3 * kTickMs);
        // With the key off the pause stays at kRegroupReviveMs from the regroup's start.
        Run o = Start(&plan);
        BeginRegroup(o);
        Facts off = f;
        off.rezHold = false;
        off.rezMembers.clear();
        off.rezzer = true; // the old 15 s wait first: the budget still runs from the regroup's start
        std::uint32_t offPaused = 0;
        for (std::uint32_t ms = kTickMs; ms <= 400000 && !offPaused; ms += kTickMs)
        {
            Tick(o, off);
            if (o.state == State::Paused)
                offPaused = ms;
        }
        CHECK(offPaused >= kRegroupReviveMs && offPaused <= kRegroupReviveMs + 3 * kTickMs);
    }
    // A rezzer that drinks again after it acted gets the budget again (the clock restarts while it drinks).
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        std::uint32_t revivedAt = 0;
        for (std::uint32_t ms = kTickMs; ms <= 300000 && !revivedAt; ms += kTickMs)
        {
            // able 0-40 s, drinking 40-60 s, able again from 60 s
            f.rezMembers = RezRoster(true, ms >= 40000 && ms < 60000 ? 20 : 500);
            if (Tick(r, f) == "revive party")
                revivedAt = ms;
        }
        CHECK(revivedAt >= 60000 + kHoldMs && revivedAt <= 60000 + kHoldMs + 3 * kTickMs);
    }
    // A new regroup starts the budget over.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = RezRoster(true, 500);
        for (int i = 0; i < 40; ++i)
            CHECK(Tick(r, f).empty());
        BeginRegroup(r);
        std::uint32_t revivedAt = 0;
        for (std::uint32_t ms = kTickMs; ms <= 200000 && !revivedAt; ms += kTickMs)
            if (Tick(r, f) == "revive party")
                revivedAt = ms;
        CHECK(revivedAt >= kHoldMs && revivedAt <= kHoldMs + 3 * kTickMs);
    }
    // The gather's restart of the regroup clock takes the hold's share of the revive budget with it: the
    // dead gone and one dead again while the gather waits, the pause is kRegroupReviveMs from the gather.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = RezRoster(true, 500);
        for (int i = 0; i < 70; ++i)
            Tick(r, f); // the hold gives up at ~60 s, the revive is out
        CHECK(r.reviveSent && r.reviveBaseMs > 0);
        Facts g = f;
        g.dead = 0;
        g.alive = 5;
        g.rezMembers = RezRoster(false, 500);
        g.maxMemberYd = 50.0f;
        g.members = {Mem("Far", 50.0f)};
        Tick(r, g); // the gather goes out and restarts the clock
        CHECK(r.regroupGatherSent && r.reviveBaseMs == 0);
        std::uint32_t pausedAt = 0;
        for (std::uint32_t ms = kTickMs; ms <= 400000 && !pausedAt; ms += kTickMs)
        {
            Tick(r, f); // dead again
            if (r.state == State::Paused)
                pausedAt = ms;
        }
        CHECK(pausedAt >= kRegroupReviveMs && pausedAt <= kRegroupReviveMs + 3 * kTickMs);
    }
    // The note names who is waited for and whom.
    {
        Run r = Start(&plan);
        BeginRegroup(r);
        f.rezMembers = RezRoster(true, 500);
        f.rezMembers[1].name = "Holysis";
        f.rezMembers[2].name = "Magnus";
        Tick(r, f);
        Tick(r, f);
        CHECK(r.note.find("Holysis for Magnus") != std::string::npos);
    }
}

// The regroup's revive is bounded: still dead after kRegroupReviveMs pauses.
static void TestRegroupRevivePauses()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {7011})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.dead = 2;
    f.alive = 3;
    BeginRegroup(r);
    int revives = 0;
    for (std::uint32_t ms = 0; ms <= kRegroupReviveMs + kTickMs && r.state == State::Running; ms += kTickMs)
        if (Tick(r, f) == "revive party")
            ++revives;
    CHECK(r.state == State::Paused && r.note.find("2 still dead after 240 s") != std::string::npos);
    CHECK(revives >= 4); // the first, then again every 60 s
}

// Review of #1055 (owner decision): a bot eating or drinking after the fight
// is resting. Uldaman pulls 17 and 61 (19z11): the tank sat eating at 82 %
// (above the 80 % start line) and the pullback found it seated.
static void TestMealWaitBeforePull()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4852})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(31, 4852, 54.7f)};
    f.minHealthPct = 82;
    f.eating = 1;
    CHECK(Tick(r, f).empty());
    CHECK(r.phase == Phase::Rest && r.note.find("1 eating") != std::string::npos);
    for (int i = 0; i < 10; ++i)
        CHECK(Tick(r, f).find("pullback") == std::string::npos);
    // The meal ends (the tank stood up full): the pull goes out.
    f.eating = 0;
    f.minHealthPct = 100;
    std::string order;
    for (int i = 0; i < 5 && order.empty(); ++i)
        order = Tick(r, f);
    CHECK(order == "pullback 31");
    // Eating on and on: kRestCapMs at most, then the pull; once per fight.
    Run c = Start(&plan);
    f.eating = 1;
    f.minHealthPct = 82;
    order.clear();
    std::uint32_t ms = 0;
    for (; ms <= kRestCapMs + 5 * kTickMs && order.find("pullback") == std::string::npos; ms += kTickMs)
        order = Tick(c, f);
    CHECK(order == "pullback 31" && ms >= kRestCapMs);
    // Eating starts a tick after the fight ends (bots sit on their next AI
    // tick), as at pull 17: "pack cleared" on one tick, the next step's pull on
    // the next. The wait still holds that pull.
    DungeonPlan two;
    two.steps = {PullStep(40, {4851}), PullStep(40, {4852})};
    Run late = Start(&two);
    f.eating = 0;
    f.minHealthPct = 100;
    f.units = {Unit(30, 4851, 20)};
    CHECK(Tick(late, f) == "pullback 30");
    f.anyInCombat = true;
    Tick(late, f);
    f.anyInCombat = false;
    f.units.clear();
    f.minHealthPct = 82;
    std::string first;
    for (int i = 0; i < 3 && late.step == 0; ++i)
        first = Tick(late, f);
    CHECK(late.step == 1 && first.find("pullback") == std::string::npos);
    f.units = {Unit(31, 4852, 54.7f)};
    f.eating = 1;
    CHECK(Tick(late, f).empty());
    CHECK(late.phase == Phase::Rest && late.restForEating);
    // The same step: the fight ends with the party scattered (a gather goes
    // out, nobody eats yet), and the tank sits down on the next tick.
    Run same = Start(&plan);
    f.eating = 0;
    f.minHealthPct = 100;
    f.units = {Unit(30, 4852, 20), Unit(31, 4852, 54.7f)};
    CHECK(Tick(same, f) == "pullback 30");
    f.anyInCombat = true;
    Tick(same, f);
    f.anyInCombat = false;
    f.units = {Unit(31, 4852, 54.7f)};
    f.minHealthPct = 82;
    f.maxMemberYd = 35;
    CHECK(Tick(same, f) == "move 0.0 0.0 0.0");
    f.maxMemberYd = 5;
    f.eating = 1;
    CHECK(Tick(same, f).empty());
    CHECK(same.phase == Phase::Rest && same.restForEating);
    // One budget per fight: after kRestCapMs of meal wait the pull goes out
    // even if the bot sits down again.
    order.clear();
    for (std::uint32_t t = 0; t <= kRestCapMs + 5 * kTickMs && order.find("pullback") == std::string::npos; t += kTickMs)
        order = Tick(late, f);
    CHECK(order == "pullback 31" && late.eatWaitMs >= kRestCapMs);
    // At the end lines already: no wait.
    Run e = Start(&plan);
    f.minHealthPct = 95;
    CHECK(Tick(e, f) == "pullback 31");
    // Not inside an event.
    Run v = Start(&plan);
    v.eventOn = true;
    f.minHealthPct = 82;
    Tick(v, f);
    CHECK(v.phase != Phase::Rest);
}

static void TestWipeAndLeadDeathPause()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40)};
    Run r = Start(&plan);
    r.autoRegroups = kAutoRegroupsPerStep;
    Facts f = Party(0);
    f.dead = 3;
    f.alive = 2;
    CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Paused && r.note.find("3 dead") == 0);
    // Resume after the regroup: the step starts over.
    f.dead = 0;
    f.alive = 5;
    CHECK(Resume(r));
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    // The lead bot dead (the owner's wait rule): the party waits for it, a
    // regroup the run counts; past kAutoRegroupsPerStep it pauses.
    f.leadAlive = false;
    f.dead = 1;
    f.alive = 4;
    Tick(r, f);
    CHECK(r.state == State::Running && r.regroup && r.walkBackDue);
    CHECK(r.note.find("waiting for 1 dead: the lead bot is dead") == 0);
    r.regroup = false; // as if the regroup ended and the lead bot fell again
    r.autoRegroups = kAutoRegroupsPerStep;
    Tick(r, f);
    CHECK(r.state == State::Paused && r.note.find("lead bot is dead") != std::string::npos);
}

static void TestFightCeiling()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.anyInCombat = true;
    for (std::uint32_t i = 0; i <= kFightCapMs / kTickMs + 1 && r.state == State::Running; ++i)
        Tick(r, f);
    CHECK(r.state == State::Paused && r.note.find("fight lasted") != std::string::npos);
}

static void TestTaskSteps()
{
    DungeonPlan plan;
    PlanStep loot = Step(PlanStepKind::Loot, 1, 2, 3);
    loot.entry = 123329;
    loot.item = 7740;
    PlanStep use = Step(PlanStepKind::UseItem);
    use.entry = 7740;
    use.item = 7733;
    PlanStep open = Step(PlanStepKind::Open, 4, 5, 6);
    open.entry = 124371;
    open.item = 7733;
    PlanStep wait = Step(PlanStepKind::WaitObject, 7, 8, 9);
    wait.entry = 124372;
    wait.param = 1;
    wait.timeoutS = 60;
    plan.steps = {loot, use, open, wait};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.minHealthPct = 100;
    CHECK(Tick(r, f) == "loot entry:123329@1.0,2.0,3.0 Tanky 7740");
    f.carrierBusy = true;
    CHECK(Tick(r, f).empty());
    // Task over without the item: pause, naming it.
    f.carrierBusy = false;
    Tick(r, f);
    CHECK(r.state == State::Paused && r.note.find("7740") != std::string::npos);
    // With the item it goes on.
    CHECK(Resume(r));
    f.carrierHasItem = true;
    Tick(r, f);
    CHECK(r.step == 1);
    // use_item: the staff is not there yet; the medallion is.
    f.carrierHasItem = false;
    f.carrierHasEntryItem = true;
    CHECK(Tick(r, f) == "use 7740 Tanky");
    f.carrierHasItem = true;
    Tick(r, f);
    CHECK(r.step == 2);
    CHECK(Tick(r, f) == "open entry:124371@4.0,5.0,6.0 Tanky");
    Tick(r, f);
    CHECK(r.step == 3);
    // The seal opens after a while; then the plan is done.
    f.objectState = 0;
    for (int i = 0; i < 20; ++i)
        Tick(r, f);
    CHECK(r.step == 3 && r.state == State::Running);
    f.objectState = 1;
    Tick(r, f);
    CHECK(r.state == State::Done);
}

static void TestOpenWithoutKeyAndTaskError()
{
    DungeonPlan plan;
    PlanStep open = Step(PlanStepKind::Open, 4, 5, 6);
    open.entry = 124371;
    open.item = 7733;
    plan.steps = {open};
    Run r = Start(&plan);
    Facts f = Party(0);
    Tick(r, f);
    CHECK(r.state == State::Paused && r.note.find("lock needs it") != std::string::npos);
    CHECK(Resume(r));
    f.carrierHasItem = true;
    CHECK(!Tick(r, f).empty());
    f.carrierBusy = true;
    f.taskReply = 2;
    f.taskText = "locked: needs key";
    Tick(r, f);
    CHECK(r.state == State::Paused && r.note.find("locked") != std::string::npos);
}

// ---- task resend (F1): an interrupted cast or an in-combat answer sends the same order again ----

static const char* const kInterruptedText =
    "failed: The opening cast was interrupted after 2.5 s (it did not move and is not in combat).";

// One AwaitTask round: the order is out (carrier busy), then the ERR answer is read.
static std::string AnswerErr(Run& r, Facts& f, const std::string& text)
{
    f.carrierBusy = true;
    f.taskReply = 2;
    f.taskText = text;
    std::string order = Tick(r, f);
    f.carrierBusy = false; // the module's task ended with the answer
    f.taskReply = 0;
    f.taskText.clear();
    return order;
}

static void TestTaskResendAfterInterruptedCast()
{
    for (PlanStepKind kind : {PlanStepKind::Loot, PlanStepKind::Open})
    {
        DungeonPlan plan;
        plan.steps = {GraveStep(kind, 0), Step(PlanStepKind::Move, 9)};
        Run r = Start(&plan);
        Facts f = Party(0);
        std::string const first = Tick(r, f);
        CHECK(first.compare(0, 5, kind == PlanStepKind::Loot ? "loot " : "open ") == 0);
        CHECK(r.orderSent && r.phase == Phase::AwaitTask);

        // 1st failure: running, back at Begin, the note says so, the next tick sends the same order.
        AnswerErr(r, f, kInterruptedText);
        CHECK(r.state == State::Running && r.step == 0 && r.phase == Phase::Begin && !r.orderSent);
        CHECK(r.note.find("sending it again (1 of 2)") != std::string::npos);
        CHECK(r.note.find("The opening cast was interrupted after 2.5 s (it did not move and is not in combat): sending") != std::string::npos);
        CHECK(Tick(r, f) == first);
        CHECK(r.orderSent && r.phase == Phase::AwaitTask);

        // 2nd failure: again.
        AnswerErr(r, f, kInterruptedText);
        CHECK(r.state == State::Running && r.step == 0 && !r.orderSent);
        CHECK(r.note.find("sending it again (2 of 2)") != std::string::npos);
        CHECK(Tick(r, f) == first);

        // 3rd failure: the old pause.
        AnswerErr(r, f, kInterruptedText);
        CHECK(r.state == State::Paused && r.step == 0);
        CHECK(r.note.find("failed: " + std::string(kInterruptedText)) != std::string::npos);
    }
}

static void TestTaskResendInCombatAndBusy()
{
    for (const char* text : {"in-combat: The bot is in combat and stopped.", "busy: The chest is in use."})
    {
        DungeonPlan plan;
        plan.steps = {GraveStep(PlanStepKind::Loot, 0), Step(PlanStepKind::Move, 9)};
        Run r = Start(&plan);
        Facts f = Party(0);
        std::string const first = Tick(r, f);
        AnswerErr(r, f, text);
        CHECK(r.state == State::Running && r.phase == Phase::Begin && !r.orderSent);
        CHECK(r.note.find("sending it again (1 of 2)") != std::string::npos);
        CHECK(Tick(r, f) == first);
    }
}

// The counter is per step: the second step gets its own two re-sends; a resume resets it too.
static void TestTaskResendCounterResetsPerStep()
{
    DungeonPlan plan;
    plan.steps = {GraveStep(PlanStepKind::Open, 0), GraveStep(PlanStepKind::Open, 0)};
    Run r = Start(&plan);
    Facts f = Party(0);
    Tick(r, f);
    AnswerErr(r, f, kInterruptedText);
    Tick(r, f);
    AnswerErr(r, f, kInterruptedText);
    Tick(r, f);
    Tick(r, f); // the 3rd try ends without an error: step 2
    CHECK(r.step == 1);
    CHECK(r.taskResends == 0);
    for (int i = 0; i < 3 && !r.orderSent; ++i)
        Tick(r, f);
    CHECK(r.orderSent);
    AnswerErr(r, f, kInterruptedText);
    CHECK(r.state == State::Running && r.note.find("sending it again (1 of 2)") != std::string::npos);
}

// A loot step sends to the same bot again: taskBot and the tried list are left as they were.
static void TestTaskResendKeepsTheLootBot()
{
    DungeonPlan plan;
    plan.steps = {GraveStep(PlanStepKind::Loot, 0), Step(PlanStepKind::Move, 9)};
    Run r = Start(&plan);
    Facts f = Party(0);
    Tick(r, f);
    std::string const bot = r.taskBot;
    std::size_t const tried = r.lootTried.size();
    AnswerErr(r, f, kInterruptedText);
    CHECK(r.taskBot == bot);
    CHECK(r.lootTried.size() == tried);
    CHECK(r.lootRefusal.empty());
}

// Other failures still pause at once.
static void TestTaskResendNotForOtherFailures()
{
    for (const char* text : {"locked: needs key", "failed: The bot was interrupted by a spell.", "too-far: x"})
    {
        DungeonPlan plan;
        plan.steps = {GraveStep(PlanStepKind::Open, 0), Step(PlanStepKind::Move, 9)};
        Run r = Start(&plan);
        Facts f = Party(0);
        Tick(r, f);
        AnswerErr(r, f, text);
        CHECK(r.state == State::Paused && r.step == 0);
        CHECK(r.note.find("sending it again") == std::string::npos);
    }
}

// ---- gossip step (talk to an NPC: `cmd gossip <entry> <lead bot> <option>`) ----

// Weegli Blastfuse (7607): option 1, the top of his menu, at x.
static PlanStep GossipRow(float x, std::uint32_t option = 1, std::uint32_t entry = 7607)
{
    PlanStep s;
    s.kind = PlanStepKind::Gossip;
    s.entry = entry;
    s.x = x;
    s.param = option;
    CHECK(ai::NormalizePlanStep(s) == ai::PlanStepRefusal::None);
    return s;
}

// The one order goes out once, the busy task is waited for, the ACK ends the step.
static void TestGossipOrderOnceThenAck()
{
    DungeonPlan plan;
    plan.steps = {GossipRow(10), Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f) == "gossip 7607 Tanky 1");
    CHECK(r.orderSent && r.phase == Phase::AwaitTask);
    // The lead bot's task is under way: no second order, the step stays.
    f.carrierBusy = true;
    for (int i = 0; i < 5; ++i)
        CHECK(Tick(r, f).empty() && r.step == 0 && r.state == State::Running);
    // The ACK.
    f.taskReply = 1;
    f.taskText = "1|Will you blow up that door now?";
    Tick(r, f);
    CHECK(r.step == 1 && r.state == State::Running && r.note == "gossip 7607 option 1 done");
}

// An ERR answer pauses with the module's own words; .bot run resume sends the order again.
static void TestGossipErrPauses()
{
    DungeonPlan plan;
    plan.steps = {GossipRow(10, 3)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f) == "gossip 7607 Tanky 3");
    f.taskReply = 2;
    f.taskText = "no-option: No option 3. The menu offers: 1 \"Will you blow up that door now?\".";
    Tick(r, f);
    CHECK(r.state == State::Paused && r.step == 0);
    CHECK(r.note.find("gossip 7607 option 3 failed") != std::string::npos);
    CHECK(r.note.find("no-option: No option 3.") != std::string::npos);
    CHECK(Resume(r));
    f.taskReply = 0;
    f.taskText.clear();
    for (int i = 0; i < 400 && !r.orderSent; ++i) // the resume's regroup first
    {
        std::string order = Tick(r, f);
        if (order.compare(0, 7, "gossip ") == 0)
            break;
    }
    CHECK(r.orderSent);
}

// The order's immediate ERR: an NPC that AiPlayerbot.BotGossipNpcs does not list pauses naming the key.
static void TestGossipNotAllowedNamesTheConfigKey()
{
    DungeonPlan plan;
    plan.steps = {GossipRow(10)};
    Run r = Start(&plan);
    Facts f = Party(0);
    std::string const order = Tick(r, f);
    CHECK(order == "gossip 7607 Tanky 1");
    OrderRefused(r, order, "not-allowed: That NPC is not in AiPlayerbot.BotGossipNpcs, so bots may not talk to it.");
    CHECK(r.state == State::Paused);
    CHECK(r.note.find("cmd gossip 7607 Tanky 1 refused") != std::string::npos);
    CHECK(r.note.find("AiPlayerbot.BotGossipNpcs") != std::string::npos);
}

// No ACK and no ERR within the step's timeout (default 60 s): the run pauses.
static void TestGossipNoAnswerPauses()
{
    DungeonPlan plan;
    PlanStep row = GossipRow(10);
    CHECK(row.timeoutS == ai::kPlanDefaultTimeoutS);
    plan.steps = {row, Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f) == "gossip 7607 Tanky 1");
    f.carrierBusy = true;
    std::uint32_t ticks = 0;
    while (r.state == State::Running && ticks < 200)
    {
        CHECK(Tick(r, f).empty());
        ++ticks;
    }
    CHECK(r.state == State::Paused && r.step == 0 && ticks >= 59 && ticks <= 61);
    CHECK(r.note.find("no answer in 60 s") != std::string::npos);
    // The step's own timeout counts: 20 s.
    DungeonPlan quick;
    PlanStep fast = GossipRow(10);
    fast.timeoutS = 20;
    quick.steps = {fast};
    Run q = Start(&quick);
    Facts g = Party(0);
    Tick(q, g);
    g.carrierBusy = true;
    ticks = 0;
    while (q.state == State::Running && ticks < 200)
    {
        Tick(q, g);
        ++ticks;
    }
    CHECK(q.state == State::Paused && ticks >= 19 && ticks <= 21 && q.note.find("no answer in 20 s") != std::string::npos);
}

// A lead bot busy with an earlier task holds the order back.
static void TestGossipWaitsForABusyLeadBot()
{
    DungeonPlan plan;
    plan.steps = {GossipRow(10)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.carrierBusy = true;
    CHECK(Tick(r, f).empty() && !r.orderSent);
    CHECK(r.note.find("waiting for Tanky to finish an earlier order") != std::string::npos);
    f.carrierBusy = false;
    CHECK(Tick(r, f) == "gossip 7607 Tanky 1");
}

// Farther than 30 yd from the point the lead bot walks there first (a plain walk in sight, navmesh hops out of
// sight); the order goes out once it is within reach, and a walk once begun is finished.
static void TestGossipWalksToFarPointFirst()
{
    DungeonPlan plan;
    plan.steps = {GossipRow(45)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f) == "move 45.0 0.0 0.0");
    // 20 yd out is within the NPC search, but the walk was begun: it goes on to the point.
    f.leadX = 25;
    CHECK(Tick(r, f).empty() && !r.orderSent);
    f.leadX = 44;
    CHECK(Tick(r, f) == "gossip 7607 Tanky 1");

    // Out of sight of the point: a navmesh hop first.
    DungeonPlan far;
    far.steps = {GossipRow(200)};
    Run q = Start(&far);
    Facts g = Party(0);
    g.seen = false;
    g.approachKnown = true;
    g.approachX = 40;
    CHECK(Tick(q, g) == "move 40.0 0.0 0.0");
    g.leadX = 40;
    CHECK(Tick(q, g).empty());
    g.seen = true;
    CHECK(Tick(q, g) == "move 200.0 0.0 0.0");
    g.leadX = 200;
    CHECK(Tick(q, g) == "gossip 7607 Tanky 1");

    // Within 30 yd there is no walk at all.
    DungeonPlan near;
    near.steps = {GossipRow(29)};
    Run n = Start(&near);
    Facts h = Party(0);
    CHECK(Tick(n, h) == "gossip 7607 Tanky 1");
}

// A gossip right before a hold starts that event (like open / use_item): the run leader treats the walk and the
// step as inside the event, and a failed event goes back to it.
static void TestGossipStartsAnEvent()
{
    DungeonPlan plan;
    plan.map = 209;
    plan.steps = {Step(PlanStepKind::Move, 1, 2, 3), GossipRow(10), Step(PlanStepKind::Move, 1, 2, 3),
                  Step(PlanStepKind::Hold, 1, 2, 3)};
    plan.steps[3].entry = 1;
    plan.steps[3].flags = ai::kPlanFlagDoneData;
    plan.steps[3].entries = {5};
    Run r = Start(&plan, 1);
    CHECK(Current(r)->kind == PlanStepKind::Gossip && EventHoldAhead(r) != nullptr && InEvent(r));
    // A start on the hold or the move leading into it goes back to the gossip.
    std::size_t step = 99;
    bool back = false;
    CHECK(ResolveStartStep(plan, StartFrom::Step, "4", step, &back) == FromResult::Ok && step == 1 && back);
    CHECK(ResolveStartStep(plan, StartFrom::Step, "3", step, &back) == FromResult::Ok && step == 1 && back);
    CHECK(ResolveStartStep(plan, StartFrom::Step, "2", step, &back) == FromResult::Ok && step == 1 && !back);
    CHECK(EventStartStep(plan, 3) == 1);
    // The failed event returns to the gossip step.
    Run h = Start(&plan, 3);
    EventFailed(h, "data 1 did not leave 0");
    CHECK(h.state == State::Paused && h.step == 1);
    CHECK(h.note.find("starts it again at step 2 (gossip)") != std::string::npos);

    // Before a wait_object it is no event start: nothing to return to, no hold ahead.
    DungeonPlan door;
    door.steps = {GossipRow(10), Step(PlanStepKind::WaitObject, 1, 2, 3)};
    door.steps[1].entry = 146084;
    Run d = Start(&door);
    CHECK(EventHoldAhead(d) == nullptr && !InEvent(d));
}

// The ZF shape: hold until the pyramid data reads 8, the gossip, then the door wait.
static void TestGossipAfterAHoldThenTheDoor()
{
    DungeonPlan plan;
    PlanStep hold = Step(PlanStepKind::Hold, 1877.5f, 1199.6f, 8.9f);
    hold.entry = 1;
    hold.param = 8;
    hold.flags = ai::kPlanFlagDoneData;
    hold.entries = {7787};
    hold.timeoutS = 600;
    ai::NormalizePlanStep(hold);
    PlanStep door = Step(PlanStepKind::WaitObject, 1854.5f, 1142.9f, 16.1f);
    door.entry = 146084;
    door.param = 1;
    ai::NormalizePlanStep(door);
    PlanStep talk = GossipRow(1877.5f, 1);
    talk.y = 1199.6f;
    talk.z = 8.9f;
    plan.steps = {hold, talk, door};
    Run r = Start(&plan);
    Facts f = Party(1877.5f);
    f.leadY = 1199.6f;
    f.leadZ = 8.9f;
    f.eventData = 8; // the hold's data already reads 8: it ends at once
    // The hold ends on the first look and the gossip step goes out in the same tick (it is in sight, the lead bot on its spot).
    CHECK(Tick(r, f) == "gossip 7607 Tanky 1");
    CHECK(r.step == 1 && r.orderSent);
    f.carrierBusy = true;
    f.taskReply = 1;
    Tick(r, f);
    CHECK(r.step == 2 && Current(r)->kind == PlanStepKind::WaitObject);
}

static void TestWaitObjectTimeoutIsBounded()
{
    DungeonPlan plan;
    PlanStep wait = Step(PlanStepKind::WaitObject);
    wait.entry = 5;
    wait.timeoutS = 600;
    plan.steps = {wait};
    Run r = Start(&plan);
    Facts f = Party(0);
    std::uint32_t ticks = 0;
    while (r.state == State::Running && ticks < 2000)
    {
        Tick(r, f);
        ++ticks;
    }
    // The wait's own timeout ends it, not the generic step ceiling (300 s).
    CHECK(r.state == State::Paused && ticks >= 600 && ticks <= 602);
    CHECK(r.note.find("not found") != std::string::npos);
}

static void TestUnsupportedPausesAndSkip()
{
    DungeonPlan plan;
    // A kind the run does not know (the loader never lets one through; the
    // machine still pauses on it rather than guess).
    PlanStep unknown;
    unknown.kind = PlanStepKind::None;
    plan.steps = {unknown, Step(PlanStepKind::Move, 10)};
    Run r = Start(&plan);
    Facts f = Party(0);
    Tick(r, f);
    CHECK(r.state == State::Paused && r.note.find("not supported yet") != std::string::npos);
    CHECK(!Supported(PlanStepKind::None) && Supported(PlanStepKind::Move));
    CHECK(Supported(PlanStepKind::Ritual) && Supported(PlanStepKind::Hold) && Supported(PlanStepKind::Gossip));
    // Skip from a pause goes on running (after the regroup).
    CHECK(Skip(r));
    CHECK(r.state == State::Running && r.step == 1 && r.regroup);
    CHECK(!Resume(r));
    CHECK(Tick(r, f) == "move 10.0 0.0 0.0");
}

static void TestPlayerOrderAndRefusalPause()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10)};
    Run r = Start(&plan);
    CHECK(PauseForPlayer(r, "cmd attack"));
    CHECK(!PauseForPlayer(r, "cmd attack"));
    CHECK(r.state == State::Paused && r.note.find("cmd attack") != std::string::npos);
    Facts f = Party(0);
    CHECK(Tick(r, f).empty());
    CHECK(Resume(r));
    OrderRefused(r, "rest", "failed: No bot of the group accepted the order.");
    CHECK(r.state == State::Running && r.note.find("rest refused") == 0);
    OrderRefused(r, "move 10.0 0.0 0.0", "no-path: The lead bot has no walkable path to that point.");
    CHECK(r.state == State::Paused);
    // The status line keeps the protocol's field count.
    r.note = "a|b";
    std::string line = StatusLine(r);
    int bars = 0;
    for (char c : line)
        bars += c == '|';
    CHECK(bars == 7);
}

// Issue #999: a gather refused (a live ZF run: "cmd move 1420.7 801.1 11.0
// refused unreachable" with the tank on that very spot) is sent once more,
// then the pull goes on without it instead of pausing the run.
static void TestRefusedGatherIsRetriedOnceThenLeftOut()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 30;
    f.units = {Unit(7, 1, 20), Unit(8, 1, 25)};
    std::string const gather = "move 0.0 0.0 0.0";
    CHECK(Tick(r, f) == gather);
    OrderRefused(r, gather, "unreachable: The lead bot has no walkable path to that point.");
    CHECK(r.state == State::Running && r.note.find("once more") != std::string::npos);
    CHECK(Tick(r, f) == gather);
    OrderRefused(r, gather, "unreachable: The lead bot has no walkable path to that point.");
    CHECK(r.state == State::Running && r.note.find("without it") != std::string::npos);
    CHECK(Tick(r, f) == "pullback 7");
    // The next pull gathers again: the waiver held for one pull only.
    f.anyInCombat = true;
    Tick(r, f);
    f.anyInCombat = false;
    f.units = {Unit(8, 1, 25)};
    CHECK(Tick(r, f) == gather);
    CHECK(r.state == State::Running);

    // A direct step: refused twice, the direct walk goes out ungathered.
    DungeonPlan direct;
    direct.steps = {Step(PlanStepKind::MoveDirect, 5)};
    Run d = Start(&direct);
    Facts g = Party(0);
    g.maxMemberYd = 30;
    CHECK(Tick(d, g) == gather);
    OrderRefused(d, gather, "unreachable");
    CHECK(Tick(d, g) == gather);
    OrderRefused(d, gather, "unreachable");
    CHECK(d.state == State::Running && Tick(d, g) == "move 5.0 0.0 0.0 direct");

    // Other refusals still pause, also a gather's text sent on an earlier tick.
    Run o = Start(&plan);
    OrderRefused(o, gather, "unreachable");
    CHECK(o.state == State::Paused);
}

// Issue #999: a wipe regroups by itself once the fight is over (revive,
// gather), then walks the plan's own move points back from the entrance to
// the step; it counts the wipe; the third at one step pauses as before.
static void TestWipeRegroupsAndWalksBack()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10), Step(PlanStepKind::Move, 50), Step(PlanStepKind::Move, 90),
        PullStep(130, {1})};
    Run r = Start(&plan, 3);
    Facts f = Party(130);
    f.dead = 5;
    f.alive = 0;
    f.leadAlive = false;
    CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Running && r.regroup && r.wipes == 1 && r.autoRegroups == 1);
    CHECK(r.note.find("(1 wipe this run)") != std::string::npos);
    CHECK(Tick(r, f) == "revive party");
    for (int i = 0; i < 30; ++i)
        Tick(r, f); // the corpse run: still one wipe
    CHECK(r.wipes == 1 && r.state == State::Running);
    // Alive at the entrance, spread: gather, then the walk back.
    f = Party(0);
    f.maxMemberYd = 40;
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    f.maxMemberYd = 5;
    CHECK(Tick(r, f).empty()); // regrouped; the walk back starts with the facts of its point
    // 19z5: straight to the last point before the step (90), in navmesh hops.
    CHECK(r.walkBack && r.walkBackDirect && r.walkBackIdx == 2 && FactStep(r) == &plan.steps[2] && OwnerFrom(r) == 2);
    f.seen = false; // FactStep's point, 90 yd away
    f.approachKnown = true;
    f.approachX = 40;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    CHECK(r.note.find("walk back: approaching") == 0);
    f.leadX = 40;
    CHECK(Tick(r, f).empty()); // at the hop
    // A pack that came back by the next hop is pulled before the move.
    f.hopUnits = {Unit(9, 4, 30)};
    CHECK(Tick(r, f) == "pullback 9");
    CHECK(r.walkBack && r.note.find("walk back: pulling") == 0);
    f.hopUnits.clear();
    f.anyInCombat = true;
    Tick(r, f);
    f.anyInCombat = false;
    // The hops fail (no path from here): the plan's points are replayed from
    // the nearest one (50, from 40).
    f.approachKnown = false;
    Tick(r, f);
    CHECK(r.walkBack && !r.walkBackDirect && r.walkBackIdx == 1 && r.note.find("replaying") != std::string::npos);
    f.seen = true;
    CHECK(Tick(r, f) == "move 50.0 0.0 0.0");
    // A point the navmesh refuses now is passed for the next one.
    OrderRefused(r, "move 50.0 0.0 0.0", "unreachable");
    CHECK(r.state == State::Running);
    CHECK(Tick(r, f) == "move 90.0 0.0 0.0");
    f.leadX = 90;
    CHECK(Tick(r, f).empty());
    CHECK(Tick(r, f).empty() && !r.walkBack && r.note.find("walked back") == 0);
    CHECK(FactStep(r) == &plan.steps[3] && OwnerFrom(r) == 3);
    f.units = {Unit(7, 1, 40)};
    CHECK(Tick(r, f) == "pullback 7");
    CHECK(r.step == 3);

    // Second wipe at the step: regroups once more; the third pauses.
    f = Party(130);
    f.dead = 5;
    f.alive = 0;
    f.leadAlive = false;
    Tick(r, f);
    CHECK(r.state == State::Running && r.wipes == 2 && r.autoRegroups == 2);
    f = Party(130); // revived where it stands, near the step: no walk back
    f.units = {Unit(7, 1, 40)};
    CHECK(Tick(r, f) == "pullback 7");
    CHECK(!r.regroup && !r.walkBack);
    f.dead = 5;
    f.alive = 0;
    f.leadAlive = false;
    Tick(r, f);
    CHECK(r.state == State::Paused && r.wipes == 3 && r.note.find("did not hold") != std::string::npos);
    CHECK(r.note.find("(cmd revive party)") != std::string::npos);
    // A resume gives the step its regroups back; a new step too.
    CHECK(Resume(r) && r.autoRegroups == 0);
}

// Three dead while the other two still fight: the fight goes on (bounded by
// its ceiling), the regroup starts when it ends; no wipe is counted.
static void TestThreeDeadRegroupAfterTheFight()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.dead = 3;
    f.alive = 2;
    f.leadAlive = false;
    f.anyInCombat = true;
    CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Running && !r.regroup && r.note.find("regroup follows") != std::string::npos);
    f.anyInCombat = false;
    Tick(r, f);
    CHECK(r.state == State::Running && r.regroup && r.wipes == 0 && r.nearWipes == 1);
    CHECK(r.note.find("(1 near wipe this run)") != std::string::npos);
    CHECK(Tick(r, f) == "revive party");
    // No earlier move point: nothing to walk back, the step goes on.
    f = Party(0);
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    CHECK(!r.walkBack);
    // The done note carries the wipes.
    r.wipes = 2;
    f.leadX = 40;
    Tick(r, f);
    CHECK(r.state == State::Done && r.note == "plan done (2 wipes, 1 near wipe)");
}

// Issue #999 (third observe run, step 35): the healer stood 82 yd behind and
// did not come; the gather paused the run again and again. At the gather's
// ceiling the farthest bot over 40 yd that came less than 5 yd closer since
// the last sample is summoned to the lead bot, once; one on its way is not.
static void TestGatherSummonsAStalledStraggler()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 90;
    f.members = {Mem("Walky", 90), Mem("Healy", 82), Mem("Nearby", 15), Mem("Stuck", 50)};
    f.units = {Unit(7, 1, 20)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the gather: first sample
    // By the ceiling Walky came 30 yd closer; Healy and Stuck did not.
    f.members = {Mem("Walky", 60), Mem("Healy", 81), Mem("Nearby", 15), Mem("Stuck", 50)};
    f.maxMemberYd = 81;
    std::string order;
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && order.empty(); ++i)
    {
        f.members[0].x += (i % 2 ? 3.0f : -3.0f); // Walky keeps walking (not standing still)
        order = Tick(r, f);
    }
    CHECK(order == "summon Healy"); // the farthest stalled one
    CHECK(r.state == State::Running && r.phase == Phase::Gather && r.note.find("Healy") != std::string::npos);
    // A refused summon does not pause: the gather's ceiling does.
    OrderRefused(r, order, "lead-unavailable");
    CHECK(r.state == State::Running);
    // Next ceiling: Walky still comes (no summon), Stuck stalled since the last sample: summoned.
    f.members = {Mem("Walky", 50), Mem("Healy", 81), Mem("Nearby", 15), Mem("Stuck", 49)};
    order.clear();
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && order.empty(); ++i)
    {
        f.members[0].x += (i % 2 ? 3.0f : -3.0f); // Walky keeps walking (not standing still)
        order = Tick(r, f);
    }
    CHECK(order == "summon Stuck");
    // Nobody left to summon (Walky still makes progress): the next ceiling pauses.
    f.members = {Mem("Walky", 44), Mem("Healy", 81), Mem("Nearby", 15), Mem("Stuck", 49)};
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && r.state == State::Running; ++i)
    {
        f.members[0].x += (i % 2 ? 3.0f : -3.0f);
        CHECK(Tick(r, f).find("summon") == std::string::npos);
    }
    CHECK(r.state == State::Paused && r.note.find("did not gather") != std::string::npos);

    // The summoned bot arrives: the pull goes out.
    Run g = Start(&plan);
    Facts h = Party(0);
    h.maxMemberYd = 82;
    h.members = {Mem("Healy", 82)};
    h.units = {Unit(7, 1, 20)};
    Tick(g, h);
    std::string got;
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && got.empty(); ++i)
        got = Tick(g, h);
    CHECK(got == "summon Healy");
    h.maxMemberYd = 5;
    h.members.clear();
    CHECK(Tick(g, h) == "pullback 7");
}

// Live (events-5 00:55:59 and again in the regroup 00:57:35, grimlok pull 93):
// a bot stuck 34.9 yd out (a navmesh pocket at the pit) was never summoned --
// the gather (kGatherYd 20) waits for it, the summon's ceiling (kSummonYd 40)
// skips it -- so the run paused "the party did not gather within 30 s (a bot is
// 34.9 yd ...)" until a person summoned it.
static void TestStillBotInsideSummonRangeIsSummoned()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 34.9f;
    f.members = {Mem("Tanky", 0), Mem("Stoutster", 34.9f)};
    f.units = {Unit(7, 1, 50)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the gather
    std::string order;
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && order.empty(); ++i)
        order = Tick(r, f);
    CHECK(order == "summon Stoutster");
    CHECK(r.state == State::Running && r.phase == Phase::Gather);
    CHECK(r.note.find("Stoutster") != std::string::npos && r.note.find("has not moved") != std::string::npos);
}

// ... and a busy bot is not stuck: one in a fight, or with something on it
// (units attacking it), is left alone, exactly as in a regroup
// (SummonStuckInRegroup) and as the revived bot's summon does -- `cmd summon`
// refuses a bot in combat, so the order would only be refused. The gather's
// ceiling then pauses as before.
static void TestStuckBotInCombatIsNotSummoned()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 34.9f;
    f.members = {Mem("Tanky", 0), Mem("Stoutster", 34.9f)};
    f.members[1].inCombat = true;
    f.units = {Unit(7, 1, 50)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the gather
    std::string order;
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && r.state == State::Running; ++i)
    {
        order = Tick(r, f);
        CHECK(order.find("summon") == std::string::npos);
    }
    CHECK(r.state == State::Paused && r.note.find("did not gather") != std::string::npos);
}

static void TestStuckBotWithAttackersIsNotSummoned()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 34.9f;
    f.members = {Mem("Tanky", 0), Mem("Stoutster", 34.9f)};
    f.members[1].attackers = 1; // a mob on it: it runs, it does not stand (RunBack)
    f.units = {Unit(7, 1, 50)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the gather
    std::string order;
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && r.state == State::Running; ++i)
    {
        order = Tick(r, f);
        CHECK(order.find("summon") == std::string::npos);
    }
    CHECK(r.state == State::Paused && r.note.find("did not gather") != std::string::npos);
}

// Live r4-19z21 slot 2 step 102 (13:57:42): the gather paused "the party did not
// gather within 30 s (a bot is 33.7 yd from the lead bot)" while that bot was
// coming at about 1 yd/s (54.9 -> 28.7 yd in the trace). A bot that keeps
// coming is waited for, within kGatherComingMaxMs (T5). A walking bot that
// does not come closer (it wobbles about 34 yd out) is summoned at the first
// ceiling, once; if it is still not here by the next, the run pauses.
static void TestMovingBotThatDoesNotGainIsSummoned()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 34.9f;
    f.members = {Mem("Tanky", 0), Mem("Stoutster", 34.9f)};
    f.units = {Unit(7, 1, 50)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    std::string order;
    std::uint32_t ticks = 0;
    for (; ticks < 2 * kGatherCapMs / kTickMs + 6 && order.empty(); ++ticks)
    {
        float const yd = (ticks % 2) ? 33.7f : 35.0f; // walking, but not coming
        f.members[1] = Mem("Stoutster", yd);
        f.members[1].x += (ticks % 2 ? 3.0f : -3.0f);
        f.maxMemberYd = yd;
        order = Tick(r, f);
        CHECK(r.state == State::Running);
    }
    CHECK(order == "summon Stoutster");
    CHECK(ticks <= kGatherCapMs / kTickMs + 2); // at the first ceiling, not at 90 s
    CHECK(r.phase == Phase::Gather);
    // Summoned and still not here at the next ceiling: pause as before.
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && r.state == State::Running; ++i)
    {
        float const yd = (i % 2) ? 33.7f : 35.0f;
        f.members[1] = Mem("Stoutster", yd);
        f.members[1].x += (i % 2 ? 3.0f : -3.0f);
        f.maxMemberYd = yd;
        CHECK(Tick(r, f).find("summon") == std::string::npos);
    }
    CHECK(r.state == State::Paused && r.note.find("did not gather") != std::string::npos);
}

// T5: the trace's straggler, 54.9 yd out and walking in at 0.6 yd/s, is waited
// for past the 30 s ceiling; once within kGatherYd the pull goes out.
static void TestComingStragglerIsWaitedFor()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 54.9f;
    f.members = {Mem("Tanky", 0), Mem("Walky", 54.9f)};
    f.units = {Unit(7, 1, 50)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    bool sawComing = false;
    std::string order;
    std::uint32_t ticks = 1;
    for (; ticks < 120 && order.empty(); ++ticks)
    {
        float const yd = 54.9f - 0.6f * static_cast<float>(ticks);
        f.members[1] = Mem("Walky", yd);
        f.maxMemberYd = yd;
        order = Tick(r, f);
        CHECK(r.state == State::Running);
        CHECK(order.find("summon") == std::string::npos);
        if (r.note.find("Walky is coming") != std::string::npos)
            sawComing = true;
    }
    CHECK(sawComing);
    CHECK(order == "pullback 7");
    CHECK(ticks > kGatherCapMs / kTickMs); // it went past the 30 s ceiling
}

// ... also at a direct step's gather and at the walk back's direct gather.
static void TestComingStragglerIsWaitedForAtDirectGathers()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::MoveDirect, 5), PullStep(100, {1})};
    for (int walkBack = 0; walkBack < 2; ++walkBack)
    {
        Run r = Start(&plan, walkBack ? 1 : 0);
        if (walkBack)
        {
            r.walkBack = true;
            r.walkBackIdx = r.walkBackStart = 0;
            r.walkBackDirect = true;
        }
        Facts f = Party(0);
        f.maxMemberYd = 54.9f;
        f.members = {Mem("Tanky", 0), Mem("Walky", 54.9f)};
        CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
        CHECK(r.phase == Phase::Gather);
        for (std::uint32_t ticks = 1; ticks < kGatherCapMs / kTickMs + 10; ++ticks)
        {
            float const yd = 54.9f - 0.6f * static_cast<float>(ticks);
            f.members[1] = Mem("Walky", yd);
            f.maxMemberYd = yd;
            std::string const order = Tick(r, f);
            CHECK(r.state == State::Running && r.phase == Phase::Gather);
            CHECK(order.find("summon") == std::string::npos);
        }
        CHECK(r.note.find("Walky is coming") != std::string::npos);
        // The same bot standing at 34 yd: no gain, summoned at the next ceiling.
        for (std::uint32_t i = 0; i < 2 * kGatherCapMs / kTickMs; ++i)
        {
            f.members[1] = Mem("Walky", 34.0f);
            f.members[1].x += (i % 2 ? 3.0f : -3.0f);
            f.maxMemberYd = 34.0f;
            std::string const order = Tick(r, f);
            if (order == "summon Walky")
                break;
            CHECK(r.state == State::Running);
        }
        CHECK(r.state == State::Running && r.summoned.count("Walky") == 1);
    }
}

// T5: a bot that comes, but too slowly, is waited for until kGatherComingMaxMs
// and then summoned once (it is far and gaining, which SummonStraggler leaves
// alone); the run pauses only if the summon does not bring it.
static void TestComingStragglerIsSummonedAfterTheComingLimit()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 150;
    f.members = {Mem("Tanky", 0), Mem("Slowpoke", 150)};
    f.units = {Unit(7, 1, 50)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    std::string order;
    std::uint32_t ticks = 1;
    for (; ticks < 200 && order.empty(); ++ticks)
    {
        float const yd = 150.0f - 0.5f * static_cast<float>(ticks);
        f.members[1] = Mem("Slowpoke", yd);
        f.maxMemberYd = yd;
        order = Tick(r, f);
        CHECK(r.state == State::Running);
    }
    CHECK(order == "summon Slowpoke");
    CHECK(ticks >= kGatherComingMaxMs / kTickMs - 5 && ticks <= kGatherComingMaxMs / kTickMs + 8);
    CHECK(r.phase == Phase::Gather && r.note.find("Slowpoke") != std::string::npos);
    // Once only: it is still far at the next ceiling (the summon was refused or did not bring it).
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && r.state == State::Running; ++i)
    {
        float const yd = 100.0f - 0.5f * static_cast<float>(i);
        f.members[1] = Mem("Slowpoke", yd);
        f.maxMemberYd = yd;
        CHECK(Tick(r, f).find("summon") == std::string::npos);
    }
    CHECK(r.state == State::Paused && r.note.find("did not gather") != std::string::npos);
}

// A bot that is not walking in is not waited for even if it is nearer than at
// the last look: one that stands still in a fight (busy, `cmd summon` refuses
// it) pauses the run as before.
static void TestStillBotInAFightIsNotWaitedFor()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 45;
    f.members = {Mem("Tanky", 0), Mem("Stoutster", 45)};
    f.members[1].inCombat = true;
    f.units = {Unit(7, 1, 50)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    f.members[1] = Mem("Stoutster", 38); // nearer by 7 yd, then it stops
    f.members[1].inCombat = true;
    f.maxMemberYd = 38;
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && r.state == State::Running; ++i)
        CHECK(Tick(r, f).find("summon") == std::string::npos);
    CHECK(r.state == State::Paused && r.note.find("did not gather") != std::string::npos);
}

// Live (events-5 01:48:43 -> 01:50:42): after the wipe the regroup's gather
// waited its whole ceiling (60 s, a bot 363 yd out) for a bot that had not moved
// at all; it is summoned after kStuckSummonMs instead.
static void TestRegroupSummonsAStuckFarBotEarly()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 120;
    f.members = {Mem("Tanky", 0), Mem("Stoutster", 120)};
    Pause(r, "test");
    CHECK(Resume(r) && r.regroup);
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the regroup's gather
    std::string order;
    std::uint32_t ticks = 1;
    for (; ticks < 30 && order.empty(); ++ticks)
        order = Tick(r, f);
    CHECK(order == "summon Stoutster");
    CHECK(ticks >= kStuckSummonMs / kTickMs && ticks <= kStuckSummonMs / kTickMs + 2);
    CHECK(r.state == State::Running && r.regroup);
    CHECK(r.note.find("has not moved for 15 s") != std::string::npos && r.note.find("summoned") != std::string::npos);
}

// The 15 s are the gather's own clock. The stand-still watch runs from before
// the gather (it keeps counting while the dead are revived), so without this a
// bot that stood still 120 yd away during the revives was summoned one tick
// after the gather went out -- before anyone could have seen it walk.
static void TestStuckSummonCountsFromTheGather()
{
    DungeonPlan plan;
    plan.steps = {PullStep(100, {1})};
    Run r = Start(&plan);
    BeginRegroup(r);
    Facts f = Party(0);
    f.total = 5;
    f.alive = 4;
    f.dead = 1;
    f.deadNames = {"Quelinator"};
    f.maxMemberYd = 120;
    f.members = {Mem("Tanky", 0), Mem("Stoutster", 120)};
    // The corpse run: 20 ticks of revive before the dead are up. The stand-still
    // watch of Stoutster runs all through them.
    for (int i = 0; i < 20; ++i)
        Tick(r, f);
    CHECK(r.still["Stoutster"].ms >= kStuckSummonMs);
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the regroup's gather: its clock starts
    CHECK(Tick(r, f).empty());               // NOT summoned yet
    std::string order;
    std::uint32_t ticks = 0;
    for (; ticks < 30 && order.empty(); ++ticks)
        order = Tick(r, f);
    CHECK(order == "summon Stoutster");
    CHECK(ticks >= kStuckSummonMs / kTickMs - 1 && ticks <= kStuckSummonMs / kTickMs + 1);
    CHECK(r.note.find("has not moved for 15 s") != std::string::npos);
}

// Codex on #1000: a walk-back point refused for the lead bot's state of the
// moment is sent again (3 times, then pause); only a path refusal passes it.
static void TestWalkBackRefusalKinds()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10), Step(PlanStepKind::Move, 50), PullStep(130, {1})};
    Run r = Start(&plan, 2);
    r.walkBack = true;
    r.walkBackIdx = 0;
    r.gatherWaived = true;
    r.gatherRefusals = 2;
    Facts f = Party(0);
    CHECK(Tick(r, f) == "move 10.0 0.0 0.0");
    for (std::uint32_t i = 1; i <= kWalkBackRetries; ++i)
    {
        OrderRefused(r, "move 10.0 0.0 0.0", "lead-unavailable: The lead bot is dead, on a taxi or teleporting.");
        CHECK(r.state == State::Running && r.walkBackIdx == 0 && r.note.find("again") != std::string::npos);
        CHECK(Tick(r, f) == "move 10.0 0.0 0.0");
    }
    OrderRefused(r, "move 10.0 0.0 0.0", "busy: a task runs");
    CHECK(r.state == State::Paused && r.walkBackIdx == 0);

    Run p = Start(&plan, 2);
    p.walkBack = true;
    p.gatherWaived = true;
    p.gatherRefusals = 2;
    CHECK(Tick(p, f) == "move 10.0 0.0 0.0");
    OrderRefused(p, "move 10.0 0.0 0.0", "lead-unavailable: teleporting");
    OrderRefused(p, "move 10.0 0.0 0.0", "off-mesh: The point is not on walkable ground.");
    // Passed: the next point, its own refusal count and gather waiver fresh.
    CHECK(p.state == State::Running && p.walkBackIdx == 1 && p.walkBackRefusals == 0);
    CHECK(!p.gatherWaived && p.gatherRefusals == 0);
    CHECK(Tick(p, f) == "move 50.0 0.0 0.0");
}

// Cold review on #1000: a pullback drags the boss to the party, so his
// corpse 30 yd from his spot is normal (radius + kSeeYd); and a boss seen
// alive in this step and now dead near the lead bot is down wherever he fell.
static void TestBossKilledAwayFromHisSpot()
{
    DungeonPlan plan;
    PlanStep boss = Step(PlanStepKind::Boss, 30);
    boss.entry = 6910;             // Uldaman: Revelosh, his adds first
    boss.entries = {7076, 7077};
    ai::NormalizePlanStep(boss);
    plan.steps = {boss, Step(PlanStepKind::Move, 60)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(5, 6910, 30)};
    CHECK(Tick(r, f) == "pullback 5");
    CHECK(r.bossSeenAlive);
    f.anyInCombat = true;
    Tick(r, f);
    f.anyInCombat = false;
    f.units.clear();
    f.bossDeadNearSpot = true; // the glue: his corpse 30 yd from the spot, within radius + kSeeYd
    Tick(r, f);
    CHECK(r.step == 1 && r.note == "boss down");

    // Fell far from his spot but near the lead bot: down, because he was seen alive.
    Run n = Start(&plan);
    Facts g = Party(0);
    g.units = {Unit(5, 6910, 30)};
    CHECK(Tick(n, g) == "pullback 5");
    g.anyInCombat = true;
    Tick(n, g);
    g.anyInCombat = false;
    g.units.clear();
    g.bossDeadNearLead = true;
    Tick(n, g);
    CHECK(n.step == 1 && n.note == "boss down");
    // Never seen alive this step: a corpse near the lead bot alone is not enough.
    Run m = Start(&plan);
    Facts h = Party(0);
    h.bossDeadNearLead = true;
    Tick(m, h);
    CHECK(m.step == 0 && m.note.find("not at his spot") != std::string::npos);
}

// Cold review on #1000: after a wipe the party stands at the entrance and the
// nearest walk-back point is more than ReactDistance away: it is approached in
// navmesh hops (as a resume does), not sent as one refused move; with no path
// toward it at all it is passed. Walk-back pulls have their own count.
static void TestWalkBackFarPointIsApproached()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 200), Step(PlanStepKind::Move, 230), PullStep(400, {1})};
    Run r = Start(&plan, 2);
    r.walkBack = true;
    Facts f = Party(0);
    f.seen = false; // FactStep: the walk-back point, 200 yd away
    f.approachKnown = true;
    f.approachX = 40;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    CHECK(r.note.find("walk back: approaching") == 0);
    f.leadX = 40;
    CHECK(Tick(r, f).empty()); // at the hop: the next one next tick
    f.approachX = 80;
    CHECK(Tick(r, f) == "move 80.0 0.0 0.0");
    f.leadX = 80;
    Tick(r, f);
    f.seen = true;
    CHECK(Tick(r, f) == "move 200.0 0.0 0.0");
    // A hostile on the way: pulled, counted apart from the step's pulls.
    f.hopUnits = {Unit(9, 3, 20)};
    CHECK(Tick(r, f) == "pullback 9");
    CHECK(r.pulls == 0 && r.walkBackPulls == 1);

    // No path toward a far point: passed for the next one.
    Run p = Start(&plan, 2);
    p.walkBack = true;
    Facts g = Party(0);
    g.seen = false;
    g.approachKnown = false;
    CHECK(Tick(p, g).empty());
    CHECK(p.state == State::Running && p.walkBackIdx == 1 && p.note.find("no walkable path") != std::string::npos);
}

// Codex on #1000 / 19z5 step 57: only an invalid-target refusal of a pullback
// leaves the unit out (for the run, no pause); a busy tank or an unavailable
// lead bot pauses as before.
static void TestRefusedPullOnlyForInvalidTarget()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    OrderRefused(r, "pullback 7", "busy: the tank is pulling");
    CHECK(r.state == State::Paused && !r.unreachable.count(7));
    Run l = Start(&plan);
    OrderRefused(l, "pullback 7", "lead-unavailable: The lead bot is dead");
    CHECK(l.state == State::Paused && !l.unreachable.count(7));
    Run i = Start(&plan);
    OrderRefused(i, "pullback 7", "invalid-target: The group cannot attack that target.");
    CHECK(i.state == State::Running && i.unreachable.count(7) && i.note.find("left out") != std::string::npos);
}

// Codex on #1001: the dead lead bot already released to the graveyard map;
// the glue lets the tick run (its map guard pauses only a living lead bot off
// the map), and the wipe starts the regroup.
static void TestWipeWithTheLeadOnTheGraveyardMap()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10), PullStep(130, {1})};
    Run r = Start(&plan, 1);
    Facts f = Party(-9000); // the lead's ghost far away on another map
    f.dead = 5;
    f.alive = 0;
    f.leadAlive = false;
    f.maxMemberYd = 0;
    f.seen = false;
    CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Running && r.regroup && r.wipes == 1);
    CHECK(Tick(r, f) == "revive party");
}

// Codex on #1000: a near wipe that becomes a wipe, then the revive one by one
// (4 dead, 3 dead with someone up again), is one near wipe and one wipe.
static void TestNearWipeLatchHoldsThroughTheWipe()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.anyInCombat = true;
    f.dead = 3;
    f.alive = 2;
    Tick(r, f);
    CHECK(r.nearWipes == 1 && r.wipes == 0);
    f.anyInCombat = false;
    f.dead = 5;
    f.alive = 0;
    f.leadAlive = false;
    Tick(r, f);
    CHECK(r.nearWipes == 1 && r.wipes == 1);
    for (std::uint32_t dead : {4u, 3u})
    {
        f.dead = dead;
        f.alive = 5 - dead;
        Tick(r, f);
        CHECK(r.nearWipes == 1 && r.wipes == 1);
    }
    // All up again: a later near wipe counts anew.
    f = Party(0);
    f.units = {Unit(7, 1, 20)};
    Tick(r, f);
    CHECK(r.state == State::Running);
    f.anyInCombat = true;
    f.dead = 3;
    f.alive = 2;
    Tick(r, f);
    CHECK(r.nearWipes == 2 && r.wipes == 1);
}

// 19z5 steps 27 and 81: the lead bot died first; the run paused "the lead bot
// is dead" and the rest died while it waited. Now the others fight on; when
// they fall too it is a wipe and the run regroups by itself. A dead lead bot
// alone after the fight still pauses.
static void TestLeadDiesFirstThenTheWipeRegroups()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10), PullStep(130, {1})};
    Run r = Start(&plan, 1);
    Facts f = Party(130);
    f.units = {Unit(7, 1, 10)};
    f.anyInCombat = true;
    f.leadAlive = false;
    f.dead = 1;
    f.alive = 4;
    Tick(r, f);
    CHECK(r.state == State::Running && r.note.find("the others fight on") != std::string::npos);
    f.dead = 5;
    f.alive = 0;
    f.anyInCombat = false;
    Tick(r, f);
    CHECK(r.state == State::Running && r.regroup && r.wipes == 1);

    Run p = Start(&plan, 1);
    Facts g = Party(130);
    g.units = {Unit(7, 1, 10)};
    g.leadAlive = false;
    g.dead = 1;
    g.alive = 4;
    Tick(p, g);
    CHECK(p.state == State::Running && p.regroup && p.note.find("the lead bot is dead") != std::string::npos);
}

// 19z5 step 80 (15:43): 4 dead and the tank alone past the fight ceiling
// paused as "a fight lasted more than 180 s". It is a wipe: the run regroups
// by itself, within the step's budget.
static void TestFightCeilingWithFourDeadRegroups()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.anyInCombat = true;
    f.dead = 4;
    f.alive = 1;
    for (std::uint32_t i = 0; i <= kFightCapMs / kTickMs + 1 && !r.regroup; ++i)
        Tick(r, f);
    CHECK(r.state == State::Running && r.regroup && r.autoRegroups == 1 && r.note.find("went past") != std::string::npos);
    // Over budget: the ceiling pauses as before.
    Run p = Start(&plan);
    p.autoRegroups = kAutoRegroupsPerStep;
    for (std::uint32_t i = 0; i <= kFightCapMs / kTickMs + 1 && p.state == State::Running; ++i)
        Tick(p, f);
    CHECK(p.state == State::Paused);
}

// 19z5 step 83: Draezur 29 yd away but on another floor (z 21 against 43);
// the gather paused twice. A bot more than 8 yd up or down that comes no
// closer is a straggler too.
static void TestStragglerOnAnotherFloorIsSummoned()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.maxMemberYd = 29.1f;
    MemberFact healer{"Draezur", 29.1f};
    healer.dz = 22;
    MemberFact near{"Nearby", 25};
    near.dz = 2;
    f.members = {near, healer};
    f.units = {Unit(7, 1, 20)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    std::string order;
    for (std::uint32_t i = 0; i < kGatherCapMs / kTickMs + 2 && order.empty(); ++i)
        order = Tick(r, f);
    CHECK(order == "summon Draezur");
}

// 19z5 step 27 and 81: after a revive by hand and `.bot run resume` the party
// stood at the entrance and the step's approach paused (no path / 8 hops).
// A resume walks back too: straight to the last plan point, in hops.
static void TestResumeFromTheEntranceWalksBack()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10), Step(PlanStepKind::Move, 300), PullStep(330, {1})};
    Run r = Start(&plan, 2);
    Pause(r, "the lead bot is dead: revive it, then .bot run resume");
    CHECK(Resume(r));
    Facts f = Party(0);
    CHECK(Tick(r, f).empty()); // regrouped
    CHECK(r.walkBack && r.walkBackDirect && r.walkBackIdx == 1);
    f.seen = false;
    f.approachKnown = true;
    f.approachX = 40;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
}

// 19z5 steps 83-84: the approach hop left the party at about 1622 1051 z 18;
// the tank chased over a ridge to 1625 1075 z 44 in the fight, and the next
// gather (on the lead's own spot) pinned the party on the hilltop, where no
// path led down. A gather now goes back to the run's anchor (the last point
// the run walked to) when the lead bot is off it, and gathers there.
static void TestGatherGoesBackToTheAnchorAfterARidgeChase()
{
    DungeonPlan plan;
    plan.steps = {PullStep(1639.4f, {5649, 5650})};
    plan.steps[0].y = 1106.8f;
    plan.steps[0].z = 9.0f;
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadX = 1584; f.leadY = 1082; f.leadZ = 10;
    f.seen = false;
    f.approachKnown = true;
    f.approachX = 1622; f.approachY = 1051; f.approachZ = 18;
    CHECK(Tick(r, f) == "move 1622.0 1051.0 18.0");
    f.leadX = 1622; f.leadY = 1051; f.leadZ = 18;
    Tick(r, f); // at the hop: the anchor
    CHECK(r.anchorKnown && r.anchorZ == 18.0f);
    // In sight; the fight drags the tank up the ridge.
    f.seen = true;
    f.units = {Unit(1, 5649, 25)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    f.leadX = 1625; f.leadY = 1075; f.leadZ = 44;
    Tick(r, f);
    f.anyInCombat = false;
    f.units = {Unit(2, 5650, 30)};
    f.maxMemberYd = 29.1f;
    // The gather walks the lead bot back down to the anchor, not up the hill.
    CHECK(Tick(r, f) == "move 1622.0 1051.0 18.0");
    CHECK(r.phase == Phase::Gather && r.note.find("gathering back at 1622.0 1051.0 18.0") == 0);
    // Even with the party close, no pull while the lead bot is still up there.
    f.maxMemberYd = 5;
    CHECK(Tick(r, f).empty());
    // Back at the anchor and gathered: the pull goes out.
    f.leadX = 1622; f.leadY = 1052; f.leadZ = 18;
    CHECK(Tick(r, f) == "pullback 2");

    // Same floor but no navmesh path back (a pocket): back to the anchor too.
    Run p = Start(&plan);
    p.anchorKnown = true;
    p.anchorX = 1622; p.anchorY = 1051; p.anchorZ = 18;
    Facts g = Party(0);
    g.leadX = 1627; g.leadY = 1078; g.leadZ = 22;
    g.units = {Unit(2, 5650, 30)};
    // The path back is read lazily: asked for on the first tick, no pull yet.
    CHECK(Tick(p, g).empty() && p.anchorPathWanted && p.pulls == 0 && p.fruitlessPulls == 0);
    g.anchorPathRead = true;
    g.anchorReachable = false;
    g.units = {Unit(2, 5650, 30)};
    CHECK(Tick(p, g) == "move 1622.0 1051.0 18.0");
    // On the anchor's floor with a path back: the gather stays on the lead's spot.
    Run q = Start(&plan);
    q.anchorKnown = true;
    q.anchorX = 1622; q.anchorY = 1051; q.anchorZ = 18;
    Facts h = Party(0);
    h.leadX = 1630; h.leadY = 1060; h.leadZ = 20;
    h.anchorPathRead = true;
    h.maxMemberYd = 30;
    h.units = {Unit(2, 5650, 30)};
    CHECK(Tick(q, h) == "move 1630.0 1060.0 20.0");
}

// Codex on #1000: the shortest navmesh path back ignores packs. A direct walk
// back whose path passes within a not-yet-cleared pull group's radius + 10 yd
// drops the shortcut and replays the plan's points; no move enters the pack.
static void TestDirectWalkBackAvoidsUnclearedPacks()
{
    DungeonPlan plan;
    PlanStep later = PullStep(60, {9});
    later.y = 20;   // a later pack, 20 yd off the straight line
    ai::NormalizePlanStep(later);
    plan.steps = {Step(PlanStepKind::Move, 30), Step(PlanStepKind::Move, 120), PullStep(150, {1}), later};
    // Geometry: the straight path from 0 to 120 passes 20 yd from the later
    // pack (radius 15 + 10); a path that bends away does not.
    std::vector<PathPoint> straight = {{0, 0, 0}, {120, 0, 0}};
    std::vector<PathPoint> around = {{0, 0, 0}, {60, -30, 0}, {120, 0, 0}};
    CHECK(PathCrossesUnclearedPack(plan, 2, straight, 0));
    CHECK(!PathCrossesUnclearedPack(plan, 2, around, 0));
    CHECK(!PathCrossesUnclearedPack(plan, 4, straight, 0)); // that pack is cleared (an earlier step)

    Run r = Start(&plan, 2);
    Facts f = Party(0);
    r.walkBack = true;
    r.walkBackDirect = true;
    r.walkBackIdx = 1;
    f.seen = false;
    f.approachKnown = true;
    f.approachX = 40;
    f.approachY = 0;
    f.directPathCrossesPack = true;
    std::string order = Tick(r, f);
    CHECK(order.empty() && !r.walkBackDirect && r.walkBack && r.walkBackIdx == 0);
    CHECK(r.note.find("passes an uncleared pack") != std::string::npos);
    // The replay walks the plan's first point (30 0 0), not the hop toward the pack.
    f.seen = true;
    f.directPathCrossesPack = false;
    CHECK(Tick(r, f) == "move 30.0 0.0 0.0");
}

// Cold review on #1000: a stalled arrival short of the point (off the mesh)
// anchors where the lead bot stands, not at the unreachable point; skipped
// packs count in the walk back's pack check, the path's last leg too.
static void TestStalledAnchorAndSkippedPacks()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    f.leadX = 35; // 5 yd short, stuck
    for (std::uint32_t i = 0; i <= kWalkRetryMs / kTickMs + 1 && r.step == 0; ++i)
        Tick(r, f);
    CHECK(r.step == 1 && r.anchorKnown && r.anchorX == 35.0f);

    DungeonPlan p;
    PlanStep skipped = PullStep(60, {9});
    skipped.y = 20;
    ai::NormalizePlanStep(skipped);
    PlanStep atEnd = PullStep(120, {8});
    ai::NormalizePlanStep(atEnd);
    p.steps = {skipped, Step(PlanStepKind::Move, 120), atEnd, Step(PlanStepKind::Move, 200)};
    std::vector<PathPoint> straight = {{0, 0, 0}, {119, 0, 0}};
    std::unordered_set<std::size_t> notCleared;
    CHECK(!PathCrossesUnclearedPack(p, 3, straight, 0, notCleared)); // both groups before step 3, cleared
    notCleared.insert(0);
    CHECK(PathCrossesUnclearedPack(p, 3, straight, 0, notCleared)); // the skipped one counts
    CHECK(PathCrossesUnclearedPack(p, 2, straight, 0));             // a group by the path's end counts too
}

// Issue #999 item 5 (Zul'Farrak): the Sandfury Witch Doctor 5650 heals its
// pack and drops a Greater Healing Ward 8179. Before the pull the mage
// polymorphs the best-ranked cc unit (never the pull target); the pull goes
// on a kill_first unit; in the fight the kill_first units are focused, the
// ward first as soon as it is up.
static PlanStep ZfPack()
{
    return PullStep(10, {5650, 5648, 7247});
}

static DungeonPlan ZfPlan()
{
    DungeonPlan plan;
    plan.steps = {ZfPack()};
    plan.ccPriority = {5650, 7247, 7246, 5648};
    plan.ccAlways = {5650, 7246};
    plan.killFirst = {8179, 5650, 7247};
    return plan;
}

static UnitFact Engaged(UnitFact u)
{
    u.engaged = true;
    return u;
}

// A Greater Healing Ward 8179 of `owner`; ownerEngaged: its owner fights the party.
static UnitFact Ward(std::uint64_t guid, std::uint64_t owner, bool ownerEngaged, float dist = 18)
{
    UnitFact u = Unit(guid, 8179, dist);
    u.owner = owner;
    u.ownerEngaged = ownerEngaged;
    return u;
}

// The gather left out for a pull after a hop was abandoned for a close unit
// holds for that pull only: combat that starts during the pre-pull crowd
// control ends it, and the next pull gathers (and goes back to the anchor).
static void TestHopWaiverEndsWhenCombatStartsDuringCrowdControl()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    r.restDue = false;
    r.anchorKnown = true; // the stand before, 20 yd below the lead bot
    r.anchorX = 0; r.anchorY = 0; r.anchorZ = -20;
    EnterPhase(r, Phase::Walk); // an approach hop the pack came in sight during
    r.hopWalk = true;
    r.hopX = r.walkX = 5; r.hopY = r.walkY = 0; r.hopZ = r.walkZ = 0;
    Facts f = Party(0);
    f.members = {MemberFact{"Magey", 5, true, true}};
    f.maxMemberYd = 40; // the followers are behind
    f.units = {Unit(1, 5648, 10), Unit(2, 5650, 20), Unit(3, 7247, 25)};
    // The hop is over, no gather: the crowd control goes first.
    CHECK(Tick(r, f) == "cc moon 2 Magey");
    CHECK(r.gatherWaived && !r.hopWaiveUnits.empty());
    // Combat starts before the pullback.
    f.anyInCombat = true;
    Tick(r, f);
    CHECK(!r.gatherWaived && r.hopWaiveUnits.empty());
    // The fight is over; the next pull gathers first, back at the anchor.
    f.anyInCombat = false;
    std::string order;
    for (int i = 0; i < 40 && order.empty() && r.state == State::Running; ++i)
        order = Tick(r, f);
    CHECK(order.compare(0, 5, "move ") == 0);
    CHECK(r.note.find("gathering") == 0);
}

static void TestPullCrowdControlsTheHealer()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.members = {MemberFact{"Magey", 5, true, true}};
    f.units = {Unit(1, 5648, 10), Unit(2, 5650, 20), Unit(3, 7247, 25)};
    // The witch doctor is the best cc rank: it is sheeped, not pulled.
    CHECK(Tick(r, f) == "cc moon 2 Magey");
    CHECK(r.note.find("crowd control on 5650") == 0 && r.state == State::Running);
    // The pull goes on the soul eater (kill_first) rather than the nearest.
    CHECK(Tick(r, f) == "pullback 3");
    CHECK(r.pulls == 1 && r.fruitlessPulls == 0);
    // In the fight: the soul eater is focused; the sheeped witch doctor is not.
    f.anyInCombat = true;
    f.killFirst = {Engaged(Unit(2, 5650, 20)), Engaged(Unit(3, 7247, 5))};
    CHECK(Tick(r, f) == "focus 3");
    CHECK(Tick(r, f).empty()); // only on a change
    // A ward comes up: it is focused first.
    f.killFirst.push_back(Ward(9, 3, true, 12)); // a pull step: within kKillFirstMaxYd
    CHECK(Tick(r, f) == "focus 9");
    // The ward dies: back on the soul eater.
    f.killFirst.pop_back();
    CHECK(Tick(r, f) == "focus 3");
    // After the fight: the sheeped witch doctor is left while another can be
    // pulled, and no second cc goes out while it lives.
    f.anyInCombat = false;
    f.killFirst.clear();
    f.units = {Unit(1, 5648, 10), Unit(2, 5650, 20)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    Tick(r, f);
    f.anyInCombat = false;
    f.units = {Unit(2, 5650, 20)};
    CHECK(Tick(r, f) == "pullback 2"); // the last one, sheeped or not
    // A refused cc or focus does not pause.
    OrderRefused(r, "cc moon 2 Magey", "no-bot");
    OrderRefused(r, "focus 3", "invalid-target");
    CHECK(r.state == State::Running);
}

static void TestNoCrowdControlWithoutAMageOrPlanList()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(1, 5648, 10), Unit(2, 5650, 20)};
    // No mage: no cc; the pull still prefers kill_first.
    CHECK(Tick(r, f) == "pullback 2");

    DungeonPlan bare;
    bare.steps = {ZfPack()};
    Run b = Start(&bare);
    Facts g = Party(0);
    g.members = {MemberFact{"Magey", 5, true, true}};
    g.units = {Unit(1, 5648, 10), Unit(2, 5650, 20)};
    CHECK(Tick(b, g) == "pullback 1"); // the nearest, as before
    g.anyInCombat = true;
    g.killFirst = {Engaged(Unit(2, 5650, 20))};
    CHECK(Tick(b, g).empty());

    // A lone cc unit is pulled, not sheeped.
    Run c = Start(&plan);
    Facts h = Party(0);
    h.members = {MemberFact{"Magey", 5, true, true}};
    h.units = {Unit(2, 5650, 20)};
    CHECK(Tick(c, h) == "pullback 2");
}

// Codex on #1001: the Polymorph is pending until its aura shows on that unit
// in the fight (the mage's cc strategy is a combat one): not within
// kCcLandMs, the mark once more; still not, noted, and the fight goes on.
static void TestCrowdControlMustLand()
{
    DungeonPlan plan = ZfPlan();
    plan.killFirst.clear();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.members = {MemberFact{"Tanky", 0, false}, MemberFact{"Magey", 5, true, true}};
    f.units = {Unit(1, 5648, 10), Unit(2, 5650, 20)};
    CHECK(Tick(r, f) == "cc moon 2 Magey");
    CHECK(r.ccPending == 2);
    CHECK(Tick(r, f) == "pullback 1"); // the mark is out; the pull starts the fight
    f.anyInCombat = true;
    CHECK(Tick(r, f).empty() && r.ccPending == 2);
    f.units[1].polymorphed = true; // landed
    CHECK(Tick(r, f).empty());
    CHECK(r.ccPending == 0 && r.note.find("polymorphed") != std::string::npos);

    // Never lands: once more after kCcLandMs, then noted.
    Run n = Start(&plan);
    Facts g = Party(0);
    g.members = {MemberFact{"Magey", 5, true, true}};
    g.units = {Unit(1, 5648, 10), Unit(2, 5650, 20)};
    CHECK(Tick(n, g) == "cc moon 2 Magey");
    CHECK(Tick(n, g) == "pullback 1");
    g.anyInCombat = true;
    std::string order;
    for (std::uint32_t i = 0; i <= kCcLandMs / kTickMs && order.empty(); ++i)
        order = Tick(n, g);
    CHECK(order == "cc moon 2 Magey" && n.ccResent && n.note.find("once more") != std::string::npos);
    order.clear();
    for (std::uint32_t i = 0; i <= kCcLandMs / kTickMs && n.ccPending; ++i)
        order += Tick(n, g);
    CHECK(order.empty() && n.ccPending == 0 && n.note.find("cc did not land on 5650") != std::string::npos);
    CHECK(n.state == State::Running);

    // A mage that does not know Polymorph is not the crowd control.
    Run m = Start(&plan);
    Facts h = Party(0);
    h.members = {MemberFact{"Lowmage", 5, false}};
    h.units = {Unit(1, 5648, 10), Unit(2, 5650, 20)};
    CHECK(Tick(m, h) == "pullback 1" && m.ccSent.empty());
}

// Codex on #1001: kill_first is this fight's only. A ward whose owner fights
// the party is focused; a ward of an unrelated witch doctor nearby is not,
// nor a kill_first unit that is not engaged.
static void TestKillFirstIsThisFightsOnly()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(1, 5648, 10)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    f.killFirst = {Ward(20, 30, false), Unit(30, 5650, 25)}; // unrelated ward and its idle owner
    CHECK(Tick(r, f).empty());
    f.killFirst.push_back(Ward(21, 1, true, 12)); // the ward of a unit in this fight (a pull step: within kKillFirstMaxYd)
    CHECK(Tick(r, f) == "focus 21");
}

// A mob hitting the healer `dist` from the lead bot (Hitter is defined further down).
static UnitFact ZfHitter(std::uint64_t guid, std::uint32_t entry, float dist)
{
    UnitFact u = Unit(guid, entry, dist);
    u.hitting = true;
    u.victimHealer = true;
    return u;
}

// ZF r5-19z23 step 27, wipe 1 (15:10:58): the tank stood 20 yd from the
// antusul camp's 5650 when two witch doctors aggroed on it; "kill 5650 first"
// sent the tank and the party into the camp, next to the north group, and
// 13 mobs joined at once. A kill_first unit more than kKillFirstMaxYd from the
// lead bot is not focused (it is left to come to the tank, as in r3); within
// it, it is. The fight goes on with the bots' own picks in between.
static void TestKillFirstFarIsNotFocusedWipe1Replay()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadIsTank = true;
    f.leadStaying = true; // on firing stand 1
    f.units = {Unit(1, 5648, 24.5f)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    // The camp's two witch doctors, engaged with the tank from 20 and 23 yd.
    f.killFirst = {Engaged(Unit(30, 5650, 20)), Engaged(Unit(31, 5650, 23))};
    for (int i = 0; i < 20; ++i)
        CHECK(Tick(r, f).empty()); // no focus: the tank stays on its stand
    CHECK(r.focusGuid == 0 && r.state == State::Running);
    // One walks in to the tank: focused now.
    f.killFirst[0].distLead = kKillFirstMaxYd - 1.0f;
    CHECK(Tick(r, f) == "focus 30");
    // At exactly the bound it counts as near; just past it, not.
    Run b = Start(&plan);
    Facts g = Party(0);
    g.units = {Unit(1, 5648, 10)};
    CHECK(Tick(b, g) == "pullback 1");
    g.anyInCombat = true;
    g.killFirst = {Engaged(Unit(40, 5650, kKillFirstMaxYd + 0.5f))};
    CHECK(Tick(b, g).empty());
    g.killFirst[0].distLead = kKillFirstMaxYd;
    CHECK(Tick(b, g) == "focus 40");
}

// The bound picks a near unit over a far better-ranked one, and a focus once
// sent stays while its unit lives: a better-ranked unit that walks back and
// forth across kKillFirstMaxYd must not flip the focus every tick.
static void TestKillFirstBoundDoesNotFlipFlop()
{
    DungeonPlan plan = ZfPlan(); // kill_first 8179, 5650, 7247
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(1, 5648, 10)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    // A 5650 (rank 1) far, a 7247 (rank 2) near: the near one.
    f.killFirst = {Engaged(Unit(30, 5650, 30)), Engaged(Unit(3, 7247, 5))};
    CHECK(Tick(r, f) == "focus 3");
    int focuses = 0;
    for (int i = 0; i < 40; ++i)
    {
        // The 5650 drifts in and out across the bound.
        f.killFirst[0].distLead = (i % 2) ? kKillFirstMaxYd - 2.0f : kKillFirstMaxYd + 2.0f;
        if (Tick(r, f).compare(0, 6, "focus ") == 0)
            ++focuses;
    }
    // One switch to the better rank once it came in, then it holds.
    CHECK(focuses == 1 && r.focusGuid == 30);
    // The focused 5650 steps out of the bound again: still the focus.
    f.killFirst[0].distLead = 30;
    CHECK(Tick(r, f).empty() && r.focusGuid == 30);
}

// A focus is never dropped for distance: no order takes it back (the skull and
// every bot's attack stay), so the policy must not think it over while the
// party still chases. A focused unit that walks back toward its camp keeps
// the focus (the known risk) and the guard keeps watching; it ends only when
// the unit is gone: dead (out of the list) or crowd-controlled.
static void TestHeldFocusEndsOnlyWhenItsUnitIsGone()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadIsTank = true;
    f.units = {Unit(1, 5648, 10)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    f.killFirst = {Engaged(Unit(30, 5650, 12))};
    CHECK(Tick(r, f) == "focus 30");
    f.leadStaying = false;
    f.killFirst[0].distLead = 40; // back toward its camp
    for (int i = 0; i < 10; ++i)
        CHECK(Tick(r, f).empty() && r.focusGuid == 30);
    // Alive but not pickable (evading): still held, the guard still watches.
    f.killFirst[0].evading = true;
    CHECK(Tick(r, f).empty() && r.focusGuid == 30);
    f.partyAttackers = {ZfHitter(50, 5649, 8)};
    CHECK(Tick(r, f) == "attack 50 Tanky" && r.guardOrders == 1);
    // Pick-ups stay available under the cap and the spacing.
    f.partyAttackers = {ZfHitter(51, 5649, 6)};
    int more = 0;
    for (int i = 0; i < 100; ++i)
        if (Tick(r, f).compare(0, 7, "attack ") == 0)
            ++more;
    CHECK(more == static_cast<int>(kGuardOrdersPerFight) - 1);
    // Crowd-controlled by the run: gone, the focus ends.
    Run c = Start(&plan);
    Facts g = Party(0);
    g.units = {Unit(1, 5648, 10)};
    CHECK(Tick(c, g) == "pullback 1");
    g.anyInCombat = true;
    g.killFirst = {Engaged(Unit(30, 5650, 12))};
    CHECK(Tick(c, g) == "focus 30");
    c.ccSent.insert(30);
    CHECK(Tick(c, g).empty() && c.focusGuid == 0);
}

// A boss step keeps the old rule: the plan's kill_first unit is focused
// wherever it stands (Zum'rah's Ward 7785 first while the tank holds his adds;
// the witch doctors, the 8179 ward and Anacondra are casters that keep ~25 yd).
static void TestKillFirstOnABossStepIsNotBounded()
{
    DungeonPlan plan = ZfPlan();
    PlanStep boss = Step(PlanStepKind::Boss, 10);
    boss.entry = 7271;
    ai::NormalizePlanStep(boss);
    plan.steps = {boss, Step(PlanStepKind::Rest)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(1, 7271, 10)};
    f.anyInCombat = true;
    f.killFirst = {Ward(9, 1, true, 20)};
    CHECK(Tick(r, f) == "focus 9");
    CHECK(KillFirstBounded(r) == false);
    // Far off and back: no drop on a boss step.
    f.killFirst[0].distLead = 38;
    CHECK(Tick(r, f).empty() && r.focusGuid == 9);
}

// The guard watches for a focus only while it is on: the focused unit dies
// (no kill_first unit left), the focus is cleared, and a mob on the healer
// with the tank off its stay and no pick-up or engage yet this fight is left
// to the tank's own AI, as before the focus rule.
static void TestGuardStopsWatchingWhenTheFocusEnds()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadIsTank = true;
    f.units = {Unit(1, 5648, 10)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    f.killFirst = {Engaged(Unit(30, 5650, 8))};
    CHECK(Tick(r, f) == "focus 30");
    f.leadStaying = false;
    f.killFirst.clear(); // it died
    CHECK(Tick(r, f).empty() && r.focusGuid == 0);
    f.partyAttackers = {ZfHitter(50, 5649, 8)};
    for (int i = 0; i < 30; ++i)
        CHECK(Tick(r, f).empty());
    CHECK(r.guardOrders == 0);
}

// ZF r5-19z23 step 27, wipe 2 (15:14:23): "kill 5650 first" took every bot off
// "stay" (CommanderFocus -> RelaxTacticalMovement), the lead included, with
// guardOrders 0 and engageGuid 0, so GuardTarget never watched: 0 pick-ups the
// whole fight while mobs hit the priest. A focus keeps the guard watching.
static void TestGuardWatchesAfterAFocusWipe2Replay()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadIsTank = true;
    f.leadStaying = true;
    f.units = {Unit(1, 7247, 24.5f)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    f.killFirst = {Engaged(Unit(30, 5650, 8))};
    CHECK(Tick(r, f) == "focus 30");
    // The focus ended the tank's stay; nothing has been picked up or engaged.
    f.leadStaying = false;
    CHECK(r.guardOrders == 0 && r.engageGuid == 0 && r.focusGuid == 30);
    // A 5649 hits the priest 8 yd from the tank: picked up at once (healer).
    f.partyAttackers = {ZfHitter(50, 5649, 8)};
    CHECK(Tick(r, f) == "attack 50 Tanky");
    CHECK(r.guardOrders == 1);
    CHECK(r.note.find("picks up 5649 off the healer") != std::string::npos);
    // No focus is re-sent: the pick-up does not change the kill_first pick.
    CHECK(Tick(r, f).empty() && r.focusGuid == 30);
}

// The order rate in the new state (focus on, the tank off its stay): healer
// attackers changing every tick give at most kGuardOrdersPerFight pick-ups,
// each kGuardRepeatMs after the last order, and the focus is never re-sent.
static void TestGuardAfterAFocusIsRateBounded()
{
    DungeonPlan plan = ZfPlan();
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadIsTank = true;
    f.units = {Unit(1, 5648, 10)};
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    f.killFirst = {Engaged(Unit(30, 5650, 6))};
    CHECK(Tick(r, f) == "focus 30");
    f.leadStaying = false;
    int pickups = 0, focuses = 0, other = 0;
    std::uint32_t sinceLast = 0, minGap = ~0u;
    bool any = false;
    for (int i = 0; i < 300; ++i) // 30 s at kTickMs
    {
        f.partyAttackers = {ZfHitter(100 + (i % 7), 5649, 5.0f + (i % 5)), ZfHitter(200 + (i % 3), 5648, 4)};
        std::string const o = Tick(r, f);
        sinceLast += kTickMs;
        if (o.compare(0, 7, "attack ") == 0)
        {
            ++pickups;
            if (any && sinceLast < minGap)
                minGap = sinceLast;
            any = true;
            sinceLast = 0;
        }
        else if (o.compare(0, 6, "focus ") == 0)
            ++focuses;
        else if (!o.empty())
            ++other;
    }
    CHECK(pickups == static_cast<int>(kGuardOrdersPerFight));
    CHECK(minGap >= kGuardRepeatMs);
    CHECK(focuses == 0 && other == 0);
}

// 19z5: the mage enters the fight seconds after the pull (it waits for the
// tank's threat, then walks into range); its Polymorphs landed 7-8 s into its
// own fight. The window counts only the mage's fight time.
static void TestCcWindowCountsTheMagesFightTime()
{
    DungeonPlan plan = ZfPlan();
    plan.killFirst.clear();
    Run r = Start(&plan);
    Facts f = Party(0);
    MemberFact mage{"Magey", 5, true, false};
    f.members = {mage};
    f.units = {Unit(1, 5648, 10), Unit(2, 5650, 20)};
    CHECK(Tick(r, f) == "cc moon 2 Magey");
    CHECK(Tick(r, f) == "pullback 1");
    f.anyInCombat = true;
    for (int i = 0; i < 20; ++i)
        CHECK(Tick(r, f).empty()); // the mage not in the fight yet: not counted
    CHECK(r.ccPending == 2 && !r.ccResent);
    f.members[0].inCombat = true;
    std::string order;
    std::uint32_t ticks = 0;
    for (; ticks <= kCcLandMs / kTickMs && order.empty(); ++ticks)
        order = Tick(r, f);
    CHECK(order == "cc moon 2 Magey" && ticks == kCcLandMs / kTickMs);
}

static void TestStepIdleCeiling()
{
    DungeonPlan plan;
    PlanStep clear = Step(PlanStepKind::WaitClear, 0, 0, 0);
    clear.radius = 20;
    ai::NormalizePlanStep(clear);
    plan.steps = {clear};
    CHECK(StepIdleCapMs(plan.steps[0]) == kStepIdleCapMs);
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = true;
    f.hostilesNear = 2;
    for (int i = 0; i < 100 && r.state == State::Running; ++i)
        Tick(r, f);
    CHECK(r.state == State::Paused && r.note.find("hostiles still near") != std::string::npos);
}

static void TestTeamFlags()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {4851}, ai::kPlanFlagHorde), Step(PlanStepKind::Rest)};
    Facts f = Party(0);
    f.team = ai::kPlanFlagAlliance;
    f.seen = false;
    // Not seen yet: a pack for the other team is skipped without a walk.
    Run r = Start(&plan);
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 1 && r.note.find("other team") != std::string::npos);
    // Seen and nothing hostile there: skipped too.
    Run r2 = Start(&plan);
    f.seen = true;
    f.units = {Unit(5, 4851, 10)};
    f.units[0].hostile = false;
    Tick(r2, f);
    CHECK(r2.step == 1);
    // Seen and hostile anyway: fought like any pack.
    Run r3 = Start(&plan);
    f.units[0].hostile = true;
    CHECK(Tick(r3, f) == "pullback 5");
    // The party's own team, or a step for both: normal.
    Run r4 = Start(&plan);
    f.team = ai::kPlanFlagHorde;
    CHECK(Tick(r4, f) == "pullback 5");
    CHECK(ai::StepForTeam(plan.steps[0], 0) && ai::StepForTeam(plan.steps[1], ai::kPlanFlagAlliance));
}

// Live bug (first bots-only Uldaman run): the Lost Dwarves (boss flagged
// horde) were skipped for an Alliance party before anybody looked at them.
static void TestOtherTeamBossIsLookedAtAndFoughtUnlessFriendly()
{
    DungeonPlan plan;
    PlanStep boss = Step(PlanStepKind::Boss, 100);
    boss.entry = 6906;
    boss.entries = {6907, 6908};
    boss.flags = ai::kPlanFlagHorde;
    ai::NormalizePlanStep(boss);
    plan.steps = {boss, Step(PlanStepKind::Rest)};
    Facts f = Party(0);
    f.team = ai::kPlanFlagAlliance;
    f.seen = false;
    // Not in sight: walk to it, never skip.
    Run r = Start(&plan);
    f.approachKnown = true;
    f.approachX = 40;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    CHECK(r.step == 0);
    // In sight, the dwarves neutral or hostile: fought, Eric first.
    f.seen = true;
    f.leadX = 70;
    f.units = {Unit(1, 6906, 30), Unit(3, 6907, 35)};
    Run r2 = Start(&plan);
    CHECK(Tick(r2, f) == "pullback 3");
    // In sight and all friendly: skipped, saying why.
    for (UnitFact& u : f.units)
        u.hostile = false;
    Run r3 = Start(&plan);
    Tick(r3, f);
    CHECK(r3.step == 1 && r3.note.find("friendly") != std::string::npos);
    // In sight and nobody there: skipped (not "boss down": it was never fought).
    f.units.clear();
    Run r4 = Start(&plan);
    Tick(r4, f);
    CHECK(r4.step == 1 && r4.note == "boss is for the other team and not here: skipped");
    // The party's own boss in sight and dead: down.
    f.team = ai::kPlanFlagHorde;
    f.bossDeadNearSpot = true;
    Run r5 = Start(&plan);
    Tick(r5, f);
    CHECK(r5.step == 1 && r5.note == "boss down");
}

// Fourth observe run (Theka, ZF steps 54-56): "boss down" 2 s after arriving,
// with no fight; Theka patrols. A boss neither alive nor dead in sight is
// waited for at his spot, then the run pauses; never counted down.
static void TestBossDownNeedsASeenDeath()
{
    DungeonPlan plan;
    PlanStep boss = Step(PlanStepKind::Boss, 30);
    boss.entry = 7272;
    ai::NormalizePlanStep(boss);
    plan.steps = {boss, Step(PlanStepKind::Move, 60)};
    Run r = Start(&plan);
    Facts f = Party(0);
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 0 && r.state == State::Running && r.note.find("not at his spot") != std::string::npos);
    // He walks in on his patrol: pulled.
    for (int i = 0; i < 20; ++i)
        Tick(r, f);
    f.units = {Unit(5, 7272, 40)};
    CHECK(Tick(r, f) == "pullback 5");
    f.anyInCombat = true;
    Tick(r, f);
    f.anyInCombat = false;
    // Killed: his corpse is seen.
    f.units.clear();
    f.bossDeadNearSpot = true;
    Tick(r, f);
    CHECK(r.step == 1 && r.note == "boss down");

    // Never there: paused after kBossSearchMs, "not found".
    Run p = Start(&plan);
    Facts g = Party(0);
    for (std::uint32_t i = 0; i <= kBossSearchMs / kTickMs + 1 && p.state == State::Running; ++i)
        Tick(p, g);
    CHECK(p.state == State::Paused && p.step == 0 && p.note.find("boss 7272 not found") == 0);
}

// Fourth observe run (step 43 -> 45) and 19z5 step 57 (Sul'lithuz Hatchlings
// 8130): a pullback refused invalid-target. The unit is left out for the
// run without a pause; the pack is skipped as out of reach, and a later step
// that sees the same unit pulls the other one. A skip clears the pull state.
static void TestSkipDropsTheRefusedPullTarget()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1}), Step(PlanStepKind::Move, 20), PullStep(30, {1, 2})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(7, 1, 10)};
    CHECK(Tick(r, f) == "pullback 7");
    OrderRefused(r, "pullback 7", "invalid-target: The group cannot attack that target.");
    CHECK(r.state == State::Running && r.pullGuid == 0);
    Tick(r, f);
    CHECK(r.step == 1 && r.note.find("out of reach") != std::string::npos);
    f = Party(20);
    Tick(r, f);
    CHECK(r.step == 2);
    f.units = {Unit(7, 1, 5), Unit(8, 2, 15)};
    CHECK(Tick(r, f) == "pullback 8");
    // A skip clears the step's pull state.
    CHECK(Skip(r));
    CHECK(r.pullGuid == 0 && r.pulls == 0 && r.pullTries.empty());
}

static void TestResumeWithLeadDead()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40)};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.dead = 1;
    f.alive = 4;
    f.leadAlive = false;
    Tick(r, f);
    // The owner's wait rule: no pause, the regroup waits for the lead bot.
    CHECK(r.state == State::Running && r.regroup);
    Pause(r, "test: the old pause");
    CHECK(Resume(r));
    CHECK(r.regroup);
    CHECK(Tick(r, f) == "revive party");
    // The ghost runs back: no pause while the regroup's ceiling holds.
    for (int i = 0; i < 100; ++i)
        Tick(r, f);
    CHECK(r.state == State::Running && r.regroup);
    // Alive again, gathered: the step goes on.
    f.dead = 0;
    f.alive = 5;
    f.leadAlive = true;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    CHECK(!r.regroup);
    // A lead bot that never comes back is bounded by kRegroupReviveMs.
    Run r2 = Start(&plan);
    Facts g = Party(0);
    g.dead = 1;
    g.alive = 4;
    g.leadAlive = false;
    Tick(r2, g);
    Resume(r2);
    for (std::uint32_t i = 0; i <= kRegroupReviveMs / kTickMs + 2 && r2.state == State::Running; ++i)
        Tick(r2, g);
    CHECK(r2.state == State::Paused && r2.note.find("still dead") != std::string::npos);
}

static UnitFact Hitter(std::uint64_t guid, std::uint32_t entry, float dist, bool healer = false)
{
    UnitFact u = Unit(guid, entry, dist);
    u.hitting = true;
    u.victimHealer = healer;
    return u;
}

// Uldaman rerun 2026-10-17, the walk to Galgann at -10 337: the tank held
// "stay" with nothing on it while the party fought 13-31 yd away. With mobs
// hitting the party for kGuardWaitMs the tank picks the nearest one up
// (`attack <guid> <tank>`), and after the fight the run walks it back to the
// stand point.
static void TestTankGuardsThePartyFromItsStand()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 0), PullStep(40, {4855, 4853})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadIsTank = true;
    f.leadStaying = true;
    Tick(r, f); // on the stand point: the anchor, next step
    CHECK(r.step == 1 && r.anchorKnown);
    f.units = {Unit(31, 4853, 40)};
    CHECK(Tick(r, f) == "pullback 31");
    f.anyInCombat = true;
    // The pullback's mobs aggroed on the healer and still run in: not hitting yet.
    UnitFact running = Unit(31, 4853, 30);
    running.victimHealer = true;
    f.partyAttackers = {running};
    for (int i = 0; i < 8; ++i)
        CHECK(Tick(r, f).empty());
    // They arrive and hit the party; nothing is on the tank.
    f.partyAttackers = {Hitter(41, 7320, 35), Hitter(51, 7320, 45), Hitter(31, 4853, 30)};
    for (std::uint32_t ms = kTickMs; ms < kGuardWaitMs; ms += kTickMs)
        CHECK(Tick(r, f).empty());
    CHECK(Tick(r, f) == "attack 31 Tanky"); // the nearest one within kGuardYd
    CHECK(r.guardReturn);
    // The order ended the stay; the tank holds that mob now.
    f.leadStaying = false;
    f.leadHit = true;
    f.leadX = 28;
    f.partyAttackers = {Hitter(41, 7320, 12)};
    for (int i = 0; i < 10; ++i)
        CHECK(Tick(r, f).empty()); // a mob on a DPS while the tank holds its own: the tank's own AI
    // The fight is over: back to the stand point first, then the step goes on.
    f.anyInCombat = false;
    f.leadHit = false;
    f.partyAttackers.clear();
    f.units.clear();
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    f.leadX = 1;
    f.leadStaying = true;
    CHECK(Tick(r, f).empty()); // arrived: the next tick reads fresh facts
    CHECK(!r.guardReturn);
    Tick(r, f);
    CHECK(r.state == State::Done);
}

// Uldaman run 3 (19z12, tests/uldaman-events-3), baelog: the leg's pulls have
// no move step, so the run's anchor stayed the leg's start point; after every
// guarded fight the tank walked 90 yd back there, and on arrival the step
// paused at once on "no walkable path toward the pack" (the arrival tick's
// facts had no approach query: the glue skips it while the run walks). The
// tank walks back to its own spot at the first pick-up, the arrival ends the
// tick, and the next tick approaches the unseen pack.
static void TestTankGuardReturnsToItsOwnSpot()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 0), PullStep(150, {4851})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadIsTank = true;
    f.leadStaying = true;
    Tick(r, f); // the leg's start point: the anchor
    CHECK(r.step == 1 && r.anchorKnown && r.anchorX == 0.0f);
    // The pull goes out from 90 yd on (the pullback stand, no walk of the run's).
    f.leadX = 90;
    f.anchorPathRead = true;
    f.units = {Unit(31, 4851, 60), Unit(32, 4851, 62)};
    CHECK(Tick(r, f) == "pullback 31");
    f.anyInCombat = true;
    f.partyAttackers = {Hitter(31, 4851, 12, true)};
    CHECK(Tick(r, f) == "attack 31 Tanky");
    CHECK(r.guardReturn && r.guardX == 90.0f);
    // The tank chased its mob 15 yd; the fight is over, one of the pack is left.
    f.leadStaying = false;
    f.leadX = 105;
    f.anyInCombat = false;
    f.partyAttackers.clear();
    f.units = {Unit(32, 4851, 60)};
    CHECK(Tick(r, f) == "move 90.0 0.0 0.0"); // its own spot, not the leg's start
    // Arrived; the pack is out of sight and this tick's facts have no approach.
    f.leadX = 90;
    f.leadStaying = true;
    f.seen = false;
    f.approachKnown = false;
    CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Running && !r.guardReturn);
    f.approachKnown = true;
    f.approachX = 125;
    std::string order;
    for (int i = 0; i < 5 && order.empty() && r.state == State::Running; ++i)
        order = Tick(r, f);
    CHECK(order == "move 125.0 0.0 0.0");
    CHECK(r.state == State::Running);

    // A spot the fight took the tank far from is left: it stays near the fight.
    Run r2 = Start(&plan);
    Facts g = Party(0);
    g.leadIsTank = true;
    g.leadStaying = true;
    Tick(r2, g);
    g.leadX = 90;
    g.anchorPathRead = true;
    g.units = {Unit(31, 4851, 60)};
    CHECK(Tick(r2, g) == "pullback 31");
    g.anyInCombat = true;
    g.partyAttackers = {Hitter(31, 4851, 12, true)};
    CHECK(Tick(r2, g) == "attack 31 Tanky");
    g.leadStaying = false;
    g.leadX = 90 + kGuardReturnMaxYd + 5;
    g.anyInCombat = false;
    g.partyAttackers.clear();
    std::string const next = Tick(r2, g);
    CHECK(next != "move 90.0 0.0 0.0" && next != "move 0.0 0.0 0.0");
    CHECK(!r2.guardReturn);
    // That spot is the party's anchor now (a later gather does not walk back).
    CHECK(r2.anchorKnown && r2.anchorX == g.leadX);

    // Uldaman run 3, pull 2: the tank on stay was still dragging the pullback
    // in (18 yd short of its stand) at the first pick-up. The walk back goes
    // to its stay point, not to where the pick-up found it.
    Run r3 = Start(&plan);
    Facts h = Party(0);
    h.leadIsTank = true;
    h.leadStaying = true;
    Tick(r3, h);
    h.leadX = 90;
    h.anchorPathRead = true;
    h.units = {Unit(31, 4851, 60)};
    CHECK(Tick(r3, h) == "pullback 31");
    h.anyInCombat = true;
    h.leadX = 108; // mid-drag
    h.leadStayKnown = true;
    h.leadStayX = 90;
    h.partyAttackers = {Hitter(31, 4851, 12, true)};
    CHECK(Tick(r3, h) == "attack 31 Tanky");
    CHECK(r3.guardX == 90.0f);
    h.leadStaying = false;
    h.leadStayKnown = false;
    h.anyInCombat = false;
    h.partyAttackers.clear();
    h.units = {Unit(32, 4851, 60)};
    CHECK(Tick(r3, h) == "move 90.0 0.0 0.0");
}

// Replay of keepers door pull 100 (trace r-...9bbf): a Brawler on the tank in
// melee reach, a Geomancer casting at the mage from 30 yd, then two Maulers on
// the mage and the healer. The tank keeps its Brawler while only the mage is
// hit; a mob hitting the healer is picked up (it needs no stay once the guard
// took the tank off it), the next one after a fresh wait, and the cap is
// reached in one fight.
static void TestTankGuardPull100Replay()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 0), PullStep(40, {4855, 4853})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.leadIsTank = true;
    f.leadStaying = true;
    Tick(r, f);
    f.units = {Unit(97, 4855, 40)};
    CHECK(Tick(r, f) == "pullback 97");
    f.anyInCombat = true;
    f.leadHit = true; // the Brawler, 1 yd
    f.partyAttackers = {Hitter(98, 4853, 30)}; // the Geomancer on the mage
    for (int i = 0; i < 8; ++i)
        CHECK(Tick(r, f).empty());
    // A Mauler reaches the healer at T (trace: 7478); she is dead at T+2-3 s
    // (botwatch 73 % at 03:07:22, 0 % at 03:07:26): the order goes out on the
    // first tick she is seen hit.
    f.partyAttackers = {Hitter(98, 4853, 30), Hitter(90, 7320, 14, true)};
    CHECK(Tick(r, f) == "attack 90 Tanky");
    // Off its stay now; the second Mauler turns on the healer: picked up once
    // kGuardRepeatMs have passed since the first, and the cap is reached in
    // one fight.
    f.leadStaying = false;
    f.partyAttackers = {Hitter(98, 4853, 30), Hitter(91, 7320, 20, true)};
    CHECK(Tick(r, f).empty());
    CHECK(Tick(r, f) == "attack 91 Tanky");
    int orders = 2;
    for (int i = 0; i < 20; ++i)
        if (Tick(r, f).compare(0, 7, "attack ") == 0)
            ++orders;
    // The first order was an engage on arrival (a healer's attacker on the
    // staying tank), which the cap does not count: it and the cap's pick-ups.
    CHECK(orders == static_cast<int>(kGuardOrdersPerFight) + 1);
    CHECK(r.guardOrders == kGuardOrdersPerFight);
    CHECK(r.state == State::Running);
}

// The guard stays out of the way: no tank, a tank off its stay the guard did
// not take off, a polymorphed, evading, too far or not yet hitting mob, a
// scripted event; a refused pick-up or walk back never pauses the run.
static void TestTankGuardLimits()
{
    auto guardRun = [](DungeonPlan& plan)
    {
        plan.steps = {Step(PlanStepKind::Move, 0), PullStep(40, {4855})};
        Run r = Start(&plan);
        Facts f = Party(0);
        Tick(r, f);
        f.units = {Unit(31, 4855, 40)};
        Tick(r, f); // pullback 31
        return r;
    };
    auto fight = [](bool tank, bool staying, bool hit)
    {
        Facts f = Party(0);
        f.anyInCombat = true;
        f.leadIsTank = tank;
        f.leadStaying = staying;
        f.leadHit = hit;
        f.partyAttackers = {Hitter(31, 4855, 20)};
        return f;
    };
    auto ordersIn = [](Run& r, const Facts& f, int ticks)
    {
        int n = 0;
        for (int i = 0; i < ticks; ++i)
            if (Tick(r, f).compare(0, 7, "attack ") == 0)
                ++n;
        return n;
    };
    {
        DungeonPlan plan;
        Run r = guardRun(plan);
        CHECK(ordersIn(r, fight(true, true, true), 8) == 0);   // the tank holds a mob; the victim is no healer
        CHECK(ordersIn(r, fight(false, true, false), 8) == 0); // the lead bot is no tank
        CHECK(ordersIn(r, fight(true, false, false), 8) == 0); // off its stay by itself: its own AI goes already
        Facts f = fight(true, true, false);
        f.partyAttackers[0].polymorphed = true;
        CHECK(ordersIn(r, f, 8) == 0);
        f.partyAttackers[0].polymorphed = false;
        f.partyAttackers[0].evading = true;
        CHECK(ordersIn(r, f, 8) == 0);
        f.partyAttackers[0].evading = false;
        f.partyAttackers[0].hitting = false;
        CHECK(ordersIn(r, f, 8) == 0);
        f.partyAttackers[0].hitting = true;
        f.partyAttackers[0].distLead = kGuardYd + 1;
        CHECK(ordersIn(r, f, 8) == 0);
        f.partyAttackers[0].distLead = 20;
        CHECK(ordersIn(r, f, 5) == 1);
        OrderRefused(r, "attack 31 Tanky", "failed: That bot did not accept the attack.");
        CHECK(r.state == State::Running);
    }
    {
        // Inside a scripted event but not in its hold (a step leading into it):
        // nothing (the hold's own guard: tools/test_dungeon_event_policy.cpp).
        DungeonPlan plan;
        Run r = guardRun(plan);
        r.eventOn = true;
        CHECK(ordersIn(r, fight(true, true, false), 8) == 0);
    }
    {
        // A refused walk back to the stand point goes on from where it stands.
        DungeonPlan plan;
        Run r = guardRun(plan);
        CHECK(ordersIn(r, fight(true, true, false), 5) == 1);
        Facts after = Party(25);
        CHECK(Tick(r, after) == "move 0.0 0.0 0.0");
        OrderRefused(r, "move 0.0 0.0 0.0", "unreachable: The lead bot has no walkable path to that point.");
        CHECK(r.state == State::Running && !r.guardReturn);
    }
}

// Uldaman Pause 2 (scorpid_cave step 38, 2026-10-20): a Scorpid had a pet as its
// victim 15 yd from the tank and never came; the guard saw only mobs on bots,
// so the tank stood on "stay" and the fight ran past the 180 s ceiling. A mob
// on a party pet is now picked up too, but after every mob on a bot, never while
// the tank holds a mob of its own, and never from beyond kGuardYd of the tank
// (a pet's attacker beside a sleeping pack must not pull the tank across).
static UnitFact PetHitter(std::uint64_t guid, std::uint32_t entry, float dist)
{
    UnitFact u = Hitter(guid, entry, dist);
    u.victimPet = true;
    return u;
}

static void TestTankGuardPicksUpMobsOnPets()
{
    auto guardRun = [](DungeonPlan& plan)
    {
        plan.steps = {Step(PlanStepKind::Move, 0), PullStep(40, {4855})};
        Run r = Start(&plan);
        Facts f = Party(0);
        Tick(r, f);
        f.units = {Unit(31, 4855, 40)};
        Tick(r, f); // pullback 31
        return r;
    };
    auto fight = [](std::vector<UnitFact> const& attackers, bool hit = false)
    {
        Facts f = Party(0);
        f.anyInCombat = true;
        f.leadIsTank = true;
        f.leadStaying = true;
        f.leadHit = hit;
        f.partyAttackers = attackers;
        return f;
    };
    std::string orderNote; // the run's note on the tick that sent the order
    auto ordersIn = [&orderNote](Run& r, const Facts& f, int ticks)
    {
        std::string last;
        int n = 0;
        for (int i = 0; i < ticks; ++i)
        {
            std::string const o = Tick(r, f);
            if (o.compare(0, 7, "attack ") == 0)
            {
                ++n;
                last = o;
                orderNote = r.note;
            }
        }
        return std::make_pair(n, last);
    };
    {
        // The Pause 2 case: only a pet has a mob on it, 15 yd away. After the
        // guard's wait the tank is sent at it, and the note says "off a pet".
        DungeonPlan plan;
        Run r = guardRun(plan);
        auto const res = ordersIn(r, fight({PetHitter(31, 4863, 15)}), 5);
        CHECK(res.first == 1 && res.second == "attack 31 Tanky");
        CHECK(orderNote.find("off a pet") != std::string::npos);
        CHECK(r.guardReturn);
    }
    {
        // Bounded by kGuardYd from the tank: a pet's attacker at the edge of a
        // sleeping pack, beyond the bound, draws nothing; just inside it does.
        DungeonPlan plan;
        Run r = guardRun(plan);
        CHECK(ordersIn(r, fight({PetHitter(31, 4863, kGuardYd + 1)}), 10).first == 0);
        CHECK(ordersIn(r, fight({PetHitter(31, 4863, kGuardYd - 1)}), 5).first == 1);
    }
    {
        // A mob on a bot goes first, though the pet's attacker is nearer; a pet's
        // attacker only when no mob on a bot qualifies (here: one is not hitting yet).
        DungeonPlan plan;
        Run r = guardRun(plan);
        auto const res = ordersIn(r, fight({PetHitter(31, 4863, 8), Hitter(41, 7320, 30)}), 5);
        CHECK(res.first == 1 && res.second == "attack 41 Tanky");
        CHECK(orderNote.find("off a pet") == std::string::npos);
        DungeonPlan plan2;
        Run r2 = guardRun(plan2);
        UnitFact running = Unit(41, 7320, 30);
        auto const res2 = ordersIn(r2, fight({PetHitter(31, 4863, 8), running}), 5);
        CHECK(res2.first == 1 && res2.second == "attack 31 Tanky");
    }
    {
        // The tank holds a mob of its own: it leaves a pet's attacker to its AI.
        DungeonPlan plan;
        Run r = guardRun(plan);
        CHECK(ordersIn(r, fight({PetHitter(31, 4863, 15)}, true), 10).first == 0);
    }
    {
        // Not hitting (still running in), evading or polymorphed: no pick-up.
        DungeonPlan plan;
        Run r = guardRun(plan);
        UnitFact u = PetHitter(31, 4863, 15);
        u.hitting = false;
        CHECK(ordersIn(r, fight({u}), 10).first == 0);
        u.hitting = true;
        u.evading = true;
        CHECK(ordersIn(r, fight({u}), 10).first == 0);
        u.evading = false;
        u.polymorphed = true;
        CHECK(ordersIn(r, fight({u}), 10).first == 0);
    }
}

// Engage on arrival (PLAN-THREAT-265 M1a). The tank on "stay" at a stand fights
// nothing (an attack action is a movement action): RFC 19z19 step 25 had it idle
// 21 s with a Trogg on the mage and the rogue 2-4 yd away, Uldaman 19z19 step 85
// three mobs on it with no target for 14 s. So a mob on the tank in melee reach
// (Facts::leadAttacker), or one hitting the healer, sends it at once
// (`attack <guid> <tank>`, which ends the stay), without the guard's wait and
// without using up a pick-up of the fight (kGuardOrdersPerFight).
static Run EngageRun(DungeonPlan& plan)
{
    plan.steps = {Step(PlanStepKind::Move, 0), PullStep(40, {4855})};
    Run r = Start(&plan);
    Facts f = Party(0);
    Tick(r, f); // on the stand point: the anchor, next step
    f.units = {Unit(31, 4855, 40)};
    Tick(r, f); // pullback 31
    return r;
}

static Facts EngageFight(bool staying = true)
{
    Facts f = Party(0);
    f.anyInCombat = true;
    f.leadIsTank = true;
    f.leadStaying = staying;
    return f;
}

// A living mob on the lead bot in melee reach (what the glue reads for leadHit).
static void OnLead(Facts& f, std::uint64_t guid, std::uint32_t entry, float dist)
{
    f.leadHit = true;
    f.leadAttacker = Hitter(guid, entry, dist);
    f.leadHasAttacker = true;
}

static int OrdersIn(Run& r, const Facts& f, int ticks)
{
    int n = 0;
    for (int i = 0; i < ticks; ++i)
        if (Tick(r, f).compare(0, 7, "attack ") == 0)
            ++n;
    return n;
}

// T1: the first tick sends the tank at the mob on it; no pick-up is used.
static void TestEngageT1()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    OnLead(f, 60, 11517, 2.0f);
    CHECK(Tick(r, f) == "attack 60 Tanky");
    CHECK(r.guardOrders == 0 && r.engageGuid == 60);
    CHECK(r.note.find("the tank engages 11517 on arrival") != std::string::npos);
    CHECK(r.guardReturn);
}

// T2: the same mob gets no second order, however long its fight lasts. In a
// live run the order ends the stay at once; the stay stays on only when the bot
// holds the attack (stunned, feared: HoldsAttackUntilControl), and then the
// order must not be repeated every tick.
static void TestEngageT2()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    OnLead(f, 60, 11517, 2.0f);
    CHECK(Tick(r, f) == "attack 60 Tanky");
    CHECK(OrdersIn(r, f, 12) == 0);
    CHECK(r.guardOrders == 0);
}

// T3: a polymorphed or evading mob is not engaged.
static void TestEngageT3()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    OnLead(f, 60, 11517, 2.0f);
    f.leadAttacker.polymorphed = true;
    CHECK(OrdersIn(r, f, 6) == 0);
    f.leadAttacker.polymorphed = false;
    f.leadAttacker.evading = true;
    CHECK(OrdersIn(r, f, 6) == 0);
    CHECK(r.engageGuid == 0);
    f.leadAttacker.evading = false;
    CHECK(Tick(r, f) == "attack 60 Tanky"); // the same mob once it is neither
}

// T3b: the same for the healer's attacker.
static void TestEngageT3b()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    f.partyAttackers = {Hitter(41, 7320, 12, true)};
    f.partyAttackers[0].polymorphed = true;
    CHECK(OrdersIn(r, f, 3) == 0 && r.engageGuid == 0);
}

// T4: the healer's attacker first, then the mob on the tank.
static void TestEngageT4()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    OnLead(f, 60, 11517, 2.0f);
    f.partyAttackers = {Hitter(41, 7320, 20, true), Hitter(42, 7320, 12, true)};
    CHECK(Tick(r, f) == "attack 42 Tanky"); // the nearer of the healer's
    CHECK(r.guardOrders == 0 && r.engageGuid == 42);
// Off its stay now; the guard keeps watching (guardOrders is still 0):
// the other mob on the healer is picked up, a guard order this time.
    f.leadStaying = false;
    f.partyAttackers = {Hitter(41, 7320, 20, true)};
    CHECK(Tick(r, f).empty()); // kGuardRepeatMs after the engage, as after a pick-up
    CHECK(Tick(r, f) == "attack 41 Tanky");
    CHECK(r.guardOrders == 1);
}

// T5: only a DPS's attacker: no engage; the guard's wait rule fires as before.
static void TestEngageT5()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    f.partyAttackers = {Hitter(31, 4855, 20)};
    for (std::uint32_t ms = kTickMs; ms < kGuardWaitMs; ms += kTickMs)
        CHECK(Tick(r, f).empty());
    CHECK(r.engageGuid == 0 && r.guardOrders == 0);
    CHECK(Tick(r, f) == "attack 31 Tanky");
    CHECK(r.guardOrders == 1 && r.engageGuid == 0);
// ... and with a mob on the tank as well, the tank takes that one and
// not the DPS's.
    DungeonPlan plan2;
    Run r2 = EngageRun(plan2);
    Facts g = EngageFight();
    OnLead(g, 60, 11517, 2.0f);
    g.partyAttackers = {Hitter(31, 4855, 20)};
    CHECK(Tick(r2, g) == "attack 60 Tanky");
}

// T6: a tank that is not staying (already engaged) gets no order; nor does a
// lead bot that is no tank, or a dead one.
static void TestEngageT6()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight(false);
    OnLead(f, 60, 11517, 2.0f);
    f.partyAttackers = {Hitter(41, 7320, 20, true)};
    CHECK(OrdersIn(r, f, 8) == 0);
    Facts g = EngageFight();
    OnLead(g, 60, 11517, 2.0f);
    g.leadIsTank = false;
    CHECK(OrdersIn(r, g, 8) == 0);
    Facts h = EngageFight();
    OnLead(h, 60, 11517, 2.0f);
    h.leadAlive = false;
    CHECK(OrdersIn(r, h, 8) == 0);
    CHECK(r.engageGuid == 0);
}

// T7: inside an event (a hold, its ritual, the moves into it): no engage.
// The hold's own engage (HoldEngageTarget) is the event policy's.
static void TestEngageT7()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    OnLead(f, 60, 11517, 2.0f);
    CHECK(EngageTarget(r, f) != nullptr);
    r.eventOn = true;
    CHECK(EngageTarget(r, f) == nullptr);
    CHECK(OrdersIn(r, f, 8) == 0);
    CHECK(r.engageGuid == 0);
}

// T8: after the fight the guard return walks the tank back to its stand.
static void TestEngageT8()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    f.leadX = 6; // dragging the pullback in, short of its stand
    f.leadStayKnown = true;
    f.leadStayX = 0;
    OnLead(f, 60, 11517, 2.0f);
    CHECK(Tick(r, f) == "attack 60 Tanky");
    CHECK(r.guardReturn && r.guardX == 0.0f);
    f.leadStaying = false;
    f.leadStayKnown = false;
    f.leadX = 14; // chased it
    CHECK(OrdersIn(r, f, 5) == 0);
    f.anyInCombat = false;
    f.leadHit = false;
    f.leadHasAttacker = false;
    f.units.clear();
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    f.leadX = 1;
    f.leadStaying = true;
    CHECK(Tick(r, f).empty());
    CHECK(!r.guardReturn);
// The next fight starts over: the same mob guid is engaged again.
    CHECK(r.engageGuid == 0);
}

// T9: the attack is held (the stay stays on) and the nearest mob changes every
// tick: the order is not sent every tick. The fight's first engage is at once,
// a further one only kGuardRepeatMs after the last order, and three at most.
static void TestEngageT9()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    std::vector<int> at;
    for (int i = 0; i < 16; ++i)
    {
        Facts f = EngageFight();
        OnLead(f, i % 2 == 0 ? 60 : 61, 11517, 2.0f);
        std::string const order = Tick(r, f);
        if (order.compare(0, 7, "attack ") == 0)
            at.push_back(i);
    }
    CHECK(at.size() == kGuardOrdersPerFight);
    CHECK(at[0] == 0);
    for (std::size_t i = 1; i < at.size(); ++i)
        CHECK(at[i] - at[i - 1] >= static_cast<int>(kGuardRepeatMs / kTickMs));
    CHECK(r.guardOrders == 0 && r.engageOrders == kGuardOrdersPerFight);
}

// T10: the end of the fight starts the engage over: the next fight, with the
// tank off its stay, is not watched by the guard on account of the old engage
// (no pick-up of a DPS's attacker, no spacing), and its first engage is at once
// again, the cap spent or not.
static void TestEngageT10()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    for (int i = 0; i < 8; ++i)
    {
        Facts f = EngageFight();
        OnLead(f, 60 + i % 2, 11517, 2.0f);
        Tick(r, f);
    }
    CHECK(r.engageOrders == kGuardOrdersPerFight && r.engageGuid != 0);
    // The fight is over: back to the stand, the step goes on.
    Facts over = EngageFight(false);
    over.anyInCombat = false;
    over.leadX = 14; // chased it
    CHECK(Tick(r, over) == "move 0.0 0.0 0.0");
    over.leadX = 1;
    over.leadStaying = true;
    Tick(r, over);
    CHECK(r.engageGuid == 0 && r.engageOrders == 0 && !r.guardReturn);
    // A new fight, the tank off its stay with a DPS being hit: its own AI; the
    // guard does not keep watching for the old engage.
    Facts next = EngageFight(false);
    next.partyAttackers = {Hitter(31, 4855, 20)};
    CHECK(OrdersIn(r, next, 8) == 0);
    // A new fight with the tank on its stay: the engage is at once again.
    Run r2 = r;
    Facts stay = EngageFight();
    OnLead(stay, 70, 11517, 2.0f);
    CHECK(Tick(r2, stay) == "attack 70 Tanky");
}

// T11: one clock for both kinds of order. After a pick-up (attack held, the
// stay still on) an engage waits kGuardRepeatMs too, and after an engage a
// pick-up candidate inside kGuardRepeatMs waits (the existing repeat rule).
static void TestEngageT11()
{
    {
        DungeonPlan plan;
        Run r = EngageRun(plan);
        Facts f = EngageFight();
        f.partyAttackers = {Hitter(31, 4855, 20)};
        for (std::uint32_t ms = kTickMs; ms < kGuardWaitMs; ms += kTickMs)
            CHECK(Tick(r, f).empty());
        CHECK(Tick(r, f) == "attack 31 Tanky"); // a pick-up; the attack is held, the stay stays on
        CHECK(r.guardOrders == 1 && r.engageOrders == 0);
        Facts g = EngageFight();
        OnLead(g, 60, 11517, 2.0f); // an engage target appears on the next tick
        CHECK(Tick(r, g).empty());  // 1 s after the pick-up
        CHECK(Tick(r, g) == "attack 60 Tanky"); // kGuardRepeatMs after it, exactly one
        CHECK(r.guardOrders == 1 && r.engageOrders == 1);
        CHECK(OrdersIn(r, g, 6) == 0);
    }
    {
        // The mirror: an engage, then a pick-up candidate (a mob on the healer)
        // inside kGuardRepeatMs; the stay is off (the order took effect).
        DungeonPlan plan;
        Run r = EngageRun(plan);
        Facts f = EngageFight();
        OnLead(f, 60, 11517, 2.0f);
        CHECK(Tick(r, f) == "attack 60 Tanky");
        f.leadStaying = false;
        f.partyAttackers = {Hitter(41, 7320, 12, true)};
        CHECK(Tick(r, f).empty()); // 1 s after the engage
        CHECK(Tick(r, f) == "attack 41 Tanky");
        CHECK(r.guardOrders == 1 && r.engageOrders == 1);
    }
    {
        // ... and with the stay still on (held attack), the same candidate is
        // an engage target: it waits the same 2 s, then one order.
        DungeonPlan plan;
        Run r = EngageRun(plan);
        Facts f = EngageFight();
        OnLead(f, 60, 11517, 2.0f);
        CHECK(Tick(r, f) == "attack 60 Tanky");
        f.partyAttackers = {Hitter(41, 7320, 12, true)};
        CHECK(Tick(r, f).empty());
        CHECK(Tick(r, f) == "attack 41 Tanky");
        CHECK(r.guardOrders == 0 && r.engageOrders == 2);
    }
}

// R1: Ragefire Chasm 19z19, Oggleflint pull (step 25), t=862-875. Oggleflint is
// on Tanky from 862 while he stands on "stay" with no target; a Ragefire Trogg
// (be05) hits the mage Gera 4 yd away and later the rogue. The old guard sent
// nothing until 885 (a mob on the healer, and the tank held one in reach, so the
// DPS's Trogg was left alone). Now the tank is sent at Oggleflint on the first
// tick, and it keeps its mob: the Trogg on the mage draws no second order.
static void TestEngageReplayRfcOggleflint()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    OnLead(f, 161, 11517, 1.5f);                  // Oggleflint on Tanky
    f.partyAttackers = {Hitter(48645, 11318, 4.0f)}; // the Trogg (be05) on Gera
    CHECK(Tick(r, f) == "attack 161 Tanky");
    CHECK(r.guardOrders == 0);
    f.leadStaying = false; // the order ended the stay; 863-875 go on like that
    for (int i = 0; i < 13; ++i)
        CHECK(Tick(r, f).empty());
    CHECK(r.guardOrders == 0 && r.state == State::Running);
}

// R2: Uldaman 19z19, step 85 (the firing stand for 4853), t=2613-2632. Three
// mobs are on Cedrias (staying, no target from 2616 to 2630); Katrien's Rain of
// Fire wakes more. The tank is sent at the nearest mob on it on the first tick
// (old: no order until 2632, for the healer), and from 2632 the guard still
// picks the healer's attacker up (a pick-up, not an engage, off the stay).
static void TestEngageReplayUldamanStep85()
{
    DungeonPlan plan;
    Run r = EngageRun(plan);
    Facts f = EngageFight();
    OnLead(f, 8201, 4853, 2.4f); // the nearest of the three; the glue reports one
    f.partyAttackers = {Hitter(8101, 4855, 9.0f), Hitter(8103, 4855, 11.0f)}; // on Katrien and the hunter
    CHECK(Tick(r, f) == "attack 8201 Tanky");
    CHECK(r.guardOrders == 0 && r.engageGuid == 8201);
    f.leadStaying = false;
    for (int i = 0; i < 18; ++i) // 2614-2631
        CHECK(Tick(r, f).empty());
    f.partyAttackers.push_back(Hitter(8204, 4853, 16.0f, true)); // 2632: 4853 on the priest
    CHECK(Tick(r, f) == "attack 8204 Tanky");
    CHECK(r.guardOrders == 1);
}

// Uldaman run 3 (19z12, 10:45-10:54): the priest and the mage revived at the
// entrance and walked ~270 yd back through the Shrike Bats (4 extra deaths);
// the regroup paused at 60 s with a bot 273.7 yd out. A revived bot far from
// the lead bot is summoned to it on its first tick alive and out of combat,
// before any gather walks it, once per revive.
static Run DeathsInAWalk(const DungeonPlan& plan, Facts& f, std::vector<std::string> dead)
{
    Run r = Start(&plan);
    f = Party(0);
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    f.dead = static_cast<std::uint32_t>(dead.size());
    f.alive = 5 - f.dead;
    f.deadNames = dead;
    f.members = {Mem("Tanky", 0)};
    CHECK(Tick(r, f).empty()); // the wait rule starts the regroup
    CHECK(r.regroup);
    CHECK(Tick(r, f) == "revive party");
    return r;
}

static void TestRevivedBotSummonedBeforeWalking()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), PullStep(60, {1})};
    Facts f;
    Run r = DeathsInAWalk(plan, f, {"Quelinator", "Tobbert"});
    // The priest is up at the entrance, the mage still a ghost: she is summoned now.
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Tobbert"};
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 270)};
    f.maxMemberYd = 270;
    CHECK(Tick(r, f) == "summon Quelinator");
    CHECK(r.note.find("summoned to it instead of walking back") != std::string::npos);
    // The mage up too: summoned on his first tick alive.
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 270), Mem("Tobbert", 268)};
    CHECK(Tick(r, f) == "summon Tobbert");
    // No gather walks them while their summons are on the way.
    for (int i = 0; i < 3; ++i)
        CHECK(Tick(r, f).empty());
    // Both at the lead bot: the regroup ends, no walk back through the dungeon.
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 3), Mem("Tobbert", 4)};
    f.maxMemberYd = 4;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    CHECK(!r.regroup && r.waitFor.empty() && r.reviveSummons.empty());
    // A bot revived near the lead bot (a resurrection in the party) needs none.
    Run n = DeathsInAWalk(plan, f, {"Quelinator"});
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 30)};
    f.maxMemberYd = 30;
    CHECK(Tick(n, f) == "move 0.0 0.0 0.0"); // the gather, no summon
}

static void TestNoReviveSummonInCombat()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), PullStep(60, {1})};
    Facts f;
    Run r = DeathsInAWalk(plan, f, {"Quelinator"});
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 200)};
    f.maxMemberYd = 200;
    // In combat on the way (the bats): never summoned (`cmd summon` refuses it).
    f.members[1].inCombat = true;
    f.anyInCombat = true;
    for (int i = 0; i < 5; ++i)
        CHECK(Tick(r, f).find("summon") == std::string::npos);
    // A member flagged in combat while the party reads out of combat: not either.
    f.anyInCombat = false;
    for (int i = 0; i < 3; ++i)
        CHECK(Tick(r, f).find("summon") == std::string::npos);
    // With attackers it runs back to the party (RunBack, #1054), no summon.
    f.members[1].inCombat = false;
    f.members[1].attackers = 1;
    f.members[1].yd = 100;
    f.maxMemberYd = 100;
    CHECK(Tick(r, f).find("summon") == std::string::npos);
    CHECK(!r.runBack.empty());
    // Its attackers gone, the run back goes on until it ends (here: it stalls);
    // then, out of combat and far, it is summoned.
    f.members[1].attackers = 0;
    std::string order;
    for (std::uint32_t ms = 0; ms < kWalkRetryMs + 3000 && order.empty(); ms += kTickMs)
    {
        bool const running = !r.runBack.empty();
        order = Tick(r, f);
        CHECK(!running || order.empty() || !r.runBack.empty() || order == "summon Quelinator");
    }
    CHECK(order == "summon Quelinator" && r.runBack.empty());
}

static void TestOneReviveSummonPerRevive()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), PullStep(60, {1})};
    Facts f;
    Run r = DeathsInAWalk(plan, f, {"Quelinator"});
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 270)};
    f.maxMemberYd = 270;
    int summons = 0;
    for (int i = 0; i < 30; ++i)
    {
        f.members[1].x += (i % 2 ? 3.0f : -3.0f); // walking: no stand-still summon either
        if (Tick(r, f) == "summon Quelinator")
            ++summons;
    }
    CHECK(summons == 1);
    // She dies again on the way and is revived: her new revive gets its summon.
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Quelinator"};
    f.members = {Mem("Tanky", 0)};
    f.maxMemberYd = 0;
    std::string order;
    for (int i = 0; i < 3 && order != "revive party"; ++i)
        order = Tick(r, f);
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 270)};
    f.maxMemberYd = 270;
    order.clear();
    for (int i = 0; i < 3 && order.empty(); ++i)
        order = Tick(r, f);
    CHECK(order == "summon Quelinator");
}

static void TestFailedReviveSummonWalks()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), PullStep(60, {1})};
    Facts f;
    Run r = DeathsInAWalk(plan, f, {"Quelinator"});
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 270)};
    f.maxMemberYd = 270;
    CHECK(Tick(r, f) == "summon Quelinator");
    // Refused (in combat by the time it arrived): she walks with the gather.
    OrderRefused(r, "summon Quelinator", "unavailable: That bot is dead, in combat, on a taxi or teleporting.");
    CHECK(r.state == State::Running && r.reviveSummons.empty() && r.note.find("it walks back") != std::string::npos);
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    // Acknowledged but she never came: after kReviveSummonWaitMs she walks.
    Run t = DeathsInAWalk(plan, f, {"Quelinator"});
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 270)};
    f.maxMemberYd = 270;
    CHECK(Tick(t, f) == "summon Quelinator");
    std::string order;
    std::uint32_t ms = 0;
    for (; ms <= kReviveSummonWaitMs + kTickMs && order.empty(); ms += kTickMs)
        order = Tick(t, f);
    CHECK(order == "move 0.0 0.0 0.0" && ms >= kReviveSummonWaitMs && t.reviveSummons.empty());
}

static void TestRegroupGatherCapScales()
{
    CHECK(RegroupGatherCap(Party(0)) == kRegroupGatherMs);
    Facts far = Party(0);
    far.maxMemberYd = 273.7f;
    CHECK(RegroupGatherCap(far) > kRegroupGatherMs && RegroupGatherCap(far) == 91233u);
    far.maxMemberYd = 2000.0f;
    CHECK(RegroupGatherCap(far) == kRegroupGatherMaxMs);
    // A walker 273.7 yd out (its summon failed) is not paused on at 60 s.
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), PullStep(60, {1})};
    Facts f;
    Run r = DeathsInAWalk(plan, f, {"Quelinator"});
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 273.7f)};
    f.maxMemberYd = 273.7f;
    CHECK(Tick(r, f) == "summon Quelinator");
    OrderRefused(r, "summon Quelinator", "unavailable: in combat");
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    std::uint32_t ms = 0;
    for (; r.state == State::Running && ms < kRegroupGatherMaxMs; ms += kTickMs)
    {
        f.members[1].x += (ms / kTickMs % 2 ? 3.0f : -3.0f);
        CHECK(Tick(r, f).find("summon") == std::string::npos);
    }
    CHECK(r.state == State::Paused && ms > kRegroupGatherMs + 20000 && r.note.find("within 91 s") != std::string::npos);
    // A bot whose summon is on its way does not count against the ceiling.
    Run p = DeathsInAWalk(plan, f, {"Quelinator"});
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.members = {Mem("Tanky", 0), Mem("Quelinator", 50)};
    f.maxMemberYd = 50;
    p.regroupGatherSent = true;
    p.regroupMs = kRegroupGatherMs + 1000;
    CHECK(Tick(p, f) == "summon Quelinator");
    for (int i = 0; i < 5; ++i)
        CHECK(Tick(p, f).empty());
    CHECK(p.state == State::Running);
}

static void TestEmptyPlan()
{
    DungeonPlan plan;
    Run r = Start(&plan);
    CHECK(r.state == State::Done);
    Facts f = Party(0);
    CHECK(Tick(r, f).empty());
}

// (a) Uldaman 19z11 01:59-02:04: the revived priest walked part of the way
// (more than kStragglerGainYd since the gather began), then stood at the
// scorpid pit's navmesh hole (-163.5 298.7). The regroup's ceiling skipped
// her and paused. A bot that stands still now is summoned, once per regroup;
// then one more gather window, then the pause.
static void TestStuckStragglerThatMovedIsSummoned()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    BeginRegroup(r);
    Facts f = Party(0);
    f.maxMemberYd = 110;
    f.members = {Mem("Tanky", 0), Mem("Fyran", 110)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the regroup's gather: first sample at 110 yd
    // She walks 30 yd, then stands at the hole, 80 yd out.
    f.members = {Mem("Tanky", 0), Mem("Fyran", 80)};
    f.maxMemberYd = 80;
    std::string order;
    for (std::uint32_t i = 0; i < kRegroupGatherMs / kTickMs + 2 && order.empty(); ++i)
        order = Tick(r, f);
    CHECK(order == "summon Fyran");
    CHECK(r.state == State::Running && r.regroup && r.note.find("has not moved") != std::string::npos);
    // Bounded: one summon per bot per regroup; the next ceiling pauses.
    order.clear();
    for (std::uint32_t i = 0; i < kRegroupGatherMs / kTickMs + 2 && r.state == State::Running; ++i)
        CHECK(Tick(r, f).find("summon") == std::string::npos);
    CHECK(r.state == State::Paused && r.note.find("did not gather") != std::string::npos);
    // A bot still on its way (moving every tick) is not summoned.
    Run w = Start(&plan);
    BeginRegroup(w);
    Facts g = Party(0);
    g.maxMemberYd = 110;
    g.members = {Mem("Tanky", 0), Mem("Walky", 110)};
    Tick(w, g);
    g.members[1].yd = 80;
    for (std::uint32_t i = 0; i < kRegroupGatherMs / kTickMs + 2 && w.state == State::Running; ++i)
    {
        g.members[1].x += (i % 2 ? 3.0f : -3.0f);
        CHECK(Tick(w, g).find("summon") == std::string::npos);
    }
    CHECK(w.state == State::Paused);
}

// Zul'Farrak r3-19z20 (PLAN-ZF-ZUMRAH-WALKBACK B-FIX-1/2): after the wipe the
// walk back pulled a Shadowhunter 7246 that stood on a ~3 x 3 yd navmesh island
// at 1588-1591 1044-1047; the tank chased it onto the island and from there
// every walk-back point was refused (no path) or an approach hop of zero length:
// 23 points passed in 13 minutes, 8 hops at the step, a pause, and a resume that
// did the same again. The slice: steps 63-67 of the plan's route and the
// Zum'rah step after them.
static DungeonPlan ZfIslandPlan()
{
    DungeonPlan plan;
    PlanStep pull = PullStep(1903, {7271});
    pull.y = 1014;
    pull.z = 11.8f;
    plan.steps = {Step(PlanStepKind::Move, 1603, 1043, 9.8f), Step(PlanStepKind::Move, 1584, 1050, 9.0f),
        Step(PlanStepKind::Move, 1588, 1080, 9.1f), Step(PlanStepKind::Move, 1584, 1088, 9.1f),
        Step(PlanStepKind::Move, 1633.6f, 1130.7f, 9.9f), pull};
    return plan;
}

// The lead bot on the island: the glue's path toward the point is partial and
// ends 1.1 yd from it (its approach hop with it).
static Facts OnIsland()
{
    Facts f = Party(0);
    f.leadX = 1589.6f;
    f.leadY = 1046.9f;
    f.leadZ = 8.9f;
    f.seen = false;
    f.approachKnown = true;
    f.approachX = 1590.7f;
    f.approachY = 1046.7f;
    f.approachZ = 8.9f;
    f.leadIsland = true;
    f.goodSpotCut = true; // the second sign: the main mesh a few yards off cannot be reached
    return f;
}

static void TestLeadOnIslandReading()
{
    CHECK(LeadOnIsland(true, 1.1f, 315.1f));             // partial, ends at the lead bot
    CHECK(LeadOnIsland(true, 3.05f, 315.1f));            // ... or across the island (B5: step 65, 1591.2 1044.3)
    CHECK(LeadOnIsland(true, kGoodPathMinYd, 315.1f));
    CHECK(!LeadOnIsland(true, kGoodPathMinYd + 0.1f, 315.1f)); // partial and gets somewhere: the length cap
    CHECK(!LeadOnIsland(false, 1.1f, 315.1f));           // complete
    CHECK(!LeadOnIsland(true, -1.0f, 315.1f));           // not measured
    CHECK(!LeadOnIsland(true, 1.1f, kArriveYd));         // at the point already
    CHECK(!LeadOnIsland(true, 6.0f, 9.0f));              // the path ends at the point: not short of it
    CHECK(LeadPathGood(false, false, 0.5f));
    CHECK(LeadPathGood(false, true, kGoodPathMinYd + 0.1f));
    CHECK(!LeadPathGood(false, true, kGoodPathMinYd));
    CHECK(!LeadPathGood(false, true, 3.05f));            // the 3.05 yd path across the island is no good path
    CHECK(!LeadPathGood(true, false, 50.0f));
    CHECK(kGoodPathMinYd > kIslandSizeYd + kArriveYd);
    // The ring search: the path must lead on from the spot AND from the lead bot
    // (a spot off the mesh snaps onto the island: its path ends at the lead bot).
    CHECK(RingSpotLeadsOn(false, true, kGoodPathMinYd + 1, kGoodPathMinYd + 1));
    CHECK(!RingSpotLeadsOn(false, true, kGoodPathMinYd + 1, 2.0f));
    CHECK(!RingSpotLeadsOn(false, true, 2.0f, kGoodPathMinYd + 1));
    CHECK(!RingSpotLeadsOn(true, false, 50.0f, 50.0f));
    // A complete path cannot start from a spot that snapped onto the island:
    // accepted, its end 5 yd away (the point is near) or not.
    CHECK(RingSpotLeadsOn(false, false, 5.0f, 5.0f));
    // The last good spot is probed for the second sign only off the lead bot, near, on its level.
    CHECK(GoodSpotProbeWanted(6.0f, 0.0f) && GoodSpotProbeWanted(kGoodSpotProbeYd, -kEscapeMaxVertical));
    CHECK(!GoodSpotProbeWanted(kArriveYd, 0.0f) && !GoodSpotProbeWanted(kGoodSpotProbeYd + 1, 0.0f) &&
        !GoodSpotProbeWanted(6.0f, kEscapeMaxVertical + 1));
    // The last good spot as the hop's end: not on the lead bot, within reach.
    CHECK(GoodSpotFits(1589.6f, 1046.9f, 8.9f, 1584.8f, 1040.2f, 8.9f));
    CHECK(!GoodSpotFits(1589.6f, 1046.9f, 8.9f, 1590.6f, 1046.9f, 8.9f)); // 1 yd: the island itself
    CHECK(!GoodSpotFits(0, 0, 0, kEscapeMaxYd + 0.5f, 0, 0));
    CHECK(!GoodSpotFits(0, 0, 0, 5, 0, kEscapeMaxVertical + 0.5f));
}

static void TestWalkBackEscapesNavmeshIsland()
{
    DungeonPlan plan = ZfIslandPlan();
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 4;
    // On the main mesh (where the forward run's tank stood): the hop toward
    // step 5's point is a real one, and this spot is the last good one.
    Facts f = Party(0);
    f.leadX = 1584.8f;
    f.leadY = 1040.2f;
    f.leadZ = 8.9f;
    f.seen = false;
    f.approachKnown = true;
    f.approachX = 1600;
    f.approachY = 1080;
    f.approachZ = 9;
    f.leadPathGood = true;
    CHECK(Tick(r, f) == "move 1600.0 1080.0 9.0");
    CHECK(r.goodLeadKnown && r.goodLeadX == 1584.8f && r.goodLeadY == 1040.2f && r.goodLeadZ == 8.9f);
    // A fight on the way took the tank onto the island.
    f.anyInCombat = true;
    Tick(r, f);
    Facts g = OnIsland();
    CHECK(Tick(r, g) == "escape from 1589.6 1046.9 8.9 to 1584.8 1040.2 8.9 toward 1633.6 1130.7 9.9");
    CHECK(r.note.find("navmesh island at 1589.6 1046.9 8.9") != std::string::npos);
    CHECK(r.note.find("(1 of 2)") != std::string::npos && r.note.find("on to the next") == std::string::npos);
    CHECK(r.state == State::Running && r.walkBackIdx == 4 && r.approachHops == 1); // no hop counted for it
    // The hop is under way: nothing else goes out.
    g.leadMoving = true;
    CHECK(Tick(r, g).empty());
    CHECK(r.note.find("hops off the navmesh island") != std::string::npos);
    // At the good spot, standing: the hop is over; the walk back goes on.
    f.anyInCombat = false;
    CHECK(Tick(r, f).empty());
    CHECK(!r.escapeOn && r.islandEscapes == 0);
    CHECK(Tick(r, f) == "move 1600.0 1080.0 9.0");
    CHECK(r.note == "walk back: approaching the point of step 5" && r.walkBackIdx == 4);
}

static void TestIslandPausesAfterTwoEscapes()
{
    DungeonPlan plan = ZfIslandPlan();
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 4;
    // A good spot 19.6 yd away is not hopped to straight: the glue looks around the lead bot.
    r.goodLeadKnown = true;
    r.goodLeadX = 1570;
    r.goodLeadY = 1046.9f;
    r.goodLeadZ = 8.9f;
    Facts g = OnIsland();
    CHECK(Tick(r, g) == "escape from 1589.6 1046.9 8.9 toward 1633.6 1130.7 9.9");
    CHECK(Tick(r, g).empty());
    CHECK(Tick(r, g).empty()); // the lead bot stands still: the hop is over
    // Still on the island: the second hop.
    CHECK(Tick(r, g) == "escape from 1589.6 1046.9 8.9 toward 1633.6 1130.7 9.9");
    CHECK(r.note.find("(2 of 2)") != std::string::npos);
    CHECK(Tick(r, g).empty());
    CHECK(Tick(r, g).empty());
    CHECK(Tick(r, g).empty());
    CHECK(r.state == State::Paused);
    CHECK(r.note.find("the lead bot stands on a navmesh island at 1589.6 1046.9 8.9: no path toward the point of step 5") == 0);
    CHECK(r.walkBackIdx == 4); // no point passed
}

static void TestResumeEscapesFirst()
{
    DungeonPlan plan = ZfIslandPlan();
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 4;
    Facts g = OnIsland();
    for (int i = 0; i < 20 && r.state == State::Running; ++i)
        Tick(r, g);
    CHECK(r.state == State::Paused && r.note.find("navmesh island") != std::string::npos);
    // `.bot run resume`: the regroup on the lead bot, the walk back from the
    // same spot, and its first order is the hop off the island, not a hop.
    CHECK(Resume(r));
    g.members = {Mem("Tanky", 0), Mem("Healy", 4)};
    g.maxMemberYd = 4;
    CHECK(Tick(r, g).empty());
    CHECK(r.walkBack && r.walkBackIdx == 4 && !r.regroup);
    CHECK(Tick(r, g) == "escape from 1589.6 1046.9 8.9 toward 1633.6 1130.7 9.9");
    CHECK(r.islandEscapes == 1 && r.state == State::Running);
}

static void TestWalkBackPointInSightOnIsland()
{
    DungeonPlan plan = ZfIslandPlan();
    // Step 3's point is in sight: no hop is read, the glue reads the lead bot's path to it.
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = 2;
    Facts g = OnIsland();
    g.seen = true;
    g.approachKnown = false;
    CHECK(Tick(r, g) == "escape from 1589.6 1046.9 8.9 toward 1588.0 1080.0 9.1");

    // No island read (the backstop): two points refused from one spot with no
    // path there that got anywhere; the second is not passed, the lead bot is hopped off first.
    Run q = Start(&plan, 5);
    q.walkBack = true;
    q.walkBackIdx = 1;
    Facts h = g;
    h.leadIsland = false;
    std::string const unreachable =
        "unreachable: The lead bot has no walkable path to that point (no navmesh route reaches it).";
    CHECK(Tick(q, h) == "move 1584.0 1050.0 9.0");
    OrderRefused(q, "move 1584.0 1050.0 9.0", unreachable);
    CHECK(q.walkBackIdx == 2 && q.note.find("on to the next") != std::string::npos);
    CHECK(Tick(q, h) == "move 1588.0 1080.0 9.1");
    OrderRefused(q, "move 1588.0 1080.0 9.1", unreachable);
    CHECK(q.state == State::Running && q.walkBackIdx == 2 && q.islandSuspect);
    CHECK(Tick(q, h) == "escape from 1589.6 1046.9 8.9 toward 1588.0 1080.0 9.1");

    // A spot whose path read got somewhere: refused points are passed as before.
    Run o = Start(&plan, 5);
    o.walkBack = true;
    o.walkBackIdx = 1;
    Facts k = h;
    k.leadPathGood = true;
    CHECK(Tick(o, k) == "move 1584.0 1050.0 9.0");
    OrderRefused(o, "move 1584.0 1050.0 9.0", "off-mesh: The point is not on walkable ground.");
    CHECK(Tick(o, k) == "move 1588.0 1080.0 9.1");
    OrderRefused(o, "move 1588.0 1080.0 9.1", "off-mesh: The point is not on walkable ground.");
    CHECK(o.walkBackIdx == 3 && !o.islandSuspect && o.note.find("on to the next") != std::string::npos);

    // Out of sight, a hop that ends where the lead bot stands (no island read):
    // no path toward the point at once, not a hop counted (r3-19z20: 8 per point).
    Run w = Start(&plan, 5);
    w.walkBack = true;
    w.walkBackIdx = w.walkBackStart = 4;
    w.walkBackRestarted = true;
    Facts z = OnIsland();
    z.leadIsland = false;
    CHECK(Tick(w, z).empty());
    CHECK(w.approachHops == 0 && w.walkBackIdx == 5);
    CHECK(w.note == "walk back: no walkable path toward the point of step 5: on to the next");
}

// The step's own approach (r3-19z20 step 102: 8 hops of zero length, then the
// pause 315.1 yd out): on the island it hops off first; a hop that ends where
// the lead bot stands is no path at once, not 8 hops.
static void TestApproachHopIslandAndZeroHop()
{
    DungeonPlan plan = ZfIslandPlan();
    Run r = Start(&plan, 5);
    r.restDue = false;
    Facts g = OnIsland();
    CHECK(Tick(r, g) == "escape from 1589.6 1046.9 8.9 toward 1903.0 1014.0 11.8");
    CHECK(r.approachHops == 0 && r.note.find("no path toward the pack") != std::string::npos);

    Run z = Start(&plan, 5);
    z.restDue = false;
    Facts h = OnIsland();
    h.leadIsland = false;
    CHECK(Tick(z, h).empty());
    CHECK(z.state == State::Paused && z.note.find("no walkable path toward the pack") == 0);
}


// Cold review on bcc23124 (Opus M1): from the frozen spot the path to step 65
// ended at 1591.2 1044.3, 3.05 yd from the lead bot: over the old 2 yd island
// line and the old 3 yd "good path" line, so the island spot became the last
// good one and each good read reset the escape count (hop, escape, hop ...).
// With the real numbers it now reads as island and resets nothing.
static void TestIslandGapIsNotAGoodPath()
{
    float const lx = 1589.6f, ly = 1046.9f, lz = 8.9f;
    float const end = ai::PlanDistance(lx, ly, lz, 1591.2f, 1044.3f, 8.9f);
    float const toStep65 = ai::PlanDistance(lx, ly, lz, 1588, 1080, 9.1f);
    CHECK(end > 3.0f && end < 3.1f);
    DungeonPlan plan = ZfIslandPlan();
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 4;
    r.goodLeadKnown = true;
    r.goodLeadX = 1584.8f;
    r.goodLeadY = 1040.2f;
    r.goodLeadZ = 8.9f;
    Facts g = OnIsland();
    g.approachX = 1591.2f;
    g.approachY = 1044.3f;
    g.leadIsland = LeadOnIsland(true, end, toStep65);
    g.leadPathGood = LeadPathGood(false, true, end);
    CHECK(g.leadIsland && !g.leadPathGood);
    CHECK(Tick(r, g) == "escape from 1589.6 1046.9 8.9 to 1584.8 1040.2 8.9 toward 1633.6 1130.7 9.9");
    for (int i = 0; i < 2; ++i)
        CHECK(Tick(r, g).empty());
    CHECK(r.islandEscapes == 1 && r.goodLeadX == 1584.8f && r.goodLeadY == 1040.2f); // not overwritten, not reset
    CHECK(Tick(r, g).compare(0, 7, "escape ") == 0);
    CHECK(r.islandEscapes == 2);
    for (int i = 0; i < 3; ++i)
        Tick(r, g);
    CHECK(r.state == State::Paused && r.note.find("navmesh island at 1589.6 1046.9 8.9") != std::string::npos);
    // A last good spot on the lead bot itself (within kArriveYd) is not hopped to.
    Run n = Start(&plan, 5);
    n.walkBack = true;
    n.walkBackIdx = n.walkBackStart = 4;
    n.goodLeadKnown = true;
    n.goodLeadX = 1590.6f;
    n.goodLeadY = 1046.9f;
    n.goodLeadZ = 8.9f;
    CHECK(Tick(n, OnIsland()) == "escape from 1589.6 1046.9 8.9 toward 1633.6 1130.7 9.9");
}

// Codex on bcc23124: a hop refused because a fight started or the lead bot
// moved since the read ("wait: ...") is not a try; "no-spot" is.
static void TestEscapeWaitIsNotATry()
{
    DungeonPlan plan = ZfIslandPlan();
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 4;
    Facts g = OnIsland();
    std::string const order = Tick(r, g);
    CHECK(order.compare(0, 7, "escape ") == 0 && r.islandEscapes == 1);
    OrderRefused(r, order, "wait: a party bot is in combat");
    CHECK(r.islandEscapes == 0 && r.stepEscapes == 0 && !r.escapeOn && r.state == State::Running &&
        r.note.find("not counted") != std::string::npos);
    CHECK(Tick(r, g) == order);
    OrderRefused(r, order, "no-spot: no spot within 10 yd");
    CHECK(r.islandEscapes == 1 && r.note.find("not counted") == std::string::npos);
}

// Codex on bcc23124: the straight line is checked before the lead bot moves.
static void TestEscapeSegmentCheck()
{
    std::vector<SegmentSample> s = EscapeSegmentSamples(0, 0, 0, 10, 0, 0);
    CHECK(s.size() == 5 && s.back().x == 10.0f && s.front().x == 2.0f);
    CHECK(EscapeSegmentSamples(0, 0, 0, 30, 0, 0).size() == kSegmentSamples);
    CHECK(EscapeSegmentSamples(0, 0, 0, 0.5f, 0, 0).size() == 1);
    auto flat = [&]()
    {
        std::vector<SegmentSample> v = EscapeSegmentSamples(0, 0, 0, 10, 0, 0);
        for (SegmentSample& x : v)
        {
            x.ground = true;
            x.groundZ = 0.0f;
        }
        return v;
    };
    CHECK(CheckEscapeSegment(0, flat(), false) == SegmentVerdict::Ok);
    CHECK(CheckEscapeSegment(0, flat(), true) == SegmentVerdict::Hostile);
    CHECK(CheckEscapeSegment(0, {}, false) == SegmentVerdict::NoGround);
    std::vector<SegmentSample> v = flat();
    v[2].ground = false;
    CHECK(CheckEscapeSegment(0, v, false) == SegmentVerdict::NoGround);
    v = flat();
    v[2].groundZ = -(kSegmentGroundYd + 0.5f); // a floor far under the line (a drop)
    CHECK(CheckEscapeSegment(0, v, false) == SegmentVerdict::NoGround);
    v = flat();
    v[2].groundZ = kSegmentRiseYd + 0.5f;
    CHECK(CheckEscapeSegment(0, v, false) == SegmentVerdict::Step);
    v = flat();
    v[0].groundZ = kSegmentRiseYd + 0.5f; // the first sample against the lead bot's own height
    CHECK(CheckEscapeSegment(0, v, false) == SegmentVerdict::Step);
    v = flat();
    v[3].liquid = true;
    CHECK(CheckEscapeSegment(0, v, false) == SegmentVerdict::Liquid);
    // A ramp of 1.9 yd per sample up a line to 9.5 yd: no step, too far up in all.
    v = EscapeSegmentSamples(0, 0, 0, 10, 0, 9.5f);
    for (SegmentSample& x : v)
    {
        x.ground = true;
        x.groundZ = x.z;
    }
    CHECK(CheckEscapeSegment(0, v, false) == SegmentVerdict::Climb);
    v.pop_back();
    CHECK(CheckEscapeSegment(0, v, false) == SegmentVerdict::Ok);
    CHECK(std::string(SegmentVerdictName(SegmentVerdict::Liquid)) == "liquid on the line");
}

// Opus notes on bcc23124: the island never hops a lead bot off a direct step's
// point (it may stand at its launch spot), a direct point passed clears the
// suspicion, and so does a point reached.
static void TestIslandLeavesDirectStepsAlone()
{
    DungeonPlan plan;
    PlanStep pull = PullStep(500, {1});
    plan.steps = {Step(PlanStepKind::Move, 10), Step(PlanStepKind::MoveDirect, 200), Step(PlanStepKind::Move, 220), pull};
    Run r = Start(&plan, 3);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 1;
    r.islandSuspect = true;
    Facts f = Party(0);
    f.seen = false;
    f.approachKnown = true;
    f.approachX = 40;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    // A crossed direct point refused: passed, and the suspicion with it.
    Run d = Start(&plan, 3);
    d.walkBack = true;
    d.walkBackIdx = 1;
    d.islandSuspect = true;
    d.routeKnown = true;
    d.checkpoint = 1;
    OrderRefused(d, "move 200.0 0.0 0.0 direct", "no-los: no line of sight");
    CHECK(d.walkBackIdx == 2 && !d.islandSuspect);
    // A point reached clears it too (the lead bot stands on it: no hop off it).
    Run q = Start(&plan, 3);
    q.walkBack = true;
    q.walkBackIdx = 0;
    q.islandSuspect = true;
    Facts a = Party(10);
    CHECK(Tick(q, a).empty());
    CHECK(q.walkBackIdx == 1 && !q.islandSuspect);
}


// Codex/Opus on 269503e0: one partial path toward one point does not prove an
// island (an unreachable ledge, a closed door, a point off the mesh). From the
// main mesh with no second sign, the walk back and the approach hop do what
// they did before (2012dfee): no escape order.
static void TestSingleIslandReadingChangesNothing()
{
    DungeonPlan plan;
    PlanStep pull = PullStep(300, {1});
    plan.steps = {Step(PlanStepKind::Move, 40), Step(PlanStepKind::Move, 200), pull};
    // A ledge point in sight: the path toward it ends 5 yd out, short of it.
    Run r = Start(&plan, 2);
    r.walkBack = true;
    r.walkBackIdx = 0;
    Facts f = Party(0);
    f.leadIsland = LeadOnIsland(true, 5.0f, 40.0f);
    CHECK(f.leadIsland && !f.goodSpotCut);
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    OrderRefused(r, "move 40.0 0.0 0.0", "unreachable: no navmesh route reaches it");
    CHECK(r.walkBackIdx == 1 && r.note.find("on to the next") != std::string::npos);
    // The next point out of sight: the hop is walked (5 yd), as before.
    Facts g = Party(0);
    g.seen = false;
    g.approachKnown = true;
    g.approachX = 5;
    g.leadIsland = LeadOnIsland(true, 5.0f, 200.0f);
    // ... read toward a point 160 yd from the first one from the same spot: that
    // is the second sign (b), so this check uses another spot (another cell).
    g.leadX = f.leadX = 20;
    g.approachX = 25;
    Run w = Start(&plan, 2);
    w.walkBack = true;
    w.walkBackIdx = 1;
    CHECK(Tick(w, g) == "move 25.0 0.0 0.0");
    // The approach hop of a pull step: the hop, not an escape.
    Run a = Start(&plan, 2);
    a.restDue = false;
    Facts h = Party(0);
    h.seen = false;
    h.approachKnown = true;
    h.approachX = 5;
    h.leadIsland = LeadOnIsland(true, 5.0f, 300.0f);
    CHECK(Tick(a, h) == "move 5.0 0.0 0.0");
    // ... and a zero-length hop with one reading only: no path, the pause (B-FIX-2), not an escape.
    Run z = Start(&plan, 2);
    z.restDue = false;
    h.approachX = 1;
    h.leadIsland = LeadOnIsland(true, 1.0f, 300.0f);
    CHECK(Tick(z, h).empty());
    CHECK(z.state == State::Paused && z.note.find("no walkable path toward the pack") == 0);
}

// The second sign (b) with the trace's numbers: from 1589.6 1046.9 the path to
// step 65 (1588 1080) ended 3.05 yd away, and the one to step 70 (1633.6 1130.7)
// came back partial too. The first reading changes nothing (the move goes out,
// is refused and passed); the second, toward a point 58 yd from the first, hops
// the lead bot off. Without the good spot probe (no good spot known yet).
static void TestSecondReadingConfirmsTheIsland()
{
    DungeonPlan plan;
    PlanStep pull = PullStep(1903, {7271});
    pull.y = 1014;
    pull.z = 11.8f;
    plan.steps = {Step(PlanStepKind::Move, 1603, 1043, 9.8f), Step(PlanStepKind::Move, 1588, 1080, 9.1f),
        Step(PlanStepKind::Move, 1633.6f, 1130.7f, 9.9f), pull};
    Run r = Start(&plan, 3);
    r.walkBack = true;
    r.walkBackIdx = 1;
    Facts g = OnIsland();
    g.goodSpotCut = false;
    g.seen = true;
    g.approachKnown = false;
    g.leadIsland = LeadOnIsland(true, 3.05f, ai::PlanDistance(1589.6f, 1046.9f, 8.9f, 1588, 1080, 9.1f));
    CHECK(Tick(r, g) == "move 1588.0 1080.0 9.1");
    CHECK(r.islandEvidence);
    OrderRefused(r, "move 1588.0 1080.0 9.1", "unreachable: no navmesh route reaches it");
    CHECK(r.walkBackIdx == 2);
    Facts h = OnIsland();
    h.goodSpotCut = false;
    h.approachX = 1591.2f;
    h.approachY = 1044.3f;
    h.leadIsland = LeadOnIsland(true, 3.05f, ai::PlanDistance(1589.6f, 1046.9f, 8.9f, 1633.6f, 1130.7f, 9.9f));
    CHECK(Tick(r, h) == "escape from 1589.6 1046.9 8.9 toward 1633.6 1130.7 9.9");
    // The same point read again is no second sign.
    Run s = Start(&plan, 3);
    s.walkBack = true;
    s.walkBackIdx = 2;
    s.walkBackStart = 2;
    s.walkBackRestarted = true;
    Facts k = h;
    k.approachX = 1600; // a hop that goes somewhere
    k.approachY = 1060;
    CHECK(Tick(s, k) == "move 1600.0 1060.0 8.9");
    EnterPhase(s, Phase::Begin); // the hop did not move it: the same point is read again
    CHECK(Tick(s, k) == "move 1600.0 1060.0 8.9");
    CHECK(s.islandEvidence && s.state == State::Running);
    // The evidence goes with the spot: the lead bot moved out of the cell, and a
    // reading there toward the other point is a first one again.
    Run m = Start(&plan, 3);
    m.walkBack = true;
    m.walkBackIdx = 1;
    CHECK(Tick(m, g) == "move 1588.0 1080.0 9.1");
    OrderRefused(m, "move 1588.0 1080.0 9.1", "unreachable: no navmesh route reaches it");
    Facts moved = h;
    moved.leadX = 1580.0f; // 9.6 yd west: another cell
    moved.approachX = 1590;
    moved.approachY = 1060;
    CHECK(Tick(m, moved) == "move 1590.0 1060.0 8.9");
    CHECK(m.evidenceX == 1580.0f);
    Facts away = moved; // no reading on this tick: the old cell's evidence is gone all the same
    away.leadIsland = false;
    away.leadX = 1570.0f;
    Run e = Start(&plan, 3);
    e.walkBack = true;
    e.walkBackIdx = 1;
    CHECK(Tick(e, g) == "move 1588.0 1080.0 9.1");
    CHECK(e.islandEvidence);
    EnterPhase(e, Phase::Begin);
    Tick(e, away);
    CHECK(!e.islandEvidence);
    // In the same cell but 2.7 yd from the first reading's spot: no second sign.
    Run n = Start(&plan, 3);
    n.walkBack = true;
    n.walkBackIdx = 1;
    CHECK(Tick(n, g) == "move 1588.0 1080.0 9.1");
    OrderRefused(n, "move 1588.0 1080.0 9.1", "unreachable: no navmesh route reaches it");
    Facts near = h;
    near.leadY = 1044.2f;
    near.approachX = 1600;
    near.approachY = 1060;
    CHECK(IslandCell(near.leadY) == IslandCell(1046.9f));
    CHECK(Tick(n, near) == "move 1600.0 1060.0 8.9");
    // That reading from 2.7 yd off does not replace the first: back on the first
    // spot, the reading toward the other point is the second sign.
    CHECK(n.evidenceY == 1046.9f);
    EnterPhase(n, Phase::Begin);
    Facts back = h;
    back.approachX = 1600;
    back.approachY = 1060;
    n.walkBackIdx = 2;
    CHECK(Tick(n, back).compare(0, 7, "escape ") == 0);
    // ... and a good read clears it.
    Run c = Start(&plan, 3);
    c.walkBack = true;
    c.walkBackIdx = 1;
    CHECK(Tick(c, g) == "move 1588.0 1080.0 9.1");
    Facts good = g;
    good.leadIsland = false;
    good.leadPathGood = true;
    Tick(c, good);
    CHECK(!c.islandEvidence);
}

// Opus on 269503e0: a walk back's hop that found no spot does not pause the
// run from an unchanged spot: the walk back goes on as before (the point passed).
static void TestWalkBackNoSpotPassesThePoint()
{
    DungeonPlan plan = ZfIslandPlan();
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 4;
    r.walkBackRestarted = true;
    Facts g = OnIsland();
    std::string const order = Tick(r, g);
    CHECK(order.compare(0, 7, "escape ") == 0);
    OrderRefused(r, order, "no-spot: no spot within 10 yd");
    CHECK(r.noSpotKnown && r.state == State::Running);
    // Same spot: no second hop; the zero hop is no path and the point is passed.
    CHECK(Tick(r, g).empty());
    CHECK(r.state == State::Running && r.walkBackIdx == 5 && r.note.find("on to the next") != std::string::npos);
    CHECK(r.islandEscapes == 1);
    // Off that spot (another island read 5 yd away): the mark is gone, the hop goes out again.
    Run q = Start(&plan, 5);
    q.walkBack = true;
    q.walkBackIdx = q.walkBackStart = 4;
    q.walkBackRestarted = true;
    std::string const first = Tick(q, g);
    OrderRefused(q, first, "no-spot: no spot within 10 yd");
    Facts there = g;
    there.leadX += 5.0f;
    CHECK(Tick(q, there).compare(0, 7, "escape ") == 0);
    // The mark goes once the lead bot leaves the spot, even when it comes back there.
    Run b = Start(&plan, 5);
    b.walkBack = true;
    b.walkBackIdx = b.walkBackStart = 4;
    b.walkBackRestarted = true;
    std::string const hop = Tick(b, g);
    OrderRefused(b, hop, "no-spot: no spot within 10 yd");
    Facts off = g;
    off.leadIsland = false;
    off.leadX += 5.0f;
    off.approachX = off.leadX + 20; // a hop elsewhere, not walked
    Tick(b, off);
    CHECK(!b.noSpotKnown);
    EnterPhase(b, Phase::Begin);
    CHECK(Tick(b, g).compare(0, 7, "escape ") == 0);
}

// A good read between the hops gives fresh tries, but one step sends at most kStepEscapes.
static void TestStepEscapeCap()
{
    DungeonPlan plan = ZfIslandPlan();
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 4;
    Facts g = OnIsland();
    Facts good = g;
    good.leadIsland = false;
    good.leadPathGood = true;
    good.approachX = 1600;
    good.approachY = 1080;
    std::uint32_t escapes = 0;
    for (int i = 0; i < 40 && r.state == State::Running; ++i)
    {
        std::string const order = Tick(r, g);
        if (order.compare(0, 7, "escape ") == 0)
        {
            ++escapes;
            Tick(r, g);
            Tick(r, g);   // the hop settles
            Tick(r, good); // a good read in between: fresh tries ...
            EnterPhase(r, Phase::Begin);
        }
    }
    CHECK(escapes == kStepEscapes);
    CHECK(r.state == State::Paused && r.note.find("navmesh island") != std::string::npos);
}


// Final Opus check on d7e94c3b: the probe. After a hop that found no spot, the
// walk back from that spot looped: a point refused from it set the island
// suspicion instead of being passed, the no-spot mark cleared it, and the move
// went out again every tick (3000 ticks at walk-back index 1). The driver here
// answers every hop "no-spot" and every move "unreachable", as from the island.
static void TestNoSpotWalkBackNeverSpins()
{
    DungeonPlan plan;
    PlanStep pull = PullStep(1903, {7271});
    pull.y = 1014;
    pull.z = 11.8f;
    plan.steps = {Step(PlanStepKind::Move, 1603, 1043, 9.8f), Step(PlanStepKind::Move, 1588, 1080, 9.1f),
        Step(PlanStepKind::Move, 1584, 1088, 9.1f), Step(PlanStepKind::Move, 1633.6f, 1130.7f, 9.9f), pull};
    Run r = Start(&plan, 4);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 0;
    r.walkBackRestarted = true;
    std::uint32_t sameOrder = 0, longestRepeat = 0, moves = 0;
    std::string last;
    std::size_t lastIdx = 0;
    int endedAt = -1;
    for (int t = 0; t < 3000 && r.state == State::Running; ++t)
    {
        Facts f = OnIsland();
        const PlanStep* fs = FactStep(r);
        float const to = ai::PlanDistance(f.leadX, f.leadY, f.leadZ, fs->x, fs->y, fs->z);
        f.seen = to <= kSeeYd;
        f.approachKnown = !f.seen;
        f.leadIsland = LeadOnIsland(true, 3.05f, to);
        f.goodSpotCut = false;
        std::string const order = Tick(r, f);
        if (!r.walkBack && endedAt < 0)
            endedAt = t;
        if (order.empty())
            continue;
        if (order == last && r.walkBackIdx == lastIdx)
            longestRepeat = std::max(longestRepeat, ++sameOrder);
        else
            sameOrder = 1;
        last = order;
        lastIdx = r.walkBackIdx;
        if (order.compare(0, 7, "escape ") == 0)
            OrderRefused(r, order, "no-spot: no spot within 10 yd");
        else if (order.compare(0, 5, "move ") == 0)
        {
            ++moves;
            OrderRefused(r, order, "unreachable: no navmesh route reaches it");
        }
    }
    CHECK(endedAt >= 0 && endedAt < 200); // every point passed, the walk back is over
    CHECK(longestRepeat <= 1); // each point refused once and passed (as on 2012dfee), never sent again
    CHECK(moves <= 3);
    CHECK(r.state == State::Paused); // the step's own approach then pauses (bounded)
}

// Sign (b) from the main mesh (two unreachable points read from one spot): one
// hop off per spot, at most kStepEscapes in the step, and the run ends bounded.
static void TestSignBFromTheMainMeshIsBounded()
{
    DungeonPlan plan;
    PlanStep pull = PullStep(600, {1});
    plan.steps = {Step(PlanStepKind::Move, 40), Step(PlanStepKind::Move, 40, 30), Step(PlanStepKind::Move, 40, 60),
        Step(PlanStepKind::Move, 40, 90), pull};
    Run r = Start(&plan, 4);
    r.walkBack = true;
    r.walkBackRestarted = true;
    float leadX = 0.0f;
    std::uint32_t escapes = 0;
    std::vector<float> escapedFrom;
    for (int t = 0; t < 3000 && r.state == State::Running; ++t)
    {
        Facts f = Party(leadX);
        const PlanStep* fs = FactStep(r);
        float const to = ai::PlanDistance(f.leadX, f.leadY, f.leadZ, fs->x, fs->y, fs->z);
        f.seen = to <= kSeeYd;
        f.approachKnown = !f.seen;
        f.approachX = leadX + 5;
        f.leadIsland = LeadOnIsland(true, 5.0f, to);
        std::string const order = Tick(r, f);
        if (order.compare(0, 7, "escape ") == 0)
        {
            ++escapes;
            for (float x : escapedFrom)
                CHECK(std::fabs(x - leadX) > kArriveYd); // one hop off per spot
            escapedFrom.push_back(leadX);
            leadX -= 6.0f; // the glue moved it 6 yd: another spot, another cell
        }
        else if (order.compare(0, 5, "move ") == 0 && f.seen)
            OrderRefused(r, order, "unreachable: no navmesh route reaches it");
    }
    CHECK(escapes >= 1 && escapes <= kStepEscapes);
    CHECK(r.state == State::Paused);
}


// The defensive bound on its own: a walk-back point tried more than
// kWalkBackResends times from one spot is passed, whatever brought it back
// (here a refusal handled elsewhere that left the point standing), in sight
// (its move) or out of sight (its way found missing).
static void TestWalkBackResendBound()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), Step(PlanStepKind::Move, 200), PullStep(400, {1})};
    Run r = Start(&plan, 2);
    r.walkBack = true;
    Facts f = Party(0);
    for (std::uint32_t i = 0; i < kWalkBackResends; ++i)
    {
        CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
        EnterPhase(r, Phase::Begin);
    }
    CHECK(Tick(r, f).empty());
    CHECK(r.walkBackIdx == 1 && r.note.find("tried 6 times from this spot: on to the next") != std::string::npos);
    // Out of sight, the way found missing each time without passing (the
    // suspicion kept standing): the point is passed after kWalkBackResends.
    Run o = Start(&plan, 2);
    o.walkBack = true;
    o.walkBackIdx = 1;
    o.walkBackStart = 1;
    o.walkBackRestarted = true;
    Facts g = Party(0);
    g.seen = false;
    g.approachKnown = false;
    for (std::uint32_t i = 0; i < kWalkBackResends; ++i)
    {
        o.passSpotKnown = true; // a point given up here before, no good path: the suspicion
        o.passX = o.passY = o.passZ = 0.0f;
        o.noSpotKnown = true;   // ... and a no-spot mark that keeps it from a hop
        o.noSpotX = 50.0f;      // (not on this spot: PassWalkBackPoint does not pass)
        Tick(o, g);
        CHECK(o.walkBackIdx == 1 && o.state == State::Running);
        o.islandSuspect = false;
    }
    Tick(o, g);
    CHECK(o.walkBackIdx == 2);
}

// (b) Uldaman 19z11 02:50: the approach paused after 8 hops while the priest
// stood still on a navmesh pocket (43 371). Before the pause a far bot that
// stands still is summoned, once; a walker is not; then the pause (the lead
// bot's own way is the reason).
static void TestApproachHopSummonsBeforePause()
{
    DungeonPlan plan;
    plan.steps = {PullStep(300, {1})};
    Run r = Start(&plan);
    r.restDue = false;
    Facts f = Party(0);
    f.seen = false;
    f.approachKnown = true;
    f.approachX = 40;
    f.maxMemberYd = 15;
    f.members = {Mem("Tanky", 0), Mem("Fyran", 150), Mem("Walky", 90)};
    f.members[2].yd = 10; // close enough for the pull's gather
    f.maxMemberYd = 150;
    r.approachHops = kMaxApproachHops;
    Run::StillState st;
    st.x = 150;
    st.ms = kStillMs;
    r.still["Fyran"] = st;
    std::string const order = Tick(r, f);
    CHECK(order == "summon Fyran");
    CHECK(r.state == State::Running && r.note.find("approach: Fyran") == 0);
    CHECK(Tick(r, f).find("summon") == std::string::npos); // Walky moves: not summoned
    CHECK(r.state == State::Paused && r.note.find("approach hops") != std::string::npos);
}

// (c) Uldaman 19z11 02:48: only the lead bot died in the fight; its own
// release put it at the entrance 300 yd away while the others won. It is the
// wipe recovery: regroup, then the walk back over the plan's points. Bounded
// by kAutoRegroupsPerStep. A lead bot raised where it fell needs nothing.
static void TestLeadOnlyDeathRecovers()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10), Step(PlanStepKind::Move, 200), PullStep(220, {1})};
    Run r = Start(&plan, 2);
    Facts f = Party(210);
    f.anyInCombat = true;
    Tick(r, f); // fighting, the lead bot alive at 210
    f.leadAlive = false;
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Tanky"};
    Tick(r, f);
    CHECK(r.leadDied && r.state == State::Running);
    // Back on its feet at the entrance, the fight won.
    f.leadAlive = true;
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.anyInCombat = false;
    f.leadX = -90;
    f.maxMemberYd = 300;
    f.members = {Mem("Tanky", 0), Mem("Fyran", 305)};
    f.members[1].x = 215; // where the party won the fight, 5 yd from where the lead bot fell
    Tick(r, f);
    CHECK(r.regroup && r.walkBackDue && r.autoRegroups == 1 && r.state == State::Running);
    CHECK(r.note.find("the lead bot died and came back") == 0);
    // The living party holds; the lead bot walks back to it, nobody is
    // gathered on (or summoned to) the entrance.
    CHECK(r.hold.size() == 1 && r.hold[0] == "Fyran");
    f.seen = false;
    f.approachKnown = true;
    f.approachX = -50;
    std::string order;
    for (int i = 0; i < 3; ++i)
    {
        order = Tick(r, f);
        CHECK(order.find("summon") != 0 && order != "move -90.0 0.0 0.0");
    }
    CHECK(!r.regroup && r.walkBack && r.hold.size() == 1);
    // Back with it: the hold ends.
    f.leadX = 200;
    f.members[1].yd = 15;
    Tick(r, f);
    CHECK(r.hold.empty());
    // Bounded: past kAutoRegroupsPerStep it pauses.
    Run p = Start(&plan, 2);
    p.autoRegroups = kAutoRegroupsPerStep;
    Facts g = Party(210);
    g.anyInCombat = true;
    Tick(p, g);
    g.leadAlive = false;
    g.dead = 1;
    g.alive = 4;
    Tick(p, g);
    g.leadAlive = true;
    g.dead = 0;
    g.alive = 5;
    g.anyInCombat = false;
    g.leadX = -90;
    Tick(p, g);
    CHECK(p.state == State::Paused && p.note.find("came back") != std::string::npos);
    // Raised where it fell (a resurrection in the party): no regroup of its own.
    Run q = Start(&plan, 2);
    Facts h = Party(210);
    h.anyInCombat = true;
    Tick(q, h);
    h.leadAlive = false;
    h.dead = 1;
    h.alive = 4;
    Tick(q, h);
    h.leadAlive = true;
    h.dead = 0;
    h.alive = 5;
    h.anyInCombat = false;
    Tick(q, h);
    CHECK(!q.regroup && q.autoRegroups == 0 && !q.leadDied);
}

// The owner's rule (2026-10-17): "Party should wait if a party member dies
// until it is back." A death in a walk holds the walk; the bot alive again but
// far is waited for (running back); within kGatherYd the step goes on.
static void TestDeathWhileWalkingBlocksTheStep()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), PullStep(60, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.seen = true;
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Fyran"};
    CHECK(Tick(r, f).empty());
    CHECK(r.regroup && r.note.find("waiting for Fyran (dead)") == 0);
    // Released, ran back, alive at the entrance: still waited for.
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.maxMemberYd = 90;
    f.members = {Mem("Tanky", 0), Mem("Fyran", 90)};
    std::string order;
    for (int i = 0; i < 20; ++i)
    {
        order = Tick(r, f);
        CHECK(order.find("move 40") != 0);
    }
    CHECK(r.regroup && r.note.find("Fyran (running back)") != std::string::npos);
    // Back with the party: the walk goes on.
    f.maxMemberYd = 5;
    f.members = {Mem("Tanky", 0), Mem("Fyran", 5)};
    for (int i = 0; i < 5 && order.find("move 40") != 0; ++i)
        order = Tick(r, f);
    CHECK(order == "move 40.0 0.0 0.0" && !r.regroup && r.waitFor.empty());
}

// The lead bot dead out of a fight: the party waits for it (no pause).
static void TestLeadDeathBlocks()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(7, 1, 10)};
    f.leadAlive = false;
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Tanky"};
    for (int i = 0; i < 30; ++i)
        CHECK(Tick(r, f).find("pullback") != 0);
    CHECK(r.state == State::Running && r.regroup && r.note.find("Tanky (dead)") != std::string::npos);
}

// Events cannot be left mid-way: the wait comes after the event.
static void TestWaitRuleEventException()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 40), Step(PlanStepKind::Move, 80)};
    Run r = Start(&plan);
    r.eventOn = true;
    Facts f = Party(0);
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Fyran"};
    CHECK(Tick(r, f) == "move 40.0 0.0 0.0");
    CHECK(!r.regroup && r.waitFor.count("Fyran"));
    r.eventOn = false; // the event is over
    CHECK(Tick(r, f).empty());
    CHECK(r.regroup && r.note.find("waiting for Fyran (dead)") == 0);
}

// N1 (Uldaman 19z11: 17 of 63 pulls with a member dead): the pull itself
// holds while anyone is dead, outside an event.
static void TestNoPullWithADeadMember()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.units = {Unit(7, 1, 10)};
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Fyran"};
    std::string order;
    PullUnit(r, f, f.units[0], order);
    CHECK(order.empty() && r.note.find("no pull while a party member is dead") != std::string::npos);
    for (int i = 0; i < 40; ++i)
        CHECK(Tick(r, f).find("pullback") != 0);
    // Inside an event the pull goes out (it cannot wait).
    Run e = Start(&plan);
    e.eventOn = true;
    order.clear();
    PullUnit(e, f, f.units[0], order);
    CHECK(order == "pullback 7");
}

// N3 (Uldaman 01:43, 02:31): three dead, the two rogues still fighting. The
// regroup neither revives nor gathers nor walks while a living bot fights:
// the fight first, then the regroup.
static void TestRegroupWaitsForTheFight()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10), PullStep(100, {1})};
    Run r = Start(&plan, 1);
    Facts f = Party(100);
    f.dead = 3;
    f.alive = 2;
    f.deadNames = {"Tanky", "Fyran", "Iront"};
    f.anyInCombat = true;
    for (int i = 0; i < 30; ++i)
    {
        std::string const order = Tick(r, f);
        CHECK(order.empty() || order.find("focus") == 0 || order.find("cc ") == 0);
    }
    CHECK(r.state == State::Running && r.note.find("fighting on") != std::string::npos);
    // A regroup already going (a resume) holds in a fight too.
    Run g = Start(&plan, 1);
    BeginRegroup(g);
    Facts h = Party(100);
    h.dead = 1;
    h.alive = 4;
    h.anyInCombat = true;
    for (int i = 0; i < 30; ++i)
        CHECK(Tick(g, h).find("revive") != 0);
    h.anyInCombat = false;
    CHECK(Tick(g, h) == "revive party");
}

// A death during the regroup's gather: back to reviving.
static void TestDeathDuringRegroup()
{
    DungeonPlan plan;
    plan.steps = {PullStep(10, {1})};
    Run r = Start(&plan);
    BeginRegroup(r);
    Facts f = Party(0);
    f.maxMemberYd = 60;
    f.members = {Mem("Tanky", 0), Mem("Fyran", 60)};
    CHECK(Tick(r, f) == "move 0.0 0.0 0.0");
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Fyran"};
    f.members = {Mem("Tanky", 0)};
    f.maxMemberYd = 0;
    CHECK(Tick(r, f) == "revive party");
    CHECK(r.regroup && r.note.find("Fyran (dead)") != std::string::npos);
}

// Review of #1060: the common case, the fight ends while the lead bot is
// still a ghost. The regroup revives it; once it is up at the entrance the
// living party deep inside holds (no gather to the entrance, no summon there).
static void TestLeadDeadAfterTheFightHolds()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, 10), Step(PlanStepKind::Move, 200), PullStep(220, {1})};
    Run r = Start(&plan, 2);
    Facts f = Party(210);
    f.anyInCombat = true;
    Tick(r, f); // fighting, the lead bot alive at 210
    f.leadAlive = false;
    f.dead = 1;
    f.alive = 4;
    f.deadNames = {"Tanky"};
    Tick(r, f); // it falls in the fight
    // The fight is over, the lead bot still dead: the regroup, its hold armed.
    f.anyInCombat = false;
    f.members = {Mem("Fyran", 5)};
    f.members[0].x = 215;
    f.maxMemberYd = 0;
    Tick(r, f);
    CHECK(r.regroup && r.holdDue && r.walkBackDue && r.autoRegroups == 1 && r.hold.empty());
    CHECK(Tick(r, f) == "revive party");
    // Up at the entrance, 300 yd from where it fell.
    f.leadAlive = true;
    f.dead = 0;
    f.alive = 5;
    f.deadNames.clear();
    f.leadX = -90;
    f.members = {Mem("Tanky", 0), Mem("Fyran", 305)};
    f.members[1].x = 215;
    f.maxMemberYd = 305;
    CHECK(Tick(r, f).empty());
    CHECK(r.hold.size() == 1 && r.hold[0] == "Fyran" && !r.holdDue);
    f.seen = false;
    f.approachKnown = true;
    f.approachX = -50;
    for (int i = 0; i < 20; ++i)
    {
        std::string const order = Tick(r, f);
        CHECK(order.find("summon") != 0 && order != "move -90.0 0.0 0.0");
    }
    CHECK(r.state == State::Running && !r.regroup && r.walkBack && r.hold.size() == 1);
    // A player's order pauses the run: the hold ends (the glue lets them go).
    CHECK(PauseForPlayer(r, "follow"));
    CHECK(r.hold.empty() && !r.holdDue);
    // Raised where it fell (a resurrection in the party): no hold.
    Run q = Start(&plan, 2);
    Facts h = Party(210);
    h.anyInCombat = true;
    Tick(q, h);
    h.leadAlive = false;
    h.dead = 1;
    h.alive = 4;
    Tick(q, h);
    h.anyInCombat = false;
    h.members = {Mem("Fyran", 30)};
    h.members[0].x = 240;
    Tick(q, h);
    CHECK(q.holdDue);
    h.leadAlive = true;
    h.dead = 0;
    h.alive = 5;
    Tick(q, h);
    CHECK(q.hold.empty() && !q.holdDue);
}

// The run's own time limit and the stop note: the 3 h cap used to read as a plain stop.
static void TestRunTimeLimitAndStopNote()
{
    CHECK(kRunMaxMinutesDefault == 180);
    CHECK(!RunTimeLimitReached(0, 180));
    CHECK(!RunTimeLimitReached(180ull * 60000ull - 1, 180));
    CHECK(RunTimeLimitReached(180ull * 60000ull, 180));
    CHECK(RunTimeLimitReached(4000ull * 3600000ull, 180));
    CHECK(!RunTimeLimitReached(4000ull * 3600000ull, 0)); // 0 = no limit
    CHECK(RunTimeLimitNote(180) == "time limit reached (3h)");
    CHECK(RunTimeLimitNote(60) == "time limit reached (1h)");
    CHECK(RunTimeLimitNote(90) == "time limit reached (90 min)");

    CHECK(RunStopNote("") == "stopped by .bot run stop");
    CHECK(RunStopNote("   ") == "stopped by .bot run stop");
    CHECK(RunStopNote("time limit reached (3h)") == "stopped: time limit reached (3h)");
    CHECK(RunStopNote("a|b  c") == "stopped: a b c"); // no protocol separator, spaces squeezed
    CHECK(RunStopNote(std::string(300, 'x')).size() == std::string("stopped: ").size() + kRunStopReasonMax);
}

// `.bot run start [plan] [from ...]`.
static DungeonPlan TwoLegPlan()
{
    DungeonPlan plan;
    plan.name = "test";
    plan.map = 70;
    plan.legs.push_back(ai::PlanLeg{0, "entrance"});
    plan.legs.push_back(ai::PlanLeg{3, "Trogg_West"});
    for (std::uint8_t leg : {0, 0, 3, 3, 3})
    {
        PlanStep st = Step(PlanStepKind::Move, 1, 2, 3);
        st.leg = leg;
        plan.steps.push_back(st);
    }
    return plan;
}

static void TestStartFromParsing()
{
    StartArgs a = ParseStartArgs("");
    CHECK(a.ok && a.plan.empty() && a.from == StartFrom::Beginning);
    a = ParseStartArgs("uldaman");
    CHECK(a.ok && a.plan == "uldaman" && a.from == StartFrom::Beginning);
    a = ParseStartArgs("uldaman from 4");
    CHECK(a.ok && a.plan == "uldaman" && a.from == StartFrom::Leg && a.value == "4");
    a = ParseStartArgs("uldaman from leg trogg_west");
    CHECK(a.ok && a.from == StartFrom::Leg && a.value == "trogg_west");
    a = ParseStartArgs("uldaman from step 57");
    CHECK(a.ok && a.from == StartFrom::Step && a.value == "57");
    a = ParseStartArgs("from step 2"); // the lead bot's map names the plan
    CHECK(a.ok && a.plan.empty() && a.from == StartFrom::Step && a.value == "2");
    a = ParseStartArgs("  uldaman   from   leg   baelog ");
    CHECK(a.ok && a.plan == "uldaman" && a.value == "baelog");
    CHECK(!ParseStartArgs("uldaman from").ok);
    CHECK(!ParseStartArgs("uldaman from step").ok);
    CHECK(!ParseStartArgs("uldaman from 3 4").ok);
    CHECK(!ParseStartArgs("uldaman two from 3").ok);
    CHECK(!ParseStartArgs("uldaman fromm 3").ok);
}

static void TestStartFromResolve()
{
    DungeonPlan plan = TwoLegPlan();
    std::size_t step = 99;
    CHECK(ResolveStartStep(plan, StartFrom::Beginning, "", step) == FromResult::Ok && step == 0);
    CHECK(ResolveStartStep(plan, StartFrom::Leg, "3", step) == FromResult::Ok && step == 2); // first step of leg 3
    CHECK(ResolveStartStep(plan, StartFrom::Leg, "0", step) == FromResult::Ok && step == 0);
    CHECK(ResolveStartStep(plan, StartFrom::Leg, "trogg_west", step) == FromResult::Ok && step == 2); // name, any case
    CHECK(ResolveStartStep(plan, StartFrom::Leg, "7", step) == FromResult::NoSuchLeg);
    CHECK(ResolveStartStep(plan, StartFrom::Leg, "nowhere", step) == FromResult::NoSuchLeg);
    CHECK(ResolveStartStep(plan, StartFrom::Step, "1", step) == FromResult::Ok && step == 0);
    CHECK(ResolveStartStep(plan, StartFrom::Step, "5", step) == FromResult::Ok && step == 4);
    CHECK(ResolveStartStep(plan, StartFrom::Step, "6", step) == FromResult::NoSuchStep);
    CHECK(ResolveStartStep(plan, StartFrom::Step, "0", step) == FromResult::NoSuchStep);
    CHECK(ResolveStartStep(plan, StartFrom::Step, "x", step) == FromResult::NoSuchStep);

    // The run begins on that step, running, with the usual first-step state.
    Run r = Start(&plan, 2);
    CHECK(r.step == 2 && r.state == State::Running);
}

// The Uldaman Keepers: the ritual ends leg 10, the hold opens leg 11. A start on the hold goes back
// to the ritual, or the event never starts.
static DungeonPlan EventPlan()
{
    DungeonPlan plan;
    plan.name = "event";
    plan.map = 70;
    plan.legs.push_back(ai::PlanLeg{10, "keepers"});
    plan.legs.push_back(ai::PlanLeg{11, "archaedas"});
    struct Row { std::uint8_t leg; PlanStepKind kind; };
    for (Row row : {Row{10, PlanStepKind::Move}, Row{10, PlanStepKind::Move}, Row{10, PlanStepKind::Ritual},
                    Row{11, PlanStepKind::Hold}, Row{11, PlanStepKind::Move}, Row{11, PlanStepKind::Pull},
                    Row{11, PlanStepKind::Move}, Row{11, PlanStepKind::Open}, Row{11, PlanStepKind::Move},
                    Row{11, PlanStepKind::Hold}})
    {
        PlanStep st = Step(row.kind, 1, 2, 3);
        st.leg = row.leg;
        plan.steps.push_back(st);
    }
    return plan;
}

static void TestStartInsideEventGoesBack()
{
    DungeonPlan plan = EventPlan();
    std::size_t step = 99;
    bool back = false;
    // Leg 11 begins on the hold (step index 3): back to the ritual (index 2).
    CHECK(ResolveStartStep(plan, StartFrom::Leg, "11", step, &back) == FromResult::Ok && step == 2 && back);
    CHECK(ResolveStartStep(plan, StartFrom::Leg, "archaedas", step, &back) == FromResult::Ok && step == 2 && back);
    // A step number on the hold (step 4), and on the moves before the ritual (steps 1, 2) leading into... the ritual.
    CHECK(ResolveStartStep(plan, StartFrom::Step, "4", step, &back) == FromResult::Ok && step == 2 && back);
    // The ritual itself and the leg before it stay as asked.
    CHECK(ResolveStartStep(plan, StartFrom::Step, "3", step, &back) == FromResult::Ok && step == 2 && !back);
    CHECK(ResolveStartStep(plan, StartFrom::Leg, "10", step, &back) == FromResult::Ok && step == 0 && !back);
    // A move that leads into the second hold (index 8) goes back to the open step (index 7); the hold itself too.
    CHECK(ResolveStartStep(plan, StartFrom::Step, "9", step, &back) == FromResult::Ok && step == 7 && back);
    CHECK(ResolveStartStep(plan, StartFrom::Step, "10", step, &back) == FromResult::Ok && step == 7 && back);
    // A pull in the middle of the leg is not an event start.
    CHECK(ResolveStartStep(plan, StartFrom::Step, "6", step, &back) == FromResult::Ok && step == 5 && !back);
    // A move that leads into no hold stays put.
    CHECK(ResolveStartStep(plan, StartFrom::Step, "7", step, &back) == FromResult::Ok && step == 6 && !back);
    // A hold with no ritual/open/use_item before it (only moves, then the plan's start) stays put.
    DungeonPlan bare;
    bare.map = 70;
    for (PlanStepKind k : {PlanStepKind::Move, PlanStepKind::Hold})
        bare.steps.push_back(Step(k, 1, 2, 3));
    CHECK(EventStartStep(bare, 1) == 1 && EventStartStep(bare, 0) == 0);
    // The run that starts there begins with the ritual, not the hold.
    Run r = Start(&plan, 2);
    CHECK(Current(r)->kind == PlanStepKind::Ritual && EventHoldAhead(r) != nullptr);
}

static void TestPartialStartGuard()
{
    CHECK(CheckPartialStart(true, 5, 5) == PartialStartVerdict::Ok);
    CHECK(CheckPartialStart(false, 5, 5) == PartialStartVerdict::NotInstance);
    CHECK(CheckPartialStart(true, 5, 4) == PartialStartVerdict::PartyScattered);
    CHECK(CheckPartialStart(true, 0, 0) == PartialStartVerdict::Ok);
}


// ---- Exact waypoints (ai::kPlanFlagExact, ExactWalkTo) ----

static PlanStep ExactStep(float x, float y, float z)
{
    PlanStep s;
    s.kind = PlanStepKind::Move;
    s.x = x;
    s.y = y;
    s.z = z;
    s.flags = ai::kPlanFlagExact;
    CHECK(ai::NormalizePlanStep(s) == ai::PlanStepRefusal::None);
    return s;
}

// The hop after an exact point: a move_hop_exact row (exactLaunch).
static PlanStep DirectExact(float x, float y, float z)
{
    PlanStep s = Step(PlanStepKind::MoveDirect, x, y, z);
    s.exactLaunch = true;
    return s;
}

// The Wailing Caverns leg 10 rows (export of 2026-10-18): three path rows, the
// exact foot 2.25 yd from the row before it, the direct hop down to the
// Serpentis ramp (47 yd drop under it), then a rest.
static DungeonPlan SerpentisPlan()
{
    DungeonPlan plan;
    plan.name = "wailing_caverns";
    plan.steps = {Step(PlanStepKind::Move, -308.6239f, -32.13346f, -60.944225f),
        Step(PlanStepKind::Move, -292.80005f, -3.4667964f, -58.160522f),
        Step(PlanStepKind::Move, -292.00006f, -1.3334637f, -57.860523f),
        ExactStep(-293.3f, 0.5f, -58.0f),
        DirectExact(-287.5f, 5.0f, -63.6f),
        Step(PlanStepKind::Rest)};
    return plan;
}

static bool IsDirectOrder(const std::string& order)
{
    return order.find("direct") != std::string::npos;
}

// The Serpentis case: the lead bot stands 2.3 yd from the foot (within the plain
// move's 3 yd). Before the exact rule the foot counted as reached at once and the
// direct hop went out from there. Now: no direct order until the proof; the lead
// bot (too near for a move onto the point) walks back to the first plan point at
// least 6.5 yd behind, comes onto the foot, stands 2 s, and only then the hop.
static void TestExactSerpentisFootIsNotSkipped()
{
    DungeonPlan plan = SerpentisPlan();
    Run r = Start(&plan, 3);
    Facts f = Party(0);
    f.leadX = -293.3f;
    f.leadY = 0.5f - 2.3f;
    f.leadZ = -58.0f;
    std::string back;
    for (int i = 0; i < 6 && back.empty(); ++i)
    {
        std::string const order = Tick(r, f);
        CHECK(!IsDirectOrder(order));
        CHECK(r.step == 3 && r.state == State::Running);
        back = order;
    }
    // The rows 1 and 2 are within 6.5 yd of the foot on the ground plane: row 0.
    CHECK(back == "move -308.6 -32.1 -60.9");
    // Walking there: no order over the move.
    f.leadMoving = true;
    f.leadX = -300.0f;
    f.leadY = -15.0f;
    CHECK(Tick(r, f).empty());
    // Arrived and standing: after 3 s, onto the foot.
    f.leadMoving = false;
    f.leadX = -308.6f;
    f.leadY = -32.1f;
    f.leadZ = -60.9f;
    std::string onto;
    for (int i = 0; i < 5 && onto.empty(); ++i)
    {
        onto = Tick(r, f);
        CHECK(!IsDirectOrder(onto) && r.step == 3);
    }
    CHECK(onto == "move -293.3 0.5 -58.0");
    // On the foot but still moving: no proof.
    f.leadX = -293.2f;
    f.leadY = 0.4f;
    f.leadZ = -58.4f;
    f.leadMoving = true;
    for (int i = 0; i < 4; ++i)
    {
        CHECK(Tick(r, f).empty());
        CHECK(r.step == 3);
    }
    // Standing: three looks on one spot (2 s), then the step is done ...
    f.leadMoving = false;
    CHECK(Tick(r, f).empty() && r.step == 3);
    CHECK(Tick(r, f).empty() && r.step == 3);
    CHECK(Tick(r, f).empty());
    CHECK(r.step == 4 && r.exactProved == 3 && r.note.find("exact point") != std::string::npos);
    // ... and the direct hop goes out from the foot.
    CHECK(Tick(r, f) == "move -287.5 5.0 -63.6 direct");
}

// A lead bot that never comes onto the point: bounded, and a pause with a clear
// note; the direct step never goes out.
static void TestExactNeverProvedPauses()
{
    DungeonPlan plan = SerpentisPlan();
    Run r = Start(&plan, 3);
    Facts f = Party(0);
    f.leadX = -293.3f;
    f.leadY = 0.5f - 2.3f;
    f.leadZ = -58.0f;
    int ticks = 0;
    for (; ticks < 200 && r.state == State::Running; ++ticks)
        CHECK(!IsDirectOrder(Tick(r, f)));
    CHECK(r.state == State::Paused && r.step == 3);
    CHECK(ticks <= static_cast<int>(kExactCapMs / kTickMs) + 2);
    CHECK(r.note.find("exact point of step 4") != std::string::npos);
    CHECK(r.note.find("not proved") != std::string::npos);
    CHECK(r.note.find(".bot run resume") != std::string::npos);
}

// 1.5-3 yd away with no plan point behind it to walk back to: the lead bot is
// sent onto the point again (a re-issued move), never counted as arrived by the
// 3 yd rule, nor by the stalled arrival within 6 yd.
static void TestExactReissuesTheMoveAndIgnoresTheStallRule()
{
    DungeonPlan plan;
    plan.steps = {ExactStep(10, 0, 0), Step(PlanStepKind::MoveDirect, 15, 0, 0)};
    Run r = Start(&plan);
    Facts f = Party(12.3f);
    std::vector<std::string> orders;
    for (int i = 0; i < 8; ++i)
    {
        std::string const order = Tick(r, f);
        CHECK(!IsDirectOrder(order) && r.step == 0);
        if (!order.empty())
            orders.push_back(order);
    }
    CHECK(!orders.empty() && orders[0] == "move 10.0 0.0 0.0");
    // A stall 4.5 yd short (the plain walk's 3 s / 6 yd arrival): not done either.
    Run r2 = Start(&plan);
    Facts g = Party(0);
    CHECK(Tick(r2, g) == "move 10.0 0.0 0.0");
    g.leadX = 5.5f;
    for (int i = 0; i < 10; ++i)
    {
        CHECK(!IsDirectOrder(Tick(r2, g)));
        CHECK(r2.step == 0);
    }
}

// A direct step launched from an exact point checks the proof itself: started
// (or resumed) at the direct step with the lead bot 2.3 yd off the foot, the hop
// waits for the tighten; on the foot and still, it goes out.
static void TestDirectFromExactPointWaitsForTheProof()
{
    DungeonPlan plan = SerpentisPlan();
    Run r = Start(&plan, 4);
    Facts f = Party(0);
    f.leadX = -293.3f;
    f.leadY = 0.5f - 2.3f;
    f.leadZ = -58.0f;
    for (int i = 0; i < 6; ++i)
        CHECK(!IsDirectOrder(Tick(r, f)));
    Run r2 = Start(&plan, 4);
    f.leadY = 0.4f;
    std::string order;
    for (int i = 0; i < 6 && order.empty(); ++i)
        order = Tick(r2, f);
    CHECK(order == "move -287.5 5.0 -63.6 direct");
    // A stand-in point (detour) is never taken for the exact point nor its hop.
    Run r3 = Start(&plan, 3);
    f.leadX = -320.0f;
    f.leadY = -40.0f;
    Tick(r3, f);
    CHECK(!UseDetour(r3, -295.0f, 0.5f, -58.0f));
}

// A walk back after a regroup replays the exact point as its step does: the
// foot is not passed by the 3 yd rule, and the hop waits for its proof.
static void TestWalkBackReplaysTheExactPoint()
{
    DungeonPlan plan = SerpentisPlan();
    Run r = Start(&plan, 5);
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 3;
    Facts f = Party(0);
    f.leadX = -293.3f;
    f.leadY = 0.5f - 2.3f;
    f.leadZ = -58.0f;
    std::string back;
    for (int i = 0; i < 7 && (back.empty() || back == "rest"); ++i) // the rest step's gate asks for a rest first
    {
        back = Tick(r, f);
        CHECK(!IsDirectOrder(back) && r.walkBackIdx == 3 && r.walkBack);
    }
    CHECK(back == "move -308.6 -32.1 -60.9");
    // Skipping the foot's replay is caught by the hop's own check.
    Run r2 = Start(&plan, 5);
    r2.walkBack = true;
    r2.walkBackIdx = r2.walkBackStart = 4;
    for (int i = 0; i < 7; ++i)
        CHECK(!IsDirectOrder(Tick(r2, f)));
    // On the foot and still: the hop is replayed.
    f.leadY = 0.4f;
    Run r3 = Start(&plan, 5);
    r3.walkBack = true;
    r3.walkBackIdx = r3.walkBackStart = 3;
    std::string order;
    for (int i = 0; i < 8 && !IsDirectOrder(order); ++i)
        order = Tick(r3, f);
    CHECK(order == "move -287.5 5.0 -63.6 direct" && r3.walkBackIdx == 4);
}

// Brings a Serpentis run to the hop (step 4) with the foot proved; the lead
// bot stands on the foot.
static Run ProvedAtTheFoot(const DungeonPlan& plan, Facts& f)
{
    Run r = Start(&plan, 3);
    f = Party(0);
    f.leadX = -293.2f;
    f.leadY = 0.4f;
    f.leadZ = -58.4f;
    for (int i = 0; i < 6 && r.step == 3; ++i)
        CHECK(Tick(r, f).empty());
    CHECK(r.step == 4 && r.exactProved == 3);
    return r;
}

// Proved, then the lead bot is 3 yd off the foot before the hop goes out (a
// knock, a player's move): no direct order; the tighten runs again.
static void TestHopRechecksTheLeadOnTheFoot()
{
    DungeonPlan plan = SerpentisPlan();
    Facts f;
    Run r = ProvedAtTheFoot(plan, f);
    f.leadY = 0.5f - 3.0f;
    f.leadX = -293.3f;
    f.leadZ = -58.0f;
    for (int i = 0; i < 4; ++i)
        CHECK(!IsDirectOrder(Tick(r, f)));
    CHECK(r.step == 4 && r.exactProved != 3);
}

// A lead bot that drifts 1 yd a tick while staying within 1.5 yd of the point,
// with the moving fact false, is never proved.
static void TestDriftWithinTheRadiusIsNoProof()
{
    DungeonPlan plan = SerpentisPlan();
    Run r = Start(&plan, 3);
    Facts f = Party(0);
    f.leadX = -293.3f;
    f.leadZ = -58.0f;
    for (int i = 0; i < 10; ++i)
    {
        f.leadY = (i % 2 == 0) ? 0.0f : 1.0f; // 0.5 yd either side of the foot, 1 yd per tick
        CHECK(!IsDirectOrder(Tick(r, f)));
        CHECK(r.step == 3);
    }
    // ... nor one that swings 0.9 yd a tick about one spot (each look within
    // 0.5 yd of it: only the tick-to-tick move shows it) ...
    Run r2 = Start(&plan, 3);
    for (int i = 0; i < 10; ++i)
    {
        f.leadY = 0.5f + ((i == 0) ? 0.0f : (i % 2 ? 0.45f : -0.45f));
        CHECK(!IsDirectOrder(Tick(r2, f)));
        CHECK(r2.step == 3);
    }
    // ... nor one that creeps 0.4 yd a tick across the point (each tick under
    // 0.5 yd: only the distance from the first spot shows it).
    Run r3 = Start(&plan, 3);
    for (int i = 0; i < 7; ++i)
    {
        f.leadY = 0.5f - 1.2f + 0.4f * static_cast<float>(i);
        CHECK(!IsDirectOrder(Tick(r3, f)));
        CHECK(r3.step == 3);
    }
}

// After a fight, and after a pause and resume, the lead bot still on the foot:
// the proof is taken again (three looks) before the hop goes out.
static void TestProofIsRetakenAfterAFightAndAResume()
{
    DungeonPlan plan = SerpentisPlan();
    Facts f;
    Run r = ProvedAtTheFoot(plan, f);
    f.anyInCombat = true;
    Tick(r, f);
    f.anyInCombat = false;
    int quiet = 0;
    std::string order;
    for (int i = 0; i < 8 && !IsDirectOrder(order); ++i)
    {
        order = Tick(r, f);
        if (!IsDirectOrder(order))
            ++quiet;
    }
    CHECK(order == "move -287.5 5.0 -63.6 direct");
    CHECK(quiet >= 3);

    Run r2 = ProvedAtTheFoot(plan, f);
    Pause(r2, "by hand");
    CHECK(Resume(r2));
    quiet = 0;
    order.clear();
    for (int i = 0; i < 12 && !IsDirectOrder(order); ++i)
    {
        order = Tick(r2, f);
        if (!IsDirectOrder(order))
            ++quiet;
    }
    CHECK(order == "move -287.5 5.0 -63.6 direct");
    CHECK(quiet >= 3);
}

// The walk back to a point behind an exact point never goes back over a direct
// step: with a hole behind, the lead bot is sent onto the point instead.
static void TestBackPointStopsAtADirectStep()
{
    DungeonPlan plan;
    plan.steps = {Step(PlanStepKind::Move, -30, 0, 0), Step(PlanStepKind::MoveDirect, -8, 0, 0),
        Step(PlanStepKind::Move, -2, 0, 0), ExactStep(0, 0, 0), DirectExact(5, 5, -5)};
    for (int which = 0; which < 2; ++which)
    {
        plan.steps[1].exactLaunch = false;
        if (which == 1)
        {
            // ... a move_hop_exact behind too (with its own exact point before it).
            plan.steps[0].flags = ai::kPlanFlagExact;
            plan.steps[1].exactLaunch = true;
        }
        Run r = Start(&plan, 3);
        Facts f = Party(0);
        f.leadX = 2.3f;
        std::string first;
        for (int i = 0; i < 6 && first.empty(); ++i)
            first = Tick(r, f);
        CHECK(first == "move 0.0 0.0 0.0");
    }
}

// A move_hop_exact with no exact move right before it is not sent: the run
// pauses (at the step and on a walk back). Then a narrow check of the rows an
// older module would load: with the hop's row skipped (the parser test in
// test_dungeon_plan_policy.cpp shows the old kind parser returns None for it)
// and the flag ignored, the plan holds no direct step, and THIS runtime run on
// those rows emits no direct order. It does not model the old runtime, and it
// says nothing about where its plain move to seq 5 ends: that deploy state is
// unsupported (the migration and the module ship together).
static void TestBrokenHopPausesAndSkippedHopRowHasNoDirectOrder()
{
    DungeonPlan plan = SerpentisPlan();
    plan.steps[3].flags = 0; // the foot without its flag
    Run r = Start(&plan, 4);
    Facts f = Party(0);
    f.leadX = -293.2f;
    f.leadY = 0.4f;
    f.leadZ = -58.4f;
    CHECK(!IsDirectOrder(Tick(r, f)));
    CHECK(r.state == State::Paused && r.note.find("move_hop_exact") != std::string::npos);
    // ... on a walk back too.
    Run w = Start(&plan, 5);
    w.walkBack = true;
    w.walkBackIdx = w.walkBackStart = 4;
    for (int i = 0; i < 4 && w.state == State::Running; ++i)
        CHECK(!IsDirectOrder(Tick(w, f)));
    CHECK(w.state == State::Paused && w.note.find("walk back") != std::string::npos &&
        w.note.find("move_hop_exact") != std::string::npos);

    DungeonPlan old = SerpentisPlan();
    old.steps[3].flags = 0;                  // the flag dropped, as an older module ignores it
    old.steps.erase(old.steps.begin() + 4);  // the hop's row skipped, as an older loader does (UnknownKind)
    old.steps.insert(old.steps.begin() + 4, Step(PlanStepKind::Move, -270.9f, 24.3f, -60.0f)); // seq 5, on the ramp
    Run o = Start(&old, 0);
    Facts g = Party(0);
    g.leadX = -320.0f;
    g.leadY = -40.0f;
    g.leadZ = -61.0f;
    for (int i = 0; i < 120 && o.state == State::Running; ++i)
        CHECK(!IsDirectOrder(Tick(o, g)));
}

// AiPlayerbot.DungeonRunAggroEstimate: a pull expected to bring more joiners
// than the ceiling waits once, on the rest wait, for the party to be rested.
static UnitFact Joiners(std::uint64_t guid, std::uint32_t entry, float dist, std::uint32_t joiners, bool patrol = false)
{
    UnitFact u = Unit(guid, entry, dist);
    u.joiners = joiners;
    u.joinersPatrol = patrol;
    return u;
}

static void TestAggroEstimateOffChangesNothing()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4852}), PullStep(40, {4851})};
    Run plain = Start(&plan);
    Run off = Start(&plan);
    Facts a = Party(0);
    a.units = {Unit(31, 4852, 54.7f)};
    a.minHealthPct = 85;
    Facts b = a;
    b.units = {Joiners(31, 4852, 54.7f, 40, true)}; // the estimate is read, the ceiling is off
    CHECK(b.joinerCeiling == 0);
    for (int i = 0; i < 40; ++i)
    {
        std::string const oa = Tick(plain, a);
        std::string const ob = Tick(off, b);
        CHECK(oa == ob);
        CHECK(plain.note == off.note && plain.phase == off.phase && plain.step == off.step);
        CHECK(StatusLine(plain) == StatusLine(off));
        CHECK(!off.joinerRest && off.joinerHeld.empty());
        if (i == 3)
        {
            a.anyInCombat = b.anyInCombat = true;
            a.units.clear();
            b.units.clear();
        }
        if (i == 5)
            a.anyInCombat = b.anyInCombat = false;
    }
}

static void TestAggroEstimateHoldsOnceForARest()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4852})};
    Facts f = Party(0);
    f.joinerCeiling = 6;
    f.units = {Joiners(31, 4852, 54.7f, 7)};
    f.minHealthPct = 85; // above the start lines, under the end lines
    // Over the ceiling: a note, no order; then the rest wait; then the pull.
    Run r = Start(&plan);
    CHECK(Tick(r, f).empty());
    CHECK(r.note == "7 joiners expected" && r.joinerHeld.count(31));
    CHECK(r.phase != Phase::AwaitPull && r.pulls == 0);
    CHECK(Tick(r, f) == "rest");
    CHECK(r.phase == Phase::Rest && r.note == "resting: 7 joiners expected");
    for (int i = 0; i < 5; ++i)
        CHECK(Tick(r, f).find("pullback") == std::string::npos);
    f.minHealthPct = 100;
    std::string order;
    for (int i = 0; i < 5 && order.find("pullback") == std::string::npos; ++i)
        order = Tick(r, f);
    CHECK(order == "pullback 31");
    // Only once per unit: after a fruitless pull the same unit goes out again at once.
    CHECK(r.joinerHeld.size() == 1 && !r.joinerRest);

    // Already rested: the note, then the pull, no rest order.
    Run e = Start(&plan);
    CHECK(Tick(e, f).empty());
    CHECK(e.note == "7 joiners expected");
    CHECK(Tick(e, f) == "pullback 31");
    CHECK(e.phase == Phase::AwaitPull);

    // At the ceiling is not over it, and 0 expected joiners never waits.
    f.minHealthPct = 85;
    f.units = {Joiners(31, 4852, 54.7f, 6)};
    Run at = Start(&plan);
    CHECK(Tick(at, f) == "pullback 31");
    f.units = {Joiners(31, 4852, 54.7f, 0)};
    Run none = Start(&plan);
    CHECK(Tick(none, f) == "pullback 31");

    // A patrol being the whole reason is said.
    f.units = {Joiners(31, 4852, 54.7f, 8, true)};
    Run patrol = Start(&plan);
    CHECK(Tick(patrol, f).empty());
    CHECK(patrol.note == "8 joiners expected (a patrol is the cause)");

    // Inside a scripted event the rest wait is not entered.
    f.units = {Joiners(31, 4852, 54.7f, 8)};
    Run ev = Start(&plan);
    ev.eventOn = true;
    CHECK(Tick(ev, f).empty());
    CHECK(Tick(ev, f) == "pullback 31");
    CHECK(ev.phase != Phase::Rest);

    // A fight that begins before the rest is due does not start one.
    Run fight = Start(&plan);
    CHECK(Tick(fight, f).empty());
    CHECK(fight.joinerRest);
    f.anyInCombat = true;
    CHECK(Tick(fight, f).empty());
    CHECK(!fight.joinerRest && fight.phase != Phase::Rest && fight.fighting);

    // A normal rest after a joiner rest counts as idle time again.
    {
        f.anyInCombat = false;
        f.units = {Joiners(31, 4852, 54.7f, 7)};
        f.minHealthPct = 85;
        Run x = Start(&plan);
        Tick(x, f);
        CHECK(Tick(x, f) == "rest" && x.restForJoiners);
        f.minHealthPct = 100;
        for (int i = 0; i < 3 && x.phase == Phase::Rest; ++i)
            Tick(x, f);
        CHECK(x.phase != Phase::Rest);
        f.minHealthPct = 60;
        x.restDue = true;
        x.phase = Phase::Begin;
        CHECK(Tick(x, f) == "rest" && x.phase == Phase::Rest && !x.restForJoiners);
        f.minHealthPct = 85;
        // ... and so does a meal wait.
        Run y = Start(&plan);
        Tick(y, f);
        Tick(y, f);
        CHECK(y.restForJoiners && y.phase == Phase::Rest);
        y.phase = Phase::Begin;
        f.eating = 1;
        Tick(y, f);
        CHECK(y.phase == Phase::Rest && y.restForEating && !y.restForJoiners);
        f.eating = 0;
    }

    // A step's reset (also a resume) forgets the hold.
    Run reset = Start(&plan);
    f.anyInCombat = false;
    CHECK(Tick(reset, f).empty() && reset.joinerRest && !reset.joinerHeld.empty());
    ResetStep(reset);
    reset.joinerRests = 2;
    ResetStep(reset);
    CHECK(!reset.joinerRest && reset.joinerHeld.empty() && reset.joinerNote.empty());
    CHECK(reset.joinerRests == 0 && !reset.restForJoiners);

    // The nearest unit is the one judged: a quiet pick goes out, and the
    // loud one is held when it is picked.
    f.anyInCombat = false;
    f.units = {Joiners(31, 4852, 54.7f, 9), Joiners(32, 4852, 30.0f, 1)};
    Run two = Start(&plan);
    CHECK(Tick(two, f) == "pullback 32");
}


// Six loud packs in one pull step: the joiner rests (60 s each when the party
// stays under the end lines) are not step idle time and are three a step, so
// the run neither pauses on the 300 s idle ceiling nor rests a fourth time.
static void TestJoinerRestsAreBudgetedAndNotIdleTime()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4852})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.joinerCeiling = 6;
    f.minHealthPct = 85; // above the start lines, under the end lines: it never gets rested
    std::vector<UnitFact> alive;
    for (std::uint64_t g = 31; g <= 36; ++g)
        alive.push_back(Joiners(g, 4852, 40.0f + static_cast<float>(g), 9));
    int rests = 0;
    std::vector<std::string> notes;
    for (std::size_t unit = 0; unit < 6; ++unit)
    {
        f.units = std::vector<UnitFact>(alive.begin() + static_cast<long>(unit), alive.end());
        f.anyInCombat = false;
        std::string order;
        Phase prev = r.phase;
        for (int i = 0; i < 400 && order.compare(0, 9, "pullback ") != 0; ++i)
        {
            order = Tick(r, f);
            if (r.phase == Phase::Rest && prev != Phase::Rest)
                ++rests;
            prev = r.phase;
            CHECK(r.state == State::Running);
        }
        CHECK(order == "pullback " + std::to_string(31 + unit));
        notes.push_back(r.note);
        f.anyInCombat = true;
        for (int i = 0; i < 3; ++i)
            Tick(r, f);
        CHECK(r.state == State::Running);
    }
    CHECK(rests == 3);
    CHECK(r.joinerRests == 3);
    CHECK(notes[3] == "9 joiners expected, rest budget used");
    CHECK(notes[5] == "9 joiners expected, rest budget used");
    CHECK(notes[0] != notes[3]);
    // 180 s of joiner rest are not idle time.
    CHECK(r.stepIdleMs < kStepIdleCapMs / 2);
}

// A loud pick the party is already rested for spends no rest slot: the three
// slots go to the rests that really happen later in the step.
static void TestJoinerRestSlotsAreSpentByRealRestsOnly()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4852})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.joinerCeiling = 6;
    std::vector<UnitFact> alive;
    for (std::uint64_t g = 31; g <= 36; ++g)
        alive.push_back(Joiners(g, 4852, 40.0f + static_cast<float>(g), 9));
    int rests = 0;
    std::vector<std::string> notes;
    for (std::size_t unit = 0; unit < 6; ++unit)
    {
        // The first two picks find the party full, the rest find it at 85 %.
        f.minHealthPct = unit < 2 ? 100 : 85;
        f.units = std::vector<UnitFact>(alive.begin() + static_cast<long>(unit), alive.end());
        f.anyInCombat = false;
        std::string order;
        Phase prev = r.phase;
        for (int i = 0; i < 400 && order.compare(0, 9, "pullback ") != 0; ++i)
        {
            order = Tick(r, f);
            if (r.phase == Phase::Rest && prev != Phase::Rest)
                ++rests;
            prev = r.phase;
            CHECK(r.state == State::Running);
        }
        CHECK(order == "pullback " + std::to_string(31 + unit));
        notes.push_back(r.note);
        if (unit == 1)
            CHECK(r.joinerRests == 0 && rests == 0);
        f.anyInCombat = true;
        for (int i = 0; i < 3; ++i)
            Tick(r, f);
    }
    CHECK(rests == 3 && r.joinerRests == 3);
    CHECK(notes[5] == "9 joiners expected, rest budget used");
    CHECK(notes[4] != notes[5]);
}

// Both keys on (AiPlayerbot.DungeonRunAggroEstimate and DungeonRunRezHold): a loud pull, a death in its
// fight, then the regroup, then the next loud pick. The joiner rest is a pull-step thing (cleared by
// ResetStep, kept by BeginRegroup, which a single death reaches without a ResetStep); the rez hold's clocks
// are a regroup thing (restarted by BeginRegroup, never by a step reset). Neither may reset the other.
static void TestAggroEstimateAndRezHoldDoNotCrossTheirResets()
{
    using namespace TortoiseBots::RezHold;
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4852})};
    Run r = Start(&plan);
    Facts f = Party(0);
    f.joinerCeiling = 6;
    f.rezHold = true;
    f.minHealthPct = 85; // under the end lines: the joiner rest really starts
    f.units = {Joiners(21, 4852, 40.0f, 9), Joiners(22, 4852, 45.0f, 9)};
    // 1. Loud pick 21: the joiner rest starts (not idle time), then the pull goes out.
    for (int i = 0; i < 4 && r.phase != Phase::Rest; ++i)
        Tick(r, f);
    CHECK(r.phase == Phase::Rest && r.restForJoiners && r.joinerRests == 1 && r.joinerHeld.count(21));
    f.minHealthPct = 100;
    std::string order;
    for (int i = 0; i < 20 && order.compare(0, 9, "pullback ") != 0; ++i)
        order = Tick(r, f);
    CHECK(order == "pullback 21");
    // 2. The fight: the mage (index 2) falls, 21 dies; 22 is still up. Out of combat the single death
    // starts the regroup with no ResetStep.
    f.anyInCombat = true;
    for (int i = 0; i < 3; ++i)
        Tick(r, f);
    f.dead = 1;
    f.alive = 4;
    f.units = {Joiners(22, 4852, 45.0f, 9)};
    f.rezMembers = RezRoster(true, 500);
    Tick(r, f);
    f.anyInCombat = false;
    for (int i = 0; i < 4 && !r.regroup; ++i)
        Tick(r, f);
    CHECK(r.regroup && r.regroupMs < 3 * kTickMs && r.rezAbleMs == 0 && r.reviveBaseMs == 0);
    // The regroup's restart left the step's joiner state alone: 21 was rested for, one slot is spent.
    CHECK(r.joinerRests == 1 && r.joinerHeld.count(21) == 1);
    // 3. The rez hold decides the regroup (well past the old 15 s) and the joiner rest does not move.
    std::uint32_t revivedAt = 0;
    std::uint32_t const idleBefore = r.stepIdleMs;
    for (std::uint32_t ms = kTickMs; ms <= 200000 && !revivedAt; ms += kTickMs)
    {
        if (Tick(r, f) == "revive party")
            revivedAt = ms;
        CHECK(r.joinerRests == 1 && r.joinerHeld.count(21) == 1 && !r.joinerRest);
    }
    CHECK(revivedAt >= kHoldMs && revivedAt <= kHoldMs + 4 * kTickMs);
    CHECK(r.stepIdleMs == idleBefore); // a regroup is not step idle time
    // The invariant in both directions: a step reset leaves the regroup clocks, a regroup start leaves
    // the joiner state.
    {
        Run c = r;
        c.rezAbleMs = 7000;
        c.reviveBaseMs = 9000;
        c.regroupMs = 11000;
        ResetStep(c);
        CHECK(c.rezAbleMs == 7000 && c.reviveBaseMs == 9000 && c.regroupMs == 11000);
        CHECK(c.joinerRests == 0 && c.joinerHeld.empty() && !c.restForJoiners);
        Run d = r;
        d.restForJoiners = true;
        BeginRegroup(d);
        CHECK(d.joinerRests == 1 && d.joinerHeld.count(21) == 1 && d.restForJoiners);
    }
    // 4. The party is raised: the regroup ends, the step goes on with 22. It is rested for (a second
    // slot, 21's kept), the pull goes out, and the whole of it was no step idle time.
    f.dead = 0;
    f.alive = 5;
    f.rezMembers = RezRoster(false, 500);
    f.minHealthPct = 85;
    for (int i = 0; i < 12 && r.regroup; ++i)
        Tick(r, f);
    CHECK(!r.regroup);
    for (int i = 0; i < 6 && r.phase != Phase::Rest; ++i)
        Tick(r, f);
    CHECK(r.phase == Phase::Rest && r.restForJoiners && r.joinerRests == 2);
    CHECK(r.joinerHeld.count(21) == 1 && r.joinerHeld.count(22) == 1);
    CHECK(r.note.find("9 joiners expected") != std::string::npos);
    f.minHealthPct = 100;
    order.clear();
    for (int i = 0; i < 20 && order.compare(0, 9, "pullback ") != 0; ++i)
        order = Tick(r, f);
    CHECK(order == "pullback 22");
    CHECK(r.state == State::Running && r.stepIdleMs < kStepIdleCapMs / 2);
    // 5. A wipe in a later step reset (ResetStep, then BeginRegroup): fresh joiner budget and fresh clocks.
    ResetStep(r);
    BeginRegroup(r);
    CHECK(r.joinerRests == 0 && r.joinerHeld.empty() && r.regroupMs == 0 && r.rezAbleMs == 0);
}

// Both keys off: a run fed the estimate's joiners and the rez roster (the glue reads neither with the keys
// off, but a stray value must change nothing) ticks exactly like one fed neither, through a loud pull, a
// death in its fight and the regroup, tick by tick.
static void TestBothKeysOffChangeNothing()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4852})};
    Run plain = Start(&plan);
    Run noisy = Start(&plan);
    Facts a = Party(0);
    a.minHealthPct = 85;
    a.units = {Unit(21, 4852, 40.0f), Unit(22, 4852, 45.0f)};
    a.rezzer = true;
    Facts b = a;
    b.units = {Joiners(21, 4852, 40.0f, 9, true), Joiners(22, 4852, 45.0f, 9)};
    b.rezMembers = RezRoster(true, 0, 500.0f);
    CHECK(b.joinerCeiling == 0 && !b.rezHold);
    for (int i = 0; i < 300; ++i)
    {
        if (i == 20)
        {
            a.minHealthPct = b.minHealthPct = 100;
        }
        if (i == 30)
            a.anyInCombat = b.anyInCombat = true;
        if (i == 40)
        {
            a.anyInCombat = b.anyInCombat = false;
            a.dead = b.dead = 1;
            a.alive = b.alive = 4;
            a.units = {Unit(22, 4852, 45.0f)};
            b.units = {Joiners(22, 4852, 45.0f, 9)};
        }
        if (i == 150)
        {
            a.dead = b.dead = 0;
            a.alive = b.alive = 5;
        }
        std::string const oa = Tick(plain, a);
        std::string const ob = Tick(noisy, b);
        CHECK(oa == ob);
        CHECK(plain.note == noisy.note && plain.phase == noisy.phase && plain.regroup == noisy.regroup);
        CHECK(plain.regroupMs == noisy.regroupMs && plain.reviveSent == noisy.reviveSent);
        CHECK(StatusLine(plain) == StatusLine(noisy));
        CHECK(!noisy.joinerRest && noisy.joinerHeld.empty() && noisy.joinerRests == 0 && noisy.rezAbleMs == 0 &&
              noisy.reviveBaseMs == 0);
    }
}

// The pull stand (AiPlayerbot.DungeonRunPullStand, HoldForShot). Shoot Bow: the
// module's GetRange 29.5, so the tank shoots from where it stands within 24.5 yd.
static Facts StandParty()
{
    Facts f = Party(0);
    f.pullStand = true;
    f.leadShotYd = 29.5f;
    return f;
}

static UnitFact Seen(std::uint64_t guid, std::uint32_t entry, float dist, bool los = true)
{
    UnitFact u = Unit(guid, entry, dist);
    u.losLead = los;
    return u;
}

static bool Has(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

// The glue's read of a stand for the unit the policy asked for (Run::shotStandFor).
static void TestPullNoteCarriesTheJoinerCount()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4852})};
    Facts f = Party(0);
    f.joinerCeiling = 6;
    std::string order;

    // Ceiling 6, the glue read 3 joiners for the unit: the count rides on the note.
    {
        Run r = Start(&plan);
        UnitFact u = Joiners(31, 4852, 40.0f, 3);
        u.joinersRead = true;
        f.units = {u};
        order.clear();
        PullUnit(r, f, f.units[0], order);
        CHECK(order == "pullback 31" && r.note == "pulling 4852; 3 joiners expected");
    }
    // Ceiling 6, the unit was held or out of the glue's index, so no count this step: the note says so,
    // and it never claims the stale count below.
    {
        Run r = Start(&plan);
        UnitFact u = Unit(31, 4852, 40.0f);
        u.joiners = 5;   // a stale read kept, but marked not read
        u.joinersPatrol = true;
        f.units = {u};
        order.clear();
        PullUnit(r, f, f.units[0], order);
        CHECK(order == "pullback 31" && r.note == "pulling 4852; joiners not read");
    }
    // Ceiling on, the pull stand on: the count goes after the stand text.
    {
        Run r = Start(&plan);
        Facts s = StandParty();
        s.joinerCeiling = 6;
        UnitFact u = Joiners(11, 4851, 24.5f, 3);
        u.joinersRead = true;
        u.losLead = true;
        s.units = {u};
        CHECK(Tick(r, s) == "pullback 11");
        CHECK(r.note ==
            "pulling 4851 0x000000000000000b (order 1; shot from where it stands); 3 joiners expected");
    }

    // Ceiling off: a stray count and stray read in the facts change nothing, both stand variants.
    f.joinerCeiling = 0;
    {
        Run r = Start(&plan);
        UnitFact u = Joiners(31, 4852, 40.0f, 9);
        u.joinersRead = true;
        f.units = {u};
        order.clear();
        PullUnit(r, f, f.units[0], order);
        CHECK(order == "pullback 31" && r.note == "pulling 4852");
    }
    {
        Run r = Start(&plan);
        Facts s = StandParty();
        UnitFact u = Seen(11, 4851, 24.5f);
        u.joiners = 9;
        u.joinersRead = true;
        u.joinersPatrol = true;
        s.units = {u};
        CHECK(Tick(r, s) == "pullback 11");
        CHECK(r.note == "pulling 4851 0x000000000000000b (order 1; shot from where it stands)");
    }
}

// The glue's read of a stand for the unit the policy asked for (Run::shotStandFor).
static void GiveStand(Facts& f, const Run& r, float x, float yd)
{
    f.shotStandGuid = r.shotStandFor;
    f.shotStandKnown = true;
    f.shotStandX = x;
    f.shotStandY = 0.0f;
    f.shotStandZ = 0.0f;
    f.shotStandYd = yd;
}

static void ClearStand(Facts& f)
{
    f.shotStandGuid = 0;
    f.shotStandKnown = false;
}

// In sight and within 24.5 yd (the edge included), standing still: the order on the first tick, its note
// carrying the guid, the order count and how the shot stood.
static void TestPullShootsFromStand()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    Run r = Start(&plan);
    Facts f = StandParty();
    f.units = {Seen(11, 4851, 24.5f)};
    CHECK(Tick(r, f) == "pullback 11");
    CHECK(r.phase == Phase::AwaitPull);
    CHECK(r.note == "pulling 4851 0x000000000000000b (order 1; shot from where it stands)");
    CHECK(r.shotStandTries[11] == 0 && r.shotStandFor == 0);
    // No fight: the re-order after kPullNoFightMs has a note of its own (the trace keeps only changes).
    std::string order;
    for (int i = 0; i < 40 && order.empty(); ++i)
        order = Tick(r, f);
    CHECK(order == "pullback 11");
    CHECK(r.note == "pulling 4851 0x000000000000000b (order 2; shot from where it stands)");
}

// A moving tank holds the order up to kShotStillWaitMs, then it goes out anyway; one that stops goes at once.
static void TestPullWaitsForStillTank()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 20.0f)};
        f.leadMoving = true;
        for (std::uint32_t ms = 0; ms < kShotStillWaitMs; ms += kTickMs)
        {
            CHECK(Tick(r, f).empty());
            CHECK(r.note == "waiting for the tank to stand still before pulling 4851 0x000000000000000b");
        }
        CHECK(Tick(r, f) == "pullback 11");
        CHECK(Has(r.note, "(order 1; shot from where it stands, still moving)"));
    }
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 20.0f)};
        f.leadMoving = true;
        CHECK(Tick(r, f).empty());
        f.leadMoving = false;
        CHECK(Tick(r, f) == "pullback 11");
    }
}

// 27 yd (inside the shot's 29.5, outside its 24.5 with the margin): the stand is read, the party walks to
// it, and the order goes out from there.
static void TestPullWalksToStandOutOfRange()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    Run r = Start(&plan);
    Facts f = StandParty();
    f.units = {Seen(11, 4851, 27.0f)};
    CHECK(Tick(r, f).empty());
    CHECK(r.shotStandFor == 11);
    CHECK(r.note == "reading a firing stand for 4851 0x000000000000000b");
    GiveStand(f, r, 5.0f, 23.0f);
    CHECK(Tick(r, f) == "move 5.0 0.0 0.0");
    CHECK(r.phase == Phase::Walk && r.shotStandTries[11] == 1 && r.shotStandFor == 0);
    CHECK(r.note == "to firing stand 1 for 4851 0x000000000000000b (23.0 yd from it; was 27.0 yd, los yes)");
    ClearStand(f);
    f.leadX = 1.0f;
    f.leadMoving = true;
    f.units[0].distLead = 26.0f;
    CHECK(Tick(r, f).empty());
    CHECK(Has(r.note, "to firing stand 1 for 4851"));
    f.leadX = 5.0f;
    f.leadMoving = false;
    f.units[0].distLead = 22.0f;
    CHECK(Tick(r, f) == "pullback 11");
    CHECK(r.note == "pulling 4851 0x000000000000000b (order 1; shot from firing stand 1)");
    // The anchor is the stand: the pullback's return leg and the next gather are there.
    CHECK(r.anchorKnown && r.anchorX == 5.0f);
}

// 22 yd, in range but out of sight: a stand too; a tank still moving on arrival is waited for.
static void TestPullWalksToStandNoLos()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    Run r = Start(&plan);
    Facts f = StandParty();
    f.units = {Seen(11, 4851, 22.0f, false)};
    CHECK(Tick(r, f).empty());
    CHECK(r.shotStandFor == 11);
    GiveStand(f, r, -6.0f, 20.0f);
    CHECK(Tick(r, f) == "move -6.0 0.0 0.0");
    CHECK(Has(r.note, "was 22.0 yd, los no"));
    ClearStand(f);
    f.leadX = -6.0f;
    f.leadMoving = true;
    f.units[0] = Seen(11, 4851, 20.0f, true);
    CHECK(Tick(r, f).empty());
    CHECK(Has(r.note, "waiting for the tank to stand still"));
    f.leadMoving = false;
    CHECK(Tick(r, f) == "pullback 11");
    CHECK(Has(r.note, "shot from firing stand 1"));
}

// At most kShotStandTries stands per unit; then, and with no stand found, the order as before.
static void TestPullStandTriesBounded()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 40.0f, false)};
        f.anchorPathRead = true; // the glue's answer to the way back (the stand before)
        std::vector<std::string> orders;
        for (int i = 0; i < 30 && (orders.empty() || orders.back().compare(0, 9, "pullback ") != 0); ++i)
        {
            if (r.shotStandFor)
                GiveStand(f, r, f.leadX + 6.0f, 34.0f);
            else
                ClearStand(f);
            std::string const o = Tick(r, f);
            if (!o.empty())
                orders.push_back(o);
            if (r.phase == Phase::Walk)
                f.leadX = r.walkX; // arrives on the next tick; still not in sight
        }
        CHECK(orders.size() == 3);
        CHECK(orders[0] == "move 6.0 0.0 0.0" && orders[1] == "move 12.0 0.0 0.0" && orders[2] == "pullback 11");
        CHECK(r.shotStandTries[11] == kShotStandTries);
        CHECK(r.note == "pulling 4851 0x000000000000000b (order 1; firing stands used up (2))");
    }
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 40.0f, false)};
        CHECK(Tick(r, f).empty());
        f.shotStandGuid = 11; // read, nothing found
        f.shotStandKnown = false;
        CHECK(Tick(r, f) == "pullback 11");
        CHECK(Has(r.note, "(order 1; no firing stand found)"));
    }
}

// A body-pull tank (leadShotYd 0) with the key on: the same orders as with the key off, tick by tick
// (only the order's note is longer). The key off with stray stand facts: everything the same, notes too.
static void TestPullNoShotDataKeepsOldOrder()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851, 4852})};
    for (int variant = 0; variant < 2; ++variant)
    {
        Run plain = Start(&plan);
        Run other = Start(&plan);
        Facts a = Party(0);
        // 12 is shootable from where the tank stands, 11 nearer and out of sight: F2 would pick 12.
        a.units = {Unit(11, 4851, 18), Unit(12, 4852, 23)};
        a.maxMemberYd = 35;
        Facts b = a;
        if (variant == 0)
        {
            b.pullStand = true; // key on, no ranged pull
            b.leadShotYd = 0.0f;
        }
        else
        {
            b.pullStand = false; // key off, stray facts
            b.leadShotYd = 29.5f;
            b.shotStandGuid = 12;
            b.shotStandKnown = true;
            b.shotStandX = 9.0f;
        }
        b.units[0].losLead = false;
        b.units[1].losLead = true;
        b.leadMoving = a.leadMoving = true;
        bool sawFirst = false;
        for (int i = 0; i < 120; ++i)
        {
            if (i == 2)
                a.maxMemberYd = b.maxMemberYd = 5;
            if (i == 60)
                a.anyInCombat = b.anyInCombat = true;
            if (i == 70)
            {
                a.anyInCombat = b.anyInCombat = false;
                a.units = {Unit(11, 4851, 18)};
                b.units = {a.units[0]};
            }
            std::string const oa = Tick(plain, a);
            std::string const ob = Tick(other, b);
            CHECK(oa == ob);
            CHECK(oa != "pullback 12" || i >= 70 || sawFirst); // the nearest (11) goes first
            sawFirst = sawFirst || oa == "pullback 11";
            CHECK(plain.phase == other.phase && plain.step == other.step && plain.state == other.state);
            CHECK(other.shotStandFor == 0 && !other.shotStandWalk && !other.shotStandPrep);
            if (variant == 1)
            {
                CHECK(plain.note == other.note);
                CHECK(StatusLine(plain) == StatusLine(other));
            }
            else if (oa.compare(0, 9, "pullback ") == 0)
                CHECK(Has(other.note, "no ranged weapon pull: stand rule off"));
        }
        CHECK(sawFirst);
    }
}

// (review 1) A shot range at or under the margin (4.5, 5.0): the glue reads no stand for it, so the rule
// is off and the order goes out on the first tick (it used to read "reading a firing stand" for 300 s).
static void TestPullStandNeedsMoreRangeThanTheMargin()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    for (float shot : {4.5f, 5.0f})
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.leadShotYd = shot;
        f.units = {Seen(11, 4851, 20.0f, false)};
        CHECK(Tick(r, f) == "pullback 11");
        CHECK(Has(r.note, "no ranged weapon pull: stand rule off") && r.shotStandFor == 0);
    }
}

// (review 2, 6) A stand request lives one tick: the next tick answers or drops it, whether the gate runs
// (a still-wait) or not (a fight); the hold's prep, walk and wait end with any tick the gate did not run on.
static void TestStandRequestIsReadOnce()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 40.0f, false)};
        CHECK(Tick(r, f).empty());
        CHECK(r.shotStandFor == 11 && r.shotStandPrep);
        f.anyInCombat = true; // a fight comes before the read is used
        Tick(r, f);
        CHECK(r.shotStandFor == 0 && !r.shotStandPrep);
        for (int i = 0; i < 5; ++i)
            Tick(r, f);
        CHECK(r.shotStandFor == 0 && !r.shotStandPrep && !r.shotStandWalk && r.shotStillMs == 0);
    }
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 40.0f, false)};
        CHECK(Tick(r, f).empty());
        GiveStand(f, r, 10.0f, 30.0f);
        f.leadMoving = true; // the read tick goes to the still-wait: the read is dropped, not asked again
        CHECK(Tick(r, f).empty());
        CHECK(r.shotStandFor == 0 && Has(r.note, "waiting for the tank to stand still"));
        CHECK(Tick(r, f).empty());
        CHECK(r.shotStandFor == 0);
    }
    {
        // A gather breaks the stand walk off: the hold ends (prep and walk off), the stand stays counted.
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 40.0f, false)};
        Tick(r, f);
        GiveStand(f, r, 10.0f, 30.0f);
        CHECK(Tick(r, f) == "move 10.0 0.0 0.0");
        ClearStand(f);
        f.maxMemberYd = 30.0f;
        CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the gather on the lead bot's spot
        CHECK(!r.shotStandPrep && !r.shotStandWalk && r.shotStandTries[11] == 1);
    }
}

// (review 4) A stand walk that gains nothing for kWalkRetryMs, or runs past kShotWalkCapMs, gives the stand
// up and orders from where the tank is; the run never pauses for it.
static void TestStandWalkStuckOrdersFromHere()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    for (int longWalk = 0; longWalk < 2; ++longWalk)
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 120.0f, false)};
        Tick(r, f);
        GiveStand(f, r, 100.0f, 24.0f);
        CHECK(Tick(r, f) == "move 100.0 0.0 0.0");
        ClearStand(f);
        f.anchorPathRead = true;
        std::string order;
        std::uint32_t ms = 0;
        for (; ms < 60000 && order.empty(); ms += kTickMs)
        {
            if (longWalk)
                f.leadX += 2.1f; // headway every tick, never there in time
            order = Tick(r, f);
            CHECK(r.state == State::Running);
        }
        CHECK(order == "pullback 11");
        CHECK(Has(r.note, "(order 1; firing stand unreachable: ordering from here)"));
        // The give-up tick holds once more (the way back is checked on the next): the order a tick later.
        CHECK(longWalk ? ms == kShotWalkCapMs + kTickMs : ms == kWalkRetryMs + kTickMs);
        CHECK(r.shotStandTries[11] == kShotStandTries);
    }
}

// (review 4) The stand walk is driven on every tick while the way back to the old anchor is read on every
// other one (the glue reads the path the tick after it is asked for).
static void TestStandWalkIsDrivenEveryTick()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    Run r = Start(&plan);
    Facts f = StandParty();
    f.units = {Seen(11, 4851, 40.0f, false)};
    Tick(r, f);
    GiveStand(f, r, 20.0f, 20.0f);
    CHECK(Tick(r, f) == "move 20.0 0.0 0.0");
    ClearStand(f);
    r.anchorKnown = true; // the anchor the walk leaves: 2 yd behind the tank
    r.anchorX = -2.0f;
    for (int i = 0; i < 5; ++i)
    {
        f.anchorPathRead = r.anchorPathWanted;
        f.leadX += 3.0f;
        f.leadMoving = true;
        CHECK(Tick(r, f).empty());
        CHECK(Has(r.note, "to firing stand 1 for 4851"));
    }
}

// (review round 2) The stuck give-up holds its own tick and drops the hold's pass on the way back, so the
// next tick checks the anchor from where the tank got stuck: more than kStragglerDzYd below it (another
// floor) the party gathers back there first; the same floor with a way back, the order goes out.
static void TestStuckStandChecksTheWayBackFirst()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    for (int otherFloor = 0; otherFloor < 2; ++otherFloor)
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 120.0f, false)};
        r.anchorKnown = true; // the anchor: (0, 0, 0)
        Tick(r, f);
        GiveStand(f, r, 100.0f, 24.0f);
        CHECK(Tick(r, f) == "move 100.0 0.0 0.0");
        ClearStand(f);
        f.leadX = 30.0f; // stuck there
        f.leadZ = otherFloor ? -12.0f : 0.0f;
        std::string order;
        int ticks = 0;
        for (; ticks < 30 && order.empty() && !Has(r.note, "firing stand unreachable"); ++ticks)
            order = Tick(r, f);
        CHECK(order.empty()); // the give-up tick sends nothing
        CHECK(r.note == "firing stand unreachable for 4851 0x000000000000000b: checking the way back before the order");
        CHECK(!r.shotStandPrep && r.shotStandTries[11] == kShotStandTries && r.state == State::Running);
        if (otherFloor)
        {
            CHECK(Tick(r, f) == "move 0.0 0.0 0.0"); // the gather back to the anchor
            CHECK(r.phase == Phase::Gather);
        }
        else
        {
            f.anchorPathRead = true; // the glue's read: the way back is there
            CHECK(Tick(r, f) == "pullback 11");
            CHECK(Has(r.note, "(order 1; firing stand unreachable: ordering from here)"));
        }
    }
}

// (review round 2) The pick switching to another unit in the middle of a hold: the new unit's first tick
// checks the way back (the hold's pass belonged to the old unit).
static void TestPickSwitchChecksTheWayBack()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    Run r = Start(&plan);
    Facts f = StandParty();
    f.units = {Seen(11, 4851, 40.0f, false)};
    r.anchorKnown = true;
    r.anchorX = -10.0f; // the lead bot is off it: the way back has to be read
    f.anchorPathRead = true;
    CHECK(Tick(r, f).empty());
    CHECK(r.shotStandPrep && r.shotHoldGuid == 11);
    f.anchorPathRead = false;
    f.units = {Seen(11, 4851, 40.0f, false), Seen(12, 4851, 30.0f, false)}; // 12 is nearer now
    CHECK(Tick(r, f).empty());
    CHECK(Has(r.note, "checking the way back"));
    CHECK(!r.shotStandPrep);
    // A move step's hop pull (PullUnit alone, no FightStep): the same.
    DungeonPlan move;
    move.steps = {Step(PlanStepKind::Move, 50, 0, 0)};
    Run m = Start(&move);
    Facts g = StandParty();
    g.hopUnits = {Seen(11, 4851, 40.0f, false)};
    m.anchorKnown = true;
    m.anchorX = -10.0f;
    g.anchorPathRead = true;
    CHECK(Tick(m, g).empty());
    CHECK(m.shotStandPrep && m.shotHoldGuid == 11);
    g.anchorPathRead = false;
    g.hopUnits = {Seen(11, 4851, 40.0f, false), Seen(12, 4851, 30.0f, false)};
    CHECK(Tick(m, g).empty());
    CHECK(Has(m.note, "checking the way back") && !m.shotStandPrep);
}

// F2: among units of the same kill_first rank one the tank can shoot from where it stands goes before a
// nearer one it cannot; the rank still comes first; the key off picks the nearest.
static void TestPickPackPullPrefersShootable()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851, 4852})};
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 18.0f, false), Seen(12, 4852, 23.0f, true)};
        CHECK(Tick(r, f) == "pullback 12");
    }
    {
        Run r = Start(&plan);
        Facts f = StandParty();
        f.pullStand = false;
        f.units = {Seen(11, 4851, 18.0f, false), Seen(12, 4852, 23.0f, true)};
        CHECK(Tick(r, f) == "pullback 11");
    }
    {
        DungeonPlan ranked = plan;
        ranked.killFirst = {4851};
        Run r = Start(&ranked);
        Facts f = StandParty();
        f.units = {Seen(11, 4851, 18.0f, false), Seen(12, 4852, 23.0f, true)};
        CHECK(Tick(r, f).empty()); // the kill_first unit, out of sight: its stand is read
        CHECK(r.shotStandFor == 11);
    }
}

// The ticks a stand rule holds a pull are one pull: a pack with a fruitless pull behind it is not skipped
// while the tank walks to its stand (FightStep's kFruitlessPulls).
static void TestStandTicksAreNotFruitlessPulls()
{
    DungeonPlan plan;
    plan.steps = {PullStep(40, {4851})};
    Run r = Start(&plan);
    Facts f = StandParty();
    f.units = {Seen(11, 4851, 20.0f), Seen(12, 4851, 40.0f)};
    CHECK(Tick(r, f) == "pullback 11");
    // 11 never comes (no fight); it is ordered again, then passed, and 12 needs a stand.
    f.units[0].losLead = false;
    f.units[0].distLead = 30.0f;
    f.anchorPathRead = true;
    std::string order;
    for (int i = 0; i < 200 && r.step == 0 && order != "pullback 12"; ++i)
    {
        if (r.shotStandFor)
            GiveStand(f, r, f.leadX + 3.5f + static_cast<float>(i % 3), 24.0f);
        else
            ClearStand(f);
        order = Tick(r, f);
        if (r.phase == Phase::Walk)
        {
            // Two ticks on the way, then there.
            f.leadMoving = true;
            Tick(r, f);
            Tick(r, f);
            f.leadMoving = false;
            f.leadX = r.walkX;
        }
    }
    CHECK(r.step == 0 && r.state == State::Running);
    CHECK(order == "pullback 12");
}

// The stand helper: beyond range, the ranged reach's own point; in range but out of sight, the first path
// point nearer the unit that sees it; nothing in sight, no stand.
static void TestShotStandOnPath()
{
    using ai::ApproachPoint;
    auto P = [](float x, float y) { ApproachPoint p; p.x = x; p.y = y; return p; };
    ApproachPoint const unit = P(0, 0);
    auto seesAll = [](const ApproachPoint&) { return true; };
    {
        std::vector<ApproachPoint> path = {P(40, 0), P(30, 0), P(20, 0), P(0, 0)};
        ShotStand const s = ShotStandOnPath(path, unit, 24.5f, seesAll);
        CHECK(s.known && std::fabs(s.x - 24.5f) < 0.01f);
    }
    {
        // 22 yd from it, the pillar hides it until the corner at (10, 10).
        std::vector<ApproachPoint> path = {P(22, 0), P(18, 6), P(10, 10), P(0, 0)};
        auto corner = [](const ApproachPoint& p) { return p.x <= 10.0f; };
        ShotStand const s = ShotStandOnPath(path, unit, 24.5f, corner);
        CHECK(s.known && s.x == 10.0f && s.y == 10.0f);
        auto none = [](const ApproachPoint&) { return false; };
        CHECK(!ShotStandOnPath(path, unit, 24.5f, none).known);
        CHECK(!ShotStandOnPath(path, unit, 0.0f, seesAll).known);
    }
}

// Uldaman run 4, step 38: a fight past the 180 s ceiling paused the run, the
// fight ended 15 s later, and the run stayed paused for 470 s. A ceiling pause
// goes on by itself once the party is out of combat kCeilingQuietMs, once per
// step; a player's pause and every other pause never do.
static void TestCeilingPauseResumesByItself()
{
    DungeonPlan plan;
    PlanStep wait = Step(PlanStepKind::WaitObject, 1, 2, 3);
    wait.entry = 124369;
    wait.param = 1;
    plan.steps = {wait, Step(PlanStepKind::Move, 9)};
    auto ceilingPause = [&](Run& r)
    {
        r.fighting = true;
        r.fightMs = kFightCapMs + 1000;
        Facts g = Party(0);
        g.anyInCombat = true;
        Tick(r, g);
    };
    // The fight past the ceiling pauses; ticks with the party in combat stay paused.
    Run r = Start(&plan);
    ceilingPause(r);
    CHECK(r.state == State::Paused && r.ceilingPaused &&
        r.note.find("a fight lasted more than 180 s") != std::string::npos);
    for (int i = 0; i < 10; ++i)
        CHECK(!CeilingFightOverWhilePaused(r, true, 1000) && r.state == State::Paused);
    // A quiet stretch broken by one combat tick starts over.
    CHECK(!CeilingFightOverWhilePaused(r, false, 4000));
    CHECK(!CeilingFightOverWhilePaused(r, true, 1000) && r.ceilingQuietMs == 0);
    CHECK(!CeilingFightOverWhilePaused(r, false, 4000) && r.state == State::Paused);
    // kCeilingQuietMs out of combat: it goes on by itself, regrouping.
    CHECK(CeilingFightOverWhilePaused(r, false, 1000));
    CHECK(r.state == State::Running && !r.ceilingPaused && r.step == 0);
    CHECK(r.note.find("going on by itself") != std::string::npos);
    // A second ceiling on the same step stays paused for good.
    ceilingPause(r);
    CHECK(r.state == State::Paused && r.ceilingPaused);
    CHECK(!CeilingFightOverWhilePaused(r, false, kCeilingQuietMs * 2) && r.state == State::Paused);
    // A player's pause never self-resumes, and clears the ceiling mark.
    Run p = Start(&plan);
    ceilingPause(p);
    PausePlayer(p, "player order (stay): .bot run resume to go on");
    CHECK(!p.ceilingPaused);
    CHECK(!CeilingFightOverWhilePaused(p, false, kCeilingQuietMs * 2) && p.state == State::Paused);
    // ... and the guard holds on its own even if the mark were left set.
    p.ceilingPaused = true;
    CHECK(!CeilingFightOverWhilePaused(p, false, kCeilingQuietMs * 2) && p.state == State::Paused);
    // Any other pause after the ceiling's clears it.
    Run o = Start(&plan);
    ceilingPause(o);
    Pause(o, "the lead bot is not on the plan's map (70)");
    CHECK(!o.ceilingPaused);
    CHECK(!CeilingFightOverWhilePaused(o, false, kCeilingQuietMs * 2) && o.state == State::Paused);
    // A manual resume and a skip clear it as well.
    Run m = Start(&plan);
    ceilingPause(m);
    CHECK(Resume(m) && !m.ceilingPaused);
    CHECK(!CeilingFightOverWhilePaused(m, false, kCeilingQuietMs * 2));
    Run k = Start(&plan);
    ceilingPause(k);
    CHECK(Skip(k) && !k.ceilingPaused && k.step == 1);
    // A running run is untouched.
    Run u = Start(&plan);
    CHECK(!CeilingFightOverWhilePaused(u, false, kCeilingQuietMs * 2) && u.state == State::Running);
}

int main()
{
    TestGoneChestWithFlagIsDone();
    TestGoneChestWithoutFlagPauses();
    TestGoneIsDoneOnlyForAGoneObject();
    TestHopRechecksTheLeadOnTheFoot();
    TestDriftWithinTheRadiusIsNoProof();
    TestProofIsRetakenAfterAFightAndAResume();
    TestBackPointStopsAtADirectStep();
    TestBrokenHopPausesAndSkippedHopRowHasNoDirectOrder();
    TestWalkBackReplaysTheExactPoint();
    TestExactSerpentisFootIsNotSkipped();
    TestExactNeverProvedPauses();
    TestExactReissuesTheMoveAndIgnoresTheStallRule();
    TestDirectFromExactPointWaitsForTheProof();
    TestRunTimeLimitAndStopNote();
    TestStartFromParsing();
    TestStartFromResolve();
    TestStartInsideEventGoesBack();
    TestPartialStartGuard();
    TestMove();
    TestWalkStuckIsBounded();
    TestShortHopArrivesWithoutTheRetryWait();
    TestShortDirectHopKeepsTheLongWait();
    TestShortHopThatMovesIsNotCut();
    TestStalledNearPointCountsAsArrived();
    TestPullLoopOnOneTargetPauses();
    TestOutOfReachPackIsSkippedNotCleared();
    TestMissingItemPausesOnce();
    TestResumeRegroups();
    TestResumeWithLeadDead();
    TestFarMoveWalksBackInHops();
    TestDirectStepGathersFirst();
    TestSkipWhileRunning();
    TestUnreachableMoveDetours();
    TestPullGatherAndFight();
    TestLeadHoldsStayWhileResting();
    TestUnseenPackIsApproachedNotCleared();
    TestFarPackHopIsNotUndoneByTheGatherBack();
    TestApproachHopCapCountsHopsWithoutProgress();
    TestItemCarrierIsPickedFromTheStepsOwnInputs();
    TestUseItemPausesOnSplitInputs();
    TestLootStepIsDoneWhenAnyPartyBotHasTheItem();
    TestLootRefusalTriesTheTurnBotThenEachBot();
    TestLootFfaOrderSequence();
    TestLootFfaRestoresOnAnErrAnswerAndNeverTwice();
    TestLootFfaStaysLiveThroughAPause();
    TestLootFfaRestoresWhenTheTaskEndsUnanswered();
    TestLootFfaRefusedFallsBackToTheOldPath();
    TestLootFfaConditions();
    TestLootFfaRegroupBetweenAckAndRestoreRestores();
    TestLootFfaRestoreRefusedRetriesThenPauses();
    TestLootFfaFlagEndsOnlyWhenTheMethodIsNotFfa();
    TestLootFfaOrderedWaitIsBounded();
    TestLootFfaOrderedThenFallenRegroupRestores();
    TestLootFfaKeyOffIsTheOldPath();
    TestOutstandingCarrierKeepsTheOrdersBot();
    TestRetiredUseItemOrderGoesOutAgain();
    TestWaitObjectDoneWinsOverAStaleFightTimer();
    TestPullThatStartsNoFightIsPassed();
    TestAmbushWalksThrough();
    TestBossKillOrderAndTeamGate();
    TestHopPullsNearUnitsFirst();
    TestWipeAndLeadDeathPause();
    TestFewDeadReviveBeforePull();
    TestFewDeadRuleSkipsEvents();
    TestRegroupFightsFirst();
    TestRegroupWaitsForPartyRez();
    TestRegroupRevivePauses();
    TestMealWaitBeforePull();
    TestFightCeiling();
    TestTaskSteps();
    TestOpenWithoutKeyAndTaskError();
    TestTaskResendAfterInterruptedCast();
    TestTaskResendInCombatAndBusy();
    TestTaskResendCounterResetsPerStep();
    TestTaskResendKeepsTheLootBot();
    TestTaskResendNotForOtherFailures();
    TestWaitObjectTimeoutIsBounded();
    TestUnsupportedPausesAndSkip();
    TestPlayerOrderAndRefusalPause();
    TestStepIdleCeiling();
    TestTeamFlags();
    TestOtherTeamBossIsLookedAtAndFoughtUnlessFriendly();
    TestRefusedGatherIsRetriedOnceThenLeftOut();
    TestWipeRegroupsAndWalksBack();
    TestThreeDeadRegroupAfterTheFight();
    TestGatherSummonsAStalledStraggler();
    TestStillBotInsideSummonRangeIsSummoned();
    TestStuckBotInCombatIsNotSummoned();
    TestStuckBotWithAttackersIsNotSummoned();
    TestMovingBotThatDoesNotGainIsSummoned();
    TestComingStragglerIsWaitedFor();
    TestComingStragglerIsWaitedForAtDirectGathers();
    TestComingStragglerIsSummonedAfterTheComingLimit();
    TestStillBotInAFightIsNotWaitedFor();
    TestRegroupSummonsAStuckFarBotEarly();
    TestStuckSummonCountsFromTheGather();
    TestWalkBackRefusalKinds();
    TestBossDownNeedsASeenDeath();
    TestSkipDropsTheRefusedPullTarget();
    TestBossKilledAwayFromHisSpot();
    TestWalkBackFarPointIsApproached();
    TestRefusedPullOnlyForInvalidTarget();
    TestWipeWithTheLeadOnTheGraveyardMap();
    TestNearWipeLatchHoldsThroughTheWipe();
    TestLeadDiesFirstThenTheWipeRegroups();
    TestFightCeilingWithFourDeadRegroups();
    TestStragglerOnAnotherFloorIsSummoned();
    TestResumeFromTheEntranceWalksBack();
    TestGatherGoesBackToTheAnchorAfterARidgeChase();
    TestDirectWalkBackAvoidsUnclearedPacks();
    TestStalledAnchorAndSkippedPacks();
    TestHopWaiverEndsWhenCombatStartsDuringCrowdControl();
    TestPullCrowdControlsTheHealer();
    TestNoCrowdControlWithoutAMageOrPlanList();
    TestCrowdControlMustLand();
    TestKillFirstIsThisFightsOnly();
    TestKillFirstFarIsNotFocusedWipe1Replay();
    TestKillFirstBoundDoesNotFlipFlop();
    TestHeldFocusEndsOnlyWhenItsUnitIsGone();
    TestKillFirstOnABossStepIsNotBounded();
    TestGuardStopsWatchingWhenTheFocusEnds();
    TestGuardWatchesAfterAFocusWipe2Replay();
    TestGuardAfterAFocusIsRateBounded();
    TestCcWindowCountsTheMagesFightTime();
    TestTankGuardsThePartyFromItsStand();
    TestTankGuardReturnsToItsOwnSpot();
    TestTankGuardPull100Replay();
    TestTankGuardLimits();
    TestTankGuardPicksUpMobsOnPets();
    TestEngageT1();
    TestEngageT2();
    TestEngageT3();
    TestEngageT3b();
    TestEngageT4();
    TestEngageT5();
    TestEngageT6();
    TestEngageT7();
    TestEngageT8();
    TestEngageT9();
    TestEngageT10();
    TestEngageT11();
    TestEngageReplayRfcOggleflint();
    TestEngageReplayUldamanStep85();
    TestEmptyPlan();
    TestStuckStragglerThatMovedIsSummoned();
    TestApproachHopSummonsBeforePause();
    TestLeadOnlyDeathRecovers();
    TestDeathWhileWalkingBlocksTheStep();
    TestLeadDeathBlocks();
    TestWaitRuleEventException();
    TestNoPullWithADeadMember();
    TestRegroupWaitsForTheFight();
    TestDeathDuringRegroup();
    TestLeadDeadAfterTheFightHolds();
    TestRevivedBotSummonedBeforeWalking();
    TestNoReviveSummonInCombat();
    TestOneReviveSummonPerRevive();
    TestFailedReviveSummonWalks();
    TestRegroupGatherCapScales();
    TestAggroEstimateOffChangesNothing();
    TestAggroEstimateHoldsOnceForARest();
    TestJoinerRestsAreBudgetedAndNotIdleTime();
    TestJoinerRestSlotsAreSpentByRealRestsOnly();
    TestRezHoldOffIsUnchanged();
    TestRezHoldOn();
    TestRezHoldClocks();
    TestRezHoldNudge();
    TestRezNudgeUnreachable();
    TestAggroEstimateAndRezHoldDoNotCrossTheirResets();
    TestBothKeysOffChangeNothing();
    TestPullNoteCarriesTheJoinerCount();
    TestPullShootsFromStand();
    TestPullWaitsForStillTank();
    TestPullWalksToStandOutOfRange();
    TestPullWalksToStandNoLos();
    TestPullStandTriesBounded();
    TestPullNoShotDataKeepsOldOrder();
    TestPickPackPullPrefersShootable();
    TestStandTicksAreNotFruitlessPulls();
    TestShotStandOnPath();
    TestPullStandNeedsMoreRangeThanTheMargin();
    TestStandRequestIsReadOnce();
    TestStandWalkStuckOrdersFromHere();
    TestStandWalkIsDrivenEveryTick();
    TestStuckStandChecksTheWayBackFirst();
    TestPickSwitchChecksTheWayBack();
    TestGossipOrderOnceThenAck();
    TestGossipErrPauses();
    TestGossipNotAllowedNamesTheConfigKey();
    TestGossipNoAnswerPauses();
    TestGossipWaitsForABusyLeadBot();
    TestGossipWalksToFarPointFirst();
    TestGossipStartsAnEvent();
    TestGossipAfterAHoldThenTheDoor();
    TestLeadOnIslandReading();
    TestWalkBackEscapesNavmeshIsland();
    TestIslandPausesAfterTwoEscapes();
    TestResumeEscapesFirst();
    TestWalkBackPointInSightOnIsland();
    TestApproachHopIslandAndZeroHop();
    TestIslandGapIsNotAGoodPath();
    TestEscapeWaitIsNotATry();
    TestEscapeSegmentCheck();
    TestIslandLeavesDirectStepsAlone();
    TestSingleIslandReadingChangesNothing();
    TestSecondReadingConfirmsTheIsland();
    TestWalkBackNoSpotPassesThePoint();
    TestStepEscapeCap();
    TestNoSpotWalkBackNeverSpins();
    TestSignBFromTheMainMeshIsBounded();
    TestWalkBackResendBound();
    TestCeilingPauseResumesByItself();
    std::cout << "test_dungeon_run_policy: ok\n";
    return 0;
}
