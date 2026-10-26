#!/usr/bin/env python3
"""The Leader Academy (Pokémon Canvas): a campus for promising Gym Leader candidates from every
region, north of Route 11 through a gap in its trees. Laid out here on Saffron City's tileset (its
buildings stamped where a campus wants them: Silph Co. as the Academy Hall, the dojo as the Battle
Hall, a Pokémon Center, the mart as the Training Supply, a house as the Exchange, the apartment
blocks as dorms), around a cobbled quad, in a frame of trees.

    python3 tools/gym_hack/build_academy.py [preview.png]

Writes:
  data/layouts/LeaderAcademy_Campus/ (+ layouts.json)   the campus
  data/maps/LeaderAcademy_*/map.json                   the campus and its buildings (interiors use
                                                       the Saffron buildings' layouts); scripts.inc
                                                       kept if there
  data/maps/map_groups.json                            gMapGroup_LeaderAcademy
  data/maps/leader_academy.inc                         their scripts (data/event_scripts.s)
  data/layouts/Route11_Frlg/map.bin, Route11's map.json  the gap in the trees, the gate guards,
                                                       the connection north
"""
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))
import paldea                                   # noqa: E402  (read_layout)
from build_exotic_safari import obj, write_layout, write_map     # noqa: E402

W, H = 44, 40
OFFSET = 18                 # (the campus's x 0 is Route 11's x 18: its path, x 20-21, is the gap at 38-39)
GAP = (20, 21)
SAFFRON, _ = paldea.read_layout("LAYOUT_SAFFRON_CITY")

COBBLE, GRASS = 0x32E9, 0x3001
SIGN = 0x0402
# Trees (the general tileset's, as Saffron draws them): pairs (left, right) by kind
TREE = {
    "A": (0x0414, 0x0415), "B": (0x041C, 0x041D), "first": (0x041E, 0x041F), "bottom": (0x0424, 0x0425),
    "crown": (0x330E, 0x330F),
}
EDGE_L = {"A": 0x0416, "B": 0x041E, "first": 0x041E, "bottom": 0x0426}
EDGE_R = {"A": 0x0417, "B": 0x041F, "first": 0x041F, "bottom": 0x0427}

# Buildings: Saffron's rectangle (x0, y0, x1, y1, inclusive), and where its door is in it
BUILDINGS = {
    "hall":    {"src": (29, 16, 37, 30), "door": (4, 14), "at": (17, 4)},     # Silph Co.
    "center":  {"src": (22, 34, 26, 38), "door": (2, 4), "at": (7, 21)},      # Pokémon Center
    "supply":  {"src": (38, 18, 41, 21), "door": (2, 3), "at": (8, 28)},      # the mart
    "battle":  {"src": (37, 9, 42, 12), "door": (3, 3), "at": (30, 22)},      # the dojo
    "exchange": {"src": (41, 34, 45, 38), "door": (2, 4), "at": (30, 27)},    # a house
    "dorm_w":  {"src": (19, 25, 23, 30), "door": None, "at": (9, 6)},          # apartment blocks
    "dorm_e":  {"src": (19, 25, 23, 30), "door": None, "at": (29, 6)},
}


