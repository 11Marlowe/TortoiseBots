#pragma once

namespace TortoiseBots {

// Built-in name pools for runtime/BotNames.h (issue #1003), one section per
// raw string: "[key]" then whitespace-separated words. Keys are described in
// BotNames.h; AiPlayerbot.RandomBotNamesFile adds more words in the same format.
// rp:* samples only train the letter chain: bots never get a sample verbatim.
// The general and race fun names and most hunter pet names were written for
// this feature by @Lunarpac (issue #1003) and contributed for free use.
inline char const* const kBotNameData[] = {
    R"([rp:human:male]
Aldric Bram Cedric Doran Edwin Garret Hadrian Ivor Jorin Kendrick Lucan Marcus Merrick Osric Perrin
Roderic Rowan Tristan Varian Wendel Alaric Benedict Corwin Darian Emrys Gareth Leoric Matthias
Tobias Willem
)",
    R"([rp:human:female]
Adela Brienne Celeste Daria Elara Fiona Gwendolyn Helena Isolde Jessamyn Katrina Lenora Maribel
Natalia Odette Rosalind Seraphina Talia Viviane Wilhelmina Annelise Beatrix Cordelia Elowen
Evangeline Imogen Liliana Marianne Rosamund Sabine
)",
    R"([rp:orc:male]
Grommak Durgash Karg Thrakk Morgash Zugrok Gorrok Brakka Draknar Garosh Kragg Lothrak Mogrash
Nazgrom Orgrim Rakthar Skorn Tharg Urgash Vorrak Zharok Dagmok Grukk Hurgash Krosh Margok Rogash
Thokk Ulgar Zorgath
)",
    R"([rp:orc:female]
Durza Garona Kargha Mogra Shazra Thura Zagra Agra Brokka Draka Gorza Hagra Krasha Lurza Morka Naza
Rashka Shara Tazra Urka Vazra Zurka Grisha Ozgra Ragna Sharga Tuzka Urza Yazra Zuga
)",
    R"([rp:dwarf:male]
Thorgrin Brannoc Durgan Magnus Torvald Bromdor Grimbald Hargrim Korvin Morgrim Ragnor Thorbin Ulfgar
Balgrim Dolgrim Fargus Gundren Haldor Kildrum Mordrin Norgrim Orsik Rurik Thoradin Bruenor Dagnar
Grundar Harbek Lodrin Torgar
)",
    R"([rp:dwarf:female]
Hilda Brunhild Dagna Esvele Gerda Helga Ingra Kathra Moira Ragna Sigrun Thora Vistra Audra Bera
Dorna Eldeth Gunnloda Hlin Kristryd Liftrasa Mardred Riswynn Sannl Torbera Ulla Veit Yrsa Brenna
Ilde
)",
    R"([rp:nightelf:male]
Kaelthor Sylvaris Aerendil Faelan Ilthor Lorethan Maelor Nathrel Rhalden Shandor Taelor Varethil
Aldoran Caelrin Elandor Fyrathil Iandril Kavorin Lunathor Melidan Nyranor Orathil Shalorin Thalendor
Valandris Arvanor Daelis Eredas Halvaris Ithilran
)",
    R"([rp:nightelf:female]
Aelindra Sylvanna Elunara Faelyn Ilyria Lunara Maevra Naelia Shalandra Taelia Velanna Ysalyn Alyssia
Celenwe Dalyria Elanthe Ilarae Kaelira Lyssandra Myrielle Nyssara Selanthe Shaiara Thalyssa Vaelith
Ashalah Elisande Feyrith Irelia Sorayah
)",
    R"([rp:undead:male]
Morbius Graven Mortimer Corvin Malachai Silas Vladimir Ezekiel Lucius Mordecai Cassius Draven
Ignatius Lazarus Nicodemus Osgood Severin Thaddeus Valerian Barnabus Cornelius Desmond Edgar Horace
Jareth Leopold Montague Percival Regis Victor
)",
    R"([rp:undead:female]
