// Standalone check for ai/playerbot/CorpseEntrancePolicy.h (the corpse run's door):
//   g++ -std=c++17 -Wall -Wextra -Werror tools/test_corpse_entrance_policy.cpp -o /tmp/test_corpse_entrance_policy && /tmp/test_corpse_entrance_policy
// Fixtures: the core's world data (~/tortoise-wow sql/base: areatrigger_teleport for the
// destinations, areatrigger_template for the map a trigger sits on, map_template for
// the instanceable maps and ghost_entrance_map).
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
using ai::CorpseEntranceRules;
using ai::PickCorpseEntrance;

static CorpseEntranceCandidate Door(std::uint32_t id, float x, float y, float z, float ghostSqYd,
    std::uint32_t sourceMap = 0, bool sourceInstanceable = false)
{
    CorpseEntranceCandidate c;
    c.triggerId = id;
    c.sourceMap = sourceMap;
    c.sourceInstanceable = sourceInstanceable;
    c.destX = x;
    c.destY = y;
    c.destZ = z;
    c.ghostSqYd = ghostSqYd;
    return c;
}

static CorpseEntranceRules Walk(std::uint32_t destMap, std::uint32_t ghostMap)
{
    CorpseEntranceRules r;
    r.destMap = destMap;
    r.ghostMap = ghostMap;
    return r;
}

static CorpseEntranceRules Timeout(std::uint32_t destMap, std::uint32_t ghostMap, std::int32_t entranceMap)
{
    CorpseEntranceRules r = Walk(destMap, ghostMap);
    r.entranceMap = entranceMap;
    r.timeout = true;
    return r;
}

static std::uint32_t Pick(std::vector<CorpseEntranceCandidate> const& doors, float x, float y, float z,
    CorpseEntranceRules const& rules)
{
    int const i = PickCorpseEntrance(doors, x, y, z, rules);
    return i < 0 ? 0 : doors[static_cast<std::size_t>(i)].triggerId;
}

// Both orders of the store give the same door (0: none).
static std::uint32_t PickBothOrders(std::vector<CorpseEntranceCandidate> doors, float x, float y, float z,
    CorpseEntranceRules const& rules)
{
    std::uint32_t const a = Pick(doors, x, y, z, rules);
    std::reverse(doors.begin(), doors.end());
    std::uint32_t const b = Pick(doors, x, y, z, rules);
    CHECK(a == b);
    return a;
}

// Scarlet Monastery (map 189, entered from map 0): the four doors, all on
// map 0. The ghost stands at the outdoor graveyard, the Graveyard wing's door
// the nearest to it (the r20-19z39 slot 7 trace).
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
    CorpseEntranceRules const walk = Walk(189, 0);
    // The nearest door to the ghost is the Graveyard's: the old pick.
    float bestGhost = doors[0].ghostSqYd;
    for (CorpseEntranceCandidate const& d : doors)
        bestGhost = std::min(bestGhost, d.ghostSqYd);
    CHECK(bestGhost == doors[0].ghostSqYd && doors[0].triggerId == 45);
    // r20 slot 7: the bodies lay in the Library (Wrexxar 178.8 -267.8, the pull of step 8).
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, walk) == 614);
    CHECK(PickBothOrders(doors, 189.9f, -257.6f, 18.7f, walk) == 614);
    // A body in the Graveyard wing: its own door.
    CHECK(PickBothOrders(doors, 1730.0f, 1110.0f, 18.7f, walk) == 45);
    // Armory and Cathedral bodies: theirs.
    CHECK(PickBothOrders(doors, 1640.0f, -250.0f, 18.7f, walk) == 612);
    CHECK(PickBothOrders(doors, 880.0f, 1360.0f, 30.0f, walk) == 610);
}

// Intra-instance teleports are never doors: a scripted trigger inside Scarlet
// Monastery (source 189, destination 189) that lands right on the body is
// passed, by the walk and by the timeout.
static void TestSameMapScriptedTriggerIsNeverPicked()
{
    std::vector<CorpseEntranceCandidate> doors = ScarletDoors();
    doors.push_back(Door(9001, 180.0f, -265.0f, 18.7f, 1.0f, 189, true));
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, Walk(189, 189)) == 0); // a ghost inside: no door on its map
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, Walk(189, 0)) == 614);
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, Timeout(189, 189, 0)) == 614);
    // Even marked open-world, a same-map trigger is not external (Thousand Needles' Test of
    // Faith 942: source 1, destination 1).
    std::vector<CorpseEntranceCandidate> faith = {Door(942, -5191.27f, -2802.59f, -7.2f, 4.0f, 1, false)};
    CHECK(!ai::IsExternalEntrance(faith[0], 1));
    CHECK(PickBothOrders(faith, -5190.0f, -2800.0f, -7.0f, Walk(1, 1)) == 0);
    CHECK(PickBothOrders(faith, -5190.0f, -2800.0f, -7.0f, Timeout(1, 0, 1)) == 0);
}

