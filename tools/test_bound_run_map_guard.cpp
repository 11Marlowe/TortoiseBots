// Standalone check for ai/playerbot/BoundRunMapGuard.h (pip S2 Deadmines 2026-10-25):
// a player's ghost on map 0 made the bound run's bots follow through the portal,
// entering and leaving map 36 168 times in 3 min. A bot of a bound party whose
// run is running or paused (HoldsItsDead) never leaves the run plan's map by its
// own move; coming onto it (a corpse run, a straggler) stays allowed.
//   g++ -std=c++17 -Wall -Wextra tools/test_bound_run_map_guard.cpp -o /tmp/test_bound_run_map_guard && /tmp/test_bound_run_map_guard
#include "../ai/playerbot/BoundRunMapGuard.h"

#include <cstdio>
#include <cstdlib>

using ai::BoundRunRefusesMapChange;

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
    // Leaving the plan's map by its own move is refused (the pip S2 loop's first half).
    CHECK(BoundRunRefusesMapChange(36, 0, 36));
    // Entering the plan's map stays allowed (a corpse run or straggler coming in).
    CHECK(!BoundRunRefusesMapChange(0, 36, 36));
    // No running or paused run: no guard.
    CHECK(!BoundRunRefusesMapChange(36, 0, 0));
    // No run (map 0 is the sentinel, never a plan's map) and the bot on map 0: still free.
    CHECK(!BoundRunRefusesMapChange(0, 36, 0));
    // A move on the same map is allowed.
    CHECK(!BoundRunRefusesMapChange(36, 36, 36));
    // A bot elsewhere, not on the plan's map, is free.
    CHECK(!BoundRunRefusesMapChange(1, 0, 36));
    std::printf("test_bound_run_map_guard: %d checks passed\n", checks);
    return 0;
}
