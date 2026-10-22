"""The Exotic Pokémon Safari's Johto zone (Pokémon Canvas), for build_exotic_safari.py.

Six areas from Nick's planner (exotic-safari-planner-Johto_update.xlsx), all HnS's own maps
(pokehns-expansion: their layouts, on their tilesets as tools/gym_hack/import_johto.py brings them
in), and a room for Lugia:

    Hall door -> GRASSLAND (New Bark Town, with Route 29 to its west, as in HGSS; the hub: in at
    Professor Elm's lab door)
        west -> RUINS (Ruins of Alph: in at the Route 32 gate's door)
                -> its north gate -> FOREST (Ilex Forest: in at the Route 34 gate's door)
        Route 29's gate -> LAKE (Lake of Rage: in from Route 43 at its south edge)
                -> its south-west path -> ICE (Ice Path's first floor: in at the Route 44 door)
                -> its ladder down -> CAVE (Dragon's Den)
                -> the shrine's door -> LUGIA (Whirl Islands' deepest chamber)

Raikou, Entei and Suicune roam the zone.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paldea import read_layout, land   # noqa: E402

HNS = Path("/home/claude/ext/pokehns-expansion")
REGION = "JOHTO"
HUB = "GRASSLAND"
LV = (48, 52)


def hns(layout_id):
    g, l = read_layout(layout_id, root=HNS)
    return [row[:] for row in g], l["width"], l["height"]


# ---------------------------------------------------------------------------------------------
# New Bark Town, Route 29 to its west (New Bark sits 5 rows down, as HGSS joins them), thick
# woods to fill the corners
# ---------------------------------------------------------------------------------------------
NB_X, NB_Y = 70, 5
GRASS_W, GRASS_H = 100, 32           # (New Bark's last rows, all woods, left off: Route 29's height)


def grassland():
    route, rw, rh = hns("LAYOUT_ROUTE29_HNS")
    town, tw, th = hns("LAYOUT_NEW_BARK_TOWN_HNS")
    keep = route[30][0] & ~0x3FF                     # (a tree's collision and elevation)
    tree = lambda x, y: keep | ((0x01C, 0x01D) if y % 2 == 0 else (0x014, 0x015))[x % 2]
    g = [[tree(x, y) for x in range(GRASS_W)] for y in range(GRASS_H)]
    for y in range(rh):
        g[y][:rw] = route[y]
    for y in range(GRASS_H - NB_Y):
        g[NB_Y + y][NB_X:NB_X + tw] = town[y]
    return g, GRASS_W, GRASS_H


def lake():
    return hns("LAYOUT_LAKE_OF_RAGE_HNS")


def ruins():
    return hns("LAYOUT_RUINS_OF_ALPH_OUTSIDE_HNS")


def forest():
    return hns("LAYOUT_ILEX_FOREST_HNS")


def ice():
    return hns("LAYOUT_ICE_PATH_1F_HNS")


def cave():
    return hns("LAYOUT_DRAGONS_DEN_CAVERN_HNS")


def lugia_room():
    return hns("LAYOUT_WHIRL_ISLANDS_LUGIA_CHAMBER_HNS")


# ---------------------------------------------------------------------------------------------
# The Red Gyarados: in the middle of the Lake of Rage, shiny (not a legendary: its own script)
# ---------------------------------------------------------------------------------------------
RED_GYARADOS = (30, 20)
RED_GYARADOS_LEVEL = 55              # (data/scripts/exotic_safari.inc: ExoticSafari_EventScript_RedGyarados)


def lake_extra(ctx):
    x, y = RED_GYARADOS
    if (x, y) not in ctx["reach"]:
        print(f"WARNING LAKE: the Red Gyarados at {(x, y)} can't be reached")
    o = ctx["obj"]("OBJ_EVENT_GFX_SPECIES_SHINY(GYARADOS)", x, y, "ExoticSafari_EventScript_RedGyarados",
                   "MOVEMENT_TYPE_FACE_DOWN", "FLAG_EXOTIC_RED_GYARADOS")
    o["elevation"] = ctx["elev"](x, y)
    ctx["objects"].append(o)


# ---------------------------------------------------------------------------------------------
# The areas
# ---------------------------------------------------------------------------------------------
JG = "gTileset_Johto_General_Hns"
JNE = "gTileset_Johto_NorthEast_Hns"
_R29_WEST = [(0, y) for y in (14, 15, 16, 17)]
_LAKE_SOUTH = [(x, 46) for x in range(26, 36)]
_LAKE_SW = [(18, 46), (19, 46)]

AREAS = {
    "GRASSLAND": {
        "map": "ExoticSafari_JohtoNewBark", "build": grassland, "tilesets": (JG, "gTileset_NewBarkTown_Hns"),
        "music": "MUS_HG_ROUTE34", "weather": "WEATHER_NONE", "type": "MAP_TYPE_CITY",
        "links": {
            "HALL": ("warp", [(NB_X + 10, NB_Y + 9)], (NB_X + 10, NB_Y + 10), ("HALL", None)),   # (Elm's lab)
            "RUINS": ("edge", _R29_WEST, (1, 15), ("RUINS", "GRASSLAND")),
            "LAKE": ("warp", [(34, 10), (35, 10)], (34, 11), ("LAKE", "GRASSLAND")),           # (the Route 46 gate)
        },
        "land": land("SENTRET", "HOOTHOOT", "MAREEP", "HOPPIP", "SUNKERN", "PICHU", "FURRET", "FLAAFFY",
                     "SKIPLOOM", "TOGEPI", ("CHIKORITA", 48, 50), ("AMPHAROS", 52, 55)),
        "water": [("WOOPER", 48, 52), ("MARILL", 48, 52), ("QUAGSIRE", 48, 52), ("AZUMARILL", 50, 54),
                  ("POLITOED", 52, 55)],
        "fishing": [("CHINCHOU", 45, 50), ("REMORAID", 45, 50),
                    ("CHINCHOU", 48, 52), ("REMORAID", 48, 52), ("QWILFISH", 48, 52),
                    ("LANTURN", 50, 54), ("OCTILLERY", 50, 54), ("QWILFISH", 50, 54),
                    ("LANTURN", 52, 55), ("OCTILLERY", 52, 55)],
        "statics": [],
        "items": [],
    },
    "LAKE": {
        "map": "ExoticSafari_JohtoLakeOfRage", "build": lake, "tilesets": (JNE, "gTileset_MahoganyTown_Hns"),
        "music": "MUS_HG_ROUTE42", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "GRASSLAND": ("edge", _LAKE_SOUTH, (30, 45), ("GRASSLAND", "LAKE")),
            "ICE": ("edge", _LAKE_SW, (18, 45), ("ICE", "LAKE")),
        },
        "extra": lake_extra,
        "land": land("GIRAFARIG", "STANTLER", "WOOPER", "DUNSPARCE", "SNUBBULL", "QUAGSIRE", "MARILL", "GRANBULL",
                     "MILTANK", "AZUMARILL", ("TOTODILE", 48, 50), ("CROCONAW", 52, 55)),
        "water": [("MARILL", 48, 52), ("WOOPER", 48, 52), ("QUAGSIRE", 48, 52), ("AZUMARILL", 50, 54),
                  ("CROCONAW", 52, 55)],
        "fishing": [("REMORAID", 45, 50), ("WOOPER", 45, 50),
                    ("REMORAID", 48, 52), ("QWILFISH", 48, 52), ("WOOPER", 48, 52),
                    ("OCTILLERY", 50, 54), ("QUAGSIRE", 50, 54), ("QWILFISH", 50, 54),
                    ("OCTILLERY", 52, 55), ("AZUMARILL", 52, 55)],
        "statics": [],
        "items": [],
    },
    "RUINS": {
        "map": "ExoticSafari_JohtoRuinsOfAlph", "build": ruins, "tilesets": (JNE, "gTileset_RuinsOfAlph_Outside_Hns"),
        "music": "MUS_HG_VIOLET", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "GRASSLAND": ("warp", [(38, 27)], (37, 27), ("GRASSLAND", "RUINS")),         # (the Route 32 gate)
            "FOREST": ("warp", [(20, 6)], (20, 7), ("FOREST", "RUINS")),                # (the Route 36 gate)
        },
        "land": land("NATU", "UNOWN", "SMEARGLE", "UNOWN_L", "XATU", "UNOWN_P", "UNOWN_H", "PHANPY",
                     "DONPHAN", "UNOWN_QUESTION", ("CYNDAQUIL", 48, 50), ("QUILAVA", 52, 55)),
        "water": [("WOOPER", 48, 52), ("MARILL", 48, 52), ("QUAGSIRE", 48, 52), ("AZUMARILL", 50, 54),
                  ("POLITOED", 52, 55)],
        "statics": [],
        "items": [],
    },
    "FOREST": {
        "map": "ExoticSafari_JohtoIlexForest", "build": forest, "tilesets": ("gTileset_Johto_South_Hns", "gTileset_IlexForest_Hns"),
        "music": "MUS_HG_AZALEA", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RUINS": ("warp", [(19, 13), (20, 13)], (19, 14), ("RUINS", "FOREST")),   # (the Route 34 gate)
        },
        "land": land("PINECO", "SPINARAK", "LEDYBA", "HOOTHOOT", "AIPOM", "YANMA", "FORRETRESS", "ARIADOS",
                     "LEDIAN", "NOCTOWL", ("HERACROSS", 50, 54), ("SCIZOR", 52, 55)),
        "water": [("WOOPER", 48, 52), ("MARILL", 48, 52), ("QUAGSIRE", 48, 52), ("AZUMARILL", 50, 54),
                  ("POLITOED", 52, 55)],
        "statics": [("CELEBI", (32, 37))],
        "items": [],
    },
    "ICE": {
        "map": "ExoticSafari_JohtoIcePath", "build": ice, "tilesets": (JG, "gTileset_Cave_Ice_Hns"),
        "music": "MUS_HG_UNION_CAVE", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND", "indoor_encounters": True,
        "links": {
            "LAKE": ("warp", [(14, 24)], (14, 23), ("LAKE", "ICE")),                 # (the Route 44 door)
            "CAVE": ("warp", [(50, 10)], (49, 10), ("CAVE", "ICE")),                 # (the ladder down)
        },
        "land": land("SWINUB", "SNEASEL", "DELIBIRD", "SMOOCHUM", "SWINUB", "SNEASEL", "PILOSWINE", "WOBBUFFET",
                     "DELIBIRD", "JYNX", ("PILOSWINE", 52, 55), ("SNEASEL", 52, 55)),
        "statics": [],
        "items": [],
    },
    "CAVE": {
        "map": "ExoticSafari_JohtoDragonsDen", "build": cave, "tilesets": (JNE, "gTileset_Cave_DragonsDen_Hns"),
        "music": "MUS_HG_UNION_CAVE", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND", "indoor_encounters": True,
        "links": {
            "ICE": ("warp", [(31, 3)], (31, 4), ("ICE", "CAVE")),
            "LUGIA": ("warp", [(31, 46)], (31, 47), ("LUGIA", "CAVE")),             # (the shrine's door)
        },
        "land": land("TEDDIURSA", "LARVITAR", "GLIGAR", "HOUNDOUR", "SKARMORY", "MURKROW", "URSARING", "MISDREAVUS",
                     "HOUNDOOM", "PUPITAR", ("SHUCKLE", 48, 50), ("TYRANITAR", 52, 55)),
        "water": [("CHINCHOU", 48, 52), ("LANTURN", 48, 52), ("CORSOLA", 48, 52), ("MANTINE", 50, 54),
                  ("KINGDRA", 52, 55)],
        "fishing": [("REMORAID", 45, 50), ("CHINCHOU", 45, 50),
                    ("REMORAID", 48, 52), ("CHINCHOU", 48, 52), ("QWILFISH", 48, 52),
                    ("LANTURN", 50, 54), ("OCTILLERY", 50, 54), ("CORSOLA", 50, 54),
                    ("MANTINE", 52, 55), ("KINGDRA", 52, 55)],
        "statics": [],
        "items": [],
    },
    "LUGIA": {
        "map": "ExoticSafari_JohtoWhirlIslands", "build": lugia_room, "tilesets": (JNE, "gTileset_WhirlIslands_Hns"),
        "music": "MUS_HG_LIGHTHOUSE", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "CAVE": ("warp", [(29, 35)], (29, 34), ("CAVE", "LUGIA")),
        },
        "statics": [("LUGIA", (29, 18))],
        "items": [],
    },
}
ROAMERS = ["RAIKOU", "ENTEI", "SUICUNE"]
CONDITIONS = {}
UNIQUE_ITEMS = set()
