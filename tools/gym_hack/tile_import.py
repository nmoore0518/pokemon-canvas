"""Bringing metatiles from one tileset into another (Pokémon Canvas, for the Exotic Safari's zones):
Emerald's beach umbrellas and deck chairs onto the Sevii Islands' sand, and drawn decorations (palm
trees). Used from a tileset copy's edit hook (build_exotic_safari.clone_tileset / convert_layout):
the copy's directory gets the new tiles, palettes and metatiles."""
import struct

from PIL import Image

import render_map
import build_exotic_safari as B


class Target:
    """A FireRed secondary tileset directory being edited: append tiles and metatiles to it"""

    def __init__(self, out):
        self.out = out
        self.img = Image.open(out / "tiles.png")
        self.mt = bytearray((out / "metatiles.bin").read_bytes())
        self.at = bytearray((out / "metatile_attributes.bin").read_bytes())
        self.used = max([(v & 0x3FF) - 640 for i in range(0, len(self.mt), 2)
                         for v in [struct.unpack_from("<H", self.mt, i)[0]] if v & 0x3FF >= 640] + [-1]) + 1

    def compact(self, keep):
        """Room for new tiles: the metatiles not in `keep` (ids from 0x280: ones no map of ours uses)
        blanked, and the tile sheet rebuilt with only the tiles the rest use"""
        n = len(self.mt) // 16
        ents = [list(struct.unpack_from("<8H", self.mt, i * 16)) for i in range(n)]
        for i in range(n):
            if 0x280 + i not in keep:
                ents[i] = [0] * 8
        used = sorted({e & 0x3FF for row in ents for e in row if e & 0x3FF >= 640})
        remap = {t: 640 + k for k, t in enumerate(used)}
        new = Image.new("P", (128, max(8, ((len(used) + 15) // 16) * 8)))
        new.putpalette(self.img.getpalette())
        for t, nt in remap.items():
            o, q = t - 640, nt - 640
            new.paste(self.img.crop(((o % 16) * 8, (o // 16) * 8, (o % 16) * 8 + 8, (o // 16) * 8 + 8)), ((q % 16) * 8, (q // 16) * 8))
        self.img = new
        self.mt = bytearray(b"".join(struct.pack("<8H", *[(remap[e & 0x3FF] | (e & ~0x3FF)) if e & 0x3FF >= 640 else e for e in row])
                                     for row in ents))
        self.used = len(used)
        self.cache = {}

    def add_tile(self, tile):
        """an 8x8 'P' image (palette indexes) -> its tile number (the same tile again: the same number)"""
        key = tile.tobytes()
        cache = self.__dict__.setdefault("cache", {})
        if key in cache:
            return cache[key]
        n = self.used
        cache[key] = 640 + n
        self.used += 1
        need = ((self.used + 15) // 16) * 8
        if need > self.img.height:
            new = Image.new("P", (128, need))
            new.putpalette(self.img.getpalette())
            new.paste(self.img, (0, 0))
            self.img = new
        self.img.paste(tile, ((n % 16) * 8, (n // 16) * 8))
        return 640 + n

    def add_metatile(self, entries, attr):
        mid = 0x280 + len(self.mt) // 16
        self.mt += struct.pack("<8H", *entries)
        self.at += struct.pack("<I", attr)
        return mid

    def entries(self, m, primary_dir):
        if m < 640:
            return struct.unpack_from("<8H", (primary_dir / "metatiles.bin").read_bytes(), m * 16)
        return struct.unpack_from("<8H", self.mt, (m - 640) * 16)

    def attr(self, m, primary_dir):
        if m < 640:
            return struct.unpack_from("<I", (primary_dir / "metatile_attributes.bin").read_bytes(), m * 4)[0]
        return struct.unpack_from("<I", self.at, (m - 640) * 4)[0]

    def save(self):
        assert self.used <= 384 and len(self.mt) // 16 <= 384, (self.out, self.used, len(self.mt) // 16)
        self.img.save(self.out / "tiles.png")
        (self.out / "metatiles.bin").write_bytes(self.mt)
        (self.out / "metatile_attributes.bin").write_bytes(self.at)


def import_metatiles(tgt, src_tilesets, version, ids, slots, ground=None, ground_like=None, match=None,
                     target_primary="gTileset_General_Frlg"):
    """Copies metatiles `ids` of another tileset pair (version 'emerald' or 'frlg') into `tgt`: their
    palettes into the free palette slots `slots` (in order), their tiles appended. ground: the
    target's ground under them (4 entries, the bottom layer) in place of the source's own ground
    (bottom-layer tiles equal to the source metatile `ground_like`'s). match: {source palette: target
    palette} - those tiles redrawn in the target's palette, nearest colors (a shadow on sand drawn
    in the target's own sand colors). Returns the new ids."""
    match = match or {}
    emerald = version == "emerald"
    nprim, npal = (512, 6) if emerald else (640, 7)
    pd, sd = (render_map.tileset_dir(s) for s in src_tilesets)
    def mts(d):
        b = (d / "metatiles.bin").read_bytes()
        return [struct.unpack_from("<8H", b, i) for i in range(0, len(b), 16)]
    def ats(d):
        b = (d / "metatile_attributes.bin").read_bytes()
        fmt = "H" if emerald else "I"
        return struct.unpack(f"<{len(b) // struct.calcsize(fmt)}{fmt}", b)
    pm, sm, pa, sa = mts(pd), mts(sd), ats(pd), ats(sd)
    pimg, simg = Image.open(pd / "tiles.png"), Image.open(sd / "tiles.png")
    get = lambda m: pm[m] if m < nprim else sm[m - nprim]
    ground_src = set(get(ground_like)[:4]) if ground_like is not None else set()
    pal_of = {}
    tile_of = {}
    new = []
    for m in ids:
        out = []
        for k, e in enumerate(get(m)):
            if ground and k < 4 and (e in ground_src or ground_like is None):
                out.append(ground[k])
                continue
            t, flips, p = e & 0x3FF, e & 0x0C00, e >> 12
            if t == 0:                                  # (nothing drawn: the blank tile)
                out.append(0)
                continue
            if p in match:
                key = (t, p)
                if key not in tile_of:
                    src_pal = render_map.load_pal((pd if p < npal else sd) / "palettes" / f"{p:02d}.pal")
                    tp = match[p]
                    tdir = render_map.tileset_dir(target_primary) if tp < 7 else tgt.out
                    dst_pal = render_map.load_pal(tdir / "palettes" / f"{tp:02d}.pal")
                    near = [0] + [min(range(1, 16), key=lambda j: sum((x - y) ** 2 for x, y in zip(src_pal[c][:3], dst_pal[j][:3])))
                                  for c in range(1, 16)]
                    img, ti = (pimg, t) if t < nprim else (simg, t - nprim)
                    tile = img.crop(((ti % 16) * 8, (ti // 16) * 8, (ti % 16) * 8 + 8, (ti // 16) * 8 + 8))
                    tile_of[key] = tgt.add_tile(tile.point(lambda c: near[c] if c < 16 else c))
                out.append(tile_of[key] | flips | (match[p] << 12))
                continue
            if p not in pal_of:
                slot = slots[len(pal_of)]
                pal_of[p] = slot
                src = (pd if p < npal else sd) / "palettes" / f"{p:02d}.pal"
                B.write_pal(tgt.out / "palettes" / f"{slot:02d}.pal", render_map.load_pal(src))
            if t not in tile_of:
                img, ti = (pimg, t) if t < nprim else (simg, t - nprim)
                tile_of[t] = tgt.add_tile(img.crop(((ti % 16) * 8, (ti // 16) * 8, (ti % 16) * 8 + 8, (ti // 16) * 8 + 8)))
            out.append(tile_of[t] | flips | (pal_of[p] << 12))
        a = pa[m] if m < nprim else sa[m - nprim]
        attr = (a & 0xFF) | (((a >> 12) & 0xF) << 29) if emerald else a
        new.append(tgt.add_metatile(out, attr))
    return new


# ---------------------------------------------------------------------------------------------
# A palm tree, drawn: 2 metatiles wide, 3 tall (fronds over the player, the trunk's foot solid)
# ---------------------------------------------------------------------------------------------
PALM_PAL = [
    (0, 0, 0), (32, 48, 24), (40, 104, 48), (72, 160, 64), (128, 208, 88), (200, 240, 136),  # 1-5 fronds
    (64, 40, 24), (120, 80, 40), (176, 128, 72), (216, 176, 112),                             # 6-9 trunk
    (96, 72, 40), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0),                       # 10 coconuts
]


def palm_art():
    """32x48 palette-index pixels"""
    import math
    W, H = 32, 48
    px = [[0] * W for _ in range(H)]
    def put(x, y, c):
        if 0 <= x < W and 0 <= y < H:
            px[y][x] = c
    # trunk: a gentle curve from the foot (x ~ 13, y 46) up to the crown (x ~ 17, y 13)
    for y in range(13, 47):
        t = (46 - y) / 33
        cx = 13 + 4 * t * t
        for dx in range(-2, 3):
            shade = 7 if dx < 0 else 8 if dx == 0 else 9
            if (y // 3) % 2 == 0 and dx == -2:
                shade = 6
            put(int(round(cx + dx)), y, shade)
        put(int(round(cx - 3)), y, 1 if y > 15 else 0)
        put(int(round(cx + 3)), y, 6)
    # fronds: arcs out from the crown
    crown = (17, 12)
    for ang, length, droop in ((-170, 15, 0.9), (-140, 14, 0.6), (-105, 10, 0.2), (-75, 10, 0.2), (-40, 14, 0.6),
                               (-10, 15, 0.9), (160, 9, 0.5), (20, 9, 0.5)):
        a = math.radians(ang)
        for i in range(length):
            f = i / length
            x = crown[0] + math.cos(a) * i
            y = crown[1] + math.sin(a) * i + droop * i * f * 1.2
            w = max(1, int(3 * (1 - f)) + 1)
            for k in range(-w, w + 1):
                nx, ny = x - math.sin(a) * k * 0.6, y + math.cos(a) * k * 0.6 + abs(k) * 0.5
                c = 4 if k <= 0 else 3
                if abs(k) == w:
                    c = 2
                put(int(round(nx)), int(round(ny)), c)
            put(int(round(x)), int(round(y)), 5 if f < 0.6 else 4)
    for y in range(H):                               # (outline the fronds)
        for x in range(W):
            if px[y][x] in (2, 3, 4, 5) and any(not (0 <= x + a < W and 0 <= y + b < H) or px[y + b][x + a] == 0
                                                for a, b in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                px[y][x] = 1
    for cx, cy in ((15, 14), (19, 14), (17, 16)):    # coconuts
        for dx in (0, 1):
            for dy in (0, 1):
                put(cx + dx, cy + dy, 10)
    return px


def add_palm(tgt, slot, ground, ground_attr):
    """a palm tree's 6 metatiles (top left, top right, middle..., foot), on `ground` (its 4 bottom-layer
    entries); palette `slot`. Returns their ids: rows top to bottom, left then right"""
    B.write_pal(tgt.out / "palettes" / f"{slot:02d}.pal", PALM_PAL)
    art = palm_art()
    ids = []
    for k in range(6):
        ox, oy = (k % 2) * 16, (k // 2) * 16
        top = []
        for q in range(4):
            tile = Image.new("P", (8, 8))
            tile.putdata([art[oy + (q // 2) * 8 + y][ox + (q % 2) * 8 + x] for y in range(8) for x in range(8)])
            top.append(tgt.add_tile(tile) | (slot << 12))
        # the crown over the player (layer type normal: its top layer above), the foot below them
        attr = (ground_attr & ~0x1FF & ~0x60000000) | (0 if k < 4 else 1 << 29)
        ids.append(tgt.add_metatile(list(ground) + top, attr))
    return ids


# ---------------------------------------------------------------------------------------------
# Lumiose City's Prism Tower, drawn: a lattice tower, its lights pulsing (colors 4-12 of its
# palette: tileset_anims.c's TilesetAnim_Lumiose)
# ---------------------------------------------------------------------------------------------
PRISM_PAL = [
    (0, 0, 0), (28, 30, 46), (70, 78, 104), (112, 122, 150),             # 1 outline, 2-3 steel (shade)
    (56, 96, 200), (96, 160, 248), (172, 214, 255), (255, 214, 96),      # 4-6 blue lights, 7 gold (these pulse)
    (255, 244, 190), (40, 64, 128), (132, 186, 255), (0, 0, 0),          # 8 pale gold, 9 glass dark, 10 glass
    (255, 255, 255), (164, 172, 198), (218, 224, 238), (238, 232, 214),  # 12 white; 13-14 steel lit; 15 wall
]


def prism_tower_art(W=112, H=208):
    """The Prism Tower seen corner-on from above, like the games' buildings: two lattice faces (the
    left lit, the right in shade) meeting at the near edge, girders bent round it, the observation
    deck a lit band of glass round the tower; it stands on a base building (roof and front wall,
    the bottom three rows)"""
    px = [[0] * W for _ in range(H)]
    cx = W // 2

    def put(x, y, c):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < W and 0 <= y < H:
            px[y][x] = c

    def line(x0, y0, x1, y1, c):
        n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
        for i in range(n + 1):
            t = i / n
            put(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, c)

    # -- the base building: a flat roof (its far edge, its top lit, a lip in front), a front wall
    #    with glass and an entrance under the tower's legs
    BY = 150                          # (the roof's far edge)
    FY = 178                          # (where the front wall starts)
    for y in range(BY, H):
        for x in range(2, W - 2):
            if y < FY - 4:
                c = 14 if (y - BY) < 3 else 13
                if x in (2, W - 3) or y == BY:
                    c = 1
            elif y < FY:
                c = 3 if y < FY - 1 else 1
                if x in (2, W - 3):
                    c = 1
            else:
                c = 15
                if x in (2, W - 3) or y == H - 1:
                    c = 1
                elif x in (3, W - 4) or y >= H - 3:
                    c = 13
            px[y][x] = c
    for wx in list(range(8, cx - 16, 10)) + list(range(cx + 18, W - 10, 10)):   # windows
        for y in range(FY + 5, FY + 19):
            for x in range(wx, wx + 6):
                edge = x in (wx, wx + 5) or y in (FY + 5, FY + 18)
                px[y][x] = 1 if edge else (6 if y < FY + 9 else 10 if x < wx + 3 else 9)
    for x in range(cx - 12, cx + 13):                                         # the entrance, arched
        t = (x - cx) / 12
        top = FY + 3 + int(6 * t * t)
        for y in range(top, H - 1):
            px[y][x] = 1 if abs(x - cx) == 12 or y == top else (9 if y < H - 8 else 2)
    for x in range(cx - 8, cx + 9, 4):
        put(x, FY + 2, 7)                                                     # lights over the door

    # -- the tower: half-width hw(y) of its (corner-on) silhouette
    def hw(y):
        if y < 34:
            return 1.5
        if y < 64:
            return 3 + (y - 34) * (5 / 30)
        if y < 82:
            return 8 + (y - 64) * (2 / 18)
        if y < 98:
            return 13                 # (the deck)
        if y < 118:
            return 10 + (y - 98) * (6 / 20)
        return 16 + (y - 118) * (22 / 46)

    def bend(y, x):                   # (girders bend round the near edge: lower in front)
        return y + 3 * (1 - abs(x - cx) / max(hw(y), 1))

    TOP = 34
    END = 168                         # (the legs reach the roof)
    for y in range(TOP, END):
        h = hw(y)
        for x in range(int(cx - h), int(cx + h) + 1):
            left = x < cx
            k = (x + y) % 2
            px[y][x] = (13 if k else 3) if left else (3 if k and (x + y) % 4 == 1 else 2)
        put(cx - h, y, 1); put(cx + h, y, 1); put(cx, y, 14)
        put(cx - h + 1, y, 14); put(cx + h - 1, y, 2)
    # girders and lattice crosses, panel by panel
    ys = list(range(TOP + 4, 82, 8)) + list(range(100, END, 10))
    for a, b in zip(ys, ys[1:]):
        if a < 98 <= b:
            continue
        for side in (-1, 1):
            ha, hb = hw(a), hw(b)
            lit = 14 if side < 0 else 13
            line(cx + side * (ha - 1), a, cx, bend(a, cx), 1)            # the girder
            line(cx + side * (ha - 1), a + 1, cx, bend(a, cx) + 1, lit)
            line(cx + side * (ha - 1), a + 1, cx + side, bend(b, cx) - 1, lit)   # the cross
            line(cx + side, bend(a, cx) + 1, cx + side * (hb - 1), b - 1, lit)
    # the arches between the legs, one in each face
    for side in (-1, 1):
        mid = cx + side * 18
        for x in range(mid - 11, mid + 12):
            t = (x - mid) / 11
            top = 136 + int(14 * t * t)
            for y in range(top + 1, END):
                if abs(x - cx) < hw(y) - 3 and abs(x - cx) > 3:
                    px[y][x] = 0 if y < BY else (13 if y < FY - 4 else px[y][x])
            put(x, top, 1)
            put(x, top + 1, 14 if side < 0 else 3)
    # its shadow on the roof (the light from the left)
    for y in range(BY + 1, FY - 4):
        h = int(hw(min(y, END - 1)))
        for x in range(cx + h + 1, min(W - 3, cx + h + 10 - (y - BY) // 4)):
            if px[y][x] in (13, 14):
                px[y][x] = 3
    # the observation deck: a lit band of glass, its roof a light rim, its floor dark
    for y in range(82, 98):
        h = hw(y)
        for x in range(int(cx - h), int(cx + h) + 1):
            left = x < cx
            if y <= 84:
                c = 14 if left else 13
            elif y >= 95:
                c = 2
            else:
                c = (6 if (x // 3) % 2 else 5) if left else (5 if (x // 3) % 2 else 4)
            px[y][x] = c
        put(cx - h, y, 1); put(cx + h, y, 1)
        if 85 <= y <= 94:
            put(cx, y, 12)
    line(cx - 13, 82, cx + 13, 82, 1); line(cx - 13, 97, cx, 99, 1); line(cx, 99, cx + 13, 97, 1)
    # spire and its light
    for y in range(6, TOP):
        put(cx - 1, y, 14); put(cx, y, 13); put(cx + 1, y, 2)
    for y in range(0, 6):
        for x in (-1, 0, 1):
            put(cx + x, y, 12 if y < 2 else 7)
    # lights up the edges (gold) and round the deck's rim
    for y in range(40, END - 4, 8):
        h = hw(y)
        put(cx - h, y, 7); put(cx + h, y, 7); put(cx, y, 8)
    for x in range(cx - 12, cx + 13, 4):
        put(x, 83, 8)
    return px


def add_prism_tower(tgt, slot, ground, ground_attr, wide=7, tall=13):
    """the tower's metatiles (wide x tall, rows top to bottom), over `ground`, palette `slot`; all
    solid (set the collision on the map). Returns their ids"""
    B.write_pal(tgt.out / "palettes" / f"{slot:02d}.pal", PRISM_PAL)
    art = prism_tower_art(wide * 16, tall * 16)
    ids = []
    for k in range(wide * tall):
        ox, oy = (k % wide) * 16, (k // wide) * 16
        top = []
        for q in range(4):
            tile = Image.new("P", (8, 8))
            tile.putdata([art[oy + (q // 2) * 8 + y][ox + (q % 2) * 8 + x] for y in range(8) for x in range(8)])
            top.append(tgt.add_tile(tile) | (slot << 12) if any(tile.getdata()) else 0)
        # (drawn over the player: the tower stands above anyone walking behind its foot)
        ids.append(tgt.add_metatile(list(ground) + top, ground_attr & ~0x1FF & ~0x60000000))
    return ids
