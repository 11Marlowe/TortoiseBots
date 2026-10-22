// Issue #1166: boost triggers (berserking / ascendance / fade and friends)
// armed every tick while the spell was unknown or on cooldown and the cast
// died in isPossible. The gate lives in ai/playerbot/BoostReadinessPolicy.h;
// these tests pin the pure decision.

#include "../ai/playerbot/BoostReadinessPolicy.h"

#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::BoostReady;
using ai::RacialBoostReady;

int main()
{
    std::cout << "Starting TortoiseBots boost-readiness policy tests...\n";

    // A boost trigger arms only on trained + off cooldown.
    CHECK(BoostReady(true, true));
    CHECK(!BoostReady(false, true));   // never trained (night elf "berserking")
    CHECK(!BoostReady(true, false));   // trained, cooldown running
    CHECK(!BoostReady(false, false));

    // The shared racial row arms when at least one of its spells is castable.
    CHECK(RacialBoostReady(true, false));
    CHECK(RacialBoostReady(false, true));
    CHECK(!RacialBoostReady(false, false));
    CHECK(RacialBoostReady(true, true));

    std::cout << "All boost-readiness policy tests passed.\n";
    return 0;
}
