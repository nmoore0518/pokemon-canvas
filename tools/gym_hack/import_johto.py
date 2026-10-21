#!/usr/bin/env python3
"""Imports the Johto maps (Gym Trainer hack) from Pokémon HnS (pokehns-expansion).

Source: https://github.com/PokemonHnS-Development/pokehns-expansion (credit: Pokémon Heart & Soul
and the chain of credits in its CREDITS.md). Takes their layouts and tilesets, not their scripts:
  - Tilesets: copied as they are, except the metatile attributes. HnS keeps FireRed's tileset sizes
    (640 primary tiles/metatiles, 7 primary palettes) but Emerald's 2-byte attributes; FireRed
    layouts here use 4-byte attributes, so each is converted (behavior bits 0-7 -> 0-8, layer bits
    12-15 -> 29-30). A few HnS-only behaviors become their nearest standard one.
  - Layouts: copied, marked "frlg".
  - Maps: renamed (GoldenrodCity_hns -> GoldenrodCity_Johto, MAP_GOLDENROD_CITY_HNS ->
    MAP_GOLDENROD_CITY), in a new map group (gMapGroup_Johto). Warps and connections to maps that
    aren't imported are dropped; NPCs, signs and triggers are dropped (ours come later), except
    each Pokémon Center's nurse and each Poké Mart's clerk, who get our scripts.
  - Wild encounters: their daytime tables.
  - Tileset animations (water, flowers, fountains): their code, into src/tileset_anims.c.
Reruns replace everything it made before.

    python3 tools/gym_hack/import_johto.py [path/to/pokehns-expansion]
"""
import json
import re
import shutil
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("/home/claude/ext/pokehns-expansion")

SKIP_MAPS = {
    "VermilionCity_PortInside_hns",                 # (we use our own Vermilion)
    "GoldenrodCity_DepartmentStoreElevator_hns",    # (needs their elevator scripts; the stairs work)
    "NationalPark_BugContest_hns",                  # (a copy of the park for the contest)
}
MAP_PREFIXES = ("Olivine", "Ecruteak", "Goldenrod", "Route34", "Route35", "Route36", "Route37", "Route38",
                "Route39", "Route40", "IlexForest", "Azalea", "NationalPark", "BurnedTower", "TinTower",
                "BellchimeTrail", "Gate_")

# FireRed/Emerald music in place of HGSS's (for now)
MUSIC = {
    "MUS_HG_AZALEA": "MUS_RG_PEWTER", "MUS_HG_BELL_TOWER": "MUS_RG_POKE_TOWER",
    "MUS_HG_BUG_CATCHING_CONTEST": "MUS_RG_ROUTE24", "MUS_HG_BURNED_TOWER": "MUS_RG_POKE_TOWER",
    "MUS_HG_DANCE_THEATER": "MUS_RG_CELADON", "MUS_HG_ECRUTEAK": "MUS_RG_LAVENDER",
    "MUS_HG_GAME_CORNER": "MUS_RG_GAME_CORNER", "MUS_HG_GOLDENROD": "MUS_RG_CELADON",
    "MUS_HG_GYM": "MUS_RG_GYM", "MUS_HG_LIGHTHOUSE": "MUS_RG_SILPH",
    "MUS_HG_NATIONAL_PARK": "MUS_RG_ROUTE24", "MUS_HG_POKE_CENTER": "MUS_RG_POKE_CENTER",
    "MUS_HG_POKE_MART": "MUS_RG_POKE_CENTER", "MUS_HG_ROCKET_TAKEOVER": "MUS_RG_ROCKET_HIDEOUT",
    "MUS_HG_ROUTE34": "MUS_RG_ROUTE24", "MUS_HG_ROUTE38": "MUS_RG_ROUTE11",
    "MUS_HG_ROUTE42": "MUS_RG_ROUTE3", "MUS_HG_UNION_CAVE": "MUS_RG_MT_MOON",
    "MUS_HG_VERMILION": "MUS_RG_VERMILLION", "MUS_HG_VIOLET": "MUS_RG_PEWTER",
}

