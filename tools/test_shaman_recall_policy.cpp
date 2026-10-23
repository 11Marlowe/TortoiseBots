#include "../ai/playerbot/policy/class/ShamanRecallPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::TotemicRecallShouldFire;

int main()
{
    std::cout << "Starting shaman totemic-recall policy tests...\n";

    // Idle with totems down: recall fires.
    CHECK(TotemicRecallShouldFire(true, true, false, false, false) == true);
    std::cout << "  [PASS] idle bots recall standing totems\n";

    // Missing spell or no totems: quiet.
    CHECK(TotemicRecallShouldFire(false, true, false, false, false) == false);
    CHECK(TotemicRecallShouldFire(true, false, false, false, false) == false);
    CHECK(TotemicRecallShouldFire(false, false, false, false, false) == false);
    std::cout << "  [PASS] untrained bots and empty fields stay quiet\n";

    // Mana tide down: never destroy the cooldown totem.
    CHECK(TotemicRecallShouldFire(true, true, true, false, false) == false);
    std::cout << "  [PASS] mana tide is never recalled\n";

    // Any combat anywhere: no recall.
    CHECK(TotemicRecallShouldFire(true, true, false, true, false) == false);
    CHECK(TotemicRecallShouldFire(true, true, false, false, true) == false);
    CHECK(TotemicRecallShouldFire(true, true, false, true, true) == false);
    std::cout << "  [PASS] combat anywhere vetoes the recall\n";

    // Fight imminent (pre-pull window): the automatic rows own the standing
    // set there and would rebuild it, so no recall until it closes.
    CHECK(TotemicRecallShouldFire(true, true, false, false, false, true) == false);
    CHECK(TotemicRecallShouldFire(true, true, false, false, false, false) == true);
    std::cout << "  [PASS] prepull wanted vetoes the recall\n";

    // No recall -> re-drop loop out of combat: the rows only drop in combat
    // or the pre-pull window (pinned in test_totem_command_policy.cpp), so
    // the recalled set stays recalled until the next fight. Nothing here
    // latches on combat, so a single combat tick cannot restart a loop.
    CHECK(TotemicRecallShouldFire(true, true, false, false, false, false) == true);
    std::cout << "  [PASS] a quiet out-of-combat recall settles (no re-drop)\n";

    std::cout << "All shaman totemic-recall policy checks PASSED!\n";
    return 0;
}
