// Standalone check for the run leader's event holds after Uldaman run 3
// (19z12, tests/uldaman-events-3): the done condition read on every tick (in a
// fight, an approach and a pause too), the regroup when the lead bot falls in a
// hold, the party's places on the hold spot, and an approach hop not read on
// the tick a walk ended (runtime/DungeonRunPolicy.h: HoldDoneNote,
// HoldDoneWhilePaused, LeadLostInHold, HoldStands, Facts::approachRead).
//   g++ -std=c++17 -Wall -Wextra tools/test_dungeon_hold_policy.cpp -o /tmp/test_dungeon_hold_policy && /tmp/test_dungeon_hold_policy
#include "../runtime/DungeonRunPolicy.h"

#include <cmath>
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
using ai::PlanStepRefusal;

static bool Has(const std::string& text, const char* part)
{
    return text.find(part) != std::string::npos;
}

static PlanStep Move(float x, float y = 0.0f)
{
    PlanStep s;
    s.kind = PlanStepKind::Move;
    s.x = x;
    s.y = y;
    ai::NormalizePlanStep(s);
    return s;
}

// Archaedas: hold at the origin until instance data 2 reads 3 (DONE), an
// encounter; Archaedas (2748) is the first entry, the statues follow.
static PlanStep ArchaedasHold(float radius = 45.0f)
{
    PlanStep s;
    s.kind = PlanStepKind::Hold;
    s.entry = 2;
    s.param = 3;
    s.flags = ai::kPlanFlagDoneData | ai::kPlanFlagEncounter;
    s.timeoutS = 600;
    s.radius = radius;
    s.entries = {2748, 7309, 7077, 7076, 10120};
    CHECK(ai::NormalizePlanStep(s) == PlanStepRefusal::None);
    return s;
}

// The vault door after it (124369 active).
static PlanStep DoorWait()
{
    PlanStep s;
    s.kind = PlanStepKind::WaitObject;
    s.entry = 124369;
    s.param = 1;
    s.timeoutS = 60;
    s.x = 40.0f;
    CHECK(ai::NormalizePlanStep(s) == PlanStepRefusal::None);
    return s;
}

static Facts Party()
{
    Facts f;
    f.total = 5;
    f.alive = 5;
    f.carrier = "Trian";
    f.seen = true;
    f.eventData = 1; // IN_PROGRESS
    const char* names[] = {"Trian", "Cogtastic", "Quelinator", "Tobbert", "Rundar"};
    const bool ranged[] = {false, false, true, true, true};
    for (int i = 0; i < 5; ++i)
    {
        MemberFact m;
        m.name = names[i];
        m.yd = i == 0 ? 0.0f : 3.0f;
        m.ranged = ranged[i];
        f.members.push_back(m);
    }
    return f;
}

static UnitFact Unit(std::uint32_t entry, float x, float y, bool awake)
{
    UnitFact u;
    u.guid = entry * 100 + static_cast<std::uint64_t>(x + 1000.0f);
    u.entry = entry;
    u.x = x;
    u.y = y;
    u.distLead = std::sqrt(x * x + y * y);
    u.hostile = awake;
    return u;
}

// The lead bot dead in a fight: off the living members, on the dead list.
static void KillLead(Facts& f)
{
    f.leadAlive = false;
    f.alive = 4;
    f.dead = 1;
    f.deadNames = {"Trian"};
    f.members.erase(f.members.begin());
}

