-- Interrupt rows corrected after the 2026-10-20 audit (pjerra/turtle-agent issue 246).
-- The core interrupts a cast only when the spell has PreventionType silence (1) and,
-- for a generic cast, InterruptFlags with the damage bit 0x02 (src/game/Spells/SpellEffects.cpp,
-- Spell::EffectInterruptCast). The earlier phases checked castingTimeIndex and the flag bits
-- but not PreventionType. Two merged rows have PreventionType 0, so a kick could never stop
-- them and each one would only waste the kick: they are removed here, key-scoped.
--   Tansha the Sleek (map 822, boss 63133): Glacial Roar 3143, preventionType 0.
--   Chieftain Shalk Blackwind (map 820, boss 62782) add 62771: Bloodguard Ritual 36732,
--   preventionType 0.
DELETE FROM `ai_boss_tactic` WHERE `map` = 822 AND `boss_entry` = 63133 AND `kind` = 'interrupt' AND `target_entry` = 0 AND `spell_id` = 3143;
DELETE FROM `ai_boss_tactic` WHERE `map` = 820 AND `boss_entry` = 62782 AND `kind` = 'interrupt' AND `target_entry` = 62771 AND `spell_id` = 36732;
