// Standalone check for the run leader's time-sink fixes of 2026-10-23 (runtime/DungeonRunPolicy.h; the evidence is
// TortoiseBots-audit WASTED-TIME-2026-10-23.md sinks 2, 3 and 5 and RUN-LESSONS r26/r27 gnomeregan slot 4):
//   (A) the walk back after a wipe or a resume: never before the run's start step, a straight target out of every
//       live pack's reach, a target moved back when the straight path passes an uncleared pack, and the replay
//       leaving out a stretch of the route that comes back by the point just reached;
//   (B) the rest gate rests to what the next fight needs;
//   g++ -std=c++17 -Wall -Wextra tools/test_run_time_sinks_policy.cpp -o /tmp/t && /tmp/t
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

static PlanStep Move(float x, float y, float z = 0.0f)
{
    PlanStep s;
    s.kind = PlanStepKind::Move;
    s.x = x;
    s.y = y;
    s.z = z;
    ai::NormalizePlanStep(s);
    return s;
}

static PlanStep Direct(float x, float y, float z = 0.0f)
{
    PlanStep s = Move(x, y, z);
    s.kind = PlanStepKind::MoveDirect;
    return s;
}

static PlanStep Pull(float x, float y, std::uint32_t entry, float radius = 15.0f, float z = 0.0f)
{
    PlanStep s;
    s.kind = PlanStepKind::Pull;
    s.x = x;
    s.y = y;
    s.z = z;
    s.radius = radius;
    s.entries = {entry};
    ai::NormalizePlanStep(s);
    return s;
}

static Facts Party(float x, float y = 0.0f, float z = 0.0f)
{
    Facts f;
    f.total = 5;
    f.alive = 5;
    f.leadX = x;
    f.leadY = y;
    f.leadZ = z;
    f.carrier = "Tanky";
    return f;
}

static bool ParseMove(const std::string& order, float& x, float& y, float& z)
{
    return order.compare(0, 5, "move ") == 0 && std::sscanf(order.c_str() + 5, "%f %f %f", &x, &y, &z) == 3;
}

// The glue's part of a walk back, on a flat world with no walls: the step facts for FactStep (seen within kSeeYd, an
// approach hop of kApproachHopYd along the straight line, the straight walk back's path checked against every live
// pack), and every `move` carried out at once. Returns the yards the lead bot walked until the walk back was over
// (or `ticks` ran out); `lowest` gets the lowest plan index a walk back point had.
struct WalkBackSim
{
    float walked = 0.0f;
    std::size_t lowest = static_cast<std::size_t>(-1);
    std::size_t points = 0; // walk back points sent (one `move` each, hops included)
    std::vector<std::string> notes;
};

static WalkBackSim SimulateWalkBack(Run& r, Facts f, int ticks = 4000)
{
    WalkBackSim out;
    bool started = false;
    for (int i = 0; i < ticks && r.state == State::Running; ++i)
    {
        const PlanStep* p = FactStep(r);
        if (!p)
            break;
        float const d = ai::PlanDistance(f.leadX, f.leadY, f.leadZ, p->x, p->y, p->z);
        f.seen = d <= kSeeYd;
        f.approachRead = true;
        f.approachKnown = true;
        float const t = d > kApproachHopYd ? kApproachHopYd / d : 1.0f;
        f.approachX = f.leadX + (p->x - f.leadX) * t;
        f.approachY = f.leadY + (p->y - f.leadY) * t;
        f.approachZ = f.leadZ + (p->z - f.leadZ) * t;
        f.directPathCrossesPack = false;
        if (r.walkBack && r.walkBackDirect && !f.seen)
        {
            std::vector<PathPoint> path(2);
            path[0].x = f.leadX;
            path[0].y = f.leadY;
            path[0].z = f.leadZ;
            path[1].x = p->x;
            path[1].y = p->y;
            path[1].z = p->z;
            f.directPathCrossesPack = PathCrossesUnclearedPack(*r.plan, LiveFrom(r, true), path, f.team,
                UnclearedForPath(r));
        }
        std::string const order = Tick(r, f);
        if (r.walkBack)
        {
            started = true;
            out.lowest = std::min(out.lowest, r.walkBackIdx);
        }
        if (out.notes.empty() || out.notes.back() != r.note)
            out.notes.push_back(r.note);
        float x = 0, y = 0, z = 0;
        if (ParseMove(order, x, y, z))
        {
            out.walked += ai::PlanDistance(f.leadX, f.leadY, f.leadZ, x, y, z);
            f.leadX = x;
            f.leadY = y;
            f.leadZ = z;
            if (r.walkBack)
                ++out.points;
        }
        if (started && !r.walkBack)
            break;
    }
    return out;
}

