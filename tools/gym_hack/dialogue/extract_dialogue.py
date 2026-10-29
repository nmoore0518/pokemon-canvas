#!/usr/bin/env python3
"""Pokémon Canvas: every line of dialogue the player can see, pulled from the game's scripts and code,
with who says it, where, and when (for the dialogue scripts: build_dialogue_docs.py).

    python3 tools/gym_hack/dialogue/extract_dialogue.py      -> build/dialogue/dialogue.json

Script text (data/maps/*/scripts.inc, data/scripts/*.inc, data/text/*.inc) is walked from each map's
people, signs, trigger squares and map scripts, in script order, following its jumps and noting the
conditions on them. Text written in the C code (the gym hack's) is taken from the hack's own files.
Each piece of text gets an ID (its label, or file:line for C text) so edits find their way back."""
import json, re, sys
from collections import defaultdict, OrderedDict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/gym_hack/guide"))
import build_encounters as be  # noqa: E402  (reachable_maps)

OUT_DIR = ROOT / "build/dialogue"

# ---------------------------------------------------------------------------------------------
# The script files
# ---------------------------------------------------------------------------------------------
LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)::?\s*(?:@.*)?$")
STRING_RE = re.compile(r'^\s*\.string\s+"(.*)"\s*(?:@.*)?$')


def parse_inc(path):
    """-> texts {label: raw}, scripts {label: [(lineno, command line)]}, order of labels"""
    texts, scripts, cur, kind = OrderedDict(), OrderedDict(), None, None
    for n, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
        m = LABEL_RE.match(line)
        if m:
            cur, kind = m[1], None
            continue
        if cur is None:
            continue
        s = STRING_RE.match(line)
        if s:
            if kind is None:
                kind = "text"
                texts[cur] = ""
            if kind == "text":
                texts[cur] += s[1]
            continue
        st = line.split("@")[0].strip()
        if not st or st.startswith("."):
            continue
        if kind is None:
            kind = "script"
            scripts[cur] = []
        if kind == "script":
            scripts[cur].append((n, st))
    return texts, scripts


# Shared script and text files the game uses (the rest are Emerald's, which this game never shows):
# FireRed's, and the gym hack's own
SHARED_USED = re.compile(r"data/(scripts|text)/(aide|day_care_frlg|hall_of_fame_frlg|pkmn_center_nurse_frlg|"
                         r"pokemon_league|route23|safari_zone|pokemon_mansion|static_pokemon|elite_four|"
                         r"silphco_doors|move_tutors_frlg|training_houses|new_game_intro_frlg|pokedex_rating(?<=scripts/pokedex_rating)|"
                         r"ingame_trade_frlg|gym_\w+|leader_academy\w*|exotic_safari\w*|"
                         r"event_mons\w*|gauntlet|incense|johto|move_extractor|palm_pilot|stat_items)\.inc$")


def all_inc_files():
    files = sorted(ROOT.glob("data/maps/*/scripts.inc")) + sorted(ROOT.glob("data/scripts/*.inc")) \
        + sorted(ROOT.glob("data/text/*.inc"))
    return [f for f in files if f.name != "debug.inc"]


# ---------------------------------------------------------------------------------------------
# Names: trainers, object graphics
# ---------------------------------------------------------------------------------------------
def trainer_names():
    classes = {}
    for m in re.finditer(r'\[(TRAINER_CLASS_\w+)\]\s*=\s*\{\s*_\("([^"]*)"\)', (ROOT / "src/battle_main.c").read_text()):
        classes[m[1]] = m[2]
    names = {}
    for f in ("src/data/trainers_frlg.h", "src/data/trainers.h"):
        txt = (ROOT / f).read_text()
        for m in re.finditer(r'\[DIFFICULTY_NORMAL\]\[(TRAINER_\w+)\]\s*=\s*\{(.*?)\n    \},', txt, re.S):
            nm = re.search(r'\.trainerName\s*=\s*_\("([^"]*)"\)', m[2])
            cl = re.search(r'\.trainerClass\s*=\s*(TRAINER_CLASS_\w+)', m[2])
            if m[1] not in names:
                names[m[1]] = (classes.get(cl[1], "") if cl else "", nm[1] if nm else "")
    return names


