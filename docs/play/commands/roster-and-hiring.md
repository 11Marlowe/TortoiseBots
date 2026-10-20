---
id: play-commands-roster-and-hiring
title: Roster & Hiring Commands
category: play
summary: Commands for managing owned alts, hiring mercenary companions, and recruiter services.
tags: [commands, roster, hiring, mercenaries]
relates_to:
  - play-commands
  - play-getting-started
  - server-settings-hiring
  - server-settings-party
---

# Roster & Hiring Commands

TortoiseBots provides tools to manage owned alternate characters (alts) as bots, hire temporary mercenary companions, and customize group compositions. Inside dungeons and raids, companion bots sharing a group with a connected player do not initiate unprovoked pulls on their own; they engage only when attacked or when given an explicit pull or attack order.

## Summary

| Command | Parameters / Syntax | What It Does |
| :--- | :--- | :--- |
| [`.bot roster`](#bot-roster) | None | Displays owned character states and active CC marks. |
| [`.bot add`](#bot-add) | `<Name>` | Logs in an owned alt as an active companion bot. |
| [`.bot remove`](#bot-remove) | `<Name>` | Logs out an owned bot or dismisses a hired companion. |
| [`.bot hire`](#bot-hire) | `<class> [role] [race] [tier] [level=<n>]` | Recruits a mercenary companion from a recruiter. |
| [`.bot hire raid`](#bot-hire-raid) | `<preset\|composition> [tier]` | Recruits a full raid group of mercenary companions. |
| [`.bot role`](#bot-role) | `<Name\|self> <tank\|healer\|dps\|clear>` | Sets group role overrides for bots or the player. |
| [`.bot invite`](#bot-invite) | `<Name>` | Sends a party invite to an online owned bot. |
| [`.bot uninvite`](#bot-uninvite) | `<Name>` | Removes an owned bot or dismisses a companion. |
| [`.bot summon`](#bot-summon) | `<Name>` | Teleports an owned bot or hire to your coordinates. |
| [`.bot maintenance`](#bot-maintenance) | `<Name>` | Refreshes bot reagents, ammunition, and gear repairs. |
| [`.bot autogear`](#bot-autogear) | `<Name> [quality\|ilvl]` | Automatically upgrades gear to level-appropriate items. |
| [`.bot release`](#bot-release) | None | Commands dead companion bots to release spirit. |
| [`.bot corpse run`](#bot-corpse-run) | None | Commands spirit bots to return to their corpses. |
| [`.bot learn`](#bot-learn) | None | Commands nearby companion bots to train new spells. |
| [`.bot trade`](#bot-trade) | Target Bot | Opens a trade window with a targeted companion. |

---

## Roster and Lifecycle Commands

### `.bot roster`

Emits an authoritative snapshot of all characters on your account, detailing whether each is offline, online, or currently in your party.

```text
.bot roster
```

Also emits crowd control (CC) assignments for active group bots, listing which bot is assigned to each raid mark. Hired mercenary companions are omitted from the account character list because their characters are temporary.

### `.bot add`

Logs in an owned alternate character from your account as a headless companion bot.

```text
.bot add <CharacterName>
```

The character enters the world and can be invited to your party. To play the character yourself, simply log into it from character selection; the server cleanly logs out the headless session and logs you in.

### `.bot remove`

Logs out an active companion bot.

```text
.bot remove <CharacterName>
```

Aliases include `.bot logout`. When used on an **owned alt**, the character safely logs out and retains all progress. When used on a **hired companion**, the hire agreement ends and the temporary character is deleted immediately.

### `.bot hire`

Recruits a fresh mercenary companion from a Mercenary Recruiter NPC at an inn or major city.

```text
.bot hire <class> [role] [race] [gender] [tier] [level=<n>]
```

The server creates a new character, provisions level-appropriate spells, talents, skills, and gear, and invites it to your party. By default, hiring requires standing near a recruiter unless [AiPlayerbot.HireAnywhere](../../server/settings/hiring.md#aiplayerbothireanywhere) is enabled (1).

<details>
<summary>Hiring options and equipment tiers</summary>

- **Level Scaling:** Companions match your level by default. To hire at a specific level, add `level=<n>` (e.g. `.bot hire mage frost level=42`). Maximum level is bounded by [AiPlayerbot.HireMaxLevelAboveHirer](../../server/settings/hiring.md#aiplayerbothiremaxlevelabovehirer).
- **Equipment Tiers:** Level 60 companions can be hired in fixed Best-in-Slot gear sets (Tiers 0 through 7). Tiers unlock per character via recruiter quests: Tier 0 (Standard), Tier 1 (Pre-raid), Tier 2 (Zul'Gurub), Tier 3 (Molten Core), Tier 4 (Blackwing Lair), Tier 5 (Ahn'Qiraj), Tier 6 (Naxxramas), and Tier 7 (Karazhan).
- **Temporary Character:** Hires exist while you are online, plus a disconnect grace period ([AiPlayerbot.HireDisconnectGracePeriod](../../server/settings/hiring.md#aiplayerbothiredisconnectgraceperiod)). Removing them from your party deletes the character.

</details>

### `.bot hire raid`

Recruits an entire raid roster of mercenary companions in a single operation.

```text
.bot hire raid <preset|composition> [tier]
```

Requires Mercenary Recruiter access or `AiPlayerbot.HireAnywhere = 1`. The server converts your group to a raid if needed and batches companion creation two at a time every quarter-second to prevent server hitches.

<details>
<summary>Raid compositions and presets</summary>

- **Role Counts:** Specify counts by role (e.g. `.bot hire raid 3t 12h 24d`). Classes are selected round-robin from available faction classes.
- **Specific Classes:** Combine roles and classes (e.g. `.bot hire raid 3 warrior:tank 12 priest:healer 19 hunter 5 mage`).
- **Presets:** Use standard instance presets: `zg`, `aq20`, `onyxia`, `mc`, `bwl`, `aq40`, `naxx`. For example: `.bot hire raid mc t3`.
- **Payment:** Total fee is calculated upfront. If you lack sufficient gold, nothing is hired.

</details>

### `.bot role`

Overrides party role assignments used by the AI strategy engine to select specs and rotations.

```text
.bot role <Name> <tank|healer|dps|clear>
.bot role self <tank|healer|dps|clear>
```

- `<Name>`: Sets the role for a specific companion bot and rebuilds its combat strategy kit.
- `self`: Sets your own player role override so companion bots know whether to treat you as a tank, healer, or DPS.
- `clear`: Clears the override and restores automatic role inference based on talents and gear.

### `.bot invite`

Sends a party invitation to an online companion bot owned by your account.

```text
.bot invite <CharacterName>
```

### `.bot uninvite`

Removes a companion bot from your current party.

```text
.bot uninvite <CharacterName>
```

Aliases include `.bot kick`. If used on an owned alt while [AiPlayerbot.AltBots](../../server/settings/party.md#aiplayerbotaltbots) is enabled, the alt enters autonomous play. Removing a hired companion immediately terminates the hire and deletes the character.

### `.bot summon`

Teleports an owned bot or hired companion safely to your current location out of combat.

```text
.bot summon <CharacterName>
```

The teleportation takes 3 seconds to complete. Upon arrival, the bot automatically resumes following you unless you issue an explicit stay or guard order while the summon is in flight.

### `.bot maintenance`

Refreshes an owned bot's consumables, reagents, arrows or bullets, and repairs damaged gear.

```text
.bot maintenance <CharacterName>
```

Requires [AiPlayerbot.OwnedBotMaintenanceEnabled](../../server/settings/party.md#aiplayerbotownedbotmaintenanceenabled) to be enabled (1). Only applies to owned alts and hired companions, never random pool bots.

### `.bot autogear`

Evaluates a bot's level and automatically equips level-appropriate equipment up to configured quality and item level caps.

```text
.bot autogear <CharacterName> [quality|ilvl]
```

Requires [AiPlayerbot.OwnedBotAutogearEnabled](../../server/settings/party.md#aiplayerbotownedbotautogearenabled) to be enabled (1). Defaults to uncommon (green) gear unless specified (e.g. `blue` or `200`). Ranged weapons are selected to match existing ammunition in the bot's bags.

### `.bot release`

Commands all dead companion bots in the party to release spirit to the graveyard. Also available as `.bot action release`.

```text
.bot release
```

### `.bot corpse run`

Commands companion bots in spirit form to path back to their corpse location or dungeon portal. Also available as `.bot action corpse run`.

```text
.bot corpse run
```

### `.bot learn`

Commands companion bots standing near class trainers to purchase all available spells and ability ranks. Also available as `.bot action learn`.

```text
.bot learn
```

### `.bot trade`

Opens a direct trading window with your targeted companion bot. Also available as `.bot action trade`.

```text
.bot trade
```

### Automatic Summon on Group Accept

When [AiPlayerbot.OwnedBotSummonWhenGroup](../../server/settings/party.md#aiplayerbotownedbotsummonwhengroup) is enabled (1), owned bots and hired companions that accept your party invitation are automatically teleported to your location if they are far away on the same map.

---

## Alt-Bots: Autonomous Alts

When [AiPlayerbot.AltBots](../../server/settings/party.md#aiplayerbotaltbots) is enabled (1), your owned alternate characters can remain logged in and play autonomously in the open world.

- **Starting an Alt-Bot:** Log in an alt using `.bot add <Name>`, then remove it from your party with `.bot uninvite <Name>`. The bot announces it is now playing on its own and begins questing, grinding, traveling flight paths, and fishing like a pool bot.
- **Stopping an Alt-Bot:** Type `.bot remove <Name>` to log it out cleanly, invite it back with `.bot add <Name>`, or simply log into the character yourself from the character selection screen.
- **Autonomous Restrictions:** Autonomous alts never spend talent points, use auction houses or mail, sell soulbound or rare gear, rebind hearthstones, abandon taken quests, or roll Need on loot (they roll Greed or Pass).
- **Stranger Interactions:** Other players may invite your alt-bot and issue tactical combat orders (`follow`, `stay`, `attack`, `pull`). Strangers cannot trade with it, equip gear, change talents, or log it out.
- **Challenge Modes:** Hardcore characters cannot be used as autonomous alt-bots to prevent accidental character loss. Other challenges are permitted at player discretion.

---

## Capital Recruiter Services

The six capital city Mercenary Recruiters (Stormwind, Ironforge, Darnassus, Orgrimmar, Undercity, Thunder Bluff) offer special services for level-60 characters.

### World Buffs at Capital Recruiters

Level-60 players can purchase world buffs for themselves and nearby group members within 40 yards. Each buff unlocks permanently for your character after completing a one-time quest:

| World Buff | Unlock Requirement | Purchase Cost |
| :--- | :--- | :--- |
| **Rallying Cry of the Dragonslayer** (2 h) | Slay Onyxia or Nefarian | 10g + 2g per person |
| **Warchief's Blessing** (1 h) | Slay Rend Blackhand | 10g + 2g per person |
| **Spirit of Zandalar** (2 h) | Slay Hakkar | 10g + 2g per person |
| **Dire Maul Tribute** (2 h, all three) | Earn guard's favor in Dire Maul | 10g + 2g per person |
| **Sayge's Dark Fortune** (2 h, choice of 8) | Consult Sayge at Darkmoon Faire | 6g + 1g per person |
| **Songflower Serenade** (1 h) | Cleanse songflower in Felwood | 4g + 1g per person |
| **Traces of Silithyst** (30 min) | Defeat 5 opposing players in Silithus | 2g + 40s per person |

See [Hiring and Recruiter Settings](../../server/settings/hiring.md) for server configuration knobs governing world buff costs and availability.

### Mercenary Contracts

When [AiPlayerbot.HireGearTiersEnabled](../../server/settings/hiring.md#aiplayerbothiregeartiersenabled) is enabled (1), capital recruiters offer the **Mercenary Contracts** dialogue. This tracks seven progressive unlock quests (Tiers 1 through 7) that grant access to high-tier mercenary equipment sets:

- Boss kill counters accumulate across character runs and raid lockouts.
- Credit is granted whenever you or a grouped player within reward range defeats an eligible raid boss.
- Abandoning and retaking a contract restores previously recorded boss kill progress.
- Detailed tier equipment lists are documented in the [Hire Gear Reference](../../reference/hire-gear.md).
