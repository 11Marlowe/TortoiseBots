
#include "playerbot/playerbot.h"
#include "ShamanPullStrategy.h"
#include "ShamanTankPolicy.h"

std::string ShamanPullStrategy::GetPullActionName() const
{
    PlayerbotAI* botAI = ai;

    // Tank branch only: a shaman tank opens a 2+ pull with Chain Lightning
    // and single pulls stay Lightning Bolt. The count prefers the
    // possible-targets value (CC-stripped pack ahead) and falls back to live
    // attackers (linked aggro by the time the pull cast runs). Both are
    // cached value reads — no world scan on the pull tick.
    if (botAI->HasStrategy("tank shaman", BotState::BOT_STATE_COMBAT) ||
        botAI->HasStrategy("tank", BotState::BOT_STATE_COMBAT))
    {
        AiObjectContext* context = botAI->GetAiObjectContext();
        uint8 pack = 0;
        if (context)
        {
            pack = context->GetValue<uint8>("possible attack targets count")->Get();
            if (pack < 2)
                pack = context->GetValue<uint8>("attackers count")->Get();
        }
        if (TankShamanPullWantsChainLightning(pack))
        {
            Unit* target = GetTarget();
            if (target && botAI->HasSpell("chain lightning") && botAI->CanCastSpell("chain lightning", target))
                return "chain lightning";
        }
    }

    return PullStrategy::GetPullActionName();
}

std::string ShamanPullStrategy::GetPreActionName() const
{
    return PullStrategy::GetPreActionName();
}
