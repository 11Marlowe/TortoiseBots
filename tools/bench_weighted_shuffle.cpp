// Micro-benchmark: ai/playerbot/WeightedShufflePolicy.h against the shuffle it replaces (a copy of the WorldPosition.h
// template as it was at 66fc42b9). Not part of verify_all.sh: the timings are for the PR, not a gate.
//   g++ -std=c++17 -O2 tools/bench_weighted_shuffle.cpp -o bench && ./bench
// Each timed call also resets the input (values and weights), the same work for both shuffles.

#include "../ai/playerbot/WeightedShufflePolicy.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <random>
#include <utility>
#include <vector>

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

// Runs fn until at least min_seconds have passed (and at least 3 times); returns the mean microseconds per call.
template <class Fn>
double MicrosPerCall(Fn fn, double min_seconds)
{
    using Clock = std::chrono::steady_clock;
    auto const start = Clock::now();
    int calls = 0;
    double elapsed = 0.0;
    do
    {
        fn();
        ++calls;
        elapsed = std::chrono::duration<double>(Clock::now() - start).count();
    } while (elapsed < min_seconds || calls < 3);
    return elapsed * 1e6 / calls;
}

int main()
{
    std::mt19937 gen(12345u);
    unsigned long long checksum = 0;

    std::printf("%6s %18s %18s %10s\n", "n", "legacy us/call", "policy us/call", "speedup");
    for (std::size_t const n : {std::size_t(100), std::size_t(1000), std::size_t(5000)})
    {
        // Weights shaped like the WorldPosition callers: 200000 / (1 + distance), distance 0..3000 yards.
        std::vector<std::uint32_t> base(n);
        std::uniform_int_distribution<int> dist_yards(0, 3000);
        for (auto& w : base)
            w = 200000u / (1u + static_cast<std::uint32_t>(dist_yards(gen)));

        std::vector<std::size_t> values(n);
        std::vector<std::uint32_t> weights(n);
        auto reset = [&]()
        {
            for (std::size_t i = 0; i < n; ++i)
                values[i] = i;
            weights = base;
        };

        double const legacy = MicrosPerCall([&]()
        {
            reset();
            LegacyWeightedShuffle(values.begin(), values.end(), weights.begin(), weights.end(), gen);
            checksum += values[0];
        }, 0.5);

        double const policy = MicrosPerCall([&]()
        {
            reset();
            ai::WeightedShuffle(values.begin(), values.end(), weights.begin(), weights.end(), gen);
            checksum += values[0];
        }, 0.5);

        std::printf("%6zu %18.2f %18.2f %9.1fx\n", n, legacy, policy, legacy / policy);
    }
    std::printf("checksum %llu\n", checksum);
    return 0;
}
