#!/usr/bin/env python3
"""Pokémon Canvas: the dialogue scripts, as two Word files to edit and hand back.

    python3 tools/gym_hack/dialogue/build_dialogue_docs.py
        -> build/dialogue/Pokemon Canvas - Dialogue 1 - Story and named characters.docx
           build/dialogue/Pokemon Canvas - Dialogue 2 - Townspeople, trainers and signs.docx

File 1: the story's scenes and every named character (Mom, Oak, the rival, Giovanni and Team Rocket,
the Gym Leaders and Elite Four, the gym hack's mentors, guests and challengers...), in the order
they're met. File 2: everyone else (townspeople, trainers, signs, shared scripts).

Every line carries an ID (D0001.2 = text D0001, its 2nd box). dialogue_ids.json keeps IDs stable
from one printout to the next; build/dialogue/dialogue_index.json says where each one lives in the
code (label or file:line) so edits made in the documents can be put back."""
import json, re, subprocess, sys
from collections import defaultdict, OrderedDict
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = ROOT / "build/dialogue"
sys.path.insert(0, str(HERE))
import extract_dialogue as ex  # noqa: E402

# ---------------------------------------------------------------------------------------------
# Story order: Kanto's map sections as the player meets them, then Johto (after the League)
# ---------------------------------------------------------------------------------------------
STORY = """PALLET_TOWN ROUTE_1 VIRIDIAN_CITY ROUTE_22 ROUTE_2 VIRIDIAN_FOREST PEWTER_CITY ROUTE_3 MT_MOON ROUTE_4
CERULEAN_CITY ROUTE_24 ROUTE_25 ROUTE_5 UNDERGROUND_PATH ROUTE_6 VERMILION_CITY S_S_ANNE ROUTE_11 DIGLETTS_CAVE
LEADER_ACADEMY ROUTE_9 ROUTE_10 ROCK_TUNNEL POWER_PLANT LAVENDER_TOWN ROUTE_8 UNDERGROUND_PATH_2 ROUTE_7
CELADON_CITY ROCKET_HIDEOUT POKEMON_TOWER SAFFRON_CITY SILPH_CO ROUTE_12 ROUTE_13 ROUTE_14 ROUTE_15 ROUTE_16
ROUTE_17 ROUTE_18 FUCHSIA_CITY KANTO_SAFARI_ZONE ROUTE_19 ROUTE_20 SEAFOAM_ISLANDS CINNABAR_ISLAND
POKEMON_MANSION ROUTE_21 ROUTE_23 KANTO_VICTORY_ROAD INDIGO_PLATEAU POKEMON_LEAGUE CERULEAN_CAVE
OLIVINE_CITY OLIVINE_LIGHTHOUSE ROUTE_39 ROUTE_38 ECRUTEAK_CITY BURNED_TOWER TIN_TOWER ROUTE_37 ROUTE_36
NATIONAL_PARK ROUTE_35 GOLDENROD_CITY ROUTE_34 ILEX_FOREST AZALEA_TOWN ROUTE_40 SPECIAL_AREA""".split()
STORY = {"MAPSEC_" + k: i for i, k in enumerate(STORY)}
JOHTO_FROM = STORY["MAPSEC_OLIVINE_CITY"]

AREA_NAMES = {"MAPSEC_S_S_ANNE": "S.S. Anne", "MAPSEC_KANTO_SAFARI_ZONE": "Safari Zone and Exotic Safari",
              "MAPSEC_KANTO_VICTORY_ROAD": "Victory Road", "MAPSEC_UNDERGROUND_PATH": "Underground Path (Routes 5-6)",
              "MAPSEC_UNDERGROUND_PATH_2": "Underground Path (Routes 7-8)", "MAPSEC_SPECIAL_AREA": "Link rooms",
              "MAPSEC_MT_MOON": "Mt. Moon", "MAPSEC_POKEMON_LEAGUE": "Pokémon League",
              "MAPSEC_POKEMON_TOWER": "Pokémon Tower", "MAPSEC_POKEMON_MANSION": "Pokémon Mansion",
              "MAPSEC_DIGLETTS_CAVE": "Diglett's Cave", "MAPSEC_SILPH_CO": "Silph Co."}

