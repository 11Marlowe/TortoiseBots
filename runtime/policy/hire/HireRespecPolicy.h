#pragma once

// Issue #490 PR 3: a respec gives a tier-kit hire the same tier's kit for
// its new spec. The respec path only knows the applied premade path name
// (e.g. "holy", "feral"); the gear table keys off the recruiter menu
// position (e.g. druid bear vs cat share the feral path but need distinct
// keys). This maps a premade path name (+ the AI's forced role, which picks
// Bear vs Cat) back to the menu index HireGearSpecKey understands.
//
// Pure policy: no core/engine headers, unit-tested standalone
// (tools/test_hire_respec_policy.cpp).

#include <cstdint>
#include <string>

#include "../../HireSpecPolicy.h"

namespace TortoiseBots
{

// Premade path name -> recruiter menu index for a class. nullptr path falls
// back to the role's first menu option (same rule as the hire path's
// ResolveHireGearSpecKey). Druid feral needs the forced role: tank -> Bear
// (index 0), anything else -> Cat (index 3). -1 when the name matches no
// option of that class (caller keeps the current kit).
inline int HireSpecIndexForPathName(uint8_t classId, char const* pathName, uint8_t forcedRole)
{
    uint32_t count = 0;
    HireSpecOption const* options = HireSpecOptions(classId, count);
    if (!options)
        return -1;
    if (classId == kHireClassDruid && pathName && std::string(pathName).find("feral") != std::string::npos)
        return (forcedRole & kHireRoleTank) ? 0 : 3;
    if (!pathName || !*pathName)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            if (options[i].role == forcedRole)
                return static_cast<int>(i);
        }
        return -1;
    }
    std::string name(pathName);
    // Exact first: premade path names are the config values the menu table
    // was written against, so "frost" never fuzzy-matches another option.
    for (uint32_t i = 0; i < count; ++i)
    {
        if (name == options[i].pathName)
        {
            // Bear/Cat share the feral path name: the forced role decides.
            if (classId == kHireClassDruid && std::string(options[i].pathName) == "feral")
                return (forcedRole & kHireRoleTank) ? 0 : 3;
            return static_cast<int>(i);
        }
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        if (name.find(options[i].pathName) != std::string::npos)
        {
            // Bear/Cat share the feral path name: the forced role decides.
            if (classId == kHireClassDruid && std::string(options[i].pathName) == "feral")
                return (forcedRole & kHireRoleTank) ? 0 : 3;
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace TortoiseBots
