// Standalone check for ai/playerbot/CreatureAuraKeepoutPolicy.h:
//   g++ -std=c++17 -Wall -Wextra tools/test_creature_aura_keepout_policy.cpp -o /tmp/test_cak && /tmp/test_cak
#include "../ai/playerbot/CreatureAuraKeepoutPolicy.h"
#include "../ai/playerbot/AvoidAoePolicy.h"

#include <cmath>
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
    std::cout << "Starting TortoiseBots creature aura keep-out tests...\n";

    // Table: the Creeping Sludge row, Turtle ids, only on Maraudon.
    {
        CreatureAuraKeepout const* row = FindCreatureAuraKeepout(349, 12222);
        CHECK(row != nullptr);
        CHECK(row->auraSpellId == 22638);
        CHECK(row->pulseRadius == 5.0f);
        CHECK(row->keepoutRadius == 8.0f);
        CHECK(FindCreatureAuraKeepout(0, 12222) == nullptr);   // other map
        CHECK(FindCreatureAuraKeepout(349, 12221) == nullptr); // Noxious Slime: no aura, not listed
        CHECK(FindCreatureAuraKeepout(349, 0) == nullptr);
    }
    std::cout << "  [PASS] table lookup by map and entry\n";

    // Every row keeps out past its pulse and fits the avoid-aoe radius cap,
    // so the existing strafe-first step-out handles it.
    for (CreatureAuraKeepout const& row : kCreatureAuraKeepouts)
    {
        CHECK(row.keepoutRadius > row.pulseRadius);
        CHECK(IsAvoidableAoeRadius(row.keepoutRadius));
        CHECK(CreatureAuraHazardRadius(row) > row.keepoutRadius);
        CHECK(IsAvoidableAoeRadius(CreatureAuraHazardRadius(row)));
    }
    std::cout << "  [PASS] keep-out wider than the pulse, inside the 15 yd cap\n";

    // Applies: toggle, row, alive, aura, and no explicit order on it.
    {
        CreatureAuraKeepout const* row = FindCreatureAuraKeepout(349, 12222);
        CHECK(CreatureAuraKeepoutApplies(true, row, true, true, false));
        CHECK(!CreatureAuraKeepoutApplies(false, row, true, true, false)); // toggle off: today's behaviour
        CHECK(!CreatureAuraKeepoutApplies(true, nullptr, true, true, false));
        CHECK(!CreatureAuraKeepoutApplies(true, row, false, true, false)); // dead
        CHECK(!CreatureAuraKeepoutApplies(true, row, true, false, false)); // aura gone
        CHECK(!CreatureAuraKeepoutApplies(true, row, true, true, true));   // player ordered the attack
    }
    std::cout << "  [PASS] applies only with toggle, live aura and no explicit order\n";

    // Step-out band and hysteresis: step out inside 8, paths kept outside 9.
    {
        CreatureAuraKeepout const& row = *FindCreatureAuraKeepout(349, 12222);
        CHECK(InsideCreatureAuraKeepout(row, 4.0f));  // melee reach: inside the pulse
        CHECK(InsideCreatureAuraKeepout(row, 8.0f));
        CHECK(!InsideCreatureAuraKeepout(row, 8.1f));
        CHECK(CreatureAuraHazardRadius(row) == 9.0f);
        // The existing escape step clears the keep-out rim from inside the pulse.
        CHECK(AoeEscapeStep(row.keepoutRadius, 8.0f) == 8.0f);
        CHECK(AoeEscapeStep(row.keepoutRadius, 15.0f) == 9.0f);
        CHECK(kCreatureAuraHazardSeconds >= 2);
    }
    std::cout << "  [PASS] step out inside 8 yd, hazard keeps paths 9 yd away\n";

    // Player order: the ordered creature is never a keep-out for that bot,
    // whatever its state; any other listed creature still is.
    {
        CreatureAuraKeepout const* row = FindCreatureAuraKeepout(349, 12222);
        bool const ordered = true;
        CHECK(!CreatureAuraKeepoutApplies(true, row, true, true, ordered));
        CHECK(CreatureAuraKeepoutApplies(true, row, true, true, !ordered));
    }
    std::cout << "  [PASS] attack order exempts only the ordered creature\n";

    // Follow hold point: sludge at origin, hazard 9 yd, hold ring 9.5 yd.
    {
        float hx = 0.0f, hy = 0.0f;
        float const r = 9.0f;
        // Master inside the hazard, bot 20 yd east: hold on the ring on the bot's side.
        CHECK(CreatureAuraFollowHoldPoint(20.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, r, hx, hy));
        CHECK(std::fabs(hx - 9.5f) < 0.01f && std::fabs(hy) < 0.01f);
        // The hold point is outside the hazard and the keep-out radius.
        CHECK(std::sqrt(hx * hx + hy * hy) > r);
        CHECK(!InsideCreatureAuraKeepout(*FindCreatureAuraKeepout(349, 12222), std::sqrt(hx * hx + hy * hy)));
        // Master outside the ring: follow normally.
        CHECK(!CreatureAuraFollowHoldPoint(20.0f, 0.0f, 12.0f, 0.0f, 0.0f, 0.0f, r, hx, hy));
        // Bot already inside: the step-out handles it, follow does not hold.
        CHECK(!CreatureAuraFollowHoldPoint(5.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, r, hx, hy));
        // Bot at the edge already: hold point is (about) where it stands.
        CHECK(CreatureAuraFollowHoldPoint(9.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, r, hx, hy));
        CHECK(std::fabs(hx - 9.5f) < 0.05f);
        // Oblique approach: entry point lies on the ring.
        CHECK(CreatureAuraFollowHoldPoint(-20.0f, 5.0f, 0.0f, 2.0f, 0.0f, 0.0f, r, hx, hy));
        CHECK(std::fabs(std::sqrt(hx * hx + hy * hy) - 9.5f) < 0.01f);
        CHECK(hx < 0.0f);
    }
    std::cout << "  [PASS] follow holds just outside the hazard on the way to the master\n";

    std::cout << "All creature aura keep-out tests passed.\n";
    return 0;
}
