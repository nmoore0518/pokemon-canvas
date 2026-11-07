#!/usr/bin/env python3
"""Pixel art for the Pallet Town Gym's opening ceremony (Pokémon Canvas): a bunch of balloons tied
down and the LEAGUE official's podium, 16x32 objects with one palette between them
(OBJ_EVENT_GFX_CEREMONY_BALLOONS, OBJ_EVENT_GFX_CEREMONY_PODIUM; data/scripts/gym_finale.inc).

    python3 tools/gym_hack/ceremony_art.py [preview.png]

Writes graphics/object_events/pics/misc/ceremony_balloons.png and ceremony_podium.png.
"""
import math
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "graphics/object_events/pics/misc"

PAL = [
    (255, 0, 255),      # 0 transparent
    (40, 40, 48),       # 1 outline
    (248, 248, 248),    # 2 white
    (232, 64, 64),      # 3 red
    (152, 32, 40),      # 4 dark red
    (88, 136, 232),     # 5 blue
    (40, 64, 152),      # 6 dark blue
    (248, 216, 72),     # 7 yellow
    (192, 144, 32),     # 8 dark yellow
    (168, 168, 176),    # 9 string
    (208, 160, 104),    # 10 light wood
    (160, 112, 64),     # 11 wood
    (104, 68, 40),      # 12 dark wood
    (232, 200, 96),     # 13 gold
    (248, 168, 200),    # 14 pink
    (88, 184, 96),      # 15 green
]


def new():
    return [[0] * 16 for _ in range(32)]


def save(px, name):
    im = Image.new("P", (16, 32))
    im.putpalette([v for c in PAL for v in c])
    for y in range(32):
        for x in range(16):
            im.putpixel((x, y), px[y][x])
    im.save(OUT / name)
    return im


def line(px, x0, y0, x1, y1, c):
    n = max(abs(x1 - x0), abs(y1 - y0), 1)
    for i in range(n + 1):
        x = round(x0 + (x1 - x0) * i / n)
        y = round(y0 + (y1 - y0) * i / n)
        if px[y][x] == 0:
            px[y][x] = c


def balloon(px, cx, cy, fill, dark):
    rx, ry = 3.6, 4.6
    for y in range(32):
        for x in range(16):
            d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
            if d <= 1.0:
                edge = ((x - cx) / (rx - 1)) ** 2 + ((y - cy) / (ry - 1)) ** 2 > 1.0
                shade = (x - cx) + (y - cy) * 0.6 > 2.2
                px[y][x] = 1 if edge else dark if shade else fill
    px[round(cy - 2)][round(cx - 1.5)] = 2                 # (its shine)
    px[round(cy - 1)][round(cx - 1.5)] = 2
    kx, ky = round(cx), round(cy + ry)
    px[ky][kx] = dark                                       # (the knot)
    return kx, ky + 1


def balloons():
    px = new()
    knots = []
    for cx, cy, fill, dark in ((11.0, 7.0, 5, 6), (4.5, 5.5, 3, 4), (8.0, 12.5, 7, 8)):
        knots.append(balloon(px, cx, cy, fill, dark))
    for kx, ky in knots:                                    # (strings down to the weight)
        line(px, kx, ky, 8, 28, 9)
    for y in range(28, 32):
        for x in range(6, 11):
            px[y][x] = 1 if y in (28, 31) or x in (6, 10) else 12
    return save(px, "ceremony_balloons.png")


def podium():
    px = new()
    # the slanted top, then the stand, a LEAGUE ball on its front
    for x in range(1, 15):
        px[12][x] = 1
        px[13][x] = 10
        px[14][x] = 10 if 2 <= x <= 13 else 1
        px[15][x] = 1
    px[13][1] = px[13][14] = 1
    px[14][1] = px[14][14] = 1
    for y in range(16, 31):
        for x in range(2, 14):
            px[y][x] = 1 if x in (2, 13) else 12 if x in (3, 12) else 11
    for x in range(2, 14):
        px[30][x] = 1
        px[31][x] = 12 if 1 <= x <= 14 else 0
    px[31][1] = px[31][14] = 1
    cx, cy, r = 7.5, 22.5, 3.6
    for y in range(17, 29):
        for x in range(3, 13):
            d = math.hypot(x - cx, y - cy)
            if d <= r:
                if d > r - 1:
                    c = 1
                elif abs(y - cy) < 0.6:
                    c = 1
                else:
                    c = 3 if y < cy else 2
                if math.hypot(x - cx, y - cy) < 1.2:
                    c = 2 if math.hypot(x - cx, y - cy) < 0.8 else 1
                px[y][x] = c
    for x in range(4, 12):                                  # (a gold trim)
        px[17][x] = 13
    return save(px, "ceremony_podium.png")


if __name__ == "__main__":
    a, b = balloons(), podium()
    if len(sys.argv) > 1:
        sheet = Image.new("RGB", (32, 32), (120, 200, 120))
        for i, im in enumerate((a, b)):
            rgb = im.convert("RGBA")
            for y in range(32):
                for x in range(16):
                    if im.getpixel((x, y)) != 0:
                        sheet.putpixel((i * 16 + x, y), rgb.getpixel((x, y))[:3])
        sheet.resize((256, 256), Image.NEAREST).save(sys.argv[1])
