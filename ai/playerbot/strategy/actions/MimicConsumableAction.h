#pragma once

#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/Action.h"
#include "UseItemAction.h"
#include "runtime/MimicConsumablePolicy.h"

namespace ai
{
    // Mimic the master's consumable: read the master's CMSG_USE_ITEM packet,
    // resolve the class/spec/level-appropriate equivalent and cast it
    // virtually (no inventory item needed). Gated behind the "mimic
    // consumables" strategy (`.bot behavior <bot> mimic on|off`, off by
    // default), out-of-combat only. Raid anti-spam: with more than 4 bots on
    // the master the action stays silent; the master's summary (see
    // BotCommands mimic summary) covers the feedback instead.
    class MimicConsumableAction : public Action
    {
    public:
        MimicConsumableAction(PlayerbotAI* ai) : Action(ai, "mimic consumable") {}

        bool Execute(Event& event) override;
        bool isPossible() override;
        bool isUseful() override;

        // Raid anti-spam cutoff: at most this many bots on one master may
        // whisper; above it every bot stays silent.
        static constexpr uint32_t kMaxWhisperBots = 4;

    private:
        uint32_t ResolveBotItem(TortoiseBots::MimicPurpose purpose, uint32_t masterItemId,
            TortoiseBots::MimicResolution const& resolution);
        bool ApplyWeaponImbue(uint32_t stoneItemId);
        bool CastMimicItem(uint32_t botItemId, bool withEatEmote);
        void RemoveConflictingAura(TortoiseBots::MimicPurpose purpose, uint32_t botItemId);
        void Feedback(char const* itemName, bool silent);
    };
}
