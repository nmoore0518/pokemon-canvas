#!/usr/bin/env python3
"""Item icons for the Lucky Egg and Amulet Coin upgrade materials (Pokémon Canvas):
Lucky Clover, Four-Leaf Clover, Golden Clover; Copper Coin, Silver Coin, Gold Coin.
24x24, 4bpp: graphics/items/icons/gym/<name>.png and icon_palettes/gym/<name>.pal.

    python3 tools/gym_hack/upgrade_item_art.py [preview.png]
"""
import math
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ICONS = ROOT / "graphics/items/icons/gym"
PALS = ROOT / "graphics/items/icon_palettes/gym"
BG = (115, 197, 164)      # the icons' transparent color (index 0)

# index: 1 outline, 2 dark, 3 mid, 4 light, 5 shine, 6 stem/emblem dark, 7 stem/emblem
CLOVERS = {
    "lucky_clover":     [(24, 64, 24), (56, 136, 48), (96, 184, 72), (152, 224, 112), (216, 248, 184), (64, 104, 40), (112, 152, 64)],
    "four_leaf_clover": [(16, 56, 32), (32, 120, 64), (64, 168, 88), (120, 216, 128), (208, 248, 200), (56, 96, 40), (104, 144, 56)],
    "golden_clover":    [(88, 56, 8), (176, 120, 16), (224, 176, 40), (248, 216, 96), (255, 248, 200), (136, 96, 24), (192, 152, 48)],
}
COINS = {
    "copper_coin": [(72, 32, 16), (144, 72, 40), (192, 112, 64), (224, 152, 104), (248, 208, 168), (120, 56, 32), (168, 96, 56)],
    "silver_coin": [(56, 64, 80), (120, 128, 144), (168, 176, 192), (208, 216, 224), (248, 248, 255), (96, 104, 120), (144, 152, 168)],
    "gold_coin":   [(88, 56, 8), (176, 120, 16), (224, 176, 40), (248, 216, 96), (255, 248, 200), (136, 96, 24), (192, 152, 48)],
}


def blank():
    return [[0] * 24 for _ in range(24)]


def outline(g):
    out = [row[:] for row in g]
    for y in range(24):
        for x in range(24):
            if g[y][x] == 0 and any(0 <= y + dy < 24 and 0 <= x + dx < 24 and g[y + dy][x + dx] not in (0, 1)
                                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = 1
    return out


def clover(leaves):
    """heart-shaped leaves around a middle, each its own (a dark line where two meet), a curved stem"""
    owner = [[-1] * 24 for _ in range(24)]
    shade = [[0] * 24 for _ in range(24)]
    cx, cy = 11.5, 10.5
    start = -math.pi / 2 if leaves == 3 else -math.pi / 4
    for k in range(leaves):
        ang = start + k * 2 * math.pi / leaves
        ux, uy = math.cos(ang), math.sin(ang)
        for y in range(24):
            for x in range(24):
                dx, dy = x + 0.5 - cx, y + 0.5 - cy
                a, b = dx * ux + dy * uy, -dx * uy + dy * ux       # along the leaf, across it
                inside = False
                for lobe in (-1, 1):                               # two round lobes at the tip...
                    if (a - 5.2) ** 2 + (b - lobe * 2.1) ** 2 <= 2.9 ** 2:
                        inside = True
                if 0 <= a <= 5.2 and abs(b) <= a * 0.75 + 0.3:      # ...narrowing to the middle
                    inside = True
                if inside and owner[y][x] == -1:
                    owner[y][x] = k
                    shade[y][x] = 4 if b < -0.8 and a > 3 else 3
                    if abs(b) < 0.5 and 1 < a < 6:
                        shade[y][x] = 2                            # the vein
    g = blank()
    for y in range(24):
        for x in range(24):
            if owner[y][x] < 0:
                continue
            edge = any(0 <= y + dy < 24 and 0 <= x + dx < 24 and owner[y + dy][x + dx] not in (-1, owner[y][x])
                       for dx, dy in ((1, 0), (0, 1)))
            g[y][x] = 2 if edge else shade[y][x]
    g[int(cy)][int(cx)] = 2
    for t in range(10):                                  # the stem, curving down-right
        x, y = int(cx + 1 + t * 0.5), int(cy + 2 + t)
        if y < 23 and g[y][x] == 0:
            g[y][x] = 6
            if x + 1 < 24 and g[y][x + 1] == 0:
                g[y][x + 1] = 7
    return outline(g)


def coin():
    g = blank()
    cx, cy, r = 11.5, 11.5, 9.5
    for y in range(24):
        for x in range(24):
            d = math.hypot(x + 0.5 - 12, y + 0.5 - 12)
            if d <= r:
                g[y][x] = 3
                if d > r - 1.6:
                    g[y][x] = 2                          # the rim
                elif d > r - 2.6:
                    g[y][x] = 4
    # a stamped clover-and-star emblem: a ring and a mark
    for y in range(24):
        for x in range(24):
            d = math.hypot(x + 0.5 - 12, y + 0.5 - 12)
            if 3.6 < d <= 4.6:
                g[y][x] = 6
    for x, y in ((11, 9), (12, 9), (10, 10), (13, 10), (11, 12), (12, 12), (11, 13), (12, 13), (11, 11), (12, 11)):
        g[y][x] = 7
    for x, y in ((7, 6), (8, 6), (6, 7), (6, 8)):
        g[y][x] = 5                                      # shine
    return outline(g)


def save(name, g, colors):
    img = Image.new("P", (24, 24))
    pal = [BG] + colors + [(0, 0, 0)] * (15 - len(colors))
    img.putpalette([c for rgb in pal for c in rgb])
    img.putdata([v for row in g for v in row])
    img.save(ICONS / f"{name}.png")
    (PALS / f"{name}.pal").write_text("JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{r} {gg} {b}\r\n" for r, gg, b in pal), newline="")
    return img


def main():
    imgs = []
    for name, cols in CLOVERS.items():
        g = clover(3 if name == "lucky_clover" else 4)
        if name == "golden_clover":
            for x, y in ((5, 4), (18, 5), (19, 16)):
                g[y][x] = 5                               # sparkles
        imgs.append(save(name, g, cols))
    for name, cols in COINS.items():
        imgs.append(save(name, coin(), cols))
    if len(sys.argv) > 1:
        sheet = Image.new("RGB", (24 * len(imgs) * 4, 24 * 4), (200, 200, 200))
        for i, im in enumerate(imgs):
            sheet.paste(im.convert("RGB").resize((96, 96), Image.NEAREST), (i * 96, 0))
        sheet.save(sys.argv[1])
    print("6 icons")


if __name__ == "__main__":
    main()
