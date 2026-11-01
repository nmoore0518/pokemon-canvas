#!/usr/bin/env python3
"""Pixel art for the berry field in Pallet Town (Pokémon Canvas): one 16x16 block per plot, a
plant in a bed of soil at each stage of its growth. build_pallet_town.py cuts them into metatiles.

The plants are drawn in named colors; each stage is then indexed into a palette:
  - primary palette 0 (the town's grass, trees and flowers: greens, dark soil, red, orange, teal)
    for the bare soil, the seed, the sprout, the young plant and red / yellow / green berries;
  - the Pallet Town tileset's palette 11, whose unused slots get the same soil and leaf colors
    plus blue, purple and pink, for blue / purple / pink berries.
A berry's color is its own (gBerries[].info.color: BERRY_COLOR_RED, BLUE, PURPLE, GREEN, YELLOW, PINK).

    python3 tools/gym_hack/berry_field_art.py preview.png
"""
import sys
from PIL import Image

# the colors, by name
SOIL, SOIL_DK, LEAF_DK, LEAF, LEAF_LT, OUTLINE = "soil", "soil_dk", "leaf_dk", "leaf", "leaf_lt", "outline"
BERRY, BERRY_HI, BERRY_DK = "berry", "berry_hi", "berry_dk"

PRIMARY_0 = {   # primary palette 0 (General_Frlg), as it is
    SOIL: 5, SOIL_DK: 6, LEAF_DK: 3, LEAF: 2, LEAF_LT: 1, OUTLINE: 4,
}
# (index into primary palette 0 for the berry, its highlight, its shade)
PRIMARY_0_BERRIES = {
    "red": (11, 9, 6), "yellow": (9, 1, 10), "green": (14, 12, 15),
}
# Pallet Town's palette 11: its slots 1-4, 9-11, 14 and 15 were unused (the tiles use 0, 5-8, 12, 13)
PAL11_SLOTS = {
    1: (65, 57, 49), 2: (115, 98, 98), 3: (57, 148, 49), 4: (131, 213, 98), 9: (57, 90, 16),
    10: (74, 123, 230), 11: (164, 98, 213), 14: (246, 139, 197), 15: (189, 255, 139),
}
PAL11 = {SOIL: 2, SOIL_DK: 1, LEAF_DK: 3, LEAF: 4, LEAF_LT: 15, OUTLINE: 9}
PAL11_BERRIES = {   # (berry, highlight: the palette's own near-white, shade: the outline)
    "blue": (10, 5, 9), "purple": (11, 5, 9), "pink": (14, 5, 9),
}
COLORS = ["red", "blue", "purple", "green", "yellow", "pink"]     # (BERRY_COLOR_* order)


def bed():
    """bare soil: a raised bed, darker rows and a dark rim"""
    g = [[SOIL] * 16 for _ in range(16)]
    for i in range(16):
        g[0][i] = g[15][i] = g[i][0] = g[i][15] = SOIL_DK
    for y in (4, 8, 12):
        for x in range(2, 14):
            if (x + y) % 3:
                g[y][x] = SOIL_DK
    return g


def dot(g, x, y, c):
    if 0 <= x < 16 and 0 <= y < 16:
        g[y][x] = c


def stem(g, x, y0, y1):
    for y in range(y0, y1 + 1):
        dot(g, x, y, LEAF_DK)


def leaf(g, x, y, d):
    """a little leaf going left (d=-1) or right (d=1) from (x, y)"""
    dot(g, x + d, y, LEAF)
    dot(g, x + 2 * d, y, LEAF)
    dot(g, x + 2 * d, y - 1, LEAF_LT)
    dot(g, x + 3 * d, y - 1, LEAF)
    dot(g, x + d, y + 1, LEAF_DK)
    dot(g, x + 2 * d, y + 1, LEAF_DK)


def seed():
    g = bed()
    for x in range(6, 10):
        dot(g, x, 9, SOIL_DK)
    for x in range(5, 11):
        dot(g, x, 10, SOIL_DK)
    dot(g, 7, 8, OUTLINE)
    dot(g, 8, 8, OUTLINE)
    return g


