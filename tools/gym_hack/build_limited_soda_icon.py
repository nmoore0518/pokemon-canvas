#!/usr/bin/env python3
"""Pokémon Canvas: the Limited Edition Soda's item icon: the Soda Pop bottle, its blues turned gold
(a special-edition bottle). Writes graphics/items/icons/gym/limited_edition_soda.png and its palette."""
import colorsys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
src = Image.open(ROOT / "graphics/items/icons/soda_pop.png")
pal = [tuple(int(v) for v in l.split()) for l in (ROOT / "graphics/items/icon_palettes/soda_pop.pal").read_text().splitlines()[3:19]]
out = []
for i, (r, g, b) in enumerate(pal):
    h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
    if i > 1 and s > 0.15 and 0.45 < h < 0.7:          # (the bottle's blues: to gold)
        r2, g2, b2 = colorsys.hls_to_rgb(0.12, l, min(1, s * 1.2))
        out.append((int(r2 * 255) >> 3 << 3, int(g2 * 255) >> 3 << 3, int(b2 * 255) >> 3 << 3))
    else:
        out.append(pal[i])
src.save(ROOT / "graphics/items/icons/gym/limited_edition_soda.png")
(ROOT / "graphics/items/icon_palettes/gym/limited_edition_soda.pal").write_text(
    "JASC-PAL\n0100\n16\n" + "".join(f"{r} {g} {b}\n" for r, g, b in out))
