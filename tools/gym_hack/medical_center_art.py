"""Pixel art for the Universal Pokémon Medical Center (Pokémon Canvas), on Vermilion City's building site.

  furniture   hospital beds, an IV stand, a heart monitor, a microscope on its bench, an X-ray light
              box, a red-cross sign, a medicine cabinet, a privacy curtain: one palette (ART), drawn
              over the floor (index 0 lets it show through). tools/gym_hack/build_medical_center.py
              cuts them into metatiles.
  slides      the 3F microscopes' slides, 64x64 each (graphics/medical_center/slides.png; src/medical_center.c)
  gurney      the 2F's runaway gurneys: a 16x16 object (OBJ_EVENT_GFX_GURNEY)
  exterior    the building itself, in Vermilion City (7 x 6 blocks, palette EXT)

    python3 tools/gym_hack/medical_center_art.py preview.png
"""
import math
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]

# ---------------------------------------------------------------------------------------------
# The furniture's palette
# ---------------------------------------------------------------------------------------------
ART = [
    (255, 0, 255),       # 0 transparent
    (40, 40, 48),        # 1 outline
    (248, 248, 248),     # 2 white
    (208, 216, 224),     # 3 light gray
    (152, 160, 176),     # 4 gray
    (96, 104, 120),      # 5 dark gray
    (176, 216, 248),     # 6 pale blue (sheets, glass)
    (104, 152, 216),     # 7 blue
    (56, 88, 160),       # 8 dark blue
    (232, 72, 72),       # 9 red
    (152, 40, 48),       # 10 dark red
    (248, 176, 192),     # 11 pink
    (88, 224, 120),      # 12 green (the monitor's trace)
    (32, 72, 56),        # 13 the screen, dark
    (232, 200, 96),      # 14 brass, yellow
    (168, 128, 72),      # 15 wood
]
O, W, LG, G, DG, PB, B, DB, R, DR, PK, GR, SCR, Y, WD = range(1, 16)


class Img:
    def __init__(self, w, h, fill=0):
        self.im = Image.new("P", (w, h), fill)
        self.px = self.im.load()
        self.w, self.h = w, h

    def set(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[x, y] = c

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, c)

    def box(self, x0, y0, x1, y1, fill, outline=O):
        self.rect(x0, y0, x1, y1, outline)
        self.rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, fill)

    def hline(self, x0, x1, y, c):
        self.rect(x0, y, x1, y, c)

    def vline(self, x, y0, y1, c):
        self.rect(x, y0, x, y1, c)


# ---------------------------------------------------------------------------------------------
# Furniture (16-pixel blocks)
# ---------------------------------------------------------------------------------------------
def bed():
    """A hospital bed, head at the top: 1 x 2 blocks (a patient Pokémon stands on it as an object)"""
    c = Img(16, 32)
    c.box(1, 1, 14, 6, LG)                      # the headboard
    c.hline(2, 13, 2, W)
    c.box(1, 6, 14, 27, W)                      # the mattress
    c.box(3, 7, 12, 11, W)                      # the pillow
    c.hline(4, 11, 8, W)
    c.rect(4, 10, 11, 10, LG)
    c.rect(2, 13, 13, 26, PB)                   # the sheet
    c.hline(2, 13, 13, B)
    c.rect(2, 14, 13, 14, W)                    # its turned-down edge
    for y in range(17, 26, 3):
        c.hline(3, 12, y, 6 if y % 2 else PB)
    c.vline(1, 8, 26, G)                        # the rails
    c.vline(14, 8, 26, G)
    c.box(1, 26, 14, 29, G)                     # the footboard
    c.hline(2, 13, 27, LG)
    c.rect(1, 30, 2, 31, DG)                    # the wheels
    c.rect(13, 30, 14, 31, DG)
    return c