MAPSECS = {   # id -> name
    "MAPSEC_OLIVINE_CITY": "OLIVINE CITY", "MAPSEC_OLIVINE_LIGHTHOUSE": "LIGHTHOUSE",
    "MAPSEC_ECRUTEAK_CITY": "ECRUTEAK CITY", "MAPSEC_BURNED_TOWER": "BURNED TOWER",
    "MAPSEC_TIN_TOWER": "BELL TOWER", "MAPSEC_GOLDENROD_CITY": "GOLDENROD CITY",
    "MAPSEC_NATIONAL_PARK": "NATIONAL PARK", "MAPSEC_ILEX_FOREST": "ILEX FOREST",
    "MAPSEC_AZALEA_TOWN": "AZALEA TOWN", "MAPSEC_ROUTE_34": "ROUTE 34", "MAPSEC_ROUTE_35": "ROUTE 35",
    "MAPSEC_ROUTE_36": "ROUTE 36", "MAPSEC_ROUTE_37": "ROUTE 37", "MAPSEC_ROUTE_38": "ROUTE 38",
    "MAPSEC_ROUTE_39": "ROUTE 39", "MAPSEC_ROUTE_40": "ROUTE 40",
}
WEATHER = {"WEATHER_LEAVES": "WEATHER_NONE", "WEATHER_RAIN": "WEATHER_NONE"}   # (no falling leaves here)
MAPSEC_SUBST = {"MAPSEC_VERMILION_CITY": "MAPSEC_OLIVINE_CITY", "MAPSEC_ROUTE_42": "MAPSEC_ECRUTEAK_CITY"}

# HnS-only metatile behaviors -> ours
BEHAVIOR_SUBST = {
    "MB_BOOKSHELF_GREEN": "MB_BOOKSHELF", "MB_SHOP_SHELF_DEPARTMENT": "MB_SHOP_SHELF",
    "MB_SHOP_SHELF_DEPARTMENT_FORWARD": "MB_SHOP_SHELF", "MB_TALL_GRASS_IMPASSABLE_NORTH": "MB_TALL_GRASS",
    "MB_CAVE_IMPASSABLE_NORTH": "MB_CAVE", "MB_HEADBUTT_TREE": "MB_NORMAL",
    "MB_WATER_NORTH_ARROW_WARP": "MB_OCEAN_WATER", "MB_BRIDGE_OVER_ICE": "MB_NORMAL",
}

# Where Johto goes on past the convention area: a guard turns the player back (data/scripts/johto.inc).
# (map, edge the player walks toward, guard graphics); the trigger line and guard spot are worked out
# from the path that reaches that edge
BORDERS = [
    ("AzaleaTown_hns", "east", "OBJ_EVENT_GFX_POLICEMAN"),        # (to Route 33)
    ("Route36_hns", "east", "OBJ_EVENT_GFX_POLICEMAN"),           # (to Violet City)
    ("Route36_hns", "south", "OBJ_EVENT_GFX_HIKER"),              # (to the Ruins of Alph)
    ("Route40_hns", "south", "OBJ_EVENT_GFX_SWIMMER_M_WATER"),    # (to Route 41 and Cianwood)
    ("Route40_hns", "north", "OBJ_EVENT_GFX_POLICEMAN"),          # (to the Trainer Hill)
]
# Gates whose far door leads out of the area: a guard stands in front of it (map, door warp index)
GATE_GUARDS = [("Gate_EcruteakCity_Route42_hns", 1), ("Gate_RuinsOfAlph_Route36_hns", 1)]
TOWN_FLAGS = {"OlivineCity_hns": "FLAG_WORLD_MAP_OLIVINE_CITY", "EcruteakCity_hns": "FLAG_WORLD_MAP_ECRUTEAK_CITY",
              "GoldenrodCity_hns": "FLAG_WORLD_MAP_GOLDENROD_CITY", "AzaleaTown_hns": "FLAG_WORLD_MAP_AZALEA_TOWN"}
DIR_PUSH = {"east": "DIR_WEST", "west": "DIR_EAST", "north": "DIR_SOUTH", "south": "DIR_NORTH"}


def wild_level(level):
    """Johto's wild Pokémon are post-game here: HGSS levels 10-40 become about 50-65."""
    return max(50, min(100, 44 + round(level * 0.5)))


