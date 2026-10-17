#!/usr/bin/env python3
"""Export FireRed's wild encounter tables to an Excel workbook for planning.

Usage: python3 tools/gym_hack/export_wild_sheet.py <species_info.i> <out.xlsx>

<species_info.i> is the preprocessed species data (types, names, evolutions), made with:
  arm-none-eabi-gcc -E -P -w -iquote include -I include -I gflib -DMODERN=1 -DTESTING=0 \
      -DFIRERED -std=gnu17 -include include/global.h -x c src/data/pokemon/species_info.h
(after one `make firered`, so the generated headers exist).

The "Encounter Tables" sheet mirrors src/data/wild_encounters.json slot for slot, so it can be
read back in with import_wild_sheet.py.
"""
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

from openpyxl import Workbook
from openpyxl.comments import Comment
from openpyxl.formatting.rule import CellIsRule, FormulaRule
from openpyxl.styles import Alignment, Border, Font, PatternFill, Side
from openpyxl.utils import get_column_letter
from openpyxl.worksheet.datavalidation import DataValidation

ROOT = Path(__file__).resolve().parents[2]
FONT = "Arial"

METHOD_NAMES = {"land_mons": "Grass/Cave", "water_mons": "Surf", "rock_smash_mons": "Rock Smash"}
ROD_NAMES = {"old_rod": "Old Rod", "good_rod": "Good Rod", "super_rod": "Super Rod"}
NEEDS = {"Grass/Cave": "", "Surf": "Surf (Soul Badge)", "Rock Smash": "Rock Smash (Sevii, post-game)",
         "Old Rod": "Old Rod (Vermilion)", "Good Rod": "Good Rod (Fuchsia)", "Super Rod": "Super Rod (Route 12)"}

