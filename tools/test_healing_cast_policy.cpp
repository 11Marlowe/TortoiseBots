#include "../ai/playerbot/HealingCastPolicy.h"

#include <cstdlib>
#include <iostream>
#include <limits>

#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while (0)

int main()
{
    using ai::HealingCastState;
    using ai::ShouldCancelWastefulHeal;
    HealingCastState cast{true, true, 1500, 1000, 1000, 0, 500};
    CHECK(ShouldCancelWastefulHeal(cast)); // another healer topped the target during our cast bar
    cast.preparing = false;
    CHECK(!ShouldCancelWastefulHeal(cast)); // already fired / channeling, mana already spent
    cast.preparing = true;
    cast.remainingMs = 0;
    CHECK(!ShouldCancelWastefulHeal(cast));
    cast.remainingMs = 1;
    CHECK(ShouldCancelWastefulHeal(cast));
    cast.singleTargetDirect = false;
    CHECK(!ShouldCancelWastefulHeal(cast)); // party/chain/HoT or hybrid: one target is insufficient
    cast.singleTargetDirect = true;

    cast.health = 950;
    cast.estimatedHeal = 100;
    CHECK(!ShouldCancelWastefulHeal(cast)); // exactly 50% useful, not a cancellation
    cast.estimatedHeal = 101;
    CHECK(ShouldCancelWastefulHeal(cast));
    cast.health = 900;
    CHECK(!ShouldCancelWastefulHeal(cast)); // exactly 90%: keep healing
    cast.health = 200;
    cast.estimatedHeal = 50000;
    CHECK(!ShouldCancelWastefulHeal(cast)); // never sacrifice an emergency to mana efficiency

    cast.health = 1000;
    cast.incomingDamage = 200;
    CHECK(!ShouldCancelWastefulHeal(cast)); // tank preheal remains useful despite current full health
    cast.incomingDamage = std::numeric_limits<std::uint64_t>::max();
    CHECK(!ShouldCancelWastefulHeal(cast)); // no predicted-health underflow
    cast.incomingDamage = 0;
    cast.health = 0;
    CHECK(!ShouldCancelWastefulHeal(cast));
    cast.health = 1;
    cast.maxHealth = 0;
    CHECK(!ShouldCancelWastefulHeal(cast));
    cast.maxHealth = std::numeric_limits<std::uint32_t>::max();
    cast.health = cast.maxHealth - 1;
    cast.estimatedHeal = 3;
    CHECK(ShouldCancelWastefulHeal(cast)); // percent and efficiency products must not overflow
    cast.estimatedHeal = 0;
    CHECK(!ShouldCancelWastefulHeal(cast)); // unknown heal size: keep a non-full target's heal
    cast.health = cast.maxHealth;
    CHECK(ShouldCancelWastefulHeal(cast)); // known full target doesn't require an amount estimate
    std::cout << "healing cast policy: OK\n";
}
