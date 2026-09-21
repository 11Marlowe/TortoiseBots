#pragma once
#include "playerbot/strategy/triggers/DungeonTriggers.h"

namespace ai
{
class EmeraldSanctumEnterDungeonTrigger : public EnterDungeonTrigger
{
public:
    EmeraldSanctumEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter emerald sanctum", "emerald sanctum", 807) {}
};

class EmeraldSanctumLeaveDungeonTrigger : public LeaveDungeonTrigger
{
public:
    EmeraldSanctumLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave emerald sanctum", "emerald sanctum", 807) {}
};

class SolniusStartFightTrigger : public StartBossFightTrigger
{
public:
    SolniusStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start solnius fight", "solnius", 60748) {}
};

class SolniusEndFightTrigger : public EndBossFightTrigger
{
public:
    SolniusEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end solnius fight", "solnius", 60748) {}
};

// Emerald Rot (56508) carrier check: reuses the spell-ID so the fight
// strategy can runout even before the universal bomb trigger ticks.
class SolniusEmeraldRotTrigger : public Trigger
{
public:
    SolniusEmeraldRotTrigger(PlayerbotAI* ai) : Trigger(ai, "solnius emerald rot", 1) {}
    std::string GetTargetName() override { return "self target"; }
    bool IsActive() override { return ai->HasAura(56508, bot); }
};

// Call of Nightmare (46079) add wave: Suppressor 61212 / Scalebane 60746 /
// Dragonkin 60743 / Wyrmkin 60745 in aggro range means switch to adds.
class SolniusAddsTrigger : public Trigger
{
public:
    SolniusAddsTrigger(PlayerbotAI* ai) : Trigger(ai, "solnius adds", 2) {}
    bool IsActive() override;
};
}
