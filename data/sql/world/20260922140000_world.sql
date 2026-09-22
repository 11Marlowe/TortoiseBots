-- Warrior protection stamina (owner-approved balance tweak from the tank
-- review). Protection warriors scored sta at 6 while the DPS specs (arms,
-- fury) score 15 — the tank spec valued stamina less than the damage specs,
-- which reads inverted: a main tank should want stam above deeps. Bump
-- protection to 18 (above the DPS15), matching the defense bump to the
-- paladin baseline in20260922130000_world.sql. Paladin protection sta=6
-- has the same shape but was left untouched pending the owner's call.
-- Item-info and equip caches are cleared separately; rebuild happens on
-- the next server boot.
UPDATE `ai_playerbot_weightscale_data` SET `val` = 18 WHERE `id` =3 AND `field` = 'sta';
