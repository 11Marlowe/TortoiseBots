#pragma once

// One gate for the items a bot must never receive: a transmog clone copy, an
// id the core transmog map answers for (a clone shadows a same-id real
// prototype, so the id itself is poisoned), or a duplicate-copy row
// (GearDuplicateItemPolicy). The facts are gathered by the caller, so this
// header stays free of core and database types (policy-header convention).
// Exercised standalone in tools/test_transmog_clone_policy.cpp.

namespace TortoiseBots
{

struct TransmogCloneFacts
{
    bool destItemSet = false;
    bool idIsCloneId = false;
    bool duplicateCopy = false;
};

inline bool IsTransmogClone(TransmogCloneFacts const& facts)
{
    return facts.destItemSet || facts.idIsCloneId || facts.duplicateCopy;
}

} // namespace TortoiseBots
