package web

import "embed"

// FS embeds the dashboard UI and data. Zone art is not shipped: it is
// generated locally from the client (scripts/gen_zone_maps.py, --maps-dir).
//
//go:embed static data index.html login.html
var FS embed.FS
