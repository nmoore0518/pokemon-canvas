#!/usr/bin/env python3
"""The basement under the player's house (Pokémon Canvas): Dad's workshop.

A concrete room down a stairwell from the house's 1F: the Ability Synthesizer (a belt carrying a
capsule into a glowing stream), the Ball Maker (a hopper and a hand crank), the Ball Changer (two
pedestals and a console; the balls on them are people, so they can jump across), a wall switch
that appears later in the story, four loose pages, a workbench, a lab table, a desk, a bookshelf,
a chalkboard, crates and a trash can. Art: tools/gym_hack/basement_art.py.

    python3 tools/gym_hack/build_basement.py [preview.png]

Writes:
  data/tilesets/secondary/players_basement_frlg/ (+ its C declarations)   gTileset_PlayersBasement
  data/layouts/PalletTown_PlayersHouse_B1F_Frlg/, layouts.json
  data/maps/PalletTown_PlayersHouse_B1F_Frlg/map.json, frames.inc (the machines' animation frames:
      scripts that set their blocks); scripts.inc is written once and then kept
  data/maps/map_groups.json      (the basement at the end of gMapGroup_IndoorPallet_Frlg: saves stay valid)
  the house's 1F: the stairwell down (x 7-9, rows 2-3: the 2F's, FireRed's own blocks), the rug,
  table and Mom one row lower, its warp
"""
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))
import basement_art as art                                  # noqa: E402
from build_exotic_safari import MB, obj, write_layout, write_map   # noqa: E402
from tileset_composer import LAYER_COVERED, NPRIM, Composer       # noqa: E402

NAME = "PalletTown_PlayersHouse_B1F_Frlg"
MAP_ID = "MAP_PALLET_TOWN_PLAYERS_HOUSE_B1F"
LAYOUT_ID = "LAYOUT_PALLET_TOWN_PLAYERS_HOUSE_B1F"
HOUSE = "PalletTown_PlayersHouse_1F_Frlg"
HOUSE_MAP = "MAP_PALLET_TOWN_PLAYERS_HOUSE_1F"
PRIMARY = "gTileset_BuildingFrlg"
BLACK = 0x008                     # (the building tileset's black, outside the rooms: the house's border)

# The stairwell down on the house's 1F (the 2F's, x 8-10 rows 2-3: FireRed's own blocks), its top
# step the warp (dx 2, dy 0: stand on it and walk left)
STAIRS_DOWN = {(0, 0): (0x005, 1), (1, 0): (0x006, 1), (2, 0): (0x007, 0),
               (0, 1): (0x00D, 1), (1, 1): (0x00E, 1), (2, 1): (0x00F, 0)}
HOUSE_STAIRWELL = (7, 2)          # on the 1F: its top-left; the warp at (9, 2)
HOUSE_RUG = (4, 3, 9, 6)          # the 1F's rug, table and chairs (x0, y0, x1, y1): moved one row down

FLAG_SWITCH_SHOWN = "FLAG_BASEMENT_SWITCH_SHOWN"
FLAG_SWITCH_ON = "FLAG_BASEMENT_SWITCH_ON"


def blk(m, coll, elev=None):
    if elev is None:
        elev = 0 if coll else 3
    return m | (coll << 10) | (elev << 12)


def solid_blocks():
    """the blocks the player can't walk on"""
    s = set()
    for x in range(art.W):
        s.add((x, 0))
        s.add((x, 1))
    sx, sy = art.SYNTH
    s |= {(sx + dx, sy + dy) for dx in range(4) for dy in range(3)}
    mx, my = art.MAKER
    s |= {(mx + dx, my + dy) for dx in range(3) for dy in range(3)}
    bx, by = art.BENCH
    s |= {(bx + dx, by + dy) for dx in range(2) for dy in range(2)}
    cx, cy = art.CHANGER
    s |= {(cx, cy), (cx, cy + 1), (cx + 3, cy), (cx + 3, cy + 1), (cx + 1, cy + 1), (cx + 2, cy + 1)}
    lx, ly = art.LABTABLE
    s |= {(lx + dx, ly + dy) for dx in range(2) for dy in range(2)}
    dx_, dy_ = art.DESK
    s |= {(dx_ + dx, dy_ + dy) for dx in range(2) for dy in range(2)}
    s.add(art.TRASH)
    sx, sy = art.STAIRS
    s |= {(sx + 1 + dx, y) for dx in range(2) for y in range(sy + 2)}
    s |= set(art.CRATES)
    return s


