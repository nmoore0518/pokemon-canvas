#!/usr/bin/env python3
"""The Exotic Pokémon Safari planner: an Excel workbook for choosing each zone's areas, wild
Pokémon and legendaries (filled in by Nick, then read back into the game).

Usage: python3 tools/gym_hack/safari_planner.py <species_info.i> <out.xlsx>

<species_info.i> is the preprocessed species data (see export_wild_sheet.py for the command).
Sheets: Start Here, Areas, Wild Pokémon, Legendaries, Pokémon List, Lists. Cells to fill in are
yellow. The "Wild Pokémon" sheet works out each slot's chance and flags Pokémon that aren't in
the list or are one of the 18 gym legendaries (gym-only).
"""
import re
import sys
from pathlib import Path

from openpyxl import Workbook
from openpyxl.formatting.rule import FormulaRule
from openpyxl.styles import Alignment, Border, Font, PatternFill, Side
from openpyxl.utils import get_column_letter
from openpyxl.worksheet.datavalidation import DataValidation

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
import build_exotic_safari as safari  # noqa: E402

FONT = "Arial"
YELLOW = PatternFill("solid", start_color="FFF2CC")
HEAD = PatternFill("solid", start_color="1F3A5F")
SUB = PatternFill("solid", start_color="DCE6F1")
GREY = PatternFill("solid", start_color="F2F2F2")
THIN = Side(style="thin", color="BFBFBF")
BOX = Border(left=THIN, right=THIN, top=THIN, bottom=THIN)

REGIONS = ["Johto", "Hoenn", "Sinnoh", "Unova", "Kalos", "Alola", "Galar", "Paldea"]
GENERATIONS = [  # (first national dex number, generation, region)
    (1, 1, "Kanto"), (152, 2, "Johto"), (252, 3, "Hoenn"), (387, 4, "Sinnoh"), (494, 5, "Unova"),
    (650, 6, "Kalos"), (722, 7, "Alola"), (810, 8, "Galar"), (906, 9, "Paldea")]
# How a method's slots split the encounters (FireRed's tables)
METHODS = {
    "Grass/Cave": [20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1],
    "Surf": [60, 30, 5, 4, 1],
    "Rock Smash": [60, 30, 5, 4, 1],
    "Old Rod": [70, 30],
    "Good Rod": [60, 20, 20],
    "Super Rod": [40, 40, 15, 4, 1],
}
FORM_SUFFIX = {"_ALOLA": "Alolan", "_GALAR": "Galarian", "_HISUI": "Hisuian", "_PALDEA": "Paldean"}
GYM_LEGENDS = ["Meloetta", "Koraidon", "Tornadus", "Eternatus", "Groudon", "Diancie", "Genesect", "Giratina",
               "Dialga", "Ho-Oh", "Palkia", "Shaymin", "Miraidon", "Cresselia", "Glastrier", "Rayquaza",
               "Darkrai", "Xerneas"]
# Suggested Paldea areas (from the earlier talk; Nick decides). The meadow is built.
PALDEA_AREAS = [
    ("Paldea", "Meadow", "South Province (Area One-Two)", "Grassland", "Small", "Built (test)", "", "", "",
     "In the game now with placeholder Pokémon. Replace them below."),
    ("Paldea", "Forest", "Tagtree Thicket", "Forest", "", "", "", "", "", "Suggestion"),
    ("Paldea", "Desert", "Asado Desert", "Desert", "", "", "", "", "", "Suggestion"),
    ("Paldea", "Snowy Mountain", "Glaseado Mountain", "Snow", "", "", "", "", "", "Suggestion"),
    ("Paldea", "Lake", "Casseroya Lake", "Water", "", "", "", "", "", "Suggestion"),
    ("Paldea", "Crater", "Area Zero", "Cave/Crater", "", "", "", "", "", "Suggestion: paradox Pokémon"),
]


