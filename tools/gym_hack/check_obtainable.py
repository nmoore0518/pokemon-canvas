#!/usr/bin/env python3
"""Check that all 386 Gen 1-3 Pokémon can be obtained in a single-player FireRed game:
either caught in the wild, or evolved from something obtainable by a method that works
without trading, a clock, or Pokéblocks.

Usage: python3 tools/gym_hack/check_obtainable.py <species_info.i>
Exits with status 1 and lists the problems if any Pokémon can't be obtained.
"""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_wild_sheet as ex  # noqa: E402

ROOT = ex.ROOT
SEVII = re.compile(r"MAP_((ONE|TWO|THREE|FOUR|FIVE|SIX|SEVEN)_ISLAND|MT_EMBER|BIRTH_ISLAND|NAVEL_ROCK|TRAINER_TOWER)")
SHOP = ROOT / "data/maps/CeladonCity_DepartmentStore_4F_Frlg/scripts.inc"

# Evolution conditions a single-player FireRed game can meet
OK_CONDITIONS = {
    "IF_MIN_FRIENDSHIP", "IF_GENDER", "IF_ATK_GT_DEF", "IF_ATK_EQ_DEF", "IF_ATK_LT_DEF",
    "IF_PID_UPPER_MODULO_10_GT", "IF_PID_UPPER_MODULO_10_EQ", "IF_PID_UPPER_MODULO_10_LT",
    "IF_PID_MODULO_100_GT", "IF_PID_MODULO_100_EQ", "IF_PID_MODULO_100_LT",
    "IF_NOT_REGION",       # "not in Alola" etc.: always true in Kanto
    "IF_BAG_ITEM_COUNT",   # Shedinja: a spare Poke Ball in the bag
}


def evolutions(si_text, species):
    """[(method, param, target, [conditions])] parsed from the preprocessed species data."""
    i = si_text.index(f"[{species}] =")
    j = si_text.find(".evolutions =", i)
    k = si_text.find("[SPECIES_", i + 5)
    if j == -1 or (k != -1 and j > k):
        return []
    end = si_text.index("{ 0xFFFF }", j)
    body = si_text[j:end]
    out = []
    for m in re.finditer(r"\{(EVO_\w+), ([^,]+), (SPECIES_\w+)(.*?)\}(?=\s*,\s*\{|\s*,\s*$)", body, re.S):
        conds = re.findall(r"\{(IF_\w+),", m[4])
        out.append((m[1], m[2].strip(), m[3], conds))
    return out


def main(si_path):
    si_text = Path(si_path).read_text()
    national, info, _ = ex.parse_species(si_path)
    data = json.loads((ROOT / "src/data/wild_encounters.json").read_text())
    wild = set()
    for e in data["wild_encounter_groups"][0]["encounters"]:
        # Sevii Islands are unreachable in this hack; Johto (which replaces them) is post-game
        if (not e["base_label"].endswith("_FireRed") or SEVII.match(e.get("map", ""))
                or "_Johto_" in e["base_label"]):
            continue
        for field in ("land_mons", "water_mons", "rock_smash_mons", "fishing_mons"):
            # The S.S. Anne sails before the player has Surf, so its water is never reachable
            if field == "water_mons" and e.get("map") == "MAP_SSANNE_EXTERIOR":
                continue
            for m in e.get(field, {}).get("mons", []):
                sp = m["species"]
                wild.add("SPECIES_UNOWN" if sp.startswith("SPECIES_UNOWN") else sp)
    shop = set(re.findall(r"\.2byte (ITEM_\w+)", SHOP.read_text().split("CeladonCity_DepartmentStore_4F_Items::")[1]))

    obtainable = {s for s in national if s in wild}
    how = {s: "wild" for s in obtainable}
    blocked = {}
    changed = True
    while changed:
        changed = False
        for src in sorted(obtainable):
            for method, param, target, conds in evolutions(si_text, src):
                if target not in national or target in obtainable:
                    continue
                ok, why = True, ""
                if method == "EVO_TRADE":
                    ok, why = False, "needs a trade"
                elif method == "EVO_ITEM" and param not in shop:
                    ok, why = False, f"{param} isn't sold at Celadon"
                bad = [c for c in conds if c not in OK_CONDITIONS]
                if bad:
                    ok, why = False, "needs " + ", ".join(bad)
                if ok:
                    obtainable.add(target)
                    how[target] = f"evolve {info[src]['name']} ({method[4:].lower()} {param})"
                    changed = True
                else:
                    blocked.setdefault(target, []).append(f"from {info[src]['name']}: {why}")
    # Johto's and Hoenn's legendaries are post-game: in the Exotic Safari (and Ho-Oh, Groudon and
    # Rayquaza only come to a gym of their type), not in Kanto's wild
    safari = {f"SPECIES_{n}" for n in ("RAIKOU", "ENTEI", "SUICUNE", "LUGIA", "HO_OH", "CELEBI", "REGIROCK",
              "REGICE", "REGISTEEL", "LATIAS", "LATIOS", "KYOGRE", "GROUDON", "RAYQUAZA", "JIRACHI", "DEOXYS_NORMAL")}
    wild_legends = sorted(s for s in safari & wild)
    if wild_legends:
        print("In Kanto's wild but meant for the Exotic Safari: " + ", ".join(info[s]["name"] for s in wild_legends))
    missing = [s for s in national if s not in obtainable and s not in safari]
    for s in missing:
        print(f"{info[s]['name']}: can't be obtained. " + "; ".join(blocked.get(s, ["not wild, no pre-evolution"])))
    print(f"{len(national) - len(missing) - len(safari)} of {386 - len(safari)} obtainable before the post-game ({len(wild & set(national))} wild)")
    sys.exit(1 if missing or wild_legends else 0)


if __name__ == "__main__":
    main(sys.argv[1])
