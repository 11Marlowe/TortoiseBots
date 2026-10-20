---
id: play-commands-addon
title: TortoiseBotsManager Addon (/tbm)
category: play
summary: Installation, GUI features, and player transport protocol for the TortoiseBotsManager addon.
tags: [addon, tbm, gui, player-controls]
relates_to:
  - play-commands
  - play-getting-started
  - play-commands-bot-panel
---

# TortoiseBotsManager Addon (/tbm)

The **[TortoiseBotsManager](https://github.com/Sagiroth/TortoiseBotsManager)** (`/tbm`) addon provides an integrated graphical interface for managing companion bots in Tortoise WoW. It allows players to spawn owned alts, issue combat commands, inspect gear, and adjust behavior toggles without typing console commands.

## Installation & Setup

1. Download the latest release from the [TortoiseBotsManager GitHub repository](https://github.com/Sagiroth/TortoiseBotsManager).
2. Extract the folder into your game client's addon directory:
   ```text
   World of Warcraft/Interface/AddOns/TortoiseBotsManager
   ```
3. Ensure the folder is named exactly `TortoiseBotsManager` (not nested inside a duplicate folder).
4. Log into the game and type `/tbm` in chat to toggle the control interface.

---

## Addon Panels & Features

The interface is divided into functional tabs designed for different stages of gameplay:

### Roster Tab

- **Character List:** Lists all characters on your account, displaying level, class, and whether they are offline, online, or already in your party.
- **Login & Logout:** Click **Login** to spawn an owned alt as a headless companion bot; click **Logout** to log it out cleanly.
- **Invite & Summon:** Group with online alts or summon them directly to your location with one click.
- **Alt-Bot Indicators:** Highlights alts playing autonomously in the world, allowing you to reclaim them or log them out remotely.

### Party Tab

- **Movement Controls:** Quick buttons for `.bot action follow`, `.bot action stay`, `.bot action come`, and `.bot action flee`.
- **Combat Orders:** Direct buttons to issue `attack`, `pull`, and `pullback` maneuvers with configurable pull delay sliders.
- **Crowd Control:** Assign crowd control raid marks (Moon, Star, Diamond, etc.) to capable party bots.
- **Live Toggles:** Quick switches for party-wide AoE, corpse looting, consumable mimicry, and smart auto CC.

### Bot Panel Tab

- **Gear Inspector:** View equipped items and durability across all paperdoll slots on owned alts and hired companions.
- **Bag Management:** Browse carried inventory items, identify equippable upgrades, and equip or unequip items remotely.
- **Behavior Toggles:** Configure individual sliders for threat backoff, mana conservation, offensive cooldowns, and potion usage.
- **Totem Manager:** For shaman companions, pin specific totem choices per element and trigger immediate totem drops.

### Guild Tab

- Displays claimed guild bots when the guild bot feature is enabled on the server, detailing bot locations, levels, and grouping state.

---

## Addon Command Transport (Player View)

The addon communicates with the server over private addon communication channels rather than standard chat channels.

- **Silent Operation:** Commands sent by clicking addon buttons do not print `.bot` text into your chat frame, and server responses do not clutter your chat log.
- **Chat Fallback:** If the private addon channel becomes unavailable (for example, while ungrouped or on an older server build), the addon automatically falls back to sending standard `.bot` chat commands.
- **Execution Order:** Addon commands execute on the server's next world tick, immediately before bots take their actions. Commands run strictly in the order they reach the server.
- **Movement Conflict Resolution:** A newer movement order always overrides an earlier pending order. For example, typing `/p stay` followed immediately by clicking **Follow** in the addon cancels the stay and results in follow.
- **Rate Limiting:** To prevent server desynchronization, the server accepts up to 128 bot commands per player per world tick. Any excess commands within that tick are dropped with a warning.

---

## Synchronization Streams

The module transmits structured protocol lines (prefixed with `TBM:`) to keep the addon interface synchronized with server state:

| Protocol Stream | Description |
| :--- | :--- |
| `TBM:TRANSPORT` | Reports whether private addon communication is active (`party` or `none`). |
| `TBM:BOTSTATE` | Streams movement modes (`follow`, `stay`, `guard`, `free`) and live toggle states. |
| `TBM:CC_ASSIGN` | Broadcasts active crowd control mark assignments across party bots. |
| `TBM:CAPS` | Advertises server feature capabilities (e.g. pull sliders, flee, inventory, mimic). |
| `TBM:ALTBOTS` | Lists owned characters currently active as autonomous alt-bots. |
| `TBM:VERSION` | Handshakes the server module version and build date with the client addon. |
| `TBM:VIEW` | Streams real-time tactical perception frames when Remote Commander view is active. |

Hand-typed `.bot` commands in chat continue to work identically alongside the addon.
