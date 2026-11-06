#!/usr/bin/env python3
"""Pokémon Canvas: every item the player can get, and where (for the player's guide's item list).

    python3 tools/gym_hack/guide/item_list.py          (prints the list, for checking)

From the maps the game can reach (Kanto, Johto, the Exotic Safari, the Leader Academy; not the
Sevii Islands or Hoenn): item balls, hidden items, shops (pokemart lists) and what their scripts give
(giveitem / finditem, and items set into a var for a shared script to give), following each map's
scripts through goto/call into the shared ones. The things C decides (prizes, the Ball Maker, the
mentor's items...) are in MANUAL. Icons come from the game's own pictures and palettes.
"""
import json
import re
import sys
from collections import OrderedDict, defaultdict
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/gym_hack/guide"))
import build_dex as D   # noqa: E402

ICONS = ROOT / "build/guide/item_icons"
GROUPS = re.compile(r"_Frlg$|^gMapGroup_(Johto|ExoticSafari|LeaderAcademy)$")
NOT_REACHED = re.compile(r"^((One|Two|Three|Four|Five|Six|Seven)Island|MtEmber|TrainerTower|NavelRock|BirthIsland|"
                         r"TanobyRuins|AlteringCave|PatternBush|DottedHole|LostCave|IcefallCave|BerryForest|BondBridge|"
                         r"CapeBrink|KindleRoad|TreasureBeach|ResortGorgeous|WaterLabyrinth|SevaultCanyon|RuinValley|"
                         r"GreenPath|OutcastIsland|WaterPath|MemorialPillar|BattleColosseum|TradeCenter|RecordCorner|"
                         r"UnionRoom|SeviiIsle|DebugMap)")
POCKETS = ["Items", "Medicine", "Poké Balls", "Battle Items", "Hold Items", "Mega Stones", "Berries", "TMs & HMs",
           "Tools", "Key Items"]
POCKET_IDS = ["POCKET_ITEMS", "POCKET_MEDICINE", "POCKET_POKE_BALLS", "POCKET_BATTLE_ITEMS", "POCKET_HOLD_ITEMS",
              "POCKET_MEGA_STONES", "POCKET_BERRIES", "POCKET_TM_HM", "POCKET_TOOLS", "POCKET_KEY_ITEMS"]
MEDICINE = {"ITEM_TYPE_HEALTH_RECOVERY", "ITEM_TYPE_STATUS_RECOVERY", "ITEM_TYPE_PP_RECOVERY", "ITEM_TYPE_STAT_BOOST_DRINK",
            "ITEM_TYPE_STAT_BOOST_FEATHER", "ITEM_TYPE_STAT_BOOST_MOCHI", "ITEM_TYPE_LEVEL_UP_ITEM"}
BATTLE = {"ITEM_TYPE_X_ITEM", "ITEM_TYPE_FLUTE", "ITEM_TYPE_AUX_ITEM"}
HOLD = {"ITEM_TYPE_HELD_ITEM", "ITEM_TYPE_TYPE_BOOST_HELD_ITEM", "ITEM_TYPE_SPECIAL_HELD_ITEM", "ITEM_TYPE_EV_BOOST_HELD_ITEM",
        "ITEM_TYPE_INCENSE", "ITEM_TYPE_GEM", "ITEM_TYPE_PLATE", "ITEM_TYPE_MEMORY", "ITEM_TYPE_DRIVE",
        "ITEM_TYPE_CONTEST_HELD_ITEM"}

# What C decides, or a script gives through a variable: (items, where, how)
TYPES18 = ["NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL", "FIRE", "WATER", "GRASS",
           "ELECTRIC", "PSYCHIC", "ICE", "DRAGON", "DARK", "FAIRY"]
