# Changelog

## 2026-09-18

### Companions & Hiring
- On-demand companion hiring is live: use the `<Mercenary Hire>` inn-recruiter gossip wizard to pick class → race → gender → spec/role → confirm, no addon or class swap required. [#205](https://github.com/Sagiroth/TortoiseBots/pull/205)
- Prefer typing? `.bot hire <class> [role] [race] [gender]` is the fast path to the same provisioning pipeline. [#205](https://github.com/Sagiroth/TortoiseBots/pull/205)
- Both entry points share one provision service, so hired bots come out consistent regardless of how you summon them. [#205](https://github.com/Sagiroth/TortoiseBots/pull/205)

### Provisioning & Loadout
- Hires reuse RNDBOT pool candidates when available and fall back to fresh character creation, then sync level via `GiveLevel` so they match your progress. [#205](https://github.com/Sagiroth/TortoiseBots/pull/205)
- Role-matching premade talents are applied with the role forced, plus spells, skills, and incremental gear — hired bots arrive combat-ready instead of naked and confused. [#205](https://github.com/Sagiroth/TortoiseBots/pull/205)
- Tank hires get a dedicated strategy kit so they actually hold threat instead of sightseeing. [#205](https://github.com/Sagiroth/TortoiseBots/pull/205)
- Invites use the native invite + mature accept flow, keeping party state clean and avoiding half-broken group joins. [#205](https://github.com/Sagiroth/TortoiseBots/pull/205)

### Economy & Costs
- Hiring is routed through a dedicated cost service, so companions are a gold sink rather than a free army. [#205](https://github.com/Sagiroth/TortoiseBots/pull/205)

---

### Combat & AI
- Divine Favor is back in the paladin boost kit (guarded by learned-spell checks), Improved Scorch now fires before the fire-vulnerability fallback, instant Slam procs land in both Arms and Fury, and Felhunter Spell Lock finally shuts down enemy healers instead of idling [#214](https://github.com/Sagiroth/TortoiseBots/pull/214)
- Shaman Earth Shock interrupt is registered at proper interrupt priority, warlocks stop wasting long DoTs on targets below 20% health, and paladin Holy Shield / Consecration now respect learned-spell gates [#213](https://github.com/Sagiroth/TortoiseBots/pull/213)

### Classes & Resources
- Warlock soul shards are capped at 5 out of combat with a matching KEEP threshold, and hunter shots are gated behind a shared ammo check so empty-quiver bots swap to melee instead of standing around dry-firing [#210](https://github.com/Sagiroth/TortoiseBots/pull/210)
- Feral druids can finally DPS: role bitmask widened to TANK|DPS, unassigned Feral defaults to Cat, forced roles from owned companions take precedence, and spec index flows through gossip and `.bot hire` (Cat kit plus stealth included) [#207](https://github.com/Sagiroth/TortoiseBots/pull/207)

### Trade & Inventory
- Conjured food, water, and level-matched healthstones now auto-populate the trade window on trade start — mages feed mana users, warlocks hand over stones, and bots/already-stoned traders are skipped. Toggle via `AiPlayerbot.AutoShareConjuredOnTrade` (default on) [#209](https://github.com/Sagiroth/TortoiseBots/pull/209)

### Auth & Access Control
- Login hashes are now uppercase SHA1 with case-insensitive SQL matching, and GM rank resolution uses the highest `account_access` gmlevel instead of getting masked by zero-rank rows. Minimum GM level drops to 2 via the configurable `MinGMLevel` flag/env [#215](https://github.com/Sagiroth/TortoiseBots/pull/215)

### Quality of Life & Automation
- Owned bots now default to auto-loot through the NonCombatStrategies fallback (`+loot`), so your roster picks up drops without babysitting. [#208](https://github.com/Sagiroth/TortoiseBots/pull/208)
- New `.bot loot [on|off]` toggle persists via DbStore, so loot preference survives relogs and restarts. [#208](https://github.com/Sagiroth/TortoiseBots/pull/208)

### Commands & Scripting
- `.bot repair` and `.bot sell` now fan out as mature actions over dynamic scope — bulk-service an entire bot roster in one command instead of one bot at a time. [#208](https://github.com/Sagiroth/TortoiseBots/pull/208)
- Full `.bot action` parity for loot, repair, and sell with TBM ACK/ERR protocol, giving scripters a consistent, machine-readable interface. [#208](https://github.com/Sagiroth/TortoiseBots/pull/208)

### Docs & Verification
- Docs synced: config.tsv `NonCombatStrategies` row, commands.tsv loot/repair/sell rows, plus player-controls direct and action tables. [#208](https://github.com/Sagiroth/TortoiseBots/pull/208)
- `verify_okf.py` PASSED (25 nodes) and `diff --check` clean; build deferred per instruction across all 9 issues. [#208](https://github.com/Sagiroth/TortoiseBots/pull/208)

### Combat & AI
- Totems now only deploy when stationary or locked in close melee, killing the pre-combat rest-totem spam that ruined pulls. [#212](https://github.com/Sagiroth/TortoiseBots/pull/212)
- Dungeon and raid bots reuse the tight raid follow leash, so they stop drifting into extra packs mid-run. [#212](https://github.com/Sagiroth/TortoiseBots/pull/212)

### Commands & Controls
- `.bot rest` now fans mature food and drink actions over dynamic scope, with `.bot drink` / `.bot eat` aliases and full `.bot action` parity. [#212](https://github.com/Sagiroth/TortoiseBots/pull/212)
- Command docs updated (`commands.tsv` and player-controls action rows) to match the new aliases. [#212](https://github.com/Sagiroth/TortoiseBots/pull/212)

## 2026-09-17

### Observability & Engine

- New Discord alert on `issues.opened`: short embed with number, title, author, labels and link — posted as **TortoiseBots Issue**. No body dump, so the channel stays readable. [#190](https://github.com/Sagiroth/TortoiseBots/pull/190)
- Issue alerts reuse `DISCORD_WEBHOOK_URL`; set optional `DISCORD_ISSUE_THREAD_ID` to route them into a thread instead of the main channel. [#190](https://github.com/Sagiroth/TortoiseBots/pull/190)
- Missing webhook secret now skips cleanly with a warning instead of failing the workflow. [#190](https://github.com/Sagiroth/TortoiseBots/pull/190)
- Changelog poster renamed to **TortoiseBots Changelog** (default + workflow) so both bots read consistently in Discord. [#190](https://github.com/Sagiroth/TortoiseBots/pull/190)

---

### Bot Progression & Completeness
- Random-pool bots now self-complete in a fixed order — talents → spells → gear — at every level, with no gold charged and no core edits required [#191](https://github.com/Sagiroth/TortoiseBots/pull/191)
- New `PlayerbotFactory::MakeComplete()` helper chains talents → knob-gated spells → skills → incremental gear, so completion is a single call instead of scattered logic [#191](https://github.com/Sagiroth/TortoiseBots/pull/191)
- Free spell learning is scoped strictly to the random pool via `IsFreeLearnBot`; the paid trainer-with-gold path is completely untouched [#191](https://github.com/Sagiroth/TortoiseBots/pull/191)

### Performance & Caching
- Per-class trainer data is now built once per server run instead of a full creature scan for every bot — a big cost win on busy servers with large bot pools [#191](https://github.com/Sagiroth/TortoiseBots/pull/191)

### Gear & Roles
- Spec-correct gear generation falls back to a class-generic weight scale when spent talents are unknown, so off-meta or partially specced bots still get sane itemization [#191](https://github.com/Sagiroth/TortoiseBots/pull/191)
- Bots now match their LFT role against their spec-correct gear, so dungeon finder groups get properly equipped tanks/healers/DPS instead of mismatched builds [#191](https://github.com/Sagiroth/TortoiseBots/pull/191)

## 2026-09-16

### Bots, Loot & Inventory
- Fresh pool bots can now start at a random level instead of always walking up from level 1: `AiPlayerbot.RandomBotStartLevelMin`/`Max` seeds the level once on a bot's first login, before its weapon skills, professions and starter gear are seeded, so a test or fresh pool starts playable (default `1`/`1` keeps the old behaviour).
- Group loot rolls are no longer a ninja-fest: bots pass Greed on armor lighter than their spec's native type (no more Enhancement shaman stealing cloth), and reserve Need for native-spec gear or solo/world-map drops (#183)
- Off-armor upgrades now actually get equipped once the slot is emptied, fixing bots hoarding upgrades in bags instead of wearing them (#183)
- Bag audit pass cleans up inventory handling so bots stop stranding useful gear and quest items (#183)
- Safe quest cleanup prevents bots from corrupting quest state during routine pruning (#183)
- Module-only changes — no core patches required, safe drop-in for existing servers (#183)

### Observability & Tooling
- New self-contained bot armory inspector ships under `tools/observability/` for DB-only bot audits (#180)
- Inspect any bot's full profile: identity, all 38 equipment/bag slots with resolved item GUIDs, live stats for online bots, spells with icon + effect metadata, skills, and talents (#180)
- Searchable bot listing with LIKE-escaped account-prefix filtering and name search, capped at 1000 results (#180)
- Stats layer falls back to `character_stats` when `character_armory_stats` is missing, so the inspector works across setups (#180)

### CI & Releases
- Changelog updates now auto-post to the dedicated Discord thread straight from GitHub Actions — no bot invite or server ownership needed (#184)
- Delta-only delivery: only newly merged PRs get announced, and anything already in `CHANGELOG.md` is skipped, so nobody gets spammed with the full history (#184)

---

### Observability & Engine
- The armory Spells tab now shows the *full* spellbook instead of a DB-only view, so starting spells like Sinister Strike and Eviscerate no longer vanish from fresh characters — race/class defaults are pulled in properly. [#186](https://github.com/Sagiroth/TortoiseBots/pull/186)
- Spells are grouped class-aware and split into active vs. passive, replacing the fragile `Rank N` name heuristic that made the CLASS SPELLS counter wildly wrong (e.g. a level-10 hunter reading as nearly empty). [#186](https://github.com/Sagiroth/TortoiseBots/pull/186)

### Gear & Itemization
- Ship the actual item weight scale data, not just the empty schema — `ai_playerbot_weightscales` and `ai_playerbot_weightscale_data` now contain rows, so `RandomItemMgr::GetPlayerSpecId()` returns a real spec instead of 0 and bots can finally judge gear. [#185](https://github.com/Sagiroth/TortoiseBots/pull/185)
- `PlayerbotFactory::InitEquipment` no longer bails out and stat comparisons stop scoring zero across the board — bots actually get equipped sensibly. [#185](https://github.com/Sagiroth/TortoiseBots/pull/185)
- Character-side caches (`ai_playerbot_equip_cache`, `ai_playerbot_rnditem_cache`) are now generated on first boot; the cache-building code was previously stuck behind a condition that could never be true. [#185](https://github.com/Sagiroth/TortoiseBots/pull/185)

### Core Sync & Fixes

- Purged the three dead `SyncLevel*` config keys (`SyncLevelWithPlayers`, `SyncLevelMaxAbove`, `SyncLevelNoPlayer`) — they were parsed in `PlayerbotAIConfig` but never consumed by any other translation unit, so this is a pure cleanup with zero behavior change. [#187](https://github.com/Sagiroth/TortoiseBots/pull/187)
- Rewrote `living-world.md` §3 to document the real Fresh-Bot Level Seed behavior: `RandomBotStartLevelMin/Max` applied via a one-shot `GiveLevel`, verified against the 10–15 test pool. No more chasing a dynamic level bracket that never existed. [#187](https://github.com/Sagiroth/TortoiseBots/pull/187)
- Dropped the stale conf comment claiming `Randomize()` reassigns levels — that code path has no callers, so the docs and `.conf.dist.in` now match reality. Fewer red herrings when tuning bots. [#187](https://github.com/Sagiroth/TortoiseBots/pull/187)

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
