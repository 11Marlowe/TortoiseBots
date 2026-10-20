#include "../ai/playerbot/strategy/shaman/ShamanTankPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::ChainLightningPackShouldFire;
using ai::TankMagmaShouldFollow;
using ai::TankShamanEarthShockFreeFire;
using ai::TankShamanPullManaReady;
using ai::TankShamanPullWantsChainLightning;
using ai::TankShamanThreatNeedsShock;
using ai::TankShamanWantsWaterShield;
using ai::TankStoneclawShouldDrop;

int main()
{
    std::cout << "Starting shaman tank policy tests...\n";

    // Pull mana gate: no pull below 80% own mana; non-mana bots exempt.
    CHECK(TankShamanPullManaReady(true, 100) == true);
    CHECK(TankShamanPullManaReady(true, 80) == true);
    CHECK(TankShamanPullManaReady(true, 79) == false);
    CHECK(TankShamanPullManaReady(true, 0) == false);
    CHECK(TankShamanPullManaReady(false, 0) == true);
    std::cout << "  [PASS] pull waits for 80% own mana\n";

    // Shield swap: Lightning Shield on pull, Water Shield at ~66% mana.
    // Before Water Shield is trained there is nothing to swap to.
    CHECK(TankShamanWantsWaterShield(true, 66) == true);
    CHECK(TankShamanWantsWaterShield(true, 30) == true);
    CHECK(TankShamanWantsWaterShield(true, 67) == false);
    CHECK(TankShamanWantsWaterShield(true, 100) == false);
    CHECK(TankShamanWantsWaterShield(false, 10) == false);
    std::cout << "  [PASS] water shield takes over at two thirds mana\n";

    // Chain Lightning: 2+ mobs, both mid-fight and on the pull.
    CHECK(ChainLightningPackShouldFire(0) == false);
    CHECK(ChainLightningPackShouldFire(1) == false);
    CHECK(ChainLightningPackShouldFire(2) == true);
    CHECK(ChainLightningPackShouldFire(5) == true);
    CHECK(TankShamanPullWantsChainLightning(1) == false);
    CHECK(TankShamanPullWantsChainLightning(2) == true);
    std::cout << "  [PASS] chain lightning fires on 2+ mobs\n";

    // Earth Shock fight gate: bosses/elites/raid instances burn it on
    // cooldown; normal 5-man trash stays economical.
    CHECK(TankShamanEarthShockFreeFire(true, false, false) == true);
    CHECK(TankShamanEarthShockFreeFire(false, true, true) == true);
    CHECK(TankShamanEarthShockFreeFire(false, false, false) == false);
    CHECK(TankShamanEarthShockFreeFire(false, true, false) == false);
    std::cout << "  [PASS] earth shock free-fires on fights that need it\n";

    // Threat-lead decision: shock when a member catches up (70%+ of our
    // threat) or a mob is loose; quiet otherwise. Zero own threat with a
    // member on the list means we already lost it.
    CHECK(TankShamanThreatNeedsShock(100.0f, 69.0f, 70, false) == false);
    CHECK(TankShamanThreatNeedsShock(100.0f, 70.0f, 70, false) == true);
    CHECK(TankShamanThreatNeedsShock(100.0f, 120.0f, 70, false) == true);
    CHECK(TankShamanThreatNeedsShock(100.0f, 0.0f, 70, true) == true);
    CHECK(TankShamanThreatNeedsShock(0.0f, 10.0f, 70, false) == true);
    CHECK(TankShamanThreatNeedsShock(0.0f, 0.0f, 70, false) == false);
    std::cout << "  [PASS] threat lead gates economical earth shock\n";

    // Tank Stoneclaw: grouped only, threat-needed or wide pack, empty earth
    // slot. Solo bots keep the panic rule; manual orders always win.
    CHECK(TankStoneclawShouldDrop(false, false, false, false, true, 3) == false);
    CHECK(TankStoneclawShouldDrop(true, false, true, false, true, 3) == false);
    CHECK(TankStoneclawShouldDrop(true, true, false, true, false, 1) == false);
    CHECK(TankStoneclawShouldDrop(true, true, false, false, false, 1) == true);
    CHECK(TankStoneclawShouldDrop(true, false, false, true, true, 5) == false);
    CHECK(TankStoneclawShouldDrop(true, false, false, false, true, 1) == true);
    CHECK(TankStoneclawShouldDrop(true, false, false, false, false, 2) == false);
    CHECK(TankStoneclawShouldDrop(true, false, false, false, false, 3) == true);
    std::cout << "  [PASS] stoneclaw drops on arrival when threat needs it\n";

    // Magma order: Nova first; Magma only as the last resort after Nova
    // with the party still AoEing, or into a wide pack with mana to burn.
    CHECK(TankMagmaShouldFollow(false, true, 3, true) == false);
    CHECK(TankMagmaShouldFollow(true, true, 3, true) == true);
    CHECK(TankMagmaShouldFollow(true, false, 3, true) == false);
    CHECK(TankMagmaShouldFollow(false, true, 4, true) == true);
    CHECK(TankMagmaShouldFollow(true, true, 5, false) == false);
    std::cout << "  [PASS] magma stays the last resort after nova\n";

    std::cout << "All shaman tank policy checks PASSED!\n";
    return 0;
}