class Cutter:
    """cuts scenes into metatiles: the room layer at the bottom (palette ROOM), the machines over it
    (palette MACH); equal blocks share a metatile"""
    def __init__(self):
        self.c = Composer(PRIMARY)
        self.room_slot = self.c.add_palette(art.ROOM)
        self.mach_slot = self.c.add_palette(art.MACH)
        self.made = {}

    def tile(self, img, x, y, slot, blank_ok):
        px = tuple(img.crop((x, y, x + 8, y + 8)).getdata())
        if blank_ok and not any(px):
            return 0
        t, f = self.c.add_tile(px)
        return (NPRIM + t) | f | (slot << 12)

    def block(self, sc, bx, by, behavior="MB_NORMAL"):
        bottom, top = [], []
        for ty in range(2):
            for tx in range(2):
                x, y = bx * 16 + tx * 8, by * 16 + ty * 8
                bottom.append(self.tile(sc.room, x, y, self.room_slot, False))
                top.append(self.tile(sc.mach, x, y, self.mach_slot, True))
        attr = MB[behavior] | (LAYER_COVERED << 29)
        key = (tuple(bottom + top), attr)
        if key not in self.made:
            self.made[key] = self.c.add_metatile(bottom + top, attr)
        return self.made[key]


def stairs_warp():
    return (art.STAIRS[0], art.STAIRS[1] + 1)


def build():
    cut = Cutter()
    solid = solid_blocks()
    base = art.scene()
    grid = [[0] * art.W for _ in range(art.H)]
    for by in range(art.H):
        for bx in range(art.W):
            beh = "MB_UP_RIGHT_STAIR_WARP" if (bx, by) == stairs_warp() else "MB_NORMAL"
            grid[by][bx] = blk(cut.block(base, bx, by, beh), 1 if (bx, by) in solid else 0)
    # the frames: every block a variant changes, by name (synth0-2, maker0-3 + maker_ball, changer0-1, switch_*)
    variants = {f"synth{i}": art.scene(synth=i) for i in range(3)}
    variants.update({f"maker{i}": art.scene(maker=i) for i in range(4)})
    variants["maker_ball"] = art.scene(ball=True)
    variants.update({f"changer{i}": art.scene(changer=i) for i in range(2)})
    variants.update({"switch_hidden": art.scene(), "switch_off": art.scene(switch="off"),
                     "switch_on": art.scene(switch="on")})
    groups = {"synth": [f"synth{i}" for i in range(3)], "maker": [f"maker{i}" for i in range(4)] + ["maker_ball"],
              "changer": ["changer0", "changer1"], "switch": ["switch_hidden", "switch_off", "switch_on"]}
    frames = {}
    for g, names in groups.items():
        changed = set()
        for n in names:
            v = variants[n]
            for by in range(art.H):
                for bx in range(art.W):
                    box = (bx * 16, by * 16, bx * 16 + 16, by * 16 + 16)
                    if v.room.crop(box).tobytes() != base.room.crop(box).tobytes() or \
                       v.mach.crop(box).tobytes() != base.mach.crop(box).tobytes():
                        changed.add((bx, by))
        for n in names:
            frames[n] = [(bx, by, cut.block(variants[n], bx, by), (bx, by) in solid) for (bx, by) in sorted(changed)]
    cut.c.write("PlayersBasement", "players_basement_frlg")
    return grid, frames


