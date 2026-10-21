#!/usr/bin/env python3
"""The title screen's logo (Gym Trainer hack): "POKéMON CANVAS".

FireRed's title logo is one 8bpp background (graphics/title_screen_frlg/firered/game_title_logo.*):
the POKéMON logo and "FIRERED VERSION" under it. This keeps the POKéMON logo and draws "CANVAS"
in its place: white block letters (the FIRERED lettering's style) on a brush stroke of paint in
the type colors, dripping. The art is drawn here, then cut back into tiles for the background.

    python3 tools/gym_hack/build_title.py [preview.png]
"""
import math
import random
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
D = ROOT / "graphics/title_screen_frlg/firered"
ORIG = Path(__file__).parent / "title_orig"            # (FireRed's logo, kept to rebuild from)
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf"
WORD, SIZE, GAP, SLANT = "CANVAS", 27, 3, 0.18
PLACE = (4, 62)                                        # where the art goes (pixels)
PAINT = [(224, 40, 48), (248, 136, 32), (248, 208, 40), (64, 176, 64), (40, 120, 232), (144, 64, 208)]
OUTLINE, WHITE, GRAY = (16, 16, 24), (248, 248, 248), (208, 208, 216)


def glyph(ch):
    f = ImageFont.truetype(FONT, SIZE)
    im = Image.new("L", (80, 80), 0)
    ImageDraw.Draw(im).text((10, 0), ch, font=f, fill=255, stroke_width=1, stroke_fill=255)
    a = np.array(im) > 128
    ys, xs = np.where(a)
    return a[ys.min():ys.max() + 1, xs.min():xs.max() + 1]


def letters():
    gs = [glyph(c) for c in WORD]
    h = max(g.shape[0] for g in gs)
    w = sum(g.shape[1] for g in gs) + GAP * (len(gs) - 1)
    out, x = np.zeros((h, w), bool), 0
    for g in gs:
        out[h - g.shape[0]:, x:x + g.shape[1]] = g
        x += g.shape[1] + GAP
    sh = np.zeros((h, w + int(SLANT * h) + 1), bool)          # (leaning forward)
    for y in range(h):
        off = int(round(SLANT * (h - 1 - y)))
        sh[y, off:off + w] = out[y]
    return sh


def grow(a, r):
    h, w = a.shape
    o = np.zeros((h + 2 * r, w + 2 * r), bool)
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if dx * dx + dy * dy <= r * r + 1:
                o[r + dy:r + dy + h, r + dx:r + dx + w] |= a
    return o


def art():
    """-> {(x, y): (r, g, b)} (only the drawn pixels)"""
    a = letters()
    ah, aw = a.shape
    W, H = aw + 22, ah + 14
    px = {}
    rnd = random.Random(5)
    bayer = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
    paint = np.zeros((H, W), bool)
    x0, x1 = 3, W - 3
    for x in range(x0, x1):                                 # the brush stroke
        t = (x - x0) / (x1 - x0)
        taper = min(1, (x - x0) / 10, (x1 - x) / 10)
        mid = ah * 0.62 - (t - 0.5) * 6
        half = (ah * 0.42 + 4) * (0.45 + 0.55 * taper) + rnd.uniform(-0.8, 0.8)
        ya, yb = int(mid - half), int(mid + half)
        if rnd.random() < 0.12 and taper > 0.8:
            yb += rnd.randint(3, 7)                         # (a drip)
        for y in range(max(0, ya), min(H, yb)):
            paint[y, x] = True
            f = t * 5
            i = int(f)
            c = PAINT[min(5, i + (1 if f - i > bayer[y % 4][x % 4] / 16 and i < 5 else 0))]
            k = 1.15 if y == ya else (0.88 if math.sin(y * 1.7 + x * 0.15) < -0.6 else 1.0)
            px[(x, y)] = tuple(min(255, int(v * k)) for v in c)
    edge = grow(paint, 1)[1:-1, 1:-1]
    for y in range(H):
        for x in range(W):
            if edge[y, x] and not paint[y, x]:
                px[(x, y)] = OUTLINE
    lo, ox, oy = grow(a, 2), 11, 3
    for y in range(lo.shape[0]):
        for x in range(lo.shape[1]):
            if lo[y, x]:
                px[(ox - 2 + x, oy - 2 + y)] = OUTLINE
    for y in range(ah):
        for x in range(aw):
            if a[y, x]:
                px[(ox + x, oy + y)] = WHITE if y < ah * 0.5 else GRAY
    return px


