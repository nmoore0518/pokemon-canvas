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
    ("Ground", "Sandshrew", "Giovanni", "Shovel", "Super Nerd Miguel (Mt. Moon)", "Drew (Route 9)", "Granite Sarcophagus", "Diamond Drill Bit"),
    ("Flying", "Spearow", "Lance", "Rocket", "the rival in Silph Co.", "Perry (Route 13)", "Aviator Goggles", "Jet Engine"),
    ("Dragon", "Dratini", "Lance", "Egg", "Lance (Elite Four)", "Shirley (Route 20)", "Fire Shroud", "Bionic Jaw"),
    ("Fighting", "Tyrogue", "Bruno", "Knockout", "the Fighting Dojo master", "Hitoshi (Dojo)", "Mouth Guard", "Steroids"),
    ("Dark", "Poochyena", "Giovanni", "Evil", "Picnicker Dana (Rock Tunnel)", "Lewis (Pokémon Mansion)", "Black Veil", "Manifesto"),
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
City holds Pokémon from every region, and at the very end an underground tournament of the world's strongest trainers hides
one last secret. Along the way, the Leader Academy on Route 11 trains Gym Leader candidates (and hands out Mega Stones).</p>
<p>This guide has spoilers throughout: the story, every legendary's hiding place, and every puzzle's solution.</p>
"""

RULES = """
<h3>Your team: the League's rules</h3>
<ul>
<li><b>One type.</b> Every Pokémon in your party must have your type as its <i>first</i> (primary) type. A Grass/Poison
Bulbasaur counts as Grass; a Poison/Grass Pokémon wouldn't.</li>
<li><b>No duplicates.</b> No two party Pokémon may be the same species (different stages of one family are fine: Ivysaur and
Venusaur together are allowed).</li>
<li>The rules are checked before Gym Leader and rival battles, and once at the League's door for the Elite Four and the
Champion. Break them and the game explains why and lets you fix your team. Ordinary trainers and wild battles have no rules,
so you can catch and train anything; only your gym battles care.</li>
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
<li><b>Mega Evolution.</b> The Leader Academy's headmaster gives you the Mega Bracelet after your 8th badge, and each Mega
Stone is earned in a trial (see <a href="#academy">the Leader Academy</a>).</li>
<li><b>Summary screen.</b> On the skills page, A switches between stats, IVs (exact values) and EVs. A new LEVEL-UP MOVES page
lists every move the species learns and when.</li>
<li><b>Gym type finder.</b> On the Town Map, SELECT marks every area where a Pokémon of your type can be found.</li>
<li><b>The Sevii Islands are gone.</b> Johto replaces them as the post-game area, and Moltres now waits on Victory Road 2F
(level 50), as in Red and Blue.</li>
</ul>
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
Pokémon Tower, and Giovanni a Multivitamin at Silph Co.</p>
<h3>EXP and money</h3>
<ul>
<li><b>Lucky Egg</b> (held: 1.5x EXP): at the base of the Chansey statue in Pallet Town, west of the Pokémon Center.
Check the statue.</li>
<li><b>Amulet Coin</b> (held: double prize money): an item ball in the Pokémon Mansion, 3F, in the room with the
machines. Oak's aide in the Route 16 gate gives another once you've caught 40 kinds of Pokémon.</li>
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
<li>Say <b>YES</b> when the grunt asks you to join Team Rocket (or, if you said no and beat him, talk to him again).</li>
<li>After you beat Giovanni at Silph Co., he finds you in Saffron City.</li>
<li>After Giovanni approves you as a Gym Leader, he finds you in Viridian City.</li>
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
new leader needs the approval of all eight Gym Leaders and the Elite Four. The Hot Air Balloon (Fly) is his gift here.</p>

<h3>2. Viridian City, the parcel, and your mentor</h3>
<p>Oak's Parcel carries a letter: Oak wrote to your <b>mentor</b> (the leader or Elite Four member of your type), and the
reply says they're coming. After the Pokédex scene they walk into the lab, introduce themselves, give advice about running a
gym, and leave a housewarming gift in your gym's PC (a decoration of your type). They'll turn up again through the story,
with your type items.</p>
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
On Nugget Bridge, say yes to the Rocket grunt for your first <b>Move Extractor</b>. Bill gives you the S.S. Ticket and the
<b>Palm Pilot</b>. Your rival's lines from Route 22 on are about the race for approvals and the Pallet gym.</p>

<h3>5. Vermilion City to Celadon City</h3>
<p>The S.S. Anne (the captain gives HM01 and the Hedge Clippers), Lt. Surge, Rock Tunnel, Lavender Town and Celadon City as in
FireRed. With your third badge, step onto Route 11: your mentor (or Oak) calls you over to a gap in the trees on the north
side, talks the guards round, and you're admitted to the <b>Leader Academy</b> (see <a href="#academy">its chapter</a>). It's
optional, but its candidates and Battle Hall are the best training in the game. Celadon's Game Corner prizes are Abra, Clefairy, Dratini, Scyther and Porygon; the Dept. Store 4F sells every
evolution stone and trade item. Erika gives Cilantro with her TM.</p>

