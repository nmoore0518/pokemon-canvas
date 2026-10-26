#!/usr/bin/env python3
"""Find tile seams in the hack's own maps that no original map has: two metatiles side by side (or one
above the other) that never touch that way in any FireRed, Emerald or HnS map. A sand block cut off
against a rock's ring in the water, bushes hanging over the sea, a tree top with no trunk under it...

Metatiles are compared by their drawing (the tiles' pixels, palettes ignored), so copies, converted
Emerald tilesets and recolored ones still match their originals. Only pairs whose two metatiles both
appear in original maps are judged (the hack's own art has nothing to compare with).

Usage: python3 tools/gym_hack/check_seams.py [out_dir]     (out_dir: a marked-up PNG per map)
"""
import hashlib

import numpy as np
import json
import struct
import sys
from collections import defaultdict
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import render_map as RM  # noqa: E402

ROOT = RM.ROOT
LAYOUTS = json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"]
# HnS's maps too (the Johto zone and the Paldea mountain are built from them): its own checkout
HNS = Path("/home/claude/ext/pokehns-expansion")
SLACK = 6          # (pixels: how much worse than the best original fit a seam may be)
GENERATED = ("LAYOUT_EXOTIC_SAFARI_", "LAYOUT_LEADER_ACADEMY_", "LAYOUT_PALLET_TOWN")
SKIP = ("LAYOUT_PALLET_TOWN_PLAYERS_HOUSE", "LAYOUT_PALLET_TOWN_RIVALS_HOUSE", "LAYOUT_PALLET_TOWN_PROFESSOR_OAKS_LAB")


def generated(lid):
    return lid.startswith(GENERATED) and not lid.startswith(SKIP)


_ts = {}


def tileset_dir(sym, root):
    if root == ROOT:
        return RM.tileset_dir(sym)
    import re
    headers = (root / "src/data/tilesets/headers.h").read_text()
    start = headers.index(f"const struct Tileset {sym} =")
    block = headers[start:headers.index("};", start)]
    tiles_sym = block.split(".tiles = ")[1].split(",")[0].strip()
    graphics = (root / "src/data/tilesets/graphics.h").read_text()
    i = graphics.index(tiles_sym + "[]")
    return root / Path(graphics[i:].split('"')[1]).parent


