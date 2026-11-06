#!/usr/bin/env python3
"""Celadon's underground fighting pit (Pokémon Canvas): the hidden way in, the hall with its
doorman, the pit, and the back room (a placeholder for now). src/pit.c runs the fights and the
betting; data/scripts/pit.inc has everyone's words.

    python3 tools/gym_hack/build_pit.py [preview_dir]

The way in: walk east into the west wall of the building east of the Game Corner's prize house
(Celadon (43, 21) -> the wall at (44, 21); src/pit.c Pit_TryHiddenDoor). A ladder takes you down to
the hall; its doorman wants the password (overheard in the Game Corner).

Writes the three layouts (data/layouts/CeladonCity_Pit*/), their maps (data/maps/...: map.json, and
scripts.inc the first time), adds them to map_groups.json and layouts.json, and adds:
- Celadon's warp back up from the hall (warp 13, at (43, 21): a plain square, it never fires)
- the two gamblers in the Game Corner who let the password slip
Re-running is safe.
"""
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# Diglett's Cave metatiles (gTileset_General_Frlg + gTileset_DiglettsCave)
ROCK = 0x291                    # the cave's rock (its top): the pit's rim, walked on, and the walls
DIRT = 0x281                    # the cave floor: the pit itself
PIT_TL, PIT_T, PIT_TR = 0x29B, 0x299, 0x29C     # the pit's far wall (seen from above: its face)
PIT_L, PIT_R = 0x292, 0x290
PIT_BL, PIT_B, PIT_BR = 0x2A3, 0x289, 0x2D2     # the near edge
LADDER = 0x285
SIGN = 0x2CD
PEBBLES = (0x282, 0x283)
BOULDER = ((0x2D5, 0x2D6), (0x2DD, 0x2DE))
EDGE_H, EDGE_V = 0x2E1, 0x2DA                   # the higher rock round the room (its edge lines)


def block(m, collision=0, elevation=3):
    return m | (collision << 10) | (elevation << 12)


class Room:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.g = [[block(ROCK, 1, 0)] * w for _ in range(h)]

    def put(self, x, y, m, solid=False):
        self.g[y][x] = block(m, 1 if solid else 0, 0 if solid else 3)

    def floor(self, x0, y0, x1, y1, m=ROCK):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, m)

    def pit(self, x0, y0, x1, y1):
        """a sunken pit: dirt inside (x0..x1, y0..y1), walls round it"""
        self.floor(x0, y0, x1, y1, DIRT)
        for x in range(x0, x1 + 1):
            self.put(x, y0 - 1, PIT_T, True)
            self.put(x, y1 + 1, PIT_B, True)
        for y in range(y0, y1 + 1):
            self.put(x0 - 1, y, PIT_L, True)
            self.put(x1 + 1, y, PIT_R, True)
        self.put(x0 - 1, y0 - 1, PIT_TL, True)
        self.put(x1 + 1, y0 - 1, PIT_TR, True)
        self.put(x0 - 1, y1 + 1, PIT_BL, True)
        self.put(x1 + 1, y1 + 1, PIT_BR, True)

    def walls(self):
        """the higher rock round the walkable area: its edges where it meets the floor"""
        walk = [[(b >> 10) & 3 == 0 for b in row] for row in self.g]
        for y in range(self.h):
            for x in range(self.w):
                if walk[y][x] or (self.g[y][x] & 0x3FF) != ROCK:
                    continue
                below = y + 1 < self.h and walk[y + 1][x]
                side = (x + 1 < self.w and walk[y][x + 1]) or (x > 0 and walk[y][x - 1])
                if below:
                    self.g[y][x] = block(EDGE_H, 1, 0)
                elif side:
                    self.g[y][x] = block(EDGE_V, 1, 0)

    def flat(self):
        return [b for row in self.g for b in row]


