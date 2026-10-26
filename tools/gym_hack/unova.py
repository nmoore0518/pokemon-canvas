"""The Exotic Pokémon Safari's Unova zone (Pokémon Canvas), for build_exotic_safari.py.

Six areas from Nick's planner (exotic-safari-planner-unova_update.xlsx):

    Hall door -> BRIDGE (Skyarrow Bridge: the Sevii Islands' Bond Bridge, a long wooden bridge
    over the sea between grasslands; the hub)
        the forest's opening -> CHASM (Giant Chasm: Six Island's Pattern Bush under deep snow)
                                -> its far path: TOWER (Dragonspiral Tower: Johto's Burned Tower in
                                   old gray-green stone)
        the cave in the cliff -> CAVE (Chargestone Cave: Rock Tunnel, its stone charged blue)
                                 -> its far ladder: CASTLE (Relic Castle: Hoenn's Mirage Tower,
                                    sand-choked)
        the old shrine on the beach (Tanoby's) -> its stairs down: RUINS (Abyssal Ruins: two of
                                 Tanoby's chambers, deep under the sea - an underwater map: the
                                 player swims in diving gear)
"""
import colorsys
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paldea import read_layout, blk, land, ROOT   # noqa: E402

REGION = "UNOVA"
HUB = "BRIDGE"
LV = (48, 52)


def tileset_dir(sym):
    import render_map
    return render_map.tileset_dir(sym)


def hsv(c):
    return colorsys.rgb_to_hsv(*(v / 255 for v in c[:3]))


def rgb(h, s, v):
    return tuple(int(max(0, min(1, k)) * 255) for k in colorsys.hsv_to_rgb(h % 1, s, v))


# ---------------------------------------------------------------------------------------------
# Skyarrow Bridge: Bond Bridge. Its forest opening (Berry Forest's) to the Giant Chasm; a cave
# cut into the cliff at its foot (Chargestone Cave); on the beach west of the bridge, one of
# Tanoby's old shrines (its stairs go down to the Abyssal Ruins)
# ---------------------------------------------------------------------------------------------
SHRINE_SRC = (101, 6, 5, 5)           # (Tanoby Ruins: x, y, width, height - the Liptoo shrine)
SHRINE_AT = (27, 8)                   # (on the beach: its door at (29, 12))
SHRINE_IDS = [0x2C8, 0x2C9, 0x2CA, 0x2D0, 0x2D1, 0x2D2, 0x2D8, 0x2D9, 0x2DA, 0x2E0, 0x2E1, 0x2E2]
CAVE_MOUTH = (27, 18)


def bridge():
    import build_exotic_safari as B
    import tile_import as T
    g, lay = read_layout("LAYOUT_THREE_ISLAND_BOND_BRIDGE")
    g = [row[:] for row in g]
    src, _ = read_layout("LAYOUT_SEVEN_ISLAND_TANOBY_RUINS")
    ids = {}

    def edit(out):
        tgt = T.Target(out)
        tgt.compact({v & 0x3FF for row in g for v in row})
        new = T.import_metatiles(tgt, ("gTileset_General_Frlg", "gTileset_SeviiIslands67"), "frlg", SHRINE_IDS, [],
                                 match={1: 1, 5: 5})
        ids.update(zip(SHRINE_IDS, new))
        tgt.save()
    B.clone_tileset("gTileset_SeviiIslands123", "UnovaSkyarrow", edit=edit)
    sx, sy, sw, sh = SHRINE_SRC
    for j in range(sh):
        for i in range(sw):
            v = src[sy + j][sx + i]
            m = v & 0x3FF
            if m in ids:
                g[SHRINE_AT[1] + j][SHRINE_AT[0] + i] = (v & ~0x3FF) | ids[m]
            elif m < 0x280:                          # (the rock round it: the general tileset's)
                g[SHRINE_AT[1] + j][SHRINE_AT[0] + i] = v
    x, y = CAVE_MOUTH
    g[y][x] = blk(0x0A9, 0, (g[y + 1][x] >> 12) & 0xF)   # (a cave's mouth, as on Route 4; at the beach's level)
    for rx, ry in ((33, 15), (33, 16)):                    # (the rock off the beach's corner: open water,
        g[ry][rx] = g[15][34]                                #  so the beach under the cliff can be surfed to)
    for ry in (15, 16):                                      # (its left half sat on the shore: the shore, as
        g[ry][32] = g[17][32]                                #  below it, carries on up to the beach)
    return g, lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Giant Chasm: Pattern Bush under deep snow (its trees and grass whitened)
