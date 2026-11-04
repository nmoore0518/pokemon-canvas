#!/usr/bin/env python3
"""Route 21's hidden cave (Pokémon Canvas): the Stadium trainer and Surfing Pikachu
(data/maps/Route21_StadiumCave_Frlg/scripts.inc, src/surfing_pikachu.c).

    python3 tools/gym_hack/build_stadium_cave.py

- Route 21 (south): a tiny rock islet behind the line of rocks on the east side (one rock taken out
  to let the player surf through, more rocks closing it in), its cave mouth at CAVE_DOOR.
- Route21_StadiumCave_Frlg: the Lost Cave entrance's room, cut down to a small one, the trainer and a
  solid gold Diglett (OBJ_EVENT_GFX_GOLD_DIGLETT: tools/gym_hack/gold_diglett_art.py).
Re-running is safe (the route part expects the route as it shipped: restore its map.bin first).
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ROUTE = ROOT / "data/maps/Route21_South_Frlg/map.json"
NAME, MAP_ID, LAYOUT_ID = "Route21_StadiumCave_Frlg", "MAP_ROUTE21_STADIUM_CAVE", "LAYOUT_ROUTE21_STADIUM_CAVE"
# A tiny islet tucked behind the line of rocks on the route's east side, in the sea pocket between
# the rocks and Cinnabar (out of reach as the route shipped): one rock (GAP) is taken out so the
# player can surf through, and more rocks (ROCKS) close the pocket round the islet, so all that's
# reachable is the gap, a little water beside the islet and its sand. Its rock is built from Route
# 20's mound pieces (same tileset): a rounded top, the cave mouth, a strip of sand.
ISLET = (19, 42)                      # (its rock's top-left)
GAP = [(17, 41), (18, 41), (17, 42), (18, 42)]
WATER = 0x112B
ROCK = [[0x517, 0x46C, 0x51F],        # (from Route 20: collision on)
        [0x4A8, 0x30A9, 0x4AA]]       # (the door in the middle)
SHORE = [0x311C, 0x311D, 0x311E]      # (Route 21's sandbars' bottom edge)
ROCK_STYLES = [[0x510, 0x511, 0x518, 0x519], [0x5CB, 0x5CC, 0x5D3, 0x5D4]]   # (the route's own rocks)
ROCKS = [(20, 40, 1), (22, 40, 0), (22, 42, 1), (22, 44, 0), (22, 46, 1), (18, 45, 1), (20, 45, 0)]
SEA = [(x, y) for y in range(41, 45) for x in range(18, 22)]
DEEP_EDGES = {(20, 47): 0x1D0, (21, 47): 0x1D1}     # (the deep water below now meets the rocks)
CAVE_DOOR = (ISLET[0] + 1, ISLET[1] + 1)
TRAINER, DIGLETT, EXIT = (4, 2), (6, 2), (4, 5)
# (the cave: the Lost Cave entrance's room cut down to a 5x4 floor: its rows and columns kept)
CAVE_ROWS, CAVE_COLS = [0, 1, 2, 3, 4, 8, 9, 10], [0, 1, 2, 3, 5, 7, 8, 9, 10]
CAVE_COLS_BOTTOM = [0, 1, 2, 4, 5, 6, 8, 9, 10]


def layouts():
    return json.loads((ROOT / "data/layouts/layouts.json").read_text())


def grid(layout):
    w, h = layout["width"], layout["height"]
    path = ROOT / layout["blockdata_filepath"]
    return w, h, path, list(struct.unpack(f"<{w * h}H", path.read_bytes()))


def island():
    """(Run on the route as it shipped: the pocket's sea is plain open water.)"""
    L = {l["id"]: l for l in layouts()["layouts"] if "id" in l}
    m = json.loads(ROUTE.read_text())
    w, h, path, raw = grid(L[m["layout"]])
    for x, y in GAP:
        raw[y * w + x] = WATER
    x0, y0 = ISLET
    for x, y in SEA:
        raw[y * w + x] = WATER
    for x, y, style in ROCKS:
        tl, tr, bl, br = ROCK_STYLES[style]
        raw[y * w + x], raw[y * w + x + 1] = tl, tr
        raw[(y + 1) * w + x], raw[(y + 1) * w + x + 1] = bl, br
    for (x, y), v in DEEP_EDGES.items():
        raw[y * w + x] = v
    for dy, row in enumerate(ROCK):
        for dx, v in enumerate(row):
            raw[(y0 + dy) * w + x0 + dx] = v
    for dx, v in enumerate(SHORE):
        raw[(y0 + 2) * w + x0 + dx] = v
    path.write_bytes(struct.pack(f"<{w * h}H", *raw))
    warps = [wp for wp in m["warp_events"] if wp["dest_map"] != MAP_ID]
    warps.append({"x": CAVE_DOOR[0], "y": CAVE_DOOR[1], "elevation": 3, "dest_map": MAP_ID, "dest_warp_id": "0"})
    m["warp_events"] = warps
    ROUTE.write_text(json.dumps(m, indent=2) + "\n")
    return len(warps) - 1


def cave(route_warp):
    data = layouts()
    L = {l["id"]: l for l in data["layouts"] if "id" in l}
    src = L["LAYOUT_FIVE_ISLAND_LOST_CAVE_ENTRANCE"]
    d = ROOT / "data/layouts" / NAME
    d.mkdir(exist_ok=True)
    sw, _, _, src_raw = grid(src)
    rows = [[src_raw[r * sw + c] for c in (CAVE_COLS_BOTTOM if r == 9 else CAVE_COLS)] for r in CAVE_ROWS]
    w, h = len(CAVE_COLS), len(CAVE_ROWS)
    raw = [v for row in rows for v in row]
    (d / "map.bin").write_bytes(struct.pack(f"<{w * h}H", *raw))
    (d / "border.bin").write_bytes((ROOT / src["border_filepath"]).read_bytes())
    data["layouts"] = [l for l in data["layouts"] if l.get("id") != LAYOUT_ID]
    if True:
        entry = dict(src, id=LAYOUT_ID, width=w, height=h, name=NAME.replace("_Frlg", "") + "_Layout",
                     border_filepath=f"data/layouts/{NAME}/border.bin", blockdata_filepath=f"data/layouts/{NAME}/map.bin")
        data["layouts"].append(entry)
        (ROOT / "data/layouts/layouts.json").write_text(json.dumps(data, indent=2) + "\n")
    m = {
        "id": MAP_ID, "name": NAME, "layout": LAYOUT_ID, "music": "MUS_RG_SEVII_CAVE", "region": "REGION_KANTO",
        "region_map_section": "MAPSEC_ROUTE_21", "requires_flash": False, "weather": "WEATHER_NONE",
        "map_type": "MAP_TYPE_UNDERGROUND", "allow_cycling": False, "allow_escaping": True, "allow_running": True,
        "show_map_name": False, "floor_number": 0, "battle_scene": "MAP_BATTLE_SCENE_NORMAL", "connections": None,
        "object_events": [
            {"type": "object", "graphics_id": "OBJ_EVENT_GFX_COOLTRAINER_M", "x": TRAINER[0], "y": TRAINER[1],
             "elevation": 3, "movement_type": "MOVEMENT_TYPE_FACE_DOWN", "movement_range_x": 0, "movement_range_y": 0,
             "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
             "script": "Route21_StadiumCave_EventScript_Trainer", "flag": "0"},
            {"type": "object", "graphics_id": "OBJ_EVENT_GFX_GOLD_DIGLETT", "x": DIGLETT[0], "y": DIGLETT[1],
             "elevation": 3, "movement_type": "MOVEMENT_TYPE_FACE_DOWN", "movement_range_x": 0, "movement_range_y": 0,
             "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
             "script": "Route21_StadiumCave_EventScript_GoldDiglett", "flag": "0"},
        ],
        "warp_events": [{"x": EXIT[0], "y": EXIT[1], "elevation": 3, "dest_map": "MAP_ROUTE21_SOUTH",
                         "dest_warp_id": str(route_warp)}],
        "coord_events": [], "bg_events": [],
    }
    md = ROOT / "data/maps" / NAME
    md.mkdir(exist_ok=True)
    (md / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    groups = ROOT / "data/maps/map_groups.json"
    g = json.loads(groups.read_text())
    if NAME not in g["gMapGroup_Dungeons_Frlg"]:
        g["gMapGroup_Dungeons_Frlg"].append(NAME)     # (last: no other map's number changes)
        groups.write_text(json.dumps(g, indent=2) + "\n")
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    inc = f'\t.include "data/maps/{NAME}/scripts.inc"\n'
    if inc not in text:
        anchor = '\t.include "data/maps/Route21_South_Frlg/scripts.inc"\n'
        assert anchor in text
        ev.write_text(text.replace(anchor, anchor + inc, 1))


if __name__ == "__main__":
    cave(island())
    print("cave door on Route 21 at", CAVE_DOOR)
