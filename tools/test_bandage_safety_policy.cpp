#include "../ai/playerbot/policy/combat/BandageSafetyPolicy.h"

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
    std::cout << "Starting TortoiseBots bandage safety policy tests...\n";

    // Clear field: bandage may start.
    CHECK(BandageGateCheck(false, false, false, false) == BANDAGE_OK);
    std::cout << "  [PASS] no attacker, no DoT, nothing in melee reach: bandage allowed\n";

    // Recently Bandaged refuses on its own (aura 11196).
    CHECK(BandageGateCheck(true, false, false, false) == BANDAGE_RECENTLY_BANDAGED);
    std::cout << "  [PASS] Recently Bandaged refuses\n";

    // An attacker on the bot refuses.
    CHECK(BandageGateCheck(false, true, false, false) == BANDAGE_ATTACKER_ON_BOT);
    std::cout << "  [PASS] attacker on the bot refuses\n";

    // A damage-over-time aura refuses even with nobody swinging (existing
    // gate): the channel would break anyway.
    CHECK(BandageGateCheck(false, false, true, false) == BANDAGE_PERIODIC_DAMAGE);
    std::cout << "  [PASS] damage over time refuses\n";

    // A hostile in melee reach that has not attacked the bot yet (not on
    // its threat list) must refuse too.
    CHECK(BandageGateCheck(false, false, false, true) == BANDAGE_HOSTILE_IN_MELEE_REACH);
    std::cout << "  [PASS] hostile in melee reach refuses\n";

    // Precedence: the first refusing gate in gate order wins, so the
    // reported reason is stable when several gates hold at once.
    CHECK(BandageGateCheck(true, true, true, true) == BANDAGE_RECENTLY_BANDAGED);
    CHECK(BandageGateCheck(false, true, true, true) == BANDAGE_ATTACKER_ON_BOT);
    CHECK(BandageGateCheck(false, false, true, true) == BANDAGE_PERIODIC_DAMAGE);
    std::cout << "  [PASS] gate order is stable under several refusals\n";

    std::cout << "All bandage safety policy tests passed.\n";
    return 0;
}
