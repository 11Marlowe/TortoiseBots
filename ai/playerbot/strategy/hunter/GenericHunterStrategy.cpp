// Forward-ported from mod-playerbots Hunter/Strategy/GenericHunterStrategy.cpp
// Source: mod-playerbots@5397110
/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GenericHunterStrategy.h"

class GenericHunterStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    GenericHunterStrategyActionNodeFactory()
    {
        creators["rapid fire"] = &rapid_fire;
        creators["mongoose bite"] = &mongoose_bite;
        creators["raptor strike"] = &raptor_strike;
        creators["explosive trap"] = &explosive_trap;
    }

private:
    static ActionNode* rapid_fire([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("rapid fire",
                              /*P*/ {},
                              /*A*/ {},
                              /*C*/ {});
    }
    static ActionNode* mongoose_bite([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("mongoose bite",
                              /*P*/ {},
                              /*A*/ { NextAction("raptor strike") },
                              /*C*/ {});
    }
    static ActionNode* raptor_strike([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("raptor strike",
                              /*P*/ { NextAction("melee") },
                              /*A*/ {},
                              /*C*/ {});
    }
    static ActionNode* explosive_trap([[maybe_unused]] PlayerbotAI* botAI)
    {
        return new ActionNode("explosive trap",
                              /*P*/ {},
                              /*A*/ { NextAction("immolation trap") },
                              /*C*/ {});
    }
};

GenericHunterStrategy::GenericHunterStrategy(PlayerbotAI* botAI) : CombatStrategy(botAI)
{
    actionNodeFactories.Add(new GenericHunterStrategyActionNodeFactory());
}

void GenericHunterStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    CombatStrategy::InitTriggers(triggers);

    // Mark/Ammo/Mana Triggers
    triggers.push_back(new TriggerNode("no ammo", { NextAction("equip upgrades", 30.0f) }));
    triggers.push_back(new TriggerNode("hunter's mark", { NextAction("hunter's mark", 29.5f) }));
    triggers.push_back(new TriggerNode("rapid fire", { NextAction("rapid fire", 29.0f) }));

    // Aggro/Threat/Defensive Triggers
    triggers.push_back(new TriggerNode("has aggro", { NextAction("concussive shot", 20.0f) }));
    triggers.push_back(new TriggerNode("low health", { NextAction("deterrence", 35.0f) }));
    triggers.push_back(new TriggerNode("concussive shot on snare target", { NextAction("concussive shot", 20.0f) }));
    triggers.push_back(new TriggerNode("medium threat", { NextAction("feign death", 35.0f) }));
    triggers.push_back(new TriggerNode("hunters pet medium health", { NextAction("mend pet", 22.0f) }));
    triggers.push_back(new TriggerNode("hunters pet low health", { NextAction("mend pet", 21.0f) }));

    // Dispel Triggers
    triggers.push_back(new TriggerNode("dispel enrage", { NextAction("tranquilizing shot", 61.0f) }));

    // Ranged-based Triggers
    triggers.push_back(new TriggerNode("enemy too close for auto shot", { NextAction("disengage", 35.0f),
                                                                          NextAction("flee", 34.0f) }));
}

// ===== AoE Strategy, 2/3+ enemies =====
// Live AoE handling queues the registered "ranged medium aoe" row
// (HunterStrategy.cpp); the donor "medium aoe"/"light aoe" trigger
// creators were never ported (issue #1179).
AoEHunterStrategy::AoEHunterStrategy(PlayerbotAI* botAI) : CombatStrategy(botAI) {}

void AoEHunterStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
}

void HunterCcStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("scare beast", { NextAction("scare beast on cc", 23.0f) }));
    triggers.push_back(new TriggerNode("freezing trap", { NextAction("freezing trap on cc", 23.0f) }));
}

void HunterTrapWeaveStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // No live trigger creators: "immolation trap no cd" ref removed (issue #1179).
}
