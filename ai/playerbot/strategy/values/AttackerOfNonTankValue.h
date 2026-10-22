#pragma once
#include "playerbot/PlayerbotAI.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/Value.h"
#include "TauntAttackerPolicy.h"

#include <unordered_map>
#include <vector>

namespace ai
{
    // One attacker of the group as the taunt rule sees it.
    struct NonTankAttacker
    {
        Unit* unit = nullptr;
        TauntAttackerCandidate facts;
    };

    // Every living attacker of the group whose victim is a player of the
    // bot's group, with the TauntAttackerPolicy facts filled in for the
    // spell `spell` (its range decides inRange). `lastTaunt` (may be null):
    // the guid -> getMSTime of this bot's last Taunt on each mob.
    void CollectNonTankAttackers(PlayerbotAI* ai, std::string const& spell,
        std::unordered_map<uint64, uint32> const* lastTaunt, std::vector<NonTankAttacker>& out);

    // The mob the tank Taunts off a non-tank party member (qualifier: the
    // taunt spell, "taunt"): healer victims first, then the lowest victim
    // health, then the nearest; never a mob out of the spell's range or
    // sight (the row does not move the tank), one on a tank, a CC'd,
    // evading or held CC mark (bar the owner's healer rule), one the tank
    // wins back by threat alone, or one it Taunted < 10 s ago.
    class AttackerOfNonTankValue : public UnitCalculatedValue, public Qualified
    {
    public:
        AttackerOfNonTankValue(PlayerbotAI* ai) :
            UnitCalculatedValue(ai, "attacker of non-tank"), Qualified() {}

        // The Taunt landed on `unit` (CastTauntOnAttackerAction::Execute).
        void NoteTaunt(Unit* unit);

    protected:
        virtual Unit* Calculate() override;

    private:
        std::unordered_map<uint64, uint32> lastTaunt;
    };
}
