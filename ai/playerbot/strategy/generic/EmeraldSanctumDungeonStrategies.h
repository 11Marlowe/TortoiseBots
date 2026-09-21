#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
// Emerald Sanctum (Map 807, Hyjal 40-man): Solnius corrupted green dragon
// (60748) + Erennius event (60747). Static census from core
// emerald_sanctum.h / boss_solnius.cpp; geometry-dependent positioning
// (dream portals, room bounds) stays in issue #237 playtesting.
class EmeraldSanctumDungeonStrategy : public Strategy
{
public:
    EmeraldSanctumDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    std::string getName() override { return "emerald sanctum"; }

private:
    void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
};

class SolniusFightStrategy : public Strategy
{
public:
    SolniusFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    std::string getName() override { return "solnius"; }

private:
    void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
};
}
