#include "../ai/playerbot/WarriorRagePolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::ShouldBattleShoutForRage;
using ai::ShouldBloodrage;
using ai::ShouldDisarm;
using ai::kBattleShoutRageCost;
using ai::kBloodrageRageCeiling;
using ai::kDisarmRageCost;

int main()
{
    std::cout << "Starting TortoiseBots warrior rage policy tests...\n";

    // Costs match the 1.12 spell table: Disarm 20, Battle Shout 10; Bloodrage
    // is only wanted while the tank is starved (< 20 rage).
    CHECK(kDisarmRageCost == 20);
    CHECK(kBattleShoutRageCost == 10);
    CHECK(kBloodrageRageCeiling == 20);
    std::cout << "  [PASS] constants\n";

    // Disarm: the trace showed it newest-verdict IMPOSSIBLE in 99.6 % of the
    // ticks it ran (median rage 6). It needs the full cost, an armed target
    // and the spell off cooldown.
    CHECK(!ShouldDisarm(0, true, true));
    CHECK(!ShouldDisarm(6, true, true));
    CHECK(!ShouldDisarm(19, true, true));
    CHECK(ShouldDisarm(20, true, true));
    CHECK(ShouldDisarm(100, true, true));
    CHECK(!ShouldDisarm(50, false, true));  // beast / unarmed mob: nothing to strip
    CHECK(!ShouldDisarm(50, true, false));  // on its 60 s cooldown
    std::cout << "  [PASS] disarm needs rage, weapon and cooldown\n";

    // Bloodrage: wanted when rage is below the ceiling and health is at or
    // above the medium band; never while the spell is on cooldown (94 %
    // impossible before).
    CHECK(ShouldBloodrage(100, 50, 0, true));
    CHECK(ShouldBloodrage(50, 50, 19, true));
    CHECK(!ShouldBloodrage(49, 50, 0, true));   // too hurt: Bloodrage costs health
    CHECK(!ShouldBloodrage(100, 50, 20, true)); // rage not starved
    CHECK(!ShouldBloodrage(100, 50, 0, false)); // on cooldown
    std::cout << "  [PASS] bloodrage needs starvation, health and cooldown\n";

    // Battle Shout: 10 rage or it is impossible (79 % before).
    CHECK(!ShouldBattleShoutForRage(0));
    CHECK(!ShouldBattleShoutForRage(9));
    CHECK(ShouldBattleShoutForRage(10));
    CHECK(ShouldBattleShoutForRage(60));
    std::cout << "  [PASS] battle shout needs 10 rage\n";

    std::cout << "All warrior rage policy tests passed.\n";
    return 0;
}
