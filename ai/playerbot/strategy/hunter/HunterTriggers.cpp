
#include "playerbot/playerbot.h"
#include "HunterTriggers.h"
#include "../../../runtime/PetRevivePolicy.h"
#include "../../../runtime/PetAttackGatePolicy.h"
#include "playerbot/strategy/values/ThreatValues.h"
#include "playerbot/strategy/values/PossibleAttackTargetsValue.h"
#include "HunterActions.h"
#include "../../../runtime/HunterViperPolicy.h"

using namespace ai;

// A player-given "aspect <x>" strategy (HunterManualAspectStrategy) in either
// engine: an explicit order beats the Viper automation. Only called after the
// cheap gates, so it costs nothing while mana is fine or the toggle is off.
static bool HasPlayerAspectOrder(PlayerbotAI* ai)
{
    static char const* const names[] = { "aspect hawk", "aspect monkey", "aspect cheetah", "aspect pack", "aspect beast", "aspect wild" };
    for (char const* name : names)
        if (ai->HasStrategy(name, BotState::BOT_STATE_COMBAT) || ai->HasStrategy(name, BotState::BOT_STATE_NON_COMBAT))
            return true;
    return false;
}

static TortoiseBots::HunterViperInputs BuildViperInputs(PlayerbotAI* ai, Player* bot)
{
    TortoiseBots::HunterViperInputs in;
    in.toggleOn = sPlayerbotAIConfig.hunterViperOnLowMana;
    in.manaPct = ai->GetManaPercent();
    in.inViper = ai->HasAura("aspect of the viper", bot);
    return in;
}

bool AspectOfTheHawkTrigger::IsActive()
{
    // Toggle off: exactly the plain buff trigger.
    if (!sPlayerbotAIConfig.hunterViperOnLowMana)
        return BuffTrigger::IsActive();

    TortoiseBots::HunterViperInputs in = BuildViperInputs(ai, bot);
    if (in.inViper && in.manaPct < TortoiseBots::HUNTER_VIPER_EXIT_MANA_PCT)
    {
        in.playerAspectOrder = HasPlayerAspectOrder(ai);
        if (TortoiseBots::ViperHoldsCombatAspect(in))
            return false;
    }
    return BuffTrigger::IsActive();
}

bool AspectOfTheViperLowManaTrigger::IsActive()
{
    if (!sPlayerbotAIConfig.hunterViperOnLowMana)
        return false;

    TortoiseBots::HunterViperInputs in = BuildViperInputs(ai, bot);
    // Cheap mana read first: most ticks stop here.
    if (in.inViper || in.manaPct >= TortoiseBots::HUNTER_VIPER_ENTER_MANA_PCT)
        return false;
    in.movementAspect = ai->HasAura("aspect of the cheetah", bot) || ai->HasAura("aspect of the pack", bot);
    in.playerAspectOrder = HasPlayerAspectOrder(ai);
    return TortoiseBots::ShouldSwitchToViper(in) && BuffTrigger::IsActive();
}

bool HunterNoStingsActiveTrigger::IsActive()
{
    if (!ai->HasSpell("serpent sting") && !ai->HasSpell("scorpid sting") && !ai->HasSpell("viper sting"))
        return false;

	Unit* target = AI_VALUE(Unit*, "current target");
    return target && AI_VALUE2(uint8, "health", "current target") > 40 &&
        !ai->HasAura("serpent sting", target) &&
        !ai->HasAura("scorpid sting", target) &&
        !ai->HasAura("viper sting", target);
}

bool HuntersPetDeadTrigger::IsActive()
{
    return AI_VALUE(bool, "pet dead") && !AI_VALUE2(bool, "mounted", "self target");
}

// PET-5: dead pet plus nobody hitting the bot — the 10s Revive channel only
// starts when it won't be interrupted on the first tick. Cheap-first: pet
// state and attacker count are scalar reads; mounted last.
bool SafeToRevivePetTrigger::IsActive()
{
    TortoiseBots::PetReviveGateInputs gate;
    gate.petDead = AI_VALUE(bool, "pet dead");
    if (!gate.petDead)
        return false;
    gate.attackerCount = AI_VALUE(uint8, "my attacker count");
    if (gate.attackerCount > 0)
        return false;
    gate.mounted = AI_VALUE2(bool, "mounted", "self target");
    return TortoiseBots::CanRevivePetNow(gate);
}

bool HuntersPetLowHealthTrigger::IsActive()
{
    Unit* pet = AI_VALUE(Unit*, "pet target");
    return pet && AI_VALUE2(uint8, "health", "pet target") < 40 &&
        !AI_VALUE2(bool, "dead", "pet target") && !AI_VALUE2(bool, "mounted", "self target");
}

// PET-3/PET-8b: fires while the pet holds the enemy's attention AND pet
// taunts are stood down (grouped with a real tank). Cheap-first: the
// victim check and group check run before the member walk inside the
// helper, so solo hunters never pay for the scan.
//
// F5: also fires on the pet's first threat lead over the tank, before it has
// become the mob's victim (PetCowerOnLead; not for a mob on the hunter): Cower after the mob has turned
// is a tank swap the party has to pay for. The victim check still catches a
// pet that holds the mob with no tank value to compare against.
bool PetHasAggroTrigger::IsActive()
{
    Unit* pet = AI_VALUE(Unit*, "pet target");
    if (!pet || !pet->IsAlive())
        return false;
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return false;
    if (!bot->GetGroup() || ai::IsPetTauntAllowed(ai, bot))
        return false;
    Unit* victim = target->GetVictim();
    if (victim == pet)
        return true;
    return TortoiseBots::PetCowerOnLead(
        sServerFacade.GetThreatManager(target).getThreat(pet), ThreatValue::GetTankThreat(ai, target), victim == bot);
}

