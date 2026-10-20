# TortoiseBots Knowledge Base & Documentation

Welcome to the **TortoiseBots** documentation catalog. This knowledge base follows the **Open Knowledge Format (OKF)** standard, providing a structured, traversable graph for players, server operators, and AI pair programmers.

The root graph specification is declared in [`manifest.yaml`](manifest.yaml). You can validate graph integrity anytime by running `python3 tools/verify_okf.py`.

---

## 🧭 Navigation by Role

| If you are... | Start here | Purpose |
| :--- | :--- | :--- |
| **A player wanting to adventure with bots** | [**Getting Started Guide**](play/getting-started.md) | How to spawn owned bots, form a party, and run dungeons |
| **Leading a dungeon party or raid** | [**Dungeon & Raid Tactics**](play/dungeons-and-raids.md) | Corner pulling, CC markers, wipes, and instance navigation |
| **Looking for in-game commands or addon controls** | [**Player Controls & /tbm Addon**](play/commands/README.md) | Tactical intents (attack, pull, pullback, CC, AoE, formations) |
| **Curious about how a specific class plays** | [**Class Catalog Overview**](classes/overview.md) | Rotations, Turtle custom spells, and spec capabilities |
| **Wanting a living world with wandering bots & AH** | [**Living World & Autonomous Bots**](world/living-world.md) | Roaming bots, quest grinding, AH economy, and guilds |
| **Configuring bot settings or population** | [**Configuration & Tuning**](server/configuration.md) | Pacing, health/mana thresholds, and random bot pools |
| **Monitoring fleet health or debugging stuck bots** | [**Observability Dashboard**](server/dashboard.md) | Live 2D world map, Prometheus metrics, and issue tracker |
| **Developing bot AI or host integration** | [**Architecture Invariants**](developers/architecture.md) | Headless sessions, modularity rules, and C++ contracts |

---

## ⚡ Quick Task Finder (How Do I...?)

| What do you want to do? | Direct Solution & Link |
| :--- | :--- |
| **Summon lost or stuck bots to your location** | Type `.bot summon <Name>` or use the Summon button in `/tbm` → [Player Controls](play/commands/roster-and-hiring.md) |
| **Have bots learn all class spells automatically** | Whisper `/w <BotName> trainer` while standing near a class trainer → [Whisper Cheat-Sheet](play/commands/whispers.md) |
| **Make bot alts automatically match my level** | Set `AiPlayerbot.SyncAltLevelToMaster = 1` in `aiplayerbot.conf` → [QoL Configuration](server/settings/party.md) |
| **Pull a pack safely and retreat around a corner** | Target enemy and type `.bot action pullback` → [Tactical Party Actions](play/commands/party-actions.md) |
| **Prevent bots from breaking Polymorph or Freezing Trap** | Target mark and type `.bot action cc <mark>` (pets auto-disengage) → [CC Discipline](developers/bot-mechanics.md) |
| **Enable an active player economy on the Auction House** | Set `AiPlayerbot.AhMarketEnabled = 1` → [Living AH Economy](world/economy.md) |
| **Blacklist or override price of an item on the AH** | Type `.bot ah item <id> 0 0` (admin only) → [AH Admin Commands](server/admin-commands.md) |
| **Autofill missing roles for LFT dungeon runs** | On by default (`AiPlayerbot.RandomBotLftEnabled = 1`); set `0` to opt out. Your own party bots accept the offer automatically — `.bot summon` stragglers to the portal → [LFT Autofill](world/queues.md) |
| **Rebuild the bot pool after changing bot gear, skills or professions** | Set `AiPlayerbot.RandomBotPoolReset = once:<new-token>` (and `RandomBotAutoCreate = 1`), restart, then watch `bot pool status` → [Resetting the managed bot pool](server/bot-pool.md) |
| **Enroll my existing hand-made `RNDBOT*` accounts (one-time upgrade)** | At the server console: `bot pool adopt preview`, then `bot pool adopt confirm <challenge>` → [Pool Administration](server/admin-commands.md) |
| **Inspect stuck bots and live positions on a 2D map** | No Go needed: `./tools/observability/run-dashboard.sh` (or `.ps1` on Windows), then open `http://localhost:8095` → [Observability Dashboard](server/dashboard.md) |
| **Understand Turtle custom spells (Viper, Holy Strike, Carve)** | Check individual class deep-dives or census TSV → [Class Overview](classes/overview.md) & [Spell Coverage](developers/data/spell-coverage.tsv) |
| **Port a new feature or fix a bug cleanly** | Follow headless session invariant & donor hierarchy → [Architecture Invariants](developers/architecture.md) & [Donor Hierarchy](developers/donor-hierarchy.md) |

---

## ⚔️ Class AI & Rotations Catalog

