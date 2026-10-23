// Standalone policy test for the Undercity lift teleport: a pool bot in the
// Ruins of Lordaeron with a destination in the city below goes down, a bot in
// the city with a destination outside it goes up, everything else walks.
// Mirrors ai/playerbot/UndercityLiftPolicy.h; no server headers needed.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_undercity_lift_policy.cpp -o /tmp/test_undercity_lift_policy && /tmp/test_undercity_lift_policy

#include "../ai/playerbot/policy/travel/UndercityLiftPolicy.h"

#include <iostream>

namespace
{
int failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { \
        std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; \
        ++failures; \
    } \
} while (0)
} // namespace

using ai::UndercityLift;
using ai::UndercityLiftFor;

int main()
{
    // Throne room (live stuck spot) -> Auctioneer Epitwee below.
    CHECK(UndercityLiftFor(0, 1567.2f, 240.6f, 57.6f, 0, 1542.4f, 255.2f, -56.8f) == UndercityLift::Down);
    // Auction house -> Brill (outside the footprint): up.
    CHECK(UndercityLiftFor(0, 1609.7f, 187.2f, -56.8f, 0, 2260.0f, 290.0f, 34.0f) == UndercityLift::Up);
    // Auction house -> ruins courtyard above: up.
    CHECK(UndercityLiftFor(0, 1609.7f, 187.2f, -56.8f, 0, 1650.0f, 240.0f, 62.0f) == UndercityLift::Up);
    // Inside the city to inside the city: walk.
    CHECK(UndercityLiftFor(0, 1609.7f, 187.2f, -56.8f, 0, 1542.4f, 255.2f, -56.8f) == UndercityLift::None);
    // Ruins to ruins, or ruins to Brill: walk.
    CHECK(UndercityLiftFor(0, 1567.2f, 240.6f, 57.6f, 0, 1650.0f, 240.0f, 62.0f) == UndercityLift::None);
    CHECK(UndercityLiftFor(0, 1567.2f, 240.6f, 57.6f, 0, 2260.0f, 290.0f, 34.0f) == UndercityLift::None);
    // Outside the ruins (Brill) to the city: walk to the ruins first.
    CHECK(UndercityLiftFor(0, 2260.0f, 290.0f, 34.0f, 0, 1542.4f, 255.2f, -56.8f) == UndercityLift::None);
    // Other map with the same coordinates: never.
    CHECK(UndercityLiftFor(1, 1567.2f, 240.6f, 57.6f, 1, 1542.4f, 255.2f, -56.8f) == UndercityLift::None);
    CHECK(UndercityLiftFor(1, 1609.7f, 187.2f, -56.8f, 1, 2260.0f, 290.0f, 34.0f) == UndercityLift::None);
    // Cross-map goal from inside the city: up (leave the city first).
    CHECK(UndercityLiftFor(0, 1609.7f, 187.2f, -56.8f, 1, 1600.0f, -4400.0f, 10.0f) == UndercityLift::Up);
    // The up landing is above the city on the same footprint.
    CHECK(ai::AboveUndercity(0, ai::kUndercityUpX, ai::kUndercityUpY, ai::kUndercityUpZ));

    if (failures)
    {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "test_undercity_lift_policy: all checks passed\n";
    return 0;
}
