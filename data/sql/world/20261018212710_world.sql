-- Boss tactics: phase 1b rows for the Turtle-custom bosses of Windhorn Canyon (820),
-- Gilneas City (815), Karazhan Crypt (800) and Crescent Grove (802).
--
-- Same table and loader as the Uldaman rows (data/sql/world/20261014120000_world.sql) and the
-- phase 1a rows (data/sql/world/20261018120925_world.sql); see docs/guides/boss-tactics.md.
-- Every creature and spell below was read from Turtle own world data (tortoise-wow sql/base plus
-- sql/database_updates/world: creature_template, creature_spells, creature_ai_events,
-- creature_ai_scripts, spell_template; src/scripts/dungeons), not from another core. The attach of
-- a creature_spells list to a creature is spelled out where a row depends on it: several of these
-- dungeons take it from a separate `UPDATE creature_template SET spell_list_id = entry` migration,
-- not from the row that creates the creature.
--
-- The cast time table (SpellCastTimes.dbc) is not in the repo, so a spell counts as a real cast
-- when its castingTimeIndex is above 1 (index 1 is instant) and its interruptFlags carry the
-- kick bits (15 or 31, as in the 1a rows). A channel (channelInterruptFlags set) is never seen
-- by the interrupt scan (it reads CURRENT_GENERIC_SPELL only), so channels get no row. No behind
-- row this phase: every cone or cleave found either has no radius index (Cleave 15579, that of
-- Marrowspike, Raxxieth and Walgan: EffectRadiusIndex 0), or has one whose yard value cannot be
-- read from the repo (the client SpellRadius table is missing; the 1a Ironaya row could use the
-- War Stomp tooltip value for radius index 8, none of these has such a source).
--
-- Idempotent: each DELETE removes only this migration own boss rows, then they go back in. The
-- core runs each migration file inside one transaction. Never edit this file once applied; add a
-- newer migration.

DELETE FROM `ai_boss_tactic` WHERE `map` = 820 AND `boss_entry` IN (62778, 62779, 62780, 62781, 62782, 62783, 62784);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Prophet Stormhoof (62781, EventAI, spell list 62781): Corruption 11672 (first at 2-3 s, then
-- every 17-22 s, castingTimeIndex 16, interruptFlags 15, periodic-damage aura 3). Curse of Agony
-- 11712 is instant (castingTimeIndex 1) and Wail of Souls 57748 is instant (knock back).
(820, 62781, 'interrupt', 0, 11672, 0, 0, 'Prophet Stormhoof: Corruption (spell list 62781)'),
-- Chieftain Shalk Blackwind (62782, script boss_chieftain_shalk_blackwind, spell list 62782):
-- Flame of Shalk 41121 (first at 11-13 s, then every 32-47 s, castingTimeIndex 5, interruptFlags
-- 15) applies periodic fire damage and a healing debuff (auras 3 and 118, radius index 22).
-- Berserker Charge 19471 is instant, and the script also strips Flame of Shalk from a player who
-- goes underwater.
(820, 62782, 'interrupt', 0, 41121, 0, 0, 'Chieftain Shalk Blackwind: Flame of Shalk (spell list 62782)'),
-- At 50 percent health Shalk summons one Blackwind Bloodguard 62771 and attack-starts it on a
-- random attacker (src/scripts/dungeons/windhorn_canyon/windhorn_canyon.cpp:37-46), so the add is
-- on the bots attacker list. The add EventAI event 6277101 casts Bloodguard Ritual 36732 at its
-- own 50 percent health (creature_ai_events 6277101, creature_ai_scripts 6277101: CAST_SPELL on
-- self, castingTimeIndex 171, interruptFlags 15, durationIndex 4).
(820, 62782, 'interrupt', 62771, 36732, 0, 0, 'Chieftain Shalk Blackwind: Bloodguard Ritual 36732 of the summoned Bloodguard (EventAI 6277101)'),
-- Ambassador Vortalus (62783, EventAI, spell list 62783): Gust of Wind 6982 (first at 4-5 s, then
-- every 14-19 s, castingTimeIndex 5, interruptFlags 31) is an AoE stun (aura 12, radius index 8).
-- Chain Lightning 22355 is instant, and the EventAI cast Engulfing Vortex 36578 at 50 percent
-- health is instant too.
(820, 62783, 'interrupt', 0, 6982, 0, 0, 'Ambassador Vortalus: Gust of Wind AoE stun (spell list 62783)');
-- No row for four of the seven Windhorn bosses. Ahgktos the Pure (62778, spell list 62778, no
-- EventAI rows) casts Lightning Cloud 23105, Lightning Shield 945 and Wing Flap 12882, all
-- instant. Pathun Duskhide (62779, spell list 62779) casts Kodo Stomp 6266 and Demoralizing Shout
-- 11554 (instant); his only cast is Hooked Net 14030 (castingTimeIndex 3) but its interruptFlags
-- is 9, without the kick bit. Bonespeaker Narlgom (62780, script boss_bonespeaker_narlgom) has no
-- spell list at all; the script casts Bone Armor 11445 (instant), Rain of Fire 57747 (a channel,
-- channelInterruptFlags 31788) and Summon Rotag 42004 (castingTimeIndex 6 but interruptFlags 9,
-- no kick bit). Walgan Bloodcaller (62784, spell list 62784) casts Cleave 16044 and War Stomp
-- 16727, both instant, and his EventAI Enrage 28131 at 15 percent health is instant.
-- Champion Rotag (62785), summoned by Narlgom at 50 percent health, gets no kill_first row: his
-- script npc_champion_rotag sets REACT_PASSIVE, overrides AttackStart with an empty body, stops
-- combat on every update and the template is UNIT_FLAG_IMMUNE_TO_PLAYER (33024), so he never
-- enters the bots attacker list.

