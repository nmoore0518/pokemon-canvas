#!/usr/bin/env python3
"""Route 19's beach house (Pokémon Canvas): Pikachu's Beach, as in Yellow. FireRed kept the house's
map (Route19_UnusedHouse_Frlg) but gave it no door; this gives it one.

    python3 tools/gym_hack/build_beach_house.py

- Route 19: one of Fuchsia City's houses (same tileset), on the beach at HOUSE, its door at DOOR;
  its roof's top row over sand (new Fuchsia metatiles: the roof, with the sand's bottom layer).
  The swimmer and the sign that were there move aside.
- Route19_UnusedHouse_Frlg: its door back out, the Surfin' Dude and his Pikachu; into the map
  groups (last in Fuchsia's indoor group: no other map's number changes) and event_scripts.s.
Re-running is safe.
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ROUTE = ROOT / "data/maps/Route19_Frlg/map.json"
NAME, MAP_ID = "Route19_UnusedHouse_Frlg", "MAP_ROUTE19_UNUSED_HOUSE"
SRC, HOUSE, SIZE = (32, 28), (10, 9), (5, 4)     # (Fuchsia City's house by the Warden's)
DOOR = (11, 12)
SAND = 0x115
MOVE = {(10, 9): (8, 10)}                          # (a swimmer standing where the house goes)
MOVE_SIGN = {(13, 12): (15, 12)}
TILESET = ROOT / "data/tilesets/secondary/fuchsia_city_frlg"
SECONDARY_START = 0x280
DUDE, PIKACHU = (8, 3), (9, 3)


def layouts():
    return {l["id"]: l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if "id" in l}


def grid(layout):
    w, h = layout["width"], layout["height"]
    path = ROOT / layout["blockdata_filepath"]
    return w, h, path, list(struct.unpack(f"<{w * h}H", path.read_bytes()))


def over_sand(mid):
    """the same metatile with the sand's bottom layer (added to the tileset once)"""
    mt = bytearray((TILESET / "metatiles.bin").read_bytes())
    attrs = bytearray((TILESET / "metatile_attributes.bin").read_bytes())
    general = (ROOT / "data/tilesets/primary/general_frlg/metatiles.bin").read_bytes()
    bottom = general[SAND * 16:SAND * 16 + 8]
    local = mid - SECONDARY_START
    entry = bottom + bytes(mt[local * 16 + 8:local * 16 + 16])
    n = len(mt) // 16
    found = next((k for k in range(n) if bytes(mt[k * 16:k * 16 + 16]) == entry), None)
    if found is None:
        mt += entry
        attrs += attrs[local * 4:local * 4 + 4]
        found = n
        (TILESET / "metatiles.bin").write_bytes(bytes(mt))
        (TILESET / "metatile_attributes.bin").write_bytes(bytes(attrs))
    return SECONDARY_START + found


def route():
    L = layouts()
    m = json.loads(ROUTE.read_text())
    fw, _, _, src = grid(L[json.loads((ROOT / "data/maps/FuchsiaCity_Frlg/map.json").read_text())["layout"]])
    w, h, path, raw = grid(L[m["layout"]])
    for (ox, oy), (nx, ny) in MOVE_SIGN.items():       # (the sign's own metatile goes with it)
        if raw[ny * w + nx] & 0x3FF == SAND:
            raw[ny * w + nx] = raw[oy * w + ox]
    for dy in range(SIZE[1]):
        for dx in range(SIZE[0]):
            v = src[(SRC[1] + dy) * fw + SRC[0] + dx]
            if dy == 0:
                v = (v & ~0x3FF) | over_sand(v & 0x3FF)
            raw[(HOUSE[1] + dy) * w + HOUSE[0] + dx] = v
    path.write_bytes(struct.pack(f"<{w * h}H", *raw))
    for o in m["object_events"]:
        if (o["x"], o["y"]) in MOVE:
            o["x"], o["y"] = MOVE[(o["x"], o["y"])]
    for b in m["bg_events"]:
        if (b["x"], b["y"]) in MOVE_SIGN:
            b["x"], b["y"] = MOVE_SIGN[(b["x"], b["y"])]
    warps = [wp for wp in m["warp_events"] if wp["dest_map"] != MAP_ID]
    warps.append({"x": DOOR[0], "y": DOOR[1], "elevation": 3, "dest_map": MAP_ID, "dest_warp_id": "0"})
    m["warp_events"] = warps
    ROUTE.write_text(json.dumps(m, indent=2) + "\n")
    return len(warps) - 1


def house(route_warp):
    p = ROOT / "data/maps" / NAME / "map.json"
    m = json.loads(p.read_text())
    m["music"] = "MUS_RG_FUCHSIA"
    m["warp_events"] = [{"x": x, "y": 7, "elevation": 0, "dest_map": "MAP_ROUTE19", "dest_warp_id": str(route_warp)}
                        for x in (3, 4, 5)]
    m["object_events"] = [
        {"type": "object", "graphics_id": "OBJ_EVENT_GFX_SWIMMER_M_LAND", "x": DUDE[0], "y": DUDE[1], "elevation": 3,
         "movement_type": "MOVEMENT_TYPE_FACE_DOWN", "movement_range_x": 0, "movement_range_y": 0,
         "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
         "script": "Route19_BeachHouse_EventScript_SurfinDude", "flag": "0"},
        {"type": "object", "graphics_id": "OBJ_EVENT_GFX_SPECIES(PIKACHU)", "x": PIKACHU[0], "y": PIKACHU[1],
         "elevation": 3, "movement_type": "MOVEMENT_TYPE_LOOK_AROUND", "movement_range_x": 0, "movement_range_y": 0,
         "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
         "script": "Route19_BeachHouse_EventScript_Pikachu", "flag": "0"},
    ]
    p.write_text(json.dumps(m, indent=2) + "\n")
    groups = ROOT / "data/maps/map_groups.json"
    g = json.loads(groups.read_text())
    if NAME not in g["gMapGroup_IndoorFuchsia_Frlg"]:
        g["gMapGroup_IndoorFuchsia_Frlg"].append(NAME)
        groups.write_text(json.dumps(g, indent=2) + "\n")
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    inc = f'\t.include "data/maps/{NAME}/scripts.inc"\n'
    if inc not in text:
        anchor = '\t.include "data/maps/Route19_Frlg/scripts.inc"\n'
        assert anchor in text
        ev.write_text(text.replace(anchor, anchor + inc, 1))


if __name__ == "__main__":
    house(route())