MANUAL = [
    ([f"ITEM_{t}_BALL" for t in TYPES18], "Pallet Town: Dad's lab (basement)",
     "made by the Ball Maker; your mom gives you your gym type's after Nugget Bridge"),
    (["ITEM_GRASS_ENERGY", "ITEM_FIRE_ENERGY", "ITEM_WATER_ENERGY", "ITEM_LIGHTNING_ENERGY", "ITEM_PSYCHIC_ENERGY",
      "ITEM_FIGHTING_ENERGY", "ITEM_DOUBLE_COLORLESS_ENERGY"], "Saffron City: Rod's card house",
     "beat Grandmaster Rod at the Trading Card Game: the energy of the deck you played (Colorless: Double Colorless)"),
    (["ITEM_LIMITED_EDITION_SODA"], "Celadon Dept. Store: the roof",
     "the girl's prize with the Drink Machine; one more for each type you master; rarely, a tournament prize"),
    (["ITEM_MOVE_EXTRACTOR"], "Nugget Bridge, Saffron City, Viridian City",
     "the Team Rocket grunt (three times); an ultra rare prize after that (gym challenges, the tournament)"),
]
MENTOR_ITEMS = "your mentor's type items"


MOVES = {}
TM_ICON, TM_PAL = "gItemIcon_TM", "gItemIconPalette_NormalTMHM"


def tm_aliases():
    """ITEM_TM09 -> ITEM_TM_BULLET_SEED (the numbered names the scripts use)"""
    text = (ROOT / "include/constants/tms_hms.h").read_text()
    tms = re.findall(r"F\((\w+)\)", text.split("#define FOREACH_HM")[0])
    hms = re.findall(r"F\((\w+)\)", text.split("#define FOREACH_HM")[1].split("#define FOREACH_TMHM")[0])
    out = {f"ITEM_TM{i + 1:02d}": f"ITEM_TM_{m}" for i, m in enumerate(tms)}
    out.update({f"ITEM_HM{i + 1:02d}": f"ITEM_HM_{m}" for i, m in enumerate(hms)})
    # (and the old names: ITEM_OAKS_PARCEL = ITEM_PARCEL)
    out.update(dict(re.findall(r"^\s+(ITEM_\w+) = (ITEM_\w+),", (ROOT / "include/constants/items.h").read_text(), re.M)))
    return out


def item_info():
    """ITEM_X -> {name, pocket, icon (path, palette path)}"""
    text = D.preprocess("src/item.c")
    gfx = dict(re.findall(r"(gItemIcon(?:Palette)?_\w+)\[\] = INCGFX_U\d+\(\"([^\"]+)\"",
                          (ROOT / "src/data/graphics/items.h").read_text()))
    out = OrderedDict()
    for k, b in D.blocks(text, r"\[(ITEM_\w+)\]\s*=\s*\n?\s*(?=\{)").items():
        names = re.findall(r'COMPOUND_STRING\("((?:[^"\\]|\\.)*)"\)', D.field(b, "name") or "")
        if not names or not names[0].strip() or names[0] in ("?????",):
            continue
        pocket = (D.field(b, "pocket") or "").strip()
        sort = (D.field(b, "sortType") or "").strip()
        use = (D.field(b, "fieldUseFunc") or "").strip()
        if pocket == "POCKET_ITEMS":
            pocket = ("POCKET_MEDICINE" if sort in MEDICINE else "POCKET_BATTLE_ITEMS" if sort in BATTLE
                      else "POCKET_HOLD_ITEMS" if sort in HOLD else "POCKET_MEGA_STONES" if sort == "ITEM_TYPE_MEGA_STONE"
                      else "POCKET_ITEMS")
        elif pocket == "POCKET_KEY_ITEMS" and use and use != "ItemUseOutOfBattle_CannotUse" and use != "NULL":
            pocket = "POCKET_TOOLS"
        desc = " ".join(x.replace("\\n", " ") for x in re.findall(r'"((?:[^"\\]|\\.)*)"', D.field(b, "description") or ""))
        hold = (D.field(b, "holdEffect") or "").strip()
        icon = (D.field(b, "iconPic") or "").strip()
        pal = (D.field(b, "iconPalette") or "").strip()
        name = names[0].replace("\\n", " ")
        if pocket == "POCKET_TM_HM" and re.fullmatch(r"(TM|HM)\d+", name) and k.count("_") >= 2:
            move = "MOVE_" + k.split("_", 2)[2]
            name += " " + MOVES.get(move, move[5:].replace("_", " ").title())
        out[k] = {"name": name, "pocket": pocket, "desc": desc, "hold": hold,
                  "icon": gfx.get(icon) or gfx.get(TM_ICON), "pal": gfx.get(pal) or gfx.get(TM_PAL)}
    return out


