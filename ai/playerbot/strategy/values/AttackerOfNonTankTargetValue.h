#pragma once
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/Value.h"

namespace ai
{
    // The engaged hostile the tank should Taunt off a non-tank group member:
    // the closest mob whose victim is a non-tank member, the healer's
    // attacker first, within the scan radius below. Players are never
    // returned (no threat list to taunt). Selection rules live in
    // AttackerOfNonTankPolicy.h; closing the last yards is the taunt cast's
    // own spell range plus reach prerequisite.
    class AttackerOfNonTankTargetValue : public UnitCalculatedValue
	{
	public:
        AttackerOfNonTankTargetValue(PlayerbotAI* ai) :
            UnitCalculatedValue(ai, "attacker of non-tank target") {}

    protected:
        virtual Unit* Calculate() override;
	};
}
