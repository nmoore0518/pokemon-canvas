"""Maps of the Exotic Safari's areas for the player's guide: each area drawn from its layout, its
legendaries shown where they stand (their sprites), and numbered markers for everything else
(items, puzzle pieces, people, the ways out), listed under the map."""
import json
import re
import struct
import sys
from collections import OrderedDict
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
import render_map as R   # noqa: E402

OUT = ROOT / "build/guide/maps"
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def words(camel):
    s = re.sub(r"(?<=[a-z])(?=[A-Z0-9])", " ", camel)
    return s.replace("Galar ", "Galarian ").strip()


def map_groups():
    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())
    return groups["gMapGroup_ExoticSafari"]


def map_const(name):
    return "MAP_" + re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name).upper()


def render(name, area_names, sprite_for, extra_labels):
    """the area's map as a PNG; its legend: [(number, text)], legendaries [(species key, x, y)]"""
    OUT.mkdir(parents=True, exist_ok=True)
    m = json.loads((ROOT / f"data/maps/{name}/map.json").read_text())
    lay = next(l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if l.get("id") == m["layout"])
    R.set_version(lay.get("layout_version"))
    prim, sec = R.Tileset(R.tileset_dir(lay["primary_tileset"])), R.Tileset(R.tileset_dir(lay["secondary_tileset"]))
    w, h = lay["width"], lay["height"]
    blocks = struct.unpack(f"<{w * h}H", (ROOT / lay["blockdata_filepath"]).read_bytes())
    img = Image.new("RGB", (w * 16, h * 16))
    for y in range(h):
        for x in range(w):
            R.draw_metatile(img, x * 16, y * 16, blocks[y * w + x] & 0x3FF, prim, sec)
    img = img.convert("RGBA")
    d = ImageDraw.Draw(img)
    font = ImageFont.truetype(FONT, 11)
    legend, marks = OrderedDict(), []

    def mark(text, x, y, color):
        if text not in legend:
            legend[text] = (len(legend) + 1, color)
        marks.append((legend[text][0], x, y, color))

    # legendaries: their sprites, where they stand
    for o in m["object_events"]:
        g = o["graphics_id"]
        sp = re.match(r"OBJ_EVENT_GFX_SPECIES\((\w+)\)", g)
        if sp and "Legend" in o["script"]:
            pic = sprite_for(sp.group(1))
            if pic:
                p = Image.open(pic).convert("RGBA").resize((40, 40), Image.NEAREST)
                img.alpha_composite(p, (o["x"] * 16 - 12, o["y"] * 16 - 26))
                d.ellipse([o["x"] * 16 - 2, o["y"] * 16 - 2, o["x"] * 16 + 17, o["y"] * 16 + 17], outline=(230, 30, 60), width=3)
            continue
        if sp:
            continue                                   # (roamers: anywhere)
        script = o["script"].split("EventScript_")[-1]
        label = extra_labels(name, script, o)
        if label:
            mark(label, o["x"], o["y"], (245, 200, 40))
    # puzzle tiles and lamps (signs over several cells: one marker at their middle)
    cells = OrderedDict()
    for b in m.get("bg_events", []) + m.get("coord_events", []):
        script = (b.get("script") or "").split("EventScript_")[-1]
        label = extra_labels(name, script, b)
        if label:
            cells.setdefault(label, []).append((b["x"], b["y"]))
    for label, pts in cells.items():
        x = round(sum(p[0] for p in pts) / len(pts))
        y = round(sum(p[1] for p in pts) / len(pts))
        mark(label, x, y, (60, 200, 240))
    # the ways out: edges (walk off: triggers) and doors, ladders, stairs (warps; an edge's
    # arrival spot is a warp too, to the same place: left out)
    edges = OrderedDict()
    for c in m.get("coord_events", []):
        tm = re.search(r"EventScript_To(\w+)$", c.get("script", ""))
        if tm:
            edges.setdefault(edge_dest(name, tm.group(1)), []).append((c["x"], c["y"]))
    def dest_text(dest):
        return "To the hall" if dest == "MAP_EXOTIC_SAFARI_HALL" else "To " + area_names.get(dest, dest)
    for dest, pts in edges.items():
        x = round(sum(p[0] for p in pts) / len(pts))
        y = round(sum(p[1] for p in pts) / len(pts))
        mark(dest_text(dest), x, y, (120, 220, 120))
    for wp in m["warp_events"]:
        if wp["dest_map"] in edges:
            continue
        mark(dest_text(wp["dest_map"]), wp["x"], wp["y"], (120, 220, 120))
    # draw the markers last (on top)
    for num, x, y, color in marks:
        cx, cy = x * 16 + 8, y * 16 + 8
        d.ellipse([cx - 9, cy - 9, cx + 9, cy + 9], fill=color, outline=(20, 20, 20), width=2)
        t = str(num)
        tw = d.textlength(t, font=font)
        d.text((cx - tw / 2, cy - 7), t, fill=(0, 0, 0), font=font)
    out = OUT / f"{name}.png"
    img.convert("RGB").save(out)
    return out, [(n, text, color) for text, (n, color) in legend.items()], (w, h)


def edge_dest(name, key):
    """an edge trigger's destination map (from the scripts: warpsilent <map>)"""
    s = (ROOT / f"data/maps/{name}/scripts.inc").read_text()
    m = re.search(rf"{name}_EventScript_To{key}::\s*\n\s*warpsilent (MAP_\w+)", s)
    return m.group(1) if m else ""
