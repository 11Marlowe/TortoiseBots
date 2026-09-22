---
id: guide-configuration-tuning
title: Configuration Knobs & Feature Flags
category: guides
summary: Complete guide to customization knobs, player quality-of-life flags, world immersion settings, and in-game tactical toggles.
tags: [guide, config, tuning, settings, feature-flags, knobs]
relates_to:
  - guide-getting-started
  - guide-player-controls
  - guide-observability-dashboard
  - concept-strategy-engine
---

# Configuration Knobs & Feature Flags

TortoiseBots provides a rich set of feature flags and tuning knobs. Whether you are running a solo private server or hosting a community realm, these settings allow you to customize bot intelligence, party convenience, world immersion, and economy.

Configuration lives in two files:
1. `conf/tortoise_bots.conf` — Native module options, diagnostic levels, and telemetry.
2. `conf/aiplayerbot.conf` — Gameplay feature flags, QoL toggles, AI thresholds, and services.

---

## 1. Player Quality-of-Life (QoL) Flags

These settings dramatically enhance the solo or small-group experience with owned bots:

| Setting | Default | Recommended | What It Does |
| :--- | :---: | :---: | :--- |
| `AiPlayerbot.SyncAltLevelToMaster` | `0` | **`1`** | **Auto-Level Bot Alts:** When enabled, all bot characters on your account automatically level up to match your main character's level as you progress. |
| `AiPlayerbot.BoostFollow` | `0` | **`1`** | **Mount Up to Catch Up:** Bots far behind the leader (beyond react distance) trigger a mount check so they ride to catch up instead of trailing on foot. No speed hack; combat-safe. |
| `AiPlayerbot.NonGmFreeSummon` | `0` | **`1`** | **Unrestricted Summoning:** Allows regular players without GM status to use `.bot summon` to gather their bots out of combat anywhere. |
| `AiPlayerbot.AutoLearnQuestSpells` | `1` | **`1`** | **Class Quest Rewards:** Automatically teaches spells awarded by completed class quests (e.g. Paladin Resurrection, Warlock pet summons, Shaman totems). |
| `AiPlayerbot.AutoLearnTrainerSpells` | `0` | **`0`** | **Free Trainer Spells (random pool only):** When on, random bots learn every green-eligible trainer spell on level-up (Dual Wield at live data level, rank upgrades, poisons). When off, no free sweep runs — but paid trainer visits with gold still teach. Owned bots never get free spells either way. |
| `AiPlayerbot.AutoLearnDroppedSpells` | `0` | **`0`** | **Level-60 Book Spells (random pool only):** Teaches dungeon/raid book spells the bot reached the level for. Same random-only scope as the trainer sweep. |
| `AiPlayerbot.RollBadItemsWithPlayer` | `0` | **`1`** | **Need on Empty Slots:** Forces party bots to roll Need on dungeon drops if their corresponding equipment slot is empty or severely under-leveled. |
| `AiPlayerbot.RandomGearUpgradeEnabled` | `1` | **`1`** | **Automatic Gear Scaling:** Periodically equips bots with level-appropriate dungeon and quest gear as they level up. |
| `AiPlayerbot.GenerateItemCaches` | `1` | **`1`** | **First-Boot Gear Caches:** Builds the `ai_playerbot_equip_cache` and `ai_playerbot_rnditem_cache` tables once, while they are empty, and loads them from the database afterwards. Leave it on for a fresh install — with empty caches bots only fill empty slots from loot and never judge an upgrade. |

The spec weights these caches are scored with come from the `ai_playerbot_weightscales` and `ai_playerbot_weightscale_data` tables, seeded by `data/sql/world/20260916090001_world.sql`. If bots wear wrong-slot gear from their bags but never swap an upgrade in, that dataset is empty — re-apply the migration and restart.
---

## 2. World Population & Immersion Flags

These flags control the behavior of autonomous random bots roaming the world:

