-- Issue #490 PR2: hire-gear tier unlock quests at the six capital <Mercenary Hire> recruiters.
--
-- 7 quests (90100-90106), one per tier 1-7 (HireUnlockQuestForTier in
-- runtime/HireUnlockPolicy.h). Unlock = rewarded flag (GetQuestRewardStatus):
-- no new table, persists in character_queststatus. Turn-in fee is native:
-- RewOrReqMoney < 0 makes the core block completion without the gold and
-- deduct it at turn-in (50g/100g/200g/300g/400g/500g/750g per tier). No XP,
-- no reward items, non-sharable (QuestFlags 8), non-repeatable, Type=62
-- (raid) so kill credit works in raid groups. PrevQuestId chains the tiers;
-- the server rewrites PrevQuestId to 0 at startup for HireGearAllQuestsAvailable.
-- Kill objectives use the invisible credit entries 95110-95121 (one slot per
-- counter in kHireUnlockCounters; the module grants credit per boss kill).
-- 95110-95121 are never spawned (credit-only creature_template rows).
-- English text only (no locales_quest table on this core). After apply:
-- `.reload quest_template`, `.reload creature_template`, or restart mangosd.
-- Idempotent: DELETEs the module's quest/credit ranges and the 7 quest
-- relations before re-inserting. Recruiter npc_flags already 3; untouched.

DELETE FROM `quest_template` WHERE `entry` BETWEEN 90100 AND 90106;
DELETE FROM `creature_template` WHERE `entry` IN (95110, 95111, 95112, 95113, 95114, 95115, 95116, 95117, 95118, 95119, 95120, 95121);
DELETE FROM `creature_questrelation` WHERE `quest` BETWEEN 90100 AND 90106;
DELETE FROM `creature_involvedrelation` WHERE `quest` BETWEEN 90100 AND 90106;

