#!/usr/bin/env python3
"""Double battles (Pokémon Canvas): from the 4th badge on, about two in three battles are doubles.

  - Ordinary trainers on the maps the story reaches after the 4th badge (POST_HALFWAY): two of
    every three, counted in the order they stand (map by map, the map.json's objects), battle doubles.
    One with a single Pokémon gets a partner: a Pokémon another trainer of the same class uses,
    at the same level (src/data/trainers_frlg.party).
  - The big battles, chosen by hand (MAJOR): Koga, Sabrina, Giovanni at his gym and at Silph,
    Lorelei, Agatha, Lance and the late rival battles (tools/gym_hack/gen_rival_teams.py:
    DOUBLES_STAGES) are doubles; Blaine, Bruno, Giovanni in the Hideout and his last battle stay
    singles.
  - Before the 4th badge everything stays as FireRed has it (singles, and its few twin and
    couple doubles).
The battles the hack builds itself (gym challengers, the Convention, the tournament, the Academy)
choose in code (src/gym_challenge.c, src/tournament.c, src/leader_academy.c).

Re-run after editing trainers_frlg.party by hand or after build_training_houses.py:

    python3 tools/gym_hack/assign_doubles.py
"""
import glob
import json
import os
import re
import zlib
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PARTY = ROOT / "src/data/trainers_frlg.party"

POST_HALFWAY = re.compile(r"^(RocketHideout|PokemonTower|Route1[2-9]|Route2[01]|SilphCo|SaffronCity|FuchsiaCity|"
                          r"CinnabarIsland|PokemonMansion|SeafoamIslands|VictoryRoad|ViridianCity_Gym)")
MAJOR = {
    "TRAINER_LEADER_KOGA": True, "TRAINER_LEADER_SABRINA": True, "TRAINER_LEADER_BLAINE": False,
    "TRAINER_LEADER_GIOVANNI": True, "TRAINER_GIOVANNI_DAD_FINAL": False,
    "TRAINER_BOSS_GIOVANNI": False, "TRAINER_BOSS_GIOVANNI_2": True,
    "TRAINER_ELITE_FOUR_LORELEI": True, "TRAINER_ELITE_FOUR_BRUNO": False,
    "TRAINER_ELITE_FOUR_AGATHA": True, "TRAINER_ELITE_FOUR_LANCE": True,
    "TRAINER_ELITE_FOUR_LORELEI_2": True, "TRAINER_ELITE_FOUR_BRUNO_2": False,
    "TRAINER_ELITE_FOUR_AGATHA_2": True, "TRAINER_ELITE_FOUR_LANCE_2": True,
}


def label_trainers():
    out = {}
    for f in glob.glob(str(ROOT / "data/**/*.inc"), recursive=True):
        t = open(f, errors="ignore").read()
        for m in re.finditer(r"^(\w+)::\s*\n((?:.*\n){0,4})", t, re.M):
            b = re.search(r"trainerbattle\w*\s+(TRAINER_\w+)", m[2])
            if b:
                out.setdefault(m[1], b[1])
    return out


def chosen():
    """{trainer: True/False}: the ordinary trainers after the halfway point, two in three doubles"""
    lab = label_trainers()
    out, n = {}, 0
    for f in sorted(glob.glob(str(ROOT / "data/maps/*_Frlg/map.json"))):
        name = os.path.basename(os.path.dirname(f))
        if not POST_HALFWAY.match(name):
            continue
        objs = json.load(open(f)).get("object_events") or []
        trainers = []
        for o in objs:
            t = lab.get(o.get("script", ""))
            if t and t not in trainers and t not in MAJOR and o.get("trainer_type", "TRAINER_TYPE_NONE") != "TRAINER_TYPE_NONE":
                trainers.append(t)
        for t in trainers:
            if t not in out:
                out[t] = n % 3 != 2
                n += 1
    out.update(MAJOR)
    return out


def parse(text):
    """[(name, header lines, [mon blocks])] and the text before the first trainer"""
    parts = re.split(r"^(=== TRAINER_\w+ ===)\n", text, flags=re.M)
    head, blocks = parts[0], []
    for i in range(1, len(parts), 2):
        name = parts[i][4:-4]
        body = parts[i + 1]
        chunks = body.rstrip("\n").split("\n\n")
        blocks.append([name, chunks[0].split("\n"), [c for c in chunks[1:] if c.strip()], body])
    return head, blocks


def main():
    text = PARTY.read_text()
    head, blocks = parse(text)
    pick = chosen()
    pools = defaultdict(list)
    for name, hdr, mons, _ in blocks:
        cls = next((l for l in hdr if l.startswith("Class:")), "")
        for mon in mons:
            sp = mon.split("\n")[0].split(" @ ")[0].split(" (")[0].strip()
            if sp and sp not in pools[cls]:
                pools[cls].append(sp)
    out, doubles, partnered, total = [head], 0, 0, 0
    for name, hdr, mons, body in blocks:
        if name in pick:
            total += 1
            want = pick[name]
            hdr = [("Double Battle: Yes" if want else "Double Battle: No") if l.startswith("Double Battle:") else l for l in hdr]
            if want:
                doubles += 1
            if want and len(mons) == 1:
                cls = next((l for l in hdr if l.startswith("Class:")), "")
                lines = mons[0].split("\n")
                sp = lines[0].split(" @ ")[0].split(" (")[0].strip()
                pool = [p for p in pools[cls] if p != sp] or [sp]
                partner = pool[zlib.crc32(name.encode()) % len(pool)]
                keep = [l for l in lines[1:] if l.startswith(("Level:", "IVs:"))]
                mons = mons + ["\n".join([partner] + keep)]
                partnered += 1
            body = "\n".join(hdr) + "\n\n" + "\n\n".join(mons) + "\n\n"
        out.append(f"=== TRAINER_{name[8:]} ===\n" if not name.startswith("TRAINER_") else f"=== {name} ===\n")
        out.append(body)
    PARTY.write_text("".join(out))
    print(f"{doubles} of {total} after the halfway point battle doubles ({partnered} got a partner)")


if __name__ == "__main__":
    main()
