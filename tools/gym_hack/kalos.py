"""The Exotic Pokémon Safari's Kalos zone (Pokémon Canvas), for build_exotic_safari.py.

Puzzles: Zygarde's Cells about the zone; Hoopa's Prison Bottle on a pedestal (the cave); Volcanion
for Fire and Water together (a lead of one, the other behind it); Yveltal for ten Kalos POKéMON
caught; a Furfrou groomer in the city.

Six areas from Nick's planner (exotic-safari-planner-kalos_update.xlsx):

    Hall door -> CITY (Lumiose: FireRed's Saffron City, its four gates the way to everything, the
    Prism Tower drawn where Silph Co. stood; the hub)
        north gate -> WOODS (Winding Woods: a maze of trees, drawn, in a dim and eerie palette)
                      -> its far side the SNOWY PATH (Mamoswine Road: Route 9 under deep snow)
        west gate -> CAVE (Glittering Cave: Mt. Moon 1F, its rock a cold blue)
        east gate -> DESERT (Lumiose Badlands: Hoenn's ash-covered Route 113 as dusty badlands)
                      -> east the OCEAN (Azure Bay: the Sevii Islands' Water Labyrinth)
"""
import colorsys
import random
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paldea import read_layout, blk, land, ROOT, draw   # noqa: E402

REGION = "KALOS"
HUB = "CITY"
LV = (48, 52)


def tileset_dir(sym):
    import render_map
    return render_map.tileset_dir(sym)


def hsv(c):
    return colorsys.rgb_to_hsv(*(v / 255 for v in c[:3]))


def rgb(h, s, v):
    return tuple(int(max(0, min(1, k)) * 255) for k in colorsys.hsv_to_rgb(h % 1, s, v))


# ---------------------------------------------------------------------------------------------
# Lumiose City: Saffron City, Silph Co. replaced by a plaza with the Prism Tower (tile_import's
# drawing, its lights pulsing: InitTilesetAnim_Lumiose) and flower gardens (tall grass)
# ---------------------------------------------------------------------------------------------
FLOOR = 0x2E9                         # (Saffron's paving)
SILPH = (29, 16, 37, 30)              # (where Silph Co. stood: Saffron's coordinates)
TOWER_SLOT = 7                        # (a palette Saffron doesn't use: the tower's, pulsing)
TALL = blk(0x00D, 0)
# The city made bigger: two columns put in inside each side of its fence ring, two rows inside
# the top and the bottom (Saffron's x/y -> ours: cx, cy)
COL_LEFT, COL_RIGHT, ROW_TOP, ROW_BOTTOM = 15, 52, 16, 40
CITY_W, CITY_H = 66 + 4, 55 + 4


def cx(x):
    return x + 2 * (x > COL_LEFT) + 2 * (x > COL_RIGHT)


def cy(y):
    return y + 2 * (y > ROW_TOP) + 2 * (y > ROW_BOTTOM)


TOWER = (cx(29) + 1, cy(18), 7, 13)   # (the Prism Tower on its base: x, y, width, height)
GROOMER = (TOWER[0] - 3, TOWER[1] + TOWER[3] + 1)   # (the Furfrou groomer, out on the plaza)
PEDESTAL, HOOPA_SPOT = (34, 20), (34, 23)          # (Glittering Cave: the Prison Bottle, where Hoopa comes)


def _periodic(v):                     # (trees and fence posts: patterns two long, kept in step)
    return 0x00E <= v & 0x3FF <= 0x027 or v & 0x3FF in (0x30E, 0x30F)


def _insert_cols(g, k_of_row):
    out = []
    for y, row in enumerate(g):
        k = k_of_row(y)
        if _periodic(row[k]) and _periodic(row[k - 1]):
            ins = [row[k - 1], row[k]]
        elif _periodic(row[k + 1]) and _periodic(row[k + 2]):
            ins = [row[k + 1], row[k + 2]]
        else:
            ins = [row[k], row[k]]
        out.append(row[:k + 1] + ins + row[k + 1:])
    return out


