
#include "playerbot/playerbot.h"
#include "ShamanActions.h"

using namespace ai;

// Self-target twin of HealPartyMemberAction::Execute: the party Execute
// only runs for the "on party" lines, so the self rows always paid top
// rank. Same wound rule via the shared DownrankSpellId (now protected),
// own target — top rank on emergency, cheapest fitting rank otherwise.
bool CastSelfDownrankHealingAction::Execute(Event& event)
{
    if (Unit* target = GetTarget())
        if (uint32 const rankId = DownrankSpellId(target))
            SetSpellId(rankId);
    return CastSpellAction::Execute(event);
}