---
id: play-commands-bot-panel
title: Bot Panel & Inspection Commands
category: play
summary: Commands for inspecting bot state, managing gear and inventory, and toggling runtime behaviors.
tags: [commands, inspection, bot-panel, gear]
relates_to:
  - play-commands
  - play-commands-addon
---

# Bot Panel & Inspection Commands

TortoiseBots includes a structured command interface for inspecting companion vitals, managing equipped gear and inventory bags, and adjusting runtime behavior toggles. These commands power the in-game bot panel in [TortoiseBotsManager](addon.md#tortoisebotsmanager-addon-tbm) and can also be executed directly via chat.

## Summary

| Command | Parameters / Syntax | What It Does |
| :--- | :--- | :--- |
| [`.bot list`](#bot-list) | None | Lists all online bots you control. |
| [`.bot status`](#bot-status) | `<Name>` | Displays detailed lifecycle and movement state for a bot. |
| [`.bot stats`](#bot-stats) | None | Summarizes online counts for your controllable bots. |
| [`.bot lease`](#bot-lease) | `[status]` | Reports autonomous activity leases and active timers. |
| [`.bot version`](#bot-version) | None | Displays module version, protocol string, and author credits. |
| [`.bot help`](#bot-help) | None | Prints the current bot system version header. |
| [`.bot inv`](#bot-inv) | `<Name>` | Emits detailed inventory, equipped gear, and coin count. |
| [`.bot item <Name> trade`](#bot-item-trade) | None | Prompts the bot to open a direct trade window with you. |
| [`.bot item <Name> equip`](#bot-item-equip) | `<bag> <slot>` | Equips an item from the bot's bags into gear slots. |
| [`.bot item <Name> unequip`](#bot-item-unequip) | `<bag> <slot>` | Moves an equipped item into the bot's inventory bags. |
| [`.bot item <Name> give`](#bot-item-give) | `<bag> <slot>` | Places an inventory item into an active trade window. |
| [`.bot behavior`](#bot-behavior) | `<Name\|scope> <key> <on\|off>` | Sets persistent bot behaviors (loot, AoE, mimic...). |
| [`.bot totem`](#bot-totem) | `<scope> <slot\|mode\|drop> [...]` | Configures and drops shaman totem loadouts. |

---

## Diagnostic and Inspection Commands

### `.bot list`

Lists all online bots you control (owned alts, claimed bots, or bots you master).

```text
.bot list
```

Displays each bot's character name, online lifecycle state (`starting` or `in world`), random-pool flag, and whether its AI brain is currently attached.

### `.bot status`

Displays detailed runtime diagnostic status for an individual bot you control.

```text
.bot status <CharacterName>
```

Shows the bot's current lifecycle stage, AI attachment, active movement strategy (`follow`, `stay`, `guard`, `free`, `custom`), and whether you or another player masters it.

### `.bot stats`

Summarizes metrics for the bots you control.

```text
.bot stats
```

Outputs the total count of online controllable bots, how many are random-pool bots, and how many currently have an active AI controller attached.

### `.bot lease`

Reports autonomous activity leases across the bot population.

```text
.bot lease [status]
```

Breaks down active bot activities across categories: `Idle`, `Grinding`, `Trading`, `LftQueued`, `BgQueued`, and `PlayerMaster`. Lists remaining durations for active leases.

### `.bot version`

Displays module version details, addon protocol handshake line, and author credits.

```text
.bot version
```

Prints `TortoiseBots <BuildVersion>`, the `TBM:VERSION|<BuildVersion>` protocol line, and the project attribution line. The same credit line is sent to every real (networked game client) player on login and written to the server startup log. Aliases include `.bot v`, `.bot about`, and `.bot credits`. Available to all players.

### `.bot help`

Prints the module version header to chat.

```text
.bot help
```

Alias: `.bot h`.

---

## Bot Panel Commands

Bot panel commands work on any online bot you control (owned alts and hired mercenary companions). They emit structured `TBM:` protocol lines that drive the graphical addon interface.

### `.bot inv`

Generates a snapshot of a bot's equipped gear, carried bag items, free bag slots, and carried money.

```text
.bot inv <CharacterName>
```

Alias: `.bot inventory <CharacterName>`.

<details>
<summary>Protocol format</summary>

Emits structured protocol records:
- `TBM:INV_BEGIN|<bot>|<copper>`: Starts the inventory stream with the bot's carried coin.
- `TBM:INV_EQ|<slot>|<itemId>|<durability>|<max>`: One record per equipped gear slot.
- `TBM:INV_ITEM|<bag>|<slot>|<itemId>|<count>|<flags>`: One record per carried bag item. Flags:
  - `e`: The bot can equip the item.
  - `eu`: The item is an equip upgrade (the `u` flag only appears together with `e`, never alone).
  - `b`: The item cannot be traded to you right now.
  - `-`: None / standard item.
- `TBM:INV_END|<bot>|<freeSlots>|<totalSlots>`: Concludes stream with bag capacity.

</details>

### `.bot item <Name> trade` <a id="bot-item-trade"></a>

Commands the bot to initiate a direct trade session with you.

```text
.bot item <CharacterName> trade
```

Place items or gold into the trade window and click accept; the companion bot accepts the trade automatically on its own. Responds with `TBM:ACTION_ACK|item trade|bot:<Name>|1|open`.

### `.bot item <Name> equip` <a id="bot-item-equip"></a>

Instructs the bot to equip an item from its inventory bags into the appropriate equipment slot.

```text
.bot item <CharacterName> equip <bag> <slot>
```

Specify the bag index and slot number as reported by `.bot inv`. On success (and when the item is no longer there after the operation), a refreshed inventory snapshot follows.

### `.bot item <Name> unequip` <a id="bot-item-unequip"></a>

Instructs the bot to remove an equipped item and place it into its inventory bags.

```text
.bot item <CharacterName> unequip <bag> <slot>
```

Equipped items use bag index `255` and slot indices `0` through `18`. Fails with `failed` if the bot's inventory bags are full. On success, a refreshed inventory snapshot follows.

### `.bot item <Name> give` <a id="bot-item-give"></a>

Places an item from the bot's inventory into an active trade window with the player.

```text
.bot item <CharacterName> give <bag> <slot>
```

If no trade window is currently open, the bot opens one and returns a `pending` status; repeat the command once the window shows (the addon does this automatically). Items the core would refuse in trade are refused upfront (`no-trade`) because the core would cancel the entire trade.

<details>
<summary>Action Protocol Replies</summary>

Replies use the structured action protocol:
- `TBM:ACTION_ACK|item <op>|bot:<Name>|1|<itemId>[ pending]` (trade ACKs detail `"open"`)
- `TBM:ACTION_ACK|behavior <key>|bot:<Name>|1|<on|off>`
- `TBM:ACTION_ACK|behavior <key>|party|<N>|<on|off>`
- `TBM:ACTION_ERR|<intent>|<code>|<message>`

The ACK scope is `party` or `bot:<Name>`, the detail never contains `|`, and a fresh `TBM:BOTSTATE` line (or lines for party scope) accompanies successful behavior changes.

</details>

### `.bot behavior`

Configures persistent behavioral toggles for an individual bot or the entire group.

```text
.bot behavior <CharacterName|party|raid> <key> <on|off>
```

Alias: `.bot behaviour`. Settings are persisted per bot across sessions.

<details>
<summary>Available behavior keys</summary>

- `loot`: Autonomous corpse looting (on by default).
- `aoe`: Multi-target and area-of-effect abilities (on by default).
- `autocc`: Smart crowd control on loose hostile adds (off by default).
- `boost`: Offensive cooldown usage (for example warrior *Death Wish* or mage *Arcane Power*).
- `threat`: Threat back-off that eases off when the bot nears the tank's threat.
- `potions`: Automatic consumption of healing and mana potions through the reaction engine.
- `spread`: Opt-in tactical spacing to avoid stacking in combat (not the default raid survival spread).
- `mimic`: Mimics consumable usage out of combat (food, drink, elixirs, flasks, and scrolls). The bot resolves a class-, spec-, and level-appropriate equivalent and casts it virtually (no inventory item needed). When more than 4 bots mimic at once, individual chat confirmations are suppressed in favor of a single `[Raid Mimic]` summary. Supports party-wide toggles: `.bot behavior party mimic on`.

</details>

### `.bot totem`

Configures and manages shaman totem loadouts and casting policies.

```text
.bot totem <bot|party|raid> <slot> <totem name|auto|none>
.bot totem <bot|party|raid> mode <auto|prepull|manual>
.bot totem <bot|party|raid> drop
.bot totem <bot|party|raid> status
```

<details>
<summary>Totem parameters and modes</summary>

- **Slot Pinning:** Assign a specific totem to an element slot (`earth`, `fire`, `water`, `air`). Passing `auto` restores automatic selection (subgroup-aware for earth, water, and air; falls back to spec-row behavior for fire). Passing `none` stores an empty order identical to unset, so the slot falls back to the automatic pick (auto behavior).
- **Casting Modes:**
  - `auto`: Bot drops totems as needed based on combat conditions (default).
  - `prepull` (or `pre-pull`): Drops totems out of combat when combat is imminent (ready check / rebuff window) and refreshes them in combat.
  - `manual`: Suppresses automatic drops; totems are dropped only via `.bot totem drop` or explicit strategies.
- **Immediate Drop:** `.bot totem <scope> drop` queues the configured totems now, in or out of combat.
- **Status Query:** `.bot totem <bot|party|raid> status` reports current loadout and mode via `TBM:TOTEM` records followed by a trailing `TBM:ACTION_ACK|totem status|<scope>|<n>|-`.

</details>
