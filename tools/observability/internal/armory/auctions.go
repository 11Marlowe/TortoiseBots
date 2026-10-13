package armory

import (
	"fmt"
	"sort"
	"strings"
)

// AuctionPageSize is the number of rows one /api/v1/auctions page returns.
const AuctionPageSize = 100

// AuctionFilter narrows the read-only auction house view. Zero values mean
// "no constraint"; Sort is one of name, quality, level, seller, bid, buyout,
// expire (default).
type AuctionFilter struct {
	Query    string // item name fragment
	Quality  int    // minimum quality, -1 = any
	MinLevel uint32 // required level range
	MaxLevel uint32
	Seller   string // seller name fragment
	Kind     string // "", "bot", "player"
	House    string // "", "alliance", "horde", "neutral"
	Sort     string
	Desc     bool
	Page     uint32 // 0-based
}

type AuctionRow struct {
	ID            uint32 `json:"id"`
	House         string `json:"house"`
	ItemTemplate  uint32 `json:"item_template"`
	Name          string `json:"name"`
	Quality       uint8  `json:"quality"`
	RequiredLevel uint32 `json:"required_level"`
	ItemLevel     uint32 `json:"item_level"`
	Icon          string `json:"icon"`
	Count         uint32 `json:"count"`
	SellerGUID    uint32 `json:"seller_guid"`
	SellerName    string `json:"seller_name"`
	SellerClass   uint8  `json:"seller_class"`
	SellerKind    string `json:"seller_kind"` // bot, player, market (module's ownerless market listing)
	StartBid      uint32 `json:"start_bid"`
	Bid           uint32 `json:"bid"`
	Buyout        uint32 `json:"buyout"`
	ExpireAt      int64  `json:"expire_at"`

	itemGUID  uint32
	displayID uint32
}

type AuctionSummary struct {
	Total       uint32            `json:"total"`
	WithBid     uint32            `json:"with_bid"`
	BuyoutValue uint64            `json:"buyout_value"`
	ByQuality   map[uint8]uint32  `json:"by_quality"`
	ByHouse     map[string]uint32 `json:"by_house"`
}

type AuctionPage struct {
	Rows     []AuctionRow   `json:"rows"`
	Matched  uint32         `json:"matched"`
	Page     uint32         `json:"page"`
	PageSize uint32         `json:"page_size"`
	Summary  AuctionSummary `json:"summary"`
}

// auctionHouse follows AuctionHouseMgr::GetAuctionHouseTeam: 1-3 Alliance,
// 4-6 Horde, anything else neutral.
func auctionHouse(id uint32) string {
	switch {
	case id >= 1 && id <= 3:
		return "alliance"
	case id >= 4 && id <= 6:
		return "horde"
	}
	return "neutral"
}

// ListAuctions returns one page of the auction house plus a whole-market
// summary. Read-only, and kept to a few short statements: auction, characters
// and item_display_info are MyISAM, so a long read holds table locks that
// stall the core's character saves. Joining characters per auction (~150 ms
// for 7k rows) or item_display_info (no index on ID: seconds) is avoided;
// filtering, sorting and paging run here over the light rows instead.
func (s *Service) ListAuctions(f AuctionFilter) (*AuctionPage, error) {
	all, err := s.loadAuctions()
	if err != nil {
		return nil, err
	}

	page := &AuctionPage{Rows: []AuctionRow{}, Page: f.Page, PageSize: AuctionPageSize,
		Summary: AuctionSummary{ByQuality: map[uint8]uint32{}, ByHouse: map[string]uint32{}}}
	query := strings.ToLower(f.Query)
	seller := strings.ToLower(f.Seller)
	var matched []AuctionRow
	for _, r := range all {
		sum := &page.Summary
		sum.Total++
		if r.Bid > 0 {
			sum.WithBid++
		}
		sum.BuyoutValue += uint64(r.Buyout)
		sum.ByQuality[r.Quality]++
		sum.ByHouse[r.House]++

		if query != "" && !strings.Contains(strings.ToLower(r.Name), query) ||
			f.Quality >= 0 && int(r.Quality) < f.Quality ||
			f.MinLevel > 0 && r.RequiredLevel < f.MinLevel ||
			f.MaxLevel > 0 && r.RequiredLevel > f.MaxLevel ||
			seller != "" && !strings.Contains(strings.ToLower(r.SellerName), seller) ||
			f.Kind != "" && r.SellerKind != f.Kind ||
			f.House != "" && r.House != f.House {
			continue
		}
		matched = append(matched, r)
	}
	page.Matched = uint32(len(matched))

	key := func(r *AuctionRow) interface{} {
		switch f.Sort {
		case "name":
			return strings.ToLower(r.Name)
		case "seller":
			return strings.ToLower(r.SellerName)
		case "quality":
			return int64(r.Quality)
		case "level":
			return int64(r.RequiredLevel)
		case "bid":
			return int64(max(r.Bid, r.StartBid))
		case "buyout":
			return int64(r.Buyout)
		}
		return r.ExpireAt
	}
	sort.SliceStable(matched, func(i, j int) bool {
		a, b := key(&matched[i]), key(&matched[j])
		if a == b {
			return matched[i].ID < matched[j].ID
		}
		var less bool
		if as, ok := a.(string); ok {
			less = as < b.(string)
		} else {
			less = a.(int64) < b.(int64)
		}
		return less != f.Desc
	})

	start := int(f.Page) * AuctionPageSize
	if start < len(matched) {
		page.Rows = matched[start:min(start+AuctionPageSize, len(matched))]
	}
	if err := s.fillPageDetails(page.Rows); err != nil {
		return nil, err
	}
	return page, nil
}

