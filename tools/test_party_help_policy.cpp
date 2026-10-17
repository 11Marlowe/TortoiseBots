// Standalone check for the run leader's help for a party member fighting alone
// on its way back (runtime/DungeonRunPolicy.h, HelpReturning; the owner's rule
// of 2026-10-17). The places are the Uldaman case it comes from
// (tests/uldaman-events-2, pull 58): the tank at -98 311, the revived priest
// and mage fighting a scorpid at -174 311, 76 yd away.
//   g++ -std=c++17 -Wall -Wextra tools/test_party_help_policy.cpp -o /tmp/test_party_help_policy && /tmp/test_party_help_policy
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

static PlanStep Step(PlanStepKind kind, float x, float y, float z)
{
    PlanStep s;
    s.kind = kind;
    s.x = x;
    s.y = y;
    s.z = z;
    ai::NormalizePlanStep(s);
    return s;
}

static PlanStep PullStep(float x, float y, float z)
{
    PlanStep s = Step(PlanStepKind::Pull, x, y, z);
    s.entries = {7011};
    s.radius = 10.0f;
    ai::NormalizePlanStep(s);
    return s;
}

// A plan shaped like the ancient_stone_keeper leg: earlier move points (the
// way the run came), then the pull the party waits at.
static DungeonPlan Plan()
{
    DungeonPlan plan;
    plan.name = "t";
    plan.steps = {
        Step(PlanStepKind::Move, -174.5f, 308.5f, -53.3f), // the scorpid pit (leg 4)
        Step(PlanStepKind::Move, -134.5f, 322.1f, -44.2f), // sentinel leg
        Step(PlanStepKind::Move, -116.0f, 319.3f, -46.0f),
        Step(PlanStepKind::Move, -97.9f, 310.8f, -50.5f),  // the tank's stand
        PullStep(-89.55f, 271.9f, -47.55f),                 // pull 58
    };
    return plan;
}

static MemberFact Member(const std::string& name, float yd, float x, float y, float z)
{
    MemberFact m;
    m.name = name;
    m.yd = yd;
    m.x = x;
    m.y = y;
    m.z = z;
    return m;
}

// The party at the tank, the priest back from the entrance and attacked.
static Facts Party()
{
    Facts f;
    f.total = 5;
    f.alive = 5;
    f.carrier = "Tank";
    f.leadX = -98.0f;
    f.leadY = 311.0f;
    f.leadZ = -51.0f;
    f.seen = true;
    f.members = {Member("Tank", 0, -98, 311, -51), Member("Rogue", 4, -96, 307, -51), Member("Rogue2", 5, -94, 306, -51),
        Member("Mage", 6, -99, 308, -50), Member("Priest", 76, -174, 311, -50)};
    f.maxMemberYd = 76;
    return f;
}

static UnitFact Attacker(std::uint64_t guid, float dist, float x, float y, float z)
{
    UnitFact u;
    u.guid = guid;
    u.entry = 4844;
    u.distLead = dist;
    u.x = x;
    u.y = y;
    u.z = z;
    return u;
}

// A run in its regroup (after a resume), the revive already out.
static Run RegroupingRun(const DungeonPlan& plan)
{
    Run r = Start(&plan, 4);
    r.state = State::Paused;
    CHECK(Resume(r));
    Facts f = Party();
    f.members.back().attackers.clear();
    f.dead = 1;
    f.alive = 4;
    CHECK(Tick(r, f) == "revive party"); // the revive goes out before any help
    return r;
}

