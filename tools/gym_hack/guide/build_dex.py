#!/usr/bin/env python3
"""Pokémon Canvas: the printable Pokédex (a PDF), made from the game's own data.

    python3 tools/gym_hack/guide/build_dex.py [--test] [--out FILE]

Everything comes from the source (after one `make firered`, so generated headers exist):
species stats, types, abilities, evolutions and level-up moves (the preprocessed C data), front
sprites (graphics/pokemon/*), wild Pokémon (src/data/wild_encounters.json, FireRed's tables and the
Exotic Safari's), the starters (src/gym_hack.c), each type's gym legendary (src/gym_legend.c), the
safari's legendaries and roamers (src/data/exotic_safari_areas.h), and gifts and one-off
battles in the map scripts. So the guide can be rebuilt any time the game changes.

--test: one page (the first few Pokémon and a legendary), to check the look.
"""
import argparse
import json
import re
import subprocess
import sys
from collections import defaultdict, OrderedDict
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
OUT_DIR = ROOT / "build/guide"
SPRITES = OUT_DIR / "sprites"
CPP = ["arm-none-eabi-gcc", "-E", "-P", "-w", "-iquote", "include", "-I", "include", "-I", "gflib", "-I", "src",
       "-DMODERN=1", "-DTESTING=0", "-DFIRERED", "-std=gnu17", "-x", "c"]


# ---------------------------------------------------------------------------------------------
# Reading the game's data
# ---------------------------------------------------------------------------------------------
def species_numbers():
    """SPECIES_* -> number, aliases (SPECIES_OGERPON = SPECIES_OGERPON_TEAL) resolved"""
    pairs = re.findall(r"\b(SPECIES_\w+)\s*=\s*(\w+)", (ROOT / "include/constants/species.h").read_text())
    nums = {n: int(v) for n, v in pairs if v.isdigit()}
    for _ in range(4):
        for n, v in pairs:
            if not v.isdigit() and v in nums:
                nums.setdefault(n, nums[v])
    return nums


def preprocess(src):
    out = OUT_DIR / (Path(src).stem + ".i")
    if not out.exists() or out.stat().st_mtime < (ROOT / src).stat().st_mtime:
        subprocess.run(CPP + [src, "-o", str(out)], cwd=ROOT, check=True)
    return out.read_text(errors="replace")


def blocks(text, key_re):
    """`[KEY] = { ... },` blocks: key -> body (braces balanced)"""
    out = OrderedDict()
    for m in re.finditer(key_re, text):
        i = text.index("{", m.end())
        depth, j = 0, i
        while True:
            c = text[j]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        out.setdefault(m.group(1), text[i + 1:j])
    return out


def cstring(s):
    """the first C string literal(s) in s, joined"""
    parts = re.findall(r'"((?:[^"\\]|\\.)*)"', s)
    return "".join(parts).replace("\\n", " ").replace('\\"', '"')


def field(body, name):
    """the value of `.name = ...` in a struct initializer (up to its comma at the same depth)"""
    m = re.search(r"\." + name + r"\s*=\s*", body)
    if not m:
        return None
    i, depth, instr = m.end(), 0, False
    j = i
    while j < len(body):
        c = body[j]
        if instr:
            if c == "\\":
                j += 1
            elif c == '"':
                instr = False
        elif c == '"':
            instr = True
        elif c in "({[":
            depth += 1
        elif c in ")}]":
            if depth == 0:
                break
            depth -= 1
        elif c == "," and depth == 0:
            break
        j += 1
    return body[i:j].strip()


def enum_values(path, prefix):
    """NAME = value lines of an enum header"""
    vals = {}
    for m in re.finditer(r"\b(" + prefix + r"\w+)\s*=\s*(\d+)", (ROOT / path).read_text()):
        vals.setdefault(m.group(1), int(m.group(2)))
    return vals


def national_dex():
    names = re.findall(r"^\s+(NATIONAL_DEX_\w+),", (ROOT / "include/constants/pokedex.h").read_text(), re.M)
    return {n: i for i, n in enumerate(names)}           # (NATIONAL_DEX_NONE is 0)


def load():
    poke = preprocess("src/pokemon.c")
    moves_i = preprocess("src/move.c")
    items_i = preprocess("src/item.c")
    data = {}
    data["moves"] = {k: cstring(field(b, "name") or "") for k, b in blocks(moves_i, r"\[(MOVE_\w+)\]\s*=\s*\n?\s*(?=\{)").items()}
    data["move_types"] = {k: (field(b, "type") or "") for k, b in blocks(moves_i, r"\[(MOVE_\w+)\]\s*=\s*\n?\s*(?=\{)").items()}
    data["abilities"] = {k: cstring(field(b, "name") or "") for k, b in blocks(poke, r"\[(ABILITY_\w+)\]\s*=\s*\n?\s*(?=\{)").items()}
    data["items"] = {}
    for k, b in blocks(items_i, r"\[(ITEM_\w+)\]\s*=\s*\n?\s*(?=\{)").items():
        names = re.findall(r'COMPOUND_STRING\("((?:[^"\\]|\\.)*)"\)', field(b, "name") or "")
        data["items"][k] = names[0] if names else k[5:].replace("_", " ").title()
    data["learnsets"] = {}
    for m in re.finditer(r"struct LevelUpMove (s\w+LevelUpLearnset)\[\] = \{(.*?)\};", poke, re.S):
        data["learnsets"][m.group(1)] = [(int(l), mv) for mv, l in re.findall(r"\.move = (MOVE_\w+), \.level = (\d+)", m.group(2))]
    data["gfx"] = {s: p for s, p in re.findall(r"(gMon(?:FrontPic|Palette)_\w+)\[\] = INCGFX_U\d+\(\"([^\"]+)\"", poke)}
    species = blocks(poke, r"\[(SPECIES_\w+)\]\s*=\s*(?=\{)")
    data["species"] = species
    return data


