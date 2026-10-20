---
id: server-admin-commands
title: Administrator & GM Commands
category: server
summary: GameMaster and server console commands for managing the bot pool, synthetic auction house, and server state.
tags: [admin, gm, commands, bot-pool, ahbot]
relates_to:
  - server-bot-pool
  - server-configuration
  - server-settings-economy
  - server-settings-world-population
---

# Administrator & GM Commands

This page documents administrative commands reserved for GameMasters and server console administrators. These commands control the managed random-bot pool, monitor and tune the synthetic auction house engine, reload configuration files, and inspect internal AI state trees.

Commands in this section require GameMaster privileges (`SEC_GAMEMASTER` or higher) or direct execution through the server console (`SEC_CONSOLE`).

## Summary

| Command | Available To | What It Does |
| :--- | :--- | :--- |
| [`.bot pool status`](#bot-pool-status) | GM & Console | Displays pool size, online counts, and reset phase. |
| [`.bot pool accounts`](#bot-pool-accounts) | Console Only | Lists managed accounts and character counts. |
| [`.bot pool adopt preview`](#bot-pool-adopt-preview) | Console Only | Previews unmanaged prefix-matching accounts. |
| [`.bot pool adopt confirm`](#bot-pool-adopt-confirm) | Console Only | Enrolls previewed accounts into managed pool. |
| [`.bot pool clean`](#bot-pool-clean) | GM & Console | Removes offline disposable bots from pool registry. |
| [`.bot pool init`](#bot-pool-init) | GM & Console | Seeds random bots up to configured minimum cap. |
| [`.bot pool reset`](#bot-pool-reset) | GM & Console | Triggers or configures managed bot pool reset modes. |
| [`.bot ah status`](#bot-ah-status) | GM & Console | Reports synthetic AH listings, buyer, and seller metrics. |
| [`.bot ah reload`](#bot-ah-reload) | GM & Console | Reloads custom prices and blacklists from database. |
| [`.bot ah rebuild`](#bot-ah-rebuild) | GM & Console | Triggers an immediate auction market evaluation pass. |
| [`.bot ah item`](#bot-ah-item) | GM & Console | Inspects, resets, or overrides item pricing rules. |
| [`.bot ah buyer`](#bot-ah-buyer) | GM & Console | Enables or disables synthetic AH buyer participation. |
| [`.bot ah synthetic`](#bot-ah-synthetic) | GM & Console | Enables or disables synthetic AH seller item creation. |
| [`.bot ah reset`](#bot-ah-reset) | GM & Console | Flushes active synthetic auctions posted by bots. |
| [`.ahbot`](#ahbot) | GM & Console | Alias for `.bot ah` auction house commands. |
| [`.reload config`](#reload-config) | GM & Console | Reloads configuration files without rebooting server. |
| [`.bot about`](#bot-about) | All Users | Prints module version, git commit hash, and build date. |
| [`.bot debug`](#bot-debug) | GM & Console | Dumps AI decision trees and runtime diagnostics. |

---

## Managed Bot Pool Administration (`.bot pool`)

These commands manage the autonomous world population and account registration. For complete pool lifecycle procedures, see [Resetting the Managed Bot Pool](bot-pool.md).

### `.bot pool status`

Displays the health, account counts, and reset state of the managed bot pool.

```text
.bot pool status
```

Reports configured reset mode, requested generation token, applied generation, managed account and character totals, online bot count, current reset phase, and the last failure reason if a reset halted.

### `.bot pool accounts`

Lists every managed pool account registered in the bot manager.

```text
.bot pool accounts
```

**Server console only.** Outputs each managed account ID, account name, registration source, and the number of characters currently residing on that account.

### `.bot pool adopt preview`

Scans the character database for pre-existing accounts matching the configured bot account prefix that are not yet managed.

```text
.bot pool adopt preview
```

**Server console only.** Read-only. Displays matching unmanaged accounts and character counts, then generates a short-lived confirmation token challenge for adoption.

### `.bot pool adopt confirm`

Enrolls previewed unmanaged bot accounts into the managed pool registry.

```text
.bot pool adopt confirm <challenge>
```

**Server console only.** Requires the confirmation token from `.bot pool adopt preview`. Idempotent. Registers accounts in the pool management table without modifying, moving, or deleting existing characters.

### `.bot pool clean`

Purges offline disposable pool bots from the character database and registry.

```text
.bot pool clean
```

Clears out stale temporary records while preserving permanent bot characters and owned alts.

### `.bot pool init`

Triggers immediate creation and initialization of random pool bots up to the configured minimum threshold.

```text
.bot pool init
```

Populates the world with bots according to the settings defined in [World Population Settings](settings/world-population.md).

### `.bot pool reset`

Configures or schedules a managed pool reset mode.

```text
.bot pool reset <off|once:<token>|always>
```

Sets the pool rebuild policy. Note that full pool rebuilds are executed during server startup based on configuration flags.

---

## Autonomous Auction House Administration (`.bot ah` / `.ahbot`)

These commands monitor and control the synthetic auction house buyer and seller engines. All `.bot ah` commands are also accessible through the `.ahbot` alias.

### `.bot ah status`

Displays real-time operational metrics for the synthetic auction house.

```text
.bot ah status
```

Shows active listing totals, seller inventory size, market price evaluations, buyer bidding activity, and transaction telemetry.

### `.bot ah reload`

Reloads custom item pricing overrides and blacklist filters from the `ahbot_items` database table into memory.

```text
.bot ah reload
```

Use this after manually updating item prices or blacklists in the database to apply changes without restarting the server.

### `.bot ah rebuild`

Forces an immediate market evaluation and restocking pass.

```text
.bot ah rebuild [all]
```

Without arguments, evaluates current market deficits and posts missing supply. Passing `all` expires all active unbid synthetic listings to completely refresh the auction house stock.

### `.bot ah item`

Inspects, resets, or overrides custom pricing and posting rules for a specific item entry ID.

```text
.bot ah item <id>
.bot ah item <id> reset
.bot ah item <id> <value> [chance] [min] [max]
```

- `<id>`: Checks current blacklist status, formula market price, and active overrides for that item ID.
- `<id> reset`: Removes custom overrides and restores formula pricing.
- `<id> <value> [chance] [min] [max]`: Sets custom price in copper, posting chance percentage, and stack size bounds.
- **Blacklisting:** Setting `<value>` and `[chance]` to `0 0` (e.g. `.bot ah item 1234 0 0`) blacklists the item, preventing it from ever being posted by the synthetic seller.

### `.bot ah buyer`

Toggles synthetic buyer participation on or off.

```text
.bot ah buyer on|off
```

When enabled, bots actively evaluate player listings on the auction house and purchase fairly priced trade goods and equipment.

### `.bot ah synthetic`

Toggles synthetic seller item generation on or off.

```text
.bot ah synthetic on|off
```

When enabled, the server automatically generates and posts market supply to maintain a vibrant economy.

### `.bot ah reset`

Flushes and cancels all active synthetic auctions posted by pool bots.

```text
.bot ah reset
```

Removes bot listings from the auction house immediately without affecting auctions posted by real players.

### `.ahbot`

Direct shorthand alias for all `.bot ah` commands.

```text
.ahbot <status|reload|rebuild|item|buyer|synthetic|reset>
```

---

## System & Diagnostic Commands

### `.reload config`

Reloads server configuration files (`mangosd.conf` and `aiplayerbot.conf`) without restarting the server process.

```text
.reload config
```

Available to GameMasters and the server console. Settings that support live reloads take effect immediately; settings requiring a server restart remain unchanged until the next process reboot.

### `.bot about`

Displays module build and version metadata.

```text
.bot about
```

Outputs the TortoiseBots version, build timestamp, git commit SHA, and project repository links. Available to all players and administrators.

### `.bot debug`

Dumps low-level AI decision trees, action priority queues, and target ratings for a bot.

```text
.bot debug [bot]
.bot dump [bot]
```

Outputs detailed evaluation traces used by developers to debug combat and movement decisions.

### GM AI Inspection and Cheats

GameMasters have access to specialized commands for inspecting and altering bot behavior:

- `cheat`: Toggles bot immunity, mana bypass, or cooldown cheats for testing.
- `cdebug` / `cs`: Displays combat engine state and active combat strategies.
- `log`: Adjusts live logging verbosity for an individual bot session.
- `set value`: Modifies runtime AI memory values and blackboard variables.
- `teleport`: Teleports a bot directly to specified map coordinates.
- `load ai` / `save ai`: Loads or saves strategy presets from disk.
- `list ai`: Lists all known strategy trees and action nodes.
- `reset ai` / `reset strats` / `reset values`: Resets internal AI trees, clearing blackboard memories and restoring default behaviors.
