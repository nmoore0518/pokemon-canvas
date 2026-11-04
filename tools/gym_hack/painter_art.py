#!/usr/bin/env python3
"""The painter's loft in Pewter City (Pokémon Canvas): its people and things (src/painter.c).

    python3 tools/gym_hack/painter_art.py [preview.png]

Writes graphics/object_events/pics/people/painter.png (+ painter.pal, paint_figure.pal):
  the painter, FireRed's woman with a red beret and a paint-spattered smock; the figure in her
  paintings is the same in wild colours.
And graphics/object_events/pics/misc/painter_decor.png (+ painter_decor.pal), one palette:
  frame 0 (32x32) the easel and its canvas (the way into the paintings)
  frame 1 (32x32) canvases leaning on the wall
  frames 2-5 (16x16, after them) paint splotches (red, blue, yellow) and a paint pot
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
PEOPLE = ROOT / "graphics/object_events/pics/people"
MISC = ROOT / "graphics/object_events/pics/misc"


def load_pal(path):
    return [tuple(int(v) for v in l.split()) for l in Path(path).read_text().splitlines()[3:19]]


def write_pal(path, cols):
    path.write_text("JASC-PAL\n0100\n16\n" + "".join(f"{r} {g} {b}\n" for r, g, b in cols))


def gba(c):
    return tuple((v >> 3) << 3 for v in c)


# ------------------------------------------------------------------------------------- the painter
RED, DARK_RED = 5, 8            # (npc_green's unused slots, now the beret's)
SMOCK = (11, 12, 13)            # (the dress's pinks, now a white smock)


def painter():
    src = Image.open(PEOPLE / "woman_1_frlg.png")
    w, h = src.size
    out = src.copy()
    p = out.load()
    for f in range(w // 16):
        x0 = f * 16
        rows = [y for y in range(h) if any(p[x0 + x, y] for x in range(16))]
        top = rows[0]
        xs = [x for x in range(16) if any(p[x0 + x, y] for y in range(top, top + 3))]
        left, right = min(xs), max(xs)
        facing_side = (right - left) < 7
        # the beret: a flat red cap over the crown, tipped to one side, a stub on top
        cx = (left + right) / 2 + (1 if facing_side else 0.5)
        half = 5.5 if not facing_side else 5
        for y in range(top - 3, top + 2):
            k = y - (top - 3)
            span = [half - 2, half - 0.5, half, half, half - 1][k]
            for x in range(16):
                if abs(x - cx) <= span:
                    edge = abs(x - cx) > span - 1 or k in (0, 4)
                    p[x0 + x, y] = 15 if edge else (DARK_RED if k == 3 else RED)
        p[x0 + int(cx), top - 4] = 15
        # the smock: paint on it
        for y in range(h):
            for x in range(16):
                if p[x0 + x, y] == 12 and (x * 7 + y * 3 + f) % 11 == 0:
                    p[x0 + x, y] = RED if (x + y) % 2 else 9
    pal = load_pal(ROOT / "graphics/object_events/palettes/npc_green.pal")
    pal[RED], pal[DARK_RED] = (216, 48, 48), (128, 24, 32)
    pal[11], pal[12], pal[13] = (248, 248, 240), (208, 208, 216), (128, 128, 144)
    out.putpalette([v for c in pal for v in c] + [0] * (768 - 48))
    out.save(PEOPLE / "painter.png")
    write_pal(PEOPLE / "painter.pal", [gba(c) for c in pal])
    figure = list(pal)
    vivid = {1: (248, 176, 248), 2: (224, 136, 240), 3: (184, 96, 216), 4: (112, 40, 152), RED: (248, 216, 40),
             6: (40, 200, 240), 7: (40, 112, 240), DARK_RED: (240, 136, 40), 9: (80, 216, 80), 10: (24, 112, 40),
             11: (248, 120, 176), 12: (232, 56, 120), 13: (144, 24, 80)}
    for k, c in vivid.items():
        figure[k] = c
    write_pal(PEOPLE / "paint_figure.pal", [gba(c) for c in figure])
    return out, pal, figure


# --------------------------------------------------------------------------------------- the loft
DECOR = [(255, 0, 255), (40, 32, 32), (104, 64, 32), (160, 104, 56), (208, 160, 96), (248, 248, 240),
         (200, 200, 192), (224, 56, 48), (248, 144, 40), (248, 216, 48), (72, 184, 72), (64, 128, 224),
         (144, 80, 200), (240, 128, 176), (152, 152, 160), (88, 88, 104)]
OUTLINE, WOOD_D, WOOD, WOOD_L, CANVAS, CANVAS_D, R, O, Y, G, B, P, PK, METAL, METAL_D = range(1, 16)


def easel():
    im = Image.new("P", (32, 32), 0)
    d = ImageDraw.Draw(im)
    # legs: an A frame, a back leg, a ledge
    d.line([(9, 31), (14, 12)], fill=WOOD_D, width=2)
    d.line([(22, 31), (17, 12)], fill=WOOD_D, width=2)
    d.line([(16, 30), (16, 10)], fill=WOOD, width=1)
    d.rectangle([7, 22, 24, 23], fill=WOOD, outline=OUTLINE)
    # the canvas, with a painting: a sunrise of stripes
    d.rectangle([5, 2, 26, 21], fill=CANVAS, outline=OUTLINE)
    for i, c in enumerate([R, O, Y, G, B, P]):
        d.line([(7, 6 + i * 2), (24, 6 + i * 2)], fill=c)
        d.line([(7, 7 + i * 2), (24, 7 + i * 2)], fill=c)
    d.ellipse([12, 4, 19, 11], fill=Y, outline=O)
    d.line([(6, 19), (25, 19)], fill=CANVAS_D)
    d.line([(15, 0), (16, 0)], fill=WOOD_D)
    d.line([(15, 1), (16, 1)], fill=WOOD)
    return im


def leaning():
    im = Image.new("P", (32, 32), 0)
    d = ImageDraw.Draw(im)
    for i, (x, colours) in enumerate([(3, [B, G]), (11, [PK, R]), (19, [Y, O])]):
        top = 6 + i * 2
        d.rectangle([x, top, x + 10, 30], fill=CANVAS, outline=OUTLINE)
        d.rectangle([x + 2, top + 3, x + 8, 27], fill=colours[0])
        d.ellipse([x + 3, top + 6, x + 7, top + 12], fill=colours[1])
        d.line([(x + 1, 30), (x + 9, 30)], fill=WOOD_D)
    return im


def splotch(colour, seed):
    im = Image.new("P", (16, 16), 0)
    d = ImageDraw.Draw(im)
    import random
    rnd = random.Random(seed)
    d.ellipse([3, 4, 12, 11], fill=colour)
    for _ in range(5):
        x, y = rnd.randrange(1, 15), rnd.randrange(2, 14)
        r = rnd.choice((1, 1, 2))
        d.ellipse([x - r, y - r, x + r, y + r], fill=colour)
    return im


def paint_pot():
    im = Image.new("P", (16, 16), 0)
    d = ImageDraw.Draw(im)
    d.rectangle([3, 6, 12, 14], fill=METAL, outline=OUTLINE)
    d.line([(4, 13), (11, 13)], fill=METAL_D)
    d.ellipse([3, 4, 12, 8], fill=B, outline=OUTLINE)
    d.line([(4, 9), (4, 12)], fill=B)
    d.line([(9, 9), (9, 11)], fill=B)
    d.line([(12, 6), (14, 2)], fill=WOOD_D)
    return im


def decor():
    # (two 32x32 frames, then four 16x16: a 32-wide sheet, the small ones in 16x16 rows of two)
    sheet = Image.new("P", (32, 32 * 2 + 16 * 2), 0)
    sheet.paste(easel(), (0, 0))
    sheet.paste(leaning(), (0, 32))
    smalls = [splotch(R, 1), splotch(B, 2), splotch(Y, 3), paint_pot()]
    for i, s in enumerate(smalls):
        sheet.paste(s, ((i % 2) * 16, 64 + (i // 2) * 16))
    sheet.putpalette([v for c in DECOR for v in c] + [0] * (768 - 48))
    sheet.save(MISC / "painter_decor.png")
    write_pal(MISC / "painter_decor.pal", [gba(c) for c in DECOR])
    # (the small frames as their own 16-wide sheet: the build's 2x2 tile order)
    small = Image.new("P", (16, 64), 0)
    for i, s in enumerate(smalls):
        small.paste(s, (0, i * 16))
    small.putpalette(sheet.getpalette())
    small.save(MISC / "painter_decor_small.png")
    return sheet


def main():
    out, pal, figure = painter()
    sheet = decor()
    if len(sys.argv) > 1:
        prev = Image.new("RGB", (160 + 40, 96), (120, 96, 72))
        for k, pl in enumerate((pal, figure)):
            im = out.copy()
            im.putpalette([v for c in pl for v in c] + [0] * (768 - 48))
            rgba = im.convert("RGBA")
            a = Image.eval(out.convert("L"), lambda v: 255)
            mask = out.point(lambda v: 255 if v else 0, mode="L") if False else None
            px = out.load()
            alpha = Image.new("L", out.size)
            al = alpha.load()
            for y in range(out.size[1]):
                for x in range(out.size[0]):
                    al[x, y] = 255 if px[x, y] else 0
            prev.paste(im.convert("RGB"), (0, k * 32), alpha)
        s = sheet.convert("RGB")
        prev.paste(s, (164, 0))
        prev.resize((prev.width * 4, prev.height * 4), Image.NEAREST).save(sys.argv[1])


if __name__ == "__main__":
    main()