def campus():
    g = [[COBBLE] * W for _ in range(H)]
    forest = [[False] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            side = x < 4 or x >= W - 4
            top = y < 4
            bottom = y >= 36 and x not in GAP
            forest[y][x] = side or top or bottom
    # grass verges along the trees, and lawns
    for y in range(4, 36):
        for x in range(4, W - 4):
            if x in (4, 5, W - 6, W - 5) or y in (4, 34, 35):
                g[y][x] = GRASS
    for (x0, y0, x1, y1) in ((14, 26, 18, 31), (25, 26, 29, 31)):          # the quad's lawns
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                g[y][x] = GRASS
    for y in range(34, 40):     # (the path south, to Route 11)
        for x in GAP:
            g[y][x] = COBBLE
    # the trees
    A = (H - 1) % 2             # (rows of this parity are tree tops: the last row continues into Route 11)
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
                g[y][x] = EDGE_L[kind]
            elif not left and x < W - 1 and not forest[y][x + 1]:
                g[y][x] = EDGE_R[kind]
            else:
                g[y][x] = TREE[kind][0 if left else 1]
            if kind == "first" and y > 0 and not forest[y - 1][x]:
                g[y - 1][x] = TREE["crown"][0 if left else 1]
    # the buildings
    doors = {}
    for name, b in BUILDINGS.items():
        sx0, sy0, sx1, sy1 = b["src"]
        ax, ay = b["at"]
        for dy in range(sy1 - sy0 + 1):
            for dx in range(sx1 - sx0 + 1):
                g[ay + dy][ax + dx] = SAFFRON[sy0 + dy][sx0 + dx]
        if b["door"]:
            doors[name] = (ax + b["door"][0], ay + b["door"][1])
    # signs (by the doors)
    signs = {"hall": (19, 19), "center": (6, 25), "supply": (7, 31), "battle": (29, 25), "exchange": (29, 31), "gate": (22, 34)}
    for (x, y) in signs.values():
        g[y][x] = SIGN
    return g, doors, signs


def preview(g, out):
    import render_map as rm
    rm.set_version("frlg")
    prim = rm.Tileset(rm.tileset_dir("gTileset_General_Frlg"))
    sec = rm.Tileset(rm.tileset_dir("gTileset_SaffronCity"))
    from PIL import Image, ImageDraw
    img = Image.new("RGB", (W * 16, H * 16))
    for y in range(H):
        for x in range(W):
            rm.draw_metatile(img, x * 16, y * 16, g[y][x] & 0x3FF, prim, sec)
    img = img.resize((W * 32, H * 32), Image.NEAREST)
    d = ImageDraw.Draw(img)
    for x in range(W):
        d.text((x * 32 + 2, 0), str(x), fill=(255, 255, 0))
    for y in range(H):
        d.text((0, y * 32 + 2), str(y), fill=(255, 255, 0))
    img.save(out)


def lobj(local, gfx, x, y, script, movement="MOVEMENT_TYPE_FACE_DOWN", flag="0", rng=1):
    o = obj(gfx, x, y, script, movement, flag)
    o["local_id"] = local
    o["movement_range_x"] = o["movement_range_y"] = rng
    return o


def warp(x, y, dest, wid):
    return {"x": x, "y": y, "elevation": 0, "dest_map": dest, "dest_warp_id": str(wid)}


def sign(x, y, script):
    return {"type": "sign", "x": x, "y": y, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_NORTH", "script": script}


# The people: (local id suffix, gfx, x, y, script, movement)
CAMPUS_PEOPLE = [
    ("WELCOME_HEADMASTER", "OBJ_EVENT_GFX_OLD_WOMAN", 21, 33, "Academy_EventScript_Nothing", "MOVEMENT_TYPE_FACE_DOWN", "FLAG_HIDE_ACADEMY_WELCOME"),
    ("WELCOME_MENTOR", "OBJ_EVENT_GFX_VAR_0", 20, 37, "Academy_EventScript_Nothing", "MOVEMENT_TYPE_FACE_UP", "FLAG_HIDE_ACADEMY_MENTOR"),
    ("CANDIDATE_KOJI", "OBJ_EVENT_GFX_BLACK_BELT_FRLG", 13, 20, "Academy_EventScript_Koji", "MOVEMENT_TYPE_FACE_RIGHT", "0"),
    ("CANDIDATE_MAREN", "OBJ_EVENT_GFX_SWIMMER_F_LAND", 16, 26, "Academy_EventScript_Maren", "MOVEMENT_TYPE_WANDER_AROUND", "0"),
    ("CANDIDATE_KEOLA", "OBJ_EVENT_GFX_CAMPER_FRLG", 27, 29, "Academy_EventScript_Keola", "MOVEMENT_TYPE_WANDER_AROUND", "0"),
    ("CANDIDATE_RENE", "OBJ_EVENT_GFX_BEAUTY_FRLG", 25, 20, "Academy_EventScript_Rene", "MOVEMENT_TYPE_FACE_LEFT", "0"),
    ("STUDENT_1", "OBJ_EVENT_GFX_LASS_FRLG", 14, 13, "Academy_EventScript_Student1", "MOVEMENT_TYPE_WANDER_AROUND", "0"),
    ("STUDENT_2", "OBJ_EVENT_GFX_YOUNGSTER_FRLG", 33, 14, "Academy_EventScript_Student2", "MOVEMENT_TYPE_WANDER_AROUND", "0"),
]
HALL_PEOPLE = [
    ("HEADMASTER", "OBJ_EVENT_GFX_OLD_WOMAN", 8, 13, "Academy_EventScript_Headmaster", "MOVEMENT_TYPE_FACE_DOWN"),
    ("TEACHER_LARKSPUR", "OBJ_EVENT_GFX_GENTLEMAN_FRLG", 12, 5, "Academy_EventScript_Larkspur", "MOVEMENT_TYPE_FACE_DOWN"),
    ("TEACHER_CAMELLIA", "OBJ_EVENT_GFX_BEAUTY_FRLG", 18, 5, "Academy_EventScript_Camellia", "MOVEMENT_TYPE_FACE_DOWN"),
    ("TEACHER_FOXGLOVE", "OBJ_EVENT_GFX_SCIENTIST", 24, 5, "Academy_EventScript_Foxglove", "MOVEMENT_TYPE_FACE_DOWN"),
    ("TEACHER_VERBENA", "OBJ_EVENT_GFX_COOLTRAINER_M", 28, 9, "Academy_EventScript_Verbena", "MOVEMENT_TYPE_FACE_LEFT"),
    ("TEACHER_MARIGOLD", "OBJ_EVENT_GFX_WOMAN_3_FRLG", 27, 16, "Academy_EventScript_Marigold", "MOVEMENT_TYPE_FACE_LEFT"),
    ("TEACHER_HYACINTH", "OBJ_EVENT_GFX_COOLTRAINER_F", 14, 17, "Academy_EventScript_Hyacinth", "MOVEMENT_TYPE_FACE_DOWN"),
    ("TEACHER_YARROW", "OBJ_EVENT_GFX_GENTLEMAN_FRLG", 21, 18, "Academy_EventScript_Yarrow", "MOVEMENT_TYPE_FACE_DOWN"),
    ("TEACHER_TANSY", "OBJ_EVENT_GFX_LASS_FRLG", 4, 10, "Academy_EventScript_Tansy", "MOVEMENT_TYPE_FACE_DOWN"),
    ("CANDIDATE_NADIA", "OBJ_EVENT_GFX_COOLTRAINER_F", 30, 5, "Academy_EventScript_Nadia", "MOVEMENT_TYPE_FACE_LEFT"),
]
MAPS = {
    # name: (layout, music, warps back to the campus (x, y)s, its door warp on the campus)
    "LeaderAcademy_Hall": ("LAYOUT_SILPH_CO_1F", "MUS_RG_OAK_LAB", [(7, 21), (8, 20), (9, 20)], "hall"),
    "LeaderAcademy_PokemonCenter": ("LAYOUT_POKEMON_CENTER_1F_FRLG", "MUS_RG_POKE_CENTER", [(6, 8), (7, 8), (8, 8)], "center"),
    "LeaderAcademy_Supply": ("LAYOUT_MART_FRLG", "MUS_RG_POKE_CENTER", [(3, 7), (4, 7), (5, 7)], "supply"),
    "LeaderAcademy_BattleHall": ("LAYOUT_SAFFRON_CITY_DOJO", "MUS_RG_PEWTER", [(5, 14), (6, 14), (7, 14)], "battle"),
    "LeaderAcademy_Exchange": ("LAYOUT_MART_FRLG", "MUS_RG_POKE_CENTER", [(3, 7), (4, 7), (5, 7)], "exchange"),
}
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


def mid(name):
    return "MAP_" + {"LeaderAcademy_Campus": "LEADER_ACADEMY_CAMPUS", "LeaderAcademy_Hall": "LEADER_ACADEMY_HALL",
                     "LeaderAcademy_PokemonCenter": "LEADER_ACADEMY_POKEMON_CENTER", "LeaderAcademy_Supply": "LEADER_ACADEMY_SUPPLY",
                     "LeaderAcademy_BattleHall": "LEADER_ACADEMY_BATTLE_HALL", "LeaderAcademy_Exchange": "LEADER_ACADEMY_EXCHANGE"}[name]


def write_campus(g, doors, signs):
    write_layout(CAMPUS, "LAYOUT_LEADER_ACADEMY_CAMPUS", g, "gTileset_General_Frlg", "gTileset_SaffronCity",
                 border=[[0x0414, 0x0415], [0x041C, 0x041D]])
    order = [m for m in MAPS]
    warps = [warp(*doors[MAPS[m][3]], mid(m), 0) for m in order]
    objects = [lobj("LOCALID_ACADEMY_" + p[0], p[1], p[2], p[3], p[4], p[5], p[6], 2 if "WANDER" in p[5] else 1)
               for p in CAMPUS_PEOPLE]
    bgs = [sign(x, y, SIGN_SCRIPTS[k]) for k, (x, y) in signs.items()]
    write_map(CAMPUS, mid(CAMPUS), "LAYOUT_LEADER_ACADEMY_CAMPUS", "MUS_RG_CELADON", "MAPSEC_ROUTE_11", "MAP_TYPE_CITY",
              objects, warps, bgs, True)
    j = json.loads((ROOT / f"data/maps/{CAMPUS}/map.json").read_text())
    j["connections"] = [{"map": "MAP_ROUTE11", "offset": -OFFSET, "direction": "down"}]
    j["weather"] = "WEATHER_SUNNY"
    (ROOT / f"data/maps/{CAMPUS}/map.json").write_text(json.dumps(j, indent=2) + "\n")
    for i, m in enumerate(order):
        layout, music, back, _ = MAPS[m]
        ws = [warp(x, y, mid(CAMPUS), i) for (x, y) in back]
        people = [lobj("LOCALID_ACADEMY_" + p[0], p[1], p[2], p[3], p[4], p[5]) for p in INTERIOR_PEOPLE.get(m, [])]
        write_map(m, mid(m), layout, music, "MAPSEC_ROUTE_11", "MAP_TYPE_INDOOR", people, ws, [], False)


def write_route11():
    """The gap in Route 11's northern trees (x 38-39), its gate guards, and the connection north"""
    lj = json.loads((ROOT / "data/layouts/layouts.json").read_text())
    lay = next(l for l in lj["layouts"] if l.get("id") == "LAYOUT_ROUTE11")
    w = lay["width"]
    path = ROOT / lay["blockdata_filepath"]
    data = bytearray(path.read_bytes())
    for y in (0, 1):
        for x in (OFFSET + GAP[0], OFFSET + GAP[1]):
            struct.pack_into("<H", data, (y * w + x) * 2, 0x3001)      # (short grass)
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
    names = [CAMPUS] + list(MAPS)
    if "gMapGroup_LeaderAcademy" not in groups["group_order"]:
        groups["group_order"].append("gMapGroup_LeaderAcademy")
    groups["gMapGroup_LeaderAcademy"] = names
    gp.write_text(json.dumps(groups, indent=2) + "\n")
    (ROOT / "data/maps/leader_academy.inc").write_text(
        "@ The Leader Academy's maps (tools/gym_hack/build_academy.py)\n"
        + '\t.include "data/scripts/leader_academy.inc"\n'
        + "".join(f'\t.include "data/maps/{n}/scripts.inc"\n' for n in names))


if __name__ == "__main__":
    g, doors, signs = campus()
    if len(sys.argv) > 1:
        preview(g, sys.argv[1])
    write_campus(g, doors, signs)
    write_route11()
    write_group()
    print("academy:", doors)