| Setting | Default | Recommended | What It Does |
| :--- | :---: | :---: | :--- |
| `AiPlayerbot.RandomBotAutologin` | `0` | **`1`** | Automatically logs in random bots (`RNDBOT*`) at server startup. |
| `AiPlayerbot.RandomBotAutoCreate` | `0` | **`1`** | Automatically creates new bot accounts/characters if the active pool is below `MinRandomBots`. |
| `AiPlayerbot.MinRandomBots` / `MaxRandomBots` | `0` | `50` / `150` | Sets the minimum and maximum active random bot population. |
| `AiPlayerbot.RandomBotStartLevelMin` / `Max` | `1` / `1` | `1` / `1` (raise for test pools) | **Fresh-Bot Level Seed:** A newly created pool bot gets a random level in this range once, on its first login, before its skills, professions and starter gear are seeded. Useful for running a pool that starts at a playable level (e.g. `10` / `15`) instead of walking up from 1. Set both to 1 to keep the historic behaviour. |
| `AiPlayerbot.RandomBotMaxLevel` | `60` | `40` (test pools) | Caps the gear/item tables random bots roll from; it does not assign bot levels. |
| `AiPlayerbot.RandomBotInvitePlayer` | `1` | `1` | Random bots in the open world will invite solo human players to form questing groups. |
| `AiPlayerbot.RandomBotGroupNearby` | `1` | `1` | Bots will organically invite each other to form questing parties and dungeon groups. |
| `AiPlayerbot.RandomBotFormGuild` | `1` | `1` | Bots will buy guild charters, collect signatures from other bots, and found their own guilds. |
| `AiPlayerbot.EnableGreet` | `1` | `1` | Bots wave or say hello when passing players in towns and roads. |
| `AiPlayerbot.RandomBotShowHelmet` / `ShowCloak`| `1` | `1` | Renders helmets and cloaks on bots. |
| `AiPlayerbot.RandomBotSayWithoutMaster` | `1` | `0` on quiet servers | Masterless bots say in `/s` what they would whisper to an owner (travel plans, cast failures). `0` keeps them silent unless owned. Needs restart. |
| `AiPlayerbot.AllowIsolatedCustomStartingZones` | `0` | `0` | When 0, blocks random bots from custom isolated starter zones (Blackstone Island, Thalassian Highlands, Alah'Thalas) and normalizes them to mainland starter zones. |
| `AiPlayerbot.HireEnabled` | `1` | `1` | Master switch for on-demand companion hiring (`.bot hire` + `<Mercenary Hire>` inn recruiters). |
| `AiPlayerbot.HireMinAccountSecurity` | `0` | `0` | Minimum account security that may hire (`0` = everyone). |
| `AiPlayerbot.HireMaxBotsPerPlayer` | `4` | `4` (up to `39` for 40-man raids) | Cap on hired companions per player. Party hires stop at 4; raise toward 39 with a raid group. |
| `AiPlayerbot.HireRequiresResting` | `1` | `1` | `.bot hire` requires resting; recruiter gossip skips the check (presence is proof). |
| `AiPlayerbot.HireBaseCostCopper` / `HirePartyMult2/3/4` | `15000` / `1.66` / `2.66` / `4.66` | defaults | Level-scaled party cost curve (~15g total for 4 bots at 60). Set base to `0` for free hiring. |
| `AiPlayerbot.HireRaidFlatCostCopper` | `10000` | `10000` | Flat per-bot rate for raid hires 5+ at 60 (1g). |
| `AiPlayerbot.HireDisconnectGracePeriod` | `300` | `300` | Seconds a hired companion guards after its master disconnects before dismissing. |

---

## 3. Autonomous Services (Default OFF)

All autonomous services are fully bounded and disabled by default. Enable only the services you need:

| Service Flag | Default | Description |
| :--- | :---: | :--- |
| `AiPlayerbot.RandomBotLftEnabled = 1` | `0` | **LFT Dungeon Autofill:** When real players queue for Looking-For-Trouble dungeons and wait for missing roles (e.g. Tank or Healer), eligible bots fill the vacant slots and run the instance. |
| `AiPlayerbot.AhMarketEnabled = 1` | `0` | **Living Auction House:** Bots post gathered trade goods and bind-on-equip gear on the Auction House, and bid on/buyout items using real player pricing models. |
| `AiPlayerbot.RandomBotBgEnabled = 1` | `0` | **Battleground Auto-Queue:** Injects random bots into Warsong Gulch, Arathi Basin, and Alterac Valley when human players queue. |

---

## 4. Combat & Reaction Thresholds

Fine-tune how aggressively bots heal, rest, or drink:

| Setting | Default | Tuning Guidance |
| :--- | :---: | :--- |
| `AiPlayerbot.CriticalHealth` | `25` | Percent health considered an emergency. Triggers *Lay on Hands*, *Last Stand*, *Shield Wall*, or *Divine Shield*. |
| `AiPlayerbot.LowHealth` | `50` | Percent health triggering prioritized heavy heals (*Greater Heal*, *Healing Wave*). Increase to `65` for safer dungeon runs. |
| `AiPlayerbot.MediumHealth` | `70` | Threshold for maintenance heals (*Renew*, *Rejuvenation*, *Flash Heal*). |
| `AiPlayerbot.AlmostFullHealth` | `90` | Health ceiling above which bots stop casting heals to conserve mana. |
| `AiPlayerbot.LowMana` | `20` | Mana floor where casters switch to low-cost wanding or conserve mana. |
| `AiPlayerbot.MediumMana` | `50` | Threshold where bots consider conservative rotations. |

---

## 5. In-Game Live Toggles (On the Fly via `/tbm` or Chat)

You do not need to restart the server to adjust tactical behavior during gameplay. You can toggle these anytime:

### Tactical Gameplay Toggles (`.bot action <toggle>`)
- `.bot action aoe <on|off>` — Enables or disables Area-of-Effect abilities (crucial around crowd-controlled mobs).
- `.bot action pullback` — Orders the tank to pull the target and sprint back to your location.
- `.bot action focus` — Focuses party damage onto the mob marked with the Skull raid marker.
- `.bot action cc <mark>` — Assigns CC to a specific raid marker (e.g. Moon, Star).

### Strategy & Stance Toggles
Using `.bot strategy <change> [Name]` (change first, optional bot name second) or the `/tbm` addon:
- `.bot strategy +conserve mana` / `.bot strategy -conserve mana` — Restricts casters to basic, high-efficiency spells.
- `.bot strategy +loot` / `.bot strategy -loot` — Toggles whether bots run to loot corpses after combat.
- `.bot strategy +silent` / `.bot strategy -silent` — Silences bot chatter in party/say chat so they execute commands quietly.
- `.bot strategy -passive` — Halts all bot attacks; bots will only follow and hold fire.
- `.bot formation <arrow|line|circle|shield>` — Changes follow positioning around the leader.

---

## 6. Diagnostic Logging

The native module layer (bot lifecycle, random-bot, Auction House, battleground queue, and LFT services — everything under `host/` and `runtime/`) has its own verbosity setting, independent of the core server's own `LogLevel`. This lets you trace what the module is doing without turning on the engine's full debug output, and vice versa.

```ini
[TortoiseBotsConf]
TortoiseBots.LogLevel = 2
```

| Level | Name | Shows |
| :---: | :--- | :--- |
| `0` | Minimal | Errors only. |
| `1` | Basic | One-off startup, shutdown, and diagnostic test results (e.g. `PendingAddRemoveTest`, `AutoTest`). |
| `2` | Detail (default) | Per-bot state transitions: session start/stop, add/remove, AH postings, BG queue entries. |
| `3` | Debug | Per-tick and per-packet traces. High volume — intended for short diagnostic sessions, not left on. |

Errors (`sLog.outError`) are always written regardless of this setting. The level is re-read on `.reload config`, so it can be raised or lowered without a server restart.

This setting is separate from the strategy AI's own action trace, which stays gated behind the `debug`/`debug action` bot strategies (`.bot strategy +debug`) rather than a server-wide config key.

---

## 7. Bot Chatter & Broadcasts

Flavor and status lines come from the `ai_playerbot_texts` table (seeded by
`data/sql/world/20260913090000_world.sql`). If bots speak raw keys such as
`quest_accepted`, that table is empty — re-apply the migration and restart.
Volume is controlled by these knobs (all need a restart):

| Setting | Default | What It Does |
| :--- | :---: | :--- |
| `AiPlayerbot.EnableBroadcasts` | `1` | Master switch. `0` disables all quest/loot/kill/level-up/suggest broadcasts. |
| `AiPlayerbot.BroadcastToWorldGlobalChance` / `BroadcastToGeneralGlobalChance` | `3000` | Main throttle on what reaches world/general chat (range `0`-`30000`). `0` re-routes most broadcasts away from that channel. |
| `AiPlayerbot.BroadcastChance*` | varies | Per-event chance, e.g. `BroadcastChanceQuestAccepted`, `BroadcastChanceSuggestSell`. `0` disables that one class. Toxic/scam lines (`*Toxic*`, `*Thunderfury*`) ship at `0` already. |
