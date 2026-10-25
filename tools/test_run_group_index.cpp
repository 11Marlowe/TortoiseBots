// Standalone test for runtime/RunGroupIndex.h: the bot -> run lookup the bot AI
// asks twice a second per bot (area triggers) and the run hooks ask per event.
// The work per question is one hash lookup whatever the number of runs, and a
// bot outside a group asks nothing.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_run_group_index.cpp -o /tmp/test_run_group_index
//   /tmp/test_run_group_index

#include "../runtime/RunGroupIndex.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

using TortoiseBots::DungeonRun::RunGroupIndex;
using TortoiseBots::DungeonRun::RunOfGroup;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// 500 bots, `runs` bound groups of 5 bots each; the rest in unbound groups or none.
// Returns the index lookups one round of checks (one question per bot) made.
static std::uint64_t OneRound(std::size_t runs, std::size_t& found)
{
    RunGroupIndex index;
    for (std::size_t i = 0; i < runs; ++i)
        index.Add(static_cast<std::uint32_t>(100 + i), 9000 + i);
    std::uint64_t const before = index.Lookups();
    found = 0;
    for (std::uint32_t bot = 0; bot < 500; ++bot)
    {
        // bots 0..(5 runs - 1) are in a run's group; then 200 in unbound groups; the rest in no group (0).
        std::uint32_t group = 0;
        if (bot < 5 * runs)
            group = static_cast<std::uint32_t>(100 + bot / 5);
        else if (bot < 5 * runs + 200)
            group = 5000 + bot;
        if (std::uint64_t const commander = RunOfGroup(index, group))
        {
            ++found;
            CHECK(commander == 9000 + (group - 100));
        }
    }
    return index.Lookups() - before;
}

static void TestLookupIsConstantPerBot()
{
    std::size_t found10 = 0, found1 = 0, found0 = 0;
    std::uint64_t const ten = OneRound(10, found10);
    std::uint64_t const one = OneRound(1, found1);
    std::uint64_t const zero = OneRound(0, found0);
    CHECK(found10 == 50 && found1 == 5 && found0 == 0);
    // One lookup per grouped bot, never one per run: 10 runs cost what 1 run costs per bot.
    CHECK(ten == 50 + 200);
    CHECK(one == 5 + 200);
    CHECK(zero == 200);
    CHECK(ten <= 500 && one <= 500);
}

static void TestNoGroupAsksNothing()
{
    RunGroupIndex index;
    index.Add(7, 1);
    std::uint64_t const before = index.Lookups();
    CHECK(RunOfGroup(index, 0) == 0);
    CHECK(index.Lookups() == before);
    CHECK(RunOfGroup(index, 7) == 1 && index.Lookups() == before + 1);
    CHECK(RunOfGroup(index, 8) == 0 && index.Lookups() == before + 2);
}

static void TestRebuild()
{
    RunGroupIndex index;
    index.Add(0, 5); // no group: not indexed
    CHECK(index.Size() == 0);
    index.Add(7, 1);
    index.Add(8, 2);
    CHECK(index.Size() == 2 && index.Find(8) == 2);
    index.Clear();
    CHECK(index.Size() == 0 && index.Find(7) == 0 && index.Find(8) == 0);
}

int main()
{
    TestLookupIsConstantPerBot();
    TestNoGroupAsksNothing();
    TestRebuild();
    std::printf("test_run_group_index: %d checks passed\n", checks);
    return 0;
}