<h3>6. Pokémon Tower, Saffron City, Silph Co.</h3>
<p>Your rival gives you a Gold Bottle Cap at the Pokémon Tower. Silph Co. as in FireRed: Lapras is the gift on 7F, Giovanni
gives a Multivitamin after his defeat, and the Rocket grunt finds you in Saffron with the second Move Extractor. The Fighting
Dojo gives Hitmonlee or Hitmonchan.</p>

<h3>7. Fuchsia City, the Safari Zone, Cinnabar Island</h3>
<p>Koga gives an Ability Capsule. The Safari Zone (Surf: HM03 and the Boogie Board in its secret house; Strength from the
warden) as in FireRed. Next to the Safari Zone office, a second door says "NEW! EXOTIC POKéMON SAFARI": it's for Gym Leaders
only, so it waits for the post-game. On Cinnabar, Blaine's badge brings your mentor with your <b>offensive type item</b>, and
the lab revives Mt. Moon's fossil and the Old Amber. Bill doesn't take you to the Sevii Islands any more.</p>

<h3>8. Viridian Gym, Victory Road, the League</h3>
<p>Beat Giovanni and hold his gym for his approval; he then disbands Team Rocket, and the grunt gives you the last Move
Extractor in Viridian City. The League's front door turns you back until all eight Gym Leaders have approved. Moltres waits on
Victory Road 2F. Each member of the Elite Four approves you after losing.</p>
<p>With the 8th badge, the Leader Academy's headmaster has something for you: the <b>Mega Bracelet</b>. Walk into the
main hall and she calls you to her desk.</p>
<p>In the Champion's room, Oak walks in before the rival battle, heals your team, and announces that the winner becomes
Champion <i>and</i> Leader of the Pallet Town Gym.</p>

<h3>9. The finale: Oak's challenge</h3>
<p>After the credits, Oak meets you outside your house and takes you to the gym, and then he can't let it go: it's his town,
next to his lab ("Remember when I said I'd run it myself? I wasn't joking!"). He challenges you for the gym. You can put it
off; he'll wait in the gym. His team (Tauros, Exeggutor, Arcanine, Gyarados, Snorlax, Dragonite) has full IVs and EVs, held
items, two Full Restores and the smartest AI, five levels above your average (never below 65). Losing just sends you home to
try again. Win, and the gym and your own badge are yours, and his aide explains the challenger system.</p>
"""

GYM = """
<h3>Challengers</h3>
<p>Talk to Oak's aide (right of the entrance statues) to open the doors: challengers come one at a time, Battle Frontier style,
until one beats you. Your team must follow the one-type rule; it's healed between battles and losing doesn't white out. You
battle on your <b>Battle Stage</b> (a free decoration: place it, and challengers walk from the door to it along whatever path
your decorations leave).</p>
<ul>
<li><b>Random trainers</b>: 4-6 different fully evolved Pokémon from the first 386, legendaries included, smart AI.</li>
<li><b>Famous challengers</b> (every 7th in a streak, and sometimes otherwise): the eight Gym Leaders, the Elite Four, your
rival, Giovanni and Oak, their real teams levelled to match.</li>
<li><b>Townsfolk</b> (now and then): Mom, Nurse Joy, Bill, Daisy and Mr. Fuji, with teams of their own.</li>
<li>Every team's average level is your party's average.</li>
</ul>
<p>Each win pays ¥2,000 plus ¥200 per win in the streak (doubled for famous challengers and townsfolk) and an item; famous
challengers, every 10th win and the odd lucky win give something rare: a Gold Bottle Cap, Ability Capsule, Cilantro,
Multivitamin or a Master Ball. Very rarely (about 1 win in 150) it's a <b>Move Extractor</b>. Whoever ends your streak earns your badge, and the statue lists them.</p>

