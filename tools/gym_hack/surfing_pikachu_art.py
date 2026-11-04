#!/usr/bin/env python3
"""Surfing Pikachu's pictures (Pokémon Canvas): Pikachu's own, standing on a surfboard.

    python3 tools/gym_hack/surfing_pikachu_art.py [preview.png]

graphics/pokemon/pikachu/surfing/: anim_front.png (Pikachu's two frames, a little higher, on the
board), back.png (the board's tail under it), icon.png, normal.pal / shiny.pal (Pikachu's, with the
board's blues in the three slots Pikachu's pictures don't use: 8, 11, 12).
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "graphics/pokemon/pikachu"
OUT = SRC / "surfing"
OUTLINE, WHITE, DARK, MID, LIGHT = 1, 15, 8, 11, 12
BOARD = {DARK: (32, 72, 160), MID: (72, 136, 232), LIGHT: (168, 208, 248)}


def pal(path):
    lines = path.read_text().split()
    vals = list(map(int, lines[3:]))
    return [tuple(vals[i:i + 3]) for i in range(0, 48, 3)]


def save_pal(colors, path):
    path.write_text("JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{r} {g} {b}\r\n" for r, g, b in colors))


def board(w, h, x0, x1, y0, y1, nose_lift=3):
    """an index grid (dict) of a side-on board: a long rounded shape, its nose (right) turned up"""
    m = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(m)
    d.ellipse((x0, y0, x1, y1), fill=255)
    cells = {}
    for y in range(h):
        for x in range(w):
            if m.getpixel((x, y)):
                lift = max(0, (x - (x1 - 10))) * nose_lift // 10   # (the nose rises)
                cells[(x, y - lift)] = MID
    out = {}
    for (x, y) in cells:
        edge = any((x + dx, y + dy) not in cells for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        top = (x, y - 1) not in cells
        bottom = (x, y + 1) not in cells or (x, y + 2) not in cells
        out[(x, y)] = OUTLINE if edge else (LIGHT if (x, y - 2) not in cells else (DARK if bottom else MID))
    # a white stripe along the middle of each column (not at the very ends)
    xs = sorted({x for x, _ in out})
    for x in xs[5:-4]:
        col = sorted(y for (cx, y) in out if cx == x)
        y = (col[0] + col[-1]) // 2
        if out.get((x, y)) in (MID, DARK, LIGHT):
            out[(x, y)] = WHITE
    return out


def frame(src, oy, b):
    """a 64x64 frame: the board, then the sprite moved up by oy on top"""
    im = Image.new("P", (64, 64), 0)
    for (x, y), c in b.items():
        if 0 <= x < 64 and 0 <= y < 64:
            im.putpixel((x, y), c)
    for y in range(64):
        for x in range(64):
            c = src.getpixel((x, y))
            if c and 0 <= y - oy < 64:
                im.putpixel((x, y - oy), c)
    return im


def main(preview=None):
    OUT.mkdir(exist_ok=True)
    normal, shiny = pal(SRC / "normal.pal"), pal(SRC / "shiny.pal")
    for p in (normal, shiny):
        for i, c in BOARD.items():
            p[i] = c
    save_pal(normal, OUT / "normal.pal")
    save_pal(shiny, OUT / "shiny.pal")
    flat = [v for c in normal for v in c] + [0] * (768 - 48)

    front = Image.open(SRC / "anim_front.png")
    b = board(64, 64, 3, 61, 49, 59, nose_lift=5)
    sheet = Image.new("P", (64, 128), 0)
    for k in range(2):
        f = frame(front.crop((0, 64 * k, 64, 64 * k + 64)), 3, b)
        sheet.paste(f, (0, 64 * k))
    sheet.putpalette(flat)
    sheet.save(OUT / "anim_front.png", transparency=0)

    back = Image.open(SRC / "back.png")
    bb = board(64, 64, -8, 72, 54, 70, nose_lift=0)
    bk = frame(back, 6, bb)
    bk.putpalette(flat)
    bk.save(OUT / "back.png", transparency=0)

    # the icon: Pikachu's, with a little board under it in the icon palette's darkest and lightest
    icon = Image.open(SRC / "icon.png")
    ic = icon.copy()
    ipal = icon.getpalette()[:48]
    cols = [tuple(ipal[i:i + 3]) for i in range(0, 48, 3)]
    dark = min(range(1, 16), key=lambda i: sum(cols[i]))
    light = max(range(1, 16), key=lambda i: sum(cols[i]))
    for k in range(2):
        base = 32 * k
        rows = [y for y in range(32) if any(icon.getpixel((x, base + y)) for x in range(32))]
        bottom = max(rows)
        sub = icon.crop((0, base, 32, base + 32))
        layer = Image.new("P", (32, 32), 0)
        for y in range(32):
            for x in range(32):
                c = sub.getpixel((x, y))
                if c and y - 2 >= 0:
                    layer.putpixel((x, y - 2), c)
        by = min(31, bottom)
        for x in range(4, 29):
            layer.putpixel((x, by - 1), dark)
            layer.putpixel((x, by), light if 6 < x < 27 else dark)
            if by + 1 < 32:
                layer.putpixel((x, by + 1), dark)
        ic.paste(layer, (0, base))
    ic.save(OUT / "icon.png", transparency=0)

    if preview:
        prev = Image.new("RGB", (64 * 3 + 32, 128), (120, 120, 120))
        prev.paste(sheet.convert("RGB"), (0, 0))
        prev.paste(bk.convert("RGB"), (64, 0))
        sh = sheet.copy(); sh.putpalette([v for c in shiny for v in c] + [0] * (768 - 48))
        prev.paste(sh.convert("RGB"), (128, 0))
        prev.paste(ic.convert("RGB"), (192, 0))
        prev.resize((prev.size[0] * 3, prev.size[1] * 3), Image.NEAREST).save(preview)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else None)
