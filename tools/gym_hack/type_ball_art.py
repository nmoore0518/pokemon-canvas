#!/usr/bin/env python3
"""Pokémon Canvas: the 18 Type Balls' art, drawn in code.

Writes, per type:
  graphics/items/icons/gym/<type>_ball.png + graphics/items/icon_palettes/gym/<type>_ball.pal   (bag icon, 24x24)
  graphics/balls/type_<type>.png                                                              (battle/summary ball, 16x48:
                                                                                                the Poké Ball's frames, recolored)
Run with an output path for a preview sheet:  python3 tools/gym_hack/type_ball_art.py preview.png
"""
import math
import os
import sys

from PIL import Image

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")

# type: (top ramp dark -> highlight, emblem, emblem shade)
TYPES = {
    "normal":   ([(112, 104, 72), (168, 160, 120), (200, 192, 152), (232, 228, 200)], (255, 255, 255), (120, 112, 84)),
    "fighting": ([(104, 24, 24), (176, 40, 32), (216, 72, 56), (248, 136, 112)], (248, 216, 160), (104, 24, 24)),
    "flying":   ([(88, 72, 160), (136, 120, 224), (168, 152, 240), (208, 200, 255)], (255, 255, 255), (88, 72, 160)),
    "poison":   ([(80, 24, 88), (144, 56, 152), (184, 88, 192), (224, 144, 224)], (184, 240, 72), (64, 104, 24)),
    "ground":   ([(136, 104, 40), (200, 168, 80), (224, 196, 112), (248, 228, 160)], (112, 72, 32), (232, 208, 136)),
    "rock":     ([(96, 80, 32), (160, 136, 56), (192, 168, 88), (224, 208, 136)], (72, 60, 32), (208, 192, 128)),
    "bug":      ([(88, 104, 16), (152, 176, 24), (184, 208, 56), (224, 240, 120)], (56, 64, 16), (200, 224, 96)),
    "ghost":    ([(56, 40, 96), (104, 80, 152), (136, 112, 184), (176, 160, 216)], (224, 232, 255), (56, 40, 96)),
    "steel":    ([(96, 96, 120), (160, 160, 184), (192, 192, 212), (232, 232, 244)], (72, 72, 96), (216, 216, 232)),
    "fire":     ([(160, 48, 8), (232, 104, 32), (248, 144, 56), (255, 200, 120)], (255, 232, 72), (184, 64, 8)),
    "water":    ([(32, 72, 168), (72, 128, 232), (112, 160, 248), (176, 208, 255)], (216, 248, 255), (32, 72, 168)),
    "grass":    ([(40, 112, 32), (88, 176, 64), (128, 208, 88), (184, 240, 144)], (232, 255, 208), (40, 112, 32)),
    "electric": ([(184, 136, 0), (240, 200, 32), (248, 224, 88), (255, 248, 168)], (56, 48, 40), (176, 136, 0)),
    "psychic":  ([(176, 40, 96), (240, 88, 136), (248, 128, 168), (255, 184, 208)], (255, 255, 255), (176, 40, 96)),
    "ice":      ([(64, 152, 168), (128, 208, 216), (168, 228, 232), (224, 248, 248)], (255, 255, 255), (64, 152, 168)),
    "dragon":   ([(64, 24, 168), (112, 64, 240), (144, 104, 248), (192, 168, 255)], (248, 208, 64), (128, 88, 8)),
    "dark":     ([(40, 32, 32), (88, 72, 64), (120, 100, 88), (160, 140, 128)], (232, 56, 48), (120, 16, 16)),
    "fairy":    ([(200, 96, 136), (240, 152, 180), (248, 184, 204), (255, 224, 236)], (255, 255, 255), (200, 96, 136)),
}

# 7x6 emblems, centered in the top half ("#" = emblem, "s" = its shade)
EMBLEMS = {
    "normal":   ["  ###  ", " #   # ", " #   # ", " #   # ", "  ###  ", "       "],
    "fighting": [" # # # ", "#######", "#######", "#s#####", " ##### ", "  ###  "],
    "flying":   ["#     #", "##   ##", " ## ## ", "  ###  ", "   #   ", "       "],
    "poison":   [" ##### ", "#######", "#s###s#", "#######", " #s#s# ", " # # # "],
    "ground":   ["   #   ", "  ###  ", "  #s#  ", " ##### ", " #s#s# ", "#######"],
    "rock":     ["  ##   ", " ####  ", " ##s## ", "###s## ", "####s##", "       "],
    "bug":      [" #   # ", "  # #  ", "  ###  ", " ##s## ", "  ###  ", "   s   "],
    "ghost":    ["  ###  ", " ##### ", " #s#s# ", " ##### ", " ##### ", " # # # "],
    "steel":    ["  ###  ", " ##### ", "## s ##", "##s s##", " ##### ", "  ###  "],
    "fire":     ["   #   ", "  ##   ", "  ### #", " ##s## ", " #sss# ", "  ###  "],
    "water":    ["   #   ", "   #   ", "  ###  ", " ##### ", " ##s## ", "  ###  "],
    "grass":    ["     ##", "   ####", "  ###s#", " ##s## ", " #s##  ", "#      "],
    "electric": ["   ### ", "  ###  ", " ##### ", "   ##  ", "  ##   ", "  #    "],
    "psychic":  ["       ", "  ###  ", " # s # ", "# sss #", " # s # ", "  ###  "],
    "ice":      ["#  #  #", " # # # ", "  ###  ", "#######", "  ###  ", " # # # "],
    "dragon":   ["#     #", "##   ##", "###s###", " ##### ", "  ###  ", "   #   "],
    "dark":     ["   ### ", "  ##   ", " ##    ", " ##    ", "  ##   ", "   ### "],
    "fairy":    ["   #   ", "   #   ", "  ###  ", "#######", "  ###  ", "   #   "],
}