Velisande Morgana Ravenna Lenore Isabeau Ophelia Mortitia Sybil Vespera Drusilla Evangeline Lucinda
Marisol Nyx Rosalie Seraphine Theodora Valeria Annabel Belladonna Carmilla Delphine Ermengarde
Griselda Hester Lilith Malvina Prudence Temperance Wilhelmina
)",
    R"([rp:tauren:male]
Rokhan Tahoma Hakan Mahko Bruhar Torok Garrah Huln Kador Lorok Maahk Nahar Oruk Taruk Uthan Hamuul
Kurahn Tohahn Bahota Ganoka Harutt Kanaho Mahota Nahotan Ruul Tagar Takoda Unahu Wahota Yohtan
)",
    R"([rp:tauren:female]
Ahanu Aponi Ayasha Halona Kaya Magena Nayeli Shima Taima Winona Adahy Chenoa Dyani Elsu Hiamovi Isi
Kachina Leotie Mausi Nita Onida Sakari Tala Ulka Wenona Yazhi Aiyana Kimi Mika Taini
)",
    R"([rp:gnome:male]
Fizzlecog Tinkerbolt Gimble Nobbin Pozzik Wizzle Bink Dimble Fenwick Gizmo Jinkle Kipper Mekkle Nim
Orrin Pip Quibble Rizzle Sprocket Tobble Widget Bixby Cogwin Dinkum Filbert Gearwin Jabber Nackle
Tink Zook
)",
    R"([rp:gnome:female]
Tinkla Fizzia Gimmie Nissa Pixie Bimble Cogsy Dinah Ellyn Fennie Gizzie Jinny Kippa Lissa Mimsy
Nibba Pippa Quilla Rinna Sprinkle Tibby Wenna Zizzie Bella Clockie Dottie Fiddla Hazzie Minna Tilly
)",
    R"([rp:troll:male]
Zanjin Rokhan Vazjin Jintha Kozjin Malakai Rastakan Tazmu Unjah Volzin Zalazane Bwonsamu Gadrin
Hexxar Jaktar Krazjin Mazzik Ozumat Rajin Senjin Tajak Vanjin Zekhan Akilzon Hakkar Jalazar Kizrak
Nekali Shazzrah Zulmar
)",
    R"([rp:troll:female]
Zanjira Vanira Jaina Malzara Shazara Talanji Yenniku Zalai Jazira Kazra Lunza Mazira Nazira Rakira
Sazzi Tazira Vajira Zelira Akira Jinzara Kalita Mira Nala Ranzi Sarji Telanji Volira Yazza Zanza
Zuli
)",
    R"([rp:goblin:male]
Gazlowe Fizzgig Grizzik Kezzik Nixx Razdunk Sprazzle Wrenix Bix Dizzle Frizz Gikkix Krazzle Mozzle
Noggle Pazzik Rizzik Snivvle Tazzik Vixx Zazzo Blixx Drizzik Fazzle Grixx Kazzle Muggs Rexxar
Skizzik Zixx
)",
    R"([rp:goblin:female]
Kezzie Fizzlya Grizzla Nixxa Razzie Sprizza Wrenna Bixxi Dizzla Frizzie Gikka Krizzle Mozzie Nogga
Pazza Rizza Snivvla Tazzie Vixxie Zazzie Blixa Drizza Fazzla Grixxie Kazzie Mugga Rexxa Skizza Zixxi
Lizzie
)",
    R"([rp:highelf:male]
Aerith Belathor Caelan Dorian Elendil Faeron Galathil Halduron Ilmaron Kaelen Lorenthas Maeron
Nathaniel Quelaron Rommath Saelorn Tirathon Valandil Aelrin Belimar Cyrandil Daelor Erevan Faelar
Ithaniel Lirandor Myrandil Orelius Sylvaren Theron
)",
    R"([rp:highelf:female]
Aelira Belinda Caelestra Delenna Elarael Faelith Galadra Halia Ilyana Kaelith Liadrin Maelis Naelia
Quelara Saelinda Tirana Valeera Aelith Belara Cyrilla Daelia Eloriel Feyra Ilaria Lirael Mirelle
Nalia Orlaith Sylvara Thalia
)",
    R"([start]
