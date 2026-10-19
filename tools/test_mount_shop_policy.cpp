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

    // Exhaustive per-race tests for every playable race (1..10) at levels 39, 40, 59, 60.
    // Assert the allowed set is exactly that race's tier list:
    //   39    -> none from the racial list
    //   40/59 -> normal only
    //   60    -> normal + epic (epic preferred)
    uint32_t const allRacialMounts[] = {
        2411, 2414, 5655, 5656, 12353, 12354, 18776, 18777, 18778, // Human / High Elf
        1132, 5665, 5668, 18796, 18797, 18798,                     // Orc
        5864, 5872, 5873, 18785, 18786, 18787,                     // Dwarf
        8629, 8631, 8632, 18766, 18767, 18902, 80446,              // Night Elf
        13331, 13332, 13333, 13334, 18791,                         // Undead
        15277, 15290, 18793, 18794, 18795,                         // Tauren
        8563, 8595, 13321, 13322, 18772, 18773, 18774,             // Gnome
        8588, 8591, 8592, 18788, 18789, 18790,                     // Troll
        80460, 80461, 80462, 81190, 81191                          // Goblin
    };

    for (uint8_t race = 1; race <= 10; ++race)
    {
        MountItemSpan const normalSpan = GetRacialMounts(race, MountTier::Normal);
        MountItemSpan const epicSpan = GetRacialMounts(race, MountTier::Epic);

        CHECK(!normalSpan.empty());
        CHECK(!epicSpan.empty());

        // Level 39: none from the racial list allowed
        for (uint32_t item : allRacialMounts)
        {
            CHECK(!IsRacialMountAllowed(race, 39, item));
        }

        // Levels 40 and 59: normal only from that race allowed; epic and cross-race disallowed
        uint8_t const normalLevels[] = { 40, 59 };
        for (uint8_t level : normalLevels)
        {
            for (uint32_t item : allRacialMounts)
            {
                bool const isOwnNormal = normalSpan.contains(item);
                CHECK(IsRacialMountAllowed(race, level, item) == isOwnNormal);

                if (isOwnNormal)
                {
                    // Normal racial mount preference is 1; preferred over turtle (0)
                    CHECK(GetMountPreference(race, level, item, 0) == 1);
                    CHECK(GetMountPreference(race, level, item, 0) >
                          GetMountPreference(race, level, TurtleMountItem, TurtleMountSpell));
                }
            }
        }

        // Level 60: normal + epic of that race allowed; cross-race disallowed; epic preferred over normal
        for (uint32_t item : allRacialMounts)
        {
            bool const isOwnNormal = normalSpan.contains(item);
            bool const isOwnEpic = epicSpan.contains(item);
            bool const isAllowed = isOwnNormal || isOwnEpic;

            CHECK(IsRacialMountAllowed(race, 60, item) == isAllowed);

            if (isOwnNormal)
            {
                CHECK(GetMountPreference(race, 60, item, 0) == 1);
                CHECK(GetMountPreference(race, 60, item, 0) >
                      GetMountPreference(race, 60, TurtleMountItem, TurtleMountSpell));
            }
            else if (isOwnEpic)
            {
                CHECK(GetMountPreference(race, 60, item, 0) == 2);
                CHECK(GetMountPreference(race, 60, item, 0) >
                      GetMountPreference(race, 60, TurtleMountItem, TurtleMountSpell));

                // Epic preferred over every normal mount of that race
                for (uint32_t normalItem : normalSpan)
                {
                    CHECK(GetMountPreference(race, 60, item, 0) >
                          GetMountPreference(race, 60, normalItem, 0));
                }
            }
        }
    }

    std::printf("PASS test_mount_shop_policy: %d checks\n", checks);
    return 0;
}
