#!/usr/bin/env python3
"""Draw the 24x24 bag icons for the HM key items (Hedge Clippers, Flashlight, Boogie Board,
Hot Air Balloon, Can of Spinach, Sledgehammer, Rocket Boots).

Writes graphics/items/icons/<name>.png and graphics/items/icon_palettes/<name>.pal.
Each icon is drawn with palette indices, then given a dark 1px outline like the
game's own icons. Edit or replace the PNGs freely; this script is just a starting point.
"""
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUTLINE = 1
TRANSPARENT_COLOR = (180, 180, 180)  # palette slot 0 is transparent in game


def finish(img):
    """Add a 1px outline around everything that isn't transparent."""
    px = img.load()
    w, h = img.size
    edge = []
    for y in range(h):
        for x in range(w):
            if px[x, y] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and px[nx, ny] not in (0, OUTLINE):
                    edge.append((x, y))
                    break
    for x, y in edge:
        px[x, y] = OUTLINE
    return img


def save(name, img, colors):
    colors = colors + [(0, 0, 0)] * (16 - len(colors))
    flat = [c for rgb in colors for c in rgb]
    img.putpalette(flat + [0] * (768 - len(flat)))
    img.save(ROOT / f"graphics/items/icons/{name}.png")
    pal = "JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{r} {g} {b}\r\n" for r, g, b in colors)
    (ROOT / f"graphics/items/icon_palettes/{name}.pal").write_text(pal, newline="")


def new():
    return Image.new("P", (24, 24), 0)


def hedge_clippers():
    # 2 blade, 3 blade shine, 4 handle, 5 handle dark, 6 bolt
    img = new()
    d = ImageDraw.Draw(img)
    d.polygon([(11, 11), (13, 10), (19, 1), (18, 1)], fill=2)       # upper blade
    d.polygon([(11, 12), (13, 13), (23, 9), (23, 8)], fill=2)       # lower blade
    d.line([(14, 9), (18, 2)], fill=3)                               # shine
    d.line([(14, 11), (21, 9)], fill=3)
    d.line([(11, 12), (1, 13)], fill=4, width=3)                     # handles
    d.line([(11, 12), (9, 22)], fill=4, width=3)
    d.line([(1, 12), (3, 14)], fill=5, width=2)                      # grips
    d.line([(8, 21), (10, 22)], fill=5, width=2)
    d.ellipse([(10, 10), (13, 13)], fill=6)                          # pivot bolt
    save("hedge_clippers", finish(img), [TRANSPARENT_COLOR, (41, 41, 49), (172, 180, 197),
         (238, 246, 255), (222, 74, 57), (148, 41, 33), (98, 98, 115)])


def flashlight():
    # 2 body, 3 body light, 4 head, 5 lens, 6 beam, 7 switch
    img = new()
    d = ImageDraw.Draw(img)
    d.polygon([(19, 6), (23, 3), (23, 21), (19, 18)], fill=6)        # beam
    d.rectangle([(2, 10), (13, 15)], fill=2)                         # body
    d.line([(3, 11), (12, 11)], fill=3)
    d.rectangle([(6, 9), (8, 9)], fill=7)                            # switch
    d.polygon([(13, 9), (17, 7), (17, 18), (13, 16)], fill=4)        # head
    d.rectangle([(18, 7), (18, 18)], fill=5)                         # lens
    save("flashlight", finish(img), [TRANSPARENT_COLOR, (41, 41, 49), (57, 82, 164),
         (115, 148, 230), (180, 188, 205), (255, 246, 148), (255, 250, 205), (230, 57, 41)])


def boogie_board():
    # 2 board, 3 board light, 4 stripe, 5 leash
    img = new()
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([(6, 1), (17, 22)], radius=5, fill=2)
    d.rounded_rectangle([(8, 3), (10, 20)], radius=1, fill=3)
    d.rectangle([(6, 9), (17, 11)], fill=4)
    d.line([(17, 18), (20, 20), (21, 23)], fill=5)
    save("boogie_board", finish(img), [TRANSPARENT_COLOR, (41, 41, 49), (41, 156, 222),
         (148, 213, 255), (255, 213, 65), (230, 82, 57)])


