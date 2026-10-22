#pragma once

// Owner rule: no item created by transmogrification may ever enter a pool a
// bot receives items from. Turtle's transmogrification clones an item: the
// core copies the base prototype, gives the copy a new id (production ids
// ~1.2M-2M), marks it (DestItemId set, soulbound) and answers
// sObjectMgr.GetItemPrototype with the clone BEFORE the real item storage —
// so any module loop over item ids can hand a bot a clone. The world DB
// snapshot carries the same idea as duplicate-copy rows (see
// GearDuplicateItemPolicy.h; that rule is reused here, not duplicated).
//
// One gate, three conditions, ANY of which excludes an item:
//  * destItemSet   — the prototype in hand is a clone copy (DestItemId set);
//  * idIsCloneId   — the id resolves through the core transmog map. A clone
//    also shadows a real prototype of the same numeric id, so the id itself
//    is poisoned for bots, not just the clone proto;
//  * duplicateCopy — the duplicate-copy rule flags it (the index is owned by
//    RandomItemMgr, built once in its ascending item walk).
//
// The facts are gathered by the caller, so this header stays free of core
// and database types (policy-header convention). RandomItemMgr::IsTransmogClone
// is the composed one-call gate every call site uses. Exercised standalone in
// tools/test_transmog_clone_policy.cpp.

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
