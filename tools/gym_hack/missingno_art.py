#!/usr/bin/env python3
"""MissingNo. (Pokémon Canvas): its pictures, drawn as Red and Blue drew it: its "picture" is whatever
bytes were lying where a picture should be, read as Game Boy 2bpp tiles, inside its backwards-L
outline (two tiles wide on top, three below, the step to the left).

    python3 tools/gym_hack/missingno_art.py [preview.png]

The garbage is real garbage: the Trading Card Game's music bytes (src/data/tcg/audio.h), read as
tiles, so it looks like memory and not like noise. Writes graphics/pokemon/missingno/: anim_front.png
(two frames, different bytes: it flickers), back.png (chunky, as RB's doubled back pictures were),
icon.png (two frames, icon palette 2), footprint.png, normal.pal and shiny.pal.
"""
import re
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "graphics/pokemon/missingno"

# its outline on Red and Blue's 7x7-tile picture grid: (row, first col, last col)
OUTLINE = [(0, 3, 4), (1, 3, 4), (2, 3, 4), (3, 2, 4), (4, 2, 4), (5, 2, 4), (6, 2, 4)]
# the Game Boy shades 0-3 (white .. black) as palette indices 1-4; 0 is clear
PALETTE = [(98, 156, 131), (248, 248, 248), (184, 168, 232), (112, 96, 184), (40, 32, 64)]
SHINY = [(98, 156, 131), (248, 248, 232), (160, 216, 168), (72, 152, 104), (24, 48, 32)]
# icon palette 2's white, lavender, purple, dark grey
ICON_SHADES = [3, 12, 11, 15]


def garbage():
    s = (ROOT / "src/data/tcg/audio.h").read_text()
    return bytes(int(x, 16) for x in re.findall(r"0x([0-9A-F]{2})", s[s.index("gTcgAudio"):s.index("};")]))


def tile(data, off):
    """8x8 shades 0-3 from 16 bytes of 2bpp"""
    out = []
    for y in range(8):
        lo, hi = data[off + y * 2], data[off + y * 2 + 1]
        out.append([((lo >> (7 - x)) & 1) | (((hi >> (7 - x)) & 1) << 1) for x in range(8)])
    return out


def mask_tiles():
    return [(r, c) for r, a, b in OUTLINE for c in range(a, b + 1)]


def picture(data, off, size=64, ox=4, oy=8, scale=1, palette_of=lambda s: s + 1):
    im = Image.new("P", (size, size), 0)
    k = off
    for r, c in mask_tiles():
        t = tile(data, k)
        k += 16
        for y in range(8):
            for x in range(8):
                for sy in range(scale):
                    for sx in range(scale):
                        px, py = ox + (c * 8 + x) * scale + sx, oy + (r * 8 + y) * scale + sy
                        if 0 <= px < size and 0 <= py < size:
                            im.putpixel((px, py), palette_of(t[y][x]))
    return im


def put_pal(im, colours):
    flat = [v for c in colours for v in c] + [0] * (48 - len(colours) * 3)
    im.putpalette(flat)


def write_pal(path, colours):
    colours = colours + [(0, 0, 0)] * (16 - len(colours))
    path.write_text("JASC-PAL\n0100\n16\n" + "".join(f"{r} {g} {b}\n" for r, g, b in colours))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    data = garbage()
    # (front: two frames from two stretches of the bytes)
    front = Image.new("P", (64, 128), 0)
    front.paste(picture(data, 0x1a40), (0, 0))
    front.paste(picture(data, 0x2c80), (0, 64))
    put_pal(front, PALETTE)
    front.save(OUT / "anim_front.png")
    # (back: half the detail, doubled; RB's back pictures were 4x4 tiles shown at twice their size)
    back = picture(data, 0x0e20).resize((32, 32), Image.NEAREST).resize((64, 64), Image.NEAREST)
    put_pal(back, PALETTE)
    back.save(OUT / "back.png")
    # (icon: its outline at half size, two frames of different bytes, in icon palette 2's colours)
    icon = Image.new("P", (32, 64), 0)
    for f, off in enumerate((0x3300, 0x4410)):
        full = picture(data, off, palette_of=lambda s: ICON_SHADES[s])
        icon.paste(full.resize((32, 32), Image.NEAREST), (0, f * 32))
    pal2 = [tuple(int(v) for v in l.split()) for l in
            (ROOT / "graphics/pokemon/icon_palettes/pal2.pal").read_text().splitlines()[3:19]]
    put_pal(icon, pal2)
    icon.save(OUT / "icon.png")
    # (footprint: a scatter of the bytes' bits)
    fp = Image.new("P", (16, 16), 0)
    for y in range(16):
        for x in range(16):
            if 3 <= x < 13 and 2 <= y < 14 and (data[0x5000 + y * 2 + x // 8] >> (7 - x % 8)) & 1:
                fp.putpixel((x, y), 1)
    fp.putpalette([255, 255, 255, 0, 0, 0] + [0] * 42)
    fp.save(OUT / "footprint.png")
    write_pal(OUT / "normal.pal", PALETTE)
    write_pal(OUT / "shiny.pal", SHINY)
    if len(sys.argv) > 1:
        pv = Image.new("RGB", (64 * 3 + 32, 128), (98, 156, 131))
        for im, pos in ((front, (0, 0)), (back, (64, 0)), (icon.convert("RGB"), (128 + 64, 0))):
            pv.paste(im.convert("RGB"), pos)
        sh = front.copy()
        put_pal(sh, SHINY)
        pv.paste(sh.convert("RGB").crop((0, 0, 64, 64)), (128, 0))
        pv.resize((pv.width * 3, pv.height * 3), Image.NEAREST).save(sys.argv[1])


if __name__ == "__main__":
    main()