static bool NoteSeen(const WalkBackSim& s, const std::string& part)
{
    for (const std::string& n : s.notes)
        if (n.find(part) != std::string::npos)
            return true;
    return false;
}

static std::size_t IndexOf(const DungeonPlan& plan, float x, float y)
{
    for (std::size_t i = 0; i < plan.steps.size(); ++i)
        if (plan.steps[i].kind == PlanStepKind::Move && plan.steps[i].x == x && plan.steps[i].y == y)
            return i;
    return static_cast<std::size_t>(-1);
}

// A Gnomeregan-shaped plan (gnomeregan 647 steps, RUN-LESSONS r26 slot 4): the entrance corridor east, a dead-end side
// leg south and back up the same corridor (its pulls at the far end), the main route on east, and the last pull's
// stand right beside its pack (the pull that wiped the party). The route folds back on itself at the side leg.
static DungeonPlan GnomereganShape(std::size_t& wipeStep, std::size_t& sideLegEnd)
{
    DungeonPlan plan;
    plan.name = "gnome-shape";
    for (int i = 0; i <= 30; ++i)
        plan.steps.push_back(Move(10.0f * i, 0.0f)); // corridor 0 -> 300
    for (int i = 1; i <= 30; ++i)
        plan.steps.push_back(Move(300.0f, -10.0f * i)); // side leg down to 300 -300
    plan.steps.push_back(Pull(300.0f, -330.0f, 7001));
    plan.steps.push_back(Pull(310.0f, -340.0f, 7002));
    for (int i = 29; i >= 0; --i)
        plan.steps.push_back(Move(303.0f, -10.0f * i)); // back up the same corridor, 3 yd east of it
    sideLegEnd = plan.steps.size() - 1;
    for (int i = 1; i <= 30; ++i)
    {
        plan.steps.push_back(Move(300.0f + 10.0f * i, 0.0f)); // on east, 300 -> 600
        if (i % 10 == 0)
            plan.steps.push_back(Pull(300.0f + 10.0f * i, 40.0f, 7100 + i)); // packs 40 yd off the corridor
    }
    plan.steps.push_back(Move(608.0f, 0.0f)); // the stand beside the pack that wiped the party
    plan.steps.push_back(Pull(620.0f, 0.0f, 7200));
    wipeStep = plan.steps.size() - 1;
    plan.steps.push_back(Move(700.0f, 0.0f));
    return plan;
}

// The yards a replay of every move point from `from` to the step walks (the old rule: ReplayWalkBack from the point
// nearest the lead bot, each point in turn).
static float RouteYards(const DungeonPlan& plan, std::size_t from, std::size_t to, float fx, float fy)
{
    float yd = 0.0f;
    float x = fx, y = fy, z = 0.0f;
    for (std::size_t i = from; i < to; ++i)
    {
        const PlanStep& p = plan.steps[i];
        if (!IsMovePoint(p))
            continue;
        yd += ai::PlanDistance(x, y, z, p.x, p.y, p.z);
        x = p.x;
        y = p.y;
        z = p.z;
    }
    return yd;
}

