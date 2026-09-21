#pragma once
#include "playerbot/strategy/actions/DungeonActions.h"
#include "playerbot/strategy/actions/ChangeStrategyAction.h"

namespace ai
{
class LowerKarazhanEnableDungeonStrategyAction : public ChangeAllStrategyAction
{
public:
    LowerKarazhanEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable lower karazhan strategy", "+lower karazhan") {}
};

class LowerKarazhanDisableDungeonStrategyAction : public ChangeAllStrategyAction
{
public:
    LowerKarazhanDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable lower karazhan strategy", "-lower karazhan") {}
};

class AraxxnaEnableFightStrategyAction : public ChangeAllStrategyAction
{
public:
    AraxxnaEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable araxxna fight strategy", "+araxxna") {}
};

class AraxxnaDisableFightStrategyAction : public ChangeAllStrategyAction
{
public:
    AraxxnaDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable araxxna fight strategy", "-araxxna") {}
};

class MoroesEnableFightStrategyAction : public ChangeAllStrategyAction
{
public:
    MoroesEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable moroes fight strategy", "+moroes") {}
};

class MoroesDisableFightStrategyAction : public ChangeAllStrategyAction
{
public:
    MoroesDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable moroes fight strategy", "-moroes") {}
};
}
