#!/usr/bin/env python3
"""The Tera berries' one-time finds (Pokémon Canvas, src/berry_field.c): one item ball per type,
on a map that fits the type, in the order the story reaches them.

Each ball goes where the player can walk to it from the map's warps and edges, not next to anyone or
any sign: a dead end if the map has one, else a corner it can't cut anything off from; as far in as
possible.
Re-running moves nothing that's already placed (a ball keeps its spot; delete it from the map to
have it placed again).

    python3 tools/gym_hack/place_berry_finds.py

Writes the maps' map.json (an object with FLAG_BERRY_FIND_<TYPE>) and the find scripts at the end
of data/scripts/berry_field.inc.
"""
import json
import struct
import sys
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))
import build_exotic_safari as B      # noqa: E402

FINDS = [   # (type, berry, map)
    ("NORMAL", "CHILAN", "Route1_Frlg"),
    ("BUG", "TANGA", "ViridianForest_Frlg"),
    ("FIGHTING", "CHOPLE", "Route3_Frlg"),
    ("ROCK", "CHARTI", "MtMoon_1F_Frlg"),
    ("GRASS", "RINDO", "Route24_Frlg"),
    ("WATER", "PASSHO", "Route25_Frlg"),
    ("POISON", "KEBIA", "Route4_Frlg"),
    ("ELECTRIC", "WACAN", "Route6_Frlg"),
    ("GROUND", "SHUCA", "DiglettsCave_B1F_Frlg"),
    ("PSYCHIC", "PAYAPA", "Route5_Frlg"),
    ("GHOST", "KASIB", "PokemonTower_6F_Frlg"),
    ("FAIRY", "ROSELI", "CeladonCity_Frlg"),
    ("DARK", "COLBUR", "RocketHideout_B1F_Frlg"),   # (not B2F/B3F: their spinning floors)
    ("FLYING", "COBA", "Route13_Frlg"),
    ("STEEL", "BABIRI", "SilphCo_7F_Frlg"),
    ("FIRE", "OCCA", "PokemonMansion_1F_Frlg"),
    ("ICE", "YACHE", "SeafoamIslands_1F_Frlg"),
    ("DRAGON", "HABAN", "VictoryRoad_2F_Frlg"),
]
LAYOUTS = {l["id"]: l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if "id" in l}
JUMPS = {B.MB["MB_JUMP_EAST"]: (1, 0), B.MB["MB_JUMP_WEST"]: (-1, 0), B.MB["MB_JUMP_NORTH"]: (0, -1), B.MB["MB_JUMP_SOUTH"]: (0, 1)}
OPEN = {B.MB["MB_NORMAL"], B.MB["MB_CAVE"]}
GRASS = {B.MB.get("MB_TALL_GRASS"), B.MB.get("MB_LONG_GRASS")}


def spot(name):
    m = json.loads((ROOT / f"data/maps/{name}/map.json").read_text())
    lay = LAYOUTS[m["layout"]]
    w, h = lay["width"], lay["height"]
    raw = struct.unpack(f"<{w * h}H", (ROOT / lay["blockdata_filepath"]).read_bytes())
    g = [[raw[y * w + x] for x in range(w)] for y in range(h)]
    beh = B.Behaviors([lay["primary_tileset"], lay["secondary_tileset"]], "frlg")
    taken = {(o["x"], o["y"]) for o in m.get("object_events") or []}
    signs = {(b["x"], b["y"]) for b in m.get("bg_events") or [] if b.get("type") == "sign"}
    warps = {(wp["x"], wp["y"]) for wp in m.get("warp_events") or []}
    coords = {(c["x"], c["y"]) for c in m.get("coord_events") or []}

    def elev(x, y):
        return g[y][x] >> 12

    def walkable(x, y):
        if not (0 <= x < w and 0 <= y < h) or (x, y) in taken or (x, y) in signs:
            return False
        b = beh(g[y][x])
        return not (g[y][x] >> 10) & 3 and b not in B.MB_SURFABLE and b not in JUMPS

    starts = set(warps)
    for c in (m.get("connections") or []):
        d = c["direction"]
        cells = ([(x, 0) for x in range(w)] if d == "up" else [(x, h - 1) for x in range(w)] if d == "down"
                 else [(0, y) for y in range(h)] if d == "left" else [(w - 1, y) for y in range(h)])
        starts |= {p for p in cells if walkable(*p)}
    def reach(blocked=None):
        dist, q = {}, deque()
        for s in starts:
            if 0 <= s[0] < w and 0 <= s[1] < h and s != blocked:
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
                    nx, ny = nx + dx, ny + dy           # (down a ledge)
                if (nx, ny) in dist or (nx, ny) == blocked or not walkable(nx, ny):
                    continue
                nz = elev(nx, ny)
                if not (z == 0 or nz in (0, 15) or nz == z):
                    continue
                dist[(nx, ny)] = dist[(x, y)] + 1
                q.append((nx, ny))
        return dist

    dist = reach()

    def near_someone(x, y):
        return any((x + dx, y + dy) in taken or (x + dx, y + dy) in signs
                   for dx in (-1, 0, 1) for dy in (-1, 0, 1))

    def connected_without(cell):
        """does every other reachable cell stay reachable with a ball on `cell`?"""
        return len(reach(cell)) == len(dist) - 1

    def near_warp(x, y):
        return any(abs(x - wx) + abs(y - wy) <= 2 for wx, wy in warps)

    cands = []
    for (x, y), d in dist.items():
        if (x, y) in warps or (x, y) in coords or (x, y) in starts or near_someone(x, y) or beh(g[y][x]) not in OPEN:
            continue
        if not (1 <= x < w - 1 and 1 <= y < h - 1) or near_warp(x, y):
            continue
        ways = [(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)) if (x + dx, y + dy) in dist]
        if not ways or any(wy in warps for wy in ways):
            continue
        cands.append((len(ways), -d, x, y))
    best = None
    for n, negd, x, y in sorted(cands)[:60]:
        if n == 1 or connected_without((x, y)):
            best = (-negd, x, y, elev(x, y))
            break
    return m, best


def main():
    scripts = []
    for ty, berry, name in FINDS:
        flag = f"FLAG_BERRY_FIND_{ty}"
        label = f"BerryField_EventScript_Find{ty.title()}"
        path = ROOT / f"data/maps/{name}/map.json"
        m, best = spot(name)
        existing = [o for o in m["object_events"] if o.get("flag") == flag]
        if existing:
            print(f"{ty:9} {berry:7} {name}: already at ({existing[0]['x']}, {existing[0]['y']})")
        elif best is None:
            raise SystemExit(f"{name}: no dead end for the {berry} BERRY")
        else:
            _, x, y, z = best
            m["object_events"].append({
                "type": "object", "graphics_id": "OBJ_EVENT_GFX_ITEM_BALL", "x": x, "y": y, "elevation": z or 3,
                "movement_type": "MOVEMENT_TYPE_FACE_DOWN", "movement_range_x": 1, "movement_range_y": 1,
                "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
                "script": label, "flag": flag})
            path.write_text(json.dumps(m, indent=2) + "\n")
            print(f"{ty:9} {berry:7} {name}: ({x}, {y}), {best[0]} steps in")
        scripts.append(f"{label}::\n\tfinditem ITEM_{berry}_BERRY\n\tend\n")
    inc = ROOT / "data/scripts/berry_field.inc"
    text = inc.read_text()
    marker = "@ FLAG_BERRY_FIND_*: the item ball's own flag)\n"
    text = text[:text.index(marker) + len(marker)] + "".join(scripts)
    inc.write_text(text)


if __name__ == "__main__":
    main()
