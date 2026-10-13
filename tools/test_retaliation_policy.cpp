#include "../ai/playerbot/RetaliationPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::CountRetaliationMeleeAttackers;
using ai::RetaliationAttacker;
using ai::RetaliationWorthCooldown;

int main()
{
    std::cout << "Starting TortoiseBots retaliation policy tests...\n";

    const RetaliationAttacker meleeOnBot{true, true, true};
    const RetaliationAttacker rangedOnBot{true, true, false};
    const RetaliationAttacker meleeOnOther{true, false, true};
    const RetaliationAttacker deadMeleeOnBot{false, true, true};

    // No attacker, or one melee attacker: keep the 30 min cooldown.
    CHECK(!RetaliationWorthCooldown({}));
    CHECK(!RetaliationWorthCooldown({meleeOnBot}));
    CHECK(!RetaliationWorthCooldown({rangedOnBot, rangedOnBot}));

    // Two live melee attackers on the warrior are enough.
    CHECK(RetaliationWorthCooldown({meleeOnBot, meleeOnBot}));
    CHECK(RetaliationWorthCooldown({meleeOnBot, rangedOnBot, meleeOnBot}));
    std::cout << "  [PASS] two melee attackers on the warrior unlock retaliation\n";

    // Casters, mobs fighting someone else, and corpses do not count.
    CHECK(!RetaliationWorthCooldown({meleeOnBot, rangedOnBot}));
    CHECK(!RetaliationWorthCooldown({meleeOnBot, meleeOnOther}));
    CHECK(!RetaliationWorthCooldown({meleeOnBot, deadMeleeOnBot}));
    CHECK(CountRetaliationMeleeAttackers({meleeOnBot, rangedOnBot, meleeOnOther, deadMeleeOnBot, meleeOnBot}) == 2);
    std::cout << "  [PASS] ranged, off-target and dead attackers are not counted\n";

    std::cout << "All retaliation policy tests passed.\n";
    return 0;
}
