---
id: server-dashboard
title: Observability Dashboard & Telemetry
category: server
summary: Setup, architecture, Prometheus metrics, and feature guide for the real-time bot observability service.
tags: [server, observability, telemetry, dashboard, metrics, prometheus]
relates_to:
  - server-settings-logging
  - server-bot-pool
  - server-admin-commands
---

# Observability Dashboard & Telemetry

TortoiseBots includes an optional real-time observability service located in `tools/observability`. When disabled, telemetry has zero overhead; once enabled, the game server emits non-blocking UDP status datagrams to a standalone daemon that hosts a web management dashboard and a Prometheus metrics endpoint.

For configuration keys controlling telemetry emission, see [Logging & diagnostic settings](settings/logging.md).

## Architecture

The observability stack consists of a lightweight C++ emitter in the game server and an external Go telemetry daemon:

```text
Game Server (TortoiseBots C++)
    │
    │  UDP Datagrams (non-blocking, default port 9195)
    ▼
Go Observability Daemon (tools/observability)
    │
    ├──► Prometheus Metrics HTTP Endpoint (:8095/metrics)
    ├──► REST API (/api/v1/status, /api/v1/bots, /api/v1/issues, /api/v1/commanders)
    └──► Embedded Web SPA Dashboard (:8095/dashboard)
```

## Quick Setup

Prebuilt dashboard binaries ship with every daily release. No Go toolchain is required for operation.

### Running with Setup Scripts

From the root of your TortoiseBots checkout:

- **Linux:**
  ```bash
  ./tools/observability/run-dashboard.sh --mangosd-conf /path/to/mangosd.conf
  ```
- **Windows (PowerShell):**
  ```powershell
  .\tools\observability\run-dashboard.ps1 -MangosdConf C:\path\to\mangosd.conf
  ```

The runner script reads your database connection settings from `mangosd.conf`, enables [`AiPlayerbot.Observability`](settings/logging.md) in `aiplayerbot.conf`, and starts the service (if already running, it continues without interrupting). Note that changing `AiPlayerbot.Observability` in `aiplayerbot.conf` requires restarting `mangosd`. Open `http://localhost:8095` in your browser and sign in using any game account with GM rank 2 or higher.

### Manual Configuration

