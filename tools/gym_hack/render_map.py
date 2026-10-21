#!/usr/bin/env python3
"""Render a FireRed-mode map layout to a PNG (with object markers) for quick previews.

Usage: python3 tools/gym_hack/render_map.py <MapFolderName> <out.png> [--metatiles <tileset> <out.png>]
Example: python3 tools/gym_hack/render_map.py PalletTown_ProfessorOaksLab_Frlg lab.png
"""
import json
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
NUM_PRIMARY_TILES = 640
NUM_PRIMARY_METATILES = 640
NUM_PRIMARY_PALS = 7


def set_version(version):
    """Emerald's primary tilesets are smaller than FireRed's (HnS uses FireRed's sizes)"""
    global NUM_PRIMARY_TILES, NUM_PRIMARY_METATILES, NUM_PRIMARY_PALS
    if version == "emerald":
        NUM_PRIMARY_TILES, NUM_PRIMARY_METATILES, NUM_PRIMARY_PALS = 512, 512, 6
    else:
        NUM_PRIMARY_TILES, NUM_PRIMARY_METATILES, NUM_PRIMARY_PALS = 640, 640, 7


def tileset_dir(symbol):
    """gTileset_Lab_Frlg -> data/tilesets/secondary/lab_frlg (by reading headers/graphics)."""
    headers = (ROOT / "src/data/tilesets/headers.h").read_text()
    start = headers.index(f"const struct Tileset {symbol} =")
    block = headers[start:headers.index("};", start)]
    tiles_sym = block.split(".tiles = ")[1].split(",")[0].strip()
    graphics = (ROOT / "src/data/tilesets/graphics.h").read_text()
    if tiles_sym + "[]" not in graphics:     # (Emerald's general tileset: defined elsewhere)
        import re
        snake = re.sub(r"(?<=[a-z])(?=[A-Z])", "_", symbol.replace("gTileset_", "")).lower()
        for kind in ("primary", "secondary"):
            if (ROOT / "data/tilesets" / kind / snake).is_dir():
                return ROOT / "data/tilesets" / kind / snake
    i = graphics.index(tiles_sym + "[]")
    path = graphics[i:].split('"')[1]
    return ROOT / Path(path).parent


def load_pal(path):
    lines = Path(path).read_text().split("\n")[3:19]
    return [tuple(int(x) for x in l.split()) for l in lines if l.strip()]


class Tileset:
    def __init__(self, d):
        self.dir = d
        img = Image.open(d / "tiles.png")
        self.tiles = img
        self.width_tiles = img.width // 8
        mt = (d / "metatiles.bin").read_bytes()
        self.metatiles = [struct.unpack_from("<8H", mt, i) for i in range(0, len(mt), 16)]
        self.pals = [load_pal(p) for p in sorted((d / "palettes").glob("*.pal"))]

    def tile(self, idx):
        x, y = (idx % self.width_tiles) * 8, (idx // self.width_tiles) * 8
        return self.tiles.crop((x, y, x + 8, y + 8))


def draw_metatile(canvas, px, py, mid, prim, sec):
    ts, local = (prim, mid) if mid < NUM_PRIMARY_METATILES else (sec, mid - NUM_PRIMARY_METATILES)
    if local >= len(ts.metatiles):
        return
    entries = ts.metatiles[local]
    for layer in range(2):
        for k in range(4):
            e = entries[layer * 4 + k]
            tidx, hf, vf, pal = e & 0x3FF, (e >> 10) & 1, (e >> 11) & 1, e >> 12
            tsrc, tl = (prim, tidx) if tidx < NUM_PRIMARY_TILES else (sec, tidx - NUM_PRIMARY_TILES)
            if tl >= tsrc.width_tiles * (tsrc.tiles.height // 8):
                continue
            t = tsrc.tile(tl)
            if hf:
                t = t.transpose(Image.FLIP_LEFT_RIGHT)
            if vf:
                t = t.transpose(Image.FLIP_TOP_BOTTOM)
            palette = (prim.pals if pal < NUM_PRIMARY_PALS else sec.pals)[pal]
            ox, oy = px + (k % 2) * 8, py + (k // 2) * 8
            for y in range(8):
                for x in range(8):
                    c = t.getpixel((x, y))
                    if c == 0 and layer == 1:
                        continue
                    canvas.putpixel((ox + x, oy + y), palette[c][:3] if c or layer == 0 else (0, 0, 0))


def render(map_name, out, scale=3):
    m = json.loads((ROOT / f"data/maps/{map_name}/map.json").read_text())
    layouts = json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"]
    lay = next(l for l in layouts if l.get("id") == m["layout"])
    set_version(lay.get("layout_version"))
    prim = Tileset(tileset_dir(lay["primary_tileset"]))
    sec = Tileset(tileset_dir(lay["secondary_tileset"]))
    w, h = lay["width"], lay["height"]
    blocks = struct.unpack(f"<{w*h}H", (ROOT / lay["blockdata_filepath"]).read_bytes())
    img = Image.new("RGB", (w * 16, h * 16))
    for i, b in enumerate(blocks):
        draw_metatile(img, (i % w) * 16, (i // w) * 16, b & 0x3FF, prim, sec)
    img = img.resize((w * 16 * scale, h * 16 * scale), Image.NEAREST)
    d = ImageDraw.Draw(img)
    s = 16 * scale
    for o in m.get("object_events", []):
        x, y = o["x"] * s, o["y"] * s
        d.rectangle([x + 2, y + 2, x + s - 3, y + s - 3], outline=(255, 0, 255), width=2)
        label = o.get("local_id", o["graphics_id"]).replace("LOCALID_", "").replace("OBJ_EVENT_GFX_", "")[:10]
        d.text((x + 4, y + 4), label, fill=(255, 0, 255))
    for x in range(0, img.width, s):
        d.text((x + 2, 0), str(x // s), fill=(255, 255, 0))
    for y in range(0, img.height, s):
        d.text((0, y + 2), str(y // s), fill=(255, 255, 0))
    img.save(out)


def render_metatiles(symbol_primary, symbol_secondary, out, scale=2, cols=16):
    prim = Tileset(tileset_dir(symbol_primary))
    sec = Tileset(tileset_dir(symbol_secondary))
    n = len(sec.metatiles)
    rows = (n + cols - 1) // cols
    img = Image.new("RGB", (cols * 16, rows * 16))
    for i in range(n):
        draw_metatile(img, (i % cols) * 16, (i // cols) * 16, NUM_PRIMARY_METATILES + i, prim, sec)
    img = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
    d = ImageDraw.Draw(img)
    for i in range(n):
        d.text(((i % cols) * 16 * scale + 1, (i // cols) * 16 * scale + 1), hex(NUM_PRIMARY_METATILES + i)[2:], fill=(255, 0, 255))
    img.save(out)


if __name__ == "__main__":
    if sys.argv[1] == "--metatiles":
        render_metatiles(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        render(sys.argv[1], sys.argv[2])