# ---------------------------------------------------------------------------------------------
# One Pokémon's facts
# ---------------------------------------------------------------------------------------------
TYPE_COLORS = {
    "NORMAL": "#A8A77A", "FIRE": "#EE8130", "WATER": "#6390F0", "ELECTRIC": "#F7D02C", "GRASS": "#7AC74C",
    "ICE": "#96D9D6", "FIGHTING": "#C22E28", "POISON": "#A33EA1", "GROUND": "#E2BF65", "FLYING": "#A98FF3",
    "PSYCHIC": "#F95587", "BUG": "#A6B91A", "ROCK": "#B6A136", "GHOST": "#735797", "DRAGON": "#6F35FC",
    "DARK": "#705746", "STEEL": "#B7B7CE", "FAIRY": "#D685AD", "STELLAR": "#40B5A5",
}
STAT_KEYS = [("baseHP", "HP"), ("baseAttack", "Atk"), ("baseDefense", "Def"), ("baseSpAttack", "SpA"),
             ("baseSpDefense", "SpD"), ("baseSpeed", "Spe")]


def ceval(expr):
    """a constant C expression (numbers, comparisons, ?:) -> its value"""
    e = expr.replace("&&", " and ").replace("||", " or ").replace("/", "//")
    m = re.match(r"^\(?\s*([^?]+?)\s*\)?\s*\?\s*([^:]+?)\s*:\s*(.+)$", e)
    if m:
        return ceval(m.group(2)) if eval(m.group(1)) else ceval(m.group(3))
    return eval(e)


def parse_species(key, body, data):
    def num(name, default=0):
        v = field(body, name)
        try:
            return int(ceval(v)) if v else default
        except Exception:
            return default
    types = re.findall(r"TYPE_(\w+)", field(body, "types") or "")
    abil = re.findall(r"ABILITY_\w+", field(body, "abilities") or "")
    evos = []
    ev = field(body, "evolutions") or ""
    for e in re.findall(r"\{(EVO_\w+), ([^,]+), (SPECIES_\w+)(.*?)\}(?=\s*,\s*[\{]|\s*,\s*\})", ev):
        evos.append((e[0], e[1].strip(), e[2], e[3]))
    return {
        "key": key, "name": cstring(field(body, "speciesName") or "") or key[8:].title(),
        "dex": field(body, "natDexNum"), "types": list(dict.fromkeys(types)),
        "stats": [num(k) for k, _ in STAT_KEYS], "abilities": abil,
        "category": cstring(field(body, "categoryName") or ""),
        "description": cstring(field(body, "description") or ""),
        "height": num("height"), "weight": num("weight"),
        "front": field(body, "frontPic"), "palette": field(body, "palette"),
        "learnset": data["learnsets"].get(field(body, "levelUpLearnset") or "", []),
        "evos": evos, "mega": ".isMegaEvolution = 1" in body or ".isGigantamax = 1" in body
        or ".isTotem = 1" in body or ".isPrimalReversion = 1" in body or ".isUltraBurst = 1" in body
        or ".isTeraForm = 1" in body or ".isBattleOnly = 1" in body,
    }


def sprite(info, data):
    """the front sprite's first frame as a PNG (transparent background); its path"""
    SPRITES.mkdir(parents=True, exist_ok=True)
    out = SPRITES / f"{info['key']}.png"
    if out.exists():
        return out
    pic, pal = data["gfx"].get(info["front"]), data["gfx"].get(info["palette"])
    if not pic or not pal or not (ROOT / pic).exists():
        return None
    import render_map
    colors = render_map.load_pal(ROOT / pal)
    rgba = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    src = Image.open(ROOT / pic)
    for y in range(64):
        for x in range(64):
            c = src.getpixel((x, y))
            if isinstance(c, int) and c:
                rgba.putpixel((x, y), tuple(colors[c][:3]) + (255,))
    rgba.save(out)
    return out


# ---------------------------------------------------------------------------------------------
# Where each Pokémon is found
# ---------------------------------------------------------------------------------------------
from safari_names import SAFARI_NAMES   # noqa: E402  (tools/gym_hack/safari_names.py)
REGION_OF_SAFARI = ["Paldea", "Galar", "Alola", "Kalos", "Unova", "Sinnoh", "Hoenn", "Johto"]     # (the roamers' region index)
TYPE_NAMES = ["Normal", "Fighting", "Flying", "Poison", "Ground", "Rock", "Bug", "Ghost", "Steel", "Fire",
              "Water", "Grass", "Electric", "Psychic", "Ice", "Dragon", "Dark", "Fairy"]

