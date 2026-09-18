#pragma once

// pi-lens-ignore: clang:pp_file_not_found
#include "ScriptObjects.h"

namespace TortoiseBots
{

// Module-owned GroupScript adapter for hired-companion dismissal. Native
// group transitions stay core-owned; this only releases hire bookkeeping when
// a companion leaves or its group disbands.
class HireGroupAdapter final : public GroupScript
{
public:
    HireGroupAdapter();

    void OnRemoveMember(Group* group, ObjectGuid guid, uint8 method) override;
    void OnDisband(Group* group) override;
};

} // namespace TortoiseBots
