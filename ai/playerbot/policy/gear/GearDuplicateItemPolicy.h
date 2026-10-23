#pragma once

// Pure duplicate-item-copy rule. The Tortoise world DB carries re-modelled
// copies of existing items: the same name, the same stats, a new display id,
// at an item id far above the original, with no loot, vendor or quest source
// (139/139 such rows in the live realm DB, e.g. 1983153 "Might of Menethil"
// copying 22798). Players never see them drop; bots must never receive them
// either. What counts: an item id at or above the custom-id floor whose exact
// name also belongs to a lower-id item — the lowest-id instance stays
// eligible, every higher-id copy is excluded. Ids below the floor are never
// flagged (all genuine 1.12-era and Tortoise content sits there), and a
// custom-id item with a unique name is never flagged (fail-open for real
// future custom content). The predicates are supplied by the caller, so this
// header stays free of core and database types. Exercised standalone in
// tools/test_gear_duplicate_item_policy.cpp.

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace TortoiseBots
{

// Every genuine Tortoise item id observed on the live realm sits below
// 100000; every shipped row at or above it is a duplicate copy.
uint32_t constexpr kDuplicateItemIdFloor = 100000;

// Name -> lowest-id instance. Feed every prototype once, in ascending item id
// order (the item walk in RandomItemMgr guarantees this); the first id seen
// for a name becomes the original, later same-name ids are copies.
class GearDuplicateItemIndex
{
public:
    void Add(uint32_t itemId, char const* name)
    {
        if (!itemId || !name || !*name)
            return;

        // Sub-floor ids are never copies, even when the vanilla data
        // legitimately re-uses a name; they still register their name so a
        // later custom-id copy is flagged against them.
        if (itemId < kDuplicateItemIdFloor)
        {
            firstIdByName_.emplace(name, itemId);
            return;
        }

        auto const inserted = firstIdByName_.emplace(name, itemId);
        if (!inserted.second)
            duplicateItems_.insert(itemId);
    }

    bool IsDuplicate(uint32_t itemId) const
    {
        return duplicateItems_.count(itemId) != 0;
    }

private:
    std::unordered_map<std::string, uint32_t> firstIdByName_;
    std::unordered_set<uint32_t> duplicateItems_;
};

} // namespace TortoiseBots
