"""The Exotic Pokémon Safari's Hoenn zone (Pokémon Canvas), for build_exotic_safari.py.

Seven areas from Nick's planner (exotic-safari-planner-Hoenn_update.xlsx):

    Hall door -> FOREST (Fortree City: the treetop city on its stilts, with Route 120's grass and
    ponds to its east; the hub)
        its pond's mound (surf to it) -> TOMB (Ancient Tomb: Registeel's chamber, steel grey)
        west -> OCEAN (Pacifidlog Town: the floating town, with Route 131's sea and Sky Pillar's
                 islet to its west)
                 -> the islet's cave -> CRATER (Sootopolis City: the city in the crater)
                 -> a cave in Pacifidlog's cliff -> ICE (Island Cave: Regice's chamber, ice blue)
        east -> VOLCANO (Mt. Chimney: the summit and its lava lake)
                -> a cave in the summit's rock -> RUINS (Desert Ruins: Regirock's chamber, sandstone)

The three Regi chambers are Emerald's own (one layout, two rooms joined by a door), each in its
own colors. Latias and Latios roam the zone.
"""
import colorsys
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paldea import read_layout, land   # noqa: E402

REGION = "HOENN"
HUB = "FOREST"
LV = (48, 52)


def hsv(c):
    return colorsys.rgb_to_hsv(*(v / 255 for v in c[:3]))


def rgb(h, s, v):
    return tuple(int(max(0, min(1, k)) * 255) for k in colorsys.hsv_to_rgb(h % 1, s, v))


# ---------------------------------------------------------------------------------------------
# Fortree City, and Route 120's north end to its east (as in Emerald: Route 120 joins Fortree's
# east side), thick forest under Fortree to match Route 120's height
# ---------------------------------------------------------------------------------------------
FOREST_W, FOREST_H = 80, 25


def forest():
    fortree, _ = read_layout("LAYOUT_FORTREE_CITY")
    route, _ = read_layout("LAYOUT_ROUTE120")
    tree = (route[0][0], route[0][1])             # (the forest's two tree halves, checkered)
    g = []
    for y in range(FOREST_H):
        left = fortree[y][:] if y < 20 else [tree[(x + y) % 2] for x in range(40)]
        g.append(left + route[y][:40])
    return g, FOREST_W, FOREST_H


# ---------------------------------------------------------------------------------------------
# Pacifidlog Town, and Route 131 to its west (Sky Pillar's islet in it, as in Emerald once Sky
# Pillar is open). A second cave (Sky Pillar's cave mouth and its beach, copied) in the cliff over
# Pacifidlog. The town's floating logs: plain floor (their sinking is an Emerald field effect)
# ---------------------------------------------------------------------------------------------
OCEAN_W, OCEAN_H = 80, 40
PAC_X = 60
ICE_DOOR = (PAC_X + 9, 8)
SAND = 0x124
SEAM_W, SEAM_SRC = 5, 28
SKY_DOOR = (36, 6)


def ocean():
    route, _ = read_layout("LAYOUT_ROUTE131_SKY_PILLAR")
    pac, _ = read_layout("LAYOUT_PACIFIDLOG_TOWN")
    g = [route[y][:] + pac[y][:] for y in range(OCEAN_H)]
    # Pacifidlog's cliff given a west end (Sky Pillar islet's own west slope), not cut off by the sea
    for y in range(10):
        for dx in range(SEAM_W):
            g[y][PAC_X - SEAM_W + dx] = route[y][SEAM_SRC + dx]
    for y in (3, 4):
        g[y][PAC_X - SEAM_W] = g[y][PAC_X - SEAM_W - 1]      # (half a sea rock: sea)
    # the cave: Sky Pillar's (its rock face and door, the beach under it) into Pacifidlog's cliff
    for dy in range(5):
        for dx in range(7):
            g[ICE_DOOR[1] - 1 + dy][ICE_DOOR[0] - 3 + dx] = route[SKY_DOOR[1] - 1 + dy][SKY_DOOR[0] - 3 + dx]
    AREAS["OCEAN"]["behaviors"] = {**pacifidlog_logs(), SAND: "MB_INDOOR_ENCOUNTER"}   # (wild Pokémon on the beaches)
    return g, OCEAN_W, OCEAN_H


def pacifidlog_logs():
    import alola
    return alola.pacifidlog_logs()


