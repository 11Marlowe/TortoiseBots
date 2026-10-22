
#include "playerbot/playerbot.h"
#include "InterruptTargetValue.h"
#include "TargetValue.h"
#include "CcMarkHoldPolicy.h"
#include "InterruptTargetPolicy.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/strategy/actions/GenericSpellActions.h"

using namespace ai;

Unit* InterruptTargetValue::Calculate()
{
    std::string const interruptSpell = qualifier;
    if (interruptSpell.empty())
        return NULL;

    // Interrupt reach: the interrupt spell's own range, floored at melee
    // distance - combat-reach spells (Kick, Shield Bash, Pummel, Bash)
    // report a bare reach here, and their actions are melee ones at
    // ATTACK_DISTANCE.
    float range = ATTACK_DISTANCE;
    float spellRange;
    if (ai->GetSpellRange(interruptSpell, &spellRange) && spellRange > range)
        range = spellRange;

    CcGuard ccGuard(ai);
    Unit* best = NULL;
    int bestTier = 0;

    std::list<ObjectGuid> const& attackers =
        ai->GetAiObjectContext()->GetValue<std::list<ObjectGuid>>("possible attack targets")->Get();
    for (std::list<ObjectGuid>::const_iterator i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || !unit->IsAlive())
            continue;

        if (sServerFacade.getDistance2d(bot, unit) > range)
            continue;

        if (!ai->IsInterruptableSpellCasting(unit, interruptSpell, true))
            continue;

        // A held CC mark (Moon or "rti cc", in its hold window or under a
        // CC cast bar) is the CC bot's: Kick and Shield Bash deal damage,
        // and the walk to it drags the bot onto the mark.
        if (!CcEnemyHealerPickAllowed(ccGuard.IsHeldCcMark(unit)))
            continue;

        // A cast that sleeps, fears, sheep-shackles or heals someone
        // outranks a plain damage cast.
        Spell* cast = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!cast || !cast->m_spellInfo)
            cast = unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        bool const priority = cast && cast->m_spellInfo &&
            (PlayerbotAI::IsHealSpell(cast->m_spellInfo) ||
             FindNonCcTargetStrategy::IsCcSpell(cast->m_spellInfo));

        bool const claimed = bot->GetGroup() &&
            BuffClaimRegistry::IsClaimedByOther(bot->GetObjectGuid(), unit->GetObjectGuid(), InterruptClaimKey());

        int const tier = InterruptTargetTier(priority, claimed);
        if (!best || tier < bestTier)
        {
            best = unit;
            bestTier = tier;
        }
        if (bestTier == 0)
            break; // a free priority cast cannot be beaten
    }

    // One interrupter per cast: claim the pick so the next interrupter that
    // recalculates (same tick or the next few seconds) prefers another
    // cast. A claim only lowers the rank (InterruptTargetPolicy.h), so it
    // never wedges coverage when the claiming bot does not cast.
    if (best && bot->GetGroup())
        BuffClaimRegistry::Claim(bot->GetObjectGuid(), best->GetObjectGuid(), InterruptClaimKey());

    return best;
}
