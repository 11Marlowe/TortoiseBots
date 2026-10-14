#pragma once
#include <cstdint>
#include <string>

// Pure parsing/decision rules for `.bot totem` (owner request 2026-10-14:
// the player chooses which totems a shaman uses and WHEN it drops them).
// No core includes: callers translate game state into plain inputs so the
// rules stay testable in tools/test_totem_command_policy.cpp.
//
// Model: per-bot, per-slot order. Each of the four slots (earth/fire/water/
// air) holds one of: "auto" (the subgroup-aware round-1/2 choice, default),
// "none" (leave the slot empty), or an explicit totem spell name. Plus a
// per-bot drop mode: auto (today's behaviour), prepull (drop OOC before the
// pull and refresh in combat), manual (only on an explicit `drop` order).
// Orders persist per bot like the other behaviour toggles (manual values,
// saved through the AI store); the slot triggers consult them first and
// manual `totem ...` strategies keep winning over everything.

namespace ai
{
// The four totem slots, in canonical order.
inline int TotemSlotIndex(std::string const& slot)
{
    if (slot == "earth")
        return 0;
    if (slot == "fire")
        return 1;
    if (slot == "water")
        return 2;
    if (slot == "air")
        return 3;
    return -1;
}

inline char const* TotemSlotName(int index)
{
    switch (index)
    {
    case 0: return "earth";
    case 1: return "fire";
    case 2: return "water";
    case 3: return "air";
    default: return "";
    }
}

// Drop modes.
enum class TotemDropMode : std::uint8_t
{
    Auto = 0,
    Prepull = 1,
    Manual = 2
};

inline TotemDropMode ParseTotemDropMode(std::string const& mode)
{
    if (mode == "prepull" || mode == "pre-pull")
        return TotemDropMode::Prepull;
    if (mode == "manual")
        return TotemDropMode::Manual;
    return TotemDropMode::Auto;
}

inline char const* TotemDropModeName(TotemDropMode mode)
{
    switch (mode)
    {
    case TotemDropMode::Prepull: return "prepull";
    case TotemDropMode::Manual: return "manual";
    default: return "auto";
    }
}

// Manual-value keys (per-bot values, zero cost when unset).
inline char const* TotemOrderKey(int slot)
{
    switch (slot)
    {
    case 0: return "totem order earth";
    case 1: return "totem order fire";
    case 2: return "totem order water";
    case 3: return "totem order air";
    default: return "";
    }
}

inline char const* TotemModeKey() { return "totem drop mode"; }

// Canonical totem spell names per slot (what the player may order). "auto"
// and "none" are handled before this table; anything else must match one
// of these or the command answers ERR. Kept lowercase: the parser lowers
// the player's words before comparing.
inline char const* const* TotemChoicesForSlot(int slot, std::uint32_t& count)
{
    static char const* const earth[] = {"stoneclaw totem", "stoneskin totem", "earthbind totem",
        "strength of earth totem", "tremor totem"};
    static char const* const fire[] = {"searing totem", "magma totem", "fire nova totem",
        "flametongue totem", "fire resistance totem", "frost resistance totem"};
    static char const* const water[] = {"healing stream totem", "mana spring totem", "mana tide totem",
        "poison cleansing totem", "disease cleansing totem", "fire resistance totem"};
    static char const* const air[] = {"windfury totem", "grace of air totem", "tranquil air totem",
        "grounding totem", "windwall totem", "nature resistance totem"};
    switch (slot)
    {
    case 0: count = 5; return earth;
    case 1: count = 6; return fire;
    case 2: count = 6; return water;
    case 3: count = 6; return air;
    default: count = 0; return nullptr;
    }
}

// Validate one order word: "auto", "none", or a canonical totem for the
// slot. Returns the canonical stored form ("" for none, "auto" for auto,
// the totem name otherwise) or "" + valid=false on rejection. "none"
// stores as "" so an unset value and an explicit none behave the same
// downstream; the command layer echoes "none" back to the player.
inline std::string NormalizeTotemOrder(int slot, std::string const& word, bool& valid)
{
    if (word == "auto")
    {
        valid = true;
        return "auto";
    }
    if (word == "none" || word.empty())
    {
        valid = true;
        return "";
    }
    std::uint32_t count = 0;
    char const* const* choices = TotemChoicesForSlot(slot, count);
    for (std::uint32_t i = 0; i < count; ++i)
        if (word == choices[i])
        {
            valid = true;
            return std::string(choices[i]);
        }
    valid = false;
    return "";
}

// Which totem the slot trigger/action should want: the explicit order when
// set, otherwise "" meaning "fall back to the subgroup-aware auto pick".
// Manual `totem <slot> ...` strategies bypass this entirely (caller checks
// HasStrategy first, as today).
inline std::string OrderedTotemForSlot(std::string const& storedOrder)
{
    if (storedOrder.empty() || storedOrder == "auto")
        return "";
    return storedOrder;
}

// Should the automatic (non-manual, non-drop) rows fire right now?
// auto: whenever useful (today's behaviour). prepull: OOC only when the
// group is assembled for a fight, plus the in-combat refresh. manual:
// never — only the explicit `drop` order queues totems.
inline bool TotemAutoRowsFire(TotemDropMode mode, bool inCombat, bool fightImminent)
{
    switch (mode)
    {
    case TotemDropMode::Manual:
        return false;
    case TotemDropMode::Prepull:
        return inCombat || fightImminent;
    default:
        return true;
    }
}

// `drop` queues the configured set now, in or out of combat: every slot
// whose order (or auto pick) names a totem the bot knows. Slots set to
// none/empty-with-no-auto-pick are skipped, never forced. Moving into
// range rides the totem actions' own reach chain ("reach party member for
// totem"), so the command only queues.
inline bool TotemDropWantsSlot(bool slotOccupied, std::string const& wantTotem)
{
    if (wantTotem.empty())
        return false;
    return !slotOccupied;
}
}
