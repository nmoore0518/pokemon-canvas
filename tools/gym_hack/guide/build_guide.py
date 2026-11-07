#!/usr/bin/env python3
"""Pokémon Canvas: the printable player's guide (a PDF): how the game works, a walkthrough from
Pallet Town to running your own gym, the Gym Leader Convention, and the Exotic Pokémon Safari
zone by zone (maps drawn from the game's own layouts, every legendary and puzzle solved).

    python3 tools/gym_hack/guide/build_guide.py [--out FILE]

The story chapters are written here by hand (from GYM_HACK.md); the tables and the safari's maps,
legendaries and puzzles come from the source, so rebuild after a zone changes.
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
sys.path.insert(0, str(ROOT / "tools/gym_hack/guide"))
OUT_DIR = ROOT / "build/guide"

import build_dex as D        # noqa: E402
import safari_maps as M      # noqa: E402
import trainer_teams as TT   # noqa: E402
import item_list as IL       # noqa: E402

TYPE_COLORS = D.TYPE_COLORS


def species_numbers():
    """SPECIES_* -> number, aliases (SPECIES_OGERPON = SPECIES_OGERPON_TEAL) resolved"""
    pairs = re.findall(r"\b(SPECIES_\w+)\s*=\s*(\w+)", (ROOT / "include/constants/species.h").read_text())
    nums = {n: int(v) for n, v in pairs if v.isdigit()}
    for _ in range(4):
        for n, v in pairs:
            if not v.isdigit() and v in nums:
                nums.setdefault(n, nums[v])
    return nums


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def badge(t):
    return f'<span class="type" style="background:{TYPE_COLORS[t.upper()]}">{t}</span>'


# ---------------------------------------------------------------------------------------------
# The 18 types (Oak's list order: easiest first)
# ---------------------------------------------------------------------------------------------
# type, starter, mentor, your badge, decorations unlocked by, trainer who joins, defensive item,
# offensive item
TYPES = [
    ("Psychic", "Ralts", "Sabrina", "ESPN", "Sabrina", "Nelson (Victory Road)", "Energy Shield", "Psionic Blade"),
    ("Fire", "Charmander", "Blaine", "Lava", "Blaine", "Thomas (S.S. Anne)", "Heat Shield", "Oxidizer"),
    ("Normal", "Porygon", "Brock", "Vanilla", "the rival in Cerulean", "Andrea (Route 8)", "Fluffy Sweater", "Third Eye"),
    ("Water", "Squirtle", "Misty", "Monsoon", "Misty", "Ned (Route 12)", "Cistern", "Nozzle"),
    ("Grass", "Bulbasaur", "Erika", "Weed", "Erika", "Alicia (Route 9)", "Chicken Wire", "Miracle Gro"),
    ("Electric", "Pikachu", "Lt. Surge", "Amp", "Lt. Surge", "Luca (Route 12)", "Capacitor", "Transformer"),
    ("Rock", "Larvitar", "Brock", "Obsidian", "Brock", "Marcos (Mt. Moon)", "Diamond Plating", "Precision Chisel"),
    ("Poison", "Ekans", "Koga", "Venom", "Koga", "Jared (Route 13)", "Titanium Coil", "Corrosive Venom"),
    ("Bug", "Caterpie", "Koga", "Infestation", "Bug Catcher Sammy (Viridian Forest)", "Rick (Viridian Forest)", "Cacoon", "Extra Stinger"),
    ("Steel", "Beldum", "Lt. Surge", "Wrench", "Giovanni (Rocket Hideout)", "Bernie (Route 11)", "Stainless Alloy", "Knife Sharpener"),
    ("Ice", "Spheal", "Lorelei", "Sub-Zero", "Giovanni (Viridian Gym)", "Nora (Route 20)", "Cooler", "Rock Salt"),
    ("Ground", "Sandshrew", "Bruno", "Shovel", "Super Nerd Miguel (Mt. Moon)", "Drew (Route 9)", "Granite Sarcophagus", "Diamond Drill Bit"),
    ("Flying", "Spearow", "Lance", "Rocket", "the rival in Silph Co.", "Perry (Route 13)", "Aviator Goggles", "Jet Engine"),
    ("Dragon", "Dratini", "Lance", "Egg", "Lance (Elite Four)", "Shirley (Route 20)", "Fire Shroud", "Bionic Jaw"),
    ("Fighting", "Tyrogue", "Bruno", "Knockout", "the Fighting Dojo master", "Hitoshi (Dojo)", "Mouth Guard", "Steroids"),
    ("Dark", "Poochyena", "Agatha", "Evil", "Picnicker Dana (Rock Tunnel)", "Lewis (Pokémon Mansion)", "Black Veil", "Manifesto"),
    ("Ghost", "Gastly", "Agatha", "Death", "the rival in Pokémon Tower", "Angelica (Pokémon Tower)", "Light Switch", "Ouija Board"),
    ("Fairy", "Clefairy", "Sabrina", "Elder", "the rival on the S.S. Anne", "Julia (Route 8)", "Twinkling Lights", "Magic Wand"),
]


def rivals_and_legends():
    rival = {}
    for sp, rt in re.findall(r"\{ SPECIES_(\w+),\s+TYPE_(\w+)\s*\}", (ROOT / "src/gym_hack.c").read_text()):
        rival[sp] = rt.title()
    legend = {t.title(): sp for t, sp in re.findall(r"\[TYPE_(\w+)\]\s*=\s*SPECIES_(\w+),", (ROOT / "src/gym_legend.c").read_text())}
    return rival, legend


def pretty_species(sp):
    return sp.replace("_", " ").title().replace("Ho Oh", "Ho-Oh")


def type_table():
    rival, legend = rivals_and_legends()
    rows = []
    for i, (t, starter, mentor, badge_name, unlock, joins, guard, edge) in enumerate(TYPES):
        rt = rival.get(starter.upper(), "")
        rows.append(f"<tr><td>{i + 1}</td><td>{badge(t)}</td><td>{starter}</td><td>{badge(rt) if rt else ''}</td>"
                    f"<td>{mentor}</td><td>{badge_name} Badge</td><td>{pretty_species(legend.get(t, ''))}</td></tr>")
    return ("<table class='grid'><tr><th>#</th><th>Type</th><th>Starter</th><th>Rival's type</th><th>Mentor</th>"
            "<th>Your badge</th><th>Gym legendary</th></tr>" + "".join(rows) + "</table>")


def type_extras_table():
    rows = "".join(f"<tr><td>{badge(t)}</td><td>{esc(unlock)}</td><td>{esc(joins)}</td><td>{guard}</td><td>{edge}</td></tr>"
                   for t, _, _, _, unlock, joins, guard, edge in TYPES)
    return ("<table class='grid'><tr><th>Type</th><th>Its decorations go on sale after beating</th><th>Trainer who joins your gym</th>"
            "<th>Defensive item</th><th>Offensive item</th></tr>" + rows + "</table>")


# ---------------------------------------------------------------------------------------------
# The Exotic Safari: zones, from the region modules and the built maps
# ---------------------------------------------------------------------------------------------
REGIONS = [
    ("Paldea", "paldea", "South Province, Paldea's open country, is the hub. The Treasures of Ruin wait behind stakes "
     "hidden all over the zone, and the paradox Pokémon live in Area Zero."),
    ("Galar", "galar", "Postwick's countryside is the hub. Glimwood Tangle glows at night, Circhester is snowed in, and "
     "Wyndon Stadium has a dojo master to beat."),
    ("Alola", "alola", "Hano Beach's resort is the hub; the islands are linked by sea (surf off an area's edge onto the "
     "next one's water)."),
    ("Kalos", "kalos", "Lumiose City, with the Prism Tower in its plaza, is the hub; its four gates lead out."),
    ("Unova", "unova", "Skyarrow Bridge is the hub: a long bridge between grasslands, with a cave in its cliff and an old "
     "shrine on its beach whose stairs go down to ruins under the sea."),
    ("Sinnoh", "sinnoh", "The Three Lakes are the hub: Lake Verity, Lake Valor and Lake Acuity in the pinewoods, an island "
     "in each. Floaroma Meadow's flower fields lie west, the Great Marsh south. Snowpoint City's cave climbs to the Hall of "
     "Origin, and one of its houses is Rotom's Room: five old appliances a Rotom can slip into (its form and move, as with "
     "the Rotom Catalog; again to come back out). Stark Mountain has a cave of its own."),
    ("Hoenn", "hoenn", "Fortree City, its houses up in the treetops, is the hub, with Route 120's grass and ponds to its "
     "east (surf to the mound in the pond for the Ancient Tomb). West is the sea: Pacifidlog Town's floating huts, and "
     "Sky Pillar's islet, whose cave leads into Sootopolis City's crater; a cave in Pacifidlog's cliff is the Island "
     "Cave. East is Mt. Chimney, with the Desert Ruins in its summit's rock. Each Regi chamber has two rooms: its "
     "Regi waits in the upper one, behind a Braille wall: see the legendaries below."),
    ("Johto", "johto_safari", "New Bark Town is the hub (in at Professor Elm's lab), with Route 29 west of it. Route 29's "
     "west end leads to the Ruins of Alph, whose north gate opens into Ilex Forest; Route 29's gatehouse leads to the "
     "Lake of Rage, where the Red Gyarados (shiny, level 55) swims in the middle of the lake. The lake's south-west path goes "
     "to the Ice Path (slide across its ice), whose ladder drops into Dragon's Den; the den's shrine opens on the "
     "Whirl Islands' deepest chamber. Raikou, Entei and Suicune roam once you've been in all six areas (the old man in "
     "New Bark Town keeps count). The Ruins of Alph have four stone panels at their chambers' doors, each spelling a "
     "word in Unown letters: ESCAPE (have an Escape Rope), WATER (a Water Stone), LIGHT (a lead that knows Flash) and "
     "HO-OH (Ho-Oh in your party). Open all four and every one of the 28 Unown forms turns up in the ruins' grass. "
     "The ruins' west half is reached through the little cave in its rocks."),
]


def safari_area_names():
    names = {}
    for m in M.map_groups():
        if m.startswith("ExoticSafari_") and m != "ExoticSafari_Hall":
            rest = m.split("_", 1)[1]
            names[M.map_const(m)] = D.SAFARI_NAMES.get(rest, rest)
    return names


def make_labeler():
    import alola
    import unova
    relics = [n for n, _, _ in alola.RELICS]
    keys = [n for n, _, _ in unova.KEYS]

    def label(name, script, obj):
        s = re.sub(r"\d+$", "", script)
        k = re.search(r"(\d+)$", script)
        k = int(k.group(1)) if k else 0
        if s.startswith("Item"):
            return M.words(s[4:])
        if s == "Relic":
            return relics[k].title()
        if s == "Key":
            return keys[k].title()
        if s == "Cell":
            return "Zygarde Cell"
        if s.startswith("Stake"):
            return f"{s[5:]} stake"
        if s == "Lamp":
            return "Dark mushroom (touch to light)"
        if s == "Stone":
            return f"Stepping stone {k + 1}"
        return {"DojoMaster": "Dojo master (Bea)", "Caretaker": "Caretaker (Type: Null)",
                "Groomer": "Furfrou groomer", "PrisonBottle": "Prison Bottle",
                "ChasmElder": "Old man (DNA Splicers)", "ShrineKeeper": "Shrine keeper (Reveal Glass)",
                "Kiosk": "Safari kiosk", "SnowpointElder": "Elder (Azure Flute)",
                "Appliance": "Appliance (Rotom forms)", "RotomRoomGuide": None}.get(s)
    return label


def sprite_lookup():
    data = D.load()
    nums = species_numbers()
    by_num = {}
    for k in data["species"]:
        if nums.get(k) is not None:
            by_num.setdefault(nums[k], k)

    def sprite_for(sp):
        key = by_num.get(nums.get("SPECIES_" + sp), "SPECIES_" + sp)
        body = data["species"].get(key)
        if not body:
            return None
        return D.sprite(D.parse_species(key, body, data), data)
    return sprite_for, data


def legend_blurbs(region_mod):
    """[(species, area, how)] for the region's statics and roamers"""
    areas_h = (ROOT / "src/data/exotic_safari_areas.h").read_text()
    names = safari_area_names()
    out = []
    for mp, sp in re.findall(r"MAP_NUM\((MAP_EXOTIC_SAFARI_\w+)\), SPECIES_(\w+), FLAG_EXOTIC_LEGEND", areas_h):
        if mp.startswith(f"MAP_EXOTIC_SAFARI_{region_mod.REGION}_"):
            out.append((sp, names.get(mp, mp), D.PUZZLES.get(sp, "Stands in this area.")))
    for r in getattr(region_mod, "ROAMERS", []):
        out.append((r, "roams", "Roams the zone: a 1 in 8 chance to be in any area you enter." + D.ROAM_NOTES.get(r, "")))
    if region_mod.REGION == "ALOLA":
        out.append(("TYPE_NULL", "Hano Beach", D.PUZZLES["TYPE_NULL"]))
    return out


