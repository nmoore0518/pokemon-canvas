#!/usr/bin/env python3
"""Pixel art for the type decorations and practice trainers (drawn in code).

Each piece is drawn on a Canvas (16 pixels per block) with the named colors below. The colors
come from three shared palettes (gym tileset slots 8-10, see build_gym_decor.py); every 8x8 tile
uses one of them, so a tile should stick to one palette's colors (others get the nearest color).

Run it to get a picture of every piece:  python3 tools/gym_hack/decor_art.py out.png
"""
import math

# ---------------------------------------------------------------------------
# Palettes. Index 0 is transparent (the gym floor shows through).
# ---------------------------------------------------------------------------
SHARED = [("K", (40, 40, 48)), ("W", (248, 248, 248)), ("L", (184, 184, 200)), ("G", (120, 120, 136))]
PALETTES = {
    8: SHARED + [("DR", (136, 24, 32)), ("R", (216, 48, 40)), ("SA", (248, 136, 112)), ("DO", (200, 96, 24)),
                 ("O", (248, 152, 40)), ("Y", (248, 216, 64)), ("PY", (255, 244, 176)), ("DB", (88, 56, 32)),
                 ("B", (144, 96, 56)), ("T", (208, 160, 104)), ("BE", (240, 212, 168))],
    9: SHARED + [("NV", (24, 40, 104)), ("BL", (48, 96, 200)), ("SK", (104, 168, 240)), ("IC", (192, 228, 248)),
                 ("TE", (32, 128, 128)), ("AQ", (96, 208, 200)), ("DG", (24, 96, 40)), ("GR", (64, 160, 64)),
                 ("LG", (152, 216, 96)), ("DK", (72, 72, 88)), ("ST", (152, 168, 188))],
    10: SHARED + [("DP", (56, 24, 88)), ("P", (112, 56, 168)), ("LV", (176, 136, 224)), ("MG", (200, 48, 152)),
                  ("PK", (248, 136, 192)), ("PP", (252, 208, 228)), ("DGO", (160, 112, 24)), ("GO", (232, 184, 48)),
                  ("BK", (16, 16, 24)), ("DK", (72, 72, 88)), ("R", (216, 48, 40))],
}
RGB = {}
for _slot, _cols in PALETTES.items():
    for _n, _c in _cols:
        RGB.setdefault(_n, _c)


