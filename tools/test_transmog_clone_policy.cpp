// Standalone test for the transmog-clone exclusion rule: no item created by
// transmogrification — and no duplicate-copy row — may ever enter a pool a
// bot receives items from. The facts struct is the pure decision (see
// TransmogClonePolicy.h); the duplicate-copy condition is the real
// GearDuplicateItemIndex, composed the way RandomItemMgr::IsTransmogClone
// composes it for the module call sites.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_transmog_clone_policy.cpp -o /tmp/test_transmog_clone_policy
//   /tmp/test_transmog_clone_policy

#include "../ai/playerbot/TransmogClonePolicy.h"
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

// The composed gate a call site evaluates: two clone facts gathered from the
// prototype/core map plus the duplicate index.
static bool Excluded(GearDuplicateItemIndex const& index, bool destItemSet, uint32_t itemId)
{
    TransmogCloneFacts facts;
    facts.destItemSet = destItemSet;
    facts.idIsCloneId = false; // no core state in the standalone test
    facts.duplicateCopy = index.IsDuplicate(itemId);
    return IsTransmogClone(facts);
}

int main()
{
    // A clone copy (DestItemId set) is excluded even with an empty index.
    {
        GearDuplicateItemIndex index;
        CHECK(Excluded(index, true, 1200001));
        CHECK(Excluded(index, true, 42));
    }

    // The pure decision: any single condition excludes, none passes.
    {
        TransmogCloneFacts facts;
        CHECK(!IsTransmogClone(facts));
        facts.destItemSet = true;
        CHECK(IsTransmogClone(facts));
        TransmogCloneFacts byId;
        byId.idIsCloneId = true;
        CHECK(IsTransmogClone(byId));
        TransmogCloneFacts byCopy;
        byCopy.duplicateCopy = true;
        CHECK(IsTransmogClone(byCopy));
    }

    // The duplicate-copy condition is the real GearDuplicateItemIndex: a
    // materialised clone row (DestItemId 0 in the world DB, same name as a
    // lower-id item, id at or above the custom-id floor) is excluded, the
    // lowest-id original stays eligible.
    {
        GearDuplicateItemIndex index;
        index.Add(22798, "Might of Menethil");
        index.Add(1983153, "Might of Menethil");
        CHECK(!Excluded(index, false, 22798));
        CHECK(Excluded(index, false, 1983153));
    }

    // A duplicate-copy exclusion works with the clone facts too: the index
    // alone flags the materialised row, no DestItemId needed.
    {
        GearDuplicateItemIndex index;
        index.Add(8544, "Heavy Linen Bandage");
        index.Add(1980010, "Heavy Linen Bandage");
        CHECK(Excluded(index, false, 1980010));
    }

    // Fail-open: a normal item and a unique-name custom id are never
    // excluded, and ids below the floor never become copies.
    {
        GearDuplicateItemIndex index;
        index.Add(2581, "Linen Bandage");
        index.Add(1980999, "Some Genuine Custom Item");
        CHECK(!Excluded(index, false, 2581));
        CHECK(!Excluded(index, false, 1980999));
        // A same-name custom-id copy of 2581 is flagged, but the original
        // (below the floor) stays eligible.
        index.Add(1980998, "Linen Bandage");
        CHECK(Excluded(index, false, 1980998));
        CHECK(!Excluded(index, false, 2581));
    }

    std::printf("ok: %d checks\n", checks);
    return 0;
}
