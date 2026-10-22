#include "../ai/playerbot/WarriorRageGatePolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace ai;

int main()
{
    std::cout << "Starting TortoiseBots warrior rage gate policy tests...\n";

    // Disarm: 20 rage and a target that carries a weapon (issue 1157).
    CHECK(ShouldDisarm(true, 20));
    CHECK(ShouldDisarm(true, 100));
    CHECK(!ShouldDisarm(true, 19));   // audit: median rage 6 -> row was IMPOSSIBLE
    CHECK(!ShouldDisarm(true, 0));
    CHECK(!ShouldDisarm(false, 100)); // weaponless mob: rage would be wasted
    CHECK(!ShouldDisarm(false, 0));
    std::cout << "  [PASS] disarm gates on rage and weapon\n";

    // Bloodrage: off cooldown, health at least medium, rage below 20.
    CHECK(ShouldBloodrage(true, true, 0));
    CHECK(ShouldBloodrage(true, true, 19));
    CHECK(!ShouldBloodrage(false, true, 0));  // 60 s cooldown running
    CHECK(!ShouldBloodrage(true, false, 0));  // too hurt to pay the health cost
    CHECK(!ShouldBloodrage(true, true, 20));  // already enough rage
    CHECK(!ShouldBloodrage(true, true, 50));
    std::cout << "  [PASS] bloodrage gates on cooldown, health and rage\n";

    // Battle Shout: only at 10+ rage; buff/might logic is BattleShoutPolicy.
    CHECK(ShouldBattleShout(10));
    CHECK(ShouldBattleShout(15));
    CHECK(!ShouldBattleShout(9));    // audit: median rage 1 -> 79 % IMPOSSIBLE
    CHECK(!ShouldBattleShout(0));
    std::cout << "  [PASS] battle shout gates on rage\n";

    // Vanilla cost constants are pinned so a tuning change cannot silently
    // desync the trigger gates from the spell data.
    CHECK(WARRIOR_DISARM_RAGE_COST == 20);
    CHECK(WARRIOR_BATTLE_SHOUT_RAGE_COST == 10);
    CHECK(WARRIOR_BLOODRAGE_RAGE_FLOOR == 20);
    std::cout << "  [PASS] cost constants pinned\n";

    std::cout << "All TortoiseBots warrior rage gate policy tests passed.\n";
    return 0;
}