class Canvas:
    def __init__(self, w, h):
        self.cw, self.ch = w, h
        self.w, self.h = w * 16, h * 16
        self.px = [[None] * self.w for _ in range(self.h)]

    def set(self, x, y, c):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def get(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.px[y][x]
        return None

    def rect(self, x0, y0, x1, y1, c, edge=None):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                e = edge and (x in (x0, x1) or y in (y0, y1))
                self.set(x, y, edge if e else c)

    def ellipse(self, cx, cy, rx, ry, c, edge=None):
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                d = ((x + 0.5 - cx) / max(rx, 0.1)) ** 2 + ((y + 0.5 - cy) / max(ry, 0.1)) ** 2
                if d <= 1:
                    inner = ((x + 0.5 - cx) / max(rx - 1, 0.1)) ** 2 + ((y + 0.5 - cy) / max(ry - 1, 0.1)) ** 2
                    self.set(x, y, edge if (edge and inner > 1) else c)

    def line(self, x0, y0, x1, y1, c, width=1):
        n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
        for i in range(n):
            t = i / max(n - 1, 1)
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            for k in range(width):
                self.set(x + (k if abs(y1 - y0) > abs(x1 - x0) else 0), y + (0 if abs(y1 - y0) > abs(x1 - x0) else k), c)

    def poly(self, pts, c):
        ys = [p[1] for p in pts]
        for y in range(int(min(ys)), int(max(ys)) + 1):
            xs = []
            yc = y + 0.5
            for (xa, ya), (xb, yb) in zip(pts, pts[1:] + pts[:1]):
                if (ya <= yc < yb) or (yb <= yc < ya):
                    xs.append(xa + (yc - ya) * (xb - xa) / (yb - ya))
            xs.sort()
            for a, b in zip(xs[::2], xs[1::2]):
                for x in range(int(math.ceil(a - 0.5)), int(math.floor(b - 0.5)) + 1):
                    self.set(x, y, c)

    def outline(self, c="K"):
        """Outlines everything drawn so far (outside edge)."""
        add = []
        for y in range(self.h):
            for x in range(self.w):
                if self.px[y][x] is None and any(self.get(x + dx, y + dy) not in (None, c)
                                                 for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                    add.append((x, y))
        for x, y in add:
            self.px[y][x] = c

    def shade_box(self, x0, y0, x1, y1, light, mid, dark, edge="K"):
        """A box lit from the top left."""
        self.rect(x0, y0, x1, y1, mid)
        self.rect(x0, y0, x1, y0, light)
        self.rect(x0, y0, x0, y1, light)
        self.rect(x1, y0, x1, y1, dark)
        self.rect(x0, y1, x1, y1, dark)
        if edge:
            self.rect(x0 - 1, y0 - 1, x1 + 1, y1 + 1, None, edge)
            self.rect(x0, y0, x1, y1, None)
            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    pass

    def frame(self, x0, y0, x1, y1, c):
        for x in range(x0, x1 + 1):
            self.set(x, y0, c)
            self.set(x, y1, c)
        for y in range(y0, y1 + 1):
            self.set(x0, y, c)
            self.set(x1, y, c)

    def fill_box(self, x0, y0, x1, y1, fill, light=None, dark=None, edge="K"):
        """Filled box with an outline, optional highlight (top/left) and shadow (bottom/right)."""
        self.rect(x0, y0, x1, y1, fill)
        if light:
            self.rect(x0 + 1, y0 + 1, x1 - 1, y0 + 1, light)
            self.rect(x0 + 1, y0 + 1, x0 + 1, y1 - 1, light)
        if dark:
            self.rect(x0 + 1, y1 - 1, x1 - 1, y1 - 1, dark)
            self.rect(x1 - 1, y0 + 1, x1 - 1, y1 - 1, dark)
        if edge:
            self.frame(x0, y0, x1, y1, edge)

    def mirror(self):
        """Copies the left half onto the right half (for symmetric pieces)."""
        for y in range(self.h):
            for x in range(self.w // 2):
                self.px[y][self.w - 1 - x] = self.px[y][x]


def C(w, h):
    return Canvas(w, h)


# ===========================================================================
# The pieces. Each returns a Canvas. Sizes are in 16x16 blocks (width, height).
# ===========================================================================

# ---- NORMAL ---------------------------------------------------------------
def couch():
    c = C(3, 2)
    c.fill_box(2, 4, 45, 18, "B", "T", "DB")            # back
    c.fill_box(0, 12, 8, 28, "B", "T", "DB")            # arms
    c.fill_box(39, 12, 47, 28, "B", "T", "DB")
    c.fill_box(8, 17, 39, 26, "T", "BE", "B")           # seat
    c.line(23, 17, 23, 25, "B")
    c.rect(4, 29, 6, 31, "DB")
    c.rect(41, 29, 43, 31, "DB")
    c.fill_box(10, 9, 20, 16, "SA", None, "R")          # cushions
    c.fill_box(27, 9, 37, 16, "SA", None, "R")
    return c


def grandfather_clock():
    c = C(1, 2)
    c.fill_box(2, 1, 13, 30, "B", "T", "DB")
    c.ellipse(8, 8, 5, 5, "PY", "K")
    c.line(8, 8, 8, 5, "K")
    c.line(8, 8, 10, 9, "K")
    c.rect(5, 15, 10, 27, "DB")
    c.line(8, 15, 8, 22, "Y")
    c.ellipse(8, 23.5, 2, 2, "Y")
    c.rect(1, 0, 14, 1, "DB")
    return c


def vending_machine():
    c = C(2, 2)
    c.fill_box(2, 1, 29, 30, "R", "SA", "DR")
    c.rect(5, 4, 19, 21, "IC")
    c.frame(5, 4, 19, 21, "K")
    for row, cols in enumerate((("BL", "R", "Y"), ("GR", "O", "BL"), ("Y", "SK", "R"))):
        for k, col in enumerate(cols):
            c.rect(7 + k * 4, 6 + row * 5, 9 + k * 4, 9 + row * 5, col)
    c.rect(22, 6, 26, 9, "K")
    c.rect(23, 12, 25, 18, "L")
    c.rect(5, 24, 19, 27, "K")
    return c


def drink_machine():
    """(Pokémon Canvas: the Drink Machine from the Celadon roof, src/drink_machine.c) a teal machine:
    three bottles in its window, a funnel on top, a cup under the spout"""
    c = C(2, 2)
    c.fill_box(2, 3, 29, 30, "TE", "AQ", "DG")
    c.rect(5, 6, 20, 20, "IC")
    c.frame(5, 6, 20, 20, "K")
    for k, (col, top) in enumerate((("BL", 9), ("GR", 10), ("SK", 9))):
        x = 7 + k * 5
        c.rect(x + 1, top - 1, x + 1, top, "L")
        c.rect(x, top + 1, x + 2, 17, col)
        c.rect(x, 17, x + 2, 18, "K")
    c.poly([(22, 1), (28, 1), (26, 6), (24, 6)], "ST")
    c.rect(24, 6, 26, 12, "ST")
    c.frame(23, 13, 27, 15, "K")
    c.rect(8, 23, 17, 27, "K")
    c.rect(11, 24, 14, 27, "W")
    c.rect(11, 25, 14, 27, "LG")
    return c


def picnic_blanket():
    c = C(2, 2)
    for y in range(2, 30):
        for x in range(2, 30):
            c.set(x, y, "R" if ((x - 2) // 4 + (y - 2) // 4) % 2 == 0 else "W")
    c.frame(1, 1, 30, 30, "DR")
    c.fill_box(18, 17, 27, 26, "B", "T", "DB")          # basket
    c.line(19, 17, 22, 13, "DB")
    c.line(26, 17, 23, 13, "DB")
    return c


def arcade_cabinet():
    c = C(1, 2)
    c.fill_box(1, 1, 14, 30, "P", "LV", "DP")
    c.rect(3, 4, 12, 12, "K")
    c.rect(4, 5, 11, 11, "NV")
    c.set(6, 7, "Y"); c.set(9, 9, "R"); c.set(8, 6, "GR")
    c.rect(2, 14, 13, 18, "DK")
    c.ellipse(5, 16, 1.5, 1.5, "R")
    c.ellipse(10, 16, 1.5, 1.5, "BL")
    c.rect(5, 22, 10, 23, "K")
    return c


# ---- FIGHTING ---------------------------------------------------------------
def punching_bag():
    c = C(1, 2)
    c.line(8, 0, 8, 4, "G")
    c.rect(6, 4, 9, 5, "DK")
    c.ellipse(8, 17, 6, 12, "R", "K")
    c.rect(4, 8, 5, 24, "SA")
    c.rect(3, 12, 12, 13, "DR")
    c.rect(3, 22, 12, 23, "DR")
    return c


def barbell_rack():
    c = C(2, 1)
    c.rect(4, 8, 5, 15, "DK")
    c.rect(26, 8, 27, 15, "DK")
    c.rect(1, 5, 30, 6, "L")
    c.frame(1, 4, 30, 7, "K")
    for x in (3, 25):
        c.fill_box(x, 0, x + 3, 11, "DK", "G", "K")
    c.fill_box(7, 1, 9, 10, "R", None, "DR")
    c.fill_box(22, 1, 24, 10, "R", None, "DR")
    return c


def training_mat():
    c = C(3, 3)
    c.rect(1, 1, 46, 46, "BL")
    c.frame(1, 1, 46, 46, "NV")
    c.frame(4, 4, 43, 43, "W")
    c.ellipse(24, 24, 9, 9, None, "W")
    c.rect(23, 4, 24, 43, "SK")
    return c


def wooden_dummy():
    c = C(1, 2)
    c.fill_box(5, 1, 10, 29, "B", "T", "DB")
    c.ellipse(7.5, 4, 3, 3, "T", "K")
    c.rect(0, 9, 4, 10, "DB")
    c.rect(11, 9, 15, 10, "DB")
    c.rect(1, 15, 5, 16, "DB")
    c.rect(10, 18, 14, 19, "DB")
    c.rect(3, 29, 12, 31, "DB")
    return c


def gong():
    c = C(2, 2)
    c.rect(2, 2, 3, 30, "DR")
    c.rect(28, 2, 29, 30, "DR")
    c.rect(1, 1, 30, 3, "R")
    c.frame(1, 1, 30, 3, "K")
    c.ellipse(16, 15, 10, 10, "GO", "DGO")
    c.ellipse(16, 15, 5, 5, "Y")
    c.ellipse(14, 13, 2, 2, "PY")
    c.rect(0, 29, 6, 31, "DB")
    c.rect(25, 29, 31, 31, "DB")
    return c


# ---- FLYING ---------------------------------------------------------------
def weather_vane():
    c = C(1, 2)
    c.rect(7, 4, 8, 28, "DK")
    c.poly([(1, 6), (12, 6), (15, 4), (15, 8), (12, 7)], "DK")
    c.poly([(1, 3), (5, 6), (1, 9)], "R")
    c.line(4, 13, 11, 13, "G")
    c.line(7, 10, 7, 16, "G")
    c.ellipse(8, 28, 5, 2.5, "G", "K")
    return c


def birdbath():
    c = C(1, 1)
    c.ellipse(8, 5, 7, 3, "L", "K")
    c.ellipse(8, 5, 5, 1.5, "SK")
    c.rect(6, 7, 9, 13, "L")
    c.frame(5, 7, 10, 13, "K")
    c.rect(4, 13, 11, 15, "G")
    return c


def hot_air_balloon():
    c = C(2, 3)
    c.ellipse(16, 14, 13, 13, "R", "K")
    for x0 in (8, 16, 24):
        c.ellipse(x0, 14, 2.5, 12, "Y")
    c.poly([(7, 22), (25, 22), (20, 32), (12, 32)], "R")
    c.line(13, 32, 13, 36, "DB")
    c.line(19, 32, 19, 36, "DB")
    c.fill_box(10, 36, 22, 44, "B", "T", "DB")
    c.outline()
    return c


def cloud_cushion():
    c = C(2, 1)
    for cx, cy, r in ((8, 10, 6), (15, 7, 7), (23, 9, 6), (14, 11, 5)):
        c.ellipse(cx, cy, r, r - 1, "W")
    for x in range(3, 29):
        if c.get(x, 13) == "W":
            c.set(x, 13, "IC")
    c.outline("SK")
    return c


def wind_turbine():
    c = C(1, 3)
    c.poly([(6, 14), (10, 14), (11, 46), (5, 46)], "W")
    c.line(10, 14, 11, 46, "L")
    c.ellipse(8, 11, 2, 2, "L")
    c.poly([(8, 11), (7, 0), (9, 0)], "W")
    c.poly([(8, 11), (0, 17), (1, 18)], "W")
    c.poly([(8, 11), (16, 17), (15, 18)], "W")
    c.outline("G")
    return c


# ---- POISON ---------------------------------------------------------------
def toxic_barrel():
    c = C(1, 1)
    c.fill_box(2, 1, 13, 15, "P", "LV", "DP")
    c.rect(2, 5, 13, 5, "DP")
    c.rect(2, 11, 13, 11, "DP")
    c.ellipse(8, 8, 2.5, 2, "LG")
    c.ellipse(7.5, 1, 4, 1.5, "LG")
    return c


def cauldron():
    c = C(2, 2)
    c.ellipse(16, 20, 13, 10, "DK", "K")
    c.ellipse(13, 17, 5, 3, "G")
    c.ellipse(16, 11, 12, 3, "P", "K")
    c.ellipse(16, 11, 9, 1.5, "LV")
    for x, y, r in ((10, 6, 2), (19, 4, 2.5), (23, 8, 1.5), (14, 2, 1.5)):
        c.ellipse(x, y, r, r, "LV", "P")
    c.rect(6, 28, 9, 31, "K")
    c.rect(22, 28, 25, 31, "K")
    return c


def sludge_puddle():
    c = C(2, 1)
    c.ellipse(16, 8, 14, 6, "P")
    c.ellipse(10, 7, 6, 3, "LV")
    c.ellipse(21, 9, 2, 1.5, "PK")
    c.ellipse(7, 10, 1.5, 1, "PP")
    c.outline("DP")
    return c


def chemistry_set():
    c = C(2, 2)
    c.fill_box(0, 16, 31, 21, "B", "T", "DB")
    c.rect(2, 22, 4, 31, "DB")
    c.rect(27, 22, 29, 31, "DB")
    c.poly([(6, 4), (10, 4), (10, 9), (14, 16), (2, 16), (6, 9)], "LG")
    c.rect(6, 4, 10, 9, "IC")
    c.rect(18, 6, 21, 16, "LV")
    c.rect(18, 4, 21, 9, "IC")
    c.rect(24, 9, 28, 16, "PK")
    c.rect(24, 7, 28, 10, "IC")
    c.outline()
    return c


def poison_mushrooms():
    c = C(1, 1)
    c.rect(4, 9, 6, 15, "PP")
    c.ellipse(5, 7, 5, 3.5, "P")
    c.set(3, 6, "PP"); c.set(6, 5, "PP")
    c.rect(11, 11, 12, 15, "PP")
    c.ellipse(11.5, 10, 3.5, 2.5, "MG")
    c.set(11, 9, "PP")
    c.outline()
    return c


# ---- GROUND ---------------------------------------------------------------
def sandbox():
    c = C(3, 2)
    c.rect(0, 0, 47, 31, "T")
    c.frame(0, 0, 47, 31, "DB")
    c.frame(1, 1, 46, 30, "B")
    for x, y in ((8, 8), (20, 20), (33, 11), (40, 24), (14, 26), (27, 5)):
        c.set(x, y, "BE"); c.set(x + 1, y, "B")
    c.ellipse(36, 18, 5, 3, "BE")
    c.fill_box(10, 12, 15, 17, "R", "SA", "DR")          # bucket
    c.line(20, 8, 26, 14, "DB")                          # shovel
    c.ellipse(27, 15, 2, 2, "G")
    return c


def mine_cart():
    c = C(2, 2)
    c.rect(0, 27, 31, 27, "B")
    c.rect(0, 29, 31, 29, "B")
    c.poly([(2, 8), (29, 8), (26, 24), (5, 24)], "DK")
    c.frame(2, 8, 29, 9, "K")
    c.rect(4, 12, 27, 13, "G")
    for x, y, r in ((9, 6, 4), (17, 4, 5), (24, 6, 3.5)):
        c.ellipse(x, y, r, 3, "B", "DB")
    c.set(16, 3, "Y"); c.set(10, 5, "L")
    c.ellipse(9, 25, 3, 3, "K")
    c.ellipse(22, 25, 3, 3, "K")
    c.outline()
    return c


def amphora():
    c = C(1, 2)
    c.ellipse(8, 18, 6, 9, "DO", "K")
    c.rect(5, 3, 10, 8, "DO")
    c.frame(4, 2, 11, 8, "K")
    c.rect(4, 2, 11, 3, "B")
    c.rect(3, 15, 12, 16, "DB")
    c.rect(4, 20, 11, 20, "BE")
    c.rect(4, 4, 4, 11, "K"); c.rect(11, 4, 11, 11, "K")
    c.ellipse(5.5, 15, 1.5, 4, "O")
    c.rect(5, 27, 10, 29, "DB")
    return c


def sandcastle():
    c = C(2, 2)
    c.rect(2, 18, 29, 29, "T")
    c.rect(4, 8, 11, 18, "T")
    c.rect(20, 8, 27, 18, "T")
    c.rect(12, 12, 19, 18, "BE")
    for x in (4, 7, 10, 20, 23, 26):
        c.rect(x, 6, x + 1, 7, "T")
    c.rect(14, 22, 17, 29, "DB")
    c.line(15, 0, 15, 11, "DB")
    c.poly([(16, 0), (22, 2), (16, 4)], "R")
    c.outline("B")
    return c


def shovel_rack():
    c = C(1, 2)
    c.rect(2, 2, 13, 4, "DB")
    c.rect(2, 26, 13, 28, "DB")
    c.rect(2, 2, 3, 30, "B")
    c.rect(12, 2, 13, 30, "B")
    c.rect(6, 3, 7, 18, "T")
    c.poly([(4, 18), (10, 18), (9, 26), (5, 26)], "L")
    c.rect(9, 0, 10, 8, "T")
    c.rect(8, 8, 11, 12, "G")
    c.outline()
    return c


# ---- ROCK -----------------------------------------------------------------
def boulder():
    c = C(2, 2)
    c.poly([(3, 30), (1, 18), (6, 6), (16, 2), (26, 5), (31, 16), (29, 30)], "G")
    c.poly([(6, 8), (16, 4), (22, 6), (12, 14)], "L")
    c.poly([(20, 22), (29, 18), (28, 29), (18, 29)], "DK")
    c.line(10, 18, 16, 23, "DK")
    c.outline()
    return c


def stalagmite():
    c = C(1, 2)
    c.poly([(8, 1), (14, 31), (2, 31)], "B")
    c.poly([(8, 1), (8, 31), (4, 31)], "T")
    c.line(9, 12, 12, 30, "DB")
    c.outline()
    return c


def fossil_display():
    c = C(2, 2)
    c.fill_box(1, 2, 30, 24, "BE", None, "T", "DB")
    c.ellipse(16, 13, 9, 8, None, "B")
    c.ellipse(16, 13, 6, 5, None, "B")
    c.ellipse(16, 13, 3, 2, None, "B")
    c.line(7, 13, 11, 13, "B")
    c.fill_box(4, 24, 27, 30, "DB", "B", "K")
    c.rect(13, 26, 18, 28, "GO")
    return c


def geode():
    c = C(1, 1)
    c.ellipse(8, 9, 7, 6, "G", "K")
    c.ellipse(8, 9, 4.5, 3.5, "P")
    c.poly([(5, 10), (7, 6), (8, 10)], "LV")
    c.poly([(8, 11), (10, 6), (12, 11)], "LV")
    c.set(7, 7, "PP"); c.set(10, 7, "PP")
    return c


def standing_stones():
    c = C(3, 2)
    c.fill_box(3, 8, 11, 30, "G", "L", "DK")
    c.fill_box(36, 8, 44, 30, "G", "L", "DK")
    c.fill_box(1, 3, 46, 9, "G", "L", "DK")
    c.fill_box(19, 14, 28, 30, "G", "L", "DK")
    c.rect(20, 18, 27, 19, "GR")
    return c


# ---- BUG --------------------------------------------------------------
def honeycomb():
    c = C(1, 2)
    c.ellipse(8, 14, 7, 12, "Y", "DO")
    for y in (5, 10, 15, 20):
        c.rect(2, y, 13, y, "O")
    c.ellipse(8, 21, 2, 1.5, "K")
    c.rect(7, 26, 8, 31, "DB")
    c.rect(4, 29, 11, 31, "B")
    return c


def ant_farm():
    c = C(2, 2)
    c.fill_box(1, 1, 30, 26, "T", None, None, "K")
    c.rect(2, 2, 29, 6, "IC")
    c.line(5, 7, 12, 14, "DB", 2)
    c.line(12, 14, 22, 12, "DB", 2)
    c.line(22, 12, 26, 20, "DB", 2)
    c.line(12, 14, 9, 22, "DB", 2)
    for x, y in ((8, 10), (17, 13), (24, 17)):
        c.set(x, y, "K"); c.set(x + 1, y, "K")
    c.fill_box(0, 26, 31, 31, "B", "T", "DB")
    return c


def bug_net_rack():
    c = C(1, 2)
    c.rect(1, 29, 14, 31, "DB")
    c.rect(7, 10, 8, 30, "T")
    c.ellipse(8, 6, 6, 6, "IC", "K")
    for k in range(3, 12, 3):
        c.line(2 + k, 1, 2 + k, 11, "L")
    c.rect(1, 5, 14, 5, "L")
    return c


def spider_web():
    c = C(2, 2)
    c.rect(0, 0, 1, 31, "DB")
    c.rect(30, 0, 31, 31, "DB")
    for a in range(0, 360, 45):
        r = math.radians(a)
        c.line(16, 15, 16 + 15 * math.cos(r), 15 + 14 * math.sin(r), "W")
    for rr in (4, 8, 12):
        pts = [(16 + rr * math.cos(math.radians(a)), 15 + rr * math.sin(math.radians(a))) for a in range(0, 361, 45)]
        for p, q in zip(pts, pts[1:]):
            c.line(p[0], p[1], q[0], q[1], "L")
    c.ellipse(20, 11, 2, 2, "K")
    c.set(19, 10, "R")
    return c


def firefly_jar():
    c = C(1, 1)
    c.rect(4, 4, 11, 15, "NV")
    c.frame(3, 3, 12, 15, "ST")
    c.rect(4, 1, 11, 3, "B")
    c.frame(4, 0, 11, 2, "K")
    for x, y in ((6, 7), (9, 10), (7, 12), (10, 6)):
        c.set(x, y, "LG")
    c.set(6, 6, "W")
    return c


# ---- GHOST ------------------------------------------------------------
def gravestone():
    c = C(1, 2)
    c.ellipse(8, 9, 6, 6, "G")
    c.rect(2, 9, 13, 27, "G")
    c.rect(3, 4, 4, 26, "L")
    c.line(8, 9, 8, 17, "DK")
    c.line(5, 12, 11, 12, "DK")
    c.outline()
    c.rect(0, 28, 15, 30, "DG")
    c.rect(2, 27, 13, 28, "GR")
    return c


def candelabra():
    c = C(1, 2)
    c.rect(7, 10, 8, 28, "GO")
    c.rect(4, 28, 11, 30, "DGO")
    c.rect(2, 12, 13, 13, "GO")
    for x in (2, 7, 12):
        c.rect(x, 6, x + 1, 11, "W")
        c.ellipse(x + 1, 3.5, 1.5, 2.5, "LV")
        c.set(x + 1, 4, "PP")
    c.outline()
    return c


def jack_o_lantern():
    c = C(1, 1)
    c.ellipse(8, 9, 7, 6, "O", "DO")
    c.rect(7, 1, 8, 3, "DG")
    c.poly([(3, 7), (6, 7), (4.5, 5)], "Y")
    c.poly([(10, 7), (13, 7), (11.5, 5)], "Y")
    c.rect(4, 10, 11, 12, "Y")
    c.set(6, 10, "O"); c.set(9, 12, "O")
    c.outline()
    return c


def haunted_wardrobe():
    c = C(2, 2)
    c.fill_box(2, 1, 29, 29, "DP", "P", "K")
    c.rect(15, 2, 16, 28, "K")
    c.rect(12, 13, 13, 16, "GO")
    c.rect(18, 13, 19, 16, "GO")
    c.ellipse(8, 9, 2, 2.5, "PP")                       # eyes peeking out
    c.ellipse(23, 9, 2, 2.5, "PP")
    c.set(8, 9, "BK"); c.set(23, 9, "BK")
    c.rect(3, 29, 5, 31, "K")
    c.rect(26, 29, 28, 31, "K")
    return c


def spirit_lantern():
    c = C(1, 1)
    c.rect(7, 0, 8, 2, "DK")
    c.fill_box(3, 2, 12, 14, "DP", None, None, "K")
    c.ellipse(7.5, 9, 3, 4, "LV")
    c.ellipse(7.5, 10, 1.5, 2, "PP")
    c.rect(2, 14, 13, 15, "K")
    return c


# ---- STEEL --------------------------------------------------------------
def anvil():
    c = C(2, 1)
    c.poly([(1, 2), (24, 2), (30, 4), (24, 7), (8, 7)], "ST")
    c.rect(1, 2, 24, 3, "W")
    c.rect(10, 7, 20, 11, "G")
    c.rect(6, 11, 24, 15, "DK")
    c.outline()
    return c


def suit_of_armor():
    c = C(1, 2)
    c.ellipse(8, 5, 4, 5, "ST", "K")
    c.rect(5, 5, 11, 5, "K")
    c.fill_box(3, 10, 12, 20, "ST", "W", "G")
    c.rect(4, 20, 6, 29, "ST")
    c.rect(9, 20, 11, 29, "ST")
    c.rect(13, 2, 13, 29, "G")
    c.poly([(12, 0), (14, 0), (13, 3)], "W")
    c.outline()
    return c


def big_gears():
    c = C(2, 2)
    def gear(cx, cy, r, col, dark):
        for a in range(0, 360, 45):
            rad = math.radians(a)
            c.ellipse(cx + (r + 1) * math.cos(rad), cy + (r + 1) * math.sin(rad), 2, 2, col)
        c.ellipse(cx, cy, r, r, col, dark)
        c.ellipse(cx, cy, r / 3, r / 3, "K")
    gear(11, 18, 8, "ST", "G")
    gear(23, 9, 6, "L", "G")
    c.outline()
    return c


def toolbox():
    c = C(1, 1)
    c.fill_box(1, 6, 14, 15, "R", "SA", "DR")
    c.rect(1, 9, 14, 9, "DR")
    c.frame(5, 2, 10, 6, "DK")
    c.rect(7, 9, 8, 11, "L")
    return c


def girders():
    c = C(3, 1)
    for y0 in (2, 8):
        c.rect(1, y0, 46, y0 + 4, "ST")
        c.rect(1, y0, 46, y0, "W")
        c.rect(1, y0 + 2, 46, y0 + 2, "G")
        for x in range(4, 46, 8):
            c.set(x, y0 + 1, "DK")
    c.outline()
    return c


# ---- FIRE ------------------------------------------------------------
def fireplace():
    c = C(2, 2)
    c.fill_box(0, 4, 31, 31, "DR", "R", "K")
    for y in range(6, 30, 4):
        c.line(1, y, 30, y, "K")
    c.fill_box(0, 2, 31, 6, "B", "T", "DB")
    c.rect(7, 13, 24, 30, "K")
    c.poly([(9, 30), (12, 18), (14, 24), (16, 15), (19, 23), (21, 19), (23, 30)], "O")
    c.poly([(12, 30), (14, 23), (16, 27), (18, 22), (20, 30)], "Y")
    c.rect(9, 28, 22, 30, "DB")
    return c


def brazier():
    c = C(1, 2)
    c.poly([(2, 13), (13, 13), (11, 19), (4, 19)], "DK")
    c.rect(7, 19, 8, 28, "DK")
    c.rect(4, 28, 11, 30, "DK")
    c.poly([(3, 13), (5, 6), (7, 9), (8, 1), (10, 8), (12, 5), (12, 13)], "O")
    c.poly([(5, 13), (7, 8), (8, 11), (10, 13)], "Y")
    c.outline()
    return c


def lava_lamp():
    c = C(1, 2)
    c.poly([(5, 26), (3, 14), (6, 4), (9, 4), (12, 14), (10, 26)], "O")
    c.ellipse(7, 9, 1.5, 2.5, "Y")
    c.ellipse(8, 18, 2.5, 3, "R")
    c.ellipse(6, 14, 1, 1.5, "Y")
    c.rect(5, 1, 10, 4, "G")
    c.poly([(4, 26), (11, 26), (13, 31), (2, 31)], "G")
    c.outline()
    return c


def campfire():
    c = C(1, 1)
    c.line(2, 14, 13, 11, "DB", 2)
    c.line(2, 11, 13, 14, "B", 2)
    c.poly([(3, 12), (5, 6), (7, 8), (8, 1), (10, 7), (12, 5), (12, 12)], "O")
    c.poly([(6, 12), (8, 6), (10, 12)], "Y")
    c.outline()
    return c


def volcano_model():
    c = C(3, 2)
    c.poly([(2, 30), (18, 6), (30, 6), (46, 30)], "DB")
    c.poly([(18, 6), (30, 6), (26, 10), (22, 10)], "R")
    c.poly([(24, 8), (21, 17), (19, 24), (23, 15), (26, 10)], "O")
    c.poly([(27, 9), (31, 18), (30, 20), (27, 13)], "O")
    c.poly([(6, 30), (18, 12), (14, 30)], "B")
    for x, y in ((20, 2), (26, 0), (30, 3), (23, 4)):
        c.ellipse(x, y, 1.5, 1.5, "G")
    c.outline()
    return c


# ---- WATER ------------------------------------------------------------
def aquarium():
    c = C(2, 2)
    c.fill_box(1, 2, 30, 24, "BL", None, None, "K")
    c.rect(2, 3, 29, 5, "SK")
    c.rect(2, 20, 29, 23, "T")
    c.line(6, 20, 6, 12, "GR"); c.line(8, 20, 9, 14, "GR"); c.line(25, 20, 24, 11, "GR")
    c.poly([(12, 11), (19, 9), (19, 14)], "O")
    c.poly([(19, 11), (22, 9), (22, 14)], "O")
    c.set(14, 11, "K")
    c.ellipse(16, 17, 1, 1, "IC"); c.ellipse(18, 7, 1, 1, "IC")
    c.fill_box(0, 24, 31, 31, "DK", "G", "K")
    return c


def fountain():
    c = C(3, 3)
    c.ellipse(24, 32, 22, 13, "L", "K")
    c.ellipse(24, 31, 18, 10, "BL")
    c.ellipse(20, 29, 8, 4, "SK")
    c.rect(21, 14, 26, 31, "L")
    c.frame(20, 14, 27, 31, "K")
    c.ellipse(24, 13, 8, 3, "L", "K")
    c.line(24, 2, 24, 12, "SK", 2)
    c.line(24, 3, 16, 12, "IC")
    c.line(25, 3, 32, 12, "IC")
    c.ellipse(24, 2, 2, 2, "IC")
    return c


def surfboard():
    c = C(1, 2)
    c.ellipse(8, 16, 5, 15, "Y", "K")
    c.rect(7, 2, 8, 30, "R")
    c.ellipse(6, 10, 1.5, 5, "PY")
    return c


def life_ring():
    c = C(1, 1)
    c.ellipse(8, 8, 7, 7, "W", "K")
    c.ellipse(8, 8, 3, 3, None, "K")
    for x, y in ((8, 2), (8, 13), (2, 8), (13, 8)):
        c.rect(x - 1, y - 1, x + 1, y + 1, "R")
    return c


def ships_wheel():
    c = C(2, 2)
    for a in range(0, 360, 45):
        r = math.radians(a)
        c.line(16, 15, 16 + 14 * math.cos(r), 15 + 14 * math.sin(r), "B", 2)
        c.ellipse(16 + 14 * math.cos(r), 15 + 14 * math.sin(r), 1.5, 1.5, "T")
    c.ellipse(16, 15, 10, 10, None, "B")
    c.ellipse(16, 15, 3, 3, "GO", "DGO")
    c.outline()
    return c


# ---- GRASS ------------------------------------------------------------
def bonsai():
    c = C(1, 1)
    c.fill_box(2, 11, 13, 15, "B", "T", "DB")
    c.line(8, 11, 6, 6, "DB", 2)
    c.line(7, 7, 11, 4, "DB")
    c.ellipse(5, 5, 4, 2.5, "GR")
    c.ellipse(11, 3, 3.5, 2.5, "GR")
    c.set(4, 4, "LG"); c.set(10, 2, "LG")
    c.outline()
    return c


def sunflowers():
    c = C(1, 2)
    for x, top in ((4, 4), (11, 9)):
        c.line(x, top + 4, x, 30, "DG")
        c.ellipse(x, top, 4, 4, "Y")
        c.ellipse(x, top, 2, 2, "DB")
    c.poly([(4, 18), (0, 15), (2, 20)], "GR")
    c.poly([(11, 22), (15, 19), (13, 24)], "GR")
    c.outline()
    return c


def flower_bed():
    c = C(3, 1)
    c.fill_box(0, 6, 47, 15, "DB", "B", "K")
    import random
    rnd = random.Random(4)
    for x in range(3, 46, 4):
        col = rnd.choice(["R", "Y", "SA", "W"])
        c.line(x, 6, x, 9, "B")
        c.ellipse(x, 4, 2, 2, col)
        c.set(x, 4, "Y" if col != "Y" else "O")
    c.outline()
    return c


def oak_tree():
    c = C(2, 3)
    c.rect(13, 26, 18, 44, "DB")
    c.rect(13, 26, 14, 44, "B")
    for cx, cy, r in ((16, 10, 10), (8, 18, 8), (24, 18, 8), (16, 22, 8)):
        c.ellipse(cx, cy, r, r - 1, "GR")
    for cx, cy, r in ((12, 8, 4), (8, 15, 3), (20, 14, 3)):
        c.ellipse(cx, cy, r, r, "LG")
    c.ellipse(22, 22, 4, 3, "DG")
    c.outline()
    c.rect(10, 44, 21, 46, "DB")
    return c


def vegetable_patch():
    c = C(2, 2)
    c.rect(0, 0, 31, 31, "DB")
    c.frame(0, 0, 31, 31, "K")
    for y in (6, 16, 26):
        c.rect(1, y, 30, y + 1, "B")
        for x in range(4, 30, 7):
            if y == 6:
                c.ellipse(x, y - 1, 2, 2, "GR"); c.set(x, y + 1, "O")
            elif y == 16:
                c.ellipse(x, y - 1, 2.5, 2, "LG")
            else:
                c.ellipse(x, y - 1, 2, 2, "R"); c.set(x, y - 3, "GR")
    return c


# ---- ELECTRIC ------------------------------------------------------------
def tesla_coil():
    c = C(1, 3)
    c.poly([(4, 46), (11, 46), (10, 40), (5, 40)], "DK")
    c.rect(6, 14, 9, 40, "B")
    for y in range(15, 40, 2):
        c.rect(6, y, 9, y, "O")
    c.ellipse(7.5, 9, 6, 4, "ST", "K")
    c.ellipse(6, 8, 2, 1.5, "W")
    c.line(1, 6, 3, 2, "Y")
    c.line(3, 2, 2, 0, "Y")
    c.line(14, 6, 12, 2, "Y")
    c.line(12, 2, 14, 0, "Y")
    c.outline()
    return c


def generator():
    c = C(2, 2)
    c.fill_box(1, 6, 30, 29, "Y", "PY", "DO")
    c.rect(5, 10, 26, 18, "K")
    for x in range(6, 26, 3):
        c.rect(x, 11, x + 1, 17, "DK")
    c.poly([(16, 20), (12, 25), (16, 25), (14, 29), (20, 23), (16, 23), (18, 20)], "K")
    c.rect(4, 2, 8, 6, "DK")
    c.rect(23, 2, 27, 6, "DK")
    c.rect(5, 0, 7, 2, "R")
    c.rect(24, 0, 26, 2, "BL")
    return c


def giant_battery():
    c = C(1, 2)
    c.fill_box(2, 4, 13, 30, "DK", "G", "K")
    c.rect(3, 4, 12, 13, "O")
    c.rect(3, 4, 4, 13, "Y")
    c.rect(5, 1, 10, 4, "L")
    c.frame(5, 1, 10, 4, "K")
    c.rect(6, 19, 9, 20, "W")
    c.rect(7, 18, 8, 21, "W")
    return c


def neon_sign():
    c = C(3, 1)
    c.fill_box(0, 1, 47, 14, "BK", None, None, "DK")
    pts = [(4, 11), (9, 4), (9, 9), (14, 3)]
    for p, q in zip(pts, pts[1:]):
        c.line(p[0], p[1], q[0], q[1], "GO")
    for i, x in enumerate(range(18, 44, 6)):
        col = ["PK", "LV", "PK", "LV", "PK"][i]
        c.frame(x, 4, x + 3, 11, col)
    return c


def plasma_ball():
    c = C(1, 1)
    c.ellipse(8, 7, 6, 6, "DP", "K")
    c.ellipse(8, 7, 1.5, 1.5, "PP")
    for a in (20, 110, 200, 290):
        r = math.radians(a)
        c.line(8, 7, 8 + 5 * math.cos(r), 7 + 5 * math.sin(r), "PK")
    c.rect(4, 13, 11, 15, "DK")
    c.frame(3, 13, 12, 15, "K")
    return c


# ---- PSYCHIC ------------------------------------------------------------
def crystal_ball():
    c = C(1, 1)
    c.ellipse(8, 7, 6, 6, "LV", "P")
    c.ellipse(6, 5, 2, 2, "PP")
    c.ellipse(9.5, 9, 2, 1.5, "PK")
    c.poly([(3, 15), (5, 11), (11, 11), (13, 15)], "GO")
    c.outline()
    return c


def zen_cushion():
    c = C(1, 1)
    c.ellipse(8, 9, 7, 5, "MG", "DP")
    c.ellipse(8, 9, 4, 2.5, "PK")
    c.ellipse(8, 9, 1.5, 1, "GO")
    return c


def bent_spoon():
    c = C(1, 2)
    c.rect(3, 26, 12, 30, "DK")
    c.frame(2, 25, 13, 31, "K")
    c.line(8, 25, 8, 14, "L", 2)
    c.line(8, 14, 4, 8, "L", 2)
    c.ellipse(4, 5, 3, 4, "L", "G")
    c.ellipse(3.5, 4, 1, 1.5, "W")
    c.outline()
    return c


def fortune_table():
    c = C(2, 2)
    c.ellipse(16, 18, 15, 7, "P", "DP")
    c.ellipse(16, 17, 12, 5, "LV")
    c.rect(3, 22, 28, 28, "P")
    c.rect(3, 26, 28, 28, "DP")
    for x in (6, 13, 20):
        c.fill_box(x, 14, x + 5, 20, "W", None, None, "DP")
        c.set(x + 2, 17, "GO")
    c.ellipse(16, 8, 5, 5, "PP", "P")
    c.ellipse(14.5, 6.5, 1.5, 1.5, "W")
    c.outline()
    return c


def hypno_spiral():
    c = C(2, 2)
    c.ellipse(16, 13, 13, 13, "W", "K")
    for t in range(0, 720, 3):
        r = 12 * t / 720
        a = math.radians(t)
        c.set(16 + r * math.cos(a), 13 + r * math.sin(a), "MG")
        c.set(16 + r * math.cos(a) + 0.6, 13 + r * math.sin(a), "MG")
    c.rect(15, 26, 16, 29, "DK")
    c.rect(10, 29, 21, 31, "DK")
    return c


# ---- ICE ------------------------------------------------------------
def ice_block():
    c = C(1, 1)
    c.fill_box(1, 2, 14, 15, "IC", "W", "SK", "BL")
    c.line(4, 5, 7, 5, "W")
    c.line(9, 10, 11, 12, "SK")
    return c


def snowman():
    c = C(1, 2)
    c.ellipse(8, 24, 7, 7, "W")
    c.ellipse(8, 13, 5.5, 5.5, "W")
    c.ellipse(8, 5, 4, 4, "W")
    c.rect(3, 0, 12, 1, "K"); c.rect(5, -2, 10, 1, "K")
    c.set(6, 5, "K"); c.set(10, 5, "K"); c.set(8, 7, "O"); c.set(9, 7, "O")
    for y in (12, 15, 21):
        c.set(8, y, "K")
    c.line(3, 11, 0, 8, "DB"); c.line(13, 11, 15, 8, "DB")
    c.rect(4, 9, 12, 10, "R")
    c.outline("SK")
    return c


def ice_sculpture():
    c = C(1, 2)
    c.poly([(8, 2), (13, 10), (11, 12), (13, 20), (8, 26), (3, 20), (5, 12), (3, 10)], "IC")
    c.line(8, 3, 8, 25, "W")
    c.line(5, 12, 11, 12, "SK")
    c.rect(2, 26, 13, 30, "SK")
    c.outline("BL")
    return c


def igloo():
    c = C(3, 2)
    c.ellipse(24, 30, 22, 26, "W", "SK")
    for y in (12, 20):
        c.line(6, y, 42, y, "IC")
    for x in (14, 24, 34):
        c.line(x, 8, x, 12, "IC")
    for x in (10, 19, 29, 38):
        c.line(x, 12, x, 20, "IC")
    c.ellipse(24, 30, 6, 9, "NV")
    c.rect(0, 30, 47, 31, None)
    return c


def frozen_pond():
    c = C(3, 2)
    c.ellipse(24, 16, 22, 14, "SK", "W")
    c.ellipse(24, 16, 17, 10, "IC")
    c.line(14, 12, 24, 20, "W")
    c.line(24, 20, 34, 14, "W")
    c.line(28, 9, 31, 13, "SK")
    return c


# ---- DRAGON ------------------------------------------------------------
def dragon_statue():
    c = C(2, 2)
    c.fill_box(4, 24, 27, 30, "G", "L", "DK")
    c.poly([(8, 24), (10, 12), (16, 8), (14, 4), (20, 2), (24, 6), (20, 10), (22, 16), (24, 24)], "GO")
    c.poly([(10, 14), (2, 6), (4, 14), (1, 18), (10, 18)], "DGO")
    c.set(20, 4, "R")
    c.line(12, 20, 20, 20, "DGO")
    c.outline()
    return c


def egg_nest():
    c = C(1, 1)
    c.ellipse(8, 12, 7, 3.5, "B", "DB")
    c.ellipse(8, 7, 4, 5.5, "PY", "K")
    c.ellipse(7, 6, 1, 1, "R"); c.ellipse(9.5, 9, 1, 1, "R")
    c.line(2, 12, 14, 12, "T")
    return c


def gold_hoard():
    c = C(2, 2)
    c.poly([(1, 30), (8, 16), (16, 10), (24, 16), (30, 30)], "GO")
    for x, y in ((8, 24), (14, 18), (20, 22), (24, 27), (12, 28), (17, 14)):
        c.ellipse(x, y, 2, 1.5, "Y" if (x + y) % 2 else "DGO")
    c.fill_box(18, 4, 28, 12, "R", None, None, "K")
    c.rect(20, 6, 26, 10, "GO")
    c.set(23, 8, "W")
    c.outline()
    return c


def dragon_banner():
    c = C(1, 2)
    c.rect(0, 0, 15, 1, "DGO")
    c.poly([(2, 2), (13, 2), (13, 28), (7.5, 24), (2, 28)], "R")
    c.ellipse(7.5, 12, 3.5, 3.5, "GO")
    c.ellipse(7.5, 12, 1.5, 1.5, "R")
    c.outline()
    return c


def stone_lantern():
    c = C(1, 2)
    c.poly([(1, 7), (8, 1), (14, 7)], "G")
    c.rect(4, 8, 11, 15, "L")
    c.rect(6, 10, 9, 13, "Y")
    c.rect(2, 15, 13, 17, "G")
    c.rect(6, 17, 9, 27, "L")
    c.rect(3, 27, 12, 30, "G")
    c.outline()
    return c


# ---- DARK ------------------------------------------------------------
def dark_throne():
    c = C(2, 2)
    c.poly([(6, 18), (6, 4), (10, 0), (16, 4), (22, 0), (26, 4), (26, 18)], "DP")
    c.rect(9, 5, 23, 17, "P")
    c.fill_box(3, 17, 28, 23, "DP", "P", "K")
    c.rect(5, 23, 7, 31, "BK")
    c.rect(24, 23, 26, 31, "BK")
    c.ellipse(16, 9, 2, 2, "R")
    c.outline()
    return c


def thieves_safe():
    c = C(1, 1)
    c.fill_box(1, 1, 14, 15, "DK", "G", "K")
    c.ellipse(7.5, 8, 3, 3, "L", "K")
    c.set(7, 8, "K")
    c.rect(11, 6, 12, 10, "L")
    return c


def crow_perch():
    c = C(1, 2)
    c.rect(7, 12, 8, 29, "DB")
    c.rect(3, 29, 12, 31, "DB")
    c.rect(1, 11, 14, 12, "B")
    c.poly([(4, 10), (6, 3), (10, 3), (12, 7), (14, 6), (11, 10)], "BK")
    c.set(7, 4, "R")
    c.poly([(10, 4), (13, 5), (10, 6)], "DK")
    c.outline("DK")
    return c


def moon_lamp():
    c = C(1, 1)
    c.ellipse(8, 7, 6, 6, "PY")
    c.ellipse(11, 5, 5, 5, None)
    c.outline("Y")
    c.rect(5, 14, 10, 15, "DK")
    c.rect(7, 12, 8, 14, "DK")
    return c


def spiked_fence():
    c = C(3, 1)
    for x in range(2, 46, 5):
        c.rect(x, 3, x + 1, 15, "BK")
        c.poly([(x - 1, 4), (x + 0.5, 0), (x + 2, 4)], "DK")
    c.rect(0, 6, 47, 7, "BK")
    c.rect(0, 12, 47, 13, "BK")
    return c


# ---- FAIRY ------------------------------------------------------------
def fairy_ring():
    c = C(2, 2)
    for a in range(0, 360, 30):
        r = math.radians(a)
        x, y = 16 + 12 * math.cos(r), 16 + 11 * math.sin(r)
        c.ellipse(x, y, 2, 1.5, "PK" if a % 60 else "PP")
        c.set(x, y + 1.5, "W")
    c.ellipse(16, 16, 2, 2, "PP")
    c.set(16, 16, "GO")
    return c


def tiered_cake():
    c = C(1, 1)
    c.fill_box(1, 10, 14, 15, "PP", "W", "PK", "MG")
    c.fill_box(3, 5, 12, 10, "PP", "W", "PK", "MG")
    c.fill_box(5, 1, 10, 5, "PP", "W", "PK", "MG")
    c.ellipse(7.5, 0.5, 1, 1, "R")
    return c


def rose_arch():
    c = C(3, 2)
    c.rect(2, 8, 5, 31, "W")
    c.rect(42, 8, 45, 31, "W")
    c.ellipse(24, 12, 22, 11, None, "W")
    for y in range(13, 32):
        for x in range(0, 48):
            if c.get(x, y) == "W" and x not in range(2, 6) and x not in range(42, 46):
                c.set(x, y, None)
    for a in range(180, 361, 18):
        r = math.radians(a)
        c.ellipse(24 + 21 * math.cos(r), 12 + 10 * math.sin(r), 2, 2, "PK")
    for y in range(12, 30, 6):
        c.ellipse(3.5, y, 2, 2, "PK")
        c.ellipse(43.5, y + 3, 2, 2, "PK")
    c.outline("MG")
    return c


def teddy_bear():
    c = C(1, 1)
    c.ellipse(8, 11, 5, 4.5, "T")
    c.ellipse(8, 5, 4, 3.5, "T")
    c.ellipse(4, 2, 1.5, 1.5, "T"); c.ellipse(12, 2, 1.5, 1.5, "T")
    c.set(7, 5, "K"); c.set(9, 5, "K"); c.set(8, 6, "DB")
    c.rect(6, 8, 10, 8, "R")
    c.ellipse(8, 12, 2.5, 2, "BE")
    c.outline("DB")
    return c


def star_lamp():
    c = C(1, 2)
    pts = []
    for k in range(10):
        r = 7 if k % 2 == 0 else 3
        a = math.radians(-90 + k * 36)
        pts.append((8 + r * math.cos(a), 8 + r * math.sin(a)))
    c.poly(pts, "GO")
    c.ellipse(8, 8, 2, 2, "PY")
    c.rect(7, 15, 8, 28, "PK")
    c.rect(4, 28, 11, 30, "MG")
    c.outline()
    return c


# ---- Practice trainers (edit-mode icons: a trainer card on a stand) ----------
def _trainer_icon(col, light, stars):
    c = C(1, 1)
    c.fill_box(1, 1, 14, 14, col, light, None, "K")
    c.ellipse(8, 6, 2.5, 2.5, "W")                       # a little person
    c.rect(5, 9, 10, 13, "W")
    for k in range(stars):
        c.set(3 + k * 4, 3, "Y")
    return c


def trainer_beginner():
    return _trainer_icon("GR", "LG", 1)


def trainer_intermediate():
    return _trainer_icon("BL", "SK", 2)


def trainer_advanced():
    return _trainer_icon("R", "SA", 3)


def trainer_recruit():
    """A trainer who joined the gym from the road: a gold card."""
    c = C(1, 1)
    c.fill_box(1, 1, 14, 14, "GO", "Y", "DGO")
    c.ellipse(8, 6, 2.5, 2.5, "W")
    c.rect(5, 9, 10, 13, "W")
    c.set(3, 3, "PY"); c.set(12, 3, "PY")
    return c


# ---- Gym Leader Convention trophies (one per hall, and one for all of them) ---------------
# The gym's video memory is nearly full, so the trophies are small (one block) and all the same
# picture: a gold cup on a pedestal whose colour is one palette index (TROPHY_COLORS: palette
# slot, index). Pictures that differ only in their palette slot share tiles, and each half of a
# trophy is the other half flipped, so all twenty need just eight tiles.
TROPHY_COLORS = {
    "FIRE": (8, 6), "WATER": (9, 6), "POISON": (10, 6),
    "ELECTRIC": (8, 10), "FLYING": (9, 10), "FAIRY": (10, 10),
    "GROUND": (8, 12), "GRASS": (9, 12), "CHAMPIONS": (10, 12),
    "FIGHTING": (8, 8), "ICE": (9, 8), "PSYCHIC": (10, 8),
    "ROCK": (8, 13), "BUG": (9, 13), "DARK": (10, 13),
    "GRAND": (8, 5), "DRAGON": (9, 5), "GHOST": (10, 5),
    "NORMAL": (8, 15), "STEEL": (9, 15),
}


def _trophy(hall):
    def piece():
        slot, idx = TROPHY_COLORS[hall]
        col = PALETTES[slot][idx - 1][0]
        c = C(1, 1)
        for x0 in (0,):          # the left half; mirrored below
            c.rect(2, 0, 7, 0, "K")                       # the cup's rim
            c.rect(3, 1, 7, 4, "GO")
            c.set(2, 1, "K"); c.rect(2, 2, 2, 3, "K")
            c.rect(4, 1, 4, 3, "W")                       # shine
            c.rect(3, 4, 4, 4, "DGO")
            c.rect(4, 5, 7, 5, "DGO"); c.set(3, 5, "K"); c.set(3, 4, "K")
            c.rect(1, 1, 1, 3, "K"); c.set(1, 4, "K")     # handle
            c.rect(6, 6, 7, 7, "DGO"); c.set(5, 6, "K"); c.set(5, 7, "K")
            c.rect(6, 8, 7, 8, col); c.set(5, 8, "K")     # neck
            c.rect(3, 9, 7, 14, col)                      # pedestal
            c.rect(3, 9, 7, 9, "K"); c.rect(2, 9, 2, 14, "K")
            c.rect(5, 11, 7, 12, "L")                     # name plate
            c.set(4, 10, "W")
            c.rect(1, 14, 7, 15, "G"); c.rect(1, 15, 7, 15, "K"); c.set(0, 14, "K"); c.set(1, 14, "K")
        c.mirror()
        return c
    return piece


for _h in TROPHY_COLORS:
    globals()[f"convention_trophy_{_h.lower()}"] = _trophy(_h)


PIECES = {name: fn for name, fn in globals().items() if callable(fn) and not name.startswith("_")
          and name not in ("C", "Canvas") and name != "piece"}


# ---------------------------------------------------------------------------
# Converting a canvas to 8x8 tiles in the gym's palettes
# ---------------------------------------------------------------------------
def _nearest(rgb, slot):
    best, bi = None, 0
    for i, (_, c) in enumerate(PALETTES[slot]):
        d = sum((a - b) ** 2 for a, b in zip(rgb, c))
        if best is None or d < best:
            best, bi = d, i
    return bi + 1


def to_tiles(canvas):
    """Returns ({(tx, ty): (slot, 64 color indices)}, number of pixels recolored)."""
    out, recolored = {}, 0
    for ty in range(canvas.h // 8):
        for tx in range(canvas.w // 8):
            names = [canvas.px[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8)]
            used = set(n for n in names if n is not None)
            slot = None
            for s, cols in PALETTES.items():
                if used <= set(n for n, _ in cols):
                    slot = s
                    break
            if slot is None:
                slot = max(PALETTES, key=lambda s: sum(1 for n in names if n in dict(PALETTES[s])))
            idx = {n: i + 1 for i, (n, _) in enumerate(PALETTES[slot])}
            px = []
            for n in names:
                if n is None:
                    px.append(0)
                elif n in idx:
                    px.append(idx[n])
                else:
                    px.append(_nearest(RGB[n], slot))
                    recolored += 1
            out[(tx, ty)] = (slot, px)
    return out, recolored


def render(canvas, scale=1, bg=(198, 190, 160)):
    from PIL import Image
    img = Image.new("RGB", (canvas.w, canvas.h), bg)
    tiles, _ = to_tiles(canvas)
    for (tx, ty), (slot, px) in tiles.items():
        for i, p in enumerate(px):
            if p:
                img.putpixel((tx * 8 + i % 8, ty * 8 + i // 8), PALETTES[slot][p - 1][1])
    return img.resize((canvas.w * scale, canvas.h * scale), Image.NEAREST)


if __name__ == "__main__":
    import sys
    from PIL import Image, ImageDraw
    names = sorted(PIECES)
    cols, cell = 8, 120
    sheet = Image.new("RGB", (cols * cell, ((len(names) + cols - 1) // cols) * cell), (40, 40, 48))
    d = ImageDraw.Draw(sheet)
    for k, n in enumerate(names):
        cv = PIECES[n]()
        _, rec = to_tiles(cv)
        sheet.paste(render(cv, 2), ((k % cols) * cell + 4, (k // cols) * cell + 4))
        d.text(((k % cols) * cell + 4, (k // cols) * cell + cell - 12), f"{n[:16]} {rec or ''}", fill=(255, 255, 255))
    sheet.save(sys.argv[1] if len(sys.argv) > 1 else "decor_art.png")
