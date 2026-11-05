#!/usr/bin/env python3
"""The Universal Pokémon Medical Center (Pokémon Canvas): Vermilion City's building site, once the
foreman's had his four helpers (src/medical_center.c).

    python3 tools/gym_hack/build_medical_center.py [preview_dir]

Writes:
  data/tilesets/secondary/medical_center_frlg/   gTileset_MedicalCenter: the Cinnabar Lab's walls,
      windows, doors, cabinets and computers, the Pokémon Center's escalators (and the same turned
      left-right), and the art in tools/gym_hack/medical_center_art.py
  data/layouts/VermilionCity_MedicalCenter_{1F,2F,3F}/, layouts.json
  data/maps/VermilionCity_MedicalCenter_{1F,2F,3F}/map.json (scripts.inc is written once, then kept)
  data/maps/map_groups.json, data/event_scripts.s
  Vermilion City: the building's art at the end of its tileset (palette 11, free there), the script
      that puts it on the lot (data/maps/VermilionCity_Frlg/medical_center.inc), the door's warp
"""
import json
import struct
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))
import medical_center_art as art                                    # noqa: E402
from build_exotic_safari import MB, obj, write_layout, write_map, write_pal   # noqa: E402
from tileset_composer import LAYER_COVERED, LAYER_NORMAL, NPRIM, Composer, _read_attr, _read_mt   # noqa: E402

PRIMARY = "gTileset_BuildingFrlg"
LAB = "gTileset_Lab_Frlg"
PC = "gTileset_PokemonCenterFrlg"
W, H = 17, 13
FLOORS = ["1F", "2F", "3F"]
NAME = "VermilionCity_MedicalCenter_{}"
MAP_ID = "MAP_VERMILION_CITY_MEDICAL_CENTER_{}"
LAYOUT_ID = "LAYOUT_VERMILION_CITY_MEDICAL_CENTER_{}"
GROUP = "gMapGroup_IndoorVermilion_Frlg"
CITY = "VermilionCity_Frlg"

# Vermilion City: the lot (the building's top-left, in blocks) and its door
LOT = (32, 8)
CITY_WARP = 10                     # (the door's warp: the city's 11th)
CITY_TILESET = ROOT / "data/tilesets/secondary/vermilion_city_frlg"
CITY_ORIG_TILES, CITY_ORIG_METATILES = 144, 168      # (FireRed's own: the building's art goes after)
CITY_PAL = 11


def blk(m, coll, elev=3):
    return m | (coll << 10) | (elev << 12)


def mirror(c, mid):
    """a metatile turned left-right (a new one)"""
    e = c.metatiles[mid - 0x280]
    flip = lambda v: v ^ 0x400 if v & 0x3FF else v
    out = [flip(e[1]), flip(e[0]), flip(e[3]), flip(e[2]), flip(e[5]), flip(e[4]), flip(e[7]), flip(e[6])]
    return c.add_metatile(out, c.attrs[mid - 0x280])


class Floorplan:
    def __init__(self, w=None, h=None):
        self.w, self.h = w or W, h or H
        W_, H_ = self.w, self.h
        self.g = [[blk(0x081, 0)] * W_ for _ in range(H_)]
        # the walls: three rows, the lab's
        for x in range(W_):
            self.g[0][x] = blk(0x069, 1, 0)
            self.g[1][x] = blk(0x071, 1, 0)
            self.g[2][x] = blk(0x079, 0)
        self.g[0][0], self.g[0][W_ - 1] = blk(0x068, 1, 0), blk(0x06A, 1, 0)
        self.g[1][0], self.g[1][W_ - 1] = blk(0x070, 1, 0), blk(0x072, 1, 0)
        self.g[2][0], self.g[2][W_ - 1] = blk(0x078, 0), blk(0x07A, 0)
        for y in range(3, H_ - 1):
            self.g[y][0] = blk(0x082, 0)
        for x in range(W_):
            self.g[H_ - 1][x] = blk(0x008, 1, 0)

    def put(self, x, y, m, coll=1, elev=None):
        self.g[y][x] = blk(m, coll, (0 if coll else 3) if elev is None else elev)

    def raw(self, x, y, v):
        self.g[y][x] = v

    def piece(self, grid, x, y, walk=()):
        """a grid of metatiles (art), solid but for `walk` (dx, dy)"""
        for dy, row in enumerate(grid):
            for dx, m in enumerate(row):
                coll = 0 if (dx, dy) in walk else 1
                self.put(x + dx, y + dy, m, coll)

    def one(self, P, name, x, y):
        """a 1 x 1 piece"""
        self.put(x, y, P[name][0][0], 1)


