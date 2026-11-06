#!/usr/bin/env python3
"""The Pallet Town Gym's extra rooms (Pokémon Canvas): three smaller rooms behind doorways in the
gym's back wall and side walls, given by Oak for catching 100, 200 and 300 kinds of Pokémon
(src/gym_room.c, src/gym_decor.c, data/scripts/gym_rooms.inc).

    python3 tools/gym_hack/build_gym_rooms.py

Each room is its own map, decorated like the gym (with its own PC, floors and wall style; the
decorations come from the gym's storage). The rooms' layout (where the walls, the PC and the way
out are) is here; tools/gym_hack/build_gym_decor.py reads it for the game's room table.

Writes:
- data/layouts/PalletTown_Gym_<Room>Room/ (map.bin, border.bin) and their layouts.json entries
- data/maps/PalletTown_Gym_<Room>Room/ (map.json; scripts.inc the first time) and map_groups.json
- the gym's map.json: the warps through its three doorways (warps 3-5)
Re-running is safe.
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# The gym (build_pallet_gym.py: 25 x 28) and its doorways: (x, y, the square in front, the way through)
GYM_DOORS = {
    "BACK":  ((21, 1), (21, 2), "NORTH"),     # (off to the side: the middle of the back wall is for decorating)
    "LEFT":  ((0, 13), (1, 13), "WEST"),
    "RIGHT": ((24, 13), (23, 13), "EAST"),
}
GYM_FIRST_DOOR_WARP = 3          # (the gym's warps 0-2 are its doormat)

# The rooms: 17 x 15 (the back wall on rows 0-1, side walls, no wall along the bottom)
W, H = 17, 15
ROOMS = {
    # key: map name, the way out (a doormat at the bottom, or a doorway in a side wall), the PC
    "BACK": dict(name="PalletTown_Gym_BackRoom", id="MAP_PALLET_TOWN_GYM_BACK_ROOM",
                 mat=(7, 8, 9), door=None, pc=(5, H - 3), reserved=(4, H - 4, 11, H - 1)),
    "LEFT": dict(name="PalletTown_Gym_LeftRoom", id="MAP_PALLET_TOWN_GYM_LEFT_ROOM",
                 mat=None, door=((W - 1, 7), (W - 2, 7), "EAST"), pc=(W - 2, 6), reserved=(W - 3, 6, W - 2, 9)),
    "RIGHT": dict(name="PalletTown_Gym_RightRoom", id="MAP_PALLET_TOWN_GYM_RIGHT_ROOM",
                  mat=None, door=((0, 7), (1, 7), "WEST"), pc=(1, 6), reserved=(1, 6, 2, 9)),
}
ORDER = ("BACK", "LEFT", "RIGHT")      # (rooms 1-3: GYM_ROOM_BACK...)
FLAGS = {"BACK": "FLAG_GYM_ROOM_BACK", "LEFT": "FLAG_GYM_ROOM_LEFT", "RIGHT": "FLAG_GYM_ROOM_RIGHT"}
EDIT_AREA = (1, 2, W - 2, H - 2)       # x0, y0, x1, y1

# Viridian Gym metatiles (as in build_pallet_gym.py)
WALL_TOP, WALL_FACE = 0x291, 0x299
FLOOR, FLOOR_SHADOW = 0x281, 0x282
MAT_TILES = (0x2AA, 0x2AB, 0x2AC)
BELOW_MAT = (0x2B2, 0x2B3, 0x2B4)
VOID = 0x008


def block(m, collision, elevation):
    return m | (collision << 10) | (elevation << 12)


def pc_front(room):
    x, y = ROOMS[room]["pc"]
    return (x, y + 1)


def grid(room, pc_bottom):
    r = ROOMS[room]
    g = [[block(FLOOR, 0, 3)] * W for _ in range(H)]
    for x in range(W):
        g[0][x] = block(WALL_TOP, 1, 0)
        g[1][x] = block(WALL_FACE, 1, 0)
        g[2][x] = block(FLOOR_SHADOW, 0, 3)
        g[H - 1][x] = block(VOID, 1, 0)
    for y in range(1, H - 1):
        g[y][0] = g[y][W - 1] = block(WALL_TOP, 1, 0)
    if r["mat"]:
        for x, m, below in zip(r["mat"], MAT_TILES, BELOW_MAT):
            g[H - 2][x] = block(m, 0, 3)
            g[H - 1][x] = block(below, 1, 0)
    if r["door"]:
        (dx, dy), _, _ = r["door"]
        g[dy][dx] = block(FLOOR, 0, 3)
    px, py = r["pc"]
    g[py][px] = block(pc_bottom, 1, 3)
    return g


def write_layouts(pc_bottom):
    path = ROOT / "data/layouts/layouts.json"
    data = json.loads(path.read_text())
    layouts = data["layouts"]
    for room in ORDER:
        r = ROOMS[room]
        d = ROOT / "data/layouts" / r["name"]
        d.mkdir(exist_ok=True)
        flat = [b for row in grid(room, pc_bottom) for b in row]
        (d / "map.bin").write_bytes(struct.pack(f"<{len(flat)}H", *flat))
        (d / "border.bin").write_bytes(struct.pack("<4H", *[block(VOID, 1, 0)] * 4))
        lid = "LAYOUT_" + r["id"][4:]
        entry = {
            "id": lid, "name": r["name"] + "_Layout", "width": W, "height": H,
            "border_width": 2, "border_height": 2,
            "primary_tileset": "gTileset_BuildingFrlg", "secondary_tileset": "gTileset_PalletGym",
            "border_filepath": f"data/layouts/{r['name']}/border.bin",
            "blockdata_filepath": f"data/layouts/{r['name']}/map.bin",
            "layout_version": "frlg",
        }
        for i, l in enumerate(layouts):
            if l.get("id") == lid:
                layouts[i] = entry
                break
        else:
            anchor = next(i for i, l in enumerate(layouts) if l.get("id") == "LAYOUT_PALLET_TOWN_GYM")
            layouts.insert(anchor + 1 + ORDER.index(room), entry)
    path.write_text(json.dumps(data, indent=2) + "\n")


def write_maps():
    groups_path = ROOT / "data/maps/map_groups.json"
    groups = json.loads(groups_path.read_text())
    group = groups["gMapGroup_IndoorPallet_Frlg"]
    for k, room in enumerate(ORDER):
        r = ROOMS[room]
        if r["name"] not in group:
            group.append(r["name"])       # (at the end: saves keep their maps' numbers)
        d = ROOT / "data/maps" / r["name"]
        d.mkdir(exist_ok=True)
        dest = GYM_FIRST_DOOR_WARP + k
        if r["mat"]:
            warps = [{"x": x, "y": H - 2, "elevation": 3, "dest_map": "MAP_PALLET_TOWN_GYM", "dest_warp_id": str(dest)}
                     for x in r["mat"]]
        else:
            (x, y), _, _ = r["door"]
            warps = [{"x": x, "y": y, "elevation": 3, "dest_map": "MAP_PALLET_TOWN_GYM", "dest_warp_id": str(dest)}]
        px, py = r["pc"]
        m = {
            "id": r["id"], "name": r["name"], "layout": "LAYOUT_" + r["id"][4:],
            "music": "MUS_RG_GYM", "region": "REGION_KANTO", "region_map_section": "MAPSEC_PALLET_TOWN",
            "requires_flash": False, "weather": "WEATHER_NONE", "map_type": "MAP_TYPE_INDOOR",
            "allow_cycling": False, "allow_escaping": False, "allow_running": False, "show_map_name": False,
            "floor_number": 0, "battle_scene": "MAP_BATTLE_SCENE_GYM", "connections": None,
            "object_events": [],
            "warp_events": warps,
            "coord_events": [],
            "bg_events": [{"type": "sign", "x": px, "y": py, "elevation": 0,
                           "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH",
                           "script": "GymDecor_EventScript_GymPC"}],
        }
        (d / "map.json").write_text(json.dumps(m, indent=2) + "\n")
        scripts = d / "scripts.inc"
        if not scripts.exists():
            scripts.write_text(f"{r['name']}_MapScripts::\n"
                               "\tmap_script MAP_SCRIPT_ON_LOAD, GymRooms_EventScript_OnLoad\n"
                               "\t.byte 0\n")
    groups_path.write_text(json.dumps(groups, indent=2) + "\n")

    # The gym's doorways
    path = ROOT / "data/maps/PalletTown_Gym_Frlg/map.json"
    gym = json.loads(path.read_text())
    gym["warp_events"] = gym["warp_events"][:GYM_FIRST_DOOR_WARP]
    for room in ORDER:
        (x, y), _, _ = GYM_DOORS[room]
        r = ROOMS[room]
        dest = 1 if r["mat"] else 0          # (the middle of a doormat)
        gym["warp_events"].append({"x": x, "y": y, "elevation": 3, "dest_map": r["id"], "dest_warp_id": str(dest)})
    path.write_text(json.dumps(gym, indent=2) + "\n")

    # Their scripts
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    anchor = '\t.include "data/maps/PalletTown_Gym_Frlg/scripts.inc"\n'
    add = "".join(f'\t.include "data/maps/{ROOMS[r]["name"]}/scripts.inc"\n' for r in ORDER)
    if add not in text:
        assert anchor in text
        text = text.replace(anchor, anchor + add, 1)
    inc = '\t.include "data/scripts/gym_rooms.inc"\n'
    dec = '\t.include "data/scripts/gym_decor.inc"\n'
    if inc not in text:
        assert dec in text
        text = text.replace(dec, dec + inc, 1)
    ev.write_text(text)


if __name__ == "__main__":
    import sys
    sys.path.insert(0, str(ROOT / "tools/gym_hack"))
    import build_gym_decor
    _, _, fixed = build_gym_decor.build()
    write_layouts(fixed["PC_BOTTOM"])
    write_maps()
