#!/usr/bin/env python3
"""Build the expanded Pallet Town map for the Gym Trainer hack.

Usage: python3 tools/gym_hack/build_pallet_town.py [preview.png]

Writes:
  data/layouts/PalletTown_Frlg/map.bin (and the size in data/layouts/layouts.json)
  data/tilesets/secondary/pallet_town_frlg/metatiles.bin + metatile_attributes.bin
    (the original 89 metatiles, plus extra ones this script composes: roofs over grass,
    the gym over rock, the bridge, recolored roofs, rocky rims on the path, decorations)
  data/tilesets/secondary/pallet_town_frlg/tiles.png and palettes/07.pal, 12.pal
    (the original 80 tiles plus decoration art from pallet_art.py and recolored roof tiles)
and, with an argument, a preview PNG with a coordinate grid.

The map is described below in plain coordinates (x to the right, y down, in 16x16 blocks).
Edit the LAYOUT section and re-run to move things around.
"""
import json
import struct
import sys
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
import render_map as rm  # noqa: E402

LAYOUT_BIN = ROOT / "data/layouts/PalletTown_Frlg/map.bin"
SEC = ROOT / "data/tilesets/secondary/pallet_town_frlg"
PRIM = ROOT / "data/tilesets/primary/general_frlg"
ORIGINAL_SECONDARY_METATILES = 89
FIRST_CUSTOM = 0x280 + ORIGINAL_SECONDARY_METATILES

W, H = 60, 52

# ---------------------------------------------------------------------------
# Custom metatiles (appended to the Pallet Town tileset)
# ---------------------------------------------------------------------------
LAYER_NORMAL, LAYER_COVERED, LAYER_SPLIT = 0, 1, 2


def read_metatiles(path):
    data = path.read_bytes()
    return [list(struct.unpack_from("<8H", data, i)) for i in range(0, len(data), 16)]


def read_attrs(path):
    data = path.read_bytes()
    return [struct.unpack_from("<I", data, i)[0] for i in range(0, len(data), 4)]


PRIM_MT = read_metatiles(PRIM / "metatiles.bin")
PRIM_ATTR = read_attrs(PRIM / "metatile_attributes.bin")
SEC_MT = read_metatiles(SEC / "metatiles.bin")[:ORIGINAL_SECONDARY_METATILES]
SEC_ATTR = read_attrs(SEC / "metatile_attributes.bin")[:ORIGINAL_SECONDARY_METATILES]


ORIGINAL_SECONDARY_TILES = 80
DECOR_PAL = 7   # decorations + purple roof
ROOF_PAL = 12   # blue roof (indices 10-14) + green roof (indices 1,2,3,8,9 on copied tiles)

from PIL import Image  # noqa: E402
import pallet_art  # noqa: E402
import berry_field_art  # noqa: E402

