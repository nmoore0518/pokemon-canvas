"""Pixel art for the Leader Academy (Pokémon Canvas), drawn with palette indices.

Two palettes outdoors (the campus tileset, tools/gym_hack/build_academy.py):
  BRICK: the brick buildings - brick, stone trim, slate roofs, glass, wood, gold
  DECOR: the Roserade statue, benches, lamps, flower beds
One indoors (the main hall: a library):
  LIB: wood, plaster, books, carpet, brass

Everything is drawn on a 16-pixel block grid with repeating textures lined up to the 8x8 tile grid,
so the tileset builder can share tiles. Index 0 is transparent (the ground shows through).

    python3 tools/gym_hack/academy_art.py preview.png
"""
import math

from PIL import Image, ImageDraw

T = 0

# ------------------------------------------------------------------------------- the brick palette
BRICK = [
    (255, 0, 255),       # 0 transparent
    (40, 24, 28),        # 1 outline
    (104, 34, 30),       # 2 brick, shadow
    (152, 56, 44),       # 3 brick
    (190, 88, 64),       # 4 brick, light
    (226, 218, 200),     # 5 stone / mortar
    (178, 168, 154),     # 6 stone, shade
    (120, 110, 106),     # 7 stone, dark
    (44, 52, 74),        # 8 slate, dark
    (70, 82, 112),       # 9 slate
    (108, 120, 156),     # 10 slate, light
    (38, 62, 100),       # 11 glass, dark
    (126, 174, 214),     # 12 glass, light
    (118, 72, 38),       # 13 wood
    (214, 170, 72),      # 14 gold
    (250, 248, 238),     # 15 white
]
OUT, BR_D, BR, BR_L, ST, ST_S, ST_D, SL_D, SL, SL_L, GL_D, GL_L, WOOD, GOLD, WHITE = range(1, 16)


def new(w, h):
    return Image.new("P", (w, h), T)


def outline(img, color=1):
    px = img.load()
    w, h = img.size
    edge = []
    for y in range(h):
        for x in range(w):
            if px[x, y] != T:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and px[nx, ny] not in (T, color):
                    edge.append((x, y))
                    break
    for p in edge:
        px[p] = color
    return img


