---
id: world-social
title: Social interaction & guilds
category: world
summary: How bots greet players, form groups, create autonomous guilds, and trade items.
tags: [world, social, groups, guilds, trading, greetings]
relates_to:
  - world-living-world
  - play-commands-roster-and-hiring
  - server-settings-world-population
---

# Social interaction & guilds

This page describes how autonomous bots interact socially with players and other bots, form groups, create guilds, and trade items. Social and population settings are configured in `aiplayerbot.conf`; see [World population & immersion settings](../server/settings/world-population.md).

## Greetings & emotes

In the open world and towns, bots add immersion by acknowledging passing players:

- **Greeting players:** A bot passing a human player greets them with a friendly text emote or say message, firing at most once per encounter. By default only bots with a live master greet; pool-wide greeting is opt-in ([`AiPlayerbot.RandomBotGreet`](../server/settings/world-population.md#aiplayerbotrandombotgreet)).
- **Bot-to-bot silence:** Bots never greet other bots. With hundreds of bots active in shared zones, bot-to-bot greetings would flood local chat channels.
- **Encounter cooldown:** After greeting a player, the bot enters a cooldown before it can greet that same player again, preventing repetitive emotes if the player walks in and out of range.
- **Settings:** Mastered greetings are enabled by [`AiPlayerbot.EnableGreet`](../server/settings/world-population.md#aiplayerbotenablegreet) and paced by [`AiPlayerbot.GreetCooldown`](../server/settings/world-population.md#aiplayerbotgreetcooldown).

## Grouping & party invites

Autonomous pool bots can participate in cooperative grouping:

| Feature | Behavior | Control |
| :--- | :--- | :--- |
| **Inviting solo players** | Bots invite nearby un-grouped players working on the same quest or grinding the same camp. | [`AiPlayerbot.RandomBotInvitePlayer`](../server/settings/world-population.md#aiplayerbotrandombotinviteplayer) |
| **Bot-to-bot parties** | Disabled by default; pool bots stay solo to avoid leashing each other into idle follow chains. | [`AiPlayerbot.RandomBotGroupNearby`](../server/settings/world-population.md#aiplayerbotrandombotgroupnearby) |
| **Elite & boss veto** | Bots only veto pulls if actively about to leave (bags 80%+ with town errand in overworld). | See [Dungeons & raids](../play/dungeons-and-raids.md). |
| **Conjured item sharing** | Mages out of combat conjure food and water and trade them to low-mana group members. | See [Sustenance](living-world.md#sustenance--resting). |

When sending party invites, bots announce the invitation in local say chat if [`AiPlayerbot.InviteChat`](../server/settings/world-population.md#aiplayerbotinvitechat) is enabled.

<details>
<summary>Player invites & DND in detail</summary>

When [`AiPlayerbot.RandomBotInvitePlayer = 1`](../server/settings/world-population.md#aiplayerbotrandombotinviteplayer) is active, an autonomous bot questing or grinding in the open world will offer a party invite to a nearby human player who is hunting the same objective.

If you prefer to play solo without interruptions, set your chat status to `/dnd`. Bots detect the `/dnd` status flag and suppress outgoing party invitations to that player.

</details>

<details>
<summary>Bot-to-bot grouping & follow dynamics in detail</summary>

Controlled by [`AiPlayerbot.RandomBotGroupNearby`](../server/settings/world-population.md#aiplayerbotrandombotgroupnearby). When disabled, pool bots decline invitations from other bots and quest independently. This prevents bot leaders from leashing followers into unproductive travel loops.

Enabling the setting allows organic grouping among bots, where they band together to take on difficult elite quests, group objectives, and dungeon entrances.

</details>

<details>
<summary>Instance stability & vendor trips during party play in detail</summary>

In an active group, elite and boss pulls are protected against disruption. A bot will only veto or delay an encounter if it is actively leaving for town (bags over 80% full with an active vendor errand in the open world).

Inside dungeons or raids, bots never leave the instance to visit a vendor. Party members remain focused on the instance until the group finishes or disbands.

</details>

## Autonomous guild formation

Bots establish player-like social organizations across the realm:

- **Guild charters & petition signing:** Unguilded bots periodically travel to capital city guild masters, purchase a Guild Charter, and solicit signatures from nearby unguilded bots. Bots approached for a signature automatically inspect the petition and sign it if unguilded.
- **Guild founding:** Once required signatures are collected, the bot registers the charter, generates an organic guild name, and displays the guild tag above members' heads.
- **Member recruitment & caps:** Founded guilds actively invite both unguilded bots and interested human players to join. Total guild membership is capped by [`AiPlayerbot.GuildMaxBotLimit`](../server/settings/world-population.md#aiplayerbotguildmaxbotlimit); if membership exceeds this cap, bots leave autonomously.
- **Guild chat & socialization:** Bots share achievements, quest milestones, and chatter in guild chat when automated broadcasts are enabled ([`AiPlayerbot.EnableBroadcasts`](../server/settings/chat.md#aiplayerbotenablebroadcasts)).
- **Whisper guild management:** Human players in bot guilds can issue direct roster commands by whispering bot officers and guild leaders (`guild invite`, `guild join`, `guild leave`, `guild promote`, `guild demote`, `guild leader`, `guild remove`; see [Whisper Commands](../play/commands/whispers.md#guild-management)).
- **Configuration:** Guild creation is controlled by [`AiPlayerbot.RandomBotFormGuild`](../server/settings/world-population.md#aiplayerbotrandombotformguild).
- **Reset protection:** Guilds containing human players are protected against automated pool resets. If a bot-led guild includes a real player, server pool resets abort safely to prevent orphaned players. For details, see [Pool reset safety guarantees](../server/bot-pool.md#safety-guarantees--multi-phase-execution).

## Trading with pool bots

Players can interact with unmanaged pool bots through the trade window:

- **Trade permissions:** Direct trading with masterless pool bots is governed by [`AiPlayerbot.PoolBotTradeMode`](../server/settings/world-population.md#aiplayerbotpoolbottrademode):
  - `0`: Off. Pool bots decline all incoming trade requests from players.
  - `1`: Trusted. Trades are accepted only from a real master, group members, or hire masters.
  - `2`: BuyOnly. Bots accept items and gold from players but give nothing in return.
  - `3`: SellOnly. Bots provide items or gold to players but accept nothing in return.
  - `4`: On. Unrestricted trading with players.
- **Addon prefix suppression:** Addon communication prefixes (such as damage meter synchronization) are filtered through [`AiPlayerbot.TradeActionExcludedPrefixes`](../server/settings/world-population.md#aiplayerbottradeactionexcludedprefixes) to prevent unintended trade rejections.
