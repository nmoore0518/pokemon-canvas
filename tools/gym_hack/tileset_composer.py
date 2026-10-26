"""Builds a FireRed secondary tileset from pieces (Pokémon Canvas): metatiles copied from other
FireRed secondary tilesets (over the same primary), and new art cut into metatiles.

    c = Composer("gTileset_General_Frlg")
    m = c.copy("gTileset_SaffronCity", 0x2E9)                  # a Saffron metatile, renumbered
    slot = c.add_palette(colors)                               # 16 (r, g, b), a palette slot 7-12
    grid = c.art(img, slot, ground=0x008, behind_rows=1)       # a P image (indices 0-15) -> metatiles
    c.write("LeaderAcademy", "leader_academy_frlg")            # files + the C declarations

Tiles are shared when equal (also flipped). The primary's metatiles are used as they are: their
numbers (below 0x280) stay valid in a map using the new tileset.
"""
import struct
from pathlib import Path

from PIL import Image

import render_map as rm
from build_exotic_safari import MB, register_tileset, write_pal

ROOT = Path(__file__).resolve().parents[2]
NPRIM = 640
LAYER_NORMAL, LAYER_COVERED, LAYER_SPLIT = 0, 1, 2


def _read_mt(d):
    b = (d / "metatiles.bin").read_bytes()
    return [list(struct.unpack_from("<8H", b, i)) for i in range(0, len(b), 16)]


def _read_attr(d):
    b = (d / "metatile_attributes.bin").read_bytes()
    return [struct.unpack_from("<I", b, i)[0] for i in range(0, len(b), 4)]