OUTLINE = (32, 32, 40)
WHITE = [(255, 255, 255), (224, 224, 232), (168, 168, 184)]   # bottom half, light to shadow
BAND = (48, 48, 56)


def icon(t):
    ramp, emb, embs = TYPES[t]
    px = [[None] * 24 for _ in range(24)]
    cx, cy, r = 11.5, 12.5, 10.9
    lx, ly = -0.55, -0.65          # light from the upper left
    for y in range(24):
        for x in range(24):
            dx, dy = x - cx, y - cy
            d = math.hypot(dx, dy)
            if d > r:
                continue
            if d > r - 1.15:
                px[y][x] = OUTLINE
                continue
            nz = math.sqrt(max(0.0, 1 - (d / r) ** 2))
            lit = (dx / r) * lx + (dy / r) * ly + nz * 0.6
            if abs(dy) <= 1.0:
                px[y][x] = BAND
            elif dy < 0:
                px[y][x] = ramp[0] if lit < 0.2 else ramp[1] if lit < 0.62 else ramp[2] if lit < 1.05 else ramp[3]
            else:
                px[y][x] = WHITE[2] if lit < 0.15 else WHITE[1] if lit < 0.45 else WHITE[0]
    # the button: band-colored ring, white middle
    for y in range(24):
        for x in range(24):
            d = math.hypot(x - cx, y - cy)
            if d <= 2.9:
                px[y][x] = BAND if d > 1.9 else WHITE[0]
    # the emblem
    rows = EMBLEMS[t]
    x0, y0 = 8, 4
    cells = {(x0 + i, y0 + j) for j, row in enumerate(rows) for i, ch in enumerate(row) if ch != " "}
    lum = lambda c: 0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2]
    rim = ramp[3] if lum(emb) < lum(ramp[1]) else ramp[0]
    for (x, y) in cells:          # a dark rim so the emblem reads on any shade of the ball
        for ax, ay in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if (ax, ay) not in cells and px[ay][ax] in ramp:
                px[ay][ax] = rim
    for j, row in enumerate(rows):
        for i, ch in enumerate(row):
            if ch == "#":
                px[y0 + j][x0 + i] = emb
            elif ch == "s":
                px[y0 + j][x0 + i] = embs
    return px


def write_icon(t, px):
    colors = []
    for row in px:
        for c in row:
            if c is not None and c not in colors:
                colors.append(c)
    assert len(colors) <= 15, (t, len(colors))
    pal = [(0, 0, 0)] + colors + [(0, 0, 0)] * (15 - len(colors))
    img = Image.new("P", (24, 24))
    img.putpalette([v for p in pal for v in p])
    for y in range(24):
        for x in range(24):
            img.putpixel((x, y), 0 if px[y][x] is None else colors.index(px[y][x]) + 1)
    name = t if t in LAB else f"{t}_ball"
    img.save(os.path.join(ROOT, f"graphics/items/icons/gym/{name}.png"), bits=4)
    with open(os.path.join(ROOT, f"graphics/items/icon_palettes/gym/{name}.pal"), "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n16\n")
        for r, g, b in pal:
            f.write(f"{r} {g} {b}\n")


def battle_ball(t):
    """The Poké Ball's battle frames with its red swapped for the type's colors (indices 1-4 are the red ramp)."""
    src = Image.open(os.path.join(ROOT, "graphics/balls/poke.png"))
    pal = src.getpalette()[:48]
    ramp = TYPES[t][0]
    for i, c in zip((1, 2, 3, 4), ramp):
        pal[i * 3:i * 3 + 3] = list(c)
    out = src.copy()
    out.putpalette(pal)
    out.save(os.path.join(ROOT, f"graphics/balls/type_{t}.png"), bits=4)
    return out


