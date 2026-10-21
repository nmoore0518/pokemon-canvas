"""The Exotic Pokémon Safari's Paldea zone (Pokémon Canvas), for build_exotic_safari.py.

Seven areas from Nick's planner (exotic-safari-planner-paldea.xlsx), linked like this:

    Hall door -> MEADOW (the hub) -> west FOREST -> south DESERT -> south CRATER (2 floors)
                                  -> east LAKE -> north SNOWY MOUNTAIN
                                  -> north ACADEMY

Meadow, Forest and Lake are drawn here (Viridian Forest trees, FireRed's ponds); the others are
copies of existing maps: the snowy mountain is HnS's Mt. Silver summit (made longer), the desert
Hoenn's Route 111 desert, the crater Cerulean Cave 1F and B1F, the academy the Pokémon Mansion 1F.

Each area: its layout, how it links to the others (walking off an edge, a door, a ladder), its wild
Pokémon, its legendaries (statics; three more roam: src/exotic_safari.c) and hidden items (the
items some Paldea Pokémon need to evolve; once found, the hall's kiosk sells them).
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HNS = Path("/home/claude/ext/pokehns-expansion")


def blk(m, collision, elevation=3):
    return m | (collision << 10) | (elevation << 12)


def read_layout(layout_id, root=ROOT):
    layouts = {l["id"]: l for l in json.loads((root / "data/layouts/layouts.json").read_text())["layouts"] if "id" in l}
    l = layouts[layout_id]
    data = (root / l["blockdata_filepath"]).read_bytes()
    w, h = l["width"], l["height"]
    grid = [[struct.unpack_from("<H", data, (y * w + x) * 2)[0] for x in range(w)] for y in range(h)]
    return grid, l


# ---------------------------------------------------------------------------------------------
# Drawn areas: Viridian Forest's trees (3x2 blocks) on FireRed's general tileset
# ---------------------------------------------------------------------------------------------
def forest(x, y):
    return (0x290, 0x291, 0x292)[x % 3] if y % 2 == 0 else (0x298, 0x299, 0x29A)[x % 3]


def ground(x, y):
    return (0x008, 0x009)[x % 2] if y % 2 == 0 else (0x010, 0x011)[x % 2]


_center, _ = read_layout("LAYOUT_SAFARI_ZONE_CENTER")
POND = {   # pond pieces (with their elevations) from the Safari Zone's pond and its island
    "tl": _center[13][18], "t": _center[13][19], "tr": _center[13][33],
    "l": _center[14][18], "w": _center[14][19], "r": _center[14][33],
    "itl": _center[15][21], "it": _center[15][22], "itr": _center[15][30],
    "il": _center[16][21], "ir": _center[16][30],
    "ibl": _center[17][21], "ib": _center[17][22], "ibr": _center[17][30],
    "ground": _center[16][22],
}
RIM = {"l": blk(0x132, 0), "m": blk(0x133, 0), "r": blk(0x134, 0)}   # the grassy rim under a pond


def check_runs(rows):
    """The trees come in 2-row blocks: every clearing must start and end on an even row"""
    h, w = len(rows), len(rows[0])
    bad = []
    for x in range(w):
        y = 0
        while y < h:
            if rows[y][x] == "T":
                y += 1
                continue
            y0 = y
            while y < h and rows[y][x] != "T":
                y += 1
            y1 = y - 1
            if (y0 % 2 and y0 != 0) or (y1 % 2 and y1 != h - 1):
                bad.append((x, y0, y1))
    return bad


def whole_trees(rows):
    """No half trees: the trees are 3 columns wide (x % 3 == 0 the left one), so a stretch of trees
    between two clearings must start and end on a tree's edge; a one-row strip of trees between two
    clearings has no top. Such pieces become clearing (ground)."""
    g = [list(r) for r in rows]
    h, w = len(g), len(g[0])
    changed = True
    while changed:
        changed = False
        for y in range(h):
            x = 0
            while x < w:
                if g[y][x] != "T":
                    x += 1
                    continue
                x0 = x
                while x < w and g[y][x] == "T":
                    x += 1
                x1 = x - 1                                  # (a stretch of trees: x0..x1)
                lo, hi = x0, x1
                if x0 > 0:                                  # (a clearing on the left)
                    while lo <= hi and lo % 3 != 0:
                        lo += 1
                if x1 < w - 1:                              # (a clearing on the right)
                    while hi >= lo and hi % 3 != 2:
                        hi -= 1
                for k in range(x0, x1 + 1):
                    if not lo <= k <= hi:
                        g[y][k] = "."
                        changed = True
        for y in range(1, h - 1):
            for x in range(w):
                if g[y][x] == "T" and g[y - 1][x] != "T" and g[y + 1][x] != "T":
                    g[y][x] = "."
                    changed = True
    return ["".join(r) for r in g]


def draw(rows, ponds=(), exit_row=None):
    """ASCII -> layout. T trees, . ground, g tall grass, f flowers, P sand path (3 wide, its first
    row the forest's dark edge when exit_row is that row: arrow warps out)."""
    assert len({len(r) for r in rows}) == 1, [len(r) for r in rows]
    rows = whole_trees(rows)
    bad = check_runs(rows)
    assert not bad, f"clearings must start and end on even rows: {bad[:6]}"
    h, w = len(rows), len(rows[0])
    is_open = lambda x, y: 0 <= y < h and 0 <= x < w and rows[y][x] != "T"
    g = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            c = rows[y][x]
            if c == "T":
                if is_open(x, y + 1) or (is_open(x, y + 2) and y % 2 == 0):
                    t = (0x2A3, 0x2A4, 0x2A5)[x % 3] if is_open(x, y + 1) else (0x2A0, 0x2A1, 0x2A2)[x % 3]
                    g[y][x] = blk(t, 1)
                elif is_open(x, y - 1) and y % 2 == 1:
                    g[y][x] = blk((0x288, 0x289, 0x28A)[x % 3], 1)
                else:
                    g[y][x] = blk(forest(x, y), 1)
            elif c == "g":
                g[y][x] = blk(0x00D, 0)
            elif c == "f":
                g[y][x] = blk(0x004, 0)
            elif c == "P":
                k = 0 if x == 0 or rows[y][x - 1] != "P" else (2 if x + 1 >= w or rows[y][x + 1] != "P" else 1)
                g[y][x] = blk((0x2A8, 0x2A9, 0x2AA)[k] if y == exit_row else (0x0DB, 0x0DC, 0x0DD)[k], 0)
            else:
                g[y][x] = blk(ground(x, y), 0)
    for p in ponds:
        x0, y0, x1, y1 = p[:4]
        island = p[4] if len(p) > 4 else None
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if y == y0:
                    k = "tl" if x == x0 else "tr" if x == x1 else "t"
                else:
                    k = "l" if x == x0 else "r" if x == x1 else "w"
                g[y][x] = POND[k]
        for x in range(x0, x1 + 1):
            g[y1 + 1][x] = RIM["l" if x == x0 else "r" if x == x1 else "m"]
        if island:
            ix0, iy0, ix1, iy1 = island
            for y in range(iy0, iy1 + 1):
                for x in range(ix0, ix1 + 1):
                    if y == iy0:
                        k = "itl" if x == ix0 else "itr" if x == ix1 else "it"
                    elif y == iy1:
                        k = "ibl" if x == ix0 else "ibr" if x == ix1 else "ib"
                    else:
                        k = "il" if x == ix0 else "ir" if x == ix1 else "ground"
                    g[y][x] = POND[k]
    return g


def framed(inner, w, h, top=4, bottom=5, openings=()):
    """A clearing framed by forest (3 columns each side, `top` rows above, `bottom` below).
    `inner`: the clearing's rows (w - 6 wide). `openings`: (x0, y0, x1, y1) rects made '.'"""
    rows = [["T"] * w for _ in range(h)]
    for i, r in enumerate(inner):
        assert len(r) == w - 6, (i, len(r), r)
        for k, c in enumerate(r):
            rows[top + i][3 + k] = c
    for x0, y0, x1, y1, *c in openings:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                rows[y][x] = c[0] if c else "."
    return ["".join(r) for r in rows]


# ---- Meadow (the hub; South Province) 30x24 ------------------------------------------------
MEADOW_INNER = [
    "..gggg....ff.......gggg.",
    ".gggggg..ffff....gggggg.",
    ".gggggg..ffff....gggggg.",
    "..gggg.....ff.....gggg..",
    "........................",
    "...ggggg........ggggg...",
    "..ggggggg..ff..ggggggg..",
    "..ggggggg.ffff.ggggggg..",
    "...ggggg...ff...ggggg...",
    "........................",
    "........gggggg....ffff..",
    "........gggggggg..ffff..",
    "........gggggggg........",
    ".ff.....gggggggg........",
    "fff......gggggg.........",
]


def meadow():
    rows = framed(MEADOW_INNER, 30, 24, openings=[
        (0, 10, 2, 12), (27, 10, 29, 12),        # west (forest), east (lake)
        (15, 0, 17, 3),                          # north (academy)
        (12, 19, 14, 19), (12, 20, 14, 23, "P"),  # south (the hall)
    ])
    g = draw(rows, ponds=[(4, 14, 8, 15)], exit_row=20)
    return g, 30, 24


# ---- Forest (Tagtree Thicket) 33x28 ----------------------------------------------------------
def forest_area():
    W, H = 33, 28
    rows = [["T"] * W for _ in range(H)]

    def carve(x0, y0, x1, y1, c="."):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                rows[y][x] = c
    # Clearings and winding paths (rows: start even, end even)
    carve(3, 4, 11, 8, "g"); carve(5, 6, 8, 6, ".")           # the shrine glade (top left)
    carve(12, 4, 29, 6, "g"); carve(15, 4, 17, 6, ".")
    carve(27, 4, 29, 18, "g")                                 # east corridor
    carve(27, 10, 32, 12, ".")                                # -> east exit (meadow)
    carve(6, 8, 8, 20, "g")                                   # west corridor
    carve(9, 12, 26, 14, "g"); carve(15, 12, 20, 14, ".")
    carve(15, 14, 17, 22, "g")
    carve(3, 18, 14, 20, "g"); carve(18, 18, 29, 20, "g")
    carve(21, 20, 23, 24, "g")
    carve(12, 22, 26, 24, "g"); carve(15, 22, 17, 24, ".")
    carve(15, 24, 17, 27, "P")                                # -> south exit (desert)
    rows = ["".join(r) for r in rows]
    return draw(rows), W, H


# ---- Lake (Casseroya Lake) 45x36 ------------------------------------------------------------
LAKE_INNER = [
    "ggg.......gggg..ggggg.........ggg......",
    "gggg......gggg..ggggg........gggg......",
    "gg.........gg...........................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "gggg.....gggggg.........gggggg.....gggg.",
    "ggggg...gggggggg.......gggggggg...ggggg.",
    "ggggg...gggggggg.......gggggggg...ggggg.",
    "gggg.....gggggg.........gggggg.....gggg.",
]


def lake():
    W, H = 45, 34
    inner = [r[:W - 6].ljust(W - 6, ".") for r in LAKE_INNER[:H - 9]]
    rows = framed(inner, W, H, openings=[
        (0, 16, 2, 18),          # west (meadow)
        (21, 0, 23, 3),          # north (snowy mountain)
    ])
    # The lake, with Chi-Yu's island
    g = draw(rows, ponds=[(6, 8, 38, 22, (17, 12, 27, 18))])
    return g, W, H


# ---------------------------------------------------------------------------------------------
# Copied areas
# ---------------------------------------------------------------------------------------------
SNOW_PAD = 8          # (columns added each side: the mountain's slopes carry on to the snowfield)


def snowy_mountain():
    """HnS's Mt. Silver summit, with a snowfield added below it. The summit's slopes run off its
    map's sides: widened here (SNOW_PAD columns each side), the slopes carried on diagonally until
    they meet the snow; the sky above becomes snow-covered peaks (solid)"""
    import struct as _s
    data = (HNS / "data/layouts/MtSilver_SummitDay_hns/map.bin").read_bytes()
    w, h = 21, 27
    g = [[_s.unpack_from("<H", data, (y * w + x) * 2)[0] for x in range(w)] for y in range(h)]
    snow = g[24][0]
    for y in (22, 23):                          # (the path up to the stairs was blocked)
        for x in range(9, 12):
            g[y][x] = snow
    g += [[snow] * w for _ in range(8)]
    P = SNOW_PAD
    W = w + 2 * P
    out = [[snow] * W for _ in range(len(g))]
    sky = 0x38C
    for y in range(len(g)):
        for X in range(W):
            x = X - P
            if 0 <= x < w:
                v = g[y][x]
            elif y > 21:
                v = snow
            else:                               # (follow the slope up-and-in to the map)
                k = -x if x < 0 else x - (w - 1)
                sx, sy = (x + k if x < 0 else x - k), y - k
                v = g[sy][sx] if sy >= 0 else g[0][0]
                if y == 21:
                    v = g[21][0]
            if v & 0x3FF == sky:
                v = snow | (1 << 10)
            elif v & 0x3FF in PEAK_ON_SNOW:
                v = (v & ~0x3FF) | PEAK_ON_SNOW[v & 0x3FF]
            out[y][X] = v
    snow_peaks(snow & 0x3FF)
    return out, W, len(out)


PEAK_ON_SNOW = {0x358: 0x350, 0x359: 0x351}


def snow_peaks(snow):
    """The slopes' top corners (0x358/0x359) are drawn over sky: copies of them over snow, in two
    of the snow tileset's empty metatile slots (0x350/0x351)"""
    import struct as _s
    f = ROOT / "data/tilesets/secondary/mt_silver_snow_hns/metatiles.bin"
    mt = bytearray(f.read_bytes())
    ground = _s.unpack_from("<4H", mt, (snow - 0x280) * 16)
    for src, dst in PEAK_ON_SNOW.items():
        top = _s.unpack_from("<4H", mt, (src - 0x280) * 16 + 8)
        _s.pack_into("<8H", mt, (dst - 0x280) * 16, *ground, *top)
    at = bytearray((f.parent / "metatile_attributes.bin").read_bytes())
    asz = len(at) // (len(mt) // 16)
    for src, dst in PEAK_ON_SNOW.items():
        at[(dst - 0x280) * asz:(dst - 0x279) * asz] = at[(src - 0x280) * asz:(src - 0x279) * asz]
    f.write_bytes(mt)
    (f.parent / "metatile_attributes.bin").write_bytes(at)


def desert():
    """Route 111's desert (Hoenn), rows 37-90 x 11-38: its way in from the north at the top, the
    Desert Ruins' door at the bottom (down to the crater)"""
    src, lay = read_layout("LAYOUT_ROUTE111_NO_MIRAGE_TOWER")
    g = [row[11:39] for row in src[37:91]]
    # The crop's edges cut through things: the cliffs' feet along the top (rock instead), the little
    # trees in the bottom-left corner (grass) and the boulders' tops along the bottom (sand)
    for x in range(12):
        g[0][x] = 0x071 | (1 << 10)            # (Emerald's metatiles: converted later)
    for x in range(1, 5):
        g[53][x] = 0x001
    for x in range(28):
        if g[53][x] & 0x3FF in (0x0A3, 0x0A4):
            g[53][x] = (g[53][x] & ~0x3FF & ~(3 << 10)) | 0x121
    return g, 28, 54


def crater(floor):
    """Cerulean Cave 1F (upper crater) or B1F (the depths)"""
    lid = "LAYOUT_CERULEAN_CAVE_1F" if floor == 1 else "LAYOUT_CERULEAN_CAVE_B1F"
    g, lay = read_layout(lid)
    return [row[:] for row in g], lay["width"], lay["height"]


def academy():
    g, lay = read_layout("LAYOUT_POKEMON_MANSION_1F")
    return [row[:] for row in g], lay["width"], lay["height"]


def snow_cliffs(g, w, h):
    """The cliffs either side of the path up to the summit can't be walked on"""
    for y in range(0, 21):
        for x in range(w):
            if not (9 + SNOW_PAD <= x <= 11 + SNOW_PAD and y >= 5):
                g[y][x] |= 1 << 10


# ---------------------------------------------------------------------------------------------
# The areas: maps, links, Pokémon
# ---------------------------------------------------------------------------------------------
REGION = "PALDEA"
HUB = "MEADOW"            # (the hall's door leads here)
LV = (48, 52)


def land(*mons):
    """12 grass/cave slots: (species, min, max) or species (default levels)"""
    out = [(m, *LV) if isinstance(m, str) else m for m in mons]
    assert len(out) == 12, len(out)
    return out


AREAS = {
    "MEADOW": {
        "map": "ExoticSafari_PaldeaMeadow", "build": meadow, "border_size": (3, 2), "tilesets": ("gTileset_General_Frlg", "gTileset_ViridianForest"),
        "music": "MUS_RG_ROUTE24", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        # links: key -> (kind, cells, arrival, (area, key)). kind: "edge" (walk off: a trigger),
        # "warp" (the cells are warp tiles: doors, arrows, ladders)
        "links": {
            "HALL": ("warp", [(12, 20), (13, 20), (14, 20)], (13, 19), ("HALL", None)),
            "FOREST": ("edge", [(0, 10), (0, 11), (0, 12)], (1, 11), ("FOREST", "MEADOW")),
            "LAKE": ("edge", [(29, 10), (29, 11), (29, 12)], (28, 11), ("LAKE", "MEADOW")),
            "ACADEMY": ("edge", [(15, 0), (16, 0), (17, 0)], (16, 1), ("ACADEMY", "MEADOW")),
        },
        "land": land("LECHONK", "FIDOUGH", "TANDEMAUS", "SQUAWKABILLY", "SQUAWKABILLY_YELLOW", "PAWMI", "FLITTLE",
                     "TAUROS_PALDEA_COMBAT", "WOOPER_PALDEA", "DUDUNSPARCE", ("SPRIGATITO", 48, 50), ("FARIGIRAF", 50, 54)),
        "statics": [("OGERPON", (22, 6))],
        "items": [],
    },
    "FOREST": {
        "map": "ExoticSafari_PaldeaForest", "build": forest_area, "border_size": (3, 2), "tilesets": ("gTileset_General_Frlg", "gTileset_ViridianForest"),
        "music": "MUS_RG_VIRIDIAN_FOREST", "weather": "WEATHER_SHADE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "MEADOW": ("edge", [(32, 10), (32, 11), (32, 12)], (31, 11), ("MEADOW", "FOREST")),
            "DESERT": ("edge", [(15, 27), (16, 27), (17, 27)], (16, 26), ("DESERT", "FOREST")),
        },
        "land": land("TAROUNTULA", "NYMBLE", "SMOLIV", "RELLOR", "BRAMBLIN", "TOEDSCOOL", "SHROODLE", "CAPSAKID",
                     "TINKATINK", ("URSALUNA_BLOODMOON", 52, 55), ("SPRIGATITO", 48, 50), ("DIPPLIN", 50, 54)),
        "statics": [("WO_CHIEN", (6, 6))],
        "items": [],
    },
    "DESERT": {
        "map": "ExoticSafari_PaldeaDesert", "build": desert, "tilesets": ("gTileset_General", "gTileset_Mauville"),
        "version": "emerald", "convert": ("ExoticDesert", "exotic_desert_frlg"), "music": "MUS_DESERT", "weather": "WEATHER_SANDSTORM", "type": "MAP_TYPE_ROUTE",
        "links": {
            "FOREST": ("edge", "top", None, ("FOREST", "DESERT")),
            "CRATER": ("warp", [(18, 50)], (18, 51), ("CRATER", "DESERT")),   # the ruins' door
        },
        "land": land("NACLI", "BRAMBLIN", "CYCLIZAR", "ORTHWORM", "KLAWF", "RELLOR", "VAROOM", "MASCHIFF",
                     "CHARCADET", "TAUROS_PALDEA_BLAZE", ("FUECOCO", 48, 50), ("ARCHALUDON", 52, 55)),
        "statics": [("TING_LU", (2, 40))],
        "items": [("AUSPICIOUS_ARMOR", (1, 1)), ("HEARTHFLAME_MASK", (24, 22))],
    },
    "CRATER": {
        "map": "ExoticSafari_PaldeaCrater", "build": lambda: crater(1), "tilesets": ("gTileset_General_Frlg", "gTileset_CeruleanCave"),
        "music": "MUS_RG_ROCKET_HIDEOUT", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "DESERT": ("warp", [(33, 21)], (33, 20), ("DESERT", "CRATER")),
            "DEPTHS": ("warp", [(1, 7)], (2, 7), ("DEPTHS", "CRATER")),
        },
        "remove_warps": [(34, 2), (10, 2), (5, 15), (30, 10), (24, 11), (2, 4)],
        "land": land("GREAT_TUSK", "SCREAM_TAIL", "BRUTE_BONNET", "FLUTTER_MANE", "SLITHER_WING", "SANDY_SHOCKS",
                     ("ROARING_MOON", 52, 55), "GLIMMET", ("WALKING_WAKE", 52, 55), ("GOUGING_FIRE", 52, 55),
                     ("RAGING_BOLT", 55, 58), ("GLIMMORA", 52, 55)),
        "water": [("WIGLETT", 48, 52), ("TATSUGIRI_DROOPY", 48, 52), ("VELUZA", 48, 52), ("DONDOZO", 50, 54), ("TATSUGIRI_STRETCHY", 50, 54)],
        "statics": [],
        "items": [("MALICIOUS_ARMOR", (7, 3)), ("CORNERSTONE_MASK", (36, 2))],
    },
    "DEPTHS": {
        "map": "ExoticSafari_PaldeaCraterDepths", "build": lambda: crater(2), "tilesets": ("gTileset_General_Frlg", "gTileset_CeruleanCave"),
        "music": "MUS_RG_ROCKET_HIDEOUT", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "CRATER": ("warp", [(5, 7)], (5, 8), ("CRATER", "DEPTHS")),
        },
        "remove_warps": [],
        "land": land("IRON_TREADS", "IRON_BUNDLE", "IRON_HANDS", "IRON_JUGULIS", "IRON_MOTH", "IRON_THORNS",
                     ("IRON_VALIANT", 52, 55), "GLIMMET", ("IRON_LEAVES", 52, 55), ("IRON_BOULDER", 52, 55),
                     ("IRON_CROWN", 55, 58), ("KINGAMBIT", 52, 55)),
        "water": [("WIGLETT", 48, 52), ("TATSUGIRI_CURLY", 48, 52), ("VELUZA", 48, 52), ("DONDOZO", 50, 54), ("TATSUGIRI_DROOPY", 50, 54)],
        "statics": [("TERAPAGOS", (7, 12)), ("PECHARUNT", (35, 4))],
        "items": [],
    },
    "LAKE": {
        "map": "ExoticSafari_PaldeaLake", "build": lake, "border_size": (3, 2), "tilesets": ("gTileset_General_Frlg", "gTileset_ViridianForest"),
        "music": "MUS_RG_ROUTE24", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "MEADOW": ("edge", [(0, 16), (0, 17), (0, 18)], (1, 17), ("MEADOW", "LAKE")),
            "SNOW": ("edge", [(21, 0), (22, 0), (23, 0)], (22, 1), ("SNOW", "LAKE")),
        },
        "land": land("WATTREL", "FLAMIGO", "TOEDSCOOL", "WIGLETT", "WOOPER_PALDEA", "TADBULB", "KILOWATTREL", "PAWMI",
                     "TAUROS_PALDEA_AQUA", "BOMBIRDIER", ("QUAXLY", 48, 50), ("DONDOZO", 50, 54)),
        "water": [("FINIZEN", 48, 52), ("WIGLETT", 48, 52), ("VELUZA", 48, 52), ("TATSUGIRI_CURLY", 50, 54), ("DONDOZO", 50, 54)],
        "fishing": [("FINIZEN", 45, 50), ("WIGLETT", 45, 50),                                   # Old Rod
                    ("FINIZEN", 48, 52), ("VELUZA", 48, 52), ("TATSUGIRI_STRETCHY", 48, 52),    # Good Rod
                    ("VELUZA", 50, 54), ("TATSUGIRI_DROOPY", 50, 54), ("DONDOZO", 50, 54),       # Super Rod
                    ("PALAFIN", 52, 55), ("QUAXLY", 48, 50)],
        "statics": [("CHI_YU", (22, 15))],
        "items": [("WELLSPRING_MASK", (18, 12))],
    },
    "SNOW": {
        "map": "ExoticSafari_PaldeaSnowyMountain", "build": snowy_mountain, "tilesets": ("gTileset_Johto_General_Hns", "gTileset_MtSilverSnow_Hns"),
        "music": "MUS_RG_ROUTE24", "weather": "WEATHER_SNOW", "type": "MAP_TYPE_ROUTE", "border": 0x281,
        "post": snow_cliffs, "patch_snow": True,
        "links": {
            "LAKE": ("edge", [(17, 34), (18, 34), (19, 34)], (18, 33), ("LAKE", "SNOW")),
        },
        "land": land("CETODDLE", "FRIGIBAX", "GREAVARD", "BOMBIRDIER", "CETITAN", "TINKATINK", ("ARCTIBAX", 50, 54),
                     "LOKIX", ("GARGANACL", 50, 54), "KILOWATTREL", ("BAXCALIBUR", 54, 56), ("ANNIHILAPE", 52, 55)),
        "statics": [("CHIEN_PAO", (18, 6))],
        "items": [("ICE_STONE", (10, 30))],
    },
    "ACADEMY": {
        "map": "ExoticSafari_PaldeaAcademy", "build": academy, "tilesets": ("gTileset_BuildingFrlg", "gTileset_PokemonMansion"),
        "music": "MUS_RG_POKE_MANSION", "weather": "WEATHER_NONE", "type": "MAP_TYPE_INDOOR",
        "links": {
            "MEADOW": ("warp", [(7, 33), (8, 33), (9, 33), (34, 33), (35, 34)], (8, 32), ("MEADOW", "ACADEMY")),
        },
        "remove_warps": [],
        "land": land("VAROOM", "GIMMIGHOUL_ROAMING", "POLTCHAGEIST", "TADBULB", "POLTCHAGEIST_ARTISAN", "GREAVARD",
                     "GIMMIGHOUL_CHEST", "CHARCADET", "SQUAWKABILLY_WHITE", "SQUAWKABILLY_BLUE", ("GHOLDENGO", 52, 55),
                     ("SINISTCHA", 52, 55)),
        "statics": [],
        "items": [("UNREMARKABLE_TEACUP", (22, 5)), ("MASTERPIECE_TEACUP", (35, 8))],
        "ropes": [(9, 13), (10, 14)],      # (in front of the stairs: they lead nowhere)
    },
}

