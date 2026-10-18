#!/usr/bin/env python3
"""Build the inside of the Pallet Town Gym: a big, empty hall ready to be decorated.

Usage: python3 tools/gym_hack/build_pallet_gym.py [preview.png]

Writes data/layouts/PalletTown_Gym_Frlg/ (map.bin, border.bin), registers LAYOUT_PALLET_TOWN_GYM
in data/layouts/layouts.json, and writes the events in data/maps/PalletTown_Gym_Frlg/map.json.
Uses the gym's own tileset (made by build_gym_decor.py, which this script runs first): the
Viridian Gym tiles (teal floor, League emblem on the back wall) plus the decorations.
Also writes include/constants/gym_room.h: the room size, where decorations may go, and the PC.
"""
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
import build_gym_decor  # noqa: E402
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

# The gym PC (decoration shop and decorating), left of the entrance
PC = (8, H - 4)          # the block above the PC (plain floor); the PC is below it, used from the front
PC_FRONT = (PC[0], PC[1] + 2)
# Decorations can go anywhere on the floor except the entrance area (statues, mat, PC)
EDIT_AREA = (0, 2, W - 1, H - 3)            # x0, y0, x1, y1
RESERVED = (PC[0] - 1, H - 6, STATUES[1] + 2, H - 1)

# Where the player stands when Oak brings everyone in, and where Oak and the rival stand
PLAYER_START = (CENTER, H - 3)
OAK_SPOT = (CENTER, H - 5)
RIVAL_SPOT = (CENTER + 1, H - 3)
# Oak's aide runs the gym challenges (data/scripts/gym_challenge.inc). Challengers appear on the
# mat and walk to the spot below the aide, where the player stands to battle.
AIDE_SPOT = (15, H - 5)
CHALLENGER_SPOT = (CENTER, H - 2)


def block(m, collision, elevation):
    return m | (collision << 10) | (elevation << 12)


def build():
    _, _, fixed = build_gym_decor.build()
    grid = [[block(FLOOR, 0, 3)] * W for _ in range(H)]
    # The PC is one block tall; the block above it stays plain floor
    grid[PC[1] + 1][PC[0]] = block(fixed["PC_BOTTOM"], 1, 3)
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
        "secondary_tileset": "gTileset_PalletGym",
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

    def person(local_id, gfx, pos, facing, script="0x0", flag="FLAG_HIDE_PALLET_GYM_OAK_AND_RIVAL"):
        return {"local_id": local_id, "type": "object", "graphics_id": gfx, "x": pos[0], "y": pos[1],
                "elevation": 3, "movement_type": f"MOVEMENT_TYPE_FACE_{facing}", "movement_range_x": 1,
                "movement_range_y": 1, "trainer_type": "TRAINER_TYPE_NONE",
                "trainer_sight_or_berry_tree_id": "0", "script": script,
                "flag": flag}

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
            person("LOCALID_PALLET_GYM_AIDE", "OBJ_EVENT_GFX_SCIENTIST", AIDE_SPOT, "DOWN",
                   "GymChallenge_EventScript_Aide", "FLAG_HIDE_PALLET_GYM_AIDE"),
            person("LOCALID_PALLET_GYM_CHALLENGER", "OBJ_EVENT_GFX_VAR_0", CHALLENGER_SPOT, "UP",
                   "0x0", "FLAG_HIDE_PALLET_GYM_CHALLENGER"),
        ],
        "warp_events": [{"x": x, "y": H - 2, "elevation": 3, "dest_map": "MAP_PALLET_TOWN", "dest_warp_id": "7"}
                        for x in MAT],
        "coord_events": [],
        "bg_events": [{"type": "sign", "x": x, "y": H - 3, "elevation": 0,
                       "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
                       "script": "PalletTown_Gym_EventScript_GymStatue"} for x in STATUES] + [
                      {"type": "sign", "x": PC[0], "y": PC[1] + 1, "elevation": 0,
                       "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH",
                       "script": "GymDecor_EventScript_GymPC"}],
    }
    (MAP_DIR / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    (ROOT / "include/constants/gym_room.h").write_text(f"""// Generated by tools/gym_hack/build_pallet_gym.py. Don't edit by hand.
#ifndef GUARD_CONSTANTS_GYM_ROOM_H
#define GUARD_CONSTANTS_GYM_ROOM_H

#define GYM_ROOM_WIDTH   {W}
#define GYM_ROOM_HEIGHT  {H}
// Decorations may go inside this rectangle (inclusive)...
#define GYM_EDIT_X0 {EDIT_AREA[0]}
#define GYM_EDIT_Y0 {EDIT_AREA[1]}
#define GYM_EDIT_X1 {EDIT_AREA[2]}
#define GYM_EDIT_Y1 {EDIT_AREA[3]}
// ...but not inside this one (the entrance, statues and PC)
#define GYM_RESERVED_X0 {RESERVED[0]}
#define GYM_RESERVED_Y0 {RESERVED[1]}
#define GYM_RESERVED_X1 {RESERVED[2]}
#define GYM_RESERVED_Y1 {RESERVED[3]}
// The doormat (on row GYM_ROOM_HEIGHT - 2), kept clear while decorating
#define GYM_MAT_X0 {MAT[0]}
#define GYM_MAT_X1 {MAT[-1]}
// The aide who sends in challengers
#define GYM_AIDE_X {AIDE_SPOT[0]}
#define GYM_AIDE_Y {AIDE_SPOT[1]}
// Where the player is put back when they finish decorating
#define GYM_PC_FRONT_X {PC_FRONT[0]}
#define GYM_PC_FRONT_Y {PC_FRONT[1]}

#endif // GUARD_CONSTANTS_GYM_ROOM_H
""")


def preview(grid, out):
    sys.path.insert(0, str(ROOT / "tools/gym_hack"))
    import render_map as rm
    from PIL import Image
    prim = rm.Tileset(rm.tileset_dir("gTileset_BuildingFrlg"))
    sec = rm.Tileset(rm.tileset_dir("gTileset_PalletGym"))
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
