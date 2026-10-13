// Standalone check for runtime/NamedLocationIndex.h:
//   g++ -std=c++17 -Wall -Wextra tools/test_named_location_index.cpp -o /tmp/test_named_location_index && /tmp/test_named_location_index
#include "../runtime/NamedLocationIndex.h"

#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace TortoiseBots;

static NamedLocationPoint Point(uint32_t mapId, float x, float y, float z, float o)
{
    NamedLocationPoint point;
    point.mapId = mapId;
    point.x = x;
    point.y = y;
    point.z = z;
    point.orientation = o;
    return point;
}

static bool SameLocation(NamedLocationPoint const& a, NamedLocationPoint const& b)
{
    return a.mapId == b.mapId && a.x == b.x && a.y == b.y && a.z == b.z && a.orientation == b.orientation;
}

int main()
{
    std::cout << "Starting TortoiseBots named-location index tests...\n";

    // Key form: ASCII case and trailing spaces do not change the key.
    CHECK(NamedLocationKey("AV_TOWERPOINT") == "AV_TOWERPOINT");
    CHECK(NamedLocationKey("av_towerpoint") == "AV_TOWERPOINT");
    CHECK(NamedLocationKey("AV_TOWERPOINT   ") == "AV_TOWERPOINT");
    CHECK(NamedLocationKey("AV_TOWER POINT") == "AV_TOWER POINT");
    CHECK(NamedLocationKey(" AV") == " AV");
    CHECK(NamedLocationKey("") == "");
    std::cout << "  [PASS] key form: case and trailing spaces\n";

    NamedLocationIndex index;
    CHECK(!index.Sealed());
    CHECK(index.Add("AV_TOWERPOINT", Point(30, -768.5f, -362.25f, 90.5f, 1.25f)));
    CHECK(index.Add("AV_SNOWFALL_GRAVEYARD", Point(30, -202.0f, -113.0f, 78.0f, -1.5f)));
    NamedLocationPoint out;
    // Nothing is served before the seal.
    CHECK(!index.Find("AV_TOWERPOINT", out));
    index.Seal();
    CHECK(index.Sealed());
    std::cout << "  [PASS] nothing served before the seal\n";

    // Found by any spelling of the name, with the stored values unchanged.
    CHECK(index.Find("AV_TOWERPOINT", out));
    CHECK(SameLocation(out, Point(30, -768.5f, -362.25f, 90.5f, 1.25f)));
    CHECK(index.Find("av_towerpoint  ", out));
    CHECK(SameLocation(out, Point(30, -768.5f, -362.25f, 90.5f, 1.25f)));
    CHECK(index.Find("AV_SNOWFALL_GRAVEYARD", out));
    CHECK(out.mapId == 30 && out.orientation == -1.5f);
    std::cout << "  [PASS] found by any spelling, values unchanged\n";

    // Misses are not found and leave the output untouched.
    out.mapId = 777;
    CHECK(!index.Find("AV", out));
    CHECK(!index.Find("AV_", out));
    CHECK(!index.Find("AV_TOWERPOINT_X", out));
    CHECK(!index.Find("", out));
    CHECK(out.mapId == 777);
    std::cout << "  [PASS] misses are not found and do not write\n";

    // Duplicates: the first row wins, as under the table's primary key.
    CHECK(!index.Add("av_towerpoint", Point(0, 1.0f, 1.0f, 1.0f, 1.0f)));
    CHECK(index.Size() == 2);
    CHECK(index.Find("AV_TOWERPOINT", out) && out.mapId == 30);
    std::cout << "  [PASS] first row wins\n";

    // Immutable after the seal: Add refuses, lookups are unchanged.
    CHECK(!index.Add("AV_NEW_POINT", Point(30, 0.0f, 0.0f, 0.0f, 0.0f)));
    CHECK(index.Size() == 2);
    CHECK(!index.Find("AV_NEW_POINT", out));
    CHECK(index.Find("AV_TOWERPOINT", out));
    std::cout << "  [PASS] refuses rows after the seal\n";

    // Scale: one pass over about the shipped row count, every sampled row found after the seal.
    {
        NamedLocationIndex big;
        constexpr uint32_t rows = 52783;
        for (uint32_t i = 0; i < rows; ++i)
        {
            std::string name = "FISH_LOCATION_0_" + std::to_string(i % 600) + "_" + std::to_string(i);
            CHECK(big.Add(name, Point(0, static_cast<float>(i), 1.0f, 2.0f, 3.0f)));
        }
        big.Seal();
        CHECK(big.Size() == rows);
        for (uint32_t i = 0; i < rows; i += 997)
        {
            std::string name = "FISH_LOCATION_0_" + std::to_string(i % 600) + "_" + std::to_string(i);
            CHECK(big.Find(name, out) && out.x == static_cast<float>(i));
        }
    }
    std::cout << "  [PASS] 52,783 rows loaded and found\n";

    std::cout << "All named-location index tests passed.\n";
    return 0;
}
