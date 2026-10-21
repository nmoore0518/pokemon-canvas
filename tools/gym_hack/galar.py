"""The Exotic Pokémon Safari's Galar zone (Pokémon Canvas), for build_exotic_safari.py.

Six areas from Nick's planner (exotic-safari-planner-galar_update.xlsx):

    Hall door -> MEADOW (Postwick's countryside; the hub) -> north the LAKE (the Wild Area's Lake
    District) -> west the FOREST (Glimwood Tangle) / east the MINE (Galar Mine, 2 floors) /
    north the CITY (Circhester) -> north the STADIUM (Wyndon Stadium: lobby and pitch)
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paldea import read_layout, blk, land   # noqa: E402

REGION = "GALAR"
HUB = "MEADOW"


def canvas(w, h, fill):
    return [[fill(x, y) if callable(fill) else fill for x in range(w)] for y in range(h)]


def paste(g, src, sx, sy, w, h, dx, dy):
    for y in range(h):
        for x in range(w):
            if 0 <= dy + y < len(g) and 0 <= dx + x < len(g[0]):
                g[dy + y][dx + x] = src[sy + y][sx + x]


def rect(g, x0, y0, x1, y1, v):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            g[y][x] = v(x, y) if callable(v) else v


# ---------------------------------------------------------------------------------------------
# Meadow: Postwick's countryside (HnS's Johto farmland pieces: wheat, fences, farmhouses, ponds)
# ---------------------------------------------------------------------------------------------
def meadow():
    r38, _ = read_layout("LAYOUT_ROUTE38")
    r39, _ = read_layout("LAYOUT_ROUTE39")
    W, H = 40, 32
    grass = lambda x, y: blk((0x000, 0x001)[x % 2], 0)
    g = canvas(W, H, grass)
    paste(g, r39, 0, 0, 40, 8, 0, 0)          # the wheat field, the canal, the fence below
    paste(g, r39, 0, 8, 14, 5, 0, 8)          # autumn trees (left)
    paste(g, r39, 14, 7, 14, 6, 26, 7)        # autumn trees (right; with their tops)
    paste(g, r39, 10, 13, 18, 7, 1, 14)       # the farm: farmhouse, blue house, fences
    paste(g, r38, 13, 34, 12, 11, 27, 15)     # the fenced pond
    rect(g, 14, 7, 25, 7, grass)              # (no trees above the gap)
    rect(g, 2, 22, 15, 27, blk(0x00D, 0))     # tall grass meadows
    rect(g, 19, 13, 25, 26, blk(0x00D, 0))
    # Trees along the south edge (Route 38's: their top edge, then rows of trees, carried on past
    # the map's edge by the border: MEADOW_BORDER)
    for x in range(W):
        g[29][x] = r38[45][14 + x % 2]
        g[30][x] = r38[46][14 - x % 2]
        g[31][x] = r38[47][14 + x % 2]
    rect(g, 17, 28, 19, 31, grass)            # the way back to the hall (south)
    rect(g, 39, 13, 39, 14, grass)            # -> the lake (east)
    import build_exotic_safari as B
    B.clone_tileset("gTileset_Route38_Farmland_Hns", "GalarMeadow", edit=add_stones, callback="InitTilesetAnim_Glimwood")
    for x, y in STONES:                       # (dark until stepped on)
        g[y][x] = blk(STONE_FIRST + (x % 2), 0)
    g[STONES_SIGN[1]][STONES_SIGN[0]] = blk(0x002, 1, 0)   # (a sign: which way the bolt falls)
    return g, W, H


# Regieleki's stepping stones: a lightning bolt of flat stones in the clearing north of the farm,
# to be stepped on from the top of the bolt down to its tip (src/exotic_safari.c: each lights up -
# palette 12, pulsing like Glimwood's mushrooms; one out of order puts them all out)
STONES = [(23, 8), (21, 9), (19, 10), (22, 10), (20, 11), (18, 12), (16, 13)]
STONES_SIGN = (14, 8)
STONE_FIRST = None                 # (dark on grass 0x000, 0x001; lit on 0x000, 0x001)
STONE_PAL = [
    (0, 0, 0), (52, 56, 64), (112, 118, 122), (164, 170, 166),   # 0 -, 1 outline, 2 shadow, 3 stone
    (40, 92, 196), (88, 188, 240), (184, 238, 255), (255, 244, 120),   # 4-7 lit: edge, stone, shine, spark
    (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0),
    (255, 255, 255), (206, 210, 202), (0, 0, 0), (0, 0, 0),     # 12 lit sparkle; 13 stone's highlight
]


def stone_art(lit):
    px = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            dx, dy = (x - 7.5) / 6.6, (y - 8.5) / 5.4
            d = dx * dx + dy * dy
            if d <= 1:
                px[y][x] = 3 if dy < 0.45 else 2
                if dy < -0.55 and dx < 0.3:
                    px[y][x] = 13
    for y in range(16):                            # outline
        for x in range(16):
            if px[y][x] and any(not (0 <= x + a < 16 and 0 <= y + b < 16) or not px[y + b][x + a]
                                for a, b in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                px[y][x] = 1
    for x, y in ((8, 5), (7, 6), (6, 7), (7, 7), (8, 7), (9, 7), (8, 8), (7, 9), (6, 10)):   # a carved bolt
        px[y][x] = 1 if not lit else 7
    if lit:
        swap = {1: 4, 2: 5, 3: 5, 13: 6}
        px = [[swap.get(c, c) for c in row] for row in px]
        for x, y in ((4, 6), (11, 9), (10, 5)):
            px[y][x] = 12
    return px


def add_stones(out):
    """The meadow's secondary tileset (a copy of Route 38's) gets the stepping stones: palette 12
    (unused by the meadow), their tiles, 4 metatiles over the plain grass (drawn under the player)"""
    import struct
    from PIL import Image
    import build_exotic_safari as B
    img = Image.open(out / "tiles.png")
    mt = bytearray((out / "metatiles.bin").read_bytes())
    at = bytearray((out / "metatile_attributes.bin").read_bytes())
    prim = (B.render_map.tileset_dir("gTileset_Johto_General_Hns") / "metatiles.bin").read_bytes()
    used = max((v & 0x3FF) for i in range(0, len(mt), 2) for v in [struct.unpack_from("<H", mt, i)[0]]
               if v & 0x3FF >= 640) - 640 + 1
    ntiles = (img.width // 8) * (img.height // 8)
    need = used + 8
    if need > ntiles:                               # (room for the new tiles)
        new = Image.new("P", (128, ((need + 15) // 16) * 8))
        new.putpalette(img.getpalette())
        new.paste(img, (0, 0))
        img = new
    pix = img.load()
    first = len(mt) // 16
    for lit in (False, True):
        art = stone_art(lit)
        tiles = []
        for k in range(4):
            n = used
            used += 1
            for y in range(8):
                for x in range(8):
                    pix[(n % 16) * 8 + x, (n // 16) * 8 + y] = art[(k // 2) * 8 + y][(k % 2) * 8 + x]
            tiles.append((640 + n) | (12 << 12))
        for grass in (0x000, 0x001):
            ground = struct.unpack_from("<4H", prim, grass * 16)
            mt += struct.pack("<8H", *ground, *tiles)
            at += struct.pack("<I", 1 << 29)        # (normal ground, drawn under the player)
    assert used <= 384 and len(mt) // 16 <= 384
    img.save(out / "tiles.png")
    (out / "metatiles.bin").write_bytes(mt)
    (out / "metatile_attributes.bin").write_bytes(at)
    B.write_pal(out / "palettes" / "12.pal", STONE_PAL)
    global STONE_FIRST
    STONE_FIRST = 0x280 + first


# ---------------------------------------------------------------------------------------------
# Lake: the Wild Area's Lake District (Hoenn's Safari Zone, the south-east area: its river,
# waterfall and lake; converted to a FireRed tileset)
# ---------------------------------------------------------------------------------------------
def lake():
    g, _ = read_layout("LAYOUT_SAFARI_ZONE_SOUTHEAST")
    g = [row[:] for row in g[:38]]
    for y, row in enumerate(g):
        for x, v in enumerate(row):
            if v & 0x3FF in (0x310, 0x318):           # (no PokéBlock feeders)
                row[x] = 0x001 | (3 << 12)
    paste(g, [r[:] for r in g], 32, 28, 8, 10, 0, 28)      # (trees over the Safari Zone's building)
    for y in range(28, 38):                                 # (its first column was the pond's edge)
        g[y][0] = g[y][2]
    return g, 40, 38


# ---------------------------------------------------------------------------------------------
# Galar Mine: Hoenn's Granite Cave 1F and B1F (its cracked floors made solid)
# ---------------------------------------------------------------------------------------------
def mine(floor):
    g, lay = read_layout("LAYOUT_GRANITE_CAVE_1F" if floor == 1 else "LAYOUT_GRANITE_CAVE_B1F")
    g = [row[:] for row in g]
    for row in g:
        for x, v in enumerate(row):
            if v & 0x3FF == 0x22F:
                row[x] = (v & ~0x3FF) | 0x201
    return g, lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Wyndon Stadium: Hoenn's Battle Dome lobby, and its battle room (the pitch, the stands around it)
# ---------------------------------------------------------------------------------------------
def stadium_lobby():
    g, lay = read_layout("LAYOUT_BATTLE_FRONTIER_BATTLE_DOME_LOBBY")
    return [row[:] for row in g], lay["width"], lay["height"]


def stadium_pitch():
    g, lay = read_layout("LAYOUT_BATTLE_FRONTIER_BATTLE_DOME_BATTLE_ROOM")
    g = [row[:] for row in g] + [[g[9][0]] * lay["width"] for _ in range(4)]   # (more floor below)
    for y in range(4):                                  # (the stands)
        for x in range(lay["width"]):
            g[y][x] |= 1 << 10
    return g, lay["width"], lay["height"] + 4


# ---------------------------------------------------------------------------------------------
# Circhester: HnS's Olivine City (its terraced stone houses, cobbled streets, the gym), snowed
# over: copies of its tilesets with the greens turned to snow
# ---------------------------------------------------------------------------------------------
def snowify(i, c):
    r, g, b = c
    if g > r + 6 and g >= b - 10:                   # grass, leaves -> snow (shadows blue-grey)
        lum = 0.3 * r + 0.59 * g + 0.11 * b
        t = max(0.0, min(1.0, (lum - 50) / 170))
        lo, hi = (96, 112, 152), (248, 248, 255)
        return tuple(int(lo[k] + (hi[k] - lo[k]) * t) for k in range(3))
    return c


def city():
    import build_exotic_safari as B
    B.clone_tileset("gTileset_Johto_General_Hns", "GalarSnowGeneral", lambda i, c: snowify(i, c))
    B.clone_tileset("gTileset_OlivineCity_Hns", "GalarSnowCity", lambda i, c: snowify(i, c))
    g, _ = read_layout("LAYOUT_OLIVINE_CITY")
    g = [row[0:38] for row in g[8:45]]
    tall = blk(0x00D, 0)
    for y in range(10, 15):                          # snowy fields where the trees were
        for x in range(0, 12):
            g[y][x] = tall
    for y in range(0, 37):                           # and every garden
        for x in range(38):
            if g[y][x] & 0x3FF in (0x000, 0x001) and not (g[y][x] >> 10) & 3:
                g[y][x] = tall
    return g, 38, 37


# ---------------------------------------------------------------------------------------------
# Glimwood Tangle: HnS's Ilex Forest, in the night's glow: copies of its tilesets recolored
# (deep blue-violet leaves and ground, glowing flowers)
# ---------------------------------------------------------------------------------------------
def glimwood_color(i, c):
    import colorsys
    r, g, b = (v / 255 for v in c)
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    deg = h * 360
    if s < 0.12:                                    # greys, whites: cool and dim (whites glow)
        if v > 0.9:
            return (232, 240, 255)
        nr, ng, nb = colorsys.hsv_to_rgb(240 / 360, 0.25, v * 0.7)
    elif 60 <= deg <= 175:                          # leaves, grass -> deep teal to violet
        nh = (200 + (deg - 60) * 0.55) / 360
        nr, ng, nb = colorsys.hsv_to_rgb(nh, min(1, s * 0.85 + 0.15), v * 0.62)
    elif deg < 25 or deg >= 300:                    # red/pink flowers -> glowing magenta
        nr, ng, nb = colorsys.hsv_to_rgb(315 / 360, 0.55, min(1, v * 1.15 + 0.1))
    elif 25 <= deg < 60:                            # browns (trunks, earth), yellows
        if s > 0.5 and v > 0.7:                     # (yellow flowers -> glowing cyan)
            nr, ng, nb = colorsys.hsv_to_rgb(180 / 360, 0.45, 1.0)
        else:
            nr, ng, nb = colorsys.hsv_to_rgb(265 / 360, s * 0.5, v * 0.55)
    else:                                           # blues (water): deeper, a little violet
        nr, ng, nb = colorsys.hsv_to_rgb(min(deg + 10, 280) / 360, s, v * 0.75)
    return tuple(int(x * 255) for x in (nr, ng, nb))


# The glowing mushrooms (Glimwood's own palette, 12: pulsed by InitTilesetAnim_Glimwood)
GLOW_PAL = [
    (0, 0, 0), (34, 22, 60),                                      # 0 -, 1 outline
    (110, 96, 150), (196, 196, 228),                              # 2-3 stem
    (24, 104, 164), (64, 196, 232), (156, 248, 255), (236, 255, 255),   # 4-7 cap (dark -> bright)
    (200, 96, 192), (248, 176, 240),                              # 8-9 gills, pink caps
    (120, 48, 168), (180, 100, 230),                              # 10-11 violet caps
    (255, 255, 255),                                              # 12 sparkle
    (30, 48, 78), (52, 74, 104), (78, 52, 92),                    # 13-15 a dark mushroom's cap, its glints, gills
]
# A big mushroom not lit yet (Zarude's lamps): its cap and gills in the colors that don't pulse
DIM_COLOR = {4: 13, 5: 13, 6: 14, 7: 14, 8: 15, 9: 15, 12: 14}


def mushroom_art(W=32):
    """(the big mushroom, Wx32; two small clusters, 16x16) as palette-index pixel lists"""
    import math
    big = [[0] * W for _ in range(32)]
    mid = W / 2
    def put(x, y, c):
        if 0 <= x < W and 0 <= y < 32:
            big[y][x] = c
    # stem
    c0 = int(mid)
    for y in range(17, 31):
        w = (4 if y < 28 else 5) + (W > 32)
        for x in range(c0 - w, c0 + w):
            put(x, y, 3 if x < c0 + w - 3 else 2)
        put(c0 - w - 1, y, 1); put(c0 + w, y, 1)
    for x in range(c0 - 6, c0 + 6):
        put(x, 31, 1)
    # cap: a dome over a flat underside
    rx = mid - 0.5
    for y in range(2, 19):
        for x in range(W):
            dx, dy = (x - rx) / rx, (y - 17.5) / 15.5
            if dx * dx + dy * dy <= 1 and y <= 17:
                shade = 6 if dy < -0.75 else 5 if dy < -0.35 else 4
                if dx > 0.55:
                    shade = max(4, shade - 1)
                put(x, y, shade)
    for y in range(2, 19):                                          # outline
        for x in range(W):
            if big[y][x] in (4, 5, 6) and any(not (0 <= x + a < W and 0 <= y + b < 32) or big[y + b][x + a] == 0
                                               for a, b in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                big[y][x] = 1
    for x in range(3, W - 3):                                       # the gills
        put(x, 17, 8); put(x, 18, 9 if 6 < x < W - 6 else 8)
    put(2, 17, 1); put(W - 3, 17, 1)
    k = W / 32
    for cx, cy, r in ((9 * k, 9, 2), (17 * k, 6, 2), (23 * k, 11, 1.5), (13 * k, 13, 1.5), (25 * k, 6, 1)):   # glowing spots
        for y in range(32):
            for x in range(W):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r and big[y][x] in (4, 5, 6):
                    big[y][x] = 7
    put(8, 8, 12); put(16, 5, 12)
    # small clusters (walkable decorations)
    def small(caps):
        s = [[0] * 16 for _ in range(16)]
        for cx, cy, r, c, lite in caps:
            for y in range(cy, cy + 5):
                s[y][cx] = 3; s[y][cx + 1] = 2
            for y in range(cy - r, cy + 1):
                for x in range(cx - r, cx + r + 2):
                    if ((x - cx - 0.5) / (r + 1)) ** 2 + ((y - cy) / (r + 0.5)) ** 2 <= 1:
                        s[y][x] = c
            s[cy - r][cx] = lite
            s[cy - r + 1][cx - 1] = lite
        return s
    small1 = small([(4, 8, 2, 5, 7), (10, 11, 2, 9, 12)])
    small2 = small([(5, 10, 2, 11, 9), (11, 7, 1, 5, 7)])
    return big, small1, small2


def add_mushrooms(out):
    """Glimwood's secondary tileset gets the mushrooms: new tiles after its own, palette 12, and
    metatiles at the end (big: 4, a 2x2 block; small: 2) over the forest floor"""
    import struct
    from PIL import Image
    import build_exotic_safari as B
    big, s1, s2 = mushroom_art()
    img = Image.open(out / "tiles.png")
    mt = bytearray((out / "metatiles.bin").read_bytes())
    at = bytearray((out / "metatile_attributes.bin").read_bytes())
    used = max((v & 0x3FF) for i in range(0, len(mt), 2) for v in [struct.unpack_from("<H", mt, i)[0]]
               if v & 0x3FF >= 640) - 640 + 1
    pix = img.load()
    def add_tile(rows, ox, oy):
        nonlocal used
        n = used
        used += 1
        for y in range(8):
            for x in range(8):
                pix[(n % 16) * 8 + x, (n // 16) * 8 + y] = rows[oy + y][ox + x]
        return 640 + n
    ground = struct.unpack_from("<8H", mt, 0x0E * 16)[:4]        # (the forest floor: 0x28E)
    ground_attr = struct.unpack_from("<I", at, 0x0E * 4)[0]
    first = len(mt) // 16
    def add_metatile(rows, ox, oy, attr):
        nonlocal mt, at
        top = [add_tile(rows, ox + (k % 2) * 8, oy + (k // 2) * 8) | (12 << 12) for k in range(4)]
        mt += struct.pack("<8H", *ground, *top)
        at += struct.pack("<I", attr)
    # (all drawn under the player: the player walking up to one stands in front of it)
    stand = (ground_attr & ~0x1FF & ~0x60000000) | (1 << 29)
    for k in range(4):                                             # big: top left, top right, bottom...
        add_metatile(big, (k % 2) * 16, (k // 2) * 16, stand)
    covered = (ground_attr & ~0x60000000) | (1 << 29)             # (drawn under the player)
    add_metatile(s1, 0, 0, covered)
    add_metatile(s2, 0, 0, covered)
    giant, _, _ = mushroom_art(48)                                 # giant (3x2, in a clearing)
    for k in range(6):
        add_metatile(giant, (k % 3) * 16, (k // 3) * 16, stand)
    dim = [[DIM_COLOR.get(c, c) for c in row] for row in big]     # big, not lit yet (Zarude's lamps)
    for k in range(4):
        add_metatile(dim, (k % 2) * 16, (k // 2) * 16, stand)
    dim = [[DIM_COLOR.get(c, c) for c in row] for row in giant]   # giant, not lit yet
    for k in range(6):
        add_metatile(dim, (k % 3) * 16, (k // 3) * 16, stand)
    assert used <= 384
    img.save(out / "tiles.png")
    (out / "metatiles.bin").write_bytes(mt)
    (out / "metatile_attributes.bin").write_bytes(at)
    B.write_pal(out / "palettes" / "12.pal", GLOW_PAL)
    global MUSHROOM_FIRST
    MUSHROOM_FIRST = 0x280 + first


MUSHROOM_FIRST = None
LAMP_COUNT = 8                     # (Zarude's lamps: big and giant mushrooms, dark until touched)
LAMPS = []                         # (their top-left corners and widths, 2 or 3: made by place_mushrooms)


def place_mushrooms(g, w, h, reach_ok, reach_all):
    """Big mushrooms on 2x2 patches of forest floor beside the trees (never blocking a way through:
    reach_ok(g) checks), small ones dotted on the floor"""
    import random
    rnd = random.Random("glimwood")
    floor = lambda x, y: 0 <= x < w and 0 <= y < h and g[y][x] & 0x3FF in (0x28E, 0x28F, 0x286, 0x287) and not (g[y][x] >> 10) & 3
    cells = [(x, y) for y in range(h - 1) for x in range(w - 1)]
    rnd.shuffle(cells)
    placed, spots = 0, []
    LAMPS.clear()
    for x, y in cells:
        if placed >= LAMP_COUNT:
            break
        if not all(floor(x + a, y + b) for a in (0, 1) for b in (0, 1)):
            continue
        # beside trees (on at least two sides of the 2x2)
        sides = sum(1 for a, b in ((-1, 0), (2, 0), (0, -1), (0, 2), (-1, 1), (2, 1), (1, -1), (1, 2))
                    if 0 <= x + a < w and 0 <= y + b < h and (g[y + b][x + a] >> 10) & 3)
        if sides < 2:
            continue
        if any(abs(x - px) + abs(y - py) < 8 for px, py in spots):      # (spread out)
            continue
        old = [g[y + b][x + a] for b in (0, 1) for a in (0, 1)]
        for k, (a, b) in enumerate(((0, 0), (1, 0), (0, 1), (1, 1))):
            g[y + b][x + a] = (MUSHROOM_FIRST + 12 + k) | (1 << 10) | (3 << 12)     # (dark until lit)
        if reach_ok(g):
            placed += 1
            spots.append((x, y))
            LAMPS.append((x, y, 2))
        else:
            for k, (a, b) in enumerate(((0, 0), (1, 0), (0, 1), (1, 1))):
                g[y + b][x + a] = old[k]
    # Giant mushrooms in clearings: two trees beside a path cleared (whole trees: the trees above
    # end in their trunks, the ones below start with their tops), the mushroom on the new floor
    giants = []
    blocks = [(x, y) for y in range(1, h - 1) for x in range(w - 2)
              if [g[y + b][x + a] & 0x3FF for b in (0, 1) for a in (0, 1, 2)] == [0x290, 0x291, 0x292, 0x298, 0x299, 0x29A]]
    rnd.shuffle(blocks)
    for x, y in blocks:
        if len(giants) >= 10:
            break
        if any(abs(x - px) + abs(y - py) < 7 for px, py in giants + spots):
            continue
        trial = [row[:] for row in g]
        rows = clear_trees(trial, x, y, h)
        if rows is None:
            continue
        gy = rows[-2]                                    # (the mushroom: the clearing's lowest two rows)
        lamp = len(LAMPS) < LAMP_COUNT                     # (the first few: lamps, dark until lit)
        for k in range(6):
            trial[gy + k // 3][x + k % 3] = (MUSHROOM_FIRST + (16 if lamp else 6) + k) | (1 << 10) | (3 << 12)
        # The player must be able to walk up to it (and nothing else may be cut off)
        reach = reach_all(trial)
        around = [(x + a, gy + b) for a in range(-1, 4) for b in (-1, 0, 1, 2) if (a in (-1, 3)) != (b in (-1, 2))]
        if reach is None or not any(p in reach for p in around):
            continue
        for r in range(len(g)):
            g[r][x:x + 3] = trial[r][x:x + 3]
        giants.append((x, gy))
        if lamp:
            LAMPS.append((x, gy, 3))
    for x, y in cells[:]:
        if floor(x, y) and rnd.random() < 0.06:
            g[y][x] = (MUSHROOM_FIRST + 4 + rnd.randrange(2)) | (3 << 12)


TREE_TOP, TREE_A, TREE_B, TREE_END, TREE_TRUNK = 0x288, 0x290, 0x298, 0x2A0, 0x2A8
TREES = {b + i for b in (0x288, 0x290, 0x298, 0x2A0, 0x2A8, 0x2B0) for i in range(3)}


def clear_trees(g, x, y, h):
    """Glimwood (Ilex Forest) trees stack in 3-wide columns: a top edge row (0x288), pairs of canopy
    rows (0x290/0x298), then the bottom (0x2A0) and the trunks (0x2A8). Clears the tree pair at rows
    y, y+1 and the pair above it (or below), then retiles the rest of the column so every tree stays
    whole: rows that can't make a whole tree are cleared too. Returns the cleared rows, or None."""
    col = lambda r: 0 <= r < h and all(g[r][x + a] & 0x3FF in TREES for a in range(3))
    clear = {y, y + 1} | ({y - 2, y - 1} if col(y - 1) and g[y - 1][x] & 0x3FF == TREE_B else {y + 2, y + 3})
    if not all(col(r) for r in clear):
        return None
    def set_row(r, base):
        for a in range(3):
            g[r][x + a] = (base + a) | (1 << 10) | (3 << 12)
    def floor_row(r):
        for a in range(3):
            g[r][x + a] = (0x28E if (x + a) % 2 == 0 else 0x28F) | (3 << 12)
    for r in clear:
        floor_row(r)
    return retile(g, x, h, clear)


