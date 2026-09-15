# Changelog

## 2026-09-15

### Performance & Engine

- Staggered the cell-grid spatial scan per bot on a 1s cadence instead of every tick, cutting the biggest chunk of world-tick CPU (~60%). Discovery of new grind targets is delayed, but whole-bot decisions stay live; in-combat, dead, low-HP/mana, and post-revive/teleport all force back to the full 100ms rate. (#179)
- Idle skips now suppress empty scans entirely: bots on taxi flights and RESTING+SANCTUARY or RESTING+stationary regen stop burning scan time for nothing. (#179)
- SweepStrandedBots no longer excludes bot-only groups, and the UpdateAI telemetry string concat + perf monitor start are gated behind perfMonEnabled. (#177)
- GetPriorityType / HasPlayerRelation now check real network sessions instead of iterating all 1000 random bots, with the IN_EMPTY_SERVER early-out restored. Noticeable tick savings on crowded servers. (#177)

### Combat & AI

- Dungeon corpse runs work cross-map: a bot that dies in an instance (e.g. Zul'Farrak) finds the entrance portal on the ghost's continent map, steps through, and pathfinds to its corpse inside. Also fixed the false master-resurrect block in FindCorpseAction. (#172)
- New `.bot role <name> tank|healer|dps|clear` command forces a role; tank kit mirrors native AiFactory strategies. IsPullCandidate now follows explicit selection > designated tank > native spec and never guesses DPS. (#167)
- Fixed the pull movement/freeze issue, added ranged fallback when no melee tank is available, and added a DPS threat window with pause support. (#167)
- Party and whisper chat commands are now routed into the AI, so bots respond to direct coordination without needing a full command channel. (#172)

### Rescue & Core Sync Fixes

- Misplaced-bot rescue (hopeless-death relocation + stranded sweep) now works for bots grouped with other bots: a group only protects a bot when a real player is in it. Bot-only-grouped bots are relocated and leave the group first. (#176)
- Death count is no longer wiped by XP packets, including exploration XP handed out when a ghost is repopped to a graveyard — so rescue thresholds fire correctly. (#176)

### Runtime & Build

- Replaced all 22 `boost::algorithm` call sites (`iequals`, `istarts_with`, `trim`) with hand-written equivalents. This drops boost-algorithm, the last compile-time Boost dep, and its ~35-package vcpkg transitive closure. Faster CI and cleaner builds. (#173)
- Removed three dead third-party includes (boost::stacktrace in MemoryMonitor, plus vestigial OpenSSL/Boost includes in PlayerbotLLMInterface and LootValues). No behavior change; less to link and maintain. (#171)
- Dropped the OpenSSL RAND_bytes fallback from GenerateRandomPassword, which was forcing `libcrypto-3-x64.dll` to load even on builds that otherwise didn't need it. (#170)

### Addon & Tooling

- New silent addon command channel: `host/BotAddonAdapter` hooks PLAYERHOOK_ON_ADDON_MESSAGE and forwards `TBM`-prefixed payloads to the existing BotCommands entry point. UI clicks drive `.bot` commands with no chat-frame spam and no echo to nearby players. (#165)
- Documented the merged host chat seams (transport, script hooks, headless drain, chat hardening, OnChatYell) in HOST_API.md and pinned the compatible baseline to merged main. (#164)
- Changelog CI now appends to an existing same-day section and updates the existing GitHub release instead of failing with HTTP 422 on tag collision. (#169)

---

## 2026-09-14

### Combat & AI
- Bots now path around obstacles to reach targets that are out of line of sight instead of walking into walls; targets that stay unreachable for 15s get blacklisted per-bot for 5 minutes, killing the `invalid target` trigger spam (#157)
- Capital city critters and NPCs are no longer grind targets — no more random bots picking fights with Gamon while the player is just trying to use the auction house; anything that attacks the bot still gets fought back (#158)

### Starter Zones & World
- Goblin and High Elf bots rescued by `TeleportMisplacedBot` are now routed to their homebind instead of being dumped on Blackstone Island or stranded in Hillsbrad at level 5 — no more bots stuck in zones with no way out (#161)

### Core Sync & Fixes
- Death is now logged and counted exactly once: repeated `OnDeath` fires during graveyard teleport and spirit-healer revive no longer inflate death counts several times per second (#159)
- Removed the bogus `UNIT_STAT_STUNNED` logout check — combat stuns no longer trigger a bot logout, restoring correct `isLogingOut()`-based behavior from the donor core (#160)

### Tooling & Docs
- Added `tools/generate_changelog.py` plus a `CHANGELOG.md` seed and a `generate-changelog.yml` workflow, so releases can be generated from merged PRs with OpenCode AI instead of hand-writing notes (#163)
- Updated canonical target core branch references from `bot-helpers` to `1181dev` in `README.md` and `CONTRIBUTING.md` after the upstream merge (#162)

---

## 2026-09-13

### Starter Zones & Survival
- **Unsurvivable Zone Repatriation:** Repatriate alive bots stranded in zones significantly above their level range ([#155](https://github.com/Sagiroth/TortoiseBots/pull/155)).
- **Custom Starter Island Blacklist:** Route Goblin and High Elf bots to standard starter zones and blacklist custom islands lacking egress paths ([#149](https://github.com/Sagiroth/TortoiseBots/pull/149)).
- **Classic Zone Level Population:** Populated `ai_playerbot_zone_level` with classic zone level mappings ([#148](https://github.com/Sagiroth/TortoiseBots/pull/148)).
- **Low-Level Travel Gating:** Reject travel destinations whose route crosses zones the bot cannot survive, keeping bots below level 10 within their starter regions ([#147](https://github.com/Sagiroth/TortoiseBots/pull/147), [#150](https://github.com/Sagiroth/TortoiseBots/pull/150), [#153](https://github.com/Sagiroth/TortoiseBots/pull/153)).

### Observability & AI
- **Rage Telemetry Units:** Converted rage values to normal 0-100 display units for dashboard visualization ([#154](https://github.com/Sagiroth/TortoiseBots/pull/154)).
- **Stuck Detector Sampling:** Throttled stuck evaluation to once per second rather than every world tick ([#152](https://github.com/Sagiroth/TortoiseBots/pull/152)).
- **AI Texts & Item Casting:** Seeded `ai_playerbot_texts` and repaired item cast validation checks ([#151](https://github.com/Sagiroth/TortoiseBots/pull/151)).