def parse(si_path):
    src = Path(si_path).read_text()
    dex = re.findall(r"^\s+(NATIONAL_DEX_\w+),", (ROOT / "include/constants/pokedex.h").read_text(), re.M)
    dex = [d for d in dex if d != "NATIONAL_DEX_NONE"]
    blocks = re.split(r"\[(SPECIES_[A-Z0-9_]+)\]\s*=", src)
    info = {}
    for i in range(1, len(blocks), 2):
        sp, b = blocks[i], blocks[i + 1]
        if sp in info:
            continue
        n = re.search(r'\.speciesName\s*=\s*_\("(.*?)"\)', b)
        t = re.search(r"\.types\s*=\s*\{(.*?)\},\n", b)
        d = re.search(r"\.natDexNum\s*=\s*(NATIONAL_DEX_\w+)", b)
        if not (n and t and d):
            continue
        types = []
        for p in re.findall(r"\([^)]*\)|TYPE_\w+", t[1]):
            m = re.fullmatch(r"\(\s*(\d+)\s*>=\s*(\d+)\s*\?\s*(TYPE_\w+)\s*:\s*(TYPE_\w+)\s*\)", p)
            types.append((m[3] if int(m[1]) >= int(m[2]) else m[4]) if m else p)
        types = [x[5:].title() for x in types]
        flags = set(re.findall(r"\.is(\w+)\s*=\s*(?:TRUE|1)\b", b))
        info[sp] = {"name": n[1], "types": types, "dex": dex.index(d[1]) + 1, "flags": flags}
    return info


def gen_of(num):
    g = [x for x in GENERATIONS if x[0] <= num][-1]
    return g[1], g[2]


def kind(flags):
    if "Mythical" in flags:
        return "Mythical"
    if flags & {"RestrictedLegendary", "SubLegendary", "Legendary"}:
        return "Legendary"
    if "UltraBeast" in flags:
        return "Ultra Beast"
    if "Paradox" in flags:
        return "Paradox"
    return ""


def style_header(ws, row, headers, widths):
    for c, (h, w) in enumerate(zip(headers, widths), 1):
        cell = ws.cell(row=row, column=c, value=h)
        cell.font = Font(name=FONT, bold=True, color="FFFFFF")
        cell.fill = HEAD
        cell.alignment = Alignment(vertical="center", wrap_text=True)
        cell.border = BOX
        ws.column_dimensions[get_column_letter(c)].width = w
    ws.freeze_panes = ws.cell(row=row + 1, column=1)


def body(cell, editable=False):
    cell.font = Font(name=FONT)
    cell.border = BOX
    if editable:
        cell.fill = YELLOW