// A returning bot attacked: the party goes to it (the navmesh hop when its
// path passes no uncleared pack) and attacks its attackers once near.
static void TestReturningBotAttackedPartyGoes()
{
    DungeonPlan plan = Plan();
    Run r = RegroupingRun(plan);
    Facts f = Party();
    f.members.back().attackers = {11, 12};
    CHECK(Tick(r, f).empty());
    CHECK(r.helpBot == "Priest");
    CHECK(r.note == "regroup: helping Priest (2 attackers)");
    CHECK(r.helpX == -174.0f && r.helpY == 311.0f);
    // Next tick: the attackers and the way there are read.
    f.helpUnits = {Attacker(11, 76, -174, 311, -51), Attacker(12, 78, -176, 313, -51)};
    f.helpHopKnown = true;
    f.helpHopX = -129.0f;
    f.helpHopY = 323.0f;
    f.helpHopZ = -44.0f;
    CHECK(Tick(r, f) == "move -129.0 323.0 -44.0");
    CHECK(r.note == "regroup: helping Priest (2 attackers)");
    // On its way: no new order while it gains ground.
    f.leadX = -115.0f;
    f.leadY = 318.0f;
    CHECK(Tick(r, f).empty());
    // Within kHelpEngageYd: attack the nearest attacker, once.
    f.leadX = -150.0f;
    f.leadY = 320.0f;
    f.helpUnits = {Attacker(11, 25, -174, 311, -51), Attacker(12, 28, -176, 313, -51)};
    CHECK(Tick(r, f) == "attack 11");
    CHECK(Tick(r, f).empty());
    // That one down: the next.
    f.helpUnits = {Attacker(12, 20, -176, 313, -51)};
    CHECK(Tick(r, f) == "attack 12");
    CHECK(r.note == "regroup: helping Priest (1 attacker)");
    // All down: the regroup goes on (its gather is sent again).
    f.helpUnits.clear();
    f.members.back().attackers.clear();
    f.anyInCombat = false;
    CHECK(Tick(r, f).empty());
    CHECK(r.helpBot.empty() && r.regroup);
    CHECK(r.note == "regroup: Priest's attackers are down: going on");
    CHECK(Tick(r, f) == "move -150.0 320.0 -51.0");
    CHECK(r.note.find("regroup: gathering") == 0);
}

// It dies again: the party still clears the attackers it had.
static void TestMemberDiesPartyStillClears()
{
    DungeonPlan plan = Plan();
    Run r = RegroupingRun(plan);
    Facts f = Party();
    f.members.back().attackers = {11};
    Tick(r, f);
    CHECK(r.helpBot == "Priest");
    // The priest dies: no longer among the living members.
    f.members.pop_back();
    f.dead = 1;
    f.alive = 4;
    f.helpUnits = {Attacker(11, 20, -170, 312, -51)};
    CHECK(Tick(r, f) == "attack 11");
    CHECK(r.helpBot == "Priest");
    // Not the revive: the help comes first while its attackers live.
    CHECK(Tick(r, f).empty());
    f.helpUnits.clear();
    // Down: the regroup goes on with the revive of the priest.
    CHECK(Tick(r, f).empty());
    CHECK(r.helpBot.empty() && r.regroup);
    Tick(r, f);
    CHECK(r.note == "regroup: 1 dead, reviving");
}

// An event hold (the Keepers, Archaedas) or the walk into it: never.
static void TestEventHoldDoesNotGo()
{
    DungeonPlan plan;
    plan.name = "t";
    PlanStep hold = Step(PlanStepKind::Hold, -98, 311, -51);
    hold.radius = 30.0f;
    hold.timeoutS = 600;
    hold.entries = {4857};
    ai::NormalizePlanStep(hold);
    plan.steps = {Step(PlanStepKind::Move, -98, 311, -51), hold};
    // At the hold itself, in a regroup.
    Run r = Start(&plan, 1);
    r.state = State::Paused;
    CHECK(Resume(r));
    Facts f = Party();
    f.members.back().attackers = {11};
    for (int i = 0; i < 3; ++i)
    {
        std::string const order = Tick(r, f);
        CHECK(order.compare(0, 7, "attack ") != 0);
        CHECK(r.helpBot.empty());
    }
    // The move into the hold, in a regroup: not either.
    Run r2 = Start(&plan, 0);
    r2.state = State::Paused;
    CHECK(Resume(r2));
    for (int i = 0; i < 3; ++i)
    {
        Tick(r2, f);
        CHECK(r2.helpBot.empty());
    }
}

