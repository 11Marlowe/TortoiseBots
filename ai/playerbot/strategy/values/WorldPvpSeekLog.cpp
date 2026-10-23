
#include "playerbot/playerbot.h"
#include "playerbot/ServerFacade.h"
#include "../../../runtime/PlayerbotAIStorage.h"
#include "../../../runtime/ObservabilityEmitter.h"
#include "WorldPvpSeekLog.h"

using namespace ai;

void WorldPvpSeekLog::LogGrant(PlayerbotAI* ai, Player* bot)
{
    if (!ai || !bot || !sPlayerbotAIConfig.hasLog("pvp_seek.csv"))
        return;

    std::ostringstream out;
    out << sPlayerbotAIConfig.GetTimestampStr() << "+00," << bot->GetName() << ",grant,,"
        << bot->GetLevel() << "," << bot->GetMapId() << "," << sServerFacade.GetAreaId(bot);
    sPlayerbotAIConfig.log("pvp_seek.csv", out.str().c_str());

    sObservabilityEmitter.EmitBotActivity("WorldPvpSeekGrant", "", "", bot);
}

void WorldPvpSeekLog::LogEngagement(PlayerbotAI* ai, Player* bot, Unit* target)
{
    if (!ai || !bot || !target)
        return;

    Player* enemy = dynamic_cast<Player*>(target);
    if (sPlayerbotAIConfig.hasLog("pvp_seek.csv"))
    {
        std::ostringstream out;
        out << sPlayerbotAIConfig.GetTimestampStr() << "+00," << bot->GetName() << ",engage," << target->GetName() << ","
            << target->GetLevel() << "," << target->GetMapId() << "," << sServerFacade.GetAreaId(target);
        sPlayerbotAIConfig.log("pvp_seek.csv", out.str().c_str());
    }

    sObservabilityEmitter.EmitBotActivity("WorldPvpEngage", target->GetName(),
        enemy && PlayerbotAIStorage::Instance().GetAI(enemy) ? "bot" : "player", bot);
}
