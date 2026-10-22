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
    // 1.12 Multi-Shot splash: every rank carries the same 15 yd EffectRadius.
    CHECK(kHunterMultiShotSplashYd == 15.0f);

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

    // A CC'd mob never counts toward the minimum, breakable or not - a sheep,
    // a trapped mob or a stunned one is out of the fight either way.
    {
        // Sheeped packmate beside the target: one fighter + the veto.
        CHECK(!ShouldHunterMultiShot(1, true));
        // Stunned mob out of the count: two real fighters needed.
        CHECK(!ShouldHunterMultiShot(1, false));
        std::cout << "  [PASS] CC'd mobs never count toward the minimum\n";
    }

    std::cout << "All TortoiseBots hunter aoe tests passed!\n";
    return 0;
}
