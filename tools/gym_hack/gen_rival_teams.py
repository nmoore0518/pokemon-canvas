#!/usr/bin/env python3
"""Generate the rival's type-specialist teams.

Writes:
  src/data/rival_teams_frlg.party      (the teams, in the same text format as trainers_frlg.party)
  include/constants/rival_teams.h      (RIVAL_TEAM_* ids)

This was used to create the first version of the teams. After that, the .party file is the
source of truth: edit it directly. Re-running this script overwrites any hand edits.

Each rival type has six "lines" (evolution families). Slots L0-L4 join the team over the
course of the game in the same pattern as the original rival; ACE is his starter. Each line
lists (level reached, species): the rival uses the most evolved form his level allows.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

ROSTERS = {
    "FIRE": [
        [(0, "Growlithe"), (36, "Arcanine")],
        [(0, "Vulpix"), (36, "Ninetales")],
        [(0, "Numel"), (33, "Camerupt")],
        [(0, "Ponyta"), (40, "Rapidash")],
        [(0, "Magby"), (30, "Magmar")],
        [(0, "Charmander"), (16, "Charmeleon"), (36, "Charizard")],
    ],
    "WATER": [
        [(0, "Psyduck"), (33, "Golduck")],
        [(0, "Poliwag"), (25, "Poliwhirl"), (40, "Politoed")],
        [(0, "Krabby"), (28, "Kingler")],
        [(0, "Horsea"), (32, "Seadra"), (50, "Kingdra")],
        [(0, "Magikarp"), (20, "Gyarados")],
        [(0, "Squirtle"), (16, "Wartortle"), (36, "Blastoise")],
    ],
    "FIGHTING": [
        [(0, "Machop"), (28, "Machoke"), (40, "Machamp")],
        [(0, "Mankey"), (28, "Primeape")],
        [(0, "Makuhita"), (24, "Hariyama")],
        [(0, "Meditite"), (37, "Medicham")],
        [(0, "Tyrogue"), (20, "Hitmonlee")],
        [(0, "Tyrogue"), (20, "Hitmontop")],
    ],
    "PSYCHIC": [
        [(0, "Chimecho")],   # Natu is Flying first in this hack
        [(0, "Drowzee"), (26, "Hypno")],
        [(0, "Spoink"), (32, "Grumpig")],
        [(0, "Abra"), (16, "Kadabra"), (36, "Alakazam")],
        [(0, "Mr. Mime")],
        [(0, "Ralts"), (20, "Kirlia"), (30, "Gardevoir")],
    ],
    "DARK": [
        [(0, "Murkrow")],
        [(0, "Houndour"), (24, "Houndoom")],
        [(0, "Sneasel")],
        [(0, "Sableye")],
        [(0, "Absol")],
        [(0, "Poochyena"), (18, "Mightyena")],
    ],
    "GROUND": [
        [(0, "Diglett"), (26, "Dugtrio")],
        [(0, "Cubone"), (28, "Marowak")],
        [(0, "Phanpy"), (25, "Donphan")],
        [(0, "Trapinch"), (35, "Vibrava"), (45, "Flygon")],
        [(0, "Rhyhorn"), (42, "Rhydon")],
        [(0, "Sandshrew"), (22, "Sandslash")],
    ],
    # Only seven Steel-primary lines exist in Gen 1-3 and two are legendaries, so the
    # Steel rival borrows Magnemite (Electric/Steel).
    "STEEL": [
        [(0, "Aron"), (32, "Lairon"), (42, "Aggron")],
        [(0, "Mawile")],
        [(0, "Skarmory")],
        [(0, "Steelix")],
        [(0, "Magnemite"), (30, "Magneton")],
        [(0, "Beldum"), (20, "Metang"), (45, "Metagross")],
    ],
    # Likewise the Ice rival borrows Lapras (Water/Ice).
    "ICE": [
        [(0, "Swinub"), (33, "Piloswine")],
        [(0, "Snorunt"), (42, "Glalie")],
        [(0, "Smoochum"), (30, "Jynx")],
        [(0, "Delibird")],
        [(0, "Lapras")],
        [(0, "Spheal"), (32, "Sealeo"), (44, "Walrein")],
    ],
    "ELECTRIC": [
        [(0, "Mareep"), (15, "Flaaffy"), (30, "Ampharos")],
        [(0, "Voltorb"), (30, "Electrode")],
        [(0, "Magnemite"), (30, "Magneton")],
        [(0, "Electrike"), (26, "Manectric")],
        [(0, "Elekid"), (30, "Electabuzz")],
        [(0, "Pikachu"), (36, "Raichu")],
    ],
    "POISON": [
        [(0, "Zubat"), (22, "Golbat"), (40, "Crobat")],
        [(0, "Grimer"), (38, "Muk")],
        [(0, "Koffing"), (35, "Weezing")],
        [(0, "Gulpin"), (26, "Swalot")],
        [(0, "Seviper")],
        [(0, "Ekans"), (22, "Arbok")],
    ],
    "GRASS": [
        [(0, "Oddish"), (21, "Gloom"), (36, "Vileplume")],
        [(0, "Bellsprout"), (21, "Weepinbell"), (36, "Victreebel")],
        [(0, "Exeggcute"), (36, "Exeggutor")],
        [(0, "Shroomish"), (23, "Breloom")],
        [(0, "Cacnea"), (32, "Cacturne")],
        [(0, "Bulbasaur"), (16, "Ivysaur"), (32, "Venusaur")],
    ],
    "ROCK": [
        [(0, "Geodude"), (25, "Graveler"), (40, "Golem")],
        [(0, "Onix")],
        [(0, "Omanyte"), (40, "Omastar")],
        [(0, "Kabuto"), (40, "Kabutops")],
        [(0, "Anorith"), (40, "Armaldo")],   # Aerodactyl is Flying first in this hack
        [(0, "Larvitar"), (30, "Pupitar"), (55, "Tyranitar")],
    ],
}

ACE = 5
HEADER_EARLY = "Class: Rival Early Frlg\nPic: Rival Early Frlg"
HEADER_LATE = "Class: Rival Late Frlg\nPic: Rival Late Frlg"
HEADER_CHAMP = "Class: Champion Frlg\nPic: Champion Rival Frlg"

# (stage id, original trainers it replaces, class/pic header, extra header lines,
#  [(line, level, iv), ...] in party order) -- mirrors the original rival's teams.
# Pokémon Canvas: the rival battles after the 4th badge that are doubles (tools/gym_hack/assign_doubles.py)
DOUBLES_STAGES = {"SILPH", "ROUTE22_LATE", "CHAMPION_FIRST", "CHAMPION_REMATCH"}

STAGES = [
    ("LAB", "OAKS_LAB", HEADER_EARLY, "", [(ACE, 5, 0)]),
    ("ROUTE22_EARLY", "ROUTE22_EARLY", HEADER_EARLY, "", [(0, 9, 6), (ACE, 9, 6)]),
    ("CERULEAN", "CERULEAN", HEADER_EARLY, "", [(0, 17, 6), (1, 16, 6), (2, 15, 6), (ACE, 18, 12)]),
    ("SS_ANNE", "SS_ANNE", HEADER_LATE, "", [(0, 19, 6), (2, 16, 6), (1, 18, 6), (ACE, 20, 12)]),
    ("POKEMON_TOWER", "POKEMON_TOWER", HEADER_LATE, "",
     [(0, 25, 12), (3, 23, 12), (4, 22, 12), (1, 20, 12), (ACE, 25, 12)]),
    ("SILPH", "SILPH", HEADER_LATE, "",
     [(0, 37, 12), (3, 38, 12), (4, 35, 12), (1, 35, 12), (ACE, 40, 18)]),
    ("ROUTE22_LATE", "ROUTE22_LATE", HEADER_LATE, "",
     [(0, 47, 18), (2, 45, 18), (3, 45, 18), (4, 45, 18), (1, 47, 18), (ACE, 53, 30)]),
    # (no battle items for the Champion: the playtest found 4 Full Restores made a counter-type
    #  team at full IVs a wall)
    ("CHAMPION_FIRST", "CHAMPION_FIRST", HEADER_CHAMP, "",
     [(0, 59, 31), (1, 57, 31), (2, 59, 31), (3, 59, 31), (4, 61, 31), (ACE, 63, 31)]),
    ("CHAMPION_REMATCH", "CHAMPION_REMATCH", HEADER_CHAMP, "",
     [(0, 72, 31), (1, 73, 31), (2, 72, 31), (3, 73, 31), (4, 73, 31), (ACE, 75, 31)]),
]


def species_at(line, level):
    name = line[0][1]
    for min_level, s in line:
        if level >= min_level:
            name = s
    return name


def main():
    party = [
        "/*",
        "Rival teams for the Gym Trainer hack.",
        "",
        "The rival specializes in the type that counters the player's (see src/gym_hack.c).",
        "Each RIVAL_TEAM_<STAGE>_<TYPE> entry replaces the original rival battle at that stage.",
        "Edit species, levels, held items and moves here just like trainers_frlg.party.",
        "Pokemon without listed moves use their normal level-up moves for their level.",
        "*/",
        "",
    ]
    ids = []
    for stage, _, header, extra, slots in STAGES:
        for rtype, lines in ROSTERS.items():
            tid = f"RIVAL_TEAM_{stage}_{rtype}"
            ids.append(tid)
            party.append(f"=== {tid} ===")
            party.append("Name: TERRY")
            party.append(header)
            party.append("Gender: Male")
            party.append("Music: Serious")
            if extra:
                party.append(extra.rstrip("\n"))
            party.append("Double Battle: " + ("Yes" if stage in DOUBLES_STAGES else "No"))
            party.append("AI: Check Bad Move / Try To Faint / Check Viability")
            if stage.startswith("CHAMPION"):
                party.append("Mugshot: Yellow")
            party.append("")
            for line, level, iv in slots:
                sp = species_at(lines[line], level)
                item = " @ Sitrus Berry" if stage.startswith("CHAMPION") and line == ACE else ""
                party.append(f"{sp}{item}")
                party.append(f"Level: {level}")
                party.append(f"IVs: {iv} HP / {iv} Atk / {iv} Def / {iv} SpA / {iv} SpD / {iv} Spe")
                party.append("")
    (ROOT / "src/data/rival_teams_frlg.party").write_text("\n".join(party))

    hdr = [
        "#ifndef GUARD_CONSTANTS_RIVAL_TEAMS_H",
        "#define GUARD_CONSTANTS_RIVAL_TEAMS_H",
        "",
        "// Ids for the rival's type-specialist teams in src/data/rival_teams_frlg.party.",
        "// Generated by tools/gym_hack/gen_rival_teams.py.",
        "",
        "enum RivalTeamId",
        "{",
    ] + [f"    {t}," for t in ids] + [
        "    RIVAL_TEAMS_COUNT",
        "};",
        "",
        "#endif // GUARD_CONSTANTS_RIVAL_TEAMS_H",
        "",
    ]
    (ROOT / "include/constants/rival_teams.h").write_text("\n".join(hdr))
    print(f"wrote {len(ids)} rival teams")


if __name__ == "__main__":
    main()
