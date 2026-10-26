"""Pokémon Canvas: the gym's floors, wall styles and pool rims, drawn in code in the shared
decoration palettes (decor_art.PALETTES: slots 8-10), for tools/gym_hack/build_gym_decor.py.

Floors are 2x2-block patterns (the game picks the block by the square's position, so a pattern
can be bigger than one square). Walls and pool water join up by themselves: each 8x8 quarter of a
square is drawn from what's beside it (see QUARTER_CASES), and a wall's front face goes in the
square below its bottom edge.
"""
import random

from decor_art import Canvas

# ---------------------------------------------------------------------------------------------
# Floors: key, name, price, unlock (a flag or None), description, flags
# ---------------------------------------------------------------------------------------------
FLOORS = [
    # (the first is the gym's own floor: it comes with the gym)
    ("GYM", "GYM TILES", 0, None, "The GYM's own teal tiles.", ""),
    ("WOOD", "WOODEN FLOOR", 2500, None, "Polished wooden boards.", ""),
    ("STONE", "STONE TILES", 2500, None, "Big gray flagstones.", ""),
    ("CARPET", "RED CARPET", 3000, None, "A plush red carpet.", ""),
    ("SAND", "SAND FLOOR", 2000, "FLAG_BADGE01_GET", "Fine sand, like a beach.", ""),
    ("GRATING", "METAL GRATING", 3000, "FLAG_BADGE03_GET", "Industrial steel grating.", ""),
    ("GRASS", "GRASS FLOOR", 2500, "FLAG_BADGE04_GET", "Soft green grass indoors.", ""),
    ("STEEL", "STEEL FLOOR", 3500, "FLAG_BADGE06_GET", "Riveted steel plates.", ""),
    ("LAVA", "LAVA FLOOR", 6000, "FLAG_BADGE07_GET", "Molten rock. Nobody can cross it!", "LAVA"),
    ("ICE", "ICE FLOOR", 4000, "FLAG_SYS_GAME_CLEAR", "Frozen solid and shiny.", ""),
]

# Wall styles: key, name, price, unlock, description
WALLS = [
    ("GYM", "GYM WALLS", 0, None, "The GYM's own gray and peach walls."),
    ("BRICK", "BRICK WALLS", 4000, None, "Sturdy red bricks."),
    ("WOOD", "WOODEN WALLS", 4000, None, "Warm wooden panels."),
    ("STONE", "STONE WALLS", 4000, "FLAG_BADGE01_GET", "Rough gray stone blocks."),
    ("HEDGE", "HEDGE WALLS", 5000, "FLAG_BADGE04_GET", "Neatly trimmed green hedges."),
    ("STEEL", "STEEL WALLS", 6000, "FLAG_BADGE06_GET", "Riveted steel plating."),
]


def _rng(key):
    return random.Random(sum(map(ord, key)) * 7919)


