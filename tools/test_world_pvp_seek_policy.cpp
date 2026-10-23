// Standalone test for the open-world PvP seek role: only free pool bots
// are eligible (never owned/hired bots or bots grouped with a real
// player), and the role is rolled again on the configured interval, never
// while the bot is in combat.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_world_pvp_seek_policy.cpp -o /tmp/test_world_pvp_seek_policy
//   /tmp/test_world_pvp_seek_policy

#include "../runtime/WorldPvpSeekPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static WorldPvpSeekInputs Eligible()
{
    WorldPvpSeekInputs in;
    in.seekEnabled = true;
    in.freePoolBot = true;
    return in;
}

int main()
{
    CHECK(WorldPvpSeekEligible(Eligible()));

    // Seeking off, or not a free pool bot: never.
    {
        WorldPvpSeekInputs in = Eligible();
        in.seekEnabled = false;
        CHECK(!WorldPvpSeekEligible(in));
    }
    {
        WorldPvpSeekInputs in = Eligible();
        in.freePoolBot = false;
        CHECK(!WorldPvpSeekEligible(in));
    }

    // Owned bots, hired bots and bots grouped with a real player never
    // get the role.
    {
        WorldPvpSeekInputs in = Eligible();
        in.ownedBot = true;
        CHECK(!WorldPvpSeekEligible(in));
    }
    {
        WorldPvpSeekInputs in = Eligible();
        in.hiredBot = true;
        CHECK(!WorldPvpSeekEligible(in));
    }
    {
        WorldPvpSeekInputs in = Eligible();
        in.groupedWithRealPlayer = true;
        CHECK(!WorldPvpSeekEligible(in));
    }

    // Battlegrounds keep their own logic.
    {
        WorldPvpSeekInputs in = Eligible();
        in.inBattleground = true;
        CHECK(!WorldPvpSeekEligible(in));
    }

    // Re-rolled on the interval, never mid-fight.
    std::uint64_t const minute = 60 * 1000;
    CHECK(WorldPvpSeekRerollDue(0, 30 * minute, 30, false));
    CHECK(WorldPvpSeekRerollDue(0, 30 * minute + 1, 30, false));
    CHECK(!WorldPvpSeekRerollDue(0, 30 * minute - 1, 30, false));
    CHECK(!WorldPvpSeekRerollDue(1000, 1000 + 60 * minute, 30, true));
    CHECK(!WorldPvpSeekRerollDue(1000, 1000, 30, false));
    CHECK(!WorldPvpSeekRerollDue(2000, 1000, 30, false));
    CHECK(!WorldPvpSeekRerollDue(0, 100 * minute, 0, false));

    std::printf("test_world_pvp_seek_policy: %d checks passed\n", checks);
    return 0;
}