# ---------------------------------------------------------------------------------------------
def snow_color(r, g, b):
    import galar
    h, s, v = hsv((r, g, b))
    deg = h * 360
    if 30 <= deg <= 62 and v > 0.62 and s < 0.6:   # sand paths: trodden snow
        t = min(1.0, (v - 0.62) / 0.3)
        return tuple(int(a + (b2 - a) * t) for a, b2 in zip((168, 184, 216), (240, 244, 255)))
    return galar.snowify(0, (r, g, b))


def chasm():
    g, lay = read_layout("LAYOUT_SIX_ISLAND_PATTERN_BUSH")
    return [row[:] for row in g], lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Dragonspiral Tower: Johto's Burned Tower, its charred wood old gray-green stone
# ---------------------------------------------------------------------------------------------
def tower_color(r, g, b):
    h, s, v = hsv((r, g, b))
    deg = h * 360
    if (deg < 70 or deg > 330) and s > 0.12:        # wood, char: weathered stone, a little green
        return rgb(150 / 360, s * 0.28, min(1, v * 0.95 + 0.04))
    return (r, g, b)


def tower():
    g, lay = read_layout("LAYOUT_BURNED_TOWER_1F")
    return [row[:] for row in g], lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Chargestone Cave: Rock Tunnel B1F, its rock a charged electric blue
# ---------------------------------------------------------------------------------------------
def cave_color(r, g, b):
    h, s, v = hsv((r, g, b))
    deg = h * 360
    if 30 <= deg <= 75:                             # the olive floor: deep blue-violet
        return rgb(240 / 360, min(1, s * 0.7 + 0.15), v * 0.6)
    if 150 <= deg <= 220 or s < 0.15:               # the pale rock: electric cyan-blue
        return rgb(198 / 360, min(1, s * 1.6 + 0.25), min(1, v * 1.02))
    return (r, g, b)


def cave():
    g, lay = read_layout("LAYOUT_ROCK_TUNNEL_B1F")
    return [row[:] for row in g], lay["width"], lay["height"]


GLOW_SLOT = 12                        # (the stones' own palette: it pulses - InitTilesetAnim_Chargestone)
GLOW_STONES = (0x282, 0x283)          # (Rock Tunnel's boulders and pebbles on the floor)


def cave_glow(out, remap):
    """Chargestone's stones glow: the boulders' and pebbles' top layer in a palette of their own,
    bright electric blue (its colors pulse)"""
    import struct
    import build_exotic_safari as B
    import render_map
    mt = bytearray((out / "metatiles.bin").read_bytes())
    src = None
    for m in GLOW_STONES:
        if m not in remap:
            continue
        k = (remap[m] - 0x280) * 16
        ents = list(struct.unpack_from("<8H", mt, k))
        for i in range(4, 8):
            if ents[i] & 0x3FF:
                src = ents[i] >> 12
                ents[i] = (ents[i] & 0x0FFF) | (GLOW_SLOT << 12)
        struct.pack_into("<8H", mt, k, *ents)
    (out / "metatiles.bin").write_bytes(bytes(mt))
    assert src is not None and src < GLOW_SLOT
    colors = render_map.load_pal(out / "palettes" / f"{src:02d}.pal")
    def glow(c):
        h, s, v = hsv(c)
        return rgb(188 / 360, 0.5 + 0.4 * (1 - v), min(1, 0.45 + 0.65 * v))
    B.write_pal(out / "palettes" / f"{GLOW_SLOT:02d}.pal", [colors[0][:3]] + [glow(c) for c in colors[1:]])


# ---------------------------------------------------------------------------------------------
# Relic Castle: Hoenn's Mirage Tower 1F, sand-choked
# ---------------------------------------------------------------------------------------------
def castle():
    g, lay = read_layout("LAYOUT_MIRAGE_TOWER_1F")
    return [row[:] for row in g], lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Abyssal Ruins: two of Tanoby's chambers side by side, deep under the sea (a deep blue-green;