def icon_png(key, info):
    """the item's icon as a PNG (transparent background); its path, or None"""
    if not info.get("icon") or not info.get("pal"):
        return None
    ICONS.mkdir(parents=True, exist_ok=True)
    out = ICONS / f"{key}.png"
    if out.exists():
        return out
    try:
        im = Image.open(ROOT / info["icon"])
        lines = (ROOT / info["pal"]).read_text().splitlines()
        cols = [tuple(int(v) for v in l.split()) for l in lines[3:19]]
        rgba = Image.new("RGBA", im.size)
        px = im.load()
        for y in range(im.size[1]):
            for x in range(im.size[0]):
                i = px[x, y]
                if i:
                    rgba.putpixel((x, y), cols[i % 16] + (255,))
        rgba.save(out)
        return out
    except Exception:
        return None


# ---------------------------------------------------------------------------------------------
# The maps and their scripts
# ---------------------------------------------------------------------------------------------
def reachable_maps():
    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())
    out = []
    for g in groups["group_order"]:
        if GROUPS.search(g):
            out += [m for m in groups[g] if not NOT_REACHED.match(m)]
    return out


def all_labels():
    """label -> (file, its segment's lines); and each file's labels in order (for falling through)"""
    labels, order = {}, {}
    for f in list((ROOT / "data").rglob("*.inc")) + list((ROOT / "data").rglob("*.s")):
        cur, lines, names = None, [], []
        for line in f.read_text(errors="replace").splitlines():
            m = re.match(r"^(\w+)::?\s*$", line)
            if m:
                if cur:
                    labels[cur] = (f, lines)
                cur, lines = m.group(1), []
                names.append(cur)
            elif cur:
                lines.append(line.split("@")[0].strip())
        if cur:
            labels[cur] = (f, lines)
        order[f] = names
    return labels, order


FINDS = re.compile(r"^(giveitem|finditem|additem)\s+(ITEM_\w+)")
GIVEMSG = re.compile(r"^giveitem_msg\s+\w+,\s*(ITEM_\w+)")
SETVAR = re.compile(r"^setvar\s+VAR_\w+,\s*(ITEM_\w+)\s*$")
MART = re.compile(r"^pokemart\s+(\w+)")
JUMPS = re.compile(r"\b(goto|call|goto_if_\w+|call_if_\w+|goto_if|call_if)\b.*?(\w+)\s*$")


def follow(label, labels, order, seen, depth=0):
    """the items a script can give: [(item, how)]"""
    if label in seen or label not in labels or depth > 12:
        return []
    seen.add(label)
    f, lines = labels[label]
    found = []
    for line in lines:
        if line.startswith(".2byte ITEM_") or line.startswith(".string"):
            continue
        m = FINDS.match(line)
        if m:
            found.append((m.group(2), "found" if m.group(1) == "finditem" else "given"))
            continue
        m = GIVEMSG.match(line)
        if m:
            found.append((m.group(1), "given"))
            continue
        m = SETVAR.match(line)
        if m:
            found.append((m.group(1), "given"))
            continue
        m = MART.match(line)
        if m and m.group(1) in labels:
            for l in labels[m.group(1)][1]:
                it = re.match(r"\.2byte (ITEM_\w+)", l)
                if it and it.group(1) not in ("ITEM_NONE",):
                    found.append((it.group(1), "shop"))
            continue
        m = JUMPS.search(line)
        if m and m.group(1) and (m.group(1).startswith("goto") or m.group(1).startswith("call")):
            target = m.group(2)
            if not target.startswith(("Common_", "EventScript_Release", "EventScript_RestorePrev")):
                found += follow(target, labels, order, seen, depth + 1)
    # (falling through into the next label)
    if lines and not re.match(r"^(end|return|goto\s|releaseall|release)", lines[-1] if lines[-1] else "end"):
        names = order.get(f, [])
        if label in names and names.index(label) + 1 < len(names):
            found += follow(names[names.index(label) + 1], labels, order, seen, depth + 1)
    return found


