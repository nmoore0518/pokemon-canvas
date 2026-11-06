#!/usr/bin/env python3
"""Saffron City's card house (Pokémon Canvas): Grandmaster Rod and the Trading Card Game (src/tcg/).

    python3 tools/gym_hack/build_tcg_house.py

- Saffron City: the doorless house left of the Copycat's (the top row's first) gets the Copycat
  house's door (its metatiles, copied over the same columns), at DOOR, and a warp in.
- gTileset_TcgHouse (data/tilesets/secondary/tcg_house_frlg): the house's usual secondary tileset
  (generic_building_1_frlg) and new metatiles from tools/gym_hack/tcg_house_art.py: a long card table
  with a playmat and cards (its own palette, 9), the card back (10) and a bookcase of card
  binders (11), each drawn over the building tiles' bottom layers they replace.
- LAYOUT_SAFFRON_CITY_TCG_HOUSE: the Saffron house's layout (House5_Frlg) with them: the table where
  the little table and its chairs were, the card back where the map was (the window beside it a column left), the binders where the
  drawers were.
- SaffronCity_TcgHouse_Frlg: the new map, its door back out and ROD (a Cool Trainer) at the
  table's end; into the map groups (last in Saffron's indoor group: no other map's number changes)
  and event_scripts.s. Its scripts.inc (ROD's dialogue) is kept as it is.
Re-running is safe.
"""
import json
import shutil
import struct
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tcg_house_art as art  # noqa: E402
from build_exotic_safari import register_tileset, write_pal  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
CITY = ROOT / "data/maps/SaffronCity_Frlg/map.json"
NAME, MAP_ID = "SaffronCity_TcgHouse_Frlg", "MAP_SAFFRON_CITY_TCG_HOUSE"
COPYCAT_X, HOUSE_X = 21, 17          # (each house's left column; the door's one to the right)
ROWS = (13, 14, 15)                  # (the house's bottom two rows and its doorstep)
DOOR = (HOUSE_X + 1, 14)
ROD = (8, 4)          # (standing at the card table's right end)
LAYOUT_ID, LAYOUT_DIR = "LAYOUT_SAFFRON_CITY_TCG_HOUSE", "data/layouts/SaffronCity_TcgHouse_Frlg"
TS_NAME, TS_DIR = "TcgHouse", ROOT / "data/tilesets/secondary/tcg_house_frlg"
TS_SRC = ROOT / "data/tilesets/secondary/generic_building_1_frlg"
SECONDARY_TILE0, SECONDARY_METATILE0 = 640, 0x280
RUG = 0xc8               # (the rug under the old table: its bottom layer)


def layouts():
    return {l["id"]: l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if "id" in l}


def city():
    m = json.loads(CITY.read_text())
    lay = layouts()[m["layout"]]
    w, h = lay["width"], lay["height"]
    path = ROOT / lay["blockdata_filepath"]
    raw = list(struct.unpack(f"<{w * h}H", path.read_bytes()))
    for y in ROWS:
        for dx in range(3):
            raw[y * w + HOUSE_X + dx] = raw[y * w + COPYCAT_X + dx]
    path.write_bytes(struct.pack(f"<{w * h}H", *raw))
    copycat = next(wp for wp in m["warp_events"] if wp["dest_map"] == "MAP_SAFFRON_CITY_COPYCATS_HOUSE_1F")
    warps = [wp for wp in m["warp_events"] if wp["dest_map"] != MAP_ID]
    warps.append({"x": DOOR[0], "y": DOOR[1], "elevation": copycat["elevation"], "dest_map": MAP_ID,
                  "dest_warp_id": "0"})
    m["warp_events"] = warps
    CITY.write_text(json.dumps(m, indent=2) + "\n")
    return len(warps) - 1


