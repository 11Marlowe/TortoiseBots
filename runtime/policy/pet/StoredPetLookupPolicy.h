#pragma once

#include <cstdint>
#include <ctime>

// Throttle for the synchronous character_pet lookup in
// InitializePetAction::isUseful (ai/playerbot/strategy/actions/
// GenericActions.cpp). Without it a hunter with a stored but not-live pet
// (dismissed, dead, stabled) re-queries the character DB on every firing of
// the "often" trigger, forever: the row is found, GetPet() is null and the
// action answers "not useful" without anything changing (issue #1178).
//
// The cache answers from memory and re-queries only when
//   - the bot's live-pet presence flipped since the previous check (tame,
//     summon, dismiss, death all show up as a GetPet() change), or
//   - kRecheckSeconds passed, whichever comes first.
// The time cap also covers row changes the presence flip cannot see, e.g.
// abandoning a pet from a stable slot, so a hunter that really needs a pet
// initialised is never locked out by more than kRecheckSeconds.
//
// One instance lives on the action, which AiObjectContext keeps for the
// bot's session (NamedObjectContext::Create caches it), so the state is
// per bot and dies with its session. No core includes: exercised standalone
// by tools/test_stored_pet_lookup_policy.cpp.

namespace TortoiseBots
{

class StoredPetLookupCache
{
public:
    // At most one character_pet query per this many seconds per bot when no
    // state flip forces a refresh.
    static constexpr time_t kRecheckSeconds = 60;

    // Call on every isUseful run with the bot's current live-pet presence.
    // Returns true when the caller must run the synchronous character_pet
    // query to refresh the stored-pet answer; otherwise CachedHasStoredPet()
    // holds the last answer.
    bool ShouldQuery(bool hasLivePet, time_t now)
    {
        if (hasLivePet != lastHasLivePet)
        {
            lastHasLivePet = hasLivePet;
            lastQueryTime = 0; // re-answer on the next no-live-pet check
        }
        // A clock set backwards (NTP correction) treats every later diff as
        // negative; re-query until real time catches up, which is the
        // pre-fix behaviour for that pathological case.
        return !lastQueryTime || now < lastQueryTime || now - lastQueryTime >= kRecheckSeconds;
    }

    bool CachedHasStoredPet() const { return cachedHasStoredPet; }

    void Store(time_t now, bool hasStoredPet)
    {
        lastQueryTime = now;
        cachedHasStoredPet = hasStoredPet;
    }

    void Reset()
    {
        lastQueryTime = 0;
        lastHasLivePet = false;
        cachedHasStoredPet = false;
    }

private:
    time_t lastQueryTime = 0;
    bool lastHasLivePet = false;
    bool cachedHasStoredPet = false;
};

} // namespace TortoiseBots
