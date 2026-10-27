#include "BotLfgAdapter.h"

#include "../runtime/BotLfgService.h"

namespace TortoiseBots {

BotLfgAdapter::BotLfgAdapter()
    : WorldScript("tortoisebots_bot_lfg", { WORLDHOOK_ON_STARTUP, WORLDHOOK_ON_UPDATE, WORLDHOOK_ON_SHUTDOWN })
{
}

void BotLfgAdapter::OnStartup()
{
    BotLfgService::Instance().Initialize();
}

void BotLfgAdapter::OnUpdate(uint32 diff)
{
    BotLfgService::Instance().Update(diff);
}

void BotLfgAdapter::OnShutdown()
{
    BotLfgService::Instance().Shutdown();
}

} // namespace TortoiseBots
