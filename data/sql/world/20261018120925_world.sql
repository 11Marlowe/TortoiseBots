-- Boss tactics: phase 1a rows for the Turtle-custom bosses of Deadmines (36),
-- Wailing Caverns (43) and Frostmane Hollow (822).
--
-- Same table and loader as the Uldaman rows (see data/sql/world/20261014120000_world.sql
-- and docs/guides/boss-tactics.md). Every creature and spell below was read from Turtle's
-- own world data (tortoise-wow sql/base and sql/database_updates: creature_template,
-- creature_spells, creature_ai_events/scripts, spell_template; src/scripts/dungeons).
--
-- Idempotent: each DELETE removes only this migration's boss rows, then they go back in. The
-- core runs each migration file inside one transaction. Never edit this file once applied;
-- add a newer migration.

DELETE FROM `ai_boss_tactic` WHERE `map` = 36 AND `boss_entry` IN (61961);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Jared Voss (61961, EventAI, spell list 180300): Poison Cloud 3815 (7-7 s first, every
-- 16-26 s) is a cast with a cast time (castingTimeIndex 4) and a periodic-damage cloud
-- centred on the caster (target type 22, self); the other three list spells (Poison 13298,
-- Withering Poison 13884, Poisonous Stab 7357) are instant (castingTimeIndex 1), so they are
-- not listed. EventAI is say-on-aggro/death only (events 2200058/2200059).
(36, 61961, 'interrupt', 0, 3815, 0, 0, 'Jared Voss: Poison Cloud (spell list 180300)');

DELETE FROM `ai_boss_tactic` WHERE `map` = 43 AND `boss_entry` IN (61968);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Zandara Windhoof (61968, EventAI, spell list 180306): Chain Lightning 15659 (every 10 s)
-- and Gust of Wind 6982 (4-4 s first, every 11-14 s) are both casts with a cast time
-- (castingTimeIndex 14 and 5); Chain Lightning is the damage, Gust of Wind is an AoE stun
-- (effectApplyAura 12). EventAI is say-on-aggro/death only (events 2200062/2200063).
(43, 61968, 'interrupt', 0, 15659, 0, 0, 'Zandara Windhoof: Chain Lightning (spell list 180306)'),
(43, 61968, 'interrupt', 0, 6982, 0, 0, 'Zandara Windhoof: Gust of Wind AoE stun (spell list 180306)');
-- Vangros (61965, no ai_name/script, spell list 180303) gets no row: Crush Armor 21055 and
-- Knock Away 18670 are both instant (castingTimeIndex 1, interruptFlags 0), so there is no
-- cast to interrupt; Knock Away is a single-target threat-drop, not a frontal cone.

DELETE FROM `ai_boss_tactic` WHERE `map` = 822 AND `boss_entry` IN (63129, 63133, 63130);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Kanza the Seer (63129, EventAI, spell list 63129): Frostbolt 837 (5-10 s first, every
-- 6-10 s) is a real cast (castingTimeIndex 24, cast time table not in the repo; treated as a
-- real cast because interruptFlags 15). Blizzard 6141 is a channel, never seen by the
-- interrupt scan (it reads CURRENT_GENERIC_SPELL only), so it has no row. EventAI is
-- aggro-line only (6312901).
(822, 63129, 'interrupt', 0, 837, 0, 0, 'Kanza the Seer: Frostbolt (spell list 63129)'),
-- Tansha the Sleek (63133, EventAI, spell list 63133): Glacial Roar 3143 (20-21 s first,
-- every 17-24 s) is a cast (castingTimeIndex 4) AoE damage+stun (effectApplyAura 12, radius
-- index 8). Her EventAI Intimidating Shout 19134 at 50% health is instant (castingTimeIndex 1).
(822, 63133, 'interrupt', 0, 3143, 0, 0, 'Tansha the Sleek: Glacial Roar (spell list 63133)'),
-- Hailar the Frigid (63130, script boss_hailar_the_frigid + spell list 63130): the script only
-- says lines and drives the ritualists; his casts come from spell list 63130, Frostbolt 8406
-- (1-3 s first, every 15-20 s, castingTimeIndex 14) and Flash Freeze 16803 (every 24-28 s, an
-- instant AoE stun, so no row).
(822, 63130, 'interrupt', 0, 8406, 0, 0, 'Hailar the Frigid: Frostbolt (spell list 63130)');
-- Battlemaster Ubukaz (63131, EventAI, no spell list) gets no row: his only events are an
-- aggro line (6313102) and Enrage 28131 at 25% health (6313101, self-buff, instant).