# shared script files: where they go (None = left out), and their heading
SHARED = OrderedDict([
    ("data/text/new_game_intro_frlg.inc", (1, "The opening: Prof. Oak's welcome")),
    ("data/scripts/dad_story.inc", (1, "Dad's story: Nugget Bridge, Silph Co., the basement, Viridian Gym")),
    ("data/text/dad_story.inc", (1, "Dad's story: the basement notes and Dad's Notebook")),
    ("data/scripts/outfits.inc", (1, "Outfits: the wardrobe, Dad's Rocket uniform, the gym uniforms")),
    ("data/scripts/gym_hack.inc", (1, "Your gym: choosing a type, the gym's basics")),
    ("data/scripts/gym_candidate.inc", (1, "Your gym: candidacy, approvals and recruits")),
    ("data/scripts/gym_challenge.inc", (1, "Your gym: challengers")),
    ("data/scripts/gym_convention.inc", (1, "Your gym: the Gym Leader Convention")),
    ("data/scripts/gym_finale.inc", (1, "Your gym: the finale")),
    ("data/scripts/gym_legend.inc", (1, "Your gym: your legendary")),
    ("data/scripts/gym_switch.inc", (1, "Your gym: changing your gym's type")),
    ("data/scripts/gym_decor.inc", (2, "Your gym: decorating (the gym PC)")),
    ("data/scripts/leader_academy.inc", (1, "Leader Academy (scripts)")),
    ("data/scripts/leader_academy_text.inc", (1, "Leader Academy (lessons and lines)")),
    ("data/scripts/gauntlet.inc", (1, "The Gauntlet")),
    ("data/scripts/johto.inc", (1, "Johto")),
    ("data/text/pokedex_rating.inc", (1, "Prof. Oak's Pokédex ratings")),
    ("data/scripts/pokedex_rating.inc", (1, "Prof. Oak's Pokédex ratings (scripts)")),
    ("data/scripts/event_mons.inc", (2, "Event Pokémon")),
    ("data/scripts/exotic_safari.inc", (2, "Exotic Safari (shared scripts)")),
    ("data/scripts/stat_items.inc", (2, "Training items")),
    ("data/scripts/incense.inc", (2, "Incense")),
    ("data/scripts/move_extractor.inc", (2, "Move Extractor")),
    ("data/scripts/palm_pilot.inc", (2, "Palm Pilot")),
    ("data/scripts/aide.inc", (2, "Prof. Oak's aides")),
    ("data/scripts/move_tutors_frlg.inc", (2, "Move tutors")),
    ("data/scripts/pkmn_center_nurse_frlg.inc", (2, "Pokémon Center nurse")),
    ("data/text/day_care_frlg.inc", (2, "Day Care")),
    ("data/scripts/safari_zone.inc", (2, "Safari Zone (shared)")),
    ("data/scripts/pokemon_league.inc", (2, "Pokémon League (shared)")),
    ("data/scripts/pokemon_mansion.inc", (2, "Pokémon Mansion (shared)")),
    ("data/scripts/static_pokemon.inc", (2, "Wild legendaries and static Pokémon")),
    ("data/text/trainers_frlg.inc", (2, "Trainers (shared)")),
])

