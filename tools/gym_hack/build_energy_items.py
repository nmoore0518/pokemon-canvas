#!/usr/bin/env python3
"""Grandmaster Rod's energy cards (Pokémon Canvas; src/energy_cards.c): their item icons, made from the
energy cards' own art in the Pokémon Trading Card Game (graphics/tcg/cards/, from pret/poketcg).

    python3 tools/gym_hack/build_energy_items.py

Writes graphics/items/icons/gym/<energy>.png and graphics/items/icon_palettes/gym/<energy>.pal:
the card's middle (its symbol), 24x24, in its own Game Boy Color colours, with a dark card edge.
"""
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
CARDS = ROOT / "graphics/tcg/cards"
ENERGIES = ["grass", "fire", "water", "lightning", "psychic", "fighting", "doublecolorless"]


def icon(name):
    src = Image.open(CARDS / f"{name}energy.png")
    pal = src.getpalette()[:12]
    cols = [tuple(pal[i * 3:i * 3 + 3]) for i in range(4)]
    # (the symbol: the card art's middle, 40x40 around its centre, to 20x20)
    w, h = src.size
    crop = src.crop((w // 2 - 20, h // 2 - 20, w // 2 + 20, h // 2 + 20)).convert("RGB")
    small = crop.resize((20, 20), Image.Resampling.BOX)
    out = Image.new("P", (24, 24))
    gba = [(0, 0, 0)] * 16
    gba[0] = (255, 0, 255)                  # (transparent)
    for i, c in enumerate(cols):
        gba[1 + i] = tuple((v >> 3) << 3 for v in c)
    gba[5] = (24, 24, 32)                   # (the card's edge)
    o = out.load()
    sp = small.load()
    for y in range(20):
        for x in range(20):
            c = sp[x, y]
            o[x + 2, y + 2] = 1 + min(range(4), key=lambda k: sum((a - b) ** 2 for a, b in zip(c, cols[k])))
    for x in range(1, 23):
        o[x, 1] = o[x, 22] = 5
    for y in range(1, 23):
        o[1, y] = o[22, y] = 5
    flat = [v for c in gba for v in c]
    out.putpalette(flat + [0] * (768 - len(flat)))
    (ROOT / "graphics/items/icons/gym").mkdir(parents=True, exist_ok=True)
    out.save(ROOT / f"graphics/items/icons/gym/{name}_energy.png")
    (ROOT / f"graphics/items/icon_palettes/gym/{name}_energy.pal").write_text(
        "JASC-PAL\n0100\n16\n" + "".join(f"{r} {g} {b}\n" for r, g, b in gba))


if __name__ == "__main__":
    for n in ENERGIES:
        icon(n)
    print("ok")
