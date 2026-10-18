#pragma once

// Pure policy for the stationary mount/dismount loop (issue #1097).
//
// Live runtime proof (Oct 2026, headless GM observer beside looping bots):
// every ~8 s the mount cast starts, lands (spell_go + aura_applied), and
// 1-4 s later the mount aura is removed with the bot still standing at the
// same spot - then the next tick casts again. Two distinct removers:
//
//  - Buff upkeep (Duskguard, paladin, Un'Goro): Summon Warhorse lands, aura
//    13819 removed ~1 s later on the same tick Blessing of Wisdom 19853 is
//    re-cast. Core Spell::CheckCast dismounts a mounted caster before any
//    non-CASTABLE_WHILE_MOUNTED cast (Spell.cpp), and module buff casts
//    dismount first (CastCustomSpellAction::Unmount). So: mount lands ->
//    upkeep tick sees a refreshable blessing -> core strips the mount to
//    cast it -> next tick re-mounts.
//  - Combat dismount on an unengaged nearby hostile (Kima, warrior 27,
//    Silverpine; Codswallop, warlock, Ashenvale): Riding Turtle 30174 lands,
//    aura removed ~3 s later with the bot never in combat and never moving,
//    a hostile wolf standing 20-28 yd away. Decide()'s "Unmount. Able to
//    attack target" fires on a dps/current target the combat engine holds
//    but never engages; the travel gate re-mounts because the trip is still
//    far. Mount rows alternate between equal-speed mounts (Warhorse /
//    Shadowhorn Stag, Turtle / Wind Rider) because Mount() shuffles the list
//    each fresh cast - cosmetic, not causal.
//
// The motion gate (ca0af148, #553) stays: a standing bot must walk, not
// re-cast. The fix is on the dismount side, in two rules:
//
//  1. Combat dismount only for a real fight. "Able to attack target" must
//     not fire for a target the bot is not actually fighting or pulling:
//     require the bot in combat, the target attacking the bot/group (victim
//     or threat), or an explicit attack order on it. A travelling bot that
//     merely holds a nearby hostile as its dps pick keeps its mount and
//     rides past, like the donor (mod-playerbots dismounts only within
//     dismount distance of the current target AND while in combat with
//     attackers, or for an enemy/dps pick it is engaging).
//  2. Out-of-combat buff upkeep must not dismount a mounted travelling bot.
//     Donor parity: buff triggers stay active while mounted (a mounted bot
//     still wants its blessings), but the cast itself must wait - the
//     donor's buff path never strips a mount for upkeep; the traveller
//     re-buffs at the next stop (dismounted for combat, arrival, or loot).
//     Core Spell::CheckCast dismount stays untouched.
//
// No core includes; tested in tools/test_mount_dismount_policy.cpp.

namespace ai
{
    // (1) May a mounted bot dismount because it "can attack" this target?
    // `inCombat`: the bot's own combat flag. `targetFightsBot`: the target
    // attacks the bot, its pet, or its group (victim or threat -
    // AttackersValue::InCombat). `botOrderedToAttack`: an explicit attack
    // order or pull on this target. Any one of them makes the dismount
    // legitimate; a travelling bot that merely holds a nearby hostile as
    // its dps pick (none true) rides past.
    inline bool CombatDismountAllowed(bool inCombat, bool targetFightsBot, bool botOrderedToAttack)
    {
        return inCombat || targetFightsBot || botOrderedToAttack;
    }

    // (2) May an out-of-combat cast start while mounted and travelling?
    // Central gate in CastSpellAction::isUseful, covering every cast line
    // (paladin pve/pvp/raid blessings and blessing-on-party, shaman totems,
    // druid/mage/priest self-buffs): never while mounted and travelling out
    // of combat unless the core lets the spell ride
    // (SPELL_ATTR_CASTABLE_WHILE_MOUNTED, checked at the call site where the
    // SpellEntry is at hand). The trigger stays active (the need is real) so
    // the action is not queued each tick, and the cast runs at the next
    // dismounted stop. In combat the combat engine owns the decision
    // (unchanged); a bot that is not travelling keeps today's behavior
    // (unchanged). The mount cast itself ("mount") is exempt at the call
    // site; gather/loot/quest flows dismount on purpose inside their own
    // actions and never pass through here.
    inline bool MountedTravelCastAllowed(bool inCombat, bool travelling)
    {
        if (inCombat)
            return true;
        return !travelling;
    }

    // (3) Stable per-bot mount choice. Mount() sorted equal-speed mounts by
    // spell id, so EVERY bot rode the same mount (Warhorse everywhere) -
    // variety died. Tie-break by a hash of bot guid + spell id instead: one
    // bot always rides the same mount (deterministic per pair), while bots
    // differ from each other. Pure integer mix (splitmix64 finalizer),
    // no state, no RNG.
    inline unsigned StableMountTieBreak(unsigned botGuidLow, unsigned spellId)
    {
        unsigned long long z = (unsigned long long)botGuidLow * 0x9E3779B97F4A7C15ULL + spellId;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        z = z ^ (z >> 31);
        return (unsigned)(z & 0xFFFFFFFFu);
    }
}
