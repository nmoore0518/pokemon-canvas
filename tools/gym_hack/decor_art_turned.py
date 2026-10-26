#!/usr/bin/env python3
"""Pixel art for decorations turned around: their sides and backs (START turns a piece while
decorating; see src/gym_decor.c). Drawn the same way as tools/gym_hack/decor_art.py.

TURNS lists, for each piece, the ways it can face after the front, in the order START shows
them: the name of a drawing here, "mirror" (the front, flipped: for pieces that face one way),
or ("mirror", name) (a drawing here, flipped). A piece that looks the same every way round
(round things, trophies...) isn't listed.

Run it to get a picture of every piece, every way round:  python3 tools/gym_hack/decor_art_turned.py out.png
"""
import math

from decor_art import C


# ---- Pieces from the indoor tileset ---------------------------------------
def chair_front():
    c = C(1, 1)
    c.fill_box(3, 1, 12, 7, "BL", "SK", "NV")          # the back, behind the seat
    c.fill_box(2, 7, 13, 12, "SK", "IC", "BL")         # the seat
    c.rect(3, 13, 4, 15, "NV")
    c.rect(11, 13, 12, 15, "NV")
    return c


def chair_back():
    c = C(1, 1)
    c.fill_box(2, 7, 13, 11, "BL", None, "NV")         # the seat, behind
    c.rect(3, 12, 4, 15, "NV")
    c.rect(11, 12, 12, 15, "NV")
    c.fill_box(3, 0, 12, 10, "BL", "SK", "NV")         # the back, in front
    c.rect(5, 3, 10, 3, "NV")
    return c


def bookshelf_back():
    c = C(2, 2)
    c.fill_box(1, 0, 30, 6, "GR", "LG", "DG")          # its top
    c.fill_box(1, 6, 30, 29, "L", "W", "G")            # the plain back
    for x in (8, 16, 24):
        c.rect(x, 7, x, 28, "G")
    c.rect(2, 29, 4, 31, "DK")
    c.rect(27, 29, 29, 31, "DK")
    return c


def bookshelf_side():
    c = C(1, 2)
    c.fill_box(2, 0, 13, 6, "GR", "LG", "DG")
    c.fill_box(2, 6, 13, 29, "L", "W", "G")
    c.rect(2, 7, 3, 28, "DK")                          # the books' edge, at the front
    for y in (11, 18, 25):
        c.rect(3, y, 12, y, "G")
    c.rect(3, 29, 5, 31, "DK")
    c.rect(10, 29, 12, 31, "DK")
    return c


def cabinet_back():
    c = C(2, 2)
    c.fill_box(1, 0, 30, 6, "Y", "PY", "DO")
    c.fill_box(1, 6, 30, 29, "T", "BE", "B")
    c.rect(15, 7, 16, 28, "B")
    c.rect(2, 29, 4, 31, "DB")
    c.rect(27, 29, 29, 31, "DB")
    return c


def cabinet_side():
    c = C(1, 2)
    c.fill_box(2, 0, 13, 6, "Y", "PY", "DO")
    c.fill_box(2, 6, 13, 29, "T", "BE", "B")
    c.rect(2, 9, 3, 18, "IC")                          # the glass doors' edge
    c.rect(3, 29, 5, 31, "DB")
    c.rect(10, 29, 12, 31, "DB")
    return c


def tv_back():
    c = C(1, 2)
    c.fill_box(1, 2, 14, 17, "L", "W", "G")
    for y in (6, 9, 12):
        c.rect(4, y, 11, y, "G")                        # vents
    c.rect(7, 17, 8, 20, "DK")
    c.fill_box(1, 20, 14, 30, "ST", "L", "DK")         # the stand
    return c


def tv_side():
    c = C(1, 2)
    c.fill_box(4, 2, 12, 17, "L", "W", "G")
    c.rect(4, 3, 5, 16, "DK")                          # the screen's edge
    c.rect(7, 17, 9, 20, "DK")
    c.fill_box(2, 20, 13, 30, "ST", "L", "DK")
    return c


# ---- NORMAL ---------------------------------------------------------------
def couch_side():
    c = C(2, 3)
    c.fill_box(19, 2, 29, 44, "B", "T", "DB")          # the back, on its right
    c.fill_box(3, 2, 27, 10, "B", "T", "DB")           # arms
    c.fill_box(3, 36, 27, 44, "B", "T", "DB")
    c.fill_box(4, 10, 19, 36, "T", "BE", "B")          # the seat
    c.line(5, 23, 18, 23, "B")
    c.fill_box(21, 11, 27, 21, "SA", None, "R")        # cushions
    c.fill_box(21, 25, 27, 35, "SA", None, "R")
    c.rect(4, 45, 6, 47, "DB")
    c.rect(25, 45, 27, 47, "DB")
    return c