def border_spots(layout_bin, width, height, warps, edge):
    """The tiles the player reaches at a map edge -> (trigger line, guard spot)."""
    from collections import deque
    block = lambda x, y: struct.unpack_from("<H", layout_bin, (y * width + x) * 2)[0]
    free = lambda x, y: 0 <= x < width and 0 <= y < height and (block(x, y) >> 10) & 3 == 0
    starts = [(w["x"], w["y"] + 1) for w in warps] + [(width // 2, height // 2)]
    seen = {p for p in starts if 0 <= p[0] < width and 0 <= p[1] < height}
    q = deque(seen)
    while q:
        x, y = q.popleft()
        for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if n not in seen and free(*n):
                seen.add(n)
                q.append(n)
    inward = {"east": (-1, 0), "west": (1, 0), "south": (0, -1), "north": (0, 1)}[edge]
    edge_cells = {"east": [(width - 1, y) for y in range(height)], "west": [(0, y) for y in range(height)],
                  "south": [(x, height - 1) for x in range(width)], "north": [(x, 0) for x in range(width)]}[edge]
    opening = [c for c in edge_cells if c in seen]
    line = [(x + 2 * inward[0], y + 2 * inward[1]) for x, y in opening]
    mid = opening[len(opening) // 2]
    return line, (mid[0] + inward[0], mid[1] + inward[1])


BEGIN, END = "// BEGIN Johto (tools/gym_hack/import_johto.py)\n", "// END Johto\n"


def behaviors(text):
    body = text[text.index("enum"):text.index("NUM_METATILE_BEHAVIORS")]
    out, v = {}, -1
    for m in re.finditer(r"^\s+(MB_\w+)(\s*=\s*(\w+))?", body, re.M):
        v = int(m.group(3), 0) if m.group(3) else v + 1
        out[m.group(1)] = v
    return out


def new_name(hns):
    return hns[:-4] + "_Johto"                      # GoldenrodCity_hns -> GoldenrodCity_Johto


def new_id(hns_id):
    return hns_id[:-4] if hns_id.endswith("_HNS") else hns_id


def replace_block(path, block, anchor=None):
    """Puts block between the BEGIN/END markers (replacing an earlier one), or at the end/before anchor."""
    text = path.read_text()
    if BEGIN in text:
        a, b = text.index(BEGIN), text.index(END) + len(END)
        text = text[:a] + BEGIN + block + END + text[b:]
    elif anchor:
        i = text.index(anchor)
        text = text[:i] + BEGIN + block + END + text[i:]
    else:
        text = text.rstrip("\n") + "\n\n" + BEGIN + block + END
    path.write_text(text)


def main():
    hns_layouts = {l["id"]: l for l in json.load(open(SRC / "data/layouts/layouts.json"))["layouts"] if "id" in l}
    hns_maps = {}
    for p in sorted((SRC / "data/maps").glob("*/map.json")):
        if p.parent.name in SKIP_MAPS or not p.parent.name.startswith(MAP_PREFIXES):
            continue
        hns_maps[p.parent.name] = json.load(open(p))
    ids = {m["id"] for m in hns_maps.values()}
    sys.path.insert(0, str(Path(__file__).parent))
    import convention_halls as conv           # (the Gym Leader Convention's halls)
    hall_srcs = {h: hns_maps.get(n) or json.load(open(SRC / "data/maps" / n / "map.json")) for h, n in conv.SOURCES.items()}

    # ---- Tilesets --------------------------------------------------------------------------
    hdr_src = (SRC / "src/data/tilesets/headers.h").read_text()
    gfx_src = (SRC / "src/data/tilesets/graphics.h").read_text()
    mt_src = (SRC / "src/data/tilesets/metatiles.h").read_text()
    ours_mb = behaviors((ROOT / "include/constants/metatile_behaviors.h").read_text())
    hns_mb = {v: k for k, v in behaviors((SRC / "include/constants/metatile_behaviors.h").read_text()
                                        if (SRC / "include/constants/metatile_behaviors.h").exists()
                                        else Path("/tmp/hns_mb.h").read_text()).items()}

    tilesets = set()
    for m in hns_maps.values():
        l = hns_layouts[m["layout"]]
        tilesets |= {l["primary_tileset"], l["secondary_tileset"]}
    for m in hall_srcs.values():
        l = hns_layouts[m["layout"]]
        tilesets |= {l["primary_tileset"], l["secondary_tileset"]}
    ts_behaviors = {}

    headers, graphics, metatiles, callbacks = [], [], [], set()
    for ts in sorted(tilesets):
        body = re.search(r"const struct Tileset " + ts + r"\s*=\s*\{(.*?)\};", hdr_src, re.S).group(1)
        tiles_sym = re.search(r"\.tiles\s*=\s*(\w+)", body).group(1)
        pal_sym = re.search(r"\.palettes\s*=\s*(\w+)", body).group(1)
        cb = re.search(r"\.callback\s*=\s*(\w+)", body).group(1)
        if cb == "InitTilesetAnim_MtSilverExp":         # (its steam isn't imported: no animation)
            body = body.replace(cb, "NULL")
            cb = "NULL"
        if cb != "NULL":
            callbacks.add(cb)
        tiles_path = re.search(re.escape(tiles_sym) + r"\[\]\s*=\s*INCBIN_U32\(\"([^\"]+)\"", gfx_src).group(1)
        d = Path(tiles_path).parent                                    # data/tilesets/<kind>/<name>
        shutil.rmtree(ROOT / d, ignore_errors=True)
        shutil.copytree(SRC / d, ROOT / d)
        # attributes: 2 bytes -> 4 bytes
        raw = (SRC / d / "metatile_attributes.bin").read_bytes()
        out = bytearray()
        for (a,) in struct.iter_unpack("<H", raw):
            name = hns_mb.get(a & 0xFF, "MB_NORMAL")
            name = BEHAVIOR_SUBST.get(name, name)
            out += struct.pack("<I", ours_mb[name] | (((a >> 12) & 3) << 29))
        (ROOT / d / "metatile_attributes.bin").write_bytes(out)
        # (behavior, whether something is drawn over objects standing there: a statue's top, a roof)
        mts = (SRC / d / "metatiles.bin").read_bytes()
        over = [((a >> 12) & 3) != 1 and any(struct.unpack_from("<H", mts, i * 16 + k * 2)[0] & 0x3FF for k in range(4, 8))
                for i, (a,) in enumerate(struct.iter_unpack("<H", raw))]
        ts_behaviors[ts] = [(BEHAVIOR_SUBST.get(hns_mb.get(a & 0xFF, "MB_NORMAL"), hns_mb.get(a & 0xFF, "MB_NORMAL")),
                             over[i]) for i, (a,) in enumerate(struct.iter_unpack("<H", raw))]
        pal_block = re.search(re.escape(pal_sym) + r"\[\]\[16\]\s*=\s*\{(.*?)\};", gfx_src, re.S).group(1)
        graphics.append(f'const u32 {tiles_sym}[] = INCGFX_U32("{d}/tiles.png", ".4bpp.fastSmol");\n\n'
                        f"const u16 {pal_sym}[][16] =\n{{{pal_block}}};\n\n")
        metatiles.append(re.search(r"const u16 gMetatiles_\w+\[\] = INCBIN_U16\(\"" + re.escape(str(d)) + r"/metatiles.bin\"\);\n"
                                   r"const u16 gMetatileAttributes_\w+\[\] = INCBIN_U16\(\"" + re.escape(str(d)) + r"/metatile_attributes.bin\"\);\n",
                                   mt_src).group(0) + "\n")
        headers.append(f"const struct Tileset {ts} =\n{{{body}}};\n\n")

    replace_block(ROOT / "src/data/tilesets/graphics.h", "".join(graphics))
    replace_block(ROOT / "src/data/tilesets/metatiles.h", "".join(metatiles))
    replace_block(ROOT / "src/data/tilesets/headers.h", "".join(headers))

    # Tileset animations: the HnS functions for the callbacks we use
    anims = (SRC / "src/tileset_anims.c").read_text() if (SRC / "src/tileset_anims.c").exists() else Path("/tmp/hns_anims.c").read_text()
    start = anims.index("// HnS Johto tileset animations")
    stop = anims.index("// Alola Islands primary tileset anims")
    chunks, depth, cur = [], 0, ""
    for ch in anims[start:stop]:          # top-level declarations (by brace depth)
        cur += ch
        depth += (ch == "{") - (ch == "}")
        if depth == 0 and ch in ";}" and not re.search(r"=\s*\{[^;]*\}$", cur.strip()) or (depth == 0 and ch == ";"):
            chunks.append(cur)
            cur = ""
    chunks.append(cur)
    keep_words = [cb.replace("InitTilesetAnim_", "") for cb in callbacks]
    drop_words = ["CeladonCity", "CeruleanCity", "SilphCo", "MtSilverExp"]
    kept = [c for c in chunks if not any(w in c for w in drop_words)]
    # (all of these are FireRed-sized layouts here)
    kept = [c.replace("sSecondaryTilesetBaseTile", "NUM_TILES_IN_PRIMARY_FRLG") for c in kept]
    replace_block(ROOT / "src/tileset_anims.c", "".join(kept).strip() + "\n")
    proto = "".join(f"void {cb}(void);\n" for cb in sorted(callbacks))
    replace_block(ROOT / "include/tileset_anims.h", proto, anchor="#endif")

    # ---- Layouts ---------------------------------------------------------------------------
    layouts_json = ROOT / "data/layouts/layouts.json"
    lj = json.load(open(layouts_json))
    lj["layouts"] = [l for l in lj["layouts"] if not l.get("name", "").endswith("_Johto_Layout")
                     and not l.get("name", "").startswith("ConventionHall_")]
    for m in hns_maps.values():
        l = hns_layouts[m["layout"]]
        name = new_name(m["name"])
        dst = ROOT / "data/layouts" / name
        shutil.rmtree(dst, ignore_errors=True)
        dst.mkdir(parents=True)
        shutil.copy(SRC / l["blockdata_filepath"], dst / "map.bin")
        border = (SRC / l["border_filepath"]).read_bytes()
        shutil.copy(SRC / l["border_filepath"], dst / "border.bin")
        bw = 2 if len(border) == 8 else int((len(border) // 2) ** 0.5)
        lj["layouts"].append({
            "id": new_id(m["layout"]), "name": name + "_Layout", "width": l["width"], "height": l["height"],
            "border_width": bw, "border_height": len(border) // 2 // bw,
            "primary_tileset": l["primary_tileset"], "secondary_tileset": l["secondary_tileset"],
            "border_filepath": f"data/layouts/{name}/border.bin",
            "blockdata_filepath": f"data/layouts/{name}/map.bin", "layout_version": "frlg"})

    # The convention halls: copies of the gyms' layouts (puzzles solved), their trainers placed
    def behavior_names(l):
        prim, sec = ts_behaviors[l["primary_tileset"]], ts_behaviors[l["secondary_tileset"]]
        return lambda mt: (prim[mt] if mt < len(prim) else ("MB_NORMAL", False)) if mt < 640 else \
            (sec[mt - 640] if mt - 640 < len(sec) else ("MB_NORMAL", False))

    def hall_layout_bin(hall, m, blockdata, width, height):
        l = hns_layouts[m["layout"]]
        name = conv.hall_name(hall)
        dst = ROOT / "data/layouts" / name
        shutil.rmtree(dst, ignore_errors=True)
        dst.mkdir(parents=True)
        (dst / "map.bin").write_bytes(blockdata)
        border = (SRC / l["border_filepath"]).read_bytes()
        (dst / "border.bin").write_bytes(border)
        bw = 2 if len(border) == 8 else int((len(border) // 2) ** 0.5)
        lj["layouts"].append({
            "id": "LAYOUT_CONVENTION_HALL_" + hall, "name": name + "_Layout", "width": width,
            "height": height, "border_width": bw, "border_height": len(border) // 2 // bw,
            "primary_tileset": l["primary_tileset"], "secondary_tileset": l["secondary_tileset"],
            "border_filepath": f"data/layouts/{name}/border.bin",
            "blockdata_filepath": f"data/layouts/{name}/map.bin", "layout_version": "frlg"})

    halls = conv.build(SRC, hns_layouts, {conv.SOURCES[h]: m for h, m in hall_srcs.items()}, behavior_names,
                     lambda hall, m: "LAYOUT_CONVENTION_HALL_" + hall, hall_layout_bin)
    layouts_json.write_text(json.dumps(lj, indent=2, ensure_ascii=False) + "\n")

    # ---- Maps ------------------------------------------------------------------------------
    for old in list((ROOT / "data/maps").glob("*_Johto")) + list((ROOT / "data/maps").glob("ConventionHall_*")):
        shutil.rmtree(old)
    # Warps out of the area are dropped (their doors do nothing); the others are renumbered
    warp_index = {}
    for m in hns_maps.values():
        kept, table = 0, {}
        for i, w in enumerate(m.get("warp_events") or []):
            if w["dest_map"] in ids:
                table[i] = kept
                kept += 1
        warp_index[m["id"]] = table
    script_includes = []
    heal = []
    for hname, m in hns_maps.items():
        name = new_name(hname)
        mid = new_id(m["id"])
        objects = []
        nurse_id = None
        for o in m.get("object_events") or []:
            g, s = o.get("graphics_id", ""), o.get("script", "")
            if g == "OBJ_EVENT_GFX_NURSE_HNS":
                nurse_id = f"LOCALID_{mid[4:]}_NURSE"
                objects.append(dict(o, local_id=nurse_id, graphics_id="OBJ_EVENT_GFX_NURSE_FRLG", elevation=3,
                                    script="Johto_EventScript_Nurse", flag="0"))
            elif "_Mart_" in hname and g == "OBJ_EVENT_GFX_MART_EMPLOYEE_HNS":
                objects.append(dict(o, graphics_id="OBJ_EVENT_GFX_CLERK", elevation=3,
                                    script="Johto_EventScript_MartClerk", flag="0"))
        for o in objects:
            o.setdefault("type", "object")
        warps = [dict(w, dest_map=new_id(w["dest_map"]), dest_warp_id=str(warp_index[w["dest_map"]][int(w["dest_warp_id"])]))
                 for w in m.get("warp_events") or [] if w["dest_map"] in ids]
        conns = [dict(c, map=new_id(c["map"])) for c in (m.get("connections") or []) if c["map"] in ids]
        coords, extra_scripts = [], ""
        if hname == conv.LOBBY:
            # The convention's lobby: the receptionist sends the player to the halls, whose doors
            # lead back here (data/scripts/gym_convention.inc)
            lobby_landing = len(warps)
            warps.append({"x": conv.LOBBY_LANDING[0], "y": conv.LOBBY_LANDING[1], "elevation": 3,
                          "dest_map": mid, "dest_warp_id": str(lobby_landing)})
            for lid, gfx, x, y, face, script in conv.LOBBY_OBJECTS:
                objects.append({"local_id": lid, "type": "object", "graphics_id": gfx, "x": x, "y": y,
                                "elevation": 3, "movement_type": f"MOVEMENT_TYPE_FACE_{face}",
                                "movement_range_x": 0, "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
                                "trainer_sight_or_berry_tree_id": "0", "script": script, "flag": "0"})
        if hname == "OlivineCity_PortInside_hns":
            # The convention greeter at the ferry dock: the goodie bag (data/scripts/johto.inc)
            objects.append({"local_id": "LOCALID_OLIVINE_PORT_GREETER", "type": "object",
                            "graphics_id": "OBJ_EVENT_GFX_COOLTRAINER_F", "x": 8, "y": 12, "elevation": 3,
                            "movement_type": "MOVEMENT_TYPE_FACE_DOWN", "movement_range_x": 0,
                            "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
                            "trainer_sight_or_berry_tree_id": "0", "script": "Johto_EventScript_Greeter", "flag": "0"})
            objects.append({"local_id": "LOCALID_OLIVINE_PORT_SAILOR", "type": "object",
                            "graphics_id": "OBJ_EVENT_GFX_SAILOR", "x": 10, "y": 14, "elevation": 3,
                            "movement_type": "MOVEMENT_TYPE_FACE_LEFT", "movement_range_x": 0,
                            "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
                            "trainer_sight_or_berry_tree_id": "0", "script": "Johto_EventScript_OlivineFerry", "flag": "0"})
            for x in (8,):      # (the top of the gangway; VAR_JOHTO_STATE == JOHTO_INVITED)
                coords.append({"type": "trigger", "x": x, "y": 14, "elevation": 0, "var": "VAR_JOHTO_STATE",
                               "var_value": "2", "script": "Johto_EventScript_GreeterStops"})
        l = hns_layouts[m["layout"]]
        blockdata = (SRC / l["blockdata_filepath"]).read_bytes()
        for n, (bmap, edge, gfx) in enumerate([b for b in BORDERS if b[0] == hname]):
            line, spot = border_spots(blockdata, l["width"], l["height"], m.get("warp_events") or [], edge)
            lid = f"LOCALID_{mid[4:]}_GUARD_{n}"
            objects.append({"local_id": lid, "type": "object", "graphics_id": gfx, "x": spot[0], "y": spot[1],
                            "elevation": 1 if "WATER" in gfx else 3, "movement_type": "MOVEMENT_TYPE_LOOK_AROUND",
                            "movement_range_x": 0, "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
                            "trainer_sight_or_berry_tree_id": "0", "script": "Johto_EventScript_GuardTalk", "flag": "0"})
            label = f"{name}_EventScript_Border{n}"
            for x, y in line:
                coords.append({"type": "trigger", "x": x, "y": y, "elevation": 0, "var": "VAR_TEMP_0",
                               "var_value": "0", "script": label})
            extra_scripts += (f"\n{label}::\n\tsetvar VAR_0x8004, {lid}\n\tsetvar VAR_0x8005, {DIR_PUSH[edge]}\n"
                              f"\tgoto Johto_EventScript_BorderGuard\n")
        for gmap, door in GATE_GUARDS:
            if gmap == hname:
                w = (m.get("warp_events") or [])[door]
                l = hns_layouts[m["layout"]]
                bd = (SRC / l["blockdata_filepath"]).read_bytes()
                free = lambda x, y: 0 <= x < l["width"] and 0 <= y < l["height"] and \
                    (struct.unpack_from("<H", bd, (y * l["width"] + x) * 2)[0] >> 10) & 3 == 0
                cx, cy = l["width"] / 2, l["height"] / 2     # (the free tile on the room's side of the door)
                dx, dy = min((d for d in ((0, -1), (-1, 0), (1, 0), (0, 1)) if free(w["x"] + d[0], w["y"] + d[1])),
                             key=lambda d: abs(w["x"] + d[0] - cx) + abs(w["y"] + d[1] - cy))
                objects.append({"local_id": f"LOCALID_{mid[4:]}_GUARD", "type": "object",
                                "graphics_id": "OBJ_EVENT_GFX_POLICEMAN", "x": w["x"] + dx, "y": w["y"] + dy,
                                "elevation": 3, "movement_type": "MOVEMENT_TYPE_FACE_DOWN" if dy >= 0 and dx == 0 else "MOVEMENT_TYPE_LOOK_AROUND",
                                "movement_range_x": 0, "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
                                "trainer_sight_or_berry_tree_id": "0", "script": "Johto_EventScript_GuardTalk", "flag": "0"})
        out = {
            "id": mid, "name": name, "layout": new_id(m["layout"]),
            "music": MUSIC.get(m["music"], m["music"]), "region": "REGION_JOHTO",
            "region_map_section": MAPSEC_SUBST.get(m["region_map_section"], m["region_map_section"]),
            "requires_flash": m["requires_flash"], "weather": WEATHER.get(m["weather"], m["weather"]), "map_type": m["map_type"],
            "allow_cycling": m["allow_cycling"], "allow_escaping": m["allow_escaping"],
            "allow_running": m["allow_running"], "show_map_name": m["show_map_name"], "floor_number": 0,
            "battle_scene": m["battle_scene"], "connections": conns or None, "object_events": objects,
            "warp_events": warps, "coord_events": coords, "bg_events": [],
        }
        d = ROOT / "data/maps" / name
        d.mkdir()
        (d / "map.json").write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n")
        scripts = f"{name}_MapScripts::\n"
        if hname in TOWN_FLAGS:
            scripts += f"\tmap_script MAP_SCRIPT_ON_TRANSITION, {name}_OnTransition\n\t.byte 0\n\n" \
                       f"{name}_OnTransition::\n\tsetflag {TOWN_FLAGS[hname]}\n\tend\n"
        elif nurse_id:
            town = name.split("_")[0]
            hl = f"HEAL_LOCATION_{re.sub(r'(?<!^)(?=[A-Z])', '_', town).upper()}"
            scripts += f"\tmap_script MAP_SCRIPT_ON_TRANSITION, {name}_OnTransition\n\t.byte 0\n\n" \
                       f"{name}_OnTransition::\n\tsetrespawn {hl}\n\tend\n"
            heal.append((hl, mid, nurse_id, warps))
        else:
            scripts += "\t.byte 0\n"
        (d / "scripts.inc").write_text(scripts + extra_scripts)
        script_includes.append(f'\t.include "data/maps/{name}/scripts.inc"\n')
    (ROOT / "src/data/gym_convention_halls.h").write_text(
        "// Generated by tools/gym_hack/import_johto.py (convention_halls.py): each convention hall's\n"
        "// map and the warp the player arrives at\nstatic const struct ConventionHall sConventionHalls[] = {\n"
        + "".join(f"    [CONVENTION_HALL_{h}] = {{MAP_GROUP({conv.hall_id(h)}), MAP_NUM({conv.hall_id(h)}), {e}}},\n"
                  for h, (_, _, e) in halls.items()) + "};\n")
    for hall, (out, scripts, _) in halls.items():
        for w in out["warp_events"]:
            if w["dest_warp_id"] == "LOBBY":
                w["dest_warp_id"] = str(lobby_landing)
        d = ROOT / "data/maps" / out["name"]
        d.mkdir()
        (d / "map.json").write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n")
        (d / "scripts.inc").write_text(scripts)
        script_includes.append(f'\t.include "data/maps/{out['name']}/scripts.inc"\n')
    (ROOT / "data/maps/johto.inc").write_text("@ Johto map scripts (tools/gym_hack/import_johto.py)\n" + "".join(sorted(script_includes)))

    # Map group
    groups_json = ROOT / "data/maps/map_groups.json"
    g = json.load(open(groups_json))
    if "gMapGroup_Johto" not in g["group_order"]:
        g["group_order"].append("gMapGroup_Johto")
    g["gMapGroup_Johto"] = sorted(new_name(n) for n in hns_maps) + [conv.hall_name(h) for h in conv.HALLS]
    groups_json.write_text(json.dumps(g, indent=2, ensure_ascii=False) + "\n")

    # Heal locations: in front of each Pokémon Center's door
    hl_json = ROOT / "src/data/heal_locations.json"
    h = json.load(open(hl_json))
    ours = {x[0] for x in heal}
    h["heal_locations"] = [x for x in h["heal_locations"] if x["id"] not in ours]
    by_id = {new_id(m["id"]): m for m in hns_maps.values()}
    for hl, pc, nurse, pc_warps in heal:
        door = pc_warps[0]
        town = by_id[door["dest_map"]]
        tw = town["warp_events"][int(door["dest_warp_id"])]
        h["heal_locations"].append({"id": hl, "map": door["dest_map"], "x": tw["x"], "y": tw["y"] + 1,
                                    "respawn_map": pc, "respawn_npc": nurse})
    hl_json.write_text(json.dumps(h, indent=2, ensure_ascii=False) + "\n")

    # Map sections
    rms = ROOT / "src/data/region_map/region_map_sections.json"
    r = json.load(open(rms))
    r["map_sections"] = [s for s in r["map_sections"] if s["id"] not in MAPSECS]
    import build_johto_map                  # (where each one is on the Johto map)
    build_johto_map.SRC = SRC
    pos = build_johto_map.positions()
    for sid, n in MAPSECS.items():
        x, y, w, h = pos.get(sid, (0, 0, 1, 1))
        r["map_sections"].append({"id": sid, "name": n, "x": x, "y": y, "width": w, "height": h})
    rms.write_text(json.dumps(r, indent=2, ensure_ascii=False) + "\n")

    # Wild encounters (daytime)
    wj = ROOT / "src/data/wild_encounters.json"
    w = json.load(open(wj))
    grp = w["wild_encounter_groups"][0]
    johto_ids = {new_id(m["id"]) for m in hns_maps.values()}
    grp["encounters"] = [e for e in grp["encounters"] if e["map"] not in johto_ids]
    for e in json.load(open(SRC / "src/data/wild_encounters.json"))["wild_encounter_groups"][0]["encounters"]:
        if e["map"] in ids and e["base_label"].endswith("_Day"):
            e = json.loads(json.dumps(e))
            for k, v in e.items():
                if k.endswith("_mons"):
                    for mon in v["mons"]:
                        mon["min_level"], mon["max_level"] = wild_level(mon["min_level"]), wild_level(mon["max_level"])
            grp["encounters"].append(dict(e, map=new_id(e["map"]), base_label=e["base_label"][:-8] + "_Johto_FireRed"))
    wj.write_text(json.dumps(w, indent=2, ensure_ascii=False) + "\n")

    print(f"Johto: {len(hns_maps)} maps, {len(tilesets)} tilesets, {len(heal)} Pokémon Centers, animations: {sorted(callbacks)}")


if __name__ == "__main__":
    main()