# C tables: heading and which file
CODE = OrderedDict([
    ("src/battle_controller_oak_old_man.c", (1, "Prof. Oak's first battle lessons (in the code)")),
    ("src/strings.c", (1, "Other lines in the code")),
    ("src/battle_message.c", (1, "Battle messages a character speaks")),
    ("src/gym_candidate.c", (1, "Your gym: mentors, guest Leaders and recruits (in the code)")),
    ("src/gym_challenge.c", (1, "Your gym: challengers (in the code)")),
    ("src/leader_academy.c", (1, "Leader Academy (in the code)")),
    ("src/gym_convention.c", (1, "Gym Leader Convention (in the code)")),
    ("src/data/gym_convention.h", (1, "Gym Leader Convention: the visiting Leaders")),
    ("src/data/gauntlet.h", (1, "The Gauntlet: famous trainers")),
    ("src/archive.c", (2, "Archive (in the code)")),
    ("src/event_mons.c", (2, "Event Pokémon (in the code)")),
    ("src/gym_decor.c", (2, "Gym decorating (in the code)")),
    ("src/stat_items.c", (2, "Training items (in the code)")),
    ("src/team_builder.c", (2, "Team builder (in the code)")),
    ("src/item_upgrades.c", (2, "Item upgrades (in the code)")),
    ("src/move_extractor.c", (2, "Move Extractor (in the code)")),
])
OWNER_TITLES = {
    "gGymCandidateLeaders": "Gym Leaders: letting you hold their gym (candidacy)",
    "sMentors": "Mentors: the Leader or Elite Four member who takes an interest in your type",
    "sMentorPeople": "Mentors: their story, question and gifts",
    "sEliteFour": "Elite Four approvals",
    "gGymRecruits": "Trainers who ask to join your gym",
    "sFamous": "Famous challengers at your gym",
    "sTownsfolk": "Townsfolk who challenge your gym",
    "sTrainerIntros": "Random challengers: before battle", "sTrainerDefeats": "Random challengers: when you beat them",
    "sTrainerAfters": "Random challengers: after the battle", "sTrainerWins": "Random challengers: if they win",
    "sGuestIntros": "Guest Leaders: before battle", "sGuestDefeats": "Guest Leaders: when you beat them",
    "sGuestAfters": "Guest Leaders: after the battle", "sGuestWins": "Guest Leaders: if they win",
    "sPracticeTiers": "Practice trainers",
    "sConventionTrainers": "Visiting Leaders", "sGauntletTrainers": "Famous trainers",
}

PER_PERSON = {"sMentorPeople", "sFamous", "sTownsfolk", "gGymCandidateLeaders", "gGymRecruits", "sEliteFour"}
ROLE_FIX = {"yes": "If you answer YES", "no": "If you answer NO", "coming around": "Coming around: giving their approval"}
TYPE_NAMES = {"TYPE_" + t.upper(): t for t in "Normal Fighting Flying Poison Ground Rock Bug Ghost Steel Fire Water Grass Electric Psychic Ice Dragon Dark Fairy".split()}

# ---------------------------------------------------------------------------------------------
# Speakers
# ---------------------------------------------------------------------------------------------
ALIASES = {"Oak": "Prof. Oak", "Professor Oak": "Prof. Oak", "Joy": "Nurse Joy", "Nurse": "Nurse Joy",
           "Lt Surge": "Lt. Surge", "Rocket": "Rocket Grunt", "Team Rocket Grunt": "Rocket Grunt",
           "Grunt": "Rocket Grunt", "Rocket M": "Rocket Grunt", "Rocket F": "Rocket Grunt",
           "Sign / Object": "Sign", "Item Ball": "Narration", "Var": "Someone", "Var D": "Someone", "0": "Someone",
           "Gba Kid": "Kid", "Poke Maniac": "Poké Maniac", "Pokedex": "Pokédex", "Blue": "Rival", "Mr Fuji": "Mr. Fuji"}
LEADERS = ["Brock", "Misty", "Lt. Surge", "Erika", "Koga", "Sabrina", "Blaine", "Giovanni",
           "Lorelei", "Bruno", "Agatha", "Lance"]
NAMED_CORE = set(LEADERS) | {"Prof. Oak", "Mom", "Rival", "Bill", "Daisy", "Mr. Fuji", "Copycat", "Mr. Psychic",
                             "Captain", "President", "Warden", "Rocket Grunt", "Player"}
