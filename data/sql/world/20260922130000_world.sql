-- Tank review (owner spec: protection warriors should value defense rating
-- like the other tank specs). Warrior protection scored defrtng at 1 while
-- paladin protection scores 8 and feral tank 12 — against hitrtng 100 and
-- critstrkrtng 81 the defense field was effectively noise, so warriors only
-- collected defense pieces by accident (bundled str/sta) instead of by
-- preference. Measured on the fresh 500: tanks averaged 4.26 defensive
-- items,1 paladin out of 13 had none at all. Bring warrior protection to
-- the paladin baseline; dodge/parry/block/sta left untouched (balance
-- judgement, owner may tune further).
UPDATE `ai_playerbot_weightscale_data` SET `val` = 8 WHERE `id` = 3 AND `field` = 'defrtng';
