#pragma once

// Pure policy: which instance door a ghost's corpse run goes through.
//
// Scarlet Monastery r20-19z39 slot 7 (SM-GRAVEYARD-2026-10-22): the party died
// in the Library, every ghost released to the outdoor graveyard and walked to
// the door nearest the GHOST - the Graveyard wing's (trigger 45) - so the core
// raised them at 1687 1050, a wing with no way to the Library. The run leader
// then paused on a "navmesh island". The 300 s corpse-run timeout had the same
// flaw: sObjectMgr.GetMapEntranceTrigger(map) returns whichever trigger comes
// first in a hash map, which on a map with several doors is arbitrary.
//
// A player runs to the door of the part where the body lies. The rule here:
// among the area triggers that lead onto the corpse's map, the one whose
// DESTINATION lies nearest the corpse (3D) wins; destinations within
// kCorpseEntranceTieYd of each other are a tie, broken by the door nearest the
// ghost (only a door on the ghost's own map has a distance), then by the lower
// trigger id, so the pick never depends on the store's order. A single-door
// map gets its one door, as before.
//
// No navmesh "destination reaches the corpse" test: the ghost stands on
// another map (the graveyard outside), and asking the instance's navmesh
// would need that instance's Map and a unit on it. Nearest destination is the
// right door for every multi-door map with a plan in use (the Scarlet
// Monastery wings lie 1300+ yd apart; the Uldaman and Gnomeregan back doors
// land beside their own halves). Dire Maul's wings may need it later.
//
// Only external entrances count (Codex review of 5e818595): a trigger that
// sits on an open-world map (not an instanceable one) and leads onto another
// map. An intra-instance scripted teleport (source map == destination map) and
// a door from inside another instance (Molten Core's 2886 in Blackrock Depths,
// Blackwing Lair's 3726 in Blackrock Spire) are never candidates. The walk
// takes only doors on the ghost's own map. The 300 s timeout (a teleport, no
// walk) takes the nearest external door only when it sits on the ghost's map
// or on the map the instance is entered from (MapEntry::ghostEntranceMap);
// with none, the caller keeps the core's GetMapEntranceTrigger as before.
//
// Callers own the core lookups; only numbers travel here (no core includes,
// mirrors GraveyardTeleportPolicy.h).

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ai
{
    // Destinations this close to each other are the same distance from a corpse.
    constexpr float kCorpseEntranceTieYd = 1.0f;

    struct CorpseEntranceCandidate
    {
        std::uint32_t triggerId = 0;
        std::uint32_t sourceMap = 0;          // the map the trigger sits on
        bool sourceInstanceable = true;       // ... is a dungeon, raid or battleground (unknown: true, never taken)
        float destX = 0.0f, destY = 0.0f, destZ = 0.0f; // where the trigger puts the ghost (on the corpse's map)
        float ghostSqYd = -1.0f; // squared 2D distance ghost -> trigger; < 0: the trigger is on another map
    };

    inline float CorpseEntranceDestYd(CorpseEntranceCandidate const& c, float x, float y, float z)
    {
        float const dx = c.destX - x, dy = c.destY - y, dz = c.destZ - z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // Where the door search stands: the corpse's (destination) map, the
    // ghost's map, the map the instance is entered from (-1: none), and
    // whether it is the 300 s timeout's teleport rather than the walk.
    struct CorpseEntranceRules
    {
        std::uint32_t destMap = 0;
        std::uint32_t ghostMap = 0;
        std::int32_t entranceMap = -1;
        bool timeout = false;
    };

    // An external entrance: on an open-world map, leading onto another map.
    inline bool IsExternalEntrance(CorpseEntranceCandidate const& c, std::uint32_t destMap)
    {
        return c.sourceMap != destMap && !c.sourceInstanceable;
    }

    // Whether the door may be taken at all under these rules.
    inline bool CorpseEntranceEligible(CorpseEntranceCandidate const& c, CorpseEntranceRules const& rules)
    {
        if (!IsExternalEntrance(c, rules.destMap))
            return false;
        if (c.sourceMap == rules.ghostMap)
            return true;
        return rules.timeout && rules.entranceMap >= 0 && c.sourceMap == static_cast<std::uint32_t>(rules.entranceMap);
    }

    // Whether candidate `a` is a better door than `b` for a corpse at x,y,z.
    inline bool CorpseEntranceBetter(CorpseEntranceCandidate const& a, CorpseEntranceCandidate const& b,
        float x, float y, float z)
    {
        float const da = CorpseEntranceDestYd(a, x, y, z);
        float const db = CorpseEntranceDestYd(b, x, y, z);
        if (da + kCorpseEntranceTieYd < db)
            return true;
        if (db + kCorpseEntranceTieYd < da)
            return false;
        // A tie on the destination: the door the ghost can reach, then the nearer one.
        bool const aHere = a.ghostSqYd >= 0.0f, bHere = b.ghostSqYd >= 0.0f;
        if (aHere != bHere)
            return aHere;
        if (aHere && a.ghostSqYd != b.ghostSqYd)
            return a.ghostSqYd < b.ghostSqYd;
        return a.triggerId < b.triggerId;
    }

    // The index of the eligible door to take, or -1 when there is none.
    inline int PickCorpseEntrance(std::vector<CorpseEntranceCandidate> const& candidates,
        float corpseX, float corpseY, float corpseZ, CorpseEntranceRules const& rules)
    {
        int best = -1;
        for (std::size_t i = 0; i < candidates.size(); ++i)
        {
            if (!CorpseEntranceEligible(candidates[i], rules))
                continue;
            if (best < 0 || CorpseEntranceBetter(candidates[i], candidates[static_cast<std::size_t>(best)],
                    corpseX, corpseY, corpseZ))
                best = static_cast<int>(i);
        }
        return best;
    }
}
