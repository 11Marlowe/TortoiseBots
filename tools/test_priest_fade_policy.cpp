// Standalone check for runtime/policy/class/PriestFadePolicy.h:
//   g++ -std=c++17 -Wall -Wextra tools/test_priest_fade_policy.cpp -o /tmp/test_priest_fade && /tmp/test_priest_fade
#include "../runtime/policy/class/PriestFadePolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using TortoiseBots::PriestFadeInputs;
using TortoiseBots::ShouldPriestFadeOnThreat;

int main()
{
    std::cout << "Starting TortoiseBots priest-fade tests...\n";

    // Pull start: the threat ratio reads 100 while the tank holds nothing,
    // but the tank has no threat on the mob, so the climbing-threat Fade
    // stays off.
    {
        PriestFadeInputs in;
        in.mediumThreat = true;
        in.tankHoldsMob = false;
        CHECK(!ShouldPriestFadeOnThreat(in));
    }
    std::cout << "  [PASS] no fade while the tank holds nothing\n";

    // Genuinely climbing past the tank: threat high and the tank holds the
    // mob, so Fade fires.
    {
        PriestFadeInputs in;
        in.mediumThreat = true;
        in.tankHoldsMob = true;
        CHECK(ShouldPriestFadeOnThreat(in));
    }
    std::cout << "  [PASS] fade when climbing with the tank holding the mob\n";

    // Below the threat row: nothing to shed regardless of the tank.
    {
        PriestFadeInputs in;
        in.mediumThreat = false;
        in.tankHoldsMob = true;
        CHECK(!ShouldPriestFadeOnThreat(in));
    }
    std::cout << "  [PASS] no fade below medium threat\n";

    // A mob on the priest sheds through the separate "target of attacker"
    // row, not this one.
    {
        PriestFadeInputs in;
        in.mediumThreat = false;
        in.tankHoldsMob = false;
        CHECK(!ShouldPriestFadeOnThreat(in));
    }
    std::cout << "  [PASS] untouched bystander sheds elsewhere\n";

    std::cout << "All priest-fade tests passed.\n";
    return 0;
}
