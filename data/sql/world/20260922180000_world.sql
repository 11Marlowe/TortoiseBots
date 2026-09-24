-- Per-spec MH distribution + heal-glove coverage (owner: deep enchant review).
-- Shaman meaning per live formula (spec = 70+DBC-tab): 70 = Elemental,
-- 71 = Enhancement, 72 = Restoration. Owner call: Enhancement = Fiery,
-- Restoration + Elemental = Mighty Intellect.
-- Rogue: 40 = Assassination -> Crusader, 41 = Combat -> Fiery,
-- 42 = Subtlety -> Lifestealing (all formulas vendor-clean or
-- dungeon/world drops; raid-linked reps excluded).
-- Healers (incl. resto shaman/druid): weapon + gloves per evidence:
-- Healing Power weapon (22750) and Healing gloves (25079) have NO raid
-- formula, vendor, or raid-loot row in this DB (Scout-verified).
-- Ring/neck: NO classic Enchant Ring/Neck spell exists in the dump; the
-- only proven 1.18.1 custom is the Topaz gem (item-use, not an enchant
-- spell), so no ring/neck rows are added here.
-- Idempotent: each statement matches zero rows once applied.
UPDATE ai_playerbot_enchants SET spellid = 20034 WHERE class = 4 AND spec = 40 AND slotid = 15 AND spellid = 13898;
UPDATE ai_playerbot_enchants SET spellid = 20032 WHERE class = 4 AND spec = 42 AND slotid = 15 AND spellid = 13898;
UPDATE ai_playerbot_enchants SET spellid = 23804 WHERE class = 7 AND spec = 70 AND slotid = 15 AND spellid = 13898;
UPDATE ai_playerbot_enchants SET spellid = 13898 WHERE class = 7 AND spec = 71 AND slotid = 15 AND spellid = 23804;
UPDATE ai_playerbot_enchants SET spellid = 25079 WHERE class = 7 AND spec = 72 AND slotid = 9 AND spellid = 20012;
UPDATE ai_playerbot_enchants SET spellid = 25079 WHERE class = 11 AND spec = 112 AND slotid = 9 AND spellid = 20012;
INSERT INTO ai_playerbot_enchants (class, spec, spellid, slotid)
SELECT 5, 50, 25079, 9 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM ai_playerbot_enchants WHERE class = 5 AND spec = 50 AND slotid = 9);
INSERT INTO ai_playerbot_enchants (class, spec, spellid, slotid)
SELECT 5, 51, 25079, 9 FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM ai_playerbot_enchants WHERE class = 5 AND spec = 51 AND slotid = 9);
