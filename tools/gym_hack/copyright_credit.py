#!/usr/bin/env python3
"""The copyright screen's small credit line for pokeemerald and pokeemerald-expansion (Pokémon
Canvas: in place of the "Powered by" splash, EXPANSION_INTRO FALSE).

    python3 tools/gym_hack/copyright_credit.py

Adds the lines to graphics/intro_frlg/copyright.png (new tiles after the first 39, the copyright's
own) and copyright.bin (under the copyright), in the game's small font. Re-running is safe.
"""
import re
import struct
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
PNG = ROOT / "graphics/intro_frlg/copyright.png"
BIN = ROOT / "graphics/intro_frlg/copyright.bin"
ORIGINAL_TILES = 39
LINES = ["Powered by pokeemerald (pret)", "and pokeemerald-expansion (RHH)"]
FIRST_Y = 128           # (pixels: the copyright's own text ends at 104)
LINE_HEIGHT = 11
FG, SHADOW = 12, 7      # (copyright.pal: greys; 15 is the copyright's white)
BLANK = 1               # (the copyright's blank tile)


def charmap():
    cm = {}
    for line in (ROOT / "charmap.txt").read_text(encoding="utf-8").splitlines():
        m = re.match(r"^'(.)'\s*=\s*([0-9A-Fa-f]{2})\s*$", line.strip())
        if m and m.group(1) not in cm:
            cm[m.group(1)] = int(m.group(2), 16)
    return cm


def widths():
    text = (ROOT / "src/fonts.c").read_text()
    body = text.split("gFontSmallLatinGlyphWidths[] = {", 1)[1].split("};", 1)[0]
    return [int(v) for v in re.findall(r"\d+", body)]


def main():
    cm, wd = charmap(), widths()
    font = Image.open(ROOT / "graphics/fonts/latin_small.png")
    fp = font.load()
    canvas = [[0] * 240 for _ in range(160)]
    for n, line in enumerate(LINES):
        codes = [cm[c] for c in line]
        total = sum(wd[c] for c in codes)
        x = (240 - total) // 2
        y0 = FIRST_Y + n * LINE_HEIGHT
        for c in codes:
            gx, gy = (c % 16) * 16, (c // 16) * 16
            for y in range(1, 13):
                for dx in range(wd[c]):
                    v = fp[gx + dx, gy + y]
                    if v in (1, 2):
                        canvas[y0 + y - 1][x + dx] = FG if v == 1 else SHADOW
            x += wd[c]
    img = Image.open(PNG)
    pal = img.getpalette()
    w = img.size[0] // 8
    old = [tuple(img.getpixel(((i % w) * 8 + x, (i // w) * 8 + y)) for y in range(8) for x in range(8))
           for i in range(ORIGINAL_TILES)]
    tiles = list(old)
    seen = {t: i for i, t in enumerate(tiles)}
    tmap = list(struct.unpack("<1024H", BIN.read_bytes()))
    for ty in range(14, 20):
        for tx in range(32):
            tmap[ty * 32 + tx] = BLANK
    for ty in range(FIRST_Y // 8, 20):
        for tx in range(30):
            t = tuple(canvas[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8))
            if not any(t):
                continue
            if t not in seen:
                seen[t] = len(tiles)
                tiles.append(t)
            tmap[ty * 32 + tx] = seen[t]
    rows = (len(tiles) + w - 1) // w
    out = Image.new("P", (w * 8, rows * 8))
    out.putpalette(pal)
    op = out.load()
    for i, t in enumerate(tiles):
        for y in range(8):
            for x in range(8):
                op[(i % w) * 8 + x, (i // w) * 8 + y] = t[y * 8 + x]
    out.save(PNG)
    BIN.write_bytes(struct.pack("<1024H", *tmap))
    print(len(tiles) - ORIGINAL_TILES, "new tiles")


if __name__ == "__main__":
    main()