To configure telemetry manually, adjust `AiPlayerbot.Observability`, `AiPlayerbot.ObservabilityHost`, and `AiPlayerbot.ObservabilityPort` in `aiplayerbot.conf` (see [Logging settings](settings/logging.md#aiplayerbotobservability)).

To run the daemon directly from source:
```bash
cd tools/observability
go run cmd/server/main.go --http-port 8095 --udp-port 9195
```

## Web Dashboard Features

The web interface provides real-time operational visibility into your realm's bot population:

| Tab | Key Questions Answered | Primary Features |
| :--- | :--- | :--- |
| **Overview** | Is the server healthy? | Active population, world tick rate, population graphs, and state census. |
| **Bots** | Who is online and what are they doing? | Complete roster list, filterable by level, class, spec, gold, or average item level (`iLvl`). |
| **Map** | Where is the fleet distributed? | 2D world and zone map with player and bot markers and breadcrumb paths. |
| **Commanders** | Who is leading bot groups? | Party health bars, tactical warnings, commander orders, and fight status. |
| **Progress** | Are bots levelling up smoothly? | XP per hour KPIs, level-up timelines, and average gear item level by level band. |
| **Activity** | What events have occurred? | Real-time feed of completed quests, rare loot drops, gathered nodes, and kills. |
| **Dungeons** | What dungeon and raid runs are active? | Live view of active instance copies with bots, member health, and player badges. |
| **Auction House** | What is currently on the market? | Searchable catalog of all active auction listings posted by bots and players. |
| **Issues** | Which bots are stuck or in trouble? | Tracked navigation stalls, long deaths, and unreachable combat targets. |
| **Server** | What settings are active? | Live server rates, active configuration flags, and one-click diagnostic reports. |

### State Census Semantics

On the Overview tab, bots are categorized into three operational states:
- **Busy:** Standing still while performing actions (casting, looting, resting to eat/drink, fight or XP gain in last 60 seconds, or moving within the last 45 seconds).
- **Stalled:** Stationary for 45+ seconds with an active destination, cycling actions without progress, or no fight/XP in 60 seconds while pursuing travel.
- **Idle:** No movement, cast, action, loot, or combat target for 45+ seconds.

### Bot Profile Inspector

Clicking any bot name or map marker opens a slide-over profile panel that inspects character data directly from the database:
- **Overview:** Vital stats, current location, active strategy flags, and group status.
- **Gear & Stats:** Paperdoll equipment view, average item level (`iLvl`), and item quality distribution.
- **Talents:** Visual 7-tier talent trees with DBC layout.
- **Spells:** Distinguishes runtime learned spells from default starting spells granted by race/class.
- **Professions & Skills:** Current skill ranks and associated trade abilities.
- **Bags & Containers:** Inventory slots and bag contents.
- **Activity History:** Recent kills, quest completions, and loot transactions.

### Dungeons Tab

The **Dungeons** tab provides a dedicated live overview of all active dungeon and raid runs containing bots:

- **Instance Isolation:** Groups bots by active instance copy (`(map, instance)` pair), distinctly separating multiple concurrent runs of the same dungeon.
- **Player Highlighting:** Real players in bot parties are visually distinguished with a distinct badge and gold ring indicator.
- **Raid Subgroups:** Raid encounters display members formatted into compact 5-player subgroup tiles.
- **Live Run View:** Selecting an active run presents an interactive instance map rendering bot positions, encounter targets, and vital health/mana meters.

## Issue Tracking & Stalls

The service monitors persistent problems and groups them into episodes on the **Issues** tab:

- **Stuck Navigation (`STUCK`):** A bot remains in a moving state but its physical coordinates stay frozen for 60+ seconds.
- **Prolonged Death (`DEAD_LONG`):** A dead bot remains a ghost without releasing or reviving for more than 2 minutes.
- **Unreachable Target (`UNREACHABLE_TARGET`):** A bot attempts to engage a hostile target that is >45 yards away or out of line of sight for 10 seconds, unreachable on the navmesh.
- **Anti-Flapping Filter:** Episodes must persist past an age gate (default 5 minutes; `UNREACHABLE_TARGET` surfaces after 2 minutes) before alerting, preventing transient pathing pauses from cluttering alerts.
- **Restart Blackout:** `DEAD_LONG` alerts are suppressed for the first 5 minutes following a game server restart.

## Telemetry APIs & Metrics

### Prometheus Metrics Endpoint

The daemon exports Prometheus metrics at `http://localhost:8095/metrics` for scraping by Prometheus and Grafana:

- `tortoisebots_active_count{class,role}`: Number of active bots grouped by class and role.
- `tortoisebots_state_ratio{state}`: Ratio of time spent in each operational state over rolling windows.
- `tortoisebots_grinding_xp_per_hour_total`: Collective XP gain rate across the bot pool.
- `tortoisebots_issues_active{type}`: Current count of active issue episodes by type.
- `tortoisebots_anomalies_total{type}`: Cumulative anomalies and loop triggers encountered.
- `tortoisebots_snapshots_total`: Number of telemetry roster snapshots ingested.

### Selected JSON REST Endpoints

The daemon provides selected admin-relevant HTTP REST endpoints under `/api/v1`:

- `GET /api/v1/status`: Daemon health, connected game server status, and pool population summary.
- `GET /api/v1/bots`: Complete bot fleet state, coordinates, active strategies, and inventory summary.
- `GET /api/v1/armory/bots`: Searchable roster summary for the bot armory inspector.
- `GET /api/v1/armory/bot/{guid}`: Detailed character profile, paperdoll gear, talents, and inventory.
- `GET /api/v1/issues`: Active and historical navigation and combat stall episodes.
- `GET /api/v1/commanders`: Active remote commander bindings, party member positions, and recent orders.
- `GET /api/v1/commanders/trails`: Historic movement trails and paths for active commanders.
- `GET /api/v1/dungeons`: Live view of active instance copies with member states and player indicators.
- `GET /api/v1/activity`, `/activity/loot`, `/activity/quests`: Real-time feed of bot actions, items, and quests.
- `GET /api/v1/grinding`: Detailed grinding analytics, XP rates, and kill distributions.
- `GET /api/v1/history`: Timeline snapshots of historical bot counts and states.
- `GET /api/v1/server-info`: Server version metadata, active rates, and configuration status.
- `GET /api/v1/anomalies/totals`: Cumulative session totals for anomalies and action loops.
- `GET /api/v1/auctions`: Searchable listings currently posted on the living auction house.
- `WS /api/v1/stream`: Real-time WebSocket event feed streaming live bot updates to connected clients.

Activity counters are periodically saved to `activity-state.json` so statistics persist across daemon restarts.

## Pool KPI Report (`tools/pool_kpi_report.py`)

For offline analysis of bot fleet health over extended runs, use the Python KPI reporting tool:

```bash
python3 tools/pool_kpi_report.py \
  --bots-json path/to/bots.json \
  --logs /path/to/logs \
  --reset fresh-build-01 \
  --baseline tools/pool_kpi_baseline_2026-10-02.json
```

Additional flags include `--db-cmd` (database query command), `--save-baseline` / `--out` (saving output baseline files), and `--anomaly-totals-json` for recording anomaly counters.

The script generates a Markdown summary evaluating:
- Bot level progression and median XP gain rates.
- Death rates per 1,000 kills.
- Zone migration fit: whether bots advance to level-appropriate zones as they outgrow starter areas.

## Map Artwork Generation

Zone and continent map images are client assets that are not bundled with the server repository. You can extract and composite them from your own client:

```bash
python3 -m pip install mpyq Pillow
python3 tools/observability/scripts/gen_zone_maps.py \
  --data "/path/to/WoW/Data" \
  --out tools/observability/maps
```

The `--out` path feeds the Map tab; Commander map views use the folder configured via `--map-dir` or the `TBDASH_MAP_DIR` environment variable. If map images are absent, the Map tab displays a note instead of the picture while bot position markers continue to function normally.