def build_tileset():
    c = Composer(PRIMARY)
    slot = c.add_palette(art.ART)
    P = {}
    floor, wall_mid, wall_low = 0x081, 0x071, 0x079

    def floor_piece(name, behind_rows=0, behavior=None):
        return c.art(art.PIECES[name]().im, slot, ground=floor, behind_rows=behind_rows, behavior=behavior)

    P["bed"] = floor_piece("bed")
    P["iv"] = floor_piece("iv", behind_rows=1)
    P["monitor"] = floor_piece("monitor", behind_rows=1)
    P["microscope"] = floor_piece("microscope")
    P["curtain"] = floor_piece("curtain", behind_rows=1)
    for k in ("partition", "wheelchair", "cart", "boxes"):
        P[k] = floor_piece(k)
    # on the wall: its two lower rows (the mid row's ground, then the low row's)
    P["xray"] = c.art(art.PIECES["xray"]().im, slot, ground=wall_mid, under={(0, 1): wall_low, (1, 1): wall_low})
    P["cabinet"] = c.art(art.PIECES["cabinet"]().im, slot, ground=wall_mid, under={(0, 1): wall_low, (1, 1): wall_low})
    P["cross"] = c.art(art.PIECES["cross"]().im, slot, ground=wall_mid)
    P["plaque"] = c.art(art.PIECES["plaque"]().im, slot, ground=wall_mid)
    P["counter"] = c.art(counter_img(), slot, ground=floor, behavior={(i, 0): "MB_COUNTER" for i in range(5)})
    # the lab's: windows, a framed picture, plants, doors, cabinets, computers, desks and stools
    cp = lambda m: c.copy(LAB, m)
    P["window"] = [cp(0x314), cp(0x31C)]
    P["picture"] = [cp(0x315), cp(0x31D)]
    P["plant"] = [cp(0x2BF), cp(0x2C7)]
    P["door"] = [[cp(0x2BD), cp(0x2BE)], [cp(0x2C5), cp(0x2C6)]]
    P["display"] = [[cp(0x2F9), cp(0x2FA)], [cp(0x301), cp(0x302)], [cp(0x309), cp(0x30A)]]
    P["lowcab"] = [[cp(0x2FB), cp(0x2FC)], [0x073, 0x074], [0x07B, 0x07C]]
    P["computer"] = [cp(0x304), 0x062, cp(0x30B)]
    P["desk"] = [[0x06F, 0x08F], [0x077, cp(0x300)], [0x07F, cp(0x308)], [0x0A7, 0x0A7]]
    P["stool"] = cp(0x2CB)
    # the Pokémon Center's escalators: up (1F's, on the left), down (2F's, on the left), and both
    # turned to stand on the right
    def cpc(m):
        """a Pokémon Center metatile over the lab's floor (theirs is beige)"""
        new = c.copy(PC, m)
        c.metatiles[new - 0x280][:4] = c.ground(floor)
        return new
    P["esc_up_l"] = [[cpc(0x2D0), cpc(0x2D1)], [cpc(0x2D8), cpc(0x2D9)], [cpc(0x2E0), cpc(0x2E1)]]
    P["esc_down_l"] = [[cpc(0x2DB), cpc(0x2DC)], [cpc(0x2E3), cpc(0x2E4)], [cpc(0x2EB), cpc(0x2EC)]]
    P["esc_up_r"] = [[mirror(c, r[1]), mirror(c, r[0])] for r in P["esc_up_l"]]
    P["esc_down_r"] = [[mirror(c, r[1]), mirror(c, r[0])] for r in P["esc_down_l"]]
    P["pc_bench"] = [[cpc(0x2F5)], [cpc(0x2FD)]]
    c.write("MedicalCenter", "medical_center_frlg")
    return c, P


def counter_img():
    """5 x 1: the reception desk (white, a teal front)"""
    im = art.Img(80, 16)
    im.box(0, 1, 79, 6, art.W)
    im.hline(1, 78, 2, art.LG)
    im.box(0, 6, 79, 15, art.PB)
    im.rect(1, 7, 78, 14, art.B)
    for x in range(8, 80, 16):
        im.vline(x, 8, 13, art.DB)
    im.rect(36, 9, 43, 12, art.W)              # a red cross on the front
    im.rect(38, 8, 41, 13, art.W)
    im.rect(39, 9, 40, 12, art.R)
    im.rect(37, 10, 42, 11, art.R)
    return im.im


