#pragma once

// Pure policy for hired-companion quest autonomy (play-test 2026-10-13:
// fresh hires in Stormwind ran their own questing right after the hire -
// QuestTravelToGiver "Special Delivery for Gaxim" in bot_events.csv plus the
// tank whispering "I can't talk with the quest giver").
//
// Owner rule: a hired companion follows its master and never runs its own
// questing/travel or talks to NPCs on its own - and never to a
// <Mercenary Hire> recruiter. Owned alts keep their explicit orders: every
// rule below keys off the hire-ledger record (HireLifecycle::IsHired), never
// off master presence, so an owned alt with a live master is untouched.
//
// Header-only and free of core/database types on purpose: the automation
// paths (strategy build, quest-talk actions, travel requests) read live game
// state, the rules here only decide. Exercised standalone in
// tools/test_hire_quest_policy.cpp.

#include <cstdint>

namespace TortoiseBots
{
// Every <Mercenary Hire> recruiter entry (95000-95067). Named distinctly
// from IsHireRecruiterEntry in runtime/WorldBuffPolicy.h (same range) so
// translation units including both never collide.
constexpr uint32_t kHireQuestRecruiterEntryFirst = 95000;
constexpr uint32_t kHireQuestRecruiterEntryLast = 95067;

inline bool IsHireQuestRecruiterEntry(uint32_t entry)
{
    return entry >= kHireQuestRecruiterEntryFirst && entry <= kHireQuestRecruiterEntryLast;
}

// Strategy build: a hire never carries the open-world autonomy kit. The
// pool-bot branch adds grind/travel/tfish/rpg/maintenance through
// AddDefaultNonCombatStrategies and RandomBotNonCombatStrategies; a hire
// keeps only the follower kit (collision/group/guild + the config list
// minus those entries), so it never requests quest travel, picks RPG quest
// targets, or idles at quest givers. Pool bots and owned alts are untouched.
inline bool HireSkipsAutonomyStrategies(bool isHired)
{
    return isHired;
}

// Quests: a hire never accepts, talks about, completes or shares a quest,
// not even on an explicit player order. Hires are ethereal, so questing
// gains nothing that survives the session. Pool bots and owned alts keep
// questing.
inline bool HireMayDoQuests(bool isHired)
{
    return !isHired;
}

// Quest travel: a hire never requests or picks quest destinations. Its
// journeys are player-ordered (follow, explicit travel orders); the travel
// strategy stays off the hire the same way the autonomy kit does.
inline bool HireMayRequestQuestTravel(bool isHired)
{
    return !isHired;
}

} // namespace TortoiseBots