# ---- The hall: a short tunnel, the ladder up at its foot, the doorman at its head ----
HALL_W, HALL_H = 9, 10
HALL_LADDER_UP = (4, 8)         # (back up to Celadon)
HALL_LADDER_DOWN = (4, 1)       # (down to the pit: past the doorman)
DOORMAN = (4, 2)


def hall():
    r = Room(HALL_W, HALL_H)
    r.floor(3, 1, 5, 8, DIRT)
    r.put(*HALL_LADDER_UP, LADDER)
    r.put(*HALL_LADDER_DOWN, LADDER)
    r.put(3, 5, PEBBLES[0])
    r.walls()
    return r


# ---- The pit: a big dirt cave, a log-fenced ring in the middle with the crowd round it, and
# the bookie in a plank booth with a counter. Its own tileset (gTileset_Pit: Diglett's Cave, plus
# blocks with the General tileset's log fence and planks over its dirt) ----
PIT_W, PIT_H = 27, 21
RING = (8, 6, 18, 13)           # the fence (x0, y0, x1, y1); the fight inside it
PIT_LADDER = (13, 18)           # (up to the hall)
BACK_LADDER = (24, 3)           # (down to the back room)
BOOTH = (20, 15, 22, 16)        # the planks behind the counter
COUNTER_Y = 14
CARD_SIGN = (19, 14)            # the card: tonight's bout (on the booth's corner)
RED_TRAINER, BLUE_TRAINER = (9, 10), (17, 10)
RED_MON, BLUE_MON = (12, 10), (14, 10)
BOOKIE = (21, 15)
TIPSTER = (3, 12)
CROWD = [  # (x, y, look, facing)
    (9, 5, "OBJ_EVENT_GFX_BIKER", "DOWN"), (12, 5, "OBJ_EVENT_GFX_ROCKER", "DOWN"),
    (16, 5, "OBJ_EVENT_GFX_MAN_2", "DOWN"), (7, 8, "OBJ_EVENT_GFX_WOMAN_3", "RIGHT"),
    (19, 9, "OBJ_EVENT_GFX_CRUSH_GIRL", "LEFT"), (7, 12, "OBJ_EVENT_GFX_BALDING_MAN", "RIGHT"),
    (10, 14, "OBJ_EVENT_GFX_GYM_GUY", "UP"), (15, 14, "OBJ_EVENT_GFX_FISHER", "UP"),
]

# gTileset_Pit's own blocks, after Diglett's Cave's (0x280 + its 157)
PIT_FIRST = 0x280 + 157
RING_H, RING_L, RING_R, COUNTER = PIT_FIRST, PIT_FIRST + 1, PIT_FIRST + 2, PIT_FIRST + 3
PLANK = 0x1BE                   # (the General tileset's deck: used as is)


def pit_room():
    r = Room(PIT_W, PIT_H)
    r.floor(2, 2, PIT_W - 3, PIT_H - 3, DIRT)
    x0, y0, x1, y1 = RING
    for x in range(x0, x1 + 1):
        r.put(x, y0, RING_H, True)
        r.put(x, y1, RING_H, True)
    for y in range(y0 + 1, y1):
        r.put(x0, y, RING_L, True)
        r.put(x1, y, RING_R, True)
    bx0, by0, bx1, by1 = BOOTH
    r.floor(bx0, by0, bx1, by1, PLANK)
    for x in range(bx0, bx1 + 1):
        r.put(x, COUNTER_Y, COUNTER, True)
        r.put(x, by1 + 1, RING_H, True)
    for y in range(by0, by1 + 1):
        r.put(bx0 - 1, y, RING_L, True)
        r.put(bx1 + 1, y, RING_R, True)
    r.put(*PIT_LADDER, LADDER)
    r.put(*BACK_LADDER, LADDER)
    r.put(*CARD_SIGN, SIGN, True)
    r.walls()
    for (x, y) in ((2, 2), (2, 16), (22, 7)):
        for dy in range(2):
            for dx in range(2):
                r.put(x + dx, y + dy, BOULDER[dy][dx], True)
    r.put(5, 4, PEBBLES[0], True)
    r.put(20, 3, PEBBLES[1], True)
    return r