def retile(g, x, h, clear):
    """(clear_trees) the tree column at x either side of the cleared rows made whole again"""
    col = lambda r: 0 <= r < h and all(g[r][x + a] & 0x3FF in TREES for a in range(3))
    def set_row(r, base):
        for a in range(3):
            g[r][x + a] = (base + a) | (1 << 10) | (3 << 12)
    def floor_row(r):
        for a in range(3):
            g[r][x + a] = (0x28E if (x + a) % 2 == 0 else 0x28F) | (3 << 12)
    clear = set(clear)
    while True:
        # The tree runs either side of the clearing
        changed = False
        for side in (-1, 1):
            r = (min(clear) - 1) if side < 0 else (max(clear) + 1)
            if not col(r):
                continue
            s_, e_ = r, r
            while col(s_ - 1):
                s_ -= 1
            while col(e_ + 1):
                e_ += 1
            top_edge, bottom_edge = s_ > 0, e_ < h - 1
            mid = (e_ - (2 if bottom_edge else 0)) - (s_ + (1 if top_edge else 0)) + 1
            if mid < 0 or mid % 2:            # (can't be whole trees: the row by the clearing goes)
                clear.add(r)
                floor_row(r)
                changed = True
                continue
            k = s_
            if top_edge:
                set_row(k, TREE_TOP); k += 1
            for i in range(mid):
                set_row(k, TREE_A if i % 2 == 0 else TREE_B); k += 1
            if bottom_edge:
                set_row(k, TREE_END); set_row(k + 1, TREE_TRUNK)
        if not changed:
            break
    rows = sorted(clear)
    return rows if rows == list(range(rows[0], rows[-1] + 1)) and len(rows) <= 6 else None


