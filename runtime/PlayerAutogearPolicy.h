#pragma once

// Pure rules for `.autogear`, the real-player gear command. The gear pass
// itself (candidate pool, score, limits) is PlayerbotFactory's; this header
// holds only what must be unit-testable without a world: spec name and alias
// parsing, the permission rule and the item-level clamp. The canonical scale
// names here match the `name` column of ai_playerbot_weightscales as shipped
// by the module's world SQL.

#include <cstddef>
#include <cstdint>
#include <string>

namespace TortoiseBots
{

// One valid spec for a class: the word players type (display) and the
// weightscale name the gear pool scores with (scale).
struct PlayerAutogearSpec
{
    const char* display;
    const char* scale;
};

// Aliases for one spec, display token first. Hyphens in stored tokens are
// written as spaces; matching normalizes them the same way.
struct PlayerAutogearSpecRow
{
    uint8_t classId;
    const char* display;
    const char* scale;
    const char* const* aliases;
    size_t aliasCount;
};

constexpr uint8_t kAutogearMaxLevel = 60;

namespace PlayerAutogearDetail
{
// Alias lists, display token first (that token is always accepted).
constexpr const char* const kWarriorArms[] = {"arms"};
constexpr const char* const kWarriorFury[] = {"fury", "dw", "dualwield"};
constexpr const char* const kWarriorProt[] = {"protection", "prot", "tank"};
constexpr const char* const kPaladinHoly[] = {"holy", "healer", "heal"};
constexpr const char* const kPaladinProt[] = {"protection", "prot", "tank"};
constexpr const char* const kPaladinRet[] = {"retribution", "ret", "retri"};
constexpr const char* const kHunterBeast[] = {"beast", "beast mastery", "bm"};
constexpr const char* const kHunterMarks[] = {"marksmanship", "marks", "mm"};
constexpr const char* const kHunterSurv[] = {"survival", "surv"};
constexpr const char* const kRogueAssas[] = {"assassination", "assas", "sin", "daggers"};
constexpr const char* const kRogueCombat[] = {"combat", "swords"};
constexpr const char* const kRogueSubtle[] = {"subtlety", "subtle", "sub"};
constexpr const char* const kPriestDisc[] = {"discipline", "disc"};
constexpr const char* const kPriestHoly[] = {"holy", "healer", "heal"};
constexpr const char* const kPriestShadow[] = {"shadow"};
constexpr const char* const kShamanElem[] = {"elemental", "elem", "ele", "caster"};
constexpr const char* const kShamanEnh[] = {"enhancement", "enhance", "enh"};
constexpr const char* const kShamanEnhTank[] = {"enhancement tank", "enh tank", "tank"};
constexpr const char* const kShamanResto[] = {"restoration", "resto", "healer", "heal"};
constexpr const char* const kMageArcane[] = {"arcane"};
constexpr const char* const kMageFire[] = {"fire"};
constexpr const char* const kMageFrost[] = {"frost"};
constexpr const char* const kWarlockAfflic[] = {"affliction", "afflic", "affli"};
constexpr const char* const kWarlockDemo[] = {"demonology", "demo"};
constexpr const char* const kWarlockDestro[] = {"destruction", "destro"};
constexpr const char* const kDruidBalance[] = {"balance", "moonkin", "boomkin"};
constexpr const char* const kDruidFeral[] = {"feral", "feral dps", "cat"};
constexpr const char* const kDruidFeralTank[] = {"feral tank", "bear", "tank"};
constexpr const char* const kDruidResto[] = {"restoration", "resto", "healer", "heal"};

#define TORTE_AUTOGEAR_ROW(classId, display_, scale_, aliases) \
    { classId, display_, scale_, aliases, sizeof(aliases) / sizeof(aliases[0]) }

inline const PlayerAutogearSpecRow* Rows(size_t& count)
{
    static const PlayerAutogearSpecRow rows[] = {
        TORTE_AUTOGEAR_ROW(1, "arms", "arms", kWarriorArms),
        TORTE_AUTOGEAR_ROW(1, "fury", "fury", kWarriorFury),
        TORTE_AUTOGEAR_ROW(1, "protection", "prot", kWarriorProt),
        TORTE_AUTOGEAR_ROW(2, "holy", "holy", kPaladinHoly),
        TORTE_AUTOGEAR_ROW(2, "protection", "prot", kPaladinProt),
        TORTE_AUTOGEAR_ROW(2, "retribution", "retrib", kPaladinRet),
        TORTE_AUTOGEAR_ROW(3, "beast", "beast", kHunterBeast),
        TORTE_AUTOGEAR_ROW(3, "marksmanship", "marks", kHunterMarks),
        TORTE_AUTOGEAR_ROW(3, "survival", "surv", kHunterSurv),
        TORTE_AUTOGEAR_ROW(4, "assassination", "assas", kRogueAssas),
        TORTE_AUTOGEAR_ROW(4, "combat", "combat", kRogueCombat),
        TORTE_AUTOGEAR_ROW(4, "subtlety", "subtle", kRogueSubtle),
        TORTE_AUTOGEAR_ROW(5, "discipline", "disc", kPriestDisc),
        TORTE_AUTOGEAR_ROW(5, "holy", "holy", kPriestHoly),
        TORTE_AUTOGEAR_ROW(5, "shadow", "shadow", kPriestShadow),
        TORTE_AUTOGEAR_ROW(7, "elemental", "elem", kShamanElem),
        TORTE_AUTOGEAR_ROW(7, "enhancement", "enhance", kShamanEnh),
        TORTE_AUTOGEAR_ROW(7, "enhancement-tank", "shamantank", kShamanEnhTank),
        TORTE_AUTOGEAR_ROW(7, "restoration", "resto", kShamanResto),
        TORTE_AUTOGEAR_ROW(8, "arcane", "arcane", kMageArcane),
        TORTE_AUTOGEAR_ROW(8, "fire", "fire", kMageFire),
        TORTE_AUTOGEAR_ROW(8, "frost", "frost", kMageFrost),
        TORTE_AUTOGEAR_ROW(9, "affliction", "afflic", kWarlockAfflic),
        TORTE_AUTOGEAR_ROW(9, "demonology", "demo", kWarlockDemo),
        TORTE_AUTOGEAR_ROW(9, "destruction", "destro", kWarlockDestro),
        TORTE_AUTOGEAR_ROW(11, "balance", "balance", kDruidBalance),
        TORTE_AUTOGEAR_ROW(11, "feral", "feraldps", kDruidFeral),
        TORTE_AUTOGEAR_ROW(11, "feral-tank", "feraltank", kDruidFeralTank),
        TORTE_AUTOGEAR_ROW(11, "restoration", "resto", kDruidResto),
    };
    count = sizeof(rows) / sizeof(rows[0]);
    return rows;
}

#undef TORTE_AUTOGEAR_ROW
} // namespace PlayerAutogearDetail

// Lowercase, hyphens to spaces, whitespace collapsed to single spaces.
inline std::string NormalizeAutogearSpecToken(std::string token)
{
    std::string out;
    out.reserve(token.size());
    bool pendingSpace = false;
    for (char& c : token)
    {
        unsigned char u = static_cast<unsigned char>(c);
        if (u == '-' || u == ' ' || u == '\t')
        {
            pendingSpace = !out.empty();
            continue;
        }
        if (pendingSpace)
        {
            out += ' ';
            pendingSpace = false;
        }
        out += static_cast<char>(u >= 'A' && u <= 'Z' ? u + ('a' - 'A') : u);
    }
    return out;
}

// The weightscale name for the normalized spec token on this class, or "".
inline std::string MatchAutogearSpec(uint8_t classId, std::string const& rawToken)
{
    std::string token = NormalizeAutogearSpecToken(rawToken);
    if (token.empty())
        return {};
    size_t count = 0;
    for (const PlayerAutogearSpecRow* row = PlayerAutogearDetail::Rows(count); count--; ++row)
    {
        if (row->classId != classId)
            continue;
        for (size_t i = 0; i < row->aliasCount; ++i)
            if (token == row->aliases[i])
                return row->scale;
    }
    return {};
}

// True when the token is a spec word of any class (drives the name/spec
// ambiguity split in the command: "enhancement tank" is a spec, "Thrall"
// is a player name).
inline bool IsAutogearSpecToken(std::string const& rawToken)
{
    std::string token = NormalizeAutogearSpecToken(rawToken);
    if (token.empty())
        return false;
    size_t count = 0;
    for (const PlayerAutogearSpecRow* row = PlayerAutogearDetail::Rows(count); count--; ++row)
        for (size_t i = 0; i < row->aliasCount; ++i)
            if (token == row->aliases[i])
                return true;
    return false;
}

// "protection, fury, arms" — the valid spec words for the class, for the
// unknown-spec reply. Empty for classes the gear pool does not score.
inline std::string ListAutogearSpecs(uint8_t classId)
{
    std::string out;
    size_t count = 0;
    for (const PlayerAutogearSpecRow* row = PlayerAutogearDetail::Rows(count); count--; ++row)
    {
        if (row->classId != classId)
            continue;
        if (!out.empty())
            out += ", ";
        out += row->display;
    }
    return out;
}

enum class PlayerAutogearPermission
{
    Allowed,
    GmOnly,           // flag off and the caller is not a GM
    NotSelf,          // flag on but the caller named another player
    ChallengeGmOnly,  // challenge-mode target: only a GM may gear it
};

// GMs gear anyone; challenge-mode characters (Hardcore, Slow & Steady, ...)
// are GM-only whatever the flag; with the for-all flag every player may gear
// only themselves; otherwise it is GM-only.
inline PlayerAutogearPermission DecidePlayerAutogearPermission(bool callerIsGm, bool forAll, bool targetsSelf,
    bool targetIsChallengeMode)
{
    if (targetIsChallengeMode && !callerIsGm)
        return PlayerAutogearPermission::ChallengeGmOnly;
    if (callerIsGm)
        return PlayerAutogearPermission::Allowed;
    if (!forAll)
        return PlayerAutogearPermission::GmOnly;
    return targetsSelf ? PlayerAutogearPermission::Allowed : PlayerAutogearPermission::NotSelf;
}

enum class PlayerAutogearTargetState
{
    Ready,
    Dead,
    InCombat,
    Shapeshifted,
};

inline PlayerAutogearTargetState DecidePlayerAutogearTargetState(bool alive, bool inCombat, bool shapeshifted)
{
    if (!alive)
        return PlayerAutogearTargetState::Dead;
    if (shapeshifted)
        return PlayerAutogearTargetState::Shapeshifted;
    if (inCombat)
        return PlayerAutogearTargetState::InCombat;
    return PlayerAutogearTargetState::Ready;
}

// A unique or unique-equippable item may be given only when the player owns
// no copy (equipped or in the bags).
inline bool AutogearUniqueAllowed(bool isUniqueFlagged, uint32_t ownedCount)
{
    return !isUniqueFlagged || ownedCount == 0;
}

// A set keeps all its equipped pieces while it triggers at least one bonus:
// `spells`/`thresholds` are the DBC ItemSetEntry columns (a piece of a
// bonus-giving set is never an upgrade worth breaking the set for).
inline bool AutogearSetBonusActive(uint32_t equippedPieces, uint32_t const* spells, uint32_t const* thresholds, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if (spells[i] && thresholds[i] && thresholds[i] <= equippedPieces)
            return true;
    return false;
}

struct PlayerAutogearIlvl
{
    uint32_t effective = 0; // the ceiling the gear pass runs with
    bool clamped = false;   // a request at or above the default was pulled down
};

// Optional `ilvl` argument against the default bot ceiling. Below max level
// the request can only lower the ceiling; at max level it may raise it (the
// best gear in the pool). The required-level rule (required level <= player
// level) is separate and always applies in the gear pass.
inline PlayerAutogearIlvl ResolvePlayerAutogearIlvl(uint32_t playerLevel, uint32_t maxLevel,
    uint32_t requested, uint32_t defaultCeiling)
{
    PlayerAutogearIlvl result;
    if (!requested)
    {
        result.effective = defaultCeiling;
        return result;
    }
    if (requested < defaultCeiling)
    {
        result.effective = requested;
        return result;
    }
    if (playerLevel >= maxLevel && requested > defaultCeiling)
    {
        result.effective = requested;
        return result;
    }
    result.effective = defaultCeiling;
    result.clamped = true;
    return result;
}

} // namespace TortoiseBots
