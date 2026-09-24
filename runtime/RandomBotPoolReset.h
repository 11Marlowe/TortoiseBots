#pragma once

#include "PoolResetPolicy.h"

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace TortoiseBots
{

// Startup pool reset (issue #265). Deleting a character is heavy work on the
// world thread, so the reset is a state machine that runs one bounded step per
// world tick instead of a loop, and it is only ever started by configuration
// read during initial world startup (never by `.reload config` and never by a
// live command).
enum class PoolResetPhase
{
    Disabled,          // off, invalid, or the generation is already applied
    Planning,          // draining pool sessions before anything is settled
    SettlingAuctions,  // settle every target-owned listing while all bidders exist
    Deleting,          // one snapshotted character per world tick
    Verifying,         // nothing may remain before the generation is recorded
    Rebuilding,        // record the generation, then let normal auto-create refill
    Complete,
    Failed,            // pool consumers stay paused; the generation is NOT recorded
};

enum class PoolResetTick
{
    Idle,      // nothing scheduled
    Working,   // still in progress
    Completed, // deletion and verification finished, generation recorded
    Failed,    // reset stopped, pool unavailable until restart
};

class RandomBotPoolReset
{
public:
    static RandomBotPoolReset& Instance();

    // Startup only, called from RandomBotService::Initialize() after the
    // managed-account registry is validated. Parses the setting, snapshots the
    // deletion targets and runs every preflight check. No character is deleted
    // here; failure before maintenance mode leaves the pool fully available.
    void PlanAtStartup(std::string const& configuredValue, bool autoCreateEnabled);

    // One bounded step on the world thread.
    PoolResetTick Update(uint32 diff);
    void Shutdown();

    // False while reset maintenance is active or after a failure: hiring,
    // auto-create, autologin, pinned resolution and BG selection stay paused.
    bool IsPoolAvailable() const { return !m_maintenance; }
    bool IsActive() const;

    PoolResetPhase Phase() const { return m_phase; }
    char const* PhaseName() const;

    // Last generation recorded as applied; the requested one is the configured
    // setting, which callers read from the module configuration.
    std::string const& AppliedGeneration() const { return m_appliedGeneration; }
    std::string const& LastFailure() const { return m_lastFailure; }

    uint32 TargetCount() const { return static_cast<uint32>(m_targets.size()); }
    // Characters already processed (deleted, or found already gone).
    uint32 ProcessedCount() const { return m_next; }
    // Characters whose auctions have been settled so far.
    uint32 SettledAuctionOwnerCount() const { return m_settleNext; }

private:
    RandomBotPoolReset() = default;
    ~RandomBotPoolReset() = default;

    struct Target
    {
        uint32 accountId = 0;
        uint32 guidLow = 0;
        std::string name;
    };

    // Startup helpers (no mutation of world state).
    bool LoadAppliedGeneration(std::string& out, std::string& error) const;
    bool SnapshotTargets(std::string& error);
    bool PreflightGuilds(std::string& error) const;
    bool PreflightHumanSessions(std::string& error) const;

    // Tick helpers.
    bool DrainPoolSessions(std::string& error, bool& busy);
    // Settles the auctions of every target, one target per tick, while all pool
    // bidders still exist (a pool bot bidding on another pool bot's listing
    // would otherwise be gone by the time that listing is reached). Returns
    // false only on failure; `done` reports whether the phase finished.
    bool SettleAllTargetAuctions(std::string& error, bool& done);
    bool DeleteNextTarget(std::string& error);
    bool VerifyDeletion(std::string& error) const;
    bool VerifyModuleRowsCleared(std::string& error) const;
    bool ApplyGeneration(std::string& error);
    // Returns false (with a reason) when a bid cannot be refunded; the caller
    // must then leave the character and its remaining listings untouched.
    bool SettleAuctions(uint32 guidLow, std::string& error);
    // Guard for the settle-then-delete invariant: a listing that appears after
    // settlement must stop the reset instead of being deleted unrefunded.
    bool TargetOwnsAuctions(uint32 guidLow, uint32& count) const;

    void Fail(std::string reason);
    void EnterSettlingAuctions();
    void EnterDeleting();

    PoolResetSetting m_setting;
    PoolResetPhase m_phase = PoolResetPhase::Disabled;
    std::vector<Target> m_targets;
    uint32 m_managedAccounts = 0;
    uint32 m_managedCharacters = 0;
    uint32 m_next = 0;
    uint32 m_settleNext = 0;
    uint32 m_settledAuctions = 0;
    uint32 m_stallMs = 0;
    uint32 m_progressLogMs = 0;
    bool m_lastTargetBlocked = false;
    bool m_maintenance = false;
    std::string m_appliedGeneration;
    std::string m_lastFailure;
};

} // namespace TortoiseBots