DELETE FROM `ai_boss_tactic` WHERE `map` = 815 AND `boss_entry` IN (61264, 61418, 61419, 61420, 61421, 61422, 61423);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Genn Greymane (61418, script genn_greymane plus spell list 180166). The list holds Drain Life
-- 17620 (a channel: castingTimeIndex 1 with channelInterruptFlags 31788), Mortal Strike 21551 and
-- Curse of Agony 11711 (both instant); the script itself casts Drain Life, Mortal Strike and Curse of Agony
-- (instance_gilneas_city.cpp:197, 206, 217) and never runs that list. The script casts Fear 22678
-- once below 50 percent health (src/scripts/dungeons/gilneas_city/instance_gilneas_city.cpp:230,
-- castingTimeIndex 16, interruptFlags 15, aura 7).
(815, 61418, 'interrupt', 0, 22678, 0, 0, 'Genn Greymane: Fear below 50 percent health (script genn_greymane)'),
-- Packmaster Ragetooth (61420, spell list 180168): Howl of Terror 5484 (first at 0 s, then every
-- 30 s, castingTimeIndex 5, interruptFlags 15) is an AoE fear (auras 7 and 31, radius index 13)
-- and Shadow Bolt 11660 (2-2 s first, then every 12 s, castingTimeIndex 14, interruptFlags 15).
(815, 61420, 'interrupt', 0, 5484, 0, 0, 'Packmaster Ragetooth: Howl of Terror AoE fear (spell list 180168)'),
(815, 61420, 'interrupt', 0, 11660, 0, 0, 'Packmaster Ragetooth: Shadow Bolt (spell list 180168)'),
-- Judge Sutherland (61421, spell list 180169): Smite 6060 (first at 4 s, then every 6 s,
-- castingTimeIndex 19), Mind Blast 10945 (every 10 s, castingTimeIndex 16) and Fireball 8402
-- (every 8 s, castingTimeIndex 22), all with interruptFlags 15. Blast Wave 13019 is instant.
(815, 61421, 'interrupt', 0, 6060, 0, 0, 'Judge Sutherland: Smite (spell list 180169)'),
(815, 61421, 'interrupt', 0, 10945, 0, 0, 'Judge Sutherland: Mind Blast (spell list 180169)'),
(815, 61421, 'interrupt', 0, 8402, 0, 0, 'Judge Sutherland: Fireball (spell list 180169)'),
-- Marshal Magnus Greystone (61423, spell list 180171): Holy Fire 17142 (every 25 s,
-- castingTimeIndex 6, interruptFlags 15) damages and leaves a periodic-damage aura (aura 3).
-- Slam 11605 is a cast (castingTimeIndex 20) but its interruptFlags is 8, without the kick bit,
-- and Sunder Armor 8380 is instant (castingTimeIndex 1).
(815, 61423, 'interrupt', 0, 17142, 0, 0, 'Marshal Magnus Greystone: Holy Fire (spell list 180171)');
-- No row for Matthias Holtz (61419, EventAI, spell list 180167), Dustivan Blackcowl (61422,
-- EventAI, spell list 180170) or Regent-Lord Mortimer Harlow (61264, script boss_lord_mortimer):
-- every cast they have is instant (castingTimeIndex 1). Holtz casts Hooked Net 13608
-- (castingTimeIndex 3 but interruptFlags 9, no kick bit) and Fire Blast 8413 (list 180167,
-- interruptFlags 8, castingTimeIndex 1) and Multi-Shot 2643 (EventAI 2200030); Blackcowl casts Rupture 15583 and Rend 11574, plus Evasion 5277 and Gouge
-- 12540 from EventAI 2200031 and 2200032; Harlow casts Holy Strike 2495, Consecration 20924 and
-- Blast Wave 13021 (src/scripts/dungeons/gilneas_city/boss_lord_mortimer.cpp:41,53,63).

