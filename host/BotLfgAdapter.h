#pragma once

// pi-lens-ignore: clang:pp_file_not_found
#include "ScriptObjects.h"

namespace TortoiseBots
{

// World tick driver for the bot LFG groups (runtime/BotLfgService); dormant
// unless AiPlayerbot.BotLfgGroups = 1 and AiPlayerbot.DungeonSelfClear = 1.
class BotLfgAdapter final : public WorldScript
{
public:
    BotLfgAdapter();

    void OnStartup() override;
    void OnUpdate(uint32 diff) override;
    void OnShutdown() override;
};

} // namespace TortoiseBots