# ---------------------------------------------------------------------------------------------
# The floors
# ---------------------------------------------------------------------------------------------
def put_escalator(f, P, kind, y=5):
    """an escalator (2 x 3) against the left or right wall, its top row at y -> the step you ride"""
    x = 0 if kind.endswith("_l") else f.w - 2
    blocks = P["esc_" + kind]
    for dy in range(3):
        for dx in range(2):
            ride = (dy == 1 and ((dx == 1) == kind.endswith("_l")))       # (the step you ride: next to the floor)
            if ride:
                f.raw(x + dx, y + dy, blk(blocks[dy][dx], 0, 4))
            elif dy == 1:
                f.raw(x + dx, y + dy, blk(blocks[dy][dx], 1, 0))
            else:
                f.raw(x + dx, y + dy, blk(blocks[dy][dx], 0, 3))
    rx = x + (1 if kind.endswith("_l") else 0)
    # the floor in front is at the escalator's height (4), the floor around that a ramp (0), as in
    # FireRed's Pokémon Centers: otherwise the player can't step on
    step = 1 if kind.endswith("_l") else -1
    fx, fy = rx + step, y + 1
    f.raw(fx, fy, (f.g[fy][fx] & 0x0FFF) | (4 << 12))
    for (ax, ay) in ((fx, fy - 1), (fx, fy + 1), (fx + step, fy)):
        if not (f.g[ay][ax] >> 10) & 3:
            f.raw(ax, ay, f.g[ay][ax] & 0x0FFF)
    return (rx, y + 1)


def mat(f, x0):
    """the door mat out: its warps"""
    for i, m in enumerate((0x088, 0x089, 0x08A)):
        f.put(x0 + i, f.h - 2, m, 0, 3)
    for i, m in enumerate((0x090, 0x091, 0x092)):
        f.put(x0 + i, f.h - 1, m, 1, 0)
    return [(x0 + i, f.h - 2) for i in range(3)]


def door(f, P, x):
    """a door in the back wall (the lab's), x its left; -> the tile you walk into (its warp)"""
    f.piece(P["door"], x, 0, walk={(0, 1)})
    f.put(x, 1, P["door"][1][0], 0, 3)
    return (x, 1)


def plant(f, P, x, y):
    f.piece([[P["plant"][0]], [P["plant"][1]]], x, y)


def benches(f, P, x, y, n):
    for i in range(n):
        f.piece(P["pc_bench"], x + i, y)


# 1F (21 x 15): reception, a pharmacy, the ER (six beds, six patients), a waiting area. The escalator
# up on the left.
W1, H1 = 21, 15
TRIAGE_BEDS = [(13, 6), (15, 6), (17, 6), (13, 10), (15, 10), (17, 10)]   # (the bed's head; the patient on its foot)


def floor_1f(P):
    f = Floorplan(W1, H1)
    f.put(3, 1, P["window"][1])
    f.put(10, 1, P["window"][1])
    f.put(19, 1, P["window"][1])
    f.piece(P["plaque"], 5, 1)
    f.piece(P["cross"], 7, 1)
    f.piece(P["counter"], 4, 3)                       # (reception)
    f.piece(P["cabinet"], 12, 1)                      # (the pharmacy: its cabinets, its counter)
    f.piece(P["cabinet"], 15, 1)
    f.piece(P["counter"], 12, 3)
    plant(f, P, 1, 1)
    for (x, y) in TRIAGE_BEDS:
        f.piece(P["bed"], x, y)
    for (x, y) in ((14, 6), (16, 6), (14, 10), (16, 10)):
        f.piece(P["curtain"], x, y)
    f.piece(P["monitor"], 19, 6)
    f.piece(P["iv"], 19, 10)
    benches(f, P, 4, 8, 4)                           # (the waiting area)
    benches(f, P, 4, 11, 4)
    f.one(P, "wheelchair", 2, 12)
    rx = put_escalator(f, P, "up_l")
    exits = mat(f, 9)
    return f, {"escalator": rx, "exits": exits}