static void TestHoldDoneSeenFromAway()
{
    // Run 3 at 11:10: Archaedas died 12 s after the lead bot. The hold rested,
    // then approached its spot from where the lead bot came back to life.
    DungeonPlan plan;
    plan.steps = {Move(-30), ArchaedasHold(), DoorWait()};
    Run r = Start(&plan, 1);
    Facts f = Party();
    f.units = {Unit(2748, 11, 0, false)};
    Tick(r, f);
    CHECK(r.eventOn && r.phase == Phase::Hold);
    // The fight: the lead bot dies, the others fight on.
    f.anyInCombat = true;
    f.units[0].hostile = true;
    KillLead(f);
    Tick(r, f);
    CHECK(r.step == 1 && r.state == State::Running);
    // Archaedas dies in the fight: data 2 reads 3. The hold ends on that tick,
    // in the fight, without the spot in sight.
    f.seen = false;
    f.eventData = 3;
    Tick(r, f);
    // (the note says "fighting" again: the fight goes on under the next step).
    CHECK(r.step == 2 && r.state == State::Running && !r.eventOn);

    // The same from an approach: the lead bot alive far away, the spot out of
    // sight, no fight. No approach hop goes out; the step ends.
    Run a = Start(&plan, 1);
    Facts g = Party();
    g.seen = false;
    g.approachKnown = true;
    g.approachX = -20;
    g.eventData = 3;
    CHECK(Tick(a, g) != "move -20.0 0.0 0.0");
    CHECK(a.step == 2 && Has(a.note, "waiting for object 124369"));
    // ... and from a rest phase.
    Run b = Start(&plan, 1);
    Facts h = Party();
    h.minHealthPct = 30;
    h.eventData = 1;
    CHECK(Tick(b, h) == "rest" && b.phase == Phase::Rest);
    h.eventData = 3;
    Tick(b, h);
    CHECK(b.step == 2);
    // Not done: an approach goes on as before.
    Run c = Start(&plan, 1);
    g.eventData = 1;
    CHECK(Tick(c, g) == "move -20.0 0.0 0.0" && c.step == 1);
    // Done by object: the door state counts the same way out of sight.
    PlanStep door;
    door.kind = PlanStepKind::Hold;
    door.entry = 124367;
    door.param = 1;
    door.flags = ai::kPlanFlagDoneObject;
    door.timeoutS = 600;
    CHECK(ai::NormalizePlanStep(door) == PlanStepRefusal::None);
    DungeonPlan doorPlan;
    doorPlan.steps = {Move(-30), door, Move(30)};
    Run d = Start(&doorPlan, 1);
    Facts k = Party();
    k.seen = false;
    k.objectState = 1;
    Tick(d, k);
    CHECK(d.step == 2);
    CHECK(Has(HoldDoneNote(door, k), "event done: object 124367 is in state 1"));
    CHECK(HoldDoneNote(DoorWait(), k).empty()); // only a hold ends this way
}

static void TestPausedHoldDoneResumes()
{
    DungeonPlan plan;
    plan.steps = {Move(-30), ArchaedasHold(), DoorWait()};
    Run r = Start(&plan, 1);
    Pause(r, "8 approach hops did not bring the party within 60.0 yd of the event spot");
    Facts f = Party();
    f.seen = false;
    // Not done yet: still paused, nothing changes.
    CHECK(!HoldDoneWhilePaused(r, f) && r.state == State::Paused && r.step == 1);
    // Done while paused: the step ends and the run resumes with a regroup.
    f.eventData = 3;
    CHECK(HoldDoneWhilePaused(r, f));
    CHECK(r.state == State::Running && r.step == 2 && r.regroup && r.walkBackDue);
    CHECK(Has(r.note, "event done: data 2 reads 3 while paused (8 approach hops") && Has(r.note, "resumed by itself"));
    // A running run is not touched.
    CHECK(!HoldDoneWhilePaused(r, f));

    // A player's pause stays a pause: the step ends, the note says so.
    Run p = Start(&plan, 1);
    CHECK(PauseForPlayer(p, "move 1 2 3"));
    CHECK(HoldDoneWhilePaused(p, f));
    CHECK(p.state == State::Paused && p.step == 2 && PausedByPlayer(p) && Has(p.note, "event done: data 2 reads 3") &&
        Has(p.note, ".bot run resume goes on with step 3"));

    // `.bot run pause` is a player's pause too, whatever its note says.
    Run q = Start(&plan, 1);
    PausePlayer(q, "paused by .bot run pause");
    CHECK(PausedByPlayer(q) && HoldDoneWhilePaused(q, f) && q.state == State::Paused && q.step == 2);
    // A run's own pause after a resume is not the player's.
    CHECK(Resume(q));
    Pause(q, "a fight lasted more than 600 s");
    CHECK(!PausedByPlayer(q));

    // The hold as the plan's last step: the run is done.
    DungeonPlan last;
    last.steps = {Move(-30), ArchaedasHold()};
    Run l = Start(&last, 1);
    Pause(l, "event not done after 600 s");
    CHECK(HoldDoneWhilePaused(l, f) && l.state == State::Done);
}