// Only that bot's attackers are engaged: not the step's pack, not the tank's
// own attacker, not a near bot's; and nothing while the run is not waiting.
static void TestOnlyThatBotsAttackers()
{
    DungeonPlan plan = Plan();
    Run r = RegroupingRun(plan);
    Facts f = Party();
    f.members.back().attackers = {11};
    f.members[1].attackers = {77}; // a rogue at the tank (4 yd): its own fight
    f.units = {Attacker(99, 30, -89, 272, -47)}; // pull 58's pack
    Tick(r, f);
    CHECK(r.helpBot == "Priest");
    CHECK(r.helpGuids.size() == 1 && r.helpGuids[0] == 11);
    // The glue resolves only the remembered guids; a stray one is ignored.
    f.helpUnits = {Attacker(99, 10, -89, 272, -47), Attacker(11, 20, -170, 312, -51)};
    CHECK(Tick(r, f) == "attack 11");
    f.helpUnits = {Attacker(99, 10, -89, 272, -47)};
    CHECK(Tick(r, f).empty());
    CHECK(r.helpBot.empty());
    // A new attacker of the same bot while it still fights joins the help.
    Run r2 = RegroupingRun(plan);
    Facts g = Party();
    g.members.back().attackers = {11};
    Tick(r2, g);
    g.members.back().attackers = {11, 13};
    g.helpUnits = {Attacker(11, 60, -174, 311, -51)};
    g.helpHopKnown = true;
    g.helpHopX = -129.0f;
    g.helpHopY = 323.0f;
    g.helpHopZ = -44.0f;
    Tick(r2, g);
    CHECK(r2.helpGuids.size() == 2 && r2.helpGuids[1] == 13);
    // The lead bot in a fight of its own: no help starts.
    Run r3 = RegroupingRun(plan);
    Facts h = Party();
    h.members.back().attackers = {11};
    h.leadInCombat = true;
    h.anyInCombat = true;
    Tick(r3, h);
    CHECK(r3.helpBot.empty());
    // Out of the run's waits (a step walking on, no regroup): no help.
    Run r4 = Start(&plan, 4);
    Facts k = Party();
    k.members.back().attackers = {11};
    k.maxMemberYd = 10;
    Tick(r4, k);
    CHECK(r4.helpBot.empty());
    // Farther than kHelpMaxYd: no help.
    Run r5 = RegroupingRun(plan);
    Facts m = Party();
    m.members.back().attackers = {11};
    m.members.back().yd = kHelpMaxYd + 1.0f;
    Tick(r5, m);
    CHECK(r5.helpBot.empty());
}

// A path that passes an uncleared pack: the plan's own points instead; none
// on the way: the help ends and the regroup goes on. A refused order ends it
// too, without a pause; and the help is bounded.
static void TestRouteAndBounds()
{
    DungeonPlan plan = Plan();
    Run r = RegroupingRun(plan);
    Facts f = Party();
    f.members.back().attackers = {11};
    Tick(r, f);
    f.helpUnits = {Attacker(11, 76, -174, 311, -51)};
    f.helpHopKnown = true;
    f.helpHopX = -129.0f;
    f.helpHopY = 323.0f;
    f.helpHopZ = -44.0f;
    f.helpPathCrossesPack = true;
    // The plan point nearest the attacker within kSeeYd: -134.5 322.1 (step 2).
    CHECK(Tick(r, f) == "move -134.5 322.1 -44.2");
    // Refused: the help ends, the run does not pause.
    OrderRefused(r, "move -134.5 322.1 -44.2", "unreachable: no path");
    CHECK(r.state == State::Running && r.helpBot.empty());
    CHECK(r.note.find("refused") != std::string::npos);
    // No plan point nearer the attacker: the help ends.
    DungeonPlan bare;
    bare.name = "b";
    bare.steps = {PullStep(-89.55f, 271.9f, -47.55f)};
    Run r2 = Start(&bare, 0);
    r2.state = State::Paused;
    CHECK(Resume(r2));
    Facts g = Party();
    g.members.back().attackers = {11};
    Tick(r2, g);
    CHECK(r2.helpBot == "Priest");
    g.helpUnits = {Attacker(11, 76, -174, 311, -51)};
    g.helpHopKnown = false;
    Tick(r2, g);
    CHECK(r2.helpBot.empty() && r2.note == "regroup: no way to Priest: going on");
    // The ceiling: attackers that keep running home end the help after kHelpCapMs.
    Run r3 = RegroupingRun(plan);
    Facts h = Party();
    h.members.back().attackers = {11};
    Tick(r3, h);
    UnitFact evading = Attacker(11, 76, -174, 311, -51);
    evading.evading = true;
    h.helpUnits = {evading};
    for (std::uint32_t ms = 0; ms <= kHelpCapMs + kTickMs && !r3.helpBot.empty(); ms += kTickMs)
        Tick(r3, h);
    CHECK(r3.helpBot.empty() && r3.state == State::Running);
}

int main()
{
    TestReturningBotAttackedPartyGoes();
    TestMemberDiesPartyStillClears();
    TestEventHoldDoesNotGo();
    TestOnlyThatBotsAttackers();
    TestRouteAndBounds();
    std::cout << "test_party_help_policy: ok\n";
    return 0;
}
