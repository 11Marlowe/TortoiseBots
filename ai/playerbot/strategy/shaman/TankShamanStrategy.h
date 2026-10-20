#pragma once
#include "ShamanStrategy.h"

namespace ai
{
    class TankShamanPlaceholderStrategy : public SpecPlaceholderStrategy
    {
    public:
        TankShamanPlaceholderStrategy(PlayerbotAI* ai) : SpecPlaceholderStrategy(ai) {}
        int GetType() override { return STRATEGY_TYPE_TANK | STRATEGY_TYPE_MELEE; }
        std::string getName() override { return "tank shaman"; }
    };

    class TankShamanStrategy : public ShamanStrategy
    {
    public:
        TankShamanStrategy(PlayerbotAI* ai);
        int GetType() override { return STRATEGY_TYPE_TANK | STRATEGY_TYPE_MELEE; }
        uint32 GetType() const override { return STRATEGY_TYPE_TANK | STRATEGY_TYPE_MELEE; }

    protected:
        virtual void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;

        virtual NextAction** GetDefaultCombatActions() override;
    };

    class TankShamanPveStrategy : public TankShamanStrategy
    {
    public:
        TankShamanPveStrategy(PlayerbotAI* ai) : TankShamanStrategy(ai) {}
        std::string getName() override { return "tank shaman pve"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanPvpStrategy : public TankShamanStrategy
    {
    public:
        TankShamanPvpStrategy(PlayerbotAI* ai) : TankShamanStrategy(ai) {}
        std::string getName() override { return "tank shaman pvp"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanRaidStrategy : public TankShamanStrategy
    {
    public:
        TankShamanRaidStrategy(PlayerbotAI* ai) : TankShamanStrategy(ai) {}
        std::string getName() override { return "tank shaman raid"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanAoeStrategy : public ShamanAoeStrategy
    {
    public:
        TankShamanAoeStrategy(PlayerbotAI* ai) : ShamanAoeStrategy(ai) {}

    protected:
        virtual void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanAoePveStrategy : public TankShamanAoeStrategy
    {
    public:
        TankShamanAoePveStrategy(PlayerbotAI* ai) : TankShamanAoeStrategy(ai) {}
        std::string getName() override { return "aoe tank shaman pve"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanAoePvpStrategy : public TankShamanAoeStrategy
    {
    public:
        TankShamanAoePvpStrategy(PlayerbotAI* ai) : TankShamanAoeStrategy(ai) {}
        std::string getName() override { return "aoe tank shaman pvp"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanAoeRaidStrategy : public TankShamanAoeStrategy
    {
    public:
        TankShamanAoeRaidStrategy(PlayerbotAI* ai) : TankShamanAoeStrategy(ai) {}
        std::string getName() override { return "aoe tank shaman raid"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanTotemsStrategy : public ShamanTotemsStrategy
    {
    public:
        TankShamanTotemsStrategy(PlayerbotAI* ai) : ShamanTotemsStrategy(ai) {}

    protected:
        virtual void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanTotemsPveStrategy : public TankShamanTotemsStrategy
    {
    public:
        TankShamanTotemsPveStrategy(PlayerbotAI* ai) : TankShamanTotemsStrategy(ai) {}
        std::string getName() override { return "totems tank shaman pve"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanTotemsPvpStrategy : public TankShamanTotemsStrategy
    {
    public:
        TankShamanTotemsPvpStrategy(PlayerbotAI* ai) : TankShamanTotemsStrategy(ai) {}
        std::string getName() override { return "totems tank shaman pvp"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanTotemsRaidStrategy : public TankShamanTotemsStrategy
    {
    public:
        TankShamanTotemsRaidStrategy(PlayerbotAI* ai) : TankShamanTotemsStrategy(ai) {}
        std::string getName() override { return "totems tank shaman raid"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanBuffStrategy : public ShamanBuffStrategy
    {
    public:
        TankShamanBuffStrategy(PlayerbotAI* ai) : ShamanBuffStrategy(ai) {}

    protected:
        virtual void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanBuffPveStrategy : public TankShamanBuffStrategy
    {
    public:
        TankShamanBuffPveStrategy(PlayerbotAI* ai) : TankShamanBuffStrategy(ai) {}
        std::string getName() override { return "buff tank shaman pve"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanBuffPvpStrategy : public TankShamanBuffStrategy
    {
    public:
        TankShamanBuffPvpStrategy(PlayerbotAI* ai) : TankShamanBuffStrategy(ai) {}
        std::string getName() override { return "buff tank shaman pvp"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanBuffRaidStrategy : public TankShamanBuffStrategy
    {
    public:
        TankShamanBuffRaidStrategy(PlayerbotAI* ai) : TankShamanBuffStrategy(ai) {}
        std::string getName() override { return "buff tank shaman raid"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanBoostStrategy : public ShamanBoostStrategy
    {
    public:
        TankShamanBoostStrategy(PlayerbotAI* ai) : ShamanBoostStrategy(ai) {}

    protected:
        virtual void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanBoostPveStrategy : public TankShamanBoostStrategy
    {
    public:
        TankShamanBoostPveStrategy(PlayerbotAI* ai) : TankShamanBoostStrategy(ai) {}
        std::string getName() override { return "boost tank shaman pve"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanBoostPvpStrategy : public TankShamanBoostStrategy
    {
    public:
        TankShamanBoostPvpStrategy(PlayerbotAI* ai) : TankShamanBoostStrategy(ai) {}
        std::string getName() override { return "boost tank shaman pvp"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanBoostRaidStrategy : public TankShamanBoostStrategy
    {
    public:
        TankShamanBoostRaidStrategy(PlayerbotAI* ai) : TankShamanBoostStrategy(ai) {}
        std::string getName() override { return "boost tank shaman raid"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanCcStrategy : public ShamanCcStrategy
    {
    public:
        TankShamanCcStrategy(PlayerbotAI* ai) : ShamanCcStrategy(ai) {}

    protected:
        virtual void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanCcPveStrategy : public TankShamanCcStrategy
    {
    public:
        TankShamanCcPveStrategy(PlayerbotAI* ai) : TankShamanCcStrategy(ai) {}
        std::string getName() override { return "cc tank shaman pve"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanCcPvpStrategy : public TankShamanCcStrategy
    {
    public:
        TankShamanCcPvpStrategy(PlayerbotAI* ai) : TankShamanCcStrategy(ai) {}
        std::string getName() override { return "cc tank shaman pvp"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanCcRaidStrategy : public TankShamanCcStrategy
    {
    public:
        TankShamanCcRaidStrategy(PlayerbotAI* ai) : TankShamanCcStrategy(ai) {}
        std::string getName() override { return "cc tank shaman raid"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // Cure rows come entirely from the ShamanCureStrategy base (poison /
    // disease on self and party): the tank keeps the class cure kit.
    class TankShamanCureStrategy : public ShamanCureStrategy
    {
    public:
        TankShamanCureStrategy(PlayerbotAI* ai) : ShamanCureStrategy(ai) {}

    protected:
        virtual void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        virtual void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanCurePveStrategy : public TankShamanCureStrategy
    {
    public:
        TankShamanCurePveStrategy(PlayerbotAI* ai) : TankShamanCureStrategy(ai) {}
        std::string getName() override { return "cure tank shaman pve"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanCurePvpStrategy : public TankShamanCureStrategy
    {
    public:
        TankShamanCurePvpStrategy(PlayerbotAI* ai) : TankShamanCureStrategy(ai) {}
        std::string getName() override { return "cure tank shaman pvp"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class TankShamanCureRaidStrategy : public TankShamanCureStrategy
    {
    public:
        TankShamanCureRaidStrategy(PlayerbotAI* ai) : TankShamanCureStrategy(ai) {}
        std::string getName() override { return "cure tank shaman raid"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };
}
