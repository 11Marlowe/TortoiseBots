# Observability daemon — developer notes

`tools/observability` is a standalone Go daemon (Prometheus + web dashboard) fed by `runtime/ObservabilityEmitter.{h,cpp}` over non-blocking UDP. It is optional and config-gated; core stays ignorant of it.

Rules that keep its state honest:

- The daemon has exactly one authoritative store (`internal/state`). REST, WebSocket, and Prometheus all read it.
- The emitter sends self-contained snapshot cycles: one `HEARTBEAT` + `BOT_BATCH` chunks sharing a `seq`. Publish a roster only from a complete cycle; clients replace their roster wholesale instead of merging deltas.
- Every datagram also carries a `session` epoch (server process start). The daemon resets sequence/roster state when it changes, so a server restart that resets `seq` cannot be locked out as "old cycles".
- Bound and prune every emitter table (bot tracking, action failures, anomaly cooldowns) on each snapshot. Macro-state ratios are windowed, never lifetime totals.
- Anomaly types are a closed set (`model.AcceptedAnomalyTypes`) so Prometheus label cardinality stays bounded.
- Bump `kProtocolVersion` in `ObservabilityEmitter.cpp` and `model.ProtocolVersion` in `internal/model/types.go` together.

## Telemetry surface (protocol v4)

Each `BOT_BATCH` bot entry carries: `name, guid, class, role, level, hp/max_hp, power/max_power, power_type, map, zone, x/y/z/o, target, strategy, state, last_action, last_trigger`.

- `power_type` is the current resource (`mana`, `rage`, `energy`, `focus`, `happiness`); druids reflect their active form. Label bars by it, never hardcode "mana".
- `last_action`/`last_trigger` feed repeated-action detection; they are sampled per 2s snapshot, not per execution.
- Anomalies carry `guid` so the daemon can key episodes; accepted types are `BOT_STUCK`, `ACTION_LOOP`, `UNREACHABLE_TARGET`, `BOT_DEATH`.
- `BOT_DEATH` is emitted from `PlayerbotAI::OnDeath` (target/zone/position/level).
- Anomaly emitters that can persist (`UNREACHABLE_TARGET`) re-report every cooldown window so the daemon has a liveness signal; do not make them fire-once.

## Armory stats

Online bots never log out, so the core `character_stats`/`character_armory_stats` tables stay empty for them. On each snapshot the emitter also writes the exact live `Player` stats of 10 bots (round-robin, ~100 s for 500) to `tortoise_bots_armory_stats` (enchants, talents, buffs included; rage in display units). The armory reads that table first (`stats.source = "module_stats"`), then the core tables, then an approximate rebuild from base values and item stats (`"live"`). Pool reset deletes the rows with the characters.

## Issue episodes (`internal/state` issue tracker)

Persistent problems are tracked as open/closed episodes per bot, surfaced in the dashboard Issues tab, map glow, roster badge, and `/api/v1/issues`.

- Snapshot-derived: `STUCK` (moving state but position frozen >= 60s), `DEAD_LONG` (dead >= 2 min).
- Anomaly-derived: `ACTION_LOOP`, `UNREACHABLE_TARGET` (refreshed by the emitter, expire after a 2 min TTL, or close early when a snapshot contradicts them).
- **Minimum age**: only episodes that persist `ISSUE_MIN_AGE_SEC` (default 300s) are shown; shorter ones are discarded entirely. This is the guard against transient false positives — keep new detectors behind it.
- Severity escalates `watch` (>= 1 min) -> `persistent` (>= 10 min).
- Resolved history survives a game-server restart (`Reset()` clears open episodes only); it is in-memory and bounded, so a daemon restart clears it.
- Metrics: `tortoisebots_issues_active{type}`; anomalies counted by type (including `BOT_DEATH`).

Dashboard UI: Bots roster supports status/class/role filters, "issues only", sortable columns, and a power bar; the Issues tab filters by type and minimum duration and highlights issue bots on the map.

## Validation

No host Go toolchain is assumed: `docker run --rm -v "$PWD/tools/observability:/src" -w /src golang:1.22-alpine sh -c 'go vet ./... && go test ./...'`. Live check: the server logs `Observability telemetry active`, and `/metrics` (default port 8095) shows `mangos_server_online 1` and a rising `tortoisebots_snapshots_total`.

Player/operator guide: [`docs/guides/observability-dashboard.md`](../../docs/guides/observability-dashboard.md).
