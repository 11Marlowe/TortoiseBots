package web

import "embed"

// FS embeds the dashboard UI and data. Client zone art is generated locally
// (see scripts/gen_zone_maps.py and --maps-dir); only the Maps of Mystery art
// is shipped.
//
//go:embed static data index.html login.html
//go:embed maps/alahthalas.webp maps/balor.webp maps/blackstoneisland.webp maps/gillijim.webp maps/gilneas.webp
//go:embed maps/gmisland.webp maps/grimreaches.webp maps/hyjal.webp maps/icepoint.webp maps/lapidis.webp
//go:embed maps/northwind.webp maps/stonetalonmountains.webp maps/telabim.webp maps/thalassianhighlands.webp
//go:embed maps/upperkarazhan2f.webp
var FS embed.FS
