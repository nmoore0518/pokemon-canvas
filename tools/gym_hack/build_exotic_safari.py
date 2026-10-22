#!/usr/bin/env python3
"""The Exotic Pokémon Safari (Pokémon Canvas): a second door on Fuchsia's Safari Zone building leads
to a hall with one door per region, each with the region's name on a plaque above it. Only the
open zones' doors open (for now, Paldea's: a small test meadow); the others are shut.

    python3 tools/gym_hack/build_exotic_safari.py [preview_dir]

Writes:
  data/tilesets/secondary/exotic_safari_frlg/  the Safari Zone building's tileset plus the plaques,
                                                plain wall pieces and shut doors (drawn here)
  data/layouts/ExoticSafari_*/, layouts.json    the hall and the zones
  data/maps/ExoticSafari_*/map.json             (their scripts.inc are kept if they exist)
  data/maps/map_groups.json                     gMapGroup_ExoticSafari
  data/maps/exotic_safari.inc                   the maps' scripts, included by data/event_scripts.s
  FuchsiaCity_Frlg: the door (the Safari Office's door, in place of a window), the sign, the warp
  src/data/wild_encounters.json                 each zone's Pokémon (ZONES below)
"""
import json
import os
import shutil
import struct
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SRC_TS = ROOT / "data/tilesets/secondary/safari_zone_building_frlg"
TS = ROOT / "data/tilesets/secondary/exotic_safari_frlg"
SEC = 0x280          # first secondary metatile id; first secondary tile is 640
PAL = 8              # the wall/door palette (index 1 dark slate, 2 grey, 3 wall, 5 frame, 6 olive, 7 brown, b white)

# The regions, west to east, and which have a zone open (name -> its map). The hall's doors.
REGIONS = ["JOHTO", "HOENN", "SINNOH", "UNOVA", "KALOS", "ALOLA", "GALAR", "PALDEA"]
# The regions with a zone open: their first area (the hall's door leads there). The areas
# themselves: tools/gym_hack/paldea.py
ZONES = {
    "HOENN": {"id": "MAP_EXOTIC_SAFARI_HOENN_FORTREE"},
    "SINNOH": {"id": "MAP_EXOTIC_SAFARI_SINNOH_LAKE"},
    "UNOVA": {"id": "MAP_EXOTIC_SAFARI_UNOVA_SKYARROW"},
    "KALOS": {"id": "MAP_EXOTIC_SAFARI_KALOS_CITY"},
    "ALOLA": {"id": "MAP_EXOTIC_SAFARI_ALOLA_RESORT"},
    "GALAR": {"id": "MAP_EXOTIC_SAFARI_GALAR_MEADOW"},
    "PALDEA": {"id": "MAP_EXOTIC_SAFARI_PALDEA_MEADOW"},
}
# Each door in its region's games' two colors (left half, right half)
DOOR_COLORS = {
    "JOHTO": ((232, 184, 48), (176, 184, 200)),     # Gold, Silver
    "HOENN": ((200, 32, 48), (40, 88, 208)),        # Ruby, Sapphire
    "SINNOH": ((88, 152, 232), (232, 160, 200)),    # Diamond, Pearl
    "UNOVA": ((56, 56, 64), (240, 240, 232)),       # Black, White
    "KALOS": ((40, 104, 208), (216, 48, 64)),       # X, Y
    "ALOLA": ((240, 144, 32), (96, 72, 200)),       # Sun, Moon
    "GALAR": ((48, 168, 232), (200, 40, 120)),      # Sword, Shield
    "PALDEA": ((216, 48, 40), (136, 64, 200)),      # Scarlet, Violet
}
DOOR_PALETTES = [7, 10, 11, 12]        # two doors per palette (free in this copy of the tileset)
DOOR_SPACING, FIRST_DOOR = 4, 3
HALL_W, HALL_H = FIRST_DOOR * 2 + DOOR_SPACING * (len(REGIONS) - 1) + 1, 12
MAT_X = HALL_W // 2                    # the entrance mat's middle, and the gap in the counter
DOOR_Y, COUNTER_Y = 3, 7
KIOSK_X = MAT_X + 6
FUCHSIA_DOOR = (26, 5)                 # the new door on the Safari Zone building (a window before)
FUCHSIA_SIGN = (27, 6)

# 5x7 letters for the plaques
FONT = {
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01111"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "J": ["00111", "00010", "00010", "00010", "00010", "10010", "01100"],
    "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
    "W": ["10001", "10001", "10001", "10101", "10101", "10101", "01010"],
    "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
}


