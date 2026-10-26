// Standalone check for the run leader's windowed wipe and lead-death counts (turtle-agent #264, Part A;
// runtime/DungeonRunPolicy.h; the evidence is RFC 2026-10-20 run 2b: the lead bot is resurrected 10-13 s after
// each death, so the party is never all dead in one tick and the one-tick rule never fires):
//   g++ -std=c++17 -Wall -Wextra tools/test_run_wipe_count_policy.cpp -o /tmp/tw && /tmp/tw
#include "../runtime/DungeonRunPolicy.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

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

static PlanStep Pull(float x)
{
    PlanStep s;
    s.kind = PlanStepKind::Pull;
    s.x = x;
    s.entries = {4851};
    ai::NormalizePlanStep(s);
    return s;
}

// A party of 5 named Tank (the lead), Heal, Mage, Rogue, Hunt. `dead` holds the dead bots' names
// after the edge; the others are the living members. The caller passes `leadDead` consistent with
// "Tank" being in `dead`.
static Facts Deaths(std::vector<std::string> dead, bool leadDead)
{
    static const char* kNames[] = {"Tank", "Heal", "Mage", "Rogue", "Hunt"};
    Facts f;
    f.total = 5;
    f.alive = static_cast<std::uint32_t>(5 - dead.size());
    f.dead = static_cast<std::uint32_t>(dead.size());
    f.leadAlive = !leadDead;
    f.carrier = "Tank";
    f.seen = true;
    f.deadNames = std::move(dead);
    for (const char* n : kNames)
    {
        bool isDead = false;
        for (const std::string& d : f.deadNames)
            if (d == n)
                isDead = true;
        if (!isDead)
        {
            MemberFact m;
            m.name = n;
            m.yd = n == std::string("Tank") ? 0.0f : 3.0f;
            m.x = m.yd;
            f.members.push_back(m);
        }
    }
    return f;
}

// RFC 2026-10-20 run 2b's death edges (t in s): the lead bot Tank dies at 374, 592, 999 and 2370 and
// is back up 10-14 s later, so the party is never all dead in one tick. All five die within 63 s at
// 999-1062 (seen on the 5 s grid at 1000-1065): a wipe the one-tick rule misses. At 2370-2605 only
// the lead's death is 235 s older than the last one: no wipe, four bots dead at the same time, a
// near wipe. One near wipe is at 1035-1070, when 3 or more are dead with the lead alive. The other
// is at 2595-2610. Counts checked by hand against the edge list above.
// Fails before the fix: no NoteWipes, no leadDeaths.
static void TestRfcRun2bDeathsCountOneWindowWipeAndFourLeadDeaths()
{
    DungeonPlan plan;
    plan.steps = {Pull(40)};
    Run r = Start(&plan);
    // Every 5 s, so the window ages as it would live; the dead set holds just after each edge.
    for (std::uint32_t t = 370; t <= 2620; t += 5)
    {
        bool const lead = (t >= 374 && t < 384) || (t >= 592 && t < 604) || (t >= 999 && t < 1009) ||
            (t >= 2370 && t < 2384);
        bool const heal = (t >= 406 && t < 463) || (t >= 1007 && t < 1072) || (t >= 2592 && t < 2614);
        bool const mage = (t >= 1034 && t < 1072) || (t >= 2573 && t < 2614);
        bool const rogue = (t >= 1062 && t < 1072) || (t >= 2584 && t < 2614);
        bool const hunt = (t >= 1035 && t < 1072) || (t >= 2402 && t < 2423) || (t >= 2605 && t < 2616);
        std::vector<std::string> dead;
        if (lead)
            dead.push_back("Tank");
        if (heal)
            dead.push_back("Heal");
        if (mage)
            dead.push_back("Mage");
        if (rogue)
            dead.push_back("Rogue");
        if (hunt)
            dead.push_back("Hunt");
        r.runMs = t * 1000;
        NoteWipes(r, Deaths(dead, lead));
    }
    CHECK(r.wipes == 1);
    CHECK(r.leadDeaths == 4);
    CHECK(r.nearWipes == 2);
}