// A door that sits inside another instance is no external entrance: Molten
// Core's 2886 in Blackrock Depths (map 230) lands where its two window doors
// on map 0 land (3528, 3529); a ghost on map 0 takes a window, and neither the
// walk nor the timeout ever takes 2886 (it would skip BRD's own way in).
static void TestDoorInsideAnotherInstanceIsNotExternal()
{
    std::vector<CorpseEntranceCandidate> const doors = {
        Door(2886, 1091.89f, -466.985f, -105.084f, -1.0f, 230, true),
        Door(3529, 1091.89f, -466.985f, -105.084f, 50.0f * 50.0f, 0, false),
        Door(3528, 1091.89f, -466.985f, -105.084f, 40.0f * 40.0f, 0, false),
    };
    CHECK(PickBothOrders(doors, 1080.0f, -470.0f, -100.0f, Walk(409, 0)) == 3528);
    CHECK(PickBothOrders(doors, 1080.0f, -470.0f, -100.0f, Walk(409, 230)) == 0);
    CHECK(PickBothOrders(doors, 1080.0f, -470.0f, -100.0f, Timeout(409, 230, 0)) == 3528);
    CHECK(!ai::IsExternalEntrance(doors[0], 409));
}

// Uldaman (map 70, entered from 0): the front door 286 and the back door 902
// (-213 382), both on map 0, both external: the back door is kept by the rules
// (open-world source, the ghost's map) and taken only for a body near it.
static void TestUldamanFrontAndBack()
{
    std::vector<CorpseEntranceCandidate> const doors = {
        Door(286, -228.859f, 46.1018f, -46.0186f, 30.0f * 30.0f),
        Door(902, -212.95f, 382.427f, -38.7486f, 500.0f * 500.0f),
    };
    CorpseEntranceRules const walk = Walk(70, 0);
    CHECK(PickBothOrders(doors, -180.0f, 90.0f, -50.0f, walk) == 286);
    // A body near the back (Archaedas' side): the back door, though the ghost stands at the front.
    CHECK(PickBothOrders(doors, -160.0f, 330.0f, -45.0f, walk) == 902);
    // The ghost at the back door: a front-half body still takes the front door.
    std::vector<CorpseEntranceCandidate> swapped = doors;
    swapped[0].ghostSqYd = 500.0f * 500.0f;
    swapped[1].ghostSqYd = 30.0f * 30.0f;
    CHECK(PickBothOrders(swapped, -180.0f, 90.0f, -50.0f, walk) == 286);
    // The ghost on another map (Kalimdor) walks to neither.
    CHECK(PickBothOrders(doors, -160.0f, 330.0f, -45.0f, Walk(70, 1)) == 0);
}

// The 300 s timeout: the nearest external door only when it sits on the
// ghost's map or the map the instance is entered from (ghost_entrance_map);
// else none, and the caller keeps GetMapEntranceTrigger.
static void TestTimeoutDoorsAndFallback()
{
    std::vector<CorpseEntranceCandidate> doors = ScarletDoors();
    for (CorpseEntranceCandidate& d : doors)
        d.ghostSqYd = -1.0f; // the ghost still inside (map 189)
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, Timeout(189, 189, 0)) == 614);
    CHECK(PickBothOrders(doors, 1700.0f, 1060.0f, 18.7f, Timeout(189, 189, 0)) == 45);
    // No entrance map known and the ghost not on the doors' map: none (the fallback).
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, Timeout(189, 189, -1)) == 0);
    // Doors on a map that is neither the ghost's nor the entrance map: none.
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, Timeout(189, 1, 1)) == 0);
    // The walk never uses the entrance map: only the ghost's own (a ghost
    // still inside walks to no door, even with the entrance map known).
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, Walk(189, 189)) == 0);
    CorpseEntranceRules walkKnown = Walk(189, 189);
    walkKnown.entranceMap = 0;
    CHECK(PickBothOrders(doors, 178.8f, -267.8f, 18.7f, walkKnown) == 0);
    // Wailing Caverns (entered from 1): 228 sits on map 1, 5337 "Grim Batol" on map 0, the same
    // destination. A timeout from map 1 takes 228; 5337 only for a ghost standing on map 0.
    std::vector<CorpseEntranceCandidate> wc = {
        Door(228, -158.441f, 131.601f, -74.2552f, -1.0f, 1, false),
        Door(5337, -158.441f, 131.601f, -74.2552f, -1.0f, 0, false),
    };
    CHECK(PickBothOrders(wc, -100.0f, 100.0f, -70.0f, Timeout(43, 1, 1)) == 228);
    CHECK(PickBothOrders(wc, -100.0f, 100.0f, -70.0f, Timeout(43, 43, 1)) == 228);
    wc[1].ghostSqYd = 100.0f;
    CHECK(PickBothOrders(wc, -100.0f, 100.0f, -70.0f, Timeout(43, 0, 1)) == 5337); // tie: the ghost's map
}