PLACE_FIXES = [("SSAnne", "S.S. Anne"), ("SS Anne", "S.S. Anne"), ("Digletts", "Diglett's"), ("Players House", "your house"),
               ("Rivals House", "your rival's house"), ("Professor Oaks Lab", "Prof. Oak's lab"), ("Department Store", "Dept. Store"),
               ("Mr Psychics House", "Mr. Psychic's house"), ("Wardens House", "the Warden's house"), ("Captains Office", "the captain's office"),
               ("Pokémon Center 1F", "Pokémon Center"), ("Game Corner Prize Room", "Game Corner prize room"),
               ("Leader Academy Supply", "Leader Academy: the Supply shop"), ("Leader Academy Exchange", "Leader Academy: the Exchange"),
               ("Volunteer Pokémon House", "the Volunteer Pokémon House"), ("Medical Center", "Medical Center")]
TOWN = re.compile(r"^(\w+(City|Town|Island|Plateau)|Route\d+)$")


def place_name(folder, m):
    if folder.startswith("ExoticSafari"):
        r = D.safari_name(m.get("id", ""))
        if r:
            return f"{r[0]}: {r[1]}"
    name = re.sub(r"_(Frlg|Johto)$", "", folder)
    first, _, rest = name.partition("_")
    if TOWN.match(first) and rest and not re.fullmatch(r"B?\d+F|North|South|East|West", rest):
        out = D.prettify_map(first) + ": " + D.prettify_map(rest)
    else:
        out = D.prettify_map(name)
    for a, b in PLACE_FIXES:
        out = out.replace(a, b)
    if folder.endswith("_Johto"):
        out += " (Johto)"
    return out


def map_place(folder, mapsecs):
    m = json.loads((ROOT / "data/maps" / folder / "map.json").read_text())
    return place_name(folder, m), m


def sources():
    """ITEM_X -> [(place, how)]"""
    labels, order = all_labels()
    src = defaultdict(list)
    for folder in reachable_maps():
        path = ROOT / "data/maps" / folder / "map.json"
        if not path.exists():
            continue
        place, m = map_place(folder, None)
        scripts = []
        for o in m.get("object_events", []):
            if o.get("script"):
                scripts.append(o["script"])
        for b in m.get("bg_events", []):
            if b.get("type") == "hidden_item" and b.get("item"):
                src[b["item"]].append((place, "hidden"))
            elif b.get("script"):
                scripts.append(b["script"])
        for c in m.get("coord_events", []):
            if c.get("script"):
                scripts.append(c["script"])
        own = ROOT / "data/maps" / folder / "scripts.inc"
        if own.exists():
            scripts += re.findall(r"^(\w+)::?\s*$", own.read_text(), re.M)
        seen = set()
        for s in scripts:
            for item, how in follow(s, labels, order, seen):
                if how == "given" and "Game Corner" in place:
                    how = "shop"
                    item_place = "Celadon City: Game Corner prize room (coins)"
                elif how == "given" and folder == "CeruleanCity_House5_Frlg":
                    how, item_place = "shop", "Cerulean City: the Berry Powder exchange"
                else:
                    item_place = place
                src[item].append((item_place, how))
    return src


