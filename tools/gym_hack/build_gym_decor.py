#!/usr/bin/env python3
"""Gym decorations: the catalog, and the gym's own tileset that draws them.

Usage: python3 tools/gym_hack/build_gym_decor.py [catalog.png]

Writes:
  data/tilesets/secondary/pallet_gym/   the Pallet Town Gym's tileset: the Viridian Gym tileset
                                        plus one block for every decoration piece, drawn over the
                                        gym floor (furniture comes from the indoor tileset)
  include/constants/gym_decor.h         GYM_DECOR_* ids
  src/data/gym_decor.h                  the catalog (names, prices, unlocks, blocks)
and, with an argument, a picture of every decoration with its name and price.

To add a decoration, add an entry to DECORATIONS below and run build_pallet_gym.py (which runs
this script first). Keep the order of existing entries:
saves store decorations by their position in this list, so new ones go at the end.
"""
import json
import shutil
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
import render_map as rm  # noqa: E402

SRC = ROOT / "data/tilesets/secondary/viridian_gym_frlg"
DST = ROOT / "data/tilesets/secondary/pallet_gym"
PRIM = ROOT / "data/tilesets/primary/building_frlg"

LAYER_NORMAL, LAYER_COVERED = 0, 1
MB_NORMAL = 0

# ---------------------------------------------------------------------------
# The catalog
#
# Each decoration is a grid of cells, top row first. A cell is:
#   "f:XX"   indoor furniture block XX (BuildingFrlg tileset), set on the gym floor
#   "f:XX~"  the same, but people can walk there (the top of a plant: you walk behind it)
#   "g:XX"   a block from the gym tileset as is (Viridian Gym: walls, statues, spinners)
#   "g:XX~"  the same, walkable
#   "rug:XX" an indoor block drawn as is, walkable (rugs)
#   "x:SET:XX"        block XX from another gym's tileset (SET, e.g. CeruleanGym), copied in
#                     (tiles and colors); its floor is swapped for this gym's floor
#   "x:SET:XX:water"  the same, as water you can SURF on
#   "wall"   an invisible wall: looks like floor, can't be walked through (shown while editing)
# price:  in Poke Dollars.   unlock: a flag (usually a badge) needed before it's sold, or None.
# keep:   keep the block's behavior (bookshelf, TV, spinner...), otherwise it's plain.
# ---------------------------------------------------------------------------
BADGE = {i: f"FLAG_BADGE0{i}_GET" for i in range(1, 9)}

