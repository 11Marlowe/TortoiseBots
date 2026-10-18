-- Repair random goblin bots created with out-of-range appearance values.
--
-- The random-bot generator used to roll skin, face, hair style, hair colour
-- and facial hair in 0..7 for every race. Goblins only have 3 skins, 4-6
-- faces, 14-17 hair styles, 5 hair colours and 5-10 facial hair styles
-- (see runtime/BotAppearancePolicy.h), so out-of-range values drew a missing
-- or wrong-race head. Only RNDBOT accounts are touched; real players are not.
--
-- Each out-of-range field is wrapped into the valid range with modulo, so
-- bots already in range keep their look and the rows touched are only the
-- broken ones. Re-running is a no-op: after this, no row matches the WHERE.
--
-- Byte layout: playerBytes = skin | face << 8 | hairStyle << 16 | hairColor << 24;
-- playerBytes2 low byte = facialHair. Gender: 0 male, 1 female.

UPDATE `characters` c
JOIN `tw_logon`.`account` a ON a.id = c.account
SET
  c.playerBytes = ((c.playerBytes & 255) % 3)
    | ((((c.playerBytes >> 8) & 255) % IF(c.gender = 0, 4, 6)) << 8)
    | ((((c.playerBytes >> 16) & 255) % IF(c.gender = 0, 14, 17)) << 16)
    | ((((c.playerBytes >> 24) & 255) % 5) << 24),
  c.playerBytes2 = (c.playerBytes2 & 0xFFFFFF00)
    | ((c.playerBytes2 & 255) % IF(c.gender = 0, 10, 5))
WHERE c.race = 9
  AND a.username LIKE 'RNDBOT%'
  AND (
    (c.playerBytes & 255) >= 3
    OR ((c.playerBytes >> 8) & 255) >= IF(c.gender = 0, 4, 6)
    OR ((c.playerBytes >> 16) & 255) >= IF(c.gender = 0, 14, 17)
    OR ((c.playerBytes >> 24) & 255) >= 5
    OR (c.playerBytes2 & 255) >= IF(c.gender = 0, 10, 5)
  );
