#!/usr/bin/env python3
"""The Exotic Safari's object sprites (Pokémon Canvas): the Treasures of Ruin's stakes (a color
each), a velvet rope, a Zygarde Cell and the Prison Bottle on its pedestal, sharing one 16-color
palette.

    python3 tools/gym_hack/exotic_safari_art.py
Writes graphics/object_events/pics/misc/exotic/*.png
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "graphics/object_events/pics/misc/exotic"
PALETTE = [
    (115, 197, 164),                                   # 0 transparent
    (40, 32, 40),                                      # 1 outline
    (88, 56, 40), (136, 88, 56), (184, 136, 88),       # 2-4 wood
    (104, 48, 160), (184, 128, 232),                   # 5-6 purple (Wo-Chien)
    (200, 152, 16), (248, 224, 96),                    # 7-8 yellow (Chien-Pao)
    (40, 136, 72), (128, 216, 136),                    # 9-10 green (Ting-Lu)
    (40, 96, 200), (128, 184, 248),                    # 11-12 blue (Chi-Yu)
    (248, 248, 240),                                   # 13 white (the talisman)
    (176, 32, 48),                                     # 14 red (velvet)
    (224, 184, 72),                                    # 15 brass
]
COLORS = {"purple": (5, 6), "yellow": (7, 8), "green": (9, 10), "blue": (11, 12)}

# The stake (16x32): D dark color, L light color (the glowing tip, band and aura), w/m/l wood,
# o outline, W talisman
STAKE = [
    "................",
    "................",
    ".......L........",
    "......L.L.......",
    "....L..o..L.....",
    ".......oL.......",
    "......oLDo......",
    "......oLDo......",
    ".....oLLDDo.....",
    ".....oLDDDo.....",
    "......oDDo......",
    "......olmo......",
    "......oDDo......",
    "......oLDo......",
    "......olmo......",
    ".....oWWWWo.....",
    ".....oWDDWo.....",
    ".....oWWWWo.....",
    ".....oWDWWo.....",
    ".....oWWWWo.....",
    "......owWo......",
    "......olmo......",
    "......olmo......",
    "......olmo......",
    "......olwo......",
    "......olmo......",
    "......omwo......",
    "......olmo......",
    ".....oomwoo.....",
    "....owmmmmwo....",
    ".....oooooo.....",
    "................",
]
# The velvet rope (16x16): a brass post on the left, the rope sagging across to the next one
ROPE = [
    "................",
    "................",
    "..oo............",
    ".oBBo...........",
    ".oBBo...........",
    "..oBoo.......oo.",
    "..oBRRo....ooRR.",
    "..oBooRRooRRoo..",
    "..oBo.ooRRoo....",
    "..oBo...oo......",
    "..oBo...........",
    "..oBo...........",
    "..oBo...........",
    ".oBBBo..........",
    "oBBBBBo.........",
    ".ooooo..........",
]

# A Zygarde Cell (16x16): a green blob, its red nucleus, a white glint (g/G green, R red)
CELL = [
    "................",
    "................",
    "................",
    "......oooo......",
    ".....oGGGGo.....",
    "....oGWGGGGo....",
    "...oGGGGGGGgo...",
    "...oGGGRRGGgo...",
    "...oGGRRRRGgo...",
    "...oGGGRRGGgo...",
    "....ogGGGGggo...",
    ".....oggggoo....",
    "......oooo......",
    "................",
    "................",
    "................",
]
# The Prison Bottle on a pedestal (16x32): a purple bottle with gold bands (P/p purple, Y/y gold) on
# a white-and-brass stand
PEDESTAL = [
    "................",
    "................",
    "................",
    "................",
    "................",
    ".......oo.......",
    "......oYYo......",
    "......oyyo......",
    ".......oo.......",
    "......oPPo......",
    ".....oPWPPo.....",
    "....oPPPPPpo....",
    "....oYYYYYyo....",
    "....oPPPPPpo....",
    "....oPWPPPpo....",
    "....oPPPPPpo....",
    "....oYYYYYyo....",
    ".....oppppo.....",
    "...ooooooooo....",
    "..oBBBBBBBBBo...",
    "...oWWWWWWWo....",
    "....oWWWWWo.....",
    "....oWWBWWo.....",
    "....oWWWWWo.....",
    "....oWWBWWo.....",
    "....oWWWWWo.....",
    "....oWWWWWo.....",
    "...oBBBBBBBo....",
    "..oBBBBBBBBBo...",
    "..ooooooooooo...",
    "................",
    "................",
]


def draw(rows, mapping):
    img = Image.new("P", (len(rows[0]), len(rows)))
    flat = [c for rgb in PALETTE for c in rgb]
    img.putpalette(flat + [0] * (768 - len(flat)))
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            img.putpixel((x, y), mapping.get(ch, 0))
    return img


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    base = {".": 0, "o": 1, "w": 2, "m": 3, "l": 4, "W": 13, "R": 14, "B": 15}
    for name, (dark, light) in COLORS.items():
        draw(STAKE, dict(base, D=dark, L=light)).save(OUT / f"stake_{name}.png")
    draw(ROPE, base).save(OUT / "velvet_rope.png")
    draw(CELL, dict(base, g=9, G=10, R=14)).save(OUT / "zygarde_cell.png")
    draw(PEDESTAL, dict(base, P=5, p=5, Y=8, y=7)).save(OUT / "prison_bottle.png")


if __name__ == "__main__":
    main()
