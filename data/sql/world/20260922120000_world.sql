-- Mage school weights (owner spec: a frost mage must not wear +fire damage
-- gear, and vice versa). The three mage scales carried identical
-- firsplpwr/frosplpwr/arcsplpwr values, so every element scored equally and
-- foreign-school items passed the spec weight check. Zero the element each
-- spec never casts; arcane stays for all three specs (Arcane Explosion is
-- in every mage kit) and the generic splpwr stays untouched. Priest/shaman
-- scales already reject foreign schools by omission (no field = weight 0),
-- warlock specs legitimately use both fire and shadow.
UPDATE `ai_playerbot_weightscale_data` SET `val` = 0 WHERE `id` = 25 AND `field` = 'firsplpwr';
UPDATE `ai_playerbot_weightscale_data` SET `val` = 0 WHERE `id` = 24 AND `field` = 'frosplpwr';
UPDATE `ai_playerbot_weightscale_data` SET `val` = 0 WHERE `id` = 23 AND `field` IN ('firsplpwr', 'frosplpwr');