def couch_back():
    c = C(3, 2)
    c.fill_box(0, 12, 8, 26, "B", "T", "DB")           # arms
    c.fill_box(39, 12, 47, 26, "B", "T", "DB")
    c.fill_box(2, 2, 45, 27, "B", "T", "DB")           # the back
    for x in (12, 24, 35):
        c.rect(x, 4, x, 25, "DB")
    c.rect(4, 28, 6, 31, "DB")
    c.rect(41, 28, 43, 31, "DB")
    return c


def grandfather_clock_side():
    c = C(1, 2)
    c.fill_box(4, 1, 11, 30, "B", "T", "DB")
    c.rect(4, 4, 5, 12, "PY")                          # the face, seen edge on
    c.rect(3, 0, 12, 1, "DB")
    c.rect(6, 15, 9, 27, "DB")
    return c


def grandfather_clock_back():
    c = C(1, 2)
    c.fill_box(2, 1, 13, 30, "B", "T", "DB")
    c.rect(5, 4, 10, 12, "DB")
    c.rect(5, 15, 10, 27, "DB")
    c.rect(1, 0, 14, 1, "DB")
    return c


def vending_machine_side():
    c = C(1, 2)
    c.fill_box(2, 1, 14, 30, "R", "SA", "DR")
    c.rect(2, 4, 3, 21, "IC")                          # its window's edge
    c.rect(5, 6, 11, 6, "DR")
    c.rect(5, 26, 11, 27, "DR")
    return c


def vending_machine_back():
    c = C(2, 2)
    c.fill_box(2, 1, 29, 30, "R", "SA", "DR")
    for y in range(6, 14, 2):
        c.rect(8, y, 23, y, "DR")                      # vents
    c.rect(6, 22, 11, 26, "L")
    c.frame(6, 22, 11, 26, "K")
    return c


def arcade_cabinet_side():
    c = C(1, 2)
    c.poly([(5, 1), (14, 1), (14, 31), (5, 31), (5, 19), (1, 17), (1, 14), (5, 13)], "P")
    c.rect(6, 2, 13, 3, "LV")
    c.rect(5, 2, 5, 12, "K")                           # the screen's edge
    c.rect(1, 14, 4, 15, "DK")                         # the controls
    c.ellipse(2, 13, 1, 1, "R")
    c.rect(13, 2, 13, 30, "DP")
    c.outline()
    return c


def arcade_cabinet_back():
    c = C(1, 2)
    c.fill_box(1, 1, 14, 30, "P", "LV", "DP")
    for y in (6, 8, 10):
        c.rect(4, y, 11, y, "DP")
    c.rect(6, 24, 9, 26, "K")
    return c


# ---- FIGHTING ---------------------------------------------------------------
def barbell_rack_side():
    c = C(1, 2)
    c.rect(6, 6, 9, 28, "DK")
    c.rect(2, 28, 13, 31, "DK")
    c.ellipse(8, 9, 6, 6, "R", "DR")                    # the weights, end on
    c.ellipse(8, 9, 2, 2, "DK")
    c.rect(7, 8, 8, 10, "L")
    c.outline()
    return c


def wooden_dummy_side():
    c = C(1, 2)
    c.fill_box(5, 1, 10, 29, "B", "T", "DB")
    c.ellipse(7.5, 4, 3, 3, "T", "K")
    c.rect(1, 9, 5, 10, "DB")                          # its arms, pointing this way
    c.rect(2, 16, 5, 17, "DB")
    c.rect(3, 29, 12, 31, "DB")
    return c


def gong_side():
    c = C(1, 2)
    c.rect(6, 2, 9, 30, "DR")
    c.fill_box(4, 1, 11, 4, "R", None, "DR")
    c.ellipse(8, 15, 2.5, 10, "GO", "DGO")             # the gong, edge on
    c.rect(2, 29, 13, 31, "DB")
    return c


def gong_back():
    c = C(2, 2)
    c.rect(2, 2, 3, 30, "DR")
    c.rect(28, 2, 29, 30, "DR")
    c.rect(1, 1, 30, 3, "R")
    c.frame(1, 1, 30, 3, "K")
    c.ellipse(16, 15, 10, 10, "DGO", "K")
    c.ellipse(16, 15, 5, 5, "GO")
    c.rect(0, 29, 6, 31, "DB")
    c.rect(25, 29, 31, 31, "DB")
    return c


