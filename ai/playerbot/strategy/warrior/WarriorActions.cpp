
#include "playerbot/playerbot.h"
#include "WarriorActions.h"
#include "playerbot/strategy/values/AttackerOfNonTankValue.h"

using namespace ai;

bool CastTauntOnAttackerAction::Execute(Event& event)
{
    Unit* target = GetTarget();
    if (!target || !CastSpellAction::Execute(event))
        return false;
    // At most one Taunt per mob per TauntAttackerRepeatMs.
    if (AttackerOfNonTankValue* value = dynamic_cast<AttackerOfNonTankValue*>(GetTargetValue()))
        value->NoteTaunt(target);
    return true;
}