def safari_section():
    import importlib
    names = safari_area_names()
    label = make_labeler()
    sprite_for, _ = sprite_lookup()
    html = []
    for region, modname, intro in REGIONS:
        mod = importlib.import_module(modname)
        html.append(f'<h2 class="newpage">The {region} zone</h2><p>{intro}</p>')
        html.append("<h3>Legendaries</h3><table class='grid legends'><tr><th></th><th>Pokémon</th><th>Where</th><th>How</th></tr>")
        for sp, where, how in legend_blurbs(mod):
            pic = sprite_for(sp)
            img = f'<img src="{pic.as_uri()}" class="mini">' if pic else ""
            html.append(f"<tr><td>{img}</td><td><b>{pretty_species(sp)}</b></td><td>{esc(where)}</td><td>{esc(how)}</td></tr>")
        html.append("</table>")
        for key, a in mod.AREAS.items():
            name = a["map"]
            if not (ROOT / f"data/maps/{name}/map.json").exists():
                continue
            png, legend, (w, h) = M.render(name, names, sprite_for, label)
            title = names.get(M.map_const(name), name)
            wild = ", ".join(dict.fromkeys(pretty_species(n) for s, *_ in a.get("land", []) for n in s.split("|")))
            water = ", ".join(dict.fromkeys(pretty_species(n) for s, *_ in a.get("water", []) + a.get("fishing", []) for n in s.split("|")))
            items = "".join(f'<li><span class="dot" style="background:rgb{color}">{n}</span> {esc(t)}</li>' for n, t, color in legend)
            if h > w * 0.8:                          # (tall maps: as tall as fits under the heading)
                size = f"height:{min(7.0, h * 0.12):.2f}in;width:auto"
            else:
                size = "width:100%" if w >= 40 else f"width:{max(45, w * 2.3):.0f}%"
            html.append(f'<div class="area"><h3>{esc(title)}</h3><img class="map" style="{size}" src="{png.as_uri()}">'
                        f'<ul class="legend">{items}</ul>'
                        f'<p class="small"><b>Wild:</b> {esc(wild)}' + (f'<br><b>Surf/fishing:</b> {esc(water)}' if water else "") + "</p></div>")
    return "".join(html)


# ---------------------------------------------------------------------------------------------
# The text
# ---------------------------------------------------------------------------------------------
CSS = """
@page { size: Letter; margin: 0.55in 0.6in; }
body { font-family: 'DejaVu Serif', Georgia, serif; font-size: 10pt; line-height: 1.42; color: #1d1d1f; }
h1 { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 22pt; margin: 0 0 6px; }
h2 { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 15pt; border-bottom: 2px solid #c33; padding-bottom: 3px; margin-top: 22px; }
h3 { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 11.5pt; margin: 14px 0 4px; color: #333; break-after: avoid; page-break-after: avoid; }
.newpage { page-break-before: always; }
.cover { height: 9.4in; display: flex; flex-direction: column; justify-content: center; align-items: center; text-align: center; page-break-after: always; }
.cover h1 { font-size: 40pt; }
.cover .sub { font-size: 14pt; color: #555; }
.type { color: white; font-family: 'DejaVu Sans', sans-serif; font-weight: 700; font-size: 7.5pt; padding: 1px 6px; border-radius: 8px;
        text-transform: uppercase; -webkit-print-color-adjust: exact; print-color-adjust: exact; white-space: nowrap; }
table.grid { border-collapse: collapse; width: 100%; font-size: 8.6pt; margin: 6px 0 10px; page-break-inside: auto; }
table.grid th, table.grid td { border: 1px solid #bbb; padding: 3px 5px; text-align: left; vertical-align: top; }
table.grid th { background: #eee; font-family: 'DejaVu Sans', sans-serif; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
table.grid tr { page-break-inside: avoid; }
.legends td { vertical-align: middle; }
img.mini { width: 40px; height: 40px; image-rendering: pixelated; }
.tip { background: #fff6d8; border-left: 4px solid #e0b020; padding: 6px 10px; margin: 8px 0; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
.area { page-break-inside: avoid; margin-bottom: 10px; }
img.map { image-rendering: pixelated; border: 1px solid #777; display: block; margin: 4px 0; }
ul.legend { columns: 2; font-size: 8.6pt; margin: 2px 0; padding-left: 0; list-style: none; }
ul.legend li { break-inside: avoid; margin-bottom: 2px; }
.dot { display: inline-block; width: 15px; height: 15px; border-radius: 8px; border: 1.5px solid #222; text-align: center; font: bold 8pt 'DejaVu Sans', sans-serif;
       line-height: 15px; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
.small { font-size: 8.6pt; }
.toc { columns: 2; }
.toc a { color: #1d1d1f; text-decoration: none; }
ol.steps li { margin-bottom: 4px; }
table.items td { vertical-align: middle; }
table.items td.ic { width: 30px; text-align: center; }
table.items td.nm { width: 30%; }
table.items .ds { font-size: 7.6pt; color: #555; }
img.icon { width: 24px; height: 24px; image-rendering: pixelated; }
"""


def chapter(title, body, newpage=True, anchor=None):
    a = f' id="{anchor}"' if anchor else ""
    return f'<h2{a} class="{"newpage" if newpage else ""}">{title}</h2>{body}'


INTRO = """
<p><b>Pokémon Canvas</b> is FireRed retold: you aren't out to beat the Gym Leaders, you're out to become one. Professor
Oak tells you on day one that the Pokémon League is opening a ninth gym, in Pallet Town, and that he has picked two
candidates: you and your rival. League rules say a Gym Leader uses one type, so you choose your type in his lab and run
the whole League challenge with it. Earn the approval of all eight Gym Leaders and the Elite Four, become Champion, and
then win the gym itself. After that the game opens up: challengers line up at your door, you decorate and train in your
own gym, you can master other types, the Gym Leader Convention waits in Johto, the Exotic Pokémon Safari in Fuchsia
City holds Pokémon from every region (until you have your gym, nothing past the first 386 can be caught or won;
your own Pokémon can still evolve or breed into them), and at the very end an underground tournament of the world's strongest trainers hides
one last secret. Along the way, the Leader Academy on Route 11 trains Gym Leader candidates (and hands out Mega Stones).</p>
<p>There's a second story running under the first: your father's. Your mom says he wanted to be a Gym Leader too,
"among other things". Team Rocket's grunts keep saying you look just like him, Giovanni says he knew him, and his old lab
is hidden under your house. What he built down there (and what he did with it) comes out at the Viridian Gym.</p>
<p>This guide has spoilers throughout: the story, every legendary's hiding place, and every puzzle's solution.</p>
"""

RULES = """
<h3>Your team: the League's rules</h3>
<ul>
<li><b>One type.</b> Every Pokémon in your party must have your type as its <i>first</i> (primary) type. A Grass/Poison
Bulbasaur counts as Grass; a Poison/Grass Pokémon wouldn't.</li>
<li><b>No duplicates.</b> No two party Pokémon may be the same species (different stages of one family are fine: Ivysaur and
Venusaur together are allowed).</li>
<li><b>No two holding the same item.</b> Like VGC's item clause: two Pokémon can't both hold, say, Leftovers (an item and
no item are always fine).</li>
<li>The rules are checked before Gym Leader and rival battles, once at the League's door for the Elite Four and the
Champion, before your gym's challengers, the Academy's candidates and Oak's finale, and before every Gym Leader Convention
battle. Break them and the game explains why and lets you fix your team. Ordinary trainers and wild battles have no rules,
so you can catch and train anything; only your gym battles care.</li>
<li><b>The tournament</b> (and Bill's simulator) has its own rules: any type goes, but no two of the same species and no two
holding the same item, checked when you sign up and before every match.</li>
<li><b>Birds are Flying first</b> in this game (Pidgey, Spearow, Doduo, Natu, Taillow, Hoothoot, Wingull lines, Farfetch'd,
Aerodactyl), so Flying is a real team type.</li>
</ul>
"""