static void TestLeadDeadInHoldRegroups()
{
    DungeonPlan plan;
    plan.steps = {Move(-30), ArchaedasHold(), DoorWait()};
    Run r = Start(&plan, 1);
    Facts f = Party();
    f.units = {Unit(2748, 11, 0, false)};
    Tick(r, f);
    CHECK(r.eventOn);
    f.anyInCombat = true;
    KillLead(f);
    Tick(r, f);
    // The fight is over with the lead bot dead and the event not done: a
    // regroup (revive), never an approach hop toward the spot.
    f.anyInCombat = false;
    f.seen = false;
    f.approachKnown = true;
    f.approachX = -20;
    std::string order = Tick(r, f);
    CHECK(r.regroup && order.compare(0, 5, "move ") != 0 && Has(r.note, "the lead bot is dead"));
    CHECK(Tick(r, f) == "revive party");

    // The lead bot back to life away from the spot (its own release): the
    // wipe recovery, not an approach.
    Run a = Start(&plan, 1);
    Facts g = Party();
    g.units = {Unit(2748, 11, 0, false)};
    Tick(a, g);
    g.anyInCombat = true;
    KillLead(g);
    Tick(a, g);
    g = Party();
    g.leadX = -200;
    g.seen = false;
    g.approachKnown = true;
    g.approachX = -150;
    for (MemberFact& m : g.members)
        if (m.name != "Trian")
        {
            m.yd = 200;
            m.x = 0;
        }
    order = Tick(a, g);
    CHECK(a.regroup && order != "move -150.0 0.0 0.0" && Has(a.note, "came back to life away from the party"));

    // A living lead bot with a dead healer inside the event: the hold goes on
    // (the event is not left mid-way for a member).
    Run b = Start(&plan, 1);
    Facts h = Party();
    h.units = {Unit(2748, 11, 0, false)};
    Tick(b, h);
    h.alive = 4;
    h.dead = 1;
    h.deadNames = {"Quelinator"};
    h.members.erase(h.members.begin() + 2);
    Tick(b, h);
    CHECK(!b.regroup && b.phase == Phase::Hold);
}

