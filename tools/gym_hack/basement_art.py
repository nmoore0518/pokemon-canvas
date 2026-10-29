"""Pixel art for the basement under the player's house (Pokémon Canvas): Dad's workshop.

Drawn as one scene, 16-pixel blocks, in two palettes (tools/gym_hack/build_basement.py cuts it into
metatiles):
  ROOM: concrete walls and floor, pipes, the caged lamps, wood, paper, cobwebs - the room layer
  MACH: steel, brass, glass, the glowing liquid, rubber, Poké Ball red and white - the machines,
        drawn over the room (index 0 lets the room show through)

The machines have frames for their animations (build_basement.FRAMES says which blocks change).

    python3 tools/gym_hack/basement_art.py preview.png
"""
from PIL import Image

W, H = 15, 11                     # the map, in blocks

ROOM = [
    (255, 0, 255),       # 0 (unused in the room layer: it's solid)
    (18, 16, 20),        # 1 outline, the dark
    (38, 40, 44),        # 2 concrete, darkest
    (56, 58, 62),        # 3 concrete, dark
    (74, 77, 78),        # 4 concrete
    (96, 98, 96),        # 5 concrete, light
    (124, 124, 116),     # 6 mortar, lit concrete
    (58, 72, 56),        # 7 grime (green)
    (84, 48, 34),        # 8 rust, dark
    (138, 84, 48),       # 9 rust, copper
    (64, 44, 30),        # 10 wood, dark
    (104, 72, 44),       # 11 wood
    (146, 104, 64),      # 12 wood, light
    (206, 196, 168),     # 13 paper
    (168, 172, 176),     # 14 cobweb, chalk
    (236, 204, 112),     # 15 lamplight
]
O, C0, C1, C2, C3, C4, GRIME, RUST_D, RUST, WD_D, WD, WD_L, PAPER, WEB, LAMP = range(1, 16)

MACH = [
    (255, 0, 255),       # 0 transparent
    (14, 14, 18),        # 1 outline
    (48, 54, 62),        # 2 steel, dark
    (78, 86, 96),        # 3 steel
    (116, 126, 136),     # 4 steel, light
    (170, 180, 188),     # 5 steel, shine
    (110, 80, 30),       # 6 brass, dark
    (176, 136, 54),      # 7 brass
    (44, 70, 76),        # 8 glass, dark
    (30, 104, 62),       # 9 liquid, deep
    (64, 186, 92),       # 10 liquid
    (176, 252, 150),     # 11 liquid, glow
    (204, 52, 44),       # 12 red (lights, Poké Balls)
    (30, 28, 30),        # 13 rubber
    (232, 230, 222),     # 14 white
    (232, 168, 40),      # 15 amber
]
M_O, S0, S1, S2, S3, BR_D, BR, GL, LQ_D, LQ, LQ_G, RED, RUB, WHITE, AMBER = range(1, 16)

# Where things are (block x, y) - build_basement.py uses the same
SYNTH = (1, 1)          # 4x3: the belt (x+0..1, rows y+1..2), the tank / nozzle / vat (x+2..3, rows y..y+2)
SWITCH = (5, 1)         # 1x1, on the wall's lower row
MAKER = (6, 1)          # 3x3: hopper on top, the body and its slot below, the crank on the right
BOARD = (9, 0)          # 2x2 chalkboard
SHELF = (11, 0)         # 1x2 bookshelf
STAIRS = (12, 1)        # the stairs up: the warp (x, y+1) at their foot, the steps at x+1..x+2, rows 0..y+1
LAMPS = (5, 12)         # caged lamps on the wall (x; row 0)
BENCH = (0, 5)          # 2x2 workbench
CHANGER = (5, 6)        # pedestals at x and x+3 (rows y..y+1), the console between (x+1..x+2, row y+1)
LABTABLE = (11, 6)      # 2x2 lab table with beakers
DESK = (13, 5)          # 2x2 desk, lamp and notes
TRASH = (0, 9)
CRATES = [(13, 9), (14, 9), (14, 8)]
PAPERS = [(2, 8), (10, 4), (7, 9), (12, 8)]


def h(x, y, k=0):
    """a fixed pseudo-random number 0-255 for a pixel"""
    v = (x * 374761393 + y * 668265263 + k * 2246822519) & 0xFFFFFFFF
    v = ((v ^ (v >> 13)) * 1274126177) & 0xFFFFFFFF
    return (v ^ (v >> 16)) & 0xFF


class Scene:
    def __init__(self):
        self.room = Image.new("P", (W * 16, H * 16), C1)
        self.mach = Image.new("P", (W * 16, H * 16), 0)
        self.r, self.m = self.room.load(), self.mach.load()

    def rset(self, x, y, c):
        if 0 <= x < W * 16 and 0 <= y < H * 16:
            self.r[x, y] = c

    def mset(self, x, y, c):
        if 0 <= x < W * 16 and 0 <= y < H * 16:
            self.m[x, y] = c

    def rrect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.rset(x, y, c)

    def mrect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.mset(x, y, c)


