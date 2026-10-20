
#include "playerbot/playerbot.h"
#include "ShamanTriggers.h"
#include "ShamanActions.h"
#include "playerbot/strategy/values/ThreatValues.h"

using namespace ai;

std::list<std::string> ShamanWeaponTrigger::spells;

bool ShamanWeaponTrigger::IsActive()
{
    // Upkeep must survive every level: before this the strategies queued one
    // fixed imbue (windfury for enhancement, flametongue elsewhere) while this
    // trigger stayed true on any known imbue, so the cast failed every tick
    // below its gate (issue #406: windfury is gated at 30). A queued imbue the
    // bot never trained also has no spell id, so the trigger now requires a
    // trained spell it can actually cast - the newest known rank, since
    // HasSpell resolves the highest rank - and skips unknown imbues.
    if (spells.empty())
    {
        spells.push_back("windfury weapon");
        spells.push_back("flametongue weapon");
        spells.push_back("frostbrand weapon");
        spells.push_back("rockbiter weapon");
    }

    for (std::list<std::string>::iterator i = spells.begin(); i != spells.end(); ++i)
    {
        std::string const& spell = *i;

        if (!ai->HasSpell(spell))
            continue;

        uint32 spellId = AI_VALUE2(uint32, "spell id", spell);
        if (!spellId)
            continue;

        if (AI_VALUE2(Item*, "item for spell", spellId))
            return true;
    }

    return false;
}

bool ShockTrigger::IsActive()
{
    if (!ai->HasSpell("earth shock") && !ai->HasSpell("flame shock") && !ai->HasSpell("frost shock"))
        return false;

    // Flame shock is exempt here: it owns the shared shock cooldown via the
    // higher-priority flame shock upkeep line, so earth shock still spends
    // the cooldown while the flame DoT ticks instead of being locked out.
    return SpellTrigger::IsActive() && !ai->HasAnyAuraOf(GetTarget(), "frost shock", "earth shock", NULL) && !HasMaxDebuffs();
}

bool TankFightTargetTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    bool bossOrElite = false;
    if (Creature* creature = dynamic_cast<Creature*>(target))
    {
        if (CreatureInfo const* info = creature->GetCreatureInfo())
            bossOrElite = TankShamanTargetIsElite(info->rank, CREATURE_ELITE_NORMAL);
    }
    Group* group = bot->GetGroup();
    bool inRaid = group && group->isRaidGroup();
    Map* map = bot->GetMap();
    bool inInstance = map && map->IsDungeon();
    return TankShamanEarthShockFreeFire(bossOrElite, inInstance, inRaid);
}

bool TankEarthShockFightTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !sServerFacade.IsAlive(target))
        return false;

    if (!ai->CanCastSpell("earth shock", target, true, nullptr, true))
        return false;

    // Reuse the shared shock gate (any shock known, no shock debuff on the
    // target, debuff slots free) so the tank row never double-dips the
    // shock cooldown against the interrupt rows in ShamanStrategy.
    ShockTrigger shock(ai);
    if (!shock.IsActive())
        return false;

    TankFightTargetTrigger fight(ai);
    return fight.IsActive();
}

bool TankStrengthOfEarthTrigger::IsActive()
{
    if (!ai->HasSpell("strength of earth totem"))
        return false;

    if (!TotemAutoRowActive(ai, bot, bot->IsInCombat()))
        return false;

    // Manual .bot totem orders win
    if (ai->HasStrategy("totem earth strength", BotState::BOT_STATE_COMBAT))
        return !AI_VALUE2(bool, "has totem", "strength of earth totem");
    if (ai->HasStrategy("totem earth stoneclaw", BotState::BOT_STATE_COMBAT) ||
        ai->HasStrategy("totem earth stoneskin", BotState::BOT_STATE_COMBAT) ||
        ai->HasStrategy("totem earth earthbind", BotState::BOT_STATE_COMBAT) ||
        ai->HasStrategy("totem earth tremor", BotState::BOT_STATE_COMBAT))
        return false;

    std::string stored = AI_VALUE2(std::string, "manual saved string", TotemOrderKey(0));
    std::string ordered = OrderedTotemForSlot(stored);
    if (!ordered.empty())
    {
        if (ordered != "strength of earth totem")
            return false;
    }
    else if (!stored.empty() && stored != "auto")
        return false;

    TankFightTargetTrigger fight(ai);
    if (!TankEarthTotemIsStrength(fight.IsActive()))
        return false;

    bool hasEarth = AI_VALUE2(bool, "has totem", "stoneclaw totem") ||
        AI_VALUE2(bool, "has totem", "stoneskin totem") ||
        AI_VALUE2(bool, "has totem", "earthbind totem") ||
        AI_VALUE2(bool, "has totem", "strength of earth totem") ||
        AI_VALUE2(bool, "has totem", "tremor totem");
    if (hasEarth)
        return false;

    return ai->CanCastSpell("strength of earth totem", bot, true, nullptr, true);
}

