#!/usr/bin/env python3
"""Pixel art for the player's gym badges: one 32x32 badge per type, 16 colors each.

Usage: python3 tools/gym_hack/badge_art.py [preview.png]

Writes graphics/gym_badges/<type>.png (indexed, color 0 transparent). The gym tileset build
(build_gym_decor.py) and src/gym_badge.c use them: the badge pedestal in the gym shows the
player's badge, and a challenger who wins is shown it as they're awarded it.

Each badge is drawn from shapes filled with "materials" (a dark, mid and light shade). Shading
is automatic: the light shade on edges facing the top-left, the dark one facing the bottom-right,
and a dark outline around everything.
"""
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "graphics/gym_badges"
N = 32


class Badge:
    def __init__(self, outline=(24, 20, 32)):
        self.mat = [[None] * N for _ in range(N)]     # material name per pixel
        self.flat = [[False] * N for _ in range(N)]   # pixels that keep the mid shade
        self.ramps = {}
        self.outline = outline
        self.extra = [[None] * N for _ in range(N)]   # single-color details drawn on top

    def material(self, name, dark, mid, light):
        self.ramps[name] = (dark, mid, light)

    # --- shapes -------------------------------------------------------------------
    def plot(self, x, y, m, flat=False):
        if 0 <= x < N and 0 <= y < N:
            self.mat[y][x] = m
            self.flat[y][x] = flat
            self.extra[y][x] = None

    def ellipse(self, cx, cy, rx, ry, m, flat=False):
        for y in range(N):
            for x in range(N):
                if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1:
                    self.plot(x, y, m, flat)

    def poly(self, pts, m, flat=False):
        for y in range(N):
            for x in range(N):
                if inside(x + 0.5, y + 0.5, pts):
                    self.plot(x, y, m, flat)

    def line(self, x0, y0, x1, y1, m, width=1.0, flat=False):
        for y in range(N):
            for x in range(N):
                if seg_dist(x + 0.5, y + 0.5, x0, y0, x1, y1) <= width / 2:
                    self.plot(x, y, m, flat)

    def rect(self, x0, y0, x1, y1, m, flat=False):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.plot(x, y, m, flat)

    def clear_ellipse(self, cx, cy, rx, ry):
        for y in range(N):
            for x in range(N):
                if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1:
                    self.mat[y][x] = None
                    self.extra[y][x] = None

    def clear_poly(self, pts):
        for y in range(N):
            for x in range(N):
                if inside(x + 0.5, y + 0.5, pts):
                    self.mat[y][x] = None
                    self.extra[y][x] = None

    def dot(self, x, y, color):
        """A single detail pixel in an exact color (drawn after shading)."""
        if 0 <= x < N and 0 <= y < N:
            self.extra[y][x] = color

    # --- output -------------------------------------------------------------------
    def render(self):
        """Returns rows of RGB tuples (None = transparent)."""
        px = [[None] * N for _ in range(N)]
        for y in range(N):
            for x in range(N):
                m = self.mat[y][x]
                if m is None:
                    continue
                dark, mid, light = self.ramps[m]
                if self.flat[y][x]:
                    px[y][x] = mid
                    continue

                def other(dx, dy):
                    xx, yy = x + dx, y + dy
                    return not (0 <= xx < N and 0 <= yy < N) or self.mat[yy][xx] != m
                if other(0, -1) or other(-1, 0):
                    px[y][x] = light
                elif other(0, 1) or other(1, 0):
                    px[y][x] = dark
                else:
                    px[y][x] = mid
        for y in range(N):
            for x in range(N):
                if self.extra[y][x] is not None:
                    px[y][x] = self.extra[y][x]
        out = [row[:] for row in px]
        for y in range(N):
            for x in range(N):
                if px[y][x] is None and any(0 <= x + dx < N and 0 <= y + dy < N and px[y + dy][x + dx] is not None
                                            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                    out[y][x] = self.outline
        return out


def inside(x, y, pts):
    c = False
    j = len(pts) - 1
    for i in range(len(pts)):
        xi, yi = pts[i]
        xj, yj = pts[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            c = not c
        j = i
    return c


def seg_dist(px, py, x0, y0, x1, y1):
    dx, dy = x1 - x0, y1 - y0
    t = max(0, min(1, ((px - x0) * dx + (py - y0) * dy) / (dx * dx + dy * dy or 1)))
    return math.hypot(px - (x0 + t * dx), py - (y0 + t * dy))


WHITE = (255, 255, 255)


# ---------------------------------------------------------------------------
# The badges
# ---------------------------------------------------------------------------

def vanilla():          # Normal: a vanilla soft-serve cone
    b = Badge()
    b.material("cone", (150, 92, 40), (206, 144, 70), (240, 196, 118))
    b.material("cream", (205, 182, 140), (243, 230, 190), (255, 252, 236))
    b.poly([(8, 16), (24, 16), (16, 31)], "cone")
    for i, y in enumerate(range(19, 30, 3)):          # waffle lines
        for x in range(9, 24):
            if b.mat[y][x] == "cone":
                b.dot(x, y, (150, 92, 40))
    for x0 in (11, 15, 19):
        for t in range(12):
            x, y = x0 + t // 3 - 2, 17 + t
            if 0 <= x < N and b.mat[y][x] == "cone" and t % 1 == 0:
                b.dot(x, y, (150, 92, 40))
    b.ellipse(16, 15, 11, 4.5, "cream")
    b.ellipse(16, 10, 8, 4, "cream")
    b.ellipse(16, 6, 5, 3.2, "cream")
    b.poly([(14, 4), (19, 4), (18, 0.5)], "cream")
    for x, y in ((9, 13), (12, 8), (14, 4), (20, 11)):  # swirl highlights
        b.dot(x, y, WHITE)
    return b


def obsidian():         # Rock: a faceted shard of volcanic glass
    b = Badge(outline=(12, 8, 18))
    b.material("a", (28, 18, 40), (52, 38, 72), (96, 80, 132))
    b.material("b", (40, 28, 58), (78, 62, 108), (140, 124, 182))
    b.material("c", (20, 14, 30), (36, 26, 52), (70, 56, 100))
    top, l, r, bl, br, mid = (16, 1), (4, 12), (28, 12), (9, 30), (23, 30), (15, 14)
    b.poly([top, l, mid], "b")
    b.poly([top, mid, r], "a")
    b.poly([l, mid, bl], "a")
    b.poly([mid, r, br], "c")
    b.poly([mid, bl, br], "c")
    b.line(15, 14, 15, 29, "b", 1.2)
    for x, y in ((13, 6), (12, 7), (11, 8), (8, 13), (20, 9), (21, 10)):
        b.dot(x, y, (220, 210, 255))
    b.dot(14, 5, WHITE)
    return b


def monsoon():          # Water: a storm cloud pouring rain
    b = Badge()
    b.material("cloud", (78, 88, 120), (132, 144, 178), (196, 206, 230))
    b.material("rain", (24, 72, 190), (58, 132, 238), (150, 206, 255))
    for x, y in ((9, 22), (16, 26), (23, 22)):           # raindrops
        b.ellipse(x, y + 3, 3, 3.4, "rain")
        b.poly([(x - 2.6, y + 2.4), (x + 2.6, y + 2.4), (x, y - 3)], "rain")
    b.ellipse(10, 12, 7, 6, "cloud")
    b.ellipse(19, 9, 8, 7.5, "cloud")
    b.ellipse(24, 13, 6, 5, "cloud")
    b.rect(6, 13, 27, 17, "cloud")
    b.ellipse(16, 16, 11, 3, "cloud")
    for x, y in ((8, 23), (15, 27), (22, 23)):
        b.dot(x, y, WHITE)
    return b


def amp():              # Electric: a lightning bolt on a dark coin
    b = Badge()
    b.material("coin", (34, 34, 64), (62, 64, 104), (104, 108, 152))
    b.material("rim", (150, 110, 10), (214, 170, 30), (255, 230, 110))
    b.material("bolt", (210, 150, 0), (252, 214, 40), (255, 250, 170))
    b.ellipse(16, 16, 15, 15, "rim")
    b.ellipse(16, 16, 12, 12, "coin")
    b.poly([(18, 4), (9, 18), (15, 18), (12, 28), (23, 13), (17, 13), (21, 4)], "bolt")
    return b


def weed():             # Grass: a dandelion gone to seed
    b = Badge()
    b.material("leaf", (40, 110, 40), (80, 170, 60), (150, 220, 100))
    b.material("stem", (60, 120, 50), (100, 160, 70), (150, 200, 110))
    b.material("puff", (190, 196, 206), (232, 236, 242), (255, 255, 255))
    b.line(16, 14, 16, 30, "stem", 2.2)
    b.poly([(15, 30), (3, 22), (7, 22), (5, 18), (10, 21), (9, 15), (15, 27)], "leaf")
    b.poly([(17, 30), (29, 21), (25, 21), (27, 17), (22, 20), (23, 14), (17, 27)], "leaf")
    b.ellipse(16, 10, 9, 9, "puff")
    for a in range(0, 360, 30):                         # seed spokes
        for r in (3, 5, 7):
            x = 16 + r * math.cos(math.radians(a))
            y = 10 + r * math.sin(math.radians(a))
            if r == 7:
                b.dot(int(x), int(y), (160, 166, 180))
    b.ellipse(16, 10, 2.2, 2.2, "puff", flat=True)
    for x, y in ((15, 9), (16, 10), (17, 9), (16, 9)):
        b.dot(x, y, (150, 156, 170))
    return b


def lava():             # Fire: an erupting volcano
    b = Badge()
    b.material("rock", (70, 40, 32), (112, 66, 48), (156, 100, 70))
    b.material("lava", (200, 40, 10), (250, 110, 20), (255, 210, 80))
    b.poly([(2, 30), (11, 13), (21, 13), (30, 30)], "rock")
    b.ellipse(16, 13, 5, 1.8, "lava", flat=True)
    b.poly([(12, 14), (20, 14), (21, 21), (18, 18), (16, 27), (14, 19), (11, 22)], "lava")
    b.ellipse(16, 7, 5, 5, "lava")
    b.ellipse(11, 4, 2.5, 2.5, "lava")
    b.ellipse(22, 3, 2, 2, "lava")
    b.ellipse(25, 8, 1.5, 1.5, "lava")
    b.dot(15, 5, WHITE)
    b.dot(16, 6, (255, 240, 170))
    return b


def espn():             # Psychic: a glowing pink eye
    b = Badge()
    b.material("lid", (190, 50, 130), (244, 112, 186), (255, 196, 228))
    b.material("white", (206, 196, 222), (244, 238, 250), (255, 255, 255))
    b.material("iris", (88, 30, 150), (140, 70, 210), (200, 150, 255))
    b.poly([(1, 16), (8, 8), (16, 6), (24, 8), (31, 16), (24, 24), (16, 26), (8, 24)], "lid")
    b.poly([(5, 16), (10, 11), (16, 10), (22, 11), (27, 16), (22, 21), (16, 22), (10, 21)], "white")
    b.ellipse(16, 16, 6, 6, "iris")
    b.ellipse(16, 16, 2.6, 2.6, "lid", flat=True)
    for (x, y) in ((16, 16), (17, 16), (16, 17), (17, 17)):
        b.dot(x, y, (40, 10, 60))
    b.dot(14, 13, WHITE)
    b.dot(13, 14, WHITE)
    for x, y in ((4, 4), (28, 4), (27, 28)):           # sparkles
        for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            b.plot(x + dx, y + dy, "lid", flat=True)
        b.dot(x, y, WHITE)
    return b


def venom():            # Poison: fangs dripping venom
    b = Badge()
    b.material("mouth", (70, 20, 80), (110, 40, 120), (150, 70, 160))
    b.material("fang", (180, 176, 190), (230, 228, 236), (255, 255, 255))
    b.material("venom", (110, 30, 160), (170, 70, 220), (220, 150, 255))
    b.ellipse(16, 8, 15, 8, "mouth")
    b.clear_ellipse(16, 15.5, 15, 3.5)
    b.poly([(6, 6), (12, 6), (9, 21)], "fang")
    b.poly([(20, 6), (26, 6), (23, 21)], "fang")
    b.ellipse(9, 26, 3, 3.5, "venom")
    b.poly([(6.6, 25), (11.4, 25), (9, 20)], "venom")
    b.ellipse(23, 27, 2.5, 3, "venom")
    b.poly([(21, 26), (25, 26), (23, 21)], "venom")
    b.ellipse(16, 24, 1.8, 2, "venom")
    b.dot(8, 25, WHITE)
    b.dot(22, 26, WHITE)
    return b


def shovel():           # Ground: a spade
    b = Badge()
    b.material("wood", (110, 66, 30), (160, 104, 50), (206, 150, 90))
    b.material("blade", (120, 84, 40), (176, 132, 70), (226, 190, 120))
    b.material("dirt", (80, 54, 30), (112, 78, 46), (140, 104, 66))
    b.line(16, 4, 16, 17, "wood", 3)
    b.rect(10, 1, 22, 3, "wood")
    b.clear_ellipse(16, 2.5, 3.2, 1.0)
    b.poly([(9, 16), (23, 16), (23, 24), (16, 31), (9, 24)], "blade")
    b.line(16, 18, 16, 28, "blade", 1, flat=True)
    for x, y in ((11, 23), (12, 25), (21, 22), (20, 25), (14, 27)):
        b.dot(x, y, (112, 78, 46))
    return b


def rocket():           # Flying: a rocket blasting off
    b = Badge()
    b.material("body", (180, 184, 200), (230, 232, 240), (255, 255, 255))
    b.material("red", (160, 20, 30), (220, 50, 50), (255, 130, 120))
    b.material("glass", (30, 90, 170), (70, 150, 230), (170, 220, 255))
    b.material("flame", (220, 80, 10), (255, 160, 30), (255, 240, 140))
    b.poly([(13, 26), (19, 26), (18, 30), (16, 31.5), (14, 30)], "flame")
    b.poly([(10, 26), (12, 18), (12, 26)], "red")
    b.poly([(22, 26), (20, 18), (20, 26)], "red")
    b.poly([(12, 9), (20, 9), (20, 26), (12, 26)], "body")
    b.ellipse(16, 9, 4, 8, "body")
    b.poly([(12, 9), (16, 0.5), (20, 9)], "red")
    b.ellipse(16, 14, 2.5, 2.5, "glass")
    b.rect(12, 21, 20, 22, "red", flat=True)
    return b


def infestation():      # Bug: a beetle, seen from above
    b = Badge()
    b.material("shell", (20, 100, 70), (40, 160, 100), (130, 220, 160))
    b.material("body", (40, 40, 50), (70, 70, 84), (110, 110, 130))
    for y, dx in ((12, 1), (18, 1), (24, 1)):           # legs
        b.line(9, y, 3, y + 3 * dx - 4, "body", 1.4)
        b.line(23, y, 29, y + 3 * dx - 4, "body", 1.4)
    b.line(14, 6, 9, 1, "body", 1.2)                    # antennae
    b.line(18, 6, 23, 1, "body", 1.2)
    b.ellipse(16, 8, 5, 4, "body")
    b.ellipse(16, 19, 9, 11, "shell")
    b.line(16, 9, 16, 30, "body", 1, flat=True)
    for x, y in ((12, 15), (20, 15), (11, 22), (21, 22)):
        b.ellipse(x, y, 1.5, 1.5, "body", flat=True)
    b.dot(12, 12, WHITE)
    return b


def death():            # Ghost: a skull
    b = Badge(outline=(30, 20, 44))
    b.material("bone", (150, 140, 170), (214, 208, 228), (250, 248, 255))
    b.material("hole", (40, 20, 60), (60, 34, 90), (90, 60, 130))
    b.ellipse(16, 13, 13, 12, "bone")
    b.rect(9, 20, 23, 28, "bone")
    b.ellipse(10.5, 15, 3.5, 4, "hole")
    b.ellipse(21.5, 15, 3.5, 4, "hole")
    b.poly([(16, 18), (14, 22), (18, 22)], "hole")
    for x in range(10, 23, 3):                          # teeth gaps
        b.line(x + 0.5, 25, x + 0.5, 28, "hole", 1, flat=True)
    b.dot(9, 14, (170, 120, 255))
    b.dot(20, 14, (170, 120, 255))
    return b


def knockout():         # Fighting: a boxing glove
    b = Badge()
    b.material("glove", (150, 20, 20), (220, 50, 40), (255, 140, 120))
    b.material("cuff", (190, 186, 196), (236, 234, 240), (255, 255, 255))
    b.ellipse(17, 12, 12, 11, "glove")
    b.ellipse(8, 15, 5, 6, "glove")                    # thumb
    b.line(7, 18, 13, 13, "glove", 2, flat=True)
    b.rect(9, 21, 25, 29, "cuff")
    b.rect(9, 21, 25, 22, "glove")
    for y in (24, 27):
        b.line(13, y, 21, y, "glove", 1, flat=True)
    b.dot(12, 5, WHITE)
    b.dot(11, 6, WHITE)
    return b


def wrench():           # Steel: an open-ended wrench
    b = Badge()
    b.material("steel", (110, 116, 130), (176, 182, 196), (236, 240, 248))
    b.line(8, 25, 20, 12, "steel", 4.5)
    b.ellipse(22, 10, 7, 7, "steel")
    # the jaw: a slot cut in from the top-right corner
    b.clear_poly([(21.5, 10.5), (25.5, 6.5), (31, 1), (31, 12), (25, 12)])
    b.clear_poly([(21.5, 10.5), (25.5, 6.5), (20, 1), (31, 1)])
    b.ellipse(7, 26, 5, 5, "steel")
    b.clear_ellipse(7, 26, 2.2, 2.2)
    for x, y in ((11, 20), (13, 18), (15, 16)):
        b.dot(x, y, WHITE)
    return b


def sub_zero():         # Ice: a snowflake
    b = Badge()
    b.material("ice", (60, 140, 200), (130, 200, 240), (220, 248, 255))
    for a in range(0, 360, 60):
        ca, sa = math.cos(math.radians(a)), math.sin(math.radians(a))
        b.line(16, 16, 16 + 14 * ca, 16 + 14 * sa, "ice", 2.2)
        for r, l in ((7, 4), (11, 3)):
            bx, by = 16 + r * ca, 16 + r * sa
            for s in (-1, 1):
                a2 = math.radians(a + s * 45)
                b.line(bx, by, bx + l * math.cos(a2), by + l * math.sin(a2), "ice", 1.6)
    b.ellipse(16, 16, 3.5, 3.5, "ice")
    b.dot(16, 16, WHITE)
    return b


def egg():              # Dragon: a dragon egg
    b = Badge(outline=(20, 16, 40))
    b.material("shell", (40, 50, 110), (70, 90, 170), (130, 160, 230))
    b.material("scale", (90, 40, 130), (140, 70, 180), (200, 140, 240))
    b.ellipse(16, 17, 11, 14, "shell")
    for row, y in enumerate(range(9, 30, 5)):           # overlapping scales
        for x in range(4 + (row % 2) * 3, 30, 6):
            for dx in range(-2, 3):
                for dy in range(0, 3):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < N and 0 <= yy < N and b.mat[yy][xx] == "shell" and abs(dx) + dy <= 2:
                        b.mat[yy][xx] = "scale"
    for x, y in ((12, 7), (11, 8), (20, 13), (14, 22)):
        b.dot(x, y, (255, 220, 120))
    b.dot(11, 6, WHITE)
    return b


def elder():            # Fairy: a knobbly wand with a sparkle
    b = Badge()
    b.material("wood", (90, 60, 50), (140, 100, 80), (190, 150, 120))
    b.material("spark", (220, 90, 160), (255, 160, 210), (255, 230, 245))
    b.line(5, 29, 22, 12, "wood", 2.6)
    for t in (0.2, 0.45, 0.7):                          # knots
        b.ellipse(5 + 17 * t, 29 - 17 * t, 2.2, 2.2, "wood")
    b.ellipse(5, 29, 2.8, 2.8, "wood")
    cx, cy = 24, 9
    pts = []
    for i in range(8):
        r = 8 if i % 2 == 0 else 3
        a = math.radians(i * 45 - 90)
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    b.poly(pts, "spark")
    for x, y in ((12, 6), (28, 22), (17, 21)):
        b.plot(x, y, "spark", flat=True)
        b.dot(x, y, WHITE)
    b.dot(24, 9, WHITE)
    return b


def evil():             # Dark: a grinning horned face
    b = Badge(outline=(10, 6, 14))
    b.material("dark", (26, 20, 36), (52, 42, 68), (90, 78, 110))
    b.material("horn", (110, 20, 30), (170, 40, 50), (230, 100, 100))
    b.material("glow", (200, 30, 30), (255, 70, 60), (255, 180, 150))
    b.poly([(5, 13), (2, 1), (11, 8)], "horn")
    b.poly([(27, 13), (30, 1), (21, 8)], "horn")
    b.ellipse(16, 17, 12, 12, "dark")
    b.poly([(8, 13), (14, 16), (9, 17)], "glow")
    b.poly([(24, 13), (18, 16), (23, 17)], "glow")
    b.poly([(8, 21), (24, 21), (16, 26)], "glow")
    for x in (11, 15, 19):                              # teeth
        b.poly([(x, 21), (x + 2, 21), (x + 1, 23)], "dark", flat=True)
    return b


# Order matches the TYPE_* names used in src/gym_badge.c
BADGES = {
    "NORMAL": ("Vanilla", vanilla), "ROCK": ("Obsidian", obsidian), "WATER": ("Monsoon", monsoon),
    "ELECTRIC": ("Amp", amp), "GRASS": ("Weed", weed), "FIRE": ("Lava", lava),
    "PSYCHIC": ("ESPN", espn), "POISON": ("Venom", venom), "GROUND": ("Shovel", shovel),
    "FLYING": ("Rocket", rocket), "BUG": ("Infestation", infestation), "GHOST": ("Death", death),
    "FIGHTING": ("Knockout", knockout), "STEEL": ("Wrench", wrench), "ICE": ("Sub-Zero", sub_zero),
    "DRAGON": ("Egg", egg), "FAIRY": ("Elder", elder), "DARK": ("Evil", evil),
}


def to_indexed(px):
    """Turns RGB pixels into (palette of 16, rows of indices). Colors snap to GBA 5-bit values."""
    def gba(c):
        return tuple((v >> 3) << 3 for v in c)
    colors = []
    rows = []
    for row in px:
        out = []
        for c in row:
            if c is None:
                out.append(0)
                continue
            c = gba(c)
            if c not in colors:
                colors.append(c)
            out.append(colors.index(c) + 1)
        rows.append(out)
    if len(colors) > 15:
        raise SystemExit(f"too many colors: {len(colors)}")
    pal = [(255, 0, 255)] + colors + [(0, 0, 0)] * (15 - len(colors))
    return pal, rows


def build():
    from PIL import Image
    OUT.mkdir(parents=True, exist_ok=True)
    images = {}
    for key, (name, fn) in BADGES.items():
        pal, rows = to_indexed(fn().render())
        img = Image.new("P", (N, N))
        img.putpalette([v for c in pal for v in c])
        img.putdata([i for row in rows for i in row])
        img.save(OUT / f"{key.lower()}.png", transparency=0)
        images[key] = img
    return images


def preview(images, out):
    from PIL import Image, ImageDraw
    cols = 6
    cell = 160
    sheet = Image.new("RGB", (cols * cell, (len(images) + cols - 1) // cols * (cell + 10)), (40, 44, 52))
    d = ImageDraw.Draw(sheet)
    for i, (key, img) in enumerate(images.items()):
        x, y = (i % cols) * cell, (i // cols) * (cell + 10)
        rgba = img.convert("RGBA")
        rgba.putdata([(0, 0, 0, 0) if i == 0 else c for i, c in zip(img.getdata(), rgba.getdata())])
        big = rgba.resize((N * 4, N * 4), Image.NEAREST)
        sheet.paste(big, (x + 16, y + 6), big)
        d.text((x + 16, y + 138), f"{BADGES[key][0]} ({key.title()})", fill=(255, 255, 255))
    sheet.save(out)


if __name__ == "__main__":
    imgs = build()
    if len(sys.argv) > 1:
        preview(imgs, sys.argv[1])
