#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
// Paladin blessing coverage (raid Kings gap, rounds 1+2, 2026-10-14): who may
// still need a blessing, and which one to give them.
//
// Core mechanics (this core, verified live): every blessing classifies
// SPELL_BLESSING (SpellEntry.cpp GetSpellSpecific), which is one-per-target-
// per-caster (SpellEntry.h IsSingleFromSpellSpecificPerTargetPerCaster). On
// aura add Unit::RemoveNoStackAurasDueToAuraHolder removes the existing
// blessing when the caster GUID matches — so a second blessing from the
// SAME paladin ALWAYS evicts the first, whatever the family (round-1's
// "different families coexist" model was wrong: _IsExclusiveSpellAura
// explicitly exempts blessings, SpellAuras.cpp, and never decides them).
// Different casters coexist for different blessings; the same chain from
// another caster fights by rank. Consequence: each paladin owns exactly one
// blessing slot per member. The on-party action/trigger therefore skips any
// member holding ANY blessing (single or greater, any caster) — the donor
// mod-playerbots single-path HasBlessingAura guard — and picks its role
// priority blessing only for members holding nothing. A Greater blessing
// lands on every group member of the target's class at once (per-CLASS, one
// cast per class present): coordination is per class bucket, not per member.
//
// Round-1 evidence (39 bots, 1 Retribution knowing Kings + 5 Holy without
// it): Greater Kings reached 22/39 — tanks and DPS, but no healer and not
// the ret himself. Two defects: the "missing EVERYTHING I know" search never
// visited healers holding Wisdom, and it excluded the bot itself (its own
// class bucket never got the Greater cast). Round 1 fixed the visit rule;
// round-2 live evidence (5-man: ret + holy paladin) showed the fix unmasked
// the eviction churn underneath: the ret alternated Light/Kings on the holy
// paladin (each cast evicting its own previous one), the holy paladin
// alternated Might/Wisdom on the priest the same way, and the GM master got
// nothing (correct — uncached real player, invisible to the bots).
//
// Donor (mod-playerbots PaladinGreaterBlessingAction.h/.cpp,
// PaladinActions.cpp single path): deterministic per-paladin assignment
// shards (GUID-sorted pool, distinct owners per bucket), class buckets with
// role-resolved priority (UsesRoleBucket), HasEquivalentOrStrongerSameFamily
// skip, and — the piece ported here — the single-target path refuses any
// member holding ANY blessing family already. Ported as pure per-target
// rules (no shared cache: every bot derives the same answer from live aura
// state, no invalidation on learn/talent/respec):
//   - one blessing slot per member per paladin (core eviction rule);
//   - within a bucket, the role priority list decides the blessing;
//   - any blessing already present blocks; a member holding Wisdom is left
//     alone, never "topped up" with Kings by the same or another bot.
//
// No core includes: callers translate game state into plain inputs, so the
// rules stay testable in tools/test_blessing_coverage_policy.cpp.

namespace ai
{
    // A group member as the coverage rule needs to see it.
    struct BlessingMember
    {
        std::uint64_t id = 0;     // stable per character (the guid)
        std::uint8_t classId = 0; // CLASS_* (1=warrior .. 11=druid)
        bool tank = false;        // PlayerbotAI::IsTank
        bool healer = false;      // PlayerbotAI::IsHeal
        bool ranged = false;      // PlayerbotAI::IsRanged (hunters want wisdom first)
        bool self = false;        // is the blessing bot itself
    };

    // Blessing families this rule reasons about. Salvation/Light rank in the
    // priority lists; only same-family presence blocks (either variant).
    enum class BlessingFamily : std::uint8_t
    {
        None = 0,
        Might,
        Wisdom,
        Kings,
        Sanctuary,
        Salvation,
        Light,
    };

    inline std::string BlessingBaseName(BlessingFamily family)
    {
        switch (family)
        {
            case BlessingFamily::Might: return "blessing of might";
            case BlessingFamily::Wisdom: return "blessing of wisdom";
            case BlessingFamily::Kings: return "blessing of kings";
            case BlessingFamily::Sanctuary: return "blessing of sanctuary";
            case BlessingFamily::Salvation: return "blessing of salvation";
            case BlessingFamily::Light: return "blessing of light";
            default: return std::string();
        }
    }

