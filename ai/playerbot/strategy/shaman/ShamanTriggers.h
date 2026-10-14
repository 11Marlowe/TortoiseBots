#pragma once
#include "playerbot/GroupMembers.h"
#include "playerbot/strategy/triggers/GenericTriggers.h"
#include "ShamanStoneclawPolicy.h"
#include "ShamanEarthShockPolicy.h"
#include "ShamanRecallPolicy.h"
#include "ShamanTotemGroupPolicy.h"

namespace ai
{
    class ShamanWeaponTrigger : public BuffTrigger
    {
    public:
        ShamanWeaponTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "rockbiter weapon") {}
        virtual bool IsActive() override;
    private:
        static std::list<std::string> spells;
    };

    class ReadyToRemoveTotemsTrigger : public Trigger
    {
    public:
        ReadyToRemoveTotemsTrigger(PlayerbotAI* ai) : Trigger(ai, "ready to remove totems", 5) {}

        virtual bool IsActive() override
        {
            // Out-of-combat mana refund (mod-playerbots parity SHM-5):
            // recall only when the bot and no group member is fighting.
            // Never recall mana tide: destroying the cooldown totem wastes
            // it for pennies. (No fire-elemental totem action exists in
            // 1.18.1, so there is nothing else to spare.)
            // Owner-scoped values: recall refunds only OUR totems, so a
            // teammate's tide must not veto us and strangers' totems must
            // not trigger us. Gate lives in ShamanRecallPolicy.h so the unit
            // test pins this exact logic.
            bool hasSpell = ai->HasSpell("totemic recall");
            bool anyOwn = AI_VALUE(bool, "have any own totem");
            bool tideOwn = AI_VALUE2(bool, "has own totem", "mana tide totem");
            bool botCombat = bot->IsInCombat();
            bool memberCombat = false;
            Group* group = bot->GetGroup();
            if (!botCombat && group)
            {
                for (Player* member : LiveGroupMembers(group))
                {
                    if (!member || member == bot)
                        continue;
                    if (member->IsInCombat())
                    {
                        memberCombat = true;
                        break;
                    }
                    Pet* pet = member->GetPet();
                    if (pet && pet->IsInCombat())
                    {
                        memberCombat = true;
                        break;
                    }
                }
            }
            return TotemicRecallShouldFire(hasSpell, anyOwn, tideOwn, botCombat, memberCombat);
        }
    };

    class TotemTrigger : public Trigger
    {
    public:
        TotemTrigger(PlayerbotAI* ai, std::string spell, int attackerCount = 0) : Trigger(ai, spell), attackerCount(attackerCount) {}

        virtual bool IsActive() override
		{
            return AI_VALUE(uint8, "attackers count") >= attackerCount && !AI_VALUE2(bool, "has totem", name);
        }

    protected:
        int attackerCount;
    };

    class FireTotemTrigger : public Trigger
    {
    public:
        FireTotemTrigger(PlayerbotAI* ai, bool inMovement = true) : Trigger(ai, "trigger spec appropriate fire totem", 5), inMovement(inMovement) {}

        virtual bool IsActive() override
        {
            if (!inMovement && bot->IsMoving() && bot->GetMotionMaster() && bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            {
                return false;
            }

            if (ai->HasStrategy("totem fire nova", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "fire nova totem");
            }
            else if (ai->HasStrategy("totem fire flametongue", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "flametongue totem");
            }
            else if (ai->HasStrategy("totem fire resistance", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "fire resistance totem");
            }
            else if (ai->HasStrategy("totem fire magma", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "magma totem");
            }
            else if (ai->HasStrategy("totem fire searing", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "searing totem");
            }
            else
            {
                return !AI_VALUE2(bool, "has totem", "searing totem") &&
                       !AI_VALUE2(bool, "has totem", "magma totem") &&
                       !AI_VALUE2(bool, "has totem", "frost resistance totem") &&
                       !AI_VALUE2(bool, "has totem", "flametongue totem");
            }
        }

    private:
        bool inMovement;
    };

    class FireTotemAoeTrigger : public Trigger
    {
    public:
        FireTotemAoeTrigger(PlayerbotAI* ai) : Trigger(ai, "trigger spec appropriate fire totem aoe", 2) {}
        virtual bool IsActive() override
        {
            return AI_VALUE(uint8, "attackers count") >= 3 &&
                !AI_VALUE2(bool, "has totem", "searing totem") &&
                !AI_VALUE2(bool, "has totem", "magma totem") &&
                !AI_VALUE2(bool, "has totem", "frost resistance totem") &&
                !AI_VALUE2(bool, "has totem", "flametongue totem");
        }
    };

    // One walk of our own subgroup for the totem selection rule: melee vs
    // mana census plus the low-mana healer/caster checks for Mana Tide.
    // SameSubGroup gate (the ImbueAction pattern): totems only reach our
    // own party, so a far-away raid subgroup must not pick our totems.
    // Map + range gate: a same-subgroup mate on another map is not buffed.
    inline TotemSubgroupCensus ReadTotemSubgroup(PlayerbotAI* ai, Player* bot,
        bool* healerLowMana = nullptr, bool* casterLowMana = nullptr)
    {
        TotemGroupMember members[6];
        uint8_t count = 0;
        bool healerLow = false;
        bool casterLow = false;
        Group* group = bot->GetGroup();
        if (group)
        {
            for (Player* member : LiveGroupMembers(group))
            {
                if (!member || member == bot || !sServerFacade.IsAlive(member))
                    continue;
                if (!member->IsInWorld() || !group->SameSubGroup(bot, member))
                    continue;
                if (member->GetMapId() != bot->GetMapId() || bot->GetDistance(member) > 100.0f)
                    continue;
                uint8 cls = member->GetClass();
                // Melee that swings for Windfury/SoE: warriors, rogues,
                // hunters (their pet swings with them), and feral druids
                // tanking in form (IsTank covers bear). No AiFactory here
                // (header-inline): enhancement shamans count as melee via
                // the generic IsMelee strategy check.
                bool melee = cls == CLASS_WARRIOR || cls == CLASS_ROGUE ||
                    cls == CLASS_HUNTER ||
                    (cls == CLASS_DRUID && ai->IsTank(member)) ||
                    (cls == CLASS_SHAMAN && ai->IsMelee(member));
                bool mana = member->GetMaxPower(POWER_MANA) > 0;
                if (count < 6)
                    members[count++] = TotemGroupMember{false, melee, mana};
                if (mana && ai->GetManaPercent(*member) < sPlayerbotAIConfig.lowMana)
                {
                    if (PlayerbotAI::IsHeal(member))
                        healerLow = true;
                    else
                        casterLow = true;
                }
            }
        }
        if (healerLowMana)
            *healerLowMana = healerLow;
        if (casterLowMana)
            *casterLowMana = casterLow;
        return CountTotemSubgroup(members, count);
    }

    class EarthTotemTrigger : public Trigger
    {
    public:
        EarthTotemTrigger(PlayerbotAI* ai, bool inMovement = true) : Trigger(ai, "trigger spec appropriate earth totem", 5), inMovement(inMovement) {}

        virtual bool IsActive() override
        {
            if (!inMovement && bot->IsMoving() && bot->GetMotionMaster() && bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            {
                return false;
            }

            if (ai->HasStrategy("totem earth stoneclaw", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "stoneclaw totem");
            }
            else if (ai->HasStrategy("totem earth stoneskin", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "stoneskin totem");
            }
            else if (ai->HasStrategy("totem earth earthbind", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "earthbind totem");
            }
            else if (ai->HasStrategy("totem earth strength", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "strength of earth totem");
            }
            else if (ai->HasStrategy("totem earth tremor", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "tremor totem");
            }
            else
            {
                // No manual earth order: Strength of Earth only for a
                // subgroup that swings it (warriors/rogues/feral/melee).
                // Caster/healer groups keep the slot empty instead of
                // buffing nobody (live raid: SoE on mage/warlock groups).
                std::string want = BestEarthTotemForSubgroup(ReadTotemSubgroup(ai, bot),
                    ai->HasSpell("strength of earth totem"));
                if (want.empty())
                    return false;
                return !AI_VALUE2(bool, "has totem", "earthbind totem") &&
                       !AI_VALUE2(bool, "has totem", "stoneskin totem") &&
                       !AI_VALUE2(bool, "has totem", "stoneclaw totem") &&
                       !AI_VALUE2(bool, "has totem", "strength of earth totem") &&
                       !AI_VALUE2(bool, "has totem", "tremor totem");
            }
        }

    private:
        bool inMovement;
    };


    class AirTotemTrigger : public Trigger
    {
    public:
        AirTotemTrigger(PlayerbotAI* ai, bool inMovement = true) : Trigger(ai, "trigger spec appropriate air totem", 5), inMovement(inMovement) {}

        virtual bool IsActive() override
        {
            if (!inMovement && bot->IsMoving() && bot->GetMotionMaster() && bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            {
                return false;
            }

            if (ai->HasStrategy("totem air grace", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "grace of air totem");
            }
            else if (ai->HasStrategy("totem air grounding", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "grounding totem");
            }
            else if (ai->HasStrategy("totem air resistance", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "nature resistance totem");
            }
            else if (ai->HasStrategy("totem air windfury", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "windfury totem");
            }
            else if (ai->HasStrategy("totem air windwall", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "windwall totem");
            }
            else
            {
                // No manual air order: pick by subgroup (totems only reach
                // our own party). Melee -> windfury; casters/healers ->
                // tranquil air (Turtle 25908) or grace of air. The trigger
                // stays shared "air totem" across the three strategy rows;
                // each action checks its own aura, so the wrong row goes
                // USELESS and the engine falls through to the right one.
                // Slot-quiet on EVERY air totem, not just the wanted one:
                // one air totem replaces another, so a live Tranquil/Grace
                // must stop the Windfury row too (live raid 2026-10-14: the
                // own-spell check re-fired Windfury 19x in 30 s while the
                // other air totems came and went). Reactive grounding keeps
                // its carve-out below so the swap still lands mid fight.
                std::string want = BestAirTotemForSubgroup(ReadTotemSubgroup(ai, bot),
                    ai->HasSpell("windfury totem"), ai->HasSpell("grace of air totem"),
                    ai->HasSpell("tranquil air totem"));
                if (want.empty())
                    return false;
                if (AI_VALUE2(bool, "has totem", "windfury totem") ||
                    AI_VALUE2(bool, "has totem", "tranquil air totem") ||
                    AI_VALUE2(bool, "has totem", "grace of air totem") ||
                    AI_VALUE2(bool, "has totem", "windwall totem") ||
                    AI_VALUE2(bool, "has totem", "sentry totem") ||
                    AI_VALUE2(bool, "has totem", "nature resistance totem"))
                    return false;
                return !AI_VALUE2(bool, "has totem", "grounding totem");
            }
        }

    private:
        bool inMovement;
    };

    class WaterTotemTrigger : public Trigger
    {
    public:
        WaterTotemTrigger(PlayerbotAI* ai, bool inMovement = true) : Trigger(ai, "trigger spec appropriate water totem", 5), inMovement(inMovement) {}

        virtual bool IsActive() override
        {
            if (!inMovement && bot->IsMoving() && bot->GetMotionMaster() && bot->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            {
                return false;
            }

            if (ai->HasStrategy("totem water cleansing", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "disease cleansing totem");
            }
            else if (ai->HasStrategy("totem water resistance", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "fire resistance totem");
            }
            else if (ai->HasStrategy("totem water healing", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "healing stream totem");
            }
            else if (ai->HasStrategy("totem water mana", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "mana spring totem");
            }
            else if (ai->HasStrategy("totem water poison", BotState::BOT_STATE_COMBAT))
            {
                return !AI_VALUE2(bool, "has totem", "poison cleansing totem");
            }
            else
            {
                // No manual water order: mana users in the subgroup -> mana
                // spring, else healing stream. Shared "water totem" trigger
                // across both rows: the wanted totem's own aura quiets the
                // trigger, the wrong row's action goes USELESS on its aura
                // scan and the engine falls through to the right one.
                // Slot-quiet on EVERY water totem, not just the wanted one
                // (same 2026-10-14 re-cast loop as air: one water totem
                // replaces another). Mana Tide keeps its carve-out: the
                // cooldown totem must still land while Spring/Stream is up.
                std::string want = BestWaterTotemForSubgroup(ReadTotemSubgroup(ai, bot),
                    ai->HasSpell("mana spring totem"), ai->HasSpell("healing stream totem"));
                if (want.empty())
                    return false;
                if (AI_VALUE2(bool, "has totem", "mana spring totem") ||
                    AI_VALUE2(bool, "has totem", "healing stream totem") ||
                    AI_VALUE2(bool, "has totem", "cleansing totem") ||
                    AI_VALUE2(bool, "has totem", "poison cleansing totem") ||
                    AI_VALUE2(bool, "has totem", "disease cleansing totem") ||
                    AI_VALUE2(bool, "has totem", "fire resistance totem"))
                    return false;
                return !AI_VALUE2(bool, "has totem", "mana tide totem");
            }
        }

    private:
        bool inMovement;
    };

    class StoneclawPanicTrigger : public Trigger
    {
    public:
        StoneclawPanicTrigger(PlayerbotAI* ai) : Trigger(ai, "stoneclaw panic", 5) {}

        virtual bool IsActive() override
        {
            // Solo panic button (mod-playerbots parity SHM-3): at low health
            // a solo bot drops Stoneclaw so the totem taunts the attackers
            // off it. Grouped bots keep the spec earth totem unless the
            // player explicitly ordered Stoneclaw for this fight, and any
            // other explicit earth order always wins over the panic drop.
            // Gate lives in ShamanStoneclawPolicy.h so the unit test pins
            // this exact logic.
            bool low = AI_VALUE2(uint8, "health", "self target") <= sPlayerbotAIConfig.lowHealth;
            bool grp = bot->GetGroup() != nullptr;
            bool man = ai->HasStrategy("totem earth stoneclaw", BotState::BOT_STATE_COMBAT);
            bool other = ai->HasStrategy("totem earth stoneskin", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth earthbind", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth strength", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth tremor", BotState::BOT_STATE_COMBAT);
            return StoneclawPanicShouldDrop(low, grp, man, other) &&
                !AI_VALUE2(bool, "has totem", "stoneclaw totem");
        }
    };

    class LightningShieldTrigger : public BuffTrigger
    {
    public:
        LightningShieldTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "lightning shield") {}
    };

    // Elemental-only pre-water fallback: Water Shield (R1 trains at 26)
    // and Lightning Shield are mutually exclusive, but each trigger only
    // checks its own aura — without this gate the two ele buff rows would
    // recast over each other every tick once Water Shield is trained.
    // Enhancement keeps using the shared LightningShieldTrigger.
    class ElementalLightningShieldFallbackTrigger : public BuffTrigger
    {
    public:
        ElementalLightningShieldFallbackTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "lightning shield") {}
        bool IsActive() override
        {
            if (ai->HasSpell("water shield"))
                return false;
            return BuffTrigger::IsActive();
        }
    };

    class LightningStrikeTrigger : public SpellCanBeCastedTrigger
    {
    public:
        LightningStrikeTrigger(PlayerbotAI* ai) : SpellCanBeCastedTrigger(ai, "lightning strike") {}
        bool IsActive() override
        {
            // Tortoise 51387 releases the active shield (lightning/water/earth
            // per imbue-shield pairing). Require a shield aura so the strike
            // never fires bare; core owns charge consumption.
            if (!SpellCanBeCastedTrigger::IsActive())
                return false;
            return ai->HasAura("lightning shield", bot) || ai->HasAura("water shield", bot) ||
                ai->HasAura("earth shield", bot);
        }
    };

    class SpiritLinkOnPartyTankTrigger : public BuffOnTankTrigger
    {
    public:
        SpiritLinkOnPartyTankTrigger(PlayerbotAI* ai) : BuffOnTankTrigger(ai, "spirit link") {}
        bool IsActive() override
        {
            // 10min redistribution link: keep one copy group-wide (mirror the
            // earth-shield dupe guard) and only link a live tank in combat.
            Group* group = bot->GetGroup();
            if (group)
            {
                for (Player* member : LiveGroupMembers(group))
                    if (ai->HasAura("spirit link", member, false, true))
                        return false;
            }
            return BuffOnTankTrigger::IsActive() && ai->IsStateActive(BotState::BOT_STATE_COMBAT);
        }
    };

    BUFF_TRIGGER(AncestralSwiftnessTrigger, "ancestral swiftness");
    HAS_AURA_TRIGGER(AncestralSwiftnessAuraTrigger, "ancestral swiftness");

    class WaterShieldTrigger : public BuffTrigger
    {
    public:
        WaterShieldTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "water shield") {}
    };

    class PurgeTrigger : public TargetAuraDispelTrigger
    {
    public:
        PurgeTrigger(PlayerbotAI* ai) : TargetAuraDispelTrigger(ai, "purge", DISPEL_MAGIC, 3) {}
        virtual bool IsActive() override
        {
            Unit* target = AI_VALUE(Unit*, "current target");
            if (!target)
                return false;

            if (sServerFacade.IsFriendlyTo(bot, target))
                return false;

            for (uint32 type = SPELL_AURA_NONE; type < TOTAL_AURAS; ++type)
            {
                Unit::AuraList const& auras = target->GetAurasByType((AuraType)type);
                for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                {
                    const Aura* aura = *itr;
                    const SpellEntry* entry = aura->GetSpellProto();
                    uint32 spellId = entry->Id;
                    if (!IsPositiveSpell(spellId))
                        continue;

                    std::vector<uint32> ignoreSpells;
                    ignoreSpells.push_back(17627);
                    ignoreSpells.push_back(24382);
                    ignoreSpells.push_back(22888);
                    ignoreSpells.push_back(24425);
                    ignoreSpells.push_back(16609);
                    ignoreSpells.push_back(22818);
                    ignoreSpells.push_back(22820);
                    ignoreSpells.push_back(15366);
                    ignoreSpells.push_back(15852);
                    ignoreSpells.push_back(22817);
                    ignoreSpells.push_back(17538);
                    ignoreSpells.push_back(11405);
                    ignoreSpells.push_back(7396);
                    ignoreSpells.push_back(17539);

                    if (find(ignoreSpells.begin(), ignoreSpells.end(), spellId) != ignoreSpells.end())
                        continue;

                    /*if (sPlayerbotAIConfig.dispelAuraDuration && aura->GetAuraDuration() && aura->GetAuraDuration() < (int32)sPlayerbotAIConfig.dispelAuraDuration)
                        return false;*/

                    if (ai->canDispel(entry, DISPEL_MAGIC))
                        return true;
                }
            }
            return false;
        }
    };

    class WaterWalkingTrigger : public BuffTrigger
    {
    public:
        WaterWalkingTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "water walking", 7) {}

        virtual bool IsActive() override
        {
            return BuffTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
        }
    };

    class WaterBreathingTrigger : public BuffTrigger
    {
    public:
        WaterBreathingTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "water breathing", 5) {}

        virtual bool IsActive() override
        {
            return BuffTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
        }
    };

    class WaterWalkingOnPartyTrigger : public BuffOnPartyTrigger
    {
    public:
        WaterWalkingOnPartyTrigger(PlayerbotAI* ai) : BuffOnPartyTrigger(ai, "water walking on party", 7) {}

        virtual bool IsActive() override
        {
            return BuffOnPartyTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
        }
    };

    class WaterBreathingOnPartyTrigger : public BuffOnPartyTrigger
    {
    public:
        WaterBreathingOnPartyTrigger(PlayerbotAI* ai) : BuffOnPartyTrigger(ai, "water breathing on party", 2) {}

        virtual bool IsActive() override
        {
            return BuffOnPartyTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
        }
    };

    class ShockTrigger : public DebuffTrigger
    {
    public:
        ShockTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "earth shock") {}
        virtual bool IsActive() override;
    };
    // Elemental execute discipline (mod-playerbots parity SHM-4): earth
    // shock only lands the killing blow (target below 25% AND below 1500
    // hp), saving the shared shock cooldown and boss debuff slots on
    // healthy targets. Enhancement keeps the ungated `shock` line; the
    // interrupt triggers are untouched.
    class EarthShockExecuteTrigger : public Trigger
    {
    public:
        EarthShockExecuteTrigger(PlayerbotAI* ai) : Trigger(ai, "earth shock execute") {}

        virtual bool IsActive() override
        {
            // Gate lives in ShamanEarthShockPolicy.h so the unit test pins
            // this exact logic.
            Unit* target = AI_VALUE(Unit*, "current target");
            if (!target)
                return false;
            return EarthShockExecuteShouldFire((float)target->GetHealth(), (float)target->GetMaxHealth(),
                sServerFacade.IsAlive(target));
        }
    };


    // Flame shock is the DoT half of the shared shock cooldown: it fires
    // whenever flame shock itself is down, above the generic shock line, so
    // the DoT lands first and earth shock spends the cooldown only when flame
    // is already up or still unknown. The interrupt trigger is untouched.
    DEBUFF_TRIGGER(FlameShockTrigger, "flame shock");

    INTERRUPT_TRIGGER(EarthShockInterruptSpellTrigger, "earth shock");

    INTERRUPT_HEALER_TRIGGER(EarthShockInterruptEnemyHealerSpellTrigger, "earth shock");

    class FrostShockSnareTrigger : public SnareTargetTrigger
    {
    public:
        FrostShockSnareTrigger(PlayerbotAI* ai) : SnareTargetTrigger(ai, "frost shock") {}
    };

    class CurePoisonTrigger : public NeedCureTrigger
    {
    public:
        CurePoisonTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "cure poison", DISPEL_POISON) {}
    };

    class PartyMemberCurePoisonTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        PartyMemberCurePoisonTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "cure poison", DISPEL_POISON) {}
    };

    class CureDiseaseTrigger : public NeedCureTrigger
    {
    public:
        CureDiseaseTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "cure disease", DISPEL_DISEASE) {}
    };

    class PartyMemberCureDiseaseTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        PartyMemberCureDiseaseTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "cure disease", DISPEL_DISEASE) {}
    };
    // Reactive situational totems (mod-playerbots parity SHM-2). Each fires
    // only when its element slot is currently empty (no flap: totems
    // persist, so once dropped the trigger goes quiet) and never against an
    // explicit player order (`totem <slot> <which>` manual strategy wins).
    class TremorTotemReactiveTrigger : public Trigger
    {
    public:
        TremorTotemReactiveTrigger(PlayerbotAI* ai) : Trigger(ai, "tremor totem reactive", 5) {}

        virtual bool IsActive() override
        {
            if (ai->HasStrategy("totem earth stoneclaw", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth stoneskin", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth earthbind", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth strength", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth tremor", BotState::BOT_STATE_COMBAT))
                return false;
            if (!ai->HasSpell("tremor totem"))
                return false;
            if (AI_VALUE2(bool, "has totem", "stoneclaw totem") ||
                AI_VALUE2(bool, "has totem", "stoneskin totem") ||
                AI_VALUE2(bool, "has totem", "earthbind totem") ||
                AI_VALUE2(bool, "has totem", "strength of earth totem") ||
                AI_VALUE2(bool, "has totem", "tremor totem"))
                return false;
            // Party fear scan first (cheap self check, then group members).
            // Fear + charm only: tremor dispels charm/fear/sleep mechanics
            // (effect 8146), never confuse/polymorph.
            if (bot->HasAuraType(SPELL_AURA_MOD_FEAR) || bot->HasAuraType(SPELL_AURA_MOD_CHARM))
                return true;
            Group* group = bot->GetGroup();
            if (group)
            {
                for (Player* member : LiveGroupMembers(group))
                {
                    if (!member || member == bot)
                        continue;
                    if (member->HasAuraType(SPELL_AURA_MOD_FEAR) || member->HasAuraType(SPELL_AURA_MOD_CHARM))
                        return true;
                }
            }
            return false;
        }
    };

    class GroundingTotemReactiveTrigger : public Trigger
    {
    public:
        GroundingTotemReactiveTrigger(PlayerbotAI* ai) : Trigger(ai, "grounding totem reactive", 5) {}

        virtual bool IsActive() override
        {
            if (ai->HasStrategy("totem air grace", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem air grounding", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem air resistance", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem air windfury", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem air windwall", BotState::BOT_STATE_COMBAT))
                return false;
            if (!ai->HasSpell("grounding totem"))
                return false;
            if (AI_VALUE2(bool, "has totem", "grace of air totem") ||
                AI_VALUE2(bool, "has totem", "windwall totem") ||
                AI_VALUE2(bool, "has totem", "windfury totem") ||
                AI_VALUE2(bool, "has totem", "grounding totem") ||
                AI_VALUE2(bool, "has totem", "sentry totem") ||
                AI_VALUE2(bool, "has totem", "nature resistance totem"))
                return false;
            // Only when the in-flight cast is aimed at us or our party:
            // grounding cannot redirect AoE, self-buffs or heals aimed
            // elsewhere, and must not suppress windfury for whole fights on
            // casts it cannot answer (DeflectSpellTrigger pattern).
            Unit* target = AI_VALUE(Unit*, "current target");
            if (!target || !target->IsNonMeleeSpellCasted(true))
                return false;
            ObjectGuid victim = target->GetTargetGuid();
            if (victim == bot->getObjectGuid())
                return true;
            Group* group = bot->GetGroup();
            if (!group)
                return false;
            for (Player* member : LiveGroupMembers(group))
                if (member && member->getObjectGuid() == victim)
                    return true;
            return false;
        }
    };

    // One trigger per debuff type: 1.12 has no "Cleansing Totem" spell, only
    // the poison / disease pair, each with its own registered action.
    class PoisonCleansingTotemReactiveTrigger : public Trigger
    {
    public:
        PoisonCleansingTotemReactiveTrigger(PlayerbotAI* ai) : Trigger(ai, "poison cleansing totem reactive", 5) {}

        virtual bool IsActive() override
        {
            if (ai->HasStrategy("totem water cleansing", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem water resistance", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem water healing", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem water mana", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem water poison", BotState::BOT_STATE_COMBAT))
                return false;
            if (!ai->HasSpell("poison cleansing totem"))
                return false;
            if (AI_VALUE2(bool, "has totem", "healing stream totem") ||
                AI_VALUE2(bool, "has totem", "mana spring totem") ||
                AI_VALUE2(bool, "has totem", "poison cleansing totem") ||
                AI_VALUE2(bool, "has totem", "disease cleansing totem") ||
                AI_VALUE2(bool, "has totem", "mana tide totem") ||
                AI_VALUE2(bool, "has totem", "fire resistance totem"))
                return false;
            if (ai->HasAuraToDispel(bot, DISPEL_POISON))
                return true;
            Group* group = bot->GetGroup();
            if (!group)
                return false;
            for (Player* member : LiveGroupMembers(group))
            {
                if (!member || member == bot || !sServerFacade.IsAlive(member))
                    continue;
                if (ai->HasAuraToDispel(member, DISPEL_POISON))
                    return true;
            }
            return false;
        }
    };

    class DiseaseCleansingTotemReactiveTrigger : public Trigger
    {
    public:
        DiseaseCleansingTotemReactiveTrigger(PlayerbotAI* ai) : Trigger(ai, "disease cleansing totem reactive", 5) {}

        virtual bool IsActive() override
        {
            if (ai->HasStrategy("totem water cleansing", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem water resistance", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem water healing", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem water mana", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem water poison", BotState::BOT_STATE_COMBAT))
                return false;
            if (!ai->HasSpell("disease cleansing totem"))
                return false;
            if (AI_VALUE2(bool, "has totem", "healing stream totem") ||
                AI_VALUE2(bool, "has totem", "mana spring totem") ||
                AI_VALUE2(bool, "has totem", "poison cleansing totem") ||
                AI_VALUE2(bool, "has totem", "disease cleansing totem") ||
                AI_VALUE2(bool, "has totem", "mana tide totem") ||
                AI_VALUE2(bool, "has totem", "fire resistance totem"))
                return false;
            if (ai->HasAuraToDispel(bot, DISPEL_DISEASE))
                return true;
            Group* group = bot->GetGroup();
            if (!group)
                return false;
            for (Player* member : LiveGroupMembers(group))
            {
                if (!member || member == bot || !sServerFacade.IsAlive(member))
                    continue;
                if (ai->HasAuraToDispel(member, DISPEL_DISEASE))
                    return true;
            }
            return false;
        }
    };

    class EarthbindTotemReactiveTrigger : public Trigger
    {
    public:
        EarthbindTotemReactiveTrigger(PlayerbotAI* ai) : Trigger(ai, "earthbind totem reactive", 5) {}

        virtual bool IsActive() override
        {
            if (ai->HasStrategy("totem earth stoneclaw", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth stoneskin", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth earthbind", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth strength", BotState::BOT_STATE_COMBAT) ||
                ai->HasStrategy("totem earth tremor", BotState::BOT_STATE_COMBAT))
                return false;
            if (!ai->HasSpell("earthbind totem"))
                return false;
            if (AI_VALUE2(bool, "has totem", "stoneclaw totem") ||
                AI_VALUE2(bool, "has totem", "stoneskin totem") ||
                AI_VALUE2(bool, "has totem", "earthbind totem") ||
                AI_VALUE2(bool, "has totem", "strength of earth totem") ||
                AI_VALUE2(bool, "has totem", "tremor totem"))
                return false;
            // Runner: fleeing/confused movement (low-health mobs running,
            // fear breaks) or a snared party member needing the slow.
            Unit* target = AI_VALUE(Unit*, "current target");
            if (target && sServerFacade.IsAlive(target) &&
                target->HasUnitState(UNIT_STAT_FLEEING | UNIT_STAT_CONFUSED))
                return true;
            return AI_VALUE(Unit*, "party member to remove roots") != nullptr;
        }
    };


    class BloodlustTrigger : public BoostTrigger
    {
    public:
        BloodlustTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "bloodlust") {}
    };

    class PartyTankEarthShieldTrigger : public BuffOnTankTrigger
    {
    public:
        PartyTankEarthShieldTrigger(PlayerbotAI* ai) : BuffOnTankTrigger(ai, "earth shield") {}

        bool IsActive() override
        {
            Group* group = bot->GetGroup();
            if (group)
            {
                for (Player* member : LiveGroupMembers(group))
                    if (ai->HasAura("earth shield", member, false, true))
                        return false;
            }
            return BuffOnTankTrigger::IsActive();
        }
    };

    CAN_CAST_TRIGGER(ChainLightningTrigger, "chain lightning");

    CAN_CAST_TRIGGER(StormstrikeTrigger, "stormstrike");
    BUFF_TRIGGER(ElementalMasteryTrigger, "elemental mastery");
}