# What each static legendary waits for (src/exotic_safari.c): its 4 stakes pulled (the Treasures
# of Ruin), Ogerpon's 3 masks found, the Loyal Three caught (Pecharunt), every gym type mastered
# (Terapagos: the power of every type). Until then a sign-like hint where it will appear.
CONDITIONS = {
    "WO_CHIEN": ("STAKES", 0), "CHIEN_PAO": ("STAKES", 1), "TING_LU": ("STAKES", 2), "CHI_YU": ("STAKES", 3),
    "OGERPON": ("MASKS", 0), "PECHARUNT": ("ROAMERS", 0), "TERAPAGOS": ("MASTERY", 0),
}
# The stakes: 4 per Treasure of Ruin, in the other areas (two stakes in each area)
STAKE_COLORS = ["PURPLE", "YELLOW", "GREEN", "BLUE"]     # Wo-Chien, Chien-Pao, Ting-Lu, Chi-Yu
STAKE_FLAG_BASE = 0x0EF                                  # (FLAG_EXOTIC_STAKE_FIRST)
STAKES = {
    "PURPLE": [("MEADOW", (26, 4)), ("DESERT", (26, 42)), ("LAKE", (41, 4)), ("ACADEMY", (1, 30))],
    "YELLOW": [("FOREST", (26, 24)), ("CRATER", (38, 13)), ("DEPTHS", (38, 1)), ("MEADOW", (4, 17))],
    "GREEN": [("SNOW", (28, 22)), ("LAKE", (41, 28)), ("ACADEMY", (36, 4)), ("DEPTHS", (34, 21))],
    "BLUE": [("FOREST", (3, 18)), ("DESERT", (25, 52)), ("SNOW", (8, 22)), ("CRATER", (5, 20))],
}
# Items the kiosk doesn't sell once found (one of a kind)
UNIQUE_ITEMS = {"WELLSPRING_MASK", "HEARTHFLAME_MASK", "CORNERSTONE_MASK"}

# Legendaries: statics (above) and the roamers (any area, a different one each time the player
# comes in: src/exotic_safari.c). Level 70, like the gym's.
ROAMERS = ["OKIDOGI", "MUNKIDORI", "FEZANDIPITI"]
LEGEND_LEVEL = 70
