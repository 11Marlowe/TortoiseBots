#pragma once

// Issue #1125, the low-level half of the shaman tank identity.
//
// A pool shaman that rolls the 7.3 tank build is made a tank at birth, and
// that role lives in memory only (PlayerbotFactory::SelectPremadeSpecNo), so a
// server restart drops it. From level 25 the key talents (Totemic Alignment /
// Spirit Armor) carry the build on their own (ShamanTankDetectionPolicy.h);
// below that the tank levelling order has learned neither, and the spent
// levelling layout is what is left - talents are persisted, the role is not.
//
// It is enough, because the 7.3 order allocates its points in a way no other
// shaman premade does (AiPlayerbot.PremadeSpecLink.7.x.<level>): matching the
// bot's spent ranks against the premade entry for its level identifies the
// build from the level the paths diverge on.
//
// The read must not claim the tank build off a shared layout: at level 10-14
// the 7.3 and 7.1 (enhancement) links are identical, so every shaman matches
// both and the character cannot tell them apart. The rule is therefore "the
// tank entry matches AND no other premade entry does", which fails closed (a
// damage shaman stays damage) wherever the layout is ambiguous.
//
// Pure policy: no core/engine headers. The rank vectors are built from
// TalentSpec in AiFactory.cpp; this file only compares them and states the
// rule. Unit-tested standalone (tools/test_shaman_tank_leveling_policy.cpp);
// pinned against the DBC and the shipped conf by
// tools/talents/validate_presets.py.

#include <cstdint>
#include <map>

namespace TortoiseBots
{

// The first level at which the 7.3 levelling order allocates a key tank
// talent (Spirit Armor, AiPlayerbot.PremadeSpecLink.7.3.25). From there the
// key-talent read is authoritative, and the gate keeps this inference - and
// its talent-store walk - off every shaman at or above it.
// tools/talents/validate_presets.py fails when this and the conf/DBC disagree.
inline constexpr uint8_t kShamanTankFirstTalentLevel = 25;

// Talent id -> allocated rank, one entry per class talent (rank 0 included).
using ShamanTalentRanks = std::map<int, int>;

// Same build? A talent missing on one side counts as rank 0 there, so a caller
// that passes only the allocated talents still compares honestly.
inline bool ShamanTalentRanksEqual(ShamanTalentRanks const& current, ShamanTalentRanks const& target)
{
    for (auto const& entry : current)
    {
        auto it = target.find(entry.first);
        if (entry.second != (it == target.end() ? 0 : it->second))
            return false;
    }
    for (auto const& entry : target)
    {
        if (entry.second == 0)
            continue;
        auto it = current.find(entry.first);
        if (it == current.end() || it->second != entry.second)
            return false;
    }
    return true;
}

// On the 7.3 tank levelling path: the tank premade's entry for the bot's level
// matches its spent ranks, and no other shaman premade's entry does.
inline bool ShamanOnTankLevelingPath(bool tankEntryMatches, int otherPremadeMatches)
{
    return tankEntryMatches && otherPremadeMatches == 0;
}

} // namespace TortoiseBots