DECORATIONS = [
    # --- Available from the start -----------------------------------------
    dict(key="POTTED_PLANT", name="POTTED PLANT", price=1000, unlock=None,
         desc="A leafy plant in a white pot.", cells=[["f:57~"], ["f:5F"]]),
    dict(key="TABLE", name="TABLE", price=2000, unlock=None,
         desc="A table with a checkered cloth.", cells=[["f:4C", "f:4D"], ["f:54", "f:55"]]),
    dict(key="CHAIR_LEFT", name="CHAIR (FACES RIGHT)", price=600, unlock=None,
         desc="A blue chair.", cells=[["f:4B~"]]),
    dict(key="CHAIR_RIGHT", name="CHAIR (FACES LEFT)", price=600, unlock=None,
         desc="A blue chair.", cells=[["f:4E~"]]),
    dict(key="BOOKSHELF", name="BOOKSHELF", price=2500, unlock=None, keep=True,
         desc="Packed with POKéMON books.", cells=[["f:33", "f:34"], ["f:3B", "f:3C"]]),
    dict(key="CABINET", name="CABINET", price=2500, unlock=None, keep=True,
         desc="A tall cabinet.", cells=[["f:36", "f:37"], ["f:3E", "f:3F"]]),
    dict(key="TV", name="TV", price=3000, unlock=None, keep=True,
         desc="A TV on a stand.", cells=[["f:35"], ["f:3D"]]),
    dict(key="GREEN_RUG", name="GREEN RUG", price=1500, unlock=None,
         desc="A soft green rug. 3x3.",
         cells=[["rug:43", "rug:44", "rug:46"], ["rug:53", "rug:45", "rug:56"], ["rug:5B", "rug:5C", "rug:5E"]]),
    dict(key="GYM_WALL", name="GYM WALL", price=300, unlock=None,
         desc="A section of GYM wall.", cells=[["g:29B"], ["g:2A3"]]),
    dict(key="GYM_STATUE", name="GYM STATUE", price=5000, unlock=None,
         desc="Every GYM needs one.", cells=[["g:2A1~"], ["g:2A9"]]),
    # --- Unlocked by beating Gym Leaders --------------------------------------
    dict(key="POOL_WATER", name="POOL WATER", price=800, unlock=BADGE[2], keep=True,
         desc="MISTY's pool water. SURF on it!", cells=[["x:CeruleanGym:291:water"]]),
    dict(key="TRASH_CAN", name="TRASH CAN", price=1200, unlock=BADGE[3],
         desc="Just like LT. SURGE's.", cells=[["x:VermilionGym:2B8"]]),
    dict(key="FERN", name="FERN", price=1800, unlock=BADGE[4],
         desc="A lush fern from ERIKA's GYM.", cells=[["f:102"]]),
    dict(key="INVISIBLE_WALL", name="INVISIBLE WALL", price=500, unlock=BADGE[5],
         desc="KOGA's trick. Can't be seen.", cells=[["wall"]]),
    dict(key="SPINNER_UP", name="SPIN TILE (UP)", price=1500, unlock=BADGE[8], keep=True,
         desc="Spins TRAINERS upward.", cells=[["g:28C~"]]),
    dict(key="SPINNER_DOWN", name="SPIN TILE (DOWN)", price=1500, unlock=BADGE[8], keep=True,
         desc="Spins TRAINERS downward.", cells=[["g:28B~"]]),
    dict(key="SPINNER_LEFT", name="SPIN TILE (LEFT)", price=1500, unlock=BADGE[8], keep=True,
         desc="Spins TRAINERS left.", cells=[["g:289~"]]),
    dict(key="SPINNER_RIGHT", name="SPIN TILE (RIGHT)", price=1500, unlock=BADGE[8], keep=True,
         desc="Spins TRAINERS right.", cells=[["g:28A~"]]),
    dict(key="SPINNER_STOP", name="STOP TILE", price=1000, unlock=BADGE[8], keep=True,
         desc="Stops a spinning TRAINER.", cells=[["g:28D~"]]),
    # --- Free, and always in storage: where gym challenges are fought (src/gym_challenge.c) ---
    # The leader stands on the top mark (column 1, row 1), challengers on the bottom one (row 3).
    dict(key="BATTLE_STAGE", name="BATTLE STAGE", price=0, unlock=None,
         desc="Challengers battle you here.",
         cells=[[f"art:STAGE:{x}:{y}" for x in range(3)] for y in range(5)]),
]

# Fixed pieces of the gym itself (not for sale)
FIXED = {
    "PC_TOP": "f:63",
    "PC_BOTTOM": "f:62",
}


# ---------------------------------------------------------------------------
def read_metatiles(path):
    data = path.read_bytes()
    return [list(struct.unpack_from("<8H", data, i)) for i in range(0, len(data), 16)]


def read_attrs(path):
    data = path.read_bytes()
    return [struct.unpack_from("<I", data, i)[0] for i in range(0, len(data), 4)]


PRIM_MT = read_metatiles(PRIM / "metatiles.bin")
PRIM_ATTR = read_attrs(PRIM / "metatile_attributes.bin")
GYM_MT = read_metatiles(SRC / "metatiles.bin")
GYM_ATTR = read_attrs(SRC / "metatile_attributes.bin")
FLOOR = 0x281
FLOOR_TILES = GYM_MT[FLOOR - 0x280][:4]


def layer_of(attr):
    return (attr >> 29) & 3


def with_layer(attr, layer):
    return (attr & ~0x60000000) | (layer << 29)


