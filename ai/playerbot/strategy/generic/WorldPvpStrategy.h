#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class WorldPvpStrategy : public Strategy
    {
    public:
        WorldPvpStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "world pvp"; }
#ifdef GenerateBotHelp
        virtual std::string GetHelpName() { return "world pvp"; } //Must equal iternal name
        virtual std::string GetHelpDescription() {
            return "This strategy makes the bot seek PvP-flagged enemy players in the open world and attack them.";
        }
        virtual std::vector<std::string> GetRelatedStrategies() { return { "pvp" }; }
#endif
    private:
        void InitCombatTriggers(std::list<TriggerNode*> &triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*> &triggers) override;
    };
}