# ---------------------------------------------------------------------------------------------
# The room
# ---------------------------------------------------------------------------------------------
def light_at(px, py):
    """how lit a spot is: the two lamps on the back wall, falling off with distance (0 dark - 1 lit);
    one value per block (so the tiles repeat: a secondary tileset has 384)"""
    px, py = (px // 16) * 16 + 8, (py // 16) * 16 + 8
    best = 0.0
    for lx in LAMPS:
        cx, cy = lx * 16 + 8, 6
        d = ((px - cx) ** 2 + ((py - cy) * 1.2) ** 2) ** 0.5
        best = max(best, 1.0 - d / 120.0)
    return best


def floor_px(x, y):
    """the concrete floor: a base shade by the light, speckles on a 16-pixel period (tiles share)"""
    lit = light_at(x, y)
    if lit > 0.5:
        base = C2
    elif lit > 0.32:
        base = C2 if (x + y) % 2 else C1          # (the light's edge: dithered)
    else:
        base = C1
    s = h(x % 16, y % 16)
    if s < 14:
        return max(C0, base - 1)
    if s > 246:
        return min(C4, base + 1)
    return base


def draw_floor(s):
    for y in range(32, H * 16):
        for x in range(W * 16):
            s.rset(x, y, floor_px(x, y))
    # the wall's shadow on the floor
    for x in range(W * 16):
        s.rset(x, 32, O)
        s.rset(x, 33, C0)
        if x % 2:
            s.rset(x, 34, C0)
    # stains: the green leak under the synthesizer, rust by the pipes, a dark blot by the changer
    def blot(cx, cy, rx, ry, c, k):
        for y in range(cy - ry, cy + ry + 1):
            for x in range(cx - rx, cx + rx + 1):
                d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
                if d < 1 and (d < 0.5 or h(x, y, k) < 150):
                    s.rset(x, y, c)
    blot(3 * 16 + 10, 4 * 16 + 6, 14, 5, GRIME, 1)
    blot(9 * 16 + 4, 9 * 16 + 8, 7, 4, RUST_D, 3)
    # cracks
    def crack(x, y, steps, k):
        for i in range(steps):
            s.rset(x, y, O if i % 3 else C0)
            r = h(i, k, 9) % 4
            x += 1 if r < 3 else 0
            y += (1 if r == 1 else -1 if r == 2 else 0)
    crack(5 * 16 + 2, 4 * 16 + 9, 26, 1)
    # a drain grate
    gx, gy = 8 * 16 + 4, 4 * 16 + 4
    s.rrect(gx, gy, gx + 8, gy + 7, O)
    for x in range(gx + 1, gx + 8, 2):
        for y in range(gy + 1, gy + 7):
            s.rset(x, y, C2)
    # cobwebs in the floor's corners
    for k, (cx, cy, dx) in enumerate(((0, H * 16 - 1, 1), (W * 16 - 1, 4 * 16, -1))):
        pass


def cinder(x, y):
    """cinder blocks: courses 8 px, blocks 16 px, staggered by 8; mortar lines lit"""
    course = y // 8
    off = 8 if course % 2 else 0
    if y % 8 == 7:
        return "mortar"
    if (x + off) % 16 == 15:
        return "mortar"
    return "block"


def draw_wall(s):
    for y in range(0, 32):
        for x in range(W * 16):
            lit = light_at(x, y + 40)
            c = cinder(x, y)
            if c == "mortar":
                col = C2 if lit < 0.4 else C3
            else:
                col = C1 if lit < 0.3 else C2 if lit < 0.6 else C3
                n = h(x % 16, y % 16, 5)
                if n < 20:
                    col = max(C0, col - 1)
                elif n > 240:
                    col = min(C4, col + 1)
            s.rset(x, y, col)
    # the ceiling's edge, dark; damp streaks down the wall
    for x in range(W * 16):
        s.rset(x, 0, O)
        s.rset(x, 1, O)
        s.rset(x, 2, C0)
        if h(x % 16, 0, 6) < 120:
            s.rset(x, 3, C0)
    for sx in (2 * 16 + 5, 7 * 16 + 11, 12 * 16 + 9):
        for y in range(3, 26):
            if h(sx, y, 7) < 200:
                s.rset(sx, y, GRIME if y % 5 else C1)
            if y > 10 and h(sx + 1, y, 7) < 90:
                s.rset(sx + 1, y, GRIME)
    # the baseboard: grime where wall meets floor
    for x in range(W * 16):
        for y in (29, 30, 31):
            if y == 31 or h(x % 16, y, 8) < (200 if y == 30 else 90):
                s.rset(x, y, C0 if y < 31 else O)
    # the pipe along the wall (y 8-12), copper gone to rust, flanges and brackets
    for x in range(W * 16):
        s.rset(x, 8, RUST_D)
        s.rset(x, 9, RUST)
        s.rset(x, 10, RUST)
        s.rset(x, 11, RUST_D)
        s.rset(x, 12, O)
        if h(x % 32, 1, 11) < 30:
            s.rset(x, 10, GRIME)
    for fx in range(10, W * 16, 32):
        s.rrect(fx, 7, fx + 2, 13, RUST_D)
        s.rset(fx + 1, 8, RUST)
    for bx in range(26, W * 16, 32):
        s.rrect(bx, 13, bx + 1, 15, C0)
    # the pipe's drop into the synthesizer's tank
    px0 = (SYNTH[0] + 3) * 16 + 6
    for y in range(12, 18):
        s.rrect(px0, y, px0 + 3, y, RUST if y % 4 else RUST_D)
        s.rset(px0, y, RUST_D)
        s.rset(px0 + 3, y, O)
    # cobwebs in the top corners
    def web(cx, cy, sgn):
        for r in range(1, 14):
            for a in range(0, r + 1):
                if (a * 3 + r) % 5 == 0 or r % 5 == 0 and a % 2 == 0:
                    s.rset(cx + sgn * a, cy + (r - a), WEB)
    web(0, 2, 1)
    web(W * 16 - 1, 2, -1)


def draw_lamp(s, bx):
    """a caged bulb on the wall: a bracket, the bulb, its cage; light on the wall around it"""
    cx, top = bx * 16 + 8, 5
    for y in range(top - 10, top + 12):        # a halo on the wall
        for x in range(cx - 12, cx + 13):
            d = ((x - cx) ** 2 + ((y - top - 3) * 1.4) ** 2) ** 0.5
            if d < 11 and s.r[x, y] in (C1, C2, C3) and (d < 7 or (x + y) % 2):
                s.rset(x, y, s.r[x, y] + 1)
    s.rrect(cx - 1, top - 3, cx, top - 1, C0)          # bracket
    for y in range(top, top + 7):                      # bulb
        w = (1, 2, 3, 3, 3, 2, 1)[y - top]
        for x in range(cx - w, cx + w):
            s.rset(x, y, LAMP)
    for y in (top + 1, top + 3, top + 5):              # cage wires
        s.rset(cx - 4, y, O)
        s.rset(cx + 3, y, O)
    for y in range(top, top + 7):
        s.rset(cx - 4 if y % 2 else cx + 3, y, C0)
    s.rrect(cx - 3, top + 7, cx + 2, top + 7, O)


def draw_board(s):
    """a chalkboard on the wall: a Poké Ball split open, arrows, a formula, scrawl"""
    x0, y0 = BOARD[0] * 16 + 1, BOARD[1] * 16 + 5
    x1, y1 = x0 + 29, y0 + 21
    s.rrect(x0, y0, x1, y1, WD_D)
    s.rrect(x0 + 2, y0 + 2, x1 - 2, y1 - 2, C0)
    s.rrect(x0, y1 + 1, x1, y1 + 1, O)
    s.rrect(x0 + 1, y1 - 1, x1 - 1, y1 - 1, WD)          # chalk ledge
    cx, cy, r = x0 + 9, y0 + 10, 5                       # a ball, its halves apart
    for a in range(0, 360, 12):
        import math
        x = cx + round(r * math.cos(math.radians(a)))
        y = cy + round(r * math.sin(math.radians(a))) + (1 if a < 180 else -1)
        s.rset(x, y, WEB)
    for x in range(cx - r, cx + r + 1):
        s.rset(x, cy - 1, WEB)
        s.rset(x, cy + 1, WEB)
    s.rset(cx, cy - 1, C0)
    for x in range(x0 + 16, x0 + 23):                    # an arrow
        s.rset(x, cy, WEB)
    s.rset(x0 + 22, cy - 1, WEB)
    s.rset(x0 + 22, cy + 1, WEB)
    for i, x in enumerate(range(x0 + 4, x1 - 3)):         # scrawled lines
        if h(x, 1, 21) < 170:
            s.rset(x, y0 + 4, WEB)
        if h(x, 2, 21) < 140 and x > x0 + 16:
            s.rset(x, y0 + 15, WEB)
        if h(x, 3, 21) < 120 and x < x0 + 20:
            s.rset(x, y0 + 17, WEB)
    s.rset(x0 + 25, y0 + 7, LAMP)                         # a question mark in yellow chalk
    s.rset(x0 + 26, y0 + 6, LAMP)
    s.rset(x0 + 27, y0 + 7, LAMP)
    s.rset(x0 + 26, y0 + 8, LAMP)
    s.rset(x0 + 26, y0 + 10, LAMP)


def draw_shelf(s):
    """a dusty bookshelf against the wall, 16x32: books, jars, a box"""
    x0, y0 = SHELF[0] * 16, SHELF[1] * 16 + 2
    s.rrect(x0, y0, x0 + 15, y0 + 29, WD_D)
    s.rrect(x0 + 1, y0 + 1, x0 + 14, y0 + 28, O)
    for sy in (y0 + 9, y0 + 18, y0 + 27):
        s.rrect(x0 + 1, sy, x0 + 14, sy + 1, WD)
    books = [C3, RUST_D, GRIME, WD_L, C4, RUST, PAPER, C2]
    for k, (ya, yb) in enumerate(((y0 + 2, y0 + 8), (y0 + 11, y0 + 17), (y0 + 20, y0 + 26))):
        x = x0 + 2
        while x < x0 + 14:
            w = 1 + h(x, k, 31) % 2
            top = ya + h(x, k, 32) % 3
            c = books[h(x, k, 33) % len(books)]
            if k == 1 and x0 + 8 <= x <= x0 + 11:         # a jar with something in it
                s.rrect(x0 + 8, ya + 2, x0 + 11, yb, WEB)
                s.rrect(x0 + 9, ya + 4, x0 + 10, yb - 1, GRIME)
                x = x0 + 12
                continue
            s.rrect(x, top, min(x + w - 1, x0 + 13), yb, c)
            x += w
    for x in range(x0, x0 + 16):                          # dust on top, a web
        s.rset(x, y0 - 1, O)
        if h(x, 9, 34) < 120:
            s.rset(x, y0, C4)
    s.rrect(x0, y0 + 30, x0 + 15, y0 + 30, O)
    for i in range(6):
        s.rset(x0 + 13 - i, y0 + 1 + i, WEB)
        s.rset(x0 + 13, y0 + 1 + i, WEB)


def draw_switch(s, state):
    """the wall switch (a knife switch on a panel): None (not there yet), 'off', 'on'"""
    if state is None:
        return
    x0, y0 = SWITCH[0] * 16 + 3, SWITCH[1] * 16 + 1
    s.rrect(x0, y0, x0 + 9, y0 + 12, O)
    s.rrect(x0 + 1, y0 + 1, x0 + 8, y0 + 11, C3)
    s.rrect(x0 + 1, y0 + 1, x0 + 8, y0 + 1, C4)
    for y in (y0 + 3, y0 + 9):                            # contacts
        s.rset(x0 + 3, y, RUST)
        s.rset(x0 + 6, y, RUST)
    # conduit up into the pipe
    s.rrect(x0 + 4, y0 - 4, x0 + 5, y0 - 1, C0)
    if state == "off":                                    # blade up
        s.rrect(x0 + 3, y0 + 2, x0 + 3, y0 + 6, LAMP)
        s.rrect(x0 + 6, y0 + 2, x0 + 6, y0 + 6, LAMP)
        s.rrect(x0 + 2, y0 + 1, x0 + 7, y0 + 2, WD_D)
    else:                                                 # thrown down; a spark
        s.rrect(x0 + 3, y0 + 6, x0 + 3, y0 + 10, LAMP)
        s.rrect(x0 + 6, y0 + 6, x0 + 6, y0 + 10, LAMP)
        s.rrect(x0 + 2, y0 + 10, x0 + 7, y0 + 11, WD_D)
        s.rset(x0 + 8, y0 + 5, LAMP)
        s.rset(x0 + 9, y0 + 4, PAPER)


def draw_bench(s):
    """the workbench: a heavy top with a vice, tools, a blueprint; shelf underneath"""
    x0, y0 = BENCH[0] * 16, BENCH[1] * 16
    s.rrect(x0, y0 + 4, x0 + 31, y0 + 14, WD)             # top
    s.rrect(x0, y0 + 4, x0 + 31, y0 + 5, WD_L)
    s.rrect(x0, y0 + 15, x0 + 31, y0 + 17, WD_D)           # front edge
    s.rrect(x0, y0 + 18, x0 + 31, y0 + 18, O)
    for lx in (x0 + 1, x0 + 28):                          # legs
        s.rrect(lx, y0 + 18, lx + 2, y0 + 29, WD_D)
        s.rset(lx, y0 + 30, O)
    s.rrect(x0 + 4, y0 + 24, x0 + 27, y0 + 25, WD_D)       # the shelf under, a box on it
    s.rrect(x0 + 8, y0 + 20, x0 + 15, y0 + 23, C3)
    s.rrect(x0 + 8, y0 + 20, x0 + 15, y0 + 20, C4)
    s.rrect(x0 + 18, y0 + 21, x0 + 24, y0 + 23, RUST_D)
    s.rrect(x0 + 4, y0 + 6, x0 + 16, y0 + 12, PAPER)       # a blueprint, its lines
    for x in range(x0 + 5, x0 + 16):
        if x % 3:
            s.rset(x, y0 + 8, C2)
        s.rset(x, y0 + 11, C2 if h(x, 4, 41) < 128 else PAPER)
    s.rrect(x0 + 9, y0 + 9, x0 + 11, y0 + 10, C2)
    s.rrect(x0 + 21, y0 + 2, x0 + 27, y0 + 9, C0)          # the vice
    s.rrect(x0 + 22, y0 + 3, x0 + 26, y0 + 5, C3)
    s.rrect(x0 + 23, y0 + 10, x0 + 25, y0 + 11, C1)
    for i in range(6):                                    # a wrench
        s.rset(x0 + 18 + i, y0 + 13 - (i // 2), C4)
    s.rrect(x0 + 0, y0 + 4, x0 + 0, y0 + 17, O)
    s.rrect(x0 + 31, y0 + 4, x0 + 31, y0 + 17, O)
    s.rrect(x0, y0 + 3, x0 + 31, y0 + 3, O)


def draw_labtable(s):
    """a lab table: flasks with green and amber liquids, a burner, tubing"""
    x0, y0 = LABTABLE[0] * 16, LABTABLE[1] * 16
    s.rrect(x0, y0 + 12, x0 + 31, y0 + 17, C4)            # steel top
    s.rrect(x0, y0 + 12, x0 + 31, y0 + 12, C4 + 1)
    s.rrect(x0, y0 + 18, x0 + 31, y0 + 19, C2)
    s.rrect(x0, y0 + 20, x0 + 31, y0 + 20, O)
    for lx in (x0 + 2, x0 + 28):
        s.rrect(lx, y0 + 20, lx + 1, y0 + 30, C2)
        s.rset(lx, y0 + 31, O)
    s.rrect(x0 + 6, y0 + 25, x0 + 25, y0 + 25, C2)        # crossbar
    def flask(cx, base, hgt, liquid):
        for y in range(base - hgt, base + 1):
            k = y - (base - hgt)
            w = 1 if k < hgt // 2 else 1 + (k - hgt // 2)
            w = min(w, 4)
            for x in range(cx - w, cx + w + 1):
                s.rset(x, y, WEB if (x in (cx - w, cx + w) or k == 0) else (liquid if k > hgt * 0.55 else C1))
        s.rrect(cx - 4, base + 1, cx + 4, base + 1, O)
    flask(x0 + 6, y0 + 12, 10, GRIME)
    flask(x0 + 16, y0 + 12, 8, LAMP)
    flask(x0 + 25, y0 + 12, 11, RUST)
    for i in range(7):                                    # tubing between two flasks
        s.rset(x0 + 8 + i, y0 + 3 + (i % 3 == 0), C4)


def draw_desk(s):
    """the desk: a lamp (lit, a little), a typewriter, stacked notes; a chair pushed back"""
    x0, y0 = DESK[0] * 16, DESK[1] * 16
    s.rrect(x0, y0 + 8, x0 + 31, y0 + 17, WD_D)
    s.rrect(x0, y0 + 8, x0 + 31, y0 + 9, WD)
    s.rrect(x0, y0 + 18, x0 + 31, y0 + 18, O)
    s.rrect(x0 + 1, y0 + 18, x0 + 3, y0 + 29, WD_D)
    s.rrect(x0 + 28, y0 + 18, x0 + 30, y0 + 29, WD_D)
    s.rrect(x0 + 14, y0 + 19, x0 + 26, y0 + 24, WD)       # a drawer
    s.rrect(x0 + 19, y0 + 21, x0 + 21, y0 + 21, C4)
    s.rrect(x0 + 3, y0 + 2, x0 + 9, y0 + 9, C0)            # the typewriter
    s.rrect(x0 + 4, y0 + 3, x0 + 8, y0 + 4, C2)
    s.rrect(x0 + 5, y0 + 0, x0 + 7, y0 + 2, PAPER)
    for k in range(4):                                    # notes, stacked crooked
        s.rrect(x0 + 13 + k, y0 + 3 + k, x0 + 20 + k, y0 + 8 + k // 2, PAPER if k % 2 else WEB)
    s.rrect(x0 + 26, y0 + 0, x0 + 26, y0 + 7, C3)          # the lamp
    s.rrect(x0 + 23, y0 - 2, x0 + 29, y0 + 0, C2)
    s.rrect(x0 + 24, y0 + 1, x0 + 28, y0 + 2, LAMP)
    for x in range(x0 + 20, x0 + 32):                     # its pool of light on the desk
        if (x + y0) % 2:
            s.rset(x, y0 + 10, LAMP)


def draw_stairs(s):
    """wooden stairs up the right wall into the dark of the house above: the steps rise to the
    upper right; a stringer and a railing on their open side; the warp block at their foot"""
    x0, y0 = (STAIRS[0] + 1) * 16, 0
    x1, y1 = x0 + 31, (STAIRS[1] + 2) * 16 - 1          # (32 wide, rows 0..y+1)
    s.rrect(x0, y0, x1, y1, O)                           # the stairwell: dark
    n = 9                                                # steps, bottom (left) to top (right)
    for k in range(n):
        sx = x0 + 2 + k * 3
        top = y1 - 4 - k * 5
        s.rrect(sx, top, x1, top + 1, WD_L)              # the tread's lit nose
        s.rrect(sx, top + 2, x1, top + 4, WD)            # the tread
        s.rrect(sx, top + 5, x1, min(top + 5, y1), WD_D)  # the riser's shadow
    for y in range(y0, y1 + 1):                          # fading into the dark at the top
        for x in range(x0, x1 + 1):
            if y < 10 and s.r[x, y] != O and (y < 5 or (x + y) % 2):
                s.rset(x, y, O if y < 5 else C0)
    for k in range(n + 1):                               # the stringer along their open side
        bx_, by_ = x0 + k * 3, y1 - k * 5
        s.rrect(bx_, by_ - 2, bx_ + 3, by_, WD_D)
    for y in range(4, y1 - 4, 1):                        # a railing above it, its posts
        x = x0 + 1 + (y1 - y) * 3 // 5
        if y > 8:
            s.rset(x, y - 8, WD_L)
            s.rset(x, y - 7, WD)
    for k in range(0, n, 3):
        px_, py_ = x0 + 1 + k * 3, y1 - 4 - k * 5
        s.rrect(px_, py_ - 8, px_, py_ - 1, WD)
    # the foot of the stairs: a worn patch of floor
    fx, fy = STAIRS[0] * 16, (STAIRS[1] + 1) * 16
    for y in range(fy + 3, fy + 13):
        for x in range(fx + 3, fx + 13):
            if h(x % 16, y % 16, 81) < 110:
                s.rset(x, y, C3 if s.r[x, y] < C3 else s.r[x, y])


def draw_trash(s):
    x0, y0 = TRASH[0] * 16 + 2, TRASH[1] * 16 + 1
    for y in range(y0 + 3, y0 + 14):
        for x in range(x0, x0 + 12):
            s.rset(x, y, C3 if (x - x0) % 3 else C2)
    s.rrect(x0, y0 + 14, x0 + 11, y0 + 14, O)
    s.rrect(x0 - 1, y0 + 2, x0 + 12, y0 + 3, C4)           # rim
    s.rrect(x0 + 2, y0, x0 + 5, y0 + 2, PAPER)             # crumpled paper sticking out
    s.rrect(x0 + 7, y0 + 1, x0 + 9, y0 + 2, WEB)
    s.rset(x0 + 3, y0 + 1, C4)
    # its lid, on the floor beside it
    s.rrect(x0 + 13, y0 + 10, x0 + 13, y0 + 14, C3)


def draw_crate(s, bx, by, top=True):
    x0, y0 = bx * 16 + 1, by * 16 + 1
    s.rrect(x0, y0, x0 + 13, y0 + 13, WD)
    for y in (y0, y0 + 13):
        s.rrect(x0, y, x0 + 13, y, WD_D)
    for x in (x0, x0 + 13):
        s.rrect(x, y0, x, y0 + 13, WD_D)
    for i in range(14):                                   # the brace
        s.rset(x0 + i, y0 + i, WD_D)
        s.rset(x0 + i, y0 + 13 - i if not top else y0 + i, WD_D)
    for x in range(x0 + 1, x0 + 13):
        if h(x, by, 51) < 90:
            s.rset(x, y0 + 4, WD_L)
    s.rrect(x0, y0 + 14, x0 + 13, y0 + 14, O)


def draw_paper(s, bx, by, k):
    """a loose page on the floor, each a little different"""
    x0, y0 = bx * 16 + 3 + k % 2, by * 16 + 4 + (k * 3) % 4
    w, hh = 9, 7
    for y in range(hh):
        sh = (y * (1 if k % 2 else -1)) // 3
        for x in range(w):
            s.rset(x0 + x + sh, y0 + y, PAPER)
    for y in (1, 3, 5):
        for x in range(1, w - 2):
            if h(x, y, 60 + k) < 170:
                sh = (y * (1 if k % 2 else -1)) // 3
                s.rset(x0 + x + sh, y0 + y, C3)
    s.rset(x0 + w - 1, y0, C4)                           # a dog-eared corner
    for x in range(w):
        s.rset(x0 + x + ((hh - 1) * (1 if k % 2 else -1)) // 3, y0 + hh, C0)


# ---------------------------------------------------------------------------------------------
# The machines
# ---------------------------------------------------------------------------------------------
def draw_synth(s, frame):
    """the Ability Synthesizer. A belt (x+0..1) carries a capsule into the stream; the tank above
    pours a glowing liquid like a waterfall into the vat. frame 0-2 (belt treads and capsule move,
    the stream falls, the vat ripples)"""
    bx, by = SYNTH
    X, Y = bx * 16, by * 16
    # --- the tank, on the wall (x+2..3, row y): glass, the liquid, bands and gauges
    tx0, tx1 = X + 33, X + 62
    s.mrect(tx0, Y + 1, tx1, Y + 15, M_O)
    s.mrect(tx0 + 1, Y + 2, tx1 - 1, Y + 14, GL)
    for y in range(Y + 5, Y + 15):
        for x in range(tx0 + 2, tx1 - 1):
            s.mset(x, y, LQ_D if y > Y + 9 else LQ)
    for x in range(tx0 + 2, tx1 - 1):                     # the surface, bubbling
        if (x + frame * 3) % 7 == 0:
            s.mset(x, Y + 4, LQ_G)
        s.mset(x, Y + 5, LQ_G if (x + frame) % 3 else LQ)
    for k in range(3):
        bxp = tx0 + 6 + k * 8 + frame * 2 % 5
        s.mset(bxp, Y + 11 - (frame + k) % 4, LQ_G)
    for y in range(Y + 2, Y + 15):                        # glass shine
        s.mset(tx0 + 3, y, S3)
    for x in range(tx0, tx1 + 1):                         # brass bands
        s.mset(x, Y + 1, BR)
        s.mset(x, Y + 15, BR_D)
    s.mrect(tx1 - 7, Y + 7, tx1 - 3, Y + 11, S1)           # a gauge
    s.mrect(tx1 - 6, Y + 8, tx1 - 4, Y + 10, WHITE)
    s.mset(tx1 - 5 + (frame % 2), Y + 9, RED)
    # --- the nozzle and the stream (row y+1)
    ny = Y + 16
    s.mrect(X + 40, ny, X + 55, ny + 3, S2)               # the head
    s.mrect(X + 40, ny, X + 55, ny, S3)
    s.mrect(X + 40, ny + 4, X + 55, ny + 4, M_O)
    s.mrect(X + 44, ny + 5, X + 51, ny + 6, S1)           # the spout
    for y in range(ny + 7, ny + 16):                      # the falling sheet of liquid
        for x in range(X + 44, X + 52):
            edge = x in (X + 44, X + 51)
            streak = (y - frame * 3 + (x - X) * 5) % 7 == 0
            s.mset(x, y, LQ_D if edge else LQ_G if streak else LQ)
        if (y + frame) % 4 == 0:                          # droplets off the sides
            s.mset(X + 43, y, LQ)
            s.mset(X + 52, y + 1, LQ_G)
    # side struts down to the vat
    for y in range(ny, ny + 16):
        s.mset(X + 34, y, S2)
        s.mset(X + 35, y, S1)
        s.mset(X + 60, y, S2)
        s.mset(X + 61, y, S1)
    # --- the vat (row y+2): a round tub of glowing liquid on steel feet
    vy = Y + 32
    s.mrect(X + 33, vy, X + 62, vy + 11, S2)
    s.mrect(X + 33, vy, X + 62, vy + 1, S3)
    s.mrect(X + 35, vy + 2, X + 60, vy + 5, LQ)            # the surface
    for x in range(X + 35, X + 61):
        if (x + frame * 2) % 5 == 0:
            s.mset(x, vy + 3, LQ_G)
        if (x + frame * 2 + 2) % 7 == 0:
            s.mset(x, vy + 4, LQ_G)
    s.mrect(X + 44, vy, X + 51, vy + 2, LQ)                # where the stream lands, splashing
    s.mset(X + 43 - frame % 2, vy - 1, LQ_G)
    s.mset(X + 53 + frame % 2, vy - 1, LQ_G)
    s.mrect(X + 33, vy + 6, X + 62, vy + 6, BR)
    s.mrect(X + 33, vy + 12, X + 62, vy + 12, M_O)
    for fx in (X + 35, X + 58):
        s.mrect(fx, vy + 12, fx + 2, vy + 14, S1)
    s.mrect(X + 54, vy + 8, X + 56, vy + 10, RED if frame != 1 else AMBER)   # a light
    for x in range(X + 33, X + 63):                       # the glow on the floor in front
        if (x + frame) % 3 == 0:
            s.mset(x, vy + 15, LQ_D)
    # --- the belt (x+0..1, rows y+1..2): its surface, rollers, frame and legs
    belt_y = Y + 24
    s.mrect(X + 2, belt_y, X + 33, belt_y + 5, RUB)
    for x in range(X + 2, X + 34):                        # treads moving right
        if (x - frame * 3) % 6 == 0:
            s.mrect(x, belt_y + 1, x, belt_y + 4, S1)
    s.mrect(X + 2, belt_y - 1, X + 33, belt_y - 1, S3)
    s.mrect(X + 1, belt_y + 6, X + 33, belt_y + 8, S2)     # the side rail
    s.mrect(X + 1, belt_y + 6, X + 33, belt_y + 6, S3)
    for rx in range(X + 4, X + 34, 7):                     # rollers
        s.mset(rx, belt_y + 7, S0 if (rx + frame) % 2 else S3)
    s.mrect(X + 1, belt_y + 9, X + 33, belt_y + 9, M_O)
    for lx in (X + 3, X + 28):
        s.mrect(lx, belt_y + 10, lx + 2, belt_y + 22, S2)
        s.mrect(lx, belt_y + 23, lx + 2, belt_y + 23, M_O)
    s.mrect(X + 1, belt_y - 1, X + 1, belt_y + 9, M_O)
    # a capsule riding the belt
    cx = X + 6 + frame * 8
    s.mrect(cx, belt_y - 3, cx + 5, belt_y + 1, M_O)
    s.mrect(cx + 1, belt_y - 2, cx + 2, belt_y, RED)
    s.mrect(cx + 3, belt_y - 2, cx + 4, belt_y, WHITE)
    # the control box at the belt's start, its lamp
    s.mrect(X + 2, Y + 18, X + 10, Y + 23, S1)
    s.mrect(X + 3, Y + 19, X + 9, Y + 22, S0)
    s.mset(X + 5, Y + 20, LQ_G if frame % 2 else LQ_D)
    s.mset(X + 7, Y + 20, AMBER)


def draw_maker(s, frame, ball=False):
    """the Ball Maker: a big hopper, the body with a slot for a Poké Ball, a hand crank on its
    right. frame 0-3: the crank's handle goes round; ball: a new ball sits in the slot"""
    bx, by = MAKER
    X, Y = bx * 16, by * 16
    # the hopper: wide at the top (row y, on the wall), narrowing into the body
    for y in range(Y + 1, Y + 26):
        k = y - (Y + 1)
        half = 20 - (k * 13) // 25
        cx = X + 24
        for x in range(cx - half, cx + half):
            c = S2 if x < cx - half + 3 else S3 if x < cx + half - 4 else S1
            s.mset(x, y, c)
        s.mset(cx - half - 1, y, M_O)
        s.mset(cx + half, y, M_O)
    s.mrect(X + 3, Y + 1, X + 44, Y + 1, M_O)
    s.mrect(X + 4, Y + 2, X + 43, Y + 3, S0)               # the open top: dark
    for x in range(X + 6, X + 42):                         # whatever's in it: red and white bits
        if h(x, 1, 70) < 60:
            s.mset(x, Y + 3, RED if h(x, 2, 70) < 128 else WHITE)
    for y in (Y + 8, Y + 16):                              # rivets / bands
        for x in range(X + 10, X + 40, 4):
            if s.m[x, y]:
                s.mset(x, y, S3 + 1)
    s.mset(X + 24 - 1 + frame % 2, Y + 20, S3 + 1)           # (a shiver while cranking)
    # the body (rows y+1..2): a steel box, its slot, a dial
    by0 = Y + 24
    s.mrect(X + 8, by0, X + 40, by0 + 21, S2)
    s.mrect(X + 9, by0 + 1, X + 39, by0 + 2, S3)
    s.mrect(X + 8, by0 + 22, X + 40, by0 + 22, M_O)
    s.mrect(X + 7, by0, X + 7, by0 + 22, M_O)
    s.mrect(X + 41, by0, X + 41, by0 + 22, M_O)
    s.mrect(X + 10, by0 + 22, X + 13, by0 + 23, S1)        # feet
    s.mrect(X + 35, by0 + 22, X + 38, by0 + 23, S1)
    sx, sy = X + 20, by0 + 8                               # the slot: a round cup
    s.mrect(sx, sy, sx + 9, sy + 9, M_O)
    s.mrect(sx + 1, sy + 1, sx + 8, sy + 8, S0)
    if ball:                                               # a new ball in it
        s.mrect(sx + 2, sy + 2, sx + 7, sy + 4, RED)
        s.mrect(sx + 2, sy + 5, sx + 7, sy + 7, WHITE)
        s.mrect(sx + 2, sy + 5, sx + 7, sy + 5, M_O)
        s.mset(sx + 4, sy + 5, WHITE)
        s.mset(sx + 5, sy + 5, WHITE)
    s.mrect(X + 12, by0 + 6, X + 16, by0 + 10, BR)          # a dial
    s.mset(X + 14, by0 + 8, M_O)
    s.mset(X + 13 + frame % 3, by0 + 7, M_O)
    s.mrect(X + 33, by0 + 15, X + 37, by0 + 16, RED if frame else S1)   # a light
    # the crank (x+2, rows y+1..2): a wheel on an axle, the handle at one of four places
    cx, cy = X + 42 + 3, by0 + 9
    s.mrect(X + 41, cy - 1, cx, cy + 1, S1)                # axle
    import math
    for a in range(0, 360, 20):
        x = cx + round(5 * math.cos(math.radians(a)))
        y = cy + round(5 * math.sin(math.radians(a)))
        s.mset(x, y, BR)
    s.mset(cx, cy, BR_D)
    ang = [0, 90, 180, 270][frame] - 90
    hx = cx + round(5 * math.cos(math.radians(ang)))
    hy = cy + round(5 * math.sin(math.radians(ang)))
    s.mrect(hx - 1, hy - 1, hx + 1, hy + 1, M_O)           # the handle's knob
    s.mset(hx, hy, RED)
    for t in range(6):                                     # the spoke
        x = cx + round(t / 5 * (hx - cx))
        y = cy + round(t / 5 * (hy - cy))
        s.mset(x, y, BR_D)


def draw_changer(s, frame):
    """the Ball Changer: two pedestals (the balls are people - object events - set on them), the
    console between with its lever and two lights. frame 0: idle; 1: lever pulled, lights on"""
    bx, by = CHANGER
    for px_ in (bx, bx + 3):
        X, Y = px_ * 16, by * 16
        # a dish on top (row y) where a ball sits, a column (rows y..y+1)
        s.mrect(X + 2, Y + 8, X + 13, Y + 11, S3)
        s.mrect(X + 2, Y + 8, X + 13, Y + 8, S3 + 1)
        s.mrect(X + 1, Y + 9, X + 1, Y + 11, M_O)
        s.mrect(X + 14, Y + 9, X + 14, Y + 11, M_O)
        s.mrect(X + 3, Y + 12, X + 12, Y + 12, LQ if frame else S0)     # a ring that lights up
        s.mrect(X + 4, Y + 13, X + 11, Y + 26, S2)
        s.mrect(X + 4, Y + 13, X + 5, Y + 26, S3)
        s.mrect(X + 3, Y + 13, X + 3, Y + 26, M_O)
        s.mrect(X + 12, Y + 13, X + 12, Y + 26, M_O)
        s.mrect(X + 2, Y + 27, X + 13, Y + 29, S1)
        s.mrect(X + 2, Y + 30, X + 13, Y + 30, M_O)
        for y in range(Y + 15, Y + 26, 4):                 # cables up the column
            s.mset(X + 9, y, BR if frame else BR_D)
    # the console (x+1..2, row y+1): a sloped panel, a screen, the lever
    X, Y = (bx + 1) * 16, (by + 1) * 16
    s.mrect(X + 1, Y + 2, X + 30, Y + 13, S2)
    s.mrect(X + 1, Y + 2, X + 30, Y + 3, S3)
    s.mrect(X + 1, Y + 14, X + 30, Y + 14, M_O)
    s.mrect(X + 0, Y + 2, X + 0, Y + 14, M_O)
    s.mrect(X + 31, Y + 2, X + 31, Y + 14, M_O)
    s.mrect(X + 4, Y + 5, X + 16, Y + 11, S0)              # a screen: two dots and an arrow
    s.mset(X + 6, Y + 8, RED)
    s.mset(X + 14, Y + 8, RED)
    for x in range(X + 8, X + 13):
        s.mset(x, Y + 8, LQ_G if frame else LQ_D)
    s.mset(X + 11 if frame else X + 9, Y + 7, LQ_G if frame else LQ_D)
    s.mrect(X + 20, Y + 9, X + 26, Y + 12, S1)              # the lever's housing
    if frame:
        for t in range(6):
            s.mset(X + 23 + t // 2, Y + 9 + t // 3, S3 + 1)
        s.mrect(X + 25, Y + 11, X + 27, Y + 13, RED)
    else:
        for t in range(6):
            s.mset(X + 23, Y + 9 - t, S3 + 1)
        s.mrect(X + 22, Y + 2, X + 24, Y + 4, RED)
    s.mset(X + 18, Y + 5, AMBER if frame else S1)
    s.mset(X + 28, Y + 5, LQ_G if frame else S1)
    # cables along the floor to the pedestals
    for x in list(range(X - 4, X + 1)) + list(range(X + 31, X + 36)):
        s.mset(x, Y + 13, RUB)


# ---------------------------------------------------------------------------------------------
def scene(synth=0, maker=0, changer=0, switch=None, ball=False):
    s = Scene()
    draw_wall(s)
    draw_floor(s)
    for lx in LAMPS:
        draw_lamp(s, lx)
    draw_board(s)
    draw_shelf(s)
    draw_switch(s, switch)
    draw_stairs(s)
    draw_bench(s)
    draw_labtable(s)
    draw_desk(s)
    draw_trash(s)
    for i, (x, y) in enumerate(CRATES):
        draw_crate(s, x, y, top=i != 2)
    for k, (x, y) in enumerate(PAPERS):
        draw_paper(s, x, y, k)
    draw_synth(s, synth)
    draw_maker(s, maker, ball)
    draw_changer(s, changer)
    return s


def to_rgb(s):
    out = Image.new("RGB", s.room.size)
    o = out.load()
    for y in range(s.room.height):
        for x in range(s.room.width):
            m = s.m[x, y]
            o[x, y] = MACH[m] if m else ROOM[s.r[x, y]]
    return out


if __name__ == "__main__":
    import sys
    sc = scene()
    to_rgb(sc).resize((W * 48, H * 48), Image.NEAREST).save(sys.argv[1])