// r26 slot 4: the last move point before the wiped step is the stand beside its pack, so a straight walk back to it
// always "passed an uncleared pack" and the replay walked every point from the entrance. Now the straight target is
// the latest point out of every live pack's reach, and the yards walked shrink to the straight line plus the last
// few points.
static void TestWipeWalkBackOnAGnomereganShapedPlan()
{
    std::size_t wipe = 0, side = 0;
    DungeonPlan plan = GnomereganShape(wipe, side);
    Run r = Start(&plan, wipe);
    Facts f = Party(0.0f, 0.0f); // revived at the entrance
    StartWalkBack(r, f);
    CHECK(r.walkBack && r.walkBackDirect);
    // 590 0: the stand at 608 0 and the point at 600 0 lie within the wiping pack's radius 15 + 10 of 620 0.
    CHECK(r.walkBackStart == IndexOf(plan, 590.0f, 0.0f));
    CHECK(r.note.find("lies by an uncleared pack: straight to step") != std::string::npos);
    WalkBackSim const sim = SimulateWalkBack(r, f);
    CHECK(r.state == State::Running && !r.walkBack && r.step == wipe);
    float const oldYd = RouteYards(plan, 0, wipe, 0.0f, 0.0f);
    std::cout << "gnomeregan-shaped wipe walk back: old replay " << oldYd << " yd (" << oldYd / 7.0f / 60.0f
              << " min at 7 yd/s), new " << sim.walked << " yd (" << sim.walked / 7.0f / 60.0f << " min)\n";
    CHECK(oldYd > 1150.0f);
    CHECK(sim.walked < 650.0f); // the straight 600 yd and the stand
}

// The same plan when the straight way is no good (a pack the party skipped lies on it, notCleared): the replay of
// the plan's points leaves out the side leg that comes back by its own start (RouteShortcut), so it walks the corridor
// once, not down and up again.
static void TestReplayLeavesOutTheFoldedSideLeg()
{
    std::size_t wipe = 0, side = 0;
    DungeonPlan plan = GnomereganShape(wipe, side);
    // A pack the run left uncleared halfway along the main corridor: the straight line from the entrance passes it.
    plan.steps.insert(plan.steps.begin() + 15, Pull(150.0f, 5.0f, 7300));
    ++wipe;
    ++side;
    Run r = Start(&plan, wipe);
    r.notCleared.insert(15);
    Facts f = Party(0.0f, 0.0f);
    StartWalkBack(r, f);
    WalkBackSim const sim = SimulateWalkBack(r, f);
    CHECK(r.state == State::Running && !r.walkBack && r.step == wipe);
    CHECK(NoteSeen(sim, "replaying the plan's points") || NoteSeen(sim, "left out"));
    float const oldYd = RouteYards(plan, 0, wipe, 0.0f, 0.0f);
    std::cout << "gnomeregan-shaped replay with the side leg: old " << oldYd << " yd, new " << sim.walked << " yd\n";
    CHECK(sim.walked < oldYd - 500.0f); // the 600 yd down and up the side leg are left out
}

// RUN-LESSONS r27 gnomeregan slot 4: a run placed at step 543 wiped at 615 and the walk back replayed from step 53.
// A walk back never goes to a point before the run's start step (Run::startStep, the glue sets it next to Start).
static void TestPlacedRunNeverWalksBackBeforeItsStartStep()
{
    std::size_t wipe = 0, side = 0;
    DungeonPlan plan = GnomereganShape(wipe, side);
    std::size_t const start = side + 1; // placed at the main route's first point
    Run r = Start(&plan, wipe);
    r.startStep = start;
    r.notCleared.insert(wipe - 3); // even with a skipped pack that sends it into the replay
    Facts f = Party(0.0f, 0.0f);
    CHECK(ReplayFloor(r) == start);
    StartWalkBack(r, f);
    CHECK(r.walkBack && r.walkBackIdx >= start);
    WalkBackSim const sim = SimulateWalkBack(r, f);
    CHECK(sim.lowest >= start);
    CHECK(r.state == State::Running && r.step == wipe);
    // The pre-start steps count as uncleared for the straight path (UnclearedForPath), the run's later ones as before.
    std::unordered_set<std::size_t> const uncleared = UnclearedForPath(r);
    CHECK(uncleared.count(0) && uncleared.count(start - 1) && !uncleared.count(start) && uncleared.count(wipe - 3));
    // From the beginning (startStep 0) the floor is the old one.
    Run from0 = Start(&plan, wipe);
    from0.notCleared.insert(wipe - 3);
    CHECK(ReplayFloor(from0) == 0 && UnclearedForPath(from0).size() == 1);
    // Placed at the very step that wiped: no move point of the run before it, so no walk back at all (the step's own
    // approach goes out from the entrance), never one to a point before it.
    Run at = Start(&plan, wipe);
    at.startStep = wipe;
    StartWalkBack(at, Party(0.0f, 0.0f));
    CHECK(!at.walkBack);
}