def tournament_prizes():
    """the tournament's prizes: ITEM_X -> rare?"""
    text = (ROOT / "src/tournament.c").read_text()
    table = text.split("sTourneyDrops[] =")[1].split("};")[0]
    lists = {"DROP_VITAMIN": "sDropVitamins", "DROP_MINT": "sDropMints"}
    out = {}
    for name, rare in re.findall(r"\{(\w+),\s*\d+(, TRUE)?\}", table):
        rare = bool(rare)
        if name in lists:
            for it in re.findall(r"ITEM_\w+", text.split(lists[name] + "[] =")[1].split(";")[0]):
                out[it] = rare
        elif name == "DROP_FUEL":
            out["ITEM_STRONG_ACID"] = out["ITEM_APPLE_JUICE"] = rare
        elif name.startswith("DROP_UPGRADE_"):
            n = int(name[-1])
            for it in (["ITEM_LUCKY_CLOVER", "ITEM_COPPER_COIN"], ["ITEM_FOUR_LEAF_CLOVER", "ITEM_SILVER_COIN"],
                       ["ITEM_GOLDEN_CLOVER", "ITEM_GOLD_COIN"])[n - 1]:
                out[it] = rare
        elif name == "DROP_TYPE_BERRY":
            for it in TERA_BERRIES:
                out[it] = rare
        elif name.startswith("ITEM_"):
            out[name] = rare
    return out


TERA_BERRIES = ["ITEM_CHILAN_BERRY", "ITEM_CHOPLE_BERRY", "ITEM_COBA_BERRY", "ITEM_KEBIA_BERRY", "ITEM_SHUCA_BERRY",
                "ITEM_CHARTI_BERRY", "ITEM_TANGA_BERRY", "ITEM_KASIB_BERRY", "ITEM_BABIRI_BERRY", "ITEM_OCCA_BERRY",
                "ITEM_PASSHO_BERRY", "ITEM_RINDO_BERRY", "ITEM_WACAN_BERRY", "ITEM_PAYAPA_BERRY", "ITEM_YACHE_BERRY",
                "ITEM_HABAN_BERRY", "ITEM_COLBUR_BERRY", "ITEM_ROSELI_BERRY"]
# (a source found by the scripts, said better)
OVERRIDE = {
    "ITEM_EVIOLATE": [("Pokémon Tower 2F-4F", "the ghost girl's gift: read the graves of the ten Pokémon that never "
                                                "evolved (one of a kind)")],
    "ITEM_SMEARGLITE": [("Pewter City: the painter's loft", "after the League: win the Mega's painting")],
    "ITEM_MEGA_RING": [("Leader Academy", "the headmaster, after your 8th badge")],
    "ITEM_DADS_NOTEBOOK": [("Viridian City Gym", "held by Dad's Chansey")],
    "ITEM_LUCKY_EGG": [("Pallet Town: the Chansey statue", "check the statue; upgrades with clovers left at its feet")],
    "ITEM_FRESH_WATER": [("Celadon City: Dept. Store roof", "the vending machines")],
    "ITEM_SODA_POP": [("Celadon City: Dept. Store roof", "the vending machines")],
    "ITEM_LEMONADE": [("Celadon City: Dept. Store roof", "the vending machines")],
    "ITEM_STRONG_ACID": [("Silph Co. 11F", "three by the door"), ("wild Poison Pokémon", "held, a quarter of the time")],
    "ITEM_APPLE_JUICE": [("wild Grass Pokémon", "held (half the time for the Applin line)")],
}
PALLET_MART = {"ITEM_BOTTLE_CAP": "after Erika's badge", "ITEM_ABILITY_CAPSULE": "after Sabrina's badge",
               "ITEM_ABILITY_PATCH": "after Sabrina's badge", "ITEM_CILANTRO": "after Blaine's badge",
               "ITEM_GOLD_BOTTLE_CAP": "after the Elite Four", "ITEM_MULTIVITAMIN": "after the Elite Four"}


