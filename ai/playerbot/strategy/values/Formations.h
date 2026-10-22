#pragma once
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/Value.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/strategy/actions/GenericActions.h"

namespace ai
{
    class Formation : public AiNamedObject
    {
    public:
        Formation(PlayerbotAI* ai, std::string name) : AiNamedObject (ai, name), serial(++lastSerial) {}

    public:
        virtual std::string GetTargetName() { return ""; }
        virtual WorldLocation GetLocation() { return NullLocation; }
        virtual float GetAngle();
        virtual float GetOffset();
        virtual float getOffset() { return GetOffset(); }
        virtual float GetMaxDistance();
        static WorldLocation NullLocation;
		static bool IsNullLocation(WorldLocation const& loc);
        // Per object, not per name: .bot formation replaces the object on every
        // set, even with the same name, so the station trigger keys on this.
        unsigned int GetSerial() const { return serial; }

    protected:
        float GetFollowAngle();

    private:
        static unsigned int lastSerial;
        unsigned int const serial;
    };

    class FollowFormation : public Formation
    {
    public:
        FollowFormation(PlayerbotAI* ai, std::string name) : Formation(ai, name) {}
        virtual WorldLocation GetLocation();
    };

    class MoveFormation : public Formation
    {
    public:
        MoveFormation(PlayerbotAI* ai, std::string name) : Formation(ai, name) {}

    protected:
        WorldLocation MoveLine(std::vector<Player*> line, float diff, float cx, float cy, float cz, float orientation, float range);
        WorldLocation MoveSingleLine(std::vector<Player*> line, float diff, float cx, float cy, float cz, float orientation, float range);
    };

    class MoveAheadFormation : public MoveFormation
    {
    public:
        MoveAheadFormation(PlayerbotAI* ai, std::string name) : MoveFormation(ai, name) {}
        virtual WorldLocation GetLocation() override;
        virtual WorldLocation GetLocationInternal() { return NullLocation; }
    };

    class FormationValue : public ManualSetValue<Formation*>
	{
	public:
        FormationValue(PlayerbotAI* ai);
        ~FormationValue() override { if (value) { delete value; value = NULL; } }
        virtual void Reset() override;
        virtual std::string Save() override;
        virtual bool Load(std::string value) override;
    };

    class FormationPositionValue : public CalculatedValue<WorldPosition>
    {
    public:
        FormationPositionValue(PlayerbotAI* ai, std::string name = "formation position") : CalculatedValue(ai, name) {}

        virtual WorldPosition Calculate() override {
            Formation* formation = AI_VALUE(Formation*, "formation");

            WorldLocation loc = formation->GetLocation();
            if (Formation::IsNullLocation(loc) || loc.mapId == -1)
                return WorldLocation();

            return loc;
        }
    };

    class SetFormationAction : public ChatCommandAction
    {
    public:
        SetFormationAction(PlayerbotAI* ai) : ChatCommandAction(ai, "set formation") {}
        virtual bool Execute(Event& event) override;
    };
};
