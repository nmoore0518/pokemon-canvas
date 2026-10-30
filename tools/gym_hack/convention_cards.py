"""The Gym Leader Convention's title cards (Gym Trainer hack): who each trainer is in the Pokémon
world, shown on a card before their battle (src/title_card.c). Read by build_convention.py.

CARDS: roster key -> (title, where they're from). Keep each line within 26 characters (the card's
text box); the build checks.
"""

RB, GS, RS, DP, BW, B2, XY, SM, SW, SV = ("RED/BLUE", "GOLD/SILVER", "RUBY/SAPPHIRE", "DIAMOND/PEARL",
                                          "BLACK/WHITE", "BLACK 2/WHITE 2", "X/Y", "SUN/MOON",
                                          "SWORD/SHIELD", "SCARLET/VIOLET")


def leader(town):
    return f"{town} GYM LEADER"


CARDS = {
    # Normal
    "whitney": (leader("GOLDENROD"), f"JOHTO · {GS}"),
    "norman": (leader("PETALBURG"), f"HOENN · {RS}"),
    "cheren": (leader("ASPERTIA"), f"UNOVA · {B2}"),
    "lenora": (leader("NACRENE"), f"UNOVA · {BW}"),
    "ilima": ("NORMAL TRIAL CAPTAIN", f"ALOLA · {SM}"),
    "larry": (leader("MEDALI"), f"PALDEA · {SV}"),
    # Fire
    "blaine": (leader("CINNABAR"), f"KANTO · {RB}"),
    "flannery": (leader("LAVARIDGE"), f"HOENN · {RS}"),
    "chili": (leader("STRIATON"), f"UNOVA · {BW}"),
    "kiawe": ("FIRE TRIAL CAPTAIN", f"ALOLA · {SM}"),
    "kabu": (leader("MOTOSTOKE"), f"GALAR · {SW}"),
    "flint": ("ELITE FOUR", f"SINNOH · {DP}"),
    "malva": ("ELITE FOUR", f"KALOS · {XY}"),
    # Water
    "misty": (leader("CERULEAN"), f"KANTO · {RB}"),
    "juan": (leader("SOOTOPOLIS"), "HOENN · EMERALD"),
    "wake": (leader("PASTORIA"), f"SINNOH · {DP}"),
    "cress": (leader("STRIATON"), f"UNOVA · {BW}"),
    "marlon": (leader("HUMILAU"), f"UNOVA · {B2}"),
    "lana": ("WATER TRIAL CAPTAIN", f"ALOLA · {SM}"),
    "nessa": (leader("HULBURY"), f"GALAR · {SW}"),
    "kofu": (leader("CASCARRAFA"), f"PALDEA · {SV}"),
    "siebold": ("ELITE FOUR", f"KALOS · {XY}"),
    # Grass
    "erika": (leader("CELADON"), f"KANTO · {RB}"),
    "gardenia": (leader("ETERNA"), f"SINNOH · {DP}"),
    "cilan": (leader("STRIATON"), f"UNOVA · {BW}"),
    "ramos": (leader("COUMARINE"), f"KALOS · {XY}"),
    "mallow": ("GRASS TRIAL CAPTAIN", f"ALOLA · {SM}"),
    "milo": (leader("TURFFIELD"), f"GALAR · {SW}"),
    "brassius": (leader("ARTAZON"), f"PALDEA · {SV}"),
    # Electric
    "surge": (leader("VERMILION"), f"KANTO · {RB}"),
    "wattson": (leader("MAUVILLE"), f"HOENN · {RS}"),
    "volkner": (leader("SUNYSHORE"), f"SINNOH · {DP}"),
    "elesa": (leader("NIMBASA"), f"UNOVA · {BW}"),
    "clemont": (leader("LUMIOSE"), f"KALOS · {XY}"),
    "sophocles": ("ELECTRIC TRIAL CAPTAIN", f"ALOLA · {SM}"),
    "iono": (leader("LEVINCIA"), f"PALDEA · {SV}"),
    # Ice
    "pryce": (leader("MAHOGANY"), f"JOHTO · {GS}"),
    "candice": (leader("SNOWPOINT"), f"SINNOH · {DP}"),
    "brycen": (leader("ICIRRUS"), f"UNOVA · {BW}"),
    "wulfric": (leader("SNOWBELLE"), f"KALOS · {XY}"),
    "melony": (leader("CIRCHESTER"), "GALAR · SHIELD"),
    "grusha": (leader("GLASEADO"), f"PALDEA · {SV}"),
    "lorelei": ("ELITE FOUR", f"KANTO · {RB}"),
    "glacia": ("ELITE FOUR", f"HOENN · {RS}"),
    # Fighting
    "chuck": (leader("CIANWOOD"), f"JOHTO · {GS}"),
    "brawly": (leader("DEWFORD"), f"HOENN · {RS}"),
    "maylene": (leader("VEILSTONE"), f"SINNOH · {DP}"),
    "korrina": (leader("SHALOUR"), f"KALOS · {XY}"),
    "hala": ("MELEMELE KAHUNA", f"ALOLA · {SM}"),
    "bea": (leader("STOW-ON-SIDE"), "GALAR · SWORD"),
    "bruno": ("ELITE FOUR", f"KANTO · {RB}"),
    "marshal": ("ELITE FOUR", f"UNOVA · {BW}"),
    # Poison
    "janine": (leader("FUCHSIA"), f"KANTO · {GS}"),
    "roxie": (leader("VIRBANK"), f"UNOVA · {B2}"),
    "koga": ("ELITE FOUR", f"JOHTO · {GS}"),
    # Ground
    "archer": (leader("VIRIDIAN"), "KANTO · HEARTGOLD/SOULSILVER"),
    "clay": (leader("DRIFTVEIL"), f"UNOVA · {BW}"),
    "hapu": ("PONI KAHUNA", f"ALOLA · {SM}"),
    "bertha": ("ELITE FOUR", f"SINNOH · {DP}"),
    "rika": ("ELITE FOUR", f"PALDEA · {SV}"),
    # Flying
    "falkner": (leader("VIOLET"), f"JOHTO · {GS}"),
    "winona": (leader("FORTREE"), f"HOENN · {RS}"),
    "skyla": (leader("MISTRALTON"), f"UNOVA · {BW}"),
    "kahili": ("ELITE FOUR", f"ALOLA · {SM}"),
    "larry_e4": ("ELITE FOUR", f"PALDEA · {SV}"),
    # Psychic
    "sabrina": (leader("SAFFRON"), f"KANTO · {RB}"),
    "tate_liza": (leader("MOSSDEEP"), f"HOENN · {RS}"),
    "olympia": (leader("ANISTAR"), f"KALOS · {XY}"),
    "tulip": (leader("ALFORNADA"), f"PALDEA · {SV}"),
    "will": ("ELITE FOUR", f"JOHTO · {GS}"),
    "lucian": ("ELITE FOUR", f"SINNOH · {DP}"),
    "caitlin": ("ELITE FOUR", f"UNOVA · {BW}"),
    # Bug
    "bugsy": (leader("AZALEA"), f"JOHTO · {GS}"),
    "burgh": (leader("CASTELIA"), f"UNOVA · {BW}"),
    "viola": (leader("SANTALUNE"), f"KALOS · {XY}"),
    "katy": (leader("CORTONDO"), f"PALDEA · {SV}"),
    "aaron": ("ELITE FOUR", f"SINNOH · {DP}"),
    # Rock
    "brock": (leader("PEWTER"), f"KANTO · {RB}"),
    "roxanne": (leader("RUSTBORO"), f"HOENN · {RS}"),
    "roark": (leader("OREBURGH"), f"SINNOH · {DP}"),
    "grant": (leader("CYLLAGE"), f"KALOS · {XY}"),
    "olivia": ("AKALA KAHUNA", f"ALOLA · {SM}"),
    "gordie": (leader("CIRCHESTER"), "GALAR · SWORD"),
    # Ghost
    "morty": (leader("ECRUTEAK"), f"JOHTO · {GS}"),
    "fantina": (leader("HEARTHOME"), f"SINNOH · {DP}"),
    "acerola": ("GHOST TRIAL CAPTAIN", f"ALOLA · {SM}"),
    "allister": (leader("STOW-ON-SIDE"), "GALAR · SHIELD"),
    "ryme": (leader("MONTENEVERA"), f"PALDEA · {SV}"),
    "agatha": ("ELITE FOUR", f"KANTO · {RB}"),
    "phoebe": ("ELITE FOUR", f"HOENN · {RS}"),
    "shauntal": ("ELITE FOUR", f"UNOVA · {BW}"),
    # Dragon
    "clair": (leader("BLACKTHORN"), f"JOHTO · {GS}"),
    "drayden": (leader("OPELUCID"), f"UNOVA · {BW}"),
    "iris_l": (leader("OPELUCID"), "UNOVA · WHITE"),
    "raihan": (leader("HAMMERLOCKE"), f"GALAR · {SW}"),
    "lance": ("ELITE FOUR", f"KANTO · {RB}"),
    "drake": ("ELITE FOUR", f"HOENN · {RS}"),
    "drasna": ("ELITE FOUR", f"KALOS · {XY}"),
    "hassel": ("ELITE FOUR", f"PALDEA · {SV}"),
    # Dark
    "piers": (leader("SPIKEMUTH"), f"GALAR · {SW}"),
    "marnie": (leader("SPIKEMUTH"), f"GALAR · {SW}"),
    "nanu": ("ULA'ULA KAHUNA", f"ALOLA · {SM}"),
    "karen": ("ELITE FOUR", f"JOHTO · {GS}"),
    "sidney": ("ELITE FOUR", f"HOENN · {RS}"),
    "grimsley": ("ELITE FOUR", f"UNOVA · {BW}"),
    # Steel
    "jasmine": (leader("OLIVINE"), f"JOHTO · {GS}"),
    "byron": (leader("CANALAVE"), f"SINNOH · {DP}"),
    "molayne": ("ELITE FOUR", "ALOLA · ULTRA SUN/MOON"),
    "wikstrom": ("ELITE FOUR", f"KALOS · {XY}"),
    "poppy": ("ELITE FOUR", f"PALDEA · {SV}"),
    # Fairy
    "valerie": (leader("LAVERRE"), f"KALOS · {XY}"),
    "mina": ("FAIRY TRIAL CAPTAIN", f"ALOLA · {SM}"),
    "opal": (leader("BALLONLEA"), f"GALAR · {SW}"),
    "bede": (leader("BALLONLEA"), f"GALAR · {SW}"),
    # Champions
    "blue": ("CHAMPION · YOUR RIVAL", f"KANTO · {RB}"),
    "red": ("THE LEGEND OF MT. SILVER", f"KANTO · {GS}"),
    "lance_c": ("CHAMPION", f"JOHTO · {GS}"),
    "steven": ("CHAMPION", f"HOENN · {RS}"),
    "wallace": ("CHAMPION", "HOENN · EMERALD"),
    "cynthia": ("CHAMPION", f"SINNOH · {DP}"),
    "alder": ("CHAMPION", f"UNOVA · {BW}"),
    "iris_c": ("CHAMPION", f"UNOVA · {B2}"),
    "diantha": ("CHAMPION", f"KALOS · {XY}"),
    "kukui": ("LEAGUE FOUNDER", f"ALOLA · {SM}"),
    "leon": ("UNBEATEN CHAMPION", f"GALAR · {SW}"),
    "geeta": ("TOP CHAMPION", f"PALDEA · {SV}"),
    "nemona": ("CHAMPION-RANKED RIVAL", f"PALDEA · {SV}"),
}


def main():
    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).parent))
    from convention_roster import all_trainers
    root = Path(__file__).resolve().parents[2]
    rows, errors = [], []
    for idx, hall, key, *_ in all_trainers():
        if key not in CARDS:
            errors.append(f"{key}: no card")
            continue
        title, where = CARDS[key]
        region, games = where.split(" · ")
        for line in (title, region + "  ·  " + games):
            if len(line) > 34:
                errors.append(f"{key}: too long: {line}")
        rows.append(f'    [{idx}] = {{COMPOUND_STRING("{title}"), COMPOUND_STRING("{region}"), COMPOUND_STRING("{games}")}},')
    if errors:
        print("\n".join(errors))
        sys.exit(1)
    (root / "src/data/gym_convention_cards.h").write_text(
        "// Generated by tools/gym_hack/convention_cards.py: each convention trainer's title card\n"
        "// (title; region; games)\n"
        "static const u8 *const sConventionCards[CONVENTION_TRAINER_COUNT][3] =\n{\n" + "\n".join(rows) + "\n};\n")
    print(f"convention cards: {len(rows)}")


if __name__ == "__main__":
    main()
