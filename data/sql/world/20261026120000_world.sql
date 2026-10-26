-- Mercenary Hire recruiters (95000-95067): a greeting of their own in place of
-- the client's "Greetings". Text 95000+n is the PvE-only greeting with its red
-- battleground line; 95100+n the same without it (AiPlayerbot.HireAllowPvP = 1).
-- Each recruiter names itself: "Recruiter Marla" says "I am Marla".

DELETE FROM `npc_text` WHERE `ID` BETWEEN 95000 AND 95067 OR `ID` BETWEEN 95100 AND 95167;
DELETE FROM `broadcast_text` WHERE `entry` BETWEEN 95000 AND 95067 OR `entry` BETWEEN 95100 AND 95167;

INSERT INTO `broadcast_text` (`entry`, `male_text`, `female_text`)
SELECT `entry`,
    CONCAT('Greetings, $N. I am ', SUBSTRING(`name`, 11), ', and I hire out mercenaries to those who need a strong arm on the road. Dungeons, wild lands, the dark places of the world: my companions will fight beside you.$B$B|cff9b0000These mercenaries are for PvE only. They leave you when you enter a battleground.|r'),
    CONCAT('Greetings, $N. I am ', SUBSTRING(`name`, 11), ', and I hire out mercenaries to those who need a strong arm on the road. Dungeons, wild lands, the dark places of the world: my companions will fight beside you.$B$B|cff9b0000These mercenaries are for PvE only. They leave you when you enter a battleground.|r')
FROM `creature_template` WHERE `entry` BETWEEN 95000 AND 95067 AND `name` LIKE 'Recruiter %';

INSERT INTO `broadcast_text` (`entry`, `male_text`, `female_text`)
SELECT `entry` + 100,
    CONCAT('Greetings, $N. I am ', SUBSTRING(`name`, 11), ', and I hire out mercenaries to those who need a strong arm on the road. Dungeons, wild lands, the dark places of the world: my companions will fight beside you.'),
    CONCAT('Greetings, $N. I am ', SUBSTRING(`name`, 11), ', and I hire out mercenaries to those who need a strong arm on the road. Dungeons, wild lands, the dark places of the world: my companions will fight beside you.')
FROM `creature_template` WHERE `entry` BETWEEN 95000 AND 95067 AND `name` LIKE 'Recruiter %';

INSERT INTO `npc_text` (`ID`, `BroadcastTextID0`, `Probability0`)
SELECT `entry`, `entry`, 1 FROM `broadcast_text` WHERE `entry` BETWEEN 95000 AND 95067 OR `entry` BETWEEN 95100 AND 95167;
