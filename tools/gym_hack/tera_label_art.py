#!/usr/bin/env python3
"""The summary screen's SKILLS page (Pokémon Canvas): the RIBBON label reads TERA instead (the box
under it shows the Pokémon's Tera type; this hack has no ribbons). The letters are the label font's
own, copied from ITEM (T, E), RIBBON (R) and STATS (A). Writes graphics/summary_screen/tiles.png
and iv_ev_tiles.png (the one in use: P_SUMMARY_SCREEN_IV_EV_TILESET); run once (it checks the label
still says RIBBON).

    python3 tools/gym_hack/tera_label_art.py
"""
import struct
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
TILESETS = [ROOT / "graphics/summary_screen/tiles.png", ROOT / "graphics/summary_screen/iv_ev_tiles.png"]
SKILLS = ROOT / "graphics/summary_screen/page_skills.bin"
W = 32
BG = 39                     # the label's yellow
LETTER_X0, LETTER_W = 18, 6  # (in a label strip: where the first letter starts, each letter's width)


def relabel(path):
    img = Image.open(path)
    tw = img.width // 8
    m = struct.unpack(f"<{W * W}H", SKILLS.read_bytes())

    def strip(row, x0, x1):
        s = Image.new("P", ((x1 - x0) * 8, 8))
        for x in range(x0, x1):
            ti = m[row * W + x] & 0x3FF
            s.paste(img.crop(((ti % tw) * 8, (ti // tw) * 8, (ti % tw) * 8 + 8, (ti // tw) * 8 + 8)), ((x - x0) * 8, 0))
        return s

    item, ribbon, stats = strip(3, 10, 20), strip(3, 20, 30), strip(6, 10, 20)

    def letter(s, k):
        x = LETTER_X0 + k * LETTER_W
        return s.crop((x, 0, x + LETTER_W, 8))

    if letter(ribbon, 1).tobytes() != letter(item, 0).tobytes():      # (RIBBON's I is ITEM's I)
        print(path.name, "isn't RIBBON any more: already done")
        return
    new = ribbon.copy()
    new.paste(Image.new("P", (6 * LETTER_W, 8), BG), (LETTER_X0, 0))
    for k, g in enumerate([letter(item, 1), letter(item, 2), letter(ribbon, 0), letter(stats, 2)]):   # T E R A
        new.paste(g, (LETTER_X0 + k * LETTER_W, 0))
    # back into RIBBON's own tiles
    for x in range(20, 30):
        ti = m[3 * W + x] & 0x3FF
        if not 0x99 <= ti <= 0x9F:
            continue
        img.paste(new.crop(((x - 20) * 8, 0, (x - 20) * 8 + 8, 8)), ((ti % tw) * 8, (ti // tw) * 8))
    img.save(path)
    print(path.name, "RIBBON -> TERA")


def main():
    for path in TILESETS:
        relabel(path)


if __name__ == "__main__":
    main()
