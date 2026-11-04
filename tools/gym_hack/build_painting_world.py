#!/usr/bin/env python3
"""The painter's paintings (Pokémon Canvas; src/painter.c): the places her easel leads to.

    python3 tools/gym_hack/build_painting_world.py

Each painting is one of DESTINATIONS, a real map seen in scrambled colours with nobody there but a
figure made of paint. For each, this finds where the player arrives (a floor tile by its main
door, or the middle of a route) and a spot for the figure a few steps away, reachable on foot (no
Surf, Cut, Strength or ledges: water, ledges, warps and anything solid count as walls), and
  - adds the figure to the map (OBJ_EVENT_GFX_PAINT_FIGURE, PaintWorld_EventScript_Figure: shown
    only inside a painting, when everyone else on the map is hidden: src/painter.c);
  - writes src/data/painting_destinations.h, the table the easel picks from.
Re-running is safe (the figure is placed again).
"""
import json
import struct
import sys
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
import build_exotic_safari as B  # noqa: E402

FIGURE_SCRIPT = "PaintWorld_EventScript_Figure"
DESTINATIONS = [
    # routes
    "Route1_Frlg", "Route5_Frlg", "Route7_Frlg",
    # caves
    "MtMoon_B1F_Frlg", "DiglettsCave_NorthEntrance_Frlg", "MtEmber_SummitPath_1F_Frlg", "MtEmber_RubyPath_B5F_Frlg",
    # rooms
    "CeladonCity_Condominiums_1F_Frlg", "CeladonCity_Hotel_Frlg", "CeladonCity_Restaurant_Frlg",
    "CeladonCity_DepartmentStore_2F_Frlg", "CeruleanCity_House3_Frlg", "FuchsiaCity_WardensHouse_Frlg",
    "LavenderTown_House2_Frlg", "SaffronCity_House_Frlg", "VermilionCity_House3_Frlg", "ViridianCity_School_Frlg",
    "Route16_House_Frlg", "Route2_EastBuilding_Frlg", "Route12_FishingHouse_Frlg", "UndergroundPath_NorthEntrance_Frlg",
]
INV = {v: k for k, v in B.MB.items()}


def map_const(name):
    j = json.loads((ROOT / "data/maps" / name / "map.json").read_text())
    return j["id"]


def walkable_grid(name):
    L = {l["id"]: l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if "id" in l}
    j = json.loads((ROOT / "data/maps" / name / "map.json").read_text())
    l = L[j["layout"]]
    w, h = l["width"], l["height"]
    raw = struct.unpack(f"<{w * h}H", (ROOT / l["blockdata_filepath"]).read_bytes())
    beh = B.Behaviors((l["primary_tileset"], l["secondary_tileset"]), "frlg")
    warps = {(wp["x"], wp["y"]) for wp in j["warp_events"]}
    ok = set()
    for y in range(h):
        for x in range(w):
            v = raw[y * w + x]
            if (v >> 10) & 3:
                continue
            b = beh(v)
            n = INV.get(b, "")
            if b in B.MB_SURFABLE or "JUMP" in n or "WARP" in n or "DOOR" in n or "LADDER" in n or "STAIRS" in n \
               or "WATERFALL" in n or "CURRENT" in n or "ICE" in n or "HOLE" in n or "CRACKED" in n:
                continue
            if (x, y) in warps:
                continue
            ok.add((x, y))
    return j, w, h, ok


def bfs(ok, start):
    dist = {start: 0}
    q = deque([start])
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n in ok and n not in dist:
                dist[n] = dist[(x, y)] + 1
                q.append(n)
    return dist


def open_neighbours(ok, p):
    x, y = p
    return sum((x + dx, y + dy) in ok for dx in (-1, 0, 1) for dy in (-1, 0, 1))


def place(name):
    j, w, h, ok = walkable_grid(name)
    # where the player arrives: by the first door (a floor tile next to it), else the map's middle
    spawn = None
    for wp in j["warp_events"]:
        for dx, dy in ((0, -1), (0, 1), (-1, 0), (1, 0)):
            p = (wp["x"] + dx, wp["y"] + dy)
            if p in ok:
                spawn = p
                break
        if spawn:
            break
    if spawn is None or len(bfs(ok, spawn)) < 12:
        cx, cy = w // 2, h // 2
        spawn = min(ok, key=lambda p: (p[0] - cx) ** 2 + (p[1] - cy) ** 2 - 4 * len(bfs(ok, p)) if False else (p[0] - cx) ** 2 + (p[1] - cy) ** 2)
        # (the middle of the biggest open area near the centre)
        best = None
        for p in sorted(ok, key=lambda p: (p[0] - cx) ** 2 + (p[1] - cy) ** 2)[:40]:
            n = len(bfs(ok, p))
            if best is None or n > best[0]:
                best = (n, p)
        spawn = best[1]
    dist = bfs(ok, spawn)
    # the figure: 4-7 steps away, on open ground (it stands where it can be walked up to)
    cands = [p for p, d in dist.items() if 4 <= d <= 7 and open_neighbours(ok, p) >= 7]
    if not cands:
        cands = [p for p, d in dist.items() if 3 <= d <= 9 and open_neighbours(ok, p) >= 5]
    figure = max(cands, key=lambda p: (open_neighbours(ok, p), -abs(dist[p] - 5), -p[1], -p[0]))
    return j, spawn, figure


def main():
    rows = []
    for name in DESTINATIONS:
        j, spawn, figure = place(name)
        objs = [o for o in j["object_events"] if o.get("script") != FIGURE_SCRIPT]
        objs.append({"type": "object", "graphics_id": "OBJ_EVENT_GFX_PAINT_FIGURE", "x": figure[0], "y": figure[1],
                     "elevation": 3, "movement_type": "MOVEMENT_TYPE_LOOK_AROUND", "movement_range_x": 0,
                     "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
                     "script": FIGURE_SCRIPT, "flag": "0"})
        j["object_events"] = objs
        (ROOT / "data/maps" / name / "map.json").write_text(json.dumps(j, indent=2) + "\n")
        rows.append((j["id"], spawn, figure, name))
        print(f"{name:42s} arrive {spawn}  figure {figure}")
    lines = ["// Generated by tools/gym_hack/build_painting_world.py: the places the painter's easel leads to",
             "// (the map, where the player arrives; the figure is in the map itself)",
             "static const struct PaintingDestination sPaintingDestinations[] =", "{"]
    for mid, s, f, name in rows:
        lines.append(f"    {{MAP_GROUP({mid}), MAP_NUM({mid}), {s[0]}, {s[1]}}},   // ({name}; the figure at {f[0]}, {f[1]})")
    lines += ["};", ""]
    (ROOT / "src/data/painting_destinations.h").write_text("\n".join(lines))


if __name__ == "__main__":
    main()
