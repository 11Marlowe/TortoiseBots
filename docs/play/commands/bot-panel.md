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

TortoiseBots includes a structured command interface for inspecting companion vitals, managing equipped gear and inventory bags, and adjusting runtime behavior toggles. These commands power the in-game bot panel in [TortoiseBotsManager](addon.md) and can also be executed directly via chat.

## Summary

| Command | Parameters / Syntax | What It Does |
| :--- | :--- | :--- |
| [`.bot list`](#bot-list) | None | Lists all online bots currently owned by your account. |
| [`.bot status`](#bot-status) | `<Name>` | Displays detailed lifecycle and movement state for a bot. |
| [`.bot stats`](#bot-stats) | None | Summarizes active fleet counts and AI attachments. |
| [`.bot lease`](#bot-lease) | `[status]` | Reports autonomous activity leases and active timers. |
| [`.bot version`](#bot-version) | None | Displays module version, build date, and author credits. |
| [`.bot help`](#bot-help) | None | Prints the current bot system version header. |
| [`.bot inv`](#bot-inv) | `<Name>` | Emits detailed inventory, equipped gear, and coin count. |
| [`.bot item trade`](#bot-item-trade) | `<Name>` | Prompts the bot to open a direct trade window with you. |
| [`.bot item equip`](#bot-item-equip) | `<Name> <bag> <slot>` | Equips an item from the bot's bags into gear slots. |
| [`.bot item unequip`](#bot-item-unequip) | `<Name> <bag> <slot>` | Moves an equipped item into the bot's inventory bags. |
| [`.bot item give`](#bot-item-give) | `<Name> <bag> <slot>` | Places an inventory item into an active trade window. |
| [`.bot behavior`](#bot-behavior) | `<Name\|scope> <key> <on\|off>` | Sets persistent bot behaviors (loot, AoE, mimic...). |
| [`.bot totem`](#bot-totem) | `<scope> <slot\|mode\|drop> [...]` | Configures and drops shaman totem loadouts. |

---

## Diagnostic and Inspection Commands

### `.bot list`

Lists all online bots belonging to your account.

```text
.bot list
```

Displays each bot's character name, online lifecycle state (`starting`, `in world`, `removing`), random-pool status, and whether its AI brain is currently active.

### `.bot status`

Displays detailed runtime diagnostic status for an individual owned bot.

```text
.bot status <CharacterName>
```

Shows the bot's current lifecycle stage, AI attachment, active movement strategy (`follow`, `stay`, `guard`, `free`, `custom`), and the account ID of the registered owner.

### `.bot stats`

Summarizes owned fleet metrics across the entire server.

```text
.bot stats
```

Outputs the total count of online bots, how many are random-pool bots, and how many currently have an active AI controller attached.

### `.bot lease`

Reports autonomous activity leases across the bot population.

```text
.bot lease [status]
```

Breaks down active bot activities across categories: Idle, Grinding, Trading, LFT Queued, Battleground Queued, and Player Master. Lists remaining durations for active leases.

### `.bot version`

Displays module version details, commit hash, build date, and developer credits.

```text
.bot version
```

Aliases include `.bot v`, `.bot about`, and `.bot credits`. Available to all players.

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

Generates an authoritative snapshot of a bot's equipped equipment, carried inventory items, free bag slots, and carried money.

```text
.bot inv <CharacterName>
```

Alias: `.bot inventory`.

<details>
<summary>Protocol format</summary>

Emits structured protocol records:
- `TBM:INV_BEGIN|<bot>|<copper>`: Starts inventory stream with total carried coin.
- `TBM:INV_EQ|<slot>|<itemId>|<durability>|<max>`: One record per equipped gear slot.
- `TBM:INV_ITEM|<bag>|<slot>|<itemId>|<count>|<flags>`: One record per carried bag item. Flags include `e` (equippable), `u` (rated an upgrade), and `b` (bound/untradeable).
- `TBM:INV_END|<bot>|<freeSlots>|<totalSlots>`: Concludes stream with bag capacity.

</details>

### `.bot item trade`

Commands the bot to initiate a direct trade session with you.

```text
.bot item <CharacterName> trade
```

Place items or gold into the trade window and click accept; the companion bot accepts the trade automatically.

### `.bot item equip`

Instructs the bot to equip an item from its inventory bags into the appropriate equipment slot.

```text
.bot item <CharacterName> equip <bag> <slot>
```

Specify the bag index and slot number as reported by `.bot inv`. The server returns an acknowledgment and a refreshed inventory snapshot.

### `.bot item unequip`

Instructs the bot to remove an equipped item and place it into its inventory bags.

```text
.bot item <CharacterName> unequip <bag> <slot>
```

Equipped items use bag index `255` and slot indices `0` through `18`. Fails with `failed` if the bot's inventory bags are full.

### `.bot item give`

Places an item from the bot's inventory into an active trade window with the player.

```text
.bot item <CharacterName> give <bag> <slot>
```

If no trade window is currently open, the bot opens one and returns a `pending` status. Soulbound and quest items cannot be traded.

### `.bot behavior`

Configures persistent behavioral toggles for an individual bot or the entire group.

```text
.bot behavior <CharacterName|party|raid> <key> <on|off>
```

Alias: `.bot behaviour`. Settings are persisted per bot across sessions.

<details>
<summary>Available behavior keys</summary>

- `loot`: Autonomous corpse looting.
- `aoe`: Multi-target and area-of-effect abilities (on by default).
- `autocc`: Smart crowd control on loose hostile adds (off by default).
- `savemana`: Resource conservation mode; reduces high-cost spell usage.
- `boost`: Offensive cooldown usage (e.g. *Death Wish*, *Arcane Power*).
- `threat`: Lowers threat output when approaching the tank's aggro ceiling.
- `potions`: Automatic consumption of healing and mana potions.
- `spread`: Opt-in tactical spacing to avoid stacking in combat.
- `mimic`: Mimics consumable usage out of combat (elixirs, flasks, food, scrolls). Supports party-wide toggles: `.bot behavior party mimic on`.

</details>

### `.bot totem`

Configures and manages shaman totem loadouts and casting policies.

```text
.bot totem <bot|party|raid> <slot> <totem name|auto|none>
.bot totem <bot|party|raid> mode <auto|prepull|manual>
.bot totem <bot|party|raid> drop
.bot totem <bot> status
```

<details>
<summary>Totem parameters and modes</summary>

- **Slot Pinning:** Assign a specific totem to an element slot (`earth`, `fire`, `water`, `air`). Passing `auto` restores group-aware automatic selection (e.g. *Windfury* for melee groups, *Mana Spring* for casters). Passing `none` disables that element.
- **Casting Modes:**
  - `auto`: Bot drops totems as needed based on combat conditions (default).
  - `prepull`: Drops totems out of combat when combat is imminent (ready check / rebuff window) and refreshes them in combat.
  - `manual`: Suppresses automatic drops; totems are dropped only via `.bot totem drop` or explicit strategies.
- **Immediate Drop:** `.bot totem <scope> drop` drops all configured totems immediately.
- **Status Query:** `.bot totem <bot> status` reports current loadout and mode via `TBM:TOTEM` records.

</details>
