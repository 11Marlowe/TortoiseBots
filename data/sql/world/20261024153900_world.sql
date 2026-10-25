-- Boss tactics for Scholomance (map 289): Vectus 10432 and Marduk Blackpool 10433 (one creature group, flags 2 = aggro
-- together: creature_groups (48805,48806,0,0,2)). Read from tortoise-wow: boss_vectus.cpp, creature_spells 104330,
-- creature_ai_events 1043302, spell_template 17228/17695/17697/18399/16046. Spec ~/TortoiseBots-audit/specs/scholo-vectus-fight.md.
-- Idempotent: the DELETE removes only these boss rows. Never edit this file once applied; add a newer migration.
DELETE FROM `ai_boss_tactic` WHERE `map` = 289 AND `boss_entry` IN (10432, 10433);
INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
(289, 10432, 'kill_first', 10433, 0, 1, 0, 'Marduk first: Defiling Aura 17695 (-100 shadow resistance, radius index 10) and Shadow Bolt Volley 17228 every 6-14 s hit every bot'),
(289, 10433, 'kill_first', 10433, 0, 1, 0, 'same row under Marduk: active while only he is on the attacker list'),
(289, 10432, 'interrupt', 0, 18399, 0, 0, 'Vectus Flamestrike 18399 every 30 s: castingTimeIndex 14, preventionType 1, interruptFlags 15');