# ---------------------------------------------------------------------------------------------
# Mt. Chimney: the summit, its lava lake, the cable car's station. A cave (Jagged Pass's) in the
# rock west of the lava
# ---------------------------------------------------------------------------------------------
RUINS_DOOR = (13, 16)


def volcano():
    g, l = read_layout("LAYOUT_MT_CHIMNEY")
    jagged, _ = read_layout("LAYOUT_JAGGED_PASS")
    g = [row[:] for row in g]
    x, y = RUINS_DOOR
    for dy in (0, 1):
        for dx in (-1, 0, 1):
            g[y - 1 + dy][x + dx] = jagged[17 + dy][16 + dx]
    return g, l["width"], l["height"]


# ---------------------------------------------------------------------------------------------
# Sootopolis City: the city in the crater, its lake and lawns (wild Pokémon in the lawns)
# ---------------------------------------------------------------------------------------------
def crater():
    g, l = read_layout("LAYOUT_SOOTOPOLIS_CITY")
    AREAS["CRATER"]["behaviors"] = lawn_behaviors()
    return [row[:] for row in g], l["width"], l["height"]


def lawn_behaviors():
    """the lawns' grass (the green at the lake's south-west and south-east): wild Pokémon"""
    import build_exotic_safari as B
    from collections import Counter
    g, l = read_layout("LAYOUT_SOOTOPOLIS_CITY")
    beh = B.Behaviors((l["primary_tileset"], l["secondary_tileset"]), "emerald")
    c = Counter(g[y][x] & 0x3FF for y in range(38, 47) for x in range(8, 15)
                if not (g[y][x] >> 10) & 3 and beh(g[y][x]) == B.MB["MB_NORMAL"])
    return {c.most_common(1)[0][0]: "MB_INDOOR_ENCOUNTER"}


# ---------------------------------------------------------------------------------------------
# The Regi chambers: Emerald's (two rooms: the way in below, the Regi's room above, a door
# between), each in its own colors
# ---------------------------------------------------------------------------------------------
# The inner wall (7-9, 19-20): open in the layout; sealed on load until its puzzle is solved
# (src/exotic_safari.c, sRegiWalls). Emerald's cover and Braille-wall metatiles aren't in the
# layout, so they're put on two cells nobody can reach for the conversion, read back (as
# converted) and the cells put back
WALL_X, WALL_Y = 7, 19
EM_COVER, EM_BRAILLE = 0x229, 0x235          # (METATILE_Cave_EntranceCover, _SealedChamberBraille_Mid)
SPARE = [(2, 16), (3, 16)]
REGI_WALLS = ["RUINS", "ICE", "TOMB"]        # (FLAG_EXOTIC_REGI_WALL_FIRST + this order)
WALLS = {}                                   # area -> {"open": [6 entries], "cover", "braille"}


def chamber(layout):
    def build():
        g, l = read_layout(layout)
        g = [row[:] for row in g]
        keep = g[WALL_Y][WALL_X] & ~0x3FF                # (collision and elevation of the wall)
        for (x, y), m in zip(SPARE, (EM_COVER, EM_BRAILLE)):
            g[y][x] = m | keep
        return g, l["width"], l["height"]
    return build


