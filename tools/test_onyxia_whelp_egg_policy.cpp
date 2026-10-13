#include "../ai/playerbot/OnyxiaWhelpEggPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace ai;

int main()
{
    // Turtle 1.18.1 ids, not 3.3.5a.
    CHECK(kOnyxiaMapId == 249);
    CHECK(kOnyxianWhelpEntry == 11262);
    CHECK(kOnyxiaEggEntry == 176511);
    CHECK(kOnyxiaEggDangerRadius > kOnyxiaEggTriggerRadius);

    // Egg zone box: covers every egg spawn row, rejects the arena centre.
    CHECK(IsInOnyxiaEggZone(-32.0f, -151.4f));
    CHECK(IsInOnyxiaEggZone(-115.2f, -236.7f));
    CHECK(IsInOnyxiaEggZone(-33.0f, -266.3f));
    CHECK(IsInOnyxiaEggZone(-60.0f, -279.3f));
    CHECK(!IsInOnyxiaEggZone(0.0f, -210.0f));
    CHECK(!IsInOnyxiaEggZone(-75.0f, -210.0f - 100.0f));
    CHECK(!IsInOnyxiaEggZone(-200.0f, -210.0f));
    CHECK(!IsInOnyxiaEggZone(-60.0f, -100.0f));

    // Danger circle: inside radius + margin only.
    CHECK(IsInsideOnyxiaEggDanger(0.0f));
    CHECK(IsInsideOnyxiaEggDanger(11.9f));
    CHECK(!IsInsideOnyxiaEggDanger(12.0f));
    CHECK(!IsInsideOnyxiaEggDanger(30.0f));
    CHECK(OnyxiaDist2d(0.0f, 0.0f, 3.0f, 4.0f) > 4.99f && OnyxiaDist2d(0.0f, 0.0f, 3.0f, 4.0f) < 5.01f);

    // Avoid decision needs all four conditions.
    CHECK(ShouldAvoidOnyxiaEggs(true, true, true, true));
    CHECK(!ShouldAvoidOnyxiaEggs(false, true, true, true)); // toggle off
    CHECK(!ShouldAvoidOnyxiaEggs(true, false, true, true)); // other map
    CHECK(!ShouldAvoidOnyxiaEggs(true, true, false, true)); // outside the egg fields
    CHECK(!ShouldAvoidOnyxiaEggs(true, true, true, false)); // no egg close

    // Nearest exit: north cave mouth vs south cave mouth.
    CHECK(NearestOnyxiaEggExit(-45.0f, -165.0f) == 0);
    CHECK(NearestOnyxiaEggExit(-45.0f, -265.0f) == 1);
    CHECK(NearestOnyxiaEggExit(-100.0f, -150.0f) == 0);
    CHECK(NearestOnyxiaEggExit(-100.0f, -280.0f) == 1);

    // Whelp duty: AoE-capable ranged DPS only.
    CHECK(IsOnyxiaWhelpHunter(kWhelpClassMage, false, false, true));
    CHECK(IsOnyxiaWhelpHunter(kWhelpClassWarlock, false, false, true));
    CHECK(IsOnyxiaWhelpHunter(kWhelpClassHunter, false, false, true));
    CHECK(IsOnyxiaWhelpHunter(kWhelpClassShaman, false, false, true));
    CHECK(IsOnyxiaWhelpHunter(kWhelpClassDruid, false, false, true));
    CHECK(!IsOnyxiaWhelpHunter(kWhelpClassMage, true, false, true));   // tank flag
    CHECK(!IsOnyxiaWhelpHunter(kWhelpClassShaman, false, true, true)); // healer
    CHECK(!IsOnyxiaWhelpHunter(kWhelpClassShaman, false, false, false)); // enhancement
    CHECK(!IsOnyxiaWhelpHunter(5, false, false, true));  // priest: no AoE
    CHECK(!IsOnyxiaWhelpHunter(1, false, false, false)); // warrior
    CHECK(!IsOnyxiaWhelpHunter(4, false, false, false)); // rogue
    CHECK(!IsOnyxiaWhelpHunter(2, false, false, false)); // paladin

    // Pickable whelps: alive, within range, not in an egg cave.
    CHECK(IsOnyxiaWhelpPickable(true, 10.0f, false));
    CHECK(IsOnyxiaWhelpPickable(true, kOnyxiaWhelpEngageRange, false));
    CHECK(!IsOnyxiaWhelpPickable(false, 10.0f, false));
    CHECK(!IsOnyxiaWhelpPickable(true, kOnyxiaWhelpEngageRange + 1.0f, false));
    CHECK(!IsOnyxiaWhelpPickable(true, 10.0f, true));

    // Hunt decision.
    CHECK(ShouldHuntOnyxiaWhelps(true, true, true, true, false, false, true));
    CHECK(!ShouldHuntOnyxiaWhelps(false, true, true, true, false, false, true)); // toggle off
    CHECK(!ShouldHuntOnyxiaWhelps(true, false, true, true, false, false, true)); // other map
    CHECK(!ShouldHuntOnyxiaWhelps(true, true, false, true, false, false, true)); // grounded phase
    CHECK(!ShouldHuntOnyxiaWhelps(true, true, true, false, false, false, true)); // not on duty
    CHECK(!ShouldHuntOnyxiaWhelps(true, true, true, true, true, false, true));   // explicit order
    CHECK(!ShouldHuntOnyxiaWhelps(true, true, true, true, false, true, true));   // already on a whelp
    CHECK(!ShouldHuntOnyxiaWhelps(true, true, true, true, false, false, false)); // no whelp near

    // Hold decision.
    CHECK(ShouldHoldOnyxiaWhelpTarget(true, true, true, true, false, true));
    CHECK(!ShouldHoldOnyxiaWhelpTarget(false, true, true, true, false, true));
    CHECK(!ShouldHoldOnyxiaWhelpTarget(true, true, false, true, false, true)); // phase 3 grounded
    CHECK(!ShouldHoldOnyxiaWhelpTarget(true, true, true, false, false, true));
    CHECK(!ShouldHoldOnyxiaWhelpTarget(true, true, true, true, true, true));
    CHECK(!ShouldHoldOnyxiaWhelpTarget(true, true, true, true, false, false));

    std::cout << "onyxia whelp/egg policy: OK\n";
    return 0;
}
