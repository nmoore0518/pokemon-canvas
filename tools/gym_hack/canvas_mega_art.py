#!/usr/bin/env python3
"""Mega Smeargle (Pokémon Canvas; tools/gym_hack/build_canvas_smeargle.py uses this).

Its sprites (graphics/pokemon/smeargle/canvas/mega_anim_front.png, mega_back.png) are hand art,
converted from Nick's reference (a big flat brush for a tail, purple, green and orange; a painter's
palette round its neck; a moustache) by tools/gym_hack/art/mega_smeargle_from_ref.py. Their palette
is MEGA_PALETTE; the shiny one has Smeargle's shiny yellow fur. One look for every type.
This draws its icon (in icon palette 1) and holds its palettes.
"""
import math
import random

from PIL import Image

MEGA_PALETTE = [(0, 176, 232), (16, 16, 16), (248, 248, 248),
                (240, 224, 200), (216, 194, 168), (184, 160, 136),      # 3-5 fur: light, mid, shadow
                (160, 96, 56), (112, 64, 40),                           # 6-7 brown
                (128, 80, 136), (88, 52, 96),                           # 8-9 purple
                (128, 168, 60), (92, 128, 44),                          # 10-11 green
                (232, 144, 32), (176, 100, 24),                         # 12-13 orange
                (224, 112, 128),                                        # 14 tongue
                (200, 140, 96)]                                         # 15 brown highlight
MEGA_SHINY_PALETTE = list(MEGA_PALETTE)
MEGA_SHINY_PALETTE[3:6] = [(248, 240, 160), (224, 208, 120), (184, 168, 88)]
MEGA_SHINY_PALETTE[6:8] = [(144, 120, 72), (104, 80, 40)]
MEGA_SHINY_PALETTE[15] = (176, 152, 96)
OUTLINE = 6


def _shift(img, dx, dy=0):
    w, h = img.size
    out = Image.new("P", (w, h))
    out.paste(img, (dx, dy))
    return out


def _outline(o, filled, w, h, colour=OUTLINE):
    for (x, y) in list(filled):
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            q = (x + dx, y + dy)
            if 0 <= q[0] < w and 0 <= q[1] < h and q not in filled and o[q] == 0:
                o[q] = colour
def icon(base_frame):
    """(32x32, icon palette 1: its brush in lilac, green and red-orange, the nearest it has)"""
    img = base_frame.copy()
    p = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            if p[x, y] in (4, 5) or (x < 9 and y < 20):
                p[x, y] = 0
    img = _shift(img, 3)
    o = img.load()
    rnd = random.Random(1)
    ang = math.radians(-115)
    ux, uy = math.cos(ang), math.sin(ang)
    vx, vy = -uy, ux
    hx, hy, length = 11, 21, 18
    bands = [8, 5, 13]     # (palette 1: lilac, green, red-orange)
    px = {}
    for y in range(h):
        for x in range(w):
            rx, ry = x + 0.5 - hx, y + 0.5 - hy
            a, b = rx * ux + ry * uy, rx * vx + ry * vy
            if a < 0 or a > length - rnd.choice((0, 0, 1)):
                continue
            if a < 3:
                if abs(b) <= 2.5:
                    px[(x, y)] = 14
                continue
            half = 3 + 3.5 * min(1, (a - 3) / 8)
            if abs(b) > half:
                continue
            s = (b + half) / (2 * half)
            px[(x, y)] = 2 if a < 7 else bands[min(int(s * 3), 2)]
    for q, v in px.items():
        if o[q] == 0:
            o[q] = v
    _outline(o, set(px), w, h, 15)
    return img