// The straight path that passes an uncleared pack: its target moves back to a clear point at least kStraightBackoffYd
// before it, kStraightBackoffs times; then the replay as before.
static void TestStraightTargetBacksOffBeforeTheReplay()
{
    DungeonPlan plan;
    for (int i = 0; i <= 20; ++i)
        plan.steps.push_back(Move(20.0f * i, 0.0f)); // 0 .. 400
    plan.steps.push_back(Pull(420.0f, 100.0f, 9));
    Run r = Start(&plan, plan.steps.size() - 1);
    Facts f = Party(-300.0f, 0.0f);
    f.seen = false;
    StartWalkBack(r, f);
    CHECK(r.walkBack && r.walkBackDirect && r.walkBackStart == 20);
    f.approachKnown = true;
    f.approachRead = true;
    f.approachX = -260.0f;
    f.directPathCrossesPack = true;
    Tick(r, f);
    CHECK(r.walkBack && r.walkBackDirect && r.walkBackStart == 18 && r.straightBackoffs == 1);
    CHECK(r.note.find("passes an uncleared pack: straight to step 19 instead (1 of 2)") != std::string::npos);
    Tick(r, f);
    CHECK(r.walkBackStart == 16 && r.straightBackoffs == 2);
    Tick(r, f);
    CHECK(!r.walkBackDirect && r.note.find("replaying the plan's points") != std::string::npos);
}

// RouteShortcut: the latest plain move point within kRouteLinkYd and kRouteLinkDzYd of the point reached; never past a
// direct step, an exact point or an uncleared pack; never onto another floor (Razorfen Kraul r26 slot 8: the z~46
// spawn side under the z~64 floor of Agathelos' pack).
static void TestRouteShortcutRules()
{
    DungeonPlan plan;
    plan.steps = {Move(0, 0), Move(0, -50), Move(0, -100), Move(0, -50), Move(4, 2), Move(50, 0)};
    Run r = Start(&plan, plan.steps.size());
    CHECK(RouteShortcut(r, 0) == 4);  // out and back: 4 yd from point 0
    CHECK(RouteShortcut(r, 1) == 3);  // and the way back's middle
    CHECK(RouteShortcut(r, 4) == 4);  // nothing comes back: the next point in turn
    // Only before the current step.
    Run early = Start(&plan, 3);
    CHECK(RouteShortcut(early, 0) == 0);
    // Not past a direct step.
    DungeonPlan hole = plan;
    hole.steps[2] = Direct(0, -100);
    Run h = Start(&hole, hole.steps.size());
    CHECK(RouteShortcut(h, 0) == 0 && RouteShortcut(h, 1) == 1);
    // Not past an uncleared pack.
    DungeonPlan pack = plan;
    pack.steps.insert(pack.steps.begin() + 2, Pull(0, -105, 5));
    Run p = Start(&pack, pack.steps.size());
    p.notCleared.insert(2);
    CHECK(RouteShortcut(p, 0) == 0);
    p.notCleared.clear();
    CHECK(RouteShortcut(p, 0) == 5);
    // Not onto another floor: 5 yd away on the ground, 18 yd below.
    DungeonPlan pit;
    pit.steps = {Move(0, 0, 64), Move(0, -40, 64), Move(30, -40, 46), Move(5, 0, 46), Move(60, 0, 46)};
    Run q = Start(&pit, pit.steps.size());
    CHECK(RouteShortcut(q, 0) == 0);
    CHECK(GroundDistance(0, 0, 5, 0) <= kRouteLinkYd);
}