# an underwater map: the player swims)
# ---------------------------------------------------------------------------------------------
def ruins_color(r, g, b):
    h, s, v = hsv((r, g, b))
    if v < 0.08:
        return (r, g, b)
    return rgb((186 + 30 * (1 - v)) / 360, min(1, s * 0.5 + 0.45), v * 0.58 + 0.04)


RUINS_W = 22 + 22


def ruins():
    a, lay = read_layout("LAYOUT_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER")
    b, _ = read_layout("LAYOUT_SEVEN_ISLAND_TANOBY_RUINS_DILFORD_CHAMBER")
    g = [ra[:22] + rb[1:] for ra, rb in zip(a, b)]
    g[15][22 + 10] = g[15][10 + 1]                   # (the second chamber's stairs: floor)
    g[16][0] = blk(0x008, 1)                         # (the void beyond: the border, border_at)
    return g, RUINS_W, lay["height"]


# ---------------------------------------------------------------------------------------------
# The areas
# ---------------------------------------------------------------------------------------------
AREAS = {
    "BRIDGE": {
        "map": "ExoticSafari_UnovaSkyarrow", "build": bridge, "border_at": (30, 1), "tilesets": ("gTileset_General_Frlg", "gTileset_UnovaSkyarrow"),
        "music": "MUS_RG_SEVII_ROUTE", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "HALL": ("edge", [(95, y) for y in range(7, 12)], (94, 9), ("HALL", None)),
            "CHASM": ("warp", [(12, 6), (13, 6)], (12, 7), ("CHASM", "BRIDGE")),
            "CAVE": ("warp", [CAVE_MOUTH], (CAVE_MOUTH[0], CAVE_MOUTH[1] + 1), ("CAVE", "BRIDGE")),
            "RUINS": ("warp", [(SHRINE_AT[0] + 2, SHRINE_AT[1] + 4)], (SHRINE_AT[0] + 2, SHRINE_AT[1] + 5), ("RUINS", "BRIDGE")),
        },
        # ("A|B": a slot shared by both - one or the other each time)
        "land": land("PATRAT|PURRLOIN", "LILLIPUP|MINCCINO", "PIDOVE", "SEWADDLE|VENIPEDE", "COTTONEE|PETILIL",
                     "BLITZLE|BOUFFALANT", "PANSAGE|PANSEAR|PANPOUR", "GOTHITA|SOLOSIS", "DEERLING", "MUNNA|TRUBBISH",
                     ("AUDINO", 48, 52), ("SNIVY", 48, 50)),
        "water": [("BASCULIN_RED_STRIPED", 48, 52), ("BASCULIN_BLUE_STRIPED", 48, 52), ("DUCKLETT", 48, 52),
                  ("FRILLISH", 50, 54), ("SWANNA", 52, 55)],
        "fishing": [("BASCULIN_RED_STRIPED", 45, 50), ("BASCULIN_BLUE_STRIPED", 45, 50),
                    ("TYMPOLE", 48, 52), ("BASCULIN_RED_STRIPED", 48, 52), ("STUNFISK", 48, 52),
                    ("ALOMOMOLA", 50, 54), ("STUNFISK", 50, 54), ("SEISMITOAD", 50, 54),
                    ("ALOMOMOLA", 52, 55), ("OSHAWOTT", 48, 50)],
        "statics": [("TERRAKION", (70, 13)), ("VIRIZION", (20, 9))],
        "items": [],
    },
    "CHASM": {
        "map": "ExoticSafari_UnovaGiantChasm", "build": chasm, "tilesets": ("gTileset_General_Frlg", "gTileset_ViridianForest"),
        "convert_frlg": True, "convert": ("UnovaChasm", "unova_chasm"), "recolor": {p: snow_color for p in range(16)},
        "music": "MUS_RG_ROUTE24", "weather": "WEATHER_SNOW", "type": "MAP_TYPE_ROUTE",
        "links": {
            "BRIDGE": ("warp", [(3, 26), (4, 26), (5, 26)], (4, 25), ("BRIDGE", "CHASM")),
            "TOWER": ("warp", [(54, 26), (55, 26), (56, 26)], (55, 25), ("TOWER", "CHASM")),
        },
        "land": land("CUBCHOO|VANILLITE", "DEERLING", "KARRABLAST|SHELMET", "FOONGUS", "EMOLGA", "CRYOGONAL",
                     "PAWNIARD", "ZORUA", "VANILLISH|BEARTIC", "SAWSBUCK", ("VANILLUXE", 50, 54), ("BISHARP", 52, 55)),
        "statics": [("COBALION", (30, 6)), ("KYUREM", (30, 15))],
        "items": [],
    },
    "TOWER": {
        "map": "ExoticSafari_UnovaDragonspiral", "build": tower, "border_at": (2, 22), "tilesets": ("gTileset_Johto_Building_Hns", "gTileset_BurnedTower_Hns"),
        "convert_frlg": True, "convert": ("UnovaDragonspiral", "unova_dragonspiral"), "recolor": {p: tower_color for p in range(16)},
        "music": "MUS_HG_BURNED_TOWER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND", "indoor_encounters": True,
        "links": {
            "CHASM": ("warp", [(15, 22)], (15, 21), ("CHASM", "TOWER")),
        },
        "remove_holes": [(24, 10), (16, 12)],
        "land": land("GOLETT|LITWICK", "MIENFOO", "AXEW", "SAWK|THROH", "DRUDDIGON", "DEINO", "RUFFLET|VULLABY",
                     "FRAXURE", "GOLURK", "MIENSHAO", ("ZWEILOUS", 50, 54), ("HAXORUS", 52, 55)),
        "statics": [("RESHIRAM", (15, 4))],
        "items": [],
    },
    "CAVE": {
        "map": "ExoticSafari_UnovaChargestone", "build": cave, "tilesets": ("gTileset_General_Frlg", "gTileset_RockTunnel"),
        "convert_frlg": True, "convert": ("UnovaChargestone", "unova_chargestone"), "recolor": {p: cave_color for p in range(16)},
        "tileset_edit": cave_glow, "tileset_callback": "InitTilesetAnim_Chargestone",
        "music": "MUS_RG_MT_MOON", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "BRIDGE": ("warp", [(38, 28)], (38, 29), ("BRIDGE", "CAVE")),
            # (Rock Tunnel's halves don't meet: the ladder up to the castle is the one in the half
            # the bridge's cave leads into)
            "CASTLE": ("warp", [(33, 3)], (33, 4), ("CASTLE", "CAVE")),
        },
        "remove_warps": [(2, 3), (27, 12)],
        "land": land("JOLTIK", "ROGGENROLA|WOOBAT", "FERROSEED", "KLINK", "TIMBURR", "DRILBUR", "TYNAMO",
                     "HEATMOR|DURANT", "GALVANTULA", "FERROTHORN", ("EELEKTRIK", 50, 54), ("KLANG", 50, 54)),
        "statics": [("ZEKROM", (24, 20))],
        "items": [],
    },
    "CASTLE": {
        "map": "ExoticSafari_UnovaRelicCastle", "build": castle, "tilesets": ("gTileset_General", "gTileset_MirageTower"),
        "version": "emerald", "convert": ("UnovaRelicCastle", "unova_relic_castle"),
        "music": "MUS_RG_SEVII_DUNGEON", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "CAVE": ("warp", [(10, 14)], (10, 13), ("CAVE", "CASTLE")),
        },
        "remove_warps": [(15, 2)],
        "land": land("SANDILE", "DARUMAKA", "YAMASK", "SIGILYPH|ELGYEM", "DWEBBLE", "SCRAGGY", "KROKOROK", "MARACTUS",
                     "COFAGRIGUS", ("ARCHEN", 48, 50), ("TEPIG", 48, 50), ("LARVESTA", 50, 52)),
        "statics": [("VICTINI", (10, 2))],
        "items": [],
    },
    "RUINS": {
        "map": "ExoticSafari_UnovaAbyssalRuins", "build": ruins, "border_at": (0, 16), "tilesets": ("gTileset_BuildingFrlg", "gTileset_TanobyRuins"),
        "convert_frlg": True, "convert": ("UnovaAbyssalRuins", "unova_abyssal_ruins"), "recolor": {p: ruins_color for p in range(16)},
        "music": "MUS_UNDERWATER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERWATER", "indoor_encounters": True, "encounter_rate": 10,
        "links": {
            "BRIDGE": ("warp", [(11, 15)], (11, 14), ("BRIDGE", "RUINS")),
        },
        "land": land("FRILLISH", "ALOMOMOLA", "BASCULIN_RED_STRIPED", "BASCULIN_BLUE_STRIPED", "JELLICENT", "TIRTOUGA",
                     "EELEKTRIK", "CARRACOSTA", "FRILLISH", "ALOMOMOLA", ("EELEKTROSS", 50, 54), ("OSHAWOTT", 48, 50)),
        "statics": [("KELDEO", (32, 9))],
        "items": [],
    },
}
ROAMERS = ["THUNDURUS", "LANDORUS"]
ROAMER_NEEDS = {"LANDORUS": "THUNDURUS"}       # (Landorus comes once Thundurus is caught)
UNIQUE_ITEMS = set()