# 2F (27 x 15): the wards. A nurses' station and a waiting area; three patient rooms off the back
# wall; and runaway gurneys in the way of the escalator up to the 3F (the box on the right). The
# puzzle (an 11 x 7 box, walls around it: the player comes in at S, out at T) was picked by a search
# for one that takes some thought: 5 pushes, 4 gurneys moved (tools/gym_hack/medical_center_puzzle.py
# checks it on the map). '#' inside: wheelchairs, carts, boxes.
W2, H2 = 27, 15
PUZZLE_AT = (13, 7)                # (the box's top-left on the map)
PUZZLE = [
    "###########",
    "S.G......##",
    "##G.#.....#",
    "#....G.##G#",
    "###...#.G.T",
    "#.#..#..G.#",
    "###########",
]
OBSTACLES = ["wheelchair", "cart", "boxes"]
GURNEYS = [(PUZZLE_AT[0] + x, PUZZLE_AT[1] + y) for y, r in enumerate(PUZZLE) for x, c in enumerate(r) if c == "G"]
PAST_GURNEYS = (PUZZLE_AT[0] + 9, PUZZLE_AT[1] + 4)   # (the box's last tile before its way out: every way through crosses it)
ORDERLY = (11, 9)
WARD_NURSE = (24, 13)
ROOM_DOORS = [4, 9, 18]            # (the doors' left columns)
ESC_UP_2F_Y = 9


def floor_2f(P):
    f = Floorplan(W2, H2)
    doors = [door(f, P, x) for x in ROOM_DOORS]
    for x in (7, 13, 22):
        f.put(x, 1, P["window"][1])
    f.piece(P["cross"], 15, 1)
    f.piece(P["counter"], 5, 7)                      # (the nurses' station)
    f.piece(P["cabinet"], 24, 1)
    benches(f, P, 2, 10, 3)                          # (the waiting areas)
    benches(f, P, 21, 4, 3)
    plant(f, P, 1, 1)
    plant(f, P, 12, 1)
    plant(f, P, 20, 1)
    zx, zy = PUZZLE_AT
    k = 0
    for y, row in enumerate(PUZZLE):
        for x, ch in enumerate(row):
            mx, my = zx + x, zy + y
            if ch != "#":
                continue
            border = y in (0, len(PUZZLE) - 1) or x in (0, len(row) - 1)
            if border:
                f.one(P, "partition", mx, my)
            else:
                f.one(P, OBSTACLES[k % len(OBSTACLES)], mx, my)
                k += 1
    # the corner beyond the box (the escalator up, the nurse with the HEAL CHARM): closed off above
    for x in range(zx + len(PUZZLE[0]), W2):
        f.one(P, "partition", x, zy)
    rd = put_escalator(f, P, "down_l")
    ru = put_escalator(f, P, "up_r", ESC_UP_2F_Y)
    return f, {"down": rd, "up": ru, "doors": doors}


# The patient rooms off the 2F (11 x 9 each): a bed with a patient, a monitor, an IV, visitors.
WR, HR = 11, 9
ROOMS = ["201", "202", "203"]


def floor_room(P, i):
    f = Floorplan(WR, HR)
    f.put(3, 1, P["window"][1])
    f.put(7, 1, P["window"][1])
    f.piece(P["bed"], 5, 3)
    f.piece(P["monitor"], 6, 3)
    f.piece(P["iv"], 4, 3)
    f.piece(P["curtain"], 9, 3) if i == 1 else f.piece(P["cabinet"], 8, 1)
    plant(f, P, 1, 1)
    f.one(P, "wheelchair", 1, 6) if i == 1 else f.one(P, "cart", 9, 6)
    exits = mat(f, 4)
    return f, {"exits": exits}


# 3F (17 x 13): the research lab. Five microscopes on their benches (the slides: src/medical_center.c),
# the head doctor, the head nurse (the physical, then Pokérus), an X-ray, a cabinet, ICU beds.
MICROSCOPES = [(3, 5), (5, 5), (7, 5), (9, 5), (11, 5)]


def floor_3f(P):
    f = Floorplan(W, H)
    f.piece(P["xray"], 2, 1)
    f.piece(P["cabinet"], 9, 1)
    f.piece(P["cross"], 6, 1)
    f.put(12, 1, P["window"][1])
    for (x, y) in MICROSCOPES:
        f.piece(P["microscope"], x, y)
    for (x, y) in ((2, 9), (4, 9), (6, 9)):
        f.piece(P["bed"], x, y)
        f.piece(P["monitor"], x + 1, y)
    f.piece(P["display"], 13, 0)
    rd = put_escalator(f, P, "down_r")
    return f, {"down": rd}


