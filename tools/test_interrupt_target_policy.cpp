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

int main()
{
    std::cout << "Starting TortoiseBots interrupt-target policy tests...\n";

    // Claim key: one key for every interrupt spell, so a rogue's Kick and a
    // mage's Counterspell never both commit to the same cast bar.
    CHECK(InterruptClaimKey() == "interrupt");
    std::cout << "  [PASS] one shared claim key across interrupt spells\n";

    // Priority casts (crowd control, sleep, fear, heal) outrank plain
    // damage casts; a free cast outranks a claimed one at the same
    // priority (two interrupters on one cast bar waste the second
    // cooldown while a free caster goes unhandled).
    CHECK(InterruptTargetTier(true, false) == 0);   // free priority cast
    CHECK(InterruptTargetTier(false, false) == 1);  // free plain cast
    CHECK(InterruptTargetTier(true, true) == 2);    // claimed priority cast
    CHECK(InterruptTargetTier(false, true) == 3);   // claimed plain cast
    std::cout << "  [PASS] tier order: free priority < free plain < claimed priority < claimed plain\n";

    // A claim only lowers the rank, never rejects: a claimed priority cast
    // still ranks above a claimed plain one, so a hogged claim cannot zero
    // coverage of the dangerous cast.
    CHECK(InterruptTargetTier(true, true) < InterruptTargetTier(false, true));
    std::cout << "  [PASS] claimed priority cast still beats claimed plain cast\n";

    std::cout << "All interrupt-target policy tests passed.\n";
    return 0;
}
