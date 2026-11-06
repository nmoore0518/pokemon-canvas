#!/usr/bin/env python3
"""Saffron City's card house (Pokémon Canvas): Grandmaster Rod and the Trading Card Game (src/tcg/).

    python3 tools/gym_hack/build_tcg_house.py

- Saffron City: the doorless house left of the Copycat's (the top row's first) gets the Copycat
  house's door (its metatiles, copied over the same columns), at DOOR, and a warp in.
- SaffronCity_TcgHouse_Frlg: a new map on the Saffron house layout (LAYOUT_HOUSE5_FRLG), its door
  back out and Rod; into the map groups (last in Saffron's indoor group: no other map's number
  changes) and event_scripts.s. Its scripts.inc is written once (Nick's dialogue goes there) and
  never overwritten.
Re-running is safe.
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CITY = ROOT / "data/maps/SaffronCity_Frlg/map.json"
NAME, MAP_ID = "SaffronCity_TcgHouse_Frlg", "MAP_SAFFRON_CITY_TCG_HOUSE"
COPYCAT_X, HOUSE_X = 21, 17          # (each house's left column; the door's one to the right)
ROWS = (13, 14, 15)                  # (the house's bottom two rows and its doorstep)
DOOR = (HOUSE_X + 1, 14)
ROD = (7, 4)          # (in the card table's right chair)


def layouts():
    return {l["id"]: l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if "id" in l}


def city():
    m = json.loads(CITY.read_text())
    lay = layouts()[m["layout"]]
    w, h = lay["width"], lay["height"]
    path = ROOT / lay["blockdata_filepath"]
    raw = list(struct.unpack(f"<{w * h}H", path.read_bytes()))
    for y in ROWS:
        for dx in range(3):
            raw[y * w + HOUSE_X + dx] = raw[y * w + COPYCAT_X + dx]
    path.write_bytes(struct.pack(f"<{w * h}H", *raw))
    copycat = next(wp for wp in m["warp_events"] if wp["dest_map"] == "MAP_SAFFRON_CITY_COPYCATS_HOUSE_1F")
    warps = [wp for wp in m["warp_events"] if wp["dest_map"] != MAP_ID]
    warps.append({"x": DOOR[0], "y": DOOR[1], "elevation": copycat["elevation"], "dest_map": MAP_ID,
                  "dest_warp_id": "0"})
    m["warp_events"] = warps
    CITY.write_text(json.dumps(m, indent=2) + "\n")
    return len(warps) - 1


def house(city_warp):
    d = ROOT / "data/maps" / NAME
    d.mkdir(exist_ok=True)
    m = json.loads((ROOT / "data/maps/SaffronCity_House_Frlg/map.json").read_text())
    m["id"], m["name"] = MAP_ID, NAME
    m["warp_events"] = [{"x": x, "y": 7, "elevation": 3, "dest_map": "MAP_SAFFRON_CITY", "dest_warp_id": str(city_warp)}
                        for x in (3, 4, 5)]
    m["object_events"] = [
        {"type": "object", "graphics_id": "OBJ_EVENT_GFX_GENTLEMAN_FRLG", "x": ROD[0], "y": ROD[1], "elevation": 3,
         "movement_type": "MOVEMENT_TYPE_FACE_LEFT", "movement_range_x": 0, "movement_range_y": 0,
         "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
         "script": "SaffronCity_TcgHouse_EventScript_Rod", "flag": "0"},
    ]
    m["bg_events"] = []
    m["coord_events"] = []
    (d / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    groups = ROOT / "data/maps/map_groups.json"
    g = json.loads(groups.read_text())
    if NAME not in g["gMapGroup_IndoorSaffron_Frlg"]:
        g["gMapGroup_IndoorSaffron_Frlg"].append(NAME)
        groups.write_text(json.dumps(g, indent=2) + "\n")
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    inc = f'\t.include "data/maps/{NAME}/scripts.inc"\n'
    if inc not in text:
        anchor = '\t.include "data/maps/SaffronCity_House_Frlg/scripts.inc"\n'
        assert anchor in text
        ev.write_text(text.replace(anchor, anchor + inc, 1))


if __name__ == "__main__":
    house(city())
