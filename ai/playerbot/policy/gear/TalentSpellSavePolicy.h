#pragma once

// This core keeps a character's talents nowhere but `character_spell`: there is
// no `character_talent` table, and Player::_LoadSpells rebuilds the spell map
// (and with it the spent-talent count) from those rows alone. A spell the core
// marks *dependent* is never written - "_SaveSpells: add only changed/new not
// dependent spells" (tortoise-wow src/game/Objects/Player.cpp:18888-18889) - the
// flag means "derived from another spell, reloading re-derives it".
//
// The core sets the flag while walking a spell CHAIN: learning a rank also
// teaches the lower ranks as dependent (Player::AddSpell, Player.cpp:4436-4442).
// Turtle's skill_line_ability data puts a few *talent* rank-1 spells at the head
// of a chain whose higher ranks are trainer-taught:
//
//   Lightning Strike 51387 (talent 258 rank 1) -> 52420 (rank 2, trainer row
//     52423 teaches it) -> 52422 (rank 3, row 52424)   [shaman, Enhancement]
//   Mortal Strike 12294 (talent 73 rank 1) -> 21551 (rank 2, row 21555) ->
//     21552 (rank 3, row 21557) -> 21553                [warrior, Arms]
//   Bloodthirst 23881, Shield Slam 23922, Moonkin Form 24858, Repentance 20066,
//   Hemorrhage 16511, 12809, 51346, 51478, 51575, 52714  [five more classes]
//
// Teaching such a higher rank therefore marks the *talent* rank dependent, and
// the talent silently stops being saved: a 51-point shaman tank hire ended with
// 16 of its spec's 17 rank spells in `character_spell` while its live spellbook
// (the intro, and the load-time source of the talent count) still read 15/36 -
// the armory dashboard, which reads the DB, showed 15/35. Same for a 40-point
// Arms warrior hire whose Mortal Strike was in the live spellbook but in no row.
//
// This core persists talents only that way, so a talent rank spell must never
// stay dependent - the mirror of the rule the core already applies to the
// sibling flag: "talents should never be marked as disabled" (Player.cpp:4633).
// The module cannot clear it through the public API: Player::LearnSpell /
// Player::AddSpell only ever *set* dependent (Player.cpp:4344-4347), never clear
// it, and RemoveSpell + LearnSpell is no alternative - removing a chain head
// cascades into disabling every higher rank the bot trained (Player.cpp:4638-4641
// with RemoveSpell(disabled = !IsPassiveSpell(...))). So the module re-asserts
// the entry instead of trusting the chain pass; both callers of the rule
// (PlayerbotFactory::EnsureTalentSpellsSaved) go through it.

#include <cstdint>

namespace ai
{

// One spellbook entry as Player::_SaveSpells will read it.
struct TalentSpellSaveInputs
{
    bool isTalentSpell;   // GetTalentSpellCost(spell) != 0
    bool dependent;       // learned as a by-product of another spell
    bool removed;         // state == PLAYERSPELL_REMOVED
    bool disabled;        // first rank known but the talent not spent
};

// True when the module must force the entry back into the saved set: a talent
// rank spell the bot knows (not removed, not disabled) whose dependent flag
// would keep it out of `character_spell`. Non-talent spells are left alone - a
// derived lower rank (52420 above) is *meant* to be unsaved and is re-derived
// from the higher rank on load.
inline bool TalentSpellMustStaySaved(TalentSpellSaveInputs const& entry)
{
    if (!entry.isTalentSpell || entry.removed || entry.disabled)
        return false;

    return entry.dependent;
}

} // namespace ai