# Rough story order and when each area is first reachable.
AREAS = [
    ("PALLET_TOWN", "Pallet Town", "Start"), ("ROUTE1", "Route 1", "Start"),
    ("VIRIDIAN_CITY", "Viridian City", "Start"), ("ROUTE22", "Route 22", "Start"),
    ("ROUTE2", "Route 2", "Start"), ("VIRIDIAN_FOREST", "Viridian Forest", "Before Badge 1"),
    ("ROUTE3", "Route 3", "After Badge 1"), ("MT_MOON_1F", "Mt. Moon 1F", "After Badge 1"),
    ("MT_MOON_B1F", "Mt. Moon B1F", "After Badge 1"), ("MT_MOON_B2F", "Mt. Moon B2F", "After Badge 1"),
    ("ROUTE4", "Route 4", "After Badge 1"), ("CERULEAN_CITY", "Cerulean City", "After Badge 1"),
    ("ROUTE24", "Route 24", "Before Badge 2"), ("ROUTE25", "Route 25", "Before Badge 2"),
    ("ROUTE5", "Route 5", "After Badge 2"), ("ROUTE6", "Route 6", "After Badge 2"),
    ("VERMILION_CITY", "Vermilion City", "After Badge 2"), ("SSANNE_EXTERIOR", "S.S. Anne (outside)", "After Badge 2"),
    ("ROUTE11", "Route 11", "After Badge 2"), ("DIGLETTS_CAVE_B1F", "Diglett's Cave", "After Badge 2"),
    ("ROUTE9", "Route 9", "After Badge 3"), ("ROUTE10", "Route 10", "After Badge 3"),
    ("ROCK_TUNNEL_1F", "Rock Tunnel 1F", "After Badge 3"), ("ROCK_TUNNEL_B1F", "Rock Tunnel B1F", "After Badge 3"),
    ("ROUTE8", "Route 8", "After Badge 3"), ("ROUTE7", "Route 7", "After Badge 3"),
    ("CELADON_CITY", "Celadon City", "After Badge 3"),
    ("POKEMON_TOWER_3F", "Pokémon Tower 3F", "After Badge 4"), ("POKEMON_TOWER_4F", "Pokémon Tower 4F", "After Badge 4"),
    ("POKEMON_TOWER_5F", "Pokémon Tower 5F", "After Badge 4"), ("POKEMON_TOWER_6F", "Pokémon Tower 6F", "After Badge 4"),
    ("POKEMON_TOWER_7F", "Pokémon Tower 7F", "After Badge 4"),
    ("ROUTE12", "Route 12", "After Badge 4"), ("ROUTE13", "Route 13", "After Badge 4"),
    ("ROUTE14", "Route 14", "After Badge 4"), ("ROUTE15", "Route 15", "After Badge 4"),
    ("ROUTE16", "Route 16", "After Badge 4"), ("ROUTE17", "Route 17", "After Badge 4"),
    ("ROUTE18", "Route 18", "After Badge 4"), ("FUCHSIA_CITY", "Fuchsia City", "After Badge 4"),
    ("SAFARI_ZONE_CENTER", "Safari Zone Center", "After Badge 4"), ("SAFARI_ZONE_EAST", "Safari Zone East", "After Badge 4"),
    ("SAFARI_ZONE_NORTH_FRLG", "Safari Zone North", "After Badge 4"), ("SAFARI_ZONE_WEST", "Safari Zone West", "After Badge 4"),
    ("POWER_PLANT", "Power Plant", "After Badge 5"), ("ROUTE19", "Route 19", "After Badge 5"),
    ("ROUTE20", "Route 20", "After Badge 5"),
    ("SEAFOAM_ISLANDS_1F", "Seafoam Islands 1F", "After Badge 5"), ("SEAFOAM_ISLANDS_B1F", "Seafoam Islands B1F", "After Badge 5"),
    ("SEAFOAM_ISLANDS_B2F", "Seafoam Islands B2F", "After Badge 5"), ("SEAFOAM_ISLANDS_B3F", "Seafoam Islands B3F", "After Badge 5"),
    ("SEAFOAM_ISLANDS_B4F", "Seafoam Islands B4F", "After Badge 5"),
    ("CINNABAR_ISLAND", "Cinnabar Island", "After Badge 5"),
    ("POKEMON_MANSION_1F", "Pokémon Mansion 1F", "After Badge 5"), ("POKEMON_MANSION_2F", "Pokémon Mansion 2F", "After Badge 5"),
    ("POKEMON_MANSION_3F", "Pokémon Mansion 3F", "After Badge 5"), ("POKEMON_MANSION_B1F", "Pokémon Mansion B1F", "After Badge 5"),
    ("ONE_ISLAND", "One Island", "After Badge 7"), ("ONE_ISLAND_KINDLE_ROAD", "Kindle Road", "After Badge 7"),
    ("ONE_ISLAND_TREASURE_BEACH", "Treasure Beach", "After Badge 7"),
    ("MT_EMBER_EXTERIOR", "Mt. Ember (outside)", "After Badge 7"),
    ("MT_EMBER_SUMMIT_PATH_1F", "Mt. Ember Summit Path 1F", "After Badge 7"),
    ("MT_EMBER_SUMMIT_PATH_2F", "Mt. Ember Summit Path 2F", "After Badge 7"),
    ("MT_EMBER_SUMMIT_PATH_3F", "Mt. Ember Summit Path 3F", "After Badge 7"),
    ("MT_EMBER_RUBY_PATH_1F", "Mt. Ember Ruby Path 1F", "After Badge 7"),
    ("MT_EMBER_RUBY_PATH_B1F", "Mt. Ember Ruby Path B1F", "After Badge 7"),
    ("MT_EMBER_RUBY_PATH_B2F", "Mt. Ember Ruby Path B2F", "After Badge 7"),
    ("MT_EMBER_RUBY_PATH_B3F", "Mt. Ember Ruby Path B3F", "After Badge 7"),
    ("MT_EMBER_RUBY_PATH_B1F_STAIRS", "Mt. Ember Ruby Path B1F (stairs)", "After Badge 7"),
    ("MT_EMBER_RUBY_PATH_B2F_STAIRS", "Mt. Ember Ruby Path B2F (stairs)", "After Badge 7"),
    ("TWO_ISLAND_CAPE_BRINK", "Cape Brink", "After Badge 7"),
    ("THREE_ISLAND_PORT", "Three Island Port", "After Badge 7"), ("THREE_ISLAND_BOND_BRIDGE", "Bond Bridge", "After Badge 7"),
    ("THREE_ISLAND_BERRY_FOREST", "Berry Forest", "After Badge 7"),
    ("ROUTE21_NORTH", "Route 21 North", "After Badge 7"), ("ROUTE21_SOUTH", "Route 21 South", "After Badge 7"),
    ("ROUTE23", "Route 23", "After Badge 8"),
    ("VICTORY_ROAD_1F_FRLG", "Victory Road 1F", "After Badge 8"), ("VICTORY_ROAD_2F", "Victory Road 2F", "After Badge 8"),
    ("VICTORY_ROAD_3F", "Victory Road 3F", "After Badge 8"),
    ("CERULEAN_CAVE_1F", "Cerulean Cave 1F", "Post-game"), ("CERULEAN_CAVE_2F", "Cerulean Cave 2F", "Post-game"),
    ("CERULEAN_CAVE_B1F", "Cerulean Cave B1F", "Post-game"),
    ("FOUR_ISLAND", "Four Island", "Post-game"),
    ("FOUR_ISLAND_ICEFALL_CAVE_ENTRANCE", "Icefall Cave Entrance", "Post-game"),
    ("FOUR_ISLAND_ICEFALL_CAVE_1F", "Icefall Cave 1F", "Post-game"),
    ("FOUR_ISLAND_ICEFALL_CAVE_B1F", "Icefall Cave B1F", "Post-game"),
    ("FOUR_ISLAND_ICEFALL_CAVE_BACK", "Icefall Cave Back", "Post-game"),
    ("FIVE_ISLAND", "Five Island", "Post-game"), ("FIVE_ISLAND_RESORT_GORGEOUS", "Resort Gorgeous", "Post-game"),
    ("FIVE_ISLAND_WATER_LABYRINTH", "Water Labyrinth", "Post-game"), ("FIVE_ISLAND_MEADOW", "Five Isle Meadow", "Post-game"),
    ("FIVE_ISLAND_MEMORIAL_PILLAR", "Memorial Pillar", "Post-game"),
] + [(f"FIVE_ISLAND_LOST_CAVE_ROOM{i}", f"Lost Cave Room {i}", "Post-game") for i in range(1, 15)] + [
    ("SIX_ISLAND_OUTCAST_ISLAND", "Outcast Island", "Post-game"), ("SIX_ISLAND_GREEN_PATH", "Green Path", "Post-game"),
    ("SIX_ISLAND_WATER_PATH", "Water Path", "Post-game"), ("SIX_ISLAND_RUIN_VALLEY", "Ruin Valley", "Post-game"),
    ("SIX_ISLAND_PATTERN_BUSH", "Pattern Bush", "Post-game"), ("SIX_ISLAND_ALTERING_CAVE", "Altering Cave", "Post-game"),
    ("SEVEN_ISLAND_TRAINER_TOWER", "Trainer Tower (outside)", "Post-game"),
    ("SEVEN_ISLAND_SEVAULT_CANYON_ENTRANCE", "Sevault Canyon Entrance", "Post-game"),
    ("SEVEN_ISLAND_SEVAULT_CANYON", "Sevault Canyon", "Post-game"),
    ("SEVEN_ISLAND_TANOBY_RUINS", "Tanoby Ruins", "Post-game"),
] + [(f"SEVEN_ISLAND_TANOBY_RUINS_{c}_CHAMBER", f"Tanoby {c.title()} Chamber", "Post-game")
     for c in ("MONEAN", "LIPTOO", "WEEPTH", "DILFORD", "SCUFIB", "RIXY", "VIAPOIS")]