# The safari legendaries: how to get each one (spoilers)
PUZZLES = {
    "OGERPON": "Find its three masks (Wellspring on Casseroya Lake's island, Hearthflame in the Asado Desert, Cornerstone in Area Zero); then it appears in South Province.",
    "WO_CHIEN": "Pull the 4 purple stakes hidden around the Paldea zone; then it appears in Tagtree Thicket.",
    "CHIEN_PAO": "Pull the 4 yellow stakes hidden around the Paldea zone; then it appears on Glaseado Mountain.",
    "TING_LU": "Pull the 4 green stakes hidden around the Paldea zone; then it appears in the Asado Desert.",
    "CHI_YU": "Pull the 4 blue stakes hidden around the Paldea zone; then it appears on Casseroya Lake's island.",
    "TERAPAGOS": "Master every gym type (a mastery battle for each); then it appears in Area Zero's depths.",
    "PECHARUNT": "Catch the Loyal Three (Okidogi, Munkidori, Fezandipiti); then it appears in Area Zero's depths.",
    "REGIELEKI": "Step on the 7 stepping stones of the lightning bolt in Postwick's countryside, from the top of the bolt down.",
    "REGIDRAGO": "Find the Dragon Fang (Galar Mine 1F), then come to Galar Mine B1F with a Dragon-type Pokémon in the lead.",
    "KUBFU": "Beat the dojo master (Bea) in Wyndon Stadium's lobby; then it appears on the pitch. Once it's caught, a second Kubfu comes to the pitch after you've been back to every Galar area.",
    "ZACIAN": "Find the Rusted Sword in Glimwood Tangle; then it appears at its plinth in Circhester.",
    "ZAMAZENTA": "Find the Rusted Shield in Galar Mine B1F; then it appears at its plinth in Circhester.",
    "ZARUDE": "Light all 8 dark mushrooms in Glimwood Tangle (4 big, 4 giant) by touching them.",
    "ENAMORUS": "Visit the pink glade in Glimwood Tangle with a Pokémon of maximum friendship in the lead.",
    "MAGEARNA": "Find its 4 machine parts (Big Wave Beach, Ruins of Conflict, Wela Volcano Park, Mount Lanakila); then it appears at Hano Beach.",
    "MELTAN": "Visit Hano Beach with a Steel-type Pokémon in the lead.",
    "COSMOG": "Catch the four Tapus (they roam the Alola zone); then it appears at Big Wave Beach.",
    "ZERAORA": "Appears in Seafolk Village only during a thunderstorm (one visit in four).",
    "MARSHADOW": "Visit the Ruins of Conflict with a Fighting- or Ghost-type Pokémon in the lead.",
    "SOLGALEO": "Find the Sun Flute in the Ruins of Conflict; then it appears in Wela Volcano Park.",
    "LUNALA": "Find the Moon Flute in Seafolk Village; then it appears on Mount Lanakila.",
    "NECROZMA": "Catch Solgaleo and Lunala; then it appears in Wela Volcano Park.",
    "ZYGARDE": "Find Zygarde Cells around the Kalos zone (12 in all). With 3 it appears at the Prism Tower's door as its 10% Forme; with 8, its 50% Forme; with all 12 it's whole (Power Construct: its 100% Complete Forme in battle), and the last Cell leaves the Zygarde Cube. Once one is caught, its other Formes turn up now and then in the Kalos zone's wild (the 50% with 8 Cells, Power Construct with all 12).",
    "HOOPA": "Take the Prison Bottle from its pedestal in Glittering Cave; Hoopa comes through a ring. The bottle turns it Unbound.",
    "YVELTAL": "Catch 10 different Kalos Pokémon; then it appears in the Lumiose Badlands.",
    "VOLCANION": "Visit Azure Bay's island with a Fire- or Water-type in the lead and a Pokémon of the other type in the party.",
    "TERRAKION": "Stands on Skyarrow Bridge's east meadow.",
    "VIRIZION": "Stands on Skyarrow Bridge's west meadow.",
    "COBALION": "Catch Terrakion and Virizion; then it appears in the Giant Chasm.",
    "KYUREM": "Catch Reshiram and Zekrom; then it appears in the Giant Chasm. The old man there then gives the DNA Splicers.",
    "RESHIRAM": "Find the Light Stone in Relic Castle; then it appears at the top of Dragonspiral Tower.",
    "ZEKROM": "Find the Dark Stone in the Abyssal Ruins; then it appears in Chargestone Cave.",
    "VICTINI": "Find the Liberty Pass on Skyarrow Bridge; then it appears in Relic Castle.",
    "KELDEO": "Catch Terrakion, Virizion and Cobalion; then it appears in the Abyssal Ruins.",
    "UXIE": "Appears on the island in Lake Verity (The Three Lakes, west) for a lead Pokémon of level 70 or higher. Surf to it.",
    "AZELF": "Appears on the island in Lake Acuity (The Three Lakes, east) for a lead Pokémon with 252 HP EVs. Surf to it.",
    "MESPRIT": "Appears on the island in Lake Valor (The Three Lakes, south) for a lead Pokémon holding a berry. Surf to it.",
    "HEATRAN": "Deep inside Stark Mountain: through the cave halfway down its slopes, at the top of the far passage.",
    "REGIGIGAS": "Wakes in Snowpoint City, west of the harbor steps, when Regirock, Regice and Registeel are all in your party.",
    "PHIONE": "Stands by the ponds in the Great Marsh's west.",
    "ARCEUS": "Master every gym type, then see the elder in Snowpoint City for the Azure Flute; Arceus then appears at the top of the Hall of Origin (Snowpoint City's cave leads up to it).",
    "REGISTEEL": "The Regi chambers' inner walls are sealed until you read the Braille tablet on the rock beside the Island Cave (Pacifidlog's cliff) with Wailord first and Relicanth last in your party. Then, in the Ancient Tomb (surf to the mound in the pond south of Route 120's bridge), read the inner wall with a lead that knows Flash; Registeel is in the upper room.",
    "REGIROCK": "The Regi chambers' inner walls are sealed until you read the Braille tablet on the rock beside the Island Cave (Pacifidlog's cliff) with Wailord first and Relicanth last in your party. Then, in the Desert Ruins (the cave in the rock west of Mt. Chimney's summit), read the inner wall with a lead that knows Rock Smash; Regirock is in the upper room.",
    "REGICE": "The Regi chambers' inner walls are sealed until you read the Braille tablet on the rock beside the Island Cave (Pacifidlog's cliff) with Wailord first and Relicanth last in your party. Then, in the Island Cave, read the inner wall and stand still in front of it for two minutes; Regice is in the upper room.",
    "KYOGRE": "Rises in the open sea west of Pacifidlog Town when your lead holds the Blue Orb (on Sootopolis City's south-west lawn). Surf to it.",
    "JIRACHI": "Wakes on the little island in Sootopolis City's lake (Sky Pillar's cave, west of Pacifidlog, leads to the city) once Regirock, Regice, Registeel, Latias, Latios, Kyogre and Deoxys are all caught. Surf to it.",
    "DEOXYS": "Find the Meteorite near Mt. Chimney's summit; then Deoxys appears on the lava lake's shore. Use the Meteorite on it to change its Forme.",
    "LUGIA": "Find the Silver Wing in the Ice Path (on the shelf by the ladder, past the ice); then the elder at Dragon's Den's shrine steps aside, and the shrine's door leads to the Whirl Islands' deepest chamber, where Lugia waits on the island in its middle.",
    "CELEBI": "Find the GS Ball in the Ruins of Alph (its west part, through the little cave in the rocks); then Celebi appears in front of Ilex Forest's shrine.",
    "TYPE_NULL": "A gift from Hano Beach's caretaker once you've caught Rowlet, Litten and Popplio.",
}
BEAST_NOTE = (" Starts roaming once you've been in all six Johto areas (New Bark Town, the Lake of Rage, the "
              "Ruins of Alph, Ilex Forest, the Ice Path, Dragon's Den); the old man in New Bark Town keeps count.")
ROAM_NOTES = {"RAIKOU": BEAST_NOTE, "ENTEI": BEAST_NOTE, "SUICUNE": BEAST_NOTE,
              "CALYREX": " Starts roaming once Spectrier is caught.",
              "LATIAS": " Starts roaming once the Soul Dew is found (on a raft in Pacifidlog Town).",
              "LATIOS": " Starts roaming once the Soul Dew is found (on a raft in Pacifidlog Town).",
              "LANDORUS": " Starts roaming once Thundurus is caught. Once both are caught, the shrine keeper on Skyarrow Bridge gives the Reveal Glass."}


def area_names():
    import export_wild_sheet as W
    names = {}
    for key, name, when in W.AREAS:
        names["MAP_" + key] = (name, when)
    return names