// Razorfen Kraul r27 slot 8: the straight walk back's hops toward the den (z 63) climbed a high route to a dead-end
// ledge at z 104 and paused stuck, three times. A straight walk back whose navmesh path is partial (it ends short of
// the point: Facts::approachIncomplete) replays the plan's own points instead (the leg's route down from the z~46 side).
static void TestIncompletePathToTheTargetReplaysThePlansPoints()
{
    DungeonPlan plan;
    plan.steps = {Move(0, 0, 46), Move(40, 0, 46), Move(80, 10, 60), Move(100, 20, 66), Move(110, 40, 64),
        Pull(130, 40, 4519, 15.0f, 64)};
    std::size_t const step = plan.steps.size() - 1;
    auto begin = [&](bool incomplete, Run& r)
    {
        r = Start(&plan, step);
        Facts f = Party(-200, 0, 46); // revived at the entrance side
        StartWalkBack(r, f);
        CHECK(r.walkBack && r.walkBackDirect && r.walkBackIdx == 3); // 110 40 is within 25 yd of the pack
        f.seen = false;
        f.approachRead = true;
        f.approachKnown = true;
        f.approachX = -160;
        f.approachZ = 50;
        f.approachIncomplete = incomplete;
        return Tick(r, f);
    };
    Run r;
    std::string const hop = begin(false, r);
    CHECK(hop == "move -160.0 0.0 50.0" && r.walkBackDirect); // a complete path: the hop as before
    Run q;
    std::string const none = begin(true, q);
    CHECK(none.empty() && q.walkBack && !q.walkBackDirect && q.walkBackIdx == 0);
    CHECK(q.note == "walk back: no complete path to the point of step 4 from here: replaying the plan's points");
}

// The same run when the replay's walk also sticks short of its point (the hops end on the ledge): no pause, the
// stranded lead bot's rescue teleports it to the last point it reached on the plan, out of combat, once a step.
static void TestStuckWalkBackIsRescuedNotPaused()
{
    DungeonPlan plan;
    plan.steps = {Move(0, 0, 46), Move(40, 0, 46), Move(80, 10, 60), Pull(130, 40, 4519, 15.0f, 64)};
    Run r = Start(&plan, 3);
    Facts f = Party(10, 30, 104); // on the ledge, 30+ yd from every point, in sight of point 2
    f.seen = true;
    r.walkBack = true;
    r.walkBackIdx = r.walkBackStart = 2;
    std::string order;
    bool rescue = false;
    for (int i = 0; i < 60 && r.state == State::Running && !rescue; ++i)
    {
        order = Tick(r, f);
        rescue = order.compare(0, 7, "rescue ") == 0;
    }
    CHECK(r.state == State::Running && rescue);
    CHECK(order == "rescue from 10.0 30.0 104.0 to 80.0 10.0 60.0");
    CHECK(r.strandIdx == 2 && r.walkBackIdx == 2);
    // The rescue is done: the lead bot stands on the point and the walk back goes on from there.
    f.leadX = 80;
    f.leadY = 10;
    f.leadZ = 60;
    for (int i = 0; i < 4; ++i)
        Tick(r, f);
    CHECK(r.strandStep == 3 && r.state == State::Running);
    // Stuck again at the same step: the rescue is spent, the old pause.
    Run again = Start(&plan, 3);
    again.strandStep = 3;
    again.walkBack = true;
    again.walkBackIdx = again.walkBackStart = 2;
    Facts g = Party(10, 30, 104);
    g.seen = true;
    for (int i = 0; i < 60 && again.state == State::Running; ++i)
        Tick(again, g);
    CHECK(again.state == State::Paused && again.note.find("the lead bot is stuck") == 0);
}

// The retreat at two dead (AiPlayerbot.DungeonRunRetreatAt2Dead): two dead, the lead bot alive, no engaged unit lost
// hp for kRetreatQuietMs: one `move` to the last stand, once a fight, bounded; off by default; never for an event boss.
static Facts LosingFight(std::uint32_t hp)
{
    Facts f = Party(20, 0);
    f.seen = true;
    f.anyInCombat = true;
    f.leadInCombat = true;
    f.dead = 2;
    f.alive = 3;
    f.engagedRead = true;
    UnitFact u;
    u.guid = 77;
    u.entry = 4519;
    u.attackable = true;
    u.health = hp;
    u.maxHealth = 1000;
    f.engagedUnits = {u};
    return f;
}

