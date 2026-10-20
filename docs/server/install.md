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

Before building, verify that you have a working build environment for Tortoise WoW:
- A clean checkout of the [`tortoise-wow`](https://github.com/tortoise-wow/tortoise-wow) repository on the `main` branch.
- Standard core compilation dependencies: CMake 3.16+, GCC or Clang with C++17 support, OpenSSL, and MariaDB development libraries.

## Quick Build & Installation

From the root directory of your `tortoise-wow` server checkout:

### 1. Clone the Module

Clone TortoiseBots into the `modules/` directory:

```bash
git clone https://github.com/Sagiroth/TortoiseBots.git modules/TortoiseBots
```

### 2. Configure with CMake

Configure CMake with the native module enabled and legacy bots disabled:

```bash
cmake -B build \
  -DBUILD_LEGACY_PLAYERBOTS=OFF \
  -DMODULES=static \
  -DMODULE_TORTOISEBOTS=static
```

| CMake Option | Recommended Value | Purpose |
| :--- | :--- | :--- |
| `MODULES` | `static` | Enables static module linkage into the core executable. |
| `MODULE_TORTOISEBOTS` | `static` | Selects TortoiseBots as the active native bot engine. |
| `BUILD_LEGACY_PLAYERBOTS` | `OFF` | Disables legacy playerbots to prevent duplicate symbols. |

### 3. Compile the Server

Compile the server binaries using all available processor cores:

```bash
cmake --build build -j"$(nproc)"
```

## Configuration Files and Database Migrations

When you run `cmake --install build` (or install binaries to your deployment directory), the module installs the following templates and assets:

### Configuration Files

- Template files are copied next to `mangosd.conf`:
  - `aiplayerbot.conf.dist`
  - `modules/tortoise_bots.conf.dist`
- On first installation, the build creates active configuration files (`aiplayerbot.conf` and `modules/tortoise_bots.conf`) from these templates.
- On subsequent installs and updates, existing `.conf` files are never overwritten, preserving your custom server tuning.

### SQL Migrations

Module database updates live in `data/sql/world/` and `data/sql/character/`. The core server's automatic database updater inspects these directories and applies new migrations at startup. You do not need to apply module SQL patches manually.

## Verifying Your Build Version

Every commit merged into the TortoiseBots repository stamps a daily per-merge version string in the format `<UTC date>-v<N>` (for example, `2026-09-25-v3` indicates the third merge on that UTC day).

You can confirm which version is active using four methods:

| Verification Method | Where to Look | Expected Output |
| :--- | :--- | :--- |
| **Server Startup Log** | Server console or `Server.log` | Displays `TortoiseBots <version>` followed by the project credit. |
| **In-Game Chat Command** | In-game chat via `.bot version` | Prints the server build version and addon protocol line. |
| **Addon Window Header** | Header bar of `/tbm` in-game | Shows `TBM <addon version> · server <server version>`. |
| **Git Release Tags** | Git repository tags | Lightweight tags match `<version>`; daily releases use `vYYYY-MM-DD`. |

If the server log displays `dev`, the checkout was built directly from an unversioned feature branch rather than a stamped release commit.

## Updating TortoiseBots

To update an existing installation to the latest version:

1. Navigate to the module directory and pull latest changes:
   ```bash
   cd modules/TortoiseBots
   git pull origin dev
   ```
2. Return to the core repository root and recompile:
   ```bash
   cd ../..
   cmake --build build -j"$(nproc)"
   ```
3. Restart `mangosd`. The core auto-updater will apply any new SQL migrations automatically, and your existing `aiplayerbot.conf` settings remain intact.
4. If a module update introduces new configuration parameters, check `aiplayerbot.conf.dist` and merge new keys as desired.