DIFFERENCES = """
<h3>What's different from FireRed</h3>
<ul>
<li><b>HM key items.</b> The people who give HMs also give a key item that does the job without a Pokémon knowing the move:
Hedge Clippers (Cut, from the S.S. Anne's captain), Flashlight (Flash, Route 2's aide), Boogie Board (Surf, the Safari
Zone's secret house), Can of Spinach (Strength, Fuchsia's warden) and the Hot Air Balloon (Fly, from Oak in his gym scene:
it needs no badge). Walk up to a tree, boulder or water and the game offers the item; you can also use them from the bag or
register one to SELECT.</li>
<li><b>No trading needed.</b> Kadabra, Machoke, Graveler and Haunter evolve at level 36 (or with a Linking Cord). Trade items
(King's Rock, Metal Coat, Dragon Scale, Up-Grade, Deep Sea Tooth and Scale, Prism Scale) are used from the bag like stones,
and every evolution stone is sold at the Celadon Dept. Store 4F. Eevee evolves into Espeon with a Sun Shard and into Umbreon
with a Moon Shard.</li>
<li><b>EVOLVE: ON/OFF.</b> In the party menu, each Pokémon can have evolution switched off (like an Everstone). Stones still
work.</li>
<li><b>Quick heal pads.</b> A glowing pad in a corner of every Pokémon Center heals your party when you step on it.</li>
<li><b>The Palm Pilot.</b> Bill gives you a pocket PC with the S.S. Ticket. Use it anywhere outside the League to reach your
boxes and everything else on the PC (see <a href="#pc">Your PC</a>).</li>
<li><b>16 PC boxes</b> instead of 14.</li>
<li><b>Renaming.</b> After the Name Rater in Lavender Town tells you the secret, the first page of a Pokémon's summary
offers RENAME (press A) for your own Pokémon in your party.</li>
<li><b>Outfits.</b> The wardrobe in your room changes your clothes (see <a href="#lab">Dad's lab</a>).</li>
<li><b>Reusable TMs.</b> Like HMs, a TM is never used up: teach it to as many Pokémon as you like.</li>
<li><b>A bag with ten pockets</b>, with room for one of every item in the game (and 46 kinds of Poké Ball): Items (evolution items, training items,
valuables), Medicine, Poké Balls, Battle Items, Hold Items, Mega Stones, Berries, TMs &amp; HMs, Tools (the key items you use, and
can register to SELECT) and Key Items.</li>
<li><b>Up to five items on SELECT.</b> REGISTER up to five tools from the bag (DESELECT takes one off). With one registered,
SELECT uses it straight away; with more, SELECT shows a short list of them to pick from (B to cancel).</li>
<li><b>Terastallization.</b> Archer gives you the Tera Orb after Viridian. It never needs charging; one Pokémon a battle.
See <a href="#tera">Terastallization</a>.</li>
<li><b>The Spirit Ward.</b> A Max Repel that never runs out: hold it up or put it away from the Tools pocket (or register
it to SELECT). Mr. Fuji gives it to you after a battle, once you have the Poké Flute.</li>
<li><b>Double battles.</b> Before your 4th badge battles are mostly singles (and FireRed's few twins and couples). From the
4th badge on, about two in three are doubles: ordinary trainers, Koga, Sabrina, Giovanni at his gym and at Silph, Lorelei,
Agatha, Lance and your rival's later battles (Blaine, Bruno and Giovanni in the Hideout and at the end stay singles). Your
gym's challengers, the Convention, the Academy's candidates and Battle Hall are a mix; the tournament is doubles the first
time, and Bill's simulator lets you choose. With only one Pokémon able to battle, you face both of theirs alone.</li>
<li><b>A berry field.</b> 24 plots in Pallet Town; berries grow as you walk. See <a href="#berries">Tera berries</a>.</li>
<li><b>Mega Evolution.</b> The Leader Academy's headmaster gives you the Mega Bracelet after your 8th badge, and each Mega
Stone is earned in a trial (see <a href="#academy">the Leader Academy</a>).</li>
<li><b>Summary screen.</b> On the skills page, A switches between stats, IVs (exact values) and EVs; the TERA box (where
FireRed had RIBBON) shows the Pokémon's Tera type once you have the Tera Orb. A new LEVEL-UP MOVES page lists every move the
species learns and when.</li>
<li><b>Gym type finder.</b> On the Town Map, SELECT marks every area where a Pokémon of your type can be found.</li>
<li><b>A new opening.</b> After the Game Freak screen: a night battle in a gym, the eight badges, your walk up to your own gym
in Pallet Town, and a painter's brush.</li>
<li><b>The Sevii Islands are gone.</b> Johto replaces them as the post-game area, and Moltres now waits on Victory Road 2F
(level 50), as in Red and Blue.</li>
</ul>
"""

