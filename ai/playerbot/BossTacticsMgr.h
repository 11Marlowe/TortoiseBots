#pragma once

#include "BossTacticsPolicy.h"
#include "DungeonPlanReloadPolicy.h"

#include <cstdint>

namespace ai
{
    // The current table, filled by LoadBossTactics() and read-only afterwards; `.bot run reloadplans` replaces it with a
    // new sealed table (DungeonPlanMgr.h ReloadDungeonPlans). Nothing keeps a row pointer past the call that read it.
    BossTacticTable& GetBossTactics();
    ReloadableTable<BossTacticTable>& BossTacticsHolder();

    // Reads `ai_boss_tactic` once at startup (one pass over the rows) and seals
    // the table. A second call does nothing.
    void LoadBossTactics();

    // One pass over `ai_boss_tactic` into an empty table, then its seal: the rows skipped. `rowsRead` (a reload) gets
    // the rows the query handed it, accepted or not.
    std::uint32_t ReadBossTacticRows(BossTacticTable& table, std::uint64_t* rowsRead = nullptr);
}
