// Standalone policy test for ai/playerbot/MountShopPolicy.h.
//
// Build and run from the repo root:
//   g++ -std=c++17 -Wall -Wextra tools/test_mount_shop_policy.cpp -o /tmp/test_mount_shop_policy
//   /tmp/test_mount_shop_policy
#include "../ai/playerbot/MountShopPolicy.h"

#include <cstdio>
#include <cstdlib>

namespace
{
    // Stand-ins for the core masks; production passes RACEMASK_ALL_PLAYABLE and
    // CLASSMASK_ALL_PLAYABLE. Only the bits used below matter.
    constexpr uint32_t kAllRaces = 0x3FFu;
    constexpr uint32_t kAllClasses = 0x1FFu;
    constexpr int32_t kAny = -1;
    constexpr uint32_t kHumanBit = 1u << 0;
    constexpr uint32_t kOrcBit = 1u << 1;
    constexpr uint32_t kWarriorBit = 1u << 0;
    constexpr uint32_t kWarlockBit = 1u << 8;

    int checks = 0;

    void Check(bool ok, const char* expr, int line)
    {
        ++checks;
        if (!ok)
        {
            std::fprintf(stderr, "FAIL line %d: %s\n", line, expr);
            std::exit(1);
        }
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

int main()
{
    using namespace MountShopPolicy;

    // Shop: every race and class can use the item.
    CHECK(IsShopMount(46100, kAny, kAny, kAllRaces, kAllClasses));
    CHECK(IsShopMount(46100, 0, 0, kAllRaces, kAllClasses));
    // Not shop: one race excluded, or class-locked.
    CHECK(!IsShopMount(46100, static_cast<int32_t>(kAllRaces & ~kHumanBit), kAny, kAllRaces, kAllClasses));
    CHECK(!IsShopMount(46100, kAny, static_cast<int32_t>(kWarlockBit), kAllRaces, kAllClasses));
    // Never shop: no spell, or the Swift Riding Turtle.
    CHECK(!IsShopMount(0, kAny, kAny, kAllRaces, kAllClasses));
    CHECK(!IsShopMount(TurtleMountSpell, kAny, kAny, kAllRaces, kAllClasses));

    // Racial fit: race and class masks both have to include the bot.
    CHECK(FitsBot(static_cast<int32_t>(kOrcBit), kAny, kOrcBit, kWarriorBit));
    CHECK(!FitsBot(static_cast<int32_t>(kOrcBit), kAny, kHumanBit, kWarriorBit));
    CHECK(FitsBot(0, 0, kHumanBit, kWarriorBit));
    CHECK(FitsBot(kAny, kAny, kHumanBit, kWarriorBit));
    CHECK(!FitsBot(kAny, static_cast<int32_t>(kWarlockBit), kHumanBit, kWarriorBit));
    CHECK(FitsBot(kAny, static_cast<int32_t>(kWarlockBit), kHumanBit, kWarlockBit));

    // Flag on: everything allowed, as before this change.
    CHECK(IsMountAllowed(true, true, false));
    CHECK(IsMountAllowed(true, false, false));
    // Flag off: shop mounts blocked even when they fit.
    CHECK(!IsMountAllowed(false, true, true));
    // Flag off: racial mounts of another race blocked; own-race racial mounts allowed.
    CHECK(!IsMountAllowed(false, false, false));
    CHECK(IsMountAllowed(false, false, true));

    std::printf("PASS test_mount_shop_policy: %d checks\n", checks);
    return 0;
}
