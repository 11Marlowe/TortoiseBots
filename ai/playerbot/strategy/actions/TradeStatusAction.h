#pragma once
#include "playerbot/PlayerbotAI.h"
#include "QueryItemUsageAction.h"

namespace ai
{
    class TradeStatusAction : public QueryItemUsageAction
    {
    public:
        TradeStatusAction(PlayerbotAI* ai) : QueryItemUsageAction(ai, "accept trade") {}
        virtual bool Execute(Event& event) override;

    private:
        void BeginTrade();
        void AutoShareConjured(Player* trader);
        bool CheckTrade();
        int32 CalculateCost(Player *player, bool sell);
    };
}
