#pragma once

#include <cstdint>
#include <string>

namespace TortoiseBots
{

// Issue #1003: a free, valid name for a new bot character of this race, class
// and gender (pool bots and hires). Passes the server's player-name checks
// (length, letters, profanity, reserved names) and is not taken right now.
// Empty only if nothing usable came up; callers retry or give up as before.
std::string PickBotCharacterName(uint8_t race, uint8_t cls, uint8_t gender);

// A hunter pet name from the [pet:hunter] pool that passes the pet-name checks;
// empty if none did.
std::string PickBotPetName();

} // namespace TortoiseBots