Detailed combat rotations, Turtle WoW 1.18.1 custom abilities, pet handling, and talent progress for all nine classes:

| Class | Primary Roles | Key Features & Custom Content |
| :--- | :--- | :--- |
| **[Warrior](classes/warrior.md)** | Tank (Prot), Melee DPS (Arms/Fury) | Stance dancing, taunt priority, *Master Strike* (54023), *Defensive Tactics* |
| **[Paladin](classes/paladin.md)** | Healer (Holy), Tank (Prot), Melee DPS (Ret) | Aura/Blessing coordination, *Holy Strike* (679), *Bulwark of the Righteous* (51346) |
| **[Hunter](classes/hunter.md)** | Ranged DPS (BM/MM), Melee DPS (Survival) | Pet feeding/CC safety, *Aspect of the Viper* (45651) regen, *Carve*, *Lacerate* |
| **[Rogue](classes/rogue.md)** | Melee DPS (Combat, Assassination, Subtlety) | Stealth openers, poison upkeep, *Surprise Attack*, *Noxious Assault*, *Envenom* |
| **[Priest](classes/priest.md)** | Healer (Holy/Disc), Ranged DPS (Shadow) | Healing ladder, Weakened Soul refusal, *Chastise* (51478), *Ascendance* |
| **[Shaman](classes/shaman.md)** | Healer (Resto), Melee DPS (Enh), Caster (Ele) | 4-element totem sets, *Earthquake* (48306), *Spirit Link*, *Bloodlust* |
| **[Mage](classes/mage.md)** | Ranged DPS (Frost, Fire, Arcane) | *Polymorph* CC, *Counterspell*, *Arcane Power* safety gate, *Evocation* cancel |
| **[Warlock](classes/warlock.md)** | Ranged DPS (Affliction, Demonology, Destruction) | DoT spread, demon summons, Soul Shard harvest, *Dark Harvest*, *Power Overwhelming* |
| **[Druid](classes/druid.md)** | Tank (Bear), Melee (Cat), Healer (Resto), Caster | Form maintenance, *Tree of Life Form* (45705), *Berserk*, *Swiftmend* gating |

---

## 📚 Guides & Operations

- [**Getting Started**](play/getting-started.md) — Spawning owned bots, account ownership, inviting, and dungeon basics.
- [**Dungeon & Raid Tactics**](play/dungeons-and-raids.md) — Field manual for clearing 5-player dungeons: corner pulling, LoS, CC discipline, and wipes.
- [**Living World & Autonomous Bots**](world/living-world.md) — Wandering bots, quest grinding, AH trading, guilds, and battlegrounds.
- [**Player Controls**](play/commands/README.md) — Roster vs. Actions, `.bot action` command intents, and the `TBM:` addon protocol.
- [**Configuration & Tuning**](server/configuration.md) — Plain-English explanation of `aiplayerbot.conf` and `tortoise_bots.conf` settings.
- [**Observability & Dashboard**](server/dashboard.md) — Go telemetry daemon, Prometheus metrics, and live browser dashboard.

---

## 🏛️ Concepts & Architecture

- [**Architecture Invariants**](developers/architecture.md) — Non-negotiable rules: decoupled C++, headless sessions, zero core coupling.
- [**Strategy Engine Lifecycle**](developers/strategy-engine.md) — Triggers, actions, multipliers, reaction queues, and failure backoffs.
- [**Bot Mechanics, Quirks & Gaps**](developers/bot-mechanics.md) — Targeting math, threat distribution, movement, interrupts, and quirks.
- [**Donor Hierarchy & Porting Rules**](developers/donor-hierarchy.md) — Shyalya runtime parity rules vs mod-playerbots behavior rules.
- [**Known Limitations & Non-Goals**](play/known-limitations.md) — Current engine boundaries, blocked features, and deliberate non-goals.

---

## 🗄️ Reference & Specifications

- [**Host API Contract**](developers/host-api.md) — Technical C++ host boundary seams, session lifecycle, and packet bridges.
- [**Player Control Catalog**](play/commands/README.md) — Technical command delivery specification and addon serialization.
- [**License Audit**](developers/licensing-audit.md) — Donor license compatibility analysis and release gates.
- [**Hire Gear Sets**](reference/hire-gear.md) — Generated static level-60 hire sets per class/spec/tier.
- [**Spell Coverage Reference**](developers/data/spell-coverage.tsv) — Census of custom Turtle WoW spells mapped into AI strategies.
- [**Talent Builds Reference**](developers/data/talent-builds.tsv) — Level 10–60 preset build progression rationale.
- [**Donor Capability Census**](developers/data/capabilities.tsv) — Granular inventory of donor features vs TortoiseBots implementation.
