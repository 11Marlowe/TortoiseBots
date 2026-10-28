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

    // Disarm: 20 rage and a target that carries a weapon.
    CHECK(ShouldDisarm(true, 20));
    CHECK(ShouldDisarm(true, 100));
    CHECK(!ShouldDisarm(true, 19));   // below the rage cost
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
    CHECK(!ShouldBattleShout(9));    // below the rage cost
    CHECK(!ShouldBattleShout(0));
    std::cout << "  [PASS] battle shout gates on rage\n";
    // Mocking Blow: the second taunt when Taunt is down and the mob left.
    CHECK(ShouldMockingBlow(true, true, false, true, true, 10));
    CHECK(ShouldMockingBlow(true, true, false, true, true, 100));
    CHECK(!ShouldMockingBlow(true, true, true, true, true, 10));  // taunt ready: taunt first
    CHECK(!ShouldMockingBlow(false, true, false, true, true, 10)); // unknown spell
    CHECK(!ShouldMockingBlow(true, false, false, true, true, 10)); // aggro held
    CHECK(!ShouldMockingBlow(true, true, false, false, true, 10)); // mocking on cooldown
    CHECK(!ShouldMockingBlow(true, true, false, true, false, 10)); // out of melee
    CHECK(!ShouldMockingBlow(true, true, false, true, true, 9));  // below the 10 rage cost
    CHECK(!ShouldMockingBlow(true, true, false, true, true, 0));
    std::cout << "  [PASS] mocking blow gates on taunt cooldown, aggro, melee and rage\n";

    // Vanilla cost constants are pinned so a tuning change cannot silently
    // desync the trigger gates from the spell data.
    CHECK(WARRIOR_DISARM_RAGE_COST == 20);
    CHECK(WARRIOR_BATTLE_SHOUT_RAGE_COST == 10);
    CHECK(WARRIOR_BLOODRAGE_RAGE_FLOOR == 20);
    CHECK(WARRIOR_MOCKING_BLOW_RAGE_COST == 10);
    std::cout << "  [PASS] cost constants pinned\n";

    std::cout << "All TortoiseBots warrior rage gate policy tests passed.\n";
    return 0;
}