def forest():
    import build_exotic_safari as B
    B.clone_tileset("gTileset_Johto_South_Hns", "GlimwoodGeneral", glimwood_color)
    B.clone_tileset("gTileset_IlexForest_Hns", "Glimwood", glimwood_color, edit=add_mushrooms,
                    callback="InitTilesetAnim_Glimwood")
    g, _ = read_layout("LAYOUT_ILEX_FOREST")
    g = [row[17:56] for row in g[10:50]]                   # (39 wide: whole trees, 3 columns each)
    paste(g, [r[:] for r in g], 12, 0, 6, 6, 0, 0)        # (trees over Ilex Forest's gatehouse)
    # (the south-east glade was cut off - Ilex's Cut tree: two whole trees and a post cleared)
    for y in (28, 29):
        for x in range(18, 25):
            g[y][x] = (0x28E if x % 2 == 0 else 0x28F) | (3 << 12)
    for x in (18, 21):
        retile(g, x, 40, {28, 29})
    # Mushrooms: each must leave everything else reachable (the floor shrinks by just its 4 cells)
    state = {"n": len(_flood(g, (15, 38)))}
    def reach_ok(grid):
        n = len(_flood(grid, (15, 38)))
        if n == state["n"] - 4:
            state["n"] = n
            return True
        return False
    def reach_all(grid):           # (what the player can reach: None if anything was cut off)
        seen = _flood(grid, (15, 38))
        return seen if len(seen) >= state["n"] else None
    place_mushrooms(g, 39, 40, reach_ok, reach_all)
    return g, 39, 40