def hot_air_balloon():
    # 2 red, 3 yellow, 4 rope, 5 basket, 6 basket dark, 7 shine
    img = new()
    d = ImageDraw.Draw(img)
    d.ellipse([(3, 0), (20, 15)], fill=2)
    d.polygon([(6, 12), (17, 12), (14, 17), (9, 17)], fill=2)
    d.ellipse([(8, 0), (15, 15)], fill=3)                            # stripe
    d.polygon([(9, 12), (14, 12), (13, 17), (10, 17)], fill=3)
    d.line([(6, 4), (6, 7)], fill=7)                                  # shine
    d.line([(9, 17), (9, 19)], fill=4)                                # ropes
    d.line([(14, 17), (14, 19)], fill=4)
    d.rectangle([(8, 19), (15, 23)], fill=5)                          # basket
    d.line([(8, 21), (15, 21)], fill=6)
    save("hot_air_balloon", finish(img), [TRANSPARENT_COLOR, (41, 41, 49), (222, 57, 49),
         (255, 213, 74), (139, 123, 98), (189, 131, 65), (139, 90, 41), (255, 180, 164)])


def can_of_spinach():
    # 2 metal, 3 metal light, 4 label green, 5 label dark, 6 leaf light
    img = new()
    d = ImageDraw.Draw(img)
    d.ellipse([(5, 18), (18, 22)], fill=2)                            # bottom rim
    d.rectangle([(5, 4), (18, 20)], fill=4)                           # label
    d.ellipse([(5, 1), (18, 6)], fill=2)                              # lid
    d.ellipse([(7, 2), (16, 5)], fill=3)
    d.line([(7, 7), (7, 18)], fill=6)                                 # highlight
    d.polygon([(9, 16), (12, 8), (15, 16), (12, 18)], fill=5)         # leaf
    d.line([(12, 10), (12, 17)], fill=6)
    save("can_of_spinach", finish(img), [TRANSPARENT_COLOR, (41, 41, 49), (164, 172, 180),
         (230, 238, 246), (65, 172, 74), (24, 98, 41), (156, 230, 139)])


def sledgehammer():
    # 2 head, 3 head light, 4 handle, 5 handle dark
    img = new()
    d = ImageDraw.Draw(img)
    d.line([(4, 21), (14, 9)], fill=4, width=3)                       # handle
    d.line([(4, 22), (6, 20)], fill=5, width=2)                       # grip
    d.polygon([(10, 6), (16, 0), (23, 7), (17, 13)], fill=2)          # head
    d.line([(12, 6), (16, 2)], fill=3)
    d.line([(17, 3), (21, 7)], fill=3)
    save("sledgehammer", finish(img), [TRANSPARENT_COLOR, (41, 41, 49), (106, 115, 131),
         (189, 197, 213), (189, 131, 74), (131, 82, 41)])


def rocket_boots():
    # 2 boot, 3 boot light, 4 sole, 5 flame outer, 6 flame inner
    img = new()
    d = ImageDraw.Draw(img)
    d.polygon([(10, 22), (12, 17), (14, 22)], fill=5)                 # flames
    d.polygon([(16, 22), (18, 17), (20, 22)], fill=5)
    d.polygon([(11, 22), (12, 19), (13, 22)], fill=6)
    d.polygon([(17, 22), (18, 19), (19, 22)], fill=6)
    d.rectangle([(9, 2), (15, 13)], fill=2)                           # shaft
    d.polygon([(9, 13), (21, 13), (22, 16), (9, 16)], fill=2)         # foot
    d.rectangle([(9, 16), (22, 17)], fill=4)                          # sole
    d.line([(10, 3), (10, 12)], fill=3)
    d.rectangle([(9, 6), (15, 7)], fill=4)                            # stripe
    save("rocket_boots", finish(img), [TRANSPARENT_COLOR, (41, 41, 49), (213, 57, 57),
         (255, 148, 131), (238, 238, 246), (255, 164, 41), (255, 238, 115)])


if __name__ == "__main__":
    for f in (hedge_clippers, flashlight, boogie_board, hot_air_balloon, can_of_spinach,
              sledgehammer, rocket_boots):
        f()
    print("icons written")
