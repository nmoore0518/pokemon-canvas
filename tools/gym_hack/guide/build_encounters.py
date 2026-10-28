#!/usr/bin/env python3
"""Pokémon Canvas: the printable wild encounter tables (a PDF), made from the game's own data.

    python3 tools/gym_hack/guide/build_encounters.py [--out FILE]

Every area the player can reach (walked from Pallet Town and the Olivine ferry through warps,
connections and script warps: the Sevii Islands aren't reachable), from src/data/wild_encounters.json
(FireRed's tables - the game is built as FireRed - and the Exotic Safari's), with each Pokémon's
share of a method's encounters (its slots added up) and its level range. The Exotic Safari's shared
slots (src/data/exotic_safari_pools.json) are split evenly between their species, as the game does
(src/exotic_safari.c PoolSpecies). Floors with the same table are merged.
"""
import argparse
import collections
import glob
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(Path(__file__).parent))
OUT_DIR = ROOT / "build/guide"

# The slots' odds (src/wild_encounter.c, FireRed's): grass/cave 12 slots, surfing and Rock Smash 5,
# fishing 10 (Old Rod 2, Good Rod 3, Super Rod 5)
RATES = {"land_mons": [20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1],
         "water_mons": [60, 30, 5, 4, 1], "rock_smash_mons": [60, 30, 5, 4, 1]}
RODS = [("Old Rod", 0, [70, 30]), ("Good Rod", 2, [60, 20, 20]), ("Super Rod", 5, [40, 40, 15, 4, 1])]
METHOD = {"land_mons": "Grass / cave", "water_mons": "Surfing", "rock_smash_mons": "Rock Smash"}
REGIONS = ["JOHTO", "HOENN", "SINNOH", "UNOVA", "KALOS", "ALOLA", "GALAR", "PALDEA"]
JOHTO_ORDER = ["ROUTE34", "ILEX_FOREST", "AZALEA_TOWN", "GOLDENROD_CITY", "ROUTE35", "NATIONAL_PARK", "ROUTE36",
               "ROUTE37", "ECRUTEAK_CITY", "BURNED_TOWER", "TIN_TOWER", "ROUTE38", "ROUTE39", "OLIVINE_CITY", "ROUTE40"]
NAME_FIXES = {  # (species whose name alone, or whose constant, reads badly)
    "SPECIES_UNOWN": "Unown A", "SPECIES_UNOWN_QUESTION": "Unown ?", "SPECIES_UNOWN_EXCLAMATION": "Unown !",
    "SPECIES_ORICORIO_POM_POM": "Oricorio (Pom-Pom)", "SPECIES_ORICORIO_PAU": "Oricorio (Pa'u)",
    "SPECIES_ORICORIO_BAILE": "Oricorio (Baile)", "SPECIES_ORICORIO_SENSU": "Oricorio (Sensu)",
    "SPECIES_SQUAWKABILLY_GREEN": "Squawkabilly (Green)", "SPECIES_SQUAWKABILLY": "Squawkabilly (Green)",
    "SPECIES_ORICORIO": "Oricorio (Baile)", "SPECIES_DARMANITAN_GALAR": "Darmanitan (Galarian)", "SPECIES_FLABEBE_RED": "Flabébé", "SPECIES_CASTFORM_NORMAL": "Castform",
}
RANDOM_FORM_FAMILIES = ["SCATTERBUG", "SPEWPA", "VIVILLON", "FLABEBE", "FLOETTE", "FLORGES", "PUMPKABOO", "GOURGEIST",
                        "DEERLING", "SAWSBUCK", "MINIOR", "FURFROU"]
FORM_WORDS = {"Galar": "Galarian", "Alola": "Alolan", "Hisui": "Hisuian", "Paldea": "Paldean"}
AREA_FIXES = {"National Park Normal": "National Park", "Olivine City Port Outside": "Olivine Port",
              "Dragons Den": "Dragon's Den", " Of ": " of ", "Mt Chimney": "Mt. Chimney",
              "Stark Inside": "Stark Mountain (inside)", "New Bark": "New Bark Town"}