def safari_name(map_const):
    """MAP_EXOTIC_SAFARI_KALOS_CITY -> ('Kalos Safari', 'Lumiose City')"""
    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())
    for m in groups.get("gMapGroup_ExoticSafari", []):
        cid = "MAP_" + re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", m).upper()
        if cid == map_const:
            rest = m.split("_", 1)[1]
            region = re.match(r"[A-Z][a-z]+", rest).group(0)
            return f"{region} Safari", SAFARI_NAMES.get(rest, rest)
    return None


def prettify_map(folder):
    """CeruleanCave_B1F_Frlg -> Cerulean Cave B1F; Route12 -> Route 12; SilphCo_7F -> Silph Co. 7F"""
    name = re.sub(r"_(Frlg|Johto)$", "", folder)
    parts = []
    for part in name.split("_"):
        if re.fullmatch(r"B?\d+F|\d+R", part):
            parts.append(part)
        else:
            parts.append(re.sub(r"(?<=[a-z])(?=[A-Z0-9])", " ", part))
    out = " ".join(parts)
    for a, b in (("Silph Co", "Silph Co."), ("Pokemon", "Pokémon"), ("Ss Anne", "S.S. Anne"), ("Mt ", "Mt. ")):
        out = out.replace(a, b)
    return out


# Gifts the scripts give by variable (prizes, choices, fossils)
MANUAL = [
    ("SPECIES_OMANYTE", "Cinnabar Lab", "Revive the Helix Fossil (Mt. Moon)", "After Badge 5"),
    ("SPECIES_KABUTO", "Cinnabar Lab", "Revive the Dome Fossil (Mt. Moon)", "After Badge 5"),
    ("SPECIES_AERODACTYL", "Cinnabar Lab", "Revive the Old Amber (Pewter Museum)", "After Badge 5"),
    ("SPECIES_HITMONLEE", "Saffron Fighting Dojo", "Gift: beat the dojo, choose Hitmonlee or Hitmonchan (Lv 25)", "After Badge 5"),
    ("SPECIES_HITMONCHAN", "Saffron Fighting Dojo", "Gift: beat the dojo, choose Hitmonlee or Hitmonchan (Lv 25)", "After Badge 5"),
]
for _sp, _coins in (("ABRA", "180"), ("CLEFAIRY", "500"), ("DRATINI", "2,800"), ("SCYTHER", "5,500"), ("PORYGON", "9,999")):
    MANUAL.append((f"SPECIES_{_sp}", "Celadon Game Corner", f"Prize for {_coins} coins", "After Badge 3"))

SEVII = re.compile(r"MAP_((ONE|TWO|THREE|FOUR|FIVE|SIX|SEVEN)_ISLAND|MT_EMBER|BIRTH_ISLAND|NAVEL_ROCK|TRAINER_TOWER)")


def const_name(map_const):
    """MAP_ROUTE34 -> Route 34; MAP_ILEX_FOREST -> Ilex Forest"""
    words = []
    for w in map_const[4:].split("_"):
        m = re.fullmatch(r"([A-Z]+)(\d+)", w)
        words.append(f"{m.group(1).title()} {m.group(2)}" if m and m.group(1) not in ("B",) else
                     w if re.fullmatch(r"B?\d+F", w) else w.title())
    return " ".join(words).replace("Pokemon", "Pokémon")


