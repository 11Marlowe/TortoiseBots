
#include "playerbot/playerbot.h"
#include "AttackerOfNonTankTargetValue.h"
#include "AttackerOfNonTankPolicy.h"
#include "TargetValue.h"
#include "CcMarkHoldPolicy.h"
#include "playerbot/ServerFacade.h"

using namespace ai;

Unit* AttackerOfNonTankTargetValue::Calculate()
{
    std::list<ObjectGuid> const& attackers =
        ai->GetAiObjectContext()->GetValue<std::list<ObjectGuid>>("possible attack targets")->Get();
    CcGuard ccGuard(ai);
    // Only the tank's Taunt may touch a held CC mark, and only under the
    // owner exception (same rule as SnareTargetValue); the IsTank walk and
    // the spell range never change inside the loop, so hoist them out.
    bool const tankTaunt = ai->IsTank(bot);
    float const spellRange = ai->GetRange("spell");
    Player* const master = ai->GetMaster();

    std::vector<AttackerOfNonTankCandidate> candidates;
    std::vector<Unit*> units;
    for (ObjectGuid guid : attackers)
    {
        Unit* unit = ai->GetUnit(guid);
        if (!unit || unit->IsPlayer())
            continue;

        Unit* victim = unit->GetVictim();
        Player* victimPlayer = victim ? victim->ToPlayer() : nullptr;
        if (!victimPlayer)
            continue;

        AttackerOfNonTankCandidate candidate;
        candidate.victimIsGroupPlayer = victimPlayer == bot || victimPlayer == master ||
            (bot->GetGroup() && victimPlayer->GetGroup() == bot->GetGroup());
        if (!candidate.victimIsGroupPlayer)
            continue;

        candidate.victimIsTank = ai->IsTank(victimPlayer);
        candidate.victimIsHealer = PlayerbotAI::IsHeal(victimPlayer);
        candidate.distance = sServerFacade.getDistance2d(bot, unit);
        candidate.inRange = candidate.distance <= spellRange;

        candidate.heldCcMark = ccGuard.IsHeldCcMark(unit);
        if (candidate.heldCcMark)
            candidate.heldMarkTauntAllowed = CcHeldMarkTauntAllowed(tankTaunt,
                FindNonCcTargetStrategy::IsCrowdControlled(ai, unit),
                ccGuard.GroupCastsCcAt(unit), candidate.victimIsHealer);

        candidates.push_back(candidate);
        units.push_back(unit);
    }

    std::size_t const pick = AttackerOfNonTankPick(candidates);
    return pick < units.size() ? units[pick] : NULL;
}