class Tileset:
    def __init__(self, tiles_img):
        self.mt = [list(m) for m in GYM_MT]
        self.attr = list(GYM_ATTR)
        self.cache = {}
        self.tiles_img = tiles_img          # [PIL image, number of tiles used]
        self.pals = [rm.load_pal(p) for p in sorted((DST / "palettes").glob("*.pal"))]
        self.free_pals = [9, 10, 11]        # slots the Viridian Gym tileset leaves empty
        self.pal_map = {}
        self.tile_map = {}
        self.sources = {}

    def source(self, name):
        if name not in self.sources:
            d = rm.tileset_dir(f"gTileset_{name}")
            self.sources[name] = (rm.Tileset(d), read_attrs(d / "metatile_attributes.bin"))
        return self.sources[name]

    def import_entry(self, name, e):
        src = self.source(name)[0]
        t, flips, p = e & 0x3FF, e & 0xC00, e >> 12
        if t >= 0x280:
            key = (name, t)
            if key not in self.tile_map:
                px = list(src.tile(t - 0x280).getdata())
                self.tile_map[key] = append_tile(self.tiles_img, px)
            t = self.tile_map[key]
        if p >= 7:
            key = (name, p)
            if key not in self.pal_map:
                colors = src.pals[p]
                same = [i for i in range(7, 13) if [tuple(c[:3]) for c in self.pals[i]] == [tuple(c[:3]) for c in colors]]
                if same:
                    self.pal_map[key] = same[0]
                else:
                    if not self.free_pals:
                        raise SystemExit(f"out of palette slots importing {name} palette {p}")
                    slot = self.free_pals.pop(0)
                    self.pals[slot] = colors
                    self.pal_map[key] = slot
            p = self.pal_map[key]
        return t | flips | (p << 12)

    def write_palettes(self):
        for i, colors in enumerate(self.pals):
            text = "JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{c[0]} {c[1]} {c[2]}\r\n" for c in colors)
            (DST / f"palettes/{i:02}.pal").write_text(text, newline="")

    def add(self, entries, attr):
        key = (tuple(entries), attr)
        if key not in self.cache:
            self.mt.append(list(entries))
            self.attr.append(attr)
            self.cache[key] = 0x280 + len(self.mt) - 1
        return self.cache[key]

    def cell(self, spec, keep):
        """Returns (metatile id, collides, edit-mode metatile id or None)."""
        walkable = spec.endswith("~")
        spec = spec.rstrip("~")
        if spec == "wall":
            marker = self.add(FLOOR_TILES + MARKER_TILES, with_layer(MB_NORMAL, LAYER_NORMAL))
            return FLOOR, True, marker
        if spec.startswith("art:"):
            _, name, cx, cy = spec.split(":")
            return self.add(art_tiles(self.tiles_img, name, int(cx), int(cy)) + GYM_MT[FLOOR - 0x280][4:],
                            with_layer(MB_NORMAL, LAYER_NORMAL)), False, None
        if spec.startswith("x:"):
            parts = spec.split(":")
            name, mid = parts[1], int(parts[2], 16)
            water = len(parts) > 3 and parts[3] == "water"
            src, attrs = self.source(name)
            ents = src.metatiles[mid - 0x280]
            floor = src.metatiles[0x281 - 0x280][:4]
            imported = [self.import_entry(name, e) for e in ents]
            if list(ents[:4]) == list(floor):
                imported = FLOOR_TILES + imported[4:]
            attr = attrs[mid - 0x280] if keep else attrs[mid - 0x280] & ~0x1FF
            return self.add(imported, attr), not (walkable or water), "water" if water else None
        kind, num = spec.split(":")
        mid = int(num, 16)
        if kind == "g":
            attr = GYM_ATTR[mid - 0x280]
            if not keep:
                attr &= ~0x1FF
            if attr == GYM_ATTR[mid - 0x280]:
                return mid, not walkable, None
            return self.add(GYM_MT[mid - 0x280], attr), not walkable, None
        src, attr = PRIM_MT[mid], PRIM_ATTR[mid]
        if not keep:
            attr &= ~0x1FF
        if kind == "rug":
            return self.add(src, attr), False, None
        # furniture: the gym floor underneath, the furniture's own top layer above
        return self.add(FLOOR_TILES + src[4:], attr), not walkable, None


# ---------------------------------------------------------------------------
# Pixel art drawn in the gym floor's colors (palette 12)
ART_PAL = 12
WHITE, LIGHT, DARK, CLAY, CLAY_LIGHT = 5, 4, 1, 6, 7