// A single-door map takes its one door wherever the body lies (unchanged).
static void TestSingleDoorUnchanged()
{
    std::vector<CorpseEntranceCandidate> const doors = {Door(2230, 0.797643f, -8.23429f, -15.5288f, 900.0f, 1)};
    CHECK(Pick(doors, 0.0f, 0.0f, 0.0f, Walk(389, 1)) == 2230);
    CHECK(Pick(doors, -300.0f, 200.0f, -60.0f, Walk(389, 1)) == 2230);
    CHECK(PickCorpseEntrance(doors, 5000.0f, 5000.0f, 0.0f, Walk(389, 1)) == 0);
}

// Ties on the destination.
static void TestTies()
{
    CorpseEntranceRules const walk = Walk(43, 1);
    std::vector<CorpseEntranceCandidate> doors = {
        Door(5337, -158.441f, 131.601f, -74.2552f, 50.0f, 1),
        Door(228, -158.441f, 131.601f, -74.2552f, 100.0f, 1),
    };
    // Both on the ghost's map: the nearer to the ghost.
    CHECK(PickBothOrders(doors, -100.0f, 100.0f, -70.0f, walk) == 5337);
    // Equal ghost distances: the lower id.
    doors[0].ghostSqYd = 100.0f;
    CHECK(PickBothOrders(doors, -100.0f, 100.0f, -70.0f, walk) == 228);
    // Both off the ghost's map but on the entrance map (a timeout): the lower id.
    doors[0].ghostSqYd = -1.0f;
    doors[1].ghostSqYd = -1.0f;
    CHECK(PickBothOrders(doors, -100.0f, 100.0f, -70.0f, Timeout(43, 43, 1)) == 228);
    // One with a ghost distance wins the tie over one without.
    doors[0].ghostSqYd = 400.0f;
    CHECK(PickBothOrders(doors, -100.0f, 100.0f, -70.0f, Timeout(43, 1, 1)) == 5337);
    // Within the tie window (0.5 yd apart from the body's view): the ghost's nearer door.
    std::vector<CorpseEntranceCandidate> near = {
        Door(1, 100.0f, 0.0f, 0.0f, 400.0f),
        Door(2, 100.5f, 0.0f, 0.0f, 100.0f),
    };
    CHECK(PickBothOrders(near, 0.0f, 0.0f, 0.0f, Walk(5, 0)) == 2);
    // Past it (2 yd): the destination decides, the ghost's distance does not.
    near[1].destX = 102.0f;
    CHECK(PickBothOrders(near, 0.0f, 0.0f, 0.0f, Walk(5, 0)) == 1);
    CHECK(ai::kCorpseEntranceTieYd == 1.0f);
}

// No door leads to the corpse's map: none, and the caller keeps its fallback.
static void TestMissingTriggers()
{
    std::vector<CorpseEntranceCandidate> const none;
    CHECK(PickCorpseEntrance(none, 0.0f, 0.0f, 0.0f, Walk(189, 0)) == -1);
    CHECK(PickCorpseEntrance(none, 0.0f, 0.0f, 0.0f, Timeout(189, 189, 0)) == -1);
    // An unknown source map counts as instanceable: never taken.
    CorpseEntranceCandidate unknown;
    unknown.triggerId = 7;
    CHECK(unknown.sourceInstanceable && !ai::IsExternalEntrance(unknown, 189));
}