GFX_NAMES = {
    "MOM": "Mom", "PROF_OAK": "Prof. Oak", "BLUE": "Rival", "GIOVANNI": "Giovanni", "BILL": "Bill",
    "NURSE": "Nurse", "CLERK": "Clerk", "MR_FUJI": "Mr. Fuji", "DAISY": "Daisy",
}


def gfx_role(gfx):
    g = re.sub(r"^OBJ_EVENT_GFX_", "", gfx or "")
    g = re.sub(r"_(FRLG|JOHTO|HGSS)$", "", g)
    for k, v in GFX_NAMES.items():
        if g == k:
            return v
    g = re.sub(r"_\d+$", "", g)
    return g.replace("_", " ").title() if g else ""


# ---------------------------------------------------------------------------------------------
# Text: control codes, speakers
# ---------------------------------------------------------------------------------------------
PLACEHOLDERS = {
    "PLAYER": "[PLAYER]", "RIVAL": "[RIVAL]", "STR_VAR_1": "[var 1]", "STR_VAR_2": "[var 2]",
    "STR_VAR_3": "[var 3]", "KUN": "", "RIGHT_ARROW": "→", "UP_ARROW": "↑", "DOWN_ARROW": "↓",
    "LEFT_ARROW": "←", "PKMN": "PKMN", "POKEBLOCK": "POKéBLOCK", "PAUSE_UNTIL_PRESS": "",
}


def clean(raw):
    """raw .string text -> list of paragraphs (a \\p starts a new box)"""
    t = raw.replace("$", "")
    t = t.replace("\\p", "\x00").replace("\\n", " ").replace("\\l", " ")

    def ph(m):
        k = m[1]
        if k in PLACEHOLDERS:
            return PLACEHOLDERS[k]
        if re.match(r"(MUS|SE)_", k):
            return ""
        if re.match(r"(COLOR|SHADOW|COLOR_HIGHLIGHT_SHADOW|FONT_\w+|PAUSE|PLAY_SE|PLAY_BGM|CLEAR|CLEAR_TO|"
                    r"SKIP|RESET_FONT|ESCAPE|JPN|ENG|EMOJI_\w+|PAUSE_MUSIC|RESUME_MUSIC|MUSIC|FILL_WINDOW|"
                    r"HIGHLIGHT|TEXT_COLORS|WAIT_SE|NAME_END|\w+ \w+|[0-9A-Fa-f ]+)", k):
            return ""
        return "[" + k.lower().replace("_", " ") + "]"
    t = re.sub(r"\{([^}]*)\}", ph, t)
    t = t.replace("\\", "")
    paras = [re.sub(r"\s+", " ", p).strip() for p in t.split("\x00")]
    return [p for p in paras if p]


SPEAKER_RE = re.compile(r"^((?:\[RIVAL\]|\[PLAYER\]|[A-Z][A-Z0-9.'é&\- ]{0,24}?)):\s+(.*)$")


def split_speaker(para):
    m = SPEAKER_RE.match(para)
    if m and not re.match(r"^(NOTE|HINT|TIP|P\.S|PS|WARNING|CAUTION|NOTICE|PS)$", m[1]):
        return m[1], m[2]
    return None, para


def nice_speaker(s):
    if s in ("[RIVAL]", "[PLAYER]"):
        return "Rival" if s == "[RIVAL]" else "Player"
    small = {"OF", "THE", "AND"}
    out = []
    for w in s.split(" "):
        if w in small:
            out.append(w.lower())
        elif re.match(r"^(MR|MRS|MS|DR|PROF|LT)\.$", w):
            out.append(w.title())
        else:
            out.append(w[:1] + w[1:].lower().replace("éé", "éé"))
    r = " ".join(out)
    return r.replace("Pokémon", "Pokémon").replace("PokéMON", "Pokémon").replace("Pokémon", "Pokémon")


