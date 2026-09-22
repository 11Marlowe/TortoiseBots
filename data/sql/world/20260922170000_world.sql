-- Enchant map corrections (owner observations, PR review round):
--1. Enchant Weapon - Spell Power (Formula18259, Ancient Incendosaur loot) is a
--    raid recipe per the owner's non-raid rule ("spell damage +30 to raidowa
--    recepta"). Swap every Spell Power weapon row to Enchant Weapon - Mighty
--    Intellect (Formula19449, sold by Lokhtos Darkbargainer — vendor, non-raid;
--    owner: "fiery weapon na resto? nie ma sensu, chyba lepiej int?").
--2. Enchant Weapon - Fiery Weapon must never target the offhand slot: vanilla
--    offhand items for shamans are shields, which take shield enchants only.
--    Shields keep Enchant Shield - Greater Stamina (row kept below).
--3. Restoration (spec71) and Elemental (spec72) shaman mains get Mighty
--    Intellect; Enhancement (spec70) keeps Fiery Weapon.
-- Idempotent: re-running matches zero rows once applied.
UPDATE ai_playerbot_enchants SET spellid =23804 WHERE spellid = 22749;
DELETE FROM ai_playerbot_enchants WHERE class =7 AND spellid =13898 AND slotid =16;
UPDATE ai_playerbot_enchants SET spellid =23804 WHERE class =7 AND slotid =15 AND spec IN (71,72) AND spellid =13898;