def locations(data):
    """species key -> list of (sort key, html line)"""
    loc = defaultdict(list)
    names = area_names()
    wild = json.loads((ROOT / "src/data/wild_encounters.json").read_text())
    group = wild["wild_encounter_groups"][0]
    rates = {f["type"]: f["encounter_rates"] for f in group["fields"]}
    rods = next(f for f in group["fields"] if f["type"] == "fishing_mons")["groups"]
    # Slots shared by several species (the Exotic Safari: one or the other each time), and families
    # whose wild Pokémon come in any of their forms (src/exotic_safari.c)
    pools = {(pl["map"], pl["species"][0]): pl["species"]
             for pl in json.loads((ROOT / "src/data/exotic_safari_pools.json").read_text())}
    fam_src = (ROOT / "src/exotic_safari.c").read_text().split("sRandomFormFamilies")[0]
    family = {}
    for macro, stage in re.findall(r"\{(\w+)\((\w+)\)\}", fam_src):
        body = re.search(r"#define " + macro + r"\(stage\)(.*?)SPECIES_NONE", fam_src, re.S)[1]
        members = [f"SPECIES_{stage}_{x}" for x in re.findall(r"SPECIES_##stage##_(\w+)", body)]
        for m in members:
            family[m] = members
    for lst in re.findall(r"static const u16 s\w+Forms\[\] = \{(SPECIES_[^}]*?)\};", fam_src, re.S):
        members = [s for s in re.findall(r"SPECIES_\w+", lst) if s != "SPECIES_NONE"]
        for m in members:
            family[m] = members
    alias = dict(re.findall(r"^\s+(SPECIES_\w+) = (SPECIES_\w+),", (ROOT / "include/constants/species.h").read_text(), re.M))
    def canon(sp):
        while sp in alias:
            sp = alias[sp]
        return sp
    def shares(m, sp):
        names = pools.get((m, sp), [sp])
        return [(s, 1 / len(names)) for s in names]
    def note(sp):
        return " (any form)" if canon(sp) in family else ""
    for e in group["encounters"]:
        if not e["base_label"].endswith("_FireRed"):
            continue
        if SEVII.match(e["map"]):
            continue                                 # (the Sevii Islands can't be reached in this game)
        safari = safari_name(e["map"]) if "EXOTIC_SAFARI" in e["map"] else None
        if safari:
            where, when, order = f"{safari[0]}: {safari[1]}", "post-game", 2
        elif "_Johto_" in e["base_label"]:
            where, when, order = "Johto: " + const_name(e["map"]), "post-game", 2
        elif e["map"] in names:
            (where, when), order = names[e["map"]], 1
        else:
            where, when, order = const_name(e["map"]), "", 1
        for method, label in (("land_mons", "Grass/Cave"), ("water_mons", "Surf"), ("rock_smash_mons", "Rock Smash")):
            mons = e.get(method, {}).get("mons", [])
            per = defaultdict(lambda: [0, 99, 0])
            for slot, mon in enumerate(mons[:len(rates[method])]):
                for sp, share in shares(e["map"], mon["species"]):
                    p = per[sp]
                    p[0] += rates[method][slot] * share
                    p[1] = min(p[1], mon["min_level"])
                    p[2] = max(p[2], mon["max_level"])
            for sp, (pct, lo, hi) in per.items():
                lv = f"Lv {lo}" if lo == hi else f"Lv {lo}–{hi}"
                for f in family.get(canon(sp), [sp]):
                    loc[f].append((order, where, f"{label} {round(pct)}%, {lv}{note(sp)}", when))
        mons = e.get("fishing_mons", {}).get("mons", [])
        for rod, slots in rods.items():
            per = defaultdict(lambda: [0, 99, 0])
            for slot in slots:
                if slot < len(mons):
                    mon = mons[slot]
                    for sp, share in shares(e["map"], mon["species"]):
                        p = per[sp]
                        p[0] += rates["fishing_mons"][slot] * share
                        p[1] = min(p[1], mon["min_level"])
                        p[2] = max(p[2], mon["max_level"])
            total = sum(rates["fishing_mons"][s] for s in slots)
            for sp, (pct, lo, hi) in per.items():
                lv = f"Lv {lo}" if lo == hi else f"Lv {lo}–{hi}"
                rodname = rod.replace("_", " ").title()
                for f in family.get(canon(sp), [sp]):
                    loc[f].append((order, where, f"{rodname} {round(100 * pct / total)}%, {lv}{note(sp)}", when))

    # Starters, gym legendaries
    for sp, kind in re.findall(r"\{ (SPECIES_\w+),\s+TYPE_\w+\s*\}, // (\w+)", (ROOT / "src/gym_hack.c").read_text()):
        loc[sp].append((0, "Starter", f"Oak's lab: choose the {kind} type", ""))
    for t, sp in re.findall(r"\[TYPE_(\w+)\]\s*=\s*(SPECIES_\w+),", (ROOT / "src/gym_legend.c").read_text()):
        loc[sp].append((0, "Gym legendary", f"Appears in your gym when it's {t.title()} type (Lv 70). If it gets away, it hides in Cerulean Cave (1%).", ""))

    # The safari's legendaries
    areas_h = (ROOT / "src/data/exotic_safari_areas.h").read_text()
    for mp, sp in re.findall(r"MAP_NUM\((MAP_EXOTIC_SAFARI_\w+)\), (SPECIES_\w+), FLAG_EXOTIC_LEGEND", areas_h):
        s = safari_name(mp)
        loc[sp].append((2, f"{s[0]}: {s[1]}", "Legendary (Lv 70). " + PUZZLES.get(sp[8:], ""), "post-game"))
    for sp, need, region in re.findall(r"\{(SPECIES_\w+), FLAG_EXOTIC_LEGEND_\w+, FLAG_TEMP_\w+, (\w+), (\d)\}", areas_h):
        loc[sp].append((2, f"{REGION_OF_SAFARI[int(region)]} Safari", "Roams: 1 in 8 chance in any area you enter (Lv 70)." + ROAM_NOTES.get(sp[8:], ""), "post-game"))
    for sp, place, how, when in MANUAL:
        loc[sp].append((1, place, how, when))
    loc["SPECIES_MOLTRES"].append((1, "Victory Road 2F", "One-time battle (Lv 50)", "After Badge 8"))
    loc["SPECIES_TYPE_NULL"].append((2, "Alola Safari: Hano Beach", "Gift. " + PUZZLES["TYPE_NULL"], "post-game"))

    # Gifts and one-off battles in Kanto's map scripts
    for f in sorted((ROOT / "data/maps").glob("*/scripts.inc")):
        folder = f.parent.name
        if folder.startswith(("ExoticSafari", "Test")):
            continue
        text = f.read_text(errors="replace")
        if re.match(r"(One|Two|Three|Four|Five|Six|Seven)Island|MtEmber|BirthIsland|NavelRock|TrainerTower", folder):
            continue
        if not (folder.endswith("_Frlg") or folder.startswith("PalletTown")):
            continue                                 # (Emerald's maps aren't in FireRed's world)
        for kind, sp, lv in re.findall(r"\b(givemon|setwildbattle)\s+(SPECIES_\w+),\s*(\d+)", text):
            if folder.startswith(("PokemonTower_6F", "CinnabarIsland_PokemonLab", "CeladonCity_GameCorner", "SaffronCity_Dojo")):
                continue                             # (the ghost: it can't be caught)
            what = ("Gift" if kind == "givemon" else "One-time battle") + f" (Lv {lv})"
            where = prettify_map(folder)
            loc[sp].append((1, where, what, ""))
    return loc


# ---------------------------------------------------------------------------------------------
# Evolutions
# ---------------------------------------------------------------------------------------------
def evo_text(method, param, cond, data):
    p = param
    if method == "EVO_LEVEL":
        t = f"Lv {p}" if p != "0" else "Level up"
    elif method in ("EVO_ITEM", "EVO_ITEM_HOLD"):
        t = data["items"].get(p, p.replace("ITEM_", "").replace("_", " ").title())
    elif method.startswith("EVO_FRIENDSHIP"):
        t = "High friendship"
    elif method == "EVO_TRADE":
        return None
    else:
        t = method[4:].replace("_", " ").title()
    reg = re.search(r"IF_REGION, REGION_(\w+)", cond)
    if reg and reg.group(1) != "KANTO":
        return None                                  # (only in another region: not here)
    tm = re.search(r"IF_(NOT_)?TIME, TIME_(\w+)", cond)
    if tm:
        t += f", {'not ' if tm.group(1) else ''}{tm.group(2).lower()}"
    if "IF_MIN_FRIENDSHIP" in cond:
        t = "High friendship" if t == "Level up" else t + ", high friendship"
    hold = re.search(r"IF_HOLD_ITEM, (ITEM_\w+)", cond)
    if hold:
        t += f" holding {data['items'].get(hold.group(1), hold.group(1))}"
    if "IF_ATK_GT_DEF" in cond:
        t += ", Atk > Def"
    elif "IF_ATK_LT_DEF" in cond:
        t += ", Atk < Def"
    elif "IF_ATK_EQ_DEF" in cond:
        t += ", Atk = Def"
    if "IF_AMPED_NATURE" in cond:
        t += ", Amped nature"
    elif "IF_LOW_KEY_NATURE" in cond:
        t += ", Low Key nature"
    if "IF_GENDER, MON_FEMALE" in cond:
        t += ", female"
    elif "IF_GENDER, MON_MALE" in cond:
        t += ", male"
    if "IF_KNOWS_MOVE" in cond:
        mv = re.search(r"IF_KNOWS_MOVE, (MOVE_\w+)", cond)
        if mv:
            t += f" knowing {data['moves'].get(mv.group(1), mv.group(1))}"
    return t