NARRATION_RE = re.compile(r"^\[PLAYER\] (received|obtained|got|put|handed|found|used|delivered|"
                          r"gave|traded|sent|learned|returned|took|stored|left|checked|tossed|"
                          r"boarded|placed|sprayed|turned|pressed|read|played|showed|inserted|is|was|"
                          r"has|and|'s)\b")


# ---------------------------------------------------------------------------------------------
# Walking scripts
# ---------------------------------------------------------------------------------------------
COND_JUMPS = re.compile(r"^(goto|call)_if_(\w+)$")


def human_cond(op, args, last_special):
    a = [x.strip() for x in args]
    if op in ("set", "unset"):
        return f"if {a[0]} is {'set' if op == 'set' else 'not set'}"
    if len(a) >= 2:
        var, val = a[0], a[1]
        sym = {"eq": "=", "ne": "≠", "lt": "<", "ge": "≥", "le": "≤", "gt": ">"}.get(op, op)
        if var == "VAR_RESULT":
            if val in ("MALE", "FEMALE") and last_special == "checkplayergender":
                return f"if the player is {'a boy' if val == 'MALE' else 'a girl'}"
            if val in ("YES", "NO") and last_special == "yesno":
                return f"if the player answers {val}"
            if last_special and last_special not in ("checkplayergender", "yesno"):
                return f"if {last_special} {sym} {val}"
        return f"if {var} {sym} {val}"
    return f"if {op} {' '.join(a)}"


class Walker:
    def __init__(self, texts, scripts, label_file):
        self.texts, self.scripts, self.label_file = texts, scripts, label_file
        self.calls, self.current = defaultdict(set), None   # scripts in other files, and who calls them

    def walk(self, entry, allowed, visited, emit, conds=(), depth=0):
        """emits (text label, role, conds) in script order from `entry`"""
        if entry in visited or entry not in self.scripts or depth > 60:
            return
        visited.add(entry)
        last_special = None
        conds = list(conds)
        pending_else = None
        for n, line in self.scripts[entry]:
            parts = line.split(None, 1)
            cmd = parts[0]
            args = [x.strip() for x in parts[1].split(",")] if len(parts) > 1 else []
            if pending_else is not None:
                conds = list(pending_else)
                pending_else = None
            if cmd == "checkplayergender":
                last_special = "checkplayergender"
            elif cmd in ("specialvar", "special") and args:
                last_special = args[-1]
            elif cmd in ("msgbox", "message") and args:
                lab = args[0]
                role = "yesno" if len(args) > 1 and "YESNO" in args[1] else None
                if lab in self.texts:
                    emit(lab, role, conds)
                if role == "yesno":
                    last_special = "yesno"
                continue
            elif cmd.startswith("trainerbattle"):
                tlabs = [x for x in args if x in self.texts]
                roles = ["Before battle", "After losing to you", "Not enough Pokémon for a double battle"]
                if cmd in ("trainerbattle_no_intro",):
                    roles = ["After losing to you"]
                trainer = next((x for x in args if x.startswith("TRAINER_")), None)
                for i, lab in enumerate(tlabs):
                    emit(lab, ("battle", trainer, roles[i] if i < len(roles) else "Battle"), conds)
                for x in args:
                    if x in self.scripts and x not in self.texts:
                        self.follow(x, allowed, visited, emit, conds + [f"after you beat {trainer}" if trainer else "after the battle"], depth)
                continue
            m = COND_JUMPS.match(cmd)
            if m and args:
                target = args[-1]
                c = human_cond(m[2], args[:-1], last_special)
                self.follow(target, allowed, visited, emit, conds + [c], depth)
                if m[1] == "goto":
                    conds = conds + ["otherwise"]
                continue
            if cmd in ("goto", "call") and args:
                self.follow(args[0], allowed, visited, emit, conds, depth)
                if cmd == "goto":
                    return
                continue
            if cmd == "switch" and args:
                last_special = args[0]
                continue
            if cmd == "case" and len(args) >= 2:
                self.follow(args[1], allowed, visited, emit, conds + [f"if {last_special} = {args[0]}"], depth)
                continue
            if cmd in ("end", "return", "releaseall_end"):
                return
            # any other command naming a text (bufferstring, etc.) or a script
            for x in args:
                if x in self.texts and cmd not in ("applymovement",):
                    emit(x, None, conds)

    def follow(self, target, allowed, visited, emit, conds, depth):
        if target in self.scripts and allowed(target):
            self.walk(target, allowed, visited, emit, conds, depth + 1)
        elif target in self.scripts and self.current:
            self.calls[target].add(self.current)