Dark Storm Iron Shadow Blood Frost Fire Grim Stone Thunder Wild Night Dread Steel Silver Gold Swift
Bright Dawn Dusk Wolf Raven Bear Hawk Red Black White Ash Ember Rune
)",
    R"([start:warrior]
Battle War Rage Axe Shield Skull Bone Gore Blade Hammer
)",
    R"([start:paladin]
Light Holy Dawn Just Valor Sun Oath Faith Grace Truth
)",
    R"([start:hunter]
Swift Hawk Eagle Wolf Arrow Wild Beast Track Bow Fang
)",
    R"([start:rogue]
Shadow Silent Quick Night Dagger Venom Sly Ghost Smoke Whisper
)",
    R"([start:priest]
Holy Light Soul Mercy Spirit Prayer Mind Grace Hope Star
)",
    R"([start:shaman]
Storm Earth Thunder Spirit Totem Tide Wind Lava Stone Sky
)",
    R"([start:mage]
Arcane Frost Fire Star Spell Rune Mana Flame Mystic Rime
)",
    R"([start:warlock]
Shadow Dread Doom Fel Soul Void Hex Gloom Grave Curse
)",
    R"([start:druid]
Moon Leaf Thorn Oak Wild Star Bark Fern Grove Root
)",
    R"([end]
blade fury bane fist heart song shield storm walker born brand caller fang forge guard hammer mane
shade strike thorn wind ward bringer runner stalker seeker breaker crest fall vale
)",
    R"([fun:warrior]
Bladestorm Shieldslam Tauntaun Ragequit Mortalstrik Chargeup Bigswordguy Execute Thunderclapz
Hamstringer Sunderwear Taunty Choppychop Whirlwindy Bonkbonk
)",
    R"([fun:paladin]
Bubblehearth Holymoly Lightbringr Retadin Shockadin Divineshield Paladean Blessyou Judgemental
Healadin Bubbleboy Consecrate Paladont Lightyear Sealteam
)",
    R"([fun:hunter]
Pewpew Aimbot Petlover Arrowdynamic Tranqshot Bowjangles Legolash Feigndeath Steadyaim Autoshot
Bestpetever Kitesurfer Trapper Shootyboi
)",
    R"([fun:rogue]
Kidneyshotz Stabbyjoe Backstabath Sneakypete Vanishingact Pickpocketz Sapsap Gougeface Stabbath
Stealthy Poisonivy Cheapshotz Shankyou Dodgy Ambushed
)",
    R"([fun:priest]
Healznot Holycrit Prayforme Renewable Shieldme Mindflayed Healbot Shadowpriest Bubbles Dispelme
Faithful Smiteme Fearward Lightwell Pwshield
)",
    R"([fun:shaman]
Totemtastic Shamwow Windfury Chainheal Shockme Totembro Earthshock Stormstrike Reincarnate Lavalamp
Totally Shammy Bloodlusty Groundhog Purgeme
)",
    R"([fun:mage]
Frostbolt Pyroblasted Iceblocked Polymorphed Magetower Blinkblink Waterboy Sheepish Arcanist
Freezeframe Manaburn Conjurer Toastymage Brrrr Fireballz
)",
    R"([fun:warlock]
Dotsnspots Lifetapper Shardhoarder Felsworth Doomsayer Soulstoned Curseyou Healthstone Imperious
Hellfire Fearless Drainyou Felbro Summonme Corruptor
)",
    R"([fun:druid]
Moonkinz Treeform Bearlytank Catzilla Shapeshifty Barkskin Rejuvenate Hottie Treant Wildgrowth
Furball Bearhug Moonfire Leafit Starsurge
)",
    R"([pet:hunter]
