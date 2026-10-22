// Standalone regression test for healer downranking: fit the rank to the
// wound (missing health after incoming heals), with a safety margin, and
// never downrank a critical emergency (at/below criticalHealth).
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_heal_downrank_policy.cpp -o /tmp/test_heal_downrank
//   /tmp/test_heal_downrank

#include "../ai/playerbot/HealDownrankPolicy.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace ai;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// Greater Heal ranks, cheapest-first (spell ids arbitrary, heals/mana shaped
// like the 1.12 ladder: rank 1 ~400 for ~180, top ~2000 for ~710).
static std::vector<HealRankOption> Ladder()
{
    return {{2060, 400, 180}, {10963, 800, 370}, {10964, 1300, 540}, {25314, 2000, 710}};
}

static HealDownrankState State(std::uint32_t missing, std::uint8_t healthPct, std::uint8_t incomingDamage = 0)
{
    return HealDownrankState{missing, healthPct, 50, 20, incomingDamage};
}

int main()
{
    std::vector<HealRankOption> const ranks = Ladder();

    // Small wound (600 missing, target at 90%): need = 480 -> rank 2 (800).
    CHECK(PickHealRank(ranks, State(600, 90)) == 10963);
    // Tiny wound (200 missing): need = max(160, 100) = 160 -> rank 1 (400).
    CHECK(PickHealRank(ranks, State(200, 95)) == 2060);
    // Mid wound (1500 missing): need = 1200 -> rank 3 (1300).
    CHECK(PickHealRank(ranks, State(1500, 70)) == 10964);
    // Huge wound (3000 missing): need = 2400, nothing covers -> top rank.
    CHECK(PickHealRank(ranks, State(3000, 60)) == 25314);
    // Emergency at/below criticalHealth (20%): top rank whatever the wound.
    CHECK(PickHealRank(ranks, State(200, 20)) == 25314);
    // Emergency below criticalHealth (15%): top rank.
    CHECK(PickHealRank(ranks, State(100, 15)) == 25314);
    // Predicted emergency: a small wound on a 30% target whose next swing
    // (15% of max health) would push it into the critical band keeps the
    // top rank; without the swing it fits a cheap rank.
    CHECK(PickHealRank(ranks, State(200, 30, 15)) == 25314);
    CHECK(PickHealRank(ranks, State(200, 30, 5)) == 2060);
    // Incoming larger than the bar saturates at the emergency gate.
    CHECK(PickHealRank(ranks, State(200, 60, 100)) == 25314);
    // Medium band (50%, old emergency line): now fits the wound — 200
    // missing -> need 160 -> rank 1. Live raid 2026-10-14 paid top rank
    // here 56x.
    CHECK(PickHealRank(ranks, State(200, 50)) == 2060);
    // Just above the old danger line (51%): downranks as before.
    // No wound left (incoming covers it): cheapest if forced to cast.
    CHECK(PickHealRank(ranks, State(0, 90)) == 2060);
    // Empty list: 0, caller falls back to the old top-rank selection.
    CHECK(PickHealRank({}, State(600, 90)) == 0);
    // Single rank known: always that rank outside emergency.
    CHECK(PickHealRank({{25314, 2000, 710}}, State(100, 90)) == 25314);
    // Exact boundary: need 480, rank healing exactly 480 covers.
    CHECK(PickHealRank({{1, 479, 10}, {2, 480, 20}}, State(600, 90)) == 2);
    // Flat floor: missing 50 -> need = max(40, 100) = 100, rank 1 covers.
    CHECK(PickHealRank(ranks, State(50, 98)) == 2060);
    // Target gate (live spell_template rows, 2026-10-14): plain friend (21)
    // and jump-less chain id (Healing Wave 25357: 45/0) downrank; real
    // jumps (Chain Heal 10623: 45/3) and totem summons (Mana Tide 16190:
    // 42/0) stay top rank.
    CHECK(HealTargetAllowsDownrank(21, 0));
    CHECK(HealTargetAllowsDownrank(45, 0));
    CHECK(!HealTargetAllowsDownrank(45, 3));
    CHECK(!HealTargetAllowsDownrank(42, 0));
    CHECK(!HealTargetAllowsDownrank(6, 0));
    // Cheapest-first violation is a caller bug; document order: ascending
    // mana picks the cheapest cover, so the ladder must stay sorted.
    CHECK(PickHealRank(ranks, State(600, 90)) != 25314);
    std::printf("heal downrank policy: %d checks passed\n", checks);
    return 0;
}
