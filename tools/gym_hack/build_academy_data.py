#!/usr/bin/env python3
"""The Leader Academy's battle data (Gym Trainer hack) -> src/data/academy.h

  - RENTALS: the Battle Hall's level-100 sets (full IVs, EVs by what the moves are), lent to the
    player 3 at a time and fought by the candidates practicing there;
  - TEACHERS: each instructor's demonstration team (3 rental-style sets that show off the lecture);
  - CANDIDATES: the other regions' leader candidates. Their teams are base forms, evolved and
    levelled to the player at battle time (src/leader_academy.c), with their own moves by level.

    python3 tools/gym_hack/build_academy_data.py
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from build_convention import const_name, known, move_categories  # noqa: E402
from build_gauntlet import party  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]

RENTALS = [
    "Garchomp @ Choice Scarf {Rough Skin}: Earthquake / Outrage / Stone Edge / Fire Fang",
    "Dragonite @ Heavy-Duty Boots {Multiscale}: Dragon Dance / Extreme Speed / Earthquake / Fire Punch",
    "Tyranitar @ Assault Vest {Sand Stream}: Crunch / Stone Edge / Earthquake / Fire Punch",
    "Metagross @ Life Orb: Meteor Mash / Zen Headbutt / Earthquake / Bullet Punch",
    "Salamence @ Life Orb {Intimidate}: Draco Meteor / Fire Blast / Earthquake / Roost",
    "Scizor @ Choice Band {Technician}: Bullet Punch / U-turn / Knock Off / Close Combat",
    "Gengar @ Life Orb: Shadow Ball / Sludge Bomb / Focus Blast / Thunderbolt",
    "Alakazam @ Focus Sash {Magic Guard}: Psychic / Shadow Ball / Focus Blast / Nasty Plot",
    "Starmie @ Life Orb {Natural Cure}: Hydro Pump / Ice Beam / Thunderbolt / Rapid Spin",
    "Gyarados @ Lum Berry {Intimidate}: Dragon Dance / Waterfall / Earthquake / Ice Fang",
    "Azumarill @ Choice Band {Huge Power}: Aqua Jet / Play Rough / Liquidation / Knock Off",
    "Swampert @ Leftovers: Earthquake / Waterfall / Stealth Rock / Toxic",
    "Rotom-Wash @ Leftovers: Volt Switch / Hydro Pump / Will-O-Wisp / Pain Split",
    "Ferrothorn @ Leftovers: Leech Seed / Power Whip / Gyro Ball / Stealth Rock",
    "Skarmory @ Rocky Helmet: Spikes / Roost / Brave Bird / Whirlwind",
    "Blissey @ Heavy-Duty Boots {Natural Cure}: Seismic Toss / Soft-Boiled / Toxic / Thunder Wave",
    "Clefable @ Leftovers {Magic Guard}: Moonblast / Calm Mind / Moonlight / Flamethrower",
    "Toxapex @ Black Sludge {Regenerator}: Scald / Toxic / Recover / Haze",
    "Volcarona @ Heavy-Duty Boots {Flame Body}: Quiver Dance / Fiery Dance / Bug Buzz / Giga Drain",
    "Weavile @ Choice Band {Pressure}: Triple Axel / Knock Off / Ice Shard / Low Kick",
    "Lucario @ Life Orb {Inner Focus}: Swords Dance / Close Combat / Extreme Speed / Meteor Mash",
    "Excadrill @ Leftovers {Mold Breaker}: Rapid Spin / Earthquake / Iron Head / Swords Dance",
    "Breloom @ Focus Sash {Technician}: Spore / Mach Punch / Bullet Seed / Rock Tomb",
    "Heracross @ Choice Scarf {Guts}: Close Combat / Megahorn / Stone Edge / Knock Off",
    "Infernape @ Life Orb {Iron Fist}: Close Combat / Flare Blitz / Mach Punch / U-turn",
    "Jolteon @ Choice Specs {Volt Absorb}: Thunderbolt / Volt Switch / Shadow Ball / Hyper Voice",
    "Vaporeon @ Leftovers {Water Absorb}: Scald / Wish / Protect / Ice Beam",
    "Umbreon @ Leftovers {Synchronize}: Foul Play / Wish / Protect / Toxic",
    "Snorlax @ Leftovers {Thick Fat}: Body Slam / Curse / Rest / Earthquake",
    "Gliscor @ Toxic Orb {Poison Heal}: Earthquake / Knock Off / Toxic / Protect",
    "Hydreigon @ Choice Specs {Levitate}: Draco Meteor / Dark Pulse / Flash Cannon / Fire Blast",
    "Kingdra @ Life Orb {Sniper}: Draco Meteor / Hydro Pump / Ice Beam / Rain Dance",
    "Mamoswine @ Life Orb {Thick Fat}: Earthquake / Icicle Crash / Ice Shard / Knock Off",
    "Chandelure @ Choice Scarf {Flash Fire}: Shadow Ball / Fire Blast / Energy Ball / Trick",
    "Conkeldurr @ Flame Orb {Guts}: Drain Punch / Mach Punch / Knock Off / Ice Punch",
    "Mimikyu @ Life Orb {Disguise}: Swords Dance / Play Rough / Shadow Claw / Shadow Sneak",
    "Corviknight @ Leftovers {Pressure}: U-turn / Brave Bird / Roost / Bulk Up",
    "Toxtricity @ Choice Specs {Punk Rock}: Overdrive / Boomburst / Sludge Bomb / Volt Switch",
    "Kingambit @ Black Glasses {Supreme Overlord}: Sucker Punch / Kowtow Cleave / Iron Head / Swords Dance",
    "Tinkaton @ Leftovers {Mold Breaker}: Gigaton Hammer / Play Rough / Stealth Rock / Encore",
]

# (key, name, lecture subject, battle class, pic, female, team)
TEACHERS = [
    ("larkspur", "LARKSPUR", "SPEED", "TRAINER_PIC_GENTLEMAN_FRLG", False, [
        "Dragonite @ Choice Band {Multiscale}: Extreme Speed / Outrage / Earthquake / Fire Punch",
        "Scizor @ Life Orb {Technician}: Bullet Punch / U-turn / Swords Dance / Knock Off",
        "Weavile @ Focus Sash {Pressure}: Ice Shard / Knock Off / Triple Axel / Low Kick"]),
    ("camellia", "CAMELLIA", "SETUP", "TRAINER_PIC_BEAUTY_FRLG", True, [
        "Volcarona @ Heavy-Duty Boots {Flame Body}: Quiver Dance / Fiery Dance / Bug Buzz / Giga Drain",
        "Gyarados @ Lum Berry {Intimidate}: Dragon Dance / Waterfall / Earthquake / Ice Fang",
        "Cloyster @ Focus Sash {Skill Link}: Shell Smash / Icicle Spear / Rock Blast / Ice Shard"]),
    ("foxglove", "FOXGLOVE", "HAZARDS", "TRAINER_PIC_SCIENTIST_FRLG", False, [
        "Skarmory @ Rocky Helmet: Spikes / Roost / Brave Bird / Whirlwind",
        "Garchomp @ Rocky Helmet {Rough Skin}: Stealth Rock / Earthquake / Dragon Tail / Fire Fang",
        "Excadrill @ Leftovers {Mold Breaker}: Rapid Spin / Earthquake / Iron Head / Swords Dance"]),
    ("verbena", "VERBENA", "PIVOTS", "TRAINER_PIC_COOLTRAINER_M_FRLG", False, [
        "Rotom-Wash @ Leftovers: Volt Switch / Hydro Pump / Will-O-Wisp / Pain Split",
        "Corviknight @ Leftovers {Pressure}: U-turn / Brave Bird / Roost / Bulk Up",
        "Incineroar @ Assault Vest {Intimidate}: Flare Blitz / Knock Off / Parting Shot / Fake Out"]),
    ("marigold", "MARIGOLD", "WEATHER", "TRAINER_PIC_AROMA_LADY_FRLG", True, [
        "Pelipper @ Damp Rock {Drizzle}: Scald / Hurricane / U-turn / Roost",
        "Kingdra @ Life Orb {Swift Swim}: Hydro Pump / Draco Meteor / Ice Beam / Rain Dance",
        "Ferrothorn @ Leftovers: Leech Seed / Power Whip / Gyro Ball / Stealth Rock"]),
    ("hyacinth", "HYACINTH", "READS", "TRAINER_PIC_COOLTRAINER_F_FRLG", True, [
        "Clefable @ Leftovers {Magic Guard}: Moonblast / Calm Mind / Moonlight / Flamethrower",
        "Toxapex @ Black Sludge {Regenerator}: Scald / Toxic / Recover / Haze",
        "Gengar @ Life Orb: Shadow Ball / Sludge Bomb / Focus Blast / Destiny Bond"]),
    ("yarrow", "YARROW", "BUILDS", "TRAINER_PIC_GENTLEMAN_FRLG", False, [
        "Azumarill @ Choice Band {Huge Power}: Aqua Jet / Play Rough / Liquidation / Knock Off",
        "Tyranitar @ Assault Vest {Sand Stream}: Crunch / Stone Edge / Earthquake / Fire Punch",
        "Amoonguss @ Rocky Helmet {Regenerator}: Spore / Giga Drain / Clear Smog / Sludge Bomb"]),
    ("tansy", "TANSY", "TEAMS", "TRAINER_PIC_LASS_FRLG", True, [
        "Gliscor @ Toxic Orb {Poison Heal}: Earthquake / Knock Off / Toxic / Protect",
        "Primarina @ Leftovers {Torrent}: Moonblast / Surf / Calm Mind / Draining Kiss",
        "Heatran @ Leftovers {Flash Fire}: Magma Storm / Earth Power / Taunt / Stealth Rock"]),
]

# (key, name, region, type, skill 0-3, battle pic, female, base species)
SKILLS = ["ROOKIE", "SKILLED", "ADVANCED", "EXPERT"]
CANDIDATES = [
    ("koji", "KOJI", "JOHTO", "FIGHTING", 0, "TRAINER_PIC_BLACK_BELT_FRLG", False,
        ["Machop", "Mankey", "Heracross", "Makuhita", "Meditite", "Tyrogue"]),
    ("maren", "MAREN", "HOENN", "WATER", 1, "TRAINER_PIC_SWIMMER_F_FRLG", True,
        ["Wingull", "Carvanha", "Wailmer", "Corphish", "Spheal", "Azurill"]),
    ("elias", "ELIAS", "SINNOH", "GROUND", 1, "TRAINER_PIC_HIKER_FRLG", False,
        ["Gible", "Hippopotas", "Gligar", "Phanpy", "Trapinch", "Barboach"]),
    ("nadia", "NADIA", "UNOVA", "ELECTRIC", 2, "TRAINER_PIC_COOLTRAINER_F_FRLG", True,
        ["Blitzle", "Tynamo", "Emolga", "Joltik", "Klink", "Rotom"]),
    ("rene", "RENE", "KALOS", "FAIRY", 2, "TRAINER_PIC_BEAUTY_FRLG", True,
        ["Flabebe", "Spritzee", "Swirlix", "Dedenne", "Ralts", "Klefki"]),
    ("keola", "KEOLA", "ALOLA", "FIRE", 0, "TRAINER_PIC_CAMPER_FRLG", False,
        ["Litten", "Salandit", "Fletchling", "Growlithe", "Turtonator", "Torkoal"]),
    ("briony", "BRIONY", "GALAR", "DRAGON", 3, "TRAINER_PIC_COOLTRAINER_F_FRLG", True,
        ["Dreepy", "Applin", "Jangmo-o", "Deino", "Axew", "Duraludon"]),
    ("teo", "TEO", "PALDEA", "STEEL", 1, "TRAINER_PIC_COOLTRAINER_M_FRLG", False,
        ["Tinkatink", "Orthworm", "Varoom", "Magnemite", "Skarmory", "Bronzor"]),
]


def main():
    species = known("include/constants/species.h", "SPECIES")
    moves = known("include/constants/moves.h", "MOVE")
    items = known("include/constants/items.h", "ITEM")
    cats = move_categories()
    errors = []
    out = ["// Generated by tools/gym_hack/build_academy_data.py: do not edit\n\n"]

    mons, _ = party("rentals", {"team": RENTALS}, species, moves, items, cats, set(), errors)
    out.append("static const struct TrainerMon sAcademyRentals[] = {\n" + "".join(mons) + "};\n\n")
    out.append("#define ACADEMY_RENTAL_COUNT ARRAY_COUNT(sAcademyRentals)\n\n")

    for key, name, subject, pic, female, team in TEACHERS:
        mons, _ = party(key, {"team": team}, species, moves, items, cats, set(), errors)
        out.append(f"static const struct TrainerMon sAcademyTeacherParty_{key}[] = {{\n" + "".join(mons) + "};\n\n")
    out.append("static const struct AcademyTeacher sAcademyTeachers[] = {\n")
    for key, name, subject, pic, female, team in TEACHERS:
        out.append(f"    {{.name = COMPOUND_STRING(\"{name}\"), .pic = {pic}, .female = {str(female).upper()}, "
                   f".party = sAcademyTeacherParty_{key}}},\n")
    out.append("};\n\n")

    out.append("static const struct AcademyCandidate sAcademyCandidates[] = {\n")
    for key, name, region, typ, skill, pic, female, team in CANDIDATES:
        sc = []
        for sp in team:
            c = const_name("SPECIES", sp)
            if c not in species:
                errors.append(f"{key}: species {sp} ({c})")
            sc.append(c)
        out.append(f"    {{.name = COMPOUND_STRING(\"{name}\"), .region = COMPOUND_STRING(\"{region}\"), "
                   f".type = TYPE_{typ}, .skill = {skill}, .pic = {pic}, .female = {str(female).upper()},\n"
                   f"     .species = {{{', '.join(sc)}}}}},\n")
    out.append("};\n")

    if errors:
        print("\n".join(errors))
        sys.exit(1)
    (ROOT / "src/data/academy.h").write_text("".join(out))
    print(f"academy: {len(RENTALS)} rentals, {len(TEACHERS)} teachers, {len(CANDIDATES)} candidates")


if __name__ == "__main__":
    main()