def mb(name):
    import re
    n = -1
    for line in (ROOT / "include/constants/metatile_behaviors.h").read_text().splitlines():
        if "enum" in line:
            n = -1
        m = re.match(r"\s+(MB_\w+)", line)
        if m:
            n += 1
            if m.group(1) == name:
                return n
    raise KeyError(name)


def write_tileset():
    """gTileset_Pit: Diglett's Cave's tiles and palettes (src/data/tilesets: the same graphics),
    its blocks, and the pit's own"""
    src = ROOT / "data/tilesets/secondary/digletts_cave_frlg"
    dst = ROOT / "data/tilesets/secondary/pit"
    prim = ROOT / "data/tilesets/primary/general_frlg"
    import shutil
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    mt = [list(struct.unpack_from("<8H", (src / "metatiles.bin").read_bytes(), i * 16)) for i in range(157)]
    at = list(struct.unpack(f"<{157}I", (src / "metatile_attributes.bin").read_bytes()))
    pmt = (prim / "metatiles.bin").read_bytes()
    gen = lambda m: list(struct.unpack_from("<8H", pmt, m * 16))
    dirt = mt[DIRT - 0x280][:4]
    covered = 1 << 29
    mt.append(dirt + gen(0xE6)[4:]); at.append(covered)                       # RING_H: logs across
    mt.append(dirt + gen(0xF1)[4:]); at.append(covered)                       # RING_L: logs down its right
    mt.append(dirt + gen(0xF0)[4:]); at.append(covered)                       # RING_R: logs down its left
    mt.append(gen(PLANK)[:4] + gen(0xE7)[4:]); at.append(covered | mb("MB_COUNTER"))   # COUNTER
    (dst / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *m) for m in mt))
    (dst / "metatile_attributes.bin").write_bytes(struct.pack(f"<{len(at)}I", *at))
    # (registered once: the same graphics as Diglett's Cave)
    mh = ROOT / "src/data/tilesets/metatiles.h"
    t = mh.read_text()
    if "gMetatiles_Pit[]" not in t:
        t += ('\nconst u16 gMetatiles_Pit[] = INCBIN_U16("data/tilesets/secondary/pit/metatiles.bin");\n'
              'const u16 gMetatileAttributes_Pit[] = INCBIN_U16("data/tilesets/secondary/pit/metatile_attributes.bin");\n')
        mh.write_text(t)
    hh = ROOT / "src/data/tilesets/headers.h"
    t = hh.read_text()
    if "gTileset_Pit =" not in t:
        t += """
// Pokémon Canvas: the pit under Celadon (tools/gym_hack/build_pit.py): Diglett's Cave, and a log fence
const struct Tileset gTileset_Pit =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_DiglettsCave,
    .palettes = gTilesetPalettes_DiglettsCave,
    .metatiles = gMetatiles_Pit,
    .metatileAttributes = gMetatileAttributes_Pit,
    .callback = NULL,
};
"""
        hh.write_text(t)


# ---- The back room (for now: a door that stays shut, and the man who keeps it) ----
BACK_W, BACK_H = 11, 9
BACK_LADDER_UP = (5, 7)
BACK_KEEPER = (5, 3)


def back_room():
    r = Room(BACK_W, BACK_H)
    r.floor(2, 2, 8, 7)
    r.floor(4, 2, 6, 4, DIRT)
    r.put(*BACK_LADDER_UP, LADDER)
    r.walls()
    return r


