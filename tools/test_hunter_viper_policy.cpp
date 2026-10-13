// Standalone check for runtime/HunterViperPolicy.h:
//   g++ -std=c++17 -Wall -Wextra tools/test_hunter_viper_policy.cpp -o /tmp/test_hunter_viper && /tmp/test_hunter_viper
#include "../runtime/HunterViperPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace TortoiseBots;

static HunterViperInputs base(uint8_t mana, bool inViper = false)
{
    HunterViperInputs in;
    in.toggleOn = true;
    in.manaPct = mana;
    in.inViper = inViper;
    return in;
}

int main()
{
    std::cout << "Starting TortoiseBots hunter-viper tests...\n";

    CHECK(HUNTER_VIPER_ENTER_MANA_PCT == 20);
    CHECK(HUNTER_VIPER_EXIT_MANA_PCT == 60);

    // Enter: strictly below 20 %.
    CHECK(ShouldSwitchToViper(base(0)));
    CHECK(ShouldSwitchToViper(base(7)));
    CHECK(ShouldSwitchToViper(base(19)));
    CHECK(!ShouldSwitchToViper(base(20)));
    CHECK(!ShouldSwitchToViper(base(50)));
    std::cout << "  [PASS] enter below 20 %\n";

    // Already in Viper: nothing to cast.
    CHECK(!ShouldSwitchToViper(base(3, true)));

    // Hold: Viper stays until 60 %, then the combat aspect may return.
    CHECK(ViperHoldsCombatAspect(base(3, true)));
    CHECK(ViperHoldsCombatAspect(base(30, true)));
    CHECK(ViperHoldsCombatAspect(base(59, true)));
    CHECK(!ViperHoldsCombatAspect(base(60, true)));
    CHECK(!ViperHoldsCombatAspect(base(100, true)));
    // Not in Viper: the hold never applies, so Hawk is cast as today.
    CHECK(!ViperHoldsCombatAspect(base(3, false)));
    std::cout << "  [PASS] hold the Viper until 60 %\n";

    // Hysteresis: between 20 % and 60 % neither rule flips the aspect.
    for (uint8_t m = 20; m < 60; ++m)
    {
        CHECK(!ShouldSwitchToViper(base(m, false)));
        CHECK(ViperHoldsCombatAspect(base(m, true)));
    }
    std::cout << "  [PASS] no flip between the two thresholds\n";

    // Toggle off: both rules are inert (today's behaviour).
    {
        HunterViperInputs in = base(1, false);
        in.toggleOn = false;
        CHECK(!ShouldSwitchToViper(in));
        in.inViper = true;
        CHECK(!ViperHoldsCombatAspect(in));
    }
    std::cout << "  [PASS] toggle off changes nothing\n";

    // Player aspect order and movement aspects win.
    {
        HunterViperInputs in = base(1, false);
        in.playerAspectOrder = true;
        CHECK(!ShouldSwitchToViper(in));
        in = base(10, true);
        in.playerAspectOrder = true;
        CHECK(!ViperHoldsCombatAspect(in));
        in = base(1, false);
        in.movementAspect = true;
        CHECK(!ShouldSwitchToViper(in));
    }
    std::cout << "  [PASS] explicit orders and Cheetah/Pack are respected\n";

    std::cout << "All hunter-viper tests passed.\n";
    return 0;
}
