#!/usr/bin/env python3
"""Every heal location's white-out spot (Pokémon Canvas): the player comes back standing two tiles
below the nurse (or under Mom), facing up, and the nurse is the object the heal location names.
(Indigo Plateau's named Ever Grande's nurse, local id 1, and the default spot by the League door:
whiting out there froze the game.)

    python3 tools/gym_hack/check_heal_locations.py
"""
import json
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    locs = json.loads((ROOT / "src/data/heal_locations.json").read_text())["heal_locations"]
    ids = {}
    for f in ("include/constants/map_event_ids.h",):
        for line in (ROOT / f).read_text().splitlines():
            m = re.match(r"#define (LOCALID_\w+)\s+(\d+)", line)
            if m:
                ids[m[1]] = int(m[2])
    maps = {}
    for d in os.listdir(ROOT / "data/maps"):
        p = ROOT / "data/maps" / d / "map.json"
        if p.exists():
            maps[json.loads(p.read_text())["id"]] = d
    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())
    used = {m for g in groups["group_order"] for m in groups[g]}
    bad = 0
    for h in locs:
        d = maps.get(h.get("respawn_map"))
        if not d or d not in used or not (d.endswith("_Frlg") or "Academy" in d or h["id"] in JOHTO_IDS(locs)):
            continue
        objs = json.loads((ROOT / "data/maps" / d / "map.json").read_text()).get("object_events") or []
        npc = h["respawn_npc"]
        o = next((ob for ob in objs if ob.get("local_id") == npc), None)
        if o is None:
            print("BAD", h["id"], d, f"no object named {npc}")
            bad += 1
            continue
        lid = objs.index(o) + 1
        x, y = h.get("respawn_x", 7), h.get("respawn_y", 4)
        if ids.get(npc) not in (None, lid):
            print("BAD", h["id"], d, f"{npc} is #{lid} on the map, {ids.get(npc)} in the constants")
            bad += 1
        if (o["x"], o["y"]) != (x, y - 1 if "MOM" in npc else y - 2):
            print("BAD", h["id"], d, f"comes back at {(x, y)}, {npc} is at {(o['x'], o['y'])}")
            bad += 1
    print("heal locations:", "all fine" if not bad else f"{bad} problems")
    return bad


def JOHTO_IDS(locs):
    return {h["id"] for h in locs if h["id"] in ("HEAL_LOCATION_AZALEA_TOWN", "HEAL_LOCATION_ECRUTEAK_CITY",
                                                 "HEAL_LOCATION_GOLDENROD_CITY", "HEAL_LOCATION_OLIVINE_CITY")}


if __name__ == "__main__":
    sys.exit(1 if main() else 0)