_prim_tiles_img = Image.open(PRIM / "tiles.png")
_sec_tiles_img = Image.open(SEC / "tiles.png")
SEC_TILES = []  # list of 64-tuples of palette indices
for _i in range(ORIGINAL_SECONDARY_TILES):
    _x, _y = (_i % 16) * 8, (_i // 16) * 8
    SEC_TILES.append(tuple(_sec_tiles_img.crop((_x, _y, _x + 8, _y + 8)).getdata()))
_tile_ids = {}


def add_tile(pixels):
    """Add an 8x8 tile (64 palette indices) to the secondary tileset; returns its tile number."""
    pixels = tuple(pixels)
    if pixels not in _tile_ids:
        SEC_TILES.append(pixels)
        _tile_ids[pixels] = 0x280 + len(SEC_TILES) - 1
    return _tile_ids[pixels]


def tile_pixels(t):
    img, local = (_prim_tiles_img, t) if t < 0x280 else (_sec_tiles_img, t - 0x280)
    x, y = (local % 16) * 8, (local // 16) * 8
    return tuple(img.crop((x, y, x + 8, y + 8)).getdata())


def entries(mid):
    return list(PRIM_MT[mid] if mid < 0x280 else SEC_MT[mid - 0x280])


def attr(mid):
    return PRIM_ATTR[mid] if mid < 0x280 else SEC_ATTR[mid - 0x280]


custom = {}  # name -> metatile id


def add_custom(name, ents, attribute):
    if name not in custom:
        custom[name] = 0x280 + len(SEC_MT)
        SEC_MT.append(list(ents))
        SEC_ATTR.append(attribute)
    return custom[name]


def with_layer(attribute, layer):
    return (attribute & ~0x60000000) | (layer << 29)


GRASS_BASE = entries(0x008)[:4]
ROCK_BASE = entries(0x071)[4:]   # the rock plateau's (opaque) surface tiles
PATH_BASE = entries(0x296)[:4]


def over(base, mid, name=None):
    """Metatile `mid` with its ground (first layer) swapped for `base` tiles."""
    e = entries(mid)
    return add_custom(name or f"{mid:x}@{id(base)}", base + e[4:], attr(mid))


def planks_over_path(mid):
    """Bridge planks (top layer, drawn over anyone walking under) above the path."""
    return add_custom(f"bridge{mid:x}", PATH_BASE + entries(mid)[:4], with_layer(0, LAYER_NORMAL))


GREEN_REMAP = {10: 1, 11: 2, 12: 3, 13: 8, 14: 9}


def roof(mid, color):
    """Metatile `mid` with its red roof (primary palette 2) repainted."""
    if color == "red":
        return mid
    out = []
    for e in entries(mid):
        t, pal, flips = e & 0x3FF, e >> 12, e & 0xC00
        if pal == 2:
            if color == "green":
                t = add_tile(GREEN_REMAP.get(c, c) for c in tile_pixels(t))
            pal = DECOR_PAL if color == "purple" else ROOF_PAL
        out.append(t | flips | (pal << 12))
    return add_custom(f"{mid:x}:{color}", out, attr(mid))


def on_path(mid):
    """A rocky rim piece standing on the path instead of grass."""
    return add_custom(f"{mid:x}:path", PATH_BASE + entries(mid)[4:], attr(mid))


def decor_metatile(ground_mid, tiles, behind):
    """Decoration tiles (palette 7) over an ordinary ground block. `behind`: drawn over people
    (the parts you walk behind); otherwise drawn under them, so someone standing just below
    isn't covered."""
    art = [(t if t else 0) | (DECOR_PAL << 12 if t else 0) for t in tiles]
    layer = LAYER_NORMAL if behind else LAYER_COVERED
    return add_custom(f"decor:{ground_mid:x}:{tiles}:{behind}", entries(ground_mid)[:4] + art, with_layer(0, layer))


BERRY_FIELD = {}    # plot stage name (berry_field_art.stages) -> metatile


def berry_field_metatiles():
    """one metatile per plot stage: the soil and plant on the bottom layer, nothing over it"""
    for name, (pal, g) in berry_field_art.stages().items():
        ts = []
        for ty in range(2):
            for tx in range(2):
                px = [g[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8)]
                ts.append(add_tile(px) | (pal << 12))
        BERRY_FIELD[name] = add_custom(f"berry:{name}", ts + [0, 0, 0, 0], with_layer(0, LAYER_NORMAL))


def sprite_tiles(name):
    """Cut a pallet_art sprite into blocks: {(bx, by): [4 tile numbers, 0 = empty]}."""
    img = pallet_art.SPRITES[name]()
    blocks = {}
    for by in range(img.height // 16):
        for bx in range(img.width // 16):
            ts = []
            for ty in range(2):
                for tx in range(2):
                    x, y = bx * 16 + tx * 8, by * 16 + ty * 8
                    px = tuple(img.crop((x, y, x + 8, y + 8)).getdata())
                    ts.append(add_tile(px) if any(px) else 0)
            blocks[(bx, by)] = ts
    return blocks


# ---------------------------------------------------------------------------
# Map grid
# ---------------------------------------------------------------------------
mt = [[None] * W for _ in range(H)]
coll = [[0] * W for _ in range(H)]
elev = [[3] * W for _ in range(H)]
ground = [["P"] * W for _ in range(H)]  # P path, G grass, X something else (counts as grass edge)
trees = set()  # top-left corners of 2x2 trees


rims = {}           # rocky rim cells -> their metatile (may move onto the path later)
raised_cells = set()
decor = []          # (x, y, sprite name, walkable rows)


def put(x, y, m, c=1, z=0, g="X"):
    mt[y][x], coll[y][x], elev[y][x], ground[y][x] = m, c, z, g
    rims.pop((x, y), None)


def walk(x, y, m, z=3, g="X"):
    put(x, y, m, 0, z, g)


def grass(x0, y0, x1, y1):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if ground[y][x] in "PG":
                ground[y][x] = "G"


def tree(x, y):
    trees.add((x, y))
    for dy in range(2):
        for dx in range(2):
            ground[y + dy][x + dx] = "T"


def tree_rect(x0, y0, x1, y1):
    """Fill with 2x2 trees on the even grid (x1/y1 inclusive, block corners)."""
    for y in range(y0, y1 + 1, 2):
        for x in range(x0, x1 + 1, 2):
            tree(x, y)


def stamp(x, y, rows, collide_rows=None, z=0, walkable=()):
    for dy, row in enumerate(rows):
        for dx, m in enumerate(row):
            if (dx, dy) in walkable:
                walk(x + dx, y + dy, m)
            else:
                put(x + dx, y + dy, m)


def house(x, y, color="red"):
    """Two-story Pallet house. (x, y) = top-left of the roof edge row. Returns the door.
    color: red, blue, green or purple roof."""
    top = [roof(over(GRASS_BASE, m, f"house_top{m:x}"), color) for m in (0x281, 0x282, 0x282, 0x282, 0x283)]
    for dx, m in enumerate(top):
        walk(x + dx, y, m)
    r = lambda m: roof(m, color)  # noqa: E731
    stamp(x, y + 1, [
        [r(0x289), r(0x28A), r(0x28A), r(0x28A), r(0x28B)],
        [r(0x291), r(0x293), r(0x292), r(0x292), r(0x294)],
        [0x298, 0x299, 0x29A, 0x29B, 0x29C],
        [0x2A0, 0x2A3, 0x2A2, 0x2A1, 0x2A4],
    ])
    return (x + 1, y + 4)


def mailbox(x, y):
    walk(x, y, 0x2A5)
    put(x, y + 1, 0x2AD)


def lab(x, y):
    """Oak's lab, 7 wide. (x, y) = top-left of the roof edge row. Returns the door."""
    top = [over(GRASS_BASE, m, f"lab_top{m:x}") for m in (0x2A8, 0x2A9, 0x2A9, 0x2A9, 0x2A9, 0x2BD, 0x2B5)]
    for dx, m in enumerate(top):
        walk(x + dx, y, m)
    stamp(x, y + 1, [
        [0x2B0, 0x2B1, 0x2B1, 0x2B1, 0x2B1, 0x2B3, 0x2B4],
        [0x2B8, 0x2B9, 0x2B9, 0x2B9, 0x2B9, 0x2BB, 0x2BC],
        [0x2C0, 0x2C1, 0x2D0, 0x2C2, 0x2C3, 0x2C4, 0x2C5],
        [0x2C8, 0x2C9, 0x2D8, 0x2AC, 0x2CB, 0x2CC, 0x2CD],
    ])
    return (x + 3, y + 4)


def pokemon_center(x, y):
    for dx, m in enumerate((0x17B, 0x17C, 0x17D, 0x17E, 0x17F)):
        walk(x + dx, y, m)
    stamp(x, y + 1, [
        [0x048, 0x049, 0x04A, 0x04B, 0x187],
        [0x050, 0x051, 0x052, 0x053, 0x18F],
        [0x058, 0x059, 0x05A, 0x05B, 0x197],
        [0x060, 0x061, 0x062, 0x186, 0x19F],
    ])
    return (x + 2, y + 4)


def mart(x, y):
    for dx, m in enumerate((0x028, 0x029, 0x02A, 0x02B)):
        walk(x + dx, y, m)
    stamp(x, y + 1, [
        [0x030, 0x031, 0x032, 0x033],
        [0x038, 0x039, 0x03A, 0x03B],
        [0x040, 0x041, 0x062, 0x063],
    ])
    return (x + 2, y + 3)


def gym(x, y):
    """The player's gym, 7 wide, on the rock plateau. Returns the door."""
    for dx, m in enumerate((0x139, 0x13A, 0x13A, 0x13A, 0x13A, 0x13A, 0x13B)):
        walk(x + dx, y, over(ROCK_BASE, m, f"gym{m:x}"))
    stamp(x, y + 1, [
        [0x141, 0x142, 0x142, 0x142, 0x142, 0x142, 0x143],
        [0x149, 0x14A, 0x14A, 0x14A, 0x14A, 0x14A, 0x14B],
        [0x150, 0x151, 0x152, 0x153, 0x154, 0x155, 0x156],
    ])
    bottom = [0x158, 0x159, 0x15A, 0x15B, 0x15C, 0x15D, 0x15E]
    for dx, m in enumerate(bottom):
        m2 = over(ROCK_BASE, m, f"gym{m:x}")
        if dx in (0, 1, 5, 6):
            walk(x + dx, y + 4, m2)
        else:
            put(x + dx, y + 4, m2)
    global GYM_DOOR_OPEN, GYM_DOOR_LOCKED
    GYM_DOOR_OPEN = custom["gym15b"]
    # The same door that doesn't open (swapped in until Oak has shown the player the gym)
    GYM_DOOR_LOCKED = add_custom("gym door locked", entries(GYM_DOOR_OPEN), attr(GYM_DOOR_OPEN) & ~0x1FF)
    return (x + 3, y + 4)


def pond(x0, y0, x1, y1, rim_row=True, openings=()):
    """Water from (x0,y0) to (x1,y1) with a rock rim, plus a grassy rim row below it.
    `openings`: x positions on the bottom rim row that stay water (e.g. a waterfall)."""
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if y == y0:
                m = 0x122 if x == x0 else 0x124 if x == x1 else 0x123
            else:
                m = 0x12A if x == x0 else 0x12C if x == x1 else 0x12B
            put(x, y, m, 0, 1)
    if rim_row:
        y = y1 + 1
        for x in range(x0, x1 + 1):
            if x in openings:
                put(x, y, 0x12B, 0, 1)
            else:
                walk(x, y, 0x132 if x == x0 else 0x134 if x == x1 else 0x133, g="G")


def raised(x0, y0, x1, y1, interior, z, stairs=(), top=True):
    """A raised block with a rocky rim. interior: 'grass' or 'rock'.
    The bottom row is the cliff face; `stairs` lists x positions of stairs in it."""
    zs = 0 if z != 3 else 3  # stairs between two different heights are transitions

    def rim(x, y, m):
        put(x, y, m)
        rims[(x, y)] = m

    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            raised_cells.add((x, y))
            left, right = x == x0, x == x1
            if y == y0 and top:
                rim(x, y, 0x68 if left else 0x6A if right else 0x69)
            elif y == y1:
                if x in stairs:
                    walk(x, y, 0x91 if x == min(stairs) else 0x89, z=zs)
                    rims[(x, y)] = None  # stairs: let the path run up to them
                else:
                    rim(x, y, 0x78 if left else 0x7A if right else 0x79)
            elif left:
                rim(x, y, 0x70)
            elif right:
                rim(x, y, 0x72)
            elif interior == "rock":
                walk(x, y, 0x71, z=z)
            else:
                ground[y][x] = "G"
                elev[y][x] = z


def cliff_band(x0, x1, y0, y1, stairs=(), falls=()):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if x in stairs:
                walk(x, y, 0x91 if x == min(stairs) else 0x89)
            elif x in falls:
                m = 0x127 if y == y0 else 0x137 if y == y1 else 0x12F
                put(x, y, m, 0, 1)
            else:
                put(x, y, 0x7C)


def flowers(x0, y0, x1, y1):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            walk(x, y, 0x004, g="G")


def fence(x0, x1, y, m=0x284):
    for x in range(x0, x1 + 1):
        put(x, y, m)


def place(name, x, y, walkable_rows=1):
    """Put a decoration from pallet_art at (x, y) (its top-left block). The top
    `walkable_rows` rows can be walked behind; the rest block the way."""
    decor.append((x, y, name, walkable_rows))
    img = pallet_art.SPRITES[name]()
    for by in range(img.height // 16):
        for bx in range(img.width // 16):
            if by >= walkable_rows:
                coll[y + by][x + bx] = 1


def statue(x, y):
    place("chansey", x, y)
    return (x, y + 2)


def lamp(x, y):
    place("lamp", x, y)


def bench(x, y):
    place("bench", x, y, walkable_rows=0)


# ---------------------------------------------------------------------------
# LAYOUT
# ---------------------------------------------------------------------------
NORTH_EXIT = (20, 21)
# Route 21's water channel (its x 7..10) lines up with these columns
CHANNEL = (30, 33)

# Borders
tree_rect(0, 0, 18, 0)
tree_rect(22, 0, 58, 0)
tree_rect(0, 2, 0, 36)
tree_rect(58, 2, 58, 36)

# Top-left yard (neighbor) and top-right yard (rival), joined by a bridge over the north road
YARD_Z = 4
raised(4, 3, 17, 12, "grass", YARD_Z, stairs=(10, 11))
raised(23, 3, 37, 12, "grass", YARD_Z, stairs=(31, 32))
NEIGHBOR_DOOR = house(9, 4, "green")
mailbox(8, 7)
RIVAL_DOOR = house(30, 4, "blue")
mailbox(29, 7)
flowers(6, 5, 7, 6)
flowers(15, 5, 16, 6)
flowers(35, 5, 36, 6)
pond(25, 5, 27, 6)
for y in (7, 8):
    for x in range(17, 24):
        if x in (17, 23):
            walk(x, y, 0x1BE if y == 7 else 0x1C6, z=YARD_Z)
        else:
            walk(x, y, planks_over_path(0x1BE if y == 7 else 0x1C6), z=15, g="P")
for y in range(4, 12):  # the yards stand at height 4
    for x in list(range(5, 17)) + list(range(24, 37)):
        if elev[y][x] == 3:
            elev[y][x] = YARD_Z
# Roof edges behind the houses: neutral height so the player is drawn behind the roof
for x in range(9, 14):
    elev[4][x] = 0
for x in range(30, 35):
    elev[4][x] = 0
elev[7][8] = elev[7][29] = 0  # mailbox tops

TOWN_SIGN = (23, 13)
put(*TOWN_SIGN, 0x003)

# Middle-left house (second neighbor)
grass(3, 17, 13, 23)
HOUSE2_DOOR = house(6, 17, "purple")
mailbox(5, 20)
flowers(11, 19, 12, 21)

# The gym plateau
raised(14, 16, 26, 25, "rock", 3, stairs=(19, 20, 21))
GYM_DOOR = gym(17, 17)
grass(16, 26, 17, 27)
GYM_SIGN = (17, 27)
walk(17, 26, 0x160)
put(*GYM_SIGN, 0x168)

# Oak's lab on its grassy rise
raised(29, 16, 46, 31, "grass", 3, stairs=(38, 39))
LAB_DOOR = lab(35, 23)
pond(41, 18, 44, 19)
tree(31, 18)
tree(31, 26)
tree(43, 26)
flowers(40, 28, 42, 29)
LAB_SIGN = (34, 28)
put(*LAB_SIGN, 0x003)

# Garden with the trainer tips sign
grass(10, 28, 17, 32)
fence(11, 15, 28)
TIPS_SIGN = (16, 28)
put(*TIPS_SIGN, 0x002)
flowers(11, 29, 14, 30)

# Pokemon Center and Mart
grass(48, 6, 56, 11)
PC_DOOR = pokemon_center(50, 6)
grass(49, 14, 56, 19)
MART_DOOR = mart(51, 15)
# The post-game training hall (its trainers: tools/gym_hack/build_training_houses.py), in a clearing
# of the woods east of the lab
grass(48, 22, 53, 29)
HALL_DOOR = house(49, 22, "red")
HALL_SIGN = (48, 26)
put(*HALL_SIGN, 0x003)
flowers(52, 27, 53, 28)
tree_rect(54, 22, 56, 30)
tree_rect(48, 30, 52, 30)

# Lake, cliffs and the sea
grass(2, 33, 57, 37)
fence(14, 47, 33, 0x284)
pond(16, 34, 47, 36, openings=(19, 20, 21))
tree_rect(2, 34, 10, 34)
tree_rect(48, 34, 56, 34)
cliff_band(2, 57, 38, 42, stairs=(12, 13), falls=(19, 20, 21))
# Peninsula with the player's house
grass(2, 43, 16, 51)
PLAYER_DOOR = house(5, 44)
mailbox(4, 47)
flowers(11, 45, 13, 46)
tree_rect(0, 38, 0, 50)
# The sea below the waterfall, draining south through Route 21's channel
SEA_BOTTOM = 47
for y in range(43, SEA_BOTTOM + 1):
    for x in range(17, 58):
        m = 0x12B
        if y == 43:
            m = 0x122 if x == 17 else 0x123
            if x in (19, 20, 21):
                m = 0x12B
        elif x == 17:
            m = 0x12A
        put(x, y, m, 0, 1)
for x in range(17, 58):
    if CHANNEL[0] <= x <= CHANNEL[1]:
        for y in range(SEA_BOTTOM + 1, H):
            put(x, y, 0x12A if x == CHANNEL[0] else 0x12C if x == CHANNEL[1] else 0x12B, 0, 1)
    else:
        rim_m = 0x132 if x in (17, CHANNEL[1] + 1) else 0x134 if x == CHANNEL[0] - 1 else 0x133
        walk(x, SEA_BOTTOM + 1, rim_m, g="G")
        grass(x, SEA_BOTTOM + 2, x, H - 1)
tree_rect(34, 50, 56, 50)
# The berry field (src/berry_field.c): two rows of plots below the sea, a path between them, its sign
BERRY_FIELD_X0, BERRY_FIELD_X1 = 18, 29
BERRY_FIELD_ROWS = (49, 51)
BERRY_FIELD_SIGN = (17, 49)
berry_field_metatiles()
for _y in BERRY_FIELD_ROWS:
    for _x in range(BERRY_FIELD_X0, BERRY_FIELD_X1 + 1):
        put(_x, _y, BERRY_FIELD["soil"], g="G")
put(*BERRY_FIELD_SIGN, 0x002, g="G")
tree_rect(58, 38, 58, 50)

# Chansey statue plaza west of the Pokemon Center
STATUE = statue(42, 4)
flowers(40, 5, 41, 6)
flowers(44, 5, 45, 6)
lamp(39, 3)
lamp(46, 3)
bench(40, 9)
bench(44, 9)

# Benches and street lamps around town
lamp(18, 10)
lamp(22, 10)
lamp(3, 24)
bench(5, 26)
flowers(8, 25, 9, 26)
bench(10, 26)
lamp(13, 24)
lamp(23, 26)
lamp(49, 9)
lamp(49, 16)
lamp(10, 47)
bench(12, 49)
lamp(46, 13)
bench(2, 14)

# ---------------------------------------------------------------------------
# Finishing: trees, and path/grass edges
# ---------------------------------------------------------------------------
TREE = {"tl": 0x1C, "tr": 0x1D, "bl": 0x14, "br": 0x15}


def finish_trees():
    for (x, y) in trees:
        def has(dx, dy):
            return (x + dx, y + dy) in trees or not (0 <= x + dx < W and 0 <= y + dy < H)
        left, right, below = has(-2, 0), has(2, 0), has(0, 2)
        put(x, y, 0x1C if left else 0x1E)
        put(x + 1, y, 0x1D if right else 0x1F)
        if below:
            bl, br = (0x14 if left else 0x16), (0x15 if right else 0x17)
        else:
            bl, br = (0x24 if left else 0x26), (0x25 if right else 0x27)
        put(x, y + 1, bl)
        put(x + 1, y + 1, br)
        for dx in range(2):
            for dy in range(2):
                ground[y + dy][x + dx] = "X"
        # treetops peeking over the block above
        if not has(0, -2) and y > 0:
            for dx, m in ((0, 0x00E), (1, 0x00F)):
                if mt[y - 1][x + dx] is None and ground[y - 1][x + dx] in "PG" and not coll[y - 1][x + dx]:
                    walk(x + dx, y - 1, m, g="G")
    # treetops of the trees just past the bottom edge (the map border, and Route 21)
    route21 = json.loads((ROOT / "data/layouts/layouts.json").read_text())
    r21 = next(l for l in route21["layouts"] if l.get("id") == "LAYOUT_ROUTE21_NORTH")
    r21_top = struct.unpack(f"<{r21['width']}H", (ROOT / r21["blockdata_filepath"]).read_bytes()[:r21["width"] * 2])
    for x in range(W):
        rx = x - (CHANNEL[0] - 7)
        if 0 <= rx < r21["width"]:
            below = r21_top[rx] & 0x3FF
            tip = {0x1C: 0x00E, 0x1E: 0x00E, 0x1D: 0x00F, 0x1F: 0x00F}.get(below)
        else:
            tip = 0x00E if x % 2 == 0 else 0x00F
        if tip and mt[H - 1][x] is None and ground[H - 1][x] in "PG" and not coll[H - 1][x]:
            walk(x, H - 1, tip, g="G")


def finish_rims():
    """Rocky rims next to the path stand on the path, so it runs right up to them."""
    for (x, y), m in list(rims.items()):
        outside = [(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
                   if 0 <= x + dx < W and 0 <= y + dy < H and (x + dx, y + dy) not in raised_cells]
        if outside and all(ground[ny][nx] == "P" for nx, ny in outside):
            if m is not None:
                mt[y][x] = on_path(m)
            ground[y][x] = "P"


def finish_decor():
    for x, y, name, walkable_rows in decor:
        for (bx, by), ts in sprite_tiles(name).items():
            mt[y + by][x + bx] = decor_metatile(mt[y + by][x + bx], ts, by < walkable_rows)


# corner pattern (TL, TR, BL, BR; True = path) -> metatile
EDGES = {
    (1, 1, 1, 1): 0x296,
    (0, 1, 0, 1): 0x2AE, (1, 0, 1, 0): 0x297, (0, 0, 1, 1): 0x28E, (1, 1, 0, 0): 0x29E,
    (1, 1, 1, 0): 0x2C7, (1, 1, 0, 1): 0x2C6, (1, 0, 1, 1): 0x2BF, (0, 1, 1, 1): 0x2BE,
    (0, 0, 0, 1): 0x2A6, (0, 0, 1, 0): 0x28F, (0, 1, 0, 0): 0x2B6, (1, 0, 0, 0): 0x29F,
}
GRASS = [[0x008, 0x009], [0x010, 0x011]]


def finish_ground():
    def is_path(x, y):
        if not (0 <= x < W and 0 <= y < H):
            return False
        return ground[y][x] == "P"

    def vertex(vx, vy):  # path only if every touching cell is path
        return all(is_path(vx - dx, vy - dy) for dx in (0, 1) for dy in (0, 1))

    warnings = []
    for y in range(H):
        for x in range(W):
            if mt[y][x] is not None:
                continue
            if ground[y][x] == "P":
                key = (vertex(x, y), vertex(x + 1, y), vertex(x, y + 1), vertex(x + 1, y + 1))
                key = tuple(int(k) for k in key)
                if key == (0, 0, 0, 0):
                    m = GRASS[y % 2][x % 2]
                elif key in EDGES:
                    m = EDGES[key]
                else:
                    warnings.append((x, y))
                    m = 0x296
                mt[y][x] = m
            else:
                mt[y][x] = GRASS[y % 2][x % 2]
    for w in warnings:
        print("path corner that has no matching tile at", w)


finish_trees()
finish_rims()
finish_ground()
finish_decor()


# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Events (warps, people, signs, triggers) and Oak's walking routes
# ---------------------------------------------------------------------------
def front(door):
    return (door[0], door[1] + 1)


# Warp ids 0-2 are the ones the original interiors already point back to.
WARPS = [
    (PLAYER_DOOR, "MAP_PALLET_TOWN_PLAYERS_HOUSE_1F", 1),
    (RIVAL_DOOR, "MAP_PALLET_TOWN_RIVALS_HOUSE", 0),
    (LAB_DOOR, "MAP_PALLET_TOWN_PROFESSOR_OAKS_LAB", 0),
    (NEIGHBOR_DOOR, "MAP_PALLET_TOWN_HOUSE1", 0),
    (HOUSE2_DOOR, "MAP_PALLET_TOWN_HOUSE2", 0),
    (PC_DOOR, "MAP_PALLET_TOWN_POKEMON_CENTER_1F", 0),
    (MART_DOOR, "MAP_PALLET_TOWN_MART", 0),
    (GYM_DOOR, "MAP_PALLET_TOWN_GYM", 0),
    (HALL_DOOR, "MAP_PALLET_TOWN_TRAINING_HALL", 1),   # (the middle warp: the doormat)
]

OAK_START = (21, 13)          # where Oak appears before walking up to the player
OAK_RATING_START = (13, 43)   # where Oak appears after the Elite Four (foot of the stairs)
SIGN_LADY_HOME = (7, 24)
SIGN_LADY_AT_SIGN = (TIPS_SIGN[0], TIPS_SIGN[1] + 1)
SIGN_LADY_AT_EXIT = (NORTH_EXIT[0], 2)
FAT_MAN_HOME = (46, 12)
# Pokémon Canvas: MOM waits two steps below the lab door to send the player off (data/scripts/dad_story.inc)
MOM_SENDOFF = (LAB_DOOR[0], LAB_DOOR[1] + 3)


def obj(local_id, gfx, pos, move, rx, ry, script, flag="0"):
    o = {"type": "object", "graphics_id": gfx, "x": pos[0], "y": pos[1], "elevation": elev[pos[1]][pos[0]] or 3,
         "movement_type": move, "movement_range_x": rx, "movement_range_y": ry,
         "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0", "script": script, "flag": flag}
    return {"local_id": local_id, **o} if local_id else o


def sign(pos, script):
    return {"type": "sign", "x": pos[0], "y": pos[1], "elevation": 0,
            "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY", "script": script}


def write_map_json():
    path = ROOT / "data/maps/PalletTown_Frlg/map.json"
    m = json.loads(path.read_text())
    m["connections"] = [
        {"map": "MAP_ROUTE1", "offset": NORTH_EXIT[0] - 12, "direction": "up"},
        {"map": "MAP_ROUTE21_NORTH", "offset": CHANNEL[0] - 7, "direction": "down"},
    ]
    m["object_events"] = [
        obj("LOCALID_PALLET_SIGN_LADY", "OBJ_EVENT_GFX_WOMAN_1_FRLG", SIGN_LADY_HOME, "MOVEMENT_TYPE_WANDER_AROUND",
            2, 2, "PalletTown_EventScript_SignLady"),
        obj("LOCALID_PALLET_FAT_MAN", "OBJ_EVENT_GFX_FAT_MAN_FRLG", FAT_MAN_HOME, "MOVEMENT_TYPE_WANDER_AROUND",
            3, 2, "PalletTown_EventScript_FatMan"),
        obj("LOCALID_PALLET_PROF_OAK", "OBJ_EVENT_GFX_PROF_OAK", OAK_START, "MOVEMENT_TYPE_FACE_UP",
            1, 1, "0x0", "FLAG_HIDE_OAK_IN_PALLET_TOWN"),
        obj("LOCALID_PALLET_SENDOFF_MOM", "OBJ_EVENT_GFX_MOM_FRLG", MOM_SENDOFF, "MOVEMENT_TYPE_FACE_UP",
            1, 1, "0x0", "FLAG_HIDE_SENDOFF_MOM"),
    ]
    m["warp_events"] = [{"x": d[0], "y": d[1], "elevation": 0, "dest_map": dest, "dest_warp_id": str(w)}
                        for d, dest, w in WARPS]
    m["coord_events"] = [
        {"type": "trigger", "x": NORTH_EXIT[0], "y": 1, "elevation": 3, "var": "VAR_MAP_SCENE_PALLET_TOWN_OAK",
         "var_value": "0", "script": "PalletTown_EventScript_OakTriggerLeft"},
        {"type": "trigger", "x": NORTH_EXIT[1], "y": 1, "elevation": 3, "var": "VAR_MAP_SCENE_PALLET_TOWN_OAK",
         "var_value": "0", "script": "PalletTown_EventScript_OakTriggerRight"},
        {"type": "trigger", "x": NORTH_EXIT[1], "y": 2, "elevation": 3, "var": "VAR_TEMP_2",
         "var_value": "1", "script": "PalletTown_EventScript_SignLadyTrigger"},
    ]
    m["bg_events"] = [
        sign(LAB_SIGN, "PalletTown_EventScript_OaksLabSign"),
        sign((PLAYER_DOOR[0] - 2, PLAYER_DOOR[1]), "PalletTown_EventScript_PlayersHouseSign"),
        sign((RIVAL_DOOR[0] - 2, RIVAL_DOOR[1]), "PalletTown_EventScript_RivalsHouseSign"),
        sign((NEIGHBOR_DOOR[0] - 2, NEIGHBOR_DOOR[1]), "PalletTown_EventScript_NeighborsHouseSign"),
        sign((HOUSE2_DOOR[0] - 2, HOUSE2_DOOR[1]), "PalletTown_EventScript_House2Sign"),
        sign(TOWN_SIGN, "PalletTown_EventScript_TownSign"),
        sign(TIPS_SIGN, "PalletTown_EventScript_TrainerTips"),
        sign(GYM_SIGN, "PalletTown_EventScript_GymSign"),
        sign(HALL_SIGN, "PalletTown_EventScript_TrainingHallSign"),
        sign(GYM_DOOR, "PalletTown_EventScript_GymDoorLocked"),
        sign(BERRY_FIELD_SIGN, "BerryField_EventScript_Sign"),
        *[sign((x, y), "BerryField_EventScript_Plot") for y in BERRY_FIELD_ROWS
          for x in range(BERRY_FIELD_X0, BERRY_FIELD_X1 + 1)],
        sign((STATUE[0], STATUE[1]), "PalletTown_EventScript_ChanseyStatue"),
        sign((STATUE[0] + 1, STATUE[1]), "PalletTown_EventScript_ChanseyStatue"),
        # the Lucky Egg at the statue's base: listed after the statue's signs, so pressing A reads the
        # statue (whose script hands the egg over) and the hidden item is only there for the Itemfinder
        {"type": "hidden_item", "x": STATUE[0], "y": STATUE[1], "elevation": 0, "item": "ITEM_LUCKY_EGG",
         "flag": "FLAG_HIDDEN_ITEM_PALLET_TOWN_LUCKY_EGG", "quantity": 1, "underfoot": False},
    ]
    path.write_text(json.dumps(m, indent=2) + "\n")


DIRS = {(0, -1): "walk_up", (0, 1): "walk_down", (-1, 0): "walk_left", (1, 0): "walk_right"}


def route(start, goal, start_z=3):
    """Shortest walk on the ground (no water, no ledges), respecting heights."""
    blocked = {SIGN_LADY_HOME, SIGN_LADY_AT_SIGN, FAT_MAN_HOME}
    q = deque([(start, start_z)])
    prev = {(start, start_z): None}
    while q:
        (x, y), z = q.popleft()
        if (x, y) == goal:
            out, k = [], ((x, y), z)
            while k:
                out.append(k[0])
                k = prev[k]
            return out[::-1]
        for dx, dy in DIRS:
            nx, ny = x + dx, y + dy
            if not (0 <= nx < W and 0 <= ny < H) or coll[ny][nx] or elev[ny][nx] == 1 or (nx, ny) in blocked:
                continue
            nz = elev[ny][nx]
            if not (z == 0 or nz in (0, 15) or nz == z):
                continue
            keep = nz == 15 or z == 15
            state = ((nx, ny), z if keep else nz)
            if state not in prev:
                prev[state] = ((x, y), z)
                q.append(state)
    raise SystemExit(f"no route from {start} to {goal}")


def steps(cells):
    return [DIRS[(b[0] - a[0], b[1] - a[1])] for a, b in zip(cells, cells[1:])]


def movement(label, cmds):
    return f"{label}::\n" + "".join(f"\t{c}\n" for c in cmds) + "\tstep_end\n\n"


def write_movements():
    lab_front = front(LAB_DOOR)
    out = ["@ Generated by tools/gym_hack/build_pallet_town.py from the map layout. Don't edit by hand.\n\n",
           f".equ PALLET_LAB_DOOR_X, {LAB_DOOR[0]}\n.equ PALLET_LAB_DOOR_Y, {LAB_DOOR[1]}\n",
           f".equ PALLET_OAK_RATING_X, {OAK_RATING_START[0]}\n.equ PALLET_OAK_RATING_Y, {OAK_RATING_START[1]}\n",
           f".equ PALLET_SIGN_LADY_SIGN_X, {SIGN_LADY_AT_SIGN[0]}\n.equ PALLET_SIGN_LADY_SIGN_Y, {SIGN_LADY_AT_SIGN[1]}\n",
           f".equ PALLET_SIGN_LADY_EXIT_X, {SIGN_LADY_AT_EXIT[0]}\n.equ PALLET_SIGN_LADY_EXIT_Y, {SIGN_LADY_AT_EXIT[1]}\n",
           f".equ PALLET_GYM_DOOR_X, {GYM_DOOR[0]}\n.equ PALLET_GYM_DOOR_Y, {GYM_DOOR[1]}\n",
           f".equ PALLET_OAK_START_X, {OAK_START[0]}\n.equ PALLET_OAK_START_Y, {OAK_START[1]}\n",
           f".equ PALLET_MOM_SENDOFF_X, {MOM_SENDOFF[0]}\n.equ PALLET_MOM_SENDOFF_Y, {MOM_SENDOFF[1]}\n",
           f".equ METATILE_PALLET_GYM_DOOR_LOCKED, 0x{GYM_DOOR_LOCKED:X}\n\n"]

    def lead(prefix, oak_from, player_from):
        """Oak walks from oak_from to the lab door; the player follows one step behind."""
        oak = route(oak_from, lab_front)
        player = [player_from] + oak[:-1]
        out.append(movement(f"{prefix}_OakWalkToLab", steps(oak) + ["walk_in_place_faster_up"]))
        out.append(movement(f"{prefix}_PlayerWalkToLab", steps(player)))
        out.append(movement(f"{prefix}_PlayerEnterLab", steps([oak[-2], lab_front]) + ["walk_up", "set_invisible"]))

    # Start of the game: Oak stops the player at the north exit
    for side, x in (("Left", NORTH_EXIT[0]), ("Right", NORTH_EXIT[1])):
        meet = (x, 2)
        out.append(movement(f"PalletTown_Movement_OakEnter{side}", steps(route(OAK_START, meet)) + ["walk_in_place_faster_up"]))
        lead(f"PalletTown_Movement_{side}", meet, (x, 1))
    # After the Elite Four: Oak meets the player outside their house
    outside = front(PLAYER_DOOR)
    below = (outside[0], outside[1] + 1)
    walk_in = route(OAK_RATING_START, below)
    out.append(movement("PalletTown_Movement_OakWalkToPlayersDoor", steps(walk_in) + ["walk_in_place_faster_up"]))
    out.append(movement("PalletTown_Movement_OakExit", steps(walk_in[::-1])))
    lead("PalletTown_Movement_FromHouse", below, outside)
    # MOM's send-off: she steps up to the player outside the lab, then walks home
    beside = (lab_front[0], lab_front[1] + 1)
    out.append(movement("PalletTown_Movement_MomSendoffApproach", steps(route(MOM_SENDOFF, beside)) + ["walk_in_place_faster_up"]))
    # (the first eleven steps of her way home take her off the screen; the script removes her then)
    out.append(movement("PalletTown_Movement_MomSendoffGoHome", steps(route(beside, outside))[:11]))
    (ROOT / "data/maps/PalletTown_Frlg/movements.inc").write_text("".join(out))
    # The gym door's metatile, for its opening animation (src/field_door.c)
    (ROOT / "include/constants/pallet_town_gym.h").write_text(
        "// Generated by tools/gym_hack/build_pallet_town.py. Don't edit by hand.\n"
        "#ifndef GUARD_CONSTANTS_PALLET_TOWN_GYM_H\n#define GUARD_CONSTANTS_PALLET_TOWN_GYM_H\n\n"
        f"#define METATILE_PALLET_GYM_DOOR        0x{GYM_DOOR_OPEN:X}\n"
        f"#define METATILE_PALLET_GYM_DOOR_LOCKED 0x{GYM_DOOR_LOCKED:X}\n\n"
        "#endif // GUARD_CONSTANTS_PALLET_TOWN_GYM_H\n")
    # The berry field's plots and their metatiles (src/berry_field.c)
    colors = berry_field_art.COLORS
    (ROOT / "include/constants/berry_field.h").write_text(
        "// Generated by tools/gym_hack/build_pallet_town.py. Don't edit by hand.\n"
        "#ifndef GUARD_CONSTANTS_BERRY_FIELD_H\n#define GUARD_CONSTANTS_BERRY_FIELD_H\n\n"
        f"#define BERRY_FIELD_X0          {BERRY_FIELD_X0}\n"
        f"#define BERRY_FIELD_COLUMNS     {BERRY_FIELD_X1 - BERRY_FIELD_X0 + 1}\n"
        f"#define BERRY_FIELD_ROW_0_Y     {BERRY_FIELD_ROWS[0]}\n"
        f"#define BERRY_FIELD_ROW_1_Y     {BERRY_FIELD_ROWS[1]}\n"
        f"#define BERRY_FIELD_PLOTS       {2 * (BERRY_FIELD_X1 - BERRY_FIELD_X0 + 1)}\n\n"
        f"#define METATILE_BERRY_SOIL     0x{BERRY_FIELD['soil']:X}\n"
        f"#define METATILE_BERRY_SEED     0x{BERRY_FIELD['seed']:X}\n"
        f"#define METATILE_BERRY_SPROUT   0x{BERRY_FIELD['sprout']:X}\n"
        f"#define METATILE_BERRY_YOUNG    0x{BERRY_FIELD['young']:X}\n"
        "// (by BERRY_COLOR_*)\n"
        f"#define METATILE_BERRY_FLOWER_FIRST 0x{BERRY_FIELD['flower_' + colors[0]]:X}\n"
        f"#define METATILE_BERRY_RIPE_FIRST   0x{BERRY_FIELD['ripe_' + colors[0]]:X}\n"
        f"#define METATILE_BERRY_STEP         0x{BERRY_FIELD['flower_' + colors[1]] - BERRY_FIELD['flower_' + colors[0]]:X}\n\n"
        "#endif // GUARD_CONSTANTS_BERRY_FIELD_H\n")


def write_palettes():
    def jasc(colors):
        return "JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{r} {g} {b}\r\n" for r, g, b in colors)
    roof_red = rm.load_pal(PRIM / "palettes/02.pal")
    decor = [pallet_art.DECOR_PALETTE.get(i, (0, 0, 0)) for i in range(16)]
    for i in (4, 5, 6, 7):
        decor[i] = tuple(roof_red[i][:3])
    roofs = [tuple(c[:3]) for c in roof_red]
    blue = [(205, 230, 255), (148, 197, 246), (106, 156, 230), (82, 123, 213), (41, 65, 156)]
    green = [(213, 246, 172), (164, 222, 115), (123, 197, 90), (90, 164, 65), (41, 115, 49)]
    for k, c in zip((10, 11, 12, 13, 14), blue):
        roofs[k] = c
    for k, c in zip((1, 2, 3, 8, 9), green):
        roofs[k] = c
    (SEC / f"palettes/{DECOR_PAL:02}.pal").write_text(jasc(decor), newline="")
    (SEC / f"palettes/{ROOF_PAL:02}.pal").write_text(jasc(roofs), newline="")
    # (palette 11: the berry field's colors in its unused slots: berry_field_art.py)
    (SEC / "palettes/11.pal").write_text(jasc(berry_field_art.palettes(ROOT)[11]), newline="")


def write():
    blocks = []
    for y in range(H):
        for x in range(W):
            blocks.append((mt[y][x] & 0x3FF) | (coll[y][x] << 10) | (elev[y][x] << 12))
    LAYOUT_BIN.write_bytes(struct.pack(f"<{len(blocks)}H", *blocks))
    (SEC / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *e) for e in SEC_MT))
    rows = (len(SEC_TILES) + 15) // 16
    img = Image.new("P", (128, rows * 8), 0)
    img.putpalette(_sec_tiles_img.getpalette())
    for i, px in enumerate(SEC_TILES):
        t = Image.new("P", (8, 8))
        t.putdata(px)
        img.paste(t, ((i % 16) * 8, (i // 16) * 8))
    img.save(SEC / "tiles.png")
    write_palettes()
    (SEC / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in SEC_ATTR))
    path = ROOT / "data/layouts/layouts.json"
    text = path.read_text()
    data = json.loads(text)
    for l in data["layouts"]:
        if l.get("id") == "LAYOUT_PALLET_TOWN":
            l["width"], l["height"] = W, H
    path.write_text(json.dumps(data, indent=2) + "\n")


def preview(out, scale=2):
    from PIL import Image, ImageDraw
    prim = rm.Tileset(rm.tileset_dir("gTileset_General_Frlg"))
    sec = rm.Tileset(rm.tileset_dir("gTileset_PalletTown"))
    img = Image.new("RGB", (W * 16, H * 16))
    for y in range(H):
        for x in range(W):
            rm.draw_metatile(img, x * 16, y * 16, mt[y][x], prim, sec)
    img = img.resize((W * 16 * scale, H * 16 * scale), Image.NEAREST)
    d = ImageDraw.Draw(img)
    s = 16 * scale
    for x in range(0, W, 5):
        d.text((x * s + 2, 2), str(x), fill=(255, 255, 0))
    for y in range(0, H, 5):
        d.text((2, y * s + 2), str(y), fill=(255, 255, 0))
    img.save(out)


if __name__ == "__main__":
    write()
    write_map_json()
    write_movements()
    if len(sys.argv) > 1:
        preview(sys.argv[1])
    print("doors:", dict(player=PLAYER_DOOR, rival=RIVAL_DOOR, neighbor=NEIGHBOR_DOOR, house2=HOUSE2_DOOR,
                         lab=LAB_DOOR, gym=GYM_DOOR, pc=PC_DOOR, mart=MART_DOOR))