// The core's managed-bot repop (SM-REVIVE-PATH-2026-10-23): a bot that releases
// INSIDE the dungeon is revived at once at a door destination, still on the
// dungeon map; RepopEntranceRules answer the core hook GetBotRepopEntrance.
static std::vector<CorpseEntranceCandidate> FromInside(std::vector<CorpseEntranceCandidate> doors)
{
    for (CorpseEntranceCandidate& d : doors)
        d.ghostSqYd = -1.0f; // the bot stands on the dungeon map, not on the doors' map
    return doors;
}

static void TestRepopEntrance()
{
    using ai::RepopEntranceRules;
    CorpseEntranceRules const sm = RepopEntranceRules(189, 0);
    CHECK(sm.destMap == 189 && sm.ghostMap == 189 && sm.entranceMap == 0 && sm.timeout);
    std::vector<CorpseEntranceCandidate> const scarlet = FromInside(ScarletDoors());
    // r21-19z40e slot 7: Lucine died in the Library at 229.7 -327.2; the core
    // picked 45 (the lowest id), the answer is the Library's 614.
    CHECK(PickBothOrders(scarlet, 229.7f, -327.2f, 18.7f, sm) == 614);
    CHECK(PickBothOrders(scarlet, 178.8f, -267.8f, 18.7f, sm) == 614);
    CHECK(PickBothOrders(scarlet, 1730.0f, 1110.0f, 18.7f, sm) == 45);
    CHECK(PickBothOrders(scarlet, 1640.0f, -250.0f, 18.7f, sm) == 612);
    CHECK(PickBothOrders(scarlet, 880.0f, 1360.0f, 30.0f, sm) == 610);
    // Uldaman: a back-half death revives at the back door 902, a front-half one at 286.
    std::vector<CorpseEntranceCandidate> const uld = FromInside({
        Door(286, -228.859f, 46.1018f, -46.0186f, 0.0f),
        Door(902, -212.95f, 382.427f, -38.7486f, 0.0f),
    });
    CHECK(PickBothOrders(uld, -160.0f, 330.0f, -45.0f, RepopEntranceRules(70, 0)) == 902);
    CHECK(PickBothOrders(uld, -180.0f, 90.0f, -50.0f, RepopEntranceRules(70, 0)) == 286);
    // A single-door map (Ragefire Chasm 389, entered from Kalimdor): its one door, as before.
    std::vector<CorpseEntranceCandidate> const rfc = FromInside({Door(2230, 0.797643f, -8.23429f, -15.5288f, 0.0f, 1)});
    CHECK(PickBothOrders(rfc, -300.0f, 200.0f, -60.0f, RepopEntranceRules(389, 1)) == 2230);
    // No eligible door: no answer, the core keeps GetMapEntranceTrigger.
    CHECK(PickBothOrders(scarlet, 229.7f, -327.2f, 18.7f, RepopEntranceRules(189, -1)) == 0); // entrance map unknown
    CHECK(PickBothOrders(scarlet, 229.7f, -327.2f, 18.7f, RepopEntranceRules(189, 1)) == 0);  // doors not on it
    CHECK(PickCorpseEntrance({}, 229.7f, -327.2f, 18.7f, sm) == -1);
    // A door inside another instance (Molten Core's 2886 in BRD) is never the answer.
    std::vector<CorpseEntranceCandidate> const mc = FromInside({
        Door(2886, 1091.89f, -466.985f, -105.084f, 0.0f, 230, true),
        Door(3528, 1091.89f, -466.985f, -105.084f, 0.0f, 0, false),
    });
    CHECK(PickBothOrders(mc, 1080.0f, -470.0f, -100.0f, RepopEntranceRules(409, 0)) == 3528);
    CHECK(PickBothOrders({mc[0]}, 1080.0f, -470.0f, -100.0f, RepopEntranceRules(409, 230)) == 0);
}

int main()
{
    std::cout << "Starting TortoiseBots corpse entrance policy tests...\n";
    TestScarletMonasteryWings();
    TestSameMapScriptedTriggerIsNeverPicked();
    TestDoorInsideAnotherInstanceIsNotExternal();
    TestUldamanFrontAndBack();
    TestTimeoutDoorsAndFallback();
    TestSingleDoorUnchanged();
    TestTies();
    TestMissingTriggers();
    TestRepopEntrance();
    std::cout << "All corpse entrance policy tests passed.\n";
    return 0;
}