QUESTS = """
<p>Each Kanto town has something new to do (Viridian's Penny and her Eevee are in <a href="#items">Items</a>; Pallet
Town's is your gym). In story order:</p>
<h3>Pewter City: the painter and her Smeargle</h3>
<p>A painter has moved into a loft in Pewter City (its door is in the northwest corner of town). Her easel is a way into her paintings: each is a real place
somewhere in the world, in scrambled colours, with nobody there but a figure made of paint who challenges you to a double
battle. Nothing gets out of a painting: walk off the edge, use a door, Fly, Dig or an Escape Rope and you come back out of
the easel.</p>
<ul>
<li><b>After your 1st badge</b> she lends you her Smeargle, unpainted and typeless, to watch a real battle. Win a painting's
battle with it and it finds its colour: a <b>Canvas Smeargle</b> of your gym's type, yours to keep.</li>
<li><b>After each badge after that</b>, one <b>Sketch quest</b>: pick a type and one of Canvas Smeargle's move slots, and she
paints Sketch into it. The figure's Pokémon use only moves of that type (more of them with each badge). Win, and the move
Smeargle sketched is kept; lose (you can retry) and the slot gets its old move back.</li>
<li><b>After the League</b>: the Mega's painting, a figure with a Mega Evolution. Win it for the <b>Smearglite</b> (a key
held item: a Canvas Smeargle holding it Mega Evolves, like any Mega Stone). After that, Sketch quests whenever you like.</li>
</ul>
<h3>Viridian City and Cinnabar Island: MissingNo.</h3>
<p>Once you've watched the old man in Viridian City show you how to catch a Pokémon (he'll show you again any time after
the first), go straight to Cinnabar Island, by Fly or the Hot Air Balloon, without walking through any grass. Surf up and
down the column of water along Cinnabar's east coast: one wild Pokémon in five there is <b>MissingNo.</b>, a real Pokémon
you can catch. Its level comes from your name, as it did in Red and Blue (so usually level 100). Step off the coast, or
through grass anywhere, and it's gone until you watch the demo again. No items are duplicated.</p>
<p>MissingNo. is Bird/Normal (its "Bird" type is typeless), with Red and Blue's stats (HP 33, Attack 136, Defense 0, Special
6, Speed 29), Water Gun, Water Gun and Sky Attack, and its old TMs. It isn't in the Pokédex (No. 000), and its Bird type
means no gym will take it.</p>
<h3>Vermilion City: the Universal Pokémon Medical Center</h3>
<p>The foreman on the building site on the east side of town needs help: bring him a <b>Machop</b>, then a <b>Machoke</b>, then a
<b>Machamp</b>, then a <b>shiny Machamp</b> (any: one from Bill's Palm Pilot counts). Each one stays to work for him, and
each turns something up in the dig: the Mt. Moon fossil you didn't take, then a Root or Claw Fossil (your pick), then a
Skull or Armor Fossil. Cinnabar's lab revives them all. With the fourth, the building goes up: the Medical Center, with a
puzzle on each floor.</p>
<ul>
<li><b>1F, triage:</b> six patients, each needing the right medicine. Psyduck: Burn Heal. Rattata: Antidote. Pidgey:
Paralyze Heal. Jigglypuff: Awakening. Charmander: Ice Heal. Spinda: Persim Berry. Treat all six and the triage nurse gives
you 5 Lum Berries and 5 Full Heals. The pharmacy sells medicine.</li>
<li><b>2F, the wards:</b> runaway gurneys block the way. Push them like boulders (no Strength needed); they reset when you
leave the floor. The nurse past them gives you the <b>Heal Charm</b> (a key item: your healing items heal 10% more HP).</li>
<li><b>3F, the research lab:</b> the head doctor describes Pokérus. Check the five microscopes: the middle one (third from the left) has it. Then pass
the head nurse's physical (a battle), and she'll give one of your Pokémon Pokérus once a "day" (a day here is 1,000 steps;
there's no clock, and Pokérus wears off a day at a time).</li>
</ul>
<h3>Lavender Town: the graves in the Pokémon Tower</h3>
<p>Thirty of the gravestones on floors 2F-4F can be read: each shows its Pokémon, its name and its trainer. Ten of them were
Pokémon that could have evolved and never did: Charmander, Pidgey, Caterpie and Rattata on 2F; Growlithe, Eevee and
Magikarp on 3F; Vulpix, Abra and Oddish on 4F. Read all ten, and a ghost girl and her Scyther appear beside you. She gives
you the <b>Eviolate</b>: Eviolite for Attack and Sp. Atk (1.5x for a Pokémon that can still evolve). There's only one. One
grave on 3F, near the bottom, has no trainer's name.</p>
<h3>Celadon City: the Drink Machine</h3>
<p>The girl on the Dept. Store roof is thirsty, as in FireRed (a Fresh Water, a Soda Pop and a Lemonade, for three TMs). Once
she's had all three, her own drink is still stuck in the vending machine; she kicks it, out comes a <b>Limited Edition
Soda</b> (yours: she has plenty), and since the store's throwing the machine out, she gives you that too: the <b>Drink
Machine</b>, a gym decoration. Talk to it in your gym to mix a drink with up to two berries; only
a recipe uses anything up.</p>
<table class="grid"><tr><th>Drink + berries</th><th>Makes</th></tr>
<tr><td>Fresh Water + a healing berry</td><td>Super Potion</td></tr>
<tr><td>Soda Pop + a healing berry</td><td>Hyper Potion</td></tr>
<tr><td>Lemonade + Oran / + Sitrus</td><td>Hyper Potion / Max Potion</td></tr>
<tr><td>a drink + a status berry</td><td>Full Heal</td></tr>
<tr><td>Lemonade + Sitrus + Lum</td><td>Full Restore</td></tr>
<tr><td>a drink + two healing berries</td><td>Revive (Lemonade + two Sitrus: Max Revive)</td></tr>
<tr><td>a drink + Leppa</td><td>Ether, Elixir or Max Elixir (by the drink)</td></tr>
<tr><td>Limited Edition Soda + a Tera berry</td><td>a Pokémon in your party gets the berry's type as its second type, for good
(through evolution and Mega Evolution); not its first type</td></tr>
</table>
<p>She gives you another Limited Edition Soda for each type you master, and they turn up, rarely, as tournament prizes.</p>
<h3>Celadon City: the pit</h3>
<p>Under Celadon there's a fighting pit where the crowd bets on other trainers' battles. It's underground because the League
would never sanction it: the way these fighters treat their Pokémon breaks every rule it has. Listen in on a pair of gamblers in the Game Corner
to learn where it is and the password. The doorman in the hall below asks for it (type it in; capitals or not).</p>
<ul>
<li><b>The card</b>: the sign on the rim shows tonight's bout: each corner's fighter and Pokémon, its level, ability, held
item and moves, and the bookie's odds. The two Pokémon stand in the pit.</li>
<li><b>Betting</b>: talk to the bookie: pick a corner and bet ¥5,000 to ¥100,000 (or just watch). The fight is a real battle,
the two trainers' AI against each other. A winning bet pays your stake times the odds; a loss costs the stake, nothing else.
A new bout goes up after every fight, and every time you come down.</li>
<li><b>The odds</b> come from the card (who knocks out whom first, and who's faster), minus the house's cut, and they're a
little off. The bouts are mostly even by type, so items, moves and base stats decide them; where one side has the type
advantage, the other makes up for it or pays more. The underdog always has a chance.</li>
<li><b>Fixed fights</b>: about one in four is fixed: the favorite takes a dive (its trainer calls bad moves), or it isn't
holding the item on the card, or it's weaker than the card says. The tipster on the rim tells you which; his price is
ten percent of whatever you bet on that bout, added on when you bet (or taken right away if your bet's already down).</li>
<li>The bookie stands behind a counter along the back wall, like a shop clerk; talk to him across it.</li>
<li><b>The game saves itself</b> before and after any bout you've bet on. There's no switching off to take a bet back.</li>
</ul>
<p><b>The back room</b>, down the ladder in the far corner, doesn't deal in money. Its boss bets Pokémon: you put one up and he
puts up his, one on one. Win, and his Pokémon is yours to keep. Lose, and he keeps yours, for good.</p>
<ul>
<li><b>His Pokémon</b> changes every "day" of steps. It's something hard to come by: a Pokémon that's rare in the wild, or the
final form of one whose earlier forms are the only ones found wild, and (once you have your gym) once in a while a
legendary (never a gym's). Before your gym, it's never a legendary or anything past the first 386. It has
perfect IVs, full EVs and a good item. It's always the same level as the Pokémon you put up.</li>
<li><b>A fair fight only</b>: he compares the two Pokémon's stats at that level, and he turns yours down if it's more than
10% stronger than his, or more than 20% weaker. Between 10% and 20% weaker, he'll warn you you're outclassed and
take the bet anyway if you insist: a long shot, but with the right type matchup, not a hopeless one.</li>
<li>You need at least one other Pokémon (not an Egg) to bet one. Once you've won his Pokémon, he has nothing to put up
until the next day. If both Pokémon go down together, the deal's off.</li>
<li><b>The game saves</b> when you shake on it and again after the battle. Switch off mid-battle and he counts it as a
loss.</li>
</ul>
<h3>Saffron City: Grandmaster Rod and the Trading Card Game</h3>
<p>The doorless house left of the Copycat's has a door now. Inside is <b>Rod</b>, Grandmaster of the Pokémon Trading Card
Game, and a real duel: the Game Boy Trading Card Game's own rules and AI, with its music. Pick one of seven classic decks
(Flower Power, Fire Charge, Go Go Rain Dance, Zapping Selfdestruct, Strange Psyshock, First-Strike, Colorless) against his
Legendary Dragonite deck. Win, and he gives you that deck's <b>energy card</b>; rematches are unlimited.</p>
<ul>
<li><b>Grass, Fire, Water, Lightning, Psychic and Fighting Energy</b> (Medicine pocket): +5 base power on one damaging move
of their types, for good. Each covers three types (Grass: Grass, Bug, Poison; Fire: Fire, Dragon, Flying; and so on: the
item's description says which). Once per move slot; the boost follows the move and goes if it's forgotten. The summary
shows it.</li>
<li><b>Double Colorless Energy</b> (from the Colorless deck): every move's PP raised as far as PP Ups go.</li>
</ul>
<h3>Route 21: Surfing Pikachu and Pikachu's Beach</h3>
<p>East of the line of rocks near Cinnabar, one rock is missing: surf through to a tiny islet with a cave. Inside, by a
solid-gold Diglett statue, is <b>COOL♂</b>, the "Cool" trainer from Pokémon Stadium's Master Ball Cup, with his Stadium team
at level 100 (Mew, Tauros, Starmie, Rhydon, Exeggutor, Jolteon). Battle him as often as you like. Beat him with a Pikachu in
your party and he gives you a level 50 <b>Surfing Pikachu</b> (Surf, Thunderbolt, Quick Attack, Agility); beat him with a team
of nothing but Pikachu for a shiny one. One of each.</p>
<p>With a Surfing Pikachu in your party, the Surfin' Dude in the beach house on Route 19 runs <b>Pikachu's Beach</b>,
Pokémon Yellow's surfing minigame: ride the waves, jump off the crests and spin for points (land at the angle of the water
or wipe out), and reach the goal before HP runs out. He keeps your best score. SELECT quits.</p>
"""
ITEMS = """
<h3>Training items</h3>
<ul>
<li><b>Silver Bottle Cap</b>: one IV becomes 31. <b>Gold Bottle Cap</b>: all six do.</li>
<li><b>Ability Capsule</b>: switch between the species' two regular abilities. <b>Ability Patch</b>: the hidden one.</li>
<li><b>Cilantro</b>: pick any nature (like a mint).</li>
<li><b>Multivitamin</b>: set all six EVs on one screen (252 per stat, 510 in all). Cancelling doesn't use it up.</li>
</ul>
<p>The Pallet Town Mart sells them as you progress: the Silver Bottle Cap after Erika's badge, the Ability Capsule and Ability Patch after
Sabrina's, Cilantro after Blaine's, and the Gold Bottle Cap and Multivitamin after the Elite Four. Every Gym Leader also gives
a Silver Bottle Cap with their TM; Erika adds Cilantro and Koga an Ability Capsule; your rival gives a Gold Bottle Cap at the
Pokémon Tower, and Giovanni a Multivitamin at Silph Co. (along with the Master Ball).</p>
<h3>Eevee and Penny</h3>
<p>Eevee (rare on Route 1) can evolve into a Pokémon of every type. Penny, in the house in Viridian City's flower garden,
tells you how: show her an Eevee and she gives you the item for your gym's type. Once the gym is yours, bring her another
Eevee for any other type. The new eeveelutions evolve by levelling up while holding the type's power-up item: Silk Scarf
(Odeon, Normal), Black Belt (Assasseon, Fighting), Sharp Beak (Hypereon, Flying), Poison Barb (Poiseon, Poison), Soft Sand
(Subterraneon, Ground), Hard Stone (Obsideon, Rock), Silver Powder (Lenneon, Bug), Spell Tag (Appariteon, Ghost), Metal Coat
(Automateon, Steel) and Dragon Fang (Valyreon, Dragon). A few trainers have one: a Beauty on Route 13 (Odeon) and a Bird
Keeper there (Hypereon), a Lass at the far end of Route 4 (Lenneon), a Biker on Route 16 (Poiseon), the Fighting Dojo
(Assasseon), Sabrina's gym (Appariteon), the Pokémon Mansion (Automateon), Giovanni's gym (Subterraneon) and Victory Road
(Obsideon, Valyreon); gym challengers can bring them too.</p>
<h3>EXP and money</h3>
<ul>
<li><b>Lucky Egg</b> (held: 1.5x EXP): at the base of the Chansey statue in Pallet Town, west of the Pokémon Center.
Check the statue.</li>
<li><b>Amulet Coin</b> (held: double prize money): an item ball in the Pokémon Mansion, 3F, in the room with the
machines. Oak's aide in the Route 16 gate gives another once you've caught 40 kinds of Pokémon.</li>
</ul>
<p>Both level up, to Lv. 10 (6x EXP, 6.5x prize money). Leave clovers at the Chansey statue's feet to make the Lucky Egg
luckier; Daisy, in your rival's house, sets Grandpa Oak's old coins into the Amulet Coin. Levels 2-4 take 1, 2 and 3 of the
common one (Lucky Clover, Copper Coin), 5-7 1, 2 and 3 of the rare one (Four-Leaf Clover, Silver Coin), and 8-10 1, 1 and 2
of the very rare one (Golden Clover, Gold Coin). They're hidden items, enough of each to reach Lv. 10, so the Itemfinder helps: clovers in the grass of the
routes (the rarer ones further along: Routes 10-15 and 21 and the Safari Zone, then Route 23), coins in caves and ruins (Mt.
Moon, Rock Tunnel and the like, then Seafoam, the Mansion and Victory Road, then Cerulean Cave). Wild Pokémon hold them too:
Grass Pokémon like Oddish and Bellsprout a Lucky Clover, Clefairy and Chansey a Four-Leaf (Chansey, rarely, a Golden),
Meowth and Diglett a Copper Coin, Persian and Dugtrio a Silver (rarely, a Gold). Thief or Covet takes them.</p>
<h3>From Dad's lab</h3>
<ul>
<li><b>Type Balls</b> (18, one per type): made by the Ball Maker in the basement, never sold. 3.5x catch rate on a Pokémon of
that type, and a Pokémon of that type kept in one has its lowest base stat raised by half. Your mom gives you one, your
gym type's, after Nugget Bridge: your father made it. Two-toned so you can tell them apart.</li>
<li><b>Strong Acid</b> and <b>Apple Juice</b>: fuel for the Ability Synthesizer (3 a run). Wild Poison Pokémon hold Strong
Acid a quarter of the time, wild Grass Pokémon Apple Juice (half the time for the Applin line). Silph Co. 11F has 3 Strong
Acid by the door.</li>
<li><b>Dad's Notebook</b>: held by Dad's Chansey. Take it and read it from the bag. It can't be lost.</li>
</ul>
<h3>Your mentor's type items</h3>
<p>Each type has two one-of-a-kind held items, brought by your mentor (the Gym Leader or Elite Four member of your type, or
the closest fit). Neither is ever used up, and they can't be stolen or knocked off.</p>
<ul>
<li><b>Defensive</b> (after Mt. Moon, at the Cerulean Gym): no move is super effective on the holder; those hits do normal
damage.</li>
<li><b>Offensive</b> (at the Cinnabar Gym, after its badge): the holder's resisted or immune-type hits do normal damage
instead. (Abilities like Levitate still block.)</li>
</ul>
<h3>The Move Extractor</h3>
<p>The Team Rocket grunt on Nugget Bridge gives you three. After that they're ultra rare prizes: about 1 gym challenge win in
150, and 1 tournament match won in 100 (Bill's simulator included). Used from the bag in battle after a foe has used a move,
it lets one of your Pokémon learn that move, and the move takes your Pokémon's primary type (a new same-type move). The foe
can't use it again that battle.</p>
<ol>
<li>The grunt on Nugget Bridge won't take no for an answer: you join Team Rocket, and he hands them over.</li>
<li>After you beat Giovanni at Silph Co., he finds you in Saffron City.</li>
<li>After Giovanni is gone (the Viridian Gym), he finds you in Viridian City.</li>
</ol>
"""