# ---------------------------------------------------------------------------------------------
# C text (the gym hack's own files)
# ---------------------------------------------------------------------------------------------
C_FILES = [
    "src/gym_candidate.c", "src/gym_challenge.c", "src/gym_convention.c", "src/data/gym_convention.h",
    "src/leader_academy.c", "src/archive.c", "src/tournament.c", "src/gauntlet.c", "src/data/gauntlet.h",
    "src/mastery_hall.c", "src/gym_badge.c", "src/gym_decor.c", "src/team_builder.c", "src/stat_items.c",
    "src/event_mons.c", "src/exotic_safari.c", "src/gym_legend.c", "src/johto.c", "src/gym_hack.c",
    "src/move_extractor.c", "src/form_items.c", "src/incense.c", "src/gym_switch.c", "src/item_upgrades.c",
    "src/tournament_board.c", "src/gym_aides.c", "src/gym_finale.c", "src/gym_practice.c",
    "src/data/rival_teams_frlg.h", "src/palm_pilot.c", "src/dex_forms.c",
    "src/battle_controller_oak_old_man.c", "src/strings.c", "src/battle_message.c",
]
# big shared files: only their lines a character speaks
C_ONLY_SPEAKERS = {"src/strings.c": r"^(OAK|GHOST):", "src/battle_message.c": r"^(OAK|GHOST):"}
C_STR = re.compile(r'(?:COMPOUND_STRING|\b_)\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\)')


MEMBER_NOTES = {}
OWNER_SPEAKERS = {
    "sTrainerIntros": "Challenger", "sTrainerDefeats": "Challenger", "sTrainerAfters": "Challenger",
    "sTrainerWins": "Challenger", "sGuestIntros": "Guest Leader", "sGuestDefeats": "Guest Leader",
    "sGuestAfters": "Guest Leader", "sGuestWins": "Guest Leader", "sCandidateIntros": "Academy student",
    "sCandidateDefeats": "Academy student", "sPracticeTiers": "Practice trainer",
}


def struct_string_fields():
    """struct name -> its const u8 * members, in order (for tables written without .field = )"""
    out = {}
    files = list(ROOT.glob("include/*.h")) + [ROOT / f for f in C_FILES if (ROOT / f).exists()]
    for f in files:
        for m in re.finditer(r"\bstruct\s+(\w+)\s*\{(.*?)\};", f.read_text(errors="ignore"), re.S):
            mem = []
            for d in m[2].split(";"):
                d = re.sub(r"//.*", "", d).strip()
                mm = re.match(r"(?:const\s+)?(u8|u16|u32|s8|s16|s32|bool8|bool32|struct\s+\w+|enum\s+\w+|\w+)\s*(\*?)\s*(?:const\s+)?\**\s*(\w+)", d)
                if mm:
                    mem.append((mm[3], mm[1] == "u8" and mm[2] == "*"))
            for cm in re.finditer(r"const u8 \*(\w+);\s*//\s*(.+)", m[2]):
                MEMBER_NOTES[(m[1], cm[1])] = cm[2].strip()
            out.setdefault(m[1], mem)
    return out