def sprout():
    g = bed()
    stem(g, 8, 8, 11)
    leaf(g, 8, 8, -1)
    leaf(g, 8, 7, 1)
    return g


def young():
    g = bed()
    stem(g, 8, 4, 12)
    for y, d in ((10, -1), (9, 1), (7, -1), (6, 1)):
        leaf(g, 8, y, d)
    dot(g, 8, 3, LEAF_LT)
    return g


def bush():
    g = bed()
    stem(g, 8, 6, 13)
    stem(g, 7, 11, 13)
    for y, d in ((11, -1), (10, 1), (8, -1), (7, 1), (5, -1), (4, 1)):
        leaf(g, 8, y, d)
        leaf(g, 7 if d < 0 else 9, y + 1, d)
    for x in range(6, 11):
        dot(g, x, 3, LEAF)
    dot(g, 8, 2, LEAF_LT)
    return g


def flowering():
    g = bush()
    for x, y in ((4, 4), (11, 3), (8, 1), (3, 8), (12, 7)):
        for dx, dy in ((0, -1), (-1, 0), (1, 0), (0, 1)):
            dot(g, x + dx, y + dy, BERRY)
        dot(g, x, y, BERRY_HI)
    return g


def ripe():
    g = bush()
    for x, y in ((4, 5), (11, 4), (7, 2), (3, 10), (12, 9), (9, 7)):
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                dot(g, x + dx, y + dy, BERRY)
        dot(g, x - 1, y - 1, BERRY_HI)
        dot(g, x + 1, y + 1, BERRY_DK)
        dot(g, x, y + 1, BERRY_DK)
    return g


def indexed(g, color=None):
    """(palette number, 16x16 palette indices) for a drawing"""
    if color in PAL11_BERRIES:
        names, (b, hi, dk), pal = dict(PAL11), PAL11_BERRIES[color], 11
    else:
        names, (b, hi, dk), pal = dict(PRIMARY_0), PRIMARY_0_BERRIES.get(color or "red"), 0
    names.update({BERRY: b, BERRY_HI: hi, BERRY_DK: dk})
    return pal, [[names[c] for c in row] for row in g]


def stages():
    """{name: (palette, 16x16 indices)}: soil, seed, sprout, young, flower_<color>, ripe_<color>"""
    out = {"soil": indexed(bed()), "seed": indexed(seed()), "sprout": indexed(sprout()),
           "young": indexed(young())}
    for c in COLORS:
        out[f"flower_{c}"] = indexed(flowering(), c)
        out[f"ripe_{c}"] = indexed(ripe(), c)
    return out


def palettes(root):
    """the two palettes as RGB lists: primary 0 (read) and Pallet's 11 (with its new slots)"""
    def load(path):
        lines = path.read_text().split()
        vals = list(map(int, lines[3:3 + 48]))
        return [tuple(vals[i:i + 3]) for i in range(0, 48, 3)]
    p0 = load(root / "data/tilesets/primary/general_frlg/palettes/00.pal")
    p11 = load(root / "data/tilesets/secondary/pallet_town_frlg/palettes/11.pal")
    for k, c in PAL11_SLOTS.items():
        p11[k] = c
    return {0: p0, 11: p11}


def preview(out):
    from pathlib import Path
    pals = palettes(Path(__file__).resolve().parents[2])
    st = stages()
    img = Image.new("RGB", (len(st) * 18, 18), (80, 160, 80))
    for i, (name, (pal, g)) in enumerate(st.items()):
        for y in range(16):
            for x in range(16):
                img.putpixel((i * 18 + 1 + x, 1 + y), pals[pal][g[y][x]][:3])
    img.resize((img.width * 4, img.height * 4), Image.NEAREST).save(out)


if __name__ == "__main__":
    preview(sys.argv[1] if len(sys.argv) > 1 else "berry_field.png")
