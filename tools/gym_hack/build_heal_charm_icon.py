#!/usr/bin/env python3
"""Pokémon Canvas: the Heal Charm's item icon (the Universal Pokémon Medical Center's 2F prize): the
Catching Charm, its green gem turned rosy pink with a white cross on it. Writes
graphics/items/icons/gym/heal_charm.png and its palette.

    python3 tools/gym_hack/build_heal_charm_icon.py [preview.png]
"""
import colorsys
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
src = Image.open(ROOT / "graphics/items/icons/catching_charm.png")
pal = [tuple(int(v) for v in l.split()) for l in (ROOT / "graphics/items/icon_palettes/catching_charm.pal").read_text().splitlines()[3:19]]
out = []
for i, (r, g, b) in enumerate(pal):
    h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
    if i > 0 and s > 0.15 and 0.25 < h < 0.55:          # (the gem's greens and teals: to pink)
        r2, g2, b2 = colorsys.hls_to_rgb(0.96, l, min(1, s * 1.1))
        out.append((int(r2 * 255) >> 3 << 3, int(g2 * 255) >> 3 << 3, int(b2 * 255) >> 3 << 3))
    else:
        out.append(pal[i])
img = src.copy()
px = img.load()
# the gem: the pixels in the recoloured range; a white cross at its middle
gem = [(x, y) for y in range(img.height) for x in range(img.width)
       if px[x, y] and out[px[x, y]] != pal[px[x, y]]]
white = min(range(1, 16), key=lambda i: -sum(out[i]))
# (the gem itself: the biggest connected patch of them; the ribbon's pink is another)
left, best = set(gem), []
while left:
    stack, part = [left.pop()], []
    while stack:
        x, y = stack.pop()
        part.append((x, y))
        for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if n in left:
                left.remove(n)
                stack.append(n)
    if len(part) > len(best):
        best = part
gem = best
cx = round(sum(x for x, _ in gem) / len(gem))
cy = round(sum(y for _, y in gem) / len(gem))
for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
    px[cx + dx, cy + dy] = white
img.save(ROOT / "graphics/items/icons/gym/heal_charm.png")
(ROOT / "graphics/items/icon_palettes/gym/heal_charm.pal").write_text(
    "JASC-PAL\n0100\n16\n" + "".join(f"{r} {g} {b}\n" for r, g, b in out))
if len(sys.argv) > 1:
    prev = img.convert("P")
    prev.putpalette([c for t in out for c in t] + [0] * (768 - 48))
    prev.convert("RGB").resize((img.width * 6, img.height * 6), Image.NEAREST).save(sys.argv[1])
