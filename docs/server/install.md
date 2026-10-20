---
id: server-install
title: Building, Installing & Updating TortoiseBots
category: server
summary: How to build, compile, and configure TortoiseBots natively with the Tortoise WoW core, and verify your running build version.
tags: [server, install, build, cmake, compile, update, version]
relates_to:
  - server-configuration
  - server-bot-pool
  - server-dashboard
---

# Building, Installing & Updating TortoiseBots

TortoiseBots is compiled natively as an in-tree module for the Tortoise WoW 1.18.1 server core. This document explains how to clone the module, compile it with CMake, set up initial configuration files, update an existing server, and verify your active build version.

For information on how configuration files are structured and edited, see [How configuration works](configuration.md).

## Prerequisites

Before building, verify that you have a working build environment meeting the [`tortoise-wow`](https://github.com/tortoise-wow/tortoise-wow) repository requirements on the `main` branch.

## Quick Build & Installation

From the root directory of your `tortoise-wow` server checkout:

### 1. Clone the Module

Clone TortoiseBots into the `modules/` directory:

```bash
git clone https://github.com/Sagiroth/TortoiseBots.git modules/TortoiseBots
```

### 2. Configure with CMake

Configure CMake with the native module enabled:

```bash
cmake -B build \
  -DMODULES=static \
  -DMODULE_TORTOISEBOTS=static
```

| CMake Option | Recommended Value | Purpose |
| :--- | :--- | :--- |
| `MODULES` | `static` | Enables static module linkage into the core executable. |
| `MODULE_TORTOISEBOTS` | `static` | Selects TortoiseBots as the active native bot engine. |

### 3. Compile the Server

Compile the server binaries using all available processor cores:

```bash
cmake --build build -j"$(nproc)"
```

### 4. Install Binaries and Assets

Install the compiled binaries and configuration templates into your deployment prefix:

```bash
cmake --install build
```

## Configuration Files and Database Migrations

When you run `cmake --install build`, the module installs templates and configuration files:

### Configuration Files

- Template files are copied next to `mangosd.conf`:
  - `aiplayerbot.conf.dist`
  - `modules/tortoise_bots.conf.dist`
- On first installation, the install step creates active configuration files (`aiplayerbot.conf` and `modules/tortoise_bots.conf`) from these templates.
- On subsequent installs and updates, existing `.conf` files are never overwritten, preserving your custom server tuning.

### SQL Migrations

Module database updates live in `data/sql/world/` and `data/sql/character/`. The core server's automatic database updater inspects these directories and applies new migrations at startup. You do not need to apply module SQL patches manually.

## Verifying Your Build Version

The active version identifier comes from the repository root `VERSION` file, written by the release workflow on every release (currently a semantic version such as `1.4.0`). If the file is unreadable or absent, the build falls back to `dev`.

You can confirm which version is active using four methods:

| Verification Method | Where to Look | Expected Output |
| :--- | :--- | :--- |
| **Server Startup Log** | Server console or `Server.log` | Displays `TortoiseBots <version> (AI enabled\|disabled)`, followed by the author/source attribution line. |
| **In-Game Chat Command** | In-game chat via `.bot version` (aliases `.bot v`, `.bot about`, `.bot credits`) | Prints `TortoiseBots <version>`, `TBM:VERSION|<version>`, and the author attribution line. |
| **Addon Roster Reply** | Addon roster responses carrying `TBM:VERSION|<version>` | The addon shows the server version from this line (shows `server ?` when the connected module never sends it). |
| **Git Release Tags** | Git repository tags | Semver release tags (e.g. `v1.4.0`), alongside older per-merge `<UTC date>-v<N>` build tags. |

If the log shows `dev`, the build had no readable root `VERSION` file.

## Updating TortoiseBots

To update an existing installation to the latest version:

1. Navigate to the module directory and pull latest changes:
   ```bash
   cd modules/TortoiseBots
   git pull origin main
   ```
2. Return to the core repository root, recompile, and reinstall:
   ```bash
   cd ../..
   cmake --build build -j"$(nproc)"
   cmake --install build
   ```
3. Restart `mangosd`. The core auto-updater will apply any new SQL migrations automatically, and your existing `aiplayerbot.conf` settings remain intact.
4. If a module update introduces new configuration parameters, check `aiplayerbot.conf.dist` and merge new keys as desired.