OWNER_ROLES = {
    "sTrainerIntros": "Before battle", "sTrainerDefeats": "When you beat them", "sTrainerAfters": "After the battle",
    "sTrainerWins": "If they win", "sGuestIntros": "Before battle", "sGuestDefeats": "When you beat them",
    "sGuestAfters": "After the battle", "sGuestWins": "If they win", "sCandidateIntros": "Before battle",
    "sCandidateDefeats": "When you beat them",
}
ROLE_NAMES = {
    "intro": "Before battle", "defeat": "When you beat them", "after": "Talking to them after the battle",
    "win": "If they win", "offer": "Offering to let you watch their gym", "approve": "Giving their approval",
    "repeatOffer": "Offering again (later visits)", "repeatThanks": "Thanks (later visits)",
    "ownGym": "After you have your own gym", "observe": "Watching you hold their gym",
    "first": "First meeting", "second": "Second meeting", "third": "Their approval", "pitch": "Asking to join your gym",
}


def c_blocks(src):
    """for every position: the innermost { it's in (its offset); skips strings and comments"""
    stack, starts = [], {}
    i, n = 0, len(src)
    events = []
    while i < n:
        c = src[i]
        if c == '"':
            j = i + 1
            while j < n and src[j] != '"':
                j += 2 if src[j] == "\\" else 1
            i = j + 1
            continue
        if src.startswith("//", i):
            i = src.find("\n", i)
            i = n if i < 0 else i
            continue
        if src.startswith("/*", i):
            i = src.find("*/", i) + 2
            continue
        if c == "{":
            stack.append(i)
        elif c == "}" and stack:
            events.append((stack.pop(), i))
        i += 1
    return events


