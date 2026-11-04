#!/usr/bin/env python3
"""The painter's loft in Pewter City (Pokémon Canvas; src/painter.c, data/scripts/painter.inc).

    python3 tools/gym_hack/build_painter_loft.py

- Pewter City: a house (a copy of its own house by the museum) in the garden north-west of the
  museum, its door at DOOR; the garden's light grass that's left becomes plain grass, and the
  hidden Poké Ball there moves aside.
- PewterCity_PainterLoft_Frlg: FireRed's little house layout with the rug and table taken up, the
  painter, her Smeargle, her easel (the way into her paintings), canvases, paint on the floor.
  Into the map groups (last in Pewter's indoor group: no other map's number changes) and
  event_scripts.s.
Re-running is safe.
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CITY = ROOT / "data/maps/PewterCity_Frlg/map.json"
NAME, MAP_ID, LAYOUT_ID = "PewterCity_PainterLoft_Frlg", "MAP_PEWTER_CITY_PAINTER_LOFT", "LAYOUT_PEWTER_CITY_PAINTER_LOFT"
SRC, HOUSE, SIZE = (32, 8), (6, 2), (5, 4)      # (the house east of the museum)
DOOR = (7, 5)
GRASS = 0x3001
OLD_PATCH = [(x, y) for x in (4, 5) for y in (2, 3, 4)]
MOVE_HIDDEN = {(6, 3): (4, 3)}
FLOOR = 0x3001
RUG = [(x, y) for x in range(3, 9) for y in range(3, 7)]

OBJECTS = [
    # (graphics, x, y, movement, script, flag)
    ("OBJ_EVENT_GFX_PAINTER", 5, 3, "MOVEMENT_TYPE_FACE_DOWN", "PainterLoft_EventScript_Painter", "0"),
    ("OBJ_EVENT_GFX_SPECIES(SMEARGLE_CANVAS)", 6, 3, "MOVEMENT_TYPE_FACE_DOWN", "PainterLoft_EventScript_Smeargle", "FLAG_PAINTER_GAVE_SMEARGLE"),
    ("OBJ_EVENT_GFX_PAINTER_EASEL", 8, 3, "MOVEMENT_TYPE_FACE_DOWN", "PainterLoft_EventScript_Easel", "0"),
    ("OBJ_EVENT_GFX_PAINTER_CANVASES", 1, 4, "MOVEMENT_TYPE_FACE_DOWN", "PainterLoft_EventScript_Canvases", "0"),
    ("OBJ_EVENT_GFX_PAINT_SPLOTCH_RED", 3, 5, "MOVEMENT_TYPE_FACE_DOWN", "PainterLoft_EventScript_Paint", "0"),
    ("OBJ_EVENT_GFX_PAINT_SPLOTCH_BLUE", 7, 6, "MOVEMENT_TYPE_FACE_DOWN", "PainterLoft_EventScript_Paint", "0"),
    ("OBJ_EVENT_GFX_PAINT_SPLOTCH_YELLOW", 9, 5, "MOVEMENT_TYPE_FACE_DOWN", "PainterLoft_EventScript_Paint", "0"),
    ("OBJ_EVENT_GFX_PAINT_POT", 1, 6, "MOVEMENT_TYPE_FACE_DOWN", "PainterLoft_EventScript_PaintPot", "0"),
]


def layouts():
    return json.loads((ROOT / "data/layouts/layouts.json").read_text())


def grid(layout):
    w, h = layout["width"], layout["height"]
    path = ROOT / layout["blockdata_filepath"]
    return w, h, path, list(struct.unpack(f"<{w * h}H", path.read_bytes()))


def city():
    L = {l["id"]: l for l in layouts()["layouts"] if "id" in l}
    m = json.loads(CITY.read_text())
    w, h, path, raw = grid(L[m["layout"]])
    if not any(wp["dest_map"] == MAP_ID for wp in m["warp_events"]):
        block = [[raw[(SRC[1] + dy) * w + SRC[0] + dx] for dx in range(SIZE[0])] for dy in range(SIZE[1])]
        for dy in range(SIZE[1]):
            for dx in range(SIZE[0]):
                raw[(HOUSE[1] + dy) * w + HOUSE[0] + dx] = block[dy][dx]
        for x, y in OLD_PATCH:
            raw[y * w + x] = GRASS
        path.write_bytes(struct.pack(f"<{w * h}H", *raw))
        m["warp_events"].append({"x": DOOR[0], "y": DOOR[1], "elevation": 0, "dest_map": MAP_ID, "dest_warp_id": "0"})
    for bg in m["bg_events"]:
        if bg.get("type") == "hidden_item" and (bg["x"], bg["y"]) in MOVE_HIDDEN:
            bg["x"], bg["y"] = MOVE_HIDDEN[(bg["x"], bg["y"])]
    CITY.write_text(json.dumps(m, indent=2) + "\n")
    return next(i for i, wp in enumerate(m["warp_events"]) if wp["dest_map"] == MAP_ID)


def loft(city_warp):
    data = layouts()
    L = {l["id"]: l for l in data["layouts"] if "id" in l}
    src = L["LAYOUT_HOUSE2_FRLG"]
    w, h, _, raw = grid(src)
    for x, y in RUG:
        raw[y * w + x] = FLOOR
    d = ROOT / "data/layouts" / NAME
    d.mkdir(exist_ok=True)
    (d / "map.bin").write_bytes(struct.pack(f"<{w * h}H", *raw))
    (d / "border.bin").write_bytes((ROOT / src["border_filepath"]).read_bytes())
    data["layouts"] = [l for l in data["layouts"] if l.get("id") != LAYOUT_ID]
    data["layouts"].append(dict(src, id=LAYOUT_ID, name=NAME.replace("_Frlg", "") + "_Layout",
                                border_filepath=f"data/layouts/{NAME}/border.bin",
                                blockdata_filepath=f"data/layouts/{NAME}/map.bin"))
    (ROOT / "data/layouts/layouts.json").write_text(json.dumps(data, indent=2) + "\n")
    m = {
        "id": MAP_ID, "name": NAME, "layout": LAYOUT_ID, "music": "MUS_RG_PEWTER", "region": "REGION_KANTO",
        "region_map_section": "MAPSEC_PEWTER_CITY", "requires_flash": False, "weather": "WEATHER_NONE",
        "map_type": "MAP_TYPE_INDOOR", "allow_cycling": False, "allow_escaping": False, "allow_running": True,
        "show_map_name": False, "floor_number": 0, "battle_scene": "MAP_BATTLE_SCENE_NORMAL", "connections": None,
        "object_events": [
            {"type": "object", "graphics_id": g, "x": x, "y": y, "elevation": 3, "movement_type": mv,
             "movement_range_x": 0, "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
             "trainer_sight_or_berry_tree_id": "0", "script": scr, "flag": flag}
            for g, x, y, mv, scr, flag in OBJECTS],
        "warp_events": [{"x": x, "y": 7, "elevation": 0, "dest_map": "MAP_PEWTER_CITY", "dest_warp_id": str(city_warp)}
                        for x in (3, 4, 5)],
        "coord_events": [], "bg_events": [],
    }
    md = ROOT / "data/maps" / NAME
    md.mkdir(exist_ok=True)
    (md / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    groups = ROOT / "data/maps/map_groups.json"
    g = json.loads(groups.read_text())
    if NAME not in g["gMapGroup_IndoorPewter_Frlg"]:
        g["gMapGroup_IndoorPewter_Frlg"].append(NAME)
        groups.write_text(json.dumps(g, indent=2) + "\n")
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    inc = f'\t.include "data/maps/{NAME}/scripts.inc"\n'
    if inc not in text:
        anchor = '\t.include "data/maps/PewterCity_House2_Frlg/scripts.inc"\n'
        assert anchor in text
        ev.write_text(text.replace(anchor, anchor + inc, 1))


if __name__ == "__main__":
    loft(city())
    print("the loft's door in Pewter City at", DOOR)