static void TestHoldStandsPlaceRangedBehindTank()
{
    DungeonPlan plan;
    plan.steps = {Move(-30), ArchaedasHold(), DoorWait()};
    Run r = Start(&plan, 1);
    Facts f = Party();
    // Archaedas (the first entry) 11 yd east of the spot, statues all round.
    f.units = {Unit(2748, 11, 0, false), Unit(7076, -20, 20, false), Unit(7076, -20, -20, false)};
    Tick(r, f);
    CHECK(r.phase == Phase::Hold && r.holdStands.size() == 3 && r.holdStandsGen == 1);
    // Every ranged bot (healer too), never the lead bot or the rogue, west of
    // the tank (away from Archaedas), within kHoldGatherYd of the spot.
    for (const HoldStand& s : r.holdStands)
    {
        CHECK(s.name == "Quelinator" || s.name == "Rundar" || s.name == "Tobbert");
        CHECK(s.x < -4.0f && std::sqrt(s.x * s.x + s.y * s.y) <= kHoldGatherYd + 0.01f);
    }
    // Spread across the line, kHoldSpreadYd apart.
    CHECK(std::fabs(r.holdStands[0].y - r.holdStands[1].y) >= kHoldSpreadYd - 0.01f);
    // The same party: the places stay (no new generation for the glue).
    f.units[0].x = 9;
    Tick(r, f);
    CHECK(r.holdStandsGen == 1);
    // The mage dies: the places change once.
    f.members.erase(f.members.begin() + 3);
    f.alive = 4;
    f.dead = 1;
    f.deadNames = {"Tobbert"};
    Tick(r, f);
    CHECK(r.holdStands.size() == 2 && r.holdStandsGen == 2);
    // The step ends: no places left (the glue lets the bots follow again).
    f.eventData = 3;
    Tick(r, f);
    CHECK(r.step == 2 && r.holdStands.empty());

    // Units that ring the spot (their middle on it): never a stand nearer to a
    // unit than the tank's spot; two units east and west leave the north or
    // south side open.
    Run k = Start(&plan, 1);
    Facts g = Party();
    g.units = {Unit(2748, 15, 0, false), Unit(2748, -15, 0, false)};
    Tick(k, g);
    CHECK(k.holdStands.size() == 3);
    for (const HoldStand& s : k.holdStands)
        for (const UnitFact& unit : g.units)
            CHECK(std::hypot(s.x - unit.x, s.y - unit.y) >= std::hypot(unit.x, unit.y) - 0.01f);
    // The Keepers' hold as in the plan (cold review of #1076): the four statues
    // ring the spot, their middle 3.4 yd off it, and the script wakes each onto
    // the NEAREST player. The old fallback (the move the party came by, south-
    // west) put the ranged 4.6-5.6 yd from the south statue that woke first.
    // Every stand must be at least as far from every statue as the spot is.
    PlanStep keepers = ArchaedasHold(30.0f);
    keepers.entry = 1;
    keepers.entries = {4857};
    keepers.x = 101.5f;
    keepers.y = 271.2f;
    PlanStep altar;
    altar.kind = PlanStepKind::Ritual;
    altar.entry = 130511;
    altar.x = 104.8f;
    altar.y = 272.5f;
    CHECK(ai::NormalizePlanStep(altar) == PlanStepRefusal::None);
    DungeonPlan uld;
    uld.steps = {Move(82.3f, 252.4f), altar, keepers};
    Run u = Start(&uld, 2);
    Facts kf = Party();
    kf.leadX = 101.5f;
    kf.leadY = 271.2f;
    kf.units = {Unit(4857, 99.7f, 259.3f, false), Unit(4857, 117.8f, 267.9f, false), Unit(4857, 109.4f, 286.1f, false),
        Unit(4857, 91.4f, 277.5f, false)};
    Tick(u, kf);
    CHECK(u.phase == Phase::Hold && u.holdStands.size() == 3);
    for (const HoldStand& s : u.holdStands)
    {
        CHECK(std::hypot(s.x - 101.5f, s.y - 271.2f) <= kHoldGatherYd + 0.01f);
        for (const UnitFact& unit : kf.units)
            CHECK(std::hypot(s.x - unit.x, s.y - unit.y) >= std::hypot(101.5f - unit.x, 271.2f - unit.y) - 0.01f);
    }
    // A small hold radius bounds the places too.
    DungeonPlan small;
    small.steps = {Move(0, 30), ArchaedasHold(4.0f)};
    Run t = Start(&small, 1);
    Facts h = Party();
    Tick(t, h);
    CHECK(t.holdStands.size() == 3);
    for (const HoldStand& s : t.holdStands)
        CHECK(s.y > 0.0f && std::sqrt(s.x * s.x + s.y * s.y) <= 4.01f);
    // A pause clears the places (every bot back to its own AI).
    Pause(t, "test");
    CHECK(t.holdStands.empty());
    // No ranged bot: no places.
    Run n = Start(&plan, 1);
    Facts m = Party();
    for (MemberFact& member : m.members)
        member.ranged = false;
    Tick(n, m);
    CHECK(n.phase == Phase::Hold && n.holdStands.empty());
}

// Zul'Farrak's pyramid event (r12-19z29 / r8-19z26): a hold at the origin with
// the five troll entries; instance data 1 reads 7 when done (never in a test).
static PlanStep ZfHold()
{
    PlanStep s;
    s.kind = PlanStepKind::Hold;
    s.entry = 1;
    s.param = 7;
    s.flags = ai::kPlanFlagDoneData;
    s.timeoutS = 600;
    s.radius = 30.0f;
    s.entries = {7787, 7788, 7789, 8876, 8877};
    CHECK(ai::NormalizePlanStep(s) == PlanStepRefusal::None);
    return s;
}

// A hold run past its arrival: the party on its places (gen 1, three ranged
// bots), the tank on stay.
struct FightHold
{
    DungeonPlan plan;
    Run r;
    Facts f;
};

