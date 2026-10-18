#!/usr/bin/env python3
"""Build the inside of the Pallet Town Gym: a big, empty hall ready to be decorated.

Usage: python3 tools/gym_hack/build_pallet_gym.py [preview.png]

Writes data/layouts/PalletTown_Gym_Frlg/ (map.bin, border.bin), registers LAYOUT_PALLET_TOWN_GYM
in data/layouts/layouts.json, and writes the events in data/maps/PalletTown_Gym_Frlg/map.json.
Uses the Viridian Gym tiles (teal floor, League emblem on the back wall).
"""
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
W, H = 25, 28
MAT = (11, 12, 13)          # entrance mat x positions, on row H - 2
STATUES = (10, 14)          # gym statues either side of the entrance
CENTER = W // 2

LAYOUT_DIR = ROOT / "data/layouts/PalletTown_Gym_Frlg"
MAP_DIR = ROOT / "data/maps/PalletTown_Gym_Frlg"

# Viridian Gym metatiles
WALL_TOP, EMBLEM = 0x291, 0x2D3
WALL_FACE = 0x299
FLOOR, FLOOR_SHADOW = 0x281, 0x282
MAT_TILES = (0x2AA, 0x2AB, 0x2AC)
BELOW_MAT = (0x2B2, 0x2B3, 0x2B4)
STATUE_TOP, STATUE_BOTTOM = 0x2A1, 0x2A9
VOID = 0x008

# Where the player stands when Oak brings everyone in, and where Oak and the rival stand
PLAYER_START = (CENTER, H - 3)
OAK_SPOT = (CENTER, H - 5)
RIVAL_SPOT = (CENTER + 1, H - 3)


def block(m, collision, elevation):
    return m | (collision << 10) | (elevation << 12)


def build():
    grid = [[block(FLOOR, 0, 3)] * W for _ in range(H)]
    for x in range(W):
        grid[0][x] = block(EMBLEM if x == CENTER else WALL_TOP, 1, 0)
        grid[1][x] = block(WALL_FACE, 1, 0)
        grid[2][x] = block(FLOOR_SHADOW, 0, 3)
        grid[H - 1][x] = block(VOID, 1, 0)
    for x, m, below in zip(MAT, MAT_TILES, BELOW_MAT):
        grid[H - 2][x] = block(m, 0, 3)
        grid[H - 1][x] = block(below, 1, 0)
    for x in STATUES:
        grid[H - 4][x] = block(STATUE_TOP, 0, 3)
        grid[H - 3][x] = block(STATUE_BOTTOM, 1, 0)
    return grid


def write(grid):
    LAYOUT_DIR.mkdir(exist_ok=True)
    flat = [b for row in grid for b in row]
    (LAYOUT_DIR / "map.bin").write_bytes(struct.pack(f"<{len(flat)}H", *flat))
    (LAYOUT_DIR / "border.bin").write_bytes(struct.pack("<4H", *[block(VOID, 1, 0)] * 4))

    path = ROOT / "data/layouts/layouts.json"
    data = json.loads(path.read_text())
    entry = {
        "id": "LAYOUT_PALLET_TOWN_GYM",
        "name": "PalletTown_Gym_Layout",
        "width": W,
        "height": H,
        "border_width": 2,
        "border_height": 2,
        "primary_tileset": "gTileset_BuildingFrlg",
        "secondary_tileset": "gTileset_ViridianGym",
        "border_filepath": "data/layouts/PalletTown_Gym_Frlg/border.bin",
        "blockdata_filepath": "data/layouts/PalletTown_Gym_Frlg/map.bin",
        "layout_version": "frlg",
    }
    layouts = data["layouts"]
    for i, l in enumerate(layouts):
        if l.get("id") == entry["id"]:
            layouts[i] = entry
            break
    else:
        anchor = next(i for i, l in enumerate(layouts) if l.get("id") == "LAYOUT_PALLET_TOWN")
        layouts.insert(anchor + 1, entry)
    path.write_text(json.dumps(data, indent=2) + "\n")

    MAP_DIR.mkdir(exist_ok=True)

    def person(local_id, gfx, pos, facing):
        return {"local_id": local_id, "type": "object", "graphics_id": gfx, "x": pos[0], "y": pos[1],
                "elevation": 3, "movement_type": f"MOVEMENT_TYPE_FACE_{facing}", "movement_range_x": 1,
                "movement_range_y": 1, "trainer_type": "TRAINER_TYPE_NONE",
                "trainer_sight_or_berry_tree_id": "0", "script": "0x0",
                "flag": "FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL"}

    m = {
        "id": "MAP_PALLET_TOWN_GYM",
        "name": "PalletTown_Gym_Frlg",
        "layout": "LAYOUT_PALLET_TOWN_GYM",
        "music": "MUS_RG_GYM",
        "region": "REGION_KANTO",
        "region_map_section": "MAPSEC_PALLET_TOWN",
        "requires_flash": False,
        "weather": "WEATHER_NONE",
        "map_type": "MAP_TYPE_INDOOR",
        "allow_cycling": False,
        "allow_escaping": False,
        "allow_running": False,
        "show_map_name": False,
        "floor_number": 0,
        "battle_scene": "MAP_BATTLE_SCENE_GYM",
        "connections": None,
        "object_events": [
            person("LOCALID_PALLET_GYM_OAK", "OBJ_EVENT_GFX_PROF_OAK", OAK_SPOT, "DOWN"),
            person("LOCALID_PALLET_GYM_RIVAL", "OBJ_EVENT_GFX_BLUE", RIVAL_SPOT, "UP"),
        ],
        "warp_events": [{"x": x, "y": H - 2, "elevation": 3, "dest_map": "MAP_PALLET_TOWN", "dest_warp_id": "7"}
                        for x in MAT],
        "coord_events": [],
        "bg_events": [{"type": "sign", "x": x, "y": H - 3, "elevation": 0,
                       "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
                       "script": "PalletTown_Gym_EventScript_GymStatue"} for x in STATUES],
    }
    (MAP_DIR / "map.json").write_text(json.dumps(m, indent=2) + "\n")


def preview(grid, out):
    sys.path.insert(0, str(ROOT / "tools/gym_hack"))
    import render_map as rm
    from PIL import Image
    prim = rm.Tileset(rm.tileset_dir("gTileset_BuildingFrlg"))
    sec = rm.Tileset(rm.tileset_dir("gTileset_ViridianGym"))
    img = Image.new("RGB", (W * 16, H * 16))
    for y in range(H):
        for x in range(W):
            rm.draw_metatile(img, x * 16, y * 16, grid[y][x] & 0x3FF, prim, sec)
    img.resize((W * 32, H * 32), Image.NEAREST).save(out)


if __name__ == "__main__":
    g = build()
    write(g)
    if len(sys.argv) > 1:
        preview(g, sys.argv[1])
    print("player start", PLAYER_START, "oak", OAK_SPOT, "rival", RIVAL_SPOT)