GENERIC = {"Man", "Girl", "Guard", "A", "Select", "Aide", "Secretary", "League Rules", "Simulation",
           "Battle Simulator", "Winning Trainers", "Pewter Pokémon Gym Leader", "Rooftop Square", "Narration",
           "Sign", "Someone", "Nurse Joy", "Clerk", "Challenger", "Practice trainer"}


def species_names():
    names = set()
    for f in (ROOT / "src/data/pokemon/species_info").glob("*.h"):
        for m in re.finditer(r'\.speciesName\s*=\s*_\("([^"]+)"\)', f.read_text(errors="ignore")):
            names.add(m[1].title())
    return names


SPECIES = species_names()


def norm_speaker(s):
    if not s:
        return s
    s = s.strip()
    s = ALIASES.get(s, s)
    s = re.sub(r"\s+[MF](\s+Water)?$", "", s)
    s = re.sub(r"^Conv\s+", "", s)
    s = re.sub(r"^Leader\s+", "", s)
    s = re.sub(r"^(Elite Four|Champion|Boss)\s+", "", s)
    if re.match(r"^Rival\b", s):
        s = "Rival"
    if s in ("Rocket", "Team Rocket"):
        s = "Rocket Grunt"
    return ALIASES.get(s, s)


def is_named(s):
    if not s or s in GENERIC or s in SPECIES or s.startswith("Area "):
        return False
    return True


# ---------------------------------------------------------------------------------------------
# Context lines
# ---------------------------------------------------------------------------------------------
def human_name(sym):
    s = re.sub(r"^(FLAG|VAR|SPECIAL)_", "", sym)
    s = s.replace("MAP_SCENE_", "scene: ").replace("_", " ").lower()
    return s


def human_cond(c):
    m = re.match(r"if (FLAG_\w+|\w+) is (set|not set)$", c)
    if m:
        return f"{'once' if m[2] == 'set' else 'before'} ‘{human_name(m[1])}’"
    m = re.match(r"if (\w+) (=|≠|<|≥|≤|>) (\w+)$", c)
    if m:
        a, op, b = m[1], m[2], m[3]
        if a == "VAR_FACING":
            return f"facing {b.replace('DIR_', '').lower()}"
        if a in ("VAR_RESULT",):
            return f"result {op} {b.lower()}"
        if a.startswith("TYPE_") or b.startswith("TYPE_"):
            return f"type {TYPE_NAMES.get(b, b)}"
        return f"‘{human_name(a)}’ {op} {b.lower() if b.isupper() and not b.isdigit() else b}"
    if c.startswith("after you beat "):
        return "after the battle"
    return c.replace("if ", "", 1) if c.startswith("if ") else c


def when_line(conds):
    real = [c for c in conds if c != "otherwise"]
    if not real:
        return "otherwise" if conds else None
    parts = [human_cond(c) for c in real[-3:]]
    lead = "… " if len(real) > 3 else ""
    return lead + "; ".join(parts) + ("  (otherwise-branch)" if conds and conds[-1] == "otherwise" else "")


