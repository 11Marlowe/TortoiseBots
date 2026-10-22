// Standalone test for the duplicate-item-copy rule: a re-modelled copy of an
// existing item (same exact name, item id at or above the custom-id floor)
// never enters the bot gear pools; the lowest-id instance stays eligible.
// Ids below the floor are never flagged, and custom ids with a unique name
// stay eligible (fail-open for genuine custom content).
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_gear_duplicate_item_policy.cpp -o /tmp/test_gear_duplicate_item_policy
//   /tmp/test_gear_duplicate_item_policy

#include "../ai/playerbot/GearDuplicateItemPolicy.h"

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

int main()
{
    // The live-realm case: 1983153 is a re-modelled copy of 22798, fed in
    // ascending item id order like the RandomItemMgr item walk.
    {
        GearDuplicateItemIndex index;
        index.Add(22798, "Might of Menethil");
        index.Add(1983153, "Might of Menethil");
        CHECK(!index.IsDuplicate(22798));
        CHECK(index.IsDuplicate(1983153));
    }

    // Ids below the floor are never flagged, however many share a name: the
    // vanilla data legitimately re-uses names (PvP rank items, class
    // variants such as the four Atiesh staves, consumables).
    {
        GearDuplicateItemIndex index;
        index.Add(22589, "Atiesh, Greatstaff of the Guardian");
        index.Add(22630, "Atiesh, Greatstaff of the Guardian");
        index.Add(22631, "Atiesh, Greatstaff of the Guardian");
        index.Add(22632, "Atiesh, Greatstaff of the Guardian");
        CHECK(!index.IsDuplicate(22589));
        CHECK(!index.IsDuplicate(22630));
        CHECK(!index.IsDuplicate(22631));
        CHECK(!index.IsDuplicate(22632));
    }

    // A custom id above the floor flags only against a lower same-name item.
    {
        GearDuplicateItemIndex index;
        index.Add(150000, "Custom Original");
        index.Add(200000, "Custom Original");
        index.Add(200001, "Custom Other");
        CHECK(!index.IsDuplicate(150000));
        CHECK(index.IsDuplicate(200000));
        CHECK(!index.IsDuplicate(200001));
    }

    // Exact-name match only: different or case-different names are not copies.
    {
        GearDuplicateItemIndex index;
        index.Add(1900, "Small Sword");
        index.Add(1983153, "Small  Sword");
        index.Add(1983154, "small sword");
        index.Add(1983155, "Small Sword ");
        CHECK(!index.IsDuplicate(1983153));
        CHECK(!index.IsDuplicate(1983154));
        CHECK(!index.IsDuplicate(1983155));
    }

    // Empty or missing names never register, so they can never flag copies.
    {
        GearDuplicateItemIndex index;
        index.Add(1, "");
        index.Add(100001, "");
        index.Add(100002, nullptr);
        CHECK(!index.IsDuplicate(100001));
        CHECK(!index.IsDuplicate(100002));
    }

    // Item id 0 never registers.
    {
        GearDuplicateItemIndex index;
        index.Add(0, "Weird Row");
        CHECK(!index.IsDuplicate(0));
    }

    // The floor keeps genuine Tortoise content (ids up to ~92000) eligible
    // while every shipped row at or above it is a copy.
    CHECK(kDuplicateItemIdFloor == 100000);

    std::printf("test_gear_duplicate_item_policy: %d checks passed\n", checks);
    return 0;
}