MAPS = {
    "hall": dict(name="CeladonCity_PitHall", id="MAP_CELADON_CITY_PIT_HALL", build=hall, w=HALL_W, h=HALL_H),
    "pit": dict(name="CeladonCity_Pit", id="MAP_CELADON_CITY_PIT", build=pit_room, w=PIT_W, h=PIT_H),
    "back": dict(name="CeladonCity_PitBackRoom", id="MAP_CELADON_CITY_PIT_BACK_ROOM", build=back_room, w=BACK_W, h=BACK_H),
}
ORDER = ("hall", "pit", "back")
CELADON_WARP = 13               # Celadon's warp at the hidden door (added here)
HIDDEN_DOOR = (43, 21)          # stand here, walk east


def obj(gfx, pos, facing, script, local_id=None, movement=None):
    o = {"type": "object", "graphics_id": gfx, "x": pos[0], "y": pos[1], "elevation": 3,
         "movement_type": movement or f"MOVEMENT_TYPE_FACE_{facing}", "movement_range_x": 0,
         "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
         "trainer_sight_or_berry_tree_id": "0", "script": script, "flag": "0"}
    if local_id:
        o = {"local_id": local_id, **o}
    return o


def warp(pos, dest, dest_warp):
    return {"x": pos[0], "y": pos[1], "elevation": 3, "dest_map": dest, "dest_warp_id": str(dest_warp)}


def events(key):
    if key == "hall":
        return dict(
            object_events=[obj("OBJ_EVENT_GFX_BIKER", DOORMAN, "DOWN", "Pit_EventScript_Doorman", "LOCALID_PIT_DOORMAN")],
            warp_events=[warp(HALL_LADDER_UP, "MAP_CELADON_CITY", CELADON_WARP),
                         warp(HALL_LADDER_DOWN, "MAP_CELADON_CITY_PIT", 0)],
            bg_events=[])
    if key == "pit":
        objs = [
            obj("OBJ_EVENT_GFX_VAR_0", RED_MON, "RIGHT", "Pit_EventScript_RedMon", "LOCALID_PIT_RED_MON"),
            obj("OBJ_EVENT_GFX_VAR_1", BLUE_MON, "LEFT", "Pit_EventScript_BlueMon", "LOCALID_PIT_BLUE_MON"),
            obj("OBJ_EVENT_GFX_VAR_2", RED_TRAINER, "RIGHT", "Pit_EventScript_RedTrainer", "LOCALID_PIT_RED_TRAINER"),
            obj("OBJ_EVENT_GFX_VAR_3", BLUE_TRAINER, "LEFT", "Pit_EventScript_BlueTrainer", "LOCALID_PIT_BLUE_TRAINER"),
            obj("OBJ_EVENT_GFX_GENTLEMAN_FRLG", BOOKIE, "UP", "Pit_EventScript_Bookie", "LOCALID_PIT_BOOKIE"),
            obj("OBJ_EVENT_GFX_MAN_4", TIPSTER, "RIGHT", "Pit_EventScript_Tipster", "LOCALID_PIT_TIPSTER"),
        ]
        for i, (x, y, gfx, facing) in enumerate(CROWD):
            objs.append(obj(gfx, (x, y), facing, f"Pit_EventScript_Crowd{i}", f"LOCALID_PIT_CROWD_{i}"))
        return dict(
            object_events=objs,
            warp_events=[warp(PIT_LADDER, "MAP_CELADON_CITY_PIT_HALL", 1),
                         warp(BACK_LADDER, "MAP_CELADON_CITY_PIT_BACK_ROOM", 0)],
            bg_events=[{"type": "sign", "x": CARD_SIGN[0], "y": CARD_SIGN[1], "elevation": 0,
                        "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY", "script": "Pit_EventScript_Card"}])
    return dict(
        object_events=[obj("OBJ_EVENT_GFX_ROCKER", BACK_KEEPER, "DOWN", "Pit_EventScript_BackRoomKeeper", "LOCALID_PIT_BACK_KEEPER")],
        warp_events=[warp(BACK_LADDER_UP, "MAP_CELADON_CITY_PIT", 1)],
        bg_events=[])