bool HuntersMarkTrigger::IsActive()
{
    if (!DebuffTrigger::IsActive())
        return false;

    // Cheap gates first: the victim read only runs for a grouped hunter that
    // is about to mark, with a living tank anywhere in the group.
    if (!bot->GetGroup() || ai::IsPetTauntAllowed(ai, bot))
        return true;

    Unit* target = AI_VALUE(Unit*, "current target");
    Unit* victim = target ? target->GetVictim() : nullptr;
    Player* victimPlayer = victim ? dynamic_cast<Player*>(victim) : nullptr;
    // Any living tank of the group counts (an off-tank's mob too), not only
    // the main tank.
    bool const victimIsLivingTank = victimPlayer && victimPlayer != bot && victimPlayer->IsAlive() &&
        PlayerbotAI::IsTank(victimPlayer);
    return HuntersMarkAllowed(true, ClassifyHuntersMarkHolder(victim != nullptr, victim == bot, victimIsLivingTank));
}

bool MultishotCanCastTrigger::IsActive()
{
    if (!SpellCanBeCastedTrigger::IsActive())
        return false;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || PossibleAttackTargetsValue::HoldsBreakableCc(ai, target, bot))
        return false;

    // The primary target counts as one; every other live, un-CC'd attacker
    // inside the AoE radius of it counts as another.
    uint32 usable = 1;
    std::list<ObjectGuid> attackers = AI_VALUE(std::list<ObjectGuid>, "attackers");
    for (std::list<ObjectGuid>::iterator i = attackers.begin(); i != attackers.end() && !MultiShotWorthIt(usable); ++i)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || unit == target || !unit->IsAlive())
            continue;
        if (!sServerFacade.IsDistanceLessOrEqualThan(
                sServerFacade.getDistance2d(unit, target), sPlayerbotAIConfig.aoeRadius))
            continue;
        if (PossibleAttackTargetsValue::HoldsBreakableCc(ai, unit, bot))
            continue;
        ++usable;
    }
    return MultiShotWorthIt(usable);
}

bool HunterSwarmedTrigger::IsActive()
{
    // The count first: it is one container size, and a hunter with one mob or
    // none on it (almost always) stops here.
    uint32 const attackers = HunterSwarmAttackerCount(ai);
    if (!HunterSwarmed(attackers))
        return false;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return false;
    bool const inReach = sServerFacade.IsDistanceLessOrEqualThan(
        AI_VALUE2(float, "distance", "current target"), sPlayerbotAIConfig.tooCloseDistance);
    return HunterSwarmClipWanted(attackers, target->GetVictim() == bot, inReach) && ai->HasSpell("wing clip");
}

// PET-4 (donor GenericHunterStrategy.cpp:72-73): the medium band below
// MediumHealth (70) — heals chip damage before it becomes the low band.
// Overlaps low below 40; the low node outranks it so low wins there.
bool HuntersPetMediumHealthTrigger::IsActive()
{
    Unit* pet = AI_VALUE(Unit*, "pet target");
    return pet && AI_VALUE2(uint8, "health", "pet target") < sPlayerbotAIConfig.mediumHealth &&
        !AI_VALUE2(bool, "dead", "pet target") && !AI_VALUE2(bool, "mounted", "self target");
}

bool HunterPetNotHappy::IsActive()
{
    return !AI_VALUE(bool, "pet happy") && !AI_VALUE2(bool, "mounted", "self target");
}

bool ViperStingOnAttackerTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (target)
    {
        const bool noStings = !ai->HasAura("serpent sting", target) &&
                              !ai->HasAura("scorpid sting", target) &&
                              !ai->HasAura("viper sting", target);
        if (noStings)
        {
            if (target->GetPower(POWER_MANA) >= 10)
            {
                return DebuffOnAttackerTrigger::IsActive();
            }
        }
    }

    return false;
}

bool SerpentStingOnAttackerTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (target)
    {
        const bool noStings = !ai->HasAura("serpent sting", target) &&
                              !ai->HasAura("scorpid sting", target) &&
                              !ai->HasAura("viper sting", target);
        if (noStings)
        {
            if (target->GetPower(POWER_MANA) < 10)
            {
                return DebuffOnAttackerTrigger::IsActive();
            }
        }
    }

    return false;
}

// Volley is a channel: stop it once the pack is gone, the same rule as the
// warlock's Rain of Fire check. Turtle has three Volley ranks (1510, 14294,
// 14295); the WotLK ranks 27022/58431/58434 are not in its spell data.
bool VolleyChannelCheckTrigger::IsActive()
{
    if (Spell* spell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
    {
        if (spell->m_spellInfo)
        {
            uint32 id = spell->m_spellInfo->Id;
            if (id == 1510 || id == 14294 || id == 14295)
                return AI_VALUE(uint8, "aoe count") < 2;
        }
    }
    return false;
}
