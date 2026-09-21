#pragma once
#include "playerbot/strategy/triggers/DungeonTriggers.h"

namespace ai
{
class LowerKarazhanEnterDungeonTrigger : public EnterDungeonTrigger
{
public:
    LowerKarazhanEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter lower karazhan", "lower karazhan", 532) {}
};

class LowerKarazhanLeaveDungeonTrigger : public LeaveDungeonTrigger
{
public:
    LowerKarazhanLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave lower karazhan", "lower karazhan", 532) {}
};

class KarazhanCryptEnterDungeonTrigger : public EnterDungeonTrigger
{
public:
    KarazhanCryptEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter karazhan crypt", "karazhan crypt", 800) {}
};

class KarazhanCryptLeaveDungeonTrigger : public LeaveDungeonTrigger
{
public:
    KarazhanCryptLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave karazhan crypt", "karazhan crypt", 800) {}
};

class AraxxnaStartFightTrigger : public StartBossFightTrigger
{
public:
    AraxxnaStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start araxxna fight", "araxxna", 61221) {}
};

class AraxxnaEndFightTrigger : public EndBossFightTrigger
{
public:
    AraxxnaEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end araxxna fight", "araxxna", 61221) {}
};

class MoroesStartFightTrigger : public StartBossFightTrigger
{
public:
    MoroesStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start moroes fight", "moroes", 61225) {}
};

class MoroesEndFightTrigger : public EndBossFightTrigger
{
public:
    MoroesEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end moroes fight", "moroes", 61225) {}
};

// Araxxna spiderling swarm (core SpawnEggs, entry 30008): any live
// spiderling in aggro range means cleave them down.
class AraxxnaSwarmTrigger : public Trigger
{
public:
    AraxxnaSwarmTrigger(PlayerbotAI* ai) : Trigger(ai, "araxxna swarm", 2) {}
    bool IsActive() override;
};

// Moroes smoke bomb (57096) blinds sight lines: re-target via tank assist
// while the cloud is up on the bot.
class MoroesVanishedTrigger : public Trigger
{
public:
    MoroesVanishedTrigger(PlayerbotAI* ai) : Trigger(ai, "moroes vanished", 1) {}
    std::string GetTargetName() override { return "self target"; }
    bool IsActive() override { return ai->HasAura(57096, bot); }
};
}
