
#include "playerbot/playerbot.h"
#include "ShamanMultipliers.h"
#include "TankShamanStrategy.h"

using namespace ai;

class TankShamanStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    TankShamanStrategyActionNodeFactory()
    {
        creators["stormstrike"] = &stormstrike;
        creators["strength of earth totem"] = &strength_of_earth_totem;
        creators["stoneclaw totem"] = &stoneclaw_totem;
        creators["magma totem"] = &magma_totem;
        creators["windfury totem"] = &windfury_totem;
        creators["earthshaker slam"] = &earthshaker_slam;
        creators["totemic slam"] = &totemic_slam;
    }

private:
    ACTION_NODE_A(stormstrike, "stormstrike", "melee");

    ACTION_NODE_A(strength_of_earth_totem, "strength of earth totem", "stoneskin totem");

    // Trash earth: the totem pulses flat threat the spec holds with
    // Totemic Alignment. Falls back to melee (no other earth pick here —
    // the threat rows decide between Stoneclaw and Strength of Earth).
    ACTION_NODE_A(stoneclaw_totem, "stoneclaw totem", "melee");

    ACTION_NODE_C(magma_totem, "magma totem", "fire nova");

    ACTION_NODE_A(windfury_totem, "windfury totem", "grace of air totem");

    // Taunt: no positional alternative (5 yd melee spell, off-GCD). Falls
    // back to melee so the bot keeps swinging while the taunt cools down.
    ACTION_NODE_A(earthshaker_slam, "earthshaker slam", "melee");

    // Tauren racial: damage plus slow, melee range. Same melee fallback.
    ACTION_NODE_A(totemic_slam, "totemic slam", "melee");
};

TankShamanStrategy::TankShamanStrategy(PlayerbotAI* ai) : ShamanStrategy(ai)
{
    actionNodeFactories.Add(std::make_unique<TankShamanStrategyActionNodeFactory>());
}


NextAction** TankShamanStrategy::GetDefaultCombatActions()
{
    // Auto-attack never stops: Earthen Bulwark builds from white hits
    // (community shaman tank guide), and the same default keeps the other three tanks swinging.
    return NextAction::array(0, new NextAction("melee", ACTION_IDLE), NULL);
}

void TankShamanStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanStrategy::InitCombatTriggers(triggers);

    // Salvation must never sit on the tank (protection/druid pattern).
    triggers.push_back(new TriggerNode(
        "has blessing of salvation",
        NextAction::array(0, new NextAction("remove blessing of salvation", ACTION_EMERGENCY), NULL)));

    triggers.push_back(new TriggerNode(
        "has greater blessing of salvation",
        NextAction::array(0, new NextAction("remove greater blessing of salvation", ACTION_EMERGENCY), NULL)));

    // Lost aggro (donor/protection shape: taunt at INTERRUPT+1): a mob
    // peeling onto a non-tank member outranks spenders and interrupts — a
    // missed kick costs damage, a missed taunt costs the healer. Sits below
    // EMERGENCY defensives, above every HIGH-band builder. The fallback ES
    // line queues right behind while the taunt cools down.
    triggers.push_back(new TriggerNode(
        "lose aggro",
        NextAction::array(0, new NextAction("earthshaker slam", ACTION_INTERRUPT + 1),
                             new NextAction("earth shock", ACTION_INTERRUPT), NULL)));

    // Stormstrike first: the nature-vulnerability debuff amplifies everything
    // after it (same order as enhancement). Lightning Strike follows, then
    // the Tauren slam while both strikes cool down.
    triggers.push_back(new TriggerNode(
        "stormstrike",
        NextAction::array(0, new NextAction("stormstrike", ACTION_NORMAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "lightning strike",
        NextAction::array(0, new NextAction("lightning strike", ACTION_NORMAL + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "totemic slam",
        NextAction::array(0, new NextAction("totemic slam", ACTION_NORMAL + 1), NULL)));

    // Chain Lightning only into a 2+ pack (the trigger gates the count) so
    // it never burns mana single-target. Sits with the builders: above the
    // shock filler, below the strikes.
    triggers.push_back(new TriggerNode(
        "tank chain lightning",
        NextAction::array(0, new NextAction("chain lightning", ACTION_NORMAL + 1), NULL)));

    // Earth Shock on fights that need it (boss/elite/raid-in-instance):
    // about 40% of single-target threat, on cooldown next to the strikes.
    triggers.push_back(new TriggerNode(
        "tank earth shock fight",
        NextAction::array(0, new NextAction("earth shock", ACTION_NORMAL + 1), NULL)));

    // Economical trash mode: Earth Shock only when the threat lead says
    // someone is catching up or a mob is loose.
    triggers.push_back(new TriggerNode(
        "tank threat lead",
        NextAction::array(0, new NextAction("earth shock", ACTION_NORMAL), NULL)));

    // Flame shock is the DoT half of the shared shock cooldown: it fires
    // before either earth shock line so the DoT lands first (enhancement
    // pattern).
    triggers.push_back(new TriggerNode(
        "flame shock upkeep",
        NextAction::array(0, new NextAction("flame shock", ACTION_NORMAL + 1), NULL)));
}

void TankShamanStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanStrategy::InitNonCombatTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "has blessing of salvation",
        NextAction::array(0, new NextAction("remove blessing of salvation", ACTION_EMERGENCY), NULL)));

    triggers.push_back(new TriggerNode(
        "has greater blessing of salvation",
        NextAction::array(0, new NextAction("remove greater blessing of salvation", ACTION_EMERGENCY), NULL)));
}

