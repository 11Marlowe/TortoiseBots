#include "playerbot/playerbot.h"
#include "AttackerOfNonTankValue.h"
#include "TargetValue.h"
#include "CcMarkHoldPolicy.h"
#include "playerbot/ServerFacade.h"

using namespace ai;

void ai::CollectNonTankAttackers(PlayerbotAI* ai, std::string const& spell,
    std::unordered_map<uint64, uint32> const* lastTaunt, std::vector<NonTankAttacker>& out)
{
    out.clear();
    Player* bot = ai->GetBot();
    Group* group = bot ? bot->GetGroup() : nullptr;
    if (!group)
        return;

    // The spell's own range: the row never walks the tank (a tank on the run
    // leader's "stay" or guard spot stays there). Melee-range spells
    // (SPELL_RANGE_IDX_COMBAT) use the melee reach, as CastSpellAction does.
    uint32 const spellId = ai->GetAiObjectContext()->GetValue<uint32>("spell id", spell)->Get();
    SpellEntry const* spellInfo = spellId ? sServerFacade.LookupSpellInfo(spellId) : nullptr;
    SpellRangeEntry const* rangeEntry = spellInfo ? sSpellRangeStore.LookupEntry(spellInfo->rangeIndex) : nullptr;
    bool const meleeRange = spellInfo && spellInfo->rangeIndex == SPELL_RANGE_IDX_COMBAT;
    float maxRange = rangeEntry ? rangeEntry->maxRange : 0.0f;
    if (spellInfo && !meleeRange)
        bot->ApplySpellMod(spellId, SPELLMOD_RANGE, maxRange);

    uint32 const now = WorldTimer::getMSTime();
    CcGuard ccGuard(ai);
    bool const tankTaunt = ai->IsTank(bot);

    for (ObjectGuid const& guid : ai->GetAiObjectContext()->GetValue<std::list<ObjectGuid>>("attackers")->Get())
    {
        Unit* unit = ai->GetUnit(guid);
        if (!unit || !unit->IsAlive() || !unit->IsInWorld() || unit->GetMapId() != bot->GetMapId())
            continue;
        Unit* victim = unit->GetVictim();
        Player* victimPlayer = victim ? victim->ToPlayer() : nullptr;
        // Players only: a pet on a mob is the guard's (and Growl's) business.
        if (!victimPlayer || victimPlayer->GetGroup() != group)
            continue;

        NonTankAttacker a;
        a.unit = unit;
        TauntAttackerCandidate& c = a.facts;
        c.victimInGroup = true;
        c.victimIsSelf = victimPlayer == bot;
        if (c.victimIsSelf)
        {
            out.push_back(a);
            continue;
        }
        c.victimIsTank = PlayerbotAI::IsTank(victimPlayer);
        c.victimIsHealer = PlayerbotAI::IsHeal(victimPlayer);
        c.victimHealthPct = victimPlayer->GetHealthPercent();
        c.attackerIsPlayer = unit->IsPlayer();
        c.crowdControlled = FindNonCcTargetStrategy::IsCrowdControlled(ai, unit);
        c.evading = unit->IsCreature() && static_cast<Creature*>(unit)->IsInEvadeMode();
        c.heldCcMark = ccGuard.IsHeldCcMark(unit);
        if (c.heldCcMark)
            c.heldMarkTauntAllowed = CcHeldMarkTauntAllowed(tankTaunt, c.crowdControlled,
                ccGuard.GroupCastsCcAt(unit), c.victimIsHealer);
        c.distance = sServerFacade.getDistance2d(bot, unit);
        c.tankInMelee = bot->CanReachWithMeleeAutoAttack(unit);
        c.inRange = meleeRange ? c.tankInMelee : (rangeEntry && bot->GetCombatDistance(unit) <= maxRange);
        c.inSight = bot->IsWithinLOSInMap(unit);
        if (!c.attackerIsPlayer)
        {
            c.tankThreat = unit->GetThreatManager().getThreat(bot);
            c.victimThreat = unit->GetThreatManager().getThreat(victimPlayer);
        }
        if (lastTaunt)
        {
            auto const it = lastTaunt->find(guid.GetRawValue());
            c.tauntedRecently = it != lastTaunt->end() && TauntedRecently(true, it->second, now);
        }
        out.push_back(a);
    }
}

Unit* AttackerOfNonTankValue::Calculate()
{
    std::vector<NonTankAttacker> attackers;
    CollectNonTankAttackers(ai, qualifier, &lastTaunt, attackers);

    std::vector<TauntAttackerCandidate> facts;
    facts.reserve(attackers.size());
    for (NonTankAttacker const& a : attackers)
        facts.push_back(a.facts);

    int const pick = TauntAttackerPick(facts);
    return pick < 0 ? nullptr : attackers[static_cast<size_t>(pick)].unit;
}

void AttackerOfNonTankValue::NoteTaunt(Unit* unit)
{
    if (!unit)
        return;
    uint32 const now = WorldTimer::getMSTime();
    // Forget mobs past the window so the map stays the size of one fight.
    for (auto it = lastTaunt.begin(); it != lastTaunt.end();)
    {
        if (!TauntedRecently(true, it->second, now))
            it = lastTaunt.erase(it);
        else
            ++it;
    }
    lastTaunt[unit->GetObjectGuid().GetRawValue()] = now;
    // The next read recalculates: the Taunted mob is no candidate now.
    Reset();
}