def floor_canvas(key):
    """A 32x32 pattern (2x2 squares)"""
    c = Canvas(2, 2)
    r = _rng(key)
    W_, H_ = 32, 32
    if key == "WOOD":
        for y in range(H_):
            for x in range(W_):
                band = y // 8
                seam = (x + band * 11) % 32 == 0
                c.set(x, y, "DB" if y % 8 == 7 or seam else ("T" if (x * 3 + y * 7 + band) % 13 == 0 else "B"))
        for y in range(0, H_, 8):
            for x in range(W_):
                if (x + y) % 9 == 3:
                    c.set(x, y + 3, "T")
    elif key == "STONE":
        c.rect(0, 0, 31, 31, "L")
        for (x0, y0, x1, y1) in ((0, 0, 14, 14), (16, 0, 31, 9), (16, 11, 31, 30), (0, 16, 14, 30)):
            c.rect(x0, y0, x1, y1, "L")
            c.rect(x1, y0, x1, y1, "G")
            c.rect(x0, y1, x1, y1, "G")
            c.rect(x0, y0, x1 - 1, y0, "W")
        for _ in range(40):
            c.set(r.randrange(32), r.randrange(32), "ST")
        for y in (15, 31):
            c.rect(0, y, 31, y, "DK")
        c.rect(15, 0, 15, 31, "DK")
        c.rect(15, 10, 31, 10, "DK")
    elif key == "CARPET":
        c.rect(0, 0, 31, 31, "R")
        for y in range(32):
            for x in range(32):
                if (x + y) % 4 == 0:
                    c.set(x, y, "DR")
                elif (x - y) % 8 == 0:
                    c.set(x, y, "SA")
    elif key == "SAND":
        c.rect(0, 0, 31, 31, "BE")
        for _ in range(110):
            c.set(r.randrange(32), r.randrange(32), r.choice(("T", "PY", "T", "Y")))
    elif key == "GRATING":
        c.rect(0, 0, 31, 31, "K")
        for y in range(32):
            for x in range(32):
                if x % 4 in (1, 2) and y % 4 != 0:
                    c.set(x, y, "G" if x % 4 == 1 else "DK")
        for x in range(32):
            c.set(x, 0, "L")
            c.set(x, 16, "L")
    elif key == "GRASS":
        c.rect(0, 0, 31, 31, "GR")
        for _ in range(80):
            x, y = r.randrange(32), r.randrange(32)
            c.set(x, y, "DG")
            c.set(x, y - 1, "DG" if r.random() < 0.5 else "LG")
        for _ in range(30):
            c.set(r.randrange(32), r.randrange(32), "LG")
    elif key == "STEEL":
        c.rect(0, 0, 31, 31, "ST")
        for (x0, y0) in ((0, 0), (16, 0), (0, 16), (16, 16)):
            c.rect(x0, y0, x0 + 15, y0, "L")
            c.rect(x0, y0, x0, y0 + 15, "L")
            c.rect(x0 + 15, y0, x0 + 15, y0 + 15, "DK")
            c.rect(x0, y0 + 15, x0 + 15, y0 + 15, "DK")
            for dx, dy in ((3, 3), (12, 3), (3, 12), (12, 12)):
                c.set(x0 + dx, y0 + dy, "W")
                c.set(x0 + dx + 1, y0 + dy + 1, "G")
        for y in range(32):
            for x in range(32):
                if (x * 5 + y * 3) % 17 == 0 and c.get(x, y) == "ST":
                    c.set(x, y, "L")
    elif key == "LAVA":
        c.rect(0, 0, 31, 31, "R")
        for y in range(32):
            for x in range(32):
                v = (x * 7 + y * 13 + (x * y) % 11) % 23
                if v < 3:
                    c.set(x, y, "Y")
                elif v < 8:
                    c.set(x, y, "O")
                elif v > 19:
                    c.set(x, y, "DR")
        for _ in range(14):                       # cooled crust
            x, y = r.randrange(32), r.randrange(32)
            c.rect(x, y, x + 1, y, "K")
    elif key == "ICE":
        c.rect(0, 0, 31, 31, "IC")
        for _ in range(60):
            c.set(r.randrange(32), r.randrange(32), "W")
        for k in range(3):                        # cracks
            x, y = r.randrange(32), r.randrange(32)
            for _ in range(12):
                c.set(x, y, "SK")
                x += r.choice((-1, 0, 1))
                y += 1
        for y in range(32):
            for x in range(32):
                if (x + y) % 16 == 0 and c.get(x, y) == "IC":
                    c.set(x, y, "W")
    else:
        raise KeyError(key)
    return c


def shadow_canvas():
    """The back wall's shadow over the floor below it (the top 3 rows of a square)"""
    c = Canvas(1, 1)
    for y in range(3):
        for x in range(16):
            if y < 2 or x % 2 == 0:
                c.set(x, y, "DK")
    return c


# ---------------------------------------------------------------------------------------------
# Quarter cases: an 8x8 quarter of a square that joins with its neighbors, drawn as the TOP LEFT
# quarter (the others are it flipped). For the top left: the neighbor above (U), to the left (L)
# and up-left (D).
#   FULL   U and L and the diagonal all join: no edge
#   INNER  U and L join, the diagonal doesn't: a notch in the corner
#   VERT   U joins, L doesn't: an edge on the left
#   HORZ   L joins, U doesn't: an edge on the top
#   OUTER  neither: a rounded corner
# ---------------------------------------------------------------------------------------------
QUARTER_CASES = ("FULL", "INNER", "VERT", "HORZ", "OUTER")


def quarter_case(u, l, diag):
    if u and l:
        return "FULL" if diag else "INNER"
    if u:
        return "VERT"
    if l:
        return "HORZ"
    return "OUTER"


# The wall styles' colors: top face (light, fill, dark, outline), front face (light, fill, dark,
# mortar/line, outline), and a texture name
WALL_COLORS = {
    "GYM":   dict(top=("W", "L", "G", "DK"), face=("BE", "T", "B", "B", "DK"), tex="gym"),
    "BRICK": dict(top=("SA", "T", "B", "DB"), face=("SA", "R", "DR", "BE", "DB"), tex="brick"),
    "WOOD":  dict(top=("BE", "T", "B", "DB"), face=("T", "B", "DB", "DB", "K"), tex="wood"),
    "STONE": dict(top=("W", "L", "G", "K"), face=("L", "G", "DK", "DK", "K"), tex="stone"),
    "HEDGE": dict(top=("LG", "GR", "DG", "K"), face=("LG", "GR", "DG", "DG", "K"), tex="hedge"),
    "STEEL": dict(top=("W", "ST", "DK", "K"), face=("L", "ST", "DK", "G", "K"), tex="steel"),
}


