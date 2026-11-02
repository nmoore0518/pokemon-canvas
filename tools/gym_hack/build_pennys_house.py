#!/usr/bin/env python3
"""Penny's house in Viridian City (Pokémon Canvas): the Eevee fan who tells the player how Eevee
evolves into their gym's type (data/maps/ViridianCity_PennysHouse_Frlg/scripts.inc, src/penny.c).

    python3 tools/gym_hack/build_pennys_house.py

- Viridian City: a copy of the town's house (its 5x4 block at (24, 8)) in the flower garden east of
  the Pokémon Center, door at (35, 26), and a warp there.
- ViridianCity_PennysHouse_Frlg: the Viridian house's layout, with Penny and plushes of Eevee and
  the official eeveelutions (the Pokémon's overworld sprites, standing still).
Re-running is safe (the house, warp and map are rewritten, not added twice).
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CITY = ROOT / "data/maps/ViridianCity_Frlg/map.json"
NAME = "ViridianCity_PennysHouse_Frlg"
MAP_ID = "MAP_VIRIDIAN_CITY_PENNYS_HOUSE"
SRC, DST, SIZE = (24, 8), (34, 23), (5, 4)
DOOR = (35, 26)

PLUSHES = [   # (species, x, y). At most 8: each is its own sprite palette, and a room only has so many
    # (with 9, the last one came out in pieces)
    ("VALYREON", 7, 2),   # (one of the new ones: so the player learns there are more)
    ("EEVEE", 1, 2), ("VAPOREON", 0, 3), ("JOLTEON", 0, 5), ("FLAREON", 2, 6), ("ESPEON", 9, 2),
    ("UMBREON", 10, 3), ("SYLVEON", 10, 5),
]
PENNY = (8, 3)


def city():
    m = json.loads(CITY.read_text())
    lay = {l["id"]: l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if "id" in l}[m["layout"]]
    w, h = lay["width"], lay["height"]
    path = ROOT / lay["blockdata_filepath"]
    raw = list(struct.unpack(f"<{w * h}H", path.read_bytes()))
    for dy in range(SIZE[1]):
        for dx in range(SIZE[0]):
            raw[(DST[1] + dy) * w + DST[0] + dx] = raw[(SRC[1] + dy) * w + SRC[0] + dx]
    # The roof's top row has the street drawn behind it (the town's house stands against the
    # street); here it's against grass, so: the same roof over grass (new metatiles, made once)
    roof = roof_on_grass([raw[DST[1] * w + DST[0] + dx] & 0x3FF for dx in range(SIZE[0])])
    for dx in range(SIZE[0]):
        i = DST[1] * w + DST[0] + dx
        raw[i] = (raw[i] & ~0x3FF) | roof[dx]
    path.write_bytes(struct.pack(f"<{w * h}H", *raw))
    warps = [wp for wp in m["warp_events"] if wp["dest_map"] != MAP_ID]
    warps.append({"x": DOOR[0], "y": DOOR[1], "elevation": 3, "dest_map": MAP_ID, "dest_warp_id": "1"})
    m["warp_events"] = warps
    CITY.write_text(json.dumps(m, indent=2) + "\n")
    return len(warps) - 1


TILESET = ROOT / "data/tilesets/secondary/viridian_city_frlg"
GRASS = 0x11          # (the general tileset's plain grass: its bottom layer goes behind the roof)
SECONDARY_START = 0x280


def roof_on_grass(roof):
    """metatile ids of the roof-top row, each with grass behind it (appended to the tileset once;
    roof ids that are already the grass versions are kept)"""
    mt = bytearray((TILESET / "metatiles.bin").read_bytes())
    attrs = bytearray((TILESET / "metatile_attributes.bin").read_bytes())
    general = (ROOT / "data/tilesets/primary/general_frlg/metatiles.bin").read_bytes()
    grass_bottom = general[GRASS * 16:GRASS * 16 + 8]
    out = []
    for mid in roof:
        local = mid - SECONDARY_START
        entry = grass_bottom + bytes(mt[local * 16 + 8:local * 16 + 16])
        if bytes(mt[local * 16:local * 16 + 8]) == grass_bottom:
            out.append(mid)
            continue
        n = len(mt) // 16
        found = next((k for k in range(n) if bytes(mt[k * 16:k * 16 + 16]) == entry), None)
        if found is None:
            mt += entry
            attrs += attrs[local * 4:local * 4 + 4]
            found = n
        out.append(SECONDARY_START + found)
    (TILESET / "metatiles.bin").write_bytes(bytes(mt))
    (TILESET / "metatile_attributes.bin").write_bytes(bytes(attrs))
    return out


def house(city_warp):
    src = json.loads((ROOT / "data/maps/ViridianCity_House_Frlg/map.json").read_text())
    m = {k: v for k, v in src.items()}
    m["id"], m["name"] = MAP_ID, NAME
    m["warp_events"] = [dict(wp, dest_warp_id=str(city_warp)) for wp in src["warp_events"]]
    objs = [{
        "type": "object", "graphics_id": "OBJ_EVENT_GFX_GAUNTLET_PENNY", "x": PENNY[0], "y": PENNY[1], "elevation": 3,
        "movement_type": "MOVEMENT_TYPE_FACE_DOWN", "movement_range_x": 0, "movement_range_y": 0,
        "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
        "script": "ViridianCity_PennysHouse_EventScript_Penny", "flag": "0"}]
    for sp, x, y in PLUSHES:
        objs.append({
            "type": "object", "graphics_id": f"OBJ_EVENT_GFX_SPECIES({sp})", "x": x, "y": y, "elevation": 3,
            "movement_type": "MOVEMENT_TYPE_FACE_DOWN", "movement_range_x": 0, "movement_range_y": 0,
            "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
            "script": "ViridianCity_PennysHouse_EventScript_Plush" + ("New" if sp == "VALYREON" else ""), "flag": "0"})
    m["object_events"] = objs
    m["bg_events"] = []
    d = ROOT / "data/maps" / NAME
    d.mkdir(exist_ok=True)
    (d / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    # (last in its group, so no other map's number changes: saves keep where they are)
    groups = ROOT / "data/maps/map_groups.json"
    g = json.loads(groups.read_text())
    if NAME not in g["gMapGroup_IndoorViridian_Frlg"]:
        g["gMapGroup_IndoorViridian_Frlg"].append(NAME)
        groups.write_text(json.dumps(g, indent=2) + "\n")
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    inc = f'\t.include "data/maps/{NAME}/scripts.inc"\n'
    if inc not in text:
        anchor = '\t.include "data/maps/ViridianCity_House_Frlg/scripts.inc"\n'
        ev.write_text(text.replace(anchor, anchor + inc, 1))


if __name__ == "__main__":
    house(city())
