#pragma once
#include "DungeonTriggers.h"

namespace ai
{
    class OnyxiasLairEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        OnyxiasLairEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter onyxia's lair", "onyxia's lair", 249) {}
    };

    class OnyxiasLairLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        OnyxiasLairLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave onyxia's lair", "onyxia's lair", 249) {}
    };

    class OnyxiaStartFightTrigger : public StartBossFightTrigger
    {
    public:
        OnyxiaStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start onyxia fight", "onyxia", 10184) {}
    };

    class OnyxiaEndFightTrigger : public EndBossFightTrigger
    {
    public:
        OnyxiaEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end onyxia fight", "onyxia", 10184) {}
    };

    // Phase 2: Onyxia takes flight (Hover aura 17131, core boss_onyxia.cpp).
    // Melee cannot reach her; the fight strategy swaps them to wands/shoot
    // and spreads the raid for Fireball splash. Grounds on phase 3.
    class OnyxiaAirborneTrigger : public Trigger
    {
    public:
        OnyxiaAirborneTrigger(PlayerbotAI* ai) : Trigger(ai, "onyxia airborne", 1) {}
        std::string GetTargetName() override { return "current target"; }
        bool IsActive() override;
    };
}
