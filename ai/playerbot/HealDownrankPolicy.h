#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

// Pure decision rules for healer downranking: pick the cheapest known rank of
// a direct heal whose estimated amount covers the target's still-missing
// health (missing minus heals already incoming from other healers), plus a
// safety margin. No core includes: callers in strategy/values translate game
// state into these plain inputs, so the rules stay testable in
// tools/test_heal_downrank_policy.cpp without the server.
//
// Why this shape (owner-confirmed 1.12 behaviour): a vanilla healer matches
// the spell to the wound - a small loss gets a low rank, a big loss the top
// rank. Rank-1 Heal costs ~30 mana for ~50 healing; top Greater Heal costs
// 710 for ~2000. The Patchwerk raid (2026-10-13) showed the failure mode:
// 17 healers cast top rank 202+ times each for ~2.55M healing on ~945k damage
// taken (~63% overheal) and ran dry before the boss died. The branch base
// already stops duplicate big heals (incoming-heal aware target selection,
// fix/raid-heal-economy); this picks a rank that fits whatever wound is left.
//
// What the "mana save level" value does and why it cannot drive this:
// AI_VALUE(double, "mana save level") is a MANUAL knob (ManaSaveLevelValue,
// default 1.0), set only by the "save mana" chat command. In SpellIdValue it
// picks the Nth rank from the top (saveMana == rank -> return *i), i.e. a
// player-ordered fixed offset with no notion of the target's wound. The
// SaveManaMultiplier only delays recasts (10 s * saveLevel), it never picks
// ranks either. So the knob stays manual; downranking is automatic per cast
// from the wound size. An explicit save-mana level above 1 still wins: the
// player asked for cheap ranks, keep returning what the old path returned.
//
// Donor survey (read-only references, no prior art to port):
//  - mod-playerbots: no rank selection anywhere. HealerAutoSaveManaMultiplier
//    (src/Ai/Base/Strategy/ConserveManaStrategy.cpp:92-129) only VETOES an
//    oversized/inefficient heal start; the estAmount/manaEfficiency table on
//    each party heal action only feeds that veto. Already ported here as
//    HealerManaPolicy::ShouldStartHeal (night2 gaps 1+2).
//  - spp-classics-cmangos: no rank selection; only the core AutoDownrank
//    mangosd.conf knob (buff/heal lower-level players), which is level-based,
//    not wound-based.
// So there is nothing to port; the fit-to-wound rule below is local.

namespace ai
{
    // One known rank of a heal spell, cheapest-first. Callers fill `heal`
    // with the rank's estimated amount on THIS target (base + spell-power
    // bonus after the 1.12 sub-level-20 penalty, from the same
    // CalculateDamage-style path the incoming-heal scan uses) and `mana`
    // with its cost. `spellId` is the rank's spell id to cast.
    struct HealRankOption
    {
        std::uint32_t spellId;
        std::uint32_t heal;
        std::uint32_t mana;
    };

    struct HealDownrankState
    {
        std::uint32_t missingHealth;  // target missing HP after incoming heals
        std::uint8_t targetHealth;    // 0-100, target's CURRENT percent (pre-incoming)
        std::uint8_t lowHealth;       // kept for callers; emergency reads criticalHealth only
        std::uint8_t criticalHealth;  // spike line: top rank regardless
    };

    // Implicit-target ids the downrank gate accepts, mirroring the core's
    // SpellTarget enum (game/Spells/SpellDefines.h, verified 2026-10-14):
    // 21 = TARGET_UNIT_FRIEND, 45 = TARGET_UNIT_FRIEND_CHAIN_HEAL.
    // Turtle's Healing Wave rows use 45 with zero jumps; Chain Heal uses
    // 45 with 3 jumps. Literals, not the enum: this header takes no core
    // includes so the rules stay testable without the server.
    constexpr std::uint32_t kDownrankTargetFriend = 21;
    constexpr std::uint32_t kDownrankTargetFriendChainHeal = 45;

    // True when the spell line aims at one friendly target: plain friend
    // targeting, or the chain-heal id WITHOUT jumps (Healing Wave). Lines
    // that really jump (Chain Heal, EffectChainTarget 3) price the jumps
    // in every rank, so fitting a rank to the first target's wound would
    // underpay the cast — they stay top rank.
    inline bool HealTargetAllowsDownrank(std::uint32_t implicitTargetA, std::uint32_t chainTargets)
    {
        return (implicitTargetA == kDownrankTargetFriend ||
                implicitTargetA == kDownrankTargetFriendChainHeal) &&
               chainTargets == 0;
    };

    // Fraction of the wound the chosen rank must cover: full coverage would
    // keep picking top rank on any rounding miss, so 4/5 with a flat floor
    // keeps small wounds on cheap ranks while big wounds still land.
    constexpr std::uint32_t kHealDownrankCoverNumerator = 4;
    constexpr std::uint32_t kHealDownrankCoverDenominator = 5;
    constexpr std::uint32_t kHealDownrankCoverFlatFloor = 100;

    // True when the target is an emergency: at or below the criticalHealth
    // spike line the caller must cast the top rank, never downrank. The old
    // rule also treated everything at/below lowHealth (50 %) as an
    // emergency, so the "party member medium health" row (50-70 %) always
    // paid top rank — live raid 2026-10-14: Healing Wave top rank 56x from
    // the party rows. Medium wounds now fit a rank to the wound like every
    // other band; only the spike line keeps the big heal unconditionally.
    inline bool HealNeedsTopRank(HealDownrankState const& state)
    {
        return state.targetHealth <= state.criticalHealth;
    }

    inline std::uint32_t HealDownrankNeed(HealDownrankState const& state)
    {
        std::uint32_t const scaled =
            (state.missingHealth * kHealDownrankCoverNumerator) / kHealDownrankCoverDenominator;
        return std::max(scaled, kHealDownrankCoverFlatFloor);
    }

    // Cheapest rank covering the need; cheapest when no wound is left, top
    // rank on emergency or when nothing covers, 0 on an empty list (caller
    // falls back to the old selection). Ranks MUST be cheapest-first
    // (ascending mana); a single pass, no alloc.
    inline std::uint32_t PickHealRank(std::vector<HealRankOption> const& ranks, HealDownrankState const& state)
    {
        if (ranks.empty())
            return 0;
        // Emergency (at/below the danger lines): the big heal, always.
        if (HealNeedsTopRank(state))
            return ranks.back().spellId;
        // No wound left (incoming covers it): waste the least if forced.
        if (state.missingHealth == 0)
            return ranks.front().spellId;
        std::uint32_t const need = HealDownrankNeed(state);
        for (HealRankOption const& rank : ranks)
            if (rank.heal >= need)
                return rank.spellId;
        return ranks.back().spellId;
    }
}