static void StartFightHold(FightHold& h, PlanStep hold, std::uint32_t entry)
{
    h.plan.steps = {Move(-30), hold, DoorWait()};
    h.r = Start(&h.plan, 1);
    h.f = Party();
    h.f.leadIsTank = true;
    h.f.leadStaying = true;
    h.f.units = {Unit(entry, 30, 0, false)};
    Tick(h.r, h.f);
    CHECK(h.r.phase == Phase::Hold && h.r.eventOn && h.r.holdPlaced);
    CHECK(h.r.holdStands.size() == 3 && h.r.holdStandsGen == 1);
}

// An awake unit of the event in the fight, `yd` from the tank.
static UnitFact Engaged(std::uint32_t entry, float x, float y, float yd)
{
    UnitFact u = Unit(entry, x, y, true);
    u.engaged = true;
    u.distLead = yd;
    return u;
}

// Plan ZF-EVENT-HOLD T1: the tank is sent at the first awake troll (the engage
// order ends its stay) and then leaves the spot; the places are let go at once
// (the glue lets the placed bots follow it), and the hold places them again
// only once the tank is back (T3).
static void TestHoldStandsLetGoWhenTheTankLeavesTheSpot()
{
    FightHold h;
    StartFightHold(h, ZfHold(), 7789);
    Run& r = h.r;
    Facts& f = h.f;
    f.anyInCombat = true;
    f.units = {Engaged(7789, 14, 5, 15.0f)};
    std::string const order = Tick(r, f);
    CHECK(order == "attack " + std::to_string(f.units[0].guid) + " Trian");
    CHECK(r.holdUnstayed && Has(r.note, "the tank takes 7789"));
    // On the spot the order changes nothing about the places.
    CHECK(r.holdStands.size() == 3 && r.holdStandsGen == 1 && r.holdPlaced);
    // The tank fights where it stands, then chases 12 yd off.
    f.leadStaying = false;
    f.leadX = 12.0f;
    Tick(r, f);
    CHECK(r.holdStands.empty() && r.holdStandsGen == 2 && !r.holdPlaced);
    CHECK(Has(r.note, "the ranged leave their places") && r.holdReleases == 1);
    // No churn while it stays away: the generation does not move again.
    for (int i = 0; i < 20; ++i)
    {
        f.leadX = 12.0f + static_cast<float>(i);
        Tick(r, f);
    }
    CHECK(r.holdStands.empty() && r.holdStandsGen == 2 && !r.holdPlaced);
}

// T2: a tank that fights on or near the spot keeps the places: no churn, every
// tick, up to exactly kHoldReturnYd (the walk back's own limit).
static void TestHoldStandsKeptWhileTheTankFightsOnTheSpot()
{
    FightHold h;
    StartFightHold(h, ZfHold(), 7789);
    Run& r = h.r;
    Facts& f = h.f;
    f.anyInCombat = true;
    f.units = {Engaged(7789, 14, 5, 15.0f)};
    Tick(r, f);
    f.leadStaying = false;
    for (int i = 0; i < 20; ++i)
    {
        f.leadX = 5.0f;
        Tick(r, f);
    }
    CHECK(r.holdStands.size() == 3 && r.holdStandsGen == 1 && r.holdPlaced);
    f.leadX = kHoldFollowYd; // exactly the walk back's limit is still the spot
    Tick(r, f);
    CHECK(r.holdStands.size() == 3 && r.holdStandsGen == 1 && r.holdPlaced);
    f.leadX = kHoldFollowYd + 0.5f;
    Tick(r, f);
    CHECK(r.holdStands.empty() && r.holdStandsGen == 2);
}

// T3: the fight ends with the tank away: the hold walks it back and, once it
// stands on the spot, places the same three bots again (generation 3).
static void TestHoldStandsBackWhenTheTankIsBack()
{
    FightHold h;
    StartFightHold(h, ZfHold(), 7789);
    Run& r = h.r;
    Facts& f = h.f;
    f.anyInCombat = true;
    f.units = {Engaged(7789, 14, 5, 15.0f)};
    Tick(r, f);
    f.leadStaying = false;
    f.leadX = 12.0f;
    Tick(r, f);
    CHECK(r.holdStands.empty() && r.holdStandsGen == 2);
    // The fight is over; the hold sends the tank back.
    f.anyInCombat = false;
    f.units = {Unit(7789, 14, 5, false)};
    std::string order;
    for (int i = 0; i < 6 && order.compare(0, 5, "move ") != 0; ++i)
        order = Tick(r, f);
    CHECK(order == "move 0.0 0.0 0.0");
    CHECK(r.holdStands.empty() && r.holdStandsGen == 2);
    // It arrives (on stay again).
    f.leadX = 0.0f;
    f.leadStaying = true;
    for (int i = 0; i < 4 && !r.holdPlaced; ++i)
        Tick(r, f);
    CHECK(r.holdPlaced && r.holdStands.size() == 3 && r.holdStandsGen == 3);
    for (const HoldStand& s : r.holdStands)
        CHECK(s.name == "Quelinator" || s.name == "Rundar" || s.name == "Tobbert");
}