def _tile_px(img, idx):
    w = img.width // 8
    x, y = (idx % w) * 8, (idx // w) * 8
    return tuple(img.crop((x, y, x + 8, y + 8)).getdata())


def _flip(px, h, v):
    rows = [px[i * 8:(i + 1) * 8] for i in range(8)]
    if h:
        rows = [r[::-1] for r in rows]
    if v:
        rows = rows[::-1]
    return tuple(c for r in rows for c in r)


class Composer:
    def __init__(self, primary):
        rm.set_version("frlg")
        self.pdir = rm.tileset_dir(primary)
        self.pmt, self.pattr = _read_mt(self.pdir), _read_attr(self.pdir)
        self.tiles = [tuple([0] * 64)]          # (tile 0: blank)
        self.index = {self.tiles[0]: (0, 0)}
        self.palettes = []                      # (slot 7 + i)
        self.pal_map = {}                       # (source dir, its palette) -> slot
        self.metatiles, self.attrs = [], []
        self.copied = {}
        self.sources = {}

    # ---- pieces ----
    def add_tile(self, px):
        """An 8x8 tile (64 indices) -> (tile number, flip bits): an equal or flipped one is reused"""
        px = tuple(px)
        for h in (0, 1):
            for v in (0, 1):
                f = _flip(px, h, v)
                if f in self.index:              # (stored = px flipped: px = stored, flipped back)
                    t, flips = self.index[f]
                    return t, flips ^ ((h << 10) | (v << 11))
        self.tiles.append(px)
        self.index[px] = (len(self.tiles) - 1, 0)
        return len(self.tiles) - 1, 0

    def add_palette(self, colors):
        self.palettes.append([tuple(c[:3]) for c in colors] + [(0, 0, 0)] * (16 - len(colors)))
        assert len(self.palettes) <= 6, "a secondary tileset has 6 palettes"
        return 7 + len(self.palettes) - 1

    def add_metatile(self, entries, attr):
        self.metatiles.append(list(entries))
        self.attrs.append(attr)
        return 0x280 + len(self.metatiles) - 1

    def ground(self, mid):
        """The bottom layer's 4 entries of a primary metatile"""
        return list(self.pmt[mid][:4])

    def _source(self, symbol):
        if symbol not in self.sources:
            d = rm.tileset_dir(symbol)
            self.sources[symbol] = (d, _read_mt(d), _read_attr(d), Image.open(d / "tiles.png"))
        return self.sources[symbol]

    def copy(self, symbol, mid):
        """A metatile of another secondary tileset (a primary one is returned as it is)"""
        if mid < 0x280:
            return mid
        key = (symbol, mid)
        if key in self.copied:
            return self.copied[key]
        d, mt, attr, img = self._source(symbol)
        out = []
        for e in mt[mid - 0x280]:
            t, flips, pal = e & 0x3FF, e & 0xC00, e >> 12
            if t >= NPRIM:
                nt, nf = self.add_tile(_tile_px(img, t - NPRIM))
                t = NPRIM + nt
                flips ^= nf
            if pal >= 7:
                pk = (str(d), pal)
                if pk not in self.pal_map:
                    self.pal_map[pk] = self.add_palette(rm.load_pal(d / "palettes" / f"{pal:02d}.pal"))
                pal = self.pal_map[pk]
            out.append(t | flips | (pal << 12))
        self.copied[key] = self.add_metatile(out, attr[mid - 0x280])
        return self.copied[key]

    def art(self, img, slot, ground=None, layer=LAYER_COVERED, behind_rows=0, behavior=None, under=None):
        """A P image (0 transparent, 1-15 the palette `slot`) cut into 16x16 metatiles: a grid of
        metatile numbers. The art is the top layer over `ground` (a primary metatile's bottom layer, or
        4 entries; blank if none). Rows from the top `behind_rows` are drawn over people (they walk
        behind them); the rest under. behavior: {(bx, by): MB name}. under: {(bx, by): ground} to
        change the ground of some blocks"""
        grid = []
        for by in range(img.height // 16):
            row = []
            for bx in range(img.width // 16):
                top = []
                for ty in range(2):
                    for tx in range(2):
                        x, y = bx * 16 + tx * 8, by * 16 + ty * 8
                        px = tuple(img.crop((x, y, x + 8, y + 8)).getdata())
                        if any(px):
                            t, f = self.add_tile(px)
                            top.append((NPRIM + t) | f | (slot << 12))
                        else:
                            top.append(0)
                g = (under or {}).get((bx, by), ground)
                base = self.ground(g) if isinstance(g, int) else (list(g) if g else [0] * 4)
                lay = LAYER_NORMAL if by < behind_rows else layer
                b = (behavior or {}).get((bx, by), "MB_NORMAL")
                beh = MB[b] if isinstance(b, str) else b
                row.append(self.add_metatile(base + top, beh | (lay << 29)))
            grid.append(row)
        return grid

    # ---- out ----
    def write(self, name, dirname):
        assert len(self.tiles) <= 384, f"{name}: {len(self.tiles)} tiles (384 at most)"
        assert len(self.metatiles) <= 384, f"{name}: {len(self.metatiles)} metatiles"
        out = ROOT / "data/tilesets/secondary" / dirname
        (out / "palettes").mkdir(parents=True, exist_ok=True)
        n = len(self.tiles)
        img = Image.new("P", (128, ((n + 15) // 16) * 8))
        img.putpalette([v for i in range(256) for v in ((i * 16) % 256,) * 3])
        for i, px in enumerate(self.tiles):
            t = Image.new("P", (8, 8))
            t.putdata(px)
            img.paste(t, ((i % 16) * 8, (i // 16) * 8))
        img.save(out / "tiles.png")
        for i in range(16):
            cols = self.palettes[i - 7] if 7 <= i < 7 + len(self.palettes) else [(0, 0, 0)] * 16
            write_pal(out / "palettes" / f"{i:02d}.pal", cols)
        (out / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *e) for e in self.metatiles))
        (out / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in self.attrs))
        register_tileset(name, dirname)
        print(f"{name}: {n} tiles, {len(self.metatiles)} metatiles, {len(self.palettes)} palettes")
