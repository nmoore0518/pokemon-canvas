"""The Gym Leader Convention's halls (Gym Trainer hack), for tools/gym_hack/import_johto.py.

Each hall is a gym from Pokémon HnS (a copy of its layout, with its puzzle switched to "solved"
where it needs one), reached from the Radio Tower's lobby: the receptionist sends the player to a
hall, and the hall's door leads back to the lobby. The hall's trainers (convention_roster.py) stand
where the gym's own trainers stood, the first of them (the hall's host) where its leader stood,
and the rest on free spots found here: never in the way (everything reachable stays reachable)
and spread out.
"""
import re
import struct
from collections import deque
from pathlib import Path

from convention_roster import HALLS, all_trainers

ROOT = Path(__file__).resolve().parents[2]

SOURCES = {
    "NORMAL": "GoldenrodCity_Gym_hns", "FIRE": "SeafoamIslands_Gym_hns", "WATER": "CeruleanCity_Gym_hns",
    "GRASS": "CeladonCity_Gym_hns", "ELECTRIC": "VermilionCity_Gym_hns", "ICE": "MahoganyTown_Gym_hns",
    "FIGHTING": "SaffronCity_FightingDojo_hns", "POISON": "FuchsiaCity_Gym_hns", "GROUND": "ViridianCity_Gym_hns",
    "FLYING": "VioletCity_Gym_hns", "PSYCHIC": "SaffronCity_Gym_hns", "BUG": "NationalPark_Normal_hns",
    "ROCK": "PewterCity_Gym_hns", "GHOST": "EcruteakCity_Gym_hns", "DRAGON": "BlackthornCity_Gym_hns",
    "DARK": "PokemonLeague_KarensRoom_hns", "STEEL": "OlivineCity_Gym_hns", "FAIRY": "EcruteakCity_Theater_hns",
    "CHAMPIONS": "PokemonLeague_ChampionsRoom_hns",
}
# Puzzles left solved: the setmetatiles of these script labels (by prefix), applied to the layout
SOLVED = {
    "DRAGON": ["BlackthornGym_MapScript_BuildBridge"],
    "ELECTRIC": ["VermilionCity_Gym_EventScript_SetBeamsOff"],
}
MUSIC = {"CHAMPIONS": "MUS_RG_VICTORY_ROAD", "BUG": "MUS_RG_VIRIDIAN_FOREST"}   # (FireRed music for now)
LOBBY = "GoldenrodCity_RadioTower_1F_hns"
LOBBY_MAP = "MAP_GOLDENROD_CITY_RADIO_TOWER_1F"
LOBBY_LANDING = (16, 8)            # where the player arrives back in the lobby (at the counter)
LOBBY_OBJECTS = [   # (local id, graphics, x, y, facing, script)
    ("LOCALID_CONVENTION_RECEPTIONIST", "OBJ_EVENT_GFX_LINK_RECEPTIONIST", 16, 10, "UP", "Convention_EventScript_Receptionist"),
]
PLACEABLE = {"MB_NORMAL", "MB_CAVE"}
WALKABLE_EXTRA = ("MB_NORMAL", "MB_CAVE", "MB_IMPASSABLE_NORTH", "MB_IMPASSABLE_SOUTH", "MB_IMPASSABLE_EAST",
                  "MB_IMPASSABLE_WEST", "MB_STOP_SPINNING", "MB_TALL_GRASS", "MB_INDOOR_ENCOUNTER", "MB_SAND", "MB_FOOTPRINTS", "MB_PUDDLE",
                  "MB_SHALLOW_WATER", "MB_MT_PYRE_HOLE", "MB_STAIRS_OUTSIDE_ABANDONED_SHIP", "MB_COUNTER")


def hall_name(hall):
    return "ConventionHall_" + hall.title()


def hall_id(hall):
    return "MAP_CONVENTION_HALL_" + hall


def gfx_by_index():
    text = (ROOT / "src/data/gym_convention_sprites.h").read_text()
    body = text[text.index("sConventionGfx"):]
    return re.findall(r"(OBJ_EVENT_GFX_\w+)", body)


def solved_patches(src, hall):
    """[(x, y, metatile, impassable)] from the hall's puzzle scripts."""
    if hall not in SOLVED:
        return []
    labels = {}
    for f in (src / "include/constants/metatile_labels.h").read_text().splitlines():
        m = re.match(r"#define (METATILE_\w+)\s+(0x[0-9A-Fa-f]+|\d+)", f)
        if m:
            labels[m.group(1)] = int(m.group(2), 0)
    text = (src / "data/maps" / SOURCES[hall] / "scripts.inc").read_text()
    out = []
    for block in re.split(r"\n(?=\w+::)", text):
        name = block.split("::")[0]
        if any(name.startswith(p) for p in SOLVED[hall]):
            for x, y, mt, imp in re.findall(r"setmetatile (\d+), (\d+), (\w+), (\w+)", block):
                v = labels[mt] if mt in labels else int(mt, 0)
                out.append((int(x), int(y), v, imp in ("TRUE", "1")))
    return out