<h3>Decorating</h3>
<p>The PC left of the entrance: <b>BUY DECORATIONS</b> lists everything unlocked so far (choose how many: up/down one at a
time, left/right ten); <b>SELL DECORATIONS</b> sells what's in storage for half price; <b>PUT ALL AWAY</b> clears the gym
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
<li><b>Walls and pools join up</b>: GYM WALLs are one square each and join into one wall (with its front face showing on the
square below, where only another wall can go). POOL WATER joins the same way: any shape of pool gets a rim only round its
outside. You can SURF in it.</li>
<li><b>Floors</b> (bought once, then use them anywhere): wood, stone, red carpet, sand, metal grating, grass, steel, ice and
<b>lava</b> (nobody can walk on it: it works like a wall, for fire gyms). Pick a floor in DECORATE (or switch to it with L/R): A paints the
square in front, hold A to keep painting as you walk, and SELECT fills the whole patch of floor in front of you.</li>
<li><b>Wall styles</b> (bought once): brick, wood, stone, hedge or steel. Choosing one in DECORATE changes every wall in the
gym, the back wall included.</li>
</ul>
<h3>Practice trainers</h3>
<p>Under TRAINERS in the PC: Rookie (¥5,000), Skilled (¥10,000) and Expert (¥20,000) trainers; up to 11 placed at once. Each
has a fresh random team of your type for practice battles (no prize, no whiteout, healed after). Some trainers on the road
ask to join your gym after you beat them; they're placed the same way and use their own favorite type.</p>
<h3>Mastering types and changing your gym's type</h3>
<p>A streak of 20 wins masters your gym's type (10 for every type after the first). The streak's last battle is your mentor
with their full Gym Leader Convention team. Beat them and Oak offers a new type: choose one the gym hasn't had, and the badge
and pedestal change. In his lab Oak also switches the gym back to any type it has had, for free. Each type's legendary
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
<li>The <b>Safari Kiosk</b> on the hall's counter sells balls, Max Repels and healing items, plus every evolution item once
you've found it in a zone.</li>
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
flowers. Everything here is single battles. The Pokémon Center makes Vermilion City your place to return to.</p>
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
<p>Eight instructors teach one subject each: Larkspur (speed and priority), Camellia (setup and win conditions), Foxglove
(entry hazards), Verbena (pivoting), Marigold (weather and terrain), Hyacinth (reading the opponent), Yarrow (EVs, natures and
items) and Tansy (team building). Each gives a short lecture and then a demonstration battle: their three level-100 Pokémon
that show the lesson, against three rentals you pick.</p>
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
<li>Her teams are levels 55-59 with a level-60 Mega last, chosen so none of them is super effective against your Pokémon.
The levels never change, so training up is the way through.</li>
<li>Rayquaza needs a stone like everyone else (the Rayquazite), and Kyogre and Groudon earn the Blue and Red Orbs the
same way.</li>
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
<li>Everyone's Pokémon are level 100, with the smartest AI. Any of your Pokémon may play, no type rule.</li>
<li>Your team is locked in when you sign up: the Pokémon with you then, no more and no fewer (any order). You're healed
between rounds, and you can rest between rounds and come back.</li>
<li>Each win pays the usual prize money for the opponent's class (a Leader, Elite Four or Champion pays ¥10,000).</li>
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
<li><a href="#pc">Your PC</a></li><li><a href="#academy">The Leader Academy</a></li>
<li><a href="#items">Items</a></li><li><a href="#training">Training houses</a></li><li><a href="#johto">Johto and the Gym Leader Convention</a></li>
<li><a href="#tournament">The tournament</a></li><li><a href="#event">Event Pokémon</a></li>
<li><a href="#teams">Trainers and their teams</a></li><li><a href="#safari">The Exotic Pokémon Safari</a></li></ol></div>""", newpage=False))
    parts.append(chapter("1. About the game", INTRO + RULES + DIFFERENCES, newpage=False, anchor="about"))
    parts.append(chapter("2. Choosing your type", "<p>Oak lists the types from the strongest possible team (the six best "
                         "species of that type in the first 386) to the weakest. Your rival's type beats yours. Your mentor "
                         "is the Kanto leader or Elite Four member of your type, or the closest fit.</p>" + type_table()
                         + type_extras_table(), anchor="types"))
    parts.append(chapter("3. Walkthrough", WALKTHROUGH, anchor="walk"))
    parts.append(chapter("4. Running your gym", GYM, anchor="gym"))
    parts.append(chapter("5. Your PC", PC, newpage=False, anchor="pc"))
    parts.append(chapter("6. The Leader Academy", ACADEMY, anchor="academy"))
    parts.append(chapter("7. Items", ITEMS, anchor="items"))
    parts.append(chapter("8. Training houses", training_section(), anchor="training"))
    parts.append(chapter("9. Johto and the Gym Leader Convention", JOHTO, anchor="johto"))
    parts.append(chapter("10. The tournament", TOURNAMENT, anchor="tournament"))
    parts.append(chapter("11. Event Pokémon", EVENT, anchor="event"))
    rival, _ = rivals_and_legends()
    player_for_rival = {}
    for t, starter, *_ in TYPES:
        rt = rival.get(starter.upper())
        if rt:
            player_for_rival.setdefault(rt, []).append(t)
    parts.append(chapter("12. Trainers and their teams", TT.build(badge, player_for_rival), anchor="teams"))
    parts.append(chapter("13. The Exotic Pokémon Safari", SAFARI_INTRO, anchor="safari"))
    parts.append(safari_section())
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
