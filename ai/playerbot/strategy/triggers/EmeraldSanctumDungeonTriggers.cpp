
#include "playerbot/playerbot.h"
#include "EmeraldSanctumDungeonTriggers.h"
#include "Maps/CellImpl.h"

using namespace ai;

bool SolniusAddsTrigger::IsActive()
{
    if (!bot->IsInWorld() || bot->IsBeingTeleported())
        return false;
    static const uint32 addEntries[] = { 61212, 60746, 60743, 60745 };
    std::list<Unit*> units;
    for (uint32 entry : addEntries)
    {
        MaNGOS::AllCreaturesOfEntryInRange check(bot, entry, sPlayerbotAIConfig.sightDistance);
        MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRange> searcher(units, check);
        Cell::VisitAllObjects(bot, searcher, sPlayerbotAIConfig.sightDistance);
    }
    for (Unit* unit : units)
    {
        if (unit && sServerFacade.IsAlive(unit) && !unit->IsFriendlyTo(bot))
            return true;
    }
    return false;
}