WALKTHROUGH = """
<p>The map, the towns and the order of the badges are FireRed's; this walkthrough covers what's new on the way. Where it says
"as in FireRed", the original game's route applies.</p>

<h3>1. Pallet Town and Oak's lab</h3>
<p>Pallet Town is rebuilt and much bigger: your house on a peninsula below the cliffs, the rival's and a neighbor's yards
joined by a bridge, a Pokémon Center and Mart in the east, Oak's lab on a rise, and in the middle, on a rocky plateau, the
new gym. In the lab, the Poké Ball shelves open the type menu: Oak lists the 18 types from the strongest possible team to
the weakest (the last four come with a warning). You get your type's starter, and your rival picks a type that's super
effective against yours. Your first rival battle follows.</p>
<p>Oak then takes you both to the empty gym: you'll decorate it whenever you're in town, as part of your "audition", and a
new leader needs the approval of all eight Gym Leaders and the Elite Four. The Hot Air Balloon (Fly) is his gift here.
He also mentions your father: he always dreamed of a gym of his own.</p>

<h3>2. Viridian City, the parcel, and your mentor</h3>
<p>Oak's Parcel carries a letter: Oak wrote to your <b>mentor</b> (the leader or Elite Four member of your type), and the
reply says they're coming. After the Pokédex scene they walk into the lab, introduce themselves, give advice about running a
gym, and leave a housewarming gift in your gym's PC (a decoration of your type). They'll turn up again through the story,
with your type items. When you leave the lab with the Pokédex, your mom is waiting outside to see you off.</p>
<div class="tip">The Pokédex is a scouting tool here: "a Gym Leader who doesn't know their opponents won't last a week".
The gym comes first.</div>

<h3>3. Viridian Forest and Pewter City</h3>
<p>Beat Brock as in FireRed. From now on, every Gym Leader works the same way after their badge:</p>
<ul>
<li><b>Guest Leader battles</b>: the leader asks you to hold their gym. A challenger walks in and you battle in the leader's
place (one challenger at Brock, Misty, Surge and Erika; two at Koga, Sabrina, Blaine and Giovanni). Win to earn the leader's
<b>approval</b>. Each challenger pays a Gym Leader's prize, doubled: this is your decoration money. Losing costs nothing.</li>
<li><b>Stand-in shifts</b> (optional, repeatable): once a leader approves, talk to them again and they'll hand you their gym
for a shift (Brock's stew, Misty's swim, Surge's R&amp;R...). One challenger, matched to your level, for prize money and EXP.</li>
<li><b>Oak's aides</b>: a scientist by every gym's entrance has tips for designing your own gym around that gym's mechanic, and
after the badge says what it unlocked.</li>
</ul>
<p>Your gym's aide in Pallet Town keeps count of your approvals.</p>

<h3>4. Mt. Moon, Cerulean City, Nugget Bridge</h3>
<p>Mt. Moon as in FireRed. After Misty's badge your mentor drops by the Cerulean Gym with your <b>defensive type item</b>.
On Nugget Bridge the Rocket grunt won't let you say no: you join Team Rocket and get your first <b>Move Extractor</b>.
Your rival catches up with you there: your mom has something for you. You're sent home, where she gives you
the <b>Type Ball</b> your father made (your gym's type) and worries about that Move Extractor. Bill gives you the S.S.
Ticket and the <b>Palm Pilot</b>. Your rival's lines from Route 22 on are about the race for approvals and the Pallet gym.</p>

<h3>5. Vermilion City to Celadon City</h3>
<p>The S.S. Anne (the captain gives HM01 and the Hedge Clippers), Lt. Surge, Rock Tunnel, Lavender Town and Celadon City as in
FireRed. With your third badge, step onto Route 11: your mentor (or Oak) calls you over to a gap in the trees on the north
side, talks the guards round, and you're admitted to the <b>Leader Academy</b> (see <a href="#academy">its chapter</a>). It's
optional, but its candidates and Battle Hall are the best training in the game. Celadon's Game Corner prizes are Abra, Clefairy, Dratini, Scyther and Porygon; the Dept. Store 4F sells every
evolution stone and trade item. Erika gives Cilantro with her TM. In the Game Corner and the Rocket Hideout, the grunts seem to recognize you;
at the bottom of the hideout, Giovanni says you remind him of your father, and wonders why your mom never told you he
knew him.</p>

<h3>6. Pokémon Tower, Saffron City, Silph Co.</h3>
<p>Your rival gives you a Gold Bottle Cap at the Pokémon Tower. Back in Lavender, after the Poké Flute, Mr. Fuji
offers you his <b>Spirit Ward</b> if you can beat him: a double battle against Haunter, Slowbro, Hypno (levels 30-31) and a
level 33 Marowak with a Thick Club. Losing costs nothing; he heals you and waits. Silph Co. as in FireRed up to 11F (Lapras is the gift on
7F), but the boardroom is different: no President, 3 Strong Acid just outside the boardroom door (grab it before you go in; you go straight home afterwards), and after his defeat Giovanni gives you the
<b>Master Ball</b> and a Multivitamin ("consider it an investment"). Then your mom runs in, and takes you home.</p>
<p>At home she moves the bookcase: behind it are stairs down to <b>your father's lab</b> (see <a href="#lab">Dad's
lab</a>). His machines work from then on; read the four pages on the floor. Back in Saffron, the Rocket grunt finds you with
the second Move Extractor. The Fighting Dojo gives Hitmonlee or Hitmonchan.</p>

<h3>7. Fuchsia City, the Safari Zone, Cinnabar Island</h3>
<p>Koga gives an Ability Capsule. The Safari Zone (Surf: HM03 and the Boogie Board in its secret house; Strength from the
warden) as in FireRed. Next to the Safari Zone office, a second door says "NEW! EXOTIC POKéMON SAFARI": it's for Gym Leaders
only, so it waits for the post-game. On Cinnabar, Blaine's badge brings your mentor with your <b>offensive type item</b>, and
the lab revives Mt. Moon's fossil and the Old Amber. Bill doesn't take you to the Sevii Islands any more.</p>

<h3>8. Viridian Gym, Victory Road, the League</h3>
<p>Beat Giovanni and he hands over the Earthbadge, TM26 and a Silver Bottle Cap at once. Then he tells you who your
father was: his top deputy and best scientist, whose inventions made Team Rocket strong, until he turned on Giovanni. And
he battles you again, with the power your father gave him: Persian, Crobat, Weezing, Nidoqueen, Rhyperior and Nidoking
(levels 50-54), every one with its synthetic ability and in a Type Ball. Losing doesn't white out: talk to him to try
again. Win, and your mom arrives with her Chansey, and Giovanni is gone. She gives you <b>Dad's Chansey</b> (level 50, a
Chansey with 100 Attack and Special Attack, Triage, perfect IVs, in a Normal Ball; it can't evolve, breed or be traded),
holding <b>Dad's Notebook</b>, and tells you what she knew. Then she takes you home: she and Oak are waiting by the Chansey
statue to remember your father, and Oak has talked to the League: Giovanni's approval counts. After this, Archer, once a
Rocket executive, runs the Viridian Gym, and the grunt gives you the last Move Extractor in Viridian City. Oak tells you at the statue that Archer is
asking for you. Go and see him: he owed your father, and he gives you the <b>Tera Orb</b> your father started (see <a href="#tera">Terastallization</a>).
From here on the strongest trainers Terastallize too.</p>
<p>The League's front door turns you back until all eight Gym Leaders have approved. Moltres waits on
Victory Road 2F. Each member of the Elite Four approves you after losing.</p>
<p>With the 8th badge, the Leader Academy's headmaster has something for you: the <b>Mega Bracelet</b>. Walk into the
main hall and she calls you to her desk.</p>
<p>In the Champion's room, Oak walks in before the rival battle, heals your team, and announces that the winner becomes
Champion <i>and</i> Leader of the Pallet Town Gym.</p>

<h3>9. The finale: Oak's challenge</h3>
<p>After the credits, Oak meets you outside your house and takes you to the gym, and then he can't let it go: it's his town,
next to his lab ("Remember when I said I'd run it myself? I wasn't joking!"). He challenges you for the gym. You can put it
off; he'll wait in the gym. His team (Tauros, Exeggutor, Arcanine, Gyarados, Snorlax, Dragonite) has full IVs and EVs, held
items and the smartest AI, one level above your average (never below 65); no healing items. Losing just sends you home to
try again. Win, and the gym and your own badge are yours, and his aide explains the challenger system.</p>
"""