def main():
    if not ORIG.exists():
        ORIG.mkdir()
        for f in ("game_title_logo.png", "game_title_logo.bin", "game_title_logo.pal"):
            (ORIG / f).write_bytes((D / f).read_bytes())
    sheet = Image.open(ORIG / "game_title_logo.png")
    sp = sheet.load()
    pal = [tuple(int(v) for v in l.split()) for l in (ORIG / "game_title_logo.pal").read_text().split("\n")[3:] if l.strip()]
    ents = struct.unpack("<640H", (ORIG / "game_title_logo.bin").read_bytes())

    # The screen as it was (32x20 tiles), without FIRERED VERSION (the tiles under the POKéMON logo)
    screen = np.zeros((160, 256), int)
    for i, e in enumerate(ents):
        tx, ty = i % 32, i // 32
        if ty >= 8:
            continue
        t, hf, vf = e & 0x3FF, (e >> 10) & 1, (e >> 11) & 1
        for y in range(8):
            for x in range(8):
                screen[ty * 8 + y, tx * 8 + x] = sp[(t % 32) * 8 + (7 - x if hf else x), (t // 32) * 8 + (7 - y if vf else y)]

    # Colors: reuse the palette's, add the paint's in free entries (0-207 are loaded)
    used = set(np.unique(screen))
    free = [i for i in range(1, 208) if i not in used]
    index = {}
    px = art()
    for (x, y), c in px.items():
        if c not in index:
            near = next((i for i in sorted(used) if i and pal[i] == c), None)
            if near is None:
                near = free.pop(0)
                pal[near] = c
                used.add(near)
            index[c] = near
        screen[PLACE[1] + y, PLACE[0] + x] = index[c]

    # Back into tiles
    tiles, lookup, out = [tuple([0] * 64)], {tuple([0] * 64): 0}, []
    for ty in range(20):
        for tx in range(32):
            t = tuple(int(v) for v in screen[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8].flatten())
            if t not in lookup:
                lookup[t] = len(tiles)
                tiles.append(t)
            out.append(lookup[t])
    assert len(tiles) <= 256, len(tiles)
    img = Image.new("P", (256, 64), 0)
    flat = []
    for c in pal + [(0, 0, 0)] * (256 - len(pal)):
        flat += list(c)
    img.putpalette(flat)
    ip = img.load()
    for n, t in enumerate(tiles):
        for k, v in enumerate(t):
            ip[(n % 32) * 8 + k % 8, (n // 32) * 8 + k // 8] = v
    img.save(D / "game_title_logo.png")
    (D / "game_title_logo.bin").write_bytes(struct.pack("<640H", *out))
    (D / "game_title_logo.pal").write_text("JASC-PAL\r\n0100\r\n256\r\n" + "".join(f"{r} {g} {b}\r\n" for r, g, b in pal))
    print(f"title logo: {len(tiles)} tiles, {len(index)} paint colors")
    if len(sys.argv) > 1:
        prev = Image.new("RGB", (240, 160), (66, 180, 170))
        pp = prev.load()
        for y in range(160):
            for x in range(240):
                if screen[y, x]:
                    pp[x, y] = pal[screen[y, x]]
        prev.resize((720, 480), Image.NEAREST).save(sys.argv[1])


if __name__ == "__main__":
    main()
