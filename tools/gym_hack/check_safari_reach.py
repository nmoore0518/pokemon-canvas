#!/usr/bin/env python3
"""Check that everything in the Exotic Safari can be reached from the hall: walks the zones' maps
(walking, surfing, jumping ledges one way) through their ways out - doors, ladders, edges - and
lists every legendary, item or way out the player can't get to, and anywhere they could get stuck
(somewhere with no way back out).

Usage: python3 tools/gym_hack/check_safari_reach.py     (after build_exotic_safari.py)
Exits with status 1 if anything is wrong."""
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_exotic_safari as B   # noqa: E402

ROOT = B.ROOT
JUMPS = {B.MB["MB_JUMP_EAST"]: (1, 0), B.MB["MB_JUMP_WEST"]: (-1, 0), B.MB["MB_JUMP_NORTH"]: (0, -1),
         B.MB["MB_JUMP_SOUTH"]: (0, 1)}


def load(name):
    layouts = {l["id"]: l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if l.get("id")}
    m = json.loads((ROOT / f"data/maps/{name}/map.json").read_text())
    lay = layouts[m["layout"]]
    w, h = lay["width"], lay["height"]
    b = struct.unpack(f"<{w * h}H", (ROOT / lay["blockdata_filepath"]).read_bytes())
    g = [[b[y * w + x] for x in range(w)] for y in range(h)]
    beh = B.Behaviors((lay["primary_tileset"], lay["secondary_tileset"]), "frlg")
    scripts = (ROOT / f"data/maps/{name}/scripts.inc").read_text()
    # ways out: warps (dest map, dest warp) and edge triggers (their script's warpsilent)
    outs = []
    for i, wp in enumerate(m["warp_events"]):
        outs.append(((wp["x"], wp["y"]), (wp["dest_map"], int(wp["dest_warp_id"]))))
    for c in m["coord_events"]:
        s = re.search(rf"{c['script']}::\s*\n\s*warpsilent (MAP_\w+), (\d+)", scripts)
        if s:
            outs.append(((c["x"], c["y"]), (s.group(1), int(s.group(2)))))
    things = [(o["x"], o["y"], o["script"].split("EventScript_")[-1]) for o in m["object_events"]
              if "Roamer" not in o["script"] and "Legend" in o["script"] or o["graphics_id"] == "OBJ_EVENT_GFX_ITEM_BALL"
              or "Key" in o["script"] or "Cell" in o["script"] or "Relic" in o["script"]]
    return {"g": g, "w": w, "h": h, "beh": beh, "warps": m["warp_events"], "outs": outs, "things": things}


def walk(a, starts):
    g, w, h, beh = a["g"], a["w"], a["h"], a["beh"]
    out_cells = {c for c, _ in a["outs"]} | {(wp["x"], wp["y"]) for wp in a["warps"]}
    seen, todo = set(), [B.start_state(g, *c) for c in starts]
    while todo:
        c = todo.pop()
        if c in seen:
            continue
        seen.add(c)
        todo.extend(B.state_moves(g, w, h, beh, c, out_cells))
    return seen          # (states: x, y, elevation)


def main():
    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())["gMapGroup_ExoticSafari"]
    ids = {B.map_id(n): n for n in groups}
    areas = {n: load(n) for n in groups if n != "ExoticSafari_Hall"}
    hall = json.loads((ROOT / "data/maps/ExoticSafari_Hall/map.json").read_text())
    problems = 0
    # start: every zone door in the hall
    todo = [(ids[wp["dest_map"]], int(wp["dest_warp_id"])) for wp in hall["warp_events"] if wp["dest_map"] in ids]
    reached = {n: set() for n in areas}
    done = set()
    while todo:
        name, wid = todo.pop()
        if (name, wid) in done or name not in areas:
            continue
        done.add((name, wid))
        a = areas[name]
        wp = a["warps"][wid]
        states = walk(a, [(wp["x"], wp["y"])])
        reached[name] |= states
        cells = {s[:2] for s in states}
        for c, (dest, dw) in a["outs"]:
            if c in cells and dest in ids:
                todo.append((ids[dest], dw))
    for name, a in areas.items():
        states = reached[name]
        cells = {s[:2] for s in states}
        if not cells:
            print(f"{name}: can't be reached at all"); problems += 1
            continue
        for x, y, what in a["things"]:
            if not any((x + dx, y + dy) in cells for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0))) and (x, y) not in cells:
                print(f"{name}: {what} at {(x, y)} can't be reached"); problems += 1
        by_dest = {}
        for c, (dest, dw) in a["outs"]:
            by_dest.setdefault(dest, []).append(c)
        for dest, cs in by_dest.items():         # (a way out: any of its cells will do)
            if not any(c in cells for c in cs):
                print(f"{name}: the way to {ids.get(dest, dest)} at {cs[0]} can't be reached"); problems += 1
        # no getting stuck: from every reached cell, some way out
        exits = [c for c, _ in a["outs"]]
        back = walk_back(a, exits)
        stuck = {s[:2] for s in states - back}
        if stuck:
            print(f"{name}: {len(stuck)} cells with no way out, e.g. {sorted(stuck)[:3]}"); problems += 1
    print(f"{len(areas)} areas checked, {problems} problems")
    sys.exit(1 if problems else 0)


def walk_back(a, exits):
    """the states (x, y, elevation) from which some way out can be reached"""
    g, w, h, beh = a["g"], a["w"], a["h"], a["beh"]
    out_cells = set(exits) | {(wp["x"], wp["y"]) for wp in a["warps"]}
    elevs = {(v >> 12) & 0xF for row in g for v in row} | {3}
    rev = {}
    for y in range(h):
        for x in range(w):
            if (g[y][x] >> 10) & 3 and (x, y) not in out_cells:
                continue
            for e in elevs:
                for n in B.state_moves(g, w, h, beh, (x, y, e), out_cells):
                    rev.setdefault(n, []).append((x, y, e))
    ok, todo = set(), [(x, y, e) for x, y in exits for e in elevs]
    while todo:
        c = todo.pop()
        if c in ok:
            continue
        ok.add(c)
        todo.extend(rev.get(c, []))
    return ok


if __name__ == "__main__":
    main()
