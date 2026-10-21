"""The Exotic Pokémon Safari's Alola zone (Pokémon Canvas), for build_exotic_safari.py.

Six areas from Nick's planner (exotic-safari-planner-alola_update.xlsx): islands linked by sea
(surfing off one area's edge comes out on the next one's water), a Hawaiian look throughout:

    Hall door -> RESORT (Hano Beach: the Sevii Islands' Resort Gorgeous, its mansion the way back
    to the hall; the hub) -> west the OCEAN (Seafolk Village: Pacifidlog's houses on rafts, out at
    sea) -> west the RUINS (Ruins of Conflict: Six Island's Ruin Valley, jungle)
                          -> east the BEACH (Big Wave Beach: One Island's Treasure Beach)
                          -> north the SNOWY MOUNTAIN (Mount Lanakila: Seven Island's Sevault Canyon,
                             snow above its snowline, green below, the sea at its foot)
                          -> south the VOLCANO (Wela Volcano Park: Mt. Ember, Kindle Road's beach
                             and sea at its foot)
"""
import random
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paldea import read_layout, blk, land, ROOT   # noqa: E402

REGION = "ALOLA"
HUB = "RESORT"
LV = (48, 52)


def tileset_dir(sym):
    import render_map
    return render_map.tileset_dir(sym)


# ---------------------------------------------------------------------------------------------
# Hano Beach: Resort Gorgeous, with palm-garden grass on its island
# ---------------------------------------------------------------------------------------------
TALL = blk(0x00D, 0)


def resort():
    import build_exotic_safari as B
    g, lay = read_layout("LAYOUT_FIVE_ISLAND_RESORT_GORGEOUS")
    g = [row[:] for row in g]
    beh = B.Behaviors((lay["primary_tileset"], lay["secondary_tileset"]), "frlg")
    for x0, y0, x1, y1 in ((21, 9, 29, 12), (40, 10, 47, 12), (27, 3, 35, 4), (61, 15, 67, 15), (11, 10, 14, 11)):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                v = g[y][x]
                if not (v >> 10) & 3 and beh(v) not in B.MB_SURFABLE and (x, y) != (39, 9):
                    g[y][x] = TALL
    return g, lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Big Wave Beach: Treasure Beach, without One Island's harbour at its top
# ---------------------------------------------------------------------------------------------
def beach():
    g, lay = read_layout("LAYOUT_ONE_ISLAND_TREASURE_BEACH")
    g = [row[:] for row in g]
    sea = g[5][5]
    for y in range(0, 4):
        for x in range(24):
            g[y][x] = sea
    # (its foot: the rock ends in a coast - the rock's foot, sand, the shore, the sea - like Kindle Road's)
    kindle, _ = read_layout("LAYOUT_ONE_ISLAND_KINDLE_ROAD")
    for y, k in ((35, kindle[17][4]), (36, kindle[18][5]), (37, kindle[19][5]), (38, kindle[20][5]), (39, kindle[20][5])):
        for x in range(24):
            g[y][x] = k if y > 35 or (g[34][x] >> 10) & 3 else kindle[18][5]
    return g, 24, 40


# ---------------------------------------------------------------------------------------------
# Seafolk Village: Pacifidlog's rafts and houses, far out at sea (Emerald's, converted)
# ---------------------------------------------------------------------------------------------
OCEAN_W, OCEAN_H, OCEAN_DX, OCEAN_DY = 36, 30, 8, 4


def ocean():
    src, lay = read_layout("LAYOUT_PACIFIDLOG_TOWN")
    sea = src[19][1]
    g = [[sea] * OCEAN_W for _ in range(OCEAN_H)]
    for y in range(9, 31):
        for x in range(20):
            v = src[y][x]
            if y == 9 and src[10][x] == sea:      # (the cliff's shadow on the water: open sea here)
                v = sea
            g[OCEAN_DY + y - 9][OCEAN_DX + x] = v
    rock = [[src[y][x] for x in (18, 19)] for y in (9, 10)]       # (a sea rock, 2x2)
    for rx, ry in ((2, 3), (31, 6), (4, 24), (30, 25), (16, 1), (22, 28)):
        for b in (0, 1):
            for a in (0, 1):
                g[ry + b][rx + a] = rock[b][a]
    AREAS["OCEAN"]["behaviors"] = pacifidlog_logs()
    return g, OCEAN_W, OCEAN_H


