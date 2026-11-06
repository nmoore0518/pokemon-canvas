#!/usr/bin/env python3
"""The Pokémon Tower's graves (Pokémon Canvas): 30 readable gravestones on 2F-4F, and the ghost girl.

    python3 tools/gym_hack/build_tower_graves.py

Each grave (a sign on a gravestone tile) shows its Pokémon's picture, its name and its trainer. Ten of
them are Pokémon that could have evolved and never did; one is DAD's ace (by the player's type; its
trainer "???"). Reading the tenth of the ten brings the ghost girl and her SCYTHER, and the Eviolate
(src/tower_graves.c, data/scripts/tower_graves.inc).

Writes:
- src/data/tower_graves.h: the graves, in order (SPECIES_NONE: DAD's ace)
- data/scripts/tower_graves_signs.inc: each grave's script (its index, then the shared one)
- the three floors' map.json: the graves' signs, and the girl and her SCYTHER (hidden by FLAG_TEMP_1F,
  which each floor's map script sets on the way in)
Re-running is safe.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DAD = None
# (floor, x, y, species, trainer); the unevolved ten are marked in their comments
GRAVES = [
    (2, 9, 2, "CHARMANDER", "TIMMY"),     # (unevolved)
    (2, 13, 2, "KANGASKHAN", "MARGARET"),
    (2, 8, 4, "PERSIAN", "LEON"),
    (2, 7, 7, "PIDGEY", "EMMA"),          # (unevolved)
    (2, 11, 7, "TAUROS", "HANK"),
    (2, 15, 7, "BUTTERFREE", "LILY"),
    (2, 19, 8, "CATERPIE", "SAMMY"),      # (unevolved)
    (2, 5, 10, "SNORLAX", "GUS"),
    (2, 15, 10, "LAPRAS", "MARINA"),
    (2, 4, 14, "RATTATA", "JOEY"),        # (unevolved)
    (2, 16, 14, "ARCANINE", "RUSTY"),
    (2, 11, 17, "CLEFABLE", "DOROTHY"),
    (3, 5, 4, "GROWLITHE", "WALT"),       # (unevolved)
    (3, 8, 6, "NIDOKING", "VINCENT"),
    (3, 14, 6, "DODRIO", "PEARL"),
    (3, 6, 10, "EEVEE", "ANNIE"),         # (unevolved)
    (3, 11, 9, "GOLDUCK", "OSCAR"),
    (3, 16, 10, "DITTO", "FELIX"),
    (3, 19, 8, "VENUSAUR", "IRENE"),
    (3, 13, 13, "MAGIKARP", "OTIS"),      # (unevolved)
    (3, 5, 15, "DEWGONG", "NORA"),
    (3, 12, 17, DAD, "???"),
    (4, 15, 4, "VULPIX", "CLARA"),        # (unevolved)
    (4, 7, 5, "NINETALES", "AUGUST"),
    (4, 12, 7, "MACHAMP", "DUKE"),
    (4, 19, 8, "ABRA", "MILO"),           # (unevolved)
    (4, 8, 10, "RAICHU", "BETTY"),
    (4, 13, 12, "HITMONCHAN", "TOMMY"),
    (4, 5, 13, "ODDISH", "FERN"),         # (unevolved)
    (4, 14, 14, "PIDGEOT", "HAROLD"),
]
GHOSTS = ("OBJ_EVENT_GFX_LITTLE_GIRL_FRLG", "OBJ_EVENT_GFX_SPECIES(SCYTHER)")
HIDE = "FLAG_TEMP_1F"


def label(i):
    return f"PokemonTower_EventScript_Grave{i:02d}"


def header():
    lines = ["// Pokémon Canvas: the Pokémon Tower's graves, in order (tools/gym_hack/build_tower_graves.py).",
             "// SPECIES_NONE: DAD's ace (by the player's type).", "",
             "static const struct TowerGrave sTowerGraves[] =", "{"]
    for floor, x, y, species, trainer in GRAVES:
        sp = "SPECIES_NONE" if species is DAD else f"SPECIES_{species}"
        lines.append(f'    {{{sp}, COMPOUND_STRING("{trainer}")}},   // ({floor}F {x},{y})')
    lines += ["};", ""]
    (ROOT / "src/data/tower_graves.h").write_text("\n".join(lines))


def signs():
    out = ["@ Pokémon Canvas: the Pokémon Tower's graves (tools/gym_hack/build_tower_graves.py)", ""]
    for i in range(len(GRAVES)):
        out += [f"{label(i)}::", f"\tsetvar VAR_0x8004, {i}", "\tgoto TowerGraves_EventScript_Read", ""]
    (ROOT / "data/scripts/tower_graves_signs.inc").write_text("\n".join(out))


def maps():
    for floor in (2, 3, 4):
        path = ROOT / f"data/maps/PokemonTower_{floor}F_Frlg/map.json"
        m = json.loads(path.read_text())
        m["bg_events"] = [b for b in m["bg_events"] if not str(b.get("script", "")).startswith("PokemonTower_EventScript_Grave")]
        for i, (f, x, y, _, _) in enumerate(GRAVES):
            if f == floor:
                m["bg_events"].append({"type": "sign", "x": x, "y": y, "elevation": 0,
                                       "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY", "script": label(i)})
        m["object_events"] = [o for o in m["object_events"] if o.get("graphics_id") not in GHOSTS]
        for gfx in GHOSTS:
            m["object_events"].append({"type": "object", "graphics_id": gfx, "x": 0, "y": 0, "elevation": 3,
                                       "movement_type": "MOVEMENT_TYPE_NONE", "movement_range_x": 0,
                                       "movement_range_y": 0, "trainer_type": "TRAINER_TYPE_NONE",
                                       "trainer_sight_or_berry_tree_id": "0",
                                       "script": "TowerGraves_EventScript_Ghost", "flag": HIDE})
        path.write_text(json.dumps(m, indent=2) + "\n")
        scripts = ROOT / f"data/maps/PokemonTower_{floor}F_Frlg/scripts.inc"
        s = scripts.read_text()
        old = f"PokemonTower_{floor}F_Frlg_MapScripts::\n\t.byte 0\n"
        new = (f"PokemonTower_{floor}F_Frlg_MapScripts::\n"
               "\tmap_script MAP_SCRIPT_ON_TRANSITION, TowerGraves_EventScript_HideGhosts\n\t.byte 0\n")
        if old in s:
            scripts.write_text(s.replace(old, new, 1))
        assert new in scripts.read_text()


def includes():
    ev = ROOT / "data/event_scripts.s"
    text = ev.read_text()
    anchor = '\t.include "data/scripts/move_extractor.inc"\n'
    add = '\t.include "data/scripts/tower_graves.inc"\n\t.include "data/scripts/tower_graves_signs.inc"\n'
    if add not in text:
        assert anchor in text
        ev.write_text(text.replace(anchor, anchor + add, 1))


if __name__ == "__main__":
    assert len(GRAVES) == 30
    header()
    signs()
    maps()
    includes()