CSS = """
@page { size: Letter; }
body { font-family: 'DejaVu Serif', Georgia, serif; font-size: 9.5pt; line-height: 1.35; color: #1d1d1f; }
h1 { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 22pt; margin: 0 0 6px; }
h2 { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 15pt; border-bottom: 2px solid #c33; padding-bottom: 3px;
     margin-top: 0; page-break-before: always; }
h3 { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 10.5pt; margin: 10px 0 3px; color: #333; }
.cover { height: 9.2in; display: flex; flex-direction: column; justify-content: center; align-items: center; text-align: center;
         page-break-after: always; }
.cover h1 { font-size: 36pt; }
.cover .sub { font-size: 14pt; color: #555; }
.area { page-break-inside: avoid; }
table { border-collapse: collapse; width: 100%; font-size: 8.3pt; margin: 2px 0 6px; }
th, td { border: 1px solid #bbb; padding: 2px 5px; text-align: left; vertical-align: top; }
th { background: #eee; font-family: 'DejaVu Sans', sans-serif; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
td.m { width: 78px; white-space: nowrap; font-family: 'DejaVu Sans', sans-serif; }
.mon { white-space: nowrap; }
.pct { font-weight: bold; }
.lv { color: #666; }
.note { font-size: 8.6pt; color: #444; }
ul { margin: 4px 0; }
.toc { columns: 3; font-size: 8.6pt; }
.toc a { color: #1d1d1f; text-decoration: none; }
"""


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def species_names():
    names = {}
    for f in sorted(glob.glob(str(ROOT / "src/data/pokemon/species_info/*.h"))):
        for m in re.finditer(r'\[(SPECIES_\w+)\]\s*=\s*\{(.*?)\.speciesName\s*=\s*_\("(.*?)"\)', open(f).read(), re.S):
            if "[SPECIES_" not in m[2]:
                names.setdefault(m[1], m[3])
    aliases = dict(re.findall(r"^\s+(SPECIES_\w+)\s*=\s*(SPECIES_\w+)", (ROOT / "include/constants/species.h").read_text(), re.M))
    return names, aliases


def reachable_maps(maps, name_of):
    """MAP_* ids reachable from Pallet Town and the Olivine ferry"""
    def outs(folder):
        j, o = maps[folder], set()
        o |= {w["dest_map"] for w in j.get("warp_events", [])}
        o |= {c["map"] for c in j.get("connections") or []}
        p = ROOT / f"data/maps/{folder}/scripts.inc"
        if p.exists():
            o |= set(re.findall(r"\bwarp\w*\s+(MAP_\w+)", p.read_text()))
        return o
    todo = ["MAP_PALLET_TOWN", "MAP_OLIVINE_CITY_PORT_INSIDE"]
    seen = set(todo)
    while todo:
        folder = name_of.get(todo.pop())
        if not folder:
            continue
        for d in outs(folder):
            if d not in seen:
                seen.add(d)
                todo.append(d)
    return seen