def tileset(sym, version, root=ROOT):
    key = (sym, version, root)
    if key not in _ts:
        d = tileset_dir(sym, root)
        img = Image.open(d / "tiles.png")
        if img.mode != "P":
            img = img.convert("P")
        mt = (d / "metatiles.bin").read_bytes()
        _ts[key] = (img, img.width // 8, [struct.unpack_from("<8H", mt, i) for i in range(0, len(mt), 16)])
    return _ts[key]


_tile_cache = {}


def tile_bytes(ts, idx, hf, vf):
    img, wt, _ = ts
    k = (id(img), idx, hf, vf)
    if k not in _tile_cache:
        if idx >= wt * (img.height // 8):
            _tile_cache[k] = b"\0" * 64
        else:
            t = img.crop(((idx % wt) * 8, (idx // wt) * 8, (idx % wt) * 8 + 8, (idx // wt) * 8 + 8))
            if hf:
                t = t.transpose(Image.FLIP_LEFT_RIGHT)
            if vf:
                t = t.transpose(Image.FLIP_TOP_BOTTOM)
            _tile_cache[k] = t.tobytes()
    return _tile_cache[k]


_rmts = {}


def rm_tileset(sym, root):
    if (sym, root) not in _rmts:
        _rmts[(sym, root)] = RM.Tileset(tileset_dir(sym, root))
    return _rmts[(sym, root)]


EDGES = {}       # drawing hash -> its four edges' colors (L, R, T, B: 16 each, as a numpy array)


def signatures(lay, root=ROOT):
    """each metatile id used by a layout -> its drawing's hash (and its edges, in EDGES)"""
    version = lay.get("layout_version")
    RM.set_version(version)
    prim, sec = tileset(lay["primary_tileset"], version, root), tileset(lay["secondary_tileset"], version, root)
    npm, npt = RM.NUM_PRIMARY_METATILES, RM.NUM_PRIMARY_TILES
    out = {}

    def edges(mid, s):
        RM.set_version(version)
        img = Image.new("RGB", (16, 16))
        RM.draw_metatile(img, 0, 0, mid, rm_tileset(lay["primary_tileset"], root), rm_tileset(lay["secondary_tileset"], root))
        a = np.asarray(img, dtype=np.int16)
        EDGES[s] = np.stack([a[:, 0], a[:, 15], a[0, :], a[15, :]])

    def sig(mid):
        if mid in out:
            return out[mid]
        ts, local = (prim, mid) if mid < npm else (sec, mid - npm)
        if local >= len(ts[2]):
            out[mid] = None
            return None
        h = hashlib.md5()
        for e in ts[2][local]:
            tidx, hf, vf = e & 0x3FF, (e >> 10) & 1, (e >> 11) & 1
            tsrc, tl = (prim, tidx) if tidx < npt else (sec, tidx - npt)
            h.update(tile_bytes(tsrc, tl, hf, vf))
        out[mid] = h.hexdigest()[:16]
        if out[mid] not in EDGES:
            edges(mid, out[mid])
        return out[mid]
    return sig


def mismatch(e1, e2):
    """how many of the 16 pixels across a seam clash"""
    return int((np.abs(e1 - e2).sum(axis=1) > 90).sum())


def grid(lay, root=ROOT):
    w, h = lay["width"], lay["height"]
    p = root / lay["blockdata_filepath"]
    if not p.exists():
        return None
    data = p.read_bytes()
    if len(data) < w * h * 2:
        return None
    b = struct.unpack(f"<{w*h}H", data[:w * h * 2])
    return [[b[y * w + x] & 0x3FF for x in range(w)] for y in range(h)]


class Reference:
    """what the original maps say: the metatile drawings they use, the pairs they put side by side or
    one above the other, and how well each drawing's edges meet their best original neighbor"""
    def __init__(self, horiz, vert, known, best):
        self.horiz, self.vert, self.known, self.best = horiz, vert, known, best

    def clash(self, a, b, horizontal):
        """a seam no original map has, and that clashes more than either side's best fit (None: can't tell)"""
        if a not in self.known or b not in self.known:
            return None
        if (a, b) in (self.horiz if horizontal else self.vert):
            return False
        m = mismatch(EDGES[a][1], EDGES[b][0]) if horizontal else mismatch(EDGES[a][3], EDGES[b][2])
        ka, kb = ((a, "R"), (b, "L")) if horizontal else ((a, "D"), (b, "U"))
        return m > max(self.best.get(ka, 99), self.best.get(kb, 99)) + SLACK


CACHE = ROOT / "build/seam_reference.pickle"


def reference():
    """(cached in build/: the original maps don't change)"""
    import pickle
    if CACHE.exists():
        horiz, vert, known, best, edges = pickle.loads(CACHE.read_bytes())
        EDGES.update(edges)
        return Reference(horiz, vert, known, best)
    horiz, vert, known = set(), set(), set()
    refs = [(l, ROOT) for l in LAYOUTS]
    if (HNS / "data/layouts/layouts.json").exists():
        refs += [(l, HNS) for l in json.loads((HNS / "data/layouts/layouts.json").read_text())["layouts"]]
    for lay, root in refs:
        if "blockdata_filepath" not in lay or (root == ROOT and generated(lay["id"])):
            continue
        try:
            g = grid(lay, root)
            if g is None:
                continue
            sig = signatures(lay, root)
        except Exception:
            continue
        for y, row in enumerate(g):
            for x, m in enumerate(row):
                s = sig(m)
                known.add(s)
                if x + 1 < len(row):
                    horiz.add((s, sig(row[x + 1])))
                if y + 1 < len(g):
                    vert.add((s, sig(g[y + 1][x])))
    best = {}
    for a, b in horiz:
        if a in EDGES and b in EDGES:
            m = mismatch(EDGES[a][1], EDGES[b][0])
            best[(a, "R")] = min(best.get((a, "R"), 99), m)
            best[(b, "L")] = min(best.get((b, "L"), 99), m)
    for a, b in vert:
        if a in EDGES and b in EDGES:
            m = mismatch(EDGES[a][3], EDGES[b][2])
            best[(a, "D")] = min(best.get((a, "D"), 99), m)
            best[(b, "U")] = min(best.get((b, "U"), 99), m)
    known.discard(None)
    CACHE.parent.mkdir(exist_ok=True)
    CACHE.write_bytes(pickle.dumps((horiz, vert, known, best, {k: v for k, v in EDGES.items() if k in known})))
    return Reference(horiz, vert, known, best)


def bad_seams(lay, ref, g=None):
    """{(x, y): [sides]} where a layout's seams clash"""
    g = g or grid(lay)
    sig = signatures(lay)
    bad = defaultdict(list)
    for y, row in enumerate(g):
        for x, m in enumerate(row):
            a = sig(m)
            if x + 1 < len(row) and ref.clash(a, sig(row[x + 1]), True):
                bad[(x, y)].append("R"); bad[(x + 1, y)].append("L")
            if y + 1 < len(g) and ref.clash(a, sig(g[y + 1][x]), False):
                bad[(x, y)].append("D"); bad[(x, y + 1)].append("U")
    return bad


def main(out_dir=None):
    ref = reference()
    gen = [l for l in LAYOUTS if "blockdata_filepath" in l and generated(l["id"])]
    horiz, vert, known = ref.horiz, ref.vert, ref.known
    print(f"{len(known)} metatile drawings, {len(horiz)} side-by-side and {len(vert)} stacked pairs in original maps")
    report = []
    for lay in gen:
        g = grid(lay)
        if g is None:
            continue
        bad = bad_seams(lay, ref, g)
        report.append((lay, bad))
        if out_dir and bad:
            name = lay["name"].replace("_Layout", "")
            maps = [p.parent.name for p in (ROOT / "data/maps").glob("*/map.json")
                    if json.loads(p.read_text())["layout"] == lay["id"]]
            if maps:
                RM.render(maps[0], f"{out_dir}/{name}.png", scale=2)
                img = Image.open(f"{out_dir}/{name}.png")
                d = ImageDraw.Draw(img)
                for (x, y), sides in bad.items():
                    for s in sides:
                        x0, y0 = x * 32, y * 32
                        seg = {"L": (x0, y0, x0, y0 + 31), "R": (x0 + 31, y0, x0 + 31, y0 + 31),
                               "U": (x0, y0, x0 + 31, y0), "D": (x0, y0 + 31, x0 + 31, y0 + 31)}[s]
                        d.line(seg, fill=(255, 0, 0), width=3)
                img.save(f"{out_dir}/{name}.png")
    total = 0
    for lay, bad in sorted(report, key=lambda r: -len(r[1])):
        if bad:
            total += len(bad)
            print(f"  {lay['name']:45} {len(bad):4} tiles at a seam no original map has")
    print(f"{total} tiles in {sum(1 for _, b in report if b)} of {len(report)} maps")
    return report


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else None)
