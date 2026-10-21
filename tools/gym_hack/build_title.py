#!/usr/bin/env python3
"""The title screen (Gym Trainer hack): "POKéMON CANVAS", with Smeargle.

FireRed's title logo is one 8bpp background (graphics/title_screen_frlg/firered/game_title_logo.*):
the POKéMON logo and "FIRERED VERSION" under it. This keeps the POKéMON logo and draws "CANVAS"
in its place: white block letters (the FIRERED lettering's style) on a brush stroke of paint in
the type colors, dripping. The art is drawn here, then cut back into tiles for the background.
Charizard's box art (box_art_mon.*, a 4bpp background in palette 13) becomes Smeargle, the
painter Pokémon: its front sprite at twice the size, clear of the logo. (The flames are turned
off in src/title_screen_frlg.c.)

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
WORD, SIZE, GAP, SLANT = "CANVAS", 23, 2, 0.18
PLACE = (2, 62)                                        # where the art goes (pixels)
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


def box_art():
    """Smeargle on BG1 (palette 13), at the screen's bottom right. -> its picture (for previews)"""
    src = Image.open(ROOT / "graphics/pokemon/smeargle/anim_front.png")
    frame = src.crop((0, 0, 64, 64))
    frame = frame.crop(frame.getbbox() or (0, 0, 64, 64))
    big = frame.resize((frame.width * 2, frame.height * 2), Image.NEAREST)
    x0, y0 = 239 - big.width, 150 - big.height          # (right of the logo, above the copyright)
    logo = art()
    right = PLACE[0] + max(x for x, y in logo) + 1
    assert x0 >= right, f"Smeargle at x {x0} would run under CANVAS (to x {right})"
    pal = src.getpalette()[:48]
    screen = np.zeros((160, 256), int)
    bp = big.load()
    for y in range(big.height):
        for x in range(big.width):
            screen[y0 + y, x0 + x] = bp[x, y]
    tiles, lookup, ents = [tuple([0] * 64)], {tuple([0] * 64): 0}, []
    for ty in range(20):
        for tx in range(32):
            t = tuple(int(v) for v in screen[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8].flatten())
            if t not in lookup:
                lookup[t] = len(tiles)
                tiles.append(t)
            ents.append((13 << 12) | lookup[t] if lookup[t] else 0)
    cols = 12
    img = Image.new("P", (cols * 8, ((len(tiles) + cols - 1) // cols) * 8), 0)
    img.putpalette(pal + [0] * (768 - len(pal)))
    ip = img.load()
    for n, t in enumerate(tiles):
        for k, v in enumerate(t):
            ip[(n % cols) * 8 + k % 8, (n // cols) * 8 + k // 8] = v
    img.save(D / "box_art_mon.png")
    (D / "box_art_mon.bin").write_bytes(struct.pack("<640H", *ents))
    (D / "box_art_mon.pal").write_text("JASC-PAL\r\n0100\r\n16\r\n" + "".join(
        f"{pal[i * 3]} {pal[i * 3 + 1]} {pal[i * 3 + 2]}\r\n" for i in range(16)))
    print(f"box art: Smeargle, {len(tiles)} tiles, at ({x0}, {y0})")
    rgba = Image.new("RGBA", (240, 160), (0, 0, 0, 0))
    rgba.paste(big.convert("RGBA"), (x0, y0), big.convert("RGBA").split()[3] if "A" in big.convert("RGBA").getbands() else None)
    mask = Image.new("L", (240, 160), 0)
    mp = mask.load()
    for y in range(160):
        for x in range(240):
            if screen[y, x]:
                mp[x, y] = 255
    out = Image.new("RGBA", (240, 160), (0, 0, 0, 0))
    rgb = Image.new("RGB", (240, 160))
    rp = rgb.load()
    for y in range(160):
        for x in range(240):
            if screen[y, x]:
                c = screen[y, x]
                rp[x, y] = tuple(pal[c * 3:c * 3 + 3])
    out.paste(rgb, (0, 0), mask)
    return out


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
    for f in ("box_art_mon.png", "box_art_mon.bin", "box_art_mon.pal"):
        if not (ORIG / f).exists():
            (ORIG / f).write_bytes((D / f).read_bytes())
    screen = np.zeros((160, 256), int)
    for i, e in enumerate(ents):
        tx, ty = i % 32, i // 32
        if ty >= 8:
            continue
        t, hf, vf = e & 0x3FF, (e >> 10) & 1, (e >> 11) & 1
        for y in range(8):
            for x in range(8):
                screen[ty * 8 + y, tx * 8 + x] = sp[(t % 32) * 8 + (7 - x if hf else x), (t // 32) * 8 + (7 - y if vf else y)]

    screen[56:64, 48:148] = 0                       # (the top of FIRERED, under the POKéMON logo)

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
    smeargle = box_art()
    if len(sys.argv) > 1:
        prev = Image.new("RGB", (240, 160), (66, 180, 170))
        prev.paste(smeargle, (0, 0), smeargle)
        pp = prev.load()
        for y in range(160):
            for x in range(240):
                if screen[y, x]:
                    pp[x, y] = pal[screen[y, x]]
        prev.resize((720, 480), Image.NEAREST).save(sys.argv[1])


if __name__ == "__main__":
    main()
