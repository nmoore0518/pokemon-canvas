#!/usr/bin/env python3
"""The Spirit Ward's item icon (Pokémon Canvas): a little burlap scarecrow doll, stitched together,
straw poking out, button-X eyes and a pin through its head. 24x24, 4bpp:
graphics/items/icons/gym/spirit_ward.png and icon_palettes/gym/spirit_ward.pal.

    python3 tools/gym_hack/spirit_ward_art.py [preview.png]
"""
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
PAL = [
    (115, 197, 164),   # 0 transparent
    (40, 26, 30),      # 1 outline
    (112, 78, 52),     # 2 burlap dark
    (160, 118, 76),    # 3 burlap
    (204, 166, 116),   # 4 burlap light
    (168, 124, 36),    # 5 straw dark
    (232, 200, 88),    # 6 straw
    (176, 36, 48),     # 7 red thread / pin head
    (240, 120, 120),   # 8 pin head shine
    (112, 72, 168),    # 9 purple patch
    (156, 116, 212),   # 10 purple patch light
    (196, 200, 216),   # 11 pin
    (248, 248, 255),   # 12 shine
    (0, 0, 0), (0, 0, 0), (0, 0, 0),
]


def draw():
    g = [[0] * 24 for _ in range(24)]

    def put(x, y, c):
        if 0 <= x < 24 and 0 <= y < 24:
            g[y][x] = c
    # straw: out of the top of the head, the sleeves and the legs
    for x, y in ((9, 2), (10, 1), (11, 2), (12, 1), (13, 2), (14, 1), (10, 2), (12, 2)):
        put(x, y, 6 if (x + y) % 2 else 5)
    for x, y in ((1, 12), (2, 13), (1, 14), (2, 12), (2, 14), (21, 12), (22, 13), (21, 14), (22, 12), (22, 14)):
        put(x, y, 6 if (x + y) % 2 else 5)
    for x, y in ((9, 21), (10, 22), (8, 22), (14, 21), (13, 22), (15, 22)):
        put(x, y, 6 if (x + y) % 2 else 5)
    # head: a lumpy sack, tied off at the neck
    for y in range(3, 11):
        for x in range(6, 18):
            if ((x + 0.5 - 12) / 5.6) ** 2 + ((y + 0.5 - 7) / 4.2) ** 2 <= 1:
                put(x, y, 3)
    for y in range(3, 11):        # shading: light upper left, dark lower right
        for x in range(6, 18):
            if g[y][x] == 3:
                if ((x + 0.5 - 10.5) / 3.2) ** 2 + ((y + 0.5 - 5) / 1.6) ** 2 <= 1:
                    put(x, y, 4)
                elif ((x + 0.5 - 12) / 5.6) ** 2 + ((y + 0.5 - 7) / 4.2) ** 2 > 0.8 and (y >= 9 or x >= 16):
                    put(x, y, 2)
    for x in range(10, 14):       # the tie at the neck
        put(x, 11, 7)
    # arms (one stick across) and body
    for y in range(12, 15):
        for x in range(3, 21):
            put(x, y, 4 if y == 12 else (2 if y == 14 else 3))
    for y in range(12, 19):
        for x in range(9, 15):
            put(x, y, 3)
    for y in range(12, 19):
        put(14, y, 2)
    for y in range(15, 19):
        put(9, y, 4)
    # legs
    for y in range(19, 21):
        for x in (9, 10, 13, 14):
            put(x, y, 2 if x in (10, 14) else 3)
    # a purple patch on its chest, sewn on
    for y in range(14, 17):
        for x in range(10, 13):
            put(x, y, 10 if (x, y) == (10, 14) else 9)
    for x, y in ((13, 14), (13, 16), (11, 17)):
        put(x, y, 7)
    # the sleeves' stitches
    for x in (5, 7, 16, 18):
        put(x, 13, 7)
    # face: X-stitched eyes, a stitched grin
    for dx, dy in ((-1, -1), (1, -1), (0, 0), (-1, 1), (1, 1)):     # one eye sewn shut with an X...
        put(9 + dx, 6 + dy, 7)
    for x, y in ((13, 5), (14, 5), (13, 6), (14, 6), (13, 7), (14, 7), (15, 6), (12, 6)):   # ...the other a button
        put(x, y, 1)
    put(13, 5, 12)
    for x in range(9, 16):        # (a stitched-shut grin)
        put(x, 8, 1)
    for x in (10, 12, 14):
        put(x, 9, 1)
    # the pin, through its head from the upper right
    for k, (x, y) in enumerate(((16, 4), (17, 3), (18, 2))):
        put(x, y, 11)
    put(19, 1, 7); put(20, 1, 7); put(19, 0, 7); put(20, 0, 8)
    # outline everything
    out = [row[:] for row in g]
    for y in range(24):
        for x in range(24):
            if g[y][x] == 0 and any(0 <= y + dy < 24 and 0 <= x + dx < 24 and g[y + dy][x + dx] not in (0, 1)
                                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = 1
    return out


def save(g):
    im = Image.new("P", (24, 24))
    im.putpalette([v for c in PAL for v in c] + [0] * (768 - 48))
    for y in range(24):
        for x in range(24):
            im.putpixel((x, y), g[y][x])
    im.save(ROOT / "graphics/items/icons/gym/spirit_ward.png")
    with open(ROOT / "graphics/items/icon_palettes/gym/spirit_ward.pal", "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n16\n" + "".join(f"{r} {g_} {b}\n" for r, g_, b in PAL))
    return im


if __name__ == "__main__":
    im = save(draw())
    if len(sys.argv) > 1:
        im.convert("RGB").resize((192, 192), Image.NEAREST).save(sys.argv[1])
