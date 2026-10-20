-- Boss tactics: Zul Farrak (map 209) rows, first content.
--
-- Same table and loader as the Uldaman rows (data/sql/world/20261014120000_world.sql), the
-- phase 1a rows (20261018120925_world.sql) and the Windhorn/Gilneas/Karazhan/Crescent rows
-- (20261018212710_world.sql); see docs/guides/boss-tactics.md.
-- Every creature and spell below was read on 2026-10-20 from Turtle own world data
-- (tortoise-wow 5d79486 sql/base: creature_template, creature_spells, spell_template;
-- src/scripts/dungeons/zulfarrak): the creature_spells list is attached through
-- creature_template.spell_list_id, which for all three bosses here is the list named in
-- the row comment.
--
-- The cast time table (SpellCastTimes.dbc) is not in the repo, so a spell counts as a real
-- cast when its castingTimeIndex is above 1 (index 1 is instant) and its interruptFlags
-- carry the kick bits (15 or 31, as in the 1a rows). A channel (channelInterruptFlags set)
-- is never seen by the interrupt scan (it reads CURRENT_GENERIC_SPELL only), so channels get
-- no row.
--
-- Idempotent: the DELETE removes only the three boss entries this migration owns, then their
-- rows go back in. The core runs each migration file inside one transaction. Never edit this
-- file once applied; add a newer migration.

DELETE FROM `ai_boss_tactic` WHERE `map` = 209 AND `boss_entry` IN (7795, 7275, 8127);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Hydromancer Velratha (7795, spell list 77950, also inline spell_id1-4). Healing Wave 12491
-- (slot 1, first at 0 s, then every 9-22 s, castingTimeIndex 14, interruptFlags 15, heal,
-- cast target 15) and Shadow Bolt Volley 15245 (slot 3, first at 5-10 s, then every 17-22 s,
-- castingTimeIndex 14, interruptFlags 15, AoE, radius index 10) are casts. Shadow Bolt 12739
-- (slot 4, repeat timer 2 s at 90 percent, castingTimeIndex 14, interruptFlags 15) is the
-- filler bolt and is deliberately not listed: FindBossInterrupt takes the first listed
-- spell in progress, so a 2 s filler would eat the kick before the 9-22 s heal. Ward of
-- Zumrah 11086 (slot 2, every 13-28 s) is instant (castingTimeIndex 1, interruptFlags 0):
-- no row.
(209, 7795, 'interrupt', 0, 12491, 0, 0, 'Hydromancer Velratha: Healing Wave (spell list 77950, cast 14, kick 15)'),
(209, 7795, 'interrupt', 0, 15245, 0, 0, 'Hydromancer Velratha: Shadow Bolt Volley (spell list 77950, cast 14, kick 15)'),
-- Shadowpriest Sezzziz (7275, EventAI for the 15 percent flee only, spell list 72750).
-- Heal 12039 (slot 2, first at 0 s, then every 14-25 s, castingTimeIndex 14, interruptFlags
-- 15, cast target 15) and Renew 8362 (slot 3, every 28-35 s, castingTimeIndex 5,
-- interruptFlags 15, cast target 15, aura 8) are casts. Psychic Scream 13704 (slot 1,
-- every 22-32 s) is instant (castingTimeIndex 1, interruptFlags 8) and Shadow Bolt 15537
-- (slots 4-5, repeat timer 2-4 s, castingTimeIndex 14) is the filler bolt: no rows, so kicks
-- are kept for the heals.
(209, 7275, 'interrupt', 0, 12039, 0, 0, 'Shadowpriest Sezzziz: Heal (spell list 72750, cast 14, kick 15)'),
(209, 7275, 'interrupt', 0, 8362, 0, 0, 'Shadowpriest Sezzziz: Renew (spell list 72750, cast 5, kick 15)'),
-- Antusul (8127, EventAI, spell list 81270 and inline spell_id1-4). Flash Heal 10915
-- (script 812709, at or below 60 percent, every 17-20 s, castingTimeIndex 16,
-- interruptFlags 15); Healing Wave of Antusul 11895 (scripts 812708 at or below 30 percent
-- every 12 s and 812707 once at 20 percent, castingTimeIndex 4, interruptFlags 15); Healing
-- Ward 11899 (random action of event 812704, scripts 812724/812725, first at 11.6 s then
-- every 13.3-21.7 s, castingTimeIndex 5, interruptFlags 15, effect 87 summon totem, summons
-- Greater Healing Ward 8179). All three are silence-prevented (preventionType 1), so the
-- interrupt effect stops the cast and no heal or ward happens.
(209, 8127, 'interrupt', 0, 10915, 0, 0, 'Antusul: Flash Heal at or below 60 percent (script 812709, cast 16, kick 15)'),
(209, 8127, 'interrupt', 0, 11895, 0, 0, 'Antusul: Healing Wave of Antusul at or below 30 percent (scripts 812708/812707, cast 4, kick 15)'),
(209, 8127, 'interrupt', 0, 11899, 0, 0, 'Antusul: Healing Ward summon 8179 (scripts 812724/812725, cast 5, kick 15)'),
-- Antusul also summons Servant of Antusul 8156 with Antusul Minion 11894 (effect 42, summon
-- at the caster) at 75 and 25 percent health: event 812705 casts it once, 812706 twice
-- (creature_ai_events, creature_ai_scripts 812705/812706). The servant is faction 107,
-- unit_flags 0, no EventAI and no script, so it is reasoned to join the fight (attacker
-- list membership not traced, live check). Servants first, in both waves.
(209, 8127, 'kill_first', 8156, 0, 1, 0, 'Antusul: Servant of Antusul first (summoned by 11894, events 812705/812706)');

-- No behind rows in Zul Farrak. Chief Ukorz Sandscalp (7267, spell list 72670) casts Cleave
-- 15496 (first at 6-20 s, then every 8-19 s) and Ruuzlu (7797, spell list 77970) casts it
-- (first at 7-12 s, then every 7-18 s): effect 58 on a unit target (implicit target 6),
-- radius index 0 and no radius in the spell text, so there is no yard value for a behind row
-- (the same reason the phase 1a and 1b migrations gave for their cleaves). Ukorz also casts
-- Wide Slash 11837 (first at 12-16 s, then every 7-24 s), a cone (implicit target 24) with
-- radius index 13, but the client SpellRadius table is not in the repo, so its yard value
-- cannot be read either. Ruuzlu casts no other spell; both open with Berserker Stance 7366
-- on themselves (instant).
--
-- No interrupt row for Earthgrab Totem 8376 (Antusul script 812724, castingTimeIndex 3,
-- interruptFlags 13, preventionType 0). It is a real cast, but 13 is 0x0D and lacks
-- SPELL_INTERRUPT_FLAG_DAMAGE (0x02, SpellDefines.h); EffectInterruptCast requires that bit
-- for CURRENT_GENERIC_SPELL (src/game/Spells/SpellEffects.cpp:3392), so a kick would not
-- stop it. The 1a kick rule (flags 15 or 31) matches that requirement.
--
-- No kill_first row for the Ward of Zumrah 7785 under Zumrah (7271): it is a ScriptedAI
-- totem (src/scripts/dungeons/zulfarrak/zulfarrak.cpp:605-632) that only casts Skeleton of
-- Zumrah 11088 on a 5 s timer and does not attack in that script, so it is reasoned (not
-- traced) that it never enters the bots attacker list. The ZF run-leader plan already names
-- 7785 in its own kill_first column; this migration does not duplicate it.
