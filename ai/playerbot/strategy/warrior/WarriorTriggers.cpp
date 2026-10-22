
#include "playerbot/playerbot.h"
#include "WarriorTriggers.h"
#include "WarriorActions.h"
#include "playerbot/strategy/values/AttackerOfNonTankValue.h"

using namespace ai;

bool BattleStanceTrigger::IsActive()
{
    return !ai->IsTank(bot) && BuffTrigger::IsActive();
}

bool DefensiveStanceTrigger::IsActive()
{
    return ai->IsTank(bot) && BuffTrigger::IsActive();
}

bool BerserkerStanceTrigger::IsActive()
{
    return !ai->IsTank(bot) && BuffTrigger::IsActive();
}

bool ShieldBlockTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    return target && target->IsAlive() && target->GetVictim() == bot &&
        sServerFacade.IsHostileTo(bot, target) && bot->CanReachWithMeleeAutoAttack(target) &&
        BuffTrigger::IsActive();
}


bool BloodrageBuffTrigger::IsActive()
{
    if (!ai->HasSpell("bloodrage"))
        return false;

    // Audit F2: Bloodrage was tried on every rage-starved tick while on its
    // 60 s cooldown (94 % impossible); the policy also needs it off cooldown.
    uint32 spellId = AI_VALUE2(uint32, "spell id", "bloodrage");
    return ShouldBloodrage(AI_VALUE2(uint8, "health", "self target"), sPlayerbotAIConfig.mediumHealth,
        AI_VALUE2(uint8, "rage", "self target"),
        spellId && sServerFacade.IsSpellReady(bot, spellId));
}

// Audit F2: Disarm was the newest verdict in 8 % of tank combat and impossible
// 99.6 % of the time (cost 20 rage, median rage 6). Try it only with the rage,
// off cooldown, on a target that carries a main-hand weapon.
bool DisarmDebuffTrigger::IsActive()
{
    uint32 spellId = AI_VALUE2(uint32, "spell id", "disarm");
    if (!spellId)
        return false;

    Unit* target = GetTarget();
    if (!target)
        return false;

    // Creature weapon in virtual item slot 0 (main hand), as Spell.cpp reads
    // it; a player target is treated as armed (its weapon is not cheap to read).
    const bool armed = target->IsPlayer() || target->GetUInt32Value(UNIT_VIRTUAL_ITEM_DISPLAY + 0) != 0;
    if (!ShouldDisarm(AI_VALUE2(uint8, "rage", "self target"), armed, sServerFacade.IsSpellReady(bot, spellId)))
        return false;

    return DebuffTrigger::IsActive();
}

bool SunderArmorDebuffTrigger::IsActive()
{
    if (!ai->HasSpell("sunder armor"))
        return false;

    Unit* target = GetTarget();
    if (!target)
        return false;

    // Stop at a full 5-stack, but re-arm to refresh an expiring stack
    // (donor refreshes at <=6s remaining): without this the 5-stack falls
    // off completely before the trigger fires again.
    if (Aura* aura = ai->GetAura("sunder armor", target))
    {
        if (aura->GetStackAmount() >= 5)
            return aura->GetAuraDuration() <= 6000;
        return true;
    }

    if (ai->IsTank(bot) && !target->IsPlayer())
        return true;

    return !ai->HasAura("sunder armor", target, true) && !HasMaxDebuffs();
}

Value<Unit*>* TauntAttackerOfNonTankTrigger::GetTargetValue()
{
    return context->GetValue<Unit*>("attacker of non-tank", spell);
}

bool TauntAttackerOfNonTankTrigger::IsActive()
{
    // Cheap gate first: the value walks the group's attackers.
    if (!ai->IsTank(bot) || !bot->GetGroup())
        return false;
    Unit* target = GetTarget();
    if (!target)
        return false;
    // Range is the value's job (it only returns mobs in Taunt range); here
    // stance, cooldown and rage, as SpellCanBeCastedTrigger asks the core.
    return TauntAttackerGate(ai->IsTank(bot), bot->GetGroup() != nullptr,
        ai->CanCastSpell(spell, target, true, nullptr, true));
}

bool ChallengingShoutOnAttackersTrigger::IsActive()
{
    if (!ai->IsTank(bot) || !bot->GetGroup())
        return false;
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !bot->CanReachWithMeleeAutoAttack(target))
        return false;
    if (!ai->CanCastSpell("challenging shout", target, true, nullptr, true))
        return false;

    std::vector<NonTankAttacker> attackers;
    CollectNonTankAttackers(ai, "taunt", nullptr, attackers);
    std::vector<TauntAttackerCandidate> facts;
    facts.reserve(attackers.size());
    for (NonTankAttacker const& a : attackers)
        facts.push_back(a.facts);
    return ChallengingShoutOnAttackers(facts, ChallengingShoutRadiusYd);
}
