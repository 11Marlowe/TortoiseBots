#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
// Karazhan Crypt (Map 800, 5-10 man): Guard Captain Gort, Hivaxxis,
// Alarus behind lever/door gating (core instance_karazhan_crypt.cpp).
// Narrow tunnels clip follow paths; the only code-safe lever is a
// tighter non-combat follow radius. Puzzle auto-solving vs prompting
// stays in issue #237.
class KarazhanCryptDungeonStrategy : public Strategy
{
public:
    KarazhanCryptDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    std::string getName() override { return "karazhan crypt"; }

private:
    void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
};
}