def preview(floors, out):
    import render_map as rm
    from PIL import ImageDraw
    rm.set_version("frlg")
    prim = rm.Tileset(rm.tileset_dir(PRIMARY))
    sec = rm.Tileset(rm.tileset_dir("gTileset_MedicalCenter"))
    out.mkdir(parents=True, exist_ok=True)
    for fl, (f, where) in floors.items():
        img = Image.new("RGB", (f.w * 16, f.h * 16))
        for y in range(f.h):
            for x in range(f.w):
                rm.draw_metatile(img, x * 16, y * 16, f.g[y][x] & 0x3FF, prim, sec)
        img = img.resize((f.w * 32, f.h * 32), Image.NEAREST)
        d = ImageDraw.Draw(img)
        m = json.loads((ROOT / f"data/maps/{NAME.format(fl)}/map.json").read_text())
        for o in m["object_events"]:
            d.rectangle((o["x"] * 32 + 4, o["y"] * 32 + 4, o["x"] * 32 + 28, o["y"] * 32 + 28), outline=(255, 0, 255), width=2)
            d.text((o["x"] * 32 + 6, o["y"] * 32 + 8), o["graphics_id"].split("(")[-1][:5], fill=(255, 0, 255))
        for wp in m["warp_events"]:
            d.rectangle((wp["x"] * 32 + 10, wp["y"] * 32 + 10, wp["x"] * 32 + 22, wp["y"] * 32 + 22), outline=(0, 160, 255), width=2)
        img.save(out / f"{fl}.png")


def sign(x, y, script, facing="BG_EVENT_PLAYER_FACING_NORTH"):
    return {"type": "sign", "x": x, "y": y, "elevation": 0, "player_facing_dir": facing, "script": script}


def mon(species, x, y, script, movement="MOVEMENT_TYPE_FACE_DOWN", flag="0", shiny=False):
    return obj(f"OBJ_EVENT_GFX_SPECIES{'_SHINY' if shiny else ''}({species})", x, y, script, movement, flag)


def warp(x, y, dest, wid, elev=3):
    return {"x": x, "y": y, "elevation": elev, "dest_map": dest, "dest_warp_id": str(wid)}


PATIENTS = ["PSYDUCK", "RATTATA", "PIDGEY", "JIGGLYPUFF", "CHARMANDER", "SPINDA"]
WARP_1F_ESC = 3                    # (the 1F's warps: the mat's three, then the escalator)
WARP_2F_ROOMS = 2                  # (the 2F's: down, up, then the rooms' doors)


def objects_1f(where):
    P = "VermilionCity_MedicalCenter_1F_EventScript_"
    o = [obj("OBJ_EVENT_GFX_NURSE_FRLG", 6, 2, P + "Receptionist"),
         mon("CHANSEY", 7, 2, P + "Chansey"),
         obj("OBJ_EVENT_GFX_MART_EMPLOYEE", 14, 2, P + "Pharmacist"),
         obj("OBJ_EVENT_GFX_NURSE_FRLG", 12, 8, P + "TriageNurse", "MOVEMENT_TYPE_FACE_RIGHT")]
    for i, (x, y) in enumerate(TRIAGE_BEDS):
        o.append(mon(PATIENTS[i], x, y + 1, P + f"Patient{i}", "MOVEMENT_TYPE_FACE_DOWN"))
    o += [obj("OBJ_EVENT_GFX_OLD_MAN_1", 5, 7, P + "OldMan", "MOVEMENT_TYPE_FACE_DOWN"),
          obj("OBJ_EVENT_GFX_LITTLE_GIRL_FRLG", 7, 10, P + "Girl", "MOVEMENT_TYPE_FACE_UP"),
          mon("HAPPINY", 8, 10, P + "Happiny", "MOVEMENT_TYPE_WANDER_AROUND"),
          obj("OBJ_EVENT_GFX_WOMAN_1_FRLG", 4, 13, P + "Woman", "MOVEMENT_TYPE_FACE_UP"),
          obj("OBJ_EVENT_GFX_YOUNGSTER_FRLG", 2, 4, P + "Youngster", "MOVEMENT_TYPE_WANDER_AROUND"),
          # the foreman and his crew, who built it
          obj("OBJ_EVENT_GFX_OLD_MAN_2", 13, 13, P + "Foreman", "MOVEMENT_TYPE_FACE_LEFT"),
          mon("MACHOP", 14, 13, P + "Machop"),
          mon("MACHOKE", 15, 13, P + "Machoke"),
          mon("MACHAMP", 16, 13, P + "Machamp"),
          mon("MACHAMP", 17, 13, P + "ShinyMachamp", shiny=True)]
    warps = [warp(x, y, "MAP_VERMILION_CITY", CITY_WARP) for (x, y) in where["exits"]]
    warps.append(warp(*where["escalator"], MAP_ID.format("2F"), 0, 4))
    bgs = [sign(12, 2, P + "Cabinet"), sign(13, 2, P + "Cabinet"), sign(15, 2, P + "Cabinet"), sign(16, 2, P + "Cabinet"),
           sign(5, 2, P + "Plaque"), sign(6, 2, P + "Plaque")]
    return o, warps, bgs


