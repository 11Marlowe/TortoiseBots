package armory

import (
	"database/sql"
	"fmt"
	"strings"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

type Config struct {
	DBHost     string
	DBPort     int
	DBUser     string
	DBPassword string
	CharDB     string
	WorldDB    string
	LoginDB    string
	// BotAccountPrefix matches AiPlayerbot.RandomBotAccountPrefix
	// (ai/playerbot/PlayerbotAIConfig.cpp, default "rndbot").
	BotAccountPrefix string
	// DBCDir optionally points at the operator's own extracted DBC files
	// (the same dir mangosd reads via DataDir). When Talent.dbc +
	// TalentTab.dbc are present, talents resolve from them; otherwise the
	// backend falls back to the world talent/talenttab mirror tables.
	DBCDir string
}

type Service struct {
	cfg Config
	db  *sql.DB
}

func NewService(cfg Config) (*Service, error) {
	if cfg.BotAccountPrefix == "" {
		cfg.BotAccountPrefix = "rndbot"
	}
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/%s?timeout=5s&parseTime=false",
		cfg.DBUser, cfg.DBPassword, cfg.DBHost, cfg.DBPort, cfg.CharDB)

	db, err := sql.Open("mysql", dsn)
	if err != nil {
		return nil, fmt.Errorf("failed to open database: %w", err)
	}

	// Diagnostic tool, not a hot path.
	db.SetMaxOpenConns(5)
	db.SetMaxIdleConns(2)
	db.SetConnMaxLifetime(5 * time.Minute)

	return &Service{cfg: cfg, db: db}, nil
}