static std::string FightFor(Run& r, int ticks, bool falling, bool key, std::uint32_t& hp, int& at)
{
    std::string got;
    at = -1;
    for (int i = 0; i < ticks && r.state == State::Running; ++i)
    {
        Facts f = LosingFight(hp);
        f.retreatAt2Dead = key;
        if (falling)
            hp = hp > 30 ? hp - 30 : 0;
        std::string const o = Tick(r, f);
        if (got.empty() && !o.empty())
        {
            got = o;
            at = i;
        }
    }
    return got;
}

static void TestRetreatAtTwoDead()
{
    DungeonPlan plan;
    plan.steps = {Move(0, 0), Pull(40, 0, 4519)};
    auto fighting = [&](std::uint32_t flags = 0, PlanStepKind kind = PlanStepKind::Pull)
    {
        plan.steps[1].kind = kind;
        plan.steps[1].entry = 4519;
        plan.steps[1].flags = flags;
        Run r = Start(&plan, 1);
        r.anchorKnown = true; // the stand the run walked to before the pull
        r.anchorX = 0;
        r.restDue = false;
        r.fighting = true;
        return r;
    };
    std::uint32_t hp = 900;
    int at = -1;
    Run r = fighting();
    std::string const order = FightFor(r, 15, false, true, hp, at);
    int const first = at;
    CHECK(r.note.find("retreat: 2 dead and no engaged unit lost hp in 15 s: the living pull back to the last stand") == 0);
    CHECK(order == "move 0.0 0.0 0.0" && first == 14); // fightHpMs reached kRetreatQuietMs
    CHECK(FightFor(r, 5, false, true, hp, at) == "move 0.0 0.0 0.0" && at == 4); // sent again once after 5 s
    CHECK(r.retreatOn);
    CHECK(r.note.find("retreat: 2 dead, the living pull back to the last stand at 0.0 0.0 0.0") == 0);
    // Arrived at the stand: over, and not again in this fight.
    Facts there = LosingFight(hp);
    there.retreatAt2Dead = true;
    there.leadX = 0;
    Tick(r, there);
    CHECK(!r.retreatOn && r.retreatDone);
    for (int i = 0; i < 30; ++i)
        CHECK(Tick(r, there).find("move") != 0);
    // The fight over: the next fight may retreat again.
    Facts calm = Party(0);
    calm.seen = true;
    Tick(r, calm);
    CHECK(!r.retreatDone);
    // Off (the default): nothing.
    Run off = fighting();
    hp = 900;
    CHECK(FightFor(off, 30, false, false, hp, at).empty() && !off.retreatOn);
    // A fight still being won (hp falling): no retreat.
    Run winning = fighting();
    hp = 1000;
    CHECK(FightFor(winning, 30, true, true, hp, at).empty());
    // One dead: no retreat.
    Run one = fighting();
    bool moved = false;
    for (int i = 0; i < 30; ++i)
    {
        Facts f = LosingFight(900);
        f.retreatAt2Dead = true;
        f.dead = 1;
        f.alive = 4;
        moved = moved || Tick(one, f).compare(0, 5, "move ") == 0;
    }
    CHECK(!moved);
    // An event boss (an encounter): never.
    Run boss = fighting(ai::kPlanFlagEncounter | ai::kPlanFlagDoneData, PlanStepKind::Boss);
    hp = 900;
    CHECK(FightFor(boss, 30, false, true, hp, at).empty());
    // A plain boss: the retreat as for a pack.
    Run plain = fighting(0, PlanStepKind::Boss);
    hp = 900;
    CHECK(FightFor(plain, 30, false, true, hp, at) == "move 0.0 0.0 0.0");
    // A stand farther than kRetreatMaxYd: none.
    Run far = fighting();
    far.anchorX = -60;
    hp = 900;
    CHECK(FightFor(far, 30, false, true, hp, at).empty());
    // Bounded: never on the stand after kRetreatCapMs, the order sent twice at most.
    Run stuck = fighting();
    hp = 900;
    int moves = 0;
    for (int i = 0; i < 60; ++i)
    {
        Facts f = LosingFight(hp);
        f.retreatAt2Dead = true;
        if (Tick(stuck, f) == "move 0.0 0.0 0.0")
            ++moves;
    }
    CHECK(moves == 2 && !stuck.retreatOn && stuck.retreatDone);
    // A refused retreat ends it without a pause.
    Run refused = fighting();
    hp = 900;
    CHECK(FightFor(refused, 20, false, true, hp, at) == "move 0.0 0.0 0.0");
    OrderRefused(refused, "move 0.0 0.0 0.0", "invalid: in combat");
    CHECK(refused.state == State::Running && !refused.retreatOn && refused.retreatDone);
}

