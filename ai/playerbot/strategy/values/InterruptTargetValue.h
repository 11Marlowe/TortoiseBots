#pragma once
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/Value.h"

namespace ai
{
    // The bot's interrupt pick: the best casting attacker the interrupt can
    // reach (InterruptTargetPolicy.h). Qualified by the interrupt spell
    // ("interrupt target::kick"), like "enemy healer target", so the trigger
    // and its action read the same pick.
    class InterruptTargetValue : public UnitCalculatedValue, public Qualified
    {
    public:
        InterruptTargetValue(PlayerbotAI* ai) : UnitCalculatedValue(ai, "interrupt target"), Qualified() {}

    protected:
        virtual Unit* Calculate() override;
    };
}
