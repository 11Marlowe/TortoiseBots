#pragma once
#include "playerbot/strategy/actions/DungeonActions.h"
#include "playerbot/strategy/actions/ChangeStrategyAction.h"

namespace ai
{
class KarazhanCryptEnableDungeonStrategyAction : public ChangeAllStrategyAction
{
public:
    KarazhanCryptEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable karazhan crypt strategy", "+karazhan crypt") {}
};

class KarazhanCryptDisableDungeonStrategyAction : public ChangeAllStrategyAction
{
public:
    KarazhanCryptDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable karazhan crypt strategy", "-karazhan crypt") {}
};
}
