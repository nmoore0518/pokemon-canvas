"""Pokémon Canvas guide: every major trainer's team (read by build_guide.py), for planning a team
around them. All of it comes from the game's own data, so rebuild the guide after a team changes:
  - the story's Gym Leaders, Elite Four and Giovanni   src/data/trainers_frlg.party
  - the rival, by his type and the battle              src/data/rival_teams_frlg.party
  - Oak's battle for the gym, the gym's famous and
    townsfolk challengers                              src/gym_challenge.c
  - the Gym Leader Convention                          tools/gym_hack/convention_roster.py, convention_teams.py,
                                                       build_convention.py (levels)
  - the tournament's entrants                          tools/gym_hack/gauntlet_roster.py (and the convention's)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
sys.path.insert(0, str(ROOT / "tools/gym_hack/guide"))
import build_dex as D                 # noqa: E402
import convention_roster as CR        # noqa: E402
import convention_teams as CT         # noqa: E402
import gauntlet_roster as GR          # noqa: E402
from build_convention import LEVELS as CONV_LEVELS   # noqa: E402

_TYPES = None


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def norm(name):
    """'Weezing-Galar' / 'Mr. Mime' / "Farfetch'd" -> 'WEEZING_GALAR' / 'MR_MIME' / 'FARFETCHD'"""
    n = name.strip().upper().replace("É", "E").replace("♀", "_F").replace("♂", "_M")
    n = re.sub(r"[.'’:]", "", n)
    n = re.sub(r"[\s\-]+", "_", n)
    return n


def species_types():
    """SPECIES name (no prefix) -> [types]"""
    global _TYPES
    if _TYPES is None:
        data = D.load()
        _TYPES = {}
        for key, body in data["species"].items():
            types = list(dict.fromkeys(re.findall(r"TYPE_(\w+)", D.field(body, "types") or "")))
            if types:
                _TYPES[key[len("SPECIES_"):]] = types
    return _TYPES


def types_of(species):
    t = species_types()
    n = norm(re.sub(r"^Shiny ", "", species.strip()))
    for cand in (n, n + "_NORMAL", n.replace("_MEGA", "")):
        if cand in t:
            return t[cand]
    forms = sorted(k for k in t if k.startswith(n + "_"))      # (Aegislash -> AEGISLASH_SHIELD...)
    if forms:
        return t[forms[0]]
    base = n.split("_")[0]
    return t.get(base, [])


def pretty(species):
    return species.replace("_", " ").title().replace("Ho Oh", "Ho-Oh").replace("Mr Mime", "Mr. Mime")


# ---------------------------------------------------------------------------------------------
# One Pokémon, one team
# ---------------------------------------------------------------------------------------------
def mon_row(badge, species, level, item="", moves=(), note=""):
    tb = " ".join(badge(t.title()) for t in types_of(species))
    lv = f"{level}" if level != "" else ""
    mv = " / ".join(moves) if moves else ("its level-up moves" if level not in ("avg", "avg+5") else "")
    return (f"<tr><td><b>{esc(species)}</b> {tb}</td><td>{lv}</td><td>{esc(item)}</td>"
            f"<td>{esc(mv)}{(' <i>' + esc(note) + '</i>') if note else ''}</td></tr>")


def team_table(badge, title, mons, sub=""):
    """mons: [(species, level, item, moves)]"""
    head = f"<div class='team'><div class='tname'>{title}" + (f" <span class='tsub'>{sub}</span>" if sub else "") + "</div>"
    rows = "".join(mon_row(badge, *m) for m in mons)
    return head + "<table class='grid teamt'><tr><th style='width:30%'>Pokémon</th><th style='width:5%'>Lv</th>" \
                  "<th style='width:17%'>Item</th><th>Moves</th></tr>" + rows + "</table></div>"


def parse_showdown(line):
    """'Species @ Item: Move / Move' -> (species, item, [moves])"""
    head, _, moves = line.partition(":")
    sp, _, item = head.partition("@")
    return sp.strip(), item.strip(), [m.strip() for m in moves.split("/") if m.strip()]


# ---------------------------------------------------------------------------------------------
# .party files (the story trainers, the rival)
# ---------------------------------------------------------------------------------------------
def party_file(path):
    """{block name: dict(fields, mons=[(species, level, item, [moves])])}"""
    out = {}
    text = (ROOT / path).read_text()
    for m in re.finditer(r"^=== (\w+) ===\n(.*?)(?=^=== |\Z)", text, re.S | re.M):
        name, body = m.group(1), m.group(2)
        chunks = [c for c in re.split(r"\n\s*\n", body.strip()) if c.strip()]
        fields = {}
        mons = []
        for c in chunks:
            lines = c.strip().split("\n")
            if all(re.match(r"^[A-Za-z ]+: ", l) for l in lines):
                for l in lines:
                    k, _, v = l.partition(": ")
                    fields[k] = v
                continue
            first = lines[0]
            sp, _, item = first.partition("@")
            sp = sp.strip()
            mm = re.match(r".*\((.+)\)$", sp)            # "Nickname (Species)"
            if mm:
                sp = mm.group(1)
            sp = re.sub(r"\s*\((M|F)\)$", "", sp)
            lv = next((l.split(":", 1)[1].strip() for l in lines if l.startswith("Level:")), "")
            moves = [l[2:].strip() for l in lines if l.startswith("- ")]
            mons.append((sp, lv, item.strip(), moves))
        out[name] = dict(fields, mons=mons)
    return out


STORY = [
    ("Gym Leaders", ["LEADER_BROCK", "LEADER_MISTY", "LEADER_LT_SURGE", "LEADER_ERIKA", "LEADER_KOGA",
                     "LEADER_SABRINA", "LEADER_BLAINE", "LEADER_GIOVANNI"]),
    ("Elite Four", ["ELITE_FOUR_LORELEI", "ELITE_FOUR_BRUNO", "ELITE_FOUR_AGATHA", "ELITE_FOUR_LANCE"]),
    ("Elite Four rematches", ["ELITE_FOUR_LORELEI_2", "ELITE_FOUR_BRUNO_2", "ELITE_FOUR_AGATHA_2", "ELITE_FOUR_LANCE_2"]),
    ("Giovanni (Team Rocket)", ["BOSS_GIOVANNI", "BOSS_GIOVANNI_2"]),
]

RIVAL_STAGES = [("LAB", "Oak's lab"), ("ROUTE22_EARLY", "Route 22 (first)"), ("CERULEAN", "Cerulean City"),
                ("SS_ANNE", "S.S. Anne"), ("POKEMON_TOWER", "Pokémon Tower"), ("SILPH", "Silph Co."),
                ("ROUTE22_LATE", "Route 22 (before the League)"), ("CHAMPION_FIRST", "The Champion"),
                ("CHAMPION_REMATCH", "The Champion (rematch)")]


def story_section(badge):
    tr = party_file("src/data/trainers_frlg.party")
    out = []
    for title, names in STORY:
        out.append(f"<h3>{title}</h3>")
        for n in names:
            t = tr.get("TRAINER_" + n)
            if not t:
                continue
            label = esc(t.get("Name", n).title())
            if n.endswith("_2"):
                label += " (rematch)"
            items = t.get("Items", "")
            sub = f"items: {esc(items)}" if items else ""
            out.append(team_table(badge, label, t["mons"], sub))
    return "".join(out)


def rival_section(badge, player_for_rival):
    rv = party_file("src/data/rival_teams_frlg.party")
    types = sorted({k.split("_")[-1] for k in rv})
    out = ["<p>Your rival's type depends on yours (the table in chapter 2): find his type below. Each battle's "
           "team is listed in the order he sends them out.</p>"]
    for t in types:
        players = player_for_rival.get(t.title(), [])
        who = (" - his type if yours is " + ", ".join(players)) if players else ""
        out.append(f"<h3>{badge(t.title())} rival{esc(who)}</h3>")
        for stage, label in RIVAL_STAGES:
            team = rv.get(f"RIVAL_TEAM_{stage}_{t}")
            if not team:
                continue
            items = team.get("Items", "")
            out.append(team_table(badge, label, team["mons"], f"items: {esc(items)}" if items else ""))
    return "".join(out)


# ---------------------------------------------------------------------------------------------
# src/gym_challenge.c: Oak, the famous and townsfolk challengers
# ---------------------------------------------------------------------------------------------
def gym_challenge_data():
    src = (ROOT / "src/gym_challenge.c").read_text()
    data = D.load()
    oak = []
    m = re.search(r"sOakFinaleTeam\[\] =\s*\{(.*?)\n\};", src, re.S)
    for sp, item, nature, moves in re.findall(r"\{SPECIES_(\w+),\s*ITEM_(\w+),\s*NATURE_(\w+),\s*\w+,\s*\{([^}]*)\}\}", m.group(1)):
        mv = [data["moves"].get(x, x) for x in re.findall(r"MOVE_\w+", moves)]
        oak.append((pretty(sp), "", data["items"].get("ITEM_" + item, item.title()), mv, nature.title()))
    folk = {}
    for name, body in re.findall(r"static const u16 s(\w+)Team\[\] = \{([^}]*)\};", src):
        folk[name] = [pretty(s) for s in re.findall(r"SPECIES_(\w+)", body) if s != "NONE"]
    extras = {n: [pretty(s) for s in re.findall(r"SPECIES_(\w+)", b) if s != "NONE"]
              for n, b in re.findall(r"static const u16 s(\w+)Extras\[\] = \{([^}]*)\};", src)}
    full_restores = len(re.findall(r"items\[\d\] = ITEM_FULL_RESTORE", src))
    return oak, folk, extras, full_restores


def gym_section(badge):
    oak, folk, extras, _ = gym_challenge_data()
    out = ["<h3>Professor Oak: the battle for your gym</h3>"
           "<p>After the Hall of Fame. His team is five levels above your team's average (never below 65), with "
           "perfect IVs, full EVs, the held items and natures below, and the smartest AI.</p>",
           team_table(badge, "OAK", [(sp, "avg+5", it, mv) for sp, _, it, mv, _ in oak],
                      "natures: " + ", ".join(f"{sp} {n}" for sp, _, _, _, n in oak))]
    out.append("<h3>Gym challengers</h3><p>Every challenger's team averages your team's level. Most are random trainers "
               "with 4-6 fully evolved Pokémon from the first three generations (base stats 420 or more). "
               "Every seventh challenger in a streak, and one in six otherwise, is famous: a Kanto Gym Leader, an "
               "Elite Four member, your rival (his Champion team), Giovanni or Oak, with the story team above (or "
               "his Champion team) brought up to your level and evolved where the level calls for it. "
               "Leaders with fewer than four Pokémon add:</p><ul class='small'>"
               + "".join(f"<li><b>{n}</b>: {', '.join(e)}</li>" for n, e in extras.items() if e)
               + f"<li><b>Oak</b> (as a challenger): {', '.join(folk.get('Oak', []))}</li></ul>"
               "<p>One challenger in twelve is someone from town, at your average level:</p>")
    names = {"Mom": "Mom", "Nurse": "Nurse Joy", "Bill": "Bill", "Daisy": "Daisy", "Fuji": "Mr. Fuji"}
    for k, label in names.items():
        if k in folk:
            out.append(team_table(badge, label, [(sp, "avg", "", ()) for sp in folk[k]]))
    out.append("<h3>Your mentor (the end of a mastery streak)</h3><p>The last battle of a streak that masters a type "
               "is your mentor with their full Gym Leader Convention team (below), three levels above your average.</p>")
    return "".join(out)


# ---------------------------------------------------------------------------------------------
# The convention and the tournament
# ---------------------------------------------------------------------------------------------
ROLE = {"L": "Gym Leader", "E": "Elite Four", "C": "Champion"}


def convention_section(badge):
    out = [f"<p>Gym Leaders are level {CONV_LEVELS['L'][0]} (their last Pokémon {CONV_LEVELS['L'][-1]}), the Elite Four "
           f"{CONV_LEVELS['E'][0]} ({CONV_LEVELS['E'][-1]}), Champions {CONV_LEVELS['C'][0]}-{CONV_LEVELS['C'][-1]}. "
           "Perfect IVs, natures to suit their moves. In the tournament they're the same teams at level 100.</p>"]
    for hall in CR.HALLS:
        out.append(f"<h3>{badge(hall.title()) if hall != 'CHAMPIONS' else 'Champions'} hall</h3>")
        for key, name, role, *_ in CR.ROSTER.get(hall, []):
            team = CT.TEAMS.get(key, [])
            mons = []
            for slot, line in enumerate(team):
                sp, item, moves = parse_showdown(line)
                mons.append((sp, CONV_LEVELS[role][min(slot, 5)], item, moves))
            out.append(team_table(badge, esc(name.title()), mons, ROLE.get(role, "")))
    return "".join(out)


def tournament_section(badge):
    out = ["<p>The tournament's 128 are drawn from the convention (above, at level 100) and these: the rivals, "
           "villains, Frontier Brains, professors and other strong trainers of every region. All level 100.</p>"]
    region = None
    for key in GR.ORDER:
        t = GR.ROSTER[key]
        if t["region"] != region:
            region = t["region"]
            out.append(f"<h3>{esc(region.title())}</h3>")
        mons = []
        for line in t["team"]:
            sp, item, moves = parse_showdown(line)
            mons.append((sp, 100, item, moves))
        out.append(team_table(badge, esc(t["name"].title()), mons, esc(f"{t['title'].title()}, {t['games']}")))
    b = GR.BILL
    out.append("<h3>After the title</h3>")
    out.append(team_table(badge, "Bill", [(sp, 100, it, mv) for sp, it, mv in map(parse_showdown, b["team"])],
                          "the one who put the tournament on (his rematch in the simulator: the same team)"))
    return "".join(out)


def build(badge, player_for_rival):
    """The chapter's body"""
    return ("<p>Every major opponent's team, for planning yours: their Pokémon (with types), levels, held items "
            "and moves. The tables come from the game's data.</p>"
            "<h2 id='teams-story'>The story</h2>" + story_section(badge)
            + "<h2 class='newpage' id='teams-rival'>Your rival</h2>" + rival_section(badge, player_for_rival)
            + "<h2 class='newpage' id='teams-gym'>Oak and your gym's challengers</h2>" + gym_section(badge)
            + "<h2 class='newpage' id='teams-convention'>The Gym Leader Convention</h2>" + convention_section(badge)
            + "<h2 class='newpage' id='teams-tournament'>The tournament</h2>" + tournament_section(badge))


TEAM_CSS = """
.team { page-break-inside: avoid; margin: 4px 0 8px; }
.tname { font-family: 'DejaVu Sans', sans-serif; font-weight: 700; font-size: 9.5pt; margin-bottom: 1px; }
.tsub { font-weight: 400; color: #666; font-size: 8pt; }
table.teamt { font-size: 7.8pt; margin: 2px 0 4px; }
table.teamt td, table.teamt th { padding: 2px 4px; }
"""
