"""The Exotic Pokémon Safari's Sinnoh zone (Pokémon Canvas), for build_exotic_safari.py.

Six areas from Nick's planner (exotic-safari-planner-sinnoh_update.xlsx), Stark Mountain in two
parts (outside and in):

    Hall door -> LAKE (The Three Lakes: drawn, three lakes in pine woods, an island in each - Lake
    Verity, Lake Valor, Lake Acuity; the hub)
        west -> MEADOW (Floaroma Meadow: Hoenn's Route 117, flowers and ponds)
        south-east -> MARSH (The Great Marsh: Hoenn's Safari Zone, murky)
        north -> CITY (Snowpoint City: the Sevii Islands' Four Island under snow)
                 -> its cave in the cliff: RUINS (Hall of Origin: Sky Pillar's top, white and gold
                    under a violet sky)
        east -> VOLCANO (Stark Mountain: Hoenn's Jagged Pass, dark basalt and ash)
                -> its cave: DEPTHS (Stark Mountain's inside: the Fiery Path)
"""
import colorsys
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paldea import read_layout, blk, land, ROOT, draw, framed   # noqa: E402

REGION = "SINNOH"
HUB = "LAKE"
LV = (48, 52)


def hsv(c):
    return colorsys.rgb_to_hsv(*(v / 255 for v in c[:3]))


def rgb(h, s, v):
    return tuple(int(max(0, min(1, k)) * 255) for k in colorsys.hsv_to_rgb(h % 1, s, v))


def solid(v):
    return (v >> 10) & 3


def edge_cells(layout_id, side):
    """the walkable cells along one edge of a layout ('left', 'right', 'top', 'bottom')"""
    g, l = read_layout(layout_id)
    w, h = l["width"], l["height"]
    if side in ("left", "right"):
        x = 0 if side == "left" else w - 1
        return [(x, y) for y in range(h) if not solid(g[y][x])]
    y = 0 if side == "top" else h - 1
    return [(x, y) for x in range(w) if not solid(g[y][x])]