# ---------------------------------------------------------------------------------------------
# The page
# ---------------------------------------------------------------------------------------------
CSS = """
@page { size: Letter; margin: 0.3in; }
* { box-sizing: border-box; }
body { margin: 0; font-family: 'DejaVu Sans', Arial, sans-serif; color: #1d1d1f; }
.page { width: 7.9in; height: 10.35in; display: grid; grid-template-columns: 1fr 1fr; grid-auto-flow: row dense;
        grid-template-rows: repeat(3, 3.38in); gap: 0.1in; page-break-after: always; }
.card { border: 1.5px solid #333; border-radius: 8px; padding: 6px 7px; overflow: hidden;
        display: flex; flex-direction: column; font-size: 7.6pt; line-height: 1.28; }
.desc { font-style: italic; color: #555; font-size: 6.9pt; margin-top: 2px; }
.card.compact { font-size: 6.9pt; line-height: 1.2; }
.card.compact .pic { width: 84px; height: 84px; }
.card.compact .moves { font-size: 6.4pt; }
.card.compact .desc { display: none; }
.head { display: flex; align-items: center; gap: 6px; border-bottom: 1px solid #bbb; padding-bottom: 3px; }
.num { font-weight: 700; color: #666; font-size: 8pt; }
.name { font-weight: 800; font-size: 11pt; flex: 1; }
.form { font-weight: 600; font-size: 8pt; color: #555; }
.type { color: white; font-weight: 700; font-size: 6.8pt; padding: 1px 6px; border-radius: 8px;
        text-transform: uppercase; letter-spacing: 0.3px; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
.top { display: flex; gap: 6px; margin-top: 3px; }
.pic { width: 104px; height: 104px; image-rendering: pixelated; flex: none; }
.meta { flex: 1; }
.stats { display: grid; grid-template-columns: 22px 20px 1fr; gap: 0 3px; align-items: center; margin: 1px 0 2px; }
.bar { height: 5px; border-radius: 3px; background: #4a90d9; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
.sv { text-align: right; font-weight: 700; }
.label { font-weight: 700; color: #444; }
.sec { margin-top: 3px; }
.moves { margin-top: 1px; }
.moves { font-size: 7pt; line-height: 1.32; }
.mv { white-space: nowrap; }
.lv { color: #777; font-size: 6pt; }
.where { margin: 1px 0 0; padding-left: 10px; }
.where li { margin: 0 0 1px; }
.when { color: #777; }
.missing { color: #a33; font-style: italic; }
"""


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def card(info, data, loc, prevo, family_line):
    types = "".join(f'<span class="type" style="background:{TYPE_COLORS.get(t, "#888")}">{t.title()}</span>' for t in info["types"])
    pic = sprite(info, data)
    img = f'<img class="pic" src="{pic.as_uri()}">' if pic else '<div class="pic"></div>'
    total = sum(info["stats"])
    def color(v):
        return "#e0533b" if v < 50 else "#f0a030" if v < 80 else "#e8d23a" if v < 100 else "#6cc04a" if v < 120 else "#2fa6a0"
    stats = "".join(f'<span class="label">{lab}</span><span class="sv">{v}</span><div class="bar" style="width:{min(100, v / 1.6):.0f}%;background:{color(v)}"></div>'
                    for (_, lab), v in zip(STAT_KEYS, info["stats"]))
    abil = [data["abilities"].get(a, a) for a in info["abilities"]]
    abil_txt = ", ".join(dict.fromkeys(a for a in abil[:2] if a and a != "-------" and a != "None"))
    if len(abil) > 2 and abil[2] and abil[2] not in ("-------", "None") and abil[2] not in abil[:2]:
        abil_txt += f" · <i>Hidden: {abil[2]}</i>"
    moves = " · ".join(f'<span class="mv"><span class="lv">{"Evo" if l == 0 else l}</span> {esc(data["moves"].get(m, m))}</span>'
                       for l, m in info["learnset"])
    lines = by_method(loc.get(info["key"], []))
    if prevo:
        lines.append(f"<li>Evolve {esc(prevo)}</li>")
    if not lines:
        lines.append(f'<li class="missing">{missing_text(info)}</li>')
    dex = info["dex_num"]
    form = info.get("form")
    return f"""<div data-key="{info['key']}" class="card{' tall' if info.get('tall') else ''}{' compact' if info.get('compact') else ''}">
  <div class="head"><span class="num">#{dex:04d}</span><span class="name">{esc(info['name'])}{f' <span class="form">{esc(form)}</span>' if form else ''}</span>{types}</div>
  <div class="top">{img}<div class="meta">
    <div><span class="label">{esc(info['category'])} Pokémon</span> · {info['height'] / 10:g} m · {info['weight'] / 10:g} kg</div>
    <div class="stats">{stats}</div>
    <div><span class="label">Total</span> {total}</div>
    <div class="sec"><span class="label">Abilities:</span> {abil_txt}</div>
  </div></div>
  <div class="desc">{esc(info['description'])}</div>
  <div class="sec"><span class="label">Evolution:</span> {family_line}</div>
  <div class="sec"><span class="label">Where to find</span><ul class="where">{''.join(lines)}</ul></div>
  <div class="sec"><span class="label">Level-up moves</span><div class="moves">{moves}</div></div>
</div>"""


WILD = re.compile(r"^(Grass/Cave|Surf|Rock Smash|Old Rod|Good Rod|Super Rod) (\d+)%, Lv (\d+)(?:–(\d+))?$")
FLOOR = re.compile(r"\s+(B?\d+F|\d+R|Room \d+|Entrance|Back|\(stairs\)|Summit Path|Ruby Path)$")
METHOD_LABEL = {"Grass/Cave": "Grass/Cave", "Surf": "Surf", "Rock Smash": "Rock Smash",
                "Old Rod": "Fishing (Old Rod)", "Good Rod": "Fishing (Good Rod)", "Super Rod": "Fishing (Super Rod)"}


