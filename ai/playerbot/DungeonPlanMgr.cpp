#include "playerbot/playerbot.h"
#include "DungeonPlanMgr.h"
#include "BossTacticsMgr.h"

using namespace ai;

// The current table, and the replaced ones a run in progress still points into (DungeonPlanReloadPolicy.h).
static ReloadableTable<DungeonPlanTable>& DungeonPlanHolder()
{
    static ReloadableTable<DungeonPlanTable> holder;
    return holder;
}

DungeonPlanTable& ai::GetDungeonPlans()
{
    return DungeonPlanHolder().Current();
}

namespace
{
    // A move this much longer than the one before it is logged (the exporter
    // should cut the leg into via points).
    constexpr float kLongHopYd = 60.0f;
}

// One pass over each table into `table` (empty, not sealed), then the seal: the startup load and `.bot run reloadplans`
// both read through here, so a reload checks every row as the startup did.
static void ReadDungeonPlanRows(DungeonPlanTable& table, PlanLoadCounts& counts)
{
    std::uint32_t& skippedPlans = counts.skippedPlans;
    if (auto result = WorldDatabase.Query(
        "SELECT `plan`, `map`, `cc_priority`, `cc_always`, `kill_first` FROM `ai_dungeon_plan`"))
    {
        do
        {
            Field* fields = result->Fetch();
            std::string const plan = fields[0].GetCppString();
            if (!table.AddPlan(plan, fields[1].GetUInt32()))
            {
                ++skippedPlans;
                counts.Note("ai_dungeon_plan '" + plan + "' (map " + std::to_string(fields[1].GetUInt32()) +
                    "): an empty name, map 0 or a second row of the name");
            }
            else if (!table.SetUnits(plan, fields[2].GetCppString(), fields[3].GetCppString(), fields[4].GetCppString()))
            {
                ++counts.badUnits;
                counts.Note("ai_dungeon_plan '" + plan + "': cc_priority, cc_always or kill_first is not a list of creature entries");
                sLog.outError("ai_dungeon_plan: plan '%s': cc_priority, cc_always or kill_first is not a list of "
                    "creature entries; the plan runs without crowd control and kill-first targets", plan.c_str());
            }
        } while (result->NextRow());
    }

    if (auto result = WorldDatabase.Query("SELECT `plan`, `leg`, `name` FROM `ai_dungeon_leg`"))
    {
        do
        {
            Field* fields = result->Fetch();
            table.AddLeg(fields[0].GetCppString(), fields[1].GetUInt8(), fields[2].GetCppString());
        } while (result->NextRow());
    }

    std::uint32_t& skippedSteps = counts.skippedSteps;
    if (auto result = WorldDatabase.Query(
        "SELECT `plan`, `leg`, `seq`, `kind`, `entry`, `x`, `y`, `z`, `radius`, `item`, `param`, `flags`, "
        "`timeout_s`, `entries` FROM `ai_dungeon_step`"))
    {
        do
        {
            Field* fields = result->Fetch();
            std::string const plan = fields[0].GetCppString();
            PlanStep step;
            step.leg = fields[1].GetUInt8();
            step.seq = fields[2].GetUInt16();
            ParsePlanStepRowKind(fields[3].GetCppString(), step.kind, step.exactLaunch);
            step.entry = fields[4].GetUInt32();
            step.x = fields[5].GetFloat();
            step.y = fields[6].GetFloat();
            step.z = fields[7].GetFloat();
            step.radius = fields[8].GetFloat();
            step.item = fields[9].GetUInt32();
            step.param = fields[10].GetUInt32();
            step.flags = fields[11].GetUInt32();
            step.timeoutS = fields[12].GetUInt16();
            PlanStepRefusal refusal = ParsePlanEntries(fields[13].GetCppString(), step.entries)
                ? table.AddStep(plan, step) : PlanStepRefusal::BadEntries;
            if (refusal != PlanStepRefusal::None)
            {
                ++skippedSteps;
                counts.Note("ai_dungeon_step '" + plan + "' leg " + std::to_string(uint32(step.leg)) + " seq " +
                    std::to_string(uint32(step.seq)) + " (" + fields[3].GetCppString() + "): " + PlanStepRefusalName(refusal));
                sLog.outError("ai_dungeon_step: plan '%s' leg %u seq %u (%s) skipped (%s)", plan.c_str(),
                    uint32(step.leg), uint32(step.seq), fields[3].GetCppString().c_str(), PlanStepRefusalName(refusal));
            }
        } while (result->NextRow());
    }

    // Bosses no boss step names (the telemetry's boss rank, DungeonPlanTable::IsBossEntry).
    std::uint32_t& skippedBosses = counts.skippedBosses;
    if (auto result = WorldDatabase.Query("SELECT `map`, `entry` FROM `ai_dungeon_boss`"))
    {
        do
        {
            Field* fields = result->Fetch();
            if (!table.AddBoss(fields[0].GetUInt32(), fields[1].GetUInt32()))
            {
                ++skippedBosses;
                counts.Note("ai_dungeon_boss map " + std::to_string(fields[0].GetUInt32()) + " entry " +
                    std::to_string(fields[1].GetUInt32()) + ": a zero map or entry, or a second row");
            }
        } while (result->NextRow());
    }

    // Sealed even when the tables are empty or missing: `.bot run` then finds no plan.
    table.Seal();
    for (const DungeonPlan* plan : table.All())
    {
        std::size_t const longHops = CountLongHops(*plan, kLongHopYd);
        if (longHops)
            sLog.outError("ai_dungeon_step: plan '%s' has %u moves more than %.0f yd from the move before them",
                plan->name.c_str(), uint32(longHops), kLongHopYd);
    }
}

void ai::LoadDungeonPlans()
{
    DungeonPlanTable& table = GetDungeonPlans();
    if (table.Sealed())
        return;
    PlanLoadCounts counts;
    ReadDungeonPlanRows(table, counts);
    sLog.outString(">> %s", PlanLoadSummary(table, counts).c_str());
    sLog.outString();
}

// `.bot run reloadplans`, world thread: both tables are read into new ones through the startup loaders, and replace the
// old ones only when CommitPlanReload accepts them (any refused row, a plan gone: the old set stays, nothing changes).
PlanReloadReport ai::ReloadDungeonPlans(std::vector<DungeonPlan const*> const& inUse)
{
    PlanReloadReport report;
    if (!GetDungeonPlans().Sealed())
    {
        report.outcome.reason = "the dungeon plans were not loaded at startup (the AI is off): nothing to reload";
        return report;
    }
    auto next = std::make_shared<DungeonPlanTable>();
    PlanLoadCounts counts;
    ReadDungeonPlanRows(*next, counts);
    auto nextTactics = std::make_shared<BossTacticTable>();
    uint32 const skippedTactics = ReadBossTacticRows(*nextTactics);
    report.summary = PlanLoadSummary(*next, counts);
    report.tacticRows = uint32(nextTactics->Size());
    report.outcome = CommitPlanReload(DungeonPlanHolder(), std::move(next), counts, BossTacticsHolder(), std::move(nextTactics),
        skippedTactics, inUse);
    if (report.outcome.swapped)
        sLog.outString(">> %s, %u boss tactic rows (reloaded by .bot run reloadplans)", report.summary.c_str(), report.tacticRows);
    else
        sLog.outError(".bot run reloadplans refused, the old dungeon plans stay: %s", report.outcome.reason.c_str());
    return report;
}
