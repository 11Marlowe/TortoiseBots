-- Boss tactics (pjerra/turtle-agent#254): interrupt rows for the Wailing Caverns
-- lords, Mutanus, Grimlok (Uldaman) and Captain Greenskin (Deadmines).
--
-- Same table and loader as the Uldaman rows (data/sql/world/20261014120000_world.sql);
-- see docs/play/boss-tactics.md. Every creature and spell below was read from Turtle's
-- own world data (tortoise-wow sql/base: creature_template, creature_spells,
-- spell_template); the creature_spells list id is named in each row comment, and every
-- listed spell is a real cast (castingTimeIndex above 1) that is silence-prevented
-- (preventionType 1). Spells checked and left out as instant (castingTimeIndex 1):
-- Pythas Thunderclap 8147, Mutanus Terrify 7399, Grimlok Tremor Totem 8143,
-- Greenskin Cleave 15584. Verdan 5775 has no interrupt list in the plan: no row.
--
-- Idempotent: each DELETE removes only this migration's boss rows, then they go back
-- in. The core runs each migration file inside one transaction. Never edit this file
-- once applied; add a newer migration.

DELETE FROM `ai_boss_tactic` WHERE `map` = 43 AND `boss_entry` IN (3673, 3669, 3670, 3674, 3654);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Lord Serpentis (3673, EventAI, spell list 36730): Healing Touch 6778 (castingTimeIndex
-- 22), Sleep 700 (castingTimeIndex 16) and Lightning Bolt 9532 (castingTimeIndex 14),
-- all silence-prevented with the kick bits (interruptFlags 15).
(43, 3673, 'interrupt', 0, 6778, 0, 0, 'Lord Serpentis: Healing Touch (spell list 36730)'),
(43, 3673, 'interrupt', 0, 700, 0, 0, 'Lord Serpentis: Sleep (spell list 36730)'),
(43, 3673, 'interrupt', 0, 9532, 0, 0, 'Lord Serpentis: Lightning Bolt (spell list 36730)'),
-- Lord Cobrahn (3669, EventAI, druid-form spell list 36690): Druid Slumber 8040
-- (castingTimeIndex 19) and Lightning Bolt 9532 (castingTimeIndex 14), both
-- silence-prevented with the kick bits (interruptFlags 15). Healing Touch 5187 is the
-- plan list third spell (a real cast: castingTimeIndex 20, interruptFlags 15,
-- preventionType 1); the druid-form list carries only the first two.
(43, 3669, 'interrupt', 0, 8040, 0, 0, 'Lord Cobrahn: Druid Slumber (spell list 36690)'),
(43, 3669, 'interrupt', 0, 9532, 0, 0, 'Lord Cobrahn: Lightning Bolt (spell list 36690)'),
(43, 3669, 'interrupt', 0, 5187, 0, 0, 'Lord Cobrahn: Healing Touch (plan list)'),
-- Lord Pythas (3670, EventAI, spell list 36700): Healing Touch 5187 (castingTimeIndex
-- 20), Sleep 700 (castingTimeIndex 16) and Lightning Bolt 9532 (castingTimeIndex 14),
-- all silence-prevented with the kick bits (interruptFlags 15). Thunderclap 8147 is
-- instant (castingTimeIndex 1): no row.
(43, 3670, 'interrupt', 0, 5187, 0, 0, 'Lord Pythas: Healing Touch (spell list 36700)'),
(43, 3670, 'interrupt', 0, 700, 0, 0, 'Lord Pythas: Sleep (spell list 36700)'),
(43, 3670, 'interrupt', 0, 9532, 0, 0, 'Lord Pythas: Lightning Bolt (spell list 36700)'),
-- Skum (3674, EventAI, no spell list; Chained Bolt 6254 is the inline spell_id1): a real
-- cast (castingTimeIndex 23), silence-prevented with the kick bits (interruptFlags 15).
(43, 3674, 'interrupt', 0, 6254, 0, 0, 'Skum: Chained Bolt (inline spell_id1)'),
-- Mutanus the Devourer (3654, EventAI, spell list 36540): Naralex Nightmare 7967 (first
-- at 8-12 s, every 18-25 s, castingTimeIndex 5), silence-prevented with the kick bits
-- (interruptFlags 15). Terrify 7399 is instant (castingTimeIndex 1): no row.
(43, 3654, 'interrupt', 0, 7967, 0, 0, 'Mutanus the Devourer: Naralex Nightmare (spell list 36540)');

DELETE FROM `ai_boss_tactic` WHERE `map` = 70 AND `boss_entry` IN (4854);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Grimlok (4854, EventAI, spell list 48540): Chain Bolt 8292 (castingTimeIndex 19) and
-- Lightning Bolt 9532 (castingTimeIndex 14), both silence-prevented (preventionType 1).
-- Shrink 11892 gets no row (not in the plan interrupt list, which names Chain Bolt and
-- Lightning Bolt); Tremor Totem 8143 is instant (castingTimeIndex 1): no row.
(70, 4854, 'interrupt', 0, 8292, 0, 0, 'Grimlok: Chain Bolt (spell list 48540)'),
(70, 4854, 'interrupt', 0, 9532, 0, 0, 'Grimlok: Lightning Bolt (spell list 48540)');

DELETE FROM `ai_boss_tactic` WHERE `map` = 36 AND `boss_entry` IN (647);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Captain Greenskin (647, no ai_name, spell list 6470 and inline spell_id2): Poisoned
-- Harpoon 5208 (castingTimeIndex 5), silence-prevented with the kick bits
-- (interruptFlags 15). Cleave 15584 is instant (castingTimeIndex 1): no row.
(36, 647, 'interrupt', 0, 5208, 0, 0, 'Captain Greenskin: Poisoned Harpoon (spell list 6470)');