# ---- textures (8-pixel periods, lined up with the tiles) ----
def brick_at(x, y):
    """Running bond: courses 4 px (3 brick + 1 mortar), joints every 8 px, staggered by 4"""
    cy = y % 4
    if cy == 3:
        return ST_S
    off = 0 if (y // 4) % 2 == 0 else 4
    if (x + off) % 8 == 7:
        return ST_S
    if cy == 0:
        return BR_L if (x + off) % 8 < 3 else BR
    return BR if cy == 1 else BR_D if (x + off) % 8 == 6 else BR


def slate_at(x, y):
    """Slate shingles: courses 4 px, staggered by 4, lit along the top"""
    cy = y % 4
    off = 0 if (y // 4) % 2 == 0 else 4
    if cy == 3:
        return SL_D
    if (x + off) % 8 == 0:
        return SL_D
    return SL_L if cy == 0 else SL


def fill(px, x0, y0, x1, y1, f):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            px[x, y] = f(x, y) if callable(f) else f


def window(px, x, y, h, arched=True):
    """A window in the 16-px block at x: 8 px of glass (x+4..x+11), h tall from y, a white frame,
    a sash bar and a stone sill"""
    for yy in range(y, y + h):
        for xx in range(x + 3, x + 13):
            px[xx, yy] = WHITE
    for yy in range(y + 1, y + h - 1):
        for xx in range(x + 4, x + 12):
            px[xx, yy] = GL_L if (xx - x - 4) + (yy - y) < 6 else GL_D
    for xx in range(x + 4, x + 12):
        px[xx, y + h // 2] = WHITE
    for yy in range(y + 1, y + h - 1):
        px[x + 8, yy] = WHITE
    if arched:                             # (rounded top: the corners back to brick)
        for (dx, dy) in ((3, 0), (4, 0), (11, 0), (12, 0), (3, 1), (12, 1)):
            px[x + dx, y + dy] = brick_at(x + dx, y + dy)
        px[x + 4, y + 1] = WHITE
        px[x + 11, y + 1] = WHITE
    for xx in range(x + 2, x + 14):        # sill
        px[xx, y + h] = ST
        px[xx, y + h + 1] = ST_D


def roof(px, x0, y0, x1, y1, hip=4):
    """A slate roof from y0 to y1, hipped in by `hip` px at the top, with a white eave and its shadow"""
    for y in range(y0, y1 + 1):
        inset = max(0, hip - (y - y0) // 2) if hip else 0
        for x in range(x0 + inset, x1 + 1 - inset):
            px[x, y] = slate_at(x, y)
        if inset or y == y0:
            pass
    for x in range(x0, x1 + 1):            # ridge
        if px[x, y0] != T:
            px[x, y0] = SL_L
        px[x, y1 - 2] = WHITE              # the eave's fascia
        px[x, y1 - 1] = ST_S
        px[x, y1] = BR_D                   # its shadow on the wall


def plinth(px, x0, x1, y1):
    """Stone base along the bottom of a wall"""
    for x in range(x0, x1 + 1):
        px[x, y1 - 3] = ST
        px[x, y1 - 2] = ST_S
        px[x, y1 - 1] = ST_S
        px[x, y1] = ST_D


def door(px, x, y, double=True):
    """A wooden door filling the block at (x, y) (16x16): frame, two leaves, brass handles"""
    fill(px, x, y, x + 15, y + 15, ST)
    fill(px, x + 2, y + 1, x + 13, y + 15, WOOD)
    for yy in range(y + 1, y + 16):
        px[x + 2, yy] = OUT
        px[x + 13, yy] = OUT
    for xx in range(x + 2, x + 14):
        px[xx, y + 1] = OUT
    if double:
        for yy in range(y + 2, y + 16):
            px[x + 7, yy] = OUT
            px[x + 8, yy] = OUT
    for (xx, yy) in ((x + 4, y + 4), (x + 4, y + 10), (x + 10, y + 4), (x + 10, y + 10)):   # panels
        for k in range(3):
            px[xx + k, yy] = BR_D
            px[xx + k, yy + 4] = BR_L
    px[x + 6, y + 9] = GOLD
    px[x + 9, y + 9] = GOLD


def building(wb, hb, roof_h=2, storeys=2, door_col=None, door_style="double", windows="arched", extra=None):
    """A brick building wb x hb blocks: roof_h blocks of slate on top, then brick walls with a row of
    windows per storey (one per block column; none in the door's column), a stone plinth, a door in
    the bottom row of door_col"""
    W, H = wb * 16, hb * 16
    img = new(W, H)
    px = img.load()
    ry = roof_h * 16
    fill(px, 0, ry, W - 1, H - 1, brick_at)
    roof(px, 0, 0, W - 1, ry + 2, hip=6)
    plinth(px, 0, W - 1, H - 1)
    wall_h = H - ry - 4
    storey_h = wall_h // storeys
    for s in range(storeys):
        top = ry + 4 + s * storey_h
        if s > 0:
            for x in range(W):              # string course between the storeys
                px[x, top - 3] = ST
                px[x, top - 2] = ST_S
        for c in range(wb):
            if c == door_col and s == storeys - 1:
                continue
            window(px, c * 16, top + 2, storey_h - 8, arched=windows == "arched")
    for x in (0, W - 1):                    # stone quoins at the corners
        for y in range(ry + 2, H - 4):
            px[x, y] = ST if (y // 4) % 2 == 0 else ST_S
    if door_col is not None:
        door(px, door_col * 16, H - 16, double=door_style == "double")
    if extra:
        extra(img, px)
    return outline(img)


def main_hall():
    """The Academy's main hall: 13 x 9 blocks. Brick wings under a slate roof, a clock tower over a
    white columned portico with a rose in its pediment, arched windows, the door at (6, 8)"""
    wb, hb = 13, 9
    W, H = wb * 16, hb * 16
    img = new(W, H)
    px = img.load()
    roof_top, wall_top = 16, 48
    fill(px, 0, wall_top, W - 1, H - 1, brick_at)
    roof(px, 0, roof_top, W - 1, wall_top + 2, hip=8)
    plinth(px, 0, W - 1, H - 1)
    # two storeys of arched windows in the wings
    for c in list(range(0, 4)) + list(range(9, 13)):
        window(px, c * 16, 54, 20)
        window(px, c * 16, 92, 24)
    for x in range(W):                      # string course
        if not 64 <= x <= 143:
            px[x, 86] = ST
            px[x, 87] = ST_S
    for x in (0, W - 1):
        for y in range(wall_top + 2, H - 4):
            px[x, y] = ST if (y // 4) % 2 == 0 else ST_S
    # the clock tower (blocks 5-7), from the top
    tx0, tx1 = 80, 127
    fill(px, tx0 + 4, 14, tx1 - 4, 47, brick_at)
    for y in range(0, 16):                  # pyramid cap
        half = 4 + int((y / 15) * 20)
        for x in range(104 - half, 104 + half):
            px[x, y] = slate_at(x, y)
    for y in range(0, 5):                   # gold finial
        px[103, y] = GOLD
        px[104, y] = GOLD
    for x in range(tx0 + 2, tx1 - 1):       # cornice under the cap
        px[x, 15] = WHITE
        px[x, 16] = ST_S
    cx, cy, r = 103.5, 30, 9.5              # the clock
    for y in range(18, 43):
        for x in range(90, 118):
            d = math.hypot(x - cx, y - cy)
            if d <= r:
                px[x, y] = GOLD if d > r - 1.5 else WHITE
    for k in range(12):
        a = k * math.pi / 6
        px[int(round(cx + math.cos(a) * 6.5)), int(round(cy + math.sin(a) * 6.5))] = OUT
    for t in range(5):                      # hands: ten past two
        px[int(round(cx + t * 0.9)), int(round(cy - t * 1.0))] = OUT
        px[int(round(cx + t * 1.2)), int(round(cy + 0.2 * t))] = OUT
    # the portico: pediment (blocks 4-8) and white columns
    px0, px1 = 64, 143
    for y in range(44, 64):
        half = int((y - 44) / 19 * 40)
        for x in range(104 - half, 104 + half):
            if px0 <= x <= px1:
                px[x, y] = WHITE if (y - 44) < 2 or abs(x - 104) >= half - 2 else ST
    for y in range(64, 68):                 # entablature
        for x in range(px0, px1 + 1):
            px[x, y] = WHITE if y < 66 else ST_S
    rose(px, 104, 56)
    for x in range(px0, px1 + 1):           # the portico's shade behind the columns
        for y in range(68, H - 4):
            if px[x, y] in (BR, BR_L, BR_D, ST_S):
                px[x, y] = BR_D if brick_at(x, y) != ST_S else ST_D
    for c in (68, 84, 120, 136):            # columns
        column(px, c, 68, H - 5)
    # tall windows between the columns; the doorway: two blocks tall, a fanlight in its arch
    for c in (5, 7):
        window(px, c * 16, 76, 44)
    for y in range(106, 128):
        for x in range(96, 112):
            d = math.hypot(x - 103.5, max(0, 114 - y))
            if d <= 8:
                px[x, y] = ST if d > 6.5 or x in (96, 111) else (WOOD if y >= 115 else (GL_L if x < 104 else GL_D))
        if y >= 115:
            px[103, y] = OUT
            px[104, y] = OUT
    for x in range(98, 110):
        px[x, 114] = WHITE
    door(px, 96, 128)
    for y in range(128, 144):               # (the door's frame continues the arch's)
        px[96, y] = ST
        px[111, y] = ST
    # steps
    for x in range(92, 116):
        px[x, H - 2] = ST
        px[x, H - 1] = ST_D
    return outline(img)


def column(px, x, y0, y1):
    """A white column 6 px wide, from y0 (capital) to y1 (base)"""
    for y in range(y0, y1 + 1):
        for dx in range(6):
            px[x + dx, y] = WHITE if dx < 3 else (ST if dx < 5 else ST_S)
    for dx in range(-1, 7):
        px[x + dx, y0] = WHITE
        px[x + dx, y0 + 1] = ST
        px[x + dx, y1] = ST
        px[x + dx, y1 - 1] = WHITE


def rose(px, cx, cy):
    """A red rose with two leaves (the Academy's crest), about 10x8, centered at (cx, cy)"""
    for y in range(cy - 4, cy + 4):
        for x in range(cx - 4, cx + 4):
            d = math.hypot(x - cx + 0.5, y - cy + 0.5)
            if d <= 3.6:
                px[x, y] = BR_L if d < 1.6 else (BR if (x + y) % 3 else BR_D)
    for (x, y) in ((cx - 6, cy + 2), (cx - 5, cy + 2), (cx - 5, cy + 3), (cx + 4, cy + 2), (cx + 5, cy + 2), (cx + 4, cy + 3)):
        px[x, y] = GOLD


def battle_hall():
    """The Battle Hall: 7 x 5 blocks, a Poké Ball banner over the door"""
    def extra(img, px):
        x0 = 3 * 16
        for y in range(34, 62):             # banner under the eave, over the door's block
            for x in range(x0 + 2, x0 + 14):
                px[x, y] = BR_D if x in (x0 + 2, x0 + 13) else BR
        for x in range(x0 + 2, x0 + 14):
            px[x, 34] = GOLD
        cx, cy = x0 + 7.5, 46
        for y in range(40, 53):
            for x in range(x0 + 2, x0 + 14):
                d = math.hypot(x - cx, y - cy)
                if d <= 5.5:
                    px[x, y] = OUT if d > 4.5 or abs(y - cy) < 0.6 else (BR_L if y < cy else WHITE)
        px[int(cx), cy] = WHITE
        px[int(cx) + 1, cy] = WHITE
    return building(7, 5, door_col=3, storeys=2, extra=extra)


def exchange():
    """The Exchange: 5 x 4 blocks, a striped awning over its door"""
    def extra(img, px):
        x0, y0 = 32, 44
        for y in range(y0, y0 + 6):
            for x in range(x0 - 4, x0 + 20):
                px[x, y] = GOLD if ((x - x0) // 4) % 2 == 0 else WHITE
        for x in range(x0 - 4, x0 + 20):
            px[x, y0 + 6] = GOLD if ((x - x0) // 4) % 2 == 0 else OUT
            px[x, y0 - 1] = OUT
    return building(5, 4, door_col=2, storeys=1, extra=extra)


def dorm():
    """A dorm: 6 x 5 blocks, three storeys of square windows, no door (it's round the back)"""
    return building(6, 5, roof_h=2, storeys=2, windows="square")


# ------------------------------------------------------------------------------- the decor palette
DECOR = [
    (255, 0, 255),       # 0 transparent
    (32, 36, 40),        # 1 outline
    (40, 96, 56),        # 2 green, dark
    (72, 150, 80),       # 3 green
    (132, 200, 112),     # 4 green, light
    (248, 248, 240),     # 5 white
    (204, 204, 210),     # 6 grey, light (pedestal)
    (150, 152, 164),     # 7 grey
    (98, 100, 116),      # 8 grey, dark
    (200, 40, 56),       # 9 red
    (240, 104, 120),     # 10 red, light
    (52, 80, 200),       # 11 blue
    (120, 152, 240),     # 12 blue, light
    (240, 208, 96),      # 13 yellow (lamp light, Roserade's eyes)
    (150, 98, 54),       # 14 wood
    (64, 66, 80),        # 15 iron
]
D_OUT, G_D, G, G_L, D_WHITE, P_L, P, P_D, RED, RED_L, BLUE, BLUE_L, YEL, D_WOOD, IRON = range(1, 16)


def roserade_statue():
    """Roserade on a stone pedestal: 32x48 (2x3 blocks). The figure is Roserade's own overworld
    sprite (facing down, graphics/pokemon/roserade/overworld.png), its colors matched to this palette"""
    from pathlib import Path
    root = Path(__file__).resolve().parents[2] / "graphics/pokemon/roserade"
    src = Image.open(root / "overworld.png")
    vals = (root / "overworld_normal.pal").read_text().split()[3:]
    cols = [tuple(int(vals[i * 3 + k]) for k in range(3)) for i in range(16)]
    near = [0] + [min(range(1, 16), key=lambda j: sum((a - b) ** 2 for a, b in zip(cols[c], DECOR[j])))
                  for c in range(1, 16)]
    img = new(32, 48)
    d = ImageDraw.Draw(img)
    px, sp = img.load(), src.load()
    # pedestal, with a brass plaque
    d.rectangle([5, 34, 26, 46], fill=P)
    d.rectangle([3, 33, 28, 35], fill=P_L)
    d.line([(3, 33), (28, 33)], fill=D_WHITE)
    d.rectangle([5, 44, 26, 46], fill=P_D)
    d.rectangle([3, 45, 28, 46], fill=P_D)
    d.rectangle([10, 38, 21, 42], fill=YEL)
    d.line([(11, 40), (20, 40)], fill=D_WOOD)
    bottom = max(y for y in range(32) for x in range(32) if sp[x, y])
    dy = 32 - bottom
    for y in range(32):
        for x in range(32):
            if sp[x, y] and 0 <= y + dy < 48:
                px[x, y + dy] = near[sp[x, y]]
    return outline(img)


def lamp():
    """An iron street lamp: 16x32 (1x2 blocks)"""
    img = new(16, 32)
    d = ImageDraw.Draw(img)
    d.polygon([(4, 4), (11, 4), (13, 7), (2, 7)], fill=IRON)
    d.rectangle([7, 1, 8, 3], fill=IRON)
    d.rectangle([4, 8, 11, 14], fill=YEL)
    d.rectangle([5, 9, 6, 12], fill=D_WHITE)
    d.line([(4, 8), (4, 14)], fill=IRON)
    d.line([(11, 8), (11, 14)], fill=IRON)
    d.rectangle([3, 15, 12, 16], fill=IRON)
    d.rectangle([7, 17, 8, 27], fill=IRON)
    d.line([(7, 17), (7, 27)], fill=P_D)
    d.rectangle([5, 27, 10, 29], fill=IRON)
    d.line([(5, 27), (10, 27)], fill=P_D)
    return outline(img)


def bench():
    """A wooden bench on iron legs: 32x16 (2x1 blocks)"""
    img = new(32, 16)
    d = ImageDraw.Draw(img)
    d.rectangle([2, 1, 29, 3], fill=D_WOOD)
    d.rectangle([2, 5, 29, 7], fill=D_WOOD)
    d.line([(2, 1), (29, 1)], fill=YEL)
    d.line([(2, 5), (29, 5)], fill=YEL)
    d.rectangle([4, 4, 5, 8], fill=IRON)
    d.rectangle([26, 4, 27, 8], fill=IRON)
    d.rectangle([1, 9, 30, 11], fill=D_WOOD)
    d.line([(1, 9), (30, 9)], fill=YEL)
    d.rectangle([3, 12, 5, 14], fill=IRON)
    d.rectangle([26, 12, 28, 14], fill=IRON)
    return outline(img)


def rose_bed():
    """A flower bed of red, blue and white roses in dark leaves: 16x16 (tiles with the next one)"""
    img = new(16, 16)
    px = img.load()
    for y in range(16):
        for x in range(16):
            px[x, y] = G_D if (x * 7 + y * 3) % 5 else G
    spots = ((3, 3, RED), (11, 2, BLUE), (7, 7, D_WHITE), (2, 11, BLUE), (12, 10, RED), (7, 13, RED))
    for (cx, cy, c) in spots:
        light = {RED: RED_L, BLUE: BLUE_L, D_WHITE: D_WHITE}[c]
        for (dx, dy) in ((0, 0), (1, 0), (0, 1), (1, 1), (-1, 0), (2, 1), (0, -1), (1, 2)):
            x, y = (cx + dx) % 16, (cy + dy) % 16
            px[x, y] = c
        px[cx % 16, cy % 16] = light
    return img


def hedge():
    """A trimmed hedge: 16x16 (repeats sideways)"""
    img = new(16, 16)
    px = img.load()
    for y in range(2, 15):
        for x in range(16):
            px[x, y] = G_L if y < 5 else (G if (x + y) % 4 else G_D)
    for x in range(16):
        px[x, 1] = D_OUT
        px[x, 15] = D_OUT
        if x % 4 == 1:
            px[x, 2] = D_WHITE if False else G_L
    return img


# ------------------------------------------------------------------------------- the library palette
LIB = [
    (255, 0, 255),       # 0 transparent
    (36, 24, 20),        # 1 outline
    (82, 48, 28),        # 2 wood, dark
    (124, 78, 42),       # 3 wood
    (168, 114, 62),      # 4 wood, light
    (206, 156, 96),      # 5 wood, highlight
    (234, 222, 192),     # 6 plaster
    (198, 182, 150),     # 7 plaster, shade
    (52, 108, 70),       # 8 green (books, the carpet's border)
    (160, 44, 44),       # 9 red (books, the carpet)
    (54, 84, 148),       # 10 blue (books)
    (222, 180, 72),      # 11 brass
    (248, 244, 232),     # 12 white (pages, paper)
    (140, 134, 128),     # 13 grey
    (78, 74, 76),        # 14 dark grey
    (164, 204, 232),     # 15 glass
]
L_OUT, W_D, WD, W_L, W_H, PL, PL_S, L_GREEN, L_RED, L_BLUE, BRASS, PAGE, L_GREY, L_DGREY, L_GLASS = range(1, 16)


def parquet():
    """Wooden floorboards: 16x16 (one block, repeats): planks 4 px tall, their ends staggered"""
    img = new(16, 16)
    px = img.load()
    for y in range(16):
        row = y // 4
        off = (row * 6) % 16
        for x in range(16):
            k = y % 4
            if k == 3 or (x + off) % 16 == 0:
                c = WD
            else:
                c = W_H if k == 0 else W_L
                if (x * 3 + row * 5) % 11 == 0 and k == 2:
                    c = WD
            px[x, y] = c
    return img


def doormat():
    """The way out: a red mat with a brass edge on the floorboards: 16x16"""
    img = parquet()
    px = img.load()
    for y in range(3, 15):
        for x in range(1, 16):
            px[x, y] = BRASS if y in (3, 14) else (L_RED if (x + y) % 4 else 9)
    return img


def carpet(part):
    """The red carpet runner: 'l', 'm', 'r' (16x16 each), gold border on its edges"""
    img = new(16, 16)
    px = img.load()
    for y in range(16):
        for x in range(16):
            c = L_RED
            if (x + y) % 6 == 0:
                c = 9
            if part == "l" and x < 3:
                c = BRASS if x == 1 else L_GREEN
            if part == "r" and x > 12:
                c = BRASS if x == 14 else L_GREEN
            px[x, y] = c
    return img


def wall(kind="plain"):
    """The wall: 16x32 (two blocks): plaster with a crown molding over dark wood wainscot"""
    img = new(16, 32)
    px = img.load()
    for y in range(32):
        for x in range(16):
            if y < 3:
                c = W_H if y == 1 else W_D
            elif y < 18:
                c = PL if (y - 3) % 7 else PL_S
                c = PL
            elif y < 20:
                c = W_H if y == 18 else W_D
            elif y < 30:
                c = WD if x % 8 not in (0, 7) else W_D
                if y in (21, 28) and x % 8 not in (0, 7):
                    c = W_L
            else:
                c = W_D
            px[x, y] = c
    return img


def bookshelf(variant=0):
    """A tall bookshelf: 16x32 (two blocks), three shelves of books in red, blue, green and brown"""
    img = new(16, 32)
    px = img.load()
    for y in range(32):
        for x in range(16):
            px[x, y] = WD
    for y in range(32):
        px[0, y] = L_OUT
        px[15, y] = L_OUT
        px[1, y] = W_L
        px[14, y] = W_D
    for x in range(16):
        px[x, 0] = L_OUT
        px[x, 1] = W_H
        px[x, 2] = W_L
    palette = [L_RED, L_BLUE, L_GREEN, W_L, L_RED, PAGE, L_BLUE, L_GREEN, W_H]
    rows = ((4, 11), (13, 20), (22, 29))
    k = variant * 5
    for (y0, y1) in rows:
        x = 2
        while x < 14:
            w = 1 + (k * 7 + x) % 2
            c = palette[(k * 3 + x * 5) % len(palette)]
            top = y0 + (k + x) % 3
            for xx in range(x, min(x + w, 14)):
                for y in range(top, y1):
                    px[xx, y] = c
                px[xx, top] = PAGE if c in (L_RED, L_BLUE, L_GREEN) and (xx + k) % 3 == 0 else c
            x += w
            k += 1
        for xx in range(2, 14):
            px[xx, y0 - 1] = W_D
            px[xx, y1] = W_L
            px[xx, y1 + 1] = W_D
    for x in range(16):
        px[x, 30] = W_D
        px[x, 31] = L_OUT
    return img


def lecture_door(letter):
    """A lecture hall's door: 16x32 (two blocks): a brass plate with its letter over a paneled door"""
    img = wall()
    px = img.load()
    font = {
        "A": ["010", "101", "111", "101", "101"], "B": ["110", "101", "110", "101", "110"],
        "C": ["011", "100", "100", "100", "011"], "D": ["110", "101", "101", "101", "110"],
    }
    for y in range(5, 13):
        for x in range(4, 12):
            px[x, y] = BRASS if 5 < x < 11 and 5 < y < 12 else W_D
    for y, row in enumerate(font[letter]):
        for x, b in enumerate(row):
            if b == "1":
                px[7 + x, 6 + y] = L_OUT
    for y in range(15, 32):
        for x in range(1, 15):
            px[x, y] = W_D if x in (1, 14) or y == 15 else WD
    for (x0, y0) in ((3, 17), (3, 25), (9, 17), (9, 25)):
        for y in range(y0, y0 + 5):
            for x in range(x0, x0 + 4):
                px[x, y] = W_L if y == y0 or x == x0 else WD
    px[11, 23] = BRASS
    px[11, 24] = BRASS
    return img


def tall_window():
    """An arched window in the wall: 16x32"""
    img = wall()
    px = img.load()
    for y in range(4, 17):
        for x in range(3, 13):
            d = math.hypot(x - 7.5, max(0, 9 - y))
            if d <= 5:
                px[x, y] = W_D if d > 4 else (L_GLASS if x + y < 17 else PAGE if (x + y) % 5 == 0 else L_GLASS)
    for x in range(3, 13):
        px[x, 17] = W_H
    return img


def reading_table():
    """A reading table with an open book and a brass lamp: 32x16 (2 blocks)"""
    img = new(32, 16)
    px = img.load()
    for y in range(2, 11):
        for x in range(1, 31):
            px[x, y] = W_L if y < 4 else WD
    for x in range(1, 31):
        px[x, 1] = L_OUT
        px[x, 11] = W_D
    for y in range(11, 15):
        for x in (3, 4, 27, 28):
            px[x, y] = W_D
    for y in range(4, 8):                   # an open book
        for x in range(7, 15):
            px[x, y] = PAGE if x != 10 else L_GREY
    for x in range(8, 14, 2):
        px[x, 5] = L_GREY
    for y in range(1, 8):                   # the lamp
        px[22, y] = BRASS
    for x in range(19, 26):
        px[x, 0] = L_GREEN
        px[x, 1] = L_GREEN
    return outline(img)


def desk():
    """The headmaster's desk: 48x16 (3 blocks), wide and dark, papers and an inkwell"""
    img = new(48, 16)
    px = img.load()
    for y in range(1, 15):
        for x in range(1, 47):
            px[x, y] = W_L if y < 4 else (W_D if y > 12 else WD)
    for x in range(1, 47):
        px[x, 0] = L_OUT
    for (x0, y0) in ((6, 6), (34, 6)):
        for y in range(y0, y0 + 5):
            for x in range(x0, x0 + 8):
                px[x, y] = W_L if y == y0 or x == x0 else W_D
    for y in range(2, 5):
        for x in range(18, 26):
            px[x, y] = PAGE
    px[30, 2] = L_DGREY
    px[30, 3] = L_DGREY
    px[31, 3] = L_DGREY
    return outline(img)


def globe():
    """A globe on a wooden stand: 16x16"""
    img = new(16, 16)
    px = img.load()
    for y in range(1, 11):
        for x in range(2, 14):
            d = math.hypot(x - 7.5, y - 5.5)
            if d <= 4.5:
                px[x, y] = L_GREEN if (x * 3 + y * 5) % 7 < 3 else L_BLUE
            elif d <= 5.4 and x > 7:
                px[x, y] = BRASS
    for y in range(11, 15):
        px[7, y] = W_D
        px[8, y] = W_D
    for x in range(4, 12):
        px[x, 14] = W_D
    return outline(img)


CAMPUS = {"main_hall": main_hall, "battle_hall": battle_hall, "exchange": exchange, "dorm": dorm}
DECOR_ART = {"statue": roserade_statue, "lamp": lamp, "bench": bench, "rose_bed": rose_bed, "hedge": hedge}
LIBRARY = {"parquet": parquet, "carpet_l": lambda: carpet("l"), "carpet_m": lambda: carpet("m"),
           "carpet_r": lambda: carpet("r"), "wall": wall, "window": tall_window,
           "shelf0": lambda: bookshelf(0), "shelf1": lambda: bookshelf(1), "shelf2": lambda: bookshelf(2),
           "door_A": lambda: lecture_door("A"), "door_B": lambda: lecture_door("B"),
           "door_C": lambda: lecture_door("C"), "door_D": lambda: lecture_door("D"),
           "table": reading_table, "desk": desk, "globe": globe}


def to_rgb(img, pal, back=(116, 190, 120)):
    out = Image.new("RGB", img.size, back)
    px, op = img.load(), out.load()
    for y in range(img.height):
        for x in range(img.width):
            if px[x, y]:
                op[x, y] = pal[px[x, y]]
    return out


def preview(path, scale=3):
    groups = [(CAMPUS, BRICK), (DECOR_ART, DECOR), (LIBRARY, LIB)]
    ims = []
    for art, pal in groups:
        for name, f in art.items():
            ims.append(to_rgb(f(), pal))
    W = 900
    x = y = rowh = 0
    pos = []
    for im in ims:
        if x + im.width > W:
            x, y = 0, y + rowh + 4
            rowh = 0
        pos.append((x, y))
        x += im.width + 4
        rowh = max(rowh, im.height)
    sheet = Image.new("RGB", (W, y + rowh), (30, 30, 30))
    for im, p in zip(ims, pos):
        sheet.paste(im, p)
    sheet.resize((sheet.width * scale, sheet.height * scale), Image.NEAREST).save(path)


if __name__ == "__main__":
    import sys
    preview(sys.argv[1])