// T4: a dead lead bot is not followed: the places stay as they are (the rez
// and regroup rules apply).
static void TestHoldStandsStayWhenTheLeadIsDead()
{
    FightHold h;
    StartFightHold(h, ZfHold(), 7789);
    Run& r = h.r;
    Facts& f = h.f;
    f.anyInCombat = true;
    f.units = {Engaged(7789, 14, 5, 15.0f)};
    Tick(r, f);
    f.leadX = 40.0f;
    KillLead(f);
    Tick(r, f);
    CHECK(r.leadDied || !f.leadAlive);
    CHECK(r.holdStands.size() == 3 && r.holdStandsGen == 1);
}

// T5: only a hold's own fight releases: another step's fight (a stale list
// cannot exist today) and a hold fought before its event is on leave it alone.
static void TestHoldStandsReleaseOnlyInTheHoldStep()
{
    // A pull step's fight, the lead bot 12 yd from the step's point.
    PlanStep pull;
    pull.kind = PlanStepKind::Pull;
    pull.x = 0.0f;
    pull.radius = 10.0f;
    pull.entries = {7787};
    CHECK(ai::NormalizePlanStep(pull) == PlanStepRefusal::None);
    DungeonPlan plan;
    plan.steps = {pull, ZfHold()};
    Run r = Start(&plan, 0);
    r.eventOn = true; // stale
    HoldStand stale;
    stale.name = "Quelinator";
    r.holdStands = {stale};
    r.holdPlaced = true;
    r.holdStandsGen = 4;
    Facts f = Party();
    f.leadIsTank = true;
    f.anyInCombat = true;
    f.leadX = 12.0f;
    f.units = {Engaged(7787, 14, 5, 15.0f)};
    for (int i = 0; i < 3; ++i)
        Tick(r, f);
    CHECK(r.holdStands.size() == 1 && r.holdStandsGen == 4 && r.holdPlaced);

    // The hold step before its event is on (eventOn false): not touched either.
    DungeonPlan plan2;
    plan2.steps = {Move(-30), ZfHold()};
    Run q = Start(&plan2, 1);
    q.holdStands = {stale};
    q.holdPlaced = true;
    q.holdStandsGen = 4;
    q.eventOn = false;
    Facts g = Party();
    g.leadIsTank = true;
    g.seen = false;
    g.anyInCombat = true;
    g.leadX = 12.0f;
    g.units = {Engaged(7787, 14, 5, 15.0f)};
    Tick(q, g);
    CHECK(!q.eventOn || q.holdStandsGen == 4);
    CHECK(q.holdStandsGen == 4 && q.holdStands.size() == 1);
}

// T6: the r12-19z29 phase 2 replay (+105.9 .. +131 s): the tank takes 7789 at
// 15 yd, then 8876 at 8.8 and 7789 at 5; it is 0, 10, 21 and 28 yd off the spot
// while the four ranged stand on their places. By the tick at 21 yd the places
// are let go (they stayed 85 % of the frames in the live run).
static void TestZfPyramidPhase2Replay()
{
    FightHold h;
    StartFightHold(h, ZfHold(), 7789);
    Run& r = h.r;
    Facts& f = h.f;
    f.anyInCombat = true;
    f.units = {Engaged(7789, 14, 5, 15.0f), Engaged(8876, 9, 0, 8.8f), Engaged(7787, 5, 0, 5.0f)};
    std::string const first = Tick(r, f);
    CHECK(first.compare(0, 7, "attack ") == 0 && Has(r.note, "the tank takes"));
    f.leadStaying = false;
    float const away[] = {0.0f, 10.0f, 21.0f, 28.0f};
    for (int i = 0; i < 4; ++i)
    {
        f.leadX = away[i];
        Tick(r, f);
        if (i < 1)
            CHECK(r.holdStands.size() == 3);
        if (i >= 2)
            CHECK(r.holdStands.empty() && !r.holdPlaced);
    }
    CHECK(r.holdStandsGen == 2);
}