def objects_2f(where):
    P = "VermilionCity_MedicalCenter_2F_EventScript_"
    # (through the gurneys: the nurse gives the HEAL CHARM straight away, and the orderlies park them,
    # so they're not in the way when the player comes back down from the 3F)
    coords = [{"type": "trigger", "x": PAST_GURNEYS[0], "y": PAST_GURNEYS[1], "elevation": 3,
               "var": "VAR_TEMP_1", "var_value": "0", "script": P + "ThroughGurneys"}]
    o = [obj("OBJ_EVENT_GFX_GURNEY", x, y, "0x0", "MOVEMENT_TYPE_NONE", "FLAG_GOT_HEAL_CHARM") for (x, y) in GURNEYS]
    for g in o:                                   # (no movement range: like a boulder, it goes where it's pushed)
        g["movement_range_x"] = g["movement_range_y"] = 0
    o += [obj("OBJ_EVENT_GFX_NURSE_FRLG", *WARD_NURSE, P + "WardNurse", "MOVEMENT_TYPE_FACE_UP"),
          obj("OBJ_EVENT_GFX_SCIENTIST", *ORDERLY, P + "Orderly", "MOVEMENT_TYPE_FACE_RIGHT"),
          obj("OBJ_EVENT_GFX_NURSE_FRLG", 7, 6, P + "StationNurse", "MOVEMENT_TYPE_FACE_DOWN"),
          mon("CHANSEY", 9, 6, P + "Chansey", "MOVEMENT_TYPE_FACE_DOWN"),
          obj("OBJ_EVENT_GFX_GENTLEMAN_FRLG", 3, 9, P + "Gentleman", "MOVEMENT_TYPE_FACE_DOWN"),
          obj("OBJ_EVENT_GFX_LASS_FRLG", 4, 12, P + "Lass", "MOVEMENT_TYPE_FACE_UP"),
          mon("MEOWTH", 5, 12, P + "Meowth", "MOVEMENT_TYPE_FACE_LEFT"),
          obj("OBJ_EVENT_GFX_OLD_WOMAN_FRLG", 22, 3, P + "OldWoman", "MOVEMENT_TYPE_FACE_DOWN"),
          obj("OBJ_EVENT_GFX_FISHER", 16, 4, P + "Fisher", "MOVEMENT_TYPE_WANDER_AROUND")]
    warps = [warp(*where["down"], MAP_ID.format("1F"), WARP_1F_ESC, 4), warp(*where["up"], MAP_ID.format("3F"), 0, 4)]
    warps += [warp(x, y, MAP_ID.format(ROOMS[i]), 1, 3) for i, (x, y) in enumerate(where["doors"])]   # (the mat's middle)
    bgs = []
    for i, x in enumerate(ROOM_DOORS):
        bgs.append(sign(x + 1, 1, P + f"RoomSign{i}"))
    return o, warps, bgs, coords


ROOM_PEOPLE = [
    # (patient, its spot on the bed; the visitors)
    ("PIKACHU", [("OBJ_EVENT_GFX_LITTLE_BOY_FRLG", 4, 5, "FACE_UP"), ("OBJ_EVENT_GFX_NURSE_FRLG", 7, 4, "FACE_LEFT")]),
    ("GROWLITHE", [("OBJ_EVENT_GFX_OLD_WOMAN_FRLG", 7, 5, "FACE_LEFT"), ("OBJ_EVENT_GFX_SCIENTIST", 3, 5, "FACE_RIGHT")]),
    ("TOGEPI", [("OBJ_EVENT_GFX_WOMAN_1_FRLG", 7, 4, "FACE_LEFT"), ("OBJ_EVENT_GFX_SPECIES(CHANSEY)", 3, 5, "FACE_RIGHT")]),
]


