-- Paladin protection stamina (owner: "tak to samo" — same treatment as the
-- warrior bump in 20260922140000_world.sql). Holy scores sta12, retribution
-- has no stamina field at all, and protection scored sta6 — the tank wanted
-- stamina least of all three specs, the same inverted shape warrior had.
-- Mirror the warrior delta: the highest other paladin spec (holy12) +3 →
-- protection sta15. Protection already scores defrtng8 and blockrtng14, so
-- defense/block need no change. Retribution str4 > protection str1 is the
-- same inversion one level down but is a balance judgement left untouched.
UPDATE `ai_playerbot_weightscale_data` SET `val` = 15 WHERE `id` = 5 AND `field` = 'sta';
