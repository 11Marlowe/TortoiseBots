
#include "playerbot/playerbot.h"
#include "LowerKarazhanDungeonTriggers.h"
#include "Maps/CellImpl.h"

using namespace ai;

bool AraxxnaSwarmTrigger::IsActive()
{
    if (!bot->IsInWorld() || bot->IsBeingTeleported())
        return false;
    std::list<Unit*> units;
    MaNGOS::AllCreaturesOfEntryInRange check(bot, 30008, sPlayerbotAIConfig.sightDistance);
    MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRange> searcher(units, check);
    Cell::VisitAllObjects(bot, searcher, sPlayerbotAIConfig.sightDistance);
    for (Unit* unit : units)
    {
        if (unit && sServerFacade.IsAlive(unit) && !unit->IsFriendlyTo(bot))
            return true;
    }
    return false;
}
