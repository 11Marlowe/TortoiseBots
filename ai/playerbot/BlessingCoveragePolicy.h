#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
// Paladin blessing coverage (raid Kings gap, 2026-10-14): who may still need
// a blessing, and which one to give them.
//
// Vanilla mechanics (1.12, this core): blessings of different families
// coexist on a target — exclusivity (SpellAuras.cpp _IsExclusiveSpellAura)
// competes only within an aura type, and Might is explicitly non-exclusive.
// Kings (stats %) and Wisdom (mana regen) are different aura types, so a
// healer keeps its Greater Wisdom AND gains Kings. Only the same family
// blocks: single and greater of one blessing share the slot (the per-target
// choice already checks both variants via HasAura). A Greater blessing lands
// on every group member of the target's class at once (per-CLASS, one cast
// per class present): coordination is per class bucket, not per member.
//
// Raid evidence (39 bots, 1 Retribution knowing Kings + 5 Holy without it):
// Greater Kings reached 22/39 — tanks and DPS, but no healer and not the ret
// himself. Two defects combined:
//
//  1. "Missing everything I know" target search. The on-party action asks for
//     the first member missing EVERY blessing it knows (might, wisdom,
//     kings, sanctuary, salvation, light). A healer that already has Wisdom
//     from a Holy paladin still needs Kings from the ret, but it is not
//     missing everything, so the ret never visits it. The search must visit
//     members missing ANYTHING the bot could give; the per-target choice
//     then skips the families already present.
//  2. Same-class exclusion. "Party member without aura" skips the bot itself
//     (PartyMemberValue::Check rejects player == bot), so the only Kings
//     source never blesses its own class. Greater Kings on a warrior covers
//     warriors only — the ret's own class still needs its cast.
//
// Donor (mod-playerbots PaladinGreaterBlessingAction.h): per-class buckets
// with a role-resolved priority list per bucket, a shared cached assignment,
// and a target scan that skips only the same family
// (HasEquivalentOrStrongerSameFamilyBlessing) — different families are
// freely overwritten/stacked. Ported here as pure per-target rules (no
// shared cache: every bot derives the same answer from its own spellbook,
// no invalidation on learn/talent/respec):
//   - one blessing per class bucket (class, or class+role where roles split
//     a class: warriors, paladins, druids, shamans — donor UsesRoleBucket);
//   - within a bucket, the role priority list decides the blessing;
//   - only the same family already present blocks; a member holding Wisdom
//     still gains Kings from the paladin that uniquely knows it.
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
    // priority-list entry the bot knows and the member lacks. Only the same
    // family already present blocks (donor HasEquivalentOrStrongerSameFamily:
    // single and greater share the slot — `present` counts either variant).
    // A member holding Wisdom still gains Kings: different aura types
    // coexist, so there is nothing to overwrite. `knows` marks families the
    // bot has trained (single or greater).
    inline BlessingFamily BlessingForMember(BlessingMember const& member,
        bool const* knows, bool const* present)
    {
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

    // Target search predicate for the on-party action: visit a member when the
    // bot could actually give it something — the member lacks at least one
    // known blessing (single or greater counts as having it). Replaces the
    // "missing every blessing I know" conjunction that starved healers
    // holding Wisdom while missing Kings. Self is visitable: the bot's own
    // class bucket still needs its Greater cast.
    inline bool MemberNeedsBlessingVisit(bool const* knows, bool const* present)
    {
        for (std::size_t f = 1; f <= 6; ++f)
            if (knows[f] && !present[f])
                return true;
        return false;
    }
}
