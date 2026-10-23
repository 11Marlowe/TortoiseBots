// Standalone regression test for the shaman tank levelling policy
// (runtime/ShamanTankLevelingPolicy.h, issue #1125): below the first key tank
// talent (level 25) a pool shaman's tank identity has to come from something
// that survives a restart, and only the spent talents do. The rule is "the
// 7.3 premade entry for the bot's level matches its spent ranks AND no other
// shaman premade's entry does", so a layout shared with another premade (the
// 10-14 links are identical across 7.1 and 7.3) falls back to the DPS spec
// instead of claiming the tank build.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_shaman_tank_leveling_policy.cpp -o /tmp/test_shaman_tank_leveling
//   /tmp/test_shaman_tank_leveling

#include "../runtime/policy/class/ShamanTankLevelingPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// One entry per class talent as TalentSpec builds it: id -> allocated rank.
// The ids below stand in for talent ids; only the allocation shape matters.
static ShamanTalentRanks Ranks(std::initializer_list<std::pair<int const, int>> entries)
{
    return ShamanTalentRanks(entries);
}

// Allocation shapes of the shipped shaman levelling links
// (AiPlayerbot.PremadeSpecLink.7.<spec>.<level>), digit for digit in talent
// order. Kept literal so a conf change that breaks the read fails here too.
static void TestShippedLayouts()
{
    // 7.3 (tank) / 7.1 (enhancement) at 15: -50001 vs -5001.
    ShamanTalentRanks const tank15 = Ranks({{1, 5}, {4, 0}, {5, 1}});
    ShamanTalentRanks const enhancement15 = Ranks({{1, 5}, {4, 1}});
    CHECK(!ShamanTalentRanksEqual(tank15, enhancement15));

    // ...and at 20: -50023001 vs -50051.
    ShamanTalentRanks const tank20 = Ranks({{1, 5}, {4, 2}, {5, 3}, {8, 1}});
    ShamanTalentRanks const enhancement20 = Ranks({{1, 5}, {4, 5}, {5, 1}});
    CHECK(!ShamanTalentRanksEqual(tank20, enhancement20));

    // At 10 both ship -1: the layouts are the same allocation.
    ShamanTalentRanks const tank10 = Ranks({{1, 1}});
    ShamanTalentRanks const enhancement10 = Ranks({{1, 1}});
    CHECK(ShamanTalentRanksEqual(tank10, enhancement10));

    std::printf("  [PASS] shipped levelling layouts: tank and enhancement diverge from 15\n");
}

static void TestRankComparison()
{
    // Same allocation, one side listing the unspent talents explicitly and
    // the other not: still the same build.
    CHECK(ShamanTalentRanksEqual(Ranks({{1, 5}, {4, 2}}), Ranks({{1, 5}, {4, 2}, {5, 0}})));
    // A rank difference anywhere is a different build.
    CHECK(!ShamanTalentRanksEqual(Ranks({{1, 5}, {4, 2}}), Ranks({{1, 5}, {4, 3}})));
    // Points spent where the other side has none.
    CHECK(!ShamanTalentRanksEqual(Ranks({{1, 5}, {5, 1}}), Ranks({{1, 5}})));
    CHECK(!ShamanTalentRanksEqual(Ranks({{1, 5}}), Ranks({{1, 5}, {5, 1}})));
    // Unspent on both sides, listed or not.
    CHECK(ShamanTalentRanksEqual(Ranks({{1, 5}}), Ranks({{1, 5}, {4, 0}})));
    // No talents at all is a build too (a bot below level 10).
    CHECK(ShamanTalentRanksEqual(Ranks({}), Ranks({{1, 0}})));
    std::printf("  [PASS] rank comparison: unspent ranks count as zero, ranks decide\n");
}

static void TestLevelingRule()
{
    // The tank entry matches and nothing else does: on the tank path.
    CHECK(ShamanOnTankLevelingPath(true, 0));
    // Shared layout (level 10-14: tank == enhancement) or an outright
    // different premade: not the tank build, the read falls back to DPS.
    CHECK(!ShamanOnTankLevelingPath(true, 1));
    CHECK(!ShamanOnTankLevelingPath(true, 3));
    CHECK(!ShamanOnTankLevelingPath(false, 0));
    CHECK(!ShamanOnTankLevelingPath(false, 2));

    // End to end over the shipped shapes: the tank layout is claimed at 15
    // and 20, a drifted layout claims nothing, and the 10 layout - which the
    // bot cannot tell apart - stays DPS.
    struct Case
    {
        ShamanTalentRanks current;
        ShamanTalentRanks tankEntry;
        ShamanTalentRanks enhancementEntry;
        bool expected;
    };
    Case const cases[] = {
        {Ranks({{1, 5}, {4, 0}, {5, 1}}), Ranks({{1, 5}, {4, 0}, {5, 1}}), Ranks({{1, 5}, {4, 1}}), true},
        {Ranks({{1, 5}, {4, 2}, {5, 3}, {8, 1}}), Ranks({{1, 5}, {4, 2}, {5, 3}, {8, 1}}),
            Ranks({{1, 5}, {4, 5}, {5, 1}}), true},
        {Ranks({{1, 5}, {4, 5}, {5, 1}}), Ranks({{1, 5}, {4, 2}, {5, 3}, {8, 1}}),
            Ranks({{1, 5}, {4, 5}, {5, 1}}), false},
        {Ranks({{1, 1}}), Ranks({{1, 1}}), Ranks({{1, 1}}), false},
        {Ranks({{1, 5}, {4, 4}, {5, 2}}), Ranks({{1, 5}, {4, 2}, {5, 3}, {8, 1}}),
            Ranks({{1, 5}, {4, 5}, {5, 1}}), false},
    };
    for (Case const& test : cases)
    {
        bool const tankEntryMatches = ShamanTalentRanksEqual(test.current, test.tankEntry);
        int otherPremadeMatches = 0;
        if (ShamanTalentRanksEqual(test.current, test.enhancementEntry))
            ++otherPremadeMatches;
        CHECK(ShamanOnTankLevelingPath(tankEntryMatches, otherPremadeMatches) == test.expected);
    }
    std::printf("  [PASS] leveling rule: tank layout claimed, shared layout stays DPS\n");
}

static void TestLevelGate()
{
    // The level the key-talent read takes over at: AiPlayerbot.
    // PremadeSpecLink.7.3.25 is the first entry allocating Spirit Armor.
    // tools/talents/validate_presets.py pins this against the DBC and conf.
    CHECK(kShamanTankFirstTalentLevel == 25);
    std::printf("  [PASS] level gate: key talents take over at 25\n");
}

int main()
{
    TestShippedLayouts();
    TestRankComparison();
    TestLevelingRule();
    TestLevelGate();
    std::printf("shaman tank levelling policy tests passed (%d checks).\n", checks);
    return 0;
}