void TankShamanStrategy::InitReactionTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanStrategy::InitReactionTriggers(triggers);
}

void TankShamanStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanStrategy::InitDeadTriggers(triggers);
}

void TankShamanPveStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitCombatTriggers(triggers);
    ShamanPveStrategy::InitCombatTriggers(triggers);
}

void TankShamanPveStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitNonCombatTriggers(triggers);
    ShamanPveStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanPveStrategy::InitReactionTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitReactionTriggers(triggers);
    ShamanPveStrategy::InitReactionTriggers(triggers);
}

void TankShamanPveStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitDeadTriggers(triggers);
    ShamanPveStrategy::InitDeadTriggers(triggers);
}

void TankShamanPvpStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitCombatTriggers(triggers);
    ShamanPvpStrategy::InitCombatTriggers(triggers);
}

void TankShamanPvpStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitNonCombatTriggers(triggers);
    ShamanPvpStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanPvpStrategy::InitReactionTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitReactionTriggers(triggers);
    ShamanPvpStrategy::InitReactionTriggers(triggers);
}

void TankShamanPvpStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitDeadTriggers(triggers);
    ShamanPvpStrategy::InitDeadTriggers(triggers);
}

void TankShamanRaidStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitCombatTriggers(triggers);
    ShamanRaidStrategy::InitCombatTriggers(triggers);
}

void TankShamanRaidStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitNonCombatTriggers(triggers);
    ShamanRaidStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanRaidStrategy::InitReactionTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitReactionTriggers(triggers);
    ShamanRaidStrategy::InitReactionTriggers(triggers);
}

void TankShamanRaidStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanStrategy::InitDeadTriggers(triggers);
    ShamanRaidStrategy::InitDeadTriggers(triggers);
}

void TankShamanAoeStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanAoeStrategy::InitCombatTriggers(triggers);

    // Pack order (community shaman tank guide): Stoneclaw first (cheap threat on every
    // mob), then Fire Nova when the party AoEs, Magma only as the last
    // resort after Nova. Medium-aoe (3+) fires above the 2+ light line so
    // the pack tool wins on a real pack.
    triggers.push_back(new TriggerNode(
        "melee medium aoe",
        NextAction::array(0, new NextAction("fire nova", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "melee medium aoe",
        NextAction::array(0, new NextAction("magma totem", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "melee light aoe",
        NextAction::array(0, new NextAction("fire nova", ACTION_HIGH), NULL)));
}

void TankShamanAoeStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanAoeStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanAoePveStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanAoeStrategy::InitCombatTriggers(triggers);
    ShamanAoePveStrategy::InitCombatTriggers(triggers);
}

void TankShamanAoePveStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanAoeStrategy::InitNonCombatTriggers(triggers);
    ShamanAoePveStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanAoePvpStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanAoeStrategy::InitCombatTriggers(triggers);
    ShamanAoePvpStrategy::InitCombatTriggers(triggers);
}

void TankShamanAoePvpStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanAoeStrategy::InitNonCombatTriggers(triggers);
    ShamanAoePvpStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanAoeRaidStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanAoeStrategy::InitCombatTriggers(triggers);
    ShamanAoeRaidStrategy::InitCombatTriggers(triggers);
}

void TankShamanAoeRaidStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanAoeStrategy::InitNonCombatTriggers(triggers);
    ShamanAoeRaidStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanTotemsStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanTotemsStrategy::InitCombatTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "grounding totem reactive",
        NextAction::array(0, new NextAction("grounding totem", ACTION_HIGH + 2), NULL)));

    // Air/water from the existing group census (enhancement pattern):
    // windfury for a melee subgroup, tranquil/grace otherwise; mana spring
    // for the water slot. Reactives unchanged.
    triggers.push_back(new TriggerNode(
        "air totem",
        NextAction::array(0, new NextAction("windfury totem", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "air totem",
        NextAction::array(0, new NextAction("tranquil air totem", ACTION_HIGH), NULL)));

    triggers.push_back(new TriggerNode(
        "air totem",
        NextAction::array(0, new NextAction("grace of air totem", ACTION_HIGH), NULL)));

    triggers.push_back(new TriggerNode(
        "water totem",
        NextAction::array(0, new NextAction("mana spring totem", ACTION_HIGH), NULL)));

    triggers.push_back(new TriggerNode(
        "poison cleansing totem reactive",
        NextAction::array(0, new NextAction("poison cleansing totem", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "disease cleansing totem reactive",
        NextAction::array(0, new NextAction("disease cleansing totem", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "tremor totem reactive",
        NextAction::array(0, new NextAction("tremor totem", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "earthbind totem reactive",
        NextAction::array(0, new NextAction("earthbind totem", ACTION_HIGH), NULL)));

    // Earth (no Stoneskin on bosses): Strength of Earth on
    // fights (boss/elite/raid) via the fight trigger shape, Stoneclaw on
    // trash via the threat-gated tank trigger. Both queue on the shared
    // "earth totem" slot trigger; the phases never overlap.
    triggers.push_back(new TriggerNode(
        "tank earth shock fight",
        NextAction::array(0, new NextAction("strength of earth totem", ACTION_HIGH), NULL)));

    triggers.push_back(new TriggerNode(
        "tank stoneclaw",
        NextAction::array(0, new NextAction("stoneclaw totem", ACTION_HIGH), NULL)));

    // Fire threat ladder: single-target searing stays the filler; the AoE
    // rows (Nova/Magma) live in the AoE strategy above, not here.
    triggers.push_back(new TriggerNode(
        "fire totem",
        NextAction::array(0, new NextAction("searing totem", ACTION_HIGH), NULL)));
}

void TankShamanTotemsStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanTotemsStrategy::InitNonCombatTriggers(triggers);

    // Pre-pull placement (enhancement pattern): same subgroup-picked totems
    // as combat, above follow so the walk-in drops them.
    triggers.push_back(new TriggerNode(
        "air totem",
        NextAction::array(0, new NextAction("windfury totem", ACTION_HIGH), NULL)));

    triggers.push_back(new TriggerNode(
        "air totem",
        NextAction::array(0, new NextAction("tranquil air totem", ACTION_HIGH - 1), NULL)));

    triggers.push_back(new TriggerNode(
        "air totem",
        NextAction::array(0, new NextAction("grace of air totem", ACTION_HIGH - 1), NULL)));

    triggers.push_back(new TriggerNode(
        "water totem",
        NextAction::array(0, new NextAction("mana spring totem", ACTION_HIGH - 1), NULL)));

    triggers.push_back(new TriggerNode(
        "earth totem",
        NextAction::array(0, new NextAction("strength of earth totem", ACTION_HIGH - 1), NULL)));
}

void TankShamanTotemsPveStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanTotemsStrategy::InitCombatTriggers(triggers);
    ShamanTotemsPveStrategy::InitCombatTriggers(triggers);
}

void TankShamanTotemsPveStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanTotemsStrategy::InitNonCombatTriggers(triggers);
    ShamanTotemsPveStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanTotemsPvpStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanTotemsStrategy::InitCombatTriggers(triggers);
    ShamanTotemsPvpStrategy::InitCombatTriggers(triggers);
}

void TankShamanTotemsPvpStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanTotemsStrategy::InitNonCombatTriggers(triggers);
    ShamanTotemsPvpStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanTotemsRaidStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanTotemsStrategy::InitCombatTriggers(triggers);
    ShamanTotemsRaidStrategy::InitCombatTriggers(triggers);
}

void TankShamanTotemsRaidStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanTotemsStrategy::InitNonCombatTriggers(triggers);
    ShamanTotemsRaidStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanBuffStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanBuffStrategy::InitCombatTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "shaman weapon",
        NextAction::array(0, new NextAction("shaman weapon upkeep", ACTION_HIGH), NULL)));

    // Shield upkeep (tank phase swap): Lightning Shield for the pull, Water
    // Shield once mana falls to two thirds. Each row's trigger picks its
    // phase, so the shields never recast over each other mid-fight.
    triggers.push_back(new TriggerNode(
        "tank lightning shield",
        NextAction::array(0, new NextAction("lightning shield", ACTION_NORMAL), NULL)));

    triggers.push_back(new TriggerNode(
        "tank water shield",
        NextAction::array(0, new NextAction("water shield", ACTION_NORMAL), NULL)));
}

void TankShamanBuffStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanBuffStrategy::InitNonCombatTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "shaman weapon",
        NextAction::array(0, new NextAction("shaman weapon upkeep", ACTION_NORMAL), NULL)));

    // Pre-pull: Lightning Shield up (the swap trigger stands down above
    // two thirds mana, so the pull shield is always lightning).
    triggers.push_back(new TriggerNode(
        "tank lightning shield",
        NextAction::array(0, new NextAction("lightning shield", ACTION_NORMAL), NULL)));
}

void TankShamanBuffPveStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBuffStrategy::InitCombatTriggers(triggers);
    ShamanBuffPveStrategy::InitCombatTriggers(triggers);
}

void TankShamanBuffPveStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBuffStrategy::InitNonCombatTriggers(triggers);
    ShamanBuffPveStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanBuffPvpStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBuffStrategy::InitCombatTriggers(triggers);
    ShamanBuffPvpStrategy::InitCombatTriggers(triggers);
}

void TankShamanBuffPvpStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBuffStrategy::InitNonCombatTriggers(triggers);
    ShamanBuffPvpStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanBuffRaidStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBuffStrategy::InitCombatTriggers(triggers);
    ShamanBuffRaidStrategy::InitCombatTriggers(triggers);
}

void TankShamanBuffRaidStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBuffStrategy::InitNonCombatTriggers(triggers);
    ShamanBuffRaidStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanBoostStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    BoostStrategy::InitCombatTriggers(triggers);

    // Bloodlust on the pull for the opening threat lead (community shaman tank guide).
    triggers.push_back(new TriggerNode(
        "bloodlust",
        NextAction::array(0, new NextAction("bloodlust", ACTION_HIGH + 2), NULL)));
}

void TankShamanBoostStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    BoostStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanBoostPveStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBoostStrategy::InitCombatTriggers(triggers);
    ShamanBoostPveStrategy::InitCombatTriggers(triggers);
}

void TankShamanBoostPveStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBoostStrategy::InitNonCombatTriggers(triggers);
    ShamanBoostPveStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanBoostPvpStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBoostStrategy::InitCombatTriggers(triggers);
    ShamanBoostPvpStrategy::InitCombatTriggers(triggers);
}

void TankShamanBoostPvpStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBoostStrategy::InitNonCombatTriggers(triggers);
    ShamanBoostPvpStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanBoostRaidStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBoostStrategy::InitCombatTriggers(triggers);
    ShamanBoostRaidStrategy::InitCombatTriggers(triggers);
}

void TankShamanBoostRaidStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanBoostStrategy::InitNonCombatTriggers(triggers);
    ShamanBoostRaidStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanCcStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanCcStrategy::InitCombatTriggers(triggers);

    // Runner snare (enhancement cc pattern): frost shock on the snare
    // target keeps fleeing mobs in melee so Stoneclaw and the shocks hold.
    triggers.push_back(new TriggerNode(
        "frost shock snare",
        NextAction::array(0, new NextAction("frost shock", ACTION_HIGH), NULL)));
}

void TankShamanCcStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanCcStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanCcPveStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCcStrategy::InitCombatTriggers(triggers);
    ShamanCcPveStrategy::InitCombatTriggers(triggers);
}

void TankShamanCcPveStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCcStrategy::InitNonCombatTriggers(triggers);
    ShamanCcPveStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanCcPvpStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCcStrategy::InitCombatTriggers(triggers);
    ShamanCcPvpStrategy::InitCombatTriggers(triggers);
}

void TankShamanCcPvpStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCcStrategy::InitNonCombatTriggers(triggers);
    ShamanCcPvpStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanCcRaidStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCcStrategy::InitCombatTriggers(triggers);
    ShamanCcRaidStrategy::InitCombatTriggers(triggers);
}

void TankShamanCcRaidStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCcStrategy::InitNonCombatTriggers(triggers);
    ShamanCcRaidStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanCureStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanCureStrategy::InitCombatTriggers(triggers);
}

void TankShamanCureStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    ShamanCureStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanCurePveStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCureStrategy::InitCombatTriggers(triggers);
    ShamanCurePveStrategy::InitCombatTriggers(triggers);
}

void TankShamanCurePveStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCureStrategy::InitNonCombatTriggers(triggers);
    ShamanCurePveStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanCurePvpStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCureStrategy::InitCombatTriggers(triggers);
    ShamanCurePvpStrategy::InitCombatTriggers(triggers);
}

void TankShamanCurePvpStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCureStrategy::InitNonCombatTriggers(triggers);
    ShamanCurePvpStrategy::InitNonCombatTriggers(triggers);
}

void TankShamanCureRaidStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCureStrategy::InitCombatTriggers(triggers);
    ShamanCureRaidStrategy::InitCombatTriggers(triggers);
}

void TankShamanCureRaidStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    TankShamanCureStrategy::InitNonCombatTriggers(triggers);
    ShamanCureRaidStrategy::InitNonCombatTriggers(triggers);
}