def ocean_doors():
    """the houses' and the Pokémon Center's doors, where they are in the ocean"""
    return [(x + OCEAN_DX, y - 9 + OCEAN_DY) for x, y in ((8, 15), (16, 13), (3, 22), (12, 24), (2, 12), (17, 21))]


def pacifidlog_logs():
    """Pacifidlog's floating logs sink under the player (an Emerald field effect that redraws
    Pacifidlog's own metatiles): plain floor here"""
    import build_exotic_safari as B
    g, lay = read_layout("LAYOUT_PACIFIDLOG_TOWN")
    beh = B.Behaviors((lay["primary_tileset"], lay["secondary_tileset"]), "emerald")
    inv = {v: k for k, v in B.MB.items()}
    return {v & 0x3FF: "MB_NORMAL" for row in g for v in row if "PACIFIDLOG" in inv.get(beh(v), "")}


# ---------------------------------------------------------------------------------------------
# Wela Volcano Park: Mt. Ember, with Kindle Road's foot of the mountain, beach and sea below it
# ---------------------------------------------------------------------------------------------
EMBER_X0, EMBER_X1 = 6, 51            # (Mt. Ember's columns kept: 45)
KINDLE_DX = 28 - 11 - EMBER_X0        # (Kindle Road's stairs under Mt. Ember's)


def volcano():
    ember, _ = read_layout("LAYOUT_MT_EMBER_EXTERIOR")
    kindle, _ = read_layout("LAYOUT_ONE_ISLAND_KINDLE_ROAD")
    W = EMBER_X1 - EMBER_X0
    g = [row[EMBER_X0:EMBER_X1] for row in ember[:48]]
    for y in range(6, 28):
        row = []
        for X in range(W):
            x = X - KINDLE_DX
            v = kindle[y][x] if 0 <= x < 24 else None
            # Beyond Kindle Road (and its own sea at its sides): a coast drawn on - rock, the rock's
            # foot, sand, the shore, then the sea
            if not 3 <= x < 22 or (y in (18, 19) and not 5 <= x < 16):
                v = (kindle[8][5] if y < 17 else kindle[17][4] if y == 17 else kindle[18][5] if y == 18
                     else kindle[19][5] if y == 19 else kindle[20][5])
            row.append(v)
        g.append(row)
    return g, W, len(g)


# ---------------------------------------------------------------------------------------------
# Mount Lanakila: Sevault Canyon, snowed over above a ragged snowline (copies of its metatiles
# in snowy palettes), green and tropical below it, the sea at its foot
# ---------------------------------------------------------------------------------------------
SNOW_SLOTS = [7, 9, 10, 11, 12]       # (palettes the canyon doesn't use: the snowy copies)


def snow_color(c):
    import colorsys
    r, g, b = c
    h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
    deg = h * 360
    lum = (0.3 * r + 0.59 * g + 0.11 * b) / 255
    if s < 0.1 or 190 <= deg <= 260:                 # greys, whites, blues: a little colder
        return (int(r * 0.92), int(g * 0.95), min(255, int(b * 1.02 + 8)))
    if 60 <= deg < 190:                               # grass -> snow, its shadows blue-grey
        t = max(0.0, min(1.0, (lum - 0.2) / 0.6))
        lo, hi = (104, 120, 160), (248, 250, 255)
        return tuple(int(lo[k] + (hi[k] - lo[k]) * t) for k in range(3))
    if lum > 0.62:                                    # sand, light rock -> snow
        return (240, 244, 255)
    t = max(0.0, min(1.0, lum * 1.15))                # rock -> icy blue-grey rock
    return (int(60 + 150 * t), int(70 + 152 * t), int(96 + 150 * t))