Mrbitey Mrwhiskers Chomps Furbert Biscuit Snuggles Waffles Pickles Meatball Sir Fuzzyface Nugget
Fang Shadowclaw Grizzle Ember Ripper Thunderhoof Nightwhisper Bramble Snarl Mittens Gnasher Ash
Frostbite Rumble Whiskers Talon Bonecrusher Pebble Razor Sable Brutus Clover Duskpaw Fluffy
Grimtooth Hazel Ironhide Juniper Kodo Lurk Muddy Nibbles Onyx Pounce Quill Rascal Scruffy Thistle
Umber Vex Wraith Yowl Zephyr Bandit Cinder Dagger Fuzzball Gale Howler Mossback Nettle Patches
Ruckus Smokey Sprout Stormy Tufts Wiggles Barkley Chomper Digger Echo Flint Gravel Hunter Jinx
Knuckles Loki Maul Nomad Oakley Pepper Rex Shade Tank Ursa Vigil Wolfie Bristle Scout Tiny Boulder
Claws Dusty Fluff Gobbles Hops Ivy Lumpy Midnight Noodle Pudding Rusty Sly Stomper Toothy Whisper
Shadowfang Bloodpaw Grimmaw Stonehide Chompers Fuzzface Whiskerton Pawsley Barnaby Rufus Duke Baron
Countess Duchess Princess Tiberius Maximus Caesar Nero Cleo Zeus Apollo Thor Odin Freya Athena Hera
Ares Atlas Titan Kraken Hydra Chimera Griffin Sphinx Basilisk Wyvern Drake Buttons Socks Boots
Slippers Cupcake Cookie Brownie Fudge Truffles Caramel Toffee Butterscotch Marshmallow Peanut Cashew
Walnut Pistachio Almond Hazelnut Chestnut Coconut Mango Papaya Kiwi Plum Peaches Cherry Berry
Bramblebush Blueberry Raspberry Gooseberry Elderberry Shadow Smoke Soot Coal Charcoal Blaze Scorch
Flare Flicker Glimmer Gleam Shimmer Sparkle Twinkle Glow Lumen Beacon Torch Ragnar Bjorn Sven Gunnar
Harald Ivar Ulf Rolf Leif Erik Thorvald Fangs Tusk Snout Muzzle Whisker Paws Stripes Spots Dots
Speckles Freckles Patch Blotch Mottle Brindle Tabby Calico Tawny Sandy Sooty Inky Jet Grizzly Kodiak
Bruin Ursus Teddy Bubba Bear Cub Pup Kitten Kitty Pupper Doggo Floof Fluffball Furball Hairball
Puffball Snowball Cottonball Dustbunny Hopscotch Butterbean Jellybean Hotdog Burrito Taco Tater Spud
Wonton Pierogi Howl Growl Grunt Roar Huff Puff Chuff Snuffle Sniff Wag Bounce Scamper Skitter Scurry
Prowl Stalk Leap Dash Zoom Zip Bolt Flash Rocket Blitz Turbo Nitro Dart Arrow Bullet Lady Lord
Captain Sergeant Major General Admiral Professor Doctor Bishop Squire Knight Baroness Marquis
Viscount Earl Regent Chancellor Pharaoh Sultan Shogun Khan Tsar Kaiser Rajah Mogul Moose Elk Bison
Yak Gnu Ibex Oryx Kudu Dingo Jackal Coyote Hyena Panther Jaguar Ocelot Lynx Bobcat Puma Cougar
Leopard Cheetah Tiger Lion Wolfgang Wolfram Foxy Vixen Reynard Ringo Rocco Bruno Gus Otis Ollie
Murphy Bailey Cooper Tucker Jasper Rocky Buster Harley Gremlin Goblin Imp Pixie Sprite
)",
    R"([prefix:warrior]
Smash Bonk Rage Tank Chop Bash
)",
    R"([prefix:paladin]
Bubble Holy Pala Bless Judge Bubbs
)",
    R"([prefix:hunter]
Pew Pet Aim Shooty Trap Kite
)",
    R"([prefix:rogue]
Stab Sneak Shank Gouge Stabby Poke
)",
    R"([prefix:priest]
Heal Holy Pray Smite Bubble Renew
)",
    R"([prefix:shaman]
Totem Shock Sham Lava Zap Wind
)",
    R"([prefix:mage]
Frost Blink Sheep Pyro Brr Mana
)",
    R"([prefix:warlock]
