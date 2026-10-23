// Standalone regression test for the character_pet lookup throttle
// (issue #1178, runtime/StoredPetLookupPolicy.h): a hunter with a stored but
// not-live pet (dismissed, dead, stabled) used to re-query the character DB on
// every firing of the "often" trigger, forever. The cache must answer from
// memory and re-query only on a live-pet presence flip or after the 60 s cap.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_stored_pet_lookup_policy.cpp -o /tmp/test_stored_pet_lookup
//   /tmp/test_stored_pet_lookup

#include "../runtime/policy/pet/StoredPetLookupPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// Fresh cache: the very first check must run the query.
static void TestFirstCheckQueries()
{
    StoredPetLookupCache cache;
    CHECK(cache.ShouldQuery(false, 1000));
    cache.Store(1000, true);
    CHECK(cache.CachedHasStoredPet());
}

// Steady state ("often" trigger fires every 5 ticks): one query, then memory
// answers for the whole 60 s window, then the cap forces a refresh.
static void TestSteadyStateQueriesOncePerInterval()
{
    StoredPetLookupCache cache;
    CHECK(cache.ShouldQuery(false, 1000));
    cache.Store(1000, true);

    for (time_t t = 1005; t < 1000 + StoredPetLookupCache::kRecheckSeconds; t += 5)
    {
        CHECK(!cache.ShouldQuery(false, t));
        CHECK(cache.CachedHasStoredPet());
    }

    CHECK(cache.ShouldQuery(false, 1000 + StoredPetLookupCache::kRecheckSeconds));
    // A refresh that still finds the row keeps answering true from memory.
    cache.Store(1000 + StoredPetLookupCache::kRecheckSeconds, true);
    CHECK(!cache.ShouldQuery(false, 1000 + StoredPetLookupCache::kRecheckSeconds + 5));
}

// Same with a "no stored pet" answer (level < 10 pet removed, empty row set).
static void TestNoStoredPetAnswerCachedToo()
{
    StoredPetLookupCache cache;
    CHECK(cache.ShouldQuery(false, 500));
    cache.Store(500, false);
    CHECK(!cache.CachedHasStoredPet());
    CHECK(!cache.ShouldQuery(false, 555));
    CHECK(!cache.CachedHasStoredPet());
}

// A live-pet presence flip (tame/summon/dismiss/death) forces a re-answer on
// the next no-live-pet check, even inside the 60 s window.
static void TestPresenceFlipForcesRequery()
{
    StoredPetLookupCache cache;
    CHECK(cache.ShouldQuery(false, 1000));
    cache.Store(1000, true);
    CHECK(!cache.ShouldQuery(false, 1030));

    // Pet summoned: the caller passes the flip in and ignores the answer,
    // the query is never reached with a live pet.
    (void)cache.ShouldQuery(true, 1031);

    // Pet dismissed 10 s later: must re-query at once, not at t+60.
    CHECK(cache.ShouldQuery(false, 1041));

    // Flip back and forth between two checks (summoned and dismissed again
    // within one trigger period): still re-answers.
    cache.Store(1041, true);
    (void)cache.ShouldQuery(true, 1042);
    CHECK(cache.ShouldQuery(false, 1043));
}

// A clock set backwards (NTP correction) must not stall the lookups: the
// cache re-queries until real time catches up.
static void TestClockBackwardsQueries()
{
    StoredPetLookupCache cache;
    CHECK(cache.ShouldQuery(false, 5000));
    cache.Store(5000, true);
    CHECK(!cache.ShouldQuery(false, 5030));
    CHECK(cache.ShouldQuery(false, 4900)); // jumped back two minutes
    cache.Store(4900, false);
    CHECK(!cache.ShouldQuery(false, 4905));
    CHECK(cache.ShouldQuery(false, 4900 + StoredPetLookupCache::kRecheckSeconds));
}

static void TestReset()
{
    StoredPetLookupCache cache;
    CHECK(cache.ShouldQuery(false, 1000));
    cache.Store(1000, true);
    cache.Reset();
    CHECK(cache.ShouldQuery(false, 1001));
    CHECK(!cache.CachedHasStoredPet());
}

int main()
{
    TestFirstCheckQueries();
    TestSteadyStateQueriesOncePerInterval();
    TestNoStoredPetAnswerCachedToo();
    TestPresenceFlipForcesRequery();
    TestClockBackwardsQueries();
    TestReset();
    std::printf("ok: %d checks\n", checks);
    return 0;
}
