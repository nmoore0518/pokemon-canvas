#!/usr/bin/env python3
"""Route 21's hidden cave (Pokémon Canvas): the Stadium trainer and Surfing Pikachu
(data/maps/Route21_StadiumCave_Frlg/scripts.inc, src/surfing_pikachu.c).

    python3 tools/gym_hack/build_stadium_cave.py

- Route 21 (south): a rocky islet out on the open water on the east side, a copy of one of the
  Seafoam Islands' mounds (Route 20's, same tileset), its cave mouth at CAVE_DOOR.
- Route21_StadiumCave_Frlg: the Lost Cave entrance's room (its ladder filled in), the trainer and a
  solid gold Diglett (OBJ_EVENT_GFX_GOLD_DIGLETT: tools/gym_hack/gold_diglett_art.py).
Re-running is safe.
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ROUTE = ROOT / "data/maps/Route21_South_Frlg/map.json"
NAME, MAP_ID, LAYOUT_ID = "Route21_StadiumCave_Frlg", "MAP_ROUTE21_STADIUM_CAVE", "LAYOUT_ROUTE21_STADIUM_CAVE"
SRC_MAP, SRC, SIZE = "Route20_Frlg", (67, 6), (11, 9)    # (Route 20's east mound: its rock only)
DST = (9, 25)
# (its east side runs into the sea on Route 20: here it gets a wall and corners like its west side's)
FIX = {(77, 10): 0x0A2, (77, 11): 0x0A2, (77, 12): 0x0AA, (76, 13): 0x0AA, (77, 13): 0x115,
       (75, 14): 0x0AA, (76, 14): 0x115, (77, 14): 0x115}
MOVE_SWIMMERS = {(10, 29): (6, 30), (15, 24): (15, 22)}     # (swimmers who'd be on the new sand)
CAVE_DOOR = (DST[0] + 72 - SRC[0], DST[1] + 14 - SRC[1])
# Route 21's own sand and shore metatiles (its sandbars): plain sand, then the shore's edges and corners
SAND, TOP, BOTTOM, LEFT, RIGHT = 0x115, 0x10D, 0x11D, 0x114, 0x116
TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT = 0x10C, 0x10E, 0x11C, 0x11E
NOT_ROCK = {0x115, 0x120, 0x121, 0x123, 0x128, 0x129, 0x12A, 0x12B, 0x12C, 0x130, 0x131, 0x280, 0x2C, 0x113, 0x11B, 0x002}
TRAINER, DIGLETT, EXIT = (5, 3), (8, 2), (5, 8)


def layouts():
    return json.loads((ROOT / "data/layouts/layouts.json").read_text())


def grid(layout):
    w, h = layout["width"], layout["height"]
    path = ROOT / layout["blockdata_filepath"]
    return w, h, path, list(struct.unpack(f"<{w * h}H", path.read_bytes()))


def island():
    L = {l["id"]: l for l in layouts()["layouts"] if "id" in l}
    m = json.loads(ROUTE.read_text())
    sw, _, _, src = grid(L[json.loads((ROOT / "data/maps" / SRC_MAP / "map.json").read_text())["layout"]])
    w, h, path, raw = grid(L[m["layout"]])
    rock = {}
    for dy in range(SIZE[1]):
        for dx in range(SIZE[0]):
            v = src[(SRC[1] + dy) * sw + SRC[0] + dx]
            if (SRC[0] + dx, SRC[1] + dy) in FIX:
                v = (v & ~0x3FF) | FIX[(SRC[0] + dx, SRC[1] + dy)]
            if v & 0x3FF not in NOT_ROCK:
                rock[(DST[0] + dx, DST[1] + dy)] = v
    # a sandy islet around the rock (a tile of sand on every side, two under the cave mouth), then
    # its shore
    xs, ys = [x for x, _ in rock], [y for _, y in rock]
    x0, x1, y0, y1 = min(xs) - 1, max(xs) + 1, min(ys) - 1, max(ys) + 2
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            top, bottom, left, right = y == y0, y == y1, x == x0, x == x1
            v = (TOP_LEFT if top and left else TOP_RIGHT if top and right else BOTTOM_LEFT if bottom and left
                 else BOTTOM_RIGHT if bottom and right else TOP if top else BOTTOM if bottom else LEFT if left
                 else RIGHT if right else SAND)
            raw[y * w + x] = v
    for (x, y), v in rock.items():
        raw[y * w + x] = v
    path.write_bytes(struct.pack(f"<{w * h}H", *raw))
    warps = [wp for wp in m["warp_events"] if wp["dest_map"] != MAP_ID]
    warps.append({"x": CAVE_DOOR[0], "y": CAVE_DOOR[1], "elevation": 3, "dest_map": MAP_ID, "dest_warp_id": "0"})
    m["warp_events"] = warps
    for o in m["object_events"]:
        if (o["x"], o["y"]) in MOVE_SWIMMERS:
            o["x"], o["y"] = MOVE_SWIMMERS[(o["x"], o["y"])]
    ROUTE.write_text(json.dumps(m, indent=2) + "\n")
    return len(warps) - 1


def cave(route_warp):
    data = layouts()
    L = {l["id"]: l for l in data["layouts"] if "id" in l}
    src = L["LAYOUT_FIVE_ISLAND_LOST_CAVE_ENTRANCE"]
    d = ROOT / "data/layouts" / NAME
    d.mkdir(exist_ok=True)
    w, h, _, raw = grid(src)
    ladder = (5, 5)
    raw[ladder[1] * w + ladder[0]] = raw[ladder[1] * w + ladder[0] - 1]   # (floor where the ladder was)
    (d / "map.bin").write_bytes(struct.pack(f"<{w * h}H", *raw))
    (d / "border.bin").write_bytes((ROOT / src["border_filepath"]).read_bytes())
    if LAYOUT_ID not in L:
        entry = dict(src, id=LAYOUT_ID, name=NAME.replace("_Frlg", "") + "_Layout",
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