def wall_read(key):
    def post(g):
        WALLS[key] = {"open": [g[WALL_Y + i // 3][WALL_X + i % 3] for i in range(6)],
                      "cover": g[SPARE[0][1]][SPARE[0][0]], "braille": g[SPARE[1][1]][SPARE[1][0]]}
        for x, y in SPARE:
            g[y][x] = g[y][1]
    return post


def wall_extra(key):
    def add(ctx):
        name, k = ctx["name"], REGI_WALLS.index(key)
        label = f"{name}_EventScript_RegiWall"
        for dx in range(3):
            ctx["bgs"].append({"type": "sign", "x": WALL_X + dx, "y": WALL_Y + 1, "elevation": 0,
                               "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH", "script": label})
        ctx["scripts"] += [f"{label}::", "\tlockall", f"\tsetvar VAR_0x8004, {k}",
                           "\tgoto ExoticSafari_EventScript_RegiWall", ""]
    return add


# The Sealed Chamber's tablet: the rock face beside the Island Cave's mouth (Pacifidlog's cliff)
TABLET = (ICE_DOOR[0] + 2, ICE_DOOR[1])


def ocean_extra(ctx):
    ctx["bgs"].append({"type": "sign", "x": TABLET[0], "y": TABLET[1], "elevation": 0,
                       "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH",
                       "script": "ExoticSafari_EventScript_SealedTablet"})


def tint(hue, sat, val, lift=0.0):
    """the cave's browns to one hue (greys stay grey, a little tinted)"""
    def f(r, g, b):
        h, s, v = hsv((r, g, b))
        return rgb(hue / 360, min(1, s * sat), min(1, v * val + lift))
    return f


STEEL = tint(212, 0.35, 0.92)            # Registeel's: cold steel grey
SANDSTONE = tint(36, 1.15, 1.06, 0.04)   # Regirock's: desert sandstone
ICE = tint(196, 0.6, 0.9, 0.22)          # Regice's: pale ice blue
REGI_SPOT = (8, 5)


def chamber_links(key, out_to):
    return {
        out_to: ("warp", [(8, 29)], (8, 28), (out_to, key)),
        "UP": ("warp", [(8, 20)], (8, 21), (key, "DOWN")),
        "DOWN": ("warp", [(8, 11)], (8, 10), (key, "UP")),
    }


# ---------------------------------------------------------------------------------------------
# The areas
# ---------------------------------------------------------------------------------------------
AREAS = {
    "FOREST": {
        "map": "ExoticSafari_HoennFortree", "build": forest, "tilesets": ("gTileset_General", "gTileset_Fortree"),
        "version": "emerald", "convert": ("HoennFortree", "hoenn_fortree"),
        "music": "MUS_FORTREE", "weather": "WEATHER_NONE", "type": "MAP_TYPE_CITY",
        "links": {
            "HALL": ("warp", [(5, 6)], (5, 7), ("HALL", None)),               # (the Pokémon Center's door)
            "OCEAN": ("edge", [(0, y) for y in range(6, 11)], (1, 8), ("OCEAN", "FOREST")),
            "VOLCANO": ("edge", [(FOREST_W - 1, 8)], (FOREST_W - 2, 8), ("VOLCANO", "FOREST")),
            "TOMB": ("warp", [(40 + 19, 23)], (40 + 19, 24), ("TOMB", "FOREST")),   # (the mound in the pond)
        },
        "land": land("SHROOMISH", "SLAKOTH", "SEEDOT", "NUZLEAF", "SWABLU", "TROPIUS", "KECLEON", "NINJASK",
                     "BRELOOM", "ALTARIA", ("TREECKO", 48, 50), ("ABSOL", 52, 55)),
        "water": [("LOTAD", 48, 52), ("SURSKIT", 48, 52), ("LOMBRE", 48, 52), ("MASQUERAIN", 50, 54),
                  ("LUDICOLO", 52, 55)],
        "fishing": [("BARBOACH", 45, 50), ("CORPHISH", 45, 50),
                    ("BARBOACH", 48, 52), ("CORPHISH", 48, 52), ("LOTAD", 48, 52),
                    ("WHISCASH", 50, 54), ("CRAWDAUNT", 50, 54), ("FEEBAS", 48, 52),
                    ("WHISCASH", 52, 55), ("MILOTIC", 52, 55)],
        "statics": [],
        "items": [],
    },
    "OCEAN": {
        "map": "ExoticSafari_HoennPacifidlog", "build": ocean, "tilesets": ("gTileset_General", "gTileset_Pacifidlog"),
        "version": "emerald", "convert": ("HoennPacifidlog", "hoenn_pacifidlog"), "border_at": (10, 20),   # (open sea past the edges)
        "music": "MUS_SAILING", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "FOREST": ("edge", [(0, y) for y in range(14, 29)], (1, 21), ("FOREST", "OCEAN")),
            "CRATER": ("warp", [SKY_DOOR], (SKY_DOOR[0], SKY_DOOR[1] + 1), ("CRATER", "OCEAN")),
            "ICE": ("warp", [ICE_DOOR], (ICE_DOOR[0], ICE_DOOR[1] + 1), ("ICE", "OCEAN")),
        },
        "land": land("WINGULL", "ZIGZAGOON", "PELIPPER", "LINOONE", "SKITTY", "PLUSLE", "MINUN", "DELCATTY",
                     "VOLBEAT", "ILLUMISE", ("MUDKIP", 48, 50), ("RELICANTH", 52, 55)),
        "water": [("WAILMER", 48, 52), ("WINGULL", 48, 52), ("PELIPPER", 48, 52), ("SPHEAL", 50, 54),
                  ("WAILORD", 52, 55)],
        "fishing": [("CARVANHA", 45, 50), ("LUVDISC", 45, 50),
                    ("CARVANHA", 48, 52), ("LUVDISC", 48, 52), ("WAILMER", 48, 52),
                    ("SHARPEDO", 50, 54), ("CLAMPERL", 50, 54), ("WAILMER", 50, 54),
                    ("SHARPEDO", 52, 55), ("RELICANTH", 52, 55)],
        "statics": [("KYOGRE", (28, 26))],
        "items": [("SOUL_DEW", (PAC_X + 5, 23))],
        "extra": ocean_extra,
    },
    "VOLCANO": {
        "map": "ExoticSafari_HoennMtChimney", "build": volcano, "tilesets": ("gTileset_General", "gTileset_Lavaridge"),
        "version": "emerald", "convert": ("HoennMtChimney", "hoenn_mt_chimney"),
        "music": "MUS_MT_CHIMNEY", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE", "indoor_encounters": True,
        "links": {
            "FOREST": ("warp", [(20, 41), (21, 41)], (20, 40), ("FOREST", "VOLCANO")),
            "RUINS": ("warp", [RUINS_DOOR], (RUINS_DOOR[0], RUINS_DOOR[1] + 1), ("RUINS", "VOLCANO")),
        },
        "land": land("NUMEL", "SPOINK", "TORKOAL", "MAKUHITA", "ARON", "MEDITITE", "CAMERUPT", "GRUMPIG",
                     "HARIYAMA", "LAIRON", ("TORCHIC", 48, 50), ("MEDICHAM", 52, 55)),
        "statics": [("DEOXYS", (20, 15))],
        "items": [("METEORITE", (31, 7))],
    },
    "CRATER": {
        "map": "ExoticSafari_HoennSootopolis", "build": crater, "tilesets": ("gTileset_General", "gTileset_Sootopolis"),
        "version": "emerald", "convert": ("HoennSootopolis", "hoenn_sootopolis"),
        "music": "MUS_SOOTOPOLIS", "weather": "WEATHER_NONE", "type": "MAP_TYPE_CITY",
        "links": {
            "OCEAN": ("warp", [(31, 16)], (31, 17), ("OCEAN", "CRATER")),     # (the Cave of Origin's door)
        },
        "land": land("RALTS", "CHIMECHO", "CASTFORM", "SPINDA", "KIRLIA", "MAWILE", "SABLEYE", "BAGON",
                     "SHELGON", "GARDEVOIR", ("BELDUM", 48, 50), ("SALAMENCE", 52, 55)),
        "water": [("WAILMER", 48, 52), ("WINGULL", 48, 52), ("SPHEAL", 48, 52), ("SEALEO", 50, 54),
                  ("WALREIN", 52, 55)],
        "fishing": [("LUVDISC", 45, 50), ("CLAMPERL", 45, 50),
                    ("LUVDISC", 48, 52), ("CLAMPERL", 48, 52), ("WAILMER", 48, 52),
                    ("HUNTAIL", 50, 54), ("GOREBYSS", 50, 54), ("SHARPEDO", 50, 54),
                    ("HUNTAIL", 52, 55), ("GOREBYSS", 52, 55)],
        "statics": [("JIRACHI", (30, 34))],
        "items": [("BLUE_ORB", (8, 43))],
    },
    "TOMB": {
        "map": "ExoticSafari_HoennAncientTomb", "build": chamber("LAYOUT_ANCIENT_TOMB"),
        "tilesets": ("gTileset_General", "gTileset_Cave"), "version": "emerald",
        "post_convert": wall_read("TOMB"), "extra": wall_extra("TOMB"),
        "convert": ("HoennTomb", "hoenn_tomb"), "recolor": {p: STEEL for p in range(16)},
        "music": "MUS_SEALED_CHAMBER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": chamber_links("TOMB", "FOREST"),
        "land": land("ARON", "NOSEPASS", "BALTOY", "DUSKULL", "SHUPPET", "LAIRON", "SABLEYE", "MAWILE",
                     "CLAYDOL", "DUSCLOPS", ("BANETTE", 50, 54), ("AGGRON", 52, 55)),
        "statics": [("REGISTEEL", REGI_SPOT)],
        "items": [],
    },
    "RUINS": {
        "map": "ExoticSafari_HoennDesertRuins", "build": chamber("LAYOUT_DESERT_RUINS"),
        "tilesets": ("gTileset_General", "gTileset_Cave"), "version": "emerald",
        "post_convert": wall_read("RUINS"), "extra": wall_extra("RUINS"),
        "convert": ("HoennRuins", "hoenn_ruins"), "recolor": {p: SANDSTONE for p in range(16)},
        "music": "MUS_SEALED_CHAMBER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": chamber_links("RUINS", "VOLCANO"),
        "land": land("TRAPINCH", "CACNEA", "BALTOY", "SOLROCK", "LUNATONE", "VIBRAVA", "CACTURNE", "LILEEP",
                     "ANORITH", "SEVIPER", ("ZANGOOSE", 50, 54), ("FLYGON", 52, 55)),
        "statics": [("REGIROCK", REGI_SPOT)],
        "items": [],
    },
    "ICE": {
        "map": "ExoticSafari_HoennIslandCave", "build": chamber("LAYOUT_ISLAND_CAVE"),
        "tilesets": ("gTileset_General", "gTileset_Cave"), "version": "emerald",
        "post_convert": wall_read("ICE"), "extra": wall_extra("ICE"),
        "convert": ("HoennIce", "hoenn_ice"), "recolor": {p: ICE for p in range(16)},
        "music": "MUS_SEALED_CHAMBER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": chamber_links("ICE", "OCEAN"),
        "land": land("SNORUNT", "SPHEAL", "SNORUNT", "SPHEAL", "WHISMUR", "SEALEO", "LOUDRED", "GLALIE",
                     "SABLEYE", "ABSOL", ("WALREIN", 52, 55), ("GLALIE", 52, 55)),
        "statics": [("REGICE", REGI_SPOT)],
        "items": [],
    },
}
ROAMERS = ["LATIAS", "LATIOS"]
# The puzzles: the Latis roam once the Soul Dew is found (Pacifidlog's rafts); Kyogre for a lead
# holding the Blue Orb (in Sootopolis); Deoxys once the Meteorite is found (Mt. Chimney's summit);
# Jirachi once every other legendary of the zone is caught. The Regis: their chambers' inner walls
# (Emerald's): sealed until the tablet by the Island Cave sees Wailord first and Relicanth last in
# the party, then each opens for its own Braille: Regirock's for a lead that knows Rock Smash,
# Registeel's for a lead that knows Flash, Regice's after two minutes standing still before it
ROAMER_NEEDS = {"LATIAS": "FLAG_EXOTIC_ITEM_SOUL_DEW", "LATIOS": "FLAG_EXOTIC_ITEM_SOUL_DEW"}
CONDITIONS = {"KYOGRE": ("BLUE_ORB", 0), "DEOXYS": ("METEORITE", 0), "JIRACHI": ("WISH", 0)}
WISH = ["REGIROCK", "REGICE", "REGISTEEL", "LATIAS", "LATIOS", "KYOGRE", "DEOXYS"]
UNIQUE_ITEMS = {"SOUL_DEW", "BLUE_ORB", "METEORITE"}


def header_lines(map_id):
    lines = ["// Hoenn: the Regi chambers' inner walls (7-9, 19-20: the open wall, then the cover and",
             "// Braille-wall metatiles that seal it), and the legendaries Jirachi waits for",
             "struct ExoticSafariRegiWall { u16 map; u16 cover, braille; u16 open[6]; };",
             "static const struct ExoticSafariRegiWall sRegiWalls[] =", "{"]
    for key in REGI_WALLS:
        w = WALLS[key]
        lines.append(f"    {{{map_id(AREAS[key]['map'])}, 0x{w['cover']:04X}, 0x{w['braille']:04X}, "
                     "{" + ", ".join(f"0x{v:04X}" for v in w["open"]) + "}},")
    lines += ["};", f"#define REGI_WALL_X {WALL_X}", f"#define REGI_WALL_Y {WALL_Y}",
              "static const u16 sWishSeal[] = {" + ", ".join(f"SPECIES_{s}" for s in WISH) + "};"]
    return lines