// The Uldaman Keepers' and Archaedas' holds (19z12, the stands' origin): a
// tank that leaves the spot by less than kHoldReturnYd for a keeper or a statue
// keeps the ranged on their places, through the whole fight.
static void TestUldamanHoldsKeepTheirPlacesForAShortChase()
{
    for (int variant = 0; variant < 2; ++variant)
    {
        PlanStep hold = variant == 0 ? ArchaedasHold(30.0f) : ArchaedasHold();
        if (variant == 0)
        {
            hold.entry = 1;
            hold.entries = {4857};
        }
        std::uint32_t const entry = variant == 0 ? 4857 : 2748;
        FightHold h;
        StartFightHold(h, hold, entry);
        Run& r = h.r;
        Facts& f = h.f;
        f.anyInCombat = true;
        f.units = {Engaged(entry, 11, 0, 11.0f)};
        std::string const order = Tick(r, f);
        CHECK(order.compare(0, 7, "attack ") == 0);
        f.leadStaying = false;
        float const path[] = {2.0f, 5.0f, 7.0f, 7.9f, 6.0f, 3.0f};
        for (int round = 0; round < 5; ++round)
            for (float x : path)
            {
                f.leadX = x;
                f.leadY = 0.0f;
                Tick(r, f);
                CHECK(r.holdStands.size() == 3 && r.holdStandsGen == 1 && r.holdPlaced);
            }
    }
}

static void TestApproachNotReadIsNotNoPath()
{
    // Run 3, baelog pull 7: the walk back to the stand point ended on a tick
    // whose facts were read during that walk (no approach hop asked for), and
    // the run paused with "no walkable path toward the pack" although the
    // navmesh path from the entrance is complete.
    PlanStep pull;
    pull.kind = PlanStepKind::Pull;
    pull.x = -274.05f;
    pull.y = 138.45f;
    pull.z = -47.3f;
    pull.radius = 13.1f;
    pull.entries = {4852, 4851};
    CHECK(ai::NormalizePlanStep(pull) == PlanStepRefusal::None);
    DungeonPlan plan;
    plan.steps = {Move(-236.5f, 54.9f), pull};
    Run r = Start(&plan, 1);
    Facts f = Party();
    f.leadX = -236.5f;
    f.leadY = 54.9f;
    f.seen = false;
    f.approachKnown = false;
    f.approachRead = false;
    CHECK(Tick(r, f).empty());
    CHECK(r.state == State::Running && Has(r.note, "reading the way") && r.approachHops == 0);
    // The next tick asks the navmesh: the hop goes out.
    f.approachRead = true;
    f.approachKnown = true;
    f.approachX = -249.9f;
    f.approachY = 132.8f;
    f.approachZ = -46.7f;
    CHECK(Tick(r, f) == "move -249.9 132.8 -46.7");
    // Read and no path at all: the pause stays.
    Run n = Start(&plan, 1);
    f.approachKnown = false;
    Tick(n, f);
    CHECK(n.state == State::Paused && Has(n.note, "no walkable path toward the pack"));
}

int main()
{
    TestHoldDoneSeenFromAway();
    TestPausedHoldDoneResumes();
    TestLeadDeadInHoldRegroups();
    TestHoldStandsPlaceRangedBehindTank();
    TestHoldStandsLetGoWhenTheTankLeavesTheSpot();
    TestHoldStandsKeptWhileTheTankFightsOnTheSpot();
    TestHoldStandsBackWhenTheTankIsBack();
    TestHoldStandsStayWhenTheLeadIsDead();
    TestHoldStandsReleaseOnlyInTheHoldStep();
    TestZfPyramidPhase2Replay();
    TestUldamanHoldsKeepTheirPlacesForAShortChase();
    TestApproachNotReadIsNotNoPath();
    std::cout << "test_dungeon_hold_policy: ok\n";
    return 0;
}
