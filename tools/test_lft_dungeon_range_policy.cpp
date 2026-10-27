#include "../runtime/policy/lfg/LftDungeonRangePolicy.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK(x) do { \
    ++checks; \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using TortoiseBots::LftDungeonRange;
using TortoiseBots::LftDungeonRangeTable;
using TortoiseBots::LftLevelRange;
using TortoiseBots::NormalizeLftToken;

namespace
{
int checks = 0;

static void TestLookups()
{
    LftLevelRange const* rfc = LftDungeonRange("Ragefire Chasm");
    CHECK(rfc && rfc->minLevel == 13 && rfc->maxLevel == 18);
    CHECK(LftDungeonRange("RFC") && LftDungeonRange("RFC")->minLevel == rfc->minLevel && LftDungeonRange("RFC")->maxLevel == rfc->maxLevel);
    CHECK(LftDungeonRange("dm") && LftDungeonRange("dm")->minLevel == 17 && LftDungeonRange("dm")->maxLevel == 24);
    CHECK(LftDungeonRange("stockade") && LftDungeonRange("stockade")->minLevel == 22);
    CHECK(LftDungeonRange("cotbm") && LftDungeonRange("cotbm")->minLevel == 60);
    CHECK(LftDungeonRange("dragonmawretreat") == nullptr);
    CHECK(LftDungeonRange("") == nullptr);
    CHECK(NormalizeLftToken("Zul'Farrak") == "zulfarrak");
}

static void TestEveryRangeValid()
{
    CHECK(LftDungeonRangeTable().size() == 76);
    for (auto const& entry : LftDungeonRangeTable())
    {
        CHECK(entry.first == NormalizeLftToken(entry.first));
        CHECK(entry.second.minLevel > 0 && entry.second.minLevel <= entry.second.maxLevel && entry.second.maxLevel <= 60);
    }
}
}

int main()
{
    TestLookups();
    TestEveryRangeValid();
    std::printf("lft dungeon range policy: %d checks passed\n", checks);
    return 0;
}
