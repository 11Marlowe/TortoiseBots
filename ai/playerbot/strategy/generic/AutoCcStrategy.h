#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    // Opt-in smart auto CC ("auto cc" strategy, OFF by default). A plain
    // marker strategy: while present on the combat engine, CcTargetValue may
    // pick the loose-add auto target and HasCcTargetTrigger bypasses the
    // 5-man dungeon mark gate. Toggled via `.bot action auto cc [on|off]`
    // (persisted through PlayerbotDbStore like the other toggles) or
    // `.bot strategy +/-auto cc`. No triggers of its own.
    class AutoCcStrategy : public Strategy
    {
    public:
        AutoCcStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "auto cc"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override { (void)triggers; }
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override { (void)triggers; }
    };
}
