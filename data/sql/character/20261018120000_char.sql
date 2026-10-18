-- Repair pool bots created with out-of-range appearance values.
--
-- The random-bot generator used to roll skin, face, hair style, hair colour
-- and facial hair in 0..7 for every race. Many races have fewer valid values
-- for some fields (goblins: 3 skins, 5 hair colours), and an out-of-range
-- value drew a missing or wrong-race head. The valid counts below mirror
-- runtime/BotAppearancePolicy.h. Only pool bot accounts are touched
-- (tortoise_bots_pool_account); real players are not.
--
-- Each out-of-range field is wrapped into its valid range with modulo, so
-- in-range fields keep their value. Re-running is a no-op: after this, no
-- row matches the WHERE.
--
-- Byte layout: playerBytes = skin | face << 8 | hairStyle << 16 | hairColor << 24;
-- playerBytes2 low byte = facialHair. Gender: 0 male, 1 female.

UPDATE `characters` c
JOIN `tortoise_bots_pool_account` p ON p.account_id = c.account
JOIN (
  SELECT 1 AS race, 0 AS gender, 10 AS skin, 12 AS face, 17 AS hair, 10 AS hcol, 9 AS facial
  UNION ALL SELECT 1, 1, 10, 15, 26, 10, 7
  UNION ALL SELECT 2, 0, 9, 10, 13, 8, 11
  UNION ALL SELECT 2, 1, 9, 9, 14, 8, 7
  UNION ALL SELECT 3, 0, 14, 10, 17, 10, 21
  UNION ALL SELECT 3, 1, 14, 10, 19, 10, 6
  UNION ALL SELECT 4, 0, 11, 10, 12, 10, 14
  UNION ALL SELECT 4, 1, 11, 9, 12, 10, 10
  UNION ALL SELECT 5, 0, 6, 10, 16, 11, 17
  UNION ALL SELECT 5, 1, 6, 10, 17, 11, 8
  UNION ALL SELECT 6, 0, 20, 5, 13, 9, 9
  UNION ALL SELECT 6, 1, 12, 4, 12, 9, 7
  UNION ALL SELECT 7, 0, 6, 7, 12, 10, 12
  UNION ALL SELECT 7, 1, 6, 7, 12, 10, 7
  UNION ALL SELECT 8, 0, 12, 5, 10, 10, 14
  UNION ALL SELECT 8, 1, 12, 6, 10, 10, 10
  UNION ALL SELECT 9, 0, 3, 4, 14, 5, 10
  UNION ALL SELECT 9, 1, 3, 6, 17, 5, 5
  UNION ALL SELECT 10, 0, 11, 10, 16, 16, 11
  UNION ALL SELECT 10, 1, 11, 10, 20, 16, 11
) v ON v.race = c.race AND v.gender = c.gender
SET
  c.playerBytes = ((c.playerBytes & 255) % v.skin)
    | ((((c.playerBytes >> 8) & 255) % v.face) << 8)
    | ((((c.playerBytes >> 16) & 255) % v.hair) << 16)
    | ((((c.playerBytes >> 24) & 255) % v.hcol) << 24),
  c.playerBytes2 = (c.playerBytes2 & 0xFFFFFF00)
    | ((c.playerBytes2 & 255) % v.facial)
WHERE (c.playerBytes & 255) >= v.skin
   OR ((c.playerBytes >> 8) & 255) >= v.face
   OR ((c.playerBytes >> 16) & 255) >= v.hair
   OR ((c.playerBytes >> 24) & 255) >= v.hcol
   OR (c.playerBytes2 & 255) >= v.facial;
