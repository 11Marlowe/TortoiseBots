#pragma once

// Weighted shuffle as a pure policy (no core includes), so tools/test_weighted_shuffle_policy.cpp and
// tools/bench_weighted_shuffle.cpp compile it standalone. WorldPosition.h includes it.
//
// WeightedShuffle orders [first, last) by successive sampling without replacement: each next item is drawn with a
// probability proportional to its weight among the items not drawn yet, and every weight moves with its item. The
// previous version rebuilt a std::discrete_distribution for every draw, O(n^2). This one draws one exponential clock
// per item, E = -ln(U) / w with U uniform in (0, 1], and sorts the items by E (the exponential-clock form of
// Efraimidis and Spirakis' weighted sampling without replacement): the same order in O(n log n).
//
// An item whose weight is not a positive finite number (zero, negative, NaN, infinity) comes after every positive
// item, in input order. That is what the old code produced: a zero-weight tail always drew its first remaining item.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <random>
#include <utility>
#include <vector>

namespace ai
{
    template <class D, class W, class URBG>
    inline void WeightedShuffle(D first, D last, W first_weight, W last_weight, URBG&& g)
    {
        using Value = typename std::iterator_traits<D>::value_type;
        using Weight = typename std::iterator_traits<W>::value_type;

        // Copy the paired ranges out, then write them back in the new order.
        std::vector<Value> values;
        std::vector<Weight> weights;
        W weight_it = first_weight;
        for (D item = first; item != last && weight_it != last_weight; ++item, ++weight_it)
        {
            values.push_back(std::move(*item));
            weights.push_back(*weight_it);
        }
        std::size_t const count = values.size();

        std::uniform_real_distribution<double> unit(0.0, 1.0);
        std::vector<std::pair<double, std::size_t>> clocks;
        std::vector<std::size_t> tail;
        clocks.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            double const w = static_cast<double>(weights[i]);
            if (!(w > 0.0) || !std::isfinite(w))
            {
                tail.push_back(i);
                continue;
            }

            double const u = 1.0 - unit(g); // in (0, 1], so log(u) is finite
            clocks.emplace_back(-std::log(u) / w, i);
        }
        std::sort(clocks.begin(), clocks.end());

        D out = first;
        W out_weight = first_weight;
        auto place = [&](std::size_t i)
        {
            *out = std::move(values[i]);
            *out_weight = weights[i];
            ++out;
            ++out_weight;
        };
        for (auto const& entry : clocks)
            place(entry.second);
        for (std::size_t i : tail)
            place(i);
    }
} // namespace ai