DELETE FROM `ai_boss_tactic` WHERE `map` = 800 AND `boss_entry` IN (91916, 91917, 91919, 91920, 91929, 92935);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- Archlich Enkhraz (91916, spell list 201002): Fear 12096 (first at 12-17 s, then every 15-16 s,
-- castingTimeIndex 16, interruptFlags 15, aura 7) and Freeze 18763 (first at 4-9 s, then every
-- 8-14 s, castingTimeIndex 22, interruptFlags 15, periodic damage plus stun). Chill Nova 18099 is
-- instant and Consuming Shadows 17715 is a channel (channelInterruptFlags 31788).
(800, 91916, 'interrupt', 0, 12096, 0, 0, 'Archlich Enkhraz: Fear (spell list 201002)'),
(800, 91916, 'interrupt', 0, 18763, 0, 0, 'Archlich Enkhraz: Freeze (spell list 201002)'),
-- Corpsemuncher (91917, spell list 201051): Acid Spit 25052 (first at 1 s, then every 4 s,
-- castingTimeIndex 14, interruptFlags 15). Its other spell, Sweep 26103 (castingTimeIndex 16,
-- interruptFlags 31), is a knock back around the caster (effectImplicitTargetA 22 = caster source
-- location, effect 98), not a frontal cone, so no behind row even though radius index 10 is set.
(800, 91917, 'interrupt', 0, 25052, 0, 0, 'Corpsemuncher: Acid Spit (spell list 201051)'),
-- Commander Andreon (91919, spell list 180104): Mind Blast 26048 (first at 3-4 s, then every
-- 45 s, castingTimeIndex 16, interruptFlags 15). Banshee Curse 16867 (cast on self, instant) and
-- Mortal Strike 15708 (instant) get no row, and the template inline spell_id2 5884 is never cast:
-- no script and no generic_spell_ai script_name uses the legacy inline spells.
(800, 91919, 'interrupt', 0, 26048, 0, 0, 'Commander Andreon: Mind Blast (spell list 180104)');
-- No row for Marrowspike (91920), Hivaxxis (91929) or Guard Captain Gort (92935). Marrowspike
-- (spell list 201052) has only instant casts: Cleave 15579 (castingTimeIndex 1, radius index 0,
-- so no behind row either), War Stomp 16727 (instant, self-centred stun, radius index 8) and Knock Away
-- 10101 (instant). Hivaxxis (spell list 201053) has Web Spray 12252, Paralyzing Poison 3609 and
-- Poison Mind 19469, all instant. Gort (spell list 111210) has only Banshee Curse 16867,
-- instant, no script and no EventAI rows; his inline spell_id2 5884 is not cast either.

DELETE FROM `ai_boss_tactic` WHERE `map` = 802 AND `boss_entry` IN (92107, 92108, 92109, 92110, 92111);

INSERT INTO `ai_boss_tactic` (`map`, `boss_entry`, `kind`, `target_entry`, `spell_id`, `priority`, `param`, `comment`) VALUES
-- High Priestess Alathea (92108, spell list 201059): Starfire 25298 (first at 1 s, then every
-- 5 s, castingTimeIndex 22, interruptFlags 15). Moonfire 23380 is instant (castingTimeIndex 1)
-- and her inline spell_id4 9613 is never cast (no script, not generic_spell_ai).
(802, 92108, 'interrupt', 0, 25298, 0, 0, 'High Priestess Alathea: Starfire (spell list 201059)'),
-- Keeper Ranathos (92109, EventAI, spell list 201060): Wrath 6780 (first at 1 s, then every 2 s,
-- castingTimeIndex 5, interruptFlags 15), Entangling Roots 11922 (every 15-20 s,
-- castingTimeIndex 16, interruptFlags 15, aura 26 root) and Regrowth 8941 (every 28 s,
-- castingTimeIndex 5, interruptFlags 15), a self-heal (cast target 0 = self, aura 8).
(802, 92109, 'interrupt', 0, 6780, 0, 0, 'Keeper Ranathos: Wrath (spell list 201060)'),
(802, 92109, 'interrupt', 0, 11922, 0, 0, 'Keeper Ranathos: Entangling Roots (spell list 201060)'),
(802, 92109, 'interrupt', 0, 8941, 0, 0, 'Keeper Ranathos: Regrowth self-heal (spell list 201060)'),
-- Master Raxxieth (92110, EventAI, spell list 201061): Shadow Bolt 12471 (first at 10 s, then
-- every 12 s, castingTimeIndex 14, interruptFlags 15) and Flamestrike 12468 (every 12 s,
-- castingTimeIndex 14, interruptFlags 15), aimed at a random attacker other than the top one
-- (cast target 5). Cleave 15579 is instant and has radius index 0, so no behind row.
(802, 92110, 'interrupt', 0, 12471, 0, 0, 'Master Raxxieth: Shadow Bolt (spell list 201061)'),
(802, 92110, 'interrupt', 0, 12468, 0, 0, 'Master Raxxieth: Flamestrike (spell list 201061)');
-- No row for Grovetender Engryss (92107) or Fenektis the Deceiver (92111): both spell lists are
-- all instant casts (castingTimeIndex 1). Engryss (spell list 201058) casts Strike 12057, Knock
-- Away 10101 and Rejuvenation 9839 (a friendly heal, cast target 15); Fenektis (spell list
-- 201062) casts Poisonous Stab 7357, Sinister Strike 15667 and Blade Flurry 13877. Engryss has
-- one EventAI row, a say line (2200008); Fenektis has none.