def iv_stand():
    """1 x 2: an IV stand, its bag and line"""
    c = Img(16, 32)
    c.hline(4, 11, 2, DG)                       # the hooks' bar
    c.vline(8, 2, 28, G)                        # the pole
    c.vline(9, 3, 28, DG)
    c.box(4, 4, 9, 13, PB)                      # the bag
    c.rect(5, 9, 8, 12, 6)
    c.set(6, 6, W)
    c.vline(6, 14, 20, LG)                      # the line
    c.box(3, 27, 13, 29, G)                     # the base
    c.set(3, 30, O)
    c.set(13, 30, O)
    c.set(8, 30, O)
    return c


def monitor():
    """1 x 2: a heart monitor on a stand"""
    c = Img(16, 32)
    c.box(1, 2, 14, 14, DG)                     # the case
    c.box(3, 4, 12, 11, SCR)                    # the screen
    trace = [8, 8, 8, 8, 6, 9, 5, 8, 8, 8]      # (the beat)
    for i, y in enumerate(trace):
        c.set(3 + i, y, GR)
    c.set(13, 6, R)                             # a light
    c.hline(4, 11, 13, G)
    c.vline(7, 15, 28, G)                       # the stand
    c.vline(8, 15, 28, DG)
    c.box(3, 27, 12, 29, G)
    return c


def microscope():
    """1 x 2: a microscope on the bench: the bench is 1 x 1 below it, the scope on top (1 x 1)"""
    c = Img(16, 32)
    # the scope (top block, sitting on the bench's back edge)
    c.box(6, 1, 9, 4, DG)                       # the eyepiece
    c.box(5, 4, 10, 9, G)                       # the arm
    c.rect(6, 5, 9, 8, LG)
    c.box(8, 9, 11, 13, DG)                     # the objective
    c.box(3, 12, 12, 14, G)                     # the stage
    c.rect(6, 12, 9, 12, PB)                    # (the slide)
    # the bench
    c.box(0, 14, 15, 18, WD)
    c.hline(1, 14, 15, Y)
    c.box(1, 18, 14, 30, LG)                    # its cupboard
    c.vline(7, 19, 29, G)
    c.set(5, 24, DG)
    c.set(9, 24, DG)
    return c


