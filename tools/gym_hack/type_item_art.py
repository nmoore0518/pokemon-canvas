#!/usr/bin/env python3
"""Item icons for the mentors' type items (drawn in code, 24x24, one 16-color palette each).

Writes graphics/items/icons/gym/<name>.png and graphics/items/icon_palettes/gym/<name>.pal.
Run with an output path to get a preview sheet:  python3 tools/gym_hack/type_item_art.py out.png
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from decor_art import Canvas, RGB as DECOR_RGB  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")

RGB = dict(DECOR_RGB)
RGB.update({
    "D": (72, 72, 88), "SI": (216, 224, 236), "CU": (184, 112, 56), "CL": (232, 168, 104),
    "BR": (200, 160, 56), "BRL": (248, 216, 120), "FL": (255, 248, 200), "MI": (232, 216, 248),
    "SN": (232, 240, 248), "GS": (168, 152, 160), "GSL": (208, 196, 200), "GSD": (112, 100, 108),
    "LM": (200, 240, 96), "VI": (152, 72, 200), "VL": (208, 152, 240), "BG": (64, 48, 40),
    "WD": (120, 80, 48), "WL": (176, 128, 80), "RS": (240, 112, 128), "CY": (64, 216, 232),
    "TN": (96, 104, 120),
})


class Icon(Canvas):
    def __init__(self):
        super().__init__(2, 2)
        self.w = self.h = 24
        self.px = [[None] * 24 for _ in range(24)]


# ---------------------------------------------------------------------------
# The icons, by type: (defensive, offensive)
# ---------------------------------------------------------------------------
def cacoon():
    c = Icon()
    c.line(12, 1, 12, 4, "B")
    c.ellipse(12, 13, 6.5, 9.5, "GR")
    for y in range(6, 22, 3):
        c.line(6, y, 18, y + 2, "LG")
    c.ellipse(10, 9, 1.5, 2.5, "LG")
    c.outline()
    return c


def extra_stinger():
    c = Icon()
    c.ellipse(6, 6, 5, 5, "Y")
    c.poly([(1.5, 8.5), (8.5, 1.5), (22, 22)], "Y")
    for y in range(24):
        for x in range(24):
            if c.get(x, y) == "Y" and x + y < 26 and ((x + y) // 3) % 2 == 1:
                c.set(x, y, "BK")
    c.line(15, 16, 22, 22, "DO")
    c.set(4, 4, "W"); c.set(5, 3, "W"); c.set(3, 5, "W")
    c.outline()
    return c


def black_veil():
    c = Icon()
    c.poly([(12, 1), (21, 21), (3, 21)], "BK")
    c.ellipse(12, 5, 4, 4, "BK")
    for x in (8, 12, 16):
        c.line(x, 9, x - 1 if x < 12 else x + (1 if x > 12 else 0), 20, "DK")
    c.line(9, 6, 11, 4, "P")
    c.rect(3, 21, 21, 22, "P")
    c.outline()
    return c


def manifesto():
    c = Icon()
    c.rect(5, 3, 18, 20, "BE")
    c.rect(3, 2, 20, 4, "T")
    c.rect(3, 19, 20, 21, "T")
    for y in range(7, 17, 2):
        c.line(7, y, 16 - (y % 3), y, "DK")
    c.ellipse(15, 17, 2.5, 2.5, "R")
    c.outline()
    return c


def fire_shroud():
    c = Icon()
    c.poly([(12, 2), (20, 8), (21, 21), (3, 21), (4, 8)], "DR")
    c.poly([(5, 21), (8, 13), (10, 17), (12, 10), (14, 17), (16, 13), (19, 21)], "O")
    c.poly([(9, 21), (12, 15), (15, 21)], "Y")
    c.ellipse(12, 6, 3, 2.5, "BK")
    c.outline()
    return c


def bionic_jaw():
    c = Icon()
    c.poly([(2, 6), (21, 6), (21, 11), (6, 11)], "ST")
    c.poly([(4, 14), (21, 14), (21, 19), (8, 19)], "ST")
    for x in range(8, 21, 3):
        c.poly([(x, 11), (x + 2, 11), (x + 1, 13)], "W")
        c.poly([(x, 14), (x + 2, 14), (x + 1, 12)], "W")
    c.rect(18, 7, 20, 9, "R")
    c.line(3, 7, 20, 7, "SI")
    c.outline()
    return c


def heat_shield():
    c = Icon()
    c.poly([(3, 3), (21, 3), (21, 11), (12, 21), (3, 11)], "SI")
    c.poly([(5, 5), (19, 5), (19, 11), (12, 19), (5, 11)], "O")
    c.poly([(12, 6), (16, 13), (12, 17), (8, 13)], "Y")
    c.line(4, 4, 20, 4, "W")
    c.outline()
    return c


def oxidizer():
    c = Icon()
    c.rect(7, 6, 16, 21, "BL")
    c.ellipse(11.5, 6, 4.5, 3, "BL")
    c.rect(10, 1, 13, 3, "G")
    c.rect(8, 2, 15, 2, "G")
    c.rect(8, 7, 9, 19, "SK")
    c.rect(9, 11, 14, 15, "W")
    c.set(10, 13, "R"); c.set(11, 12, "R"); c.set(11, 14, "R"); c.set(12, 13, "R")
    c.set(13, 14, "R")
    c.outline()
    return c


def cistern():
    c = Icon()
    c.rect(4, 5, 19, 20, "ST")
    c.ellipse(11.5, 5, 7.5, 3, "DK")
    c.ellipse(11.5, 5.5, 6, 2, "BL")
    c.ellipse(11.5, 20, 7.5, 2.5, "ST")
    for y in (9, 15):
        c.line(4, y, 19, y, "G")
    c.rect(19, 16, 22, 17, "G")
    c.set(22, 19, "SK")
    c.outline()
    return c


def nozzle():
    c = Icon()
    c.rect(1, 13, 7, 17, "GR")
    c.poly([(7, 11), (15, 13), (15, 17), (7, 19)], "BR")
    c.rect(15, 13, 17, 17, "BRL")
    c.line(8, 12, 14, 13, "BRL")
    c.outline()
    for i, (x, y) in enumerate([(19, 11), (21, 14), (20, 17), (22, 19), (19, 20), (22, 10), (21, 22)]):
        c.set(x, y, "SK" if i % 2 else "BL")
    return c


def chicken_wire():
    c = Icon()
    c.ellipse(12, 12, 10, 9, "GR")
    for y in range(3, 22, 4):
        for x in range(2 + (y // 4 % 2) * 2, 23, 4):
            if c.get(x, y) == "GR":
                c.set(x, y, "LG")
    for y in range(2, 23):
        for x in range(1, 23):
            if c.get(x, y) == "GR" and (x + y) % 4 == 0 or c.get(x, y) == "GR" and (x - y) % 4 == 0:
                c.set(x, y, "L")
    c.ellipse(12, 12, 3, 3, "DG")
    c.outline()
    return c


def miracle_gro():
    c = Icon()
    c.rect(6, 7, 17, 21, "GR")
    c.rect(9, 3, 14, 6, "DG")
    c.rect(8, 11, 15, 18, "Y")
    c.poly([(11, 17), (9, 13), (12, 12)], "DG")
    c.poly([(12, 17), (15, 12), (13, 15)], "LG")
    c.rect(7, 8, 7, 20, "LG")
    c.outline()
    return c


def capacitor():
    c = Icon()
    c.line(9, 18, 9, 23, "G")
    c.line(14, 18, 14, 23, "G")
    c.rect(6, 4, 17, 18, "BL")
    c.ellipse(11.5, 4, 5.5, 2, "ST")
    c.rect(6, 9, 9, 18, "NV")
    c.rect(15, 6, 16, 17, "SK")
    c.line(7, 12, 8, 12, "W")
    c.outline()
    return c


def transformer():
    c = Icon()
    c.rect(3, 8, 20, 21, "DK")
    c.rect(4, 9, 19, 20, "TN")
    for x in (6, 15):
        c.rect(x, 3, x + 2, 8, "CU")
        c.rect(x - 1, 2, x + 3, 3, "CL")
    c.poly([(13, 10), (9, 15), (12, 15), (10, 19), (15, 13), (12, 13)], "Y")
    c.outline()
    return c


def cooler():
    c = Icon()
    c.rect(2, 9, 21, 21, "R")
    c.rect(2, 6, 21, 9, "W")
    c.rect(8, 3, 15, 4, "G")
    c.line(8, 4, 8, 6, "G"); c.line(15, 4, 15, 6, "G")
    c.rect(3, 10, 20, 10, "SA")
    c.rect(2, 18, 21, 21, "DR")
    c.outline()
    return c


def rock_salt():
    c = Icon()
    for (x, y, s) in [(6, 15, 4), (14, 14, 5), (10, 9, 4), (17, 7, 3), (4, 8, 2)]:
        c.poly([(x, y - s), (x + s, y), (x, y + s), (x - s, y)], "SN")
        c.line(x - s + 1, y, x, y - s + 1, "W")
        c.line(x, y + s - 1, x + s - 1, y, "L")
    c.rect(1, 20, 22, 21, "GSL")
    c.outline()
    return c


def mouth_guard():
    c = Icon()
    c.ellipse(12, 12, 10, 8, "RS")
    c.ellipse(12, 13, 6, 5, None)
    c.rect(6, 13, 18, 22, None)
    for x in (5, 8, 15, 18):
        c.set(x, 6 + (1 if x in (5, 18) else 0), "PP")
    c.ellipse(12, 6, 3, 1.5, "PP")
    c.outline()
    return c


def steroids():
    c = Icon()
    c.ellipse(8, 9, 5.5, 5.5, "R")
    c.ellipse(15, 15, 5.5, 5.5, "W")
    c.poly([(4, 13), (12, 5), (19, 12), (11, 19)], "R")
    c.poly([(8, 16), (16, 8), (19, 12), (11, 19)], "W")
    c.ellipse(16, 16, 5.5, 5.5, "W")
    c.line(5, 9, 9, 5, "SA")
    c.line(15, 19, 19, 15, "L")
    c.outline()
    return c


def titanium_coil():
    c = Icon()
    for k in range(6):
        y = 4 + k * 3
        c.line(5, y + 2, 18, y, "ST", 2)
        c.line(6, y + 2, 17, y, "SI")
    c.line(5, 4, 5, 21, "DK")
    c.outline()
    return c


def corrosive_venom():
    c = Icon()
    c.rect(10, 2, 13, 6, "BE")
    c.ellipse(11.5, 14, 7, 8, "P")
    c.rect(8, 6, 15, 9, "P")
    c.ellipse(11.5, 15, 5.5, 5.5, "LM")
    c.ellipse(9.5, 13, 1.2, 1.2, "W")
    c.set(14, 17, "GR"); c.set(12, 18, "GR")
    c.outline()
    c.set(17, 23, "LM"); c.set(18, 22, "LM")
    return c


def granite_sarcophagus():
    c = Icon()
    c.poly([(7, 1), (16, 1), (20, 7), (17, 22), (6, 22), (3, 7)], "GS")
    c.poly([(8, 3), (15, 3), (18, 7), (15, 20), (8, 20), (5, 7)], "GSL")
    c.ellipse(11.5, 7, 2.5, 2.5, "GS")
    c.line(8, 12, 15, 12, "GSD")
    for (x, y) in [(7, 15), (13, 9), (10, 17), (16, 14)]:
        c.set(x, y, "GSD")
    c.outline()
    return c


def diamond_drill_bit():
    c = Icon()
    c.rect(10, 1, 13, 6, "DK")
    c.poly([(7, 6), (16, 6), (12, 20)], "ST")
    for y in (8, 11, 14):
        c.line(8 + (y - 6) // 3, y, 15 - (y - 6) // 3, y + 2, "DK")
    c.poly([(12, 18), (14, 21), (12, 23), (10, 21)], "IC")
    c.set(11, 20, "W")
    c.outline()
    return c


def aviator_goggles():
    c = Icon()
    c.line(0, 11, 23, 11, "B", 2)
    c.ellipse(7, 12, 5, 5, "BR")
    c.ellipse(17, 12, 5, 5, "BR")
    c.ellipse(7, 12, 3.5, 3.5, "SK")
    c.ellipse(17, 12, 3.5, 3.5, "SK")
    c.set(6, 10, "W"); c.set(16, 10, "W")
    c.rect(11, 11, 13, 12, "DB")
    c.outline()
    return c


def jet_engine():
    c = Icon()
    c.ellipse(12, 12, 10, 10, "ST")
    c.ellipse(12, 12, 8, 8, "DK")
    for k in range(8):
        import math
        a = k * math.pi / 4
        c.line(12, 12, 12 + 7 * math.cos(a), 12 + 7 * math.sin(a), "SI")
    c.ellipse(12, 12, 2.5, 2.5, "R")
    c.outline()
    return c


def energy_shield():
    c = Icon()
    c.poly([(12, 1), (21, 6), (21, 17), (12, 22), (3, 17), (3, 6)], "PK")
    c.poly([(12, 4), (18, 8), (18, 15), (12, 19), (6, 15), (6, 8)], "PP")
    c.poly([(12, 8), (15, 10), (15, 14), (12, 16), (9, 14), (9, 10)], "MG")
    c.outline("DP")
    return c


def psionic_blade():
    c = Icon()
    c.poly([(19, 1), (22, 2), (9, 15), (7, 13)], "LV")
    c.line(20, 2, 9, 13, "W")
    c.line(5, 12, 11, 18, "GO", 2)
    c.line(3, 20, 7, 16, "DP", 2)
    c.outline()
    return c


def diamond_plating():
    c = Icon()
    c.rect(2, 3, 21, 20, "ST")
    for y in range(5, 19, 4):
        for x in range(4 + (y // 4 % 2) * 2, 20, 4):
            c.line(x, y + 1, x + 1, y, "SI")
    for (x, y) in [(3, 4), (20, 4), (3, 19), (20, 19)]:
        c.set(x, y, "DK")
    c.outline()
    return c


def precision_chisel():
    c = Icon()
    c.poly([(2, 20), (10, 12), (13, 15), (5, 23)], "WL")
    c.line(3, 20, 10, 13, "T")
    c.poly([(10, 12), (19, 3), (22, 6), (13, 15)], "ST")
    c.line(11, 12, 20, 3, "SI")
    c.line(19, 2, 23, 6, "W")
    c.outline()
    return c


def stainless_alloy():
    c = Icon()
    c.poly([(2, 18), (6, 9), (21, 9), (21, 18)], "ST")
    c.poly([(6, 9), (9, 5), (23, 5), (21, 9)], "SI")
    c.poly([(21, 9), (23, 5), (23, 14), (21, 18)], "TN")
    c.line(5, 11, 18, 11, "W")
    c.outline()
    return c


def knife_sharpener():
    c = Icon()
    c.rect(2, 15, 21, 21, "GS")
    c.rect(2, 15, 21, 16, "GSL")
    c.poly([(4, 13), (18, 3), (20, 5), (7, 14)], "SI")
    c.rect(17, 2, 21, 5, "WD")
    c.line(5, 13, 17, 4, "W")
    c.outline()
    return c


def light_switch():
    c = Icon()
    c.rect(5, 2, 18, 21, "BE")
    c.rect(6, 3, 17, 3, "W")
    c.rect(9, 7, 14, 16, "T")
    c.rect(10, 7, 13, 11, "W")
    c.set(11, 4, "B"); c.set(11, 19, "B")
    c.outline()
    return c


def ouija_board():
    c = Icon()
    c.rect(1, 5, 22, 19, "WL")
    c.rect(2, 6, 21, 6, "T")
    for x in range(3, 21, 2):
        c.set(x, 9, "DB")
        c.set(x, 13, "DB")
    c.ellipse(5, 16, 1, 1, "DB")
    c.ellipse(18, 16, 1, 1, "DB")
    c.poly([(9, 12), (15, 12), (12, 20)], "W")
    c.ellipse(12, 14.5, 1.3, 1.3, "K")
    c.outline()
    return c


def twinkling_lights():
    c = Icon()
    pts = [(1, 5), (6, 10), (12, 12), (18, 10), (23, 5)]
    for a, b in zip(pts, pts[1:]):
        c.line(a[0], a[1], b[0], b[1], "DG")
    for (x, y), col in zip([(4, 9), (9, 13), (15, 13), (20, 9)], ["Y", "PK", "SK", "LG"]):
        c.ellipse(x, y + 2, 1.8, 2.4, col)
        c.set(x - 0.5, y + 1, "W")
    c.outline()
    for (x, y) in [(3, 17), (11, 20), (18, 18), (21, 15)]:
        c.set(x, y, "PY")
    return c


def magic_wand():
    c = Icon()
    c.line(3, 21, 12, 12, "DP", 2)
    c.line(4, 21, 12, 13, "LV")
    c.poly([(16, 2), (18, 7), (23, 7), (19, 10), (21, 15), (16, 12), (11, 15), (13, 10), (9, 7), (14, 7)], "PK")
    c.poly([(16, 5), (17, 8), (15, 10), (16, 8)], "PP")
    c.outline()
    return c


def fluffy_sweater():
    c = Icon()
    c.poly([(7, 3), (17, 3), (23, 9), (20, 12), (18, 10), (18, 21), (6, 21), (6, 10), (4, 12), (1, 9)], "SA")
    c.ellipse(12, 3, 3, 1.5, "R")
    c.rect(6, 18, 18, 20, "R")
    for (x, y) in [(9, 8), (14, 9), (11, 13), (15, 15), (8, 15)]:
        c.set(x, y, "W")
    c.outline()
    return c


def third_eye():
    c = Icon()
    c.poly([(1, 12), (7, 6), (17, 6), (23, 12), (17, 18), (7, 18)], "W")
    c.ellipse(12, 12, 5, 5, "VI")
    c.ellipse(12, 12, 2.5, 2.5, "BK")
    c.set(10, 10, "W")
    c.line(5, 4, 8, 2, "GO"); c.line(19, 4, 16, 2, "GO"); c.line(12, 1, 12, 3, "GO")
    c.outline()
    return c


ITEMS = [
    # (type, defensive name, function, offensive name, function)
    ("NORMAL", "Fluffy Sweater", fluffy_sweater, "Third Eye", third_eye),
    ("FIGHTING", "Mouth Guard", mouth_guard, "Steroids", steroids),
    ("FLYING", "Aviator Goggles", aviator_goggles, "Jet Engine", jet_engine),
    ("POISON", "Titanium Coil", titanium_coil, "Corrosive Venom", corrosive_venom),
    ("GROUND", "Granite Sarcophagus", granite_sarcophagus, "Diamond Drill Bit", diamond_drill_bit),
    ("ROCK", "Diamond Plating", diamond_plating, "Precision Chisel", precision_chisel),
    ("BUG", "Cacoon", cacoon, "Extra Stinger", extra_stinger),
    ("GHOST", "Light Switch", light_switch, "Ouija Board", ouija_board),
    ("STEEL", "Stainless Alloy", stainless_alloy, "Knife Sharpener", knife_sharpener),
    ("FIRE", "Heat Shield", heat_shield, "Oxidizer", oxidizer),
    ("WATER", "Cistern", cistern, "Nozzle", nozzle),
    ("GRASS", "Chicken Wire", chicken_wire, "Miracle Gro", miracle_gro),
    ("ELECTRIC", "Capacitor", capacitor, "Transformer", transformer),
    ("PSYCHIC", "Energy Shield", energy_shield, "Psionic Blade", psionic_blade),
    ("ICE", "Cooler", cooler, "Rock Salt", rock_salt),
    ("DRAGON", "Fire Shroud", fire_shroud, "Bionic Jaw", bionic_jaw),
    ("DARK", "Black Veil", black_veil, "Manifesto", manifesto),
    ("FAIRY", "Twinkling Lights", twinkling_lights, "Magic Wand", magic_wand),
]


def slug(name):
    return name.lower().replace(" ", "_")


def all_icons():
    for t, dname, dfn, oname, ofn in ITEMS:
        yield dname, dfn()
        yield oname, ofn()


def write_icon(name, canvas):
    from PIL import Image
    colors = []
    for row in canvas.px:
        for c in row:
            if c is not None and c not in colors:
                colors.append(c)
    assert len(colors) <= 15, (name, colors)
    pal = [(115, 197, 165)] + [RGB[c] for c in colors]
    pal += [(0, 0, 0)] * (16 - len(pal))
    img = Image.new("P", (24, 24), 0)
    flat = []
    for p in pal:
        flat += list(p)
    img.putpalette(flat)
    for y in range(24):
        for x in range(24):
            c = canvas.px[y][x]
            img.putpixel((x, y), 0 if c is None else colors.index(c) + 1)
    s = slug(name)
    img.save(os.path.join(ROOT, "graphics/items/icons/gym", s + ".png"), bits=4)
    with open(os.path.join(ROOT, "graphics/items/icon_palettes/gym", s + ".pal"), "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n16\n")
        for r, g, b in pal:
            f.write(f"{r} {g} {b}\n")


def main():
    os.makedirs(os.path.join(ROOT, "graphics/items/icons/gym"), exist_ok=True)
    os.makedirs(os.path.join(ROOT, "graphics/items/icon_palettes/gym"), exist_ok=True)
    icons = list(all_icons())
    for name, canvas in icons:
        write_icon(name, canvas)
    if len(sys.argv) > 1:
        from PIL import Image, ImageDraw
        scale, cell = 4, 24 * 4 + 8
        sheet = Image.new("RGB", (6 * cell, 6 * (cell + 12)), (248, 248, 232))
        d = ImageDraw.Draw(sheet)
        for i, (name, canvas) in enumerate(icons):
            img = Image.new("RGB", (24, 24), (248, 248, 232))
            for y in range(24):
                for x in range(24):
                    if canvas.px[y][x]:
                        img.putpixel((x, y), RGB[canvas.px[y][x]])
            x0, y0 = (i % 6) * cell, (i // 6) * (cell + 12)
            sheet.paste(img.resize((96, 96), Image.NEAREST), (x0 + 4, y0 + 4))
            d.text((x0 + 4, y0 + cell - 2), name[:16], fill=(0, 0, 0))
        sheet.save(sys.argv[1])


if __name__ == "__main__":
    main()
