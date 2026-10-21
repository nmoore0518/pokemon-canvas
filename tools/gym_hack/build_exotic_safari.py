#!/usr/bin/env python3
"""The Exotic Pokémon Safari (Pokémon Canvas): a second door on Fuchsia's Safari Zone building leads
to a hall with one door per region, each with the region's name on a plaque above it. Only the
open zones' doors open (for now, Paldea's: a small test meadow); the others are shut.

    python3 tools/gym_hack/build_exotic_safari.py [preview_dir]

Writes:
  data/tilesets/secondary/exotic_safari_frlg/  the Safari Zone building's tileset plus the plaques,
                                                plain wall pieces and shut doors (drawn here)
  data/layouts/ExoticSafari_*/, layouts.json    the hall and the zones
  data/maps/ExoticSafari_*/map.json             (their scripts.inc are kept if they exist)
  data/maps/map_groups.json                     gMapGroup_ExoticSafari
  data/maps/exotic_safari.inc                   the maps' scripts, included by data/event_scripts.s
  FuchsiaCity_Frlg: the door (the Safari Office's door, in place of a window), the sign, the warp
  src/data/wild_encounters.json                 each zone's Pokémon (ZONES below)
"""
import json
import shutil
import struct
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SRC_TS = ROOT / "data/tilesets/secondary/safari_zone_building_frlg"
TS = ROOT / "data/tilesets/secondary/exotic_safari_frlg"
SEC = 0x280          # first secondary metatile id; first secondary tile is 640
PAL = 8              # the wall/door palette (index 1 dark slate, 2 grey, 3 wall, 5 frame, 6 olive, 7 brown, b white)

# The regions, west to east, and which have a zone open (name -> its map). The hall's doors.
REGIONS = ["JOHTO", "HOENN", "SINNOH", "UNOVA", "KALOS", "ALOLA", "GALAR", "PALDEA"]
ZONES = {
    "PALDEA": {
        "map": "ExoticSafari_PaldeaMeadow",
        "id": "MAP_EXOTIC_SAFARI_PALDEA_MEADOW",
        "label": "gExoticSafari_PaldeaMeadow",
        # Placeholder Pokémon for the test meadow (Nick's planner will replace them): FireRed's 12
        # grass slots (20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1 %)
        "land": [("LECHONK", 48, 52), ("TAROUNTULA", 48, 52), ("SMOLIV", 48, 52), ("FIDOUGH", 48, 52),
                 ("NYMBLE", 48, 52), ("PAWMI", 48, 52), ("FLITTLE", 48, 52), ("TANDEMAUS", 48, 52),
                 ("SQUAWKABILLY", 48, 52), ("SHROODLE", 48, 52), ("TOEDSCOOL", 50, 54), ("WATTREL", 50, 54)],
    },
}
# Each door in its region's games' two colors (left half, right half)
DOOR_COLORS = {
    "JOHTO": ((232, 184, 48), (176, 184, 200)),     # Gold, Silver
    "HOENN": ((200, 32, 48), (40, 88, 208)),        # Ruby, Sapphire
    "SINNOH": ((88, 152, 232), (232, 160, 200)),    # Diamond, Pearl
    "UNOVA": ((56, 56, 64), (240, 240, 232)),       # Black, White
    "KALOS": ((40, 104, 208), (216, 48, 64)),       # X, Y
    "ALOLA": ((240, 144, 32), (96, 72, 200)),       # Sun, Moon
    "GALAR": ((48, 168, 232), (200, 40, 120)),      # Sword, Shield
    "PALDEA": ((216, 48, 40), (136, 64, 200)),      # Scarlet, Violet
}
DOOR_PALETTES = [7, 10, 11, 12]        # two doors per palette (free in this copy of the tileset)
DOOR_SPACING, FIRST_DOOR = 4, 3
HALL_W, HALL_H = FIRST_DOOR * 2 + DOOR_SPACING * (len(REGIONS) - 1) + 1, 12
MAT_X = HALL_W // 2                    # the entrance mat's middle, and the gap in the counter
DOOR_Y, COUNTER_Y = 3, 7
KIOSK_X = MAT_X + 6
FUCHSIA_DOOR = (26, 5)                 # the new door on the Safari Zone building (a window before)
FUCHSIA_SIGN = (27, 6)