def xray():
    """2 x 2 on the wall: an X-ray light box with a Pokémon's bones on it"""
    c = Img(32, 32)
    c.box(2, 3, 29, 26, LG)
    c.box(4, 5, 27, 24, 6)
    c.rect(5, 6, 26, 23, PB)
    # the bones: a little skeleton (a ribcage, a spine, a skull)
    c.box(13, 7, 18, 11, W)                     # the skull
    c.set(14, 9, DG)
    c.set(17, 9, DG)
    c.vline(15, 12, 21, W)                      # the spine
    c.vline(16, 12, 21, W)
    for i, y in enumerate(range(13, 20, 2)):
        c.hline(10 + i // 2, 14, y, W)
        c.hline(17, 21 - i // 2, y, W)
    c.hline(11, 20, 21, W)                      # the hips
    c.vline(11, 22, 23, W)
    c.vline(20, 22, 23, W)
    c.box(2, 26, 29, 28, G)                     # the frame's ledge
    return c


def cross_sign():
    """2 x 1 on the wall: a red cross on white"""
    c = Img(32, 16)
    c.box(6, 1, 25, 14, W)
    c.rect(13, 3, 18, 12, R)
    c.rect(9, 6, 22, 9, R)
    c.vline(19, 3, 12, DR)
    c.hline(10, 22, 10, DR)
    return c


def cabinet():
    """2 x 2 against the wall: a glass medicine cabinet"""
    c = Img(32, 32)
    c.box(1, 0, 30, 30, LG)
    c.box(3, 2, 14, 18, PB)                     # the glass doors
    c.box(17, 2, 28, 18, PB)
    for x0 in (4, 18):                          # the shelves of bottles
        c.hline(x0, x0 + 9, 9, G)
        c.hline(x0, x0 + 9, 17, G)
        for k, col in enumerate((R, GR, Y, B, PK)):
            x = x0 + k * 2
            c.rect(x, 6, x, 8, col)
            c.rect(x, 14 - (k % 2), x, 16, (R, B, GR, Y, PK)[k])
    c.vline(15, 2, 18, O)
    c.box(3, 20, 28, 29, W)                     # the drawers
    c.hline(4, 27, 24, LG)
    c.hline(14, 17, 22, DG)
    c.hline(14, 17, 27, DG)
    c.hline(1, 30, 31, O)
    return c


def curtain():
    """1 x 2: a privacy curtain on its rail"""
    c = Img(16, 32)
    c.hline(0, 15, 1, DG)
    for x in range(0, 16):
        shade = PB if (x // 3) % 2 == 0 else 6
        c.vline(x, 2, 29, shade)
        if (x // 3) % 2 and x % 3 == 2:
            c.vline(x, 2, 29, B)
    c.hline(0, 15, 30, B)
    return c


def counter_sign():
    """2 x 1 on the wall: RECEPTION, as a little blue plaque (a cross and lines: no letters)"""
    c = Img(32, 16)
    c.box(2, 3, 29, 12, B)
    c.rect(5, 5, 8, 10, W)
    c.rect(6, 4, 7, 11, W)
    c.rect(4, 6, 9, 9, W)
    c.rect(6, 5, 7, 10, R)
    c.rect(5, 7, 8, 8, R)
    c.hline(12, 26, 6, W)
    c.hline(12, 22, 9, PB)
    return c


def partition():
    """1 x 1: a ward partition panel (pale blue, on little feet)"""
    c = Img(16, 16)
    c.box(1, 0, 14, 13, PB)
    c.rect(2, 1, 13, 3, 6)
    c.hline(2, 13, 7, 6)
    c.vline(1, 13, 15, G)
    c.vline(14, 13, 15, G)
    return c


def wheelchair():
    """1 x 1: a wheelchair"""
    c = Img(16, 16)
    c.box(4, 1, 11, 8, B)                       # the back
    c.box(3, 7, 12, 10, DB)                     # the seat
    for cx in (3, 12):                          # the big wheels
        for y in range(6, 16):
            for x in range(cx - 3, cx + 4):
                d = (x - cx) ** 2 + (y - 11) ** 2
                if 8 <= d <= 14:
                    c.set(x, y, DG)
    c.set(3, 11, LG)
    c.set(12, 11, LG)
    return c


def cart():
    """1 x 1: a supply cart"""
    c = Img(16, 16)
    c.box(1, 2, 14, 12, LG)
    c.hline(2, 13, 6, G)
    c.rect(3, 3, 5, 5, R)                       # boxes of supplies on it
    c.rect(7, 3, 10, 5, W)
    c.rect(11, 4, 12, 5, GR)
    c.rect(3, 8, 6, 10, PB)
    c.rect(8, 8, 12, 10, W)
    c.rect(2, 13, 3, 15, DG)
    c.rect(12, 13, 13, 15, DG)
    return c


def boxes():
    """1 x 1: a stack of supply boxes"""
    c = Img(16, 16)
    c.box(1, 6, 14, 15, WD)
    c.hline(2, 13, 10, Y)
    c.box(4, 1, 12, 7, Y)
    c.hline(5, 11, 4, WD)
    c.rect(7, 8, 8, 12, R)                      # a red cross on the front
    c.rect(6, 9, 9, 10, R)
    return c


PIECES = {
    "bed": bed, "iv": iv_stand, "monitor": monitor, "microscope": microscope, "xray": xray,
    "cross": cross_sign, "cabinet": cabinet, "curtain": curtain, "plaque": counter_sign,
    "partition": partition, "wheelchair": wheelchair, "cart": cart, "boxes": boxes,
}


# ---------------------------------------------------------------------------------------------
# The microscope slides (64 x 64 each, one palette)
# ---------------------------------------------------------------------------------------------
SLIDE_PAL = [
    (255, 0, 255),       # 0 transparent
    (24, 24, 32),        # 1 the dark around the view
    (248, 248, 232),     # 2 the light
    (224, 228, 208),     # 3 the light, dimmer
    (112, 176, 72),      # 4 green
    (56, 112, 48),       # 5 green, dark
    (176, 224, 120),     # 6 green, light
    (88, 136, 224),      # 7 blue
    (40, 72, 152),       # 8 blue, dark
    (224, 112, 192),     # 9 pink-purple
    (144, 56, 136),      # 10 pink-purple, dark
    (248, 184, 232),     # 11 pink, light
    (160, 112, 200),     # 12 purple
    (96, 56, 136),       # 13 purple, dark
    (232, 152, 72),      # 14 orange
    (168, 168, 160),     # 15 gray
]
S_DARK, S_LIGHT, S_DIM, S_G, S_GD, S_GL, S_B, S_BD, S_P, S_PD, S_PL, S_V, S_VD, S_OR, S_GRAY = range(1, 16)
SLIDE_NAMES = ["green cells", "blue rods", "Pokérus", "a big star", "purple blobs"]


def _rand(i, k):
    v = (i * 374761393 + k * 668265263) & 0xFFFFFFFF
    v = ((v ^ (v >> 13)) * 1274126177) & 0xFFFFFFFF
    return (v ^ (v >> 16)) & 0xFFFF


def _view():
    c = Img(64, 64, 0)
    for y in range(64):
        for x in range(64):
            d = math.hypot(x - 31.5, y - 31.5)
            if d < 29:
                c.set(x, y, S_DIM if d > 26 else S_LIGHT)
            elif d < 32:
                c.set(x, y, S_DARK)
    return c


def _inside(x, y, r=25):
    return math.hypot(x - 31.5, y - 31.5) < r


def _blob(c, cx, cy, rx, ry, fill, edge, inner=None):
    for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
        for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
            if not _inside(x, y, 27):
                continue
            d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
            if d <= 1:
                c.set(x, y, edge if d > 0.6 else fill)
    if inner:
        c.set(int(cx), int(cy), inner)
        c.set(int(cx) + 1, int(cy), inner)


def _star(c, cx, cy, size, fill, edge):
    """a little 4- or 5-pointed star"""
    if size == 1:                                     # (tiny: a plus with a dark heart)
        for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            c.set(cx + dx, cy + dy, fill)
        c.set(cx, cy, edge)
        return
    for y in range(cy - size * 3, cy + size * 3 + 1):
        for x in range(cx - size * 3, cx + size * 3 + 1):
            a = math.atan2(y - cy, x - cx)
            r = math.hypot(x - cx, y - cy)
            lim = size * (1.6 + 1.4 * abs(math.cos(2.5 * a)))
            if r <= lim and _inside(x, y, 27):
                c.set(x, y, edge if r > lim - 1.2 else fill)


def slides():
    out = []
    # 0: green plant cells (big, each with a nucleus)
    c = _view()
    for i in range(9):
        cx, cy = 14 + (i % 3) * 17 + _rand(i, 1) % 4, 14 + (i // 3) * 17 + _rand(i, 2) % 4
        _blob(c, cx, cy, 7, 6, S_GL, S_G)
        _blob(c, cx + 1, cy, 2, 2, S_GD, S_GD)
    out.append(c)
    # 1: blue rods
    c = _view()
    for i in range(16):
        cx, cy = 10 + _rand(i, 3) % 44, 10 + _rand(i, 4) % 44
        vertical = _rand(i, 5) % 2
        for k in range(-4, 5):
            x, y = (cx, cy + k) if vertical else (cx + k, cy)
            for w in (0, 1):
                xx, yy = (x + w, y) if vertical else (x, y + w)
                if _inside(xx, yy, 27):
                    c.set(xx, yy, S_BD if abs(k) == 4 else S_B)
    out.append(c)
    # 2: Pokérus: tiny pink-purple stars, in little clusters
    c = _view()
    for k in range(7):
        gx, gy = 14 + _rand(k, 6) % 36, 14 + _rand(k, 7) % 36
        for i in range(5):
            x, y = gx + _rand(k * 9 + i, 8) % 9 - 4, gy + _rand(k * 9 + i, 9) % 9 - 4
            if _inside(x, y, 25):
                _star(c, x, y, 1, S_P, S_PD)
    out.append(c)
    # 3: one big pink star (a Staryu's cell, the doctor says)
    c = _view()
    _star(c, 32, 32, 6, S_PL, S_P)
    _blob(c, 32, 32, 2, 2, S_PD, S_PD)
    out.append(c)
    # 4: purple blobs (a Grimer's)
    c = _view()
    for i in range(6):
        cx, cy = 13 + _rand(i, 10) % 38, 13 + _rand(i, 11) % 38
        _blob(c, cx, cy, 6 + _rand(i, 12) % 3, 5, S_V, S_VD)
        c.set(cx - 2, cy - 2, S_PL)
    out.append(c)
    sheet = Image.new("P", (64, 64 * len(out)), 0)
    for i, s in enumerate(out):
        sheet.paste(s.im, (0, 64 * i))
    sheet.putpalette([v for col in SLIDE_PAL for v in col])
    return sheet


# ---------------------------------------------------------------------------------------------
# The gurney (an object, 16 x 16)
# ---------------------------------------------------------------------------------------------
GURNEY_PAL = [(255, 0, 255), (40, 40, 48), (248, 248, 248), (208, 216, 224), (152, 160, 176),
              (96, 104, 120), (176, 216, 248), (104, 152, 216)] + [(0, 0, 0)] * 8


def gurney():
    c = Img(16, 16)
    c.box(1, 3, 14, 11, W)              # the mattress (seen from the front-top)
    c.box(2, 4, 6, 7, W)                # the pillow
    c.rect(7, 4, 13, 10, 6)             # the sheet
    c.hline(7, 13, 4, 7)
    c.rect(2, 8, 6, 10, 6)
    c.hline(1, 14, 11, 4)               # the frame
    c.box(1, 11, 14, 13, 4)
    c.vline(2, 13, 14, 5)               # the legs
    c.vline(13, 13, 14, 5)
    c.rect(1, 14, 3, 15, 1)             # the wheels
    c.rect(12, 14, 14, 15, 1)
    im = c.im
    im.putpalette([v for col in GURNEY_PAL for v in col])
    return im


# ---------------------------------------------------------------------------------------------
# The building, in Vermilion City (7 x 6 blocks), its own palette (the city's tileset's slot 11)
# ---------------------------------------------------------------------------------------------
EXT = [
    (255, 0, 255),       # 0 (transparent: Vermilion's grass)
    (41, 41, 57),        # 1 outline
    (255, 255, 255),     # 2 white, highlights
    (222, 230, 238),     # 3 wall
    (189, 197, 213),     # 4 wall, shade
    (131, 139, 156),     # 5 wall, dark (under the eaves; the plinth)
    (115, 205, 197),     # 6 teal
    (57, 148, 148),      # 7 teal, dark
    (180, 230, 255),     # 8 window
    (74, 139, 205),      # 9 window, dark
    (230, 65, 65),       # 10 red
    (156, 32, 41),       # 11 red, dark
    (189, 197, 172),     # 12 roof
    (139, 148, 123),     # 13 roof, dark
    (115, 197, 82),      # 14 the planters' leaves
    (57, 123, 49),       # 15 leaves, dark
]
E_O, E_W, E_WALL, E_WS, E_WD, E_T, E_TD, E_WIN, E_WIND, E_R, E_RD, E_ROOF, E_ROOFD, E_LEAF, E_LEAFD = range(1, 16)
EXT_W, EXT_H = 7, 6
EXT_DOOR = (3, 5)                        # (the door: dx, dy in the building)


def exterior():
    """drawn the way FireRed draws its buildings: a flat roof with ridges and a lit top edge, an
    overhang that shades the wall under it, the wall lit from the left and shaded on the right, framed
    windows with sills, a plinth, planters by the door"""
    W_, H_ = EXT_W * 16, EXT_H * 16
    c = Img(W_, H_, 0)
    L, R = 2, W_ - 3                      # (the walls' outer edges)
    # ---- the roof: rounded top corners, ridges, a light top edge, a dark front lip ----
    c.rect(L + 1, 1, R - 1, 17, E_ROOF)
    for x in range(L + 2, R - 1):
        if (x - L) % 4 == 0:
            c.vline(x, 3, 15, E_ROOFD)
    c.rect(R - 4, 3, R - 1, 15, E_ROOFD)  # (the roof's right side, in shade)
    c.vline(L + 1, 3, 15, E_W)            # (its left, lit)
    c.hline(L + 2, R - 2, 2, E_W)         # (the lit edge)
    c.hline(L + 1, R - 1, 16, E_ROOFD)
    c.hline(L + 2, R - 2, 0, E_O)
    c.set(L + 1, 1, E_O); c.set(R - 1, 1, E_O)
    c.vline(L, 2, 18, E_O); c.vline(R, 2, 18, E_O)
    # the parapet (teal), lit on top, shadowed under
    c.rect(L, 17, R, 21, E_T)
    c.hline(L + 1, R - 1, 18, E_W)
    c.hline(L, R, 21, E_TD)
    c.hline(L - 1, R + 1, 22, E_O)
    c.vline(L - 1, 17, 22, E_O); c.vline(R + 1, 17, 22, E_O)
    # ---- the walls ----
    wl, wr = L + 2, R - 2
    c.rect(wl, 23, wr, H_ - 1, E_WALL)
    c.vline(wl - 1, 23, H_ - 1, E_O); c.vline(wr + 1, 23, H_ - 1, E_O)
    c.vline(wl, 23, H_ - 2, E_W)                       # (lit from the left)
    c.rect(wr - 5, 23, wr, H_ - 2, E_WS)               # (shaded on the right)
    c.vline(wr, 23, H_ - 2, E_WD)
    c.rect(wl, 23, wr, 25, E_WS)                       # (the overhang's shadow)
    c.hline(wl, wr, 23, E_WD)
    for y in range(29, H_ - 6, 6):                     # (panel lines, faint)
        for x in range(wl + 1, wr - 3, 2):
            if (x // 2 + y // 6) % 3 == 0:
                c.set(x, y, E_WS)
    # ---- windows: a frame lit top-left, the glass with a glint, a sill and its shadow ----
    def window(x, y, w, h):
        c.box(x, y, x + w, y + h, E_WIN)
        c.rect(x + 1, y + h // 2 + 1, x + w - 1, y + h - 1, E_WIND)
        c.line_glint = None
        for k in range(min(w, h) // 2):                 # (the glint)
            c.set(x + 2 + k, y + h // 2 - k, E_W)
        c.hline(x - 1, x + w + 1, y + h + 1, E_W)       # (the sill)
        c.hline(x, x + w + 1, y + h + 2, E_WS)
    for row, y in enumerate((28, 46)):
        for k in range(6):
            x = 12 + k * 15
            if row == 0 and 2 <= k <= 3:
                continue
            window(x, y, 10, 10)
    # ---- the sign: a white disc with a red cross, lit top-left, shaded bottom-right ----
    cx, cy = W_ // 2, 34
    for y in range(cy - 11, cy + 12):
        for x in range(cx - 13, cx + 14):
            d = math.hypot(x - cx + 0.5, (y - cy + 0.5) * 1.15)
            if d < 12.5:
                shade = (x - cx) + (y - cy) > 9
                c.set(x, y, E_O if d > 11.5 else (E_WS if shade else E_W))
    c.rect(cx - 3, cy - 8, cx + 2, cy + 7, E_R)
    c.rect(cx - 8, cy - 3, cx + 7, cy + 2, E_R)
    c.vline(cx + 2, cy - 8, cy + 7, E_RD)
    c.hline(cx - 8, cx + 7, cy + 2, E_RD)
    c.hline(cx - 2, cx + 1, cy - 7, E_W)
    # ---- the band between the 2F and the 1F ----
    c.rect(wl, 60, wr, 63, E_T)
    c.hline(wl, wr, 60, E_W)
    c.hline(wl, wr, 63, E_TD)
    c.hline(wl, wr, 64, E_WS)
    # ---- the 1F: windows either side of the door ----
    for x in (12, 27, 72, 87):
        window(x, 68, 11, 13)
    # ---- the door: an awning (striped, its shadow under), glass doors with a glint ----
    dx = EXT_DOOR[0] * 16
    c.rect(dx - 6, 63, dx + 21, 68, E_R)
    for x in range(dx - 6, dx + 22, 4):
        c.vline(x, 64, 68, E_W)
    c.hline(dx - 6, dx + 21, 63, E_W)
    c.hline(dx - 7, dx + 22, 69, E_O)
    c.vline(dx - 7, 63, 69, E_O); c.vline(dx + 22, 63, 69, E_O)
    c.rect(dx - 1, 70, dx + 16, H_ - 1, E_WD)
    c.hline(dx - 1, dx + 16, 70, E_O)
    c.box(dx + 1, 72, dx + 7, H_ - 1, E_WIN)
    c.box(dx + 8, 72, dx + 14, H_ - 1, E_WIN)
    c.rect(dx + 2, 85, dx + 6, H_ - 2, E_WIND)
    c.rect(dx + 9, 85, dx + 13, H_ - 2, E_WIND)
    for k in range(4):
        c.set(dx + 3 + k, 80 - k, E_W)
        c.set(dx + 10 + k, 80 - k, E_W)
    c.rect(dx + 6, 73, dx + 9, 74, E_W)                # (the light over the door)
    # ---- the plinth along the bottom ----
    c.rect(wl, H_ - 3, dx - 2, H_ - 1, E_WD)
    c.rect(dx + 17, H_ - 3, wr, H_ - 1, E_WD)
    c.hline(wl, dx - 2, H_ - 3, E_WS)
    c.hline(dx + 17, wr, H_ - 3, E_WS)
    # ---- planters by the door (like the town's houses) ----
    for px in (dx - 15, dx + 19):
        c.box(px, H_ - 7, px + 10, H_ - 1, 15 if False else E_RD)
        c.rect(px + 1, H_ - 6, px + 9, H_ - 2, E_R)
        for k in range(5):
            c.set(px + 1 + 2 * k, H_ - 9, E_LEAF)
            c.rect(px + 1 + 2 * k, H_ - 8, px + 2 + 2 * k, H_ - 7, E_LEAF if k % 2 else E_LEAFD)
    im = c.im
    im.putpalette([v for col in EXT for v in col])
    return im


def write_slides():
    out = ROOT / "graphics/medical_center/slides.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    slides().save(out)
    g = ROOT / "graphics/object_events/pics/misc/gurney.png"
    gurney().save(g)


def preview(path):
    pal = [v for col in ART for v in col]
    pieces = [f() for f in PIECES.values()]
    w = sum(p.w for p in pieces) + 8 * len(pieces)
    out = Image.new("RGB", (max(w, 64 * 5 + 40, EXT_W * 16 + 20), 32 + 8 + 64 + 8 + EXT_H * 16), (220, 220, 210))
    x = 0
    for p in pieces:
        for yy in range(p.h):
            for xx in range(p.w):
                v = p.px[xx, yy]
                if v:
                    out.putpixel((x + xx, yy), ART[v])
        x += p.w + 8
    s = slides().convert("RGB")
    for i in range(5):
        out.paste(s.crop((0, 64 * i, 64, 64 * i + 64)), (i * 72, 40))
    g = gurney().convert("RGB")
    out.paste(g, (5 * 72, 40))
    ex = exterior()
    for yy in range(ex.height):
        for xx in range(ex.width):
            v = ex.getpixel((xx, yy))
            out.putpixel((xx, 40 + 72 + yy), EXT[v] if v else (112, 200, 120))
    out.resize((out.width * 3, out.height * 3), Image.NEAREST).save(path)


if __name__ == "__main__":
    write_slides()
    if len(sys.argv) > 1:
        preview(sys.argv[1])