def _insert_rows(g, k):
    t = [list(r) for r in zip(*g)]
    t = _insert_cols(t, lambda _: k)
    return [list(r) for r in zip(*t)]


def city():
    import build_exotic_safari as B
    import tile_import as T
    g, lay = read_layout("LAYOUT_SAFFRON_CITY")
    g = [row[:] for row in g]
    x0, y0, x1, y1 = SILPH
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            g[y][x] = blk(FLOOR, 0)
    for y in range(17, 22):                         # (the house beside it: its right side, which
        g[y][x0] = g[y - 7][24]                     # Silph stood over; another house's)
    for y in (22, 23, 24):                          # (Silph's sides)
        g[y][x0 - 1] = g[y][x1 + 1] = blk(FLOOR, 0)
    # bigger: columns (the top-right house runs up to the fence: there, the trees beyond move out)
    g = _insert_cols(g, lambda y: COL_LEFT)
    g = _insert_cols(g, lambda y: cx(COL_RIGHT) if y >= 14 else cx(COL_RIGHT) + 1)
    rx = cx(COL_RIGHT) + 1                          # (the new trees above the fence's corner end)
    g[12][rx + 1], g[12][rx + 2] = blk(0x024, 1), blk(0x025, 1)
    g[13][rx] = g[13][rx + 1] = blk(FLOOR, 0)
    g[13][rx + 2] = blk(0x2EA, 0)
    g = _insert_rows(g, ROW_TOP)
    g = _insert_rows(g, ROW_BOTTOM + 2)
    assert len(g) == CITY_H and len(g[0]) == CITY_W, (len(g[0]), len(g))
    # grass: a little round the tower's foot, and in the city's four corners
    tx, ty, tw, th = TOWER
    for y in range(ty + th - 4, ty + th):
        g[y][tx - 1] = g[y][tx + tw] = TALL
    corners = [(cx(COL_LEFT), cy(10)), (cx(COL_RIGHT) + 2 - 3, cy(15)),
               (cx(COL_LEFT), cy(ROW_BOTTOM) + 1), (cx(COL_RIGHT) + 2 - 3, cy(ROW_BOTTOM) + 1)]
    for gx, gy in corners:
        for y in range(gy, gy + 3):
            for x in range(gx, gx + 4):
                if g[y][x] == blk(FLOOR, 0):
                    g[y][x] = TALL
    ids = {}

    def edit(out):
        tgt = T.Target(out)
        tgt.compact({v & 0x3FF for row in g for v in row})
        prim = tileset_dir("gTileset_General_Frlg")
        floor = tgt.entries(FLOOR, prim)
        ids["tower"] = T.add_prism_tower(tgt, TOWER_SLOT, floor[:4], tgt.attr(FLOOR, prim), tw, th)
        tgt.save()

    B.clone_tileset("gTileset_SaffronCity", "KalosLumiose", edit=edit, callback="InitTilesetAnim_Lumiose")
    for k, m in enumerate(ids["tower"]):
        g[ty + k // tw][tx + k % tw] = blk(m, 1)
    return g, CITY_W, CITY_H


# ---------------------------------------------------------------------------------------------
# Winding Woods: a maze of trees (drawn with paldea.draw: Viridian Forest's 3x2 trees), in a dim,
# misty palette - easy to get lost in, ghosts about
# ---------------------------------------------------------------------------------------------
MAZE_N, MAZE_M = 8, 7                 # (rooms across, down)
WOODS_W, WOODS_H = 3 * (2 * MAZE_N + 1), 3 * (2 * MAZE_M + 1) + 3
WOODS_BOTTOM_BX, WOODS_TOP_BX = 7, 13  # (the ways out: block columns, odd)


def woods_color(i, c):
    h, s, v = hsv(c)
    deg = h * 360
    if 60 <= deg <= 180 and s > 0.15:               # leaves, grass: deep blue-green, dim
        return rgb((168 + (deg - 60) * 0.25) / 360, min(1, s * 0.7 + 0.1), v * 0.62)
    if s < 0.12:                                    # greys: cold
        return rgb(230 / 360, 0.12, v * 0.8)
    if 15 <= deg < 60:                              # browns, trunks, earth: dark and grey
        return rgb((deg + 10) / 360, s * 0.55, v * 0.6)
    return rgb(h, s * 0.8, v * 0.75)


def woods_rows():
    rnd = random.Random("winding woods")
    W, H = WOODS_W, WOODS_H
    rows = [["T"] * W for _ in range(H)]
    def carve_block(bx, by, c=None):
        c = c or ("." if rnd.random() < 0.22 else "g")        # (bare forest floor here and there)
        y0 = 3 * by + 3
        for y in range(y0, y0 + 3):
            for x in range(3 * bx, 3 * bx + 3):
                rows[y][x] = c
    # a perfect maze (depth first), then a few walls knocked through (loops: more ways to wander)
    seen = set()
    stack = [(1, 2 * MAZE_M - 1)]
    seen.add(stack[0])
    carve_block(*stack[0])
    while stack:
        bx, by = stack[-1]
        nbrs = [(bx + dx, by + dy) for dx, dy in ((2, 0), (-2, 0), (0, 2), (0, -2))
                if 1 <= bx + dx <= 2 * MAZE_N - 1 and 1 <= by + dy <= 2 * MAZE_M - 1 and (bx + dx, by + dy) not in seen]
        if not nbrs:
            stack.pop()
            continue
        nx, ny = rnd.choice(nbrs)
        carve_block((bx + nx) // 2, (by + ny) // 2)
        carve_block(nx, ny)
        seen.add((nx, ny))
        stack.append((nx, ny))
    walls = [(bx, by) for bx in range(1, 2 * MAZE_N) for by in range(1, 2 * MAZE_M) if (bx + by) % 2 == 1]
    rnd.shuffle(walls)
    for bx, by in walls[:6]:
        carve_block(bx, by)
    # the ways out (bottom: to the city; top: the snowy path)
    carve_block(WOODS_BOTTOM_BX, 2 * MAZE_M)
    carve_block(WOODS_TOP_BX, 0)
    for y in range(0, 3):
        for x in range(3 * WOODS_TOP_BX, 3 * WOODS_TOP_BX + 3):
            rows[y][x] = "g"
    return ["".join(r) for r in rows]


def woods():
    import build_exotic_safari as B
    B.clone_tileset("gTileset_General_Frlg", "KalosWoodsGeneral", woods_color)
    # The treetops: a tree's crown pokes up into the row above it (Viridian Forest's 0x281: a tip
    # over grass); draw leaves it out (its forests are deep), so walls standing in a clearing had
    # flat tops. Tips over each clearing ground, the ground's own attributes kept
    import tile_import as T
    tips = {}

    def edit(out):
        tgt = T.Target(out)
        prim = tileset_dir("gTileset_General_Frlg")
        for m in (0x00D, 0x008, 0x009, 0x010, 0x011):
            tips[m] = tgt.add_metatile(list(tgt.entries(m, prim)[:4]) + [0, 0, 0x8284, 0x8684], tgt.attr(m, prim))
        tgt.save()
    B.clone_tileset("gTileset_ViridianForest", "KalosWoods", woods_color, edit=edit)
    g = draw(woods_rows())
    for y in range(WOODS_H - 1):
        for x in range(WOODS_W):
            if g[y + 1][x] & 0x3FF == 0x289 and g[y][x] & 0x3FF in tips:
                g[y][x] = (g[y][x] & ~0x3FF) | tips[g[y][x] & 0x3FF]
    return g, WOODS_W, WOODS_H


# ---------------------------------------------------------------------------------------------
# Mamoswine Road: Route 9 under deep snow (copies of its tilesets, greens to snow; its meadows
# all deep snow - tall grass)
# ---------------------------------------------------------------------------------------------
def snow_color(i, c):
    import galar
    h, s, v = hsv(c[:3])
    deg = h * 360
    if 30 <= deg <= 62 and v > 0.62 and s < 0.6:   # the dirt road: trodden snow, faintly blue
        t = min(1.0, (v - 0.62) / 0.3)
        lo, hi = (168, 184, 216), (240, 244, 255)
        return tuple(int(lo[k] + (hi[k] - lo[k]) * t) for k in range(3))
    if (deg < 40 or deg > 330) and s > 0.15:        # the red-brown rock: frosted slate
        return rgb(222 / 360, min(1, s * 0.45), min(1, v * 0.9 + 0.08))
    return galar.snowify(i, c)


def snow_path():
    import build_exotic_safari as B
    B.clone_tileset("gTileset_General_Frlg", "KalosSnowGeneral", snow_color)
    B.clone_tileset("gTileset_CeruleanCity", "KalosSnowRoute", snow_color)
    g, lay = read_layout("LAYOUT_ROUTE9")
    g = [row[:] for row in g]
    for y, row in enumerate(g):
        for x, v in enumerate(row):
            if v & 0x3FF in (0x008, 0x009, 0x010, 0x011, 0x001) and not (v >> 10) & 3:
                row[x] = TALL
    return g, lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Glittering Cave: Mt. Moon 1F, its rock a cold, glinting blue
# ---------------------------------------------------------------------------------------------
def cave_color(i, c):
    h, s, v = hsv(c)
    deg = h * 360
    if deg < 70 or deg > 300:                       # browns, tans: slate blue, a little violet
        return rgb((228 + 20 * (1 - v)) / 360, min(1, s * 0.6 + 0.12), min(1, v * 0.85 + 0.05))
    return c[:3]


def cave():
    import build_exotic_safari as B
    B.clone_tileset("gTileset_Cave_Frlg", "KalosCave", cave_color)
    g, lay = read_layout("LAYOUT_MT_MOON_1F")
    return [row[:] for row in g], lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Lumiose Badlands: Route 113 (Hoenn's, its ash fall) as dusty, sun-baked badlands
# ---------------------------------------------------------------------------------------------
BADLANDS_X0, BADLANDS_X1 = 14, 74


def badlands_color(r, g, b):
    h, s, v = hsv((r, g, b))
    deg = h * 360
    if s < 0.16:                                    # ash greys -> dust: tan to rust
        return rgb(30 / 360, 0.32 + 0.1 * (1 - v), min(1, v * 0.95 + 0.04))
    if 60 <= deg <= 180:                            # green -> dry olive
        return rgb(52 / 360, s * 0.7, v * 0.85)
    if 15 <= deg < 60:                              # rock: redder
        return rgb((deg - 8) / 360, min(1, s * 1.1), v)
    return (r, g, b)


def desert():
    g, lay = read_layout("LAYOUT_ROUTE113")
    return [row[BADLANDS_X0:BADLANDS_X1] for row in g], BADLANDS_X1 - BADLANDS_X0, lay["height"]


# ---------------------------------------------------------------------------------------------
# Azure Bay: the Water Labyrinth (Five Island's), its rocks and islet
# ---------------------------------------------------------------------------------------------
def ocean():
    g, lay = read_layout("LAYOUT_FIVE_ISLAND_WATER_LABYRINTH")
    g = [row[:] for row in g]
    for y in (8, 9):                                # a gap in the islet's ring of rocks, by the way in
        for x in (2, 3):
            g[y][x] = SEA_BLOCK
    return g, lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# The areas
# ---------------------------------------------------------------------------------------------
SEA_BLOCK = 0x112B
WOODS_BOTTOM = [(x, WOODS_H - 1) for x in range(3 * WOODS_BOTTOM_BX, 3 * WOODS_BOTTOM_BX + 3)]
WOODS_TOP = [(x, 0) for x in range(3 * WOODS_TOP_BX, 3 * WOODS_TOP_BX + 3)]
AREAS = {
    "CITY": {
        "map": "ExoticSafari_KalosCity", "build": city, "tilesets": ("gTileset_General_Frlg", "gTileset_KalosLumiose"),
        "music": "MUS_RG_CELADON", "weather": "WEATHER_NONE", "type": "MAP_TYPE_CITY",
        "links": {
            "HALL": ("warp", [(cx(34), cy(46)), (cx(35), cy(46))], (cx(34), cy(45)), ("HALL", None)),              # (the south gate)
            "WOODS": ("warp", [(cx(34), 5), (cx(35), 5)], (cx(34), 6), ("WOODS", "CITY")),            # (the north gate)
            "CAVE": ("warp", [(8, cy(27))], (9, cy(27)), ("CAVE", "CITY")),                         # (the west gate)
            "DESERT": ("warp", [(cx(58), cy(27))], (cx(57), cy(27)), ("DESERT", "CITY")),                   # (the east gate)
        },
        # (the buildings' doors stay, leading nowhere: the city's homes are private)
        "land": land("ESPURR", "FLABEBE", "FLABEBE_YELLOW", "FURFROU", "SPRITZEE", "SWIRLIX", "FLABEBE_ORANGE", "LITLEO",
                     "FLABEBE_BLUE", "FLABEBE_WHITE", ("SCATTERBUG_ELEGANT", 48, 50), ("VIVILLON_MODERN", 50, 54)),
        "statics": [("ZYGARDE", (TOWER[0] + 3, TOWER[1] + TOWER[3]))],
        "items": [("SACHET", (cx(17), cy(33))), ("WHIPPED_DREAM", (cx(50), cy(33)))],
    },
    "WOODS": {
        "map": "ExoticSafari_KalosWoods", "build": woods, "border_size": (3, 2),
        "tilesets": ("gTileset_KalosWoodsGeneral", "gTileset_KalosWoods"),
        "music": "MUS_RG_VIRIDIAN_FOREST", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "CITY": ("edge", WOODS_BOTTOM, (WOODS_BOTTOM[1][0], WOODS_H - 2), ("CITY", "WOODS")),
            "SNOW": ("edge", WOODS_TOP, (WOODS_TOP[1][0], 1), ("SNOW", "WOODS")),
        },
        "land": land("PHANTUMP", "PUMPKABOO_AVERAGE", "PUMPKABOO_SMALL", "PUMPKABOO_LARGE", "HONEDGE", "SPEWPA_GARDEN",
                     "TREVENANT", "DOUBLADE", "PUMPKABOO_SUPER", "VIVILLON_JUNGLE", ("CHESPIN", 48, 50), ("GOURGEIST_SUPER", 52, 55)),
        "statics": [],
        "items": [],
    },
    "SNOW": {
        "map": "ExoticSafari_KalosSnowyPath", "build": snow_path, "tilesets": ("gTileset_KalosSnowGeneral", "gTileset_KalosSnowRoute"),
        "music": "MUS_RG_ROUTE24", "weather": "WEATHER_SNOW", "type": "MAP_TYPE_ROUTE",
        "links": {
            "WOODS": ("edge", [(0, y) for y in range(20)], (1, 9), ("WOODS", "SNOW")),
        },
        "land": land("BERGMITE", "SNOVER", "PILOSWINE", "CRYOGONAL", "SCATTERBUG_POLAR", "VIVILLON_TUNDRA", "MAMOSWINE",
                     "ABOMASNOW", "AVALUGG", "VIVILLON_ICY_SNOW", ("AMAURA", 48, 50), ("AURORUS", 52, 55)),
        "statics": [],
        "items": [("SHINY_STONE", (63, 2))],
    },
    "CAVE": {
        "map": "ExoticSafari_KalosCave", "build": cave, "tilesets": ("gTileset_General_Frlg", "gTileset_KalosCave"),
        "music": "MUS_RG_MT_MOON", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "CITY": ("warp", [(18, 37)], (18, 36), ("CITY", "CAVE")),
        },
        "remove_holes": [(5, 6), (19, 14), (31, 16)],
        "land": land("CARBINK", "NOIBAT", "BINACLE", "GOOMY", "INKAY", "HAWLUCHA", "BARBARACLE", "SLIGGOO",
                     "NOIVERN", "MALAMAR", ("TYRUNT", 48, 50), ("AMAURA", 48, 50)),
        "statics": [("HOOPA", HOOPA_SPOT)],
        "items": [],
    },
    "DESERT": {
        "map": "ExoticSafari_KalosBadlands", "build": desert, "tilesets": ("gTileset_General", "gTileset_Fallarbor"),
        "version": "emerald", "convert": ("KalosBadlands", "kalos_badlands_frlg"),
        "recolor": {p: badlands_color for p in range(16)},
        "music": "MUS_RG_ROUTE3", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "CITY": ("edge", [(0, y) for y in range(20)], (1, 10), ("CITY", "DESERT")),
            "OCEAN": ("edge", [(BADLANDS_X1 - BADLANDS_X0 - 1, y) for y in range(20)], (BADLANDS_X1 - BADLANDS_X0 - 2, 11),
                      ("OCEAN", "DESERT")),
        },
        "land": land("HELIOPTILE", "PANCHAM", "SKIDDO", "LITLEO", "FLETCHINDER", "DEDENNE", "HELIOLISK", "PANGORO",
                     "GOGOAT", "PYROAR", ("FENNEKIN", 48, 50), ("VIVILLON_SANDSTORM", 50, 54)),
        "statics": [("YVELTAL", (40, 4))],
        "items": [],
    },
    "OCEAN": {
        "map": "ExoticSafari_KalosOcean", "build": ocean, "tilesets": ("gTileset_General_Frlg", "gTileset_SeviiIslands45"),
        "border": SEA_BLOCK, "music": "MUS_RG_SEVII_ROUTE", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "DESERT": ("edge", [(0, y) for y in range(20)], (1, 10), ("DESERT", "OCEAN")),
        },
        "land": land("FLETCHLING", "WINGULL", "BINACLE", "SKRELP", "CLAUNCHER", "VIVILLON_MARINE", "PELIPPER", "SCATTERBUG_OCEAN",
                     "VIVILLON_ARCHIPELAGO", "HAWLUCHA", ("FROAKIE", 48, 50), ("VIVILLON_OCEAN", 50, 54)),
        "water": [("CLAUNCHER", 48, 52), ("SKRELP", 48, 52), ("TENTACOOL", 48, 52), ("CLAWITZER", 50, 54), ("DRAGALGE", 52, 55)],
        "fishing": [("LUVDISC", 45, 50), ("CLAUNCHER", 45, 50),
                    ("CLAUNCHER", 48, 52), ("SKRELP", 48, 52), ("LUVDISC", 48, 52),
                    ("SKRELP", 50, 54), ("CLAUNCHER", 50, 54), ("CLAWITZER", 50, 54), ("DRAGALGE", 52, 55), ("FROAKIE", 48, 50)],
        "statics": [("VOLCANION", (12, 9))],
        "items": [],
    },
}
ROAMERS = []
UNIQUE_ITEMS = set()

