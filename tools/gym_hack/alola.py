"""The Exotic Pokémon Safari's Alola zone (Pokémon Canvas), for build_exotic_safari.py.

Six areas from Nick's planner (exotic-safari-planner-alola_update.xlsx): islands linked by sea
(surfing off one area's edge comes out on the next one's water), a Hawaiian look throughout:

    Hall door -> RESORT (Hano Beach: the Sevii Islands' Resort Gorgeous, its mansion the way back
    to the hall; the hub) -> west the OCEAN (Seafolk Village: Pacifidlog's houses on rafts, out at
    sea) -> west the RUINS (Ruins of Conflict: Six Island's Ruin Valley, jungle)
                          -> east the BEACH (Big Wave Beach: One Island's Treasure Beach)
                          -> north the SNOWY MOUNTAIN (Mount Lanakila: Seven Island's Sevault Canyon,
                             snow above its snowline, green below, the sea at its foot)
                          -> south the VOLCANO (Wela Volcano Park: Mt. Ember, Kindle Road's beach
                             and sea at its foot)
"""
import random
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paldea import read_layout, blk, land, ROOT   # noqa: E402

REGION = "ALOLA"
HUB = "RESORT"
LV = (48, 52)


def tileset_dir(sym):
    import render_map
    return render_map.tileset_dir(sym)


# ---------------------------------------------------------------------------------------------
# Beach things: Emerald's umbrellas and deck chairs (Route 109's), and palm trees (drawn:
# tile_import.py), added to copies of the Sevii Islands' tilesets and set about on the sand/grass
# ---------------------------------------------------------------------------------------------
SLATEPORT = ("gTileset_General", "gTileset_Slateport")
UMBRELLAS = [[0x2E4, 0x2E5, 0x2EC, 0x2ED, 0x2F4, 0x2F5],      # red, blue, green (2x3)
             [0x2E2, 0x2E3, 0x2EA, 0x2EB, 0x2F2, 0x2F3],
             [0x2E6, 0x2E7, 0x2EE, 0x2EF, 0x2F6, 0x2F7]]
CHAIR = [0x2E1, 0x2E9, 0x2F1]                                    # (1x3)
SAND, GRASS = 0x115, 0x008                                       # (FireRed's general tileset)


