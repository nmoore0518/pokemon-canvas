#!/usr/bin/env python3
"""The quick heal pad in Pokémon Centers (Gym Trainer hack).

Adds one metatile to the FireRed Pokémon Center tileset (gTileset_PokemonCenterFrlg): the usual
floor underneath, a glowing pad on top (palette 7, both layers under the player), with the
MB_HEAL_PAD behavior. Then puts it in the three layouts that use that tileset. Safe to rerun.

    python3 tools/gym_hack/build_heal_pad.py
"""
import re
import struct
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
TS = ROOT / "data/tilesets/secondary/pokemon_center_frlg"
NUM_PRIMARY_TILES = 640
NUM_PRIMARY_METATILES = 640

PAD_TILE = 376          # first of the 4 new tiles (secondary tile index; 376-383 were empty)
PAD_METATILE = 255      # secondary metatile index (0-254 are FireRed's)
PAD_PALETTE = 7
FLOOR_METATILE = 1      # the plain floor (its bottom layer goes under the pad)
LAYER_COVERED = 1

# Where the pad goes: layout map.bin, width, (x, y)
SPOTS = [
    ("data/layouts/PokemonCenter_1F_Frlg/map.bin", 15, (2, 8)),            # 19 Pokémon Centers
    ("data/layouts/IndigoPlateau_PokemonCenter_1F_Frlg/map.bin", 25, (2, 16)),
    ("data/layouts/OneIsland_PokemonCenter_1F_Frlg/map.bin", 19, (12, 9)),
]

# Palette 7 colors used: 0 clear, 1 dark outline, 2 gray, 3 light gray, 4 white,
# 8 blue, 9 light blue, 10 pale, 13 red, 14 pink
PAD = [
    "................",
    "...1111111111...",
    "..133333333332..",
    ".13999999999992.",
    ".13988888888992.",
    ".13989aaaaa8992.",
    ".1398a44d44a892.",
    ".1398a4ddd4a892.",
    ".1398addddda892.",
    ".1398a4ddd4a892.",
    ".1398a44d44a892.",
    ".13989aaaaa8992.",
    ".13988888888992.",
    "..122222222222..",
    "...1111111111...",
    "................",
]


def behavior_value(name="MB_HEAL_PAD"):
    text = (ROOT / "include/constants/metatile_behaviors.h").read_text()
    body = text[text.index("enum"):text.index("NUM_METATILE_BEHAVIORS")]
    names = re.findall(r"^\s+(MB_\w+)", body, re.M)
    return names.index(name)


def main():
    # Tiles
    img = Image.open(TS / "tiles.png")
    px = img.load()
    for t in range(4):
        tx, ty = ((PAD_TILE + t) % 16) * 8, ((PAD_TILE + t) // 16) * 8
        ox, oy = (t % 2) * 8, (t // 2) * 8
        for y in range(8):
            for x in range(8):
                c = PAD[oy + y][ox + x]
                px[tx + x, ty + y] = 0 if c == "." else int(c, 16)
    img.save(TS / "tiles.png")

    # Metatile: bottom = floor, top = pad
    mt = bytearray((TS / "metatiles.bin").read_bytes())
    at = bytearray((TS / "metatile_attributes.bin").read_bytes())
    floor = struct.unpack_from("<8H", mt, FLOOR_METATILE * 16)
    top = [((NUM_PRIMARY_TILES + PAD_TILE + t) | (PAD_PALETTE << 12)) for t in range(4)]
    entry = struct.pack("<8H", *floor[:4], *top)
    attr = struct.pack("<I", behavior_value() | (LAYER_COVERED << 29))
    if len(mt) // 16 == PAD_METATILE:
        mt += entry
        at += attr
    else:
        mt[PAD_METATILE * 16:PAD_METATILE * 16 + 16] = entry
        at[PAD_METATILE * 4:PAD_METATILE * 4 + 4] = attr
    (TS / "metatiles.bin").write_bytes(mt)
    (TS / "metatile_attributes.bin").write_bytes(at)

    # Layouts (keep each spot's collision and elevation)
    for path, width, (x, y) in SPOTS:
        data = bytearray((ROOT / path).read_bytes())
        old = struct.unpack_from("<H", data, (y * width + x) * 2)[0]
        struct.pack_into("<H", data, (y * width + x) * 2, (old & 0xFC00) | (NUM_PRIMARY_METATILES + PAD_METATILE))
        (ROOT / path).write_bytes(data)
    print("heal pad: metatile", NUM_PRIMARY_METATILES + PAD_METATILE, "behavior", behavior_value())


if __name__ == "__main__":
    main()