Dot Fel Imp Fear Doom Curse
)",
    R"([prefix:druid]
Moon Tree Bear Kitty Boom Leaf
)",
    R"([prefix:gnome]
Gnome Tiny Cog
)",
    R"([prefix:tauren]
Moo Cow Hoof
)",
    R"([prefix:orc]
Zug Grunt Orc
)",
    R"([prefix:undead]
Bone Rot Grave
)",
    R"([prefix:dwarf]
Ale Beard Stout
)",
    R"([prefix:goblin]
Boom Coin Gob
)",
    R"([prefix:troll]
Mon Voodoo Juju
)",
    R"([prefix:nightelf]
Moon Elf Leaf
)",
    R"([prefix:highelf]
Quel Sun Fancy
)",
    R"([prefix:human]
Basic Farm Plain
)",
    R"([suffix]
z y ie ster tastic kinz bot inator ito meister lord face boi
)",
    R"([fun]
Moss Fern Pebble Thistle Bramble Clover Acorn Birch Hazel Sorrel Nettle Tansy Yarrow Sage Juniper
Bracken Lichen Truffle Burdock Sedge Rowan Alder Aspen Cedar Larch Linden Hemlock Primrose Bluebell
Foxglove Lupin Marigold Poppy Daisy Tulip Petunia Snapdragon Biscuit Pudding Waffles Pretzel
Dumpling Noodle Toast Crumpet Muffin Pickle Gumdrop Taffy Nougat Brisket Gravy Pancake Strudel Kebab
Nacho Tofu Wasabi Paprika Nutmeg Cinnamon Ginger Pepper Sprinkles Custard Meringue Sourdough Bagel
Scone Turnip Radish Parsnip Rutabaga Kumquat Quince Lantern Teapot Spoon Kettle Button Thimble
Doorknob Ladle Sprocket Widget Gizmo Doohickey Gadget Thingamajig Whatsit Spatula Trowel Anvil
Bellows Hinge Latch Cogwheel Bobbin Spindle Teacup Saucer Candle Wick Inkwell Quill Parchment Bucket
Barrel Crate Mitten Bootlace Pocket Satchel Ennui Vex Whim Murmur Echo Hush Nimbus Lull Quirk Gambit
Hubris Malaise Reverie Solace Tumult Vertigo Paradox Riddle Enigma Cipher Whimsy Folly Mirth Glee
Grudge Nonsense Hullabaloo Kerfuffle Brouhaha Shenanigan Balderdash Gobbledygook Hogwash Poppycock
Codswallop Flummox Bamboozle Hiccup Yawn Snore Squeak Burble Gurgle Sniffle Wheeze Snort Grumble
Mumble Chortle Giggle Cackle Guffaw Hoot Honk Bleat Purr Chirp Twitter Warble Rattle Clatter Clunk
Thud Thump Bonk Zap Fizz Pop Crackle Sizzle Whoosh Drizzle Squall Gust Sleet Hail Fog Haze Frost
Thaw Monsoon Zephyr Gale Tempest Cyclone Breeze Puddle Rainbow Overcast Sunbeam Moonbeam Twilight
Dusk Dawn Noon Midnight Eclipse Equinox Solstice Comet Meteor Nebula Quasar Pulsar Orbit Nadir Teal
Umber Sepia Ochre Cerise Mauve Taupe Russet Vermilion Indigo Cobalt Azure Saffron Amber Scarlet
Crimson Magenta Chartreuse Lavender Lilac Olive Slate Ivory Ebony Jade Onyx Opal Garnet Topaz Quartz
Flint Obsidian Basalt Granite Cobble Gravel Shale Zed Taz Bo Ziggy Iggy Pip Pog Bix Dax Lux Nyx Rook
Wren Finch Lark Robin Crow Raven Magpie Heron Puffin Penguin Gecko Newt Toad Ferret Badger Otter
Weasel Stoat Hedgehog Platypus Wombat Lemur Sloth Gibbon Walrus Narwhal Krill Plankton Barnacle
Limpet Squid Wobblesworth Fuzzbucket Noodlebrain Sockpuppet Bumblefoot Grumplestilt Muddlehead
Thunderpants Wigglebottom Pudgeworth Snorkel Bubbles Doodle Scribble Smudge Splotch Blot Fumble
Tumble Bungle Jumble Scramble Ramble Bramblewick Puddlejump Moonwalker Sunflower Stargazer Daydream
Nightowl Snowflake Firefly Dragonfly Butterfly Ladybug Tumbleweed Nimble Clumsy Grumpy Sleepy Sneezy
Dopey Bashful Moody Snarky Cheeky Perky Spunky Funky Groovy Nifty Swell Dandy Spiffy Zesty Peppy
Jolly Merry Gloomy Rowdy Shady Shifty Sketchy Sneaky Wonky Quirky Dizzy Fuzzy Frosty Rusty Dusty
Musty Crusty Sprocketeer Ratchet Rivet Piston Gearbox Torque Tinker Fiddle Fidget Doodad Knickknack
Trinket Bauble Gewgaw Doodlebug Whirligig Thingummy Mishmash Hodgepodge Potpourri Smorgasbord
Rigmarole Hootenanny Jamboree Shindig Hoedown Fandango Tarantella Pixel Byte Glitch Static Fizzbang
Kaboom Kapow Zoinks Yikes Egads Gadzooks Crikey Blimey Golly Gosh Shucks Phooey Pshaw Meh Hmm Huzzah
Hurrah Yonder Thither Whence Hither Anon Forsooth Prithee Verily Tinsel Glitter Sequin Ribbon Tassel
Confetti Streamer Bunting Garland Wreath Mistletoe Holly Ivy Sprig Bough Twig Stump Burl Knot Gnarl
Thorn Briar Spur Prickle Burr Nettlecap Toadstool Mushroom Morel Chanterelle
)",
    R"([fun:human]