def map_title(folder):
    s = re.sub(r"_(Frlg|Johto)$", "", folder)
    s = s.replace("ExoticSafari_", "Exotic Safari_")
    parts = [re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", p) for p in s.split("_")]
    out = " — ".join(parts[:1] + [" ".join(parts[1:])]) if len(parts) > 1 else parts[0]
    out = re.sub(r"\b(B?\d+)\s*F\b", r"\1F", out).replace("Pokemon", "Pokémon").replace("S S Anne", "S.S. Anne")
    return out.replace("Oaks", "Oak's").replace("Players", "Player's").replace("Rivals", "Rival's")


def entry_title(e, names):
    k = e["kind"]
    if k == "person":
        who = norm_speaker(e["who"]) or "Someone"
        if e.get("trainer_name"):
            return f"{e['trainer_name']} (trainer)"
        return who
    if k == "sign":
        return "Sign or object you check"
    if k == "scene":
        w = e["who"] or ""
        m = re.match(r"When you enter, if (\w+) = (\w+)", w)
        if m:
            return f"Scene — when you enter the map (‘{human_name(m[1])}’ = {m[2]})"
        m = re.match(r"Stepping on a spot \(when (\w+) = (\w+)\)", w)
        if m:
            return f"Scene — when you step on a spot (‘{human_name(m[1])}’ = {m[2]})"
        return "Scene — " + (w or "when you enter the map")
    if k == "script":
        lab = e["entry"] or ""
        m = re.match(r"^(\w+?)_EventScript_(\w+)$", lab) or re.match(r"^EventScript_(\w+?)_(\w+)$", lab)
        sp = lambda t: re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", t).replace("_", " ")
        if m:
            return f"{sp(m[1])}: {sp(m[2])}"
        return sp(re.sub(r"^EventScript_", "", lab))
    if k == "unlinked":
        return "Text no script shows directly (shown from the code, or left over from FireRed)"
    return e["entry"] or ""


# ---------------------------------------------------------------------------------------------
def main():
    OUT.mkdir(parents=True, exist_ok=True)
    ex.main()
    data = json.loads((OUT / "dialogue.json").read_text())
    tnames = ex.trainer_names()

    # not dialogue: the intro's name suggestions
    data = [x for x in data if not re.match(r"^(gNameChoice_|gOtherText_NewName)", x["id"])]
    # leftover Japanese text (never shown)
    data = [x for x in data if not any(re.search(r"[\u3040-\u30ff\u4e00-\u9fff]", l["text"]) for l in x["lines"])]
    for x in data:
        if x["id"].startswith(("gOakSpeech_", "gControlsGuide_", "gPikachuIntro_")):
            for l in x["lines"]:
                l["speaker"], l["explicit"] = ("Prof. Oak" if x["id"].startswith("gOakSpeech_") else "Narration"), True

    # stable IDs
    idfile = HERE / "dialogue_ids.json"
    ids = json.loads(idfile.read_text()) if idfile.exists() else {}
    nxt = max([int(v[1:]) for v in ids.values()] + [0]) + 1
    for x in data:
        if x["id"] not in ids:
            ids[x["id"]] = f"D{nxt:04d}"
            nxt += 1

    # the maps' areas
    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())
    folder_sec, folder_ord = {}, {}
    n = 0
    for g in groups["group_order"]:
        for f in groups[g]:
            j = json.loads((ROOT / f"data/maps/{f}/map.json").read_text())
            folder_sec[f] = j.get("region_map_section")
            folder_ord[f] = n
            n += 1

    # speakers: trainers, carry-forward within one script entry
    for x in data:
        if x.get("trainer"):
            cl, nm = tnames.get(x["trainer"], ("", ""))
            full = norm_speaker(ex.nice_speaker(f"{cl} {nm}".strip())) if (cl or nm) else "Trainer"
            if cl.upper() in ("LEADER", "ELITE FOUR", "CHAMPION", "BOSS") or "RIVAL" in cl.upper() or cl.upper() == "PKMN TRAINER" and nm in ("TERRY",):
                full = norm_speaker(ex.nice_speaker(nm)) if "RIVAL" not in cl.upper() else "Rival"
            if cl.upper() == "BOSS" or nm == "GIOVANNI":
                full = "Giovanni"
            x["trainer_name"] = full
    OBJECTISH = {"", "Item Ball", "Cuttable Tree", "Breakable Rock", "Pushable Boulder", "Sign", "Someone"}
    last = {}
    for x in data:
        key = (x["section"], x["entry"])
        # shared scripts: whoever calls them
        if x["kind"] in ("script", "unlinked"):
            people = {norm_speaker(w) for _, w in x.get("callers", []) if w}
            people -= {norm_speaker(o) for o in OBJECTISH}
            if len(people) == 1:
                base = people.pop()
            elif x.get("callers") and not people:
                base = "Narration"
            else:
                base = "Someone"
            for l in x["lines"]:
                if not l["explicit"] and l["speaker"] in ("Narration", "?") and not re.match(r"^\[(PLAYER|RIVAL)\]\s", l["text"]):
                    l["speaker"] = base
        for l in x["lines"]:
            l["speaker"] = norm_speaker(l["speaker"])
            if l["explicit"]:
                last[key] = l["speaker"]
            elif x.get("trainer_name") and x["role"] in ("Before battle", "After losing to you", "Not enough Pokémon for a double battle"):
                l["speaker"] = x["trainer_name"]
            elif l["speaker"] != "Narration" and key in last and x["kind"] in ("scene", "script", "sign", "unlinked"):
                l["speaker"] = last[key]
            elif l["speaker"] in ("Narration", "Someone") and key in last and x["kind"] in ("scene", "script") and not ex.NARRATION_RE.match("[PLAYER] " + l["text"].split(" ", 1)[-1]) and not re.match(r"^\[(PLAYER|RIVAL)\]\s", l["text"]):
                l["speaker"] = last[key]
            elif x["kind"] == "person" and l["speaker"] == "Narration" and not re.match(r"^\[(PLAYER|RIVAL)\]\s", l["text"]):
                l["speaker"] = norm_speaker(x["who"]) or "Someone"
            if re.match(r"^\[(PLAYER|RIVAL)\] (received|obtained|got|put|handed|found|delivered|gave|traded|sent|learned|took)\b", l["text"]):
                l["speaker"] = "Narration"

    # a trainer's lines after their battle text: talking to them afterwards
    seen_battle = set()
    for x in data:
        key = (x["section"], x["entry"])
        if x["role"] in ("After losing to you",):
            seen_battle.add(key)
        elif x["kind"] == "person" and key in seen_battle and not x["role"]:
            x["role"] = "Talking to them after the battle"
    # the opening: Oak asks boy or girl after "tell me about yourself"
    order = {x["id"]: i for i, x in enumerate(data)}
    if "gOakSpeech_Text_AskPlayerGender" in order and "gOakSpeech_Text_TellMeALittleAboutYourself" in order:
        a = data.pop(order["gOakSpeech_Text_AskPlayerGender"])
        j = next(i for i, x in enumerate(data) if x["id"] == "gOakSpeech_Text_TellMeALittleAboutYourself")
        data.insert(j + 1, a)

    # group into entries
    entries = OrderedDict()
    for x in data:
        if x["kind"] == "code":
            key = ("code", x["source"], x["entry"], x["conds"][0] if x["conds"] else None,
                   (x["who"] or "") if x["source"].startswith("src/data/") else (x.get("group") if x["entry"] in PER_PERSON else ""))
        else:
            key = ("map", x["section"], x["entry"], x["kind"])
        entries.setdefault(key, []).append(x)

    def entry_named(xs):
        for x in xs:
            if x["kind"] == "scene" or (x.get("trainer_name") and x["trainer_name"] in NAMED_CORE | {"Giovanni", "Rival"}):
                return True
            if x.get("trainer") and re.search(r"ROCKET|LEADER|ELITE|CHAMPION|RIVAL|GIOVANNI", x["trainer"]):
                return True
            for l in x["lines"]:
                if l["speaker"] in NAMED_CORE or (l["explicit"] and is_named(l["speaker"])):
                    return True
        return False

    docs = {1: [], 2: []}   # each: list of (sort key, area, map, entry title, xs)
    for key, xs in entries.items():
        if key[0] == "map":
            sec = key[1]
            if sec.startswith("data/"):
                if sec not in SHARED:
                    continue
                which, title = SHARED[sec]
                if which is None:
                    continue
                order = (2, list(SHARED).index(sec), 0) if sec != "data/text/new_game_intro_frlg.inc" else (0, 0, 0)
                area, mp = ("Your gym and the hack's systems" if which == 1 else "Shared scripts"), title
                if sec == "data/text/new_game_intro_frlg.inc":
                    area = "Pallet Town"
                docs[which].append((order, area, mp, xs[0], xs))
            else:
                ms = folder_sec.get(sec)
                so = STORY.get(ms, 999)
                which = 1 if entry_named(xs) else 2
                area = AREA_NAMES.get(ms, ex.nice_speaker(ms.replace("MAPSEC_", "").replace("_", " ")) if ms else sec)
                if so >= JOHTO_FROM and ms != "MAPSEC_SPECIAL_AREA":
                    area = "Johto (after the League) — " + area
                docs[which].append(((1, so, folder_ord.get(sec, 0)), area, map_title(sec), xs[0], xs))
        else:
            src = key[1]
            which, title = CODE.get(src, (2, src))
            docs[which].append(((3, list(CODE).index(src) if src in CODE else 99, 0), "Your gym and the hack's systems" if which == 1 else "Shared scripts",
                                title, xs[0], xs))
    # build the document models
    index = {}
    for which in (1, 2):
        items = sorted(enumerate(docs[which]), key=lambda t: (t[1][0], t[0]))
        blocks = []
        cur_area = cur_map = None
        for _, (_, area, mp, head, xs) in items:
            if area != cur_area:
                blocks.append({"t": "h1", "text": area})
                cur_area, cur_map = area, None
            if mp != cur_map:
                blocks.append({"t": "h2", "text": mp})
                cur_map = mp
            if head["kind"] == "code":
                owner = head["entry"]
                ttl = OWNER_TITLES.get(owner, re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", owner or "").strip())
                if head["conds"]:
                    c = head["conds"][0]
                    ttl += f" — if your type is {TYPE_NAMES[c]}" if c in TYPE_NAMES else (f" — #{c}" if c.isdigit() else f" — {c}")
                if head["source"].startswith("src/data/") and head["who"]:
                    ttl = ex.nice_speaker(head["who"])
                elif owner in PER_PERSON:
                    ttl += " — " + (head["lines"][0]["speaker"] if head["lines"] else "")
                blocks.append({"t": "h3", "text": ttl})
            else:
                blocks.append({"t": "h3", "text": entry_title(head, tnames)})
            last_when = None
            callers = [c for c in head.get("callers", []) if not c[0].startswith("data/")]
            if head["kind"] in ("script",) and callers:
                where = []
                for sec_, who_ in callers:
                    t = map_title(sec_) + (f" ({norm_speaker(who_)})" if who_ else "")
                    if t not in where:
                        where.append(t)
                more = f" and {len(where) - 3} more" if len(where) > 3 else ""
                blocks.append({"t": "ctx", "text": "Used in: " + "; ".join(where[:3]) + more})
            for x in xs:
                w = when_line(x["conds"]) if x["kind"] != "code" else None
                role = x["role"]
                if role:
                    role = ROLE_FIX.get(role, role[:1].upper() + role[1:])
                ctx = " · ".join(p for p in [role, ("When: " + w) if w else None] if p)
                if ctx and ctx != last_when:
                    blocks.append({"t": "ctx", "text": ctx})
                    last_when = ctx
                tid = ids[x["id"]]
                index[tid] = {"source": x["source"], "label": x["id"], "doc": which,
                              "boxes": [{"speaker": l["speaker"], "text": l["text"]} for l in x["lines"]]}
                for i, l in enumerate(x["lines"], 1):
                    blocks.append({"t": "line", "id": f"{tid}.{i}" if len(x["lines"]) > 1 else tid,
                                   "speaker": l["speaker"], "text": l["text"]})
        (OUT / f"model_{which}.json").write_text(json.dumps(blocks, ensure_ascii=False))
        print(f"file {which}: {sum(1 for b in blocks if b['t'] == 'line')} boxes, "
              f"{sum(1 for b in blocks if b['t'] == 'h3')} conversations")
    idfile.write_text(json.dumps(ids, indent=0, sort_keys=True))
    (OUT / "dialogue_index.json").write_text(json.dumps(index, ensure_ascii=False, indent=1))
    subprocess.run(["node", str(HERE / "render_docx.js"), str(OUT)], check=True)


if __name__ == "__main__":
    main()