# ---- FLYING ---------------------------------------------------------------
def wind_turbine_side():
    c = C(1, 3)
    c.poly([(6, 14), (10, 14), (11, 46), (5, 46)], "W")
    c.line(10, 14, 11, 46, "L")
    c.ellipse(9, 11, 4, 2, "L")                         # the body, pointing this way
    c.ellipse(4, 11, 1.5, 1.5, "G")
    c.poly([(3, 0), (5, 0), (5, 22), (3, 22)], "W")     # the blades, edge on
    c.outline("G")
    return c


# ---- POISON ---------------------------------------------------------------
def chemistry_set_side():
    c = C(1, 2)
    c.fill_box(0, 16, 15, 21, "B", "T", "DB")
    c.rect(2, 22, 4, 31, "DB")
    c.rect(11, 22, 13, 31, "DB")
    c.rect(9, 6, 12, 16, "LV")
    c.rect(9, 4, 12, 8, "IC")
    c.poly([(3, 4), (7, 4), (7, 9), (10, 16), (0, 16), (3, 9)], "LG")
    c.rect(3, 4, 7, 9, "IC")
    c.outline()
    return c


# ---- GROUND ---------------------------------------------------------------
def sandbox_side():
    c = C(2, 3)
    c.rect(0, 0, 31, 47, "T")
    c.frame(0, 0, 31, 47, "DB")
    c.frame(1, 1, 30, 46, "B")
    for x, y in ((8, 8), (20, 20), (11, 33), (24, 40), (6, 25), (25, 6)):
        c.set(x, y, "BE"); c.set(x + 1, y, "B")
    c.ellipse(18, 36, 3, 5, "BE")
    c.fill_box(16, 10, 21, 15, "R", "SA", "DR")        # bucket
    c.line(8, 18, 14, 24, "DB")                        # shovel
    c.ellipse(15, 25, 2, 2, "G")
    return c


def mine_cart_side():
    c = C(1, 2)
    c.rect(2, 0, 3, 31, "B")                           # the rails, running this way
    c.rect(12, 0, 13, 31, "B")
    c.poly([(1, 10), (14, 10), (13, 26), (2, 26)], "DK")
    c.frame(1, 10, 14, 11, "K")
    c.rect(3, 15, 12, 16, "G")
    for x, y, r in ((5, 8, 3), (9, 6, 3.5), (11, 9, 2)):
        c.ellipse(x, y, r, 3, "B", "DB")
    c.set(9, 5, "Y")
    c.rect(1, 26, 3, 29, "K")
    c.rect(12, 26, 14, 29, "K")
    c.outline()
    return c


def sandcastle_back():
    c = C(2, 2)
    c.rect(2, 18, 29, 29, "T")
    c.rect(4, 8, 11, 18, "T")
    c.rect(20, 8, 27, 18, "T")
    c.rect(12, 12, 19, 18, "BE")
    for x in (4, 7, 10, 20, 23, 26):
        c.rect(x, 6, x + 1, 7, "T")
    c.line(16, 0, 16, 11, "DB")
    c.poly([(15, 0), (9, 2), (15, 4)], "R")
    c.rect(4, 22, 27, 22, "BE")
    c.outline("B")
    return c


def shovel_rack_side():
    c = C(1, 2)
    c.rect(2, 2, 13, 4, "DB")
    c.rect(2, 26, 13, 28, "DB")
    c.rect(7, 2, 8, 30, "B")
    c.rect(5, 3, 5, 18, "T")
    c.rect(4, 18, 6, 26, "L")                          # the shovel, edge on
    c.rect(10, 0, 11, 8, "T")
    c.rect(9, 8, 12, 12, "G")
    c.outline()
    return c


# ---- ROCK -----------------------------------------------------------------
def fossil_display_side():
    c = C(1, 2)
    c.fill_box(5, 2, 10, 24, "T", None, "B", "DB")
    c.rect(5, 3, 5, 23, "BE")
    c.fill_box(2, 24, 13, 30, "DB", "B", "K")
    return c


def fossil_display_back():
    c = C(2, 2)
    c.fill_box(1, 2, 30, 24, "B", "T", "DB")
    c.rect(15, 3, 16, 23, "DB")
    c.fill_box(4, 24, 27, 30, "DB", "B", "K")
    return c


def standing_stones_side():
    c = C(2, 3)
    c.fill_box(2, 22, 9, 46, "G", "L", "DK")           # the stone by itself
    c.fill_box(17, 30, 26, 46, "G", "L", "DK")         # the near pillar
    c.fill_box(16, 2, 27, 31, "G", "L", "DK")          # the long stone on top, running away
    c.rect(18, 10, 25, 10, "DK")
    c.rect(18, 26, 25, 27, "GR")
    return c