// loadAuctions reads every auction with its item and seller in two short
// statements (~5 ms and ~20 ms on a 7k-auction, 1k-character realm).
func (s *Service) loadAuctions() ([]AuctionRow, error) {
	rows, err := s.db.Query(fmt.Sprintf(`
		SELECT a.id, a.houseid, a.itemguid, a.item_template, a.itemowner, a.startbid, a.lastbid, a.buyoutprice, a.time,
		       it.name, it.quality, it.required_level, it.item_level, it.display_id
		FROM auction a
		JOIN %s.item_template it ON it.entry = a.item_template
	`, s.cfg.WorldDB))
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	all := []AuctionRow{}
	for rows.Next() {
		var r AuctionRow
		var houseID uint32
		if err := rows.Scan(&r.ID, &houseID, &r.itemGUID, &r.ItemTemplate, &r.SellerGUID, &r.StartBid, &r.Bid, &r.Buyout, &r.ExpireAt,
			&r.Name, &r.Quality, &r.RequiredLevel, &r.ItemLevel, &r.displayID); err != nil {
			return nil, err
		}
		r.House = auctionHouse(houseID)
		r.Count = 1
		all = append(all, r)
	}
	if err := rows.Err(); err != nil {
		return nil, err
	}

	// A seller is a bot when its account is in the module's pool registry
	// (same identity rule as ListBots); owner 0 is the module's ownerless
	// market listing.
	type sellerInfo struct {
		name  string
		class uint8
		bot   bool
	}
	sellers := map[uint32]sellerInfo{}
	srows, err := s.db.Query(`
		SELECT c.guid, c.name, c.class, p.account_id IS NOT NULL
		FROM characters c
		LEFT JOIN tortoise_bots_pool_account p ON p.account_id = c.account
		WHERE c.guid IN (SELECT DISTINCT itemowner FROM auction)
	`)
	if err != nil {
		return nil, err
	}
	defer srows.Close()
	for srows.Next() {
		var guid uint32
		var si sellerInfo
		if err := srows.Scan(&guid, &si.name, &si.class, &si.bot); err != nil {
			return nil, err
		}
		sellers[guid] = si
	}
	if err := srows.Err(); err != nil {
		return nil, err
	}

	for i := range all {
		r := &all[i]
		si, ok := sellers[r.SellerGUID]
		switch {
		case r.SellerGUID == 0:
			r.SellerKind = "market"
		case ok && si.bot:
			r.SellerKind = "bot"
		default:
			r.SellerKind = "player"
		}
		r.SellerName, r.SellerClass = si.name, si.class
	}
	return all, nil
}

// fillPageDetails adds stack counts and icons for one page only.
// item_display_info has no index on ID, so it is never joined to the whole
// auction table.
func (s *Service) fillPageDetails(page []AuctionRow) error {
	if len(page) == 0 {
		return nil
	}
	placeholders := "?" + strings.Repeat(",?", len(page)-1)
	itemArgs := make([]interface{}, len(page))
	displayArgs := make([]interface{}, len(page))
	for i, r := range page {
		itemArgs[i] = r.itemGUID
		displayArgs[i] = r.displayID
	}

	counts := map[uint32]uint32{}
	rows, err := s.db.Query("SELECT guid, `count` FROM item_instance WHERE guid IN ("+placeholders+")", itemArgs...)
	if err != nil {
		return err
	}
	for rows.Next() {
		var guid, count uint32
		if err := rows.Scan(&guid, &count); err != nil {
			rows.Close()
			return err
		}
		counts[guid] = count
	}
	rows.Close()
	if err := rows.Err(); err != nil {
		return err
	}

	icons := map[uint32]string{}
	rows, err = s.db.Query(fmt.Sprintf("SELECT ID, icon FROM %s.item_display_info WHERE ID IN (%s)", s.cfg.WorldDB, placeholders), displayArgs...)
	if err != nil {
		return err
	}
	defer rows.Close()
	for rows.Next() {
		var id uint32
		var icon string
		if err := rows.Scan(&id, &icon); err != nil {
			return err
		}
		icons[id] = icon
	}
	if err := rows.Err(); err != nil {
		return err
	}

	for i := range page {
		if c := counts[page[i].itemGUID]; c > 0 {
			page[i].Count = c
		}
		page[i].Icon = icons[page[i].displayID]
	}
	return nil
}
