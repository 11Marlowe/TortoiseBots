// Standalone test for the empty-slot backstop selection (EmptySlotFillPolicy.h):
// an empty slot takes a fitting item, a slot whose candidates all exceed the
// bot's level, item-level cap or quality band stays empty, a unique-equipped
// ring or a paired-slot duplicate is never taken, and the pick does not depend
// on candidate order.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_empty_slot_fill_policy.cpp -o /tmp/test_empty_slot_fill_policy
//   /tmp/test_empty_slot_fill_policy

#include "../ai/playerbot/strategy/values/EmptySlotFillPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace ai;

static int checks = 0;

#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static EmptySlotCandidate Item(uint32_t id, uint32_t ilvl, uint32_t reqLevel, uint32_t quality)
{
    EmptySlotCandidate c;
    c.itemId = id;
    c.itemLevel = ilvl;
    c.requiredLevel = reqLevel;
    c.quality = quality;
    c.equippable = true;
    return c;
}

static EmptySlotGates Gates(uint32_t botLevel, uint32_t maxIlvl)
{
    EmptySlotGates g;
    g.botLevel = botLevel;
    g.maxItemLevel = maxIlvl;
    g.minQuality = 0;
    g.maxQuality = 3; // RARE
    return g;
}

int main()
{
    // 1. Empty slot with a fitting low-level item takes it.
    {
        std::vector<EmptySlotCandidate> cands = { Item(7001, 12, 5, 2) };
        auto ranked = RankEmptySlotCandidates(Gates(10, 60), cands);
        CHECK(ranked.size() == 1);
        CHECK(cands[ranked[0]].itemId == 7001);
    }

    // 2. Candidates that exceed the bot's level stay empty.
    {
        std::vector<EmptySlotCandidate> cands = { Item(7002, 40, 35, 2), Item(7003, 30, 20, 2) };
        CHECK(RankEmptySlotCandidates(Gates(10, 60), cands).empty());
    }

    // 3. Candidates that exceed the item-level cap stay empty.
    {
        std::vector<EmptySlotCandidate> cands = { Item(7004, 71, 5, 2) };
        CHECK(RankEmptySlotCandidates(Gates(60, 70), cands).empty());
    }

    // 4. Quality outside the band is rejected (epic above RARE ceiling).
    {
        std::vector<EmptySlotCandidate> cands = { Item(7005, 20, 5, 4) };
        CHECK(RankEmptySlotCandidates(Gates(10, 60), cands).empty());
    }

    // 5. A unique-equipped ring (game equip check refuses) is never taken.
    {
        EmptySlotCandidate ring = Item(7006, 20, 5, 2);
        ring.equippable = false;
        std::vector<EmptySlotCandidate> cands = { ring };
        CHECK(RankEmptySlotCandidates(Gates(10, 60), cands).empty());
    }

    // 6. The same entry already worn in the paired ring/trinket slot is not doubled.
    {
        EmptySlotCandidate dup = Item(7007, 20, 5, 2);
        dup.pairedDuplicate = true;
        std::vector<EmptySlotCandidate> cands = { dup };
        CHECK(RankEmptySlotCandidates(Gates(10, 60), cands).empty());
    }

    // 7. Fresh-seed provenance failure rejects the candidate.
    {
        EmptySlotCandidate raid = Item(7008, 20, 5, 2);
        raid.passesProvenance = false;
        std::vector<EmptySlotCandidate> cands = { raid };
        CHECK(RankEmptySlotCandidates(Gates(10, 60), cands).empty());
    }

    // 8. A rejected candidate does not hide a fitting one; the best fitting wins
    //    regardless of input order (higher ilvl first, then quality, then lowest id).
    {
        std::vector<EmptySlotCandidate> cands = {
            Item(7009, 90, 5, 2),   // over the ilvl cap, rejected
            Item(7010, 30, 5, 2),
            Item(7011, 30, 5, 3),   // same ilvl, higher quality: wins
            Item(7012, 25, 5, 3),
        };
        auto ranked = RankEmptySlotCandidates(Gates(10, 60), cands);
        CHECK(ranked.size() == 3);
        CHECK(cands[ranked[0]].itemId == 7011);
        CHECK(cands[ranked[1]].itemId == 7010);
        CHECK(cands[ranked[2]].itemId == 7012);

        std::vector<EmptySlotCandidate> reversed(cands.rbegin(), cands.rend());
        auto rankedRev = RankEmptySlotCandidates(Gates(10, 60), reversed);
        CHECK(rankedRev.size() == 3);
        CHECK(reversed[rankedRev[0]].itemId == 7011);
    }

    std::printf("Standalone empty-slot fill policy tests passed (%d checks).\n", checks);
    return 0;
}
