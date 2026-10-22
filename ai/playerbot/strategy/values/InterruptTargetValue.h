#pragma once
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/Value.h"

namespace ai
{
    // Issue #1158: the bot's interrupt pick - any engaged hostile in the
    // interrupt spell's range that is casting something the interrupt can
    // stop, best cast first (InterruptTargetPolicy.h). Qualified by the
    // interrupt spell ("interrupt target::kick"), like "enemy healer
    // target", so the trigger and its action read the same pick.
    class InterruptTargetValue : public UnitCalculatedValue, public Qualified
    {
    public:
        InterruptTargetValue(PlayerbotAI* ai) : UnitCalculatedValue(ai, "interrupt target"), Qualified() {}

    protected:
        virtual Unit* Calculate() override;
    };
}
