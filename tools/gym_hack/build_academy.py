#!/usr/bin/env python3
"""The Leader Academy (Pokémon Canvas): a campus for promising Gym Leader candidates from every
region, north of Route 11 through a gap in its trees.

The campus has its own tileset (gTileset_LeaderAcademy, tools/gym_hack/tileset_composer.py): red brick
buildings drawn in tools/gym_hack/academy_art.py (the main hall with its clock tower and portico, the
Battle Hall, the Exchange, two dorms), the Roserade statue, lamps, benches and rose beds, plus the
Pokémon Center and the mart (FireRed's, from Saffron's tileset) and Saffron's cobbles. Lawns, trees
and flowers are the general tileset's.

The main hall inside is a library (gTileset_LeaderAcademyHall, over the building tileset): plank
floors, a red carpet to the headmaster's desk, bookshelves and stacks, reading tables, and four doors
to the lecture halls (Viridian's classroom layout), two instructors in each.

    python3 tools/gym_hack/build_academy.py [campus.png hall.png]

Writes:
  data/tilesets/secondary/leader_academy_frlg/, leader_academy_hall_frlg/ (+ their C declarations)
  data/layouts/LeaderAcademy_Campus/, LeaderAcademy_Hall/ (+ layouts.json)
  data/maps/LeaderAcademy_*/map.json (scripts.inc kept if there)
  data/maps/map_groups.json        gMapGroup_LeaderAcademy (new maps go at the end: saves stay valid)
  data/maps/leader_academy.inc     their scripts (data/event_scripts.s)
  data/layouts/Route11_Frlg/map.bin, Route11's map.json   the gap in the trees, the gate guards,
                                                          the connection north
"""
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))
import academy_art as art                       # noqa: E402
import paldea                                   # noqa: E402  (read_layout)
from build_exotic_safari import obj, write_layout, write_map     # noqa: E402
from tileset_composer import LAYER_COVERED, Composer             # noqa: E402

W, H = 44, 40
OFFSET = 18                 # (the campus's x 0 is Route 11's x 18: its path, x 20-21, is the gap at 38-39)
GAP = (20, 21)
SAFFRON_SYM = "gTileset_SaffronCity"
SAFFRON, _ = paldea.read_layout("LAYOUT_SAFFRON_CITY")


def blk(m, coll=0, elev=3):
    return m | (coll << 10) | (elev << 12)


# The general tileset's: lawn (a 2x2 pattern), flowers, sign, trees
GRASS = [[0x008, 0x009], [0x010, 0x011]]
FLOWERS = 0x004
SIGN = 0x002
ROAD_L, ROAD, ROAD_R = 0x0D8, 0x0D9, 0x0DA     # Route 11's dirt road (grass rim left / plain / rim right)
TREE = {
    "A": (0x0014, 0x0015), "B": (0x001C, 0x001D), "first": (0x001E, 0x001F), "bottom": (0x0024, 0x0025),
}
CROWN = (0x30E, 0x30F)      # (Saffron's: a treetop over the lawn)
EDGE_L = {"A": 0x0016, "B": 0x001E, "first": 0x001E, "bottom": 0x0026}
EDGE_R = {"A": 0x0017, "B": 0x001F, "first": 0x001F, "bottom": 0x0027}

# Where everything is. Buildings: (x, y) of the top-left block
HALL_AT = (14, 4)           # 14 x 9: its doors at (20, 12) and (21, 12), in line with the path
CENTER = {"src": (22, 34, 26, 38), "door": (2, 4), "at": (6, 19)}      # Saffron's Pokémon Center
SUPPLY = {"src": (38, 18, 41, 21), "door": (2, 3), "at": (7, 29)}      # Saffron's mart
BATTLE_AT = (30, 19)        # 7 x 5: door at (33, 23)
EXCHANGE_AT = (31, 29)      # 5 x 4: door at (33, 32)
DORMS = ((5, 5), (33, 5))   # 6 x 5
STATUE_AT = (20, 15)        # 2 x 3
LAMPS = ((18, 20), (23, 20), (18, 27), (23, 27), (16, 13), (25, 13))
BENCHES = ((16, 21), (24, 21), (16, 30), (24, 30))
ROSE_BEDS = [(x, 13) for x in (17, 18, 23, 24)] + [(x, y) for x in (16, 17, 24, 25) for y in (16, 17)]
TREES = ((12, 18), (28, 18), (12, 27), (28, 27), (13, 30), (27, 30), (5, 13), (35, 13))
FLOWER_PATCHES = ((12, 14, 13, 15), (28, 14, 29, 15), (5, 11, 10, 11), (33, 11, 38, 11),
                  (15, 32, 18, 32), (23, 32, 26, 32), (5, 27, 6, 28), (37, 27, 38, 28))
