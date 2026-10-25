#pragma once

// The server's own run of a bot-only party (bot LFG part 1): a system binding
// keys on the lead bot itself (RemoteCommander::Binding with system set), so
// the run machinery runs unchanged with the lead bot as its stand-in
// commander, and every reply that would go to that "commander" goes to the
// server log instead. A GameMaster drives it with `.bot run system ...` (the
// lead bot targeted). A real player in the group pauses the run until the
// group is bots only again.
//
// Everything that decides here is a pure function of plain facts, so
// tools/test_system_run_policy.cpp exercises it standalone: the bind gate,
// the joined-player step, the debug command grammar and the log line. The
// world-facing glue lives in commands/Commander.cpp and runs on the world
// thread only, like every `.bot` command.
//
// Plain data and the standard library only.

#include "RemoteCommanderPolicy.h"

#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace TortoiseBots {
namespace SystemRun {

struct Member
{
    bool liveBot = false;
    bool worldBot = false;
    bool hired = false;
    bool inOtherBinding = false;
    bool inOtherGroup = false;
};

struct BindFacts
{
    bool enabled = false;
    Member lead;
    bool leadGrouped = false;
    bool leadLeads = false;
    bool raidOrBg = false;
    bool realPlayerInGroup = false;
    bool groupBound = false;
    // The lead's group's other members plus the named joiners. A joiner's
    // inOtherGroup is "in any group that is not the lead's".
    std::vector<Member> members;
};

enum class Refusal : uint8_t
{
    None,
    Disabled,
    LeadNotWorldBot,
    LeadHired,
    LeadBound,
    NotLeader,
    Raid,
    HasPlayer,
    GroupBound,
    NoMembers,
    TooLarge,
    MemberNotWorldBot,
    MemberHired,
    MemberBound,
    MemberGrouped,
};

// A server-owned party is world bots only, led by its lead, with no player in
// it and nobody else commanding it. A lead alone with joiners passes (the
// joiners make the party); anything else is refused fail closed.
inline Refusal CheckBind(BindFacts const& facts)
{
    if (!facts.enabled)
        return Refusal::Disabled;
    if (!facts.lead.liveBot)
        return Refusal::LeadNotWorldBot;
    if (facts.lead.hired)
        return Refusal::LeadHired;
    if (!facts.lead.worldBot)
        return Refusal::LeadNotWorldBot;
    if (facts.lead.inOtherBinding)
        return Refusal::LeadBound;
    if (facts.leadGrouped && !facts.leadLeads)
        return Refusal::NotLeader;
    if (facts.leadGrouped && facts.raidOrBg)
        return Refusal::Raid;
    if (facts.leadGrouped && facts.realPlayerInGroup)
        return Refusal::HasPlayer;
    if (facts.leadGrouped && facts.groupBound)
        return Refusal::GroupBound;
    if (facts.members.empty())
        return Refusal::NoMembers;
    if (facts.members.size() + 1 > RemoteCommander::kMaxGroupSize)
        return Refusal::TooLarge;
    for (Member const& member : facts.members)
    {
        if (!member.liveBot)
            return Refusal::MemberNotWorldBot;
        if (member.hired)
            return Refusal::MemberHired;
        if (!member.worldBot)
            return Refusal::MemberNotWorldBot;
        if (member.inOtherBinding)
            return Refusal::MemberBound;
        if (member.inOtherGroup)
            return Refusal::MemberGrouped;
    }
    return Refusal::None;
}

inline char const* RefusalCode(Refusal refusal)
{
    switch (refusal)
    {
        case Refusal::None: return "ok";
        case Refusal::Disabled: return "disabled";
        case Refusal::LeadNotWorldBot: return "no-bot";
        case Refusal::LeadHired: return "hired";
        case Refusal::LeadBound: return "bot-bound";
        case Refusal::NotLeader: return "not-leader";
        case Refusal::Raid: return "group-raid";
        case Refusal::HasPlayer: return "group-has-player";
        case Refusal::GroupBound: return "group-bound";
        case Refusal::NoMembers: return "no-members";
        case Refusal::TooLarge: return "group-too-large";
        case Refusal::MemberNotWorldBot: return "no-bot";
        case Refusal::MemberHired: return "hired";
        case Refusal::MemberBound: return "bot-bound";
        case Refusal::MemberGrouped: return "member-grouped";
    }
    return "rejected";
}

inline char const* RefusalText(Refusal refusal)
{
    switch (refusal)
    {
        case Refusal::None: return "ok";
        case Refusal::Disabled:
            return "Server-owned runs are off (AiPlayerbot.DungeonSelfClear = 1 and AiPlayerbot.BotLfgGroups = 1 turn them on).";
        case Refusal::LeadNotWorldBot:
        case Refusal::MemberNotWorldBot:
            return "A server-owned party is world bots only: online, not owned, hired, claimed or an alt, and no player's companion.";
        case Refusal::LeadHired:
        case Refusal::MemberHired:
            return "A hired companion cannot join a server-owned party (its hire ends when it leaves its hirer's party).";
        case Refusal::LeadBound:
        case Refusal::MemberBound:
            return "A bot of the party is still in another commander's group. That commander must unbind first.";
        case Refusal::NotLeader: return "The lead bot must lead the party.";
        case Refusal::Raid: return "The lead bot is in a raid or battleground group.";
        case Refusal::HasPlayer: return "The lead bot's group has a player in it. Server-owned parties are bots only.";
        case Refusal::GroupBound: return "Another commander already commands that group.";
        case Refusal::NoMembers: return "Name the bots to group with the lead bot: .bot run system bind <Bot> [<Bot>...].";
        case Refusal::TooLarge: return "A server-owned party holds at most five bots.";
        case Refusal::MemberGrouped: return "A named bot is in another group. Remove it from that group first.";
    }
    return "Request rejected.";
}

enum class JoinStep : uint8_t
{
    None,
    Pause,
    Resume,
};

constexpr char const* kPlayerJoinedNote =
    "a player joined the group: the run waits until the group is bots only again";

// A joined player pauses a running run; when no real player is in the group
// any more a run paused for him resumes by itself. Anything else is no step.
inline JoinStep DecideJoinStep(bool running, bool paused, bool pausedForPlayer, bool realPlayerInGroup)
{
    if (running && realPlayerInGroup)
        return JoinStep::Pause;
    if (paused && pausedForPlayer && !realPlayerInGroup)
        return JoinStep::Resume;
    return JoinStep::None;
}

enum class Verb : uint8_t
{
    Invalid,
    Bind,
    Unbind,
    Cmd,
    Forward,
};

struct Args
{
    Verb verb = Verb::Invalid;
    std::string rest;
};

inline std::string Trimmed(std::string value)
{
    size_t first = value.find_first_not_of(" \t");
    if (first == std::string::npos)
        return {};
    size_t last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

inline bool HasPlayerOrSpeedWord(std::string const& rest)
{
    size_t i = 0;
    while (i < rest.size())
    {
        while (i < rest.size() && (rest[i] == ' ' || rest[i] == '\t'))
            ++i;
        size_t start = i;
        while (i < rest.size() && rest[i] != ' ' && rest[i] != '\t')
            ++i;
        if (start == i)
            break;
        std::string word;
        for (size_t j = start; j < i; ++j)
            word += static_cast<char>(std::tolower(static_cast<unsigned char>(rest[j])));
        if (word == "player" || word.compare(0, 6, "speed=") == 0)
            return true;
    }
    return false;
}

// `.bot run system bind [<Bot>...] | cmd <order> | start [plan] [from ...] |
// pause | resume | skip | stop | status | unbind`. System runs are bot-led at
// speed 1.0, so `player` and `speed=` are refused everywhere.
inline Args ParseArgs(std::string const& text)
{
    Args out;
    std::string trimmed = Trimmed(text);
    if (trimmed.empty())
        return out;
    size_t split = trimmed.find_first_of(" \t");
    std::string verb = split == std::string::npos ? trimmed : trimmed.substr(0, split);
    std::string rest = split == std::string::npos ? std::string() : Trimmed(trimmed.substr(split));
    for (char& c : verb)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (verb == "bind")
    {
        out.verb = Verb::Bind;
        out.rest = rest;
        return out;
    }
    if (verb == "unbind")
    {
        if (!rest.empty())
            return out;
        out.verb = Verb::Unbind;
        return out;
    }
    if (verb == "cmd")
    {
        if (rest.empty())
            return out;
        out.verb = Verb::Cmd;
        out.rest = rest;
        return out;
    }
    if (verb == "start" || verb == "pause" || verb == "resume" || verb == "skip" ||
        verb == "stop" || verb == "status")
    {
        std::string full = rest.empty() ? verb : verb + " " + rest;
        if (HasPlayerOrSpeedWord(rest))
            return out;
        out.verb = Verb::Forward;
        out.rest = full;
        return out;
    }
    return out;
}

inline std::string LogLine(std::string const& leadName, std::string const& line)
{
    return "TortoiseBots: system run of " + leadName + ": " + line;
}

} // namespace SystemRun
} // namespace TortoiseBots
