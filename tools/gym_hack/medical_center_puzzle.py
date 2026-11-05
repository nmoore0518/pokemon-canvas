#!/usr/bin/env python3
"""Checks the Universal Pokémon Medical Center's 2F gurney puzzle on the map as built (collision from
the layout, the gurneys from map.json): solvable from the escalator up from the 1F to the one up to the
3F, and not without moving gurneys. Prints the fewest pushes and a solution.

    python3 tools/gym_hack/medical_center_puzzle.py
"""
import json
import struct
import sys
from heapq import heappop, heappush
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
NAME = "VermilionCity_MedicalCenter_2F"
D = {"U": (0, -1), "D": (0, 1), "L": (-1, 0), "R": (1, 0)}


def load():
    m = json.loads((ROOT / f"data/maps/{NAME}/map.json").read_text())
    lay = next(l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if l.get("id") == m["layout"])
    w, h = lay["width"], lay["height"]
    b = (ROOT / lay["blockdata_filepath"]).read_bytes()
    solid = {(x, y) for y in range(h) for x in range(w) if (struct.unpack_from("<H", b, (y * w + x) * 2)[0] >> 10) & 3}
    gurneys = frozenset((o["x"], o["y"]) for o in m["object_events"] if o["graphics_id"] == "OBJ_EVENT_GFX_GURNEY")
    people = {(o["x"], o["y"]) for o in m["object_events"] if o["graphics_id"] != "OBJ_EVENT_GFX_GURNEY"}
    warps = {(wp["x"], wp["y"]) for wp in m["warp_events"]}
    return w, h, solid | people | warps, gurneys


def solve(w, h, walls, gurneys, start, goal):
    """the fewest pushes (and the moves) from start to goal"""
    s0 = (start, gurneys)
    dist, prev, heap, n = {s0: 0}, {s0: None}, [(0, 0, s0)], 1
    while heap:
        d, _, st = heappop(heap)
        if dist[st] != d:
            continue
        p, gs = st
        if p == goal:
            moves = []
            while prev[st]:
                st, k = prev[st]
                moves.append(k)
            return d, "".join(reversed(moves))
        for k, (dx, dy) in D.items():
            q = (p[0] + dx, p[1] + dy)
            if not (0 <= q[0] < w and 0 <= q[1] < h) or q in walls:
                continue
            if q in gs:
                b = (q[0] + dx, q[1] + dy)
                if not (0 <= b[0] < w and 0 <= b[1] < h) or b in walls or b in gs:
                    continue
                ns, nd = (q, frozenset(gs - {q} | {b})), d + 1
            else:
                ns, nd = (q, gs), d
            if nd < dist.get(ns, 1 << 30):
                dist[ns], prev[ns] = nd, (st, k)
                n += 1
                heappush(heap, (nd, n, ns))
    return None, None


if __name__ == "__main__":
    w, h, walls, gurneys = load()
    start, goal = (2, 6), (24, 10)           # (off the escalator up from the 1F; before the one up to the 3F)
    pushes, moves = solve(w, h, walls, gurneys, start, goal)
    print("fewest pushes:", pushes, "moves:", moves)
    free, _ = solve(w, h, walls, frozenset(), start, goal)
    print("without the gurneys: a way through" if free == 0 else "without the gurneys: none (bad)")
    blocked, _ = solve(w, h, walls | gurneys, frozenset(), start, goal)
    print("past them without pushing:", "yes (bad)" if blocked is not None else "no")
    sys.exit(0 if pushes and blocked is None else 1)
