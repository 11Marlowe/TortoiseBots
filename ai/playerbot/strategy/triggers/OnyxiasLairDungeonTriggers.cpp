
#include "playerbot/playerbot.h"
#include "OnyxiasLairDungeonTriggers.h"

using namespace ai;

bool OnyxiaAirborneTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target || !sServerFacade.IsAlive(target) || target->GetEntry() != 10184)
        return false;
    // Core drives phase 2 flight via the Hover aura (17131); IsFlying()
    // covers the transition frames where auras are mid-update.
    return target->HasAura(17131) || target->IsFlying();
}