def _plain(like):
    """FireRed's general metatiles drawn in just `like`'s colors (plain sand, plain grass, ripples or
    tufts in any arrangement: no water's or rock's edge), nothing on top"""
    from PIL import Image
    d = tileset_dir("gTileset_General_Frlg")
    prim = (d / "metatiles.bin").read_bytes()
    img = Image.open(d / "tiles.png")
    ent = lambda m: struct.unpack_from("<8H", prim, m * 16)
    def colors(e):
        t = e & 0x3FF
        return set(img.crop(((t % 16) * 8, (t // 16) * 8, (t % 16) * 8 + 8, (t // 16) * 8 + 8)).getdata())
    pal = ent(like)[0] >> 12
    ok = set().union(*(colors(e) for e in ent(like)[:4]))
    return {m for m in range(len(prim) // 16) if all(e >> 12 == pal and e & 0x3FF and colors(e) <= ok for e in ent(m)[:4])
            and not any(e & 0x3FF for e in ent(m)[4:])}


SANDS, GRASSES = _plain(0x115), {0x008, 0x010, 0x011, 0x009}
DECOR = {}     # tileset name -> {"umbrellas": [...], "chair": [...], "palm_sand": [...], "palm_grass": [...]}


def decor_edit(name, slots, keep=None):
    """the tileset copy's edit hook: beach things in palette slots `slots` (umbrellas and chairs
    share one; the palms another). Their ids in DECOR[name]"""
    def edit(out):
        import tile_import as T
        tgt = T.Target(out)
        if keep is not None:
            tgt.compact(keep)
        prim = tileset_dir("gTileset_General_Frlg")
        sand = tgt.entries(SAND, prim)[:4]
        d = {}
        d["umbrellas"] = [T.import_metatiles(tgt, SLATEPORT, "emerald", u, [slots[0]], ground=sand, ground_like=0x124,
                                             match={5: 5}) for u in UMBRELLAS]
        d["chair"] = T.import_metatiles(tgt, SLATEPORT, "emerald", CHAIR, [slots[0]], ground=sand, ground_like=0x124,
                                        match={5: 5})
        d["palm_sand"] = T.add_palm(tgt, slots[1], sand, tgt.attr(SAND, prim))
        d["palm_grass"] = T.add_palm(tgt, slots[1], tgt.entries(GRASS, prim)[:4], tgt.attr(GRASS, prim))
        tgt.save()
        DECOR[name] = d
    return edit


# Stamps: (width, ids key, which, collision mask by cell)
STAMPS = {
    "umbrella": (2, [0, 0, 1, 1, 0, 0]),
    "chair": (1, [1, 1, 1]),
    "palm": (2, [0, 0, 0, 0, 1, 0]),
}


def flood(g, start):
    seen, todo = set(), [start]
    h, w = len(g), len(g[0])
    while todo:
        x, y = todo.pop()
        if (x, y) in seen or not (0 <= x < w and 0 <= y < h) or (g[y][x] >> 10) & 3:
            continue
        seen.add((x, y))
        todo += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    return seen


def scatter(g, name, plan, start, keep=(), seed="alola", rows=None):
    """Sets beach things about: plan [(stamp, ground ('sand'/'grass'), count)]; each on plain sand or
    grass (all its cells), spaced out, away from `keep` (links, legendaries, items), and never
    cutting anything off from `start`"""
    d = DECOR[name]
    h, w = len(g), len(g[0])
    rnd = random.Random(seed)
    placed = []
    base = len(flood(g, start))
    for stamp, ground, count in plan:
        sw, mask = STAMPS[stamp]
        sh = len(mask) // sw
        if stamp == "palm":
            ids = d["palm_" + ground]
        elif stamp == "umbrella":
            ids = None
        else:
            ids = d["chair"]
        ok_ids = SANDS if ground == "sand" else GRASSES
        cells = [(x, y) for y in range(h - sh + 1) for x in range(w - sw + 1) if rows is None or rows[0] <= y <= rows[1]]
        rnd.shuffle(cells)
        n = 0
        for x, y in cells:
            if n >= count:
                break
            box = [(x + k % sw, y + k // sw) for k in range(sw * sh)]
            if not all(g[cy][cx] & 0x3FF in ok_ids and not (g[cy][cx] >> 10) & 3 for cx, cy in box):
                continue
            if any(abs(cx - kx) <= 1 and abs(cy - ky) <= 1 for cx, cy in box for kx, ky in keep):
                continue
            if any(abs(x - px) + abs(y - py) < {"chair": 2, "palm": 4}.get(stamp, 5) for px, py in placed):
                continue
            use = ids or d["umbrellas"][n % 3]
            old = [g[cy][cx] for cx, cy in box]
            for k, (cx, cy) in enumerate(box):
                g[cy][cx] = (g[cy][cx] & 0xF000) | use[k] | (mask[k] << 10)
            if len(flood(g, start)) != base - sum(mask):
                for k, (cx, cy) in enumerate(box):
                    g[cy][cx] = old[k]
                continue
            base -= sum(mask)
            placed.append((x, y))
            n += 1
    return placed


def keep_cells(area):
    """an area's links' cells and arrivals, legendaries, items: kept clear of beach things"""
    a = AREAS[area]
    out = [p for _, (_, cells, arr, _) in a["links"].items() for p in (cells if isinstance(cells, list) else []) + [arr]]
    out += [p for n, ar, p in RELICS if ar == area] + ([CARETAKER] if area == "RESORT" else [])
    return out + [p for _, p in a["statics"]] + [p for _, p in a["items"] if p]


# ---------------------------------------------------------------------------------------------
# Hano Beach: Resort Gorgeous, with palm-garden grass on its island
# ---------------------------------------------------------------------------------------------
TALL = blk(0x00D, 0)


SHORE = {0x10C: (1, 0, 0, 1), 0x10D: (1, 0, 0, 0), 0x10E: (1, 0, 1, 0), 0x114: (0, 0, 0, 1), 0x115: (0, 0, 0, 0),
         0x116: (0, 0, 1, 0), 0x11C: (0, 1, 0, 1), 0x11D: (0, 1, 0, 0), 0x11E: (0, 1, 1, 0),
         0x125: None, 0x126: None, 0x12D: None, 0x12E: None}            # (sand and its edges on water)
OPEN_SEA = 0x12B
SEA_ROCK = (0x110, 0x111, 0x118, 0x119)                                 # (a rock in the sea, 2x2)
SEA_ROCK_2 = (0x1CB, 0x1CC, 0x1D3, 0x1D4)                               # (another, by the shore's shallows)


def paint_beach(g, x0, y0, x1, y1):
    """Widens a sandy island: the rectangle made sand (any sea rocks there gone), and every bit of
    sand around it given the right edge against the water"""
    h, w = len(g), len(g[0])
    for y in range(max(0, y0 - 2), min(h, y1 + 3)):           # (sea rocks in the way: open sea)
        for x in range(max(0, x0 - 2), min(w, x1 + 3)):
            if g[y][x] & 0x3FF in (SEA_ROCK[0], SEA_ROCK_2[0]) and any(x0 - 1 <= x + a <= x1 + 1 and y0 - 1 <= y + b <= y1 + 1
                                                                        for a in (0, 1) for b in (0, 1)):
                for a in (0, 1):
                    for b in (0, 1):
                        g[y + b][x + a] = OPEN_SEA | (1 << 12)
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            g[y][x] = 0x115 | (3 << 12)
    sea = lambda x, y: not (0 <= x < w and 0 <= y < h) or g[y][x] & 0x3FF in (OPEN_SEA,) + SEA_ROCK + SEA_ROCK_2
    for y in range(max(0, y0 - 2), min(h, y1 + 3)):
        for x in range(max(0, x0 - 2), min(w, x1 + 3)):
            if g[y][x] & 0x3FF not in SHORE:
                continue
            n, s_, e, w_ = sea(x, y - 1), sea(x, y + 1), sea(x + 1, y), sea(x - 1, y)
            m = (0x10C if n and w_ else 0x10E if n and e else 0x11C if s_ and w_ else 0x11E if s_ and e else
                 0x10D if n else 0x11D if s_ else 0x114 if w_ else 0x116 if e else
                 0x125 if sea(x - 1, y - 1) else 0x126 if sea(x + 1, y - 1) else 0x12D if sea(x - 1, y + 1) else
                 0x12E if sea(x + 1, y + 1) else 0x115)
            g[y][x] = m | (3 << 12)


def resort():
    import build_exotic_safari as B
    g, lay = read_layout("LAYOUT_FIVE_ISLAND_RESORT_GORGEOUS")
    g = [row[:] for row in g]
    beh = B.Behaviors((lay["primary_tileset"], lay["secondary_tileset"]), "frlg")
    for x0, y0, x1, y1 in ((21, 11, 29, 12), (40, 10, 47, 12), (27, 3, 35, 4), (61, 15, 67, 15), (11, 10, 14, 11)):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                v = g[y][x]
                # (the island's inside only: its shore stays sand, so no bush stands in the water)
                if (not (v >> 10) & 3 and beh(v) not in B.MB_SURFABLE and (x, y) != (39, 9)
                        and all(beh(g[y + b][x + a]) not in B.MB_SURFABLE for a in (-1, 0, 1) for b in (-1, 0, 1))
                        and v & 0x3FF not in SHORE.keys() - {0x115}):
                    g[y][x] = TALL
    paint_beach(g, 21, 13, 50, 16)                          # (a long beach along the island's south)
    paint_beach(g, 46, 3, 50, 12)                           # (and round its east end)
    B.clone_tileset("gTileset_SeviiIslands45", "AlolaResort", edit=decor_edit("AlolaResort", [7, 8], used_ids(g)))
    scatter(g, "AlolaResort", [("umbrella", "sand", 4), ("chair", "sand", 5), ("palm", "sand", 6)], (39, 9),
            keep_cells("RESORT"))
    return g, lay["width"], lay["height"]


# ---------------------------------------------------------------------------------------------
# Big Wave Beach: Treasure Beach, without One Island's harbour at its top
# ---------------------------------------------------------------------------------------------
def beach():
    g, lay = read_layout("LAYOUT_ONE_ISLAND_TREASURE_BEACH")
    g = [row[:] for row in g]
    sea = g[5][5]
    for y in range(0, 4):
        for x in range(24):
            g[y][x] = sea
    # (its foot: the rock ends in a coast - the rock's foot, sand, the shore, the sea - like Kindle Road's)
    kindle, _ = read_layout("LAYOUT_ONE_ISLAND_KINDLE_ROAD")
    for y, k in ((35, kindle[17][4]), (36, kindle[18][5]), (37, kindle[19][5]), (38, kindle[20][5]), (39, kindle[20][5])):
        for x in range(24):
            g[y][x] = k if y > 35 or (g[34][x] >> 10) & 3 else kindle[18][5]
    paint_beach(g, 5, 13, 18, 19)                           # (a wider beach above the grass)
    import build_exotic_safari as B
    B.clone_tileset("gTileset_SeviiIslands123", "AlolaBeach", edit=decor_edit("AlolaBeach", [7, 8], used_ids(g)))
    scatter(g, "AlolaBeach", [("umbrella", "sand", 3), ("chair", "sand", 4), ("palm", "sand", 4)], (11, 24),
            keep_cells("BEACH"))
    return g, 24, 40


def used_ids(g):
    return {v & 0x3FF for row in g for v in row}


# ---------------------------------------------------------------------------------------------
# Seafolk Village: Pacifidlog's rafts and houses, far out at sea (Emerald's, converted)
# ---------------------------------------------------------------------------------------------
OCEAN_W, OCEAN_H, OCEAN_DX, OCEAN_DY = 36, 30, 8, 4


def ocean():
    src, lay = read_layout("LAYOUT_PACIFIDLOG_TOWN")
    src = [row[:] for row in src]
    sea = src[19][1]
    # The Pokémon Center's raft: an open platform (the building gone; beach things on it: ocean_decor)
    for y in (12, 13):
        for x in range(6, 12):
            src[y][x] = sea
    for y in (14, 15, 16):
        for x in range(7, 11):
            src[y][x] = src[17][8]
    g = [[sea] * OCEAN_W for _ in range(OCEAN_H)]
    for y in range(9, 31):
        for x in range(20):
            v = src[y][x]
            if y == 9 and v & 0x3FF in (0x189, 0x198, 0x19A):   # (the cliff's shadow on the water: open sea)
                v = sea
            g[OCEAN_DY + y - 9][OCEAN_DX + x] = v
    rock = [[src[y][x] for x in (18, 19)] for y in (9, 10)]       # (a sea rock, 2x2)
    for rx, ry in ((2, 3), (31, 6), (4, 24), (30, 25), (16, 1), (22, 28)):
        for b in (0, 1):
            for a in (0, 1):
                g[ry + b][rx + a] = rock[b][a]
    AREAS["OCEAN"]["behaviors"] = pacifidlog_logs()
    return g, OCEAN_W, OCEAN_H


def ocean_doors():
    """the houses' and the Pokémon Center's doors, where they are in the ocean"""
    return [(x + OCEAN_DX, y - 9 + OCEAN_DY) for x, y in ((16, 13), (3, 22), (12, 24), (2, 12), (17, 21))]


RAFT_FLOOR = 0x221            # (Pacifidlog's raft floor: Emerald's metatile)


def ocean_decor_edit(out, remap):
    """(the ocean's converted tileset) an umbrella and deck chairs for the platform, on its raft floor"""
    import tile_import as T
    tgt = T.Target(out)
    floor = remap[RAFT_FLOOR]
    ground = tgt.entries(floor, None)[4:]          # (the raft's planks are its top layer, over the sea)
    raft_pal = ground[0] >> 12
    # (every palette slot is taken: the umbrella's and chairs' colors matched to the closest palette)
    import render_map
    want = render_map.load_pal(tileset_dir(SLATEPORT[1]) / "palettes" / "11.pal")
    def cost(k):
        have = B_load_pal(out / "palettes" / f"{k:02d}.pal")
        return sum(min(sum((x - y) ** 2 for x, y in zip(c[:3], h[:3])) for h in have[1:]) for c in want[1:])
    best = min(range(7, 13), key=cost)
    match = {5: raft_pal, 11: best}
    # (no shadows on the wooden platform: the raft's floor under everything)
    d = {"umbrellas": [T.import_metatiles(tgt, SLATEPORT, "emerald", UMBRELLAS[0], [], ground=ground, match=match)],
         "chair": T.import_metatiles(tgt, SLATEPORT, "emerald", CHAIR, [], ground=ground, match=match)}
    tgt.save()
    DECOR["AlolaOcean"] = d


def B_load_pal(path):
    import render_map
    return render_map.load_pal(path)


def ocean_platform(g):
    """(after conversion) the platform's umbrella and two deck chairs either side"""
    d = DECOR["AlolaOcean"]
    px, py = OCEAN_DX + 7, OCEAN_DY + 14 - 9
    put = lambda x, y, ids, w, mask: [g[y + k // w].__setitem__(x + k % w, (g[y + k // w][x + k % w] & 0xF000) | ids[k] | (mask[k] << 10))
                                      for k in range(len(ids))]
    put(px + 1, py, d["umbrellas"][0], 2, STAMPS["umbrella"][1])
    put(px, py, d["chair"], 1, STAMPS["chair"][1])
    put(px + 3, py, d["chair"], 1, STAMPS["chair"][1])


def pacifidlog_logs():
    """Pacifidlog's floating logs sink under the player (an Emerald field effect that redraws
    Pacifidlog's own metatiles): plain floor here"""
    import build_exotic_safari as B
    g, lay = read_layout("LAYOUT_PACIFIDLOG_TOWN")
    beh = B.Behaviors((lay["primary_tileset"], lay["secondary_tileset"]), "emerald")
    inv = {v: k for k, v in B.MB.items()}
    return {v & 0x3FF: "MB_NORMAL" for row in g for v in row if "PACIFIDLOG" in inv.get(beh(v), "")}


# ---------------------------------------------------------------------------------------------
# Wela Volcano Park: Mt. Ember, with Kindle Road's foot of the mountain, beach and sea below it
# ---------------------------------------------------------------------------------------------
EMBER_X0, EMBER_X1 = 6, 51            # (Mt. Ember's columns kept: 45)
KINDLE_DX = 28 - 11 - EMBER_X0        # (Kindle Road's stairs under Mt. Ember's)


def volcano():
    ember, _ = read_layout("LAYOUT_MT_EMBER_EXTERIOR")
    kindle, _ = read_layout("LAYOUT_ONE_ISLAND_KINDLE_ROAD")
    W = EMBER_X1 - EMBER_X0
    g = [row[EMBER_X0:EMBER_X1] for row in ember[:48]]
    for y in range(6, 28):
        row = []
        for X in range(W):
            x = X - KINDLE_DX
            v = kindle[y][x] if 0 <= x < 24 else None
            # Beyond Kindle Road (and its own sea at its sides): a coast drawn on - rock, the rock's
            # foot, sand, the shore, then the sea
            if not 3 <= x < 22 or (y in (18, 19) and not 5 <= x < 16):
                v = (kindle[8][5] if y < 17 else kindle[17][4] if y == 17 else kindle[18][5] if y == 18
                     else kindle[19][5] if y == 19 else kindle[20][5])
            row.append(v)
        g.append(row)
    import build_exotic_safari as B
    B.clone_tileset("gTileset_SeviiIslands123", "AlolaVolcano", edit=decor_edit("AlolaVolcano", [7, 8], used_ids(g)))
    # (palms at the mountain's green foot and on its beach: the tropics below the volcano)
    scatter(g, "AlolaVolcano", [("palm", "grass", 5)], (KINDLE_DX + 17, len(g) - 2), keep_cells("VOLCANO"), rows=(30, 48))
    scatter(g, "AlolaVolcano", [("palm", "sand", 4)], (KINDLE_DX + 17, len(g) - 2), keep_cells("VOLCANO"), rows=(48, len(g)))
    return g, W, len(g)


# ---------------------------------------------------------------------------------------------
# Mount Lanakila: Sevault Canyon, snowed over above a ragged snowline (copies of its metatiles
# in snowy palettes), green and tropical below it, the sea at its foot
# ---------------------------------------------------------------------------------------------
SNOW_SLOTS = [7, 9, 10, 11, 12]       # (palettes the canyon doesn't use: the snowy copies)


def snow_color(c):
    import colorsys
    r, g, b = c
    h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
    deg = h * 360
    lum = (0.3 * r + 0.59 * g + 0.11 * b) / 255
    if s < 0.1 or 190 <= deg <= 260:                 # greys, whites, blues: a little colder
        return (int(r * 0.92), int(g * 0.95), min(255, int(b * 1.02 + 8)))
    if 60 <= deg < 190:                               # grass -> snow, its shadows blue-grey
        t = max(0.0, min(1.0, (lum - 0.2) / 0.6))
        lo, hi = (104, 120, 160), (248, 250, 255)
        return tuple(int(lo[k] + (hi[k] - lo[k]) * t) for k in range(3))
    if lum > 0.62:                                    # sand, light rock -> snow
        return (240, 244, 255)
    t = max(0.0, min(1.0, lum * 1.15))                # rock -> icy blue-grey rock
    return (int(60 + 150 * t), int(70 + 152 * t), int(96 + 150 * t))


def lanakila():
    import build_exotic_safari as B
    g, lay = read_layout("LAYOUT_SEVEN_ISLAND_SEVAULT_CANYON")
    g = [row[:] for row in g]
    w, h = lay["width"], lay["height"]
    rnd = random.Random("lanakila")
    line, cur = [], 44
    for x in range(w):                                # (a ragged snowline)
        cur = max(38, min(50, cur + rnd.choice((-2, -1, 0, 0, 1, 2))))
        line.append(cur)
    prim = tileset_dir("gTileset_General_Frlg")
    sec = tileset_dir("gTileset_SeviiIslands67")
    pmt, smt = (prim / "metatiles.bin").read_bytes(), (sec / "metatiles.bin").read_bytes()
    entries = lambda m: struct.unpack_from("<8H", pmt, m * 16) if m < 640 else struct.unpack_from("<8H", smt, (m - 640) * 16)
    snowy = sorted({g[y][x] & 0x3FF for x in range(w) for y in range(line[x])})
    pals = sorted({v >> 12 for m in snowy for v in entries(m)})
    assert len(pals) <= len(SNOW_SLOTS), pals
    slot = dict(zip(pals, SNOW_SLOTS))
    new_id = {}

    def edit(out):
        mt = bytearray((out / "metatiles.bin").read_bytes())
        at = bytearray((out / "metatile_attributes.bin").read_bytes())
        pat = (prim / "metatile_attributes.bin").read_bytes()
        asz = len(at) // (len(mt) // 16)
        for p, s in slot.items():
            src = (prim if p < 7 else sec) / "palettes" / f"{p:02d}.pal"
            B.write_pal(out / "palettes" / f"{s:02d}.pal", [snow_color(c[:3]) for c in B.render_map.load_pal(src)])
        for m in snowy:
            new_id[m] = 0x280 + len(mt) // 16
            mt += struct.pack("<8H", *[(v & 0x0FFF) | (slot[v >> 12] << 12) for v in entries(m)])
            at += (pat[m * asz:(m + 1) * asz] if m < 640 else at[(m - 640) * asz:(m - 639) * asz])
        assert len(mt) // 16 <= 384
        (out / "metatiles.bin").write_bytes(mt)
        (out / "metatile_attributes.bin").write_bytes(at)
        # Palm trees for its green foot (a palette slot the snow didn't take)
        import tile_import as T
        free = [k for k in SNOW_SLOTS if k not in slot.values()]
        if free:
            tgt = T.Target(out)
            tgt.compact({v & 0x3FF for row in g for v in row} | set(new_id.values()))
            DECOR["AlolaLanakila"] = {"palm_grass": T.add_palm(tgt, free[0], tgt.entries(GRASS, prim)[:4], tgt.attr(GRASS, prim))}
            tgt.save()

    B.clone_tileset("gTileset_SeviiIslands67", "AlolaLanakila", edit=edit)
    for x in range(w):
        for y in range(line[x]):
            v = g[y][x]
            g[y][x] = (v & ~0x3FF) | new_id[v & 0x3FF]
    if "AlolaLanakila" in DECOR:
        scatter(g, "AlolaLanakila", [("palm", "grass", 4)], (15, 78), keep_cells("SNOW"), rows=(max(line) + 2, h - 1))
    return g, w, h


# ---------------------------------------------------------------------------------------------
# Ruins of Conflict: Six Island's Ruin Valley (its jungle of tall grass; the Dotted Hole the
# ruins' shrine)
# ---------------------------------------------------------------------------------------------
RUINS_X0, RUINS_Y0 = 4, 4


def ruins():
    g, _ = read_layout("LAYOUT_SIX_ISLAND_RUIN_VALLEY")
    g = [row[RUINS_X0:48] for row in g[RUINS_Y0:38]]
    import build_exotic_safari as B
    B.clone_tileset("gTileset_SeviiIslands67", "AlolaRuins", edit=decor_edit("AlolaRuins", [8, 7], used_ids(g)))
    scatter(g, "AlolaRuins", [("palm", "grass", 6)], R(46, 10), keep_cells("RUINS"))
    return g, 48 - RUINS_X0, 38 - RUINS_Y0


def R(x, y):
    """Ruin Valley's coordinates -> the ruins area's"""
    return (x - RUINS_X0, y - RUINS_Y0)


def V(x, y):
    """Mt. Ember's coordinates -> the volcano area's"""
    return (x - EMBER_X0, y)


# ---------------------------------------------------------------------------------------------
# The areas
# ---------------------------------------------------------------------------------------------
SEA = ("WISHIWASHI", 48, 52)
SEA_BLOCK = 0x112B            # (FireRed's open sea, General_Frlg: past the islands' edges)
VOLCANO_H = 48 + 22
AREAS = {
    "RESORT": {
        "map": "ExoticSafari_AlolaResort", "build": resort, "tilesets": ("gTileset_General_Frlg", "gTileset_AlolaResort"),
        "border": SEA_BLOCK, "music": "MUS_RG_SEVII_45", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "HALL": ("warp", [(39, 8)], (39, 9), ("HALL", None)),             # (the resort's mansion)
            "OCEAN": ("edge", [(0, y) for y in range(20)], (1, 10), ("OCEAN", "RESORT")),
            "BEACH": ("edge", [(71, y) for y in range(17)], (70, 5), ("BEACH", "RESORT")),
            "SNOW": ("edge", [(x, 0) for x in range(16, 60)], (48, 1), ("SNOW", "RESORT")),
            "VOLCANO": ("edge", [(x, 19) for x in range(16, 60)], (40, 18), ("VOLCANO", "RESORT")),
        },
        "remove_holes": [(64, 13)],
        "land": land("YUNGOOS", "PIKIPEK", "RATTATA_ALOLA", "MEOWTH_ALOLA", "ORICORIO_POM_POM", "CUTIEFLY|COMFEY",
                     "GRIMER_ALOLA", "EXEGGUTOR_ALOLA", "RAICHU_ALOLA", "MUDBRAY", ("ROWLET", 48, 50), ("PHEROMOSA", 55, 58)),
        "water": [SEA, ("PYUKUMUKU", 48, 52), ("MAREANIE", 48, 52), ("BRUXISH", 50, 54), ("DHELMISE", 52, 55)],
        "fishing": [("MAGIKARP", 45, 50), ("WISHIWASHI", 45, 50),
                    ("WISHIWASHI", 48, 52), ("MAREANIE", 48, 52), ("BRUXISH", 48, 52),
                    ("BRUXISH", 50, 54), ("WISHIWASHI", 50, 54), ("TOXAPEX", 50, 54), ("DHELMISE", 52, 55), ("GYARADOS", 52, 55)],
        "statics": [("MAGEARNA", (65, 14)), ("MELTAN", (10, 7))],
        "items": [],
        # (Silvally's Memories, hidden about the zone: each where its type would be at home)
        "hidden": [("FLYING_MEMORY", None), ("ELECTRIC_MEMORY", None), ("FAIRY_MEMORY", None)],
    },
    "BEACH": {
        "map": "ExoticSafari_AlolaBeach", "build": beach, "tilesets": ("gTileset_General_Frlg", "gTileset_AlolaBeach"),
        "border": SEA_BLOCK, "music": "MUS_RG_SEVII_123", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RESORT": ("edge", [(0, y) for y in range(24)], (1, 11), ("RESORT", "BEACH")),
        },
        "land": land("SANDYGAST", "WIMPOD", "DIGLETT_ALOLA", "WINGULL", "ORICORIO_PAU", "CRABRAWLER", "PALOSSAND",
                     "DUGTRIO_ALOLA", "PELIPPER", "GOLISOPOD", ("BUZZWOLE", 55, 58), ("STAKATAKA", 55, 58)),
        "water": [("MANTYKE", 48, 52), ("WINGULL", 48, 52), ("WAILMER", 48, 52), ("MANTINE", 50, 54), ("WAILORD", 52, 55)],
        "fishing": [("MAGIKARP", 45, 50), ("WISHIWASHI", 45, 50),
                    ("WISHIWASHI", 48, 52), ("BRUXISH", 48, 52), ("MAREANIE", 48, 52),
                    ("BRUXISH", 50, 54), ("TOXAPEX", 50, 54), ("WISHIWASHI", 50, 54), ("DHELMISE", 52, 55), ("POPPLIO", 48, 50)],
        "statics": [("COSMOG", (11, 21))],
        "items": [],
        "hidden": [("GROUND_MEMORY", None), ("FIGHTING_MEMORY", None), ("BUG_MEMORY", None)],
    },
    "OCEAN": {
        "map": "ExoticSafari_AlolaOcean", "build": ocean, "tilesets": ("gTileset_General", "gTileset_Pacifidlog"),
        "version": "emerald", "convert": ("AlolaOcean", "alola_ocean_frlg"), "border_at": (0, 0),
        "tileset_edit": ocean_decor_edit, "post_convert": ocean_platform,
        "music": "MUS_RG_SEVII_ROUTE", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RESORT": ("edge", [(OCEAN_W - 1, y) for y in range(OCEAN_H)], (OCEAN_W - 2, 14), ("RESORT", "OCEAN")),
            "RUINS": ("edge", [(0, y) for y in range(OCEAN_H)], (1, 14), ("RUINS", "OCEAN")),
        },
        "remove_holes": ocean_doors(),
        "land": land("WINGULL", "PELIPPER", "WIMPOD", "WISHIWASHI", "PYUKUMUKU", "MAREANIE", "BRUXISH", "DHELMISE",
                     "GOLISOPOD", "TOXAPEX", ("POPPLIO", 48, 50), ("NIHILEGO", 55, 58)),
        "water": [SEA, ("TENTACOOL", 48, 52), ("PYUKUMUKU", 48, 52), ("DHELMISE", 52, 55), ("NIHILEGO", 55, 58)],
        "fishing": [("MAGIKARP", 45, 50), ("WISHIWASHI", 45, 50),
                    ("WISHIWASHI", 48, 52), ("BRUXISH", 48, 52), ("MAREANIE", 48, 52),
                    ("BRUXISH", 50, 54), ("DHELMISE", 50, 54), ("TOXAPEX", 50, 54), ("WAILORD", 52, 55), ("POIPOLE", 55, 58)],
        "statics": [("ZERAORA", (OCEAN_DX + 18, OCEAN_DY + 14 - 9))],
        "items": [],
        "hidden": [("WATER_MEMORY", None), ("POISON_MEMORY", None)],
    },
    "RUINS": {
        "map": "ExoticSafari_AlolaRuins", "build": ruins, "tilesets": ("gTileset_General_Frlg", "gTileset_AlolaRuins"),
        "music": "MUS_RG_SEVII_DUNGEON", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "OCEAN": ("edge", [R(47, y) for y in range(9, 13)], R(46, 10), ("OCEAN", "RUINS")),
        },
        "remove_holes": [R(24, 24)],
        "land": land("FOMANTIS", "MORELULL", "GRUBBIN", "BOUNSWEET", "KOMALA", "STUFFUL", "ORICORIO_SENSU", "MIMIKYU",
                     "LURANTIS", "ORANGURU|PASSIMIAN", ("KARTANA", 55, 58), ("XURKITREE", 55, 58)),
        "water": [("DEWPIDER", 48, 52), ("WISHIWASHI", 48, 52), ("ARAQUANID", 50, 54), ("BRUXISH", 50, 54), ("ARAQUANID", 52, 55)],
        "statics": [("MARSHADOW", R(24, 25))],
        "items": [("DUSK_STONE", R(6, 33))],
        "hidden": [("GRASS_MEMORY", None), ("GHOST_MEMORY", None), ("PSYCHIC_MEMORY", None)],
    },
    "VOLCANO": {
        "map": "ExoticSafari_AlolaVolcano", "build": volcano, "tilesets": ("gTileset_General_Frlg", "gTileset_AlolaVolcano"),
        "music": "MUS_RG_SEVII_CAVE", "weather": "WEATHER_SUNNY", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RESORT": ("edge", [(x, VOLCANO_H - 1) for x in range(EMBER_X1 - EMBER_X0)], (KINDLE_DX + 17, VOLCANO_H - 2),
                       ("RESORT", "VOLCANO")),
            # (Mt. Ember's caves: one leads up the mountain to the other)
            "CAVE_UP": ("warp", [V(14, 24)], V(14, 25), ("VOLCANO", "CAVE_DOWN")),
            "CAVE_DOWN": ("warp", [V(39, 19)], V(39, 20), ("VOLCANO", "CAVE_UP")),
        },
        "remove_holes": [V(29, 7), V(42, 39)],
        "land": land("SALANDIT", "ROCKRUFF", "GEODUDE_ALOLA", "CUBONE", "ORICORIO", "MAROWAK_ALOLA", "JANGMO_O",
                     "TURTONATOR", "MINIOR", "TOGEDEMARU", ("LITTEN", 48, 50), ("BLACEPHALON", 55, 58)),
        "water": [SEA, ("MAREANIE", 48, 52), ("TENTACOOL", 48, 52), ("TOXAPEX", 50, 54), ("WAILMER", 50, 54)],
        "fishing": [("MAGIKARP", 45, 50), ("WISHIWASHI", 45, 50),
                    ("WISHIWASHI", 48, 52), ("MAREANIE", 48, 52), ("BRUXISH", 48, 52),
                    ("BRUXISH", 50, 54), ("TOXAPEX", 50, 54), ("WISHIWASHI", 50, 54), ("DHELMISE", 52, 55), ("GYARADOS", 52, 55)],
        "statics": [("SOLGALEO", V(29, 8)), ("NECROZMA", V(38, 11))],
        "items": [("SUN_STONE", V(19, 10)), ("N_SOLARIZER", None)],
        "hidden": [("FIRE_MEMORY", None), ("ROCK_MEMORY", None), ("DRAGON_MEMORY", None)],
    },
    "SNOW": {
        "map": "ExoticSafari_AlolaLanakila", "build": lanakila, "tilesets": ("gTileset_General_Frlg", "gTileset_AlolaLanakila"),
        "border": 0x471, "music": "MUS_RG_SEVII_67", "weather": "WEATHER_NONE", "type": "MAP_TYPE_ROUTE",
        "links": {
            "RESORT": ("edge", [(15, 79), (16, 79)] + [(x, 79) for x in list(range(0, 9)) + list(range(20, 24))], (15, 78),
                       ("RESORT", "SNOW")),
        },
        "remove_holes": [(7, 17), (14, 61)],
        "land": land("VULPIX_ALOLA", "SANDSHREW_ALOLA", "CRABRAWLER", "SNORUNT", "DRAMPA", "CRABOMINABLE",
                     "SANDSLASH_ALOLA", "ABSOL", "SNEASEL", "FROSLASS", ("CELESTEELA", 55, 58), ("GUZZLORD", 55, 58)),
        "water": [SEA, ("MANTYKE", 48, 52), ("PYUKUMUKU", 48, 52), ("MANTINE", 50, 54), ("DHELMISE", 52, 55)],
        "statics": [("LUNALA", (11, 5))],
        "items": [("MOON_STONE", (5, 9)), ("N_LUNARIZER", None)],
        "hidden": [("ICE_MEMORY", None), ("STEEL_MEMORY", None), ("DARK_MEMORY", None)],
    },
}
ROAMERS = ["TAPU_KOKO", "TAPU_LELE", "TAPU_BULU", "TAPU_FINI"]
UNIQUE_ITEMS = {"N_SOLARIZER", "N_LUNARIZER"}

# What each static legendary waits for (src/exotic_safari.c; until then a hint where it will be)
CONDITIONS = {
    "COSMOG": ("GUARDIANS", 0),                         # (the four Tapus caught)
    "SOLGALEO": ("FLUTE", 0), "LUNALA": ("FLUTE", 1),   # (the Sun Flute, the Moon Flute: relics)
    "NECROZMA": ("LIGHT", 0),                           # (Solgaleo and Lunala caught)
    "MAGEARNA": ("PARTS", 2),                           # (its four machine parts: relics 2-5)
    "MELTAN": ("STEEL", 0), "MARSHADOW": ("SHADOW", 0),
    "ZERAORA": ("STORM", 0),                            # (a thunderstorm over the village)
}
# The relics, found about the zone (flags from FLAG_EXOTIC_RELIC_FIRST, in this order)
RELICS = [
    ("SUN FLUTE", "RUINS", R(16, 12)),
    ("MOON FLUTE", "OCEAN", (OCEAN_DX + 1, OCEAN_DY + 14 - 9)),
    ("MACHINE PART", "BEACH", (19, 21)),
    ("MACHINE PART", "RUINS", R(40, 33)),
    ("MACHINE PART", "VOLCANO", V(33, 45)),
    ("MACHINE PART", "SNOW", (8, 19)),
]
CARETAKER = (37, 10)
STARTERS = ["ROWLET", "LITTEN", "POPPLIO"]


def extra(area):
    """an area's relics (and the resort's caretaker)"""
    def add(ctx):
        name = ctx["name"]
        for k, (rname, rarea, (x, y)) in enumerate(RELICS):
            if rarea != area:
                continue
            if (x, y) not in ctx["reach"]:
                print(f"WARNING {area}: relic {rname} at {(x, y)} can't be reached")
            label = f"{name}_EventScript_Relic{k}"
            o = ctx["obj"]("OBJ_EVENT_GFX_ITEM_BALL", x, y, label, "MOVEMENT_TYPE_FACE_DOWN", f"0x{0x330 + k:03X}")
            o["elevation"] = ctx["elev"](x, y)
            ctx["objects"].append(o)
            ctx["scripts"] += [f"{label}::", "\tlock", f"\tsetvar VAR_0x8004, {k}", "\tgoto ExoticSafari_EventScript_Relic", ""]
        if area == "RESORT":
            o = ctx["obj"]("OBJ_EVENT_GFX_GENTLEMAN_FRLG", *CARETAKER, "ExoticSafari_EventScript_Caretaker", "MOVEMENT_TYPE_FACE_DOWN")
            o["elevation"] = ctx["elev"](*CARETAKER)
            ctx["objects"].append(o)
    return add


for _k in AREAS:
    AREAS[_k]["extra"] = extra(_k)


def header_lines(map_id):
    lines = ["// Alola: Seafolk Village (its thunderstorms), the relics, the three first partners",
             f"#define EXOTIC_SAFARI_STORM_MAP {map_id(AREAS['OCEAN']['map'])}",
             "struct ExoticSafariRelic { u16 flag; const u8 *name; };",
             "static const struct ExoticSafariRelic sRelics[] =", "{"]
    lines += [f'    {{FLAG_EXOTIC_RELIC_FIRST + {k}, COMPOUND_STRING("{n}")}},' for k, (n, _, _) in enumerate(RELICS)]
    lines += ["};", "#define EXOTIC_SAFARI_FIRST_PART 2",
              "static const u16 sAlolaStarters[] = {" + ", ".join(f"SPECIES_{s}" for s in STARTERS) + "};"]
    return lines
