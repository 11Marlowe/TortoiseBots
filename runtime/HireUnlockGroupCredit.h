// Issue #490 PR 3 (credit): who gets hire-unlock kill credit for one boss
// death. Pure, core-free: given the killer's group roster (guids, bot
// flags, reward-distance flags), returns the guid-lows to credit. Rules:
// the killer always counts (they are in reward distance of their own kill),
// every non-bot group member in reward distance counts, bots never count
// (a bot landing the blow must not starve the party's real players), and
// the list is deduped. Group iteration, bot checks and distance checks live
// in the adapter (they need core types); this header only encodes the
// filter so it stays unit-testable standalone.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace TortoiseBots
{

struct HireUnlockCreditCandidate
{
    uint32_t guidLow;
    bool isBot;
    bool inRewardDistance;
};

// guidLow first so the adapter can reuse one vector for both passes.
inline void HireUnlockCreditTargets(uint32_t killerGuidLow,
    HireUnlockCreditCandidate const* members, size_t memberCount,
    std::vector<uint32_t>& targets)
{
    targets.clear();
    targets.push_back(killerGuidLow);
    for (size_t i = 0; i < memberCount; ++i)
    {
        if (members[i].guidLow == killerGuidLow)
            continue;
        if (members[i].isBot)
            continue;
        if (!members[i].inRewardDistance)
            continue;
        targets.push_back(members[i].guidLow);
    }
}

} // namespace TortoiseBots
