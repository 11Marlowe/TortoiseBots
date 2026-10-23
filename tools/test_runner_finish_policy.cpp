#include "../ai/playerbot/RunnerFinishPolicy.h"

#include <cmath>
#include <cstdlib>
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
    // way back). A runner more than 6 yd below the bot's floor, or running to
    // a point more than 6 yd below it, is let go, by every bot.
    {
        // The drop: the lower of the mob and the end of its run.
        CHECK(RunnerDropYd(-172.9f, -172.9f, false, 0.0f) == 0.0f);
        CHECK(std::fabs(RunnerDropYd(-172.9f, -173.0f, true, -185.7f) - 12.8f) < 0.01f);
        CHECK(std::fabs(RunnerDropYd(-172.9f, -185.7f, true, -172.0f) - 12.8f) < 0.01f); // coming back up: where it is
        CHECK(RunnerDropYd(-172.9f, -170.0f, true, -160.0f) < 0.0f);                       // above the bot: no drop

        RunnerFacts ledge = Runner(15.0f, 8.0f);
        ledge.dropYd = 6.5f;
        CHECK(RunnerOverLedge(ledge));
        CHECK(IsFinishableRunner(ledge));                        // still a runner of the fight ...
        CHECK(!ShouldFinishRunner(ledge, Ranged(false)));        // ... nobody goes after it
        CHECK(!ShouldFinishRunner(ledge, Melee(true, false)));
        RunnerFacts edge = Runner(15.0f, 8.0f);
        edge.dropYd = 6.0f;                                      // exactly 6 yd: a stair, not a ledge
        CHECK(!RunnerOverLedge(edge));
        CHECK(ShouldFinishRunner(edge, Ranged(false)));
        CHECK(ShouldFinishRunner(edge, Melee(true, false)));
        // Any running unit, whatever its health (the tank's victim at 60 %).
        RunnerFacts healthy = Runner(60.0f, 3.0f);
        healthy.dropYd = 13.0f;
        CHECK(RunnerOverLedge(healthy));
        // A unit that stands below (not running) is no runner over a ledge.
        RunnerFacts standing = healthy;
        standing.running = false;
        CHECK(!RunnerOverLedge(standing));
        RunnerFacts dead = healthy;
        dead.alive = false;
        CHECK(!RunnerOverLedge(dead));

        // The mark: let go while it holds and the unit is still below.
        CHECK(RunnerStaysLetGo(true, 1000, 31000, 12.0f));
        CHECK(!RunnerStaysLetGo(true, 31000, 31000, 12.0f));     // the mark ran out
        CHECK(!RunnerStaysLetGo(true, 1000, 31000, 6.0f));       // back up on the bot's floor
        CHECK(!RunnerStaysLetGo(false, 1000, 31000, 12.0f));     // never let go
        CHECK(kRunnerLetGoMs == 30000);
        std::cout << "  [PASS] chases stop at ledges\n";
    }

    std::cout << "All runner-finish tests passed.\n";
    return 0;
}