def write_frames(frames):
    out = ["@ The basement's animation frames (tools/gym_hack/build_basement.py: don't edit)", ""]
    for n, blocks in frames.items():
        label = "PlayersHouse_B1F_EventScript_Frame_" + n
        out.append(f"{label}::")
        for bx, by, m, solid in blocks:
            out.append(f"\tsetmetatile {bx}, {by}, 0x{m:03X}, {'TRUE' if solid else 'FALSE'}")
        out.append("\treturn")
        out.append("")
    (ROOT / "data/maps" / NAME / "frames.inc").write_text("\n".join(out))


def sign(x, y, script, facing="BG_EVENT_PLAYER_FACING_ANY"):
    return {"type": "sign", "x": x, "y": y, "elevation": 0, "player_facing_dir": facing, "script": script}


def write_basement_map(grid):
    write_layout(NAME, LAYOUT_ID, grid, PRIMARY, "gTileset_PlayersBasement", border=BLACK)
    sx, sy = art.STAIRS
    warps = [{"x": sx, "y": sy + 1, "elevation": 3, "dest_map": HOUSE_MAP, "dest_warp_id": "4"}]   # (the house's stairwell)
    cx, cy = art.CHANGER
    objects = []
    for k, x in (("LEFT", cx), ("RIGHT", cx + 3)):
        o = obj("OBJ_EVENT_GFX_ITEM_BALL", x, cy, f"PlayersHouse_B1F_EventScript_Ball", "MOVEMENT_TYPE_NONE")
        o["local_id"] = f"LOCALID_BASEMENT_BALL_{k}"
        o["movement_range_x"] = o["movement_range_y"] = 0
        objects.append(o)
    N = "BG_EVENT_PLAYER_FACING_NORTH"
    bgs = []
    syx, syy = art.SYNTH
    bgs += [sign(syx + i, syy + 2, "PlayersHouse_B1F_EventScript_Synthesizer", N) for i in range(4)]
    mx, my = art.MAKER
    bgs += [sign(mx + i, my + 2, "PlayersHouse_B1F_EventScript_BallMaker", N) for i in range(3)]
    bgs += [sign(cx + 1, cy + 1, "PlayersHouse_B1F_EventScript_BallChanger", N),
            sign(cx + 2, cy + 1, "PlayersHouse_B1F_EventScript_BallChanger", N)]
    bgs.append(sign(art.SWITCH[0], art.SWITCH[1], "PlayersHouse_B1F_EventScript_Switch", N))
    for k, (x, y) in enumerate(art.PAPERS, 1):
        bgs.append(sign(x, y, f"PlayersHouse_B1F_EventScript_Paper{k}"))
    bx, by = art.BOARD
    bgs += [sign(bx, by + 1, "PlayersHouse_B1F_EventScript_Chalkboard", N), sign(bx + 1, by + 1, "PlayersHouse_B1F_EventScript_Chalkboard", N)]
    bgs.append(sign(art.SHELF[0], art.SHELF[1] + 1, "PlayersHouse_B1F_EventScript_Bookshelf", N))
    bx, by = art.BENCH
    bgs += [sign(bx + i, by + j, "PlayersHouse_B1F_EventScript_Workbench") for i in range(2) for j in range(2)]
    bx, by = art.LABTABLE
    bgs += [sign(bx + i, by + j, "PlayersHouse_B1F_EventScript_LabTable") for i in range(2) for j in range(2)]
    bx, by = art.DESK
    bgs += [sign(bx + i, by + j, "PlayersHouse_B1F_EventScript_Desk") for i in range(2) for j in range(2)]
    bgs.append(sign(art.TRASH[0], art.TRASH[1], "PlayersHouse_B1F_EventScript_TrashCan"))
    bgs += [sign(x, y, "PlayersHouse_B1F_EventScript_Crates") for (x, y) in art.CRATES]
    write_map(NAME, MAP_ID, LAYOUT_ID, "MUS_RG_POKE_MANSION", "MAPSEC_PALLET_TOWN", "MAP_TYPE_INDOOR",
              objects, warps, bgs, False)