SIGNS = {"hall": (19, 13), "center": (11, 23), "supply": (11, 32),
         "battle": (29, 23), "exchange": (30, 32), "gate": (22, 32)}     # (the gate's: where the path meets the lower walk)


def build_campus():
    c = Composer("gTileset_General_Frlg")
    brick = c.add_palette(art.BRICK)
    decor = c.add_palette(art.DECOR)
    cobble = c.copy(SAFFRON_SYM, 0x2E9)
    cobble_ground = c.metatiles[cobble - 0x280][:4]
    crown = [c.copy(SAFFRON_SYM, m) for m in CROWN]

    g = [[blk(GRASS[y % 2][x % 2]) for x in range(W)] for y in range(H)]

    def put(x, y, m, coll=1, elev=None):
        g[y][x] = blk(m, coll, (0 if coll else 3) if elev is None else elev)

    def path(x0, y0, x1, y1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                put(x, y, cobble, 0)

    # ---- the frame of trees ----
    forest = [[x < 4 or x >= W - 4 or y < 4 or (y >= 36 and x not in GAP) for x in range(W)] for y in range(H)]
    A = (H - 1) % 2
    for y in range(H):
        for x in range(W):
            if not forest[y][x]:
                continue
            above = y > 0 and forest[y - 1][x]
            below = y == H - 1 or forest[y + 1][x]
            kind = "bottom" if not below else ("first" if y > 0 and not above else ("A" if y % 2 == A else "B"))
            if y == 0:
                kind = "A" if y % 2 == A else "B"
            left = x % 2 == 0
            if left and x > 0 and not forest[y][x - 1]:
                m = EDGE_L[kind]
            elif not left and x < W - 1 and not forest[y][x + 1]:
                m = EDGE_R[kind]
            else:
                m = TREE[kind][0 if left else 1]
            put(x, y, m)
            if kind == "first" and y > 0 and not forest[y - 1][x]:
                put(x, y - 1, crown[0 if left else 1], 0)

    # ---- paths: the gate up to the plaza, the plaza, two cross walks, the buildings' fronts ----
    path(20, 20, 21, 39)
    path(15, 13, 26, 19)                    # the plaza
    path(5, 24, 38, 25)                     # the upper walk
    path(5, 33, 38, 34)                     # the lower walk
    # the gap through the trees is Route 11's dirt road, coming up to meet the cobbles
    for y in range(36, H):
        put(GAP[0], y, ROAD_L, 0)
        put(GAP[1], y, ROAD_R, 0)

    # ---- flowers (the general tileset's, they sway) ----
    for (x0, y0, x1, y1) in FLOWER_PATCHES:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                put(x, y, FLOWERS, 0)

    # ---- single trees in the lawns (2x2, a treetop peeking over the block above) ----
    for (x, y) in TREES:
        put(x, y, 0x1E)
        put(x + 1, y, 0x1F)
        put(x, y + 1, 0x26)
        put(x + 1, y + 1, 0x27)
        put(x, y - 1, 0x0E, 0)
        put(x + 1, y - 1, 0x0F, 0)

    # ---- the brick buildings ----
    doors = {}

    def stamp(img, at, door=None, ground=None, behind_rows=0):
        door = [] if door is None else (door if isinstance(door, list) else [door])
        door_blocks = [(d[0] - at[0], d[1] - at[1]) for d in door]
        beh = {b: "MB_NON_ANIMATED_DOOR" for b in door_blocks}
        grid = c.art(img, brick, ground=ground or GRASS[0][0], behind_rows=behind_rows, behavior=beh)
        for by, row in enumerate(grid):
            for bx, m in enumerate(row):
                walk = (bx, by) in door_blocks
                put(at[0] + bx, at[1] + by, m, 0 if walk else 1)

    stamp(art.main_hall(), HALL_AT, door=[(HALL_AT[0] + 6, HALL_AT[1] + 8), (HALL_AT[0] + 7, HALL_AT[1] + 8)])
    doors["hall"] = (HALL_AT[0] + 6, HALL_AT[1] + 8)
    doors["hall2"] = (HALL_AT[0] + 7, HALL_AT[1] + 8)
    stamp(art.battle_hall(), BATTLE_AT, door=(BATTLE_AT[0] + 3, BATTLE_AT[1] + 4))
    doors["battle"] = (BATTLE_AT[0] + 3, BATTLE_AT[1] + 4)
    stamp(art.exchange(), EXCHANGE_AT, door=(EXCHANGE_AT[0] + 2, EXCHANGE_AT[1] + 3))
    doors["exchange"] = (EXCHANGE_AT[0] + 2, EXCHANGE_AT[1] + 3)
    dorm = art.dorm()
    for at in DORMS:
        stamp(dorm, at)

    # ---- FireRed's Pokémon Center and mart ----
    for name, b in (("center", CENTER), ("supply", SUPPLY)):
        sx0, sy0, sx1, sy1 = b["src"]
        ax, ay = b["at"]
        for dy in range(sy1 - sy0 + 1):
            for dx in range(sx1 - sx0 + 1):
                v = SAFFRON[sy0 + dy][sx0 + dx]
                m = c.copy(SAFFRON_SYM, v & 0x3FF)
                if not (v >> 10) & 3 and (dx, dy) != b["door"] and m >= 0x280:
                    # (the roof's top edge, drawn over Saffron's pavement: over the lawn instead)
                    m = c.add_metatile(c.ground(GRASS[(ay + dy) % 2][(ax + dx) % 2]) + c.metatiles[m - 0x280][4:],
                                       c.attrs[m - 0x280])
                g[ay + dy][ax + dx] = m | (v & 0xFC00)
        doors[name] = (ax + b["door"][0], ay + b["door"][1])

    # ---- the statue, lamps, benches, rose beds ----
    def decor_at(img, at, ground, behind_rows, walk_rows=0):
        grid = c.art(img, decor, ground=ground, behind_rows=behind_rows)
        for by, row in enumerate(grid):
            for bx, m in enumerate(row):
                put(at[0] + bx, at[1] + by, m, 0 if by < walk_rows else 1)

    decor_at(art.roserade_statue(), STATUE_AT, cobble_ground, behind_rows=1, walk_rows=1)
    for at in LAMPS:
        on_path = (g[at[1] + 1][at[0]] & 0x3FF) == cobble
        decor_at(art.lamp(), at, cobble_ground if on_path else GRASS[0][0], behind_rows=1, walk_rows=1)
    for at in BENCHES:
        decor_at(art.bench(), at, GRASS[0][0], behind_rows=0)
    bed = art.rose_bed()
    for at in ROSE_BEDS:
        decor_at(bed, at, GRASS[0][0], behind_rows=0)

    # ---- signs ----
    for (x, y) in SIGNS.values():
        put(x, y, SIGN)

    c.write("LeaderAcademy", "leader_academy_frlg")
    return g, doors


# ---------------------------------------------------------------------------------------------
# The main hall: a library
# ---------------------------------------------------------------------------------------------
HW, HH = 25, 17
HALL_DOORS = {"A": 3, "B": 8, "C": 16, "D": 21, "M": 12}  # x of each lecture hall's door; M: the Mega room's
HALL_WINDOWS = (6, 18)
HALL_MAT = (11, 12, 13)


def build_hall():
    c = Composer("gTileset_BuildingFrlg")
    lib = c.add_palette(art.LIB)
    g = [[0] * HW for _ in range(HH)]

    def put(x, y, m, coll=1):
        g[y][x] = blk(m, coll, 0 if coll else 3)

    def one(img, behavior=None):
        return c.art(img, lib, behavior=behavior)

    floor = one(art.parquet())[0][0]
    carpet = [one(art.carpet(p))[0][0] for p in "lmr"]
    for y in range(2, HH - 1):
        for x in range(HW):
            put(x, y, floor, 0)
    for y in range(4, HH - 1):
        for k, x in enumerate(HALL_MAT):
            put(x, y, carpet[k], 0)
    # the wall: shelves along it, the lecture halls' doors, windows
    wall = one(art.wall())
    shelves = [one(art.bookshelf(v)) for v in range(3)]
    window = one(art.tall_window())
    doors = {k: one(art.lecture_door(k), behavior={(0, 1): "MB_NON_ANIMATED_DOOR"}) for k in HALL_DOORS}
    for x in range(HW):
        piece, walk = shelves[x % 3], False
        for k, dx in HALL_DOORS.items():
            if x == dx:
                piece, walk = doors[k], True
        if x in HALL_WINDOWS:
            piece = window
        if any(x in (dx - 1, dx + 1) for dx in HALL_DOORS.values()):
            piece = wall
        put(x, 0, piece[0][0])
        put(x, 1, piece[1][0], 0 if walk else 1)
    # the headmaster's desk (a counter: talked to across it), a globe either side
    desk = c.art(art.desk(), lib, ground=floor_ground(c, floor), behavior={(i, 0): "MB_COUNTER" for i in range(3)})
    for i, m in enumerate(desk[0]):
        put(11 + i, 4, m)
    glob = c.art(art.globe(), lib, ground=floor_ground(c, floor))
    for x in (8, 16):
        put(x, 3, glob[0][0])
    # the stacks: two rows of shelves either side, reading tables between them and the carpet
    for y in (6, 10):
        for x in list(range(1, 7)) + list(range(18, 24)):
            s = shelves[(x + y) % 3]
            put(x, y, s[0][0])
            put(x, y + 1, s[1][0])
    table = c.art(art.reading_table(), lib, ground=floor_ground(c, floor))
    for (x, y) in ((8, 8), (15, 8), (8, 12), (15, 12)):
        put(x, y, table[0][0])
        put(x + 1, y, table[0][1])
    # the way out: the mat (the school's), the doorway under it
    mat_beh = c._source("gTileset_School")[2][0x2A6 - 0x280] & 0x1FF       # (the school mat's: a way out)
    mat = c.art(art.doormat(), lib, behavior={(0, 0): mat_beh})[0][0]
    for x in HALL_MAT:
        put(x, HH - 2, mat, 0)
    for x in range(HW):
        g[HH - 1][x] = blk(0x008, 1, 0)
    for k, x in enumerate(HALL_MAT):
        g[HH - 1][x] = blk(0x01A + k, 1, 0)
    c.write("LeaderAcademyHall", "leader_academy_hall_frlg")
    return g


def floor_ground(c, floor):
    return c.metatiles[floor - 0x280][4:]       # (the floor art is the parquet's top layer)


# ---------------------------------------------------------------------------------------------
# Maps and people
# ---------------------------------------------------------------------------------------------
def lobj(local, gfx, x, y, script, movement="MOVEMENT_TYPE_FACE_DOWN", flag="0", rng=1):
    o = obj(gfx, x, y, script, movement, flag)
    o["local_id"] = local
    o["movement_range_x"] = o["movement_range_y"] = rng
    return o


def warp(x, y, dest, wid):
    return {"x": x, "y": y, "elevation": 0, "dest_map": dest, "dest_warp_id": str(wid)}


def sign(x, y, script):
    return {"type": "sign", "x": x, "y": y, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH", "script": script}


# The people: (local id suffix, gfx, x, y, script, movement[, flag])
CAMPUS_PEOPLE = [
    ("WELCOME_HEADMASTER", "OBJ_EVENT_GFX_OLD_WOMAN", 21, 33, "Academy_EventScript_Nothing", "MOVEMENT_TYPE_FACE_DOWN", "FLAG_HIDE_ACADEMY_WELCOME"),
    ("WELCOME_MENTOR", "OBJ_EVENT_GFX_VAR_0", 20, 37, "Academy_EventScript_Nothing", "MOVEMENT_TYPE_FACE_UP", "FLAG_HIDE_ACADEMY_MENTOR"),
    ("CANDIDATE_KOJI", "OBJ_EVENT_GFX_BLACK_BELT_FRLG", 13, 25, "Academy_EventScript_Koji", "MOVEMENT_TYPE_FACE_RIGHT", "0"),
    ("CANDIDATE_MAREN", "OBJ_EVENT_GFX_SWIMMER_F_LAND", 17, 18, "Academy_EventScript_Maren", "MOVEMENT_TYPE_WANDER_AROUND", "0"),
    ("CANDIDATE_KEOLA", "OBJ_EVENT_GFX_CAMPER_FRLG", 30, 34, "Academy_EventScript_Keola", "MOVEMENT_TYPE_WANDER_AROUND", "0"),
    ("CANDIDATE_RENE", "OBJ_EVENT_GFX_BEAUTY_FRLG", 25, 22, "Academy_EventScript_Rene", "MOVEMENT_TYPE_FACE_LEFT", "0"),
    ("STUDENT_1", "OBJ_EVENT_GFX_LASS_FRLG", 17, 22, "Academy_EventScript_Student1", "MOVEMENT_TYPE_FACE_UP", "0"),
    ("STUDENT_2", "OBJ_EVENT_GFX_YOUNGSTER_FRLG", 14, 33, "Academy_EventScript_Student2", "MOVEMENT_TYPE_WANDER_AROUND", "0"),
]
HALL_PEOPLE = [
    ("HEADMASTER", "OBJ_EVENT_GFX_OLD_WOMAN", 12, 3, "Academy_EventScript_Headmaster", "MOVEMENT_TYPE_FACE_DOWN"),
    ("CANDIDATE_NADIA", "OBJ_EVENT_GFX_COOLTRAINER_F", 20, 13, "Academy_EventScript_Nadia", "MOVEMENT_TYPE_FACE_LEFT"),
    ("READER_1", "OBJ_EVENT_GFX_LASS_FRLG", 9, 9, "Academy_EventScript_Reader1", "MOVEMENT_TYPE_FACE_UP"),
    ("READER_2", "OBJ_EVENT_GFX_SCIENTIST", 16, 13, "Academy_EventScript_Reader2", "MOVEMENT_TYPE_FACE_UP"),
]
MEGA_DOOR_GUARD = ("MEGA_DOOR_GUARD", "OBJ_EVENT_GFX_COOLTRAINER_F", 12, 2, "Academy_EventScript_MegaDoorGuard",
                   "MOVEMENT_TYPE_FACE_DOWN", "FLAG_GOT_MEGA_BRACELET")
# The lecture halls: (letter, two instructors)
LECTURES = {
    "A": [("TEACHER_LARKSPUR", "OBJ_EVENT_GFX_GENTLEMAN_FRLG", "Larkspur"), ("TEACHER_CAMELLIA", "OBJ_EVENT_GFX_BEAUTY_FRLG", "Camellia")],
    "B": [("TEACHER_FOXGLOVE", "OBJ_EVENT_GFX_SCIENTIST", "Foxglove"), ("TEACHER_VERBENA", "OBJ_EVENT_GFX_COOLTRAINER_M", "Verbena")],
    "C": [("TEACHER_MARIGOLD", "OBJ_EVENT_GFX_WOMAN_3_FRLG", "Marigold"), ("TEACHER_HYACINTH", "OBJ_EVENT_GFX_COOLTRAINER_F", "Hyacinth")],
    "D": [("TEACHER_YARROW", "OBJ_EVENT_GFX_GENTLEMAN_FRLG", "Yarrow"), ("TEACHER_TANSY", "OBJ_EVENT_GFX_LASS_FRLG", "Tansy")],
}
LECTURE_STUDENTS = {"A": "OBJ_EVENT_GFX_YOUNGSTER_FRLG", "B": "OBJ_EVENT_GFX_LASS_FRLG",
                    "C": "OBJ_EVENT_GFX_BUG_CATCHER", "D": "OBJ_EVENT_GFX_POKE_MANIAC_FRLG"}
MAPS = {
    # name: (layout, music, warps back to the campus (x, y)s, its door on the campus)
    "LeaderAcademy_Hall": ("LAYOUT_LEADER_ACADEMY_HALL", "MUS_RG_OAK_LAB", [(x, HH - 2) for x in HALL_MAT], "hall"),
    "LeaderAcademy_PokemonCenter": ("LAYOUT_POKEMON_CENTER_1F_FRLG", "MUS_RG_POKE_CENTER", [(6, 8), (7, 8), (8, 8)], "center"),
    "LeaderAcademy_Supply": ("LAYOUT_MART_FRLG", "MUS_RG_POKE_CENTER", [(3, 7), (4, 7), (5, 7)], "supply"),
    "LeaderAcademy_BattleHall": ("LAYOUT_SAFFRON_CITY_DOJO", "MUS_RG_PEWTER", [(5, 14), (6, 14), (7, 14)], "battle"),
    "LeaderAcademy_Exchange": ("LAYOUT_MART_FRLG", "MUS_RG_POKE_CENTER", [(3, 7), (4, 7), (5, 7)], "exchange"),
}
LECTURE_MAPS = {k: f"LeaderAcademy_Lecture{k}" for k in LECTURES}
MEGA_ROOM = "LeaderAcademy_MegaRoom"
DOOR_MAPS = dict(LECTURE_MAPS, M=MEGA_ROOM)
INTERIOR_PEOPLE = {
    "LeaderAcademy_Hall": HALL_PEOPLE,
    "LeaderAcademy_PokemonCenter": [
        ("NURSE", "OBJ_EVENT_GFX_NURSE_FRLG", 7, 2, "Academy_EventScript_Nurse", "MOVEMENT_TYPE_FACE_DOWN"),
        ("CANDIDATE_ELIAS", "OBJ_EVENT_GFX_HIKER_FRLG", 12, 5, "Academy_EventScript_Elias", "MOVEMENT_TYPE_FACE_LEFT"),
    ],
    "LeaderAcademy_Supply": [
        ("SUPPLY_CLERK", "OBJ_EVENT_GFX_CLERK", 2, 3, "Academy_EventScript_SupplyClerk", "MOVEMENT_TYPE_FACE_RIGHT"),
        ("STUDENT_3", "OBJ_EVENT_GFX_POKE_MANIAC_FRLG", 8, 4, "Academy_EventScript_Student3", "MOVEMENT_TYPE_FACE_UP"),
    ],
    "LeaderAcademy_BattleHall": [
        ("RENTAL_CLERK", "OBJ_EVENT_GFX_COOLTRAINER_M", 6, 5, "Academy_EventScript_RentalClerk", "MOVEMENT_TYPE_FACE_DOWN"),
        ("CANDIDATE_BRIONY", "OBJ_EVENT_GFX_COOLTRAINER_F", 1, 8, "Academy_EventScript_Briony", "MOVEMENT_TYPE_FACE_RIGHT"),
        ("CANDIDATE_TEO", "OBJ_EVENT_GFX_COOLTRAINER_M", 11, 8, "Academy_EventScript_Teo", "MOVEMENT_TYPE_FACE_LEFT"),
    ],
    "LeaderAcademy_Exchange": [
        ("EXCHANGE_CLERK_MONEY", "OBJ_EVENT_GFX_CLERK", 2, 3, "Academy_EventScript_ExchangeMoney", "MOVEMENT_TYPE_FACE_RIGHT"),
        ("EXCHANGE_CLERK_BP", "OBJ_EVENT_GFX_CLERK", 2, 5, "Academy_EventScript_ExchangeBP", "MOVEMENT_TYPE_FACE_RIGHT"),
    ],
}
SIGN_SCRIPTS = {"hall": "Academy_EventScript_HallSign", "center": "Academy_EventScript_CenterSign",
                "supply": "Academy_EventScript_SupplySign", "battle": "Academy_EventScript_BattleSign",
                "exchange": "Academy_EventScript_ExchangeSign", "gate": "Academy_EventScript_GateSign"}
CAMPUS = "LeaderAcademy_Campus"
STATUE_SCRIPT = "Academy_EventScript_Statue"


def mid(name):
    import re
    return "MAP_" + re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name).upper()


def write_campus(g, doors):
    write_layout(CAMPUS, "LAYOUT_LEADER_ACADEMY_CAMPUS", g, "gTileset_General_Frlg", "gTileset_LeaderAcademy",
                 border=[[0x0414, 0x0415], [0x041C, 0x041D]])
    order = list(MAPS)
    warps = [warp(*doors[MAPS[m][3]], mid(m), 0) for m in order]
    warps.append(warp(*doors["hall2"], mid("LeaderAcademy_Hall"), 1))     # (warp 5: the hall's right door)
    objects = [lobj("LOCALID_ACADEMY_" + p[0], p[1], p[2], p[3], p[4], p[5], p[6], 2 if "WANDER" in p[5] else 1)
               for p in CAMPUS_PEOPLE]
    bgs = [sign(x, y, SIGN_SCRIPTS[k]) for k, (x, y) in SIGNS.items()]
    bgs += [sign(STATUE_AT[0] + dx, STATUE_AT[1] + 2, STATUE_SCRIPT) for dx in (0, 1)]
    write_map(CAMPUS, mid(CAMPUS), "LAYOUT_LEADER_ACADEMY_CAMPUS", "MUS_RG_CELADON", "MAPSEC_ROUTE_11", "MAP_TYPE_CITY",
              objects, warps, bgs, True)
    j = json.loads((ROOT / f"data/maps/{CAMPUS}/map.json").read_text())
    j["connections"] = [{"map": "MAP_ROUTE11", "offset": -OFFSET, "direction": "down"}]
    j["weather"] = "WEATHER_SUNNY"
    (ROOT / f"data/maps/{CAMPUS}/map.json").write_text(json.dumps(j, indent=2) + "\n")
    for i, m in enumerate(order):
        layout, music, back, _ = MAPS[m]
        ws = [warp(x, y, mid(CAMPUS), i) for (x, y) in back]
        if m == "LeaderAcademy_Hall":       # (and the lecture halls' doors: warps 3-6; the Mega room's: 7)
            ws += [warp(x, 1, mid(DOOR_MAPS[k]), 0) for k, x in HALL_DOORS.items()]
        people = [lobj("LOCALID_ACADEMY_" + p[0], p[1], p[2], p[3], p[4], p[5]) for p in INTERIOR_PEOPLE.get(m, [])]
        if m == "LeaderAcademy_Hall":
            g_ = MEGA_DOOR_GUARD
            people.append(lobj("LOCALID_ACADEMY_" + g_[0], g_[1], g_[2], g_[3], g_[4], g_[5], g_[6]))
        write_map(m, mid(m), layout, music, "MAPSEC_ROUTE_11", "MAP_TYPE_INDOOR", people, ws, [], False)
    for n, (k, m) in enumerate(LECTURE_MAPS.items()):
        ws = [warp(x, 7, mid("LeaderAcademy_Hall"), 3 + n) for x in (3, 4, 5)]
        (a, b) = LECTURES[k]
        people = [lobj("LOCALID_ACADEMY_" + a[0], a[1], 2, 2, f"Academy_EventScript_{a[2]}"),
                  lobj("LOCALID_ACADEMY_" + b[0], b[1], 7, 2, f"Academy_EventScript_{b[2]}"),
                  lobj(f"LOCALID_ACADEMY_LECTURE_{k}_STUDENT", LECTURE_STUDENTS[k], 4, 6,
                       f"Academy_EventScript_Lecture{k}Student", "MOVEMENT_TYPE_FACE_UP")]
        write_map(m, mid(m), "LAYOUT_VIRIDIAN_CITY_SCHOOL", "MUS_RG_OAK_LAB", "MAPSEC_ROUTE_11", "MAP_TYPE_INDOOR",
                  people, ws, [], False)
    ws = [warp(x, 7, mid("LeaderAcademy_Hall"), 7) for x in (3, 4, 5)]
    people = [lobj("LOCALID_ACADEMY_MEGA_KEEPER", "OBJ_EVENT_GFX_WOMAN_1_FRLG", 4, 2, "Academy_EventScript_MegaKeeper")]
    write_map(MEGA_ROOM, mid(MEGA_ROOM), "LAYOUT_VIRIDIAN_CITY_SCHOOL", "MUS_RG_OAK_LAB", "MAPSEC_ROUTE_11",
              "MAP_TYPE_INDOOR", people, ws, [], False)


def write_hall_layout(g):
    write_layout("LeaderAcademy_Hall", "LAYOUT_LEADER_ACADEMY_HALL", g, "gTileset_BuildingFrlg",
                 "gTileset_LeaderAcademyHall", border=0x008)


def write_route11():
    """The gap in Route 11's northern trees (x 38-39), its gate guards, and the connection north"""
    lj = json.loads((ROOT / "data/layouts/layouts.json").read_text())
    lay = next(l for l in lj["layouts"] if l.get("id") == "LAYOUT_ROUTE11")
    w = lay["width"]
    path = ROOT / lay["blockdata_filepath"]
    data = bytearray(path.read_bytes())
    # the road runs up from Route 11's (row 2 was its grassy top edge) through the gap
    for y, row in ((0, (ROAD_L, ROAD_R)), (1, (ROAD_L, ROAD_R)), (2, (ROAD, ROAD))):
        for x, m in zip((OFFSET + GAP[0], OFFSET + GAP[1]), row):
            struct.pack_into("<H", data, (y * w + x) * 2, blk(m))
    path.write_bytes(bytes(data))
    mp = ROOT / "data/maps/Route11_Frlg/map.json"
    j = json.loads(mp.read_text())
    conns = [c for c in j["connections"] if c["map"] != mid(CAMPUS)]
    conns.append({"map": mid(CAMPUS), "offset": OFFSET, "direction": "up"})
    j["connections"] = conns
    objs = [o for o in j["object_events"] if not o.get("local_id", "").startswith("LOCALID_ROUTE11_ACADEMY")]
    objs += [
        lobj("LOCALID_ROUTE11_ACADEMY_GUARD_L", "OBJ_EVENT_GFX_COOLTRAINER_M", OFFSET + GAP[0], 1, "Academy_EventScript_GateGuard",
             "MOVEMENT_TYPE_FACE_DOWN", "FLAG_HIDE_ACADEMY_GATE"),
        lobj("LOCALID_ROUTE11_ACADEMY_GUARD_R", "OBJ_EVENT_GFX_COOLTRAINER_F", OFFSET + GAP[1], 1, "Academy_EventScript_GateGuard",
             "MOVEMENT_TYPE_FACE_DOWN", "FLAG_HIDE_ACADEMY_GATE"),
        lobj("LOCALID_ROUTE11_ACADEMY_MENTOR", "OBJ_EVENT_GFX_VAR_0", OFFSET + GAP[1], 4, "Academy_EventScript_Nothing",
             "MOVEMENT_TYPE_FACE_LEFT", "FLAG_HIDE_ACADEMY_MENTOR"),
    ]
    j["object_events"] = objs
    mp.write_text(json.dumps(j, indent=2) + "\n")


def write_group():
    gp = ROOT / "data/maps/map_groups.json"
    groups = json.loads(gp.read_text())
    names = [CAMPUS] + list(MAPS) + list(LECTURE_MAPS.values()) + [MEGA_ROOM]
    if "gMapGroup_LeaderAcademy" not in groups["group_order"]:
        groups["group_order"].append("gMapGroup_LeaderAcademy")
    groups["gMapGroup_LeaderAcademy"] = names
    gp.write_text(json.dumps(groups, indent=2) + "\n")
    (ROOT / "data/maps/leader_academy.inc").write_text(
        "@ The Leader Academy's maps (tools/gym_hack/build_academy.py)\n"
        + '\t.include "data/scripts/leader_academy.inc"\n'
        + "".join(f'\t.include "data/maps/{n}/scripts.inc"\n' for n in names))


def preview(map_name, out):
    import render_map as rm
    rm.set_version("frlg")
    rm.render(map_name, out)


if __name__ == "__main__":
    g, doors = build_campus()
    write_campus(g, doors)
    write_hall_layout(build_hall())
    write_route11()
    write_group()
    if len(sys.argv) > 2:
        preview(CAMPUS, sys.argv[1])
        preview("LeaderAcademy_Hall", sys.argv[2])
    print("academy:", doors)