GYM = """
<h3>Challengers</h3>
<p>Talk to Oak's aide (right of the entrance statues) to open the doors: challengers come one at a time, Battle Frontier style,
until one beats you. Your team must follow the League's rules (one type, no two alike, no two holding the same item); it's healed between battles and losing doesn't white out. You
battle on your <b>Battle Stage</b> (a free decoration: place it, and challengers walk from the door to it along whatever path
your decorations leave).</p>
<ul>
<li><b>Random trainers</b>: 4-6 different fully evolved Pokémon from the first 386, legendaries included, smart AI.</li>
<li><b>Famous challengers</b> (every 7th in a streak, and sometimes otherwise): the Kanto Gym Leaders (not Giovanni: he's gone), the Elite Four, your
rival and Oak, their real teams levelled to match.</li>
<li><b>Townsfolk</b> (now and then): Mom, Nurse Joy, Bill, Daisy and Mr. Fuji, with teams of their own.</li>
<li>Every team's average level is your party's average.</li>
</ul>
<p>Each win pays ¥2,000 plus ¥200 per win in the streak (up to ¥10,000, at 40 wins; doubled for famous challengers,
townsfolk and your mentor) and an item. Usually it's a common one: a Rare Candy, Nugget, PP Up, Full Restore, Max Revive,
Max Elixir or a vitamin (less often a Bottle Cap, PP Max or Big Nugget). Famous challengers, townsfolk, every 10th win and
about 1 win in 16 give something rare instead: a Gold Bottle Cap, Ability Capsule, Cilantro, Multivitamin or a Master Ball.
About 1 win in 150 it's a <b>Move Extractor</b>. Whoever ends your streak earns your badge, and the statue lists them.</p>

<h3>Decorating</h3>
<p>The PC left of the entrance: <b>BUY DECORATIONS</b> lists everything unlocked so far, with a picture of each piece (choose
how many: up/down one at a time, left/right ten); <b>SELL DECORATIONS</b> sells what's in storage for half price; <b>PUT ALL AWAY</b> clears the gym
(floors and walls stay); <b>DECORATE</b> lets you walk the gym
(through things) and place the piece shown in front of you: A places, <b>START turns it</b>, SELECT picks up the one in
front (to move it: it's now the piece in hand), L/R switch pieces, B finishes. A piece turns red where it can't go, and the area by the door, statues and PC is shaded
(nothing goes there). There are general pieces, pieces from the Kanto gyms (Viridian's spinners and stop tiles, Cerulean's
water, Vermilion's trash cans, which keep working), and 5 decorations for every type. A type's pieces go on sale after a key
battle (table below), and all of them once the gym is yours. "New GYM decorations available!" tells you when something
unlocks.</p>
<ul>
<li><b>Turning</b>: most furniture can be turned to face any way, and changes shape to match (a couch is 3 wide from the front
and 3 long from the side). Round things look the same every way, so they don't turn.</li>
<li><b>Walls and pools join up</b>: GYM WALLs are one square each and join into one wall, with its front face showing on the
square below. Furniture can stand right against any wall, the gym's own back wall included: a tall piece (a bookshelf, a
plant) overlaps the wall with its top, the wall showing behind it, and a short one (a trash can) stands on the first square of
floor in front. Rugs, mats and pools can't overlap a wall. POOL WATER joins the same way: any shape of pool gets a rim only round its
outside. You can SURF in it.</li>
<li><b>Floors</b> (bought once, then use them anywhere): wood, stone, red carpet, sand, metal grating, grass, steel, ice and
<b>lava</b> (nobody can walk on it: it works like a wall, for fire gyms). Pick a floor in DECORATE (or switch to it with L/R): A paints the
square in front, hold A to keep painting as you walk, and SELECT fills the whole patch of floor in front of you. The floor
round the statues, the PC and the doormat can be painted too.</li>
<li><b>Wall styles</b> (bought once): brick, wood, stone, hedge or steel. Choosing one in DECORATE changes every wall in the
room you're in, the back wall included.</li>
</ul>

<h3>Extra rooms</h3>
<p>Oak has a room built onto your gym for every 100 kinds of POKéMON you catch: talk to him in his lab once you've caught
<b>100</b>, <b>200</b> and <b>300</b>. Each one opens a doorway in the gym: the first in the back wall (near its right end), the second in the left wall and the third in the right wall, halfway down. Walk through and keep going to go in.</p>
<p>The rooms are smaller than the gym (15 by 12 squares of floor) and each has its own PC by the way in. Decorate them as you
would the gym: the decorations come from the same storage, and each room has its own floors and its own wall style (bought
once, used anywhere), so one can be a trophy room and another a room for your favorite type. Trainers and the Battle Stage
stay in the gym itself. The square in front of each doorway is kept clear (anything already there goes back to storage when
the doorway opens). PUT ALL AWAY only clears the room you're in.</p>
<h3>Practice trainers</h3>
<p>Under TRAINERS in the PC: Rookie (¥5,000), Skilled (¥10,000) and Expert (¥20,000) trainers; up to 11 placed at once. Each
has a fresh random team of your type for practice battles (no prize, no whiteout, healed after). Some trainers on the road
ask to join your gym after you beat them; they're placed the same way and use their own favorite type. While one is placed in your gym, they're no longer at
their old spot on the road (they come back if you put them back in storage).</p>
<h3>Mastering types and changing your gym's type</h3>
<p>A streak of 15 wins masters your gym's type (10 for every type after the first). The streak's last battle is your mentor
with their full Gym Leader Convention team, three levels above your average. Random challengers bring at most one legendary,
and no post-game opponent uses healing items. Beat them and Oak offers a new type: choose one the gym hasn't had, and the badge
and pedestal change. The first time, you also get a gym uniform for every type (see <a href="#lab">the wardrobe</a>). In his lab Oak also switches the gym back to any type it has had, for free. Each type's legendary
arrives when the gym first becomes that type.</p>
<h3>Your type's legendary</h3>
<p>Right after Oak hands the gym over, the lights go out and the most powerful Pokémon of your type appears in the gym
(level 70, one battle). If it gets away, it hides in Cerulean Cave: a 1% encounter anywhere in the cave until it's caught.
The Gym Charm (from your mentor with the convention invitation) makes 1 in 250 Pokémon of your gym's type Shiny.
Cerulean Cave's guard steps aside once you've entered the Hall of Fame.</p>
"""

JOHTO = """
<p>Once the gym is yours, your mentor walks in with an invitation to the <b>Gym Leader Convention</b> in Goldenrod City.
Vermilion's harbor sailor takes you to Olivine; the greeter on the dock hands over the convention badge, Johto on your Town
Map, 5 Ultra Balls and 3 Full Restores. Olivine, Routes 34-40, Ecruteak, the National Park, Goldenrod, Ilex Forest and Azalea
are open (guards turn you back at the edges), with wild Pokémon around levels 50-66, and you can Fly between Johto's towns
(R on the Fly map switches regions).</p>
<p>The convention fills Goldenrod's Radio Tower: 127 Gym Leaders, Kahunas, Trial Captains, Elite Four and Champions from every
generation, in 18 type halls and a Champions' Hall. The receptionist sends you to any hall. Talk to anyone to battle: Gym
Leaders are level 75 (aces 76), Elite Four 78, Champions 85-88, full teams with perfect IVs. Your party is healed after every
battle, won or lost. Clearing a hall wins its trophy (a decoration for your gym); clearing all 19 wins the Grand Trophy.
Before each battle a title card shows who you're facing: their name, title, region and games.</p>
<p>Clearing every hall and mastering every type opens the last part of the game: <a href="#tournament">the tournament</a>.</p>
"""

SAFARI_INTRO = """
<p>The <b>Exotic Pokémon Safari</b> is the second door on Fuchsia City's Safari Zone building. Inside, a hall has a door for each
region: Johto, Hoenn, Sinnoh, Unova, Kalos, Alola, Galar, Paldea. Doors still being built say "COMING SOON". Once the Pallet
gym is yours, the attendant sells the <b>Exotic Pass</b> (¥10,000, once); then an open zone's door slides open when you walk
up to it.</p>
<ul>
<li>Normal wild battles with your own Poké Balls: no Safari Balls, no step limit. Wild Pokémon are levels 48-55.</li>
<li>Two kiosks stand behind the hall's counter. The <b>Safari Kiosk</b> (right) sells balls, Max Repels, healing items and
<b>Incense</b> (¥50,000). The <b>Evolution Kiosk</b> (left) sells every evolution item once you've found it in a zone.</li>
<li><b>Incense</b>: for 600 steps, any wild Pokémon that's 10% or less of a place's encounters is 5 times as likely (1%
becomes 5%, 10% becomes 50%), and the common ones share what's left. Where the rare ones add up to more than 20%, they
take all the encounters between them. It works everywhere, with every method (grass, caves, surfing, rods, Rock
Smash), and when it burns out you can light another straight away, like a Repel.</li>
<li>Every legendary is level 70. Most stand in one spot and appear only after a puzzle is solved (until then, checking their
spot gives a hint). If a battle doesn't catch one, it's gone for the rest of the visit and back next time you enter the
area.</li>
<li>Roaming legendaries can be in any area of their zone: each time you enter an area, there's a 1 in 8 chance each one is
there, wandering. They run if not caught.</li>
<li>Regional forms are wild in their own generation's zone, and the gym legendaries never appear here.</li>
</ul>
<p>On the maps: a legendary's picture marks where it appears (circled in red); yellow numbers are items and people, blue are
puzzle pieces, and green are the ways out.</p>
"""


ACADEMY = """
<p>A school for Gym Leader candidates from every region, through a gap in the trees on Route 11's north side. You're
admitted with your third badge (see the walkthrough). The headmaster, Wisteria, welcomes you; the staff are named for
flowers. Everything here is single battles. Once you've been, it's on the Town Map and you can FLY here; you land at
its Pokémon Center, which is also where you return to after a loss.</p>
<h3>Battle Points (BP)</h3>
<p>Beating candidates earns 1-4 BP by their skill, an instructor's demonstration 3 BP, and Battle Hall streaks 1 BP a win plus
1 for every 3 in the streak (and 5 more at 7 wins). Everything BP buys can also be bought with money.</p>
<h3>The candidates</h3>
<p>Eight future Gym Leaders practice on campus: Koji (Johto, Fighting, rookie), Maren (Hoenn, Water), Elias (Sinnoh, Ground),
Nadia (Unova, Electric, advanced), Rene (Kalos, Fairy, advanced), Keola (Alola, Fire, rookie), Briony (Galar, Dragon, expert)
and Teo (Paldea, Steel). They'll battle as often as you like, against your own team: their level follows yours (a little
below for rookies, above for experts), with items and smarter AI higher up. No money is lost, nobody whites out, and both
sides are healed after.</p>
<h3>The instructors and the lecture halls</h3>
<p>Eight instructors teach one subject each, all for double battles: Larkspur (speed control: Tailwind, Trick Room, Fake
Out), Camellia (setting up with a partner: Follow Me, Rage Powder), Foxglove (spread moves), Verbena (positioning and
switching), Marigold (weather and terrain), Hyacinth (targeting and Protect), Yarrow (EVs, natures and items) and Tansy
(team building). Each gives a short lecture and then a demonstration double battle: their three level-100 Pokémon that show
the lesson, against three rentals you pick.</p>
<h3>The Battle Hall</h3>
<p>Rental battles: pick 3 of 6 offered Pokémon (each shown with its item and moves), then battle candidates with rentals of their
own, the AI getting smarter as your streak grows. Keep going or stop after each win. Your own party is set aside and given
back afterwards, whatever happens.</p>
<h3>The Exchange</h3>
<p>Two counters side by side sell the same competitive items, one for money and one for BP: Choice items, Life Orb, Focus
Sash, Assault Vest, Booster Energy, Heavy-Duty Boots, weather rocks, berries and more. The BP counter also has Bottle Caps,
the Ability Capsule and Patch, Cilantro, the Multivitamin and mints. The supply shop sells training items for money: caps,
ability items, Cilantro, the Multivitamin, mints, vitamins, power items, the Macho Brace and the EV berries.</p>
<h3>The team builder</h3>
<p>Tansy's team builder, on the PC next to her in Lecture Hall D (use it any time), plans up to 8 teams: choose a type, then one of its Pokémon (a ball marks the ones you have), then up
to 4 of the moves it can learn. It only offers Pokémon you can get by now, so a plan never depends on something out of reach.
The edit screen shows the team's worst weaknesses and the types it can't hit hard; START shows the whole type chart for the
team. Teams are kept when you save. It's a planner: it doesn't change your party.</p>
<h3>Mega Evolution: Primrose's trials</h3>
<p>After your 8th badge, the headmaster gives you the <b>Mega Bracelet</b>, and the guard at door M lets you into Primrose's
room. A Mega Stone, she says, only answers a bond it has seen proven, so you earn each one in a trial:</p>
<ul>
<li>Pick a party Pokémon that can Mega Evolve (Charizard and Mewtwo pick X or Y). Primrose lends it the stone, the rest of
your party is set aside, and it battles her three Pokémon alone.</li>
<li>Win and the stone is yours to keep. Lose and try again any time.</li>
<li>Her teams are levels 45-49 with a level-50 Mega last, chosen so none of them is super effective against your Pokémon.
The levels never change, so training up is the way through.</li>
<li>Rayquaza needs a stone like everyone else (the Rayquazite), and Kyogre and Groudon earn the Blue and Red Orbs the
same way.</li>
</ul>
"""