# ---- BUG --------------------------------------------------------------
def ant_farm_side():
    c = C(1, 2)
    c.fill_box(5, 1, 10, 26, "T", None, None, "K")
    c.rect(6, 2, 9, 6, "IC")
    c.line(7, 9, 8, 22, "DB")
    c.fill_box(2, 26, 13, 31, "B", "T", "DB")
    return c


# ---- GHOST ------------------------------------------------------------
def gravestone_side():
    c = C(1, 2)
    c.ellipse(8, 8, 3, 5, "G")
    c.rect(5, 8, 10, 27, "G")
    c.rect(6, 5, 6, 26, "L")
    c.outline()
    c.rect(0, 28, 15, 30, "DG")
    c.rect(2, 27, 13, 28, "GR")
    return c


def gravestone_back():
    c = C(1, 2)
    c.ellipse(8, 9, 6, 6, "G")
    c.rect(2, 9, 13, 27, "G")
    c.rect(11, 4, 12, 26, "L")
    c.line(4, 20, 7, 23, "DK")                         # a crack
    c.outline()
    c.rect(0, 28, 15, 30, "DG")
    c.rect(2, 27, 13, 28, "GR")
    return c


def jack_o_lantern_side():
    c = C(1, 1)
    c.ellipse(8, 9, 7, 6, "O", "DO")
    c.rect(7, 1, 8, 3, "DG")
    c.poly([(2, 7), (4, 7), (3, 5)], "Y")              # an eye, round the side
    c.rect(2, 10, 5, 12, "Y")
    c.line(10, 4, 10, 14, "DO")
    c.outline()
    return c


def jack_o_lantern_back():
    c = C(1, 1)
    c.ellipse(8, 9, 7, 6, "O", "DO")
    c.rect(7, 1, 8, 3, "DG")
    c.line(5, 4, 5, 14, "DO")
    c.line(10, 4, 10, 14, "DO")
    c.outline()
    return c


def haunted_wardrobe_side():
    c = C(1, 2)
    c.fill_box(2, 1, 14, 29, "DP", "P", "K")
    c.rect(2, 3, 3, 27, "K")                           # the doors, ajar
    c.ellipse(4, 9, 1.5, 2.5, "PP")
    c.set(4, 9, "BK")
    c.rect(3, 29, 5, 31, "K")
    c.rect(11, 29, 13, 31, "K")
    return c


def haunted_wardrobe_back():
    c = C(2, 2)
    c.fill_box(2, 1, 29, 29, "DP", "P", "K")
    for x in (9, 16, 23):
        c.rect(x, 2, x, 28, "K")
    c.rect(3, 29, 5, 31, "K")
    c.rect(26, 29, 28, 31, "K")
    return c


# ---- STEEL --------------------------------------------------------------
def anvil_side():
    c = C(1, 1)
    c.fill_box(3, 2, 12, 6, "ST", "W", "G")
    c.rect(5, 7, 10, 10, "G")
    c.rect(2, 11, 13, 15, "DK")
    c.outline()
    return c


def suit_of_armor_side():
    c = C(1, 2)
    c.ellipse(8, 5, 3.5, 5, "ST", "K")
    c.rect(3, 4, 5, 6, "ST")                           # the visor, pointing this way
    c.set(5, 5, "K")
    c.fill_box(5, 10, 11, 20, "ST", "W", "G")
    c.rect(6, 20, 9, 29, "ST")
    c.rect(2, 2, 2, 29, "G")                           # the spear, in front
    c.poly([(1, 0), (3, 0), (2, 3)], "W")
    c.outline()
    return c


def suit_of_armor_back():
    c = C(1, 2)
    c.ellipse(8, 5, 4, 5, "ST", "K")
    c.fill_box(3, 10, 12, 20, "ST", "W", "G")
    c.rect(4, 20, 6, 29, "ST")
    c.rect(9, 20, 11, 29, "ST")
    c.rect(2, 2, 2, 29, "G")
    c.poly([(1, 0), (3, 0), (2, 3)], "W")
    c.outline()
    return c


def big_gears_side():
    c = C(1, 2)
    c.fill_box(3, 10, 8, 30, "ST", "W", "G")           # the gears, edge on
    c.fill_box(8, 3, 12, 18, "L", "W", "G")
    c.rect(2, 19, 13, 20, "K")
    c.outline()
    return c


def toolbox_side():
    c = C(1, 1)
    c.fill_box(3, 6, 12, 15, "R", "SA", "DR")
    c.rect(3, 9, 12, 9, "DR")
    c.rect(7, 2, 8, 6, "DK")
    return c