// (B) Rest to what the next step needs (WASTED-TIME sink 3). A fight is won on pull step 0; what the rest gate does
// then depends on the next fight: a small pull or nothing (the light lines), a boss, an event or a big pull (full).
static PlanStep Boss(float x, std::uint32_t entry)
{
    PlanStep s;
    s.kind = PlanStepKind::Boss;
    s.x = x;
    s.entry = entry;
    s.radius = 15.0f;
    ai::NormalizePlanStep(s);
    return s;
}

// The fight on step 0 is over (its pack gone); the ticks after it with `party`'s health and mana. The first order.
static std::string AfterFight(Run& r, Facts party, int ticks = 1)
{
    Facts f = Party(0);
    f.seen = true;
    f.units = {};
    f.anyInCombat = true;
    UnitFact u;
    u.guid = 5;
    u.entry = 1;
    f.units = {u};
    Tick(r, f); // the fight
    party.seen = true;
    std::string got;
    for (int i = 0; i < ticks && got.empty(); ++i)
        got = Tick(r, party);
    return got;
}

static Facts Health(std::uint32_t health, std::uint32_t healerMana = 100, std::uint32_t mana = 100)
{
    Facts f = Party(0);
    f.minHealthPct = health;
    f.minHealerManaPct = healerMana;
    f.minManaPct = mana;
    return f;
}

static void TestRestLinesFollowTheNextFight()
{
    DungeonPlan small;
    small.steps = {Pull(0, 0, 1), Move(30, 0), Pull(60, 0, 2)};
    DungeonPlan boss;
    boss.steps = {Pull(0, 0, 1), Move(30, 0), Boss(60, 3)};
    // 70 % health: within the budget of a small pull, no rest (the old lines rested under 80 %).
    Run a = Start(&small);
    a.fighting = true;
    CHECK(AfterFight(a, Health(70)) != "rest" && a.phase != Phase::Rest);
    // ... before a boss: the full lines, the rest as before.
    Run b = Start(&boss);
    b.fighting = true;
    CHECK(AfterFight(b, Health(70)) == "rest" && b.phase == Phase::Rest && !b.restLight);
    Tick(b, Health(85));
    CHECK(b.phase == Phase::Rest); // under the full end line (90)
    Tick(b, Health(90));
    CHECK(b.phase != Phase::Rest);
    // Under the light start line before a small pull: a rest, ended at the light end line (75), not 90.
    Run c = Start(&small);
    c.fighting = true;
    CHECK(AfterFight(c, Health(50)) == "rest" && c.restLight);
    Tick(c, Health(70));
    CHECK(c.phase == Phase::Rest);
    Tick(c, Health(76));
    CHECK(c.phase != Phase::Rest && !c.restLight);
    // Healer mana: 40 % is within a small pull's budget (35), 30 % rests to 60, never only the healer while the others
    // walk on: the order is the party's `rest`.
    Run d = Start(&small);
    d.fighting = true;
    CHECK(AfterFight(d, Health(100, 40)) != "rest");
    Run e = Start(&small);
    e.fighting = true;
    CHECK(AfterFight(e, Health(100, 30)) == "rest");
    Tick(e, Health(100, 55));
    CHECK(e.phase == Phase::Rest);
    Tick(e, Health(100, 60));
    CHECK(e.phase != Phase::Rest);
    // Nothing but moves ahead: the light lines.
    DungeonPlan walk;
    walk.steps = {Pull(0, 0, 1), Move(30, 0), Move(60, 0)};
    Run w = Start(&walk);
    w.fighting = true;
    CHECK(AfterFight(w, Health(70)) != "rest");
    // A boss beyond the lookahead (kRestLookaheadSteps) does not count yet.
    DungeonPlan far;
    far.steps = {Pull(0, 0, 1)};
    for (std::size_t i = 0; i < kRestLookaheadSteps; ++i)
        far.steps.push_back(Move(10.0f * (i + 1), 0));
    far.steps.push_back(Boss(200, 3));
    Run fr = Start(&far);
    fr.fighting = true;
    CHECK(AfterFight(fr, Health(70)) != "rest");
    // An event ahead (a hold) and a big pull (the aggro estimate's kBigPackJoiners): the full lines.
    DungeonPlan event;
    event.steps = small.steps;
    PlanStep hold;
    hold.kind = PlanStepKind::Hold;
    hold.x = 90;
    hold.radius = 20;
    hold.timeoutS = 60;
    hold.entries = {9};
    ai::NormalizePlanStep(hold);
    event.steps.push_back(hold);
    Run h = Start(&event);
    CHECK(LightRestAhead(h, 1));  // the next fight is the small pull at 60 ...
    CHECK(!LightRestAhead(h, 3)); // ... and after it the hold: the full lines before it
    Run on = Start(&small);
    on.eventOn = true; // inside an event already: the full lines
    CHECK(!LightRestAhead(on, 1));
    Run big = Start(&small);
    big.step = 2;
    big.joinerSeen[44] = kBigPackJoiners;
    CHECK(!LightRestAhead(big, 2));
    big.joinerSeen[44] = kBigPackJoiners - 1;
    CHECK(LightRestAhead(big, 2));
    // A plan `rest` step rests to the full lines whatever comes after it.
    DungeonPlan planned;
    PlanStep rest;
    rest.kind = PlanStepKind::Rest;
    ai::NormalizePlanStep(rest);
    planned.steps = {rest, Pull(60, 0, 2)};
    Run pr = Start(&planned);
    CHECK(Tick(pr, Health(85)) == "rest" && !pr.restLight);
    Tick(pr, Health(85));
    CHECK(pr.phase == Phase::Rest);
}

