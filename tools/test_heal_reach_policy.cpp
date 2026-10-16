#include "../ai/playerbot/HealReachPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::HealReachWanted;
using ai::HealerCoversAlly;

int main()
{
    std::cout << "Starting TortoiseBots heal reach policy tests...\n";

    // Out of combat: the full-health tank the value always names does not pull
    // the healer away from its leader (Uldaman, full-health party).
    {
        CHECK(!HealReachWanted(false, 100.0f, 90.0f));
        CHECK(!HealReachWanted(false, 90.0f, 90.0f));
        std::cout << "  [PASS] out of combat: no reach to a member at or above almost-full health\n";
    }

    // Out of combat, a member who really is hurt after a fight still gets the walk.
    {
        CHECK(HealReachWanted(false, 89.0f, 90.0f));
        CHECK(HealReachWanted(false, 20.0f, 90.0f));
        std::cout << "  [PASS] out of combat: reach to a hurt member\n";
    }

    // In combat nothing changes: the healer keeps within reach of the tank
    // even before the first hit.
    {
        CHECK(HealReachWanted(true, 100.0f, 90.0f));
        CHECK(HealReachWanted(true, 40.0f, 90.0f));
        std::cout << "  [PASS] in combat: unchanged\n";
    }

    // Issue #904: in combat, another healer that covers the ally means no walk.
    {
        CHECK(!HealReachWanted(true, 100.0f, 90.0f, true));
        CHECK(!HealReachWanted(true, 40.0f, 90.0f, true));
        CHECK(HealReachWanted(true, 40.0f, 90.0f, false));
        std::cout << "  [PASS] in combat: no reach when another healer covers the ally\n";
    }

    // Out of combat the cover flag changes nothing.
    {
        CHECK(HealReachWanted(false, 40.0f, 90.0f, true));
        CHECK(!HealReachWanted(false, 100.0f, 90.0f, true));
        std::cout << "  [PASS] out of combat: cover flag ignored\n";
    }

    // What counts as cover: a living healer on the map, in range and in sight.
    {
        CHECK(HealerCoversAlly(true, true, true, 30.0f, 38.0f, true));
        CHECK(HealerCoversAlly(true, true, true, 38.0f, 38.0f, true));
        CHECK(!HealerCoversAlly(true, true, true, 38.5f, 38.0f, true));  // too far
        CHECK(!HealerCoversAlly(true, true, true, 30.0f, 38.0f, false)); // no line of sight
        CHECK(!HealerCoversAlly(true, false, true, 30.0f, 38.0f, true)); // dead
        CHECK(!HealerCoversAlly(true, true, false, 30.0f, 38.0f, true)); // other map
        CHECK(!HealerCoversAlly(false, true, true, 30.0f, 38.0f, true)); // not a healer
        std::cout << "  [PASS] cover: living healer on the map, in range and sight\n";
    }

    std::cout << "All heal reach policy tests passed.\n";
    return 0;
}
