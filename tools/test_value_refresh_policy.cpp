#include "../ai/playerbot/strategy/ValueRefreshPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::ValueNeedsRefresh;
using ai::ValueRefreshSeconds;

namespace
{
    // The pre-toggle formula, copied verbatim from CalculatedValue::Refresh.
    bool OldFormula(std::time_t now, std::time_t lastCheckTime, int checkInterval)
    {
        return !lastCheckTime || (checkInterval < 2 && (now - lastCheckTime > 0.1)) || now - lastCheckTime >= checkInterval / 2;
    }
}

int main()
{
    std::time_t const now = 1'000'000;

    // Toggle off is the old formula exactly, for every interval and age,
    // whether or not the value is marked keep-legacy.
    for (int interval = 0; interval <= 130; ++interval)
        for (int age = 0; age <= 200; ++age)
            for (bool keep : { false, true })
            {
                CHECK(ValueNeedsRefresh(now, now - age, interval, false, keep) == OldFormula(now, now - age, interval));
                CHECK(ValueRefreshSeconds(interval, false, keep) == interval / 2);
            }

    // Never calculated: always refresh, under either rule.
    for (int interval : { 0, 1, 2, 5 })
        for (bool on : { false, true })
            CHECK(ValueNeedsRefresh(now, 0, interval, on, false));

    // Old rule: 0 and 1 recalculate on every read, 2 after 1 s, 5 after 2 s.
    CHECK(ValueNeedsRefresh(now, now, 0, false, false));
    CHECK(ValueNeedsRefresh(now, now, 1, false, false));
    CHECK(!ValueNeedsRefresh(now, now, 2, false, false));
    CHECK(ValueNeedsRefresh(now, now - 1, 2, false, false));
    CHECK(!ValueNeedsRefresh(now, now - 1, 5, false, false));
    CHECK(ValueNeedsRefresh(now, now - 2, 5, false, false));

    // Toggle on: 0 and 1 still recalculate on every read.
    CHECK(ValueNeedsRefresh(now, now, 0, true, false));
    CHECK(ValueNeedsRefresh(now, now, 1, true, false));
    CHECK(ValueRefreshSeconds(0, true, false) == 0);
    CHECK(ValueRefreshSeconds(1, true, false) == 0);

    // Toggle on: 2 and 5 wait their full interval.
    CHECK(!ValueNeedsRefresh(now, now - 1, 2, true, false));
    CHECK(ValueNeedsRefresh(now, now - 2, 2, true, false));
    CHECK(!ValueNeedsRefresh(now, now - 2, 5, true, false));
    CHECK(!ValueNeedsRefresh(now, now - 4, 5, true, false));
    CHECK(ValueNeedsRefresh(now, now - 5, 5, true, false));
    CHECK(ValueRefreshSeconds(2, true, false) == 2);
    CHECK(ValueRefreshSeconds(5, true, false) == 5);

    // Toggle on, fight-sensing value (keep-legacy): the old rule stays.
    for (int interval = 0; interval <= 130; ++interval)
        for (int age = 0; age <= 200; ++age)
            CHECK(ValueNeedsRefresh(now, now - age, interval, true, true) == OldFormula(now, now - age, interval));
    CHECK(ValueNeedsRefresh(now, now - 1, 2, true, true));
    CHECK(ValueNeedsRefresh(now, now - 2, 5, true, true));

    // The constructor seed (one period back, keepLegacy false) makes the
    // first read calculate at once under both rules, for a marked value too.
    for (int interval = 0; interval <= 130; ++interval)
        for (bool on : { false, true })
        {
            std::time_t const seed = now - ValueRefreshSeconds(interval, on, false);
            CHECK(ValueNeedsRefresh(now, seed, interval, on, false));
            CHECK(ValueNeedsRefresh(now, seed, interval, on, true));
        }

    // The runtime switch starts off.
    CHECK(!ai::ValueRefreshAtIntervalEnabled());
    ai::gValueRefreshAtInterval.store(true);
    CHECK(ai::ValueRefreshAtIntervalEnabled());

    std::cout << "value refresh policy: OK\n";
    return 0;
}
