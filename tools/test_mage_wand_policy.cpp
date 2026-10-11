// Standalone regression test for "a mage with mana casts instead of
// wanding": stop a running wand while the nuke is held back by the
// GCD each Turtle wand shot renews, keep it when the nuke is blocked
// for any other reason.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_mage_wand_policy.cpp -o /tmp/test_mage_wand
//   /tmp/test_mage_wand

#include "../ai/playerbot/MageWandPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace ai;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

int main()
{
    // Wand Shoot (5019) category in Turtle spell data.
    CHECK(MAGE_WAND_SHOOT_CATEGORY == 351);

    // Live case: wanding at full mana, Frostbolt refused only by the
    // wand's GCD -> stop the wand.
    CHECK(ShouldStopWandForNuke(MageWandState{true, MageNukeProbe::GlobalCooldownOnly, false, false}));
    CHECK(ShouldStopWandForNuke(MageWandState{true, MageNukeProbe::Castable, false, false}));

    // Out of mana (or silenced, or no nuke known): the wand is the right tool.
    CHECK(!ShouldStopWandForNuke(MageWandState{true, MageNukeProbe::Blocked, false, false}));

    // Not wanding: nothing to stop.
    CHECK(!ShouldStopWandForNuke(MageWandState{false, MageNukeProbe::Castable, false, false}));
    CHECK(!ShouldStopWandForNuke(MageWandState{false, MageNukeProbe::GlobalCooldownOnly, false, false}));

    // Raid "has aggro -> shoot" wands on purpose: leave it alone.
    CHECK(!ShouldStopWandForNuke(MageWandState{true, MageNukeProbe::Castable, true, false}));
    CHECK(!ShouldStopWandForNuke(MageWandState{true, MageNukeProbe::GlobalCooldownOnly, true, false}));

    // Below the "no mana" line the existing "no mana -> shoot" row wands on
    // purpose; stopping there would only ping-pong with it.
    CHECK(!ShouldStopWandForNuke(MageWandState{true, MageNukeProbe::Castable, false, true}));
    CHECK(!ShouldStopWandForNuke(MageWandState{true, MageNukeProbe::GlobalCooldownOnly, false, true}));

    // Probe combination: the better outcome wins (Frostbolt not known at
    // level 1-3, Fireball is).
    CHECK(BetterNukeProbe(MageNukeProbe::Blocked, MageNukeProbe::GlobalCooldownOnly) == MageNukeProbe::GlobalCooldownOnly);
    CHECK(BetterNukeProbe(MageNukeProbe::GlobalCooldownOnly, MageNukeProbe::Blocked) == MageNukeProbe::GlobalCooldownOnly);
    CHECK(BetterNukeProbe(MageNukeProbe::Castable, MageNukeProbe::Blocked) == MageNukeProbe::Castable);
    CHECK(BetterNukeProbe(MageNukeProbe::GlobalCooldownOnly, MageNukeProbe::Castable) == MageNukeProbe::Castable);
    CHECK(BetterNukeProbe(MageNukeProbe::Blocked, MageNukeProbe::Blocked) == MageNukeProbe::Blocked);

    // Wait after the stop: until the next shot would have fired, plus the
    // react delay; never more than one swing.
    CHECK(MageWandStopWaitMs(1200, 1800, 100) == 1300);
    CHECK(MageWandStopWaitMs(0, 1800, 100) == 100);
    CHECK(MageWandStopWaitMs(5000, 1800, 100) == 1900);
    CHECK(MageWandStopWaitMs(700, 0, 100) == 800);

    std::printf("mage wand policy: %d checks passed\n", checks);
    return 0;
}
