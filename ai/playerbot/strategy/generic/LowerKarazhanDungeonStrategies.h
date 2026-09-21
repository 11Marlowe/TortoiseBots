#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
// Lower Karazhan Halls (Map 532, Deadwind 10-man): Brood Queen Araxxna
// (61221, spiderling waves), Lord Blackwald II (61222), Moroes (61225,
// rogue kit), Dark Rider Champion (61204). Static census from core
// lower_karazhan_halls.h + boss scripts; kill-order policy stays in #237.
class LowerKarazhanDungeonStrategy : public Strategy
{
public:
    LowerKarazhanDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    std::string getName() override { return "lower karazhan"; }

private:
    void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
};

class AraxxnaFightStrategy : public Strategy
{
public:
    AraxxnaFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    std::string getName() override { return "araxxna"; }

private:
    void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
};

class MoroesFightStrategy : public Strategy
{
public:
    MoroesFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    std::string getName() override { return "moroes"; }

private:
    void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
};
}