    // Classes whose tanks/healers/dps want different blessings (donor
    // UsesRoleBucket: warrior, paladin, druid, shaman; death knights do not
    // exist on 1.12). Other classes share one bucket per class.
    inline bool BlessingUsesRoleBucket(std::uint8_t classId)
    {
        // CLASS_WARRIOR=1, CLASS_PALADIN=2, CLASS_SHAMAN=7, CLASS_DRUID=11.
        return classId == 1 || classId == 2 || classId == 7 || classId == 11;
    }

    // Role priority list for a member: the blessing order the per-role lists
    // in PaladinActions.cpp already use (pve/raid rows). Index 0 is the most
    // wanted. Kings ranks second for healers and most dps — the fallback the
    // old "missing everything" search never reached.
    inline std::vector<BlessingFamily> BlessingPriorityFor(BlessingMember const& member)
    {
        if (member.tank)
        {
            if (member.classId == 2) // paladin tank: wisdom first (donor ROLE_PALADIN_TANK)
                return { BlessingFamily::Wisdom, BlessingFamily::Kings, BlessingFamily::Might,
                         BlessingFamily::Sanctuary, BlessingFamily::Light };
            return { BlessingFamily::Kings, BlessingFamily::Might, BlessingFamily::Sanctuary,
                     BlessingFamily::Light, BlessingFamily::Wisdom };
        }
        if (member.healer)
            return { BlessingFamily::Wisdom, BlessingFamily::Kings, BlessingFamily::Light,
                     BlessingFamily::Sanctuary, BlessingFamily::Might };
        if (member.classId == 3) // hunter: mana pool wants wisdom first
            return { BlessingFamily::Wisdom, BlessingFamily::Kings, BlessingFamily::Might,
                     BlessingFamily::Light, BlessingFamily::Sanctuary };
        if (member.ranged)
            return { BlessingFamily::Kings, BlessingFamily::Wisdom, BlessingFamily::Light,
                     BlessingFamily::Sanctuary, BlessingFamily::Might };
        if (member.classId == 2) // paladin dps: wisdom first, kings third
            return { BlessingFamily::Wisdom, BlessingFamily::Might, BlessingFamily::Kings,
                     BlessingFamily::Light, BlessingFamily::Sanctuary };
        return { BlessingFamily::Might, BlessingFamily::Kings, BlessingFamily::Light,
                 BlessingFamily::Wisdom, BlessingFamily::Sanctuary };
    }

    // Two members share a Greater-blessing bucket when one Greater cast covers
    // both: same class, and same role where the class splits by role.
    inline bool SameBlessingBucket(BlessingMember const& a, BlessingMember const& b)
    {
        if (a.classId != b.classId)
            return false;
        if (!BlessingUsesRoleBucket(a.classId))
            return true;
        if (a.tank != b.tank || a.healer != b.healer)
            return false;
        return a.ranged == b.ranged;
    }

    // The blessing this member should carry from this bot: the first
    // priority-list entry the bot knows, but only when the member holds NO
    // blessing at all (single or greater, any caster — `present` counts any
    // family). One slot per member per paladin (core eviction rule): a member
    // holding Wisdom is left alone, never "topped up" with Kings — the cast
    // would evict the live aura (same caster) or churn against the other
    // paladin's. `knows` marks families the bot has trained.
    inline BlessingFamily BlessingForMember(BlessingMember const& member,
        bool const* knows, bool const* present)
    {
        for (std::size_t f = 1; f <= 6; ++f)
            if (present[f])
                return BlessingFamily::None;
        std::vector<BlessingFamily> priority = BlessingPriorityFor(member);
        for (std::size_t i = 0; i < priority.size(); ++i)
        {
            BlessingFamily const family = priority[i];
            std::size_t const f = static_cast<std::size_t>(family);
            if (present[f] || !knows[f])
                continue;
            return family;
        }
        return BlessingFamily::None;
    }

    // Target search predicate for the on-party action: visit a member only
    // when it holds NO blessing at all and the bot knows at least one family.
    // Any live blessing means the slot is spoken for (see above) — visiting
    // it could only overwrite. Self is visitable: the bot's own class bucket
    // still needs its Greater cast.
    inline bool MemberNeedsBlessingVisit(bool const* knows, bool const* present)
    {
        for (std::size_t f = 1; f <= 6; ++f)
            if (present[f])
                return false;
        for (std::size_t f = 1; f <= 6; ++f)
            if (knows[f])
                return true;
        return false;
    }
}
