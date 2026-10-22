#pragma once

// Pure decision rules for attacker target selection and AoE density
// (issue #1180): the loops inside AttackersValue::Calculate (shared-target
// copy validation) and AoeCountValue::FindMaxDensity (densest cluster pick)
// used to exist twice - once in the value, once as a mock copy inside
// tools/test_attackers_aoe_density.cpp that stayed green when the value
// changed. They live here std-only so the test compiles the real rules.
// Callers own the game state: unit lookup, target validity and 2D distance
// arrive as callables; range/aoeRadius and the distance tolerance are read
// from config by the caller.
// No core includes: callers translate game state into plain inputs.

#include <list>
#include <map>
#include <set>
#include <vector>

namespace ai
{
    // First-seen stable deduplication over an ordered guid list, keeping
    // only guids the predicate accepts (AttackersValue passes "unit exists
    // and IsValidForGroup"); getOne stops after the first accepted target.
    template <typename Guid, typename IsValidFn>
    inline std::list<Guid> DistinctValidTargets(const std::list<Guid>& guids, IsValidFn isValid, bool getOne)
    {
        std::list<Guid> distinctResult;
        std::set<Guid> seen;

        for (const Guid& guid : guids)
        {
            if (!seen.insert(guid).second)
                continue;

            if (isValid(guid))
            {
                distinctResult.push_back(guid);
                if (getOne)
                    break;
            }
        }

        return distinctResult;
    }

    // Densest AoE cluster among the attackers (AoeCountValue::FindMaxDensity):
    // dedup first-seen, then every attacker within `range` of the bot scores
    // the set of attackers within aoeRadius * 2 of itself; the biggest set
    // wins, first-seen wins ties. The result is the winning cluster in the
    // original attackers priority order.
    // getUnit(guid) returns the unit or null when unknown; dist2d returns
    // plain 2D distance. Distance checks tolerate `distanceEpsilon`
    // (production: sPlayerbotAIConfig.targetPosRecalcDistance) exactly like
    // sServerFacade.IsDistanceLessOrEqualThan, so callers pass 0 for an
    // exact comparison.
    template <typename Guid, typename Unit, typename GetUnitFn, typename DistFn>
    inline std::list<Guid> FindMaxDensityCluster(const std::list<Guid>& units, GetUnitFn getUnit,
        DistFn dist2d, Unit* bot, float range, float aoeRadius, float distanceEpsilon)
    {
        size_t maxCount = 0;
        Guid maxGroup{};
        std::map<Guid, std::set<Guid>> groups;
        std::vector<Guid> uniqueUnits;

        // Stable first-seen deduplication preserving attackers priority order
        std::set<Guid> seen;
        for (const Guid& guid : units)
        {
            if (seen.insert(guid).second)
                uniqueUnits.push_back(guid);
        }

        for (const Guid& guid : uniqueUnits)
        {
            Unit* unit = getUnit(guid);
            if (!unit)
                continue;

            float distanceToPlayer = dist2d(unit, bot);
            if (distanceToPlayer - range <= distanceEpsilon)
            {
                for (const Guid& otherGuid : uniqueUnits)
                {
                    Unit* other = getUnit(otherGuid);
                    if (!other)
                        continue;

                    float d = dist2d(unit, other);
                    if (d - aoeRadius * 2.0f <= distanceEpsilon)
                        groups[guid].insert(otherGuid);
                }

                if (maxCount < groups[guid].size())
                {
                    maxCount = groups[guid].size();
                    maxGroup = guid;
                }
            }
        }

        if (!maxCount)
            return std::list<Guid>();

        // Return the cluster members in original attackers priority order
        std::list<Guid> result;
        const std::set<Guid>& maxSet = groups[maxGroup];
        for (const Guid& guid : uniqueUnits)
        {
            if (maxSet.find(guid) != maxSet.end())
                result.push_back(guid);
        }

        return result;
    }
} // namespace ai
