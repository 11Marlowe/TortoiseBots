#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    // Opt-in mimicry strategy (off by default; `.bot behavior <bot> mimic
    // on|off`). Lives on the non-combat engine; the trigger fires only from
    // the master's CMSG_USE_ITEM packet and the action re-checks range,
    // combat and mount state before casting.
    class MimicConsumableStrategy : public Strategy
    {
    public:
        MimicConsumableStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "mimic consumables"; }

    private:
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };
}