def place_list(places, most=8):
    """['Route 11', 'Route 12', 'Mt. Moon', 'Johto: Route 34', 'Johto: Ilex Forest']
    -> 'Routes 11, 12, Mt. Moon, Johto: Route 34, Ilex Forest' (at most `most` names, then 'and N more')"""
    groups = OrderedDict()                 # prefix ('' or 'Johto') -> [routes], [places]
    for p in places:
        pre, _, name = p.rpartition(": ")
        g = groups.setdefault(pre, ([], []))
        m = re.fullmatch(r"Route (\d+)(?: North| South)?", name)
        if m:
            if int(m.group(1)) not in g[0]:
                g[0].append(int(m.group(1)))
        elif name not in g[1]:
            g[1].append(name)
    parts, count = [], 0
    for pre, (routes, names) in groups.items():
        items = []
        if routes:
            routes.sort()
            items.append(f"Route{'s' if len(routes) > 1 else ''} {', '.join(map(str, routes))}")
            count += len(routes)
        items += names
        count += len(names)
        parts.append((pre + ": " if pre else "") + ", ".join(items))
    text = "; ".join(parts)
    total = len(places)
    words = text.split(", ")
    if len(words) > most:
        text = ", ".join(words[:most]) + f" and {len(words) - most} more"
    return text


def by_method(where):
    """the wild ones grouped by how they're caught, one line each (main game, then post-game); the
    rest (gifts, starters, legendaries...) a line each"""
    lines, groups = [], OrderedDict()
    for order, place, how, when in sorted(where, key=lambda w: (w[0], w[1])):
        m = WILD.match(how)
        if not m:
            when_txt = f' <span class="when">({when})</span>' if when and when not in ("Start", "post-game") else ""
            lines.append((order, f"<li><b>{esc(place)}</b>: {esc(how)}{when_txt}</li>"))
            continue
        base = place
        while FLOOR.search(base):
            base = FLOOR.sub("", base)
        post = order >= 2 or when == "post-game"
        g = groups.setdefault((post, m.group(1)), {"places": [], "lv": [], "pct": [], "when": []})
        if base not in g["places"]:
            g["places"].append(base)
        g["pct"].append(int(m.group(2)))
        g["lv"] += [int(m.group(3)), int(m.group(4) or m.group(3))]
        g["when"].append(when)
    out = []
    for (post, method), g in sorted(groups.items(), key=lambda kv: (kv[0][0], list(METHOD_LABEL).index(kv[0][1]))):
        lo, hi = min(g["lv"]), max(g["lv"])
        lv = f"Lv {lo}" if lo == hi else f"Lv {lo}–{hi}"
        bits = [lv]
        if len(g["places"]) == 1:
            plo, phi = min(g["pct"]), max(g["pct"])
            bits.append(f"{plo}%" if plo == phi else f"{plo}–{phi}%")
            when = next((w for w in g["when"] if w and w not in ("Start", "post-game")), "")
            if when:
                bits.append(when)
        label = METHOD_LABEL[method] + (" (post-game)" if post else "")
        out.append(f'<li><b>{label}</b>: {esc(place_list(g["places"]))} <span class="when">({esc(", ".join(bits))})</span></li>')
    main = [l for o, l in lines if o < 2]
    late = [l for o, l in lines if o >= 2]
    wild_main = [l for l in out if "(post-game)" not in l]
    wild_post = [l for l in out if "(post-game)" in l]
    return main + wild_main + late + wild_post


def merge_floors(where):
    """one line per cave or building, not per floor: 'Seafoam Islands (1F, B1F, B2F): Grass/Cave 5–24%, Lv 22–30'"""
    rx = re.compile(r"^(.*?) (\d+)%, Lv (\d+)(?:–(\d+))?$")
    suf = re.compile(r"\s+(B?\d+F|\d+R|Room \d+|Entrance|Back|\(stairs\))$")
    groups, out = OrderedDict(), []
    for order, place, how, when in where:
        m = rx.match(how)
        base, tail = place, []
        while suf.search(base):
            tail.insert(0, suf.search(base).group(1))
            base = suf.sub("", base)
        if not m or not tail:
            out.append((order, place, how, when))
            continue
        key = (order, base, m.group(1), when)
        g = groups.setdefault(key, {"parts": [], "pct": [], "lv": []})
        g["parts"].append(" ".join(tail))
        g["pct"].append(int(m.group(2)))
        g["lv"] += [int(m.group(3)), int(m.group(4) or m.group(3))]
    for (order, base, method, when), g in groups.items():
        parts = list(dict.fromkeys(g["parts"]))
        if len(parts) == 1:
            place = f"{base} {parts[0]}"
        elif len(parts) <= 4:
            place = f"{base} ({', '.join(parts)})"
        else:
            place = f"{base} ({len(parts)} areas)"
        lo, hi = min(g["pct"]), max(g["pct"])
        pct = f"{lo}%" if lo == hi else f"{lo}–{hi}%"
        llo, lhi = min(g["lv"]), max(g["lv"])
        lv = f"Lv {llo}" if llo == lhi else f"Lv {llo}–{lhi}"
        out.append((order, place, f"{method} {pct}, {lv}", when))
    return out


def missing_text(info):
    n = info["dex_num"]
    if 152 <= n <= 251:
        return "Not in the game yet: coming with the Johto Safari Zone."
    if 252 <= n <= 386:
        return "Not in the game yet: coming with the Hoenn Safari Zone."
    if 387 <= n <= 493:
        return "Not in the game yet: coming with the Sinnoh Safari Zone."
    return "Not obtainable yet."


