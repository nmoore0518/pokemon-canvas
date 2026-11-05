#!/usr/bin/env python3
"""Mega Smeargle's art (Pokémon Canvas; tools/gym_hack/build_canvas_smeargle.py calls this).

Drawn from Smeargle's own frames, after Nick's sketch: a huge flat paintbrush for a tail, its
bristles painted in stripes of the rainbow, a painter's palette hung round its neck, and a curled
moustache. One look for every type (it's every colour at once): one palette, one shiny palette.

Smeargle's palette has room for six rainbow colours once its two odd browns go: index 1 (a reddish
brown) is drawn as 4 and index 10 (one grey pixel) as 5. Then 13 red, 8 orange, 11 yellow, 9 green,
10 blue, 1 purple.
"""
import math
import random

from PIL import Image

RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE = 13, 8, 11, 9, 10, 1
RAINBOW = [RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE]
RAINBOW_RGB = {RED: (232, 64, 56), ORANGE: (248, 144, 40), YELLOW: (248, 216, 56), GREEN: (88, 192, 64),
               BLUE: (64, 128, 232), PURPLE: (144, 88, 200)}
CREAM, FUR, FUR_DARK, DARK, BROWN, OUTLINE, WHITE = 2, 5, 12, 3, 4, 6, 7
OLD_PAINT = (13, 8, 11, 9)


def mega_palette(base):
    pal = list(base)
    for idx, c in RAINBOW_RGB.items():
        pal[idx] = tuple((v >> 3) << 3 for v in c)
    return pal


def _remap(img):
    return img.point(lambda v: BROWN if v == 1 else FUR if v == 10 else v)


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


def draw_brush(o, w, h, hx, hy, angle, length, w0, w1, ferrule, seed, wobble=0.0):
    """A flat brush from its handle end (hx, hy) pointing `angle` degrees (0 = right, -90 = up),
    drawn behind whatever's already there. Returns its pixels."""
    rnd = random.Random(seed)
    ux, uy = math.cos(math.radians(angle)), math.sin(math.radians(angle))
    vx, vy = -uy, ux                      # (across the brush)
    tips = [length + rnd.choice((-2, -1, 0, 0, 1)) for _ in range(64)]
    cream_end = [length * 0.42 + 2 * math.sin(i * 1.3 + seed) for i in range(64)]
    px = {}
    for y in range(h):
        for x in range(w):
            rx, ry = x + 0.5 - hx, y + 0.5 - hy
            a = rx * ux + ry * uy            # (along)
            b = rx * vx + ry * vy            # (across)
            if a < 0:
                continue
            if a < ferrule:
                half = w0 / 2 * 0.8
                if abs(b) <= half:
                    px[(x, y)] = DARK if b > half - 1.5 else BROWN
                continue
            t = (a - ferrule) / (length - ferrule)
            half = (w0 + (w1 - w0) * min(1, t * 1.6)) / 2
            if abs(b) > half:
                continue
            s = (b + half) / (2 * half)       # (0..1 across)
            col = min(int(s * 6), 5)
            # (a little sway: the far end leans)
            if a > tips[int(s * 63)] + wobble * (s - 0.5) * 2:
                continue
            if a - ferrule < cream_end[int(s * 63)] - ferrule * 0.2:
                px[(x, y)] = CREAM if (abs(b) < half - 1) else FUR
            else:
                px[(x, y)] = RAINBOW[col]
    for q, v in px.items():
        if o[q] == 0:
            o[q] = v
    _outline(o, set(px), w, h)
    return px


def draw_palette(o, cx, cy, rx, ry, w, h):
    """The painter's palette round its neck: a brown oval with blobs of paint on it."""
    pts = set()
    for y in range(h):
        for x in range(w):
            if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1:
                pts.add((x, y))
    for q in pts:
        o[q] = BROWN
    for (x, y) in pts:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if (x + dx, y + dy) not in pts:
                o[x, y] = DARK
                break
    blobs = [(-rx * 0.5, -0.3, RED), (-rx * 0.1, 0.6, YELLOW), (rx * 0.25, -0.4, GREEN), (rx * 0.6, 0.4, PURPLE)]
    for bx, by, c in blobs:
        x0, y0 = int(round(cx + bx)), int(round(cy + by))
        for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
            if (x0 + dx, y0 + dy) in pts:
                o[x0 + dx, y0 + dy] = c
    return pts


def draw_moustache(o, pts):
    for q in pts:
        o[q] = DARK


def front(base_frame, flourish=False):
    """(64x64, Smeargle's palette indices)"""
    img = _remap(base_frame)
    p = img.load()
    w, h = img.size
    # (its own brush, gone)
    for y in range(h):
        for x in range(w):
            if (x < 16 and y < 37) or p[x, y] in OLD_PAINT:
                p[x, y] = 0
    img = _shift(img, 6)
    o = img.load()
    draw_brush(o, w, h, 21, 37, -104 + (3 if flourish else 0), 32, 10, 22, 6, seed=3, wobble=2 if flourish else 0)
    draw_palette(o, 40.5, 40.5, 8.2, 4.6, w, h)
    # (a curled moustache under its snout)
    draw_moustache(o, [(x, 31) for x in range(29, 37)] + [(x, 32) for x in range(31, 35)]
                   + [(28, 30), (27, 29), (27, 28), (28, 27), (37, 30), (38, 29), (38, 28), (37, 27)])
    return img


def back(base):
    img = _remap(base)
    p = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            if p[x, y] in OLD_PAINT or (x > 40 and y < 34):
                p[x, y] = 0
    img = _shift(img, -4)
    o = img.load()
    draw_brush(o, w, h, 44, 40, -76, 33, 10, 22, 6, seed=5)
    # (the palette's strap, round its neck)
    for x in range(14, 40):
        for y in (35, 36):
            if o[x, y] in (FUR, CREAM, FUR_DARK, BROWN):
                o[x, y] = DARK if y == 35 else BROWN
    return img


def icon(base_frame):
    """(32x32, icon palette 1: its brush in pink, yellow and green)"""
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
    bands = [8, 11, 5]     # (palette 1: pink, yellow, green)
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