AREA_INFO = {k: (i, name, when) for i, (k, name, when) in enumerate(AREAS)}


def parse_species(path):
    src = Path(path).read_text()
    enum = (ROOT / "include/constants/species.h").read_text()
    order = {int(n): s for s, n in re.findall(r"^\s+(SPECIES_[A-Z0-9_]+)\s*=\s*(\d+)", enum, re.M)}

    def ev(x):
        m = re.fullmatch(r"\(\s*(\d+)\s*>=\s*(\d+)\s*\?\s*(TYPE_\w+)\s*:\s*(TYPE_\w+)\s*\)", x)
        if m:
            return (m[3] if int(m[1]) >= int(m[2]) else m[4])[5:]
        return x[5:]

    blocks = re.split(r"\[(SPECIES_[A-Z0-9_]+)\]\s*=", src)
    info = {}
    for i in range(1, len(blocks), 2):
        sp, b = blocks[i], blocks[i + 1]
        if sp in info:
            continue
        m = re.search(r"\.types\s*=\s*\{(.*?)\},\n", b)
        n = re.search(r'\.speciesName\s*=\s*_\("(.*?)"\)', b)
        e = re.search(r"\.evolutions = \(const struct Evolution\[\]\) \{(.*?)\{ 0xFFFF \}", b)
        if not m or not n:
            continue
        types = [ev(p) for p in re.findall(r"\([^)]*\)|TYPE_\w+", m[1])]
        evos = re.findall(r"\{(EVO_\w+), ([^,]+), (SPECIES_\w+)", e[1]) if e else []
        info[sp] = {"types": types, "name": n[1], "evos": evos}
    national = [order[i] for i in range(1, 387)]
    pre = {}
    for sp in national:
        for _, _, target in info[sp]["evos"]:
            if target in national:
                pre.setdefault(target, sp)
    return national, info, pre