bool TankThreatLeadTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || !sServerFacade.IsAlive(target))
        return false;

    if (!ai->CanCastSpell("earth shock", target, true, nullptr, true))
        return false;

    float ownThreat = ThreatValue::GetThreat(bot, target);

    // Another member catching up on our target: their threat at 70%+ of
    // ours (the economical-trash threshold in ShamanTankPolicy.h).
    bool catchingUp = false;
    Group* group = bot->GetGroup();
    if (group)
    {
        for (Player* member : LiveGroupMembers(group))
        {
            if (!member || member == bot || !sServerFacade.IsAlive(member) || !ai->IsSafe(member))
                continue;
            // Manager-direct read (ThreatValues.cpp idiom): works for bot and
            // player members alike, no member AI needed.
            float memberThreat = ThreatValue::GetThreat(member, target);
            if (TankShamanThreatNeedsShock(ownThreat, memberThreat, 70, false))
            {
                catchingUp = true;
                break;
            }
        }
    }

    // A loose pack mob: any attacker whose victim is a live non-tank group
    // member (not us — losing our own target is the taunt row's job).
    bool mobLoose = false;
    std::list<ObjectGuid> attackers = AI_VALUE(std::list<ObjectGuid>, "attackers");
    for (ObjectGuid guid : attackers)
    {
        Unit* attacker = ai->GetUnit(guid);
        if (!attacker || attacker == target || !sServerFacade.IsAlive(attacker))
            continue;
        Unit* victim = attacker->GetVictim();
        if (!victim || victim == bot || !victim->IsPlayer())
            continue;
        Player* victimPlayer = (Player*)victim;
        if (!sServerFacade.IsAlive(victimPlayer))
            continue;
        if (ai->IsTank(victimPlayer))
            continue;
        if (group && !group->IsMember(victimPlayer->getObjectGuid()))
            continue;
        mobLoose = true;
        break;
    }

    return catchingUp || mobLoose;
}

bool TankStoneclawTrigger::IsActive()
{
    if (!ai->HasSpell("stoneclaw totem"))
        return false;
    Group* group = bot->GetGroup();
    bool inGroup = group != nullptr;
    bool manual = ai->HasStrategy("totem earth stoneclaw", BotState::BOT_STATE_COMBAT);
    bool other = ai->HasStrategy("totem earth stoneskin", BotState::BOT_STATE_COMBAT) ||
        ai->HasStrategy("totem earth earthbind", BotState::BOT_STATE_COMBAT) ||
        ai->HasStrategy("totem earth strength", BotState::BOT_STATE_COMBAT) ||
        ai->HasStrategy("totem earth tremor", BotState::BOT_STATE_COMBAT);
    bool hasEarth = AI_VALUE2(bool, "has totem", "stoneclaw totem") ||
        AI_VALUE2(bool, "has totem", "stoneskin totem") ||
        AI_VALUE2(bool, "has totem", "earthbind totem") ||
        AI_VALUE2(bool, "has totem", "strength of earth totem") ||
        AI_VALUE2(bool, "has totem", "tremor totem");
    TankFightTargetTrigger fight(ai);
    bool isFight = fight.IsActive();
    TankThreatLeadTrigger lead(ai);
    bool threatNeeded = lead.IsActive();
    uint8 attackers = AI_VALUE(uint8, "attackers count");
    if (!TankStoneclawShouldDrop(inGroup, manual, other, hasEarth, threatNeeded, attackers, isFight))
        return false;
    return ai->CanCastSpell("stoneclaw totem", bot, true, nullptr, true);
}

bool TankLightningShieldTrigger::IsActive()
{
    // Lightning Shield owns the pull and the top of the mana bar; once
    // Water Shield is trained and mana falls to two thirds, the water row
    // takes over and this one stands down.
    if (TankShamanWantsWaterShield(ai->HasSpell("water shield"), AI_VALUE2(uint8, "mana", "self target")))
        return false;
    return BuffTrigger::IsActive();
}

bool TankWaterShieldTrigger::IsActive()
{
    if (!TankShamanWantsWaterShield(true, AI_VALUE2(uint8, "mana", "self target")))
        return false;
    return BuffTrigger::IsActive();
}

bool TotemicSlamTrigger::IsActive()
{
    if (bot->GetRace() != RACE_TAUREN)
        return false;
    uint32 spellId = AI_VALUE2(uint32, "spell id", "totemic slam");
    if (!spellId || !sServerFacade.IsSpellReady(bot, spellId))
        return false;
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || AI_VALUE2(float, "distance", "current target") > ATTACK_DISTANCE)
        return false;
    // Damage-and-slow filler: fire only while both strikes cool down, so it
    // never steals the GCD from Stormstrike or Lightning Strike.
    if (!ai->HasSpell("stormstrike") || !ai->HasSpell("lightning strike"))
        return true;
    uint32 stormstrike = AI_VALUE2(uint32, "spell id", "stormstrike");
    uint32 strike = AI_VALUE2(uint32, "spell id", "lightning strike");
    bool strikesReady = (!stormstrike || sServerFacade.IsSpellReady(bot, stormstrike)) ||
        (!strike || sServerFacade.IsSpellReady(bot, strike));
    return !strikesReady;
}