Justhuman Averagejoe Plainjane Humanbean Plainvanilla Goldshirekid Farmerjoe Basicbob Defiasdan
Elwynnwalker
)",
    R"([fun:orc]
Zugzug Workwork Orcward Orcsome Smashley Orcestra Greenmachine Orcanizer Lokthar Choppy Grumpgut
Smashmouth Orcwardly Ogrelord
)",
    R"([fun:dwarf]
Alecoholic Pintsize Ironbeer Brewtus Hammertime Shortnstout Aleluia Kegstand Brewbeard Drunkenstein
Stoutmeister Rockandstone Beardsley Tankard
)",
    R"([fun:nightelf]
Treehugger Moonsplain Shelfelf Nightelfie Mailboxdance Shadowmelt Leafblower Purplehaze Owlrighty
Moonpie Tranquilizer
)",
    R"([fun:undead]
Grimreefer Notdeadyet Bonejour Rottweiler Skeletony Ribcage Decaydance Necrofancy Lostmyjaw Deadwait
Bonafide Undeadline Cryptkicker Kneebones
)",
    R"([fun:tauren]
Hasslehoof Moolah Cowabunga Hoofhearted Moodini Steakholder Cowlick Beefcake Moozart Cudsworth
Mootilda Bovinity Holycow Milkshake Udderchaos Grassfed Taurentino Moonaroo
)",
    R"([fun:gnome]
Tinyterror Kneecapper Gnometown Gnomad Gnomie Gnomebody Gnometheus Pocketrocket Gnomadic Halfpint
Cogsworth Gnomaste Wrenchwizard Shortcircuit Anklebiter
)",
    R"([fun:troll]
Trollolo Youknowmon Trollbridge Hexmex Stayawaymon Berserkmon Trollhunter Spearitual Mojorising Juju
Tuskany Voodoodle
)",
    R"([fun:goblin]
Moneybags Timeismoney Explodey Boomstick Kaboomer Profiteer Dealmaker Rustbucket Greenback Bigbucks
Toxicwaste Sparkplug
)",
    R"([fun:highelf]
Pointyears Snobbyelf Highmaintain Elfesteem Arcanejunkie Quelfie Sunbather Fancyfeet Elfrespect
Highborner
)",
};

} // namespace TortoiseBots
