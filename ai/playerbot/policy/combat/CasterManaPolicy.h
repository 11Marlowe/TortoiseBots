#pragma once

// Pure policy for the DPS-caster mana loop (caster-mana-5): on a long
// fight a mana DPS spends every independent mana line on its own
// cooldown, cheapest-first, instead of wanding once the first line is
// spent. Live H8 (no Wisdom, 7-min boss): mages burned the gem
// ~hourly-on-cooldown then wanded below 5% with the rune row
// class-blocked, the potion row armed 40 s late (<40% instead of <65%),
// and Evocation fired once as a <15% last rite.
//
// Order encoded here (runtime triggers mirror it):
//   gem (free, conjured) -> potion (2 min line, from <65%) ->
//   rune (separate line, once the gem is spent/on cooldown) ->
//   evocation (channel, with the gem at <40%, not at <15%).
// Test-only header; runtime uses the triggers/actions.
namespace ai
{
    // Mana-gem rank ids low to high (Turtle 1.18.1).
    inline bool IsManaGemItem(unsigned id)
    {
        return id == 5514 || id == 5513 || id == 8007 || id == 8008;
    }

    // A mage with a ready gem in the bags prefers it over the rune (they
    // share a cooldown line); with no ready gem the rune must fire.
    inline bool MagePrefersGemOverRune(bool holdsReadyGem)
    {
        return holdsReadyGem;
    }

    // Potion arm starts at the high-water mark (<65%) so the 2 min
    // cooldown rolls early; waiting for <40% slips every later cycle.
    inline bool PotionArmedAtManaPercent(unsigned manaPct)
    {
        return manaPct < 65;
    }

    // Evocation arms with the gem (<40%), not as a <15% last rite.
    inline bool EvocationArmedAtManaPercent(unsigned manaPct)
    {
        return manaPct < 40;
    }
}