def stage_art():
    """The battle stage: a clay court with white lines, a center circle and two start marks."""
    w, h = 48, 80
    px = [[CLAY_LIGHT] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            if x in (0, w - 1) or y in (0, h - 1):
                px[y][x] = DARK
            elif x in (2, 3, w - 4, w - 3) or y in (2, 3, h - 4, h - 3):
                px[y][x] = WHITE
            elif y in (h // 2 - 1, h // 2):
                px[y][x] = WHITE
            elif (x + y) % 6 == 0:
                px[y][x] = CLAY
    def ring(cx, cy, r0, r1, color):
        for y in range(h):
            for x in range(w):
                d = (x - cx + 0.5) ** 2 + (y - cy + 0.5) ** 2
                if r0 * r0 <= d <= r1 * r1:
                    px[y][x] = color
    ring(24, 40, 5.5, 7, WHITE)
    for cy in (24, 56):                  # the two start marks, on rows 1 and 3
        ring(24, cy, 0, 5, LIGHT)
        ring(24, cy, 5, 6.5, WHITE)
    return px


ART = {"STAGE": stage_art}
_ART_CACHE = {}


def art_tiles(tiles_img, name, cx, cy):
    """The 4 tile entries for block (cx, cy) of a piece of art, adding its tiles once."""
    if name not in _ART_CACHE:
        _ART_CACHE[name] = (ART[name](), {})
    px, done = _ART_CACHE[name]
    entries = []
    for ty in range(2):
        for tx in range(2):
            key = (cx * 2 + tx, cy * 2 + ty)
            if key not in done:
                x0, y0 = key[0] * 8, key[1] * 8
                tile = [px[y0 + y][x0 + x] for y in range(8) for x in range(8)]
                done[key] = append_tile(tiles_img, tile) | (ART_PAL << 12)
            entries.append(done[key])
    return entries


# A dotted outline, shown on invisible walls while decorating (gym palette 12, index 7: dark)
MARKER_TILES = []


def make_marker_tiles(tiles_img):
    """Adds the marker's 8x8 tiles to the tileset image; returns the 4 tile entries."""
    dark = 7
    tl = [[0] * 8 for _ in range(8)]
    for i in range(0, 8, 2):
        tl[1][i] = dark
        tl[i][1] = dark
    import copy
    tr = [row[::-1] for row in tl]
    bl = tl[::-1]
    br = [row[::-1] for row in bl]
    entries = []
    for t in (tl, tr, bl, br):
        entries.append(append_tile(tiles_img, [c for row in t for c in row]) | (12 << 12))
    return entries


def append_tile(tiles_img, pixels):
    from PIL import Image
    n_tiles = (tiles_img[0].height // 8) * 16
    used = tiles_img[1]
    idx = used
    if idx >= n_tiles:
        img = Image.new("P", (128, tiles_img[0].height + 8), 0)
        img.putpalette(tiles_img[0].getpalette())
        img.paste(tiles_img[0], (0, 0))
        tiles_img[0] = img
    t = Image.new("P", (8, 8))
    t.putdata(pixels)
    tiles_img[0].paste(t, ((idx % 16) * 8, (idx // 16) * 8))
    tiles_img[1] += 1
    return 0x280 + idx


def count_used_tiles(img):
    """Number of tiles up to the last non-empty one."""
    rows = img.height // 8
    last = 0
    for i in range(rows * 16):
        x, y = (i % 16) * 8, (i // 16) * 8
        if any(img.crop((x, y, x + 8, y + 8)).getdata()):
            last = i + 1
    return last


def build():
    from PIL import Image
    if DST.exists():
        shutil.rmtree(DST)
    shutil.copytree(SRC, DST)
    tiles = Image.open(SRC / "tiles.png")
    tiles_img = [tiles, count_used_tiles(tiles)]
    MARKER_TILES[:] = make_marker_tiles(tiles_img)
    _ART_CACHE.clear()

    ts = Tileset(tiles_img)
    catalog = []
    for d in DECORATIONS:
        rows = []
        for row in d["cells"]:
            rows.append([ts.cell(c, d.get("keep", False)) for c in row])
        catalog.append((d, rows))
    fixed = {k: ts.cell(v, False)[0] for k, v in FIXED.items()}
    tiles_img[0].save(DST / "tiles.png")
    ts.write_palettes()
    if tiles_img[1] > 384:
        raise SystemExit(f"the gym tileset has {tiles_img[1]} tiles; the limit is 384")
    if len(ts.mt) > 384:
        raise SystemExit(f"the gym tileset has {len(ts.mt)} blocks; the limit is 384")
    (DST / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *m) for m in ts.mt))
    (DST / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in ts.attr))
    write_c(catalog)
    return ts, catalog, fixed


def write_c(catalog):
    ids = "\n".join(f"    GYM_DECOR_{d['key']}," for d, _ in catalog)
    (ROOT / "include/constants/gym_decor.h").write_text(f"""// Generated by tools/gym_hack/build_gym_decor.py. Don't edit by hand.
#ifndef GUARD_CONSTANTS_GYM_DECOR_H
#define GUARD_CONSTANTS_GYM_DECOR_H

// Saves store decorations by these numbers: only ever add new ones at the end.
enum GymDecorId
{{
{ids}
    GYM_DECOR_COUNT
}};

#endif // GUARD_CONSTANTS_GYM_DECOR_H
""")
    parts = ["// Generated by tools/gym_hack/build_gym_decor.py from its DECORATIONS list. Don't edit by hand.\n"]
    for d, rows in catalog:
        def flags(c, e):
            return (" | GYM_DECOR_SOLID" if c else "") + (" | GYM_DECOR_WATER" if e == "water" else "")
        blocks = ", ".join(f"0x{m:03X}{flags(c, e)}" for row in rows for m, c, e in row)
        parts.append(f"static const u16 sGymDecorBlocks_{d['key']}[] = {{ {blocks} }};\n")
        if any(isinstance(e, int) for row in rows for _, _, e in row):
            edits = ", ".join(f"0x{(e if isinstance(e, int) else m):03X}" for row in rows for m, _, e in row)
            parts.append(f"static const u16 sGymDecorEditBlocks_{d['key']}[] = {{ {edits} }};\n")
    parts.append("\nconst struct GymDecor gGymDecor[GYM_DECOR_COUNT] =\n{\n")
    for d, rows in catalog:
        has_edit = any(isinstance(e, int) for row in rows for _, _, e in row)
        unlock = d["unlock"] or "0"
        parts.append(f"""    [GYM_DECOR_{d['key']}] =
    {{
        .name = COMPOUND_STRING("{d['name']}"),
        .description = COMPOUND_STRING("{d['desc']}"),
        .price = {d['price']},
        .unlockFlag = {unlock},
        .width = {len(rows[0])},
        .height = {len(rows)},
        .blocks = sGymDecorBlocks_{d['key']},
        .editBlocks = {f"sGymDecorEditBlocks_{d['key']}" if has_edit else "NULL"},
    }},
""")
    parts.append("};\n")
    (ROOT / "src/data/gym_decor.h").write_text("".join(parts))


def preview(catalog, out):
    from PIL import Image, ImageDraw
    prim = rm.Tileset(rm.tileset_dir("gTileset_BuildingFrlg"))
    sec = rm.Tileset(DST)
    cell_w, cell_h = 120, 144
    cols = 6
    img = Image.new("RGB", (cols * cell_w, ((len(catalog) + cols - 1) // cols) * cell_h), (40, 40, 48))
    d = ImageDraw.Draw(img)
    for k, (dec, rows) in enumerate(catalog):
        ox, oy = (k % cols) * cell_w, (k // cols) * cell_h
        w, h = len(rows[0]), len(rows)
        tile = Image.new("RGB", ((w + 2) * 16, (h + 2) * 16))
        for y in range(h + 2):
            for x in range(w + 2):
                rm.draw_metatile(tile, x * 16, y * 16, FLOOR, prim, sec)
        for y, row in enumerate(rows):
            for x, (m, c, e) in enumerate(row):
                rm.draw_metatile(tile, (x + 1) * 16, (y + 1) * 16, e if isinstance(e, int) else m, prim, sec)
        img.paste(tile, (ox + 4, oy + 4))
        d.text((ox + 4, oy + cell_h - 26), dec["name"][:19], fill=(255, 255, 255))
        lock = "" if not dec["unlock"] else f"  badge {dec['unlock'][11]}"
        d.text((ox + 4, oy + cell_h - 14), f"${dec['price']}{lock}", fill=(255, 220, 120))
    img = img.resize((img.width * 2, img.height * 2), Image.NEAREST)
    img.save(out)


if __name__ == "__main__":
    ts, catalog, fixed = build()
    print(f"{len(catalog)} decorations, {len(ts.mt)} of 384 gym blocks used")
    print("fixed:", {k: hex(v) for k, v in fixed.items()})
    if len(sys.argv) > 1:
        preview(catalog, sys.argv[1])