def tileset():
    """the new tiles, palettes and metatiles; {piece: [metatile ids, row by row]}"""
    if TS_DIR.exists():
        shutil.rmtree(TS_DIR)
    shutil.copytree(TS_SRC, TS_DIR)
    tiles = Image.open(TS_DIR / "tiles.png")
    tw = tiles.size[0] // 8
    old = [tiles.crop(((i % tw) * 8, (i // tw) * 8, (i % tw) * 8 + 8, (i // tw) * 8 + 8))
           for i in range(tw * (tiles.size[1] // 8))]
    new_tiles = list(old)
    mt = bytearray((TS_DIR / "metatiles.bin").read_bytes())
    attrs = bytearray((TS_DIR / "metatile_attributes.bin").read_bytes())
    pmt = art.PMT
    pattrs = (ROOT / "data/tilesets/primary/building_frlg/metatile_attributes.bin").read_bytes()
    pieces = {}
    # (piece, its art, its palette slot, the primary metatiles whose bottom layers it goes over, row by row)
    for name, im, slot, under in (("table", art.table(), 9, [[RUG] * 4, [RUG] * 4]),
                                  ("poster", art.poster(), 10, [[0x170, 0x171], [0x178, 0x179]]),
                                  ("shelf", art.shelf(), 11, [[0xaf], [0xb7], [0xbf]])):
        colors = sorted(set(c[:3] for c in im.get_flattened_data() if c[3]))
        assert len(colors) <= 15, (name, len(colors))
        write_pal(TS_DIR / "palettes" / f"{slot:02d}.pal", [(255, 0, 255)] + colors)
        index = {c: i + 1 for i, c in enumerate(colors)}
        ids = []
        for my, row in enumerate(under):
            ids_row = []
            for mx, m in enumerate(row):
                entry = list(struct.unpack("<4H", pmt[m * 16:m * 16 + 8])) + [0, 0, 0, 0]
                for k in range(4):
                    x0, y0 = mx * 16 + (k % 2) * 8, my * 16 + (k // 2) * 8
                    t = Image.new("P", (8, 8), 0)
                    for y in range(8):
                        for x in range(8):
                            c = im.getpixel((x0 + x, y0 + y))
                            if c[3]:
                                t.putpixel((x, y), index[c[:3]])
                    if not any(t.get_flattened_data()):
                        continue
                    data = bytes(t.get_flattened_data())
                    found = next((i for i, o in enumerate(new_tiles) if bytes(o.get_flattened_data()) == data), None)
                    if found is None:
                        new_tiles.append(t)
                        found = len(new_tiles) - 1
                    entry[4 + k] = (SECONDARY_TILE0 + found) | (slot << 12)
                mt += struct.pack("<8H", *entry)
                attrs += pattrs[m * 4:m * 4 + 4]
                ids_row.append(SECONDARY_METATILE0 + len(mt) // 16 - 1)
            ids.append(ids_row)
        pieces[name] = ids
    rows = (len(new_tiles) + tw - 1) // tw
    sheet = Image.new("P", (tw * 8, rows * 8), 0)
    sheet.putpalette(tiles.getpalette())
    for i, t in enumerate(new_tiles):
        sheet.paste(t, ((i % tw) * 8, (i // tw) * 8))
    sheet.save(TS_DIR / "tiles.png")
    (TS_DIR / "metatiles.bin").write_bytes(bytes(mt))
    (TS_DIR / "metatile_attributes.bin").write_bytes(bytes(attrs))
    register_tileset(TS_NAME, TS_DIR.name)
    return pieces


def layout(pieces):
    """the house's layout, with the table, the poster and the binders"""
    src = layouts()["LAYOUT_HOUSE5_FRLG"]
    w, h = src["width"], src["height"]
    raw = list(struct.unpack(f"<{w * h}H", (ROOT / src["blockdata_filepath"]).read_bytes()))

    def put(x0, y0, ids, collision=None):
        for dy, row in enumerate(ids):
            for dx, m in enumerate(row):
                v = raw[(y0 + dy) * w + x0 + dx]
                c = (v >> 10) & 3 if collision is None else collision
                raw[(y0 + dy) * w + x0 + dx] = m | (c << 10) | (v & 0xF000)
    put(4, 4, pieces["table"], 1)
    put(9, 0, pieces["poster"])
    for y in (0, 1):    # (the right window: a column left, out of the card's way)
        raw[y * w + 7], raw[y * w + 8] = raw[y * w + 8], raw[y * w + 7]
    put(2, 0, pieces["shelf"], 1)
    d = ROOT / LAYOUT_DIR
    d.mkdir(parents=True, exist_ok=True)
    (d / "map.bin").write_bytes(struct.pack(f"<{w * h}H", *raw))
    shutil.copy(ROOT / src["border_filepath"], d / "border.bin")
    path = ROOT / "data/layouts/layouts.json"
    j = json.loads(path.read_text())
    j["layouts"] = [l for l in j["layouts"] if l.get("id") != LAYOUT_ID]
    j["layouts"].append(dict(src, id=LAYOUT_ID, name="SaffronCity_TcgHouse_Layout",
                             secondary_tileset=f"gTileset_{TS_NAME}",
                             border_filepath=f"{LAYOUT_DIR}/border.bin", blockdata_filepath=f"{LAYOUT_DIR}/map.bin"))
    path.write_text(json.dumps(j, indent=2) + "\n")


def house(city_warp):
    d = ROOT / "data/maps" / NAME
    d.mkdir(exist_ok=True)
    m = json.loads((ROOT / "data/maps/SaffronCity_House_Frlg/map.json").read_text())
    m["id"], m["name"], m["layout"] = MAP_ID, NAME, LAYOUT_ID
    m["music"] = "MUS_NONE"     # (its own: the Pokémon Dome's, on the TCG's engine, from its map scripts)
    m["warp_events"] = [{"x": x, "y": 7, "elevation": 3, "dest_map": "MAP_SAFFRON_CITY", "dest_warp_id": str(city_warp)}
                        for x in (3, 4, 5)]
    m["object_events"] = [
        {"type": "object", "graphics_id": "OBJ_EVENT_GFX_COOLTRAINER_M", "x": ROD[0], "y": ROD[1], "elevation": 3,
         "movement_type": "MOVEMENT_TYPE_FACE_LEFT", "movement_range_x": 0, "movement_range_y": 0,
         "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
         "script": "SaffronCity_TcgHouse_EventScript_Rod", "flag": "0"},
    ]
    m["bg_events"] = []
    m["coord_events"] = []
    (d / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    groups = ROOT / "data/maps/map_groups.json"
    g = json.loads(groups.read_text())
    if NAME not in g["gMapGroup_IndoorSaffron_Frlg"]:
        g["gMapGroup_IndoorSaffron_Frlg"].append(NAME)
        groups.write_text(json.dumps(g, indent=2) + "\n")
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    inc = f'\t.include "data/maps/{NAME}/scripts.inc"\n'
    if inc not in text:
        anchor = '\t.include "data/maps/SaffronCity_House_Frlg/scripts.inc"\n'
        assert anchor in text
        ev.write_text(text.replace(anchor, anchor + inc, 1))


if __name__ == "__main__":
    layout(tileset())
    house(city())