def story_order():
    """Kanto's areas in story order, with names (from export_wild_sheet.py's list)"""
    src = (ROOT / "tools/gym_hack/export_wild_sheet.py").read_text()
    block = src[src.index("AREAS = ["):]
    ns = {}
    exec(block[:block.index("]\n") + 2], ns)
    return {"MAP_" + k: (i, name) for i, (k, name, _) in enumerate(ns["AREAS"])}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(OUT_DIR / "encounters.pdf"))
    out = Path(ap.parse_args().out)
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())
    maps, name_of = {}, {}
    for g in groups["group_order"]:
        for folder in groups[g]:
            j = json.loads((ROOT / f"data/maps/{folder}/map.json").read_text())
            maps[folder] = j
            name_of[j["id"]] = folder
    reach = reachable_maps(maps, name_of)
    order = story_order()
    names, aliases = species_names()
    pools = {}
    for p in json.loads((ROOT / "src/data/exotic_safari_pools.json").read_text()):
        pools[(p["map"], p["species"][0])] = p["species"]

    def sname(sp):
        if sp in NAME_FIXES:
            return NAME_FIXES[sp]
        if sp in aliases:               # (the default form: SPECIES_DEERLING = SPECIES_DEERLING_SPRING)
            return names.get(aliases[sp]) or sp[8:].replace("_", " ").title()
        for fam in RANDOM_FORM_FAMILIES:  # (random forms in the safari: the species is the point)
            if sp.startswith(f"SPECIES_{fam}_"):
                return names.get(sp) or fam.title()
        n = names.get(sp)
        if not n:
            return sp[8:].replace("_", " ").title()
        base = re.sub(r"[^A-Z0-9]", "_", n.upper().replace("♀", "_F").replace("♂", "_M")).strip("_")
        rest = sp[8:]
        if rest.startswith(base) and len(rest) > len(base):
            words = rest[len(base):].strip("_").replace("_", " ").title()
            words = " ".join(FORM_WORDS.get(w, w) for w in words.split() if w != "Standard")
            return f"{n} ({words})"
        return n

    def area_name(mapid):
        if mapid in order:
            return order[mapid][1]
        w = re.sub(r"^EXOTIC_SAFARI_", "", mapid[4:]).replace("_", " ").title()
        w = re.sub(r"\b(B?\d+)f\b", lambda m: m[1].upper() + "F", w)
        w = re.sub(r"^Route(\d)", r"Route \1", w.replace("Pokemon", "Pokémon").replace("Frlg", "").strip())
        for a, b in AREA_FIXES.items():
            w = w.replace(a, b)
        return w

    def rows(mapid, e):
        out_rows = []

        def add(label, mons, rates):
            tab = collections.OrderedDict()
            for mon, r in zip(mons, rates):
                sp = mon["species"]
                if sp == "SPECIES_NONE":
                    continue
                group = pools.get((mapid, sp), [sp])
                for s in group:
                    t = tab.setdefault(s, [0.0, 999, 0, len(group) > 1])
                    t[0] += r / len(group)
                    t[1] = min(t[1], mon["min_level"])
                    t[2] = max(t[2], mon["max_level"])
            if tab:
                out_rows.append((label, sorted(tab.items(), key=lambda kv: -kv[1][0])))
        for f in ("land_mons", "water_mons", "rock_smash_mons"):
            if f in e:
                add(METHOD[f], e[f]["mons"], RATES[f])
        if "fishing_mons" in e:
            for rod, start, rates in RODS:
                add(rod, e["fishing_mons"]["mons"][start:start + len(rates)], rates)
        return out_rows

    areas = []
    wj = json.loads((ROOT / "src/data/wild_encounters.json").read_text())
    for e in wj["wild_encounter_groups"][0]["encounters"]:
        if "FireRed" not in e["base_label"] or e["map"] not in reach:
            continue
        r = rows(e["map"], e)
        if r:
            areas.append((e["map"], area_name(e["map"]), r))

    def section(mapid):
        if mapid.startswith("MAP_EXOTIC_SAFARI"):
            return "safari"
        return "kanto" if mapid in order else "johto"

    def johto_key(a):
        m = a[0][4:]
        return (next((i for i, k in enumerate(JOHTO_ORDER) if m.startswith(k)), 99), m)

    def safari_key(a):
        m = a[0][18:]
        return (next((i for i, r in enumerate(REGIONS) if m.startswith(r)), 99), a[1])

    kanto = sorted([a for a in areas if section(a[0]) == "kanto"], key=lambda a: order[a[0]][0])
    johto = sorted([a for a in areas if section(a[0]) == "johto"], key=johto_key)
    safari = sorted([a for a in areas if section(a[0]) == "safari"], key=safari_key)

    def merge_floors(lst):
        """Floors in a row with the same table: one entry (Tin Tower 3F-7F)"""
        merged = []
        for mapid, name, r in lst:
            m = re.match(r"(.*) (B?\d+F)$", name)
            if merged and m and merged[-1][2] == r:
                pm = re.match(r"(.*) (B?\d+F)(?:–(B?\d+F))?$", merged[-1][1])
                if pm and pm[1] == m[1]:
                    merged[-1] = (merged[-1][0], f"{pm[1]} {pm[2]}–{m[2]}", r)
                    continue
            merged.append((mapid, name, r))
        return merged

    def table(r):
        body = []
        for label, mons in r:
            cells = []
            for s, (pct, lo, hi, shared) in mons:
                p = f"{pct:.1f}".rstrip("0").rstrip(".") + "%"
                lv = f"{lo}" if lo == hi else f"{lo}–{hi}"
                cells.append(f'<span class="mon">{esc(sname(s))}{"*" if shared else ""} <span class="pct">{p}</span> '
                             f'<span class="lv">Lv {lv}</span></span>')
            body.append(f'<tr><td class="m">{label}</td><td>{", ".join(cells)}</td></tr>')
        return "<table>" + "".join(body) + "</table>"

    def region_html(title, anchor, lead, lst):
        parts = [f'<h2 id="{anchor}">{title}</h2><p class="note">{lead}</p>']
        for i, (mapid, name, r) in enumerate(merge_floors(lst)):
            parts.append(f'<div class="area"><h3 id="{anchor}{i}">{esc(name)}</h3>{table(r)}</div>')
        return "".join(parts)

    n_areas = len(kanto) + len(johto) + len(safari)
    intro = f"""
<div class="cover"><h1>Pokémon Canvas</h1><div class="sub">Wild Pokémon: every area, every encounter rate</div></div>
<h2 id="how" style="page-break-before: avoid">How to read these tables</h2>
<p>Every area you can reach that has wild Pokémon: {len(kanto)} in Kanto, {len(johto)} in Johto and {len(safari)} Exotic
Safari zones (the Sevii Islands can't be reached). Every floor of a cave or tower is its own entry; floors with the
same Pokémon share one.</p>
<ul>
<li><b>The percentage</b> is that Pokémon's share of the encounters by that method there (its encounter slots added
together); each method adds up to 100%.</li>
<li><b>Grass / cave</b> is walking in tall grass, or anywhere on a cave or tower floor. Surfing, each rod and Rock Smash
are counted on their own.</li>
<li><b>Lv</b> is the level range across all of that Pokémon's slots.</li>
<li><b>*</b> marks a shared slot in the Exotic Safari: each encounter in it is one of its species at random, so each
gets an equal part of the slot.</li>
<li>Also in the Exotic Safari: Vivillon, Flabébé, Deerling, Minior and others come in a random form; once every panel
in the Ruins of Alph is open, an Unown there can be any of its 28 forms; and after the first Zygarde is caught,
1 wild encounter in 50 in the Kalos zone is another Zygarde.</li>
</ul>
<h3>Contents</h3>
<div class="toc">{"".join(f'<div><a href="#{a}">{t}</a></div>' for a, t in (("kanto", "Kanto"), ("johto", "Johto"), ("safari", "The Exotic Safari")))}</div>
"""
    html = ("<!doctype html><html><head><meta charset='utf-8'><style>" + CSS + "</style></head><body>" + intro
            + region_html("Kanto", "kanto", "In the order you reach them in the story. Towns are listed only where "
                          "you can surf or fish.", kanto)
            + region_html("Johto", "johto", "Reached by the ferry to Olivine; level 50 to 66. From Route 34 north "
                          "to Route 40.", johto)
            + region_html("The Exotic Safari", "safari", "Eight regions' zones; level 45 to 58.", safari)
            + "</body></html>")
    hp = OUT_DIR / "encounters.html"
    hp.write_text(html)
    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        b = p.chromium.launch()
        pg = b.new_page()
        pg.goto(hp.as_uri())
        pg.pdf(path=str(out), format="Letter", print_background=True,
               display_header_footer=True, header_template="<span></span>",
               footer_template="<div style='font-size:8px;width:100%;text-align:center;color:#888'>Pokémon Canvas · "
                               "Wild Pokémon · <span class='pageNumber'></span></div>",
               margin={"top": "0.5in", "bottom": "0.6in", "left": "0.55in", "right": "0.55in"})
        b.close()
    print(out, f"({n_areas} areas)")


if __name__ == "__main__":
    main()
