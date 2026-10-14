#include "../ai/playerbot/strategy/shaman/ShamanTotemGroupPolicy.h"

#include <cstdio>
#include <cstdlib>

#define CHECK(x) do { \
    if (!(x)) { \
        std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); \
        std::exit(1); \
    } \
} while (0)

using ai::BestAirTotemForSubgroup;
using ai::BestEarthTotemForSubgroup;
using ai::BestWaterTotemForSubgroup;
using ai::CountTotemSubgroup;
using ai::ManaTideShouldFire;
using ai::TotemGroupMember;
using ai::TotemSubgroupCensus;

static TotemSubgroupCensus CasterGroup()
{
    // Resto shaman + priest healer + mage + warlock: no melee, all mana.
    TotemGroupMember m[4] = {{true, false, true}, {false, false, true}, {false, false, true}, {false, false, true}};
    return CountTotemSubgroup(m, 4);
}

static TotemSubgroupCensus MeleeGroup()
{
    // Resto shaman + warrior + rogue + hunter pet-less: 2 melee, mana on shaman only... shaman excluded.
    TotemGroupMember m[4] = {{true, false, true}, {false, true, false}, {false, true, false}, {false, false, true}};
    return CountTotemSubgroup(m, 4);
}

int main()
{
    // Census: self excluded.
    TotemSubgroupCensus caster = CasterGroup();
    CHECK(caster.members == 3 && caster.meleeAttackers == 0 && caster.manaUsers == 3);
    TotemSubgroupCensus melee = MeleeGroup();
    CHECK(melee.members == 3 && melee.meleeAttackers == 2 && melee.manaUsers == 1);
    TotemSubgroupCensus solo = CountTotemSubgroup(nullptr, 0);
    CHECK(solo.members == 0);
    std::puts("  [PASS] census counts subgroup mates, skips self");

    // Air: melee -> windfury; casters -> tranquil > grace; unknown -> empty.
    CHECK(BestAirTotemForSubgroup(melee, true, true, true) == "windfury totem");
    CHECK(BestAirTotemForSubgroup(melee, false, true, true) == "");
    CHECK(BestAirTotemForSubgroup(caster, true, true, true) == "tranquil air totem");
    CHECK(BestAirTotemForSubgroup(caster, true, true, false) == "grace of air totem");
    CHECK(BestAirTotemForSubgroup(caster, true, false, false) == "");
    CHECK(BestAirTotemForSubgroup(solo, true, true, true) == "tranquil air totem");
    std::puts("  [PASS] air picks windfury for melee, tranquil/grace for casters");

    // Water: any mana user -> spring; pure melee -> stream.
    CHECK(BestWaterTotemForSubgroup(caster, true, true) == "mana spring totem");
    CHECK(BestWaterTotemForSubgroup(melee, true, true) == "mana spring totem");
    CHECK(BestWaterTotemForSubgroup(melee, false, true) == "healing stream totem");
    TotemGroupMember noMana[3] = {{true, false, true}, {false, true, false}, {false, true, false}};
    CHECK(BestWaterTotemForSubgroup(CountTotemSubgroup(noMana, 3), true, true) == "healing stream totem");
    CHECK(BestWaterTotemForSubgroup(caster, false, false) == "");
    std::puts("  [PASS] water picks mana spring for mana groups, stream otherwise");

    // Mana Tide: self OR subgroup healer/caster low mana.
    CHECK(ManaTideShouldFire(true, false, false));
    CHECK(ManaTideShouldFire(false, true, false));
    CHECK(ManaTideShouldFire(false, false, true));
    CHECK(!ManaTideShouldFire(false, false, false));
    std::puts("  [PASS] tide fires for subgroup healer/caster low mana, not just self");

    // Earth: melee -> strength; caster groups -> empty (no buff-nobody SoE).
    CHECK(BestEarthTotemForSubgroup(melee, true) == "strength of earth totem");
    CHECK(BestEarthTotemForSubgroup(melee, false) == "");
    CHECK(BestEarthTotemForSubgroup(caster, true) == "");
    CHECK(BestEarthTotemForSubgroup(solo, true) == "");
    std::puts("  [PASS] earth picks strength for melee, empty for casters");

    std::puts("All shaman totem group policy checks PASSED!");
    return 0;
}
