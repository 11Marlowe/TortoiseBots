#include "../ai/playerbot/RunnerFinishPolicy.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <vector>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace ai;

static RunnerFacts Runner(float healthPct, float distance)
{
    RunnerFacts mob;
    mob.running = true;
    mob.alive = true;
    mob.inPartyFight = true;
    mob.healthPct = healthPct;
    mob.distance = distance;
    return mob;
}

static RunnerChaser Ranged(bool holding)
{
    RunnerChaser bot;
    bot.ranged = true;
    bot.holding = holding;
    bot.spellRange = 30.0f;
    return bot;
}

static RunnerChaser Melee(bool gapCloser, bool holding)
{
    RunnerChaser bot;
    bot.hasGapCloser = gapCloser;
    bot.holding = holding;
    bot.meleeReach = 6.5f;
    return bot;
}

int main()
{
    std::cout << "Starting TortoiseBots runner-finish tests...\n";

    // (1) What counts as a runner: running, alive, in the party's fight, low.
    {
        CHECK(IsFinishableRunner(Runner(15.0f, 10.0f)));
        CHECK(IsFinishableRunner(Runner(2.0f, 10.0f)));
        CHECK(!IsFinishableRunner(Runner(30.0f, 10.0f)));  // threshold is exclusive
        CHECK(!IsFinishableRunner(Runner(60.0f, 10.0f)));
        RunnerFacts still = Runner(10.0f, 10.0f); still.running = false;
        CHECK(!IsFinishableRunner(still));
        RunnerFacts feared = Runner(10.0f, 10.0f); feared.feared = true;
        CHECK(!IsFinishableRunner(feared));                 // our own fear CC
        RunnerFacts cc = Runner(10.0f, 10.0f); cc.ccTarget = true;
        CHECK(!IsFinishableRunner(cc));                     // the bot's CC target / held mark
        CHECK(!ShouldFinishRunner(cc, Ranged(false)));
        RunnerFacts dead = Runner(10.0f, 10.0f); dead.alive = false;
        CHECK(!IsFinishableRunner(dead));
        RunnerFacts stranger = Runner(10.0f, 10.0f); stranger.inPartyFight = false;
        CHECK(!IsFinishableRunner(stranger));
        std::cout << "  [PASS] runner facts\n";
    }

    // (2) Ranged finish runners in spell range, a little more when free.
    {
        CHECK(ShouldFinishRunner(Runner(10.0f, 25.0f), Ranged(false)));
        CHECK(ShouldFinishRunner(Runner(10.0f, 34.0f), Ranged(false)));
        CHECK(!ShouldFinishRunner(Runner(10.0f, 36.0f), Ranged(false)));
        std::cout << "  [PASS] ranged leash\n";
    }

    // (3) Melee: a gap closer allows a short leash, none only what it reaches.
    {
        CHECK(ShouldFinishRunner(Runner(10.0f, 20.0f), Melee(true, false)));
        CHECK(ShouldFinishRunner(Runner(10.0f, kRunnerMeleeLeash), Melee(true, false)));
        CHECK(!ShouldFinishRunner(Runner(10.0f, kRunnerMeleeLeash + 1.0f), Melee(true, false)));
        CHECK(!ShouldFinishRunner(Runner(10.0f, 15.0f), Melee(false, false)));
        CHECK(ShouldFinishRunner(Runner(10.0f, 4.0f), Melee(false, false)));
        // A known but not castable gap closer (Charge in combat, Intercept
        // out of stance, rage or on cooldown) falls back to melee reach.
        RunnerChaser notReady = Melee(false, false);
        CHECK(RunnerLeash(notReady) == notReady.meleeReach);
        CHECK(!ShouldFinishRunner(Runner(10.0f, 12.0f), notReady));
        CHECK(ShouldFinishRunner(Runner(10.0f, 12.0f), Melee(true, false)));
        std::cout << "  [PASS] melee leash\n";
    }

    // (4) A hold: ranged only within spell range, melee never leaves its spot.
    {
        CHECK(ShouldFinishRunner(Runner(10.0f, 30.0f), Ranged(true)));
        CHECK(!ShouldFinishRunner(Runner(10.0f, 32.0f), Ranged(true)));
        CHECK(!ShouldFinishRunner(Runner(10.0f, 15.0f), Melee(true, true)));
        CHECK(ShouldFinishRunner(Runner(10.0f, 4.0f), Melee(true, true)));
        std::cout << "  [PASS] hold respected\n";
    }

    // (5) Healers heal and the tank stays on the pack.
    {
        RunnerChaser healer = Ranged(false); healer.healer = true;
        RunnerChaser tank = Melee(true, false); tank.tank = true;
        CHECK(RunnerLeash(healer) == 0.0f);
        CHECK(RunnerLeash(tank) == 0.0f);
        CHECK(!ShouldFinishRunner(Runner(10.0f, 3.0f), healer));
        CHECK(!ShouldFinishRunner(Runner(10.0f, 3.0f), tank));
        std::cout << "  [PASS] healer and tank stay out\n";
    }

    // (6) The current runner is never dropped for a fresh one.
    {
        RunnerFacts current = Runner(25.0f, 20.0f);
        RunnerFacts fresh = Runner(5.0f, 5.0f);
        CHECK(RunnerBetter(current, true, fresh, false));
        CHECK(!RunnerBetter(fresh, false, current, true));
        // Without a current runner: lower health, then nearer.
        CHECK(RunnerBetter(fresh, false, current, false));
        RunnerFacts nearer = Runner(25.0f, 10.0f);
        CHECK(RunnerBetter(nearer, false, current, false));
        std::cout << "  [PASS] current runner kept\n";
    }

    // (7) Chases stop at ledges (owner 2026-10-23, Sunken Temple r26 slot 6b:
    // the lead bot went from z -172.9 to -185.7 in 5 s onto an island with no
    // way back), scoped by the review of 565751aa: run bots only, never on an
    // explicit order, and only on a cliff or an incomplete path.
    {
        // The drop: the lower of the mob and the end of its run.
        CHECK(RunnerDropYd(-172.9f, -172.9f, false, 0.0f) == 0.0f);
        CHECK(std::fabs(RunnerDropYd(-172.9f, -173.0f, true, -185.7f) - 12.8f) < 0.01f);
        CHECK(std::fabs(RunnerDropYd(-172.9f, -185.7f, true, -172.0f) - 12.8f) < 0.01f); // coming back up: where it is
        CHECK(RunnerDropYd(-172.9f, -170.0f, true, -160.0f) < 0.0f);                       // above the bot: no drop

        RunnerFacts below = Runner(15.0f, 8.0f);
        below.dropYd = 12.8f;
        CHECK(RunnerOverLedge(below));
        RunnerFacts edge = Runner(15.0f, 8.0f);
        edge.dropYd = 6.0f; // exactly 6 yd: no candidate
        CHECK(!RunnerOverLedge(edge));
        RunnerFacts standing = below;
        standing.running = false;
        CHECK(!RunnerOverLedge(standing));
        RunnerFacts dead = below;
        dead.alive = false;
        CHECK(!RunnerOverLedge(dead));

        // The way there: a staircase and a slope down are no cliff; a drop over a short run is.
        auto P = [](float x, float z) { RunnerPathPoint p; p.x = x; p.z = z; return p; };
        std::vector<RunnerPathPoint> stairs = {P(0, -172.9f), P(3, -175.0f), P(6, -177.1f), P(9, -179.2f), P(12, -181.3f),
            P(15, -183.4f), P(18, -185.7f)};
        std::vector<RunnerPathPoint> slope = {P(0, -172.9f), P(21, -185.7f)}; // 12.8 yd down over 21 yd
        std::vector<RunnerPathPoint> cliff = {P(0, -172.9f), P(2, -173.0f), P(5, -185.7f), P(10, -186.0f)};
        std::vector<RunnerPathPoint> brink = {P(0, -172.9f), P(4.9f, -179.0f)};             // 6.1 yd over 4.9 yd
        std::vector<RunnerPathPoint> wide = {P(0, -172.9f), P(5.1f, -179.0f)};              // 6.1 yd over 5.1 yd
        std::vector<RunnerPathPoint> step6 = {P(0, -172.9f), P(1, -178.8f)};                // 5.9 yd: no ledge
        CHECK(!PathHasCliff(stairs));
        CHECK(!PathHasCliff(slope));
        CHECK(PathHasCliff(cliff));
        CHECK(PathHasCliff(brink));
        CHECK(!PathHasCliff(wide));
        CHECK(!PathHasCliff(step6));
        std::vector<RunnerPathPoint> up = {P(0, -185.7f), P(2, -172.9f)};                   // a climb is no drop
        CHECK(!PathHasCliff(up));
        CHECK(!PathHasCliff({}));

        // The verdict: run bot, no explicit order, a candidate, and a cliff or no complete path.
        CHECK(!LetRunnerGoAtLedge(true, false, below, true, PathHasCliff(stairs)));  // stairs down: chased
        CHECK(!LetRunnerGoAtLedge(true, false, below, true, PathHasCliff(slope)));   // slope down: chased
        CHECK(LetRunnerGoAtLedge(true, false, below, true, PathHasCliff(cliff)));    // a cliff: let go
        CHECK(LetRunnerGoAtLedge(true, false, below, false, false));                 // no complete path: let go
        CHECK(!LetRunnerGoAtLedge(false, false, below, false, true));                // no run: unchanged
        CHECK(!LetRunnerGoAtLedge(true, true, below, false, true));                  // explicit order: chased
        CHECK(!LetRunnerGoAtLedge(true, false, edge, false, true));                  // not below: chased
        CHECK(!LetRunnerGoAtLedge(true, false, standing, false, true));              // not running

        // The finisher reads the glue's verdict, not the height alone.
        RunnerFacts letGo = below;
        letGo.letGo = true;
        CHECK(!ShouldFinishRunner(letGo, Ranged(false)));
        CHECK(!ShouldFinishRunner(letGo, Melee(true, false)));
        CHECK(ShouldFinishRunner(below, Ranged(false)));       // below on a stair: still finished
        CHECK(ShouldFinishRunner(below, Melee(true, false)));

        // The mark: let go while it holds and the unit is still below.
        CHECK(RunnerStaysLetGo(true, 1000, 31000, 12.0f));
        CHECK(!RunnerStaysLetGo(true, 31000, 31000, 12.0f));     // the mark ran out
        CHECK(!RunnerStaysLetGo(true, 1000, 31000, 6.0f));       // back up on the bot's floor
        CHECK(!RunnerStaysLetGo(false, 1000, 31000, 12.0f));     // never let go
        CHECK(kRunnerLetGoMs == 30000 && kRunnerLedgeReadMs == 1000 && kRunnerCliffRunYd == 5.0f);
        std::cout << "  [PASS] chases stop at ledges (run bots, cliffs)\n";
    }

    // (8) The book: every access drops what ran out, and it never grows past its cap.
    {
        std::map<int, RunnerLedgeEntry> book;
        RunnerLedgeEntry gone;     // both ran out
        gone.letGoUntil = 900;
        gone.readUntil = 950;
        RunnerLedgeEntry read;     // a fresh read only
        read.readUntil = 2000;
        RunnerLedgeEntry held;     // let go
        held.letGoUntil = 31000;
        book[1] = gone;
        book[2] = read;
        book[3] = held;
        PurgeRunnerBook(book, 1000);
        CHECK(book.size() == 2 && !book.count(1) && book.count(2) && book.count(3));
        PurgeRunnerBook(book, 2000); // the read ran out at 2000
        CHECK(book.size() == 1 && book.count(3));
        for (int i = 0; i < 40; ++i)
        {
            RunnerLedgeEntry e;
            e.letGoUntil = 5000 + static_cast<unsigned>(i);
            book[100 + i] = e;
        }
        PurgeRunnerBook(book, 2000);
        CHECK(book.size() == kRunnerBookCap);
        CHECK(book.count(3) && !book.count(100) && book.count(139)); // the soonest to run out went first
        std::cout << "  [PASS] runner book purged and capped\n";
    }

    std::cout << "All runner-finish tests passed.\n";
    return 0;
}
