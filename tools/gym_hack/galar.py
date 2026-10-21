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
    paste(g, r39, 14, 8, 14, 5, 26, 8)        # autumn trees (right)
    paste(g, r39, 10, 13, 18, 7, 1, 14)       # the farm: farmhouse, blue house, fences
    paste(g, r38, 13, 34, 12, 11, 27, 15)     # the fenced pond
    rect(g, 14, 7, 25, 7, grass)              # (no trees above the gap)
    rect(g, 2, 22, 15, 27, blk(0x00D, 0))     # tall grass meadows
    rect(g, 19, 13, 25, 26, blk(0x00D, 0))
    paste(g, r38, 12, 45, 40, 3, 0, 29)       # trees along the south edge
    rect(g, 17, 28, 19, 31, grass)            # the way back to the hall (south)
    rect(g, 39, 13, 39, 14, grass)            # -> the lake (east)
    return g, W, H


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
    (255, 255, 255), (60, 40, 100), (0, 0, 0), (0, 0, 0),         # 12 sparkle, 13 shade
]


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
    for k in range(4):                                             # big: top left, top right, bottom...
        add_metatile(big, (k % 2) * 16, (k // 2) * 16, ground_attr & ~0x1FF)
    covered = (ground_attr & ~0x60000000) | (1 << 29)             # (drawn under the player)
    add_metatile(s1, 0, 0, covered)
    add_metatile(s2, 0, 0, covered)
    giant, _, _ = mushroom_art(48)                                 # giant (3x2: where a tree was)
    for k in range(6):
        add_metatile(giant, (k % 3) * 16, (k // 3) * 16, ground_attr & ~0x1FF)
    assert used <= 384
    img.save(out / "tiles.png")
    (out / "metatiles.bin").write_bytes(mt)
    (out / "metatile_attributes.bin").write_bytes(at)
    B.write_pal(out / "palettes" / "12.pal", GLOW_PAL)
    global MUSHROOM_FIRST
    MUSHROOM_FIRST = 0x280 + first


MUSHROOM_FIRST = None


def place_mushrooms(g, w, h, reach_ok):
    """Big mushrooms on 2x2 patches of forest floor beside the trees (never blocking a way through:
    reach_ok(g) checks), small ones dotted on the floor"""
    import random
    rnd = random.Random("glimwood")
    floor = lambda x, y: 0 <= x < w and 0 <= y < h and g[y][x] & 0x3FF in (0x28E, 0x28F, 0x286, 0x287) and not (g[y][x] >> 10) & 3
    cells = [(x, y) for y in range(h - 1) for x in range(w - 1)]
    rnd.shuffle(cells)
    placed, spots = 0, []
    for x, y in cells:
        if placed >= 14:
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
            g[y + b][x + a] = (MUSHROOM_FIRST + k) | (1 << 10) | (3 << 12)
        if reach_ok(g):
            placed += 1
            spots.append((x, y))
        else:
            for k, (a, b) in enumerate(((0, 0), (1, 0), (0, 1), (1, 1))):
                g[y + b][x + a] = old[k]
    # Giant mushrooms in place of trees (a whole 3x2 tree) along the paths
    tree = (0x290, 0x291, 0x292, 0x298, 0x299, 0x29A)
    blocks = [(x, y) for y in range(h - 1) for x in range(w - 2)
              if [g[y + b][x + a] & 0x3FF for b in (0, 1) for a in (0, 1, 2)] == list(tree)
              and any(floor(x + a, y + d) for a in (-1, 0, 1, 2, 3) for d in (-2, -1, 2, 3))]
    rnd.shuffle(blocks)
    giants = []
    for x, y in blocks:
        if len(giants) >= 12:
            break
        if any(abs(x - px) + abs(y - py) < 7 for px, py in giants + spots):
            continue
        for k in range(6):
            g[y + k // 3][x + k % 3] = (MUSHROOM_FIRST + 6 + k) | (1 << 10) | (3 << 12)
        giants.append((x, y))
    for x, y in cells[:]:
        if floor(x, y) and rnd.random() < 0.06:
            g[y][x] = (MUSHROOM_FIRST + 4 + rnd.randrange(2)) | (3 << 12)


def forest():
    import build_exotic_safari as B
    B.clone_tileset("gTileset_Johto_South_Hns", "GlimwoodGeneral", glimwood_color)
    B.clone_tileset("gTileset_IlexForest_Hns", "Glimwood", glimwood_color, edit=add_mushrooms,
                    callback="InitTilesetAnim_Glimwood")
    g, _ = read_layout("LAYOUT_ILEX_FOREST")
    g = [row[17:57] for row in g[10:50]]
    paste(g, [r[:] for r in g], 12, 0, 6, 6, 0, 0)        # (trees over Ilex Forest's gatehouse)
    # Mushrooms: each must leave everything else reachable (the floor shrinks by just its 4 cells)
    state = {"n": len(_flood(g, (15, 38)))}
    def reach_ok(grid):
        n = len(_flood(grid, (15, 38)))
        if n == state["n"] - 4:
            state["n"] = n
            return True
        return False
    place_mushrooms(g, 40, 40, reach_ok)
    return g, 40, 40


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
    """The stands' banner says WYNDON in place of BATTLE FRONTIER: its 5 text metatiles (0x2D5-0x2D9)
    get new tiles (the banner's own colors, the text redrawn) and no top layer"""
    import struct
    from PIL import Image
    from collections import Counter
    import build_exotic_safari as B
    mt_path, img_path = out / "metatiles.bin", out / "tiles.png"
    b = bytearray(mt_path.read_bytes())
    img = Image.open(img_path)
    ids = [remap[m] - 0x280 for m in range(0x2D5, 0x2DA)]
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
        "map": "ExoticSafari_GalarMeadow", "build": meadow, "tilesets": ("gTileset_Johto_General_Hns", "gTileset_Route38_Farmland_Hns"),
        "music": "MUS_RG_ROUTE1", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "HALL": ("edge", [(17, 31), (18, 31), (19, 31)], (18, 30), ("HALL", None)),
            "LAKE": ("edge", [(39, 13), (39, 14)], (38, 13), ("LAKE", "MEADOW")),
        },
        "land": land("WOOLOO", "SKWOVET", "ROOKIDEE", "BLIPBUG", "NICKIT", "GOSSIFLEUR", "YAMPER", "APPLIN",
                     "WYRDEER", "CORVISQUIRE", ("GROOKEY", 48, 50), ("DUBWOOL", 50, 54)),
        "statics": [("REGIELEKI", (30, 28))],
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
        "land": land("CHEWTLE", "WOOLOO", "CRAMORANT", "TOXEL", "ELDEGOSS", "STUNFISK_GALAR", "PINCURCHIN",
                     "URSALUNA", "DREEPY", "CORVIKNIGHT", ("SOBBLE", 48, 50), ("DREDNAW", 50, 54)),
        "water": [("ARROKUDA", 48, 52), ("CHEWTLE", 48, 52), ("BASCULIN_WHITE_STRIPED", 48, 52), ("BARRASKEWDA", 50, 54), ("DREDNAW", 50, 54)],
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
        "items": [("BLACK_AUGURITE", (5, 8))],
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
        "land": land("CUFANT", "YAMASK_GALAR", "DURALUDON", "CARKOL", "SANDACONDA", "STONJOURNER", "CENTISKORCH",
                     "RUNERIGUS", "DREEPY", "COPPERAJAH", ("DRAKLOAK", 52, 55), ("DRAGAPULT", 55, 58)),
        "statics": [("REGIDRAGO", (20, 13))],
        "items": [("PEAT_BLOCK", (1, 1))],
    },
    "STADIUM": {
        "map": "ExoticSafari_GalarStadium", "build": stadium_lobby, "tilesets": ("gTileset_Building", "gTileset_BattleDome"),
        "version": "emerald", "convert": ("GalarStadium", "galar_stadium_frlg"), "indoor_encounters": True,
        "music": "MUS_RG_POKE_TOWER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_INDOOR",
        "links": {
            "CITY": ("warp", [(11, 16), (12, 16)], (11, 15), ("CITY", "STADIUM")),
            "PITCH": ("edge", [(5, 5), (17, 5)], (5, 6), ("PITCH", "LOBBY")),
        },
        "land": land("INDEEDEE", "MORPEKO", "SINISTEA", "MILCERY", "MR_RIME", "TOXEL", "PERRSERKER", "CUFANT",
                     "POLTEAGEIST", "ALCREMIE", ("INDEEDEE_F", 50, 54), ("TOXTRICITY_AMPED", 52, 55)),
        "statics": [],
        "items": [("CRACKED_POT", (2, 6))],
    },
    "PITCH": {
        "map": "ExoticSafari_GalarStadiumPitch", "build": stadium_pitch, "tilesets": ("gTileset_Building", "gTileset_BattleDome"),
        "version": "emerald", "convert": ("GalarPitch", "galar_pitch_frlg"), "indoor_encounters": True,
        "tileset_edit": pitch_banner,
        "music": "MUS_RG_POKE_TOWER", "weather": "WEATHER_NONE", "type": "MAP_TYPE_INDOOR",
        "links": {
            "LOBBY": ("edge", [(9, 13), (10, 13)], (9, 12), ("STADIUM", "PITCH")),
        },
        "land": land("FALINKS", "CLOBBOPUS", "TOXEL", "SIRFETCHD", "PERRSERKER", "OBSTAGOON", "SNEASLER", "OVERQWIL",
                     "GRAPPLOCT", "MR_RIME", ("TOXTRICITY_LOW_KEY", 52, 55), ("COPPERAJAH", 52, 55)),
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
        "land": land("SNOM", "DARUMAKA_GALAR", "EISCUE", "FROSMOTH", "SNEASEL_HISUI", "MR_RIME", "MORPEKO",
                     "ARCTOZOLT", "ARCTOVISH", "CUFANT", "DARMANITAN_GALAR", ("AVALUGG_HISUI", 52, 55)),
        "statics": [("ZACIAN", (17, 30)), ("ZAMAZENTA", (21, 30))],
        "items": [("RAZOR_CLAW", (6, 24))],
    },
})
AREAS.update({
    "FOREST": {
        "map": "ExoticSafari_GalarForest", "build": forest, "tilesets": ("gTileset_GlimwoodGeneral", "gTileset_Glimwood"),
        "music": "MUS_RG_VIRIDIAN_FOREST", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "LAKE": ("edge", [(15, 39), (16, 39)], (15, 38), ("LAKE", "FOREST")),
        },
        "land": land("IMPIDIMP", "HATENNA", "PONYTA_GALAR", "MILCERY", "SINISTEA", "DOTTLER", "MORGREM", "HATTREM",
                     "INDEEDEE", "SINISTEA_ANTIQUE", ("GRIMMSNARL", 52, 55), ("HATTERENE", 52, 55)),
        "statics": [("ZARUDE", (32, 3)), ("ENAMORUS", (10, 32))],
        "items": [("CHIPPED_POT", (35, 2)), ("STRAWBERRY_SWEET", (3, 7)), ("GALARICA_WREATH", (8, 28))],
    },
})
ROAMERS = ["SPECTRIER", "CALYREX"]
# Found once, never sold (Kubfu's scrolls: one of each); the Strawberry Sweet sells all seven Sweets
UNIQUE_ITEMS = {"SCROLL_OF_DARKNESS", "SCROLL_OF_WATERS"}
KIOSK_ALSO = {"STRAWBERRY_SWEET": ["STRAWBERRY_SWEET", "BERRY_SWEET", "LOVE_SWEET", "STAR_SWEET", "CLOVER_SWEET",
                                   "FLOWER_SWEET", "RIBBON_SWEET"]}
