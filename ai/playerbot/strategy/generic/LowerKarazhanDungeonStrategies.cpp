
#include "playerbot/playerbot.h"
#include "LowerKarazhanDungeonStrategies.h"

using namespace ai;

void LowerKarazhanDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "start araxxna fight",
        NextAction::array(0, new NextAction("enable araxxna fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start moroes fight",
        NextAction::array(0, new NextAction("enable moroes fight strategy", 100.0f), NULL)));
}

void LowerKarazhanDungeonStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Kill-order policy (Moroes dinner guests) needs raid-lead input (#237).
}

void AraxxnaFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Brood Queen Araxxna (61221) spawns spiderling waves every ~30s
    // (core SpawnEggs): cleave them down before they bracket the healers.
    triggers.push_back(new TriggerNode(
        "araxxna swarm",
        NextAction::array(0, new NextAction("dps aoe", ACTION_HIGH + 2), NULL)));
}

void AraxxnaFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end araxxna fight",
        NextAction::array(0, new NextAction("disable araxxna fight strategy", 100.0f), NULL)));
}

void AraxxnaFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end araxxna fight",
        NextAction::array(0, new NextAction("disable araxxna fight strategy", 100.0f), NULL)));
}

void MoroesFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Moroes (61225) rogue kit: smoke bomb (57096) blinds the raid's sight
    // lines, glittering dust (57095) + shuffle kick (57097) punish casters.
    // Off-tank taunts when the main tank is gouged; shared interrupt
    // triggers already cover shadow blast (57099) casts.
    triggers.push_back(new TriggerNode(
        "moroes vanished",
        NextAction::array(0, new NextAction("tank assist", ACTION_HIGH + 1), NULL)));
}

void MoroesFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end moroes fight",
        NextAction::array(0, new NextAction("disable moroes fight strategy", 100.0f), NULL)));
}

void MoroesFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end moroes fight",
        NextAction::array(0, new NextAction("disable moroes fight strategy", 100.0f), NULL)));
}