LAB = """
<p>Under your house, behind the bookcase your mom moves after Silph Co., is your father's workshop. Three machines and a
wall switch; four loose pages; crates.</p>
<h3>The switch: Evil and Nice</h3>
<p>The machines start in <b>Evil Mode</b>. The switch on the back wall appears after the Viridian Gym, and flips them to
<b>Nice Mode</b> (and back). All three take Pokémon from your party, never an Egg.</p>
<h3>The Ability Synthesizer</h3>
<p>Every species has one <b>synthetic ability</b>: an existing ability, a little better than its own (the Pokédex lists
each). Feed the machine 3 Strong Acid (Evil) or 3 Apple Juice (Nice) and a Pokémon, and it comes out with it. In Evil
Mode its friendship drops to zero; in Nice Mode it loved the juice so much it's a little happier. A synthetic ability
stays through evolution (it becomes the evolved species' one) and isn't passed on by breeding. It isn't for good: an
Ability Capsule switches it back to a regular ability and an Ability Patch to the hidden one, and the Synthesizer can put it
back again (3 more Strong Acid or Apple Juice), as often as you like. A few Pokémon whose ability changes their form (Aegislash, Wishiwashi, Mimikyu...) can't be
synthesized.</p>
<h3>The Ball Maker</h3>
<p>Put in a Pokémon of level 50 or more and get a <b>Type Ball</b> of its primary type. In Evil Mode the Pokémon doesn't
come back (its held item goes to your bag, and it won't take your last Pokémon). In Nice Mode it only gives a little DNA:
it loses 5 levels (moves kept) and comes back.</p>
<h3>The Ball Changer</h3>
<p>Choose a Pokémon and a ball from your bag: it moves into the new ball, and the old one goes back in your bag.</p>
<h3 id="tera">Terastallization</h3>
<p>Archer, the Viridian Gym's new leader, gives you the <b>Tera Orb</b> after the Viridian finale. In battle, press START on
the move list to Terastallize the Pokémon in front: it becomes a single type and its moves of that type hit harder. One
Pokémon a battle; the Orb never needs charging.</p>
<ul>
<li><b>League battles: your gym's type.</b> In every battle under the League's one-type rule (gym leaders, your rival, the
Elite Four and Champion, Oak, and your gym's challengers) your Pokémon Terastallize into your gym's type, whatever their
own Tera type is.</li>
<li><b>Everywhere else: its own Tera type, if you've mastered it.</b> In any other battle (the tournament, the Gym Leader
Convention, ordinary trainers, wild Pokémon) a Pokémon uses its own Tera type, but only if it's a type you've mastered
(or your gym's type). Until you master a type, it's your gym's type everywhere. A mastered type stays yours when your gym
changes type.</li>
<li><b>Changing a Tera type.</b> Once you've mastered a type, talk to <b>Archer at the Viridian Gym</b> (he stands where
Giovanni did): he sets any party Pokémon's Tera type to your gym's type or any type you've mastered. It costs 3 of that type's Tera berry (below).</li>
<li><b>Who else Terastallizes.</b> From the moment you have the Orb, the important opponents' aces do too. In League battles
they pick a type that resists your gym's type; in the tournament, Bill's battles and the Convention, they go for the
type their team is built around. Ordinary trainers never do.</li>
</ul>
<h3 id="berries">Tera berries and the berry field</h3>
<p>Each type has a berry: Chilan (Normal), Chople (Fighting), Coba (Flying), Kebia (Poison), Shuca (Ground), Charti (Rock),
Tanga (Bug), Kasib (Ghost), Babiri (Steel), Occa (Fire), Passho (Water), Rindo (Grass), Wacan (Electric), Payapa (Psychic),
Yache (Ice), Haban (Dragon), Colbur (Dark) and Roseli (Fairy). Three of a type's berry pay for a Tera type change to it.</p>
<ul>
<li><b>Wild Pokémon</b> hold their (primary) type's berry 5% of the time: catch them, or take it with Thief or Covet.</li>
<li><b>One of each is lying around Kanto</b> in an item ball: Route 1 (Chilan), Viridian Forest (Tanga), Route 3 (Chople),
Mt. Moon 1F (Charti), Route 24 (Rindo), Route 25 (Passho), Route 4 (Kebia), Route 6 (Wacan), Diglett's Cave (Shuca), Route 5
(Payapa), Pokémon Tower 6F (Kasib), Celadon City (Roseli), Rocket Hideout B1F (Colbur), Route 13 (Coba), Silph Co. 7F (Babiri),
Pokémon Mansion 1F (Occa), Seafoam Islands 1F (Yache) and Victory Road 2F (Haban).</li>
<li><b>The berry field</b> in Pallet Town, east of your house below the water: 24 plots. Plant any berry (A on the soil); it
grows as you walk, about 400 steps from seed to ripe, and waits for you. Picking it gives 2-5 berries. Changed your mind? Pull a
growing plant up and you get the berry back.</li>
</ul>
<h3>The wardrobe: outfits</h3>
<p>The dresser in your room (the yellow one beside the bookshelf). It lists the outfits you have, and you change into each one as you scroll through them:
everything you do (walking, biking, surfing, fishing, battle) shows it.</p>
<ul>
<li><b>Casual clothes</b>: what you start in.</li>
<li><b>Rocket uniform</b>: one of the crates in the basement has a loose lid.</li>
<li><b>Gym uniforms</b>: your clothes in a type's Type Ball colors, with its emblem on your chest. Your type's comes from
your mentor with their first gift (after Mt. Moon); the first time you change your gym's type, you get all 18.</li>
</ul>
"""

PC = """
<p>Besides your boxes and items, the PC (and the Palm Pilot) has these once they've something in them:</p>
<ul>
<li><b>BILL'S ARCHIVE</b> (once you've met Bill): a living Pokédex. From the box menu, ARCHIVE sends a Pokémon there for
good: it's released, but its slot (and whether it was Shiny) is kept. Taking one out gives you a new one at level 5 (random
IVs and nature, Shiny if the stored one was) and empties the slot. One slot per species and form; the screen is laid out
like a box, 30 to a page in Pokédex order, with what you own in color. L/R turns a page, START jumps to the next
generation.</li>
<li><b>TYPE MASTERY</b>: for each type you've mastered, the team of your longest streak with it.</li>
<li><b>TOURNAMENT</b>: your first tournament title's team, kept for good, and your three most recent, each with the title's
number and whom you beat in the final.</li>
<li><b>HALL OF FAME</b>: the Elite Four's, as always.</li>
</ul>
"""

TOURNAMENT = """
<p>Clear every hall of the Gym Leader Convention and master every type, then walk into your gym: a hooded figure is waiting
by the door. A bookie, he says, running the book on an underground tournament of the strongest trainers in the world.
Your name's in the draw.</p>
<h3>The rules</h3>
<ul>
<li><b>128 entrants, single elimination.</b> Each tournament draws 127 from the convention's Gym Leaders, Elite Four and
Champions and the rest of the Pokémon world's rivals, villains, Frontier Brains and professors, plus you. Seven wins takes the
title.</li>
<li>Seeding is rough: champions and legends are spread through the bracket so they meet late; everyone else is placed at
random. You're unseeded. Every tournament is a new draw.</li>
<li>Everyone's Pokémon are level 100, with the smartest AI. Any of your Pokémon may play, no type rule; but no two of the
same species, and no two holding the same item.</li>
<li>Your team is locked in when you sign up: the Pokémon with you then, no more and no fewer (any order). You're healed
between rounds, and you can rest between rounds and come back.</li>
<li>Each win pays the usual prize money for the opponent's class (a Leader, Elite Four or Champion pays ¥10,000), and a
prize: usually a Tera berry, a valuable, vitamins, a Rare Candy or the like; now and then something rare (a Bottle Cap,
Cilantro, an Ability Capsule or Patch, a Multivitamin, a Gold Bottle Cap, a Move Extractor, the Incense, a clover or coin for
the Lucky Egg and Amulet Coin, a Limited Edition Soda once you have the Drink Machine). The item list says which.</li>
<li><b>One loss ends it.</b> The rest of the tournament plays out without you; then you can sign up for a new one. You can
also forfeit.</li>
</ul>
<h3>The board</h3>
<p>From the bookie's menu, between rounds and after you're knocked out, the board shows the bracket. 128 names don't fit on
one screen, so it has nine pages: blocks A to H (each a sixteenth of the bracket, down to the round of 16) and the final 16
(down to the champion). L/R turns the page. Winners' lines are lit orange; your path is blue and your name red; anyone
knocked out is grayed.</p>
<h3>After the title</h3>
<p>Win, and the bookie admits the one who put the tournament on wants to meet you. When you're ready, the hood comes off:
it's <b>Bill</b>. He ran his Cell Separation System backward on himself, trying to become a Clefable, and it went wrong:
what came out is a monster, CLEFFABILL (Fairy/Ghost), and it's eating what's left of him. He put on the tournament to find
someone strong enough to end it. His team: five Shiny legendaries and himself. Losing changes nothing; he asks you to try
again.</p>
<p>Beat him and he fades away, leaving his <b>Palm Pilot</b>:</p>
<ul>
<li><b>WITHDRAW</b>: any Pokémon at all, Shiny, at level 70 (a list of every species and form; L/R a page, START the next
generation).</li>
<li><b>Battle simulator</b>: new tournaments from Bill's records, the same rules. Win one, then beat Bill's simulation (his
same team; as many tries as it takes) for another WITHDRAW.</li>
</ul>
"""

