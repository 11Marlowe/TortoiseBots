-- Enchant coverage round (owner: rings/legs/gloves/neck + ranged asked, plus
-- "other non-raid +damage weapon enchants").
--
-- Verified vanilla boundaries first: this DB contains ZERO ring and ZERO neck
-- enchant spells (TBC feature) and ZERO "Enchant Ranged%" spells; the weapon
-- enchant subclass masks exclude bows/guns — so the missing ring/neck/ranged
-- enchants are correct, not gaps. Classic armor kits carry effectMiscValue 0
-- (no enchant payload), so legs cannot go through this pipeline either —
-- vanilla-correct (kits are consumables).
--
-- Real gaps closed here:
-- 1. Caster main hands got Winter's Might (+FROST) regardless of spec — the
--    same school mismatch fixed for gear. Now per-spec: frost mage keeps
--    Winter's Might, fire/arcane mages, warlocks and shadow priests get
--    Enchant Weapon - Spell Power (all schools, Blackrock Spire 5-man drop
--    tier, not a raid formula), discipline/holy priests get Healing Power
--    (same drop tier).
-- 2. Gloves were never enchanted. Physical specs get Greater Agility /
--    Greater Strength (world formulas); casters get the school-matching
--    glove power enchants shipped in this DB (Shadow/Fire/Frost/Arcane/
--    Healing Power — custom-content spells, no raid provenance).
-- All spells verified: SPELL_EFFECT_ENCHANT_ITEM (53) with a nonzero
-- enchant id and a sane EquippedItemSubClassMask, so EnchantItemT's
-- type check accepts them on the intended armor/weapon classes only.
DELETE FROM `ai_playerbot_enchants` WHERE `slotid` = 15 AND `class` IN (5, 8, 9);
INSERT IGNORE INTO `ai_playerbot_enchants` (`class`, `spec`, `spellid`, `slotid`, `name`) VALUES
  (5, 50, 22750, 15, 'Healing Power - disc'),
  (5, 51, 22750, 15, 'Healing Power - holy'),
  (5, 52, 22749, 15, 'Spell Power - shadow'),
  (8, 80, 22749, 15, 'Spell Power - arcane'),
  (8, 81, 22749, 15, 'Spell Power - fire'),
  (8, 82, 21931, 15, 'Winter''s Might - frost'),
  (9, 90, 22749, 15, 'Spell Power - affliction'),
  (9, 91, 22749, 15, 'Spell Power - demonology'),
  (9, 92, 22749, 15, 'Spell Power - destruction'),
  (1, 10, 20013, 9, 'Gloves Greater Strength - arms'),
  (1, 11, 20013, 9, 'Gloves Greater Strength - fury'),
  (1, 12, 20013, 9, 'Gloves Greater Strength - prot'),
  (2, 20, 20013, 9, 'Gloves Greater Strength - holy'),
  (2, 21, 20013, 9, 'Gloves Greater Strength - prot'),
  (2, 22, 20013, 9, 'Gloves Greater Strength - retri'),
  (3, 30, 20012, 9, 'Gloves Greater Agility - bm'),
  (3, 31, 20012, 9, 'Gloves Greater Agility - mm'),
  (3, 32, 20012, 9, 'Gloves Greater Agility - surv'),
  (4, 40, 20012, 9, 'Gloves Greater Agility - assa'),
  (4, 41, 20012, 9, 'Gloves Greater Agility - combat'),
  (4, 42, 20012, 9, 'Gloves Greater Agility - sub'),
  (7, 70, 20012, 9, 'Gloves Greater Agility - elem'),
  (7, 71, 20012, 9, 'Gloves Greater Agility - enhance'),
  (7, 72, 20012, 9, 'Gloves Greater Agility - resto'),
  (11, 110, 20012, 9, 'Gloves Greater Agility - balance'),
  (11, 111, 20012, 9, 'Gloves Greater Agility - feral'),
  (11, 112, 20012, 9, 'Gloves Greater Agility - resto'),
  (5, 50, 25079, 9, 'Gloves Healing Power - disc'),
  (5, 51, 25079, 9, 'Gloves Healing Power - holy'),
  (5, 52, 25073, 9, 'Gloves Shadow Power - shadow'),
  (8, 80, 46601, 9, 'Gloves Arcane Power - arcane'),
  (8, 81, 25078, 9, 'Gloves Fire Power - fire'),
  (8, 82, 25074, 9, 'Gloves Frost Power - frost'),
  (9, 90, 25073, 9, 'Gloves Shadow Power - affliction'),
  (9, 91, 25073, 9, 'Gloves Shadow Power - demonology'),
  (9, 92, 25073, 9, 'Gloves Shadow Power - destruction');