def c_dialogue():
    out = []
    fields = struct_string_fields()
    tnames = trainer_names()
    for f in C_FILES:
        p = ROOT / f
        if not p.exists():
            continue
        src = p.read_text(errors="ignore")
        blocks = sorted(c_blocks(src))
        decls = []
        for m in re.finditer(r"^(?=[a-z])([^;{}=()]*?)(\w+)\s*(?:\[[^\]]*\]\s*)*(?:=|\()", src, re.M):
            sm = re.search(r"\bstruct\s+(\w+)", m[1])
            decls.append((m.start(), sm[1] if sm else None, m[2], src[m.end() - 1] == "("))
        # tables of an unnamed struct: static const struct { ... } sName[] =
        for m in re.finditer(r"^(?:static\s+)?(?:const\s+)?struct\s*\{(.*?)\}\s*(\w+)\s*(?:\[[^\]]*\]\s*)*=", src, re.M | re.S):
            mem = []
            for d in m[1].split(";"):
                d2 = re.sub(r"//.*", "", d).strip()
                mm = re.match(r"(?:const\s+)?(\w+)\s*(\*?)\s*(\w+)", d2)
                if mm:
                    mem.append((mm[3], mm[1] == "u8" and mm[2] == "*"))
            for cm in re.finditer(r"const u8 \*(\w+);\s*//\s*(.+)", m[1]):
                MEMBER_NOTES[("anon:" + m[2], cm[1])] = cm[2].strip()
            fields["anon:" + m[2]] = mem
            decls.append((m.start(2), "anon:" + m[2], m[2], False))
        decls.sort()
        strs = [(m.start(), m.end(), "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', m[1]))) for m in C_STR.finditer(src)]

        def owner_of(pos):
            d = None
            for st, sname, name, isfunc in decls:
                if st > pos:
                    break
                d = (sname, name, isfunc)
            return d or (None, "", False)

        def inner(pos):
            best = None
            for a, b in blocks:
                if a < pos < b and (best is None or a > best[0]):
                    best = (a, b)
            return best
        groups = defaultdict(list)
        for st, en, raw in strs:
            blk = inner(st)
            groups[blk].append((st, en, raw))
        for blk, items in groups.items():
            sname, owner, isfunc = owner_of(items[0][0])
            body = src[blk[0]:blk[1]] if blk else ""
            # the entry's name and index ([TYPE_ROCK] = {...)
            idx = None
            if blk:
                pre = src[max(0, blk[0] - 80):blk[0]]
                im = re.search(r"\[(\w+)\]\s*=\s*$", pre)
                idx = im[1] if im else None
            name = None
            nm = re.search(r'\.(?:name|trainerName)\s*=\s*(?:COMPOUND_STRING|_)\("([^"]*)"\)', body)
            if nm:
                name = nm[1]
            tm = re.search(r"\b(TRAINER_(?!CLASS|PIC|TYPE)\w+)\b", body)
            if not name and tm and tm[1] in tnames:
                name = tnames[tm[1]][1] or tnames[tm[1]][0]
            if not name:
                short = [r for _, _, r in items if re.fullmatch(r"[A-Z][A-Z .'é&]{1,20}", r)]
                if short:
                    name = short[0]
            # positional string members of the struct this block is an entry of
            members = None
            if sname and sname in fields and blk and blk[0] > src.find(owner):
                members = [m for m, isstr in fields[sname] if isstr]
            pos_i = 0
            group_speaker = None
            for st, en, raw in items:
                pre = src[max(0, st - 40):st]
                fm = re.search(r"\.(\w+)\s*=\s*$", pre)
                field = fm[1] if fm else None
                letters = re.sub(r"\{[^}]*\}", "", raw)
                is_dialogue = re.search(r"[a-z]{2}", letters) and (len(letters) >= 18 or "\\p" in raw)
                if not field and members and pos_i < len(members):
                    field = members[pos_i]
                pos_i += 1
                if not is_dialogue:
                    continue
                if f in C_ONLY_SPEAKERS and not re.match(C_ONLY_SPEAKERS[f], raw):
                    continue
                ln = src.count("\n", 0, st) + 1
                sp = SPEAKER_RE.match(clean(raw)[0]) if clean(raw) else None
                if sp:
                    group_speaker = group_speaker or sp[1]
                out.append({"id": f"{f}:{ln}", "file": f, "line": ln, "owner": owner, "index": idx,
                            "field": field, "name": name, "raw": raw, "group": (blk or (0, 0))[0],
                            "note": MEMBER_NOTES.get((sname, field)) if field else None,
                            "default": OWNER_SPEAKERS.get(owner, "Narration" if isfunc else None)})
        # a speaker named anywhere in an entry speaks its unnamed lines too
        by_group = defaultdict(list)
        for o in out:
            if o["file"] == f:
                by_group[o["group"]].append(o)
        for g, os_ in by_group.items():
            sp = None
            for o in os_:
                c = clean(o["raw"])
                m = SPEAKER_RE.match(c[0]) if c else None
                if m:
                    sp = m[1]
                    break
            for o in os_:
                o["group_speaker"] = sp
    return out


# ---------------------------------------------------------------------------------------------
def main():
    groups = json.loads((ROOT / "data/maps/map_groups.json").read_text())
    maps, name_of = {}, {}
    for g in groups["group_order"]:
        for folder in groups[g]:
            j = json.loads((ROOT / f"data/maps/{folder}/map.json").read_text())
            maps[folder] = j
            name_of[j["id"]] = folder
    reach = be.reachable_maps(maps, name_of)
    reach_folders = {name_of[m] for m in reach if m in name_of}

    texts, scripts, label_file = {}, {}, {}
    for f in all_inc_files():
        t, s = parse_inc(f)
        rel = str(f.relative_to(ROOT))
        for k, v in t.items():
            texts.setdefault(k, v)
            label_file.setdefault(k, rel)
        for k, v in s.items():
            scripts.setdefault(k, v)
            label_file.setdefault(k, rel)
    walker = Walker(texts, scripts, label_file)
    tnames = trainer_names()

    used = {}          # text label -> first use
    uses = defaultdict(list)
    events = []        # in order: {map, entry, kind, who, label, role, conds}

    def entries_of(folder):
        j = maps[folder]
        out = []
        p = ROOT / f"data/maps/{folder}/scripts.inc"
        if p.exists():
            _, s = parse_inc(p)
            for lab, cmds in s.items():
                for n, c in cmds:
                    m = re.match(r"map_script(?:_2)?\s+(.*)$", c)
                    if m:
                        a = [x.strip() for x in m[1].split(",")]
                        if len(a) == 2:
                            out.append(("scene", a[1], "When you enter the map" if "TRANSITION" not in a[0] and "LOAD" not in a[0] else None, None))
                        elif len(a) == 3:
                            out.append(("scene", a[2], f"When you enter, if {a[0]} = {a[1]}", None))
                # map script tables (ON_FRAME etc. point to tables of map_script_2)
        for o in j.get("object_events") or []:
            if o.get("script") and o["script"] != "0x0":
                trainer = o.get("trainer_type", "TRAINER_TYPE_NONE") != "TRAINER_TYPE_NONE"
                out.append(("person", o["script"], gfx_role(o.get("graphics_id")), {"gfx": o.get("graphics_id"), "trainer": trainer}))
        for c in j.get("coord_events") or []:
            if c.get("script"):
                out.append(("scene", c["script"], f"Stepping on a spot (when {c.get('var')} = {c.get('var_value')})", None))
        for b in j.get("bg_events") or []:
            if b.get("type") == "sign" and b.get("script"):
                out.append(("sign", b["script"], "Sign / object", None))
        return out

    def allowed_for(entry_file, prefix):
        def ok(target):
            f = label_file.get(target)
            return f == entry_file or target.startswith(prefix)
        return ok

    def walk_entry(section, kind, entry, who, info):
        if entry not in scripts:
            return
        efile = label_file[entry]
        prefix = entry.split("_EventScript_")[0] + "_" if "_EventScript_" in entry else entry
        visited = set()
        walker.current = (section, who if kind == "person" else None, kind)

        def emit(lab, role, conds):
            uses[lab].append((section, entry))
            if lab in used:
                return
            used[lab] = (section, entry)
            events.append({"section": section, "kind": kind, "entry": entry, "who": who, "info": info,
                           "label": lab, "role": role, "conds": list(conds), "file": label_file.get(lab)})
        walker.walk(entry, allowed_for(efile, prefix), visited, emit)

    # every reachable map's people, signs and scenes
    ordered = sorted(reach_folders)
    for folder in ordered:
        for kind, entry, who, info in entries_of(folder):
            walk_entry(folder, kind, entry, who, info)
    # map scripts' frame/transition tables: their map_script_2 entries are inside table labels
    for folder in ordered:
        p = ROOT / f"data/maps/{folder}/scripts.inc"
        if not p.exists():
            continue
        _, s = parse_inc(p)
        for lab, cmds in s.items():
            for n, c in cmds:
                m = re.match(r"map_script_2\s+(\w+)\s*,\s*(\w+)\s*,\s*(\w+)", c)
                if m:
                    walk_entry(folder, "scene", m[3], f"When you enter, if {m[1]} = {m[2]}", None)
    # everything else: scripts nothing reachable calls (run from the code, shared scripts...)
    referenced = set()
    for lab, cmds in scripts.items():
        for n, c in cmds:
            for tok in re.findall(r"\b\w+\b", c):
                if tok in scripts and tok != lab:
                    referenced.add(tok)
    for f in all_inc_files():
        rel = str(f.relative_to(ROOT))
        if rel.startswith("data/maps/") and f.parent.name not in reach_folders:
            continue
        if not rel.startswith("data/maps/") and not SHARED_USED.match(rel):
            continue
        _, s = parse_inc(f)
        sec = f.parent.name if rel.startswith("data/maps/") else rel
        for lab in s:
            if lab not in referenced:
                walk_entry(sec, "script", lab, None, None)
        # and then any script left (reached only from code we can't see)
        for lab in s:
            walk_entry(sec, "script", lab, None, None)
    # texts no script shows (picked up by the code, buffered...) in files the game uses
    for lab, raw in texts.items():
        if lab in used:
            continue
        f = label_file[lab]
        if f.startswith("data/maps/") and Path(f).parent.name not in reach_folders:
            continue
        if not f.startswith("data/maps/") and not SHARED_USED.match(f):
            continue
        sec = Path(f).parent.name if f.startswith("data/maps/") else f
        used[lab] = (sec, None)
        events.append({"section": sec, "kind": "unlinked", "entry": None, "who": None, "info": None,
                       "label": lab, "role": None, "conds": [], "file": f})

    # speakers
    out = []
    for e in events:
        paras = clean(texts[e["label"]])
        lines, speaker = [], None
        default = None
        if isinstance(e["role"], tuple) and e["role"][0] == "battle":
            cl, nm = tnames.get(e["role"][1], ("", ""))
            default = nice_speaker(f"{cl} {nm}".strip()) if (cl or nm) else "Trainer"
        elif e["kind"] == "person" and e["who"]:
            default = e["who"]
        elif e["kind"] == "sign":
            default = "Sign"
        for p in paras:
            s, rest = split_speaker(p)
            if s:
                speaker = nice_speaker(s)
                lines.append({"speaker": speaker, "text": rest, "explicit": True})
            else:
                who = speaker or default
                if NARRATION_RE.match(p) or not who:
                    who = "Narration" if NARRATION_RE.match(p) or e["kind"] in ("scene", "script", "unlinked") else who
                lines.append({"speaker": who or "Narration", "text": p, "explicit": False})
        role = e["role"][2] if isinstance(e["role"], tuple) else ("Yes/No question" if e["role"] == "yesno" else None)
        out.append({"id": e["label"], "source": e["file"], "section": e["section"], "kind": e["kind"],
                    "entry": e["entry"], "who": e["who"], "trainer": e["role"][1] if isinstance(e["role"], tuple) else None,
                    "role": role, "conds": e["conds"], "lines": lines,
                    "gfx": (e["info"] or {}).get("gfx"), "also": len({u for u in uses[e["label"]]}) - 1,
                    "callers": sorted({(c[0], c[1] or "") for c in walker.calls.get(e["entry"], ())})})
    for c in c_dialogue():
        paras = clean(c["raw"])
        lines, speaker = [], None
        default = nice_speaker(c["group_speaker"] or c["name"]) if (c.get("group_speaker") or c["name"]) else c.get("default")
        for p in paras:
            s, rest = split_speaker(p)
            if s:
                speaker = nice_speaker(s)
                lines.append({"speaker": speaker, "text": rest, "explicit": True})
            else:
                lines.append({"speaker": speaker or default or "?", "text": p, "explicit": False})
        out.append({"id": c["id"], "source": c["file"], "section": c["file"], "kind": "code", "entry": c["owner"],
                    "who": c["name"], "trainer": None, "role": (c["note"] or ROLE_NAMES.get(c["field"], c["field"]) or OWNER_ROLES.get(c["owner"])) if True else ROLE_NAMES.get(c["field"], c["field"]), "conds": [c["index"]] if c["index"] else [],
                    "lines": lines, "gfx": None, "also": 0, "group": c["group"]})
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "dialogue.json").write_text(json.dumps(out, indent=1, ensure_ascii=False))
    print(len(out), "texts;", sum(1 for x in out if x["kind"] == "code"), "from the code")


if __name__ == "__main__":
    main()
