#pragma once
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/Value.h"
#include "TargetValue.h"

namespace ai
{
    // A crowd control spell and the aura it leaves on its victim do not
    // always share a name: the Freezing Trap spell applies spell 3355,
    // "freezing trap effect", so an aura lookup by the cast name never
    // matches and a held mob reads as free (re-trapping would destroy the
    // live trap). Translate the cast name to its aura name; every other CC
    // spell names its own aura.
    inline std::string CcAuraName(const std::string& spell)
    {
        if (spell == "freezing trap")
            return "freezing trap effect";
        return spell;
    }

    class CurrentCcTargetValue : public TargetValue, public Qualified
	{
	public:
        CurrentCcTargetValue(PlayerbotAI* ai, std::string name = "current cc target") : TargetValue(ai, name), Qualified() {}

    public:
        Unit* Calculate() override;
    };
}
