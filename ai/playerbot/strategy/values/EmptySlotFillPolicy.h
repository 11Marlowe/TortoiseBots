#pragma once

// Empty-slot backstop (PlayerbotFactory::InitEquipment). After the normal pass,
// a slot that is still empty takes the best candidate that passes the same hard
// gates the normal pass applies to that bot: required level, item-level cap,
// quality band, fresh-seed provenance, no second copy in the paired ring/trinket
// slot, and the game's own equip check (unique-equipped rule). Stat weight and
// spec scoring are the soft preference and are relaxed here.
//
// Pure and core-free, so tools/test_empty_slot_fill_policy.cpp includes it directly.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ai
{
    struct EmptySlotCandidate
    {
        uint32_t itemId = 0;
        uint32_t itemLevel = 0;
        uint32_t requiredLevel = 0;
        uint32_t quality = 0;
        bool passesProvenance = true;       // fresh-seed provenance (true off the seed path)
        bool pairedDuplicate = false;       // same entry already worn in the paired ring/trinket slot
        bool equippable = false;            // RandomBotFacade::CanEquipUnseenItem == EQUIP_ERR_OK
    };

    constexpr uint32_t kItemLevelWindow = 10;

    struct EmptySlotGates
    {
        uint32_t botLevel = 0;
        uint32_t maxItemLevel = 0;
        uint32_t minQuality = 0;
        uint32_t maxQuality = 0;
    };

    // True when the candidate passes every hard gate for an empty slot.
    inline bool EmptySlotAccepts(EmptySlotGates const& g, EmptySlotCandidate const& c)
    {
        // Item level may sit at most kItemLevelWindow above the bot's level:
        // many rings/necks/trinkets require level 1-5 yet carry item level 30+,
        // and the ranking below would otherwise hand them to a fresh bot.
        return c.requiredLevel <= g.botLevel
            && c.itemLevel <= g.maxItemLevel
            && c.itemLevel <= g.botLevel + kItemLevelWindow
            && c.quality >= g.minQuality
            && c.quality <= g.maxQuality
            && c.passesProvenance
            && !c.pairedDuplicate
            && c.equippable;
    }

    // Indices of the accepted candidates, best first: highest item level, then
    // higher quality, then lowest item id. The order never depends on input order.
    inline std::vector<size_t> RankEmptySlotCandidates(EmptySlotGates const& g, std::vector<EmptySlotCandidate> const& cands)
    {
        std::vector<size_t> ranked;
        for (size_t i = 0; i < cands.size(); ++i)
            if (EmptySlotAccepts(g, cands[i]))
                ranked.push_back(i);
        std::sort(ranked.begin(), ranked.end(), [&cands](size_t a, size_t b)
        {
            EmptySlotCandidate const& x = cands[a];
            EmptySlotCandidate const& y = cands[b];
            if (x.itemLevel != y.itemLevel)
                return x.itemLevel > y.itemLevel;
            if (x.quality != y.quality)
                return x.quality > y.quality;
            return x.itemId < y.itemId;
        });
        return ranked;
    }
}
