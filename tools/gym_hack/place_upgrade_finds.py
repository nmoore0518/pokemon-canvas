#!/usr/bin/env python3
"""The Lucky Egg's and Amulet Coin's upgrade materials as hidden items (src/item_upgrades.c).

Clovers grow in the grass of the routes (the rarer ones further along); Grandpa Oak's old coins are
in caves and ruins. Each is a hidden item (the Itemfinder finds it) with its own flag,
FLAG_UPGRADE_FIND_FIRST + n (include/constants/flags.h).

A spot is a cell the player can reach from the map's warps and edges (tools/gym_hack/place_berry_finds.py
spot's reach), of the right kind (grass for a clover, floor for a coin), not near a warp, a person
or a sign, and as far in as possible, preferring cells against a wall.
Re-running moves nothing that's already placed (a find keeps its spot: delete it from the map to have
it placed again).

    python3 tools/gym_hack/place_upgrade_finds.py
"""
import json
import struct
import sys
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))
import build_exotic_safari as B      # noqa: E402
from place_berry_finds import LAYOUTS, JUMPS, OPEN, GRASS   # noqa: E402

FLAG_FIRST = 0x8C3
FINDS = [   # (item, map) - the order is the flags' order: only add to the end
    ("LUCKY_CLOVER", "Route1_Frlg"),
    ("LUCKY_CLOVER", "Route22_Frlg"),
    ("LUCKY_CLOVER", "Route2_Frlg"),
    ("LUCKY_CLOVER", "Route24_Frlg"),
    ("LUCKY_CLOVER", "Route25_Frlg"),
    ("LUCKY_CLOVER", "Route6_Frlg"),
    ("LUCKY_CLOVER", "Route11_Frlg"),
    ("LUCKY_CLOVER", "Route3_Frlg"),
    ("FOUR_LEAF_CLOVER", "Route10_Frlg"),
    ("FOUR_LEAF_CLOVER", "Route12_Frlg"),
    ("FOUR_LEAF_CLOVER", "Route14_Frlg"),
    ("FOUR_LEAF_CLOVER", "Route15_Frlg"),
    ("FOUR_LEAF_CLOVER", "SafariZone_East_Frlg"),
    ("FOUR_LEAF_CLOVER", "Route21_North_Frlg"),
    ("GOLDEN_CLOVER", "SafariZone_North_Frlg"),
    ("GOLDEN_CLOVER", "Route23_Frlg"),
    ("COPPER_COIN", "MtMoon_1F_Frlg"),
    ("COPPER_COIN", "MtMoon_B1F_Frlg"),
    ("COPPER_COIN", "MtMoon_B2F_Frlg"),
    ("COPPER_COIN", "DiglettsCave_B1F_Frlg"),
    ("COPPER_COIN", "RockTunnel_1F_Frlg"),
    ("COPPER_COIN", "RockTunnel_B1F_Frlg"),
    ("COPPER_COIN", "PokemonTower_4F_Frlg"),
    ("COPPER_COIN", "PowerPlant_Frlg"),
    ("SILVER_COIN", "SeafoamIslands_B1F_Frlg"),
    ("SILVER_COIN", "SeafoamIslands_B3F_Frlg"),
    ("SILVER_COIN", "PokemonMansion_2F_Frlg"),
    ("SILVER_COIN", "VictoryRoad_1F_Frlg"),
    ("SILVER_COIN", "VictoryRoad_3F_Frlg"),
    ("SILVER_COIN", "CeruleanCave_1F_Frlg"),
    ("GOLD_COIN", "PokemonMansion_B1F_Frlg"),
    ("GOLD_COIN", "CeruleanCave_B1F_Frlg"),
]


# (late maps, where the player has Surf: the search can cross water)
SURF = {"Route21_North_Frlg", "SeafoamIslands_B1F_Frlg", "SeafoamIslands_B3F_Frlg", "CeruleanCave_1F_Frlg",
        "CeruleanCave_B1F_Frlg"}


def flag_name(n):
    return f"FLAG_UPGRADE_FIND_{n + 1:02}"