def collect():
    global MOVES
    MOVES = D.load()["moves"]
    info = item_info()
    alias = tm_aliases()
    raw = sources()
    src = defaultdict(list)
    for k, v in raw.items():
        src[alias.get(k, k)] += v
    for items, where, how in MANUAL:
        for it in items:
            src[it].insert(0, (where, how))
    for it, when in PALLET_MART.items():
        src[it].insert(0, ("Pallet Town: the Mart", f"shop, {when}"))
    for k, i in info.items():
        if i["pocket"] == "POCKET_MEGA_STONES" and not src.get(k):
            src[k].append(("Leader Academy", "the Mega trial for a Pokémon that can use it (after the Mega Bracelet)"))
        if i["hold"] in ("HOLD_EFFECT_TYPE_GUARD", "HOLD_EFFECT_TYPE_EDGE") and not src.get(k):
            src[k].append(("your mentor", "defensive one at the Cerulean Gym, offensive one at the Cinnabar Gym, "
                                          "for your gym's type; the rest when you master other types"))
    for it, rare in tournament_prizes().items():
        src[it].append(("the tournament", "rare prize for a match won" if rare else "prize for a match won"))
    if "ITEM_EVIOLITE" in src:
        src["ITEM_EVIOLITE"] = [(w, h + " (money or 24 BP)" if "Exchange" in w else h) for w, h in src["ITEM_EVIOLITE"]]
    for k, v in OVERRIDE.items():
        src[k] = v + [x for x in src.get(k, []) if x[1] in ("shop",) or "prize" in x[1]]
    return info, src


HOW_ORDER = [("found", "Item ball"), ("hidden", "Hidden"), ("shop", "Shop"), ("given", "Gift")]


def where_html(sources, esc, most=8):
    """one item's sources, grouped by how: 'Item ball: Mt. Moon 1F, Route 4 · Shop: ...'"""
    groups = OrderedDict()
    special = []
    for place, how in sources:
        key = next((k for k, _ in HOW_ORDER if how == k), None)
        if key:
            groups.setdefault(key, OrderedDict())
            groups[key][place] = groups[key].get(place, 0) + 1
        elif (place, how) not in special:
            special.append((place, how))
    parts = [f"<b>{esc(p)}</b>: {esc(h)}" for p, h in special]
    for key, label in HOW_ORDER:
        if key not in groups:
            continue
        places = [p + (f" ×{n}" if n > 1 else "") for p, n in groups[key].items()]
        more = f", and {len(places) - most} more" if len(places) > most else ""
        parts.append(f"<i>{label}:</i> " + esc(", ".join(places[:most])) + more)
    return "<br>".join(parts)


def html(esc):
    """the guide's item list: per pocket, a table of icon, name, what it does and where it's found"""
    info, src = collect()
    out = []
    total = 0
    for p, label in zip(POCKET_IDS, POCKETS):
        rows = []
        for k, i in info.items():
            if i["pocket"] != p or not src.get(k):
                continue
            icon = icon_png(k, i)
            img = f'<img class="icon" src="{icon.as_uri()}">' if icon else ""
            desc = esc(i["desc"]) if p not in ("POCKET_TM_HM",) else ""
            rows.append(f"<tr><td class='ic'>{img}</td><td class='nm'><b>{esc(i['name'])}</b>"
                        f"<div class='ds'>{desc}</div></td><td>{where_html(src[k], esc)}</td></tr>")
        if rows:
            total += len(rows)
            out.append(f"<h3>{label}</h3><table class='grid items'><tr><th></th><th>Item</th><th>Where</th></tr>"
                       + "".join(rows) + "</table>")
    return out, total


if __name__ == "__main__":
    info, src = collect()
    n = 0
    for p, label in zip(POCKET_IDS, POCKETS):
        print(f"== {label}")
        for k, i in info.items():
            if i["pocket"] == p and src.get(k):
                n += 1
                print(f"  {i['name']:<20} " + "; ".join(f"{w} ({h})" for w, h in src[k][:6]))
    print(n, "items")