# ---------------------------------------------------------------------------------------------
# The tileset
# ---------------------------------------------------------------------------------------------
class Tileset:
    def __init__(self):
        self.img = Image.open(SRC_TS / "tiles.png")
        self.tiles = []
        w = self.img.width // 8
        px = self.img.load()
        for t in range(w * (self.img.height // 8)):
            self.tiles.append(tuple(px[(t % w) * 8 + x, (t // w) * 8 + y] for y in range(8) for x in range(8)))
        raw = (SRC_TS / "metatiles.bin").read_bytes()
        self.metatiles = [list(struct.unpack_from("<8H", raw, i * 16)) for i in range(len(raw) // 16)]
        raw = (SRC_TS / "metatile_attributes.bin").read_bytes()
        self.attrs = [struct.unpack_from("<I", raw, i * 4)[0] for i in range(len(raw) // 4)]

    def tile(self, entry):
        """The 8x8 pixels of a metatile entry (flips applied)"""
        t = self.tiles[(entry & 0x3FF) - 640]
        rows = [list(t[y * 8:y * 8 + 8]) for y in range(8)]
        if entry >> 10 & 1:
            rows = [r[::-1] for r in rows]
        if entry >> 11 & 1:
            rows = rows[::-1]
        return rows

    def pixels(self, mid):
        """A metatile's bottom layer as 16x16 palette indices"""
        e = self.metatiles[mid - SEC]
        out = [[0] * 16 for _ in range(16)]
        for k in range(4):
            t = self.tile(e[k])
            for y in range(8):
                for x in range(8):
                    out[(k // 2) * 8 + y][(k % 2) * 8 + x] = t[y][x]
        return out

    def add(self, pixels, attr=0, pal=PAL):
        """A new metatile (bottom layer only) from 16x16 pixels; returns its id"""
        entries = []
        for k in range(4):
            t = tuple(pixels[(k // 2) * 8 + y][(k % 2) * 8 + x] for y in range(8) for x in range(8))
            if t not in self.tiles:
                self.tiles.append(t)
            entries.append((self.tiles.index(t) + 640) | (pal << 12))
        self.metatiles.append(entries + [0, 0, 0, 0])
        self.attrs.append(attr)
        return SEC + len(self.metatiles) - 1

    def write(self):
        if TS.exists():
            shutil.rmtree(TS)
        shutil.copytree(SRC_TS, TS)
        cols = 16
        rows = (len(self.tiles) + cols - 1) // cols
        img = Image.new("P", (cols * 8, rows * 8))
        img.putpalette(self.img.getpalette())
        px = img.load()
        for n, t in enumerate(self.tiles):
            for k, v in enumerate(t):
                px[(n % cols) * 8 + k % 8, (n // cols) * 8 + k // 8] = v
        img.save(TS / "tiles.png")
        (TS / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *m) for m in self.metatiles))
        (TS / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in self.attrs))
        # The door palettes: the wall palette's colors, with two doors' colors in the free slots
        base = [tuple(map(int, l.split())) for l in (SRC_TS / "palettes/08.pal").read_text().split("\n")[3:] if l.strip()]
        for i, pal in enumerate(DOOR_PALETTES):
            cols = list(base)
            for slot, name in enumerate(REGIONS[i * 2:i * 2 + 2]):
                for half, c in enumerate(DOOR_COLORS[name]):
                    base_i, shade_i = DOOR_INDICES[slot][half]
                    cols[base_i] = c
                    cols[shade_i] = tuple(int(v * 0.68) for v in c) if sum(c) > 150 else tuple(min(255, v + 40) for v in c)
            (TS / f"palettes/{pal:02d}.pal").write_text("JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{r} {g} {b}\r\n" for r, g, b in cols))
        assert len(self.tiles) <= 384 and len(self.metatiles) <= 384, (len(self.tiles), len(self.metatiles))


# Palette indices of a door's colors: slot (first or second door in the palette) -> (left, right)
# -> (base, shade). The rest (1 dark, 2 grey, 3 wall, 5 frame) are the wall palette's.
DOOR_INDICES = [((4, 6), (7, 8)), ((9, 10), (11, 12))]


def fill(c):
    return [[c] * 16 for _ in range(16)]


def recolor(p, mapping):
    return [[mapping.get(v, v) for v in row] for row in p]


def build_tileset():
    ts = Tileset()
    m = {}
    m["wall"] = ts.add(fill(3))                                            # plain wall
    base = ts.pixels(0x28F)                                               # wall with the baseboard
    m["wall_base"] = ts.add(base)
    # Door tops without the white band above them (a plain wall row goes above)
    for k, src in (("top_l", 0x283), ("top_m", 0x284), ("top_r", 0x285)):
        p = ts.pixels(src)
        for y in range(8):
            p[y] = [3] * 16
        m[k] = ts.add(p)
    # Closed doors, in the region's colors: the doorway's dark (index 1) becomes the door, left
    # half one color and right half the other, with darker edges, a seam and handles.
    # The door runs 3 pixels into the left and right metatiles: door x -3..18 (0-15 the middle one).
    pieces = (("top_l", m["top_l"], -16), ("top_m", m["top_m"], 0), ("top_r", m["top_r"], 16),
              ("door_l", 0x28B, -16), ("door_m", 0x28C, 0), ("door_r", 0x28D, 16))
    for i, name in enumerate(REGIONS):
        pal, slot = DOOR_PALETTES[i // 2], i % 2
        for k, src, off in pieces:
            p = ts.pixels(src)
            top = k.startswith("top")
            for y in range(16):
                for x in range(16):
                    if p[y][x] != 1:
                        continue
                    gx = x + off
                    if not -3 <= gx <= 18:
                        continue                                           # (the baseboard)
                    half = 0 if gx < 8 else 1
                    base_i, shade_i = DOOR_INDICES[slot][half]
                    shade = gx in (-3, 7, 8, 18) or (not top and y in (4, 5, 6) and gx in (5, 10))
                    p[y][x] = shade_i if shade else base_i
            m[f"closed_{name}_{k}"] = ts.add(p, pal=pal)
    # The counter between the lobby and the doors (a counter: people talk across it)
    counter = [[9] * 16] + [[0xA] * 16 for _ in range(8)] + [[9] * 16] + [[7] * 16 for _ in range(5)] + [[6] * 16]
    m["counter"] = ts.add(counter, attr=0x80)
    # The plaques: the region's name in white on dark slate, in a light frame, over the wall
    for name in REGIONS:
        img = fill(3) + []
        img = [[3] * 48 for _ in range(16)]
        for y in range(2, 14):
            for x in range(2, 46):
                edge = y in (2, 13) or x in (2, 45)
                img[y][x] = 5 if edge else 1
        for x in range(3, 46):
            img[14][x] = 2                                                # shadow under it
        width = len(name) * 6 - 1
        x0, y0 = 24 - width // 2, 4
        for i, ch in enumerate(name):
            for y, row in enumerate(FONT[ch]):
                for x, bit in enumerate(row):
                    if bit == "1":
                        img[y0 + y][x0 + i * 6 + x] = 0xB
        for part in range(3):
            m[f"plaque_{name}_{part}"] = ts.add([row[part * 16:part * 16 + 16] for row in img])
    ts.write()
    return m


def set_tileset_callback(name, callback):
    """gTileset_<name>'s animation callback (headers.h), e.g. a palette that pulses"""
    import re
    path = ROOT / "src/data/tilesets/headers.h"
    s = path.read_text()
    start = s.index(f"const struct Tileset gTileset_{name} =")
    end = s.index("};", start)
    block = re.sub(r"\.callback = \w+", f".callback = {callback}", s[start:end])
    path.write_text(s[:start] + block + s[end:])


def register_tileset(name="ExoticSafari", dirname="exotic_safari_frlg"):
    """headers.h, graphics.h, metatiles.h: gTileset_<name> (a FireRed secondary tileset, like
    gTileset_SafariZoneBuilding)"""
    def put(path, anchor, text):
        s = path.read_text()
        if text.splitlines()[0] in s:
            return
        i = s.index(anchor)
        path.write_text(s[:i] + text + s[i:])
    d = f"data/tilesets/secondary/{dirname}"
    put(ROOT / "src/data/tilesets/headers.h", "const struct Tileset gTileset_SafariZoneBuilding =",
        f"const struct Tileset gTileset_{name} =\n{{\n    .isCompressed = TRUE,\n    .isSecondary = TRUE,\n"
        f"    .tiles = gTilesetTiles_{name},\n    .palettes = gTilesetPalettes_{name},\n"
        f"    .metatiles = gMetatiles_{name},\n    .metatileAttributes = gMetatileAttributes_{name},\n"
        "    .callback = NULL,\n};\n\n")
    put(ROOT / "src/data/tilesets/graphics.h", "const u32 gTilesetTiles_SafariZoneBuilding[]",
        f'const u32 gTilesetTiles_{name}[] = INCGFX_U32("{d}/tiles.png", ".4bpp.fastSmol");\n\n'
        f"const u16 gTilesetPalettes_{name}[][16] =\n{{\n" + "".join(
            f'    INCGFX_U16("{d}/palettes/{i:02d}.pal", ".gbapal"),\n' for i in range(16))
        + "};\n\n")
    put(ROOT / "src/data/tilesets/metatiles.h", "const u16 gMetatiles_SafariZoneBuilding[]",
        f'const u16 gMetatiles_{name}[] = INCBIN_U16("{d}/metatiles.bin");\n'
        f'const u16 gMetatileAttributes_{name}[] = INCBIN_U16("{d}/metatile_attributes.bin");\n')


def convert_layout(grid, tilesets, version, name, dirname, behaviors=None, recolor=None, edit=None):
    """A layout's piece (Emerald's or FireRed's) -> a FireRed one with its own secondary tileset: the
    metatiles it uses copied into gTileset_<name> (over FireRed's general primary, unused). Tileset
    animations (water, flowers) don't come along: those tiles stay still.
    behaviors: {source metatile id: behavior name} to change (e.g. wild Pokémon indoors);
    recolor: {palette index (source): function (r, g, b) -> (r, g, b)}"""
    from collections import OrderedDict
    emerald = version == "emerald"
    nprim, npal = (512, 6) if emerald else (640, 7)
    pd, sd = (render_map.tileset_dir(s) for s in tilesets)
    def metatiles(d):
        b = (d / "metatiles.bin").read_bytes()
        return [struct.unpack_from("<8H", b, i) for i in range(0, len(b), 16)]
    def attrs(d):
        b = (d / "metatile_attributes.bin").read_bytes()
        fmt = "H" if emerald else "I"
        return struct.unpack(f"<{len(b) // struct.calcsize(fmt)}{fmt}", b)
    pm, sm, pa, sa = metatiles(pd), metatiles(sd), attrs(pd), attrs(sd)
    pimg, simg = Image.open(pd / "tiles.png"), Image.open(sd / "tiles.png")
    used = sorted({v & 0x3FF for row in grid for v in row})
    assert len(used) <= 384
    def pal_colors(p):
        return render_map.load_pal((pd if p < npal else sd) / "palettes" / f"{p:02d}.pal")
    # More than 6 palettes: the least used ones fold into the closest kept one (their tiles' colors
    # matched to it, pixel by pixel)
    from collections import Counter
    use = Counter(e >> 12 for m in used for e in (pm[m] if m < nprim else sm[m - nprim]))
    fold = {}
    while len(use) - len(fold) > 6:
        drop = min((p for p in use if p not in fold), key=lambda p: use[p])
        keep = [p for p in use if p not in fold and p != drop]
        dist = lambda a, b: sum((x - y) ** 2 for ca, cb in zip(a[1:], b[1:]) for x, y in zip(ca[:3], cb[:3]))
        fold[drop] = min(keep, key=lambda p: dist(pal_colors(drop), pal_colors(p)))
    for p in list(fold):                     # (a palette folded into one later folded: its last stop)
        while fold[p] in fold:
            fold[p] = fold[fold[p]]
    tiles, pals = OrderedDict(), OrderedDict()
    new_mt, new_attr, remap = [], [], {}
    for k, m in enumerate(used):
        entries = pm[m] if m < nprim else sm[m - nprim]
        out = []
        for e in entries:
            t, flips, pal = e & 0x3FF, e & 0x0C00, e >> 12
            key = (t, pal) if pal in fold else t
            if pal in fold:
                pal = fold[pal]
            if key not in tiles:
                tiles[key] = len(tiles)
            if pal not in pals:
                pals[pal] = len(pals)
            out.append((640 + tiles[key]) | flips | ((7 + pals[pal]) << 12))
        new_mt.append(out)
        a = pa[m] if m < nprim else sa[m - nprim]
        attr = (a & 0xFF) | (((a >> 12) & 0xF) << 29) if emerald else a
        if behaviors and m in behaviors:
            attr = (attr & ~0x1FF) | MB[behaviors[m]]
        new_attr.append(attr)
        remap[m] = 0x280 + k
    assert len(tiles) <= 384 and len(pals) <= 6, (name, len(tiles), len(pals))
    out = ROOT / "data/tilesets/secondary" / dirname
    if out.exists():
        shutil.rmtree(out)
    (out / "palettes").mkdir(parents=True, exist_ok=True)
    n = len(tiles)
    img = Image.new("P", (128, ((n + 15) // 16) * 8))
    img.putpalette(pimg.getpalette())
    for key, i in tiles.items():
        t = key[0] if isinstance(key, tuple) else key
        src, ti = (pimg, t) if t < nprim else (simg, t - nprim)
        tile = src.crop(((ti % 16) * 8, (ti // 16) * 8, (ti % 16) * 8 + 8, (ti // 16) * 8 + 8))
        if isinstance(key, tuple):                  # (a folded palette's tile: nearest colors)
            a, b = pal_colors(key[1]), pal_colors(fold[key[1]])
            near = [0] + [min(range(1, 16), key=lambda j: sum((x - y) ** 2 for x, y in zip(a[c][:3], b[j][:3])))
                          for c in range(1, 16)]
            tile = tile.point(lambda c: near[c] if c < 16 else c)
        img.paste(tile, ((i % 16) * 8, (i // 16) * 8))
    img.save(out / "tiles.png")
    black = "JASC-PAL\n0100\n16\n" + "0 0 0\n" * 16
    for i in range(16):
        (out / "palettes" / f"{i:02d}.pal").write_text(black)
    for p, i in pals.items():
        src = (pd if p < npal else sd) / "palettes" / f"{p:02d}.pal"
        colors = render_map.load_pal(src)
        if recolor and p in recolor:
            colors = [recolor[p](*c[:3]) for c in colors]
        write_pal(out / "palettes" / f"{7 + i:02d}.pal", colors)
    (out / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *e) for e in new_mt))
    (out / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in new_attr))
    if edit:
        edit(out, remap)                            # (the area's own changes: new tiles, text)
    register_tileset(name, dirname)
    g = [[(v & ~0x3FF) | remap[v & 0x3FF] for v in row] for row in grid]
    return g, ("gTileset_General_Frlg", f"gTileset_{name}")


def clone_tileset(symbol, new_name, recolor=None, edit=None, callback=None):
    """A copy of a tileset (gTileset_<new_name>, in data/tilesets/<primary|secondary>/<new_name
    lowercased>), its palettes recolored: recolor(palette index, (r, g, b)) -> (r, g, b). The copy
    keeps the original's animation callback. edit(dir): a chance to change its files (add metatiles,
    tiles). Returns the new symbol."""
    import re
    headers = (ROOT / "src/data/tilesets/headers.h").read_text()
    start = headers.index(f"const struct Tileset {symbol} =")
    block = headers[start:headers.index("};", start) + 3]
    src_dir = render_map.tileset_dir(symbol)
    kind = src_dir.parent.name
    dirname = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", new_name).lower()
    out = ROOT / "data/tilesets" / kind / dirname
    if out.exists():
        shutil.rmtree(out)
    shutil.copytree(src_dir, out, ignore=shutil.ignore_patterns("anim", "*.4bpp", "*.gbapal", "*.lz", "*.smol*", "*.fastSmol"))
    if recolor:
        nprim = 7
        for f in sorted((out / "palettes").glob("*.pal")):
            i = int(f.stem)
            if (kind == "primary" and i >= nprim) or (kind == "secondary" and i < nprim):
                continue
            write_pal(f, [recolor(i, c[:3]) for c in render_map.load_pal(f)])
    for i in range(16):                          # (all 16 palette files, black where the original had none)
        if not (out / "palettes" / f"{i:02d}.pal").exists():
            write_pal(out / "palettes" / f"{i:02d}.pal", [(0, 0, 0)] * 16)
    for f in (out / "palettes").glob("*.pla"):
        f.unlink()
    if edit:
        edit(out)
    tiles_sym = block.split(".tiles = ")[1].split(",")[0].strip()
    pals_sym = block.split(".palettes = ")[1].split(",")[0].strip()
    mt_sym = block.split(".metatiles = ")[1].split(",")[0].strip()
    at_sym = block.split(".metatileAttributes = ")[1].split(",")[0].strip()
    nb = (block.replace(symbol, f"gTileset_{new_name}").replace(tiles_sym, f"gTilesetTiles_{new_name}")
          .replace(pals_sym, f"gTilesetPalettes_{new_name}").replace(mt_sym, f"gMetatiles_{new_name}")
          .replace(at_sym, f"gMetatileAttributes_{new_name}"))
    if callback:
        nb = re.sub(r"\.callback = \w+", f".callback = {callback}", nb)
    rel = f"data/tilesets/{kind}/{dirname}"
    def put(path, marker, text):
        s = path.read_text()
        begin, end = f"// <clone {new_name}>\n", f"// </clone {new_name}>\n"
        if begin in s:
            s = s[:s.index(begin)] + s[s.index(end) + len(end):]
        path.write_text(s + begin + text + end)
    put(ROOT / "src/data/tilesets/headers.h", None, nb + "\n")
    put(ROOT / "src/data/tilesets/graphics.h", None,
        f'const u32 gTilesetTiles_{new_name}[] = INCGFX_U32("{rel}/tiles.png", ".4bpp.fastSmol");\n'
        f"const u16 gTilesetPalettes_{new_name}[][16] =\n{{\n" + "".join(
            f'    INCGFX_U16("{rel}/palettes/{i:02d}.pal", ".gbapal"),\n' for i in range(16)) + "};\n")
    put(ROOT / "src/data/tilesets/metatiles.h", None,
        f'const u16 gMetatiles_{new_name}[] = INCBIN_U16("{rel}/metatiles.bin");\n'
        f'const u16 gMetatileAttributes_{new_name}[] = INCBIN_U16("{rel}/metatile_attributes.bin");\n')
    return f"gTileset_{new_name}"


def write_pal(path, colors):
    colors = [tuple(max(0, min(255, int(c))) for c in col[:3]) for col in colors] + [(0, 0, 0)] * (16 - len(colors))
    path.write_text("JASC-PAL\n0100\n16\n" + "".join(f"{r} {g} {b}\n" for r, g, b in colors[:16]))


# ---------------------------------------------------------------------------------------------
# Maps
# ---------------------------------------------------------------------------------------------
def blk(m, collision, elevation=3):
    return m | (collision << 10) | (elevation << 12)


def door_x(i):
    return FIRST_DOOR + i * DOOR_SPACING


def build_hall(m):
    W, H = HALL_W, HALL_H
    g = [[blk(0x2A2, 0)] * W for _ in range(H)]                            # floor
    for x in range(W):
        g[0][x] = blk(0x282, 1)                                            # the wall's top band
        g[1][x] = blk(m["wall"], 1)
        g[2][x] = blk(m["wall"], 1)
        g[3][x] = blk(m["wall_base"], 1)
        g[H - 1][x] = blk(0x008, 1, 0)                                     # (outside the room)
    for i, name in enumerate(REGIONS):
        x = door_x(i)
        for part in range(3):
            g[1][x - 1 + part] = blk(m[f"plaque_{name}_{part}"], 1)
        # Every door starts closed; an open zone's door slides open when the player stands in front
        # of it (src/exotic_safari.c)
        g[2][x - 1:x + 2] = [blk(m[f"closed_{name}_top_{s}"], 1) for s in "lmr"]
        g[3][x - 1:x + 2] = [blk(m[f"closed_{name}_door_{s}"], 1) for s in "lmr"]
    # The counter between the lobby and the doors, with a gap in the middle (the attendant stands in
    # it until the player buys a pass)
    for x in range(W):
        if x != MAT_X:
            g[COUNTER_Y][x] = blk(m["counter"], 1)
    # The entrance mat (warps out to Fuchsia) and the doorway shadow under it
    g[H - 2][MAT_X - 1:MAT_X + 2] = [blk(0x2BC, 0), blk(0x2BD, 0), blk(0x2BE, 0)]
    g[H - 1][MAT_X - 1:MAT_X + 2] = [blk(0x01A, 1, 0), blk(0x01B, 1, 0), blk(0x01C, 1, 0)]
    # Potted plants in the corners (the safari entrance's)
    for x in (0, W - 1):
        g[3][x] = blk(0x28F, 1)
        g[4][x] = blk(0x297, 1)
    return g


def write_layout(name, lid, grid, primary, secondary, border=None):
    W, H = len(grid[0]), len(grid)
    d = ROOT / "data/layouts" / name
    d.mkdir(parents=True, exist_ok=True)
    (d / "map.bin").write_bytes(b"".join(struct.pack("<H", v) for row in grid for v in row))
    if border is None:
        border = grid[0][0]
    if isinstance(border, int):
        border = [[border] * 2] * 2
    bh, bw = len(border), len(border[0])            # (a block that repeats beyond the edges, aligned
    flat = [v for row in border for v in row]       #  with the map: x % bw, y % bh)
    (d / "border.bin").write_bytes(struct.pack(f"<{len(flat)}H", *flat))
    lj_path = ROOT / "data/layouts/layouts.json"
    lj = json.loads(lj_path.read_text())
    entry = {"id": lid, "name": f"{name}_Layout", "width": W, "height": H, "border_width": bw, "border_height": bh,
             "primary_tileset": primary, "secondary_tileset": secondary,
             "border_filepath": f"data/layouts/{name}/border.bin", "blockdata_filepath": f"data/layouts/{name}/map.bin",
             "layout_version": "frlg"}
    lj["layouts"] = [l for l in lj["layouts"] if l.get("id") != lid] + [entry]
    lj_path.write_text(json.dumps(lj, indent=2, ensure_ascii=False) + "\n")


def obj(gfx, x, y, script, movement="MOVEMENT_TYPE_FACE_DOWN", flag="0"):
    return {"type": "object", "graphics_id": gfx, "x": x, "y": y, "elevation": 3, "movement_type": movement,
            "movement_range_x": 1, "movement_range_y": 1, "trainer_type": "TRAINER_TYPE_NONE",
            "trainer_sight_or_berry_tree_id": "0", "script": script, "flag": flag}


def write_map(name, mid, layout, music, mapsec, map_type, objects, warps, bgs, show_name, coords=(), weather="WEATHER_NONE"):
    d = ROOT / "data/maps" / name
    d.mkdir(parents=True, exist_ok=True)
    m = {"id": mid, "name": name, "layout": layout, "music": music, "region": "REGION_KANTO",
         "region_map_section": mapsec, "requires_flash": False, "weather": weather, "map_type": map_type,
         "allow_cycling": map_type != "MAP_TYPE_INDOOR", "allow_escaping": False, "allow_running": True,
         "show_map_name": show_name, "floor_number": 0, "battle_scene": "MAP_BATTLE_SCENE_NORMAL",
         "connections": None, "object_events": objects, "warp_events": warps, "coord_events": list(coords),
         "bg_events": bgs}
    (d / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    if not (d / "scripts.inc").exists():
        (d / "scripts.inc").write_text(f"{name}_MapScripts::\n\t.byte 0\n")


# ---------------------------------------------------------------------------------------------
# The zones' areas (tools/gym_hack/paldea.py: layouts, links, Pokémon, legendaries, items)
# ---------------------------------------------------------------------------------------------
import re
import random

sys.path.insert(0, str(Path(__file__).resolve().parent))
import paldea                                   # noqa: E402
import galar                                    # noqa: E402
import alola                                    # noqa: E402
import kalos                                    # noqa: E402
import unova                                    # noqa: E402
import sinnoh                                   # noqa: E402
import hoenn                                    # noqa: E402
import safari_names                             # noqa: E402
import render_map                               # noqa: E402  (tileset_dir)

LEGEND_LEVEL = paldea.LEGEND_LEVEL
REGION_MODULES = [paldea, galar, alola, kalos, unova, sinnoh, hoenn]
KIOSK_SUPPLIES = ["POKE_BALL", "GREAT_BALL", "ULTRA_BALL", "QUICK_BALL", "DUSK_BALL", "NET_BALL",
                  "MAX_REPEL", "HYPER_POTION", "MAX_POTION", "FULL_HEAL", "REVIVE"]
ENCOUNTER_RATES = {"MAP_TYPE_ROUTE": 21, "MAP_TYPE_CITY": 15, "MAP_TYPE_UNDERGROUND": 10, "MAP_TYPE_INDOOR": 10, "MAP_TYPE_UNDERWATER": 10}
ROAMER_FLAG_BASE = 0x1B             # (the roamers' hide flags: FLAG_TEMP_1B-1F; statics' from FLAG_TEMP_11)
ROAMER_SPOTS = 48                    # (per area: a sample of its reachable grass/sand/cave floor)


def map_id(name):
    return "MAP_" + re.sub(r"(?<=[a-z])(?=[A-Z])", "_", name).upper()


def camel(s):
    return "".join(w.title() for w in s.split("_"))


def behavior_info():
    """MB_* values, and which have wild encounters / are water"""
    names = re.findall(r"^\s+(MB_\w+)", (ROOT / "include/constants/metatile_behaviors.h").read_text(), re.M)
    value = {n: i for i, n in enumerate(names)}
    src = (ROOT / "src/metatile_behavior.c").read_text()
    enc, surf = set(), set()
    for n, flags in re.findall(r"\[(MB_\w+)\]\s*=\s*([^,\n]+)", src):
        if "HAS_ENCOUNTERS" in flags:
            enc.add(value[n])
        if "SURFABLE" in flags:
            surf.add(value[n])
    return value, enc, surf


MB, MB_ENCOUNTERS, MB_SURFABLE = behavior_info()


class Behaviors:
    """metatile id -> behavior, for a layout's two tilesets"""
    def __init__(self, tilesets, version):
        self.emerald = version == "emerald"
        self.nprim = 512 if self.emerald else 640
        self.attrs = []
        for sym in tilesets:
            b = (render_map.tileset_dir(sym) / "metatile_attributes.bin").read_bytes()
            fmt = "H" if self.emerald else "I"
            self.attrs.append(struct.unpack(f"<{len(b) // struct.calcsize(fmt)}{fmt}", b))

    def __call__(self, block):
        m = block & 0x3FF
        ts, i = (self.attrs[0], m) if m < self.nprim else (self.attrs[1], m - self.nprim)
        if i >= len(ts):
            return 0
        return ts[i] & (0xFF if self.emerald else 0x1FF)


def floor_near(g, x, y, beh):
    """the most common open (dry) metatile around (x, y): to replace a ladder with"""
    from collections import Counter
    c = Counter(g[yy][xx] for yy in range(max(0, y - 3), min(len(g), y + 4)) for xx in range(max(0, x - 3), min(len(g[0]), x + 4))
                if not (g[yy][xx] >> 10) & 3 and beh(g[yy][xx]) not in MB_SURFABLE and (xx, yy) != (x, y))
    return c.most_common(1)[0][0]


def edge_run(g, row):
    """the longest run of open cells on a row (an area's way out at its top or bottom)"""
    best, run = [], []
    for x in range(len(g[0])):
        if not (g[row][x] >> 10) & 3:
            run.append(x)
            if len(run) > len(best):
                best = run[:]
        else:
            run = []
    return best


def patch_snow(tilesets):
    """HnS's snow has no encounters: its plain snow gets footprints (and wild Pokémon)"""
    d = render_map.tileset_dir(tilesets[1])
    b = bytearray((d / "metatile_attributes.bin").read_bytes())
    for m in (0x281,):
        i = (m - 640) * 4
        v = struct.unpack_from("<I", b, i)[0]
        struct.pack_into("<I", b, i, (v & ~0x1FF) | MB["MB_FOOTPRINTS"])
    (d / "metatile_attributes.bin").write_bytes(b)


def build_region(mod, zone_warp, out):
    """One region's zone (mod: tools/gym_hack/<region>.py): its areas' layouts, maps, scripts, wild
    Pokémon; adds its rows to `out` (the header's tables)"""
    areas = mod.AREAS
    region = mod.REGION
    built = {}
    # 1. Layouts
    for key, a in areas.items():
        g, w, h = a["build"]()
        beh0 = Behaviors(a["tilesets"], a.get("version", "frlg"))
        for x, y in a.get("remove_warps", []):
            floor = floor_near(g, x, y + 1, beh0)
            for yy in (y - 1, y):             # (ladders are two tiles tall)
                g[yy][x] = floor
        for x, y in a.get("remove_holes", []):     # (one tile: holes, doors)
            g[y][x] = floor_near(g, x, y, beh0)
        if "post" in a:
            a["post"](g, w, h)
        behaviors = dict(a.get("behaviors", {}))
        if a.get("indoor_encounters"):
            # Wild Pokémon indoors: the plain floor (walkable, MB_NORMAL) becomes MB_INDOOR_ENCOUNTER
            for row in g:
                for v in row:
                    if not (v >> 10) & 3 and beh0(v) in (MB["MB_NORMAL"], MB["MB_MOUNTAIN_TOP"]):
                        behaviors.setdefault(v & 0x3FF, "MB_INDOOR_ENCOUNTER")
        if a.get("patch_snow"):
            patch_snow(a["tilesets"])
        links = {}
        for lk, (kind, cells, arrival, target) in a["links"].items():
            if cells in ("top", "bottom"):
                row = 0 if cells == "top" else h - 1
                xs = edge_run(g, row)
                cells = [(x, row) for x in xs]
                arrival = (xs[len(xs) // 2], 1 if row == 0 else h - 2)
            links[lk] = (kind, cells, arrival, target)
            if (g[arrival[1]][arrival[0]] >> 10) & 3:
                print(f"WARNING {key}: the arrival from {lk} at {arrival} is blocked")
        tilesets = a["tilesets"]
        if a.get("version") == "emerald" or a.get("convert_frlg"):
            g, tilesets = convert_layout(g, tilesets, a.get("version", "frlg"), a["convert"][0], a["convert"][1],
                                         behaviors, a.get("recolor"), a.get("tileset_edit"))
            if "post_convert" in a:                  # (changes in the converted tileset's metatiles)
                a["post_convert"](g)
            set_tileset_callback(a["convert"][0], a.get("tileset_callback", "NULL"))
        for x, y in unstick(g, w, h, Behaviors(tilesets, "frlg"), links, f"{region} {key}"):
            print(f"  {region} {key}: opened the ledge at {(x, y)} (no getting stuck)")
        built[key] = {"g": g, "w": w, "h": h, "links": links, "beh": Behaviors(tilesets, "frlg")}
        # The border (beyond the map's edges): its most common wall (trees, rock), or as given
        from collections import Counter
        ring = [g[y][x] for y in range(h) for x in range(w) if (x in (0, w - 1) or y in (0, h - 1)) and (g[y][x] >> 10) & 3]
        border = a.get("border", Counter(ring).most_common(1)[0][0] if ring else g[0][0])
        if "border_at" in a:                     # (a cell of the map, as converted: e.g. open sea)
            border = g[a["border_at"][1]][a["border_at"][0]]
        elif "border" not in a:
            # Better: the map's own trees (or rock) carried on past its edges - the most common solid
            # block of the given size, at an aligned spot - so tree lines don't end in a cut
            bw, bh = a.get("border_size", (2, 2))
            blocks = Counter(tuple(tuple(g[y + j][x + i] for i in range(bw)) for j in range(bh))
                             for y in range(0, h - bh + 1, bh) for x in range(0, w - bw + 1, bw)
                             if all((g[y + j][x + i] >> 10) & 3 for i in range(bw) for j in range(bh)))
            if blocks:
                border = [list(r) for r in blocks.most_common(1)[0][0]]
        write_layout(a["map"], "LAYOUT_" + map_id(a["map"])[4:], g, tilesets[0], tilesets[1], border)

    # 2. Warps: each link's cells (warp tiles) or its arrival spot (edges: walking off is a trigger)
    for key, b in built.items():
        warps, arrive = [], {}
        for lk, (kind, cells, arrival, target) in b["links"].items():
            if kind == "warp":
                near = min(range(len(cells)), key=lambda i: abs(cells[i][0] - arrival[0]) + abs(cells[i][1] - arrival[1]))
                arrive[lk] = len(warps) + near
                for x, y in cells:
                    warps.append({"x": x, "y": y, "elevation": 3, "link": lk})
            else:
                arrive[lk] = len(warps)
                warps.append({"x": arrival[0], "y": arrival[1], "elevation": 3, "link": lk})
        b["warps"], b["arrive"] = warps, arrive
    hall_index, hall_warps = zone_warp[region]
    hall_warps[hall_index]["dest_map"] = map_id(areas[mod.HUB]["map"])
    hall_warps[hall_index]["dest_warp_id"] = str(built[mod.HUB]["arrive"]["HALL"])

    def dest(key, lk):
        tk, tl = areas[key]["links"][lk][3]
        if tk == "HALL":
            return "MAP_EXOTIC_SAFARI_HALL", hall_index
        return map_id(areas[tk]["map"]), built[tk]["arrive"][tl]

    stakes = getattr(mod, "STAKES", {})
    # 3. Each area's map: objects, warps, triggers, scripts
    for key, a in areas.items():
        b, name = built[key], a["map"]
        g, w, h = b["g"], b["w"], b["h"]
        elev = lambda x, y: (g[y][x] >> 12) & 0xF
        warps = []
        for wp in b["warps"]:
            dm, di = dest(key, wp["link"])
            kind = b["links"][wp["link"]][0]
            # (an edge's arrival spot is only somewhere to arrive: it leads back to where it came from)
            warps.append({"x": wp["x"], "y": wp["y"], "elevation": elev(wp["x"], wp["y"]) if kind == "warp" else 0,
                          "dest_map": dm, "dest_warp_id": str(di)})
        coords, scripts = [], [f"@ {name} (made by tools/gym_hack/build_exotic_safari.py from tools/gym_hack/{mod.__name__}.py)",
                                f"{name}_MapScripts::", f"\tmap_script MAP_SCRIPT_ON_TRANSITION, {name}_OnTransition",
                                f"\tmap_script MAP_SCRIPT_ON_LOAD, {name}_OnLoad",
                                "\t.byte 0", "", f"{name}_OnTransition::", "\tspecial ExoticSafari_AreaOnTransition", "\tend", "",
                                f"{name}_OnLoad::", "\tspecial ExoticSafari_AreaOnLoad", "\tend", ""]
        for lk, (kind, cells, arrival, target) in b["links"].items():
            if kind != "edge":
                continue
            dm, di = dest(key, lk)
            label = f"{name}_EventScript_To{camel(lk)}"
            for x, y in cells:
                coords.append({"type": "trigger", "x": x, "y": y, "elevation": 0, "var": "VAR_TEMP_1",
                               "var_value": "0", "script": label})
            scripts += [f"{label}::", f"\twarpsilent {dm}, {di}", "\twaitstate", "\tend", ""]
        # Objects: the roamers' slots (local ids 1-3), the legendaries, the items
        objects = []
        for i, r in enumerate(mod.ROAMERS if a.get("land") else []):   # (not in rooms without wild Pokémon)
            o = obj(f"OBJ_EVENT_GFX_SPECIES({r})", 0, 0, f"ExoticSafari_EventScript_Roamer{camel(r)}",
                    "MOVEMENT_TYPE_WANDER_AROUND", f"FLAG_TEMP_{ROAMER_FLAG_BASE + i:X}")
            o["movement_range_x"] = o["movement_range_y"] = 2
            objects.append(o)
        reach = reachable(b)
        if os.environ.get("SAFARI_REACH_DUMP"):     # (debugging: where the player can get to)
            dump = Path(os.environ["SAFARI_REACH_DUMP"]) / f"{name}.json"
            dump.write_text(json.dumps({"w": b["w"], "h": b["h"], "reach": sorted(reach)}))
        bgs = []
        for i, (species, (x, y)) in enumerate(a["statics"]):
            if (x, y) not in reach:
                print(f"WARNING {key}: {species} at {(x, y)} can't be reached")
            label = f"{name}_EventScript_Legend{camel(species)}"
            # Hidden (a temp flag, set on the way in: src/exotic_safari.c) once caught, until what it
            # waits for is done, and for the rest of the visit after a battle that didn't catch it
            o = obj(f"OBJ_EVENT_GFX_SPECIES({species})", x, y, label, "MOVEMENT_TYPE_FACE_DOWN", f"FLAG_TEMP_{0x11 + i:X}")
            o["elevation"] = elev(x, y)
            objects.append(o)
            assert 0x11 + i < ROAMER_FLAG_BASE
            pre = getattr(mod, "PRE_BATTLE", {}).get(species)     # (a word before the battle: may end it)
            scripts += [f"{label}::", "\tlock", "\tfaceplayer"] + ([f"\tcall {pre}"] if pre else []) + [
                        f"\tsetvar VAR_0x8004, SPECIES_{species}",
                        "\tcall ExoticSafari_EventScript_LegendBattle",
                        "\tgoto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_LegendGone",
                        "\tremoveobject VAR_LAST_TALKED", f"\tsetflag FLAG_EXOTIC_LEGEND_{species}",
                        "\tgoto ExoticSafari_EventScript_Release", ""]
            seal, arg = getattr(mod, "CONDITIONS", {}).get(species, ("NONE", 0))
            if seal in ("ROAMERS", "GUARDIANS"):     # (the roamers of its own region)
                arg = out["region_index"]
            out["statics"].append((name, species, i, (seal, arg), len(objects)))
            if species in getattr(mod, "CONDITIONS", {}):
                # Where it will be: a hint at what it waits for (talking to the spot while it isn't there)
                seal = f"{name}_EventScript_Seal{camel(species)}"
                bgs.append({"type": "sign", "x": x, "y": y, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
                            "script": seal})
                scripts += [f"{seal}::", "\tlockall", f"\tsetvar VAR_0x8004, {len(out['statics']) - 1}",
                            "\tgoto ExoticSafari_EventScript_Seal", ""]
        for item, (x, y) in a["items"]:
            if (x, y) not in reach:
                print(f"WARNING {key}: {item} at {(x, y)} can't be reached")
            label = f"{name}_EventScript_Item{camel(item)}"
            o = obj("OBJ_EVENT_GFX_ITEM_BALL", x, y, label, "MOVEMENT_TYPE_LOOK_AROUND", f"FLAG_EXOTIC_ITEM_{item}")
            o["elevation"] = elev(x, y)
            objects.append(o)
            scripts += [f"{label}::", f"\tfinditem ITEM_{item}", "\tend", ""]
            if item not in getattr(mod, "UNIQUE_ITEMS", set()):
                for sold in getattr(mod, "KIOSK_ALSO", {}).get(item, [item]):
                    out["finds"].append((sold, f"FLAG_EXOTIC_ITEM_{item}"))
        for c, color in enumerate(getattr(mod, "STAKE_COLORS", [])):
            for k, (area, (x, y)) in enumerate(stakes[color]):
                if area != key:
                    continue
                if (x, y) not in reach:
                    print(f"WARNING {key}: {color} stake at {(x, y)} can't be reached")
                o = obj(f"OBJ_EVENT_GFX_EXOTIC_STAKE_{color}", x, y, f"ExoticSafari_EventScript_Stake{color.title()}",
                        "MOVEMENT_TYPE_FACE_DOWN", f"0x{mod.STAKE_FLAG_BASE + c * 4 + k:03X}")
                o["elevation"] = elev(x, y)
                objects.append(o)
        for x, y in a.get("ropes", []):
            o = obj("OBJ_EVENT_GFX_VELVET_ROPE", x, y, "ExoticSafari_EventScript_Rope", "MOVEMENT_TYPE_FACE_DOWN")
            o["elevation"] = elev(x, y)
            objects.append(o)
        if "extra" in a:                         # (anything else: puzzles' signs, triggers, people)
            a["extra"]({"name": name, "objects": objects, "bgs": bgs, "coords": coords, "scripts": scripts,
                        "elev": elev, "g": g, "reach": reach, "obj": obj, "beh": b["beh"],
                        "warps": b["warps"]})
        for o in a.get("objects", []):          # (anything else the area wants: decorations)
            objects.append(dict(obj(o["gfx"], o["x"], o["y"], o.get("script", "0"), o.get("movement", "MOVEMENT_TYPE_FACE_DOWN")),
                                elevation=elev(o["x"], o["y"])))
        write_map(name, map_id(name), "LAYOUT_" + map_id(name)[4:], a["music"], "MAPSEC_KANTO_SAFARI_ZONE",
                  a["type"], objects, warps, bgs, a.get("show_name", True), coords=coords, weather=a["weather"])   # (its name pops up)
        (ROOT / "data/maps" / name / "scripts.inc").write_text("\n".join(scripts))
        # Where the roamers can turn up: land encounter spots the player can reach
        spots = sorted(p for p in reach if b["beh"](g[p[1]][p[0]]) in MB_ENCOUNTERS - MB_SURFABLE
                       and p not in {(x, y) for _, (x, y) in a["statics"] + a["items"]})
        random.Random(key).shuffle(spots)
        spots = sorted(spots[:ROAMER_SPOTS], key=lambda p: (p[1], p[0]))
        out.setdefault("all_maps", []).append(name)
        if a.get("land"):                    # (rooms without wild Pokémon: not an area for roamers)
            out["areas"].append((name, out["region_index"], [(x, y, elev(x, y)) for x, y in spots]))
        water = sum(1 for p in reach if b["beh"](g[p[1]][p[0]]) in MB_SURFABLE)
        land_n = sum(1 for p in reach if b["beh"](g[p[1]][p[0]]) in MB_ENCOUNTERS - MB_SURFABLE)
        out["report"].append(f"  {region:6} {key:8} {name:34} {w}x{h}  {land_n} wild-Pokémon tiles, {water} water")
    assert len(mod.ROAMERS) <= 0x20 - ROAMER_FLAG_BASE
    for r in mod.ROAMERS:
        out["roamers"].append((r, out["region_index"]))
    out["roamer_needs"].update(getattr(mod, "ROAMER_NEEDS", {}))
    if hasattr(mod, "header_lines"):
        out["header"] += mod.header_lines(map_id)
    for color in getattr(mod, "STAKE_COLORS", []):
        out["stake_colors"].append(color)

    # Wild Pokémon
    mons = lambda lst: [{"min_level": lo, "max_level": hi, "species": "SPECIES_" + s} for s, lo, hi in lst]
    for a in areas.values():
        if not a.get("land"):
            continue
        e = {"map": map_id(a["map"]), "base_label": "gExoticSafari_" + a["map"].split("_", 1)[1] + "_FireRed",
             "land_mons": {"encounter_rate": a.get("encounter_rate", ENCOUNTER_RATES[a["type"]]), "mons": mons(a["land"])}}
        if a.get("water"):
            assert len(a["water"]) == 5
            e["water_mons"] = {"encounter_rate": 2, "mons": mons(a["water"])}
        if a.get("fishing"):
            assert len(a["fishing"]) == 10
            e["fishing_mons"] = {"encounter_rate": 20, "mons": mons(a["fishing"])}
        out["wild"].append(e)
    return [a["map"] for a in areas.values()]


STATICS = []                         # (the static legendaries, in sExoticSafariStatics' order)


def build_zones(zone_warp):
    out = {"areas": [], "roamers": [], "statics": [], "finds": [], "stake_colors": [], "report": [], "wild": [],
           "roamer_needs": {}, "header": []}
    maps = []
    for i, mod in enumerate(REGION_MODULES):
        out["region_index"] = i
        maps += build_region(mod, zone_warp, out)
    STATICS[:] = [species for _, species, *_ in out["statics"]]

    # The header for src/exotic_safari.c: the areas (and their roamer spots), the roamers, the kiosk
    c = ["// The Exotic Pokémon Safari's areas (made by tools/gym_hack/build_exotic_safari.py)",
         "struct ExoticSafariArea { u8 mapGroup, mapNum; u8 region; u8 spotCount; const u8 (*spots)[3]; };",
         "struct ExoticSafariRoamer { u16 species, caughtFlag, hideFlag, needFlag; u8 region; };",
         "struct ExoticSafariFind { u16 item, flag; };", ""]
    for name, region, spots in out["areas"]:
        c.append(f"static const u8 sSpots_{name}[][3] = {{" + ", ".join(f"{{{x}, {y}, {e}}}" for x, y, e in spots) + "};")
    c += ["", "static const struct ExoticSafariArea sExoticSafariAreas[] =", "{"]
    for name, region, spots in out["areas"]:
        mid = map_id(name)
        c.append(f"    {{MAP_GROUP({mid}), MAP_NUM({mid}), {region}, ARRAY_COUNT(sSpots_{name}), sSpots_{name}}},")
    c += ["};", "", "// Every zone map's name (rooms too: ExoticSafari_GetAreaName)",
          "struct ExoticSafariName { u8 mapGroup, mapNum; const u8 *name; };",
          "static const struct ExoticSafariName sExoticSafariNames[] =", "{"]
    for name in out["all_maps"]:
        mid = map_id(name)
        c.append(f'    {{MAP_GROUP({mid}), MAP_NUM({mid}), COMPOUND_STRING("{safari_names.SAFARI_NAMES[name.split("_", 1)[1]].upper()}")}},')
    c += ["};", "", f"#define EXOTIC_SAFARI_ROAMER_COUNT {len(out['roamers'])}",
          "static const struct ExoticSafariRoamer sExoticSafariRoamers[EXOTIC_SAFARI_ROAMER_COUNT] =", "{"]
    slot = {}
    needs = out["roamer_needs"]
    for r, region in out["roamers"]:
        i = slot[region] = slot.get(region, -1) + 1
        need = (needs[r] if needs[r].startswith("FLAG_") else f"FLAG_EXOTIC_LEGEND_{needs[r]}") if r in needs else "0"
        c.append(f"    {{SPECIES_{r}, FLAG_EXOTIC_LEGEND_{r}, FLAG_TEMP_{ROAMER_FLAG_BASE + i:X}, {need}, {region}}},")
    c += ["};", f"#define EXOTIC_SAFARI_ROAMER_LOCALID(hideFlag) (1 + (hideFlag) - FLAG_TEMP_{ROAMER_FLAG_BASE:X})", "",
          "static const u16 sKioskSupplies[] = {" + ", ".join(f"ITEM_{i}" for i in KIOSK_SUPPLIES) + "};",
          "static const struct ExoticSafariFind sKioskFinds[] =", "{"]
    for item, flag in out["finds"]:
        c.append(f"    {{ITEM_{item}, {flag}}},")
    c += ["};", "", "// The legendaries that stand in one place, and what each waits for",
          "struct ExoticSafariStatic { u8 mapGroup, mapNum; u16 species, caughtFlag, hideFlag; u8 seal, sealArg, localId; };",
          "static const struct ExoticSafariStatic sExoticSafariStatics[] =", "{"]
    for mname, species, i, (seal, arg), local in out["statics"]:
        mid = map_id(mname)
        c.append(f"    {{MAP_GROUP({mid}), MAP_NUM({mid}), SPECIES_{species}, FLAG_EXOTIC_LEGEND_{species}, "
                 f"FLAG_TEMP_{0x11 + i:X}, SEAL_{seal}, {arg}, {local}}},")
    c += ["};", "", f"#define EXOTIC_SAFARI_STAKE_COLORS {len(out['stake_colors'])}",
          "#define EXOTIC_SAFARI_STAKES_PER_COLOR 4",
          f"#define EXOTIC_SAFARI_STAKE_FLAGS 0x{paldea.STAKE_FLAG_BASE:03X}",
          "static const u8 *const sStakeColorNames[] = {" + ", ".join(f'COMPOUND_STRING("{c}")' for c in out["stake_colors"]) + "};",
          "static const u16 sMaskFlags[] = {FLAG_EXOTIC_ITEM_WELLSPRING_MASK, FLAG_EXOTIC_ITEM_HEARTHFLAME_MASK, FLAG_EXOTIC_ITEM_CORNERSTONE_MASK};",
          ""] + out["header"]
    (ROOT / "src/data/exotic_safari_areas.h").write_text("\n".join(c) + "\n")

    # The roamers' and stakes' scripts (any area)
    rs = ["@ The roaming legendaries (made by tools/gym_hack/build_exotic_safari.py)"]
    for r, region in out["roamers"]:
        rs += [f"ExoticSafari_EventScript_Roamer{camel(r)}::", "\tlock", "\tfaceplayer", f"\tsetvar VAR_0x8004, SPECIES_{r}",
               "\tcall ExoticSafari_EventScript_LegendBattle",
               f"\tgoto_if_ne VAR_RESULT, B_OUTCOME_CAUGHT, ExoticSafari_EventScript_RoamerGone",
               f"\tsetflag FLAG_EXOTIC_LEGEND_{r}", "\tgoto ExoticSafari_EventScript_RoamerGone", ""]
    for c, color in enumerate(out["stake_colors"]):
        rs += [f"ExoticSafari_EventScript_Stake{color.title()}::", f"\tsetvar VAR_0x8004, {c}",
               "\tgoto ExoticSafari_EventScript_Stake", ""]
    (ROOT / "data/scripts/exotic_safari_roamers.inc").write_text("\n".join(rs))

    # Wild Pokémon
    wp = ROOT / "src/data/wild_encounters.json"
    wild = json.loads(wp.read_text())
    group = wild["wild_encounter_groups"][0]
    ids = {e["map"] for e in out["wild"]}
    group["encounters"] = [e for e in group["encounters"] if e.get("map") not in ids] + out["wild"]
    wp.write_text(json.dumps(wild, indent=2, ensure_ascii=False) + "\n")

    print("exotic safari areas:\n" + "\n".join(out["report"]))
    return maps


def unstick(g, w, h, beh, links, where=""):
    """Ledges only go one way: wherever one would leave the player somewhere they can't get back
    to every way out, open a gap in it (one tile of the ledge becomes the floor below it), as few
    ledges as it takes. Returns the opened cells."""
    jumps = {MB["MB_JUMP_EAST"]: (1, 0), MB["MB_JUMP_WEST"]: (-1, 0), MB["MB_JUMP_NORTH"]: (0, -1), MB["MB_JUMP_SOUTH"]: (0, 1)}
    exits = [set(cells) for _, cells, _, _ in links.values()]
    starts = set().union(*exits) if exits else set()

    def moves(x, y):
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if not (0 <= nx < w and 0 <= ny < h):
                continue
            j = jumps.get(beh(g[ny][nx]))
            if j is None:
                if not (g[ny][nx] >> 10) & 3 or (nx, ny) in starts:
                    yield nx, ny
            elif j == (dx, dy) and 0 <= nx + dx < w and 0 <= ny + dy < h and not (g[ny + dy][nx + dx] >> 10) & 3:
                yield nx + dx, ny + dy

    def forward(srcs):
        seen, todo = set(), list(srcs)
        while todo:
            c = todo.pop()
            if c in seen:
                continue
            seen.add(c)
            todo.extend(moves(*c))
        return seen

    def stuck():
        """cells the player can get to from a way in but not back to it (to backtrack)"""
        bad = set()
        for ex in exits:
            reach = forward(ex)
            back = {c: [] for c in reach}
            for c in reach:
                for n in moves(*c):
                    if n in back:
                        back[n].append(c)
            ok, todo = set(), list(ex)
            while todo:
                c = todo.pop()
                if c in ok:
                    continue
                ok.add(c)
                todo.extend(back.get(c, []))
            bad |= reach - ok
        return bad

    # the ledges, in runs (a line of the same jump direction)
    ledge = {(x, y): jumps[beh(g[y][x])] for y in range(h) for x in range(w) if beh(g[y][x]) in jumps}
    runs, done = [], set()
    for c, d in ledge.items():
        if c in done:
            continue
        step = (1, 0) if d[1] else (0, 1)                 # (a ledge runs across its jump)
        run, todo = [], [c]
        while todo:
            x, y = todo.pop()
            if (x, y) in done or ledge.get((x, y)) != d:
                continue
            done.add((x, y))
            run.append((x, y))
            todo += [(x + step[0], y + step[1]), (x - step[0], y - step[1])]
        runs.append(sorted(run))
    # every way out can be got to from every other (but ways back to the same area)
    targets = [t for _, _, _, t in links.values()]
    for a_i, a_ex in enumerate(exits):
        reach = forward(a_ex)
        for b_i, b_ex in enumerate(exits):
            if targets[a_i][0] != targets[b_i][0] and not reach & b_ex and os.environ.get("SAFARI_STUCK_DEBUG"):
                print(f"WARNING {where}: the way to {targets[b_i][0]} can't be reached from the way to {targets[a_i][0]}")
    opened = []
    bad = stuck()
    while bad:
        best = None
        for run in runs:
            for cell in (run[len(run) // 2], run[0], run[-1]):
                x, y = cell
                dx, dy = ledge[cell]
                if not (0 <= x + dx < w and 0 <= y + dy < h):
                    continue
                old = g[y][x]
                g[y][x] = g[y + dy][x + dx]                   # (the floor below the ledge)
                n = len(stuck())
                g[y][x] = old
                if best is None or n < best[0]:
                    best = (n, cell, run)
        if best is None or best[0] >= len(bad):
            print(f"WARNING {where}: {len(bad)} cells can't get back to every way out, e.g. {sorted(bad)[:4]}")
            if os.environ.get("SAFARI_STUCK_DEBUG"):
                reach = forward(starts)
                for lk, (_, cells, _, _) in links.items():
                    print("   ", lk, cells[:3], "reachable:", any(c in reach for c in cells))
            break
        n, (x, y), run = best
        dx, dy = ledge[(x, y)]
        g[y][x] = g[y + dy][x + dx]
        opened.append((x, y))
        runs.remove(run)
        bad = stuck()
    return opened


def reachable(b):
    """the cells the player can get to (walking or surfing) from where they arrive"""
    g, w, h = b["g"], b["w"], b["h"]
    start = [wp for wp in b["warps"]]
    starts = {(wp["x"], wp["y"]) for wp in start}
    beh = b.get("beh")
    jumps = {MB["MB_JUMP_EAST"]: (1, 0), MB["MB_JUMP_WEST"]: (-1, 0), MB["MB_JUMP_NORTH"]: (0, -1), MB["MB_JUMP_SOUTH"]: (0, 1)}
    def ok(x, y):
        return 0 <= x < w and 0 <= y < h and (not (g[y][x] >> 10) & 3 or (x, y) in starts)
    seen, todo = set(), list(starts)
    while todo:
        x, y = todo.pop()
        if (x, y) in seen or not ok(x, y):
            continue
        seen.add((x, y))
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if not (0 <= nx < w and 0 <= ny < h):
                continue
            j = jumps.get(beh(g[ny][nx])) if beh else None
            if j is None:
                todo.append((nx, ny))
            elif j == (dx, dy):                     # a ledge: only jumped down, landing past it
                todo.append((nx + dx, ny + dy))
    return seen


def main():
    m = build_tileset()
    register_tileset()

    # Fuchsia: the door (the Safari Office's door metatiles) and the sign; its warp is the last
    fl = json.loads((ROOT / "data/layouts/layouts.json").read_text())
    fuchsia = next(l for l in fl["layouts"] if l["id"] == "LAYOUT_FUCHSIA_CITY")
    path = ROOT / fuchsia["blockdata_filepath"]
    data = bytearray(path.read_bytes())
    fw = fuchsia["width"]
    def put(x, y, v):
        struct.pack_into("<H", data, (y * fw + x) * 2, v)
    put(FUCHSIA_DOOR[0], FUCHSIA_DOOR[1] - 1, blk(0x2D6, 1, 0))   # (the wall above: solid)
    put(FUCHSIA_DOOR[0], FUCHSIA_DOOR[1], blk(0x2BF, 1, 0))
    put(FUCHSIA_SIGN[0], FUCHSIA_SIGN[1], blk(0x002, 1, 0))
    path.write_bytes(data)
    fmap_path = ROOT / "data/maps/FuchsiaCity_Frlg/map.json"
    fmap = json.loads(fmap_path.read_text())
    fmap["warp_events"] = [w for w in fmap["warp_events"] if w["dest_map"] != "MAP_EXOTIC_SAFARI_HALL"]
    fmap["warp_events"].append({"x": FUCHSIA_DOOR[0], "y": FUCHSIA_DOOR[1], "elevation": 0,
                                "dest_map": "MAP_EXOTIC_SAFARI_HALL", "dest_warp_id": "0"})
    fuchsia_warp = len(fmap["warp_events"]) - 1
    fmap["bg_events"] = [b for b in fmap["bg_events"] if b.get("script") != "FuchsiaCity_EventScript_ExoticSafariSign"]
    fmap["bg_events"].append({"type": "sign", "x": FUCHSIA_SIGN[0], "y": FUCHSIA_SIGN[1], "elevation": 0,
                              "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
                              "script": "FuchsiaCity_EventScript_ExoticSafariSign"})
    fmap_path.write_text(json.dumps(fmap, indent=2, ensure_ascii=False) + "\n")

    # The hall: warps 0-2 the mat (to Fuchsia), then one per open zone's door
    hall = build_hall(m)
    write_layout("ExoticSafari_Hall", "LAYOUT_EXOTIC_SAFARI_HALL", hall, "gTileset_BuildingFrlg", "gTileset_ExoticSafari")
    warps = [{"x": x, "y": HALL_H - 2, "elevation": 3, "dest_map": "MAP_FUCHSIA_CITY", "dest_warp_id": str(fuchsia_warp)}
             for x in (MAT_X - 1, MAT_X, MAT_X + 1)]
    bgs, zone_warp = [], {}
    for i, name in enumerate(REGIONS):
        x = door_x(i)
        if name in ZONES:
            zone_warp[name] = (len(warps), warps)
            warps.append({"x": x, "y": DOOR_Y, "elevation": 3, "dest_map": ZONES[name]["id"], "dest_warp_id": "0"})
        bgs.append({"type": "sign", "x": x, "y": 3, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH",
                    "script": f"ExoticSafari_Hall_EventScript_Door{name.title()}"})
    write_map("ExoticSafari_Hall", "MAP_EXOTIC_SAFARI_HALL", "LAYOUT_EXOTIC_SAFARI_HALL", "MUS_RG_FUCHSIA",
              "MAPSEC_FUCHSIA_CITY", "MAP_TYPE_INDOOR",
              [obj("OBJ_EVENT_GFX_WORKER_M", MAT_X, COUNTER_Y, "ExoticSafari_Hall_EventScript_Attendant",
                   flag="FLAG_EXOTIC_SAFARI_PASS"),
               obj("OBJ_EVENT_GFX_WORKER_M", MAT_X - 1, COUNTER_Y - 1, "ExoticSafari_Hall_EventScript_AttendantDesk",
                   flag="FLAG_HIDE_EXOTIC_SAFARI_DESK"),
               obj("OBJ_EVENT_GFX_CLERK", KIOSK_X, COUNTER_Y - 1, "ExoticSafari_Hall_EventScript_Kiosk")],
              warps, bgs, False)

    # The zones
    zone_maps = build_zones(zone_warp)

    # Constants for the hall's scripts, and the doors for src/exotic_safari.c
    consts = ["// The Exotic Pokémon Safari (made by tools/gym_hack/build_exotic_safari.py)",
              "#ifndef GUARD_CONSTANTS_EXOTIC_SAFARI_H", "#define GUARD_CONSTANTS_EXOTIC_SAFARI_H",
              f"#define EXOTIC_SAFARI_PASS_PRICE      10000",
              f"#define LOCALID_EXOTIC_SAFARI_ATTENDANT 1",
              f"#define LOCALID_EXOTIC_SAFARI_DESK      2",
              f"#define EXOTIC_SAFARI_LEGEND_LEVEL      {LEGEND_LEVEL}",
              "// What a static legendary waits for (src/exotic_safari.c)",
              "#define SEAL_STAKES   0", "#define SEAL_MASKS    1", "#define SEAL_ROAMERS  2", "#define SEAL_MASTERY  3", "#define SEAL_NONE     4",
              "#define SEAL_ITEM     5     // (an item found: Zacian's Rusted Sword, Zamazenta's Rusted Shield)",
              "#define SEAL_LAMPS    6     // (Zarude: Glimwood's big mushrooms lit)",
              "#define SEAL_FRIENDSHIP 7   // (Enamorus: a lead POKéMON with the most friendship)",
              "#define SEAL_DOJO     8     // (Kubfu: the dojo master beaten)",
              "#define SEAL_STONES   9     // (Regieleki: the stepping stones lit in order)",
              "#define SEAL_DRAGON   10    // (Regidrago: the Dragon Fang found ...)",
              "#define SEAL_DRAGON_LEAD 11 // (... and a Dragon-type POKéMON in the lead)",
              "#define SEAL_GUARDIANS 12   // (Cosmog: its region's guardians - the roamers - caught)",
              "#define SEAL_FLUTE    13    // (Solgaleo, Lunala: their flute found)",
              "#define SEAL_LIGHT    14    // (Necrozma: Solgaleo and Lunala caught)",
              "#define SEAL_PARTS    15    // (Magearna: its machine parts found)",
              "#define SEAL_STEEL    16    // (Meltan: a Steel-type lead)",
              "#define SEAL_SHADOW   17    // (Marshadow: a Fighting- or Ghost-type lead)",
              "#define SEAL_STORM    18    // (Zeraora: a thunderstorm)",
              "#define SEAL_CELLS    19    // (Zygarde: enough of its Cells found)",
              "#define SEAL_BOTTLE   20    // (Hoopa: the Prison Bottle taken from its pedestal)",
              "#define SEAL_STEAM    21    // (Volcanion: a Fire- or Water-type lead, the other type behind it)",
              "#define SEAL_DRAIN    22    // (Yveltal: Kalos POKéMON caught)",
              "#define SEAL_CAUGHT   23    // (other legendaries caught first: Cobalion, Keldeo, Kyurem)",
              "#define SEAL_KEY      24    // (a keepsake found: Reshiram's Light Stone, Zekrom's Dark Stone, Victini's Liberty Pass)",
              "#define SEAL_LEVEL    25    // (Uxie: a lead of at least this level)",
              "#define SEAL_BERRY    26    // (Mesprit: a lead holding a berry)",
              "#define SEAL_HP_EVS   27    // (Azelf: a lead with this many HP EVs)",
              "#define SEAL_REGIS    28    // (Regigigas: Regirock, Regice and Registeel in the party)",
              "#define SEAL_AZURE_FLUTE 29 // (Arceus: the Azure Flute, from the Snowpoint elder)",
              "#define SEAL_METEORITE 30   // (Deoxys: the Meteorite found on Mt. Chimney)",
              "#define SEAL_BLUE_ORB 31    // (Kyogre: a lead holding the Blue Orb)",
              "#define SEAL_WISH     32    // (Jirachi: the zone's other legendaries caught)",
              "#define SEAL_RESULT_CAUGHT 0xFF", "#define SEAL_RESULT_AWAY 0xFE",
              "// Each static legendary's index (sExoticSafariStatics)"]
    consts += [f"#define EXOTIC_SAFARI_STATIC_{sp} {i}" for i, sp in enumerate(STATICS)]
    consts += ["#endif"]
    (ROOT / "include/constants/exotic_safari.h").write_text("\n".join(consts) + "\n")
    c = ["// The Exotic Pokémon Safari's doors (made by tools/gym_hack/build_exotic_safari.py)",
         f"#define EXOTIC_SAFARI_DOOR_COUNT {len(REGIONS)}",
         f"#define EXOTIC_SAFARI_DOOR_Y {DOOR_Y}",
         "static const u8 sDoorX[EXOTIC_SAFARI_DOOR_COUNT] = {" + ", ".join(str(door_x(i)) for i in range(len(REGIONS))) + "};",
         "static const bool8 sDoorHasZone[EXOTIC_SAFARI_DOOR_COUNT] = {" + ", ".join("TRUE" if n in ZONES else "FALSE" for n in REGIONS) + "};",
         "// top left, top middle, top right, door left, door middle, door right",
         "static const u16 sDoorOpen[6] = {" + ", ".join(f"0x{v:03X}" for v in (m["top_l"], m["top_m"], m["top_r"], 0x28B, 0x28C, 0x28D)) + "};",
         "static const u16 sDoorClosed[EXOTIC_SAFARI_DOOR_COUNT][6] =", "{"]
    for name in REGIONS:
        c.append("    {" + ", ".join(f"0x{m[f'closed_{name}_{k}']:03X}" for k in ("top_l", "top_m", "top_r", "door_l", "door_m", "door_r")) + f"}},  // {name}")
    c.append("};")
    (ROOT / "src/data/exotic_safari_doors.h").write_text("\n".join(c) + "\n")

    # Map group, script includes
    gp = ROOT / "data/maps/map_groups.json"
    groups = json.loads(gp.read_text())
    names = ["ExoticSafari_Hall"] + zone_maps
    if "gMapGroup_ExoticSafari" not in groups["group_order"]:
        groups["group_order"].append("gMapGroup_ExoticSafari")
    groups["gMapGroup_ExoticSafari"] = names
    gp.write_text(json.dumps(groups, indent=2) + "\n")
    (ROOT / "data/maps/exotic_safari.inc").write_text(
        "@ The Exotic Pokémon Safari's maps (tools/gym_hack/build_exotic_safari.py)\n"
        + '\t.include "data/scripts/exotic_safari.inc"\n\t.include "data/scripts/exotic_safari_roamers.inc"\n'
        + "".join(f'\t.include "data/maps/{n}/scripts.inc"\n' for n in names))

    # (the hall's map.json again: the Paldea door's warp now points at the meadow)
    hall_json = ROOT / "data/maps/ExoticSafari_Hall/map.json"
    hj = json.loads(hall_json.read_text())
    hj["warp_events"] = warps
    hall_json.write_text(json.dumps(hj, indent=2) + "\n")
    print(f"exotic safari: hall {HALL_W}x{HALL_H}, {len(zone_maps)} zone maps, Fuchsia warp {fuchsia_warp}")
    import subprocess                        # (everything reachable from the hall, no getting stuck)
    subprocess.run([sys.executable, str(ROOT / "tools/gym_hack/check_safari_reach.py")])


if __name__ == "__main__":
    main()
