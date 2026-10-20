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

Commands in this section require GameMaster privileges or direct execution through the server console.

## Summary

| Command | Available To | What It Does |
| :--- | :--- | :--- |
| [`.bot pool status`](#bot-pool-status) | GM & Console | Displays pool reset status, generation, and account totals. |
| [`.bot pool accounts`](#bot-pool-accounts) | Console Only | Lists managed accounts and character counts. |
| [`.bot pool adopt preview`](#bot-pool-adopt-preview) | Console Only | Previews unmanaged prefix-matching accounts. |
| [`.bot pool adopt confirm`](#bot-pool-adopt-confirm) | Console Only | Enrolls previewed accounts into managed pool. |
| [`.bot ah status`](#bot-ah-status) | GM & Console | Reports synthetic AH listings, buyer, and seller metrics. |
| [`.bot ah reload`](#bot-ah-reload) | GM & Console | Reloads custom prices and blacklists from database. |
| [`.bot ah rebuild`](#bot-ah-rebuild) | GM & Console | Triggers an immediate auction market evaluation pass. |
| [`.bot ah item`](#bot-ah-item) | GM & Console | Inspects, resets, or overrides item pricing rules. |
| [`.ahbot`](#ahbot) | GM & Console | Shorthand alias for `.bot ah` commands. |
| [`.reload config`](#reload-config) | GM & Console | Reloads core configuration files. |

---

## Managed Bot Pool Administration (`.bot pool`)

These commands manage the autonomous world population and account registration. For complete pool lifecycle procedures, see [Resetting the Managed Bot Pool](bot-pool.md).

Pool population refill is automatic via `RandomBotService` up to the minimum configured in [World Population Settings](settings/world-population.md#aiplayerbotminrandombots--maxrandombots). Pool reset modes are configured at startup via `AiPlayerbot.RandomBotPoolReset` (see [World Population Settings](settings/world-population.md#aiplayerbotrandombotpoolreset)); there is no live `.bot pool clean`, `.bot pool init`, or `.bot pool reset` command.

### `.bot pool status`

Displays the health, account counts, and reset state of the managed bot pool.

```text
.bot pool status
```

Reports configured reset mode, requested generation token (when configured for a one-time reset), applied generation, managed account total (and whether validated), managed character total, current reset phase, reset progress (auctions settled or characters deleted), pool availability, and the last failure reason if a reset halted.

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

**Server console only.** Read-only. Displays matching unmanaged accounts and character counts, then generates a confirmation token challenge for adoption valid for 5 minutes.

### `.bot pool adopt confirm`

Enrolls previewed unmanaged bot accounts into the managed pool registry.

```text
.bot pool adopt confirm <challenge>
```

**Server console only.** Requires the confirmation token from `.bot pool adopt preview` (valid for 5 minutes). Idempotent. Registers accounts in the pool management table without modifying, moving, or deleting existing characters.

---

## Autonomous Auction House Administration (`.bot ah` / `.ahbot`)

These commands monitor and control the synthetic auction house buyer and seller engines. All `.bot ah` commands are also accessible through the `.ahbot` alias.

Buyer participation and synthetic supply generation are configured in [Economy Settings](settings/economy.md#aiplayerbotahmarketbuyer) (`AiPlayerbot.AhMarketBuyer` and `AiPlayerbot.AhMarketSyntheticSupply`); there are no live `.bot ah buyer`, `.bot ah synthetic`, or `.bot ah reset` toggle commands.

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

- `<id>`: Reports blacklist status only for that item ID.
- `<id> reset`: Removes custom overrides and restores formula pricing.
- `<id> <value> [chance] [min] [max]`: Sets custom price in copper, posting chance percentage, and stack size bounds.
- **Blacklisting:** Setting `<value>` to `0` or `[chance]` to `0` (e.g. `.bot ah item 1234 0 0`) blacklists the item, preventing it from ever being posted by the synthetic seller.

### `.ahbot`

Direct shorthand alias for `.bot ah` commands.

```text
.ahbot <status|reload|rebuild|item>
```

Accepts the same arguments as `.bot ah status`, `.bot ah reload`, `.bot ah rebuild`, and `.bot ah item`.

---

## System & Diagnostic Commands

### `.reload config`

Reloads server configuration files without restarting the server process.

```text
.reload config
```

Available to GameMasters and the server console. Core `.reload config` reloads core server configuration (`mangosd.conf`). Module gameplay settings in `aiplayerbot.conf` are read once at startup and require a server restart to change (only `TortoiseBots.LogLevel` re-reads dynamically upon reload).

---

## GM AI Inspection and Cheats (Whisper Commands)

GameMasters can inspect bot internals, alter combat state, and adjust debug logging using whisper commands (`/w <BotName> <command>`) or the console command proxy (`.bot command <BotName> <command>`). These commands are strictly restricted to GameMaster accounts:

- `cheat`: Toggles bot cheats such as immunity, mana bypass, or cooldown overrides.
- `debug` / `cdebug` / `cs`: Displays combat engine state, active strategies, and action priority evaluations.
- `log`: Adjusts live logging verbosity for an individual bot session.
- `set value <variable> <value>`: Modifies runtime AI memory blackboard variables.
- `teleport`: Teleports a bot directly to specified map coordinates.
- `load ai <profile>` / `save ai <profile>`: Loads or saves strategy presets to and from the `ai_playerbot_db_store` database table.
- `list ai`: Lists all known strategy trees and action nodes.
- `reset ai` / `reset strats` / `reset values`: Resets internal AI trees, clearing blackboard memories and restoring default behaviors.
