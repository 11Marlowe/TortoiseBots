#pragma once

#include "PullStrategy.h"

// Shaman pull spell selection (issue #1125, Part B2): tank shamans open
// 2+ pulls with Chain Lightning and single pulls with Lightning Bolt
// (owner REVISION). Every other shaman spec keeps the base Lightning Bolt
// pull: the Chain Lightning branch is gated on the tank strategy, the spell
// being known and castable, and a 2+ pack count — never a blind AoE opener
// (the possible-targets count strips CC'd marks; the attackers count covers
// the linked-aggro moment the pull action itself executes in).
class ShamanPullStrategy : public PullStrategy
{
public:
    ShamanPullStrategy(PlayerbotAI* botAI) : PullStrategy(botAI, "lightning bolt") {}

    std::string GetPullActionName() const override;
    std::string GetPreActionName() const override;
};
