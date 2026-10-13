#include "../ai/playerbot/WeightedShufflePolicy.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

// The shuffle this replaces (WorldPosition.h before the change), kept as the reference for the distribution check.
template <class D, class W, class URBG>
void LegacyWeightedShuffle(D first, D last, W first_weight, W last_weight, URBG&& g)
{
    while (first != last && first_weight != last_weight)
    {
        std::discrete_distribution<int> dd(first_weight, last_weight);
        auto i = dd(g);

        if (i)
        {
            std::swap(*first, *std::next(first, i));
            std::swap(*first_weight, *std::next(first_weight, i));
        }
        ++first;
        ++first_weight;
    }
}

namespace
{
    struct PolicyShuffle
    {
        template <class D, class W, class G>
        void operator()(D a, D b, W c, W d, G& g) const { ai::WeightedShuffle(a, b, c, d, g); }
    };

    struct LegacyShuffle
    {
        template <class D, class W, class G>
        void operator()(D a, D b, W c, W d, G& g) const { LegacyWeightedShuffle(a, b, c, d, g); }
    };

    // Counts how often each order of 0..n-1 comes out of `shuffle` over `trials` runs.
    template <class Shuffle>
    std::map<std::vector<int>, int> CountOrders(std::vector<int> const& weights, int trials, Shuffle shuffle, std::mt19937& gen)
    {
        std::map<std::vector<int>, int> counts;
        for (int t = 0; t < trials; ++t)
        {
            std::vector<int> values(weights.size());
            for (std::size_t i = 0; i < values.size(); ++i)
                values[i] = static_cast<int>(i);
            std::vector<int> w = weights;
            shuffle(values.begin(), values.end(), w.begin(), w.end(), gen);
            counts[values] += 1;
        }
        return counts;
    }
}

int main()
{
    // Fixed seed: the checks below are deterministic, the tolerances only have to hold for this seed.
    std::mt19937 gen(20261013u);

    // 1. The first item drawn follows weight / total, as successive sampling does.
    {
        std::vector<int> const weights = {1, 2, 3, 4};
        int const trials = 200000;
        std::vector<int> first_count(weights.size(), 0);
        for (int t = 0; t < trials; ++t)
        {
            std::vector<int> values = {0, 1, 2, 3};
            std::vector<int> w = weights;
            ai::WeightedShuffle(values.begin(), values.end(), w.begin(), w.end(), gen);
            first_count[values[0]] += 1;
        }
        for (std::size_t i = 0; i < weights.size(); ++i)
        {
            double const expect = weights[i] / 10.0;
            double const got = static_cast<double>(first_count[i]) / trials;
            CHECK(std::fabs(got - expect) < 0.008);
        }
        std::cout << "  [PASS] first pick follows weight / total\n";
    }

    // 2. Every order of four items comes out as often as with the legacy shuffle.
    {
        std::vector<int> const weights = {5, 1, 3, 2};
        int const trials = 200000;
        auto fast = CountOrders(weights, trials, PolicyShuffle(), gen);
        auto legacy = CountOrders(weights, trials, LegacyShuffle(), gen);
        CHECK(fast.size() == 24);
        CHECK(legacy.size() == 24);
        for (auto const& entry : legacy)
        {
            auto const found = fast.find(entry.first);
            CHECK(found != fast.end());
            double const p = static_cast<double>(entry.second) / trials;
            double const q = static_cast<double>(found->second) / trials;
            CHECK(std::fabs(p - q) < 0.008);
        }
        std::cout << "  [PASS] all 24 orders match the legacy distribution\n";
    }

    // 3. A weight stays with its item.
    {
        std::vector<int> values(50);
        std::vector<int> weights(50);
        for (int i = 0; i < 50; ++i)
        {
            values[i] = i;
            weights[i] = 1 + (i * 7) % 13;
        }
        std::vector<int> const original = weights;
        ai::WeightedShuffle(values.begin(), values.end(), weights.begin(), weights.end(), gen);
        for (std::size_t k = 0; k < values.size(); ++k)
            CHECK(weights[k] == original[values[k]]);
        std::cout << "  [PASS] weights move with their items\n";
    }

    // 4. Zero, negative, NaN and infinite weights come after every positive weight, in input order.
    {
        std::vector<int> values = {0, 1, 2, 3, 4};
        std::vector<double> weights = {0.0, 2.0, -1.0, 0.0, std::numeric_limits<double>::quiet_NaN()};
        ai::WeightedShuffle(values.begin(), values.end(), weights.begin(), weights.end(), gen);
        CHECK((values == std::vector<int>{1, 0, 2, 3, 4}));

        std::vector<int> pair_values = {0, 1, 2, 3};
        std::vector<double> pair_weights = {0.0, 3.0, std::numeric_limits<double>::infinity(), 1.0};
        ai::WeightedShuffle(pair_values.begin(), pair_values.end(), pair_weights.begin(), pair_weights.end(), gen);
        CHECK(pair_values[2] == 0 && pair_values[3] == 2);
        CHECK((pair_values[0] == 1 && pair_values[1] == 3) || (pair_values[0] == 3 && pair_values[1] == 1));
        std::cout << "  [PASS] zero and invalid weights go last\n";
    }

    // 5. No positive weight at all: the order is left as it was.
    {
        std::vector<int> values = {0, 1, 2, 3, 4};
        std::vector<int> weights = {0, 0, 0, 0, 0};
        ai::WeightedShuffle(values.begin(), values.end(), weights.begin(), weights.end(), gen);
        CHECK((values == std::vector<int>{0, 1, 2, 3, 4}));
        std::cout << "  [PASS] all-zero weights keep the input order\n";
    }

    // 6. Empty, single and mismatched ranges: only the paired prefix is shuffled.
    {
        std::vector<int> none;
        std::vector<double> no_weights;
        ai::WeightedShuffle(none.begin(), none.end(), no_weights.begin(), no_weights.end(), gen);
        CHECK(none.empty());

        std::vector<int> one = {7};
        std::vector<double> one_weight = {0.5};
        ai::WeightedShuffle(one.begin(), one.end(), one_weight.begin(), one_weight.end(), gen);
        CHECK(one[0] == 7);

        std::vector<int> values = {0, 1, 2, 3, 4, 5};
        std::vector<double> weights = {1.0, 1.0, 1.0};
        ai::WeightedShuffle(values.begin(), values.end(), weights.begin(), weights.end(), gen);
        CHECK(values[3] == 3 && values[4] == 4 && values[5] == 5);
        CHECK(values[0] + values[1] + values[2] == 3);
        std::cout << "  [PASS] empty, single and mismatched ranges\n";
    }

    // 7. A std::list of strings with float weights: names and weights still move together.
    {
        std::list<std::string> names = {"a", "b", "c", "d"};
        std::vector<float> weights = {4.0f, 3.0f, 2.0f, 1.0f};
        std::map<std::string, float> weight_of = {{"a", 4.0f}, {"b", 3.0f}, {"c", 2.0f}, {"d", 1.0f}};
        ai::WeightedShuffle(names.begin(), names.end(), weights.begin(), weights.end(), gen);
        std::size_t k = 0;
        for (auto const& name : names)
        {
            CHECK(weights[k] == weight_of[name]);
            ++k;
        }
        CHECK(k == 4);
        std::cout << "  [PASS] list of strings with float weights\n";
    }

    std::cout << "All weighted shuffle policy tests passed.\n";
    return 0;
}