def objects_room(where, i):
    P = f"VermilionCity_MedicalCenter_{ROOMS[i]}_EventScript_"
    patient, people = ROOM_PEOPLE[i]
    o = [mon(patient, 5, 4, P + "Patient", "MOVEMENT_TYPE_FACE_DOWN")]
    for k, (gfx, x, y, face) in enumerate(people):
        o.append(obj(gfx, x, y, P + f"Visitor{k}", "MOVEMENT_TYPE_" + face))
    warps = [warp(x, y, MAP_ID.format("2F"), WARP_2F_ROOMS + i) for (x, y) in where["exits"]]
    return o, warps, []


def objects_3f(where):
    P = "VermilionCity_MedicalCenter_3F_EventScript_"
    o = [obj("OBJ_EVENT_GFX_SCIENTIST", 7, 3, P + "Doctor", "MOVEMENT_TYPE_FACE_DOWN"),
         obj("OBJ_EVENT_GFX_NURSE_FRLG", 12, 8, P + "HeadNurse", "MOVEMENT_TYPE_FACE_DOWN"),
         mon("CHANSEY", 11, 8, P + "Chansey"),
         mon("BLISSEY", 13, 8, P + "Blissey"),
         mon("SNORLAX", 2, 10, P + "Snorlax", "MOVEMENT_TYPE_FACE_UP")]
    warps = [warp(*where["down"], MAP_ID.format("2F"), 1, 4)]
    bgs = [sign(x, y + 1, P + f"Microscope{i}") for i, (x, y) in enumerate(MICROSCOPES)]
    bgs += [sign(2, 3, P + "XRay"), sign(3, 3, P + "XRay")]
    return o, warps, bgs


ALL_MAPS = FLOORS + ROOMS


def write_maps(P):
    floors = {"1F": floor_1f(P), "2F": floor_2f(P), "3F": floor_3f(P)}
    makers = {"1F": objects_1f, "2F": objects_2f, "3F": objects_3f}
    for i, r in enumerate(ROOMS):
        floors[r] = floor_room(P, i)
        makers[r] = (lambda i: lambda where: objects_room(where, i))(i)
    for fl, (f, where) in floors.items():
        name = NAME.format(fl)
        write_layout(name, LAYOUT_ID.format(fl), f.g, PRIMARY, "gTileset_MedicalCenter", border=0x008 | (1 << 10))
        o, warps, bgs, *coords = makers[fl](where)
        write_map(name, MAP_ID.format(fl), LAYOUT_ID.format(fl), "MUS_RG_POKE_CENTER", "MAPSEC_VERMILION_CITY",
                  "MAP_TYPE_INDOOR", o, warps, bgs, False, coords=coords[0] if coords else ())
    write_group()
    return floors


def write_group():
    gp = ROOT / "data/maps/map_groups.json"
    groups = json.loads(gp.read_text())
    g = groups[GROUP]
    for fl in ALL_MAPS:
        if NAME.format(fl) not in g:
            g.append(NAME.format(fl))
    gp.write_text(json.dumps(groups, indent=2) + "\n")
    es = ROOT / "data/event_scripts.s"
    s = es.read_text()
    anchor = '\t.include "data/maps/VermilionCity_PokemonCenter_2F_Frlg/scripts.inc"\n'
    for fl in reversed(ALL_MAPS):
        inc = f'\t.include "data/maps/{NAME.format(fl)}/scripts.inc"\n'
        if inc not in s:
            s = s.replace(anchor, anchor + inc)
    inc = '\t.include "data/scripts/medical_center_lot.inc"\n'
    if inc not in s:
        s = s.replace(anchor, anchor + inc)
    es.write_text(s)
    # Vermilion City: the door's warp (at the end: the others keep their numbers)
    mp = ROOT / f"data/maps/{CITY}/map.json"
    j = json.loads(mp.read_text())
    door_at = (LOT[0] + art.EXT_DOOR[0], LOT[1] + art.EXT_DOOR[1])
    ws = [w for w in j["warp_events"] if w["dest_map"] != MAP_ID.format("1F")]
    assert len(ws) == CITY_WARP, len(ws)
    ws.append(warp(*door_at, MAP_ID.format("1F"), 1, 0))                        # (the mat's middle)
    j["warp_events"] = ws
    for o in j["object_events"]:                       # (the foreman and his MACHOP: gone once it's built)
        if o["script"] in ("VermilionCity_EventScript_OldMan2", "VermilionCity_EventScript_Machop",
                           "VermilionCity_EventScript_Foreman"):
            o["flag"] = "FLAG_MEDCENTER_OPEN"
            if o["script"] == "VermilionCity_EventScript_OldMan2":
                o["script"] = "VermilionCity_EventScript_Foreman"
    mp.write_text(json.dumps(j, indent=2) + "\n")


