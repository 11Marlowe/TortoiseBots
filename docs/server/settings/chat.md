---
id: server-settings-chat
title: Chat & broadcast settings
category: server
summary: Configuration settings for bot chatter, world/general channel broadcasts, and autonomous speech.
tags: [config, chat, broadcasts, immersion]
relates_to:
  - world-social
  - server-settings-world-population
---

# Chat & Broadcast Settings

These settings configure bot communication volume, public channel broadcasts, and autonomous chat chatter. Settings go in `aiplayerbot.conf`; see [How configuration works](../configuration.md).

Bot speech lines and flavor texts are stored in the `ai_playerbot_texts` database table (seeded by `data/sql/world/20260913090000_world.sql`). If bots output raw text keys like `quest_accepted` instead of spoken sentences, re-apply the database migration.

## Summary

| Setting | Default | What it does |
| :--- | :---: | :--- |
| [`AiPlayerbot.EnableBroadcasts`](#aiplayerbotenablebroadcasts) | `1` | Master switch controlling automated public chat, channel announcements, and emotes. |
| [`AiPlayerbot.BroadcastToWorldGlobalChance`](#aiplayerbotbroadcasttoworldglobalchance) | `3000` | Global frequency throttle for broadcasts routed to the server World channel. |
| [`AiPlayerbot.BroadcastToGeneralGlobalChance`](#aiplayerbotbroadcasttogeneralglobalchance) | `3000` | Global frequency throttle for broadcasts routed to zone General channels. |
| [`AiPlayerbot.BroadcastChance*`](#aiplayerbotbroadcastchance) | varies | Per-event broadcast chance multipliers for specific quest, kill, or joke lines. |
| [`AiPlayerbot.BroadcastChanceSuggestSell`](#aiplayerbotbroadcastchancesuggestsell) | `0` | Probability of bots advertising items for sale in trade and public channels. |
| [`AiPlayerbot.BroadcastChanceSuggestThunderfury`](#aiplayerbotbroadcastchancesuggestthunderfury) | `1` | Controls whether bots participate in the classic Thunderfury chat joke. |
| [`AiPlayerbot.RandomBotSayWithoutMaster`](#aiplayerbotrandombotsaywithoutmaster) | `0` | Causes masterless bots to speak travel goals and cast errors in say chat. |

## Settings

### `AiPlayerbot.EnableBroadcasts`
Master switch for all autonomous bot speech, channel announcements, and roleplay chatter.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Controls all automated public broadcasts from bots:

- `1`: Bots post milestone announcements (quest completions, boss kills, level-ups), trade suggestions, guild recruiting messages, and scripted battleground callouts.
- `0`: Completely silences automated public chat. Bots will not speak in `/say`, `/yell`, guild channels, or global channels.

Direct communications are not broadcasts and remain active even when set to `0`:
- Private whispers to players.
- Party and raid chat with grouped players.
- Responses to player commands (such as `.bot` queries or `speak` orders).
- Addon communication protocols with the TortoiseBotsManager addon.

</details>

### `AiPlayerbot.BroadcastToWorldGlobalChance`
Global volume throttle controlling how often bot announcements reach the World channel.
**Default:** `3000` · **Values:** integers (0..30000) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Acts as a master probability gate for messages destined for the public World channel. A value of `0` redirects or suppresses all automated bot messages from World chat, leaving the channel clear for human conversation. Higher values allow more milestone broadcasts to show.

</details>

### `AiPlayerbot.BroadcastToGeneralGlobalChance`
Global volume throttle controlling how often bot announcements reach zone General channels.
**Default:** `3000` · **Values:** integers (0..30000) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Acts as a probability gate for bot broadcasts sent to zone-wide General channels. Set to `0` to keep regional General channels completely quiet.

</details>

### `AiPlayerbot.BroadcastChance*`
Per-event probability multipliers for individual speech triggers and announcement categories.
**Default:** varies · **Values:** integer weights · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Allows fine-grained tuning of specific announcement types (such as `BroadcastChanceQuestAccepted` or `BroadcastChanceKillCreature`). Setting any specific event key to `0` silences that category of broadcast while leaving other announcements active.

Aggressive or toxic flavor lines default to `0` in shipped templates.

Travel destination narration ("Traveling 120y to...") and vendor transaction confirmations are permanently silent for masterless bots, preventing spam in busy cities.

</details>

### `AiPlayerbot.BroadcastChanceSuggestSell`
Frequency at which pool bots advertise goods for sale in trade and city chat.
**Default:** `0` · **Values:** integer weight (0 = disabled) · **Restart:** needed

<details>
<summary>How it works in detail</summary>

When greater than `0`, bots will periodically hawk items in Trade and General channels (posting "WTS" lines). Disabled by default (`0`) to prevent public trade chat spam.

Direct player inquiries and native Auction House sales operate independently of this setting.

</details>

### `AiPlayerbot.BroadcastChanceSuggestThunderfury`
Toggles bot participation in the classic Thunderfury channel chat joke.
**Default:** `1` · **Values:** `0` off, `1` on · **Restart:** needed

<details>
<summary>How it works in detail</summary>

Controls whether bots periodically link Thunderfury, Blessed Blade of the Windseeker in public chat channels when triggered. Set to `0` to silence the joke.

</details>

### `AiPlayerbot.RandomBotSayWithoutMaster`
Directs masterless pool bots to speak status messages and travel destinations in local say chat.
**Default:** `0` · **Values:** `0` silent, `1` speak in /say · **Restart:** needed

<details>
<summary>How it works in detail</summary>

By default (`0`), masterless pool bots wander silently, and only whisper notifications (such as out-of-mana warnings or pathing obstacles) to a live player master when grouped.

Setting this to `1` causes masterless bots to vocalize these internal notifications aloud in `/say`. This is primarily useful for local debugging or immersive spectator viewing.

</details>