def write_maps():
    write_tileset()
    lay_path = ROOT / "data/layouts/layouts.json"
    lays = json.loads(lay_path.read_text())
    groups_path = ROOT / "data/maps/map_groups.json"
    groups = json.loads(groups_path.read_text())
    group = groups["gMapGroup_IndoorCeladon_Frlg"]
    for key in ORDER:
        m = MAPS[key]
        r = m["build"]()
        d = ROOT / "data/layouts" / m["name"]
        d.mkdir(exist_ok=True)
        (d / "map.bin").write_bytes(struct.pack(f"<{r.w * r.h}H", *r.flat()))
        (d / "border.bin").write_bytes(struct.pack("<4H", *[block(ROCK, 1, 0)] * 4))
        lid = "LAYOUT_" + m["id"][4:]
        entry = {"id": lid, "name": m["name"] + "_Layout", "width": r.w, "height": r.h,
                 "border_width": 2, "border_height": 2,
                 "primary_tileset": "gTileset_General_Frlg",
                 "secondary_tileset": "gTileset_Pit" if key == "pit" else "gTileset_DiglettsCave",
                 "border_filepath": f"data/layouts/{m['name']}/border.bin",
                 "blockdata_filepath": f"data/layouts/{m['name']}/map.bin", "layout_version": "frlg"}
        for i, l in enumerate(lays["layouts"]):
            if l.get("id") == lid:
                lays["layouts"][i] = entry
                break
        else:
            lays["layouts"].append(entry)
        if m["name"] not in group:
            group.append(m["name"])         # (at the end: saves keep their maps' numbers)
        md = ROOT / "data/maps" / m["name"]
        md.mkdir(exist_ok=True)
        mj = {"id": m["id"], "name": m["name"], "layout": lid,
              "music": "MUS_RG_ROCKET_HIDEOUT", "region": "REGION_KANTO",
              "region_map_section": "MAPSEC_CELADON_CITY", "requires_flash": False,
              "weather": "WEATHER_NONE", "map_type": "MAP_TYPE_UNDERGROUND",
              "allow_cycling": False, "allow_escaping": False, "allow_running": True,
              "show_map_name": False, "floor_number": 0, "battle_scene": "MAP_BATTLE_SCENE_NORMAL",
              "connections": None, **events(key), "coord_events": []}
        mj = {k: mj[k] for k in ["id", "name", "layout", "music", "region", "region_map_section", "requires_flash",
                                 "weather", "map_type", "allow_cycling", "allow_escaping", "allow_running",
                                 "show_map_name", "floor_number", "battle_scene", "connections", "object_events",
                                 "warp_events", "coord_events", "bg_events"]}
        (md / "map.json").write_text(json.dumps(mj, indent=2) + "\n")
        scripts = md / "scripts.inc"
        if not scripts.exists():
            body = {"hall": "\tmap_script MAP_SCRIPT_ON_TRANSITION, Pit_EventScript_HallTransition\n",
                    "pit": "\tmap_script MAP_SCRIPT_ON_TRANSITION, Pit_EventScript_PitTransition\n",
                    "back": ""}[key]
            scripts.write_text(f"{m['name']}_MapScripts::\n{body}\t.byte 0\n")
    lay_path.write_text(json.dumps(lays, indent=2) + "\n")
    groups_path.write_text(json.dumps(groups, indent=2) + "\n")

    # Celadon: the warp back up (a plain square: it never fires; src/pit.c opens the hidden door)
    cp = ROOT / "data/maps/CeladonCity_Frlg/map.json"
    c = json.loads(cp.read_text())
    w = warp(HIDDEN_DOOR, "MAP_CELADON_CITY_PIT_HALL", 0)
    w["elevation"] = 0
    c["warp_events"] = c["warp_events"][:CELADON_WARP] + [w]
    cp.write_text(json.dumps(c, indent=2) + "\n")

    # The Game Corner: two gamblers, one letting the password slip
    gp = ROOT / "data/maps/CeladonCity_GameCorner_Frlg/map.json"
    gc = json.loads(gp.read_text())
    gc["object_events"] = [o for o in gc["object_events"] if not str(o.get("script", "")).startswith("Pit_")]
    gc["object_events"] += [obj("OBJ_EVENT_GFX_MAN_2", (14, 12), "RIGHT", "Pit_EventScript_GameCornerWhisperer"),
                            obj("OBJ_EVENT_GFX_BALDING_MAN", (15, 12), "LEFT", "Pit_EventScript_GameCornerListener")]
    gp.write_text(json.dumps(gc, indent=2) + "\n")

    # The scripts
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    anchor = '\t.include "data/maps/CeladonCity_GameCorner_Frlg/scripts.inc"\n'
    add = "".join(f'\t.include "data/maps/{MAPS[k]["name"]}/scripts.inc"\n' for k in ORDER)
    if add not in text:
        assert anchor in text
        text = text.replace(anchor, anchor + add, 1)
    inc = '\t.include "data/scripts/pit.inc"\n'
    if inc not in text:
        dec = '\t.include "data/scripts/gym_rooms.inc"\n'
        assert dec in text
        text = text.replace(dec, dec + inc, 1)
    ev.write_text(text)

    # The pit's spots, for src/pit.c
    (ROOT / "include/constants/pit.h").write_text(f"""// Generated by tools/gym_hack/build_pit.py. Don't edit by hand.
#ifndef GUARD_CONSTANTS_PIT_H
#define GUARD_CONSTANTS_PIT_H

// Celadon: stand here and walk east into the wall: the hidden door
#define PIT_DOOR_X {HIDDEN_DOOR[0]}
#define PIT_DOOR_Y {HIDDEN_DOOR[1]}
// In the pit: where the fighters and their Pokémon stand
#define PIT_RED_TRAINER_X {RED_TRAINER[0]}
#define PIT_RED_TRAINER_Y {RED_TRAINER[1]}
#define PIT_BLUE_TRAINER_X {BLUE_TRAINER[0]}
#define PIT_BLUE_TRAINER_Y {BLUE_TRAINER[1]}
#define PIT_RED_MON_X {RED_MON[0]}
#define PIT_RED_MON_Y {RED_MON[1]}
#define PIT_BLUE_MON_X {BLUE_MON[0]}
#define PIT_BLUE_MON_Y {BLUE_MON[1]}

#endif // GUARD_CONSTANTS_PIT_H
""")


def preview(out_dir):
    sys.path.insert(0, str(ROOT / "tools/gym_hack"))
    import render_map as rm
    from PIL import Image, ImageDraw
    prim = rm.Tileset(rm.tileset_dir("gTileset_General_Frlg"))
    for key in ORDER:
        sec = rm.Tileset(ROOT / ("data/tilesets/secondary/pit" if key == "pit" else "data/tilesets/secondary/digletts_cave_frlg"))
        r = MAPS[key]["build"]()
        img = Image.new("RGB", (r.w * 16, r.h * 16))
        for y in range(r.h):
            for x in range(r.w):
                rm.draw_metatile(img, x * 16, y * 16, r.g[y][x] & 0x3FF, prim, sec)
        dr = ImageDraw.Draw(img)
        for o in events(key)["object_events"]:
            dr.rectangle((o["x"] * 16 + 3, o["y"] * 16 + 3, o["x"] * 16 + 12, o["y"] * 16 + 12), outline=(255, 0, 255))
        img.resize((r.w * 32, r.h * 32), Image.NEAREST).save(Path(out_dir) / f"pit_{key}.png")


if __name__ == "__main__":
    if len(sys.argv) > 1:
        write_tileset()
        preview(sys.argv[1])
    else:
        write_maps()
