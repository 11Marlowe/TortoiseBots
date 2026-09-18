#include "Module.h"

// pi-lens-ignore: clang:pp_file_not_found
#include "BotChatAdapter.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "BotAddonAdapter.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "BotHostAdapter.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "LftFillAdapter.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "BotPacketAdapter.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "BotPlayerAdapter.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "HireRecruiterAdapter.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "HireGroupAdapter.h"

namespace TortoiseBots {

void RegisterScripts()
{
    new BotHostAdapter();
    new LftFillAdapter();
    new BotPacketAdapter();
    new BotPlayerAdapter();
    new BotUnitAdapter();
    new BotChatAdapter();
    new BotAddonAdapter();
    // Issue #192: on-demand companion hiring (module-only gossip + group hooks).
    new HireRecruiterAdapter();
    new HireGroupAdapter();
}

} // namespace TortoiseBots