def lanakila():
    import build_exotic_safari as B
    g, lay = read_layout("LAYOUT_SEVEN_ISLAND_SEVAULT_CANYON")
    g = [row[:] for row in g]
    w, h = lay["width"], lay["height"]
    rnd = random.Random("lanakila")
    line, cur = [], 44
    for x in range(w):                                # (a ragged snowline)
        cur = max(38, min(50, cur + rnd.choice((-2, -1, 0, 0, 1, 2))))
        line.append(cur)
    prim = tileset_dir("gTileset_General_Frlg")
    sec = tileset_dir("gTileset_SeviiIslands67")
    pmt, smt = (prim / "metatiles.bin").read_bytes(), (sec / "metatiles.bin").read_bytes()
    entries = lambda m: struct.unpack_from("<8H", pmt, m * 16) if m < 640 else struct.unpack_from("<8H", smt, (m - 640) * 16)
    snowy = sorted({g[y][x] & 0x3FF for x in range(w) for y in range(line[x])})
    pals = sorted({v >> 12 for m in snowy for v in entries(m)})
    assert len(pals) <= len(SNOW_SLOTS), pals
    slot = dict(zip(pals, SNOW_SLOTS))
    new_id = {}

    def edit(out):
        mt = bytearray((out / "metatiles.bin").read_bytes())
        at = bytearray((out / "metatile_attributes.bin").read_bytes())
        pat = (prim / "metatile_attributes.bin").read_bytes()
        asz = len(at) // (len(mt) // 16)
        for p, s in slot.items():
            src = (prim if p < 7 else sec) / "palettes" / f"{p:02d}.pal"
            B.write_pal(out / "palettes" / f"{s:02d}.pal", [snow_color(c[:3]) for c in B.render_map.load_pal(src)])
        for m in snowy:
            new_id[m] = 0x280 + len(mt) // 16
            mt += struct.pack("<8H", *[(v & 0x0FFF) | (slot[v >> 12] << 12) for v in entries(m)])
            at += (pat[m * asz:(m + 1) * asz] if m < 640 else at[(m - 640) * asz:(m - 639) * asz])
        assert len(mt) // 16 <= 384
        (out / "metatiles.bin").write_bytes(mt)
        (out / "metatile_attributes.bin").write_bytes(at)

    B.clone_tileset("gTileset_SeviiIslands67", "AlolaLanakila", edit=edit)
    for x in range(w):
        for y in range(line[x]):
            v = g[y][x]
            g[y][x] = (v & ~0x3FF) | new_id[v & 0x3FF]
    return g, w, h


# ---------------------------------------------------------------------------------------------
# Ruins of Conflict: Six Island's Ruin Valley (its jungle of tall grass; the Dotted Hole the
# ruins' shrine)
# ---------------------------------------------------------------------------------------------
RUINS_X0, RUINS_Y0 = 4, 4


def ruins():
    g, _ = read_layout("LAYOUT_SIX_ISLAND_RUIN_VALLEY")
    g = [row[RUINS_X0:48] for row in g[RUINS_Y0:38]]
    return g, 48 - RUINS_X0, 38 - RUINS_Y0


def R(x, y):
    """Ruin Valley's coordinates -> the ruins area's"""
    return (x - RUINS_X0, y - RUINS_Y0)


def V(x, y):
    """Mt. Ember's coordinates -> the volcano area's"""
    return (x - EMBER_X0, y)