# ---------------------------------------------------------------------------------------------
# Vermilion City: the building
# ---------------------------------------------------------------------------------------------
def build_exterior():
    """the building's art at the end of Vermilion's tileset (FireRed's own first: rerunning starts over)
    -> the grid of its metatiles (EXT_W x EXT_H), the door's"""
    from tileset_composer import _flip
    d = CITY_TILESET
    tiles_img = Image.open(d / "tiles.png")
    per_row = tiles_img.width // 8
    tiles = []
    for i in range(CITY_ORIG_TILES):
        x, y = (i % per_row) * 8, (i // per_row) * 8
        tiles.append(tuple(tiles_img.crop((x, y, x + 8, y + 8)).getdata()))
    index = {}
    mt = _read_mt(d)[:CITY_ORIG_METATILES]
    attrs = _read_attr(d)[:CITY_ORIG_METATILES]
    gpmt = _read_mt(ROOT / "data/tilesets/primary/general_frlg")
    grass = list(gpmt[0x001][:4])

    def add_tile(px):
        for h in (0, 1):
            for v in (0, 1):
                f = _flip(px, h, v)
                if f in index:
                    return index[f], (h << 10) | (v << 11)
        tiles.append(px)
        index[px] = len(tiles) - 1
        return len(tiles) - 1, 0

    im = art.exterior()
    grid = []
    for by in range(art.EXT_H):
        row = []
        for bx in range(art.EXT_W):
            top = []
            for ty in range(2):
                for tx in range(2):
                    x, y = bx * 16 + tx * 8, by * 16 + ty * 8
                    px = tuple(im.crop((x, y, x + 8, y + 8)).getdata())
                    if any(px):
                        t, f = add_tile(px)
                        top.append((NPRIM + t) | f | (CITY_PAL << 12))
                    else:
                        top.append(0)
            beh = MB["MB_NON_ANIMATED_DOOR"] if (bx, by) == art.EXT_DOOR else MB["MB_NORMAL"]
            mt.append(grass + top)
            attrs.append(beh | (LAYER_COVERED << 29))
            row.append(0x280 + len(mt) - 1)
        grid.append(row)
    assert len(tiles) <= 384 and len(mt) <= 384, (len(tiles), len(mt))
    out = Image.new("P", (per_row * 8, ((len(tiles) + per_row - 1) // per_row) * 8))
    out.putpalette(tiles_img.getpalette())
    for i, px in enumerate(tiles):
        t = Image.new("P", (8, 8))
        t.putdata(px)
        out.paste(t, ((i % per_row) * 8, (i // per_row) * 8))
    out.save(d / "tiles.png")
    (d / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *e) for e in mt))
    (d / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in attrs))
    write_pal(d / "palettes" / f"{CITY_PAL:02d}.pal", art.EXT)
    print(f"Vermilion City: {len(tiles)} tiles, {len(mt)} metatiles")
    return grid


def write_lot_script(grid):
    """the building on the lot (data/scripts/medical_center_lot.inc: Vermilion's ON_LOAD, once it's up)"""
    x0, y0 = LOT
    out = ["@ The Universal Pokémon Medical Center on Vermilion City's lot (tools/gym_hack/build_medical_center.py:",
           "@ don't edit). Called from the city's ON_LOAD once FLAG_MEDCENTER_OPEN is set.",
           "VermilionCity_EventScript_MedicalCenterBuilt::"]
    for by, row in enumerate(grid):
        for bx, m in enumerate(row):
            solid = "FALSE" if (bx, by) == art.EXT_DOOR else "TRUE"
            out.append(f"\tsetmetatile {x0 + bx}, {y0 + by}, 0x{m:03X}, {solid}")
    for bx in range(art.EXT_W):                          # (the lot's edge in front: grass)
        out.append(f"\tsetmetatile {x0 + bx}, {y0 + art.EXT_H}, 0x001, FALSE")
    out.append("\treturn")
    (ROOT / "data/scripts/medical_center_lot.inc").write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    c, P = build_tileset()
    floors = write_maps(P)
    write_lot_script(build_exterior())
    if len(sys.argv) > 1:
        preview(floors, Path(sys.argv[1]))
