// pi-lens-ignore-file: clang:pp_file_not_found,clang:unknown_typename,clang:use_of_undeclared_identifier,clang:unknown_type_name,clang:undeclared_var_use,clang:incomplete_member_access
#include "HireGroupAdapter.h"
#include "../runtime/HireLifecycle.h"

#include "Group/Group.h"

namespace TortoiseBots
{

HireGroupAdapter::HireGroupAdapter() : GroupScript("tortoisebots_hire_groups")
{
}

void HireGroupAdapter::OnRemoveMember(Group* group, ObjectGuid guid, uint8 /*method*/)
{
    HireLifecycle::Instance().OnGroupMemberRemoved(group, guid);
}

void HireGroupAdapter::OnDisband(Group* group)
{
    HireLifecycle::Instance().OnGroupDisband(group);
}

} // namespace TortoiseBots
