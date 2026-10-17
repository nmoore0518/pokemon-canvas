#!/usr/bin/env python3
"""One-time asset step: add a "Poke Ball shelf" to the lab tileset and place it in Oak's lab.

- Builds two new metatiles from the lab bookshelf, with Poke Balls painted over the books,
  using a new palette (lab_frlg palette 09, previously unused).
- Replaces the three-ball table in Oak's lab with two of these shelves.

Running it twice would add the shelf twice, so it refuses if the tileset already has it.
"""
import struct
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
PRIM = ROOT / "data/tilesets/primary/building_frlg"
SEC = ROOT / "data/tilesets/secondary/lab_frlg"
LAYOUT = ROOT / "data/layouts/PalletTown_ProfessorOaksLab_Frlg/map.bin"
NUM_PRIMARY_TILES = 640
NUM_PRIMARY_METATILES = 640
SHELF_PAL = 9
ORIG_SEC_METATILES = 175

# Bookshelf body metatiles in the primary tileset (left, right half)
BOOKSHELF_LEFT, BOOKSHELF_RIGHT = 0x073, 0x074


def read_pal(p):
    lines = p.read_text().split("\n")[3:19]
    return [tuple(int(v) for v in l.split()) for l in lines]


def write_pal(p, colors):
    p.write_text("JASC-PAL\n0100\n16\n" + "".join(f"{r} {g} {b}\n" for r, g, b in colors))


def main():
    sec_mt = (SEC / "metatiles.bin").read_bytes()
    if len(sec_mt) // 16 != ORIG_SEC_METATILES:
        raise SystemExit("The lab tileset was already changed; not adding the shelf again.")

    prim_tiles = Image.open(PRIM / "tiles.png")
    prim_mt = (PRIM / "metatiles.bin").read_bytes()
    prim_attr = (PRIM / "metatile_attributes.bin").read_bytes()

    def mt(data, i):
        return list(struct.unpack_from("<8H", data, i * 16))

    def tile(idx):
        x, y = (idx % 16) * 8, (idx // 16) * 8
        return prim_tiles.crop((x, y, x + 8, y + 8))

    # Compose the 32x16 bookshelf body from its top-layer tiles.
    body = Image.new("P", (32, 16))
    for half, mid in enumerate((BOOKSHELF_LEFT, BOOKSHELF_RIGHT)):
        top = mt(prim_mt, mid)[4:]
        for k, e in enumerate(top):
            t = tile(e & 0x3FF)
            if (e >> 10) & 1:
                t = t.transpose(Image.FLIP_LEFT_RIGHT)
            if (e >> 11) & 1:
                t = t.transpose(Image.FLIP_TOP_BOTTOM)
            body.paste(t, (half * 16 + (k % 2) * 8, (k // 2) * 8))

    # Paint Poke Balls over the two rows of books (rows 6-8 and 12-14, inside the frame).
    RED, BAND, WHITE, GAP = 5, 6, 7, 1
    ball = [[RED, RED, RED], [BAND, WHITE, BAND], [WHITE, WHITE, WHITE]]
    px = body.load()
    for top in (6, 12):
        for x in range(3, 29):
            for dy in range(3):
                col = (x - 3) % 4
                px[x, top + dy] = ball[dy][col] if col < 3 and x <= 25 else GAP
    used = {px[x, y] for x in range(32) for y in range(16)}
    assert used <= {0, 1, 2, 3, 4, RED, BAND, WHITE}, used

    # Palette 09: the bookshelf's palette (primary palette 2) for the frame,
    # plus Poke Ball red, black band and white.
    pal2 = read_pal(PRIM / "palettes/02.pal")
    pal = pal2[:5] + [(230, 49, 41), (49, 49, 57), (248, 248, 248)] + [(0, 0, 0)] * 8
    write_pal(SEC / f"palettes/{SHELF_PAL:02}.pal", pal)

    # Append the 8 new tiles to the lab tileset image (keeping its palette and width).
    sec_img = Image.open(SEC / "tiles.png")
    n_tiles = (sec_img.width // 8) * (sec_img.height // 8)
    rows_needed = (n_tiles + 8 + 15) // 16
    new_img = Image.new("P", (sec_img.width, rows_needed * 8))
    new_img.putpalette(sec_img.getpalette())
    new_img.paste(sec_img, (0, 0))
    first_new = n_tiles
    order = [(0, 0), (8, 0), (0, 8), (8, 8), (16, 0), (24, 0), (16, 8), (24, 8)]
    for k, (x, y) in enumerate(order):
        t = body.crop((x, y, x + 8, y + 8))
        idx = first_new + k
        new_img.paste(t, ((idx % 16) * 8, (idx // 16) * 8))
    new_img.save(SEC / "tiles.png", transparency=None) if False else new_img.save(SEC / "tiles.png")

    # Two new metatiles: original bottom layer, new top layer on palette 09.
    out = bytearray(sec_mt)
    attrs = bytearray((SEC / "metatile_attributes.bin").read_bytes())
    new_ids = []
    for half, mid in enumerate((BOOKSHELF_LEFT, BOOKSHELF_RIGHT)):
        entries = mt(prim_mt, mid)[:4]
        for k in range(4):
            entries.append((NUM_PRIMARY_TILES + first_new + half * 4 + k) | (SHELF_PAL << 12))
        out += struct.pack("<8H", *entries)
        attrs += prim_attr[mid * 4:mid * 4 + 4]
        new_ids.append(NUM_PRIMARY_METATILES + len(out) // 16 - 1)
    (SEC / "metatiles.bin").write_bytes(bytes(out))
    (SEC / "metatile_attributes.bin").write_bytes(bytes(attrs))

    # Place two shelves where the table was: top at y=3, body at y=4, shadow at y=5, x=8..11.
    w = 13
    blocks = list(struct.unpack(f"<{len(LAYOUT.read_bytes()) // 2}H", LAYOUT.read_bytes()))
    floor = blocks[6 * w + 8]
    shelf_top = blocks[7 * w + 8] & ~0x3FF, (0x28B, 0x28C)
    body_raw = blocks[8 * w + 8] & ~0x3FF
    shadow_raw = blocks[9 * w + 8] & ~0x3FF
    for x in range(8, 11):
        for y in (4, 5):
            blocks[y * w + x] = floor
    for i, x in enumerate(range(8, 12)):
        blocks[3 * w + x] = shelf_top[0] | shelf_top[1][i % 2]
        blocks[4 * w + x] = body_raw | new_ids[i % 2]
        blocks[5 * w + x] = shadow_raw | (0x283, 0x284)[i % 2]
    LAYOUT.write_bytes(struct.pack(f"<{len(blocks)}H", *blocks))
    print("new metatiles:", [hex(i) for i in new_ids], "first new tile:", NUM_PRIMARY_TILES + first_new)


if __name__ == "__main__":
    main()