def main(si_path, out):
    info = parse(si_path)
    wb = Workbook()

    # ---- Pokémon List (the reference everything checks against) ----------------------------
    pl = wb.active
    pl.title = "Pokémon List"
    rows = []
    dex_names = [d[len("NATIONAL_DEX_"):] for d in re.findall(r"^\s+(NATIONAL_DEX_\w+),",
                 (ROOT / "include/constants/pokedex.h").read_text(), re.M) if d != "NATIONAL_DEX_NONE"]
    FORM_GEN = {"Alolan": (7, "Alola"), "Galarian": (8, "Galar"), "Hisuian": (8, "Sinnoh"), "Paldean": (9, "Paldea")}
    seen = set()
    for sp, v in info.items():
        base = v["dex"]
        form = next((label for suf, label in FORM_SUFFIX.items() if sp.endswith(suf) or suf + "_" in sp), None)
        if form:
            if "TOTEM" in sp or base == 25:      # (no totems; Pikachu's caps aren't regional forms)
                continue
            extra = sp.split(next(s for s in FORM_SUFFIX if s in sp))[1].strip("_").title()
            name = f"{v['name']} ({form}{' ' + extra if extra else ''})"
            g, region = FORM_GEN[form]
        elif base not in seen:
            seen.add(base)        # (the first entry for a dex number is the default form)
            name = v["name"]
            g, region = gen_of(base)
        else:
            continue
        rows.append((base, name, v["types"][0], v["types"][1] if len(v["types"]) > 1 and v["types"][1] != v["types"][0] else "",
                     g, region, kind(v["flags"]), "Gym-only" if v["name"] in GYM_LEGENDS and not form else ("Already in Kanto" if base <= 386 and kind(v["flags"]) == "" else "")))
    # Defined through macros the parser can't read
    for num, name, t1, t2 in ((666, "Vivillon", "Bug", "Flying"), (671, "Florges", "Fairy", "")):
        if num not in seen:
            rows.append((num, name, t1, t2, 6, "Kalos", "", ""))
    rows.sort(key=lambda r: (r[0], r[1]))
    style_header(pl, 1, ["Dex #", "Pokémon", "Type 1", "Type 2", "Gen", "Region", "Legendary?", "Note"],
                 [8, 26, 11, 11, 6, 10, 13, 12])
    for r, row in enumerate(rows, 2):
        for c, v in enumerate(row, 1):
            body(pl.cell(row=r, column=c, value=v))
    n_species = len(rows)
    pl.auto_filter.ref = f"A1:H{n_species + 1}"

    # ---- Lists (dropdown sources, slot chances) ----------------------------------------------
    ls = wb.create_sheet("Lists")
    style_header(ls, 1, ["Region", "", "Method", "Slots", *[f"Slot {i}" for i in range(1, 13)], "", "Gym-only legendaries"],
                 [10, 3, 13, 7] + [7] * 12 + [3, 22])
    for i, reg in enumerate(REGIONS, 2):
        body(ls.cell(row=i, column=1, value=reg))
    for i, (meth, rates) in enumerate(METHODS.items(), 2):
        body(ls.cell(row=i, column=3, value=meth))
        body(ls.cell(row=i, column=4, value=len(rates)))
        for k, rate in enumerate(rates):
            c = ls.cell(row=i, column=5 + k, value=rate / 100)
            body(c)
            c.number_format = "0%"
    for i, name in enumerate(GYM_LEGENDS, 2):
        body(ls.cell(row=i, column=18, value=name))
    n_methods = len(METHODS)
    ls.cell(row=12, column=1, value="Slot chances are FireRed's. The 18 gym legendaries can't go in a safari.").font = Font(name=FONT, italic=True)

    # ---- Areas ------------------------------------------------------------------------------
    ar = wb.create_sheet("Areas", 0)
    style_header(ar, 1, ["Region", "Area", "Inspired by (game location)", "Biome / look", "Size (S/M/L)",
                         "Status", "Surf?", "Fishing?", "Rock Smash?", "Notes"],
                 [10, 18, 30, 16, 12, 14, 8, 9, 12, 46])
    r = 2
    for reg in REGIONS:
        own = [a for a in PALDEA_AREAS if a[0] == reg]
        for k in range(max(6, len(own) + 2)):
            vals = own[k] if k < len(own) else (reg,) + ("",) * 9
            for c, v in enumerate(vals, 1):
                body(ar.cell(row=r, column=c, value=v), editable=c > 1)
            if reg != REGIONS[-1] or True:
                pass
            r += 1
    n_area_rows = r - 1
    dv = DataValidation(type="list", formula1='"Yes,No"', allow_blank=True)
    ar.add_data_validation(dv)
    dv.add(f"G2:I{n_area_rows}")

    # ---- Wild Pokémon -------------------------------------------------------------------------
    wp = wb.create_sheet("Wild Pokémon", 1)
    style_header(wp, 1, ["Region", "Area", "Method", "Slot", "Chance", "Pokémon", "Min Lv", "Max Lv", "Check", "Notes"],
                 [10, 18, 13, 6, 9, 26, 8, 8, 26, 30])
    r = 2
    meadow = safari.ZONES["PALDEA"]["land"]
    names = {sp: v["name"] for sp, v in info.items()}
    for k, (sp, lo, hi) in enumerate(meadow):
        vals = ["Paldea", "Meadow", "Grass/Cave", k + 1, None, names.get("SPECIES_" + sp, sp.title()), lo, hi, None,
                "Placeholder (in the game now)" if k == 0 else ""]
        for c, v in enumerate(vals, 1):
            body(wp.cell(row=r, column=c, value=v), editable=c not in (5, 9))
        r += 1
    first_blank = r
    last = 1200
    for rr in range(2, last + 1):
        if rr >= first_blank:
            for c in range(1, 11):
                body(wp.cell(row=rr, column=c), editable=c not in (5, 9))
        wp.cell(row=rr, column=5, value=(
            f'=IF(OR(C{rr}="",D{rr}=""),"",IFERROR(INDEX(Lists!$E$2:$P${n_methods + 1},'
            f'MATCH(C{rr},Lists!$C$2:$C${n_methods + 1},0),D{rr}),"no such slot"))')).number_format = "0%"
        wp.cell(row=rr, column=9, value=(
            f'=IF(F{rr}="","",IF(COUNTIF(Lists!$R$2:$R$19,F{rr})>0,"Gym-only legendary",'
            f'IF(COUNTIF(\'Pokémon List\'!$B$2:$B${n_species + 1},F{rr})=0,"Not in the list",'
            f'IF(AND(G{rr}<>"",H{rr}<>"",G{rr}>H{rr}),"Min Lv above Max Lv","OK"))))'))
        wp.cell(row=rr, column=5).fill = GREY
        wp.cell(row=rr, column=9).fill = GREY
        wp.cell(row=rr, column=9).font = Font(name=FONT)
    dv_reg = DataValidation(type="list", formula1=f"=Lists!$A$2:$A${len(REGIONS) + 1}", allow_blank=True)
    dv_meth = DataValidation(type="list", formula1=f"=Lists!$C$2:$C${n_methods + 1}", allow_blank=True)
    dv_slot = DataValidation(type="whole", operator="between", formula1="1", formula2="12", allow_blank=True)
    dv_lv = DataValidation(type="whole", operator="between", formula1="1", formula2="100", allow_blank=True)
    dv_mon = DataValidation(type="list", formula1=f"='Pokémon List'!$B$2:$B${n_species + 1}", allow_blank=True)
    dv_mon.showErrorMessage = False      # (typing a name works too; Check flags typos)
    for v, rng in ((dv_reg, "A"), (dv_meth, "C"), (dv_slot, "D"), (dv_mon, "F")):
        wp.add_data_validation(v)
        v.add(f"{rng}2:{rng}{last}")
    wp.add_data_validation(dv_lv)
    dv_lv.add(f"G2:H{last}")
    red = PatternFill("solid", start_color="F8CBAD")
    wp.conditional_formatting.add(f"I2:I{last}", FormulaRule(formula=[f'AND(I2<>"",I2<>"OK")'], fill=red))

    # ---- Legendaries ----------------------------------------------------------------------
    lg = wb.create_sheet("Legendaries", 2)
    style_header(lg, 1, ["Region", "Area", "Pokémon", "Level", "How the player finds it", "Check", "Notes"],
                 [10, 18, 24, 7, 44, 22, 30])
    # Johto's and Hoenn's legendaries are post-game too now (only Kanto's stay in Kanto)
    statics = [r_ for r_ in rows if r_[6] in ("Legendary", "Mythical") and r_[7] != "Gym-only" and r_[0] >= 152
               and "(" not in r_[1]]
    r = 2
    for row in statics:
        for c, v in enumerate([row[5], "", row[1], "", "", None, "Suggested (every legendary not in a gym)"], 1):
            body(lg.cell(row=r, column=c, value=v), editable=c not in (6,))
        r += 1
    for rr in range(2, r + 40):
        if rr >= r:
            for c in range(1, 8):
                body(lg.cell(row=rr, column=c), editable=c != 6)
        lg.cell(row=rr, column=6, value=(
            f'=IF(C{rr}="","",IF(COUNTIF(Lists!$R$2:$R$19,C{rr})>0,"Gym-only legendary",'
            f'IF(COUNTIF(\'Pokémon List\'!$B$2:$B${n_species + 1},C{rr})=0,"Not in the list","OK")))')).fill = GREY
    lg.conditional_formatting.add(f"F2:F{r + 40}", FormulaRule(formula=[f'AND(F2<>"",F2<>"OK")'], fill=red))
    dv_reg2 = DataValidation(type="list", formula1=f"=Lists!$A$2:$A${len(REGIONS) + 1}", allow_blank=True)
    lg.add_data_validation(dv_reg2)
    dv_reg2.add(f"A2:A{r + 40}")

    # ---- Start Here ---------------------------------------------------------------------------
    st = wb.create_sheet("Start Here", 0)
    st.column_dimensions["A"].width = 110
    lines = [
        ("Exotic Pokémon Safari planner", Font(name=FONT, bold=True, size=16)),
        ("Fill in the yellow cells. Grey cells work themselves out. When you're done, send the file back "
         "and I'll build the zones from it.", None),
        ("", None),
        ("Areas", Font(name=FONT, bold=True, size=12)),
        ("One row per area in a region's zone: its name, the place in the games it's based on, how big it "
         "should be, and whether it has water (Surf, fishing) or rocks to smash. The Paldea rows are my "
         "suggestions (only the Meadow is built); change or delete any of them.", None),
        ("", None),
        ("Wild Pokémon", Font(name=FONT, bold=True, size=12)),
        ("One row per slot. Pick the region, area and method; the slot number gives its chance (Chance). "
         "Grass/Cave has 12 slots (20%, 20%, 10% x4, 5% x2, 4% x2, 1% x2); Surf and Rock Smash 5; Old Rod 2, "
         "Good Rod 3, Super Rod 5. The same Pokémon can fill several slots to add up its chance.", None),
        ("Check says OK, or what's wrong: a name that isn't in the Pokémon List (a typo), one of the 18 gym "
         "legendaries (those stay gym-only), or levels the wrong way round.", None),
        ("Example: the Paldea Meadow rows are what's in the game now (placeholders, Lv 48-54). Keep, change "
         "or replace them.", None),
        ("", None),
        ("Legendaries", Font(name=FONT, bold=True, size=12)),
        ("Every legendary and mythical Pokémon from Johto on, except the 18 gym ones, is listed with "
         "its home region (Johto's and Hoenn's aren't in Kanto any more). Give each an area, a level and how the player finds it (a puzzle, a hidden "
         "room...). Delete the ones you don't want.", None),
        ("", None),
        ("Pokémon List", Font(name=FONT, bold=True, size=12)),
        (f"All {n_species} Pokémon, Gen 1 to 9, plus the regional forms (Alolan, Galarian, Hisuian, "
         "Paldean). The ones marked \"Already in Kanto\" can be caught in the main game, but you can put "
         "them in a zone too. Filter by Region or type to browse.", None),
    ]
    for i, (text, font) in enumerate(lines, 1):
        c = st.cell(row=i, column=1, value=text)
        c.font = font or Font(name=FONT, size=11)
        c.alignment = Alignment(wrap_text=True, vertical="top")
    legend = st.cell(row=len(lines) + 2, column=1, value="Yellow = fill in")
    legend.fill = YELLOW
    legend.font = Font(name=FONT)
    legend2 = st.cell(row=len(lines) + 3, column=1, value="Grey = worked out for you")
    legend2.fill = GREY
    legend2.font = Font(name=FONT)

    wb.save(out)
    print(f"{out}: {n_species} Pokémon, {len(statics)} legendaries suggested")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