// Four die at 10-40 s and the lead at 50 s: one wipe; everyone still dead at 55 s must not count again.
static void TestWindowWipeNotCountedTwiceWhenAllAreThenDead()
{
    DungeonPlan plan;
    plan.steps = {Pull(40)};
    Run r = Start(&plan);
    auto tick = [&](std::uint32_t t, std::vector<std::string> dead, bool leadDead)
    {
        r.runMs = t * 1000;
        NoteWipes(r, Deaths(std::move(dead), leadDead));
    };
    tick(10, {"Heal"}, false);
    tick(20, {"Heal", "Mage"}, false);
    tick(30, {"Heal", "Mage", "Rogue"}, false);
    tick(40, {"Heal", "Mage", "Rogue", "Hunt"}, false);
    tick(50, {"Tank", "Heal", "Mage", "Rogue", "Hunt"}, true);
    CHECK(r.wipes == 1);
    tick(55, {"Tank", "Heal", "Mage", "Rogue", "Hunt"}, true);
    // The stale-death guard: Hunt stays dead from the wipe (its death is not after it), the rest revive
    // at 60 s and die again at 110-120 s. Not a wipe: Hunt's death predates the counted one, however
    // recent it still is. The 105 s tick re-arms the latch (Heal's old death has aged out of any window).
    // Without the `<= r.lastWipeMs` guard the second spread counts (all within 90 s of 110 s).
    tick(60, {"Hunt"}, false);
    CHECK(r.wipes == 1);
    tick(105, {"Hunt"}, false);
    tick(110, {"Hunt", "Mage"}, false);
    tick(115, {"Hunt", "Mage", "Rogue"}, false);
    tick(120, {"Heal", "Hunt", "Mage", "Rogue"}, false);
    CHECK(r.wipes == 1);
}

// The lead dies at 0 s and is up at 10 s; the others die at 95-100 s, outside the window: no wipe.
static void TestDeathsOutsideTheWindowAreNoWipe()
{
    DungeonPlan plan;
    plan.steps = {Pull(40)};
    Run r = Start(&plan);
    auto tick = [&](std::uint32_t t, std::vector<std::string> dead, bool leadDead)
    {
        r.runMs = t * 1000;
        NoteWipes(r, Deaths(std::move(dead), leadDead));
    };
    tick(0, {"Tank"}, true);
    tick(10, {}, false);
    CHECK(r.leadDeaths == 1);
    tick(95, {"Heal"}, false);
    tick(97, {"Heal", "Mage"}, false);
    tick(100, {"Heal", "Mage", "Rogue"}, false);
    CHECK(r.wipes == 0);
    CHECK(r.nearWipes == 1);
}

// The staggered deaths of case 2 with the lead alive throughout, but with `members` / `deadNames`
// empty and only counts set: no window wipe fires (names must cover Facts::total), while the
// near-wipe latch still counts. Then an all-dead tick from counts alone: the old rule still counts
// it once (existing tests depend on it).
static void TestAWindowWipeNeedsEveryBotsName()
{
    DungeonPlan plan;
    plan.steps = {Pull(40)};
    Run r = Start(&plan);
    auto tick = [&](std::uint32_t t, std::uint32_t dead, std::uint32_t alive, bool leadAlive)
    {
        Facts f;
        f.total = 5;
        f.dead = dead;
        f.alive = alive;
        f.leadAlive = leadAlive;
        r.runMs = t * 1000;
        NoteWipes(r, f);
    };
    tick(10, 1, 4, true);
    tick(20, 2, 3, true);
    tick(30, 3, 2, true);
    tick(40, 4, 1, true);
    CHECK(r.wipes == 0);
    CHECK(r.nearWipes == 1);
    tick(50, 5, 0, false);
    CHECK(r.wipes == 1);
}

// The done note names the lead deaths.
static void TestDoneNoteNamesLeadDeaths()
{
    DungeonPlan plan;
    plan.steps = {Pull(40)};
    Run r = Start(&plan);
    r.wipes = 2;
    r.nearWipes = 1;
    r.leadDeaths = 4;
    CHECK(DoneNote(r) == "plan done (2 wipes, 1 near wipe, 4 lead deaths)");
    r.wipes = 0;
    r.nearWipes = 0;
    r.leadDeaths = 1;
    CHECK(DoneNote(r) == "plan done (1 lead death)");
}

int main()
{
    TestRfcRun2bDeathsCountOneWindowWipeAndFourLeadDeaths();
    TestWindowWipeNotCountedTwiceWhenAllAreThenDead();
    TestDeathsOutsideTheWindowAreNoWipe();
    TestAWindowWipeNeedsEveryBotsName();
    TestDoneNoteNamesLeadDeaths();
    std::cout << "test_run_wipe_count_policy: ok\n";
    return 0;
}
