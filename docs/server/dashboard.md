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

The runner script reads your database connection settings from `mangosd.conf`, enables [`AiPlayerbot.Observability`](settings/logging.md#aiplayerbotobservability) in `aiplayerbot.conf`, and starts the service. Open `http://localhost:8095` in your browser and sign in using any game account with GM rank 2 or higher.

### Manual Configuration

To configure telemetry manually, adjust these settings in `aiplayerbot.conf`:

```ini
AiPlayerbot.Observability = 1
AiPlayerbot.ObservabilityHost = 127.0.0.1
AiPlayerbot.ObservabilityPort = 9195
```

> [!NOTE]
> The template default for `AiPlayerbot.ObservabilityHost` is `127.0.0.1`. If omitted, the C++ fallback is an empty string, which binds to localhost.

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
| **Bots** | Who is online and what are they doing? | Complete roster list, filterable by level, class, spec, gold, or gear score. |
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
- **Busy:** Standing still while performing actions (casting, looting, resting to eat/drink, or moving within the last 45 seconds).
- **Stalled:** Stationary for 45+ seconds with an active destination or cycling actions without making forward progress.
- **Idle:** No movement, cast, action, or target for 45+ seconds.

### Bot Profile Inspector

Clicking any bot name or map marker opens a slide-over profile panel that inspects character data directly from the database:
- **Gear & Stats:** Paperdoll equipment view, average item level, and item quality distribution.
- **Spells:** Distinguishes runtime learned spells from default starting spells granted by race/class.
- **Talents:** Visual 7-tier talent trees with DBC layout.
- **Professions & Skills:** Current skill ranks and associated trade abilities.

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
- **Unreachable Target (`UNREACHABLE_TARGET`):** A bot attempts to engage a hostile target that cannot be reached on the navmesh.
- **Anti-Flapping Filter:** Episodes must persist past a 5-minute age gate before surfacing, preventing transient pathing pauses from cluttering alerts.
- **Restart Blackout:** Death alerts are suppressed for the first 5 minutes following a game server restart.

## Telemetry APIs & Metrics

### Prometheus Metrics Endpoint

The daemon exports Prometheus metrics at `http://localhost:8095/metrics` for scraping by Prometheus and Grafana:

- `tortoisebots_active_count{class,role}`: Number of active bots grouped by class and role.
- `tortoisebots_state_ratio`: Ratio of time spent in combat, moving, busy, resting, or stalled.
- `tortoisebots_grinding_xp_per_hour_total`: Collective XP gain rate across the bot pool.
- `tortoisebots_issues_active{type}`: Current count of active stuck episodes.
- `tortoisebots_snapshots_total`: Number of telemetry roster snapshots ingested.

### JSON REST Endpoints

- `GET /api/v1/commanders`: Returns active remote commander bindings, party member positions, and recent orders.
- `GET /api/v1/activity`: Returns pool-wide activity statistics and level progression timelines.
- `GET /api/v1/activity/loot`: Recent notable item, gathering, and money drops.
- `GET /api/v1/activity/quests`: Quest acceptances, completions, and rewards.
- `GET /api/v1/anomalies/totals`: Cumulative session totals for anomalies and action loops.

Activity counters are periodically saved to `activity-state.json` so statistics persist across daemon restarts.

## Pool KPI Report (`tools/pool_kpi_report.py`)

For offline analysis of bot fleet health over extended runs, use the Python KPI reporting tool:

```bash
python3 tools/pool_kpi_report.py \
  --bots-json path/to/bots.json \
  --levelup-log logs/levelup.log \
  --baseline tools/pool_kpi_baseline_2026-10-02.json
```

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

The tool builds composite map canvases from game MPQ archives. If map images are absent, the Map tab functions normally on a blank grid.
