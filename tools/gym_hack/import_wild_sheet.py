#!/usr/bin/env python3
"""Load an edited wild Pokémon planner (from export_wild_sheet.py) back into the game.

Usage: python3 tools/gym_hack/import_wild_sheet.py <species_info.i> <planner.xlsx>

Writes the Pokémon and levels from the "Encounter Tables" sheet into
src/data/wild_encounters.json (FireRed tables only). Each row is matched by its Table id.
Refuses to write anything if a row has an unknown Pokémon or impossible levels.
"""
import json
import sys
from pathlib import Path

from openpyxl import load_workbook

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_wild_sheet as ex  # noqa: E402

ROOT = ex.ROOT


def main(si_path, xlsx):
    national, info, _ = ex.parse_species(si_path)
    by_name = {info[s]["name"]: s for s in national}
    name_of = {s: v["name"] for s, v in info.items()}

    path = ROOT / "src/data/wild_encounters.json"
    data = json.loads(path.read_text())
    tables = {e["base_label"]: e for e in data["wild_encounter_groups"][0]["encounters"]}

    ws = load_workbook(xlsx, data_only=True)["Encounter Tables"]
    errors, changes = [], 0
    for r, row in enumerate(ws.iter_rows(min_row=2, values_only=True), 2):
        area, _, method, slot, _, name, lo, hi, _, table_id = row[:10]
        if not table_id:
            continue
        label, field, idx = table_id.split(":")
        mon = tables[label][field]["mons"][int(idx)]
        where = f"row {r} ({area}, {method}, slot {slot})"
        name = str(name).strip() if name is not None else ""
        # (Pokémon past #386 are shown by their constant's name, as the exporter writes them)
        kept = mon["species"][8:].replace("_", " ").title() == name
        if name not in by_name and not kept:
            errors.append(f"{where}: unknown Pokémon {name!r}")
            continue
        if not all(isinstance(v, (int, float)) and int(v) == v and 1 <= v <= 100 for v in (lo, hi)) or lo > hi:
            errors.append(f"{where}: bad levels {lo!r}-{hi!r}")
            continue
        # Keep the original constant when the name is unchanged (e.g. Unown letter forms)
        species = mon["species"] if kept or name_of.get(mon["species"]) == name else by_name[name]
        new = {"min_level": int(lo), "max_level": int(hi), "species": species}
        if new != mon:
            changes += 1
            mon.update(new)
    if errors:
        print("Not imported:\n  " + "\n  ".join(errors))
        sys.exit(1)
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")
    print(f"updated {changes} slots")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