def _top_texture(c, style, x0, y0):
    """The top face's fill over an 8x8 area"""
    light, fill, dark, _ = WALL_COLORS[style]["top"]
    tex = WALL_COLORS[style]["tex"]
    for y in range(8):
        for x in range(8):
            col = fill
            if tex == "hedge" and (x * 3 + y * 5) % 7 in (0, 3):
                col = light if (x + y) % 2 else dark
            elif tex == "brick" and (y % 4 == 3):
                col = dark
            elif tex == "wood" and x % 4 == 3:
                col = dark
            elif tex == "stone" and (x * 5 + y * 3) % 11 == 0:
                col = dark
            elif tex == "steel" and (x, y) in ((2, 2), (6, 6)):
                col = light
            elif tex == "gym" and y == 3:
                col = light
            c.set(x0 + x, y0 + y, col)


def wall_top_quarter(style, case):
    """The top left quarter of a wall square's top face (8x8, in a 16x16 canvas's corner)"""
    light, fill, dark, edge = WALL_COLORS[style]["top"]
    c = Canvas(1, 1)
    _top_texture(c, style, 0, 0)
    if case in ("HORZ", "OUTER"):
        c.rect(0, 0, 7, 0, edge)
        c.rect(0, 1, 7, 1, light)
    if case in ("VERT", "OUTER"):
        c.rect(0, 0, 0, 7, edge)
        c.rect(1, 1 if case == "OUTER" else 0, 1, 7, light)
    if case == "OUTER":
        c.set(1, 1, edge)
        c.set(0, 0, None)
    if case == "INNER":
        c.set(0, 0, edge)
        c.set(1, 0, edge)
        c.set(0, 1, edge)
    return c


def wall_face(style, left_cap, right_cap):
    """The front face below a wall's bottom edge (16x16)"""
    light, fill, dark, line, edge = WALL_COLORS[style]["face"]
    tex = WALL_COLORS[style]["tex"]
    c = Canvas(1, 1)
    c.rect(0, 0, 15, 15, fill)
    if tex == "brick":
        for y in range(16):
            for x in range(16):
                if y % 4 == 3 or (x + (4 if (y // 4) % 2 else 0)) % 8 == 7:
                    c.set(x, y, line)
    elif tex == "wood":
        for x in range(0, 16, 4):
            c.rect(x + 3, 0, x + 3, 15, line)
        c.rect(0, 1, 15, 1, light)
    elif tex == "stone":
        for (x0, y0, x1, y1) in ((0, 0, 7, 6), (8, 0, 15, 6), (0, 8, 4, 14), (5, 8, 15, 14)):
            c.rect(x0, y0, x1, y0, light)
            c.rect(x1, y0, x1, y1, line)
            c.rect(x0, y1, x1, y1, line)
    elif tex == "hedge":
        for y in range(16):
            for x in range(16):
                v = (x * 7 + y * 5 + (x * y) % 5) % 9
                c.set(x, y, light if v == 0 else (dark if v in (4, 7) else fill))
    elif tex == "steel":
        c.rect(0, 7, 15, 7, line)
        for x in (2, 13):
            for y in (2, 11):
                c.set(x, y, light)
    elif tex == "gym":
        for x in range(16):                       # the gym's peaks, as on its back wall
            h = 3 - abs((x % 8) - 3)
            for y in range(12 - h, 13):
                c.set(x, y, light)
    c.rect(0, 0, 15, 0, dark)                     # under the top face's edge
    c.rect(0, 14, 15, 14, dark)                   # the baseboard
    c.rect(0, 15, 15, 15, edge)
    if left_cap:
        c.rect(0, 0, 0, 15, edge)
    if right_cap:
        c.rect(15, 0, 15, 15, edge)
    return c


def water_quarter(case):
    """The top left quarter of a pool square: water, and a stone rim where it meets the floor"""
    c = Canvas(1, 1)
    for y in range(8):
        for x in range(8):
            c.set(x, y, "SK" if (x + 2 * y) % 7 else "IC")
        if y % 4 == 2:
            for x in range(1, 5):
                c.set((x + y) % 8, y, "W")
    rim, rim_dark, edge = "L", "G", "DK"
    if case in ("HORZ", "OUTER"):
        c.rect(0, 0, 7, 2, rim)
        c.rect(0, 2, 7, 2, rim_dark)
        c.rect(0, 0, 7, 0, edge)
    if case in ("VERT", "OUTER"):
        c.rect(0, 0, 2, 7, rim)
        c.rect(2, 2 if case == "OUTER" else 0, 2, 7, rim_dark)
        c.rect(0, 0, 0, 7, edge)
    if case == "INNER":
        c.rect(0, 0, 2, 2, rim)
        c.set(2, 2, rim_dark)
        c.set(0, 0, edge)
    if case == "OUTER":
        c.set(0, 0, None)
        c.set(1, 0, edge)
        c.set(0, 1, edge)
    return c