def inward(cells, side):
    x, y = cells[len(cells) // 2]
    return {"left": (x + 1, y), "right": (x - 1, y), "top": (x, y + 1), "bottom": (x, y - 1)}[side]


# ---------------------------------------------------------------------------------------------
# The Three Lakes: drawn (Viridian Forest's trees, FireRed's ponds), pines a cool blue-green.
# Lake Verity (west), Lake Acuity (east), Lake Valor (south); an island in each
# ---------------------------------------------------------------------------------------------
LAKE_W, LAKE_H = 60, 40
LAKES = {   # name: (centre x, centre y, radius x, radius y), its island (centre, radii)
    "VERITY": ((14, 12, 8.6, 5.6), (14, 12, 2.6, 1.7)),
    "ACUITY": ((45, 12, 8.6, 5.6), (45, 12, 2.6, 1.7)),
    "VALOR": ((30, 26, 9.6, 5.6), (30, 26, 2.6, 1.7)),
}
W_TL, W_TR, W_T, W_L, W_R, W_MID = 0x122, 0x121, 0x123, 0x12A, 0x12C, 0x12B
W_IN_TR, W_IN_TL = 0x130, 0x131          # (inner corners: land up-right, up-left)
W_R_OPEN, W_L_OPEN = 0x128, 0x129        # (an edge whose diagonal above is water)
RIM_L, RIM_M, RIM_R = 0x132, 0x133, 0x134


def island_middle(name):
    return LAKES[name][1][:2]


def lake_mask():
    """water cells: ovals minus their islands, smoothed so every cell has an edge piece (no
    one-cell-wide water or land between water)"""
    water = set()
    for (cx, cy, rx, ry), (ix, iy, irx, iry) in LAKES.values():
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1 and ((x - ix) / irx) ** 2 + ((y - iy) / iry) ** 2 > 1:
                    water.add((x, y))
    changed = True
    while changed:
        changed = False
        for (x, y) in list(water):
            W = lambda dx, dy: (x + dx, y + dy) in water
            if (not W(-1, 0) and not W(1, 0)) or (not W(0, -1) and not W(0, 1)):
                water.discard((x, y)); changed = True
        for (x, y) in {(x + dx, y + dy) for x, y in water for dx in (-1, 0, 1) for dy in (-1, 0, 1)} - water:
            W = lambda dx, dy: (x + dx, y + dy) in water
            if (W(-1, 0) and W(1, 0)) or (W(0, -1) and W(0, 1)):
                water.add((x, y)); changed = True
    return water


def paint_water(g, water):
    for (x, y) in water:
        W = lambda dx, dy: (x + dx, y + dy) in water
        if not W(0, -1):
            m = W_TL if not W(-1, 0) else W_TR if not W(1, 0) else W_T
        elif not W(-1, 0):
            m = W_L if not W(-1, -1) else W_L_OPEN
        elif not W(1, 0):
            m = W_R if not W(1, -1) else W_R_OPEN
        elif not W(1, -1):
            m = W_IN_TR
        elif not W(-1, -1):
            m = W_IN_TL
        else:
            m = W_MID
        g[y][x] = blk(m, 0, 1)
    for (x, y) in {(x, y + 1) for x, y in water} - water:      # (the grassy rim below the water)
        W = lambda dx, dy: (x + dx, y + dy) in water
        m = RIM_M if (W(-1, -1) and W(1, -1)) or (not W(-1, -1) and not W(1, -1)) else RIM_L if not W(-1, -1) else RIM_R
        g[y][x] = blk(m, 0)


def lake_color(r, g, b):
    h, s, v = hsv((r, g, b))
    deg = h * 360
    if 70 <= deg <= 165 and s > 0.25 and v < 0.72:  # the pines' leaves: a cooler, bluer green
        return rgb((deg + 16) / 360, s * 0.92, v * 0.96)
    return (r, g, b)


def lake():
    W, H = LAKE_W, LAKE_H
    inner = [["."] * (W - 6) for _ in range(H - 9)]

    def grass(x0, y0, x1, y1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                inner[y - 4][x - 3] = "g"
    def put(x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                inner[y - 4][x - 3] = c
    grass(4, 21, 14, 31)
    grass(43, 21, 55, 33)
    grass(25, 4, 27, 12)
    grass(31, 4, 33, 12)
    grass(4, 4, 6, 9)
    grass(52, 4, 55, 9)
    # clumps of pines (3 rows, odd to odd: the woods' 2-row trees) and flowers
    for x0, y0 in ((21, 19), (36, 19), (51, 27), (3, 31), (54, 31)):
        put(x0, y0, x0 + 2, y0 + 2, "T")
    for x0, y0 in ((24, 14), (34, 14), (17, 30), (41, 30), (8, 19), (49, 19)):
        put(x0, y0, x0 + 1, y0 + 1, "f")
    inner = ["".join(r) for r in inner]
    rows = framed(inner, W, H, openings=[
        (0, 22, 2, 24),                     # west (the meadow)
        (57, 22, 59, 24),                   # east (Stark Mountain)
        (28, 0, 30, 3),                     # north (Snowpoint City)
        (48, 35, 50, 39),                   # south-east (the Great Marsh)
        (9, 35, 11, 35), (9, 36, 11, 39, "P"),   # south (the hall)
    ])
    g = draw(rows, exit_row=36)
    water = lake_mask()
    for name, (_l, (ix, iy, irx, iry)) in LAKES.items():
        assert (ix, iy) not in water and (ix, iy + 1) not in water, name
    paint_water(g, water)
    return g, W, H


# ---------------------------------------------------------------------------------------------
# Floaroma Meadow: Hoenn's Route 117 (its Day Care's house gone: woods)
# ---------------------------------------------------------------------------------------------
def meadow():
    g, l = read_layout("LAYOUT_ROUTE117")
    g = [row[:] for row in g]
    grass, fence = g[6][50], g[5][56]
    for y in range(2, 5):                    # (the Day Care: grass; its sign and door: fence)
        for x in range(50, 54):
            g[y][x] = grass
    for x in range(49, 54):
        g[5][x] = fence
    return g, l["width"], l["height"]


# ---------------------------------------------------------------------------------------------
# The Great Marsh: Hoenn's Safari Zone (south-west), its water murky and its grass a swampy olive;
# its rest house gone
# ---------------------------------------------------------------------------------------------
def marsh_color(r, g, b):
    h, s, v = hsv((r, g, b))
    deg = h * 360
    if 70 <= deg <= 165 and s > 0.12:               # grass, leaves: swampy olive
        return rgb((deg - 22) / 360, min(1, s * 1.05), v * 0.86)
    if 180 <= deg <= 240 and s > 0.2:               # water: murky green-brown
        return rgb(172 / 360, min(1, s * 0.75), v * 0.5)
    return (r, g, b)


def marsh():
    g, l = read_layout("LAYOUT_SAFARI_ZONE_SOUTHWEST")
    g = [row[:] for row in g]
    for y, row in enumerate(g):
        for x, v in enumerate(row):
            if v & 0x3FF in (0x310, 0x318):           # (no PokéBlock feeders)
                row[x] = g[y][x - 1]
    g[17][18] = g[17][17]                             # (the feeder's lower half)
    for y in range(3, 9):                             # (the rest house: the grass around it)
        for x in range(26, 34):
            g[y][x] = g[9][x] if not solid(g[9][x]) else g[9][25]
    return g, l["width"], l["height"]


# ---------------------------------------------------------------------------------------------
# Snowpoint City: Four Island under snow (Unova's snow colors); Icefall Cave's mouth leads up to
# the Hall of Origin
# ---------------------------------------------------------------------------------------------
LAND = {0x001, 0x008, 0x009, 0x00E, 0x00F, 0x010, 0x011, 0x115}     # (Four Island's ground, sand)


def city():
    g, l = read_layout("LAYOUT_FOUR_ISLAND")
    g = [row[:] for row in g]
    tall = blk(0x00D, 0)
    for x0, y0, x1, y1 in ((5, 14, 7, 23), (18, 28, 24, 29)):   # (snowy fields)
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if not solid(g[y][x]) and g[y][x] & 0x3FF in LAND:
                    g[y][x] = tall
    return g, l["width"], l["height"]


def snow_color(r, g, b):
    import unova
    return unova.snow_color(r, g, b)


# ---------------------------------------------------------------------------------------------
# Stark Mountain: Hoenn's Jagged Pass (outside) and Fiery Path (inside), dark basalt and ash
# ---------------------------------------------------------------------------------------------
def basalt_color(r, g, b):
    h, s, v = hsv((r, g, b))
    deg = h * 360
    if (deg < 40 or deg > 320) and s > 0.12 and v > 0.25:   # the pink-red rock: dark basalt, ember-tinted
        return rgb(8 / 360, s * 0.55, v * 0.62)
    if 40 <= deg < 70 and s > 0.15:                          # sandy ash: grey ash
        return rgb(30 / 360, s * 0.25, v * 0.78)
    return (r, g, b)


def volcano():
    g, l = read_layout("LAYOUT_JAGGED_PASS")
    return [row[:] for row in g], l["width"], l["height"]


def depths():
    g, l = read_layout("LAYOUT_FIERY_PATH")
    return [row[:] for row in g], l["width"], l["height"]


# ---------------------------------------------------------------------------------------------
# Hall of Origin: Sky Pillar's top (without its cracks), white stone and gold under a violet sky
# ---------------------------------------------------------------------------------------------
def origin_color(r, g, b):
    h, s, v = hsv((r, g, b))
    deg = h * 360
    if 185 <= deg <= 215 and s > 0.35 and v > 0.7:          # the sky: deep violet
        return rgb(268 / 360, 0.55, v * 0.62)
    if 200 <= deg <= 260 and s <= 0.35:                      # the blue-grey stone: white marble
        return rgb(46 / 360, s * 0.25, min(1, 0.55 + v * 0.55))
    if 15 <= deg <= 45:                                      # brown: gold
        return rgb(44 / 360, min(1, s * 1.1), min(1, v * 1.1))
    return (r, g, b)


def ruins():
    g, l = read_layout("LAYOUT_SKY_PILLAR_TOP_CLEAN")
    return [row[:] for row in g], l["width"], l["height"]


# ---------------------------------------------------------------------------------------------
# The areas
# ---------------------------------------------------------------------------------------------
_MEADOW_EAST = edge_cells("LAYOUT_ROUTE117", "right")
_MARSH_TOP = [c for c in edge_cells("LAYOUT_SAFARI_ZONE_SOUTHWEST", "top") if 7 <= c[0] <= 10]

AREAS = {
    "LAKE": {
        "map": "ExoticSafari_SinnohLake", "build": lake, "border_size": (3, 2),
        "tilesets": ("gTileset_General_Frlg", "gTileset_ViridianForest"),
        "convert_frlg": True, "convert": ("SinnohLake", "sinnoh_lake"), "recolor": {p: lake_color for p in range(16)},
        "music": "MUS_RG_SEVII_45", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "HALL": ("warp", [(9, 36), (10, 36), (11, 36)], (10, 35), ("HALL", None)),
            "MEADOW": ("edge", [(0, 22), (0, 23), (0, 24)], (1, 23), ("MEADOW", "LAKE")),
            "VOLCANO": ("edge", [(59, 22), (59, 23), (59, 24)], (58, 23), ("VOLCANO", "LAKE")),
            "CITY": ("edge", [(28, 0), (29, 0), (30, 0)], (29, 1), ("CITY", "LAKE")),
            "MARSH": ("edge", [(48, 39), (49, 39), (50, 39)], (49, 38), ("MARSH", "LAKE")),
        },
        "land": land("BIDOOF", "STARLY", "SHINX", "BUIZEL", "BUNEARY", "KRICKETOT", "STARAVIA", "LUXIO",
                     "BIBAREL", "LOPUNNY", ("PIPLUP", 48, 50), ("LUXRAY", 52, 55)),
        "water": [("SHELLOS_WEST", 48, 52), ("BUIZEL", 48, 52), ("FINNEON", 48, 52), ("GASTRODON_WEST", 50, 54),
                  ("FLOATZEL", 52, 55)],
        "fishing": [("FINNEON", 45, 50), ("BUIZEL", 45, 50),
                    ("FINNEON", 48, 52), ("SHELLOS_WEST", 48, 52), ("BUIZEL", 48, 52),
                    ("LUMINEON", 50, 54), ("GASTRODON_WEST", 50, 54), ("FLOATZEL", 50, 54),
                    ("LUMINEON", 52, 55), ("PIPLUP", 48, 50)],
        "statics": [("UXIE", island_middle("VERITY")), ("AZELF", island_middle("ACUITY")),
                    ("MESPRIT", island_middle("VALOR"))],
        "items": [],
    },
    "MEADOW": {
        "map": "ExoticSafari_SinnohMeadow", "build": meadow, "tilesets": ("gTileset_General", "gTileset_Mauville"),
        "version": "emerald", "convert": ("SinnohMeadow", "sinnoh_meadow"),
        "music": "MUS_RG_ROUTE3", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "LAKE": ("edge", _MEADOW_EAST, inward(_MEADOW_EAST, "right"), ("LAKE", "MEADOW")),
        },
        "land": land("BUDEW", "COMBEE", "CHERUBI", "PACHIRISU", "BURMY_PLANT", "CHATOT", "DRIFLOON", "MUNCHLAX",
                     "CHERRIM", "MOTHIM", ("TURTWIG", 48, 50), ("ROSERADE", 52, 55)),
        "water": [("SHELLOS_WEST", 48, 52), ("BUIZEL", 48, 52), ("FINNEON", 48, 52), ("SHELLOS_WEST", 50, 54),
                  ("FLOATZEL", 52, 55)],
        "statics": [],
        "items": [],
    },
    "MARSH": {
        "map": "ExoticSafari_SinnohMarsh", "build": marsh, "tilesets": ("gTileset_General", "gTileset_Lilycove"),
        "version": "emerald", "convert": ("SinnohMarsh", "sinnoh_marsh"), "recolor": {p: marsh_color for p in range(16)},
        "music": "MUS_HG_NATIONAL_PARK", "weather": "WEATHER_RAIN", "type": "MAP_TYPE_ROUTE",
        "links": {
            "LAKE": ("edge", _MARSH_TOP, inward(_MARSH_TOP, "top"), ("LAKE", "MARSH")),
        },
        "land": land("SKORUPI", "CROAGUNK", "CARNIVINE", "SHELLOS_EAST", "YANMEGA", "HAPPINY", "STUNKY",
                     "GASTRODON_EAST", "TOXICROAK", "DRAPION", ("MIME_JR", 48, 50), ("TANGROWTH", 52, 55)),
        "water": [("SHELLOS_EAST", 48, 52), ("GASTRODON_EAST", 48, 52), ("FINNEON", 48, 52), ("BUIZEL", 50, 54),
                  ("LUMINEON", 52, 55)],
        "statics": [("PHIONE", (6, 25))],
        "items": [],
    },
    "CITY": {
        "map": "ExoticSafari_SinnohCity", "build": city, "tilesets": ("gTileset_General_Frlg", "gTileset_SeviiIslands45"),
        "convert_frlg": True, "convert": ("SinnohCity", "sinnoh_city"), "recolor": {p: snow_color for p in range(16)},
        "music": "MUS_RG_SEVII_123", "weather": "WEATHER_SNOW", "type": "MAP_TYPE_CITY",
        "links": {
            "LAKE": ("warp", [(10, 28)], (10, 27), ("LAKE", "CITY")),          # (the harbor's steps)
            "RUINS": ("warp", [(38, 12)], (38, 13), ("RUINS", "CITY")),        # (Icefall Cave's mouth)
        },
        "land": land("SNOVER", "GLAMEOW", "BRONZOR", "ABOMASNOW", "SNOVER", "PURUGLY", "WEAVILE", "MAMOSWINE",
                     "FROSLASS", "BURMY_TRASH", ("RIOLU", 48, 50), ("GLACEON", 50, 54)),
        "water": [("MANTYKE", 48, 52), ("FINNEON", 48, 52), ("BUIZEL", 48, 52), ("LUMINEON", 50, 54),
                  ("FLOATZEL", 52, 55)],
        "statics": [("REGIGIGAS", (14, 25))],
        "items": [],
    },
    "VOLCANO": {
        "map": "ExoticSafari_SinnohStarkMountain", "build": volcano, "tilesets": ("gTileset_General", "gTileset_Lavaridge"),
        "version": "emerald", "convert": ("SinnohStark", "sinnoh_stark"), "recolor": {p: basalt_color for p in range(16)},
        "border_at": (0, 2), "music": "MUS_RG_VICTORY_ROAD", "weather": "WEATHER_VOLCANIC_ASH", "type": "MAP_TYPE_ROUTE", "indoor_encounters": True,
        "links": {
            "LAKE": ("warp", [(13, 5), (14, 5)], (13, 6), ("LAKE", "VOLCANO")),
            "LAKE_SOUTH": ("warp", [(14, 40), (15, 40)], (14, 39), ("LAKE", "VOLCANO")),
            "DEPTHS": ("warp", [(16, 18)], (16, 19), ("DEPTHS", "VOLCANO")),
        },
        "land": land("HIPPOPOTAS", "CRANIDOS", "SHIELDON", "GLISCOR", "BONSLY", "BURMY_SANDY", "HIPPOWDON", "SKUNTANK",
                     "MAGMORTAR", "RHYPERIOR", ("CHIMCHAR", 48, 50), ("GARCHOMP", 52, 55)),
        "statics": [],
        "items": [],
    },
    "DEPTHS": {
        "map": "ExoticSafari_SinnohStarkInside", "build": depths, "tilesets": ("gTileset_General", "gTileset_Lavaridge"),
        "version": "emerald", "convert": ("SinnohStarkInside", "sinnoh_stark_inside"), "recolor": {p: basalt_color for p in range(16)},
        "music": "MUS_RG_SEVII_CAVE", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "VOLCANO": ("warp", [(26, 36)], (26, 35), ("VOLCANO", "DEPTHS")),
        },
        "remove_warps": [(26, 4)],
        "land": land("GIBLE", "BRONZOR", "CRANIDOS", "SHIELDON", "PROBOPASS", "GABITE", "RAMPARDOS", "BASTIODON",
                     "MAGMORTAR", "BRONZONG", ("INFERNAPE", 52, 55), ("GARCHOMP", 54, 56)),
        "statics": [("HEATRAN", (23, 3))],
        "items": [],
    },
    "RUINS": {
        "map": "ExoticSafari_SinnohHallOfOrigin", "build": ruins, "tilesets": ("gTileset_General", "gTileset_Pacifidlog"),
        "version": "emerald", "convert": ("SinnohOrigin", "sinnoh_origin"), "recolor": {p: origin_color for p in range(16)},
        "music": "MUS_RG_HALL_OF_FAME", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE", "indoor_encounters": True,
        "links": {
            "CITY": ("warp", [(16, 14)], (16, 15), ("CITY", "RUINS")),
        },
        "land": land("CHINGLING", "BRONZOR", "BRONZONG", "MISMAGIUS", "HONCHKROW", "TOGEKISS", "GALLADE", "DUSKNOIR",
                     "PORYGON_Z", "SPIRITOMB", ("ROTOM", 50, 54), ("ELECTIVIRE", 52, 55)),
        "statics": [("ARCEUS", (13, 4))],
        "items": [],
    },
}
ROAMERS = ["MANAPHY"]
UNIQUE_ITEMS = set()
