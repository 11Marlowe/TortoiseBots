
#include "playerbot/playerbot.h"
#include "WorldPvpStrategy.h"

using namespace ai;

static float const kWorldPvpSeekRelevance = 55.0f;

void WorldPvpStrategy::InitCombatTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "enemy player near",
        NextAction::array(0, new NextAction("attack enemy player", kWorldPvpSeekRelevance), NULL)));
}

void WorldPvpStrategy::InitNonCombatTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "enemy player near",
        NextAction::array(0, new NextAction("attack enemy player", kWorldPvpSeekRelevance), NULL)));
}
