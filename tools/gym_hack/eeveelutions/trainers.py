#!/usr/bin/env python3
"""The new eeveelutions on story trainers (Pokémon Canvas): one each, on a trainer whose class and
type suit it (about the rate any one species turns up at that stage of the game). Each takes the
place of one Pokémon; its moves are its own level-up moves (any moves listed for the old one go).

    python3 tools/gym_hack/eeveelutions/trainers.py

Random trainers (gym challengers, practice battles) draw them from the same pool as everything else
(src/gym_challenge.c PoolSpecies).
"""
import re
from pathlib import Path

PARTY = Path(__file__).resolve().parents[3] / "src/data/trainers_frlg.party"
PLACES = [   # (trainer, the Pokémon it replaces, the eeveelution)
    ("TRAINER_BEAUTY_LOLA", "Rattata", "Odeon"),                 # a Normal-type team, Route 13
    ("TRAINER_BLACK_BELT_AARON", "Primeape", "Assasseon"),        # Saffron's dojo
    ("TRAINER_BIRD_KEEPER_SEBASTIAN", "Pidgeotto", "Hypereon"),  # Route 13
    ("TRAINER_BIKER_HIDEO", "Grimer", "Poiseon"),
    ("TRAINER_COOLTRAINER_WARREN", "Marowak", "Subterraneon"),    # Viridian Gym
    ("TRAINER_COOLTRAINER_GEORGE", "Sandslash", "Obsideon"),      # Victory Road
    ("TRAINER_LASS_CRISSY", "Parasect", "Lenneon"),              # a Bug-type team, Route 4
    ("TRAINER_CHANNELER_TASHA", "Gastly", "Appariteon"),          # Saffron Gym
    ("TRAINER_SCIENTIST_BRAYDON", "Voltorb", "Automateon"),       # Pokémon Mansion
    ("TRAINER_POKEMANIAC_DAWSON", "Lickitung", "Valyreon"),       # Victory Road: he collects rare ones
]


def main():
    text = PARTY.read_text()
    for trainer, old, new in PLACES:
        start = text.index(f"=== {trainer} ===")
        end = text.find("\n=== ", start + 5)
        end = len(text) if end < 0 else end
        block = text[start:end]
        if re.search(rf"^{new}( @|$)", block, re.M):
            print(f"{trainer}: has {new} already")
            continue
        mons = block.split("\n\n")
        for i, mon in enumerate(mons):
            lines = mon.split("\n")
            if re.match(rf"{old}( @|$)", lines[0]):
                lines[0] = new + lines[0][len(old):]
                mons[i] = "\n".join(l for l in lines if not l.startswith("- "))
                break
        else:
            raise SystemExit(f"{trainer}: no {old}")
        text = text[:start] + "\n\n".join(mons) + text[end:]
        print(f"{trainer}: {old} -> {new}")
    PARTY.write_text(text)


if __name__ == "__main__":
    main()