def pretty(s):
    return s.title()


def main(si_path, out):
    national, info, pre = parse_species(si_path)
    data = json.loads((ROOT / "src/data/wild_encounters.json").read_text())
    grp = data["wild_encounter_groups"][0]
    rates = {f["type"]: f["encounter_rates"] for f in grp["fields"]}
    fish_groups = next(f for f in grp["fields"] if f["type"] == "fishing_mons")["groups"]
    slot_rod = {i: ROD_NAMES[r] for r, idx in fish_groups.items() for i in idx}

    tables = [e for e in grp["encounters"] if e["base_label"].endswith("_FireRed")]
    seen_labels = defaultdict(int)
    rows = []  # one per slot
    for e in tables:
        mp = e["map"][4:]
        seen_labels[mp] += 1
        if mp == "SIX_ISLAND_ALTERING_CAVE" and seen_labels[mp] > 1:
            continue  # extra tables are Mystery Gift variants
        order, name, when = AREA_INFO.get(mp, (999, pretty(mp.replace("_", " ")), "?"))
        for field in ("land_mons", "water_mons", "rock_smash_mons", "fishing_mons"):
            if field not in e:
                continue
            for i, m in enumerate(e[field]["mons"]):
                method = slot_rod[i] if field == "fishing_mons" else METHOD_NAMES[field]
                rows.append({
                    "order": order, "map": e["map"], "label": e["base_label"], "area": name, "when": when,
                    "field": field, "method": method, "slot": i + 1, "rate": rates[field][i],
                    "species": info[m["species"]]["name"], "min": m["min_level"], "max": m["max_level"],
                })
    field_order = {"land_mons": 0, "water_mons": 1, "rock_smash_mons": 2, "fishing_mons": 3}
    rows.sort(key=lambda r: (r["order"], field_order[r["field"]], r["slot"]))

    # where each species is found today
    where = defaultdict(lambda: defaultdict(int))
    for r in rows:
        where[r["species"]][(r["order"], r["area"], r["method"])] += r["rate"]
    wild_now = set(where)

    wb = Workbook()
    hdr_font = Font(name=FONT, bold=True, color="FFFFFF")
    hdr_fill = PatternFill("solid", fgColor="3B4A6B")
    edit_fill = PatternFill("solid", fgColor="FFF2CC")
    thin = Side(style="thin", color="D0D0D0")
    border = Border(bottom=thin)
    base_font = Font(name=FONT, size=10)

    def header(ws, cols, widths):
        for c, (h, w) in enumerate(zip(cols, widths), 1):
            cell = ws.cell(row=1, column=c, value=h)
            cell.font = hdr_font
            cell.fill = hdr_fill
            cell.alignment = Alignment(vertical="center", wrap_text=True)
            ws.column_dimensions[get_column_letter(c)].width = w
        ws.row_dimensions[1].height = 30
        ws.freeze_panes = "A2"

    # ---- How to use ----
    ws = wb.active
    ws.title = "How to use"
    ws.column_dimensions["A"].width = 110
    lines = [
        ("Gym Trainer hack: wild Pokémon planner", Font(name=FONT, bold=True, size=14)),
        ("", None),
        ("How the game picks a wild Pokémon", Font(name=FONT, bold=True, size=11)),
        ("Every area has a fixed set of slots for each way of encountering Pokémon, and each slot has a fixed chance. "
         "You choose which Pokémon (and levels) go in each slot. To give a Pokémon a higher chance, put it in "
         "more slots or in a higher-chance slot.", None),
        ("Grass/Cave: 12 slots: 20%, 20%, 10%, 10%, 10%, 10%, 5%, 5%, 4%, 4%, 1%, 1%", None),
        ("Surf and Rock Smash: 5 slots: 60%, 30%, 5%, 4%, 1%", None),
        ("Old Rod: 70%, 30%   Good Rod: 60%, 20%, 20%   Super Rod: 40%, 40%, 15%, 4%, 1%", None),
        ("", None),
        ("What to edit", Font(name=FONT, bold=True, size=11)),
        ("Encounter Tables sheet: change the yellow cells only (Pokémon, Min Lv, Max Lv). Pick Pokémon from the "
         "dropdown so names match the game exactly. Example: the first row says Pallet Town's 60% Surf slot "
         "holds Tentacool at levels 5 to 10.", None),
        ("Pokémon sheet: a checklist of all 386. 'Slots now' counts how many slots hold that Pokémon and updates "
         "as you edit the tables; anything still at 0 is highlighted red. 'Found in the original game' shows where it is in vanilla FireRed, for "
         "reference. The 'Notes' column is yours for ideas.", None),
        ("Areas sheet: one row per area and method, with the total of its slots (always 100%) and how many "
         "different Pokémon you've put there.", None),
        ("", None),
        ("Things to keep in mind", Font(name=FONT, bold=True, size=11)),
        ("Only these areas and methods can have wild Pokémon, because they're the places with grass, cave floors "
         "or water. New ones are possible but need map changes, so ask first.", None),
        ("The 'When' column is roughly when you can first reach an area. Water needs Surf (Soul Badge, 5th), and "
         "the rods come from Vermilion, Fuchsia and Route 12. Rock Smash spots are in the post-game Sevii Islands.", None),
        ("Your team is locked to Pokémon whose primary type matches yours, so make sure every type has enough "
         "Pokémon reachable early. The Pokémon sheet's 'Primary type' column helps with that.", None),
        ("Lost Cave rooms and Tanoby chambers are small special areas; Tanoby chambers hold the 28 Unown forms.", None),
        ("When you're done, send the file back and I'll load it into the game and check it.", None),
    ]
    for r, (text, font) in enumerate(lines, 1):
        c = ws.cell(row=r, column=1, value=text)
        c.font = font or base_font
        c.alignment = Alignment(wrap_text=True, vertical="top")

    # ---- Encounter Tables ----
    wt = wb.create_sheet("Encounter Tables")
    cols = ["Area", "When", "Method", "Slot", "Chance", "Pokémon", "Min Lv", "Max Lv", "Needs", "Table id"]
    header(wt, cols, [30, 15, 13, 6, 9, 18, 8, 8, 28, 34])
    for i, r in enumerate(rows, 2):
        vals = [r["area"], r["when"], r["method"], r["slot"], r["rate"] / 100, r["species"], r["min"], r["max"],
                NEEDS[r["method"]], f'{r["label"]}:{r["field"]}:{r["slot"] - 1}']
        for c, v in enumerate(vals, 1):
            cell = wt.cell(row=i, column=c, value=v)
            cell.font = base_font
            cell.border = border
        wt.cell(row=i, column=5).number_format = "0%"
        for c in (6, 7, 8):
            wt.cell(row=i, column=c).fill = edit_fill
            wt.cell(row=i, column=c).font = Font(name=FONT, size=10, color="0000FF")
        wt.cell(row=i, column=10).font = Font(name=FONT, size=8, color="999999")
    last_slot_row = len(rows) + 1
    wt.auto_filter.ref = f"A1:J{last_slot_row}"
    wt["F1"].comment = Comment("Edit these. Choose from the dropdown so the name matches the game.", "Claude")
    wt["J1"].comment = Comment("Used to load your changes back into the game. Please don't edit.", "Claude")

    # ---- Pokémon ----
    wp = wb.create_sheet("Pokémon")
    cols = ["#", "Pokémon", "Primary type", "Second type", "Evolves from", "Slots now",
            "Found in the original game", "Notes"]
    header(wp, cols, [5, 14, 12, 12, 14, 9, 70, 40])
    species_range = f"'Encounter Tables'!$F$2:$F${last_slot_row}"
    for n, sp in enumerate(national, 1):
        r = n + 1
        si = info[sp]
        t1 = si["types"][0].title()
        t2 = si["types"][1].title() if len(si["types"]) > 1 and si["types"][1] != si["types"][0] else ""
        found = "; ".join(f"{area} ({method}) {pct}%" for (_, area, method), pct in sorted(where.get(si["name"], {}).items()))
        vals = [n, si["name"], t1, t2, info[pre[sp]]["name"] if sp in pre else "",
                f'=COUNTIF({species_range},B{r})', found or "Not wild in FireRed", ""]
        for c, v in enumerate(vals, 1):
            cell = wp.cell(row=r, column=c, value=v)
            cell.font = base_font
            cell.border = border
        wp.cell(row=r, column=8).fill = edit_fill
    last_mon_row = len(national) + 1
    wp.auto_filter.ref = f"A1:H{last_mon_row}"
    red = PatternFill("solid", fgColor="F4CCCC")
    wp.conditional_formatting.add(f"A2:F{last_mon_row}", FormulaRule(formula=[f"$F2=0"], fill=red))
    wp["F1"].comment = Comment("How many slots hold this Pokémon across all areas. Updates as you edit.", "Claude")

    # summary under the header area on How to use
    ws.cell(row=len(lines) + 2, column=1, value="Progress").font = Font(name=FONT, bold=True, size=11)
    ws.cell(row=len(lines) + 3, column=1,
            value=f"=\"Pokémon with at least one wild slot: \"&COUNTIF('Pokémon'!$F$2:$F${last_mon_row},\">0\")&\" of 386\"").font = base_font

    # dropdown of names on a hidden list sheet
    wl = wb.create_sheet("Lists")
    for n, sp in enumerate(sorted(info[s]["name"] for s in national), 1):
        wl.cell(row=n, column=1, value=sp).font = base_font
    wl.sheet_state = "hidden"
    dv = DataValidation(type="list", formula1=f"=Lists!$A$1:$A${len(national)}", allow_blank=False,
                        showErrorMessage=True, errorTitle="Unknown Pokémon",
                        error="Pick a Pokémon from the list so the name matches the game.")
    wt.add_data_validation(dv)
    dv.add(f"F2:F{last_slot_row}")
    lv = DataValidation(type="whole", operator="between", formula1="1", formula2="100", showErrorMessage=True,
                        error="Levels go from 1 to 100.")
    wt.add_data_validation(lv)
    lv.add(f"G2:H{last_slot_row}")
    wt.conditional_formatting.add(f"G2:H{last_slot_row}", FormulaRule(formula=["$G2>$H2"], fill=red))

    # ---- Areas ----
    wa = wb.create_sheet("Areas")
    cols = ["Area", "When", "Method", "Slots", "Total chance", "Different Pokémon"]
    header(wa, cols, [30, 15, 13, 7, 13, 17])
    areas = []
    for r in rows:
        key = (r["order"], r["area"], r["when"], r["method"])
        if key not in areas:
            areas.append(key)
    for i, (_, area, when, method) in enumerate(areas, 2):
        rng = lambda col: f"'Encounter Tables'!${col}$2:${col}${last_slot_row}"
        crit = f'{rng("A")},A{i},{rng("C")},C{i}'
        first = next(j for j, r in enumerate(rows, 2) if r["area"] == area and r["method"] == method)
        count = sum(1 for r in rows if r["area"] == area and r["method"] == method)
        span = f"'Encounter Tables'!$F${first}:$F${first + count - 1}"
        vals = [area, when, method, f"=COUNTIFS({crit})", f'=SUMIFS({rng("E")},{crit})',
                f'=SUMPRODUCT(({span}<>"")/COUNTIF({span},{span}&""))']
        for c, v in enumerate(vals, 1):
            cell = wa.cell(row=i, column=c, value=v)
            cell.font = base_font
            cell.border = border
        wa.cell(row=i, column=5).number_format = "0%"

    wb.save(out)
    print(f"{len(rows)} slots in {len(areas)} area/methods; "
          f"{sum(1 for s in national if info[s]['name'] in wild_now)} of 386 wild today")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
