// Standalone check for ai/playerbot/CorpseEntrancePolicy.h (the corpse run's door):
//   g++ -std=c++17 -Wall -Wextra -Werror tools/test_corpse_entrance_policy.cpp -o /tmp/test_corpse_entrance_policy && /tmp/test_corpse_entrance_policy
#include "../ai/playerbot/CorpseEntrancePolicy.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::CorpseEntranceCandidate;
using ai::PickCorpseEntrance;

static CorpseEntranceCandidate Door(std::uint32_t id, float x, float y, float z, float ghostSqYd)
{
    CorpseEntranceCandidate c;
    c.triggerId = id;
    c.destX = x;
    c.destY = y;
    c.destZ = z;
    c.ghostSqYd = ghostSqYd;
    return c;
}

static std::uint32_t Pick(std::vector<CorpseEntranceCandidate> const& doors, float x, float y, float z)
{
    int const i = PickCorpseEntrance(doors, x, y, z);
    return i < 0 ? 0 : doors[static_cast<std::size_t>(i)].triggerId;
}

// Both orders of the store give the same door.
static std::uint32_t PickBothOrders(std::vector<CorpseEntranceCandidate> doors, float x, float y, float z)
{
    std::uint32_t const a = Pick(doors, x, y, z);
    std::reverse(doors.begin(), doors.end());
    std::uint32_t const b = Pick(doors, x, y, z);
    CHECK(a == b);
    return a;
}

// Scarlet Monastery (map 189), tw_world_areatrigger_teleport: the four doors'
// destinations. The ghost stands at the outdoor graveyard, the Graveyard
// wing's door the nearest to it (that is the r20-19z39 slot 7 trace).
static std::vector<CorpseEntranceCandidate> ScarletDoors()
{
    return {
        Door(45, 1687.27f, 1050.09f, 18.6773f, 20.0f * 20.0f),    // Graveyard
        Door(614, 253.672f, -206.624f, 18.6773f, 75.0f * 75.0f), // Library
        Door(612, 1608.1f, -318.919f, 18.6714f, 60.0f * 60.0f),  // Armory
        Door(610, 853.179f, 1319.18f, 18.6714f, 40.0f * 40.0f),  // Cathedral
    };
}

static void TestScarletMonasteryWings()
{
    std::vector<CorpseEntranceCandidate> const doors = ScarletDoors();
    // The nearest door to the ghost is the Graveyard's: the old pick.
    float bestGhost = doors[0].ghostSqYd;
    for (CorpseEntranceCandidate const& d : doors)
        bestGhost = std::min(bestGhost, d.ghostSqYd);
    CHECK(bestGhost == doors[0].ghostSqYd && doors[0].triggerId == 45);
    // r20 slot 7: the bodies lay in the Library (Wrexxar 178.8 -267.8, the pull of step 8).
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f) == 614);
    CHECK(PickBothOrders(doors, 189.9f, -257.6f, 18.7f) == 614);
    // A body in the Graveyard wing: its own door.
    CHECK(PickBothOrders(doors, 1730.0f, 1110.0f, 18.7f) == 45);
    // Armory and Cathedral bodies: theirs.
    CHECK(PickBothOrders(doors, 1640.0f, -250.0f, 18.7f) == 612);
    CHECK(PickBothOrders(doors, 880.0f, 1360.0f, 30.0f) == 610);
}

// Uldaman (map 70): the front door 286 and the back door 902 (-213 382).
static void TestUldamanFrontAndBack()
{
    std::vector<CorpseEntranceCandidate> const doors = {
        Door(286, -228.859f, 46.1018f, -46.0186f, 30.0f * 30.0f),
        Door(902, -212.95f, 382.427f, -38.7486f, 500.0f * 500.0f),
    };
    // A body in the front half: the front door.
    CHECK(PickBothOrders(doors, -180.0f, 90.0f, -50.0f) == 286);
    // A body near the back (Archaedas' side): the back door, though the ghost stands at the front.
    CHECK(PickBothOrders(doors, -160.0f, 330.0f, -45.0f) == 902);
    // The same with the ghost at the back door: a front-half body still takes the front door.
    std::vector<CorpseEntranceCandidate> swapped = doors;
    swapped[0].ghostSqYd = 500.0f * 500.0f;
    swapped[1].ghostSqYd = 30.0f * 30.0f;
    CHECK(PickBothOrders(swapped, -180.0f, 90.0f, -50.0f) == 286);
}