// The meal wait under the light lines: a bot other than the lead bot still drinking above them does not hold a small
// pull; the lead bot seated or eating does (the pullback would find the tank seated, Uldaman pulls 17 and 61), and so
// does a party under them.
static void TestMealWaitUnderTheLightLines()
{
    DungeonPlan plan;
    plan.steps = {Pull(40, 0, 4852)};
    UnitFact u;
    u.guid = 31;
    u.entry = 4852;
    u.distLead = 30;
    Facts f = Health(82, 70);
    f.seen = true;
    f.units = {u};
    f.eating = 1;
    Run r = Start(&plan);
    CHECK(Tick(r, f) == "pullback 31"); // the healer sips at 70 %: the pull goes out
    Run t = Start(&plan);
    f.leadResting = true;
    CHECK(Tick(t, f).empty() && t.phase == Phase::Rest && t.restForEating);
    // The tank stands up: the wait is over at the light lines, the pull goes out with the healer still drinking.
    f.leadResting = false;
    std::string order;
    for (int i = 0; i < 3 && order.empty(); ++i)
        order = Tick(t, f);
    CHECK(order == "pullback 31");
    // Under the light end lines a drinking bot holds the pull as before.
    Run low = Start(&plan);
    Facts g = Health(82, 50);
    g.seen = true;
    g.units = {u};
    g.eating = 1;
    CHECK(Tick(low, g).empty() && low.phase == Phase::Rest);
    // Before a boss: every drinking bot holds, as before.
    DungeonPlan boss;
    boss.steps = {Boss(40, 4852)};
    Run b = Start(&boss);
    Facts bf = f;
    bf.units.clear();
    CHECK(Tick(b, bf).empty() && b.phase == Phase::Rest && b.restForEating);
}

int main()
{
    TestWipeWalkBackOnAGnomereganShapedPlan();
    TestReplayLeavesOutTheFoldedSideLeg();
    TestPlacedRunNeverWalksBackBeforeItsStartStep();
    TestStraightTargetBacksOffBeforeTheReplay();
    TestRouteShortcutRules();
    TestIncompletePathToTheTargetReplaysThePlansPoints();
    TestStuckWalkBackIsRescuedNotPaused();
    TestRetreatAtTwoDead();
    TestRestLinesFollowTheNextFight();
    TestMealWaitUnderTheLightLines();
    std::cout << "test_run_time_sinks_policy: ok\n";
    return 0;
}
