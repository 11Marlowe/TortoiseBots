#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

// Pure policy: what a failed movement action adds to its action-log line
// ("A:move to position - FAILED ..."), and when a bot's last "move to
// position" counts as failing now.
//
// Blackfathom Deeps 2026-10-23: the lead bot's "move to position" FAILED 172
// times in a row from a deep pool, and the log could not say why (the
// suspected cause, the core's swim answer thrown away by getPathStepFrom, was
// inferred, not read). The line now carries the move's fail reason
// (MoveFailReasonName), the bot's water state (in water, under water, the
// swimming flag) and the last path type the core's PathFinder gave the bot's
// own route on that try (0: no pathfinding on that try, the route was reused).
namespace ai
{
    inline std::string MoveFailNote(const char* reason, bool inWater, bool underWater, bool swimming,
        std::uint32_t pathType)
    {
        char buf[96];
        std::snprintf(buf, sizeof(buf), " why=%s water=%d under=%d swim=%d path=0x%x", reason ? reason : "none",
            inWater ? 1 : 0, underWater ? 1 : 0, swimming ? 1 : 0, static_cast<unsigned>(pathType));
        return buf;
    }

    // A "move to position" that failed within this long counts as failing now:
    // the action runs every ~2 s while its anchor is out of reach.
    constexpr std::uint32_t kPositionMoveFailFreshMs = 5000;

    // failMs: the WorldTimer ms of the last failed "move to position" (0: the
    // last one did not fail). Wraps like the core's getMSTimeDiff.
    inline bool PositionMoveFailing(std::uint32_t failMs, std::uint32_t nowMs)
    {
        if (!failMs)
            return false;
        return static_cast<std::uint32_t>(nowMs - failMs) <= kPositionMoveFailFreshMs;
    }
}