def _flood(grid, start):
    seen, todo = set(), [start]
    h, w = len(grid), len(grid[0])
    while todo:
        x, y = todo.pop()
        if (x, y) in seen or not (0 <= x < w and 0 <= y < h) or (grid[y][x] >> 10) & 3:
            continue
        seen.add((x, y))
        todo += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    return seen


def pitch_banner(out, remap):
    """The stands' banner says WYNDON in place of BATTLE FRONTIER: its 6 text metatiles (0x2D5-0x2DA)
    get new tiles (the banner's own colors, the text redrawn) and no top layer"""
    import struct
    from PIL import Image
    from collections import Counter
    import build_exotic_safari as B
    mt_path, img_path = out / "metatiles.bin", out / "tiles.png"
    b = bytearray(mt_path.read_bytes())
    img = Image.open(img_path)
    ids = [remap[m] - 0x280 for m in range(0x2D5, 0x2DB)]
    def tile(n):
        n -= 640
        return img.crop(((n % 16) * 8, (n // 16) * 8, (n % 16) * 8 + 8, (n // 16) * 8 + 8))
    # The strip as it is (the bottom layer), its palette
    strip = Image.new("P", (16 * len(ids), 16))
    entries = [struct.unpack_from("<8H", b, k * 16) for k in ids]
    pal = entries[0][0] >> 12
    for i, e in enumerate(entries):
        for k in range(4):
            v = e[k]
            tl = tile(v & 0x3FF)
            if (v >> 10) & 1:
                tl = tl.transpose(Image.FLIP_LEFT_RIGHT)
            if (v >> 11) & 1:
                tl = tl.transpose(Image.FLIP_TOP_BOTTOM)
            strip.paste(tl, (i * 16 + (k % 2) * 8, (k // 2) * 8))
    px = strip.load()
    bg = Counter(px[x, 8] for x in range(strip.width)).most_common(1)[0][0]
    colors = B.render_map.load_pal(out / "palettes" / f"{pal:02d}.pal")
    white = max(range(1, 16), key=lambda c: sum(colors[c][:3]))
    for y in range(3, 13):
        for x in range(strip.width):
            px[x, y] = bg
    text = "WYNDON"
    x0 = (strip.width - (len(text) * 6 - 1)) // 2
    for i, ch in enumerate(text):
        for y, row in enumerate(B.FONT[ch]):
            for x, bit in enumerate(row):
                if bit == "1":
                    px[x0 + i * 6 + x, 4 + y] = white
    # New tiles at the end of the tileset, plus a blank one for the top layer
    n0 = (img.width // 8) * (img.height // 8)
    new = Image.new("P", (128, ((n0 + len(ids) * 4 + 1 + 15) // 16) * 8))
    new.putpalette(img.getpalette())
    new.paste(img, (0, 0))
    for i in range(len(ids)):
        for k in range(4):
            n = n0 + i * 4 + k
            new.paste(strip.crop((i * 16 + (k % 2) * 8, (k // 2) * 8, i * 16 + (k % 2) * 8 + 8, (k // 2) * 8 + 8)),
                      ((n % 16) * 8, (n // 16) * 8))
    blank = n0 + len(ids) * 4
    for i, k in enumerate(ids):
        e = [640 + n0 + i * 4 + q | (pal << 12) for q in range(4)] + [640 + blank | (pal << 12)] * 4
        struct.pack_into("<8H", b, k * 16, *e)
    assert blank < 384
    new.save(img_path)
    mt_path.write_bytes(b)


LV = (48, 52)
AREAS = {
    "MEADOW": {
        "map": "ExoticSafari_GalarMeadow", "build": meadow, "border": [[0x41D, 0x41C], [0x415, 0x414]], "tilesets": ("gTileset_Johto_General_Hns", "gTileset_GalarMeadow"),
        "music": "MUS_RG_ROUTE1", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "HALL": ("edge", [(17, 31), (18, 31), (19, 31)], (18, 30), ("HALL", None)),
            "LAKE": ("edge", [(39, 13), (39, 14)], (38, 13), ("LAKE", "MEADOW")),
        },
        "land": land("WOOLOO", "ZIGZAGOON_GALAR", "ROOKIDEE", "SKWOVET", "NICKIT", "FARFETCHD_GALAR", "YAMPER", "GOSSIFLEUR",
                     "APPLIN", "GROWLITHE_HISUI", ("GROOKEY", 48, 50), ("ARCANINE_HISUI", 52, 55)),
        "statics": [("REGIELEKI", (19, 7))],
        "extra": lambda ctx: meadow_stones(ctx),
        "items": [("TART_APPLE", (14, 9)), ("SWEET_APPLE", (25, 9))],
    },
}
AREAS.update({
    "LAKE": {
        "map": "ExoticSafari_GalarLake", "build": lake, "tilesets": ("gTileset_General", "gTileset_Lilycove"),
        "version": "emerald", "convert": ("GalarLake", "galar_lake_frlg"),
        "music": "MUS_RG_ROUTE24", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "MEADOW": ("edge", [(0, 16), (0, 17)], (1, 16), ("MEADOW", "LAKE")),
            "FOREST": ("edge", [(6, 0), (7, 0), (8, 0)], (7, 1), ("FOREST", "LAKE")),
            "CITY": ("edge", [(x, 0) for x in range(27, 33)], (29, 1), ("CITY", "LAKE")),
            "MINE": ("edge", [(39, 7)], (38, 7), ("MINE", "LAKE")),
        },
        "land": land("CHEWTLE", "SLOWPOKE_GALAR", "CRAMORANT", "ELDEGOSS", "STUNFISK_GALAR", "CORSOLA_GALAR", "PINCURCHIN",
                     "SLOWBRO_GALAR", "URSALUNA", "DREEPY", ("SOBBLE", 48, 50), ("SAMUROTT_HISUI", 52, 55)),
        "water": [("ARROKUDA", 48, 52), ("SLOWPOKE_GALAR", 48, 52), ("BASCULIN_WHITE_STRIPED", 48, 52), ("CORSOLA_GALAR", 50, 54), ("DREDNAW", 50, 54)],
        "fishing": [("ARROKUDA", 45, 50), ("CHEWTLE", 45, 50),                                          # Old Rod
                    ("ARROKUDA", 48, 52), ("BASCULIN_WHITE_STRIPED", 48, 52), ("QWILFISH_HISUI", 48, 52),  # Good Rod
                    ("BARRASKEWDA", 50, 54), ("QWILFISH_HISUI", 50, 54), ("BASCULIN_WHITE_STRIPED", 50, 54),  # Super Rod
                    ("DRACOVISH", 50, 54), ("SOBBLE", 48, 50)],
        "statics": [],
        "items": [("GALARICA_CUFF", (30, 13))],
    },
    "MINE": {
        "map": "ExoticSafari_GalarMine", "build": lambda: mine(1), "tilesets": ("gTileset_General", "gTileset_Cave"),
        "version": "emerald", "convert": ("GalarMine", "galar_mine_frlg"),
        "music": "MUS_RG_MT_MOON", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "LAKE": ("warp", [(37, 12)], (37, 11), ("LAKE", "MINE")),
            "DEEP_A": ("warp", [(35, 3)], (35, 4), ("DEPTHS", "UP_A")),
            "DEEP_B": ("warp", [(17, 11)], (17, 12), ("DEPTHS", "UP_B")),
        },
        "land": land("ROLYCOLY", "SILICOBRA", "CARKOL", "STONJOURNER", "DURALUDON", "YAMASK_GALAR", "SIZZLIPEDE",
                     "CUFANT", "DRACOZOLT", "KLEAVOR", ("SCORBUNNY", 48, 50), ("COALOSSAL", 50, 54)),
        "statics": [],
        "items": [("BLACK_AUGURITE", (5, 8)), ("DRAGON_FANG", (26, 3))],
    },
    "DEPTHS": {
        "map": "ExoticSafari_GalarMineDepths", "build": lambda: mine(2), "tilesets": ("gTileset_General", "gTileset_Cave"),
        "version": "emerald", "convert": ("GalarMineDepths", "galar_mine_depths_frlg"),
        "music": "MUS_RG_MT_MOON", "weather": "WEATHER_NONE", "type": "MAP_TYPE_UNDERGROUND",
        "links": {
            "UP_A": ("warp", [(25, 13)], (25, 14), ("MINE", "DEEP_A")),
            "UP_B": ("warp", [(4, 21)], (4, 22), ("MINE", "DEEP_B")),
        },
        "remove_holes": [(29, 13), (28, 21), (8, 5), (12, 3), (29, 2)],
        "land": land("YAMASK_GALAR", "SANDACONDA", "ZORUA_HISUI", "SLIGGOO_HISUI", "DURALUDON", "STONJOURNER", "CENTISKORCH",
                     "RUNERIGUS", "DREEPY", "COPPERAJAH", ("ZOROARK_HISUI", 52, 55), ("DRAGAPULT", 55, 58)),
        "statics": [("REGIDRAGO", (20, 13))],
        "items": [("PEAT_BLOCK", (1, 1)), ("RUSTED_SHIELD", (30, 1))],
    },
    "STADIUM": {
        "map": "ExoticSafari_GalarStadium", "build": stadium_lobby, "tilesets": ("gTileset_Building", "gTileset_BattleDome"),
        "version": "emerald", "convert": ("GalarStadium", "galar_stadium_frlg"), "indoor_encounters": True,
        "music": "MUS_RG_POKE_TOWER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_INDOOR",
        "links": {
            "CITY": ("warp", [(11, 16), (12, 16)], (11, 15), ("CITY", "STADIUM")),
            "PITCH": ("edge", [(5, 5), (17, 5)], (5, 6), ("PITCH", "LOBBY")),
        },
        "land": land("MEOWTH_GALAR", "INDEEDEE", "VOLTORB_HISUI", "MILCERY", "SINISTEA", "TOXEL", "MORPEKO", "WEEZING_GALAR",
                     "PERRSERKER", "ELECTRODE_HISUI", ("INDEEDEE_F", 50, 54), ("TOXTRICITY_AMPED", 52, 55)),
        "statics": [],
        "items": [("CRACKED_POT", (2, 6))],
        "extra": lambda ctx: dojo_master(ctx),
    },
    "PITCH": {
        "map": "ExoticSafari_GalarStadiumPitch", "build": stadium_pitch, "tilesets": ("gTileset_Building", "gTileset_BattleDome"),
        "version": "emerald", "convert": ("GalarPitch", "galar_pitch_frlg"), "indoor_encounters": True,
        "tileset_edit": pitch_banner,
        "music": "MUS_RG_POKE_TOWER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_INDOOR",
        "links": {
            "LOBBY": ("edge", [(9, 13), (10, 13)], (9, 12), ("STADIUM", "PITCH")),
        },
        "land": land("FALINKS", "CLOBBOPUS", "LINOONE_GALAR", "SIRFETCHD", "LILLIGANT_HISUI", "SNEASLER", "OBSTAGOON", "GRAPPLOCT",
                     "BRAVIARY_HISUI", "OVERQWIL", ("TOXTRICITY_LOW_KEY", 52, 55), ("DECIDUEYE_HISUI", 52, 55)),
        "statics": [("KUBFU", (9, 6))],
        "items": [("SCROLL_OF_DARKNESS", (1, 12)), ("SCROLL_OF_WATERS", (18, 12))],
    },
})
AREAS.update({
    "CITY": {
        "map": "ExoticSafari_GalarCity", "build": city, "tilesets": ("gTileset_GalarSnowGeneral", "gTileset_GalarSnowCity"),
        "music": "MUS_RG_LAVENDER", "weather": "WEATHER_SNOW", "type": "MAP_TYPE_CITY",
        "links": {
            "LAKE": ("edge", [(x, 36) for x in range(5, 9)], (6, 35), ("LAKE", "CITY")),
            "STADIUM": ("edge", [(18, 0), (19, 0), (20, 0)], (19, 1), ("STADIUM", "CITY")),
        },
        "land": land("SNOM", "DARUMAKA_GALAR", "MR_MIME_GALAR", "SNEASEL_HISUI", "EISCUE", "WYRDEER", "FROSMOTH",
                     "MR_RIME", "ARCTOZOLT", "ARCTOVISH", ("DARMANITAN_GALAR", 52, 55), ("AVALUGG_HISUI", 52, 55)),
        "statics": [("ZACIAN", (17, 30)), ("ZAMAZENTA", (21, 30))],
        "items": [("RAZOR_CLAW", (6, 24))],
    },
})
AREAS.update({
    "FOREST": {
        "map": "ExoticSafari_GalarForest", "build": forest, "border_size": (3, 2), "tilesets": ("gTileset_GlimwoodGeneral", "gTileset_Glimwood"),
        "music": "MUS_RG_VIRIDIAN_FOREST", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "LAKE": ("edge", [(15, 39), (16, 39)], (15, 38), ("LAKE", "FOREST")),
        },
        "land": land("IMPIDIMP", "HATENNA", "PONYTA_GALAR", "DOTTLER", "MILCERY", "SINISTEA", "RAPIDASH_GALAR", "MORGREM",
                     "SINISTEA_ANTIQUE", "HATTREM", ("TYPHLOSION_HISUI", 52, 55), ("HATTERENE", 52, 55)),
        "statics": [("ZARUDE", (32, 3)), ("ENAMORUS", (10, 32))],
        "items": [("CHIPPED_POT", (35, 2)), ("STRAWBERRY_SWEET", (3, 7)), ("GALARICA_WREATH", (8, 28)),
                  ("RUSTED_SWORD", (38, 29))],
        "extra": lambda ctx: forest_lamps(ctx),
    },
})
# Roamers: Spectrier, the Galarian birds; Calyrex only once Spectrier is caught (its steed)
ROAMERS = ["SPECTRIER", "CALYREX", "ARTICUNO_GALAR", "ZAPDOS_GALAR", "MOLTRES_GALAR"]
ROAMER_NEEDS = {"CALYREX": "SPECTRIER"}
# What each static legendary waits for (src/exotic_safari.c; until then a hint where it will be)
CONDITIONS = {
    "ZACIAN": ("ITEM", 0), "ZAMAZENTA": ("ITEM", 1),      # (the Rusted Sword, the Rusted Shield found)
    "ZARUDE": ("LAMPS", 0), "ENAMORUS": ("FRIENDSHIP", 0), "KUBFU": ("DOJO", 0),
    "REGIELEKI": ("STONES", 0), "REGIDRAGO": ("DRAGON", 2),  # (the Dragon Fang, a Dragon-type lead)
}
# Found once, never sold (Kubfu's scrolls: one of each; the legendaries' relics); the Strawberry
# Sweet sells all seven Sweets
UNIQUE_ITEMS = {"SCROLL_OF_DARKNESS", "SCROLL_OF_WATERS", "RUSTED_SWORD", "RUSTED_SHIELD", "DRAGON_FANG"}
KIOSK_ALSO = {"STRAWBERRY_SWEET": ["STRAWBERRY_SWEET", "BERRY_SWEET", "LOVE_SWEET", "STAR_SWEET", "CLOVER_SWEET",
                                   "FLOWER_SWEET", "RIBBON_SWEET"]}


# ---------------------------------------------------------------------------------------------
# The legendaries' puzzles: their signs, triggers and people (build_exotic_safari.py's "extra"),
# and their data for src/exotic_safari.c
# ---------------------------------------------------------------------------------------------
def sign(ctx, x, y, label, body):
    ctx["bgs"].append({"type": "sign", "x": x, "y": y, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
                       "script": label})
    ctx["scripts"] += [f"{label}::"] + body + [""]


def meadow_stones(ctx):
    name = ctx["name"]
    for k, (x, y) in enumerate(STONES):
        label = f"{name}_EventScript_Stone{k}"
        ctx["coords"].append({"type": "trigger", "x": x, "y": y, "elevation": 0, "var": "VAR_TEMP_1",
                              "var_value": "0", "script": label})
        ctx["scripts"] += [f"{label}::", f"\tsetvar VAR_0x8004, {k}", "\tgoto ExoticSafari_EventScript_Stone", ""]
    sign(ctx, *STONES_SIGN, f"{name}_EventScript_StonesSign",
         ["\tmsgbox ExoticSafari_Text_StonesSign, MSGBOX_SIGN", "\tend"])


def forest_lamps(ctx):
    name = ctx["name"]
    for k, (x, y, wide) in enumerate(LAMPS):
        label = f"{name}_EventScript_Lamp{k}"
        around = [(x + a, y + b) for a in range(-1, wide + 1) for b in (-1, 0, 1, 2)
                  if (a in (-1, wide)) != (b in (-1, 2))]
        if not any(p in ctx["reach"] for p in around):
            print(f"WARNING FOREST: lamp {k} at {(x, y)} can't be touched")
        for a, b in [(a, b) for b in (0, 1) for a in range(wide)]:
            ctx["bgs"].append({"type": "sign", "x": x + a, "y": y + b, "elevation": 0,
                               "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY", "script": label})
        ctx["scripts"] += [f"{label}::", f"\tsetvar VAR_0x8004, {k}", "\tgoto ExoticSafari_EventScript_Lamp", ""]


DOJO_MASTER = (8, 12)


def dojo_master(ctx):
    o = ctx["obj"]("OBJ_EVENT_GFX_CONV_BEA", *DOJO_MASTER, "ExoticSafari_EventScript_DojoMaster", "MOVEMENT_TYPE_FACE_DOWN")
    o["elevation"] = ctx["elev"](*DOJO_MASTER)
    ctx["objects"].append(o)


def header_lines(map_id):
    assert len(LAMPS) <= 8
    return [
        "// Zarude's lamps: Glimwood's big mushrooms (top-left corners), dark until touched",
        f"#define EXOTIC_SAFARI_LAMP_MAP {map_id(AREAS['FOREST']['map'])}",
        f"#define EXOTIC_SAFARI_LAMP_BIG_LIT 0x{MUSHROOM_FIRST:03X}     // (2x2)",
        f"#define EXOTIC_SAFARI_LAMP_BIG_DARK 0x{MUSHROOM_FIRST + 12:03X}",
        f"#define EXOTIC_SAFARI_LAMP_GIANT_LIT 0x{MUSHROOM_FIRST + 6:03X}   // (3x2, in front of a tree)",
        f"#define EXOTIC_SAFARI_LAMP_GIANT_DARK 0x{MUSHROOM_FIRST + 16:03X}",
        "static const u8 sLamps[][3] = {" + ", ".join(f"{{{x}, {y}, {wd}}}" for x, y, wd in LAMPS) + "};   // x, y, width",
        "// Regieleki's stepping stones, in the order they're to be stepped on",
        f"#define EXOTIC_SAFARI_STONE_MAP {map_id(AREAS['MEADOW']['map'])}",
        f"#define EXOTIC_SAFARI_STONE_DARK 0x{STONE_FIRST:03X}   // (+ x % 2: the grass under it)",
        f"#define EXOTIC_SAFARI_STONE_LIT 0x{STONE_FIRST + 2:03X}",
        "static const u8 sStones[][2] = {" + ", ".join(f"{{{x}, {y}}}" for x, y in STONES) + "};",
        "// The items the Galar legendaries wait for (SEAL_ITEM, SEAL_DRAGON: their index)",
        "static const u16 sSealItems[][2] = {{ITEM_RUSTED_SWORD, FLAG_EXOTIC_ITEM_RUSTED_SWORD}, "
        "{ITEM_RUSTED_SHIELD, FLAG_EXOTIC_ITEM_RUSTED_SHIELD}, {ITEM_DRAGON_FANG, FLAG_EXOTIC_ITEM_DRAGON_FANG}};",
    ]