def write_house():
    """the 1F: the stairwell down, the rug one row lower, Mom with it, the warp"""
    lj = json.loads((ROOT / "data/layouts/layouts.json").read_text())
    lay = next(l for l in lj["layouts"] if l.get("id") == "LAYOUT_PALLET_TOWN_PLAYERS_HOUSE_1F_FRLG")
    w, path = lay["width"], ROOT / lay["blockdata_filepath"]
    data = bytearray(path.read_bytes())
    get = lambda x, y: struct.unpack_from("<H", data, (y * w + x) * 2)[0]
    put = lambda x, y, v: struct.pack_into("<H", data, (y * w + x) * 2, v)
    hx, hy = HOUSE_STAIRWELL
    if get(hx, hy) & 0x3FF != STAIRS_DOWN[(0, 0)][0]:          # (not done yet)
        x0, y0, x1, y1 = HOUSE_RUG
        floor = get(x0, y1 + 1)                                  # (the plain floor under the rug)
        for y in range(y1, y0 - 1, -1):
            for x in range(x0, x1 + 1):
                put(x, y + 1, get(x, y))
        for x in range(x0, x1 + 1):
            put(x, y0, floor)
        for (dx, dy), (m, coll) in STAIRS_DOWN.items():
            put(hx + dx, hy + dy, blk(m, coll, 3))
        path.write_bytes(bytes(data))
    mp = ROOT / f"data/maps/{HOUSE}/map.json"
    j = json.loads(mp.read_text())
    for o in j["object_events"]:
        if o.get("local_id") == "LOCALID_PLAYERS_HOUSE_1F_MOM":
            o["y"] = HOUSE_RUG[1] + 2                         # (her chair: row 4 -> 5)
    ws = [wp for wp in j["warp_events"] if wp["dest_map"] != MAP_ID]
    while len(ws) < 4:
        raise SystemExit("the house's warps aren't as expected")
    ws = ws[:4] + [{"x": hx + 2, "y": hy, "elevation": 3, "dest_map": MAP_ID, "dest_warp_id": "0"}]
    j["warp_events"] = ws
    mp.write_text(json.dumps(j, indent=2) + "\n")
    # where you wake up after a whiteout, and the finale's trip home: the chair next to Mom
    hp = ROOT / "src/data/heal_locations.json"
    hj = json.loads(hp.read_text())
    for e in hj["heal_locations"]:
        if e.get("respawn_map") == HOUSE_MAP:
            e["respawn_y"] = HOUSE_RUG[1] + 3
    hp.write_text(json.dumps(hj, indent=2) + "\n")
    fp = ROOT / "data/scripts/gym_finale.inc"
    fp.write_text(fp.read_text().replace(f"warp {HOUSE_MAP}, 8, 5", f"warp {HOUSE_MAP}, 8, {HOUSE_RUG[1] + 3}"))


def write_group():
    gp = ROOT / "data/maps/map_groups.json"
    groups = json.loads(gp.read_text())
    g = groups["gMapGroup_IndoorPallet_Frlg"]
    if NAME not in g:
        g.append(NAME)
    gp.write_text(json.dumps(groups, indent=2) + "\n")
    es = ROOT / "data/event_scripts.s"
    s = es.read_text()
    inc = f'\t.include "data/maps/{NAME}/scripts.inc"\n'
    if inc not in s:
        anchor = '\t.include "data/maps/PalletTown_TrainingHall_Frlg/scripts.inc"\n'
        s = s.replace(anchor, anchor + inc)
        es.write_text(s)


if __name__ == "__main__":
    g, frames = build()
    write_basement_map(g)
    write_frames(frames)
    write_house()
    write_group()
    if len(sys.argv) > 1:
        import render_map as rm
        rm.set_version("frlg")
        rm.render(NAME, sys.argv[1])
    print({k: len(v) for k, v in frames.items()})