def toolbox_back():
    c = C(1, 1)
    c.fill_box(1, 6, 14, 15, "R", "SA", "DR")
    c.rect(1, 9, 14, 9, "DR")
    c.frame(5, 2, 10, 6, "DK")
    return c


def girders_side():
    c = C(1, 3)
    for x0 in (2, 9):
        c.rect(x0, 1, x0 + 4, 46, "ST")
        c.rect(x0, 1, x0, 46, "W")
        c.rect(x0 + 2, 1, x0 + 2, 46, "G")
        for y in range(4, 46, 8):
            c.set(x0 + 1, y, "DK")
    c.outline()
    return c


# ---- FIRE ------------------------------------------------------------
def fireplace_side():
    c = C(1, 2)
    c.fill_box(2, 4, 15, 31, "DR", "R", "K")
    for y in range(6, 30, 4):
        c.line(3, y, 14, y, "K")
    c.fill_box(0, 2, 15, 6, "B", "T", "DB")
    c.rect(2, 13, 3, 30, "K")                          # the opening's edge, and its glow
    c.rect(0, 16, 1, 30, "O")
    c.rect(0, 22, 0, 30, "Y")
    return c


def fireplace_back():
    c = C(2, 2)
    c.fill_box(0, 4, 31, 31, "DR", "R", "K")
    for y in range(6, 30, 4):
        c.line(1, y, 30, y, "K")
        for x in range(4 + (y // 4 % 2) * 4, 30, 8):
            c.line(x, y, x, y + 3, "K")
    c.fill_box(8, 0, 23, 4, "DR", "R", "K")            # the chimney
    return c


# ---- WATER ------------------------------------------------------------
def aquarium_side():
    c = C(1, 2)
    c.fill_box(1, 2, 14, 24, "BL", None, None, "K")
    c.rect(2, 3, 13, 5, "SK")
    c.rect(2, 20, 13, 23, "T")
    c.line(4, 20, 4, 13, "GR"); c.line(11, 20, 10, 12, "GR")
    c.ellipse(7, 12, 2, 2, "O")                        # a fish, coming this way
    c.set(6, 12, "K")
    c.ellipse(9, 7, 1, 1, "IC")
    c.fill_box(0, 24, 15, 31, "DK", "G", "K")
    return c


def surfboard_side():
    c = C(1, 2)
    c.ellipse(8, 16, 2, 15, "Y", "K")
    c.poly([(9, 24), (13, 27), (9, 28)], "R")          # the fin
    return c


def surfboard_back():
    c = C(1, 2)
    c.ellipse(8, 16, 5, 15, "Y", "K")
    c.ellipse(10, 22, 1.5, 4, "PY")
    return c


def ships_wheel_side():
    c = C(1, 2)
    c.fill_box(6, 1, 9, 29, "B", "T", "DB")            # the wheel, edge on
    for y in (1, 8, 15, 22, 29):
        c.ellipse(4, y, 1.5, 1.5, "T")
    c.ellipse(4, 15, 2, 2, "GO", "DGO")
    c.outline()
    return c


# ---- GRASS ------------------------------------------------------------
def sunflowers_side():
    c = C(1, 2)
    for x, top in ((6, 4), (10, 9)):
        c.line(x + 1, top + 4, x + 1, 30, "DG")
        c.ellipse(x, top, 1.5, 4, "Y")                  # the flowers, turned this way
        c.ellipse(x - 1, top, 0.8, 2.5, "DB")
    c.poly([(7, 18), (3, 15), (5, 20)], "GR")
    c.poly([(11, 22), (15, 19), (13, 24)], "GR")
    c.outline()
    return c


def sunflowers_back():
    c = C(1, 2)
    for x, top in ((4, 4), (11, 9)):
        c.line(x, top + 4, x, 30, "DG")
        c.ellipse(x, top, 4, 4, "Y")
        c.ellipse(x, top, 3, 3, "GR")                   # the backs of the flowers
    c.poly([(4, 18), (0, 15), (2, 20)], "GR")
    c.poly([(11, 22), (15, 19), (13, 24)], "GR")
    c.outline()
    return c


def flower_bed_side():
    c = C(1, 3)
    c.fill_box(4, 2, 13, 47, "DB", "B", "K")
    import random
    rnd = random.Random(4)
    for y in range(4, 46, 4):
        col = rnd.choice(["R", "Y", "SA", "W"])
        c.ellipse(8, y, 2, 2, col)
        c.set(8, y, "Y" if col != "Y" else "O")
    c.outline()
    return c


def vegetable_patch_side():
    c = C(2, 2)
    c.rect(0, 0, 31, 31, "DB")
    c.frame(0, 0, 31, 31, "K")
    for x in (5, 15, 25):
        c.rect(x, 1, x + 1, 30, "B")
        for y in range(4, 30, 7):
            if x == 5:
                c.ellipse(x, y, 2, 2, "GR"); c.set(x + 2, y, "O")
            elif x == 15:
                c.ellipse(x, y, 2, 2.5, "LG")
            else:
                c.ellipse(x, y, 2, 2, "R"); c.set(x, y - 2, "GR")
    return c


# ---- ELECTRIC ------------------------------------------------------------
def generator_side():
    c = C(1, 2)
    c.fill_box(2, 6, 14, 29, "Y", "PY", "DO")
    c.rect(2, 10, 3, 18, "K")                          # the vent's edge
    c.rect(6, 2, 10, 6, "DK")
    c.rect(7, 0, 9, 2, "R")
    return c


def generator_back():
    c = C(2, 2)
    c.fill_box(1, 6, 30, 29, "Y", "PY", "DO")
    c.rect(6, 12, 25, 22, "DO")
    c.frame(6, 12, 25, 22, "K")
    c.rect(4, 2, 8, 6, "DK")
    c.rect(23, 2, 27, 6, "DK")
    c.rect(5, 0, 7, 2, "BL")
    c.rect(24, 0, 26, 2, "R")
    return c


def neon_sign_side():
    c = C(1, 3)
    c.fill_box(6, 0, 10, 47, "BK", None, None, "DK")
    c.rect(5, 3, 5, 44, "PK")                          # the lights, edge on
    return c


def neon_sign_back():
    c = C(3, 1)
    c.fill_box(0, 1, 47, 14, "BK", None, None, "DK")
    c.rect(4, 4, 43, 4, "DK")
    c.rect(4, 11, 43, 11, "DK")
    return c


# ---- PSYCHIC ------------------------------------------------------------
def hypno_spiral_side():
    c = C(1, 2)
    c.ellipse(8, 13, 2.5, 13, "W", "K")                # the disc, edge on
    c.rect(7, 2, 7, 24, "MG")
    c.rect(7, 26, 8, 29, "DK")
    c.rect(2, 29, 13, 31, "DK")
    return c


def hypno_spiral_back():
    c = C(2, 2)
    c.ellipse(16, 13, 13, 13, "L", "K")
    c.ellipse(16, 13, 4, 4, "G")
    c.rect(15, 26, 16, 29, "DK")
    c.rect(10, 29, 21, 31, "DK")
    return c


# ---- ICE ------------------------------------------------------------
def snowman_side():
    c = C(1, 2)
    c.ellipse(8, 24, 6, 7, "W")
    c.ellipse(8, 13, 5, 5.5, "W")
    c.ellipse(8, 5, 4, 4, "W")
    c.rect(4, 0, 12, 1, "K"); c.rect(6, -2, 10, 1, "K")
    c.set(6, 5, "K")
    c.rect(2, 7, 4, 7, "O")                            # the nose, pointing this way
    c.set(6, 12, "K"); c.set(6, 15, "K")
    c.line(8, 11, 8, 7, "DB")
    c.rect(4, 9, 12, 10, "R")
    c.rect(11, 10, 12, 14, "R")
    c.outline("SK")
    return c


def snowman_back():
    c = C(1, 2)
    c.ellipse(8, 24, 7, 7, "W")
    c.ellipse(8, 13, 5.5, 5.5, "W")
    c.ellipse(8, 5, 4, 4, "W")
    c.rect(3, 0, 12, 1, "K"); c.rect(5, -2, 10, 1, "K")
    c.line(3, 11, 0, 8, "DB"); c.line(13, 11, 15, 8, "DB")
    c.rect(4, 9, 12, 10, "R")
    c.rect(6, 10, 7, 15, "R")                          # the scarf's ends
    c.outline("SK")
    return c


def igloo_side():
    c = C(3, 2)
    c.ellipse(26, 30, 20, 26, "W", "SK")
    for y in (12, 20):
        c.line(10, y, 44, y, "IC")
    for x in (18, 28, 38):
        c.line(x, 8, x, 12, "IC")
    c.ellipse(10, 31, 9, 10, "W", "SK")                # the tunnel, coming this way
    c.ellipse(3, 31, 2.5, 6, "NV")
    c.rect(0, 30, 47, 31, None)
    return c


def igloo_back():
    c = C(3, 2)
    c.ellipse(24, 30, 22, 26, "W", "SK")
    for y in (12, 20, 27):
        c.line(6, y, 42, y, "IC")
    for x in (14, 24, 34):
        c.line(x, 8, x, 12, "IC")
    for x in (10, 19, 29, 38):
        c.line(x, 12, x, 20, "IC")
    for x in (15, 24, 33):
        c.line(x, 20, x, 27, "IC")
    c.rect(0, 30, 47, 31, None)
    return c


# ---- DRAGON ------------------------------------------------------------
def dragon_banner_side():
    c = C(1, 2)
    c.rect(5, 0, 10, 1, "DGO")
    c.rect(7, 2, 8, 28, "R")                           # the banner, edge on
    c.rect(7, 9, 8, 15, "GO")
    c.outline()
    return c


# ---- DARK ------------------------------------------------------------
def dark_throne_side():
    c = C(2, 2)
    c.poly([(20, 18), (20, 4), (24, 0), (28, 4), (28, 18)], "DP")
    c.rect(22, 5, 26, 17, "P")
    c.fill_box(4, 17, 28, 23, "DP", "P", "K")          # the seat, this way
    c.rect(5, 23, 7, 31, "BK")
    c.rect(25, 23, 27, 31, "BK")
    c.rect(4, 13, 18, 14, "DP")                        # an arm
    c.rect(5, 14, 6, 17, "DP")
    c.outline()
    return c


def dark_throne_back():
    c = C(2, 2)
    c.poly([(6, 22), (6, 4), (10, 0), (16, 4), (22, 0), (26, 4), (26, 22)], "DP")
    c.rect(10, 6, 22, 20, "P")
    c.rect(15, 6, 16, 20, "DP")
    c.rect(3, 18, 5, 23, "DP")
    c.rect(27, 18, 29, 23, "DP")
    c.rect(5, 23, 7, 31, "BK")
    c.rect(24, 23, 26, 31, "BK")
    c.outline()
    return c


def thieves_safe_side():
    c = C(1, 1)
    c.fill_box(3, 1, 14, 15, "DK", "G", "K")
    c.rect(1, 6, 3, 10, "L")                           # the dial, edge on
    c.rect(5, 4, 12, 4, "K")
    return c


def thieves_safe_back():
    c = C(1, 1)
    c.fill_box(1, 1, 14, 15, "DK", "G", "K")
    c.rect(4, 4, 11, 4, "K")
    c.rect(4, 12, 11, 12, "K")
    return c


def spiked_fence_side():
    c = C(1, 3)
    c.rect(6, 0, 9, 47, "BK")
    for y in range(2, 46, 5):
        c.poly([(5, y + 2), (7.5, y - 2), (10, y + 2)], "DK")
        c.rect(6, y + 2, 9, y + 3, "DK")
    return c


# ---- FAIRY ------------------------------------------------------------
def rose_arch_side():
    c = C(1, 3)
    c.rect(6, 34, 9, 47, "W")                          # the near post
    c.fill_box(4, 2, 11, 36, "W", None, None, "MG")    # the arch, running away
    for y in range(5, 36, 5):
        c.ellipse(7.5, y, 2, 2, "PK")
    c.outline("MG")
    return c


def teddy_bear_side():
    c = C(1, 1)
    c.ellipse(9, 11, 4, 4.5, "T")
    c.ellipse(8, 5, 3.5, 3.5, "T")
    c.ellipse(10, 2, 1.5, 1.5, "T")
    c.ellipse(4.5, 6, 1.5, 1.2, "BE")                  # its snout, this way
    c.set(4, 5, "DB")
    c.set(6, 4, "K")
    c.rect(6, 8, 10, 8, "R")
    c.ellipse(6, 12, 1.5, 2, "BE")
    c.outline("DB")
    return c


def teddy_bear_back():
    c = C(1, 1)
    c.ellipse(8, 11, 5, 4.5, "T")
    c.ellipse(8, 5, 4, 3.5, "T")
    c.ellipse(4, 2, 1.5, 1.5, "T"); c.ellipse(12, 2, 1.5, 1.5, "T")
    c.rect(6, 8, 10, 8, "R")
    c.ellipse(8, 14, 1.2, 1, "BE")                     # its tail
    c.outline("DB")
    return c


# ---------------------------------------------------------------------------
# The ways round each piece goes (after the front), in order. Left-facing sides come first, then
# the back, then the right side (the left side flipped).
# ---------------------------------------------------------------------------
def _four(name, back=True):
    side = f"{name}_side"
    return [side, f"{name}_back", ("mirror", side)] if back else [side, ("mirror", side)]


TURNS = {
    # (the indoor furniture: by catalog key)
    "BOOKSHELF": _four("bookshelf"),
    "CABINET": _four("cabinet"),
    "TV": _four("tv"),
    # (the type decorations: by art name)
    "couch": _four("couch"),
    "grandfather_clock": _four("grandfather_clock"),
    "vending_machine": _four("vending_machine"),
    "arcade_cabinet": _four("arcade_cabinet"),
    "barbell_rack": ["barbell_rack_side"],
    "wooden_dummy": ["wooden_dummy_side", ("mirror", "wooden_dummy_side")],
    "gong": _four("gong"),
    "wind_turbine": ["wind_turbine_side", ("mirror", "wind_turbine_side")],
    "chemistry_set": ["chemistry_set_side", "mirror", ("mirror", "chemistry_set_side")],
    "sandbox": ["sandbox_side", "mirror", ("mirror", "sandbox_side")],
    "mine_cart": ["mine_cart_side", "mirror"],
    "sandcastle": ["sandcastle_back"],
    "shovel_rack": ["shovel_rack_side", "mirror", ("mirror", "shovel_rack_side")],
    "fossil_display": _four("fossil_display"),
    "standing_stones": ["standing_stones_side", ("mirror", "standing_stones_side")],
    "ant_farm": ["ant_farm_side", "mirror", ("mirror", "ant_farm_side")],
    "gravestone": _four("gravestone"),
    "jack_o_lantern": _four("jack_o_lantern"),
    "haunted_wardrobe": _four("haunted_wardrobe"),
    "anvil": ["anvil_side", "mirror", ("mirror", "anvil_side")],
    "suit_of_armor": _four("suit_of_armor"),
    "big_gears": ["big_gears_side", "mirror", ("mirror", "big_gears_side")],
    "toolbox": _four("toolbox"),
    "girders": ["girders_side"],
    "fireplace": _four("fireplace"),
    "aquarium": ["aquarium_side", "mirror", ("mirror", "aquarium_side")],
    "surfboard": _four("surfboard"),
    "ships_wheel": ["ships_wheel_side", "mirror", ("mirror", "ships_wheel_side")],
    "sunflowers": _four("sunflowers"),
    "flower_bed": ["flower_bed_side"],
    "vegetable_patch": ["vegetable_patch_side"],
    "generator": _four("generator"),
    "neon_sign": _four("neon_sign"),
    "hypno_spiral": _four("hypno_spiral"),
    "snowman": _four("snowman"),
    "igloo": _four("igloo"),
    "dragon_statue": ["mirror"],
    "dragon_banner": ["dragon_banner_side"],
    "dark_throne": _four("dark_throne"),
    "thieves_safe": _four("thieves_safe"),
    "spiked_fence": ["spiked_fence_side"],
    "rose_arch": ["rose_arch_side"],
    "teddy_bear": _four("teddy_bear"),
    "crow_perch": ["mirror"],
    "bent_spoon": ["mirror"],
    "weather_vane": ["mirror"],
}

PIECES = {name: fn for name, fn in globals().items() if callable(fn) and not name.startswith("_")
          and name not in ("C",) and (name.endswith("_side") or name.endswith("_back")
                                      or name.startswith("chair_"))}


if __name__ == "__main__":
    import sys
    from PIL import Image, ImageDraw
    import decor_art as DA
    allp = dict(DA.PIECES, **PIECES)

    def flip(img):
        return img.transpose(Image.FLIP_LEFT_RIGHT)

    rows = []
    for name, turns in TURNS.items():
        front = DA.render(allp[name](), 2) if name in allp else None
        imgs = [front] if front else []
        for t in turns:
            if t == "mirror":
                imgs.append(flip(front))
            elif isinstance(t, tuple):
                imgs.append(flip(DA.render(allp[t[1]](), 2)))
            else:
                imgs.append(DA.render(allp[t](), 2))
        rows.append((name, imgs))
    extra = [("chairs", [DA.render(chair_front(), 2), DA.render(chair_back(), 2)])]
    rows = extra + rows
    cols = 3
    cw, ch = 330, 116
    sheet = Image.new("RGB", (cols * cw, ((len(rows) + cols - 1) // cols) * ch), (40, 40, 48))
    d = ImageDraw.Draw(sheet)
    for k, (name, imgs) in enumerate(rows):
        x0, y0 = (k % cols) * cw + 4, (k // cols) * ch + 4
        x = x0
        for im in imgs:
            sheet.paste(im, (x, y0))
            x += im.width + 4
        d.text((x0, y0 + ch - 16), name, fill=(255, 255, 255))
    sheet.save(sys.argv[1] if len(sys.argv) > 1 else "decor_turned.png")