def family(info, by_key, data, parents):
    """the evolution line, from its first stage: 'A → B (Lv 16) → C (Lv 32)'"""
    root = info["key"]
    seen = set()
    while root in parents and root not in seen:
        seen.add(root)
        root = parents[root][0]
    def walk(k):
        sp = by_key.get(k)
        if not sp:
            return esc(k[8:].title())
        nm = sp["name"] + (f" ({sp['form']})" if sp.get("form") else "")
        label = f"<b>{esc(nm)}</b>" if k == info["key"] else esc(nm)
        kids = [(t, txt) for t, txt in sp.get("children", [])]
        if not kids:
            return label
        step = lambda t, txt: f'<span class="lv">({esc(txt)})</span> {walk(t)}'
        if len(kids) == 1:
            return f"{label} → {step(*kids[0])}"
        return f"{label} → " + " / ".join(step(t, txt) for t, txt in kids)
    line = walk(root)
    return line if "→" in line else "Does not evolve"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--test", action="store_true")
    ap.add_argument("--out", default=None)
    args = ap.parse_args()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    data = load()
    dexnums = national_dex()
    infos = OrderedDict()
    for key, body in data["species"].items():
        if key in ("SPECIES_NONE", "SPECIES_EGG"):
            continue
        info = parse_species(key, body, data)
        if not info["dex"] or info["mega"]:
            continue
        info["dex_num"] = dexnums.get(info["dex"], 0)
        if not info["dex_num"]:
            continue
        infos[key] = info
    # evolution links
    parents = {}
    for k, sp in infos.items():
        sp["children"] = []
        for method, param, target, cond in sp["evos"]:
            txt = evo_text(method, param, cond, data)
            if txt is None or target not in infos:
                continue
            sp["children"].append((target, txt))
            parents.setdefault(target, (k, txt))
    loc = locations(data)
    # (the data's names for a species can be aliases: SPECIES_TORNADUS is SPECIES_TORNADUS_INCARNATE)
    nums = species_numbers()
    canon = {}
    for k in infos:
        if nums.get(k) is not None:
            canon.setdefault(nums[k], k)
    merged = defaultdict(list)
    for k, v in loc.items():
        merged[canon.get(nums.get(k), k)] += v
    loc = merged
    # which entries: every species' base form, plus other forms that can be found or evolved into
    base_of = {}
    for k, sp in infos.items():
        base_of.setdefault(sp["dex_num"], k)
    chosen = []
    for k, sp in infos.items():
        is_base = base_of[sp["dex_num"]] == k
        if is_base or loc.get(k) or (k in parents and (loc.get(parents[k][0]) or base_of[infos[parents[k][0]]["dex_num"]] == parents[k][0] and False)):
            chosen.append(k)
    # a species' first form with nothing of its own, when another of its forms can be found: left out
    by_dex = defaultdict(list)
    for k in chosen:
        by_dex[infos[k]["dex_num"]].append(k)
    for n, ks in by_dex.items():
        if len(ks) > 1:
            for k in ks:
                if not loc.get(k) and k not in parents:
                    chosen.remove(k)
    # form names: what the constant adds to the species' own name (SPECIES_DEERLING_WINTER: Winter)
    REGIONAL = {"ALOLA": "Alolan Form", "GALAR": "Galarian Form", "HISUI": "Hisuian Form", "PALDEA": "Paldean Form",
                "F": "Female", "M": "Male"}
    per_dex = defaultdict(int)
    for k in chosen:
        per_dex[infos[k]["dex_num"]] += 1
    for k in chosen:
        sp = infos[k]
        root = sp["dex"][len("NATIONAL_DEX_"):]
        suffix = k[8:][len(root):].strip("_") if k[8:].startswith(root) else ""
        if not suffix:
            sp["form"] = None
            continue
        words = suffix.split("_")
        if words[0] in REGIONAL:
            label = REGIONAL[words[0]] + (" (" + " ".join(w.title() for w in words[1:]) + ")" if words[1:] else "")
        else:
            label = " ".join(REGIONAL.get(w, w.title()) for w in words)
        sp["form"] = label if (per_dex[sp["dex_num"]] > 1 or k != base_of[sp["dex_num"]]) else label
    chosen.sort(key=lambda k: (infos[k]["dex_num"], 0 if base_of[infos[k]["dex_num"]] == k else 1))
    if args.test:
        chosen = ["SPECIES_BULBASAUR", "SPECIES_IVYSAUR", "SPECIES_VENUSAUR", "SPECIES_CHARMANDER", "SPECIES_CHARMELEON", "SPECIES_RESHIRAM"]
    def render_cards(tall):
        cards = []
        for k in chosen:
            sp = infos[k]
            sp["tall"] = k in tall
            sp["compact"] = k in compact and k not in tall
            prevo = None
            if k in parents:
                pk, txt = parents[k]
                prevo = f"{infos[pk]['name']} ({txt})"
            cards.append((k, card(sp, data, loc, prevo, family(sp, infos, data, parents))))
        # pages of 3 rows x 2 columns, filled in dex order, left to right and top to bottom; a tall
        # card takes two rows of one column (if it can't fit where it's next, it moves on and the
        # spot stays empty: the order never changes)
        cells = [(r, c) for r in range(3) for c in range(2)]
        pages, cur, free, at = [], [], set(cells), 0
        for k, c in cards:
            t = k in tall
            while True:
                spot = next((i for i in range(at, 6) if cells[i] in free
                             and (not t or (cells[i][0] + 1, cells[i][1]) in free)), None)
                if spot is not None:
                    break
                pages.append(cur)
                cur, free, at = [], set(cells), 0
            r, col = cells[spot]
            free.discard((r, col))
            if t:
                free.discard((r + 1, col))
            at = spot + 1
            style = f'grid-row: {r + 1} / span {2 if t else 1}; grid-column: {col + 1};'
            cur.append(c.replace('<div data-key=', f'<div style="{style}" data-key=', 1))
        if cur:
            pages.append(cur)
        html = ("<!doctype html><html><head><meta charset='utf-8'><style>" + CSS + "</style></head><body>"
                + "".join(f'<div class="page">{"".join(p)}</div>' for p in pages) + "</body></html>")
        return cards, pages, html

    from playwright.sync_api import sync_playwright
    hp = OUT_DIR / ("dex_test.html" if args.test else "dex.html")
    out = Path(args.out) if args.out else OUT_DIR / ("dex_test.pdf" if args.test else "dex.pdf")
    with sync_playwright() as p:
        b = p.chromium.launch()
        pg = b.new_page()
        # cards whose text runs over: smaller print first (no dex text), then twice the room
        tall, compact = set(), set()
        for _ in range(3):
            cards, pages, html = render_cards(tall)
            hp.write_text(html)
            pg.goto(hp.as_uri())
            pg.emulate_media(media="print")
            over = pg.evaluate("""() => Object.fromEntries([...document.querySelectorAll('.card')].map(c => [c.dataset.key, c.scrollHeight > c.clientHeight + 1]))""")
            new = {k for k, o in over.items() if o}
            if not new:
                break
            tall |= new & compact
            compact |= new
        pg.pdf(path=str(out), format="Letter", margin={"top": "0.3in", "bottom": "0.3in", "left": "0.3in", "right": "0.3in"},
               print_background=True)
        b.close()
    print(out, len(cards), "entries,", len(pages), "pages")


if __name__ == "__main__":
    main()
