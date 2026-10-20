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
    "MAPSEC_ROUTE_39": "ROUTE 39", "MAPSEC_ROUTE_40": "ROUTE 40", "MAPSEC_ROUTE_42": "ROUTE 42",
}
WEATHER = {"WEATHER_LEAVES": "WEATHER_NONE", "WEATHER_RAIN": "WEATHER_NONE"}   # (no falling leaves here)
MAPSEC_SUBST = {"MAPSEC_VERMILION_CITY": "MAPSEC_OLIVINE_CITY"}

# HnS-only metatile behaviors -> ours
BEHAVIOR_SUBST = {
    "MB_BOOKSHELF_GREEN": "MB_BOOKSHELF", "MB_SHOP_SHELF_DEPARTMENT": "MB_SHOP_SHELF",
    "MB_SHOP_SHELF_DEPARTMENT_FORWARD": "MB_SHOP_SHELF", "MB_TALL_GRASS_IMPASSABLE_NORTH": "MB_TALL_GRASS",
    "MB_CAVE_IMPASSABLE_NORTH": "MB_CAVE", "MB_HEADBUTT_TREE": "MB_NORMAL",
    "MB_WATER_NORTH_ARROW_WARP": "MB_NORMAL", "MB_BRIDGE_OVER_ICE": "MB_NORMAL",
}

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

    headers, graphics, metatiles, callbacks = [], [], [], set()
    for ts in sorted(tilesets):
        body = re.search(r"const struct Tileset " + ts + r"\s*=\s*\{(.*?)\};", hdr_src, re.S).group(1)
        tiles_sym = re.search(r"\.tiles\s*=\s*(\w+)", body).group(1)
        pal_sym = re.search(r"\.palettes\s*=\s*(\w+)", body).group(1)
        cb = re.search(r"\.callback\s*=\s*(\w+)", body).group(1)
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
    drop_words = ["CeladonCity", "CeruleanCity", "SilphCo", "BlackthornGym", "MtSilverExp"]
    kept = [c for c in chunks if not any(w in c for w in drop_words)]
    # (all of these are FireRed-sized layouts here)
    kept = [c.replace("sSecondaryTilesetBaseTile", "NUM_TILES_IN_PRIMARY_FRLG") for c in kept]
    replace_block(ROOT / "src/tileset_anims.c", "".join(kept).strip() + "\n")
    proto = "".join(f"void {cb}(void);\n" for cb in sorted(callbacks))
    replace_block(ROOT / "include/tileset_anims.h", proto, anchor="#endif")

    # ---- Layouts ---------------------------------------------------------------------------
    layouts_json = ROOT / "data/layouts/layouts.json"
    lj = json.load(open(layouts_json))
    lj["layouts"] = [l for l in lj["layouts"] if not l.get("name", "").endswith("_Johto_Layout")]
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
    layouts_json.write_text(json.dumps(lj, indent=2, ensure_ascii=False) + "\n")

    # ---- Maps ------------------------------------------------------------------------------
    for old in (ROOT / "data/maps").glob("*_Johto"):
        shutil.rmtree(old)
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
        warps = []
        for w in m.get("warp_events") or []:
            if w["dest_map"] in ids:
                warps.append(dict(w, dest_map=new_id(w["dest_map"])))
            else:   # (keeps the warp ids of the others in order)
                warps.append(dict(w, dest_map=mid, dest_warp_id="0"))
        conns = [dict(c, map=new_id(c["map"])) for c in (m.get("connections") or []) if c["map"] in ids]
        out = {
            "id": mid, "name": name, "layout": new_id(m["layout"]),
            "music": MUSIC.get(m["music"], m["music"]), "region": "REGION_JOHTO",
            "region_map_section": MAPSEC_SUBST.get(m["region_map_section"], m["region_map_section"]),
            "requires_flash": m["requires_flash"], "weather": WEATHER.get(m["weather"], m["weather"]), "map_type": m["map_type"],
            "allow_cycling": m["allow_cycling"], "allow_escaping": m["allow_escaping"],
            "allow_running": m["allow_running"], "show_map_name": m["show_map_name"], "floor_number": 0,
            "battle_scene": m["battle_scene"], "connections": conns or None, "object_events": objects,
            "warp_events": warps, "coord_events": [], "bg_events": [],
        }
        d = ROOT / "data/maps" / name
        d.mkdir()
        (d / "map.json").write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n")
        scripts = f"{name}_MapScripts::\n"
        if nurse_id:
            town = name.split("_")[0]
            hl = f"HEAL_LOCATION_{re.sub(r'(?<!^)(?=[A-Z])', '_', town).upper()}"
            scripts += f"\tmap_script MAP_SCRIPT_ON_TRANSITION, {name}_OnTransition\n\t.byte 0\n\n" \
                       f"{name}_OnTransition::\n\tsetrespawn {hl}\n\tend\n"
            heal.append((hl, mid, nurse_id, warps))
        else:
            scripts += "\t.byte 0\n"
        (d / "scripts.inc").write_text(scripts)
        script_includes.append(f'\t.include "data/maps/{name}/scripts.inc"\n')
    (ROOT / "data/maps/johto.inc").write_text("@ Johto map scripts (tools/gym_hack/import_johto.py)\n" + "".join(sorted(script_includes)))

    # Map group
    groups_json = ROOT / "data/maps/map_groups.json"
    g = json.load(open(groups_json))
    if "gMapGroup_Johto" not in g["group_order"]:
        g["group_order"].append("gMapGroup_Johto")
    g["gMapGroup_Johto"] = sorted(new_name(n) for n in hns_maps)
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
    verm = next(s for s in r["map_sections"] if s["id"] == "MAPSEC_VERMILION_CITY")
    for sid, n in MAPSECS.items():   # (on the KANTO map, at VERMILION: see include/regions.h)
        r["map_sections"].append({"id": sid, "name": n, "x": verm["x"], "y": verm["y"], "width": 1, "height": 1})
    rms.write_text(json.dumps(r, indent=2, ensure_ascii=False) + "\n")

    # Wild encounters (daytime)
    wj = ROOT / "src/data/wild_encounters.json"
    w = json.load(open(wj))
    grp = w["wild_encounter_groups"][0]
    johto_ids = {new_id(m["id"]) for m in hns_maps.values()}
    grp["encounters"] = [e for e in grp["encounters"] if e["map"] not in johto_ids]
    for e in json.load(open(SRC / "src/data/wild_encounters.json"))["wild_encounter_groups"][0]["encounters"]:
        if e["map"] in ids and e["base_label"].endswith("_Day"):
            grp["encounters"].append(dict(e, map=new_id(e["map"]), base_label=e["base_label"][:-8] + "_Johto_FireRed"))
    wj.write_text(json.dumps(w, indent=2, ensure_ascii=False) + "\n")

    print(f"Johto: {len(hns_maps)} maps, {len(tilesets)} tilesets, {len(heal)} Pokémon Centers, animations: {sorted(callbacks)}")


if __name__ == "__main__":
    main()
