
#include "playerbot/playerbot.h"
#include "MimicConsumableStrategy.h"

using namespace ai;

void MimicConsumableStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "master use item",
        NextAction::array(0, new NextAction("mimic consumable", 1.5f), NULL)));
}

void MimicConsumableStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    (void)triggers;
}
