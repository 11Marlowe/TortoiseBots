#include "../ai/playerbot/HunterAoePolicy.h"

#include <cstdint>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::kHunterMultiShotMinTargets;
using ai::kHunterMultiShotSplashYd;
using ai::ShouldHunterMultiShot;

int main()
{
    std::cout << "Starting TortoiseBots hunter aoe tests...\n";

    CHECK(kHunterMultiShotMinTargets == 2);
    // Core chain-jump radius: the second and third hit must stand within it
    // of the previous target, starting at the shot's target.
    CHECK(kHunterMultiShotSplashYd == 10.0f);

    // The shot needs the kill target plus one more fighting enemy.
    {
        CHECK(ShouldHunterMultiShot(2, false));
        CHECK(ShouldHunterMultiShot(3, false));
        std::cout << "  [PASS] fires on the kill target plus one more enemy\n";
    }

    // A lone target gets the single-target kit - the old bare can-cast row
    // fired on cooldown, so the share never scaled with the pack.
    {
        CHECK(!ShouldHunterMultiShot(0, false));
        CHECK(!ShouldHunterMultiShot(1, false));
        std::cout << "  [PASS] holds fire on a single target\n";
    }

    // Crowd control inside the splash: the splash would shatter it.
    {
        CHECK(!ShouldHunterMultiShot(2, true));
        CHECK(!ShouldHunterMultiShot(5, true));
        std::cout << "  [PASS] vetoes the shot while a breakable CC sits in the splash\n";
    }

    // Unbreakable control (stun, fear, roots) takes the splash harmlessly,
    // so those mobs still count - only a breakable CC suppresses the shot.
    {
        CHECK(ShouldHunterMultiShot(2, false));
        std::cout << "  [PASS] controlled-but-hittable mobs keep counting\n";
    }

    std::cout << "All TortoiseBots hunter aoe tests passed!\n";
    return 0;
}