# ---------------------------------------------------------------------------------------------
# The areas
# ---------------------------------------------------------------------------------------------
SEA = ("WISHIWASHI", 48, 52)
SEA_BLOCK = 0x112B            # (FireRed's open sea, General_Frlg: past the islands' edges)
VOLCANO_H = 48 + 22
AREAS = {
    "RESORT": {
        "map": "ExoticSafari_AlolaResort", "build": resort, "tilesets": ("gTileset_General_Frlg", "gTileset_SeviiIslands45"),
        "border": SEA_BLOCK, "music": "MUS_RG_SEVII_45", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "HALL": ("warp", [(39, 8)], (39, 9), ("HALL", None)),             # (the resort's mansion)
            "OCEAN": ("edge", [(0, y) for y in range(20)], (1, 10), ("OCEAN", "RESORT")),
            "BEACH": ("edge", [(71, y) for y in range(17)], (70, 5), ("BEACH", "RESORT")),
            "SNOW": ("edge", [(x, 0) for x in range(16, 60)], (48, 1), ("SNOW", "RESORT")),
            "VOLCANO": ("edge", [(x, 19) for x in range(16, 60)], (40, 18), ("VOLCANO", "RESORT")),
        },
        "remove_holes": [(64, 13)],
        "land": land("YUNGOOS", "PIKIPEK", "RATTATA_ALOLA", "MEOWTH_ALOLA", "ORICORIO_POM_POM", "CUTIEFLY",
                     "GRIMER_ALOLA", "EXEGGUTOR_ALOLA", "RAICHU_ALOLA", "PERSIAN_ALOLA", ("ROWLET", 48, 50), ("PHEROMOSA", 55, 58)),
        "water": [SEA, ("PYUKUMUKU", 48, 52), ("MAREANIE", 48, 52), ("BRUXISH", 50, 54), ("DHELMISE", 52, 55)],
        "fishing": [("MAGIKARP", 45, 50), ("WISHIWASHI", 45, 50),
                    ("WISHIWASHI", 48, 52), ("MAREANIE", 48, 52), ("BRUXISH", 48, 52),
                    ("BRUXISH", 50, 54), ("WISHIWASHI", 50, 54), ("TOXAPEX", 50, 54), ("DHELMISE", 52, 55), ("GYARADOS", 52, 55)],
        "statics": [("TYPE_NULL", (27, 6)), ("MAGEARNA", (65, 14)), ("MELTAN", (10, 7))],
        "items": [],
    },
    "BEACH": {
        "map": "ExoticSafari_AlolaBeach", "build": beach, "tilesets": ("gTileset_General_Frlg", "gTileset_SeviiIslands123"),
        "border": SEA_BLOCK, "music": "MUS_RG_SEVII_123", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RESORT": ("edge", [(0, y) for y in range(24)], (1, 11), ("RESORT", "BEACH")),
        },
        "land": land("SANDYGAST", "WIMPOD", "DIGLETT_ALOLA", "WINGULL", "ORICORIO_PAU", "CRABRAWLER", "PALOSSAND",
                     "DUGTRIO_ALOLA", "PELIPPER", "GOLISOPOD", ("BUZZWOLE", 55, 58), ("STAKATAKA", 55, 58)),
        "water": [("MANTYKE", 48, 52), ("WINGULL", 48, 52), ("WAILMER", 48, 52), ("MANTINE", 50, 54), ("WAILORD", 52, 55)],
        "fishing": [("MAGIKARP", 45, 50), ("WISHIWASHI", 45, 50),
                    ("WISHIWASHI", 48, 52), ("BRUXISH", 48, 52), ("MAREANIE", 48, 52),
                    ("BRUXISH", 50, 54), ("TOXAPEX", 50, 54), ("WISHIWASHI", 50, 54), ("DHELMISE", 52, 55), ("POPPLIO", 48, 50)],
        "statics": [("COSMOG", (11, 21))],
        "items": [],
    },
    "OCEAN": {
        "map": "ExoticSafari_AlolaOcean", "build": ocean, "tilesets": ("gTileset_General", "gTileset_Pacifidlog"),
        "version": "emerald", "convert": ("AlolaOcean", "alola_ocean_frlg"), "border_at": (0, 0),
        "music": "MUS_RG_SEVII_ROUTE", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RESORT": ("edge", [(OCEAN_W - 1, y) for y in range(OCEAN_H)], (OCEAN_W - 2, 14), ("RESORT", "OCEAN")),
            "RUINS": ("edge", [(0, y) for y in range(OCEAN_H)], (1, 14), ("RUINS", "OCEAN")),
        },
        "remove_holes": ocean_doors(),
        "land": land("WINGULL", "PELIPPER", "WIMPOD", "WISHIWASHI", "PYUKUMUKU", "MAREANIE", "BRUXISH", "DHELMISE",
                     "GOLISOPOD", "TOXAPEX", ("POPPLIO", 48, 50), ("NIHILEGO", 55, 58)),
        "water": [SEA, ("TENTACOOL", 48, 52), ("PYUKUMUKU", 48, 52), ("DHELMISE", 52, 55), ("NIHILEGO", 55, 58)],
        "fishing": [("MAGIKARP", 45, 50), ("WISHIWASHI", 45, 50),
                    ("WISHIWASHI", 48, 52), ("BRUXISH", 48, 52), ("MAREANIE", 48, 52),
                    ("BRUXISH", 50, 54), ("DHELMISE", 50, 54), ("TOXAPEX", 50, 54), ("WAILORD", 52, 55), ("POIPOLE", 55, 58)],
        "statics": [("ZERAORA", (OCEAN_DX + 18, OCEAN_DY + 14 - 9))],
        "items": [],
    },
    "RUINS": {
        "map": "ExoticSafari_AlolaRuins", "build": ruins, "tilesets": ("gTileset_General_Frlg", "gTileset_SeviiIslands67"),
        "music": "MUS_RG_SEVII_DUNGEON", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "OCEAN": ("edge", [R(47, y) for y in range(9, 13)], R(46, 10), ("OCEAN", "RUINS")),
        },
        "remove_holes": [R(24, 24)],
        "land": land("FOMANTIS", "MORELULL", "GRUBBIN", "CHARJABUG", "DEWPIDER", "STUFFUL", "ORICORIO_SENSU", "MIMIKYU",
                     "LURANTIS", "SHIINOTIC", ("KARTANA", 55, 58), ("XURKITREE", 55, 58)),
        "water": [("DEWPIDER", 48, 52), ("WISHIWASHI", 48, 52), ("ARAQUANID", 50, 54), ("BRUXISH", 50, 54), ("ARAQUANID", 52, 55)],
        "statics": [("MARSHADOW", R(24, 25))],
        "items": [("DUSK_STONE", R(6, 33))],
    },
    "VOLCANO": {
        "map": "ExoticSafari_AlolaVolcano", "build": volcano, "tilesets": ("gTileset_General_Frlg", "gTileset_SeviiIslands123"),
        "music": "MUS_RG_SEVII_CAVE", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RESORT": ("edge", [(x, VOLCANO_H - 1) for x in range(EMBER_X1 - EMBER_X0)], (KINDLE_DX + 17, VOLCANO_H - 2),
                       ("RESORT", "VOLCANO")),
            # (Mt. Ember's caves: one leads up the mountain to the other)
            "CAVE_UP": ("warp", [V(14, 24)], V(14, 25), ("VOLCANO", "CAVE_DOWN")),
            "CAVE_DOWN": ("warp", [V(39, 19)], V(39, 20), ("VOLCANO", "CAVE_UP")),
        },
        "remove_holes": [V(29, 7), V(42, 39)],
        "land": land("SALANDIT", "ROCKRUFF", "GEODUDE_ALOLA", "CUBONE", "ORICORIO", "MAROWAK_ALOLA", "GRAVELER_ALOLA",
                     "TURTONATOR", "MINIOR", "SALAZZLE", ("LITTEN", 48, 50), ("BLACEPHALON", 55, 58)),
        "water": [SEA, ("MAREANIE", 48, 52), ("TENTACOOL", 48, 52), ("TOXAPEX", 50, 54), ("WAILMER", 50, 54)],
        "fishing": [("MAGIKARP", 45, 50), ("WISHIWASHI", 45, 50),
                    ("WISHIWASHI", 48, 52), ("MAREANIE", 48, 52), ("BRUXISH", 48, 52),
                    ("BRUXISH", 50, 54), ("TOXAPEX", 50, 54), ("WISHIWASHI", 50, 54), ("DHELMISE", 52, 55), ("GYARADOS", 52, 55)],
        "statics": [("SOLGALEO", V(29, 8)), ("NECROZMA", V(38, 11))],
        "items": [("SUN_STONE", V(19, 10))],
    },
    "SNOW": {
        "map": "ExoticSafari_AlolaLanakila", "build": lanakila, "tilesets": ("gTileset_General_Frlg", "gTileset_AlolaLanakila"),
        "border": 0x471, "music": "MUS_RG_SEVII_67", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RESORT": ("edge", [(15, 79), (16, 79)] + [(x, 79) for x in list(range(0, 9)) + list(range(20, 24))], (15, 78),
                       ("RESORT", "SNOW")),
        },
        "remove_holes": [(7, 17), (14, 61)],
        "land": land("VULPIX_ALOLA", "SANDSHREW_ALOLA", "CRABRAWLER", "SNORUNT", "VANILLISH", "CRABOMINABLE",
                     "SANDSLASH_ALOLA", "ABSOL", "SNEASEL", "FROSLASS", ("CELESTEELA", 55, 58), ("GUZZLORD", 55, 58)),
        "water": [SEA, ("MANTYKE", 48, 52), ("PYUKUMUKU", 48, 52), ("MANTINE", 50, 54), ("DHELMISE", 52, 55)],
        "statics": [("LUNALA", (11, 5))],
        "items": [("MOON_STONE", (5, 9))],
    },
}
ROAMERS = ["TAPU_KOKO", "TAPU_LELE", "TAPU_BULU", "TAPU_FINI"]
UNIQUE_ITEMS = set()