# 5x7 letters for the plaques
FONT = {
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01111"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "J": ["00111", "00010", "00010", "00010", "00010", "10010", "01100"],
    "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
}


# ---------------------------------------------------------------------------------------------
# The tileset
# ---------------------------------------------------------------------------------------------
class Tileset:
    def __init__(self):
        self.img = Image.open(SRC_TS / "tiles.png")
        self.tiles = []
        w = self.img.width // 8
        px = self.img.load()
        for t in range(w * (self.img.height // 8)):
            self.tiles.append(tuple(px[(t % w) * 8 + x, (t // w) * 8 + y] for y in range(8) for x in range(8)))
        raw = (SRC_TS / "metatiles.bin").read_bytes()
        self.metatiles = [list(struct.unpack_from("<8H", raw, i * 16)) for i in range(len(raw) // 16)]
        raw = (SRC_TS / "metatile_attributes.bin").read_bytes()
        self.attrs = [struct.unpack_from("<I", raw, i * 4)[0] for i in range(len(raw) // 4)]

    def tile(self, entry):
        """The 8x8 pixels of a metatile entry (flips applied)"""
        t = self.tiles[(entry & 0x3FF) - 640]
        rows = [list(t[y * 8:y * 8 + 8]) for y in range(8)]
        if entry >> 10 & 1:
            rows = [r[::-1] for r in rows]
        if entry >> 11 & 1:
            rows = rows[::-1]
        return rows

    def pixels(self, mid):
        """A metatile's bottom layer as 16x16 palette indices"""
        e = self.metatiles[mid - SEC]
        out = [[0] * 16 for _ in range(16)]
        for k in range(4):
            t = self.tile(e[k])
            for y in range(8):
                for x in range(8):
                    out[(k // 2) * 8 + y][(k % 2) * 8 + x] = t[y][x]
        return out

    def add(self, pixels, attr=0, pal=PAL):
        """A new metatile (bottom layer only) from 16x16 pixels; returns its id"""
        entries = []
        for k in range(4):
            t = tuple(pixels[(k // 2) * 8 + y][(k % 2) * 8 + x] for y in range(8) for x in range(8))
            if t not in self.tiles:
                self.tiles.append(t)
            entries.append((self.tiles.index(t) + 640) | (pal << 12))
        self.metatiles.append(entries + [0, 0, 0, 0])
        self.attrs.append(attr)
        return SEC + len(self.metatiles) - 1

    def write(self):
        if TS.exists():
            shutil.rmtree(TS)
        shutil.copytree(SRC_TS, TS)
        cols = 16
        rows = (len(self.tiles) + cols - 1) // cols
        img = Image.new("P", (cols * 8, rows * 8))
        img.putpalette(self.img.getpalette())
        px = img.load()
        for n, t in enumerate(self.tiles):
            for k, v in enumerate(t):
                px[(n % cols) * 8 + k % 8, (n // cols) * 8 + k // 8] = v
        img.save(TS / "tiles.png")
        (TS / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *m) for m in self.metatiles))
        (TS / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in self.attrs))
        # The door palettes: the wall palette's colors, with two doors' colors in the free slots
        base = [tuple(map(int, l.split())) for l in (SRC_TS / "palettes/08.pal").read_text().split("\n")[3:] if l.strip()]
        for i, pal in enumerate(DOOR_PALETTES):
            cols = list(base)
            for slot, name in enumerate(REGIONS[i * 2:i * 2 + 2]):
                for half, c in enumerate(DOOR_COLORS[name]):
                    base_i, shade_i = DOOR_INDICES[slot][half]
                    cols[base_i] = c
                    cols[shade_i] = tuple(int(v * 0.68) for v in c) if sum(c) > 150 else tuple(min(255, v + 40) for v in c)
            (TS / f"palettes/{pal:02d}.pal").write_text("JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{r} {g} {b}\r\n" for r, g, b in cols))
        assert len(self.tiles) <= 384 and len(self.metatiles) <= 384, (len(self.tiles), len(self.metatiles))


# Palette indices of a door's colors: slot (first or second door in the palette) -> (left, right)
# -> (base, shade). The rest (1 dark, 2 grey, 3 wall, 5 frame) are the wall palette's.
DOOR_INDICES = [((4, 6), (7, 8)), ((9, 10), (11, 12))]


def fill(c):
    return [[c] * 16 for _ in range(16)]


def recolor(p, mapping):
    return [[mapping.get(v, v) for v in row] for row in p]


def build_tileset():
    ts = Tileset()
    m = {}
    m["wall"] = ts.add(fill(3))                                            # plain wall
    base = ts.pixels(0x28F)                                               # wall with the baseboard
    m["wall_base"] = ts.add(base)
    # Door tops without the white band above them (a plain wall row goes above)
    for k, src in (("top_l", 0x283), ("top_m", 0x284), ("top_r", 0x285)):
        p = ts.pixels(src)
        for y in range(8):
            p[y] = [3] * 16
        m[k] = ts.add(p)
    # Closed doors, in the region's colors: the doorway's dark (index 1) becomes the door, left
    # half one color and right half the other, with darker edges, a seam and handles.
    # The door runs 3 pixels into the left and right metatiles: door x -3..18 (0-15 the middle one).
    pieces = (("top_l", m["top_l"], -16), ("top_m", m["top_m"], 0), ("top_r", m["top_r"], 16),
              ("door_l", 0x28B, -16), ("door_m", 0x28C, 0), ("door_r", 0x28D, 16))
    for i, name in enumerate(REGIONS):
        pal, slot = DOOR_PALETTES[i // 2], i % 2
        for k, src, off in pieces:
            p = ts.pixels(src)
            top = k.startswith("top")
            for y in range(16):
                for x in range(16):
                    if p[y][x] != 1:
                        continue
                    gx = x + off
                    if not -3 <= gx <= 18:
                        continue                                           # (the baseboard)
                    half = 0 if gx < 8 else 1
                    base_i, shade_i = DOOR_INDICES[slot][half]
                    shade = gx in (-3, 7, 8, 18) or (not top and y in (4, 5, 6) and gx in (5, 10))
                    p[y][x] = shade_i if shade else base_i
            m[f"closed_{name}_{k}"] = ts.add(p, pal=pal)
    # The counter between the lobby and the doors (a counter: people talk across it)
    counter = [[9] * 16] + [[0xA] * 16 for _ in range(8)] + [[9] * 16] + [[7] * 16 for _ in range(5)] + [[6] * 16]
    m["counter"] = ts.add(counter, attr=0x80)
    # The plaques: the region's name in white on dark slate, in a light frame, over the wall
    for name in REGIONS:
        img = fill(3) + []
        img = [[3] * 48 for _ in range(16)]
        for y in range(2, 14):
            for x in range(2, 46):
                edge = y in (2, 13) or x in (2, 45)
                img[y][x] = 5 if edge else 1
        for x in range(3, 46):
            img[14][x] = 2                                                # shadow under it
        width = len(name) * 6 - 1
        x0, y0 = 24 - width // 2, 4
        for i, ch in enumerate(name):
            for y, row in enumerate(FONT[ch]):
                for x, bit in enumerate(row):
                    if bit == "1":
                        img[y0 + y][x0 + i * 6 + x] = 0xB
        for part in range(3):
            m[f"plaque_{name}_{part}"] = ts.add([row[part * 16:part * 16 + 16] for row in img])
    ts.write()
    return m


def register_tileset():
    """headers.h, graphics.h, metatiles.h: gTileset_ExoticSafari (like gTileset_SafariZoneBuilding)"""
    def put(path, anchor, text):
        s = path.read_text()
        if text.splitlines()[0] in s:
            return
        i = s.index(anchor)
        path.write_text(s[:i] + text + s[i:])
    put(ROOT / "src/data/tilesets/headers.h", "const struct Tileset gTileset_SafariZoneBuilding =",
        "const struct Tileset gTileset_ExoticSafari =\n{\n    .isCompressed = TRUE,\n    .isSecondary = TRUE,\n"
        "    .tiles = gTilesetTiles_ExoticSafari,\n    .palettes = gTilesetPalettes_ExoticSafari,\n"
        "    .metatiles = gMetatiles_ExoticSafari,\n    .metatileAttributes = gMetatileAttributes_ExoticSafari,\n"
        "    .callback = NULL,\n};\n\n")
    put(ROOT / "src/data/tilesets/graphics.h", "const u32 gTilesetTiles_SafariZoneBuilding[]",
        'const u32 gTilesetTiles_ExoticSafari[] = INCGFX_U32("data/tilesets/secondary/exotic_safari_frlg/tiles.png", ".4bpp.fastSmol");\n\n'
        "const u16 gTilesetPalettes_ExoticSafari[][16] =\n{\n" + "".join(
            f'    INCGFX_U16("data/tilesets/secondary/exotic_safari_frlg/palettes/{i:02d}.pal", ".gbapal"),\n' for i in range(16))
        + "};\n\n")
    put(ROOT / "src/data/tilesets/metatiles.h", "const u16 gMetatiles_SafariZoneBuilding[]",
        'const u16 gMetatiles_ExoticSafari[] = INCBIN_U16("data/tilesets/secondary/exotic_safari_frlg/metatiles.bin");\n'
        'const u16 gMetatileAttributes_ExoticSafari[] = INCBIN_U16("data/tilesets/secondary/exotic_safari_frlg/metatile_attributes.bin");\n')


# ---------------------------------------------------------------------------------------------
# Maps
# ---------------------------------------------------------------------------------------------
def blk(m, collision, elevation=3):
    return m | (collision << 10) | (elevation << 12)


def door_x(i):
    return FIRST_DOOR + i * DOOR_SPACING


def build_hall(m):
    W, H = HALL_W, HALL_H
    g = [[blk(0x2A2, 0)] * W for _ in range(H)]                            # floor
    for x in range(W):
        g[0][x] = blk(0x282, 1)                                            # the wall's top band
        g[1][x] = blk(m["wall"], 1)
        g[2][x] = blk(m["wall"], 1)
        g[3][x] = blk(m["wall_base"], 1)
        g[H - 1][x] = blk(0x008, 1, 0)                                     # (outside the room)
    for i, name in enumerate(REGIONS):
        x = door_x(i)
        for part in range(3):
            g[1][x - 1 + part] = blk(m[f"plaque_{name}_{part}"], 1)
        # Every door starts closed; an open zone's door slides open when the player stands in front
        # of it (src/exotic_safari.c)
        g[2][x - 1:x + 2] = [blk(m[f"closed_{name}_top_{s}"], 1) for s in "lmr"]
        g[3][x - 1:x + 2] = [blk(m[f"closed_{name}_door_{s}"], 1) for s in "lmr"]
    # The counter between the lobby and the doors, with a gap in the middle (the attendant stands in
    # it until the player buys a pass)
    for x in range(W):
        if x != MAT_X:
            g[COUNTER_Y][x] = blk(m["counter"], 1)
    # The entrance mat (warps out to Fuchsia) and the doorway shadow under it
    g[H - 2][MAT_X - 1:MAT_X + 2] = [blk(0x2BC, 0), blk(0x2BD, 0), blk(0x2BE, 0)]
    g[H - 1][MAT_X - 1:MAT_X + 2] = [blk(0x01A, 1, 0), blk(0x01B, 1, 0), blk(0x01C, 1, 0)]
    # Potted plants in the corners (the safari entrance's)
    for x in (0, W - 1):
        g[3][x] = blk(0x28F, 1)
        g[4][x] = blk(0x297, 1)
    return g


def forest(x, y):
    return (0x290, 0x291, 0x292)[x % 3] if y % 2 == 0 else (0x298, 0x299, 0x29A)[x % 3]


def ground(x, y):
    return (0x008, 0x009)[x % 2] if y % 2 == 0 else (0x010, 0x011)[x % 2]


MEADOW = [   # the test meadow: T trees, . ground, g tall grass, P path (the exit)
    "TTTTTTTTTTTTTTTTTTTTTTTT",
    "TTTTTTTTTTTTTTTTTTTTTTTT",
    "TTTTTTTTTTTTTTTTTTTTTTTT",
    "TTTTTTTTTTTTTTTTTTTTTTTT",
    "TTT..gggggg....ggggg.TTT",
    "TTT.gggggggg..gggggg.TTT",
    "TTT.gggggggg..gggggg.TTT",
    "TTT..ggggg....ggggg..TTT",
    "TTT........gg........TTT",
    "TTTgg.....gggg....gg.TTT",
    "TTTggg..gggggg...ggg.TTT",
    "TTTggg..gggggg..gggg.TTT",
    "TTT......gggg...ggg..TTT",
    "TTT......gg..........TTT",
    "TTT..................TTT",
    "TTTTTTTTT...TTTTTTTTTTTT",
    "TTTTTTTTTPPPTTTTTTTTTTTT",
    "TTTTTTTTTPPPTTTTTTTTTTTT",
    "TTTTTTTTTPPPTTTTTTTTTTTT",
    "TTTTTTTTTPPPTTTTTTTTTTTT",
]
MEADOW_EXIT = 16      # the row of the exit (arrow warps) on the path


def build_meadow():
    rows = MEADOW
    H, W = len(rows), len(rows[0])
    g = [[0] * W for _ in range(H)]
    open_ = lambda x, y: 0 <= y < H and 0 <= x < W and rows[y][x] != "T"
    for y in range(H):
        for x in range(W):
            c = rows[y][x]
            if c == "T":
                if open_(x, y + 1) or open_(x, y + 2) and y % 2 == 0:
                    # the forest's bottom edge (trunks over the ground), 2 rows
                    below = y + 1 if open_(x, y + 1) else y + 2
                    t = (0x2A0, 0x2A1, 0x2A2)[x % 3] if (below - y) == 2 else (0x2A3, 0x2A4, 0x2A5)[x % 3]
                    g[y][x] = blk(t, 1)
                elif open_(x, y - 1) and y % 2 == 1 and rows[y][x] == "T":
                    g[y][x] = blk((0x288, 0x289, 0x28A)[x % 3], 1)          # the forest's top edge
                else:
                    g[y][x] = blk(forest(x, y), 1)
            elif c == "g":
                g[y][x] = blk(0x00D, 0)
            elif c == "P":
                g[y][x] = blk((0x2A8, 0x2A9, 0x2AA)[(x - 9)] if y == MEADOW_EXIT else (0x0DB, 0x0DC, 0x0DD)[x - 9], 0)
            else:
                g[y][x] = blk(ground(x, y), 0)
    return g, W, H


def write_layout(name, lid, grid, primary, secondary):
    W, H = len(grid[0]), len(grid)
    d = ROOT / "data/layouts" / name
    d.mkdir(parents=True, exist_ok=True)
    (d / "map.bin").write_bytes(b"".join(struct.pack("<H", v) for row in grid for v in row))
    border = [grid[0][0]] * 4
    (d / "border.bin").write_bytes(struct.pack("<4H", *border))
    lj_path = ROOT / "data/layouts/layouts.json"
    lj = json.loads(lj_path.read_text())
    entry = {"id": lid, "name": f"{name}_Layout", "width": W, "height": H, "border_width": 2, "border_height": 2,
             "primary_tileset": primary, "secondary_tileset": secondary,
             "border_filepath": f"data/layouts/{name}/border.bin", "blockdata_filepath": f"data/layouts/{name}/map.bin",
             "layout_version": "frlg"}
    lj["layouts"] = [l for l in lj["layouts"] if l.get("id") != lid] + [entry]
    lj_path.write_text(json.dumps(lj, indent=2, ensure_ascii=False) + "\n")


def obj(gfx, x, y, script, movement="MOVEMENT_TYPE_FACE_DOWN", flag="0"):
    return {"type": "object", "graphics_id": gfx, "x": x, "y": y, "elevation": 3, "movement_type": movement,
            "movement_range_x": 1, "movement_range_y": 1, "trainer_type": "TRAINER_TYPE_NONE",
            "trainer_sight_or_berry_tree_id": "0", "script": script, "flag": flag}


def write_map(name, mid, layout, music, mapsec, map_type, objects, warps, bgs, show_name):
    d = ROOT / "data/maps" / name
    d.mkdir(parents=True, exist_ok=True)
    m = {"id": mid, "name": name, "layout": layout, "music": music, "region": "REGION_KANTO",
         "region_map_section": mapsec, "requires_flash": False, "weather": "WEATHER_NONE", "map_type": map_type,
         "allow_cycling": map_type != "MAP_TYPE_INDOOR", "allow_escaping": False, "allow_running": True,
         "show_map_name": show_name, "floor_number": 0, "battle_scene": "MAP_BATTLE_SCENE_NORMAL",
         "connections": None, "object_events": objects, "warp_events": warps, "coord_events": [],
         "bg_events": bgs}
    (d / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    if not (d / "scripts.inc").exists():
        (d / "scripts.inc").write_text(f"{name}_MapScripts::\n\t.byte 0\n")


def main():
    m = build_tileset()
    register_tileset()

    # Fuchsia: the door (the Safari Office's door metatiles) and the sign; its warp is the last
    fl = json.loads((ROOT / "data/layouts/layouts.json").read_text())
    fuchsia = next(l for l in fl["layouts"] if l["id"] == "LAYOUT_FUCHSIA_CITY")
    path = ROOT / fuchsia["blockdata_filepath"]
    data = bytearray(path.read_bytes())
    fw = fuchsia["width"]
    def put(x, y, v):
        struct.pack_into("<H", data, (y * fw + x) * 2, v)
    put(FUCHSIA_DOOR[0], FUCHSIA_DOOR[1] - 1, blk(0x2D6, 1, 0))   # (the wall above: solid)
    put(FUCHSIA_DOOR[0], FUCHSIA_DOOR[1], blk(0x2BF, 1, 0))
    put(FUCHSIA_SIGN[0], FUCHSIA_SIGN[1], blk(0x002, 1, 0))
    path.write_bytes(data)
    fmap_path = ROOT / "data/maps/FuchsiaCity_Frlg/map.json"
    fmap = json.loads(fmap_path.read_text())
    fmap["warp_events"] = [w for w in fmap["warp_events"] if w["dest_map"] != "MAP_EXOTIC_SAFARI_HALL"]
    fmap["warp_events"].append({"x": FUCHSIA_DOOR[0], "y": FUCHSIA_DOOR[1], "elevation": 0,
                                "dest_map": "MAP_EXOTIC_SAFARI_HALL", "dest_warp_id": "0"})
    fuchsia_warp = len(fmap["warp_events"]) - 1
    fmap["bg_events"] = [b for b in fmap["bg_events"] if b.get("script") != "FuchsiaCity_EventScript_ExoticSafariSign"]
    fmap["bg_events"].append({"type": "sign", "x": FUCHSIA_SIGN[0], "y": FUCHSIA_SIGN[1], "elevation": 0,
                              "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
                              "script": "FuchsiaCity_EventScript_ExoticSafariSign"})
    fmap_path.write_text(json.dumps(fmap, indent=2, ensure_ascii=False) + "\n")

    # The hall: warps 0-2 the mat (to Fuchsia), then one per open zone's door
    hall = build_hall(m)
    write_layout("ExoticSafari_Hall", "LAYOUT_EXOTIC_SAFARI_HALL", hall, "gTileset_BuildingFrlg", "gTileset_ExoticSafari")
    warps = [{"x": x, "y": HALL_H - 2, "elevation": 3, "dest_map": "MAP_FUCHSIA_CITY", "dest_warp_id": str(fuchsia_warp)}
             for x in (MAT_X - 1, MAT_X, MAT_X + 1)]
    bgs, zone_warp = [], {}
    for i, name in enumerate(REGIONS):
        x = door_x(i)
        if name in ZONES:
            zone_warp[name] = len(warps)
            warps.append({"x": x, "y": DOOR_Y, "elevation": 3, "dest_map": ZONES[name]["id"], "dest_warp_id": "0"})
        bgs.append({"type": "sign", "x": x, "y": 3, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH",
                    "script": f"ExoticSafari_Hall_EventScript_Door{name.title()}"})
    write_map("ExoticSafari_Hall", "MAP_EXOTIC_SAFARI_HALL", "LAYOUT_EXOTIC_SAFARI_HALL", "MUS_RG_FUCHSIA",
              "MAPSEC_FUCHSIA_CITY", "MAP_TYPE_INDOOR",
              [obj("OBJ_EVENT_GFX_WORKER_M", MAT_X, COUNTER_Y, "ExoticSafari_Hall_EventScript_Attendant",
                   flag="FLAG_EXOTIC_SAFARI_PASS"),
               obj("OBJ_EVENT_GFX_WORKER_M", MAT_X - 1, COUNTER_Y - 1, "ExoticSafari_Hall_EventScript_AttendantDesk",
                   flag="FLAG_HIDE_EXOTIC_SAFARI_DESK"),
               obj("OBJ_EVENT_GFX_CLERK", KIOSK_X, COUNTER_Y - 1, "ExoticSafari_Hall_EventScript_Kiosk")],
              warps, bgs, False)

    # The zones
    for name, z in ZONES.items():
        g, W, H = build_meadow()
        lid = "LAYOUT_" + z["id"][4:]
        write_layout(z["map"], lid, g, "gTileset_General_Frlg", "gTileset_ViridianForest")
        write_map(z["map"], z["id"], lid, "MUS_RG_ROUTE24", "MAPSEC_SAFARI_ZONE", "MAP_TYPE_ROUTE", [],
                  [{"x": x, "y": MEADOW_EXIT, "elevation": 3, "dest_map": "MAP_EXOTIC_SAFARI_HALL",
                    "dest_warp_id": str(zone_warp[name])} for x in (9, 10, 11)], [], True)

    # Constants for the hall's scripts, and the doors for src/exotic_safari.c
    consts = ["// The Exotic Pokémon Safari (made by tools/gym_hack/build_exotic_safari.py)",
              "#ifndef GUARD_CONSTANTS_EXOTIC_SAFARI_H", "#define GUARD_CONSTANTS_EXOTIC_SAFARI_H",
              f"#define EXOTIC_SAFARI_PASS_PRICE      10000",
              f"#define LOCALID_EXOTIC_SAFARI_ATTENDANT 1",
              f"#define LOCALID_EXOTIC_SAFARI_DESK      2",
              "#endif"]
    (ROOT / "include/constants/exotic_safari.h").write_text("\n".join(consts) + "\n")
    c = ["// The Exotic Pokémon Safari's doors (made by tools/gym_hack/build_exotic_safari.py)",
         f"#define EXOTIC_SAFARI_DOOR_COUNT {len(REGIONS)}",
         f"#define EXOTIC_SAFARI_DOOR_Y {DOOR_Y}",
         "static const u8 sDoorX[EXOTIC_SAFARI_DOOR_COUNT] = {" + ", ".join(str(door_x(i)) for i in range(len(REGIONS))) + "};",
         "static const bool8 sDoorHasZone[EXOTIC_SAFARI_DOOR_COUNT] = {" + ", ".join("TRUE" if n in ZONES else "FALSE" for n in REGIONS) + "};",
         "// top left, top middle, top right, door left, door middle, door right",
         "static const u16 sDoorOpen[6] = {" + ", ".join(f"0x{v:03X}" for v in (m["top_l"], m["top_m"], m["top_r"], 0x28B, 0x28C, 0x28D)) + "};",
         "static const u16 sDoorClosed[EXOTIC_SAFARI_DOOR_COUNT][6] =", "{"]
    for name in REGIONS:
        c.append("    {" + ", ".join(f"0x{m[f'closed_{name}_{k}']:03X}" for k in ("top_l", "top_m", "top_r", "door_l", "door_m", "door_r")) + f"}},  // {name}")
    c.append("};")
    (ROOT / "src/data/exotic_safari_doors.h").write_text("\n".join(c) + "\n")

    # Map group, script includes
    gp = ROOT / "data/maps/map_groups.json"
    groups = json.loads(gp.read_text())
    names = ["ExoticSafari_Hall"] + [z["map"] for z in ZONES.values()]
    if "gMapGroup_ExoticSafari" not in groups["group_order"]:
        groups["group_order"].append("gMapGroup_ExoticSafari")
    groups["gMapGroup_ExoticSafari"] = names
    gp.write_text(json.dumps(groups, indent=2) + "\n")
    (ROOT / "data/maps/exotic_safari.inc").write_text(
        "@ The Exotic Pokémon Safari's maps (tools/gym_hack/build_exotic_safari.py)\n"
        + "".join(f'\t.include "data/maps/{n}/scripts.inc"\n' for n in names))

    # Wild Pokémon
    wp = ROOT / "src/data/wild_encounters.json"
    wild = json.loads(wp.read_text())
    group = wild["wild_encounter_groups"][0]
    for name, z in ZONES.items():
        group["encounters"] = [e for e in group["encounters"] if e.get("map") != z["id"]]
        group["encounters"].append({"map": z["id"], "base_label": z["label"] + "_FireRed",
                                    "land_mons": {"encounter_rate": 21, "mons": [
                                        {"min_level": lo, "max_level": hi, "species": "SPECIES_" + s}
                                        for s, lo, hi in z["land"]]}})
    wp.write_text(json.dumps(wild, indent=2, ensure_ascii=False) + "\n")
    print(f"exotic safari: hall {HALL_W}x{HALL_H}, {len(ZONES)} zone(s), Fuchsia warp {fuchsia_warp}")


if __name__ == "__main__":
    main()
