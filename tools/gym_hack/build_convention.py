#!/usr/bin/env python3
"""The Gym Leader Convention's trainers (Gym Trainer hack) -> src/data/gym_convention.h.

From tools/gym_hack/convention_roster.py (who, which hall) and convention_teams.py (teams, lines):
each trainer's party (struct TrainerMon, checked against the game's species/move/item names), their
lines (wrapped to the text box) and class. Levels: Gym Leaders 75 (their ace 76), Elite Four 78
(ace 79), Champions 85-88. IVs 31; natures by what the moves are (physical: Adamant, special:
Modest, mostly status: Calm).

    python3 tools/gym_hack/build_convention.py
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from convention_roster import HALLS, all_trainers  # noqa: E402
from convention_teams import LINES, TEAMS  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
WRAP = 32

CLASS = {"L": "TRAINER_CLASS_LEADER_FRLG", "E": "TRAINER_CLASS_ELITE_FOUR_FRLG", "C": "TRAINER_CLASS_CHAMPION_FRLG"}
LEVELS = {"L": [75, 75, 75, 75, 75, 76], "E": [78, 78, 78, 78, 78, 79], "C": [85, 85, 86, 86, 87, 88]}


def const_name(prefix, text):
    t = text.upper().replace("É", "E").replace("&", "AND")
    t = t.replace("'", "").replace(".", "").replace(":", "")
    t = re.sub(r"[^A-Z0-9]+", "_", t).strip("_")
    return f"{prefix}_{t}"


def known(path, prefix):
    return set(re.findall(r"\b(" + prefix + r"_[A-Z0-9_]+)\b", (ROOT / path).read_text()))


def move_categories():
    text = (ROOT / "src/data/moves_info.h").read_text()
    cats = {}
    for m in re.finditer(r"\[(MOVE_[A-Z0-9_]+)\]\s*=\s*\{(.*?)\n    \},", text, re.S):
        c = re.search(r"\.category\s*=\s*DAMAGE_CATEGORY_(\w+)", m.group(2))
        cats[m.group(1)] = c.group(1) if c else "STATUS"
    return cats


def wrap(text):
    """Text box lines: two to a box (\\n), then a new box (\\p)."""
    out = []
    for para in text.split("\n\n"):
        words, lines, line = para.split(), [], ""
        for w in words:
            if line and len(line) + 1 + len(w) > WRAP:
                lines.append(line)
                line = w
            else:
                line = f"{line} {w}" if line else w
        if line:
            lines.append(line)
        s = ""
        for i, ln in enumerate(lines):
            s += ln if i == 0 else ("\\n" if i % 2 == 1 else "\\p") + ln
        out.append(s)
    return "\\p".join(out).replace('"', '\\"')


def main():
    species = known("include/constants/species.h", "SPECIES")
    moves = known("include/constants/moves.h", "MOVE")
    items = known("include/constants/items.h", "ITEM")
    cats = move_categories()
    errors, parties, entries = [], [], []
    trainers = all_trainers()
    for idx, hall, key, name, role, female, pic, ow in trainers:
        team = TEAMS.get(key)
        if not team or len(team) != 6:
            errors.append(f"{key}: team of {len(team or [])}")
            continue
        mons = []
        for slot, line in enumerate(team):
            m = re.match(r"^(.*?)(?:\s*@\s*(.*?))?:\s*(.*)$", line)
            sp, item, mv = m.group(1).strip(), (m.group(2) or "").strip(), [x.strip() for x in m.group(3).split("/")]
            sc = const_name("SPECIES", sp)
            if sc not in species:
                errors.append(f"{key}: species {sp} ({sc})")
            ic = const_name("ITEM", item) if item else "ITEM_NONE"
            if item and ic not in items:
                errors.append(f"{key}: item {item} ({ic})")
            mcs = []
            for x in mv:
                mc = const_name("MOVE", x)
                if mc not in moves:
                    errors.append(f"{key}: move {x} ({mc})")
                mcs.append(mc)
            kinds = [cats.get(c, "STATUS") for c in mcs]
            phys, spec = kinds.count("PHYSICAL"), kinds.count("SPECIAL")
            nature = "NATURE_ADAMANT" if phys > spec else ("NATURE_MODEST" if spec > phys else ("NATURE_CALM" if phys == 0 else "NATURE_HARDY"))
            mons.append(f"    {{\n        .species = {sc},\n        .lvl = {LEVELS[role][slot]},\n        .heldItem = {ic},\n"
                        f"        .moves = {{{', '.join(mcs)}}},\n        .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31),\n"
                        f"        .nature = {nature},\n        .gender = TRAINER_MON_RANDOM_GENDER,\n        .ball = POKEBALL_COUNT,\n"
                        f"        .friendship = MAX_FRIENDSHIP,\n    }},\n")
        parties.append(f"static const struct TrainerMon sConventionParty_{key}[] = {{\n" + "".join(mons) + "};\n\n")
        if key not in LINES:
            errors.append(f"{key}: no lines")
            continue
        intro, defeat, after = (wrap(t) for t in LINES[key])
        nm = "NULL" if key == "blue" else f'COMPOUND_STRING("{name}")'
        entries.append(f"    [{idx}] = {{ // {hall}\n        .name = {nm},\n        .trainerClass = {CLASS[role]},\n"
                       f"        .female = {'TRUE' if female else 'FALSE'},\n        .hall = CONVENTION_HALL_{hall},\n"
                       f"        .party = sConventionParty_{key},\n"
                       f'        .intro = COMPOUND_STRING("{intro}"),\n'
                       f'        .defeat = COMPOUND_STRING("{defeat}"),\n'
                       f'        .after = COMPOUND_STRING("{after}"),\n    }},\n')
    if errors:
        print("\n".join(errors))
        sys.exit(1)
    halls = "".join(f"#define CONVENTION_HALL_{h} {i}\n" for i, h in enumerate(HALLS))
    (ROOT / "include/constants/gym_convention.h").write_text(
        "// Generated by tools/gym_hack/build_convention.py\n#ifndef GUARD_CONSTANTS_GYM_CONVENTION_H\n"
        "#define GUARD_CONSTANTS_GYM_CONVENTION_H\n\n" + halls + f"#define CONVENTION_HALL_COUNT {len(HALLS)}\n"
        f"#define CONVENTION_TRAINER_COUNT {len(trainers)}\n\n#endif // GUARD_CONSTANTS_GYM_CONVENTION_H\n")
    (ROOT / "src/data/gym_convention.h").write_text(
        "// Generated by tools/gym_hack/build_convention.py from tools/gym_hack/convention_roster.py and\n"
        "// convention_teams.py: the Gym Leader Convention's trainers\n\n" + "".join(parties)
        + "static const struct ConventionTrainer sConventionTrainers[CONVENTION_TRAINER_COUNT] = {\n"
        + "".join(entries) + "};\n")
    print(f"convention: {len(trainers)} trainers in {len(HALLS)} halls")


if __name__ == "__main__":
    main()
