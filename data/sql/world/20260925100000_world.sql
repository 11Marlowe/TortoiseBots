-- <Mercenary Hire> recruiters: opposite gender to the innkeeper beside them.
--
-- Base migration 20260918120000_world.sql clones each innkeeper's display and
-- first name, so every recruiter (95000-95067) is a twin of its innkeeper.
-- Give each one a display of the same race and the opposite gender, and a new
-- name that fits that gender (a male "Recruiter Renee" would read wrong).
--
-- Displays come from the 1.18.1 CreatureDisplayInfo/-Extra DBCs: same
-- DisplayRaceID, opposite DisplaySexID, fully dressed (3+ item slots), taken
-- from an existing vendor/trainer NPC so the look is known to render. No
-- innkeeper display is reused (no recruiter looks like another innkeeper),
-- and each recruiter gets a distinct display.
--
-- Recruiter Lard (95046) keeps the base display: its innkeeper uses a
-- non-humanoid model (display 10714, model 32) with no gendered variant.
--
-- Idempotent: plain UPDATEs keyed by entry. The base migration is left
-- untouched (already applied; the updater tracks files by hash).
-- After apply: .reload creature_template (or restart mangosd).
UPDATE `creature_template` SET `display_id1` = 1287, `name` = 'Recruiter Marla' WHERE `entry` = 95000; -- female, display of Corina Steele
UPDATE `creature_template` SET `display_id1` = 1620, `name` = 'Recruiter Bryndis' WHERE `entry` = 95001; -- female, display of Wenna Silkbeard
UPDATE `creature_template` SET `display_id1` = 1839, `name` = 'Recruiter Gerdi' WHERE `entry` = 95002; -- female, display of Irene Sureshot
UPDATE `creature_template` SET `display_id1` = 1295, `name` = 'Recruiter Edith' WHERE `entry` = 95003; -- female, display of Priestess Josetta
UPDATE `creature_template` SET `display_id1` = 1592, `name` = 'Recruiter Morwen' WHERE `entry` = 95004; -- female, display of Isabella
UPDATE `creature_template` SET `display_id1` = 1322, `name` = 'Recruiter Nahla Plainswind' WHERE `entry` = 95005; -- female, display of Kaja
UPDATE `creature_template` SET `display_id1` = 1841, `name` = 'Recruiter Dagna Firebrew' WHERE `entry` = 95006; -- female, display of Kali Healtouch
UPDATE `creature_template` SET `display_id1` = 1575, `name` = 'Recruiter Aldous' WHERE `entry` = 95007; -- male, display of Blacksmith Rand
UPDATE `creature_template` SET `display_id1` = 1312, `name` = 'Recruiter Grasha' WHERE `entry` = 95008; -- female, display of Nulda
UPDATE `creature_template` SET `display_id1` = 1288, `name` = 'Recruiter Tobias' WHERE `entry` = 95009; -- male, display of Smith Argus
UPDATE `creature_template` SET `display_id1` = 1289, `name` = 'Recruiter Gerard' WHERE `entry` = 95010; -- male, display of Ovan Gradal
UPDATE `creature_template` SET `display_id1` = 1843, `name` = 'Recruiter Helga Hearthstove' WHERE `entry` = 95011; -- female, display of Khara Deepwater
UPDATE `creature_template` SET `display_id1` = 1701, `name` = 'Recruiter Taeloran' WHERE `entry` = 95012; -- male, display of Brannol Eaglemoon
UPDATE `creature_template` SET `display_id1` = 1702, `name` = 'Recruiter Elunara' WHERE `entry` = 95013; -- female, display of Cyndra Kindwhisper
UPDATE `creature_template` SET `display_id1` = 1703, `name` = 'Recruiter Arathel' WHERE `entry` = 95014; -- male, display of Dazalar
UPDATE `creature_template` SET `display_id1` = 1704, `name` = 'Recruiter Vaerion' WHERE `entry` = 95015; -- male, display of Jannok Breezesong
UPDATE `creature_template` SET `display_id1` = 1601, `name` = 'Recruiter Lucinda' WHERE `entry` = 95016; -- female, display of Carolai Anise
UPDATE `creature_template` SET `display_id1` = 1290, `name` = 'Recruiter Walter' WHERE `entry` = 95017; -- male, display of Eldrin
UPDATE `creature_template` SET `display_id1` = 1603, `name` = 'Recruiter Agatha' WHERE `entry` = 95018; -- female, display of Marion Call
UPDATE `creature_template` SET `display_id1` = 2082, `name` = 'Recruiter Tahkan' WHERE `entry` = 95019; -- male, display of Beram Skychaser
UPDATE `creature_template` SET `display_id1` = 2107, `name` = 'Recruiter Hania' WHERE `entry` = 95020; -- female, display of Aska Mistrunner
UPDATE `creature_template` SET `display_id1` = 1293, `name` = 'Recruiter Roland' WHERE `entry` = 95021; -- male, display of Tomas
UPDATE `creature_template` SET `display_id1` = 7909, `name` = 'Recruiter Zixi' WHERE `entry` = 95022; -- female, display of Krixil Slogswitch
UPDATE `creature_template` SET `display_id1` = 8630, `name` = 'Recruiter Zazzle' WHERE `entry` = 95023; -- female, display of Sasha Linelight
UPDATE `creature_template` SET `display_id1` = 1319, `name` = 'Recruiter Durza' WHERE `entry` = 95024; -- female, display of Sana
UPDATE `creature_template` SET `display_id1` = 1314, `name` = 'Recruiter Throk' WHERE `entry` = 95025; -- male, display of Otor Nar'gakk
UPDATE `creature_template` SET `display_id1` = 1325, `name` = 'Recruiter Kashka' WHERE `entry` = 95026; -- female, display of Mirket
UPDATE `creature_template` SET `display_id1` = 2108, `name` = 'Recruiter Mahala' WHERE `entry` = 95027; -- female, display of Bena Winterhoof
UPDATE `creature_template` SET `display_id1` = 1315, `name` = 'Recruiter Gorzak' WHERE `entry` = 95028; -- male, display of Tor'phan
UPDATE `creature_template` SET `display_id1` = 8665, `name` = 'Recruiter Fizzlie' WHERE `entry` = 95029; -- female, display of Yuka Screwspigot
UPDATE `creature_template` SET `display_id1` = 1706, `name` = 'Recruiter Ilthalan' WHERE `entry` = 95030; -- male, display of Kal
UPDATE `creature_template` SET `display_id1` = 2083, `name` = 'Recruiter Tarak' WHERE `entry` = 95031; -- male, display of Brek Stonehoof
UPDATE `creature_template` SET `display_id1` = 1981, `name` = 'Recruiter Kerra' WHERE `entry` = 95032; -- female, display of Yarlyn Amberstill
UPDATE `creature_template` SET `display_id1` = 1294, `name` = 'Recruiter Harold' WHERE `entry` = 95033; -- male, display of Zaldimar Wefhellt
UPDATE `creature_template` SET `display_id1` = 2109, `name` = 'Recruiter Sahnee' WHERE `entry` = 95034; -- female, display of Fyr Mistrunner
UPDATE `creature_template` SET `display_id1` = 1709, `name` = 'Recruiter Mylandris' WHERE `entry` = 95035; -- male, display of Malorne Bladeleaf
UPDATE `creature_template` SET `display_id1` = 2084, `name` = 'Recruiter Takoda' WHERE `entry` = 95036; -- male, display of Delgo Ragetotem
UPDATE `creature_template` SET `display_id1` = 2110, `name` = 'Recruiter Wenona' WHERE `entry` = 95037; -- female, display of Jyn Stonehoof
UPDATE `creature_template` SET `display_id1` = 7031, `name` = 'Recruiter Sprock' WHERE `entry` = 95038; -- male, display of Bro'kin
UPDATE `creature_template` SET `display_id1` = 1316, `name` = 'Recruiter Brogar' WHERE `entry` = 95039; -- male, display of Handor
UPDATE `creature_template` SET `display_id1` = 1711, `name` = 'Recruiter Kaldris' WHERE `entry` = 95040; -- male, display of Narret Shadowgrove
UPDATE `creature_template` SET `display_id1` = 9132, `name` = 'Recruiter Tizzy' WHERE `entry` = 95041; -- female, display of Ohgi Cardya
UPDATE `creature_template` SET `display_id1` = 2111, `name` = 'Recruiter Taima' WHERE `entry` = 95042; -- female, display of Kaga Mistrunner
UPDATE `creature_template` SET `display_id1` = 9780, `name` = 'Recruiter Pixxa' WHERE `entry` = 95043; -- female, display of Waitress Peenqi
UPDATE `creature_template` SET `display_id1` = 1298, `name` = 'Recruiter Bernard' WHERE `entry` = 95044; -- male, display of Tharynn Bouden
UPDATE `creature_template` SET `display_id1` = 1607, `name` = 'Recruiter Hester' WHERE `entry` = 95045; -- female, display of Shelene Rhobart
UPDATE `creature_template` SET `display_id1` = 1712, `name` = 'Recruiter Faeren' WHERE `entry` = 95047; -- male, display of Shalomon
UPDATE `creature_template` SET `display_id1` = 1299, `name` = 'Recruiter Jonas Chambers' WHERE `entry` = 95048; -- male, display of Brother Wilhelm
UPDATE `creature_template` SET `display_id1` = 2735, `name` = 'Recruiter Rakjin' WHERE `entry` = 95049; -- male, display of Xen'to
UPDATE `creature_template` SET `display_id1` = 1296, `name` = 'Recruiter Ruby' WHERE `entry` = 95050; -- female, display of Michelle Belle
UPDATE `creature_template` SET `display_id1` = 1317, `name` = 'Recruiter Garuk' WHERE `entry` = 95051; -- male, display of Ollanus
UPDATE `creature_template` SET `display_id1` = 9791, `name` = 'Recruiter Nixxi' WHERE `entry` = 95052; -- female, display of Razza Sparkfizzle
UPDATE `creature_template` SET `display_id1` = 1622, `name` = 'Recruiter Bolgar' WHERE `entry` = 95053; -- male, display of Azar Stronghammer
UPDATE `creature_template` SET `display_id1` = 10744, `name` = 'Recruiter Wizza' WHERE `entry` = 95054; -- female, display of Lunnix Sprocketslip
UPDATE `creature_template` SET `display_id1` = 1423, `name` = 'Recruiter Laurent' WHERE `entry` = 95055; -- male, display of Aldric Moore
UPDATE `creature_template` SET `display_id1` = 1705, `name` = 'Recruiter Talwyn' WHERE `entry` = 95056; -- female, display of Jeena Featherbow
UPDATE `creature_template` SET `display_id1` = 1890, `name` = 'Recruiter Bimsy' WHERE `entry` = 95057; -- female, display of Clover Spinpistol
UPDATE `creature_template` SET `display_id1` = 10745, `name` = 'Recruiter Kezzi' WHERE `entry` = 95058; -- female, display of Wizette Icewhistle
UPDATE `creature_template` SET `display_id1` = 7010, `name` = 'Recruiter Aeldran' WHERE `entry` = 95059; -- male, display of Lothos Riftwaker
UPDATE `creature_template` SET `display_id1` = 16037, `name` = 'Recruiter Belorin' WHERE `entry` = 95060; -- male, display of Scribe Nahele
UPDATE `creature_template` SET `display_id1` = 3293, `name` = 'Recruiter Lirael' WHERE `entry` = 95061; -- female, display of Elsharin
UPDATE `creature_template` SET `display_id1` = 16055, `name` = 'Recruiter Caelen' WHERE `entry` = 95062; -- male, display of Callon Sunsail
UPDATE `creature_template` SET `display_id1` = 1832, `name` = 'Recruiter Fizzwick' WHERE `entry` = 95063; -- male, display of Deek Fizzlebizz
UPDATE `creature_template` SET `display_id1` = 7036, `name` = 'Recruiter Grizz' WHERE `entry` = 95064; -- male, display of Cherox Whizzbake
UPDATE `creature_template` SET `display_id1` = 4068, `name` = 'Recruiter Zanjo' WHERE `entry` = 95065; -- male, display of Ken'jai
UPDATE `creature_template` SET `display_id1` = 1685, `name` = 'Recruiter Durgan' WHERE `entry` = 95066; -- male, display of Rufus Hardwick
UPDATE `creature_template` SET `display_id1` = 1297, `name` = 'Recruiter Martha' WHERE `entry` = 95067; -- female, display of Keryn Sylvius