EVENT = """
<p>Every Pokémon form that was only ever given out at events is a side quest here. Gifts after your gym is yours are level 50
(first stages level 5); the main-game ones are level 5.</p>
<h3>Main game</h3>
<ul>
<li><b>The cap collector</b> (the boy in the house in Pallet Town): his seven Pikachu caps are lost around Kanto. He gives you
the Cap Case; each cap you find is traded for its cap Pikachu (Original, Hoenn, Sinnoh, Unova, Kalos, Alola, World), and all
seven for the Partner Cap Pikachu. The caps are in Viridian Forest, Mt. Moon B2F, Route 25, Route 9, Route 10 and both
Underground Paths, where small things get snagged: check carefully.</li>
</ul>
<h3>After the gym is yours</h3>
<ul>
<li><b>The costume maker</b> (Celadon Condominiums 2F) gives you Cosplay Pikachu, then dresses it up as each costume is
earned: Rock Star (a 7-win Battle Hall streak), Belle (20 decorations placed in your gym), Pop Star (clear the
Convention's Fairy hall), Ph.D. (attend all 8 Academy lectures) and Libre (clear the Fighting hall). Each costume comes with
its special move.</li>
<li><b>Partner Pikachu and Eevee</b>: once you've mastered Electric (or Normal), Oak has a five-question quiz on that type in
his lab. Pass for the partner.</li>
<li><b>Spiky-eared Pichu</b>: beside the shrine in the safari's Ilex Forest, when Celebi is in your party.</li>
<li><b>Vivillon patterns</b>: the collector in Lumiose City (Kalos zone) gives a Fancy-pattern Scatterbug once you've shown him
6 Vivillon patterns, and a Poké Ball-pattern one for all 18. After that, each turns up wild in the Kalos zone now and
then.</li>
<li><b>Battle Bond Greninja</b>: the trainer at Azure Bay (Kalos zone) gives you a special Froakie once you've beaten a Kalos
trainer in the tournament (or won it). Its Greninja has Battle Bond.</li>
<li><b>The Eternal Flower Floette</b>: AZ, in the Lumiose Badlands, once you've beaten him in the tournament (or won it). Show
him a Floette, then come back with Xerneas, Yveltal and Zygarde caught.</li>
<li><b>Own Tempo Rockruff</b>: Tala at Hano Beach (Alola zone) battles you with her Lycanroc pack; win for the Rockruff.</li>
<li><b>Original Color Magearna</b>: the artisan at Hano Beach. Catch Magearna, then find her four antique paint pots, one each
in Big Wave Beach, the Ruins of Conflict, Wela Volcano Park and Mount Lanakila.</li>
<li><b>Dada Zarude</b>: in Glimwood Tangle's glade once Zarude is caught, for a party with Celebi.</li>
<li><b>Eternamax Eternatus</b>: on the Wyndon Stadium pitch, for a party with Zacian, Zamazenta and Eternatus (the Darkest
Day). It's Eternamax only on the pitch; anywhere else it's Eternatus again.</li>
</ul>
"""


PLACE_NAMES = {
    "ViridianCity_House_Frlg": "Viridian City, house", "PewterCity_House2_Frlg": "Pewter City, house",
    "CeruleanCity_House3_Frlg": "Cerulean City, house", "CeruleanCity_House4_Frlg": "Cerulean City, house",
    "VermilionCity_House1_Frlg": "Vermilion City, house", "VermilionCity_House2_Frlg": "Vermilion City, house",
    "LavenderTown_House2_Frlg": "Lavender Town, house", "CeladonCity_Hotel_Frlg": "Celadon City, hotel",
    "SaffronCity_House_Frlg": "Saffron City, house", "FuchsiaCity_House1_Frlg": "Fuchsia City, house",
    "FuchsiaCity_House3_Frlg": "Fuchsia City, house", "CinnabarIsland_PokemonLab_Lounge_Frlg": "Cinnabar Lab, lounge",
    "PalletTown_TrainingHall_Frlg": "Pallet Town, Training Hall",
}


def training_section():
    import build_training_houses as T
    rows = []
    for name, cls, mp, _pos, _f, _line, team in T.TRAINERS:
        lv = f"{min(m[1] for m in team)}-{max(m[1] for m in team)}"
        mons = ", ".join(f"{m[0]} {m[1]}" for m in team)
        rows.append(f"<tr><td>{esc(PLACE_NAMES.get(mp, mp))}</td><td>{cls} {name.title()}</td><td>{lv}</td><td>{esc(mons)}</td></tr>")
    return ("<p>After your first Hall of Fame, trainers move into houses around Kanto to help you raise a new team (say, after "
            "changing your gym's type). Talk to one and say YES to battle; they battle again as often as you like. They have no "
            "type rule, so bring anyone. Most teams are built around Chansey, Blissey and Audino, which give a lot of EXP; "
            "every town also has a trainer whose whole team can be hit by every type (no Normal, Ghost, Flying, Ground, Steel, "
            "Dark or Fairy Pokémon, no Levitate or Flash Fire), so a Ghost or Ground team can train too. They're Ladies, Gentlemen and Beauties, so they pay well (hold the Amulet Coin for double). The levels rise "
            "through the towns in story order.</p><p>The top tier is the <b>Pallet Training Hall</b>, the house in the clearing "
            "east of Oak's lab: six trainers at levels 63-82 (three of them with no immunities), and a nurse by the door who heals your team.</p>"
            "<table class='grid'><tr><th>Where</th><th>Trainer</th><th>Levels</th><th>Team</th></tr>" + "".join(rows) + "</table>")


def build(out):
    parts = ['<div class="cover"><h1>Pokémon Canvas</h1><div class="sub">Player\'s Guide</div>'
             '<p style="margin-top:30px;color:#777">The story, your gym, the Gym Leader Convention and the Exotic Pokémon Safari'
             '<br>(spoilers throughout)</p></div>']
    parts.append(chapter("Contents", """<div class="toc"><ol>
<li><a href="#about">About the game</a></li><li><a href="#types">Choosing your type</a></li>
<li><a href="#walk">Walkthrough</a></li><li><a href="#gym">Running your gym</a></li>
<li><a href="#pc">Your PC</a></li><li><a href="#lab">Dad's lab and the wardrobe</a></li><li><a href="#academy">The Leader Academy</a></li>
<li><a href="#quests">Side quests around Kanto</a></li><li><a href="#items">Items</a></li><li><a href="#training">Training houses</a></li><li><a href="#johto">Johto and the Gym Leader Convention</a></li>
<li><a href="#tournament">The tournament</a></li><li><a href="#event">Event Pokémon</a></li>
<li><a href="#teams">Trainers and their teams</a></li><li><a href="#safari">The Exotic Pokémon Safari</a></li><li><a href="#itemlist">Item list</a></li></ol></div>""", newpage=False))
    parts.append(chapter("1. About the game", INTRO + RULES + DIFFERENCES, newpage=False, anchor="about"))
    parts.append(chapter("2. Choosing your type", "<p>Oak lists the types from the strongest possible team (the six best "
                         "species of that type in the first 386) to the weakest. Your rival's type beats yours. Your mentor "
                         "is the Kanto leader or Elite Four member of your type, or the closest fit.</p>" + type_table()
                         + type_extras_table(), anchor="types"))
    parts.append(chapter("3. Walkthrough", WALKTHROUGH, anchor="walk"))
    parts.append(chapter("4. Running your gym", GYM, anchor="gym"))
    parts.append(chapter("5. Your PC", PC, newpage=False, anchor="pc"))
    parts.append(chapter("6. Dad's lab and the wardrobe", LAB, anchor="lab"))
    parts.append(chapter("7. The Leader Academy", ACADEMY, anchor="academy"))
    parts.append(chapter("8. Side quests around Kanto", QUESTS, anchor="quests"))
    parts.append(chapter("9. Items", ITEMS, anchor="items"))
    parts.append(chapter("10. Training houses", training_section(), anchor="training"))
    parts.append(chapter("11. Johto and the Gym Leader Convention", JOHTO, anchor="johto"))
    parts.append(chapter("12. The tournament", TOURNAMENT, anchor="tournament"))
    parts.append(chapter("13. Event Pokémon", EVENT, anchor="event"))
    rival, _ = rivals_and_legends()
    player_for_rival = {}
    for t, starter, *_ in TYPES:
        rt = rival.get(starter.upper())
        if rt:
            player_for_rival.setdefault(rt, []).append(t)
    parts.append(chapter("14. Trainers and their teams", TT.build(badge, player_for_rival), anchor="teams"))
    parts.append(chapter("15. The Exotic Pokémon Safari", SAFARI_INTRO, anchor="safari"))
    parts.append(safari_section())
    tables, n = IL.html(esc)
    parts.append(chapter("16. Item list", f"<p>Every item you can get ({n} of them), pocket by pocket, with what it does and "
                         "where it's found: item balls, hidden items (the Itemfinder helps), shops and gifts. Lists of more than "
                         "eight places are cut short. Many wild Pokémon also hold items, and Thief or Covet takes them.</p>"
                         + "".join(tables), anchor="itemlist"))
    html = "<!doctype html><html><head><meta charset='utf-8'><style>" + CSS + TT.TEAM_CSS + "</style></head><body>" + "".join(parts) + "</body></html>"
    hp = OUT_DIR / "guide.html"
    hp.write_text(html)
    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        b = p.chromium.launch()
        pg = b.new_page()
        pg.goto(hp.as_uri())
        pg.pdf(path=str(out), format="Letter", print_background=True,
               display_header_footer=True, header_template="<span></span>",
               footer_template="<div style='font-size:8px;width:100%;text-align:center;color:#888'>Pokémon Canvas · Player's Guide · <span class='pageNumber'></span></div>",
               margin={"top": "0.55in", "bottom": "0.65in", "left": "0.6in", "right": "0.6in"})
        b.close()
    print(out)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(OUT_DIR / "guide.pdf"))
    build(Path(ap.parse_args().out))