# ---- the Synthesizer's two ingredients (bag icons) ----
LAB = {
    "strong_acid": ({
        "o": (40, 40, 48), "g": (200, 216, 224), "G": (240, 248, 255), "c": (120, 112, 104), "C": (168, 160, 150),
        "a": (96, 176, 24), "A": (160, 232, 48), "L": (224, 255, 128), "b": (56, 112, 16)}, [
        "                        ",
        "         oooooo         ",
        "         oCCCco         ",
        "         oCCCco         ",
        "          oggo          ",
        "          oGgo          ",
        "          oGgo          ",
        "          oGgo          ",
        "         oGgggo         ",
        "         oGgggo         ",
        "        oGgggggo        ",
        "        oAAAAAAo        ",
        "       oALAAAAAao       ",
        "       oALAAAAAao       ",
        "      oALAAbAAAAao      ",
        "      oAAAAAAAbAao      ",
        "     oAAAbAAAAAAAao     ",
        "     oAAAAAAAAbAAao     ",
        "    oaAAAAAAAAAAAaao    ",
        "    oaaaaaaaaaaaaaao    ",
        "     oooooooooooooo     ",
        "                        ",
        "                        ",
        "                        "]),
    "apple_juice": ({
        "o": (48, 32, 24), "g": (216, 224, 232), "G": (248, 252, 255), "k": (56, 128, 40), "K": (96, 176, 64),
        "j": (216, 136, 24), "J": (248, 184, 56), "L": (255, 224, 128), "r": (200, 40, 40), "R": (240, 88, 72), "w": (255, 255, 255)}, [
        "                        ",
        "          oooo          ",
        "          okKo          ",
        "          okKo          ",
        "          oggo          ",
        "         oGggo          ",
        "        oGggggo         ",
        "       oGgggggggo       ",
        "       oJLJJJJJjo       ",
        "       oJLJJJJJjo       ",
        "       oJLwwwwwjo       ",
        "       oJwwkKwwjo       ",
        "       oJwRRRRwjo       ",
        "       oJwRwRRwjo       ",
        "       oJwRRRrwjo       ",
        "       oJwwrrwwjo       ",
        "       oJLwwwwwjo       ",
        "       oJLJJJJJjo       ",
        "       oJLJJJJjjo       ",
        "       oJJJJJjjjo       ",
        "        oooooooo        ",
        "                        ",
        "                        ",
        "                        "]),
    "dads_notebook": ({
        "o": (40, 28, 20), "b": (112, 72, 40), "B": (148, 100, 56), "h": (184, 136, 80), "p": (240, 232, 208),
        "P": (208, 196, 168), "s": (64, 44, 28), "g": (200, 168, 72)}, [
        "                        ",
        "                        ",
        "    oooooooooooooooo    ",
        "    obBBBBBBBBBBBBBbo   ",
        "    obBhhhhhhhhhhhBbop  ",
        "    obBhBBBBBBBBBhBbop  ",
        "    obBhBggggggBBhBbop  ",
        "    obBhBBBBBBBBBhBbop  ",
        "    obBhhhhhhhhhhhBbop  ",
        "    obBBBBBBBBBBBBBbop  ",
        "   ssssobBBBBBBBBBBbop  ",
        "   sggsobBBBBBBBBBBbop  ",
        "   ssssobBBBBBBBBBBbop  ",
        "    obBBBBBBBBBBBBBbop  ",
        "    obBBBBBBBBBBBBBbop  ",
        "    obBBBBBBBBBBBBBbop  ",
        "    obBBBBBBBBBBBBBbop  ",
        "    obBBBBBBBBBBBBBboP  ",
        "    obbbbbbbbbbbbbbboP  ",
        "    ooooooooooooooooopP ",
        "      PPPPPPPPPPPPPPP   ",
        "                        ",
        "                        ",
        "                        "]),
}


def lab_icon(name):
    colors, rows = LAB[name]
    return [[colors[ch] if ch != " " else None for ch in row] for row in rows]


def main():
    os.makedirs(os.path.join(ROOT, "graphics/items/icons/gym"), exist_ok=True)
    os.makedirs(os.path.join(ROOT, "graphics/items/icon_palettes/gym"), exist_ok=True)
    made = []
    for t in TYPES:
        px = icon(t)
        write_icon(t, px)
        made.append((t, px, battle_ball(t)))
    for name in LAB:
        px = lab_icon(name)
        write_icon(name, px)
        made.append((name.replace("_", " "), px, None))
    if len(sys.argv) > 1:
        from PIL import ImageDraw
        cols, cw, ch = 6, 200, 190
        sheet = Image.new("RGB", (cols * cw, 4 * ch), (244, 244, 236))
        d = ImageDraw.Draw(sheet)
        for n, (t, px, bb) in enumerate(made):
            x0, y0 = (n % cols) * cw, (n // cols) * ch
            img = Image.new("RGB", (24, 24), (244, 244, 236))
            for y in range(24):
                for x in range(24):
                    if px[y][x]:
                        img.putpixel((x, y), px[y][x])
            sheet.paste(img.resize((120, 120), Image.NEAREST), (x0 + 10, y0 + 10))
            b = bb.convert("RGBA") if bb else None
            for k in range(2 if bb else 0):
                fr = b.crop((0, k * 16, 16, k * 16 + 16)).resize((48, 48), Image.NEAREST)
                sheet.paste(fr, (x0 + 140, y0 + 14 + k * 56), fr)
            d.text((x0 + 12, y0 + 140), t.title() + ("" if bb is None else " Ball"), fill=(20, 20, 20))
        sheet.save(sys.argv[1])


if __name__ == "__main__":
    main()