INSERT INTO `quest_template` (`entry`, `Method`, `ZoneOrSort`, `MinLevel`, `MaxLevel`, `QuestLevel`, `Type`, `RequiredClasses`, `RequiredRaces`, `RequiredSkill`, `RequiredSkillValue`, `RequiredCondition`, `RepObjectiveFaction`, `RepObjectiveValue`, `RequiredMinRepFaction`, `RequiredMinRepValue`, `RequiredMaxRepFaction`, `RequiredMaxRepValue`, `SuggestedPlayers`, `LimitTime`, `QuestFlags`, `SpecialFlags`, `PrevQuestId`, `NextQuestId`, `ExclusiveGroup`, `NextQuestInChain`, `SrcItemId`, `SrcItemCount`, `SrcSpell`, `Title`, `Details`, `Objectives`, `OfferRewardText`, `RequestItemsText`, `EndText`, `ObjectiveText1`, `ObjectiveText2`, `ObjectiveText3`, `ObjectiveText4`, `ReqItemId1`, `ReqItemId2`, `ReqItemId3`, `ReqItemId4`, `ReqItemCount1`, `ReqItemCount2`, `ReqItemCount3`, `ReqItemCount4`, `ReqSourceId1`, `ReqSourceId2`, `ReqSourceId3`, `ReqSourceId4`, `ReqSourceCount1`, `ReqSourceCount2`, `ReqSourceCount3`, `ReqSourceCount4`, `ReqCreatureOrGOId1`, `ReqCreatureOrGOId2`, `ReqCreatureOrGOId3`, `ReqCreatureOrGOId4`, `ReqCreatureOrGOCount1`, `ReqCreatureOrGOCount2`, `ReqCreatureOrGOCount3`, `ReqCreatureOrGOCount4`, `ReqSpellCast1`, `ReqSpellCast2`, `ReqSpellCast3`, `ReqSpellCast4`, `RewChoiceItemId1`, `RewChoiceItemId2`, `RewChoiceItemId3`, `RewChoiceItemId4`, `RewChoiceItemId5`, `RewChoiceItemId6`, `RewChoiceItemCount1`, `RewChoiceItemCount2`, `RewChoiceItemCount3`, `RewChoiceItemCount4`, `RewChoiceItemCount5`, `RewChoiceItemCount6`, `RewItemId1`, `RewItemId2`, `RewItemId3`, `RewItemId4`, `RewItemCount1`, `RewItemCount2`, `RewItemCount3`, `RewItemCount4`, `RewRepFaction1`, `RewRepFaction2`, `RewRepFaction3`, `RewRepFaction4`, `RewRepFaction5`, `RewRepValue1`, `RewRepValue2`, `RewRepValue3`, `RewRepValue4`, `RewRepValue5`, `RewXP`, `RewOrReqMoney`, `RewMoneyMaxLevel`, `RewSpell`, `RewSpellCast`, `RewMailTemplateId`, `RewMailDelaySecs`, `RewMailMoney`, `PointMapId`, `PointX`, `PointY`, `PointOpt`, `DetailsEmote1`, `DetailsEmote2`, `DetailsEmote3`, `DetailsEmote4`, `DetailsEmoteDelay1`, `DetailsEmoteDelay2`, `DetailsEmoteDelay3`, `DetailsEmoteDelay4`, `IncompleteEmote`, `CompleteEmote`, `OfferRewardEmote1`, `OfferRewardEmote2`, `OfferRewardEmote3`, `OfferRewardEmote4`, `OfferRewardEmoteDelay1`, `OfferRewardEmoteDelay2`, `OfferRewardEmoteDelay3`, `OfferRewardEmoteDelay4`, `StartScript`, `CompleteScript`) VALUES
(90100, 2, 0, 60, 0, 60, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 'Proven Before the Raid', 'So you want veterans in company kit, and you want them now. Fair enough. But veterans will not sign with someone who has not bled a little first.$B$BProve yourself against the dungeon lords of Blackrock, Scholomance, Stratholme, Dire Maul and the Karazhan Crypt. Bring my armourers six Righteous Orbs, twenty Dark Iron Ore, eight Essences of Undeath and six Living Essences, and pay the guild''s due of fifty gold. Do that, and your next hires march in the company''s Pre-raid kit.', 'Slay the dungeon lords. Bring 6 Righteous Orbs, 20 Dark Iron Ore, 8 Essences of Undeath and 6 Living Essences to a Mercenary Hire broker, and pay 50 gold.', 'Proof, materials, and the due paid in full. Well done.$B$BFrom now on your new level-60 hires can march in the company''s Pre-raid kit. Choose it in "Choose equipment" when you hire.', 'The dungeon lords still walk, or my stores are still short. Come back when both are settled, $N.', '', 'Dungeon lords slain', '', '', '', 12811, 11370, 12808, 12803, 6, 20, 8, 6, 0, 0, 0, 0, 0, 0, 0, 0, 95110, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -500000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(90101, 2, 0, 60, 0, 60, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 90100, 0, 0, 0, 0, 0, 0, 'Proven in Zul''Gurub', 'The Pre-raid kit earned you a hearing. The Zul''Gurub kit wants more: blood in Zul''Gurub, in the Ruins of Ahn''Qiraj and in the Lower Karazhan Halls.$B$BMy quartermasters also need four Bloodvine, two Massive Mojo, four Stone Scarabs and two Ancient Warfare Texts, and the guild''s due is one hundred gold. Bring all three, $N, and your hires dress for the jungle.', 'Slay the bosses of Zul''Gurub, the Ruins of Ahn''Qiraj and the Lower Karazhan Halls. Bring 4 Bloodvine, 2 Massive Mojo, 4 Stone Scarabs and 2 Ancient Warfare Texts to a Mercenary Hire broker, and pay 100 gold.', 'Zul''Gurub, the Ruins and the Halls behind you, and the due paid. Fine work.$B$BYour new level-60 hires can now wear the company''s Zul''Gurub kit. Choose it in "Choose equipment" when you hire.', 'Jungle, desert and haunted halls, $N, and my shelves are still not full. Finish all of it.', '', 'Zul''Gurub bosses slain', 'Ruins of Ahn''Qiraj bosses slain', 'Lower Karazhan Halls bosses slain', '', 19726, 19943, 20858, 70226, 4, 2, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 95111, 95112, 95113, 0, 8, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(90102, 2, 0, 60, 0, 60, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 90101, 0, 0, 0, 0, 0, 0, 'Proven in the Molten Core', 'Ragnaros does not care about your ambitions, and neither do his servants. Show the veterans the Molten Core bosses, Onyxia herself, and the brutes of Timbermaw Hold dead, and they will listen.$B$BThe forge needs ten Essences of Fire, six Fiery Cores, six Lava Cores and one Sulfuron Ingot. The due is two hundred gold. Hot work, $C, but the company''s Molten Core kit is worth it.', 'Slay the bosses of the Molten Core, Onyxia, and the bosses of Timbermaw Hold. Bring 10 Essences of Fire, 6 Fiery Cores, 6 Lava Cores and 1 Sulfuron Ingot to a Mercenary Hire broker, and pay 200 gold.', 'The Core quenched, Onyxia fallen, the due paid. The veterans approve.$B$BYour new level-60 hires can now wear the company''s Molten Core kit. Choose it in "Choose equipment" when you hire.', 'The Core still burns, Onyxia still flies, or my forge is short. Come back when it is all done.', '', 'Molten Core bosses slain', 'Onyxia slain', 'Timbermaw Hold bosses slain', '', 7078, 17010, 17011, 17203, 10, 6, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 95114, 95115, 95116, 0, 14, 1, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(90103, 2, 0, 60, 0, 60, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 90102, 0, 0, 0, 0, 0, 0, 'Proven in Blackwing Lair', 'Nefarian''s brood and the nightmares of the Emerald Sanctum. That is the price of the Blackwing Lair kit, and the veterans will take nothing less.$B$BBring six Elementium Ore, three Fading Dream Fragments, two Small Dream Shards and three Dreamscales for the armourers, and three hundred gold for the guild''s due.', 'Slay the bosses of Blackwing Lair and the Emerald Sanctum. Bring 6 Elementium Ore, 3 Fading Dream Fragments, 2 Small Dream Shards and 3 Dreamscales to a Mercenary Hire broker, and pay 300 gold.', 'Nefarian''s brood broken and the due paid. Impressive.$B$BYour new level-60 hires can now wear the company''s Blackwing Lair kit. Choose it in "Choose equipment" when you hire.', 'Blackwing Lair stands, the Sanctum dreams on, or my stores want more. Finish it, $N.', '', 'Blackwing Lair bosses slain', 'Emerald Sanctum bosses slain', '', '', 18562, 61197, 61198, 20381, 6, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 95117, 95118, 0, 0, 8, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -3000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(90104, 2, 0, 60, 0, 60, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 90103, 0, 0, 0, 0, 0, 0, 'Proven in Ahn''Qiraj', 'The Temple of Ahn''Qiraj. C''Thun stirs behind its walls, and only proven killers come back out. The veterans want proof you are one of them.$B$BThe guild''s scribes pay well for Qiraji relics: eight Gold Scarabs, eight Silver Scarabs and three Idols of the Sun, plus a due of four hundred gold. Then your hires march in the company''s Ahn''Qiraj kit.', 'Slay the bosses of the Temple of Ahn''Qiraj. Bring 8 Gold Scarabs, 8 Silver Scarabs and 3 Idols of the Sun to a Mercenary Hire broker, and pay 400 gold.', 'The Temple silenced and the due paid. Even the old hands are nodding.$B$BYour new level-60 hires can now wear the company''s Ahn''Qiraj kit. Choose it in "Choose equipment" when you hire.', 'The Temple still stands, or my scribes still wait on their relics. Come back when both are settled.', '', 'Temple of Ahn''Qiraj bosses slain', '', '', '', 20859, 20860, 20874, 0, 8, 8, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 95119, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(90105, 2, 0, 60, 0, 60, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 90104, 0, 0, 0, 0, 0, 0, 'Proven in Naxxramas', 'Naxxramas floats over the Plaguelands, and Kel''Thuzad''s dead do not scare easily. Neither do our veterans, which is why they ask this of you.$B$BClear the necropolis, bring eight of each Wartorn Scrap, cloth, leather, chain and plate, for the company''s seamstresses and smiths, and pay five hundred gold of the guild''s due. The Naxxramas kit will be yours to give.', 'Slay the bosses of Naxxramas. Bring 8 Wartorn Cloth Scraps, 8 Wartorn Leather Scraps, 8 Wartorn Chain Scraps and 8 Wartorn Plate Scraps to a Mercenary Hire broker, and pay 500 gold.', 'Naxxramas cleansed and the due paid. The company salutes you.$B$BYour new level-60 hires can now wear the company''s Naxxramas kit. Choose it in "Choose equipment" when you hire.', 'The dead still hold Naxxramas, or my workshops still want scraps. Finish it all, $N.', '', 'Naxxramas bosses slain', '', '', '', 22376, 22373, 22374, 22375, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 95120, 0, 0, 0, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -5000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(90106, 2, 0, 60, 0, 60, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 90105, 0, 0, 0, 0, 0, 0, 'Proven in Karazhan', 'The Tower of Karazhan. Medivh''s old halls, full of echoes and worse. No veteran signs with someone the Tower has not tested.$B$BBring proof from the Tower''s masters, four Pristine Ley Crystals for our enchanters, and seven hundred and fifty gold of the guild''s due. Do this, and your hires march in the company''s finest: the Karazhan kit.', 'Slay the bosses of the Tower of Karazhan. Bring 4 Pristine Ley Crystals to a Mercenary Hire broker, and pay 750 gold.', 'The Tower mastered and the due paid. There is nothing finer.$B$BYour new level-60 hires can now wear the company''s Karazhan kit, the finest we own. Choose it in "Choose equipment" when you hire.', 'The Tower still holds its masters, or my enchanters still want their crystals. Come back when it is done.', '', 'Tower of Karazhan bosses slain', '', '', '', 41485, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 95121, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -7500000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

-- Invisible kill-credit entries (never spawned; the module grants credit by
-- entry id). Minimal template rows: neutral critters, no gossip, no loot.
INSERT INTO `creature_template` (`entry`, `display_id1`, `name`, `subname`, `level_min`, `level_max`, `health_min`, `health_max`, `faction`, `npc_flags`, `rank`, `type`, `type_flags`, `script_name`) VALUES
(95110, 0, 'Hire Unlock Credit: Dungeons', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95111, 0, 'Hire Unlock Credit: Zul''Gurub', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95112, 0, 'Hire Unlock Credit: Ruins of Ahn''Qiraj', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95113, 0, 'Hire Unlock Credit: Lower Karazhan', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95114, 0, 'Hire Unlock Credit: Molten Core', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95115, 0, 'Hire Unlock Credit: Onyxia', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95116, 0, 'Hire Unlock Credit: Timbermaw Hold', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95117, 0, 'Hire Unlock Credit: Blackwing Lair', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95118, 0, 'Hire Unlock Credit: Emerald Sanctum', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95119, 0, 'Hire Unlock Credit: Temple of Ahn''Qiraj', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95120, 0, 'Hire Unlock Credit: Naxxramas', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, ''),
(95121, 0, 'Hire Unlock Credit: Tower of Karazhan', '', 60, 60, 100, 100, 35, 0, 0, 7, 0, '');

-- Starters + enders: all 7 neutral quests on all 6 capital recruiters.
-- Alliance: SW 95017, IF 95006, Darnassus 95012. Horde: Org 95025, UC 95018, TB 95019.
INSERT INTO `creature_questrelation` (`id`, `quest`) VALUES
(95017, 90100),
(95006, 90100),
(95012, 90100),
(95025, 90100),
(95018, 90100),
(95019, 90100),
(95017, 90101),
(95006, 90101),
(95012, 90101),
(95025, 90101),
(95018, 90101),
(95019, 90101),
(95017, 90102),
(95006, 90102),
(95012, 90102),
(95025, 90102),
(95018, 90102),
(95019, 90102),
(95017, 90103),
(95006, 90103),
(95012, 90103),
(95025, 90103),
(95018, 90103),
(95019, 90103),
(95017, 90104),
(95006, 90104),
(95012, 90104),
(95025, 90104),
(95018, 90104),
(95019, 90104),
(95017, 90105),
(95006, 90105),
(95012, 90105),
(95025, 90105),
(95018, 90105),
(95019, 90105),
(95017, 90106),
(95006, 90106),
(95012, 90106),
(95025, 90106),
(95018, 90106),
(95019, 90106);
INSERT INTO `creature_involvedrelation` (`id`, `quest`) VALUES
(95017, 90100),
(95006, 90100),
(95012, 90100),
(95025, 90100),
(95018, 90100),
(95019, 90100),
(95017, 90101),
(95006, 90101),
(95012, 90101),
(95025, 90101),
(95018, 90101),
(95019, 90101),
(95017, 90102),
(95006, 90102),
(95012, 90102),
(95025, 90102),
(95018, 90102),
(95019, 90102),
(95017, 90103),
(95006, 90103),
(95012, 90103),
(95025, 90103),
(95018, 90103),
(95019, 90103),
(95017, 90104),
(95006, 90104),
(95012, 90104),
(95025, 90104),
(95018, 90104),
(95019, 90104),
(95017, 90105),
(95006, 90105),
(95012, 90105),
(95025, 90105),
(95018, 90105),
(95019, 90105),
(95017, 90106),
(95006, 90106),
(95012, 90106),
(95025, 90106),
(95018, 90106),
(95019, 90106);
