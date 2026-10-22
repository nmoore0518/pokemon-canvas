#!/usr/bin/env python3
"""The post-game training trainers: people in houses around Kanto (and a hall of them in Pallet
Town) who battle again and again, so a new team (after a type switch) can be trained up quickly.

Usage: python3 tools/gym_hack/build_training_houses.py

They appear after the first Hall of Fame (FLAG_HIDE_POSTGAME_GOSSIPERS), ask before every battle,
and have no type rule. Their teams lean on Chansey, Blissey and Audino (lots of EXP; given plain
attacking moves so they don't stall with Soft-Boiled) with a mix of types around them, and they
are rich trainer classes (Lady, Gentleman, Beauty) so they pay well. Levels rise through the towns
in story order; the Pallet hall is the top tier.

Edit TRAINERS below and re-run. Writes:
  src/data/trainers_frlg.party        the TRAINER_TRAINING_* blocks (replaced at the end of the file)
  include/constants/opponents_frlg.h  their ids (after TRAINER_GYM_CHALLENGER) and TRAINERS_COUNT_FRLG
  data/scripts/training_houses.inc    their scripts and texts
  data/maps/<map>/map.json            their objects (any earlier TrainingHouse_ ones replaced)
  data/maps/PalletTown_TrainingHall_Frlg/  the hall (the Saffron dojo's layout)
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PARTY = ROOT / "src/data/trainers_frlg.party"
OPPONENTS = ROOT / "include/constants/opponents_frlg.h"
SCRIPTS = ROOT / "data/scripts/training_houses.inc"
HALL = "PalletTown_TrainingHall_Frlg"
FIRST_ID = 625            # right after TRAINER_GYM_CHALLENGER (624)

# class: (party Class, Pic, gender, music, overworld sprite)
CLASSES = {
    "Gentleman": ("Gentleman Frlg", "Gentleman Frlg", "Male", "Serious", "OBJ_EVENT_GFX_GENTLEMAN_FRLG"),
    "Lady": ("Lady Frlg", "Lady Frlg", "Female", "Cute", "OBJ_EVENT_GFX_WOMAN_2_FRLG"),
    "Beauty": ("Beauty Frlg", "Beauty Frlg", "Female", "Cute", "OBJ_EVENT_GFX_BEAUTY_FRLG"),
}

# The EXP-givers' moves (no healing moves, so battles stay quick)
MOVES = {
    "Chansey": ["Pound", "Double Slap", "Growl", "Tail Whip"],
    "Blissey": ["Pound", "Double-Edge", "Growl", "Tail Whip"],
    "Audino": ["Pound", "Double Slap", "Disarming Voice", "Take Down"],
}

FACE = {"left": "MOVEMENT_TYPE_FACE_LEFT", "right": "MOVEMENT_TYPE_FACE_RIGHT",
        "up": "MOVEMENT_TYPE_FACE_UP", "down": "MOVEMENT_TYPE_FACE_DOWN"}

# (name, class, map, (x, y), facing, line, team [(species, level)])
TRAINERS = [
    ("MORRIS", "Gentleman", "ViridianCity_House_Frlg", (9, 4), "left",
     "My CHANSEY love a good spar.\nThey're worth plenty of EXP, too.",
     [("Chansey", 20), ("Staryu", 18), ("Gloom", 18), ("Pidgeotto", 18), ("Audino", 22)]),
    ("CLAIRE", "Lady", "PewterCity_House2_Frlg", (9, 4), "left",
     "Training a new team? I'd be\ndelighted to help.",
     [("Chansey", 26), ("Onix", 24), ("Clefairy", 24), ("Magnemite", 24), ("Audino", 27)]),
    ("MARINA", "Beauty", "CeruleanCity_House3_Frlg", (9, 4), "left",
     "Every CHAMPION starts over\nsometime. Let's battle!",
     [("Chansey", 30), ("Seadra", 28), ("Butterfree", 28), ("Kadabra", 29), ("Audino", 31)]),
    ("EDMUND", "Gentleman", "CeruleanCity_House4_Frlg", (9, 4), "left",
     "I keep my team in shape by\nbattling young trainers.",
     [("Chansey", 33), ("Electabuzz", 30), ("Ponyta", 30), ("Weepinbell", 31), ("Audino", 32)]),
    ("VIVIAN", "Lady", "VermilionCity_House1_Frlg", (9, 4), "left",
     "The sea air makes my POKéMON\nfeisty. Care to try them?",
     [("Chansey", 35), ("Raichu", 33), ("Dewgong", 33), ("Scyther", 34), ("Audino", 36)]),
    ("SERENA", "Beauty", "VermilionCity_House2_Frlg", (9, 4), "left",
     "A battle a day keeps a team\nsharp. Shall we?",
     [("Chansey", 38), ("Arcanine", 35), ("Graveler", 35), ("Slowbro", 36), ("Audino", 37)]),
    ("WINSTON", "Gentleman", "LavenderTown_House2_Frlg", (9, 4), "left",
     "A quiet town needs a lively\nbattle now and then.",
     [("Chansey", 40), ("Haunter", 38), ("Hypno", 38), ("Nidoqueen", 39), ("Audino", 42)]),
    ("ROSALIND", "Lady", "CeladonCity_Hotel_Frlg", (11, 3), "down",
     "I'm staying in CELADON for the\nshopping, and the battles.",
     [("Chansey", 44), ("Vileplume", 42), ("Ninetales", 42), ("Starmie", 43), ("Audino", 45)]),
    ("PERCIVAL", "Gentleman", "CeladonCity_Hotel_Frlg", (16, 5), "left",
     "The hotel lets me battle in the\nlobby. Splendid, isn't it?",
     [("Blissey", 47), ("Porygon2", 44), ("Jynx", 44), ("Rhydon", 45), ("Audino", 46)]),
    ("CELESTE", "Beauty", "SaffronCity_House_Frlg", (9, 4), "left",
     "SAFFRON trainers are the best\nin KANTO. Shall I prove it?",
     [("Blissey", 50), ("Alakazam", 47), ("Machamp", 47), ("Magneton", 48), ("Audino", 49)]),
    ("GENEVIEVE", "Lady", "SaffronCity_House_Frlg", (1, 3), "right",
     "My sister and I take turns\ntraining visitors.",
     [("Blissey", 52), ("Gyarados", 49), ("Mr. Mime", 49), ("Steelix", 50), ("Audino", 51)]),
    ("OPHELIA", "Beauty", "FuchsiaCity_House1_Frlg", (9, 4), "left",
     "I came for the SAFARI ZONE and\nstayed for the battles!",
     [("Blissey", 55), ("Venomoth", 52), ("Kangaskhan", 52), ("Lapras", 53), ("Audino", 54)]),
    ("AUGUSTUS", "Gentleman", "FuchsiaCity_House3_Frlg", (9, 4), "left",
     "Ho ho! A CHAMPION in my house?\nLet's see your new team!",
     [("Blissey", 57), ("Tauros", 54), ("Exeggutor", 54), ("Scizor", 55), ("Audino", 56)]),
    ("BEATRIX", "Lady", "CinnabarIsland_PokemonLab_Lounge_Frlg", (12, 5), "left",
     "The LAB lets me raise my team\nhere. Want to battle them?",
     [("Blissey", 60), ("Arcanine", 57), ("Omastar", 57), ("Kabutops", 57), ("Electrode", 58), ("Audino", 61)]),
    # The Pallet Training Hall (the dojo layout): the sides, then the middle, then the master at the top
    ("HAROLD", "Gentleman", HALL, (1, 10), "right",
     "Welcome to the PALLET TRAINING\nHALL. I'm the first test!",
     [("Blissey", 66), ("Dragonite", 63), ("Gengar", 63), ("Snorlax", 64), ("Audino", 65)]),
    ("PENELOPE", "Lady", HALL, (11, 10), "left",
     "Only CHAMPIONS train here.\nShow me what you've got!",
     [("Blissey", 69), ("Lapras", 66), ("Tyranitar", 66), ("Espeon", 67), ("Audino", 68)]),
    ("ISABELLA", "Beauty", HALL, (1, 8), "right",
     "Strong opponents mean lots of\nEXP. Ready?",
     [("Blissey", 72), ("Salamence", 69), ("Metagross", 69), ("Milotic", 70), ("Audino", 71)]),
    ("REGINALD", "Gentleman", HALL, (11, 8), "left",
     "I raised all three of OAK's\nfavorite POKéMON. Come!",
     [("Blissey", 75), ("Charizard", 72), ("Venusaur", 72), ("Blastoise", 72), ("Audino", 74)]),
    ("ARABELLA", "Lady", HALL, (6, 7), "down",
     "Few make it this far. Let's\nmake it worth your while.",
     [("Blissey", 78), ("Aerodactyl", 75), ("Machamp", 75), ("Kingdra", 76), ("Audino", 77)]),
    ("MATILDA", "Lady", HALL, (6, 4), "down",
     "I'm the HALL's master. My CHANSEY\nfamily has trained CHAMPIONS!",
     [("Blissey", 82), ("Chansey", 80), ("Snorlax", 78), ("Dragonite", 78), ("Audino", 80), ("Blissey", 80)]),
]

HALL_WARPS = [(5, 14), (6, 14), (7, 14)]
HALL_ATTENDANT = (4, 13)
PALLET_HALL_WARP_ID = 8     # the hall's door in build_pallet_town.py's WARPS


def const(name):
    return f"TRAINER_TRAINING_{name}"


def label(name):
    return "TrainingHouse_EventScript_" + name.title()


def text_label(name, what):
    return f"TrainingHouse_Text_{name.title()}{what}"


def ivs(level):
    v = 0 if level < 40 else 10 if level < 62 else 20
    return f"IVs: {v} HP / {v} Atk / {v} Def / {v} SpA / {v} SpD / {v} Spe"


def write_party():
    text = PARTY.read_text()
    text = re.sub(r"\n=== TRAINER_TRAINING_\w+ ===\n.*?(?=\n=== |\Z)", "", text, flags=re.S).rstrip("\n") + "\n"
    out = []
    for name, cls, *_rest, team in TRAINERS:
        c, pic, gender, music, _ = CLASSES[cls]
        out.append(f"\n=== {const(name)} ===\nName: {name}\nClass: {c}\nPic: {pic}\nGender: {gender}\n"
                   f"Music: {music}\nDouble Battle: No\nAI: Check Bad Move\n")
        for species, level in team:
            mon = f"\n{species}\nLevel: {level}\n{ivs(level)}\n"
            for mv in MOVES.get(species, []):
                mon += f"- {mv}\n"
            out.append(mon)
    PARTY.write_text(text + "".join(out))


def write_opponents():
    text = OPPONENTS.read_text()
    text = re.sub(r"#define TRAINER_TRAINING_\w+ +\d+\n", "", text)
    anchor = re.search(r"#define TRAINER_GYM_CHALLENGER +624[^\n]*\n", text)
    lines = "".join(f"#define {const(n):<43}{FIRST_ID + i}\n" for i, (n, *_r) in enumerate(TRAINERS))
    text = text[:anchor.end()] + lines + text[anchor.end():]
    text = re.sub(r"(#define TRAINERS_COUNT_FRLG +)\d+", lambda m: m.group(1) + str(FIRST_ID + len(TRAINERS)), text)
    OPPONENTS.write_text(text)


def write_scripts():
    s = ["@ Generated by tools/gym_hack/build_training_houses.py: the post-game training trainers.",
         "@ They battle as often as the player likes (their trainer flag is cleared first), with no type rule.",
         ""]
    for name, *_r in TRAINERS:
        s += [f"{label(name)}::",
              "\tlock",
              "\tfaceplayer",
              f"\tmsgbox {text_label(name, 'Intro')}, MSGBOX_YESNO",
              "\tgoto_if_eq VAR_RESULT, NO, TrainingHouse_EventScript_Declined",
              f"\tcleartrainerflag {const(name)}",
              f"\ttrainerbattle_no_intro {const(name)}, TrainingHouse_Text_Defeat",
              "\tmsgbox TrainingHouse_Text_After",
              "\trelease",
              "\tend",
              ""]
    s += ["TrainingHouse_EventScript_Declined::",
          "\tmsgbox TrainingHouse_Text_Declined",
          "\trelease",
          "\tend",
          "",
          "TrainingHouse_EventScript_HallAttendant::",
          "\tlock",
          "\tfaceplayer",
          "\tmsgbox TrainingHouse_Text_RestTeam, MSGBOX_YESNO",
          "\tgoto_if_eq VAR_RESULT, NO, TrainingHouse_EventScript_AttendantBye",
          "\tclosemessage",
          "\tfadescreen FADE_TO_BLACK",
          "\tplayfanfare MUS_RG_HEAL",
          "\tspecial HealPlayerParty",
          "\twaitfanfare",
          "\tfadescreen FADE_FROM_BLACK",
          "\tmsgbox TrainingHouse_Text_TeamRested",
          "\trelease",
          "\tend",
          "",
          "TrainingHouse_EventScript_AttendantBye::",
          "\tmsgbox TrainingHouse_Text_AttendantBye",
          "\trelease",
          "\tend",
          "",
          "TrainingHouse_EventScript_HallStatue::",
          "\tmsgbox TrainingHouse_Text_HallStatue, MSGBOX_SIGN",
          "\tend",
          "",
          "TrainingHouse_EventScript_HallScrollLeft::",
          "\tmsgbox TrainingHouse_Text_HallScrollLeft, MSGBOX_SIGN",
          "\tend",
          "",
          "TrainingHouse_EventScript_HallScrollRight::",
          "\tmsgbox TrainingHouse_Text_HallScrollRight, MSGBOX_SIGN",
          "\tend",
          ""]
    for name, _cls, _m, _p, _f, line, _t in TRAINERS:
        first, second = line.split("\n")
        s += [f"{text_label(name, 'Intro')}::",
              f'\t.string "{first}\\n"',
              f'\t.string "{second}\\p"',
              '\t.string "Want to battle?$"',
              ""]
    for lab, lines in [
        ("Defeat", ["Wonderful! Your team grows", "stronger with every battle."]),
        ("After", ["Come back any time you want", "to battle again!"]),
        ("Declined", ["Whenever you're ready, then."]),
        ("RestTeam", ["Welcome to the PALLET TRAINING", "HALL. Shall I rest your team?"]),
        ("TeamRested", ["Your POKéMON are good as new.", "Train hard!"]),
        ("AttendantBye", ["Train hard!"]),
        ("HallStatue", ["PALLET TRAINING HALL"]),
        ("HallScrollLeft", ["“Every battle is a lesson.”"]),
        ("HallScrollRight", ["“Rest well, then battle again.”"]),
    ]:
        s.append(f"TrainingHouse_Text_{lab}::")
        for i, ln in enumerate(lines):
            end = "$" if i == len(lines) - 1 else "\\n" if i == 0 else "\\l"
            s.append(f'\t.string "{ln}{end}"')
        s.append("")
    SCRIPTS.write_text("\n".join(s))


def trainer_obj(name, cls, pos, facing, elevation):
    return {"type": "object", "graphics_id": CLASSES[cls][4], "x": pos[0], "y": pos[1], "elevation": elevation,
            "movement_type": FACE[facing], "movement_range_x": 0, "movement_range_y": 0,
            "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0", "script": label(name),
            "flag": "FLAG_HIDE_POSTGAME_GOSSIPERS"}


def write_town_objects():
    maps = {}
    for name, cls, mp, pos, facing, *_r in TRAINERS:
        if mp != HALL:
            maps.setdefault(mp, []).append((name, cls, pos, facing))
    for mp, people in maps.items():
        path = ROOT / f"data/maps/{mp}/map.json"
        m = json.loads(path.read_text())
        objs = [o for o in m["object_events"] if not o["script"].startswith("TrainingHouse_")]
        elev = next((o["elevation"] for o in objs if o.get("elevation")), 3)
        objs += [trainer_obj(n, c, p, f, elev) for n, c, p, f in people]
        m["object_events"] = objs
        path.write_text(json.dumps(m, indent=2) + "\n")


def write_hall():
    d = ROOT / f"data/maps/{HALL}"
    d.mkdir(exist_ok=True)
    m = {
        "id": "MAP_PALLET_TOWN_TRAINING_HALL", "name": HALL, "layout": "LAYOUT_SAFFRON_CITY_DOJO",
        "music": "MUS_RG_PEWTER", "region": "REGION_KANTO", "region_map_section": "MAPSEC_PALLET_TOWN",
        "requires_flash": False, "weather": "WEATHER_NONE", "map_type": "MAP_TYPE_INDOOR",
        "allow_cycling": False, "allow_escaping": False, "allow_running": False, "show_map_name": False,
        "floor_number": 0, "battle_scene": "MAP_BATTLE_SCENE_NORMAL", "connections": None,
        "object_events": [trainer_obj(n, c, p, f, 3) for n, c, mp, p, f, *_r in TRAINERS if mp == HALL] + [{
            "type": "object", "graphics_id": "OBJ_EVENT_GFX_NURSE_FRLG", "x": HALL_ATTENDANT[0], "y": HALL_ATTENDANT[1],
            "elevation": 3, "movement_type": "MOVEMENT_TYPE_FACE_RIGHT", "movement_range_x": 0, "movement_range_y": 0,
            "trainer_type": "TRAINER_TYPE_NONE", "trainer_sight_or_berry_tree_id": "0",
            "script": "TrainingHouse_EventScript_HallAttendant", "flag": "0"}],
        "warp_events": [{"x": x, "y": y, "elevation": 3, "dest_map": "MAP_PALLET_TOWN",
                         "dest_warp_id": str(PALLET_HALL_WARP_ID)} for x, y in HALL_WARPS],
        "coord_events": [],
        "bg_events": [
            {"type": "sign", "x": 3, "y": 13, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
             "script": "TrainingHouse_EventScript_HallStatue"},
            {"type": "sign", "x": 9, "y": 13, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
             "script": "TrainingHouse_EventScript_HallStatue"},
            {"type": "sign", "x": 5, "y": 2, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
             "script": "TrainingHouse_EventScript_HallScrollLeft"},
            {"type": "sign", "x": 7, "y": 2, "elevation": 0, "player_facing_dir": "BG_EVENT_PLAYER_FACING_ANY",
             "script": "TrainingHouse_EventScript_HallScrollRight"},
        ],
    }
    (d / "map.json").write_text(json.dumps(m, indent=2) + "\n")
    (d / "scripts.inc").write_text(f"{HALL}_MapScripts::\n\t.byte 0\n")
    groups_path = ROOT / "data/maps/map_groups.json"
    groups = json.loads(groups_path.read_text())
    grp = groups["gMapGroup_IndoorPallet_Frlg"]
    if HALL not in grp:
        grp.append(HALL)
        groups_path.write_text(json.dumps(groups, indent=2) + "\n")
    es = ROOT / "data/event_scripts.s"
    text = es.read_text()
    inc = f'\t.include "data/maps/{HALL}/scripts.inc"\n'
    if inc not in text:
        anchor = '\t.include "data/maps/PalletTown_Gym_Frlg/scripts.inc"\n'
        text = text.replace(anchor, anchor + inc)
    inc2 = '\t.include "data/scripts/training_houses.inc"\n'
    if inc2 not in text:
        anchor = '\t.include "data/scripts/hall_of_fame_frlg.inc"\n'
        assert anchor in text, "hall_of_fame_frlg.inc include not found"
        text = text.replace(anchor, anchor + inc2)
    es.write_text(text)


def main():
    write_party()
    write_opponents()
    write_scripts()
    write_town_objects()
    write_hall()
    for name, cls, mp, pos, _f, _l, team in TRAINERS:
        c = CLASSES[cls]
        print(f"{name:10} {cls:9} {mp:40} Lv {min(l for _, l in team)}-{max(l for _, l in team)}")


if __name__ == "__main__":
    main()