def spot(name, want_grass):
    m = json.loads((ROOT / f"data/maps/{name}/map.json").read_text())
    lay = LAYOUTS[m["layout"]]
    w, h = lay["width"], lay["height"]
    raw = struct.unpack(f"<{w * h}H", (ROOT / lay["blockdata_filepath"]).read_bytes())
    g = [[raw[y * w + x] for x in range(w)] for y in range(h)]
    beh = B.Behaviors([lay["primary_tileset"], lay["secondary_tileset"]], "frlg")
    people = {(o["x"], o["y"]) for o in m.get("object_events") or []
              if "CUTTABLE_TREE" not in o.get("graphics_id", "")}   # (the player can Cut by then)
    bgs = {(b["x"], b["y"]) for b in m.get("bg_events") or []}
    warps = {(wp["x"], wp["y"]) for wp in m.get("warp_events") or []}
    coords = {(c["x"], c["y"]) for c in m.get("coord_events") or []}

    def elev(x, y):
        return g[y][x] >> 12

    def walkable(x, y):
        if not (0 <= x < w and 0 <= y < h) or (x, y) in people:
            return False
        b = beh(g[y][x])
        return not (g[y][x] >> 10) & 3 and (name in SURF or b not in B.MB_SURFABLE) and b not in JUMPS

    starts = set(warps)
    for c in (m.get("connections") or []):
        d = c["direction"]
        cells = ([(x, 0) for x in range(w)] if d == "up" else [(x, h - 1) for x in range(w)] if d == "down"
                 else [(0, y) for y in range(h)] if d == "left" else [(w - 1, y) for y in range(h)])
        starts |= {p for p in cells if walkable(*p)}
    dist, q = {}, deque()
    for s in starts:
        if 0 <= s[0] < w and 0 <= s[1] < h:
            dist[s] = 0
            q.append(s)
    while q:
        x, y = q.popleft()
        z = elev(x, y)
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if not (0 <= nx < w and 0 <= ny < h):
                continue
            if beh(g[ny][nx]) in JUMPS and JUMPS[beh(g[ny][nx])] == (dx, dy):
                nx, ny = nx + dx, ny + dy
            if (nx, ny) in dist or not walkable(nx, ny):
                continue
            nz = elev(nx, ny)
            surfing = name in SURF and (beh(g[y][x]) in B.MB_SURFABLE or beh(g[ny][nx]) in B.MB_SURFABLE)
            if not (z == 0 or nz in (0, 15) or nz == z or surfing):
                continue
            dist[(nx, ny)] = dist[(x, y)] + 1
            q.append((nx, ny))

    def near(x, y, cells, r):
        return any(abs(x - a) <= r and abs(y - b) <= r for a, b in cells)

    best = None
    for (x, y), d in dist.items():
        if (x, y) in starts or (x, y) in coords or (x, y) in bgs:
            continue
        b = beh(g[y][x])
        if (b not in GRASS) if want_grass else (b not in OPEN):
            continue
        if near(x, y, warps, 2) or near(x, y, people, 1) or near(x, y, bgs, 1):
            continue
        walls = sum(1 for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)) if (x + dx, y + dy) not in dist)
        score = (min(walls, 2), d)
        if best is None or score > best[0]:
            best = (score, x, y)
    return m, best


def main():
    for n, (item, name) in enumerate(FINDS):
        flag = flag_name(n)
        path = ROOT / f"data/maps/{name}/map.json"
        m, best = spot(name, "CLOVER" in item)
        existing = [b for b in m.get("bg_events") or [] if b.get("flag") == flag]
        if existing:
            print(f"{flag} {item:16} {name}: already at ({existing[0]['x']}, {existing[0]['y']})")
            continue
        if best is None:
            raise SystemExit(f"{name}: nowhere for the {item}")
        (_, d), x, y = best
        m.setdefault("bg_events", []).append({
            "type": "hidden_item", "x": x, "y": y, "elevation": 0, "item": f"ITEM_{item}",
            "flag": flag, "quantity": 1, "underfoot": False})
        path.write_text(json.dumps(m, indent=2) + "\n")
        print(f"{flag} {item:16} {name}: ({x}, {y}), {d} steps in")

    lines = [f"#define {flag_name(n):26} 0x{FLAG_FIRST + n:03X}   // ({item.replace('_', ' ').lower()}: {name.removesuffix('_Frlg')})"
             for n, (item, name) in enumerate(FINDS)]
    hdr = ROOT / "include/constants/flags.h"
    text = hdr.read_text()
    begin, end = "// (generated: tools/gym_hack/place_upgrade_finds.py)\n", "// (end of the upgrade finds)\n"
    block = (begin + "// The upgrade materials' hidden items (src/item_upgrades.c): FireRed's spare system flags 0x8C3 on\n"
             + "\n".join(lines) + "\n" + end)
    if begin in text:
        text = text[:text.index(begin)] + block + text[text.index(end) + len(end):]
    else:
        anchor = "\n#if TESTING\n"
        text = text.replace(anchor, "\n" + block + anchor, 1)
    hdr.write_text(text)


if __name__ == "__main__":
    main()