def patch_layout(blockdata, width, patches):
    data = bytearray(blockdata)
    for x, y, mt, imp in patches:
        o = (y * width + x) * 2
        old = struct.unpack_from("<H", data, o)[0]
        struct.pack_into("<H", data, o, (old & 0xF000) | (0xC00 if imp else 0) | mt)
    return bytes(data)


class Grid:
    def __init__(self, blockdata, width, height, behavior_of):
        self.w, self.h = width, height
        self.b = [struct.unpack_from("<H", blockdata, i * 2)[0] for i in range(width * height)]
        self.behavior_of = behavior_of

    def inside(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h

    def block(self, x, y):
        return self.b[y * self.w + x]

    def behavior(self, x, y):
        return self.behavior_of(self.block(x, y) & 0x3FF)[0]

    def layer(self, x, y):
        return self.behavior_of(self.block(x, y) & 0x3FF)[1]

    def elevation(self, x, y):
        return self.block(x, y) >> 12

    def passable(self, x, y):
        return self.inside(x, y) and (self.block(x, y) >> 10) & 3 == 0


def reachable(grid, start, warps, blocked=frozenset()):
    """BFS from start: walking (no surfing, ledges or holes; sliding across ice), and through warps
    that lead elsewhere in the same map (teleporters). -> {tile: steps}"""
    warp_at = {(w["x"], w["y"]): w for w in warps}

    def walkable(n, e):
        if n in blocked or not grid.passable(*n):
            return False
        if n not in warp_at and grid.behavior(*n) not in WALKABLE_EXTRA and not is_ice(n):
            return False
        ne = grid.elevation(*n)
        return e in (0, 15) or ne in (0, 15) or ne == e

    def is_ice(p):
        return grid.inside(*p) and grid.behavior(*p) == "MB_ICE"

    dist = {start: 0}
    q = deque([start])
    while q:
        p = q.popleft()
        x, y = p
        e = grid.elevation(x, y)
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if not walkable(n, e):
                continue
            while is_ice(n) and n not in warp_at:            # (slides on until something stops it)
                nn = (n[0] + dx, n[1] + dy)
                if not walkable(nn, grid.elevation(*n)):
                    break
                n = nn
            if n in warp_at and "internal" in warp_at[n]:     # a teleporter: lands on the other pad
                d = warps[warp_at[n]["internal"]]
                n = (d["x"], d["y"])
            elif n in warp_at:
                continue                                      # (the way out)
            if n not in dist:
                dist[n] = dist[p] + 1
                q.append(n)
    return dist


def place(grid, entrance, warps, preferred, count):
    """Spots for count trainers: [(x, y, facing)], the first the farthest preferred spot.
    The gym's own trainers' spots come first, as they were (they may be part of its puzzle: the
    ice gym's trainers stop the player sliding); then free spots."""
    warp_tiles = {(w["x"], w["y"]) for w in warps}
    fixed = [p for p in preferred if grid.passable(*p) and p not in warp_tiles][:count]
    base = reachable(grid, entrance, warps, frozenset(fixed))
    fixed = [p for p in fixed if any(n in base for n in ((p[0] + 1, p[1]), (p[0] - 1, p[1]), (p[0], p[1] + 1), (p[0], p[1] - 1)))]
    base = reachable(grid, entrance, warps, frozenset(fixed))
    near_entrance = {p for p, d in base.items() if d <= 3}

    def ok_spot(p, taken):
        x, y = p
        if p not in base or p in warp_tiles or p in near_entrance or p in taken:
            return False
        if grid.behavior(x, y) not in PLACEABLE or grid.layer(x, y):
            return False          # (on plain floor, not under a statue's top or a roof)
        new = taken | {p}
        r = reachable(grid, entrance, warps, frozenset(new))
        if len(r) < len(base) - len(new):          # something else got cut off
            return False
        for t in new:                                # everyone can still be talked to
            tx, ty = t
            if not any(n in r for n in ((tx + 1, ty), (tx - 1, ty), (tx, ty + 1), (tx, ty - 1))):
                return False
        return True

    def far(p):     # (steps from the entrance to beside it)
        return -min((base[n] for n in ((p[0] + 1, p[1]), (p[0] - 1, p[1]), (p[0], p[1] + 1), (p[0], p[1] - 1)) if n in base), default=0)
    spots = sorted(fixed, key=far)
    while len(spots) < count:
        best, score = None, None
        for p in sorted(base):
            if not ok_spot(p, set(spots)):
                continue
            x, y = p
            others = spots + [entrance]
            md = min(abs(x - a) + abs(y - b) for a, b in others)
            wall = sum(not grid.passable(*n) for n in ((x + 1, y), (x - 1, y), (x, y - 1)))
            s = min(md, 6) * 10 + wall * 3 + base[p] / 100
            if score is None or s > score:
                best, score = p, s
        if best is None:
            print(f"convention: WARNING: no room for {count} trainers (placed {len(spots)})")
            break
        spots.append(best)
    out = []
    final = reachable(grid, entrance, warps, frozenset(spots))
    for x, y in spots:
        opts = [(final[n], d) for n, d in (((x, y + 1), "DOWN"), ((x - 1, y), "LEFT"), ((x + 1, y), "RIGHT"),
                                            ((x, y - 1), "UP")) if n in final]
        out.append((x, y, min(opts)[1]))
    return out


def build(src, hns_layouts, hns_maps, behavior_names, layout_id_for, layout_bin_for):
    """-> {hall: (map json dict, scripts text)}; layouts are written by the caller via layout_bin_for."""
    import json
    gfx = gfx_by_index()
    trainers = all_trainers()
    out = {}
    for hall in HALLS:
        sname = SOURCES[hall]
        m = hns_maps.get(sname) or json.load(open(src / "data/maps" / sname / "map.json"))
        l = hns_layouts[m["layout"]]
        blockdata = patch_layout((src / l["blockdata_filepath"]).read_bytes(), l["width"],
                                 solved_patches(src, hall))
        layout_bin_for(hall, m, blockdata)
        grid = Grid(blockdata, l["width"], l["height"], behavior_names(l))
        warps = []
        exits = []
        for i, w in enumerate(m.get("warp_events") or []):
            if w["dest_map"] == m["id"]:
                warps.append(dict(w, dest_map=hall_id(hall), internal=int(w["dest_warp_id"])))
            else:
                warps.append(dict(w, dest_map=LOBBY_MAP, dest_warp_id="LOBBY"))
                exits.append(i)
        entrance_warp = warps[exits[0]]
        ex, ey = entrance_warp["x"], entrance_warp["y"]
        start = next(((ex + dx, ey + dy) for dx, dy in ((0, -1), (0, 1), (-1, 0), (1, 0))
                      if grid.passable(ex + dx, ey + dy) and grid.behavior(ex + dx, ey + dy) in WALKABLE_EXTRA), (ex, ey - 1))
        members = [t for t in trainers if t[1] == hall]
        preferred = [(o["x"], o["y"]) for o in (m.get("object_events") or [])
                     if not any(w in o["graphics_id"] for w in ("GYM_GUY", "MON_BASE", "ITEM_BALL", "PROF_OAK", "GIRL_1"))]
        print(hall, end=": ")
        spots = place(grid, start, warps, preferred, len(members))
        print(len(spots), "of", len(members), "placed;", len(reachable(grid, start, warps)), "tiles reachable")
        objects, scripts = [], f"{hall_name(hall)}_MapScripts::\n\t.byte 0\n"
        for (idx, _, key, name, role, female, pic, ow), (x, y, face) in zip(members, spots):
            label = f"{hall_name(hall)}_EventScript_{key.title().replace('_', '')}"
            e = grid.elevation(x, y)
            objects.append({"local_id": f"LOCALID_CONV_{key.upper()}", "type": "object", "graphics_id": gfx[idx],
                            "x": x, "y": y, "elevation": e if e not in (0, 15) else 3,
                            "movement_type": f"MOVEMENT_TYPE_FACE_{face}", "movement_range_x": 0,
                            "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
                            "trainer_sight_or_berry_tree_id": "0", "script": label, "flag": "0"})
            scripts += f"\n{label}::\n\tsetvar VAR_0x8004, {idx}\n\tgoto Convention_EventScript_Trainer\n"
        for w in warps:
            w.pop("internal", None)
        out[hall] = ({
            "id": hall_id(hall), "name": hall_name(hall), "layout": layout_id_for(hall, m),
            "music": MUSIC.get(hall, "MUS_RG_GYM"), "region": "REGION_JOHTO",
            "region_map_section": "MAPSEC_GOLDENROD_CITY", "requires_flash": False, "weather": "WEATHER_NONE",
            "map_type": m["map_type"], "allow_cycling": False, "allow_escaping": False, "allow_running": True,
            "show_map_name": False, "floor_number": 0,
            "battle_scene": "MAP_BATTLE_SCENE_NORMAL" if m["map_type"] != "MAP_TYPE_INDOOR" else "MAP_BATTLE_SCENE_GYM",
            "connections": None, "object_events": objects, "warp_events": warps, "coord_events": [], "bg_events": [],
        }, scripts, exits[0])
    return out