// A single-door map takes its one door wherever the body lies (unchanged).
static void TestSingleDoorUnchanged()
{
    std::vector<CorpseEntranceCandidate> const doors = { Door(2230, 0.797643f, -8.23429f, -15.5288f, 900.0f) };
    CHECK(Pick(doors, 0.0f, 0.0f, 0.0f) == 2230);
    CHECK(Pick(doors, -300.0f, 200.0f, -60.0f) == 2230);
    CHECK(PickCorpseEntrance(doors, 5000.0f, 5000.0f, 0.0f) == 0);
}

// Ties: Wailing Caverns' 228 and 5337 share one destination.
static void TestTies()
{
    // The one on the ghost's map wins over one elsewhere.
    std::vector<CorpseEntranceCandidate> doors = {
        Door(5337, -158.441f, 131.601f, -74.2552f, -1.0f),
        Door(228, -158.441f, 131.601f, -74.2552f, 100.0f),
    };
    CHECK(PickBothOrders(doors, -100.0f, 100.0f, -70.0f) == 228);
    // Both on the ghost's map: the nearer to the ghost.
    doors[0].ghostSqYd = 50.0f;
    CHECK(PickBothOrders(doors, -100.0f, 100.0f, -70.0f) == 5337);
    // Both elsewhere (the 300 s timeout from inside the instance): the lower id.
    doors[0].ghostSqYd = -1.0f;
    doors[1].ghostSqYd = -1.0f;
    CHECK(PickBothOrders(doors, -100.0f, 100.0f, -70.0f) == 228);
    // Equal ghost distances too: the lower id.
    doors[0].ghostSqYd = 100.0f;
    doors[1].ghostSqYd = 100.0f;
    CHECK(PickBothOrders(doors, -100.0f, 100.0f, -70.0f) == 228);
    // Within the tie window (0.5 yd apart from the body's view): the ghost's nearer door.
    std::vector<CorpseEntranceCandidate> near = {
        Door(1, 100.0f, 0.0f, 0.0f, 400.0f),
        Door(2, 100.5f, 0.0f, 0.0f, 100.0f),
    };
    CHECK(PickBothOrders(near, 0.0f, 0.0f, 0.0f) == 2);
    // Past it (2 yd): the destination decides, the ghost's distance does not.
    near[1].destX = 102.0f;
    CHECK(PickBothOrders(near, 0.0f, 0.0f, 0.0f) == 1);
    // The window is kCorpseEntranceTieYd.
    CHECK(ai::kCorpseEntranceTieYd == 1.0f);
}

// No door leads to the corpse's map: none, and the caller keeps its fallback.
static void TestMissingTriggers()
{
    std::vector<CorpseEntranceCandidate> const none;
    CHECK(PickCorpseEntrance(none, 0.0f, 0.0f, 0.0f) == -1);
}

// The 300 s timeout (the ghost on any map): every door counts, the nearest destination wins.
static void TestTimeoutPathIgnoresTheGhost()
{
    std::vector<CorpseEntranceCandidate> doors = ScarletDoors();
    for (CorpseEntranceCandidate& d : doors)
        d.ghostSqYd = -1.0f;
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f) == 614);
    CHECK(PickBothOrders(doors, 1700.0f, 1060.0f, 18.7f) == 45);
}

int main()
{
    std::cout << "Starting TortoiseBots corpse entrance policy tests...\n";
    TestScarletMonasteryWings();
    TestUldamanFrontAndBack();
    TestSingleDoorUnchanged();
    TestTies();
    TestMissingTriggers();
    TestTimeoutPathIgnoresTheGhost();
    std::cout << "All corpse entrance policy tests passed.\n";
    return 0;
}