# ---------------------------------------------------------------------------------------------
# The puzzles
# ---------------------------------------------------------------------------------------------
# Legendaries that wait for others to be caught (sCaughtSeals: SEAL_CAUGHT's argument)
CAUGHT_SEALS = [["TERRAKION", "VIRIZION"], ["TERRAKION", "VIRIZION", "COBALION"], ["RESHIRAM", "ZEKROM"]]
# Keepsakes found about the zone (flags from FLAG_EXOTIC_UNOVA_KEY_FIRST): name, area, where
KEYS = [("LIGHT STONE", "CASTLE", (3, 12)), ("DARK STONE", "RUINS", (26, 6)), ("LIBERTY PASS", "BRIDGE", (11, 9))]
CONDITIONS = {
    "COBALION": ("CAUGHT", 0),        # (Terrakion and Virizion caught: the Swords of Justice's leader)
    "KELDEO": ("CAUGHT", 1),          # (all three Swords caught)
    "KYUREM": ("CAUGHT", 2),          # (Reshiram and Zekrom caught)
    "RESHIRAM": ("KEY", 0), "ZEKROM": ("KEY", 1), "VICTINI": ("KEY", 2),
}
CHASM_ELDER = (6, 23)                 # (the Giant Chasm's old man: the DNA Splicers)
SHRINE_KEEPER = (32, 13)              # (Skyarrow's shrine keeper: the Reveal Glass)


