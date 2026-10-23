#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
// Paladin blessing coverage (raid Kings gap, rounds 1-3, 2026-10-14): who
// may still need a blessing, and which one to give them.
//
// Core mechanics (this core, verified live): every blessing classifies
// SPELL_BLESSING (SpellEntry.cpp GetSpellSpecific), which is one-per-target-
// per-caster (SpellEntry.h IsSingleFromSpellSpecificPerTargetPerCaster). On
// aura add Unit::RemoveNoStackAurasDueToAuraHolder removes the existing
// blessing when the caster GUID matches — so a second blessing from the
// SAME paladin ALWAYS evicts the first, whatever the family. Blessings from
// DIFFERENT casters stack: each member converges to one blessing from EACH
// paladin (e.g. Kings from the ret + Wisdom from the holy). Consequence per
// paladin: never cast a second, different blessing where one of ITS OWN
// blessings already sits (that cast evicts its own live aura — the
// Light/Kings and Might/Wisdom churn seen live 2026-10-14); where the member
// holds no blessing from this bot, pick the highest role-priority blessing
// the bot knows that no caster already provides (same family from any caster
// covers the member — single and greater share the slot per caster, a second
// family copy adds nothing). The paladin that uniquely knows a blessing
// (Kings, a Retribution talent on Turtle — verified in tw_char for the live
// rets) therefore prefers it wherever the member lacks it, whatever else the
// member holds from others. Hidden GM masters (VISIBILITY_OFF) are skipped
// in the scans: the core drops them in Spell::CheckTarget post-cast, so they
// read as blessable forever and wedge the loop (same skip as
// PartyMemberValue::Check).
//
// Donor (mod-playerbots PaladinGreaterBlessingAction.h/.cpp,
// PaladinActions.cpp single path): deterministic per-paladin assignment
// shards (GUID-sorted pool, distinct owners per bucket), class buckets with
// role-resolved priority (UsesRoleBucket), HasEquivalentOrStrongerSameFamily
// skip, and the single-target HasBlessingAura guard. Ported here as pure
// per-target rules, caster-aware (HasMyAura for own slot, HasAura for other
// casters' cover) with no shared cache: every bot derives the same answer
// from live aura state, no invalidation on learn/talent/respec:
//   - one blessing slot per member PER PALADIN (core eviction rule);
//   - within a bucket, the role priority list decides the blessing;
//   - own live blessing blocks everything; same-family cover from any caster
//     blocks that family only; other families from other casters never block.
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
    // priority-list entry the bot knows that no caster already provides.
    // `mine` marks families THIS bot already provides (single or greater —
    // own live aura stands, never overwrite it: the cast would evict it).
    // `present` marks families ANY caster provides (single or greater —
    // same-family cover from anyone blocks that family only; other families
    // from other casters never block, they stack). `knows` marks families
    // the bot has trained (single or greater).
    inline BlessingFamily BlessingForMember(BlessingMember const& member,
        bool const* knows, bool const* present, bool const* mine)
    {
        for (std::size_t f = 1; f <= 6; ++f)
            if (mine[f])
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

    // Target search predicate for the on-party action: visit a member when
    // this bot holds no live blessing on it and at least one known family is
    // uncovered by any caster. Another paladin's aura never blocks the visit
    // (different casters stack). Self is visitable: the bot's own class
    // bucket still needs its Greater cast.
    inline bool MemberNeedsBlessingVisit(bool const* knows, bool const* present, bool const* mine)
    {
        for (std::size_t f = 1; f <= 6; ++f)
            if (mine[f])
                return false;
        for (std::size_t f = 1; f <= 6; ++f)
            if (knows[f] && !present[f])
                return true;
        return false;
    }
}