// likeEscape guards the two LIKE inputs (bot prefix, name search) against
// callers smuggling % _ or \ into the match.
func likeEscape(s string) string {
	s = strings.ReplaceAll(s, `\`, `\\`)
	s = strings.ReplaceAll(s, `%`, `\%`)
	s = strings.ReplaceAll(s, `_`, `\_`)
	return s
}

func (s *Service) ListBots(query string) ([]BotSummary, error) {
	sqlQuery := fmt.Sprintf(`
		SELECT c.guid, c.name, c.race, c.class, c.gender, c.level, c.money, c.totaltime, c.online
		FROM characters c
		JOIN %s.account a ON c.account = a.id
		WHERE a.username LIKE ? ESCAPE '\\' AND c.deleteDate IS NULL
	`, s.cfg.LoginDB)

	args := []interface{}{likeEscape(s.cfg.BotAccountPrefix) + "%"}
	if query != "" {
		sqlQuery += " AND c.name LIKE ? ESCAPE '\\\\'"
		args = append(args, "%"+likeEscape(query)+"%")
	}
	sqlQuery += " ORDER BY c.name LIMIT 1000"

	rows, err := s.db.Query(sqlQuery, args...)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	bots := []BotSummary{}
	for rows.Next() {
		var b BotSummary
		if err := rows.Scan(&b.GUID, &b.Name, &b.Race, &b.Class, &b.Gender, &b.Level, &b.Money, &b.TotalTime, &b.Online); err != nil {
			return nil, err
		}
		bots = append(bots, b)
	}
	return bots, rows.Err()
}

func (s *Service) GetBotProfile(guid uint32) (*BotProfile, error) {
	var profile BotProfile

	// 1. Identity: same columns the login query holder already loads; the
	// caller decides whether a name is a bot by the account prefix check.
	summaryQuery := fmt.Sprintf(`
		SELECT c.guid, c.name, c.race, c.class, c.gender, c.level, c.money, c.totaltime, c.online
		FROM characters c
		JOIN %s.account a ON c.account = a.id
		WHERE c.guid = ? AND a.username LIKE ? ESCAPE '\\' AND c.deleteDate IS NULL
	`, s.cfg.LoginDB)
	err := s.db.QueryRow(summaryQuery, guid, likeEscape(s.cfg.BotAccountPrefix)+"%").Scan(
		&profile.Summary.GUID, &profile.Summary.Name, &profile.Summary.Race,
		&profile.Summary.Class, &profile.Summary.Gender, &profile.Summary.Level,
		&profile.Summary.Money, &profile.Summary.TotalTime, &profile.Summary.Online)
	if err != nil {
		if err == sql.ErrNoRows {
			return nil, fmt.Errorf("bot %d not found", guid)
		}
		return nil, err
	}

	// 2. Equipment: bag=0 + slot 0-18, mirroring Player::_LoadInventory
	// (IsEquipmentPos(INVENTORY_SLOT_BAG_0, slot)). Stack count comes from
	// item_instance; that JOIN is the same one the core login query uses.
	// detailSelect appends the tooltip columns consumed by scanDetailTail.
	eqQuery := fmt.Sprintf(`
		SELECT ci.slot, ci.item, ci.item_template, COALESCE(ii.`+"`count`"+`, 1),
		       it.name, it.quality, it.item_level, it.inventory_type, it.display_id,
		       COALESCE(idi.icon, '') AS icon, %s
		FROM character_inventory ci
		JOIN %s.item_template it ON ci.item_template = it.entry
		LEFT JOIN %s.item_display_info idi ON it.display_id = idi.ID
		LEFT JOIN item_instance ii ON ii.guid = ci.item
		WHERE ci.guid = ? AND ci.bag = 0 AND ci.slot >= 0 AND ci.slot < %d
		ORDER BY ci.slot
	`, detailSelect, s.cfg.WorldDB, s.cfg.WorldDB, EquipmentSlotEnd)

	if err := s.scanEquipped(eqQuery, guid, &profile); err != nil {
		return nil, err
	}
	bagQuery := fmt.Sprintf(`
		SELECT ci.slot, ci.item, ci.item_template, COALESCE(ii.`+"`count`"+`, 1),
		       it.name, it.quality, it.container_slots, it.display_id,
		       COALESCE(idi.icon, '') AS icon, %s
		FROM character_inventory ci
		JOIN %s.item_template it ON ci.item_template = it.entry
		LEFT JOIN %s.item_display_info idi ON it.display_id = idi.ID
		LEFT JOIN item_instance ii ON ii.guid = ci.item
		WHERE ci.guid = ? AND ci.bag = 0 AND ci.slot >= %d AND ci.slot < %d
	`, detailSelect, s.cfg.WorldDB, s.cfg.WorldDB, BagSlotStart, BagSlotEnd)

	if err := s.scanBags(bagQuery, guid, &profile); err != nil {
		return nil, err
	}

	// Bag contents: ci.bag holds the item-instance GUID of the container
	// (Player::_SaveInventory writes container->GetGUIDLow() into bag). The
	// legacy frontend compared against the visible 19-22 number; that never
	// matches, so this joins back through the container row to resolve the
	// instance GUID for each equipped bag.
	for i := range profile.Bags {
		contentQuery := fmt.Sprintf(`
			SELECT ci.slot, ci.item_template, COALESCE(ii.`+"`count`"+`, 1),
			       it.name, it.quality, it.display_id,
			       COALESCE(idi.icon, '') AS icon, %s
			FROM character_inventory ci
			JOIN character_inventory container
			  ON container.guid = ci.guid
			 AND container.bag = 0
			 AND container.slot = ?
			 AND container.item = ci.bag
			JOIN %s.item_template it ON ci.item_template = it.entry
			LEFT JOIN %s.item_display_info idi ON it.display_id = idi.ID
			LEFT JOIN item_instance ii ON ii.guid = ci.item
			WHERE ci.guid = ? AND ci.bag != 0
			ORDER BY ci.slot
		`, detailSelect, s.cfg.WorldDB, s.cfg.WorldDB)
		rows, err := s.db.Query(contentQuery, profile.Bags[i].Slot, guid)
		if err != nil {
			return nil, err
		}
		for rows.Next() {
			var bi BagItem
			var st, sv [10]int32
			var sp [5]uint32
			var tr [5]uint8
			base := []interface{}{&bi.Slot, &bi.ItemTemplate, &bi.Count, &bi.Name, &bi.Quality, &bi.DisplayID, &bi.Icon}
			if err := rows.Scan(append(base, detailDests(&bi.Detail, &st, &sv, &sp, &tr)...)...); err != nil {
				rows.Close()
				return nil, err
			}
			foldDetail(&bi.Detail, st, sv, sp, tr)
			s.resolveSpellNames(&bi.Detail)
			profile.Bags[i].Items = append(profile.Bags[i].Items, bi)
		}
		rows.Close()
		if err := rows.Err(); err != nil {
			return nil, err
		}
		if profile.Bags[i].Items == nil {
			profile.Bags[i].Items = []BagItem{}
		}
	}

	bpQuery := fmt.Sprintf(`
		SELECT ci.slot, ci.item_template, COALESCE(ii.`+"`count`"+`, 1),
		       it.name, it.quality, it.display_id,
		       COALESCE(idi.icon, '') AS icon, %s
		FROM character_inventory ci
		JOIN %s.item_template it ON ci.item_template = it.entry
		LEFT JOIN %s.item_display_info idi ON it.display_id = idi.ID
		LEFT JOIN item_instance ii ON ii.guid = ci.item
		WHERE ci.guid = ? AND ci.bag = 0 AND ci.slot >= %d AND ci.slot < %d
		ORDER BY ci.slot
	`, detailSelect, s.cfg.WorldDB, s.cfg.WorldDB, BackpackStart, BackpackEnd)

	if err := s.scanBackpack(bpQuery, guid, &profile); err != nil {
		return nil, err
	}

	// 4b. Buyback: vendor-sold items still recoverable (slots 69-80).
	bbQuery := fmt.Sprintf(`
		SELECT ci.slot, ci.item_template, COALESCE(ii.`+"`count`"+`, 1),
		       it.name, it.quality, it.display_id,
		       COALESCE(idi.icon, '') AS icon, %s
		FROM character_inventory ci
		JOIN %s.item_template it ON ci.item_template = it.entry
		LEFT JOIN %s.item_display_info idi ON it.display_id = idi.ID
		LEFT JOIN item_instance ii ON ii.guid = ci.item
		WHERE ci.guid = ? AND ci.bag = 0 AND ci.slot >= %d AND ci.slot < %d
		ORDER BY ci.slot
	`, detailSelect, s.cfg.WorldDB, s.cfg.WorldDB, BuybackStart, BuybackEnd)

	if err := s.scanBuyback(bbQuery, guid, &profile); err != nil {
		return nil, err
	}
	// 5. Stats: full armory row first, core character_stats subset, then live
	// characters.health/power + player_levelstats base attributes. Both
	// snapshot tables stay empty while a bot is online (core writes them
	// only on logout), so without the live layer every online bot is 0/0%.
	statQuery := `SELECT maxhealth, maxpower1, maxpower2, maxpower3, maxpower4, maxpower5,
		strength, agility, stamina, intellect, spirit, armor,
		resHoly, resFire, resNature, resFrost, resShadow, resArcane,
		blockPct, dodgePct, parryPct, meleeCritPct, rangedCritPct,
		attackPower, rangedAttackPower, meleeDamage, rangedDamage,
		meleeWeaponSpeed, rangedWeaponSpeed, castSpeed, meleeHit, rangedHit, spellHit
		FROM character_armory_stats WHERE guid = ?`
	err = s.db.QueryRow(statQuery, guid).Scan(
		&profile.Stats.MaxHealth, &profile.Stats.MaxPower1, &profile.Stats.MaxPower2,
		&profile.Stats.MaxPower3, &profile.Stats.MaxPower4, &profile.Stats.MaxPower5,
		&profile.Stats.Strength, &profile.Stats.Agility, &profile.Stats.Stamina,
		&profile.Stats.Intellect, &profile.Stats.Spirit, &profile.Stats.Armor,
		&profile.Stats.ResHoly, &profile.Stats.ResFire, &profile.Stats.ResNature,
		&profile.Stats.ResFrost, &profile.Stats.ResShadow, &profile.Stats.ResArcane,
		&profile.Stats.BlockPct, &profile.Stats.DodgePct, &profile.Stats.ParryPct,
		&profile.Stats.MeleeCritPct, &profile.Stats.RangedCritPct,
		&profile.Stats.AttackPower, &profile.Stats.RangedAttackPower,
		&profile.Stats.MeleeDamage, &profile.Stats.RangedDamage,
		&profile.Stats.MeleeSpeed, &profile.Stats.RangedSpeed, &profile.Stats.CastSpeed,
		&profile.Stats.MeleeHit, &profile.Stats.RangedHit, &profile.Stats.SpellHit)
	if err != nil {
		fallbackQuery := `SELECT maxhealth, maxpower1, maxpower2, maxpower3, maxpower4,
			strength, agility, stamina, intellect, spirit, armor,
			resHoly, resFire, resNature, resFrost, resShadow, resArcane,
			blockPct, dodgePct, parryPct, critPct, rangedCritPct,
			attackPower, rangedAttackPower
			FROM character_stats WHERE guid = ?`
		if ferr := s.db.QueryRow(fallbackQuery, guid).Scan(
			&profile.Stats.MaxHealth, &profile.Stats.MaxPower1, &profile.Stats.MaxPower2,
			&profile.Stats.MaxPower3, &profile.Stats.MaxPower4,
			&profile.Stats.Strength, &profile.Stats.Agility, &profile.Stats.Stamina,
			&profile.Stats.Intellect, &profile.Stats.Spirit, &profile.Stats.Armor,
			&profile.Stats.ResHoly, &profile.Stats.ResFire, &profile.Stats.ResNature,
			&profile.Stats.ResFrost, &profile.Stats.ResShadow, &profile.Stats.ResArcane,
			&profile.Stats.BlockPct, &profile.Stats.DodgePct, &profile.Stats.ParryPct,
			&profile.Stats.MeleeCritPct, &profile.Stats.RangedCritPct,
			&profile.Stats.AttackPower, &profile.Stats.RangedAttackPower); ferr == nil {
			profile.Stats.Source = "character_stats"
		} else {
			s.loadLiveStats(guid, &profile.Stats)
		}
	} else {
		profile.Stats.Source = "armory_stats"
	}

	// 6. Spells: same columns the core loads (spell,active,disabled) plus
	// display metadata from the operator's own world DB. Description
	// disambiguates shared names (pet-teach Survival Instinct vs talent).
	spellQuery := fmt.Sprintf(`
		SELECT cs.spell, cs.active, cs.disabled,
		       COALESCE(st.name, ''), COALESCE(st.nameSubtext, ''), COALESCE(st.description, ''),
		       COALESCE(st.effectBasePoints1, 0) + COALESCE(st.effectBaseDice1, 0),
		       COALESCE(st.effectBasePoints2, 0) + COALESCE(st.effectBaseDice2, 0),
		       COALESCE(st.effectBasePoints3, 0) + COALESCE(st.effectBaseDice3, 0),
		       COALESCE(st.effectMiscValue1, 0), COALESCE(st.effectMiscValue2, 0), COALESCE(st.effectMiscValue3, 0),
		       COALESCE(st.effectTriggerSpell1, 0), COALESCE(st.effectTriggerSpell2, 0), COALESCE(st.effectTriggerSpell3, 0),
		       COALESCE(st.durationIndex, 0), COALESCE(st.rangeIndex, 0), COALESCE(st.castingTimeIndex, 0),
		       COALESCE(st.school, 0), COALESCE(st.spellIconId, 0),
		       COALESCE(si.Name, '')
		FROM character_spell cs
		LEFT JOIN %s.spell_template st ON st.entry = cs.spell
		LEFT JOIN %s.spellicon si ON si.ID = st.spellIconId
		WHERE cs.guid = ?
		ORDER BY st.name, cs.spell
	`, s.cfg.WorldDB, s.cfg.WorldDB)
	spRows, err := s.db.Query(spellQuery, guid)
	if err != nil {
		return nil, err
	}
	defer spRows.Close()
	profile.Spells = []SpellEntry{}
	for spRows.Next() {
		var se SpellEntry
		var v1, v2, v3 int32
		var m1, m2, m3 int32
		var t1, t2, t3 uint32
		var durIdx, rngIdx, castIdx uint32
		if err := spRows.Scan(&se.Spell, &se.Active, &se.Disabled, &se.Name, &se.Subtext, &se.Description,
			&v1, &v2, &v3, &m1, &m2, &m3, &t1, &t2, &t3, &durIdx, &rngIdx, &castIdx,
			&se.School, &se.IconID, &se.Icon); err != nil {
			return nil, err
		}
		if se.Icon == "" && se.IconID != 0 {
			se.Icon = s.spellIconByID(se.IconID)
		}
		se.Values = []int32{v1, v2, v3}
		se.Misc = []int32{m1, m2, m3}
		se.Triggers = []uint32{t1, t2, t3}
		se.DurationMs, se.RangeYd, se.CastMs = s.spellTiming(durIdx, rngIdx, castIdx)
		profile.Spells = append(profile.Spells, se)
	}
	if err := spRows.Err(); err != nil {
		return nil, err
	}

	// 7. Skills: every row; the frontend filters weapon/armor categories.
	skillQuery := "SELECT skill, value, `max` FROM character_skills WHERE guid = ? ORDER BY skill"
	skRows, err := s.db.Query(skillQuery, guid)
	if err != nil {
		return nil, err
	}
	defer skRows.Close()
	profile.Skills = []SkillEntry{}
	for skRows.Next() {
		var se SkillEntry
		if err := skRows.Scan(&se.Skill, &se.Value, &se.Max); err != nil {
			return nil, err
		}
		profile.Skills = append(profile.Skills, se)
	}
	if err := skRows.Err(); err != nil {
		return nil, err
	}

	// 8. Talents: resolved from the operator's world DB talent/talenttab
	// mirror tables when present. The core itself loads Talent/TalentTab from
	// the operator's own DBC files at startup, so these mirrors are often
	// empty; an empty Talents list then means "no data", not "no talents".
	talents, err := s.loadTalents(guid)
	if err != nil {
		return nil, err
	}
	profile.Talents = talents

	return &profile, nil
}

func (s *Service) scanEquipped(query string, guid uint32, profile *BotProfile) error {
	rows, err := s.db.Query(query, guid)
	if err != nil {
		return err
	}
	defer rows.Close()
	profile.Equipment = []EquippedItem{}
	for rows.Next() {
		var eq EquippedItem
		var itemGUID uint32
		var st, sv [10]int32
		var sp [5]uint32
		var tr [5]uint8
		base := []interface{}{&eq.Slot, &itemGUID, &eq.ItemTemplate, &eq.Count, &eq.Name, &eq.Quality, &eq.ItemLevel, &eq.InventoryType, &eq.DisplayID, &eq.Icon}
		if err := rows.Scan(append(base, detailDests(&eq.Detail, &st, &sv, &sp, &tr)...)...); err != nil {
			return err
		}
		foldDetail(&eq.Detail, st, sv, sp, tr)
		s.resolveSpellNames(&eq.Detail)
		profile.Equipment = append(profile.Equipment, eq)
	}
	return rows.Err()
}

func (s *Service) scanBags(query string, guid uint32, profile *BotProfile) error {
	rows, err := s.db.Query(query, guid)
	if err != nil {
		return err
	}
	defer rows.Close()
	profile.Bags = []BagContainer{}
	for rows.Next() {
		var b BagContainer
		var st, sv [10]int32
		var sp [5]uint32
		var tr [5]uint8
		base := []interface{}{&b.Slot, &b.Item, &b.ItemTemplate, &b.Count, &b.Name, &b.Quality, &b.ContainerSlots, &b.DisplayID, &b.Icon}
		if err := rows.Scan(append(base, detailDests(&b.Detail, &st, &sv, &sp, &tr)...)...); err != nil {
			return err
		}
		foldDetail(&b.Detail, st, sv, sp, tr)
		s.resolveSpellNames(&b.Detail)
		b.Items = []BagItem{}
		profile.Bags = append(profile.Bags, b)
	}
	return rows.Err()
}

func (s *Service) scanBackpack(query string, guid uint32, profile *BotProfile) error {
	rows, err := s.db.Query(query, guid)
	if err != nil {
		return err
	}
	defer rows.Close()
	profile.Backpack = []BagItem{}
	for rows.Next() {
		var bi BagItem
		var st, sv [10]int32
		var sp [5]uint32
		var tr [5]uint8
		base := []interface{}{&bi.Slot, &bi.ItemTemplate, &bi.Count, &bi.Name, &bi.Quality, &bi.DisplayID, &bi.Icon}
		if err := rows.Scan(append(base, detailDests(&bi.Detail, &st, &sv, &sp, &tr)...)...); err != nil {
			return err
		}
		foldDetail(&bi.Detail, st, sv, sp, tr)
		s.resolveSpellNames(&bi.Detail)
		profile.Backpack = append(profile.Backpack, bi)
	}
	return rows.Err()
}

func (s *Service) scanBuyback(query string, guid uint32, profile *BotProfile) error {
	rows, err := s.db.Query(query, guid)
	if err != nil {
		return err
	}
	defer rows.Close()
	profile.Buyback = []BagItem{}
	for rows.Next() {
		var bi BagItem
		var st, sv [10]int32
		var sp [5]uint32
		var tr [5]uint8
		base := []interface{}{&bi.Slot, &bi.ItemTemplate, &bi.Count, &bi.Name, &bi.Quality, &bi.DisplayID, &bi.Icon}
		if err := rows.Scan(append(base, detailDests(&bi.Detail, &st, &sv, &sp, &tr)...)...); err != nil {
			return err
		}
		foldDetail(&bi.Detail, st, sv, sp, tr)
		s.resolveSpellNames(&bi.Detail)
		profile.Buyback = append(profile.Buyback, bi)
	}
	return rows.Err()
}

// loadLiveStats fills stats from live characters.health/power plus base
// attributes from player_levelstats. Both snapshot tables stay empty while
// a bot is online (core writes them only on logout), so without this every
// online bot renders 0/0%. characters.health can be stale (1 HP corpse row),
// so prefer player_classlevelstats.basehp and only fall back to the live row.
func MathRound(v float64) float64 {
	if v < 0 {
		return float64(int(v - 0.5))
	}
	return float64(int(v + 0.5))
}

// loadLiveStats fills stats from live characters.health/power, base attributes,
// and equipped gear stat bonuses (Str, Agi, Sta, Int, Spi, Armor, Resists).
func (s *Service) loadLiveStats(guid uint32, st *CharacterStats) {
	var race, class, level uint32
	var liveHP, p1, p2, p3, p4, p5 uint32
	if err := s.db.QueryRow(`SELECT race, class, level, health, power1, power2, power3, power4, power5
		FROM characters WHERE guid = ?`, guid).Scan(
		&race, &class, &level, &liveHP, &p1, &p2, &p3, &p4, &p5); err != nil {
		st.Source = "none"
		return
	}
	st.MaxPower1, st.MaxPower2, st.MaxPower3, st.MaxPower4, st.MaxPower5 = p1, p2, p3, p4, p5
	_ = s.db.QueryRow(fmt.Sprintf(`SELECT str, agi, sta, inte, spi
		FROM %s.player_levelstats WHERE race = ? AND class = ? AND level = ?`,
		s.cfg.WorldDB), race, class, level).Scan(
		&st.Strength, &st.Agility, &st.Stamina, &st.Intellect, &st.Spirit)
	_ = s.db.QueryRow(fmt.Sprintf(`SELECT basehp, basemana FROM %s.player_classlevelstats
		WHERE class = ? AND level = ?`, s.cfg.WorldDB), class, level).Scan(&st.MaxHealth, &st.MaxPower1)
	if st.MaxHealth == 0 {
		st.MaxHealth = liveHP
	}

	var totalArmor, resHoly, resFire, resNature, resFrost, resShadow, resArcane uint32
	var bonusStr, bonusAgi, bonusSta, bonusInt, bonusSpi uint32
	var mainDmgMin, mainDmgMax, mainDelay float64
	var rangedDmgMin, rangedDmgMax, rangedDelay float64
	var hasShield bool

	q := fmt.Sprintf(`SELECT ci.slot, it.armor, it.holy_res, it.fire_res, it.nature_res, it.frost_res, it.shadow_res, it.arcane_res,
		it.stat_type1, it.stat_value1, it.stat_type2, it.stat_value2,
		it.stat_type3, it.stat_value3, it.stat_type4, it.stat_value4,
		it.stat_type5, it.stat_value5, it.stat_type6, it.stat_value6,
		it.stat_type7, it.stat_value7, it.stat_type8, it.stat_value8,
		it.stat_type9, it.stat_value9, it.stat_type10, it.stat_value10,
		it.delay, it.dmg_min1, it.dmg_max1, it.subclass, it.inventory_type
		FROM character_inventory ci
		JOIN %s.item_template it ON it.entry = ci.item_template
		WHERE ci.guid = ? AND ci.bag = 0 AND ci.slot >= 0 AND ci.slot < %d`,
		s.cfg.WorldDB, EquipmentSlotEnd)

	rows, err := s.db.Query(q, guid)
	if err == nil {
		defer rows.Close()
		for rows.Next() {
			var slot uint32
			var armor, rHoly, rFire, rNat, rFrst, rShad, rArc uint32
			var st1, st2, st3, st4, st5, st6, st7, st8, st9, st10 int32
			var sv1, sv2, sv3, sv4, sv5, sv6, sv7, sv8, sv9, sv10 int32
			var delay, dmgMin, dmgMax uint32
			var subClass, invType uint32

			if err := rows.Scan(&slot, &armor, &rHoly, &rFire, &rNat, &rFrst, &rShad, &rArc,
				&st1, &sv1, &st2, &sv2, &st3, &sv3, &st4, &sv4, &st5, &sv5,
				&st6, &sv6, &st7, &sv7, &st8, &sv8, &st9, &sv9, &st10, &sv10,
				&delay, &dmgMin, &dmgMax, &subClass, &invType); err == nil {

				totalArmor += armor
				resHoly += rHoly; resFire += rFire; resNature += rNat; resFrost += rFrst; resShadow += rShad; resArcane += rArc
				if slot == 14 && (subClass == 6 || invType == 14) {
					hasShield = true
				}
				if slot == 15 {
					mainDmgMin = float64(dmgMin)
					mainDmgMax = float64(dmgMax)
					mainDelay = float64(delay)
				} else if slot == 17 {
					rangedDmgMin = float64(dmgMin)
					rangedDmgMax = float64(dmgMax)
					rangedDelay = float64(delay)
				}

				stats := [10][2]int32{{st1, sv1}, {st2, sv2}, {st3, sv3}, {st4, sv4}, {st5, sv5}, {st6, sv6}, {st7, sv7}, {st8, sv8}, {st9, sv9}, {st10, sv10}}
				for _, pair := range stats {
					if pair[1] <= 0 {
						continue
					}
					switch pair[0] {
					case 3: bonusAgi += uint32(pair[1])
					case 4: bonusStr += uint32(pair[1])
					case 5: bonusInt += uint32(pair[1])
					case 6: bonusSpi += uint32(pair[1])
					case 7: bonusSta += uint32(pair[1])
					}
				}
			}
		}
	}

	st.Strength += float64(bonusStr)
	st.Agility += float64(bonusAgi)
	st.Stamina += float64(bonusSta)
	st.Intellect += float64(bonusInt)
	st.Spirit += float64(bonusSpi)
	st.Armor = totalArmor + uint32(st.Agility*2.0)
	st.ResHoly = resHoly; st.ResFire = resFire; st.ResNature = resNature; st.ResFrost = resFrost; st.ResShadow = resShadow; st.ResArcane = resArcane

	var ap, rap float64
	lvl := float64(level)
	str := st.Strength
	agi := st.Agility

	switch class {
	case 1, 2:
		if lvl*3.0+str*2.0 > 20.0 { ap = lvl*3.0 + str*2.0 - 20.0 }
	case 3:
		if lvl*2.0+agi*2.0 > 20.0 { ap = lvl*2.0 + agi*2.0 - 20.0 }
		if lvl*2.0+agi*2.0 > 10.0 { rap = lvl*2.0 + agi*2.0 - 10.0 }
	case 4:
		if lvl*2.0+str+agi*2.0 > 20.0 { ap = lvl*2.0 + str + agi*2.0 - 20.0 }
		if lvl*2.0+agi*2.0 > 20.0 { rap = lvl*2.0 + agi*2.0 - 20.0 }
	case 7:
		if lvl*2.0+str*2.0 > 20.0 { ap = lvl*2.0 + str*2.0 - 20.0 }
	default:
		if str*2.0 > 20.0 { ap = str*2.0 - 20.0 }
	}

	st.AttackPower = ap
	st.RangedAttackPower = rap

	critBase := 5.0 + agi/20.0
	dodgeBase := 3.0 + agi/20.0
	st.MeleeCritPct = critBase
	st.RangedCritPct = critBase
	st.DodgePct = dodgeBase
	if class == 1 || class == 2 || class == 4 || class == 7 {
		st.ParryPct = 5.0
	}
	if hasShield {
		st.BlockPct = 5.0
	}

	if mainDmgMax > 0 {
		apBonus := ap / 14.0 * (mainDelay / 1000.0)
		dMin := MathRound(mainDmgMin + apBonus)
		dMax := MathRound(mainDmgMax + apBonus)
		st.MeleeDamage = fmt.Sprintf("%.0f – %.0f", dMin, dMax)
		st.MeleeSpeed = mainDelay / 1000.0
	}
	if rangedDmgMax > 0 {
		rapBonus := rap / 14.0 * (rangedDelay / 1000.0)
		dMin := MathRound(rangedDmgMin + rapBonus)
		dMax := MathRound(rangedDmgMax + rapBonus)
		st.RangedDamage = fmt.Sprintf("%.0f – %.0f", dMin, dMax)
		st.RangedSpeed = rangedDelay / 1000.0
	}

	st.Source = "live"
}

// loadTalents resolves allocated talent ranks from known spells. DBC files
// from the operator (same dir mangosd reads) win when configured; the world
// talent/talenttab mirrors are the fallback. A talent node matches when any
// of its rank spells appears in character_spell; the rank is the highest
// matching position (Player::LearnTalent unlearns other ranks).
func (s *Service) loadTalents(guid uint32) ([]TalentTree, error) {
	known := map[uint32]bool{}
	spRows, err := s.db.Query("SELECT spell FROM character_spell WHERE guid = ?", guid)
	if err != nil {
		return nil, err
	}
	for spRows.Next() {
		var spell uint32
		if err := spRows.Scan(&spell); err != nil {
			spRows.Close()
			return nil, err
		}
		known[spell] = true
	}
	if err := spRows.Err(); err != nil {
		return nil, err
	}

	var classID uint32
	if err := s.db.QueryRow("SELECT class FROM characters WHERE guid = ?", guid).Scan(&classID); err != nil {
		return nil, err
	}
	if s.cfg.DBCDir != "" {
		if trees, err := s.talentsFromDBC(guid, classID, known); err == nil {
			return trees, nil
		}
		// Fall through to SQL mirrors on any DBC read error.
	}
	talentQuery := fmt.Sprintf(`
		SELECT t.id, t.talentTabId, t.tierId, t.columnIndex,
		       t.spellRank1, t.spellRank2, t.spellRank3, t.spellRank4, t.spellRank5,
		       tt.Name1, tt.orderIndex,
		       COALESCE(st.name, ''), COALESCE(si.Name, ''), COALESCE(st.spellIconId, 0)
		FROM %s.talent t
		JOIN %s.talenttab tt ON tt.id = t.talentTabId
		LEFT JOIN %s.spell_template st ON st.entry = t.spellRank1
		LEFT JOIN %s.spellicon si ON si.ID = st.spellIconId
		WHERE (tt.classMask & (1 << (? - 1))) != 0
		ORDER BY tt.orderIndex, t.tierId, t.columnIndex, t.id
	`, s.cfg.WorldDB, s.cfg.WorldDB, s.cfg.WorldDB, s.cfg.WorldDB)

	rows, err := s.db.Query(talentQuery, classID)
	if err != nil {
		// Mirror tables missing or empty: not an error, just no talent data.
		if isMissingTable(err) {
			return []TalentTree{}, nil
		}
		return nil, err
	}
	defer rows.Close()

	trees := []TalentTree{}
	byTab := map[uint32]int{}
	for rows.Next() {
		var (
			talentID, tabID, row, col uint32
			r1, r2, r3, r4, r5        uint32
			tabName                   sql.NullString
			page                      uint32
			spellName, spellIcon      sql.NullString
			iconID                    uint32
		)
		if err := rows.Scan(&talentID, &tabID, &row, &col,
			&r1, &r2, &r3, &r4, &r5, &tabName, &page, &spellName, &spellIcon, &iconID); err != nil {
			return nil, err
		}
		ranks := []uint32{r1, r2, r3, r4, r5}
		maxRank := uint32(0)
		activeSpell := uint32(0)
		activeRank := uint32(0)
		activeName, activeIcon := "", ""
		for i, rs := range ranks {
			if rs == 0 {
				continue
			}
			maxRank = uint32(i + 1)
			if known[rs] {
				activeRank = uint32(i + 1)
				activeSpell = rs
				if spellName.Valid {
					activeName = spellName.String
				}
				if spellIcon.Valid {
					activeIcon = spellIcon.String
				}
			}
		}
		if activeName == "" && spellName.Valid {
			activeName = spellName.String
		}
		if activeIcon == "" && spellIcon.Valid {
			activeIcon = spellIcon.String
		}
		if activeIcon == "" && iconID != 0 {
			activeIcon = s.spellIconByID(iconID)
		}
		if maxRank == 0 {
			continue
		}
		idx, ok := byTab[tabID]
		if !ok {
			name := fmt.Sprintf("Tree %d", tabID)
			if tabName.Valid && tabName.String != "" {
				name = tabName.String
			}
			trees = append(trees, TalentTree{TabID: tabID, Name: name, Page: page, Talents: []TalentNode{}})
			idx = len(trees) - 1
			byTab[tabID] = idx
		}
		trees[idx].Talents = append(trees[idx].Talents, TalentNode{
			TalentID: talentID, Row: row, Col: col,
			Rank: activeRank, MaxRank: maxRank, SpellID: activeSpell, Name: activeName, Icon: activeIcon,
		})
		if activeRank > 0 {
			trees[idx].Points += activeRank
		}
	}
	if err := rows.Err(); err != nil {
		if isMissingTable(err) {
			return []TalentTree{}, nil
		}
		return nil, err
	}
	if trees == nil {
		trees = []TalentTree{}
	}
	return trees, nil
}

func isMissingTable(err error) bool {
	if err == nil {
		return false
	}
	msg := err.Error()
	return strings.Contains(msg, "doesn't exist") || strings.Contains(msg, "does not exist") ||
		strings.Contains(msg, "Table") && strings.Contains(msg, "unknown")
}