# ---------------------------------------------------------------------------------------------
# The puzzles
# ---------------------------------------------------------------------------------------------
CONDITIONS = {
    "ZYGARDE": ("CELLS", 3),          # (three of its Cells found: its 10% Forme; all: whole)
    "HOOPA": ("BOTTLE", 0),           # (the Prison Bottle taken from its pedestal)
    "VOLCANION": ("STEAM", 0),        # (a Fire- or Water-type lead, the other type behind it)
    "YVELTAL": ("DRAIN", 10),         # (ten Kalos POKéMON caught)
}
PRE_BATTLE = {"ZYGARDE": "ExoticSafari_EventScript_ZygardePre"}
# Zygarde's Cells, about the zone (flags from FLAG_EXOTIC_CELL_FIRST): how many in each area, put
# on out-of-the-way ground the player can reach (picked: far from the way in, never in a corridor)
CELLS = {"CITY": 1, "WOODS": 2, "SNOW": 1, "CAVE": 2, "DESERT": 1, "OCEAN": 1}


def _pick_spots(ctx, n, seed, taken):
    import build_exotic_safari as B
    reach, g = ctx["reach"], ctx["g"]
    starts = [(o["x"], o["y"]) for o in ctx.get("warps", [])]
    def land(p):
        return p in reach and not (g[p[1]][p[0]] >> 10) & 3 and ctx["beh"](g[p[1]][p[0]]) not in B.MB_SURFABLE
    cands = [p for p in sorted(reach) if land(p) and p not in taken
             and sum(land((p[0] + dx, p[1] + dy)) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))) >= 3
             and all(abs(p[0] - q[0]) + abs(p[1] - q[1]) > 3 for q in taken)]
    rnd = random.Random(seed)
    out = []
    for _ in range(n):
        far = sorted(cands, key=lambda p: -min([abs(p[0] - q[0]) + abs(p[1] - q[1]) for q in out + starts] or [0]))
        p = rnd.choice(far[:max(1, len(far) // 4)])
        out.append(p)
        cands = [q for q in cands if abs(q[0] - p[0]) + abs(q[1] - p[1]) > 6]
    return out


def extra(area):
    def add(ctx):
        name = ctx["name"]
        taken = {(o["x"], o["y"]) for o in ctx["objects"]} | {PEDESTAL, (PEDESTAL[0], PEDESTAL[1] + 1), GROOMER}
        first = sum(CELLS[k] for k in list(CELLS)[:list(CELLS).index(area)])
        for k, (x, y) in enumerate(_pick_spots(ctx, CELLS[area], "cell " + area, taken)):
            label = f"{name}_EventScript_Cell{k}"
            o = ctx["obj"]("OBJ_EVENT_GFX_ZYGARDE_CELL", x, y, label, "MOVEMENT_TYPE_FACE_DOWN", f"0x{0x344 + first + k:03X}")
            o["elevation"] = ctx["elev"](x, y)
            ctx["objects"].append(o)
            ctx["scripts"] += [f"{label}::", "\tlock", f"\tsetvar VAR_0x8004, {first + k}", "\tgoto ExoticSafari_EventScript_Cell", ""]
        if area == "CAVE":
            x, y = PEDESTAL
            if (x, y + 1) not in ctx["reach"]:
                print(f"WARNING CAVE: the Prison Bottle's pedestal at {(x, y)} can't be reached")
            o = ctx["obj"]("OBJ_EVENT_GFX_PRISON_BOTTLE", x, y, "ExoticSafari_EventScript_PrisonBottle", "MOVEMENT_TYPE_FACE_DOWN",
                           "FLAG_EXOTIC_PRISON_BOTTLE")
            o["elevation"] = ctx["elev"](x, y)
            ctx["objects"].append(o)
        if area == "CITY":
            x, y = GROOMER
            if (x, y) not in ctx["reach"]:
                print(f"WARNING CITY: the groomer at {(x, y)} can't be reached")
            o = ctx["obj"]("OBJ_EVENT_GFX_WOMAN_2_FRLG", x, y, "ExoticSafari_EventScript_Groomer", "MOVEMENT_TYPE_FACE_DOWN")
            o["elevation"] = ctx["elev"](x, y)
            ctx["objects"].append(o)
    return add


for _k in AREAS:
    AREAS[_k]["extra"] = extra(_k)


def header_lines(map_id):
    return ["// Kalos: Zygarde's Cells", f"#define EXOTIC_SAFARI_CELLS {sum(CELLS.values())}"]