def extra(area):
    def add(ctx):
        name = ctx["name"]
        for k, (kname, karea, (x, y)) in enumerate(KEYS):
            if karea != area:
                continue
            if (x, y) not in ctx["reach"]:
                print(f"WARNING {area}: the {kname} at {(x, y)} can't be reached")
            label = f"{name}_EventScript_Key{k}"
            o = ctx["obj"]("OBJ_EVENT_GFX_ITEM_BALL", x, y, label, "MOVEMENT_TYPE_FACE_DOWN", f"0x{0x372 + k:03X}")
            o["elevation"] = ctx["elev"](x, y)
            ctx["objects"].append(o)
            ctx["scripts"] += [f"{label}::", "\tlock", f"\tsetvar VAR_0x8004, {k}", "\tgoto ExoticSafari_EventScript_Key", ""]
        for who, where, gfx, script in (("CHASM", CHASM_ELDER, "OBJ_EVENT_GFX_OLD_MAN_1", "ExoticSafari_EventScript_ChasmElder"),
                                        ("BRIDGE", SHRINE_KEEPER, "OBJ_EVENT_GFX_OLD_WOMAN_FRLG", "ExoticSafari_EventScript_ShrineKeeper")):
            if who != area:
                continue
            if where not in ctx["reach"]:
                print(f"WARNING {area}: {script} at {where} can't be reached")
            o = ctx["obj"](gfx, *where, script, "MOVEMENT_TYPE_FACE_DOWN")
            o["elevation"] = ctx["elev"](*where)
            ctx["objects"].append(o)
    return add


for _k in AREAS:
    AREAS[_k]["extra"] = extra(_k)


def header_lines(map_id):
    lines = ["// Unova: legendaries that wait for others, and the keepsakes others answer to",
             "static const u16 sCaughtSeals[][3] =", "{"]
    lines += ["    {" + ", ".join(f"SPECIES_{s}" for s in seal + ["NONE"] * (3 - len(seal))) + "}," for seal in CAUGHT_SEALS]
    lines += ["};", "struct ExoticSafariKey { u16 flag; const u8 *name; };", "static const struct ExoticSafariKey sUnovaKeys[] =", "{"]
    lines += [f'    {{FLAG_EXOTIC_UNOVA_KEY_FIRST + {k}, COMPOUND_STRING("{n}")}},' for k, (n, _, _) in enumerate(KEYS)]
    return lines + ["};"]
