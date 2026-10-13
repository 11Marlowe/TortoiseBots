#include "../ai/playerbot/RpgRelevanceWeightPolicy.h"
#include "../ai/playerbot/WeightedShufflePolicy.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

int main()
{
    // 1. Relevance 1 to 2 keeps the old weights: 0, 500 and 1000.
    CHECK(ai::RpgRelevanceWeight(1.0f) == 0);
    CHECK(ai::RpgRelevanceWeight(1.5f) == 500);
    CHECK(ai::RpgRelevanceWeight(2.0f) == 1000);
    std::cout << "  [PASS] relevance 1 to 2 keeps the weights 0 to 1000\n";

    // 2. Relevance below 1 is weight 0, never a wrapped uint32 (the old conversion gave about 4294966796 for 0.5).
    CHECK(ai::RpgRelevanceWeight(0.0f) == 0);
    CHECK(ai::RpgRelevanceWeight(0.5f) == 0);
    CHECK(ai::RpgRelevanceWeight(0.999f) == 0);
    std::cout << "  [PASS] relevance below 1 is weight 0\n";

    // 3. In the weighted pick, an action below relevance 1 never comes first while an action above 1 is available.
    std::mt19937 gen(20261013u);
    std::vector<float> const relevances = {0.0f, 0.5f, 1.5f, 2.0f};
    int const trials = 20000;
    int low_first = 0;
    for (int t = 0; t < trials; ++t)
    {
        std::vector<int> values = {0, 1, 2, 3};
        std::vector<std::uint32_t> weights;
        for (float relevance : relevances)
            weights.push_back(ai::RpgRelevanceWeight(relevance));
        ai::WeightedShuffle(values.begin(), values.end(), weights.begin(), weights.end(), gen);
        if (values[0] == 0 || values[0] == 1)
            ++low_first;
    }
    CHECK(low_first == 0);
    std::cout << "  [PASS] relevance below 1 never wins the weighted pick\n";

    std::cout << "All RPG relevance weight policy tests passed.\n";
    return 0;
}
