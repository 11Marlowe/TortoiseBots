#include "../ai/playerbot/InterruptTargetPolicy.h"

#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::InterruptClaimKey;
using ai::InterruptTargetTier;
using ai::kInterruptTargetRejected;

int main()
{
    std::cout << "Starting TortoiseBots interrupt-target policy tests...\n";

    // Claim key: one key for every interrupt spell, so a rogue's Kick and a
    // mage's Counterspell never both commit to the same cast bar.
    CHECK(InterruptClaimKey() == "interrupt");
    std::cout << "  [PASS] one shared claim key across interrupt spells\n";

    // Priority casts (crowd control, sleep, fear, heal) outrank plain
    // damage casts...
    CHECK(InterruptTargetTier(true, false) == 0);
    CHECK(InterruptTargetTier(false, false) == 1);
    std::cout << "  [PASS] free casts: priority cast beats plain cast\n";

    // ...but one interrupter per cast is strict: a claimed cast is
    // rejected outright (a second interrupt would waste its cooldown on a
    // cast bar someone else is already stopping), so a bot with one
    // caster only never double-commits, whatever its priority read.
    CHECK(InterruptTargetTier(true, true) == kInterruptTargetRejected);
    CHECK(InterruptTargetTier(false, true) == kInterruptTargetRejected);
    CHECK(kInterruptTargetRejected != 0 && kInterruptTargetRejected != 1);
    std::cout << "  [PASS] claimed casts are rejected outright\n";

    std::cout << "All interrupt-target policy tests passed.\n";
    return 0;
}
