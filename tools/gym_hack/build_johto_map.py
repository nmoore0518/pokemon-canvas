#!/usr/bin/env python3
"""The Johto region map (Gym Trainer hack), from Pokémon HnS's.

Takes HnS's Johto map (graphics/pokenav/region_map/map_johto.*, an 8bpp affine background) and
dims everything outside the part of Johto the player may visit (the convention area: the map
sections the Johto import brings in), in a few muted shades so it still fits the background's
256 tiles. Also writes the cursor's map section grid (src/data/region_map/region_map_layout_johto.h,
with only our map sections; the rest are MAPSEC_NONE) and copies the Pokédex area map.

    python3 tools/gym_hack/build_johto_map.py [path/to/pokehns-expansion]
"""
import re
import shutil
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("/home/claude/ext/pokehns-expansion")
OUT = ROOT / "graphics/pokenav/region_map"

ALLOWED = ["MAPSEC_OLIVINE_CITY", "MAPSEC_OLIVINE_LIGHTHOUSE", "MAPSEC_ECRUTEAK_CITY", "MAPSEC_BURNED_TOWER",
           "MAPSEC_TIN_TOWER", "MAPSEC_GOLDENROD_CITY", "MAPSEC_NATIONAL_PARK", "MAPSEC_ILEX_FOREST",
           "MAPSEC_AZALEA_TOWN", "MAPSEC_ROUTE_34", "MAPSEC_ROUTE_35", "MAPSEC_ROUTE_36", "MAPSEC_ROUTE_37",
           "MAPSEC_ROUTE_38", "MAPSEC_ROUTE_39", "MAPSEC_ROUTE_40"]
X_MIN, Y_MIN, W, H = 1, 2, 28, 15       # where the cursor's grid sits on the map (region_map.c)
VIS_W, VIS_H = 30, 20                   # the visible part of the 64x64 tilemap
SHADES = 5
PAL_BASE = 112                          # the map's colors are palette entries 112-159 (3 palettes)
DARK = [(40, 48, 72), (58, 68, 96), (78, 90, 120), (104, 116, 144), (136, 146, 170)]   # dark -> light


def hns_grid():
    layout = (SRC / "src/data/region_map/region_map_layout_johto.h").read_text()
    return [[c.strip() for c in r.split(",")] for r in re.findall(r"\{(MAPSEC[^}]*)\}", layout)]


def positions():
    """Our map sections' places on the Johto map: id -> (x, y, width, height)."""
    pos = {}
    for y, row in enumerate(hns_grid()):
        for x, c in enumerate(row):
            if c in ALLOWED:
                p = pos.setdefault(c, [x, y, x, y])
                p[0], p[1], p[2], p[3] = min(p[0], x), min(p[1], y), max(p[2], x), max(p[3], y)
    return {k: (v[0], v[1], v[2] - v[0] + 1, v[3] - v[1] + 1) for k, v in pos.items()}


def main():
    grid = hns_grid()
    cells = {(x, y) for y, row in enumerate(grid) for x, c in enumerate(row) if c in ALLOWED}

    def lit(cx, cy):   # our map sections, and the empty land/sea next to them (not other places)
        if not (0 <= cx < W and 0 <= cy < H):
            return False
        if grid[cy][cx] in ALLOWED:
            return True
        return grid[cy][cx] == "MAPSEC_NONE" and any(abs(cx - a) <= 1 and abs(cy - b) <= 1 for a, b in cells)

    b = SRC / "graphics/pokenav/region_map"
    im = Image.open(b / "map_johto.png")
    tilemap = (b / "map_johto.bin").read_bytes()
    px, pp = im.load(), im.getpalette()
    tiles = [tuple(px[(t % 16) * 8 + x, (t // 16) * 8 + y] for y in range(8) for x in range(8)) for t in range(256)]
    used = sorted({c for t in tiles for c in t if c})
    first_dark = max(used) + 1
    assert first_dark + SHADES <= PAL_BASE + 48
    lum = {c: 0.3 * pp[c * 3] + 0.59 * pp[c * 3 + 1] + 0.11 * pp[c * 3 + 2] for c in used}
    lo, hi = min(lum.values()), max(lum.values())
    shade = {c: first_dark + min(SHADES - 1, int((lum[c] - lo) / (hi - lo + 1e-9) * SHADES)) for c in used}

    new_tiles, index, new_map = [], {}, bytearray(len(tilemap))
    for i, t in enumerate(tilemap):
        x, y = i % 64, i // 64
        if x >= VIS_W or y >= VIS_H:
            continue                                   # (never shown)
        tile = tiles[t]
        if not lit(x - X_MIN, y - Y_MIN):
            tile = tuple(shade[c] if c else 0 for c in tile)
        if tile not in index:
            index[tile] = len(new_tiles)
            new_tiles.append(tile)
        new_map[i] = index[tile]
    assert len(new_tiles) <= 256, len(new_tiles)

    out = Image.new("P", (128, 128), 0)
    pal = pp[:] + [0] * (768 - len(pp))
    for s, rgb in enumerate(DARK):
        pal[(first_dark + s) * 3:(first_dark + s) * 3 + 3] = list(rgb)
    out.putpalette(pal)
    op = out.load()
    for n, tile in enumerate(new_tiles):
        for k, c in enumerate(tile):
            op[(n % 16) * 8 + k % 8, (n // 16) * 8 + k // 8] = c
    out.save(OUT / "map_johto.png")
    (OUT / "map_johto.bin").write_bytes(bytes(new_map))
    lines = ["JASC-PAL", "0100", "48"] + [" ".join(str(v) for v in pal[(PAL_BASE + i) * 3:(PAL_BASE + i) * 3 + 3]) for i in range(48)]
    (OUT / "map_johto.pal").write_text("\r\n".join(lines) + "\r\n")

    # The cursor's grid: only our map sections
    rows = []
    for row in grid:
        rows.append("    {" + ", ".join(c if c in ALLOWED else "MAPSEC_NONE" for c in row) + "},")
    (ROOT / "src/data/region_map/region_map_layout_johto.h").write_text(
        "// Johto's map sections on the region map (tools/gym_hack/build_johto_map.py, from Pokémon HnS)\n"
        "static const u8 sRegionMapSections_Johto[MAP_HEIGHT][MAP_WIDTH] = {\n" + "\n".join(rows) + "\n};\n")

    for f in ("region_map_johto.png", "region_map_johto.pal", "region_map_johto.bin"):
        shutil.copy(SRC / "graphics/pokedex" / f, ROOT / "graphics/pokedex" / f)
    print(f"Johto map: {len(new_tiles)} tiles, dark colors {first_dark}-{first_dark + SHADES - 1}")


if __name__ == "__main__":
    main()
