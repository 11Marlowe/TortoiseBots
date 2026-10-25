#pragma once

// RPG action weight as a pure policy (no core includes), so tools/test_rpg_relevance_weight_policy.cpp compiles it
// standalone. RpgAction::Execute uses it for the weights that WeightedShuffle draws the action from.
//
// The weight is (relevance - 1) * 1000, as before, clamped to the uint32 range. A relevance below 1 used to make that
// a negative float, and converting a negative float to uint32 is undefined behaviour (in practice a weight near
// 4.29e9, which won almost every pick). Weight 0 now goes after every positive weight (WeightedShufflePolicy.h).
// Relevance 1 is still weight 0, and nothing above 1 changes.

#include <algorithm>
#include <cstdint>

namespace ai
{
    inline std::uint32_t RpgRelevanceWeight(float relevance)
    {
        // Largest float below UINT32_MAX; float(UINT32_MAX) rounds up to 2^32, which the cast cannot take.
        constexpr float kMaxWeight = 4294967040.0f;
        // std::max(0.0f, x) maps NaN to 0 by argument order (the first argument wins a failed comparison).
        float const weight = std::min(kMaxWeight, std::max(0.0f, (relevance - 1.0f) * 1000.0f));
        return static_cast<std::uint32_t>(weight);
    }
} // namespace ai
