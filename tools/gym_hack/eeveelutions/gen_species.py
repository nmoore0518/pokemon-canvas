#!/usr/bin/env python3
"""The ten new eeveelutions' species data (Pokémon Canvas).

    python3 tools/gym_hack/eeveelutions/gen_species.py

Writes:
  src/data/pokemon/species_info/canvas_families.h   the species (stats, types, abilities, Pokédex)
  src/data/pokemon/level_up_learnsets/canvas.h      their level-up moves
  src/data/pokemon/all_learnables.json              their TM/tutor moves (make_teachables.py makes
                                                    the teachable learnsets from this)
and the graphics block in src/data/graphics/pokemon.h. Run build.py first for the graphics; its
sizes and offsets are measured here from the PNGs.

Each evolves from Eevee when it levels up holding its type's power-up item (EVOLUTIONS, in Eevee's
entry in gen_1_families.h). All are 525 total, like the official eeveelutions. The Pokédex text is a
draft.
"""
import json
import re
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]

# (key, Name, type, HP/Atk/Def/SpA/SpD/Spe, abilities, cry, body colour, category, height dm, weight hg,
#  evolution item, Pokédex text (4 lines), evolution move, level-up moves 20-50, extra teachables)
SPECIES = [
    ("odeon", "Odeon", "NORMAL", (90, 90, 85, 90, 85, 85), ("ADAPTABILITY", "ADAPTABILITY", "PROTEAN"), "FLAREON", "WHITE",
     "All-Seeing", 10, 270, "SILK_SCARF",
     ["It gave the sight of one eye to see all",
      "things. Its mane holds a little of every",
      "other evolution's power, and it can call",
      "on any of them when it needs to."],
     "SWIFT", ["HEADBUTT", "WORK_UP", "TRI_ATTACK", "BODY_SLAM", "EXTREME_SPEED", "WISH", "HYPER_VOICE"],
     ["FLAMETHROWER", "THUNDERBOLT", "ICE_BEAM", "PSYCHIC", "EARTHQUAKE", "BRICK_BREAK", "AERIAL_ACE", "STRENGTH",
      "TRI_ATTACK", "EXTREME_SPEED", "BOOMBURST"]),
    ("assasseon", "Assasseon", "FIGHTING", (65, 130, 65, 60, 95, 110), ("INNER_FOCUS", "INNER_FOCUS", "TECHNICIAN"), "UMBREON", "BLUE",
     "Shadow Fist", 9, 245, "BLACK_BELT",
     ["It strikes from the shadows before its",
      "foe can sense it is there. The cloth",
      "wrapped around its legs is said to hold",
      "the scent of every rival it has beaten."],
     "MACH_PUNCH", ["LOW_KICK", "FEINT", "BRICK_BREAK", "SUCKER_PUNCH", "BULK_UP", "NIGHT_SLASH", "CLOSE_COMBAT"],
     ["BRICK_BREAK", "BULK_UP", "FOCUS_PUNCH", "STRENGTH", "TAUNT", "TORMENT", "THIEF", "AERIAL_ACE", "ROCK_TOMB",
      "LOW_KICK", "DRAIN_PUNCH", "CLOSE_COMBAT", "SUCKER_PUNCH", "KNOCK_OFF", "U_TURN", "POISON_JAB"]),
    ("hypereon", "Hypereon", "FLYING", (65, 110, 60, 65, 95, 130), ("KEEN_EYE", "KEEN_EYE", "AERILATE"), "SYLVEON", "WHITE",
     "Gale", 11, 210, "SHARP_BEAK",
     ["Its wings grew from the fur of its",
      "shoulders. It rides high winds for days",
      "without landing, and dives faster than",
      "the eye can follow."],
     "WING_ATTACK", ["AERIAL_ACE", "FEATHER_DANCE", "TAILWIND", "ROOST", "ACROBATICS", "EXTREME_SPEED", "BRAVE_BIRD"],
     ["FLY", "AERIAL_ACE", "STEEL_WING", "BRAVE_BIRD", "ROOST", "TAILWIND", "U_TURN", "AIR_SLASH", "ACROBATICS",
      "DUAL_WINGBEAT", "EXTREME_SPEED"]),
    ("poiseon", "Poiseon", "POISON", (95, 60, 80, 95, 130, 65), ("POISON_POINT", "POISON_POINT", "CORROSION"), "VAPOREON", "PURPLE",
     "Venom", 9, 260, "POISON_BARB",
     ["The spots on its hide hold a poison that",
      "changes with what it eats. It is gentle,",
      "and only lets the poison seep out when",
      "its friends are threatened."],
     "ACID", ["VENOSHOCK", "TOXIC_SPIKES", "ACID_ARMOR", "TOXIC", "SLUDGE_BOMB", "RECOVER", "SLUDGE_WAVE"],
     ["SLUDGE_BOMB", "VENOSHOCK", "SLUDGE_WAVE", "TOXIC_SPIKES", "ACID_SPRAY", "GUNK_SHOT", "PSYCHIC", "TAUNT",
      "HEX", "ENERGY_BALL", "GIGA_DRAIN"]),
    ("subterraneon", "Subterraneon", "GROUND", (95, 115, 110, 60, 65, 80), ("SAND_VEIL", "SAND_VEIL", "SAND_FORCE"), "LEAFEON", "BROWN",
     "Burrow", 10, 320, "SOFT_SAND",
     ["It digs tunnels so long that whole towns",
      "have been built on top of them without",
      "anyone noticing. Its huge ears hear",
      "footsteps far above it."],
     "MUD_SLAP", ["BULLDOZE", "SAND_TOMB", "DIG", "STEALTH_ROCK", "HIGH_HORSEPOWER", "SANDSTORM", "EARTHQUAKE"],
     ["EARTHQUAKE", "SANDSTORM", "ROCK_TOMB", "STRENGTH", "STEALTH_ROCK", "HIGH_HORSEPOWER", "BULLDOZE",
      "ROCK_SLIDE", "STONE_EDGE", "STOMPING_TANTRUM", "SCORCHING_SANDS", "EARTH_POWER"]),
    ("obsideon", "Obsideon", "ROCK", (70, 130, 110, 60, 65, 90), ("STURDY", "STURDY", "SOLID_ROCK"), "GLACEON", "GRAY",
     "Obsidian", 10, 410, "HARD_STONE",
     ["Its body is as dark as volcanic glass.",
      "The crystals on its head and tail grow",
      "larger every year, and they ring like",
      "bells when it runs."],
     "ROCK_THROW", ["ACCELEROCK", "ROCK_TOMB", "ROCK_SLIDE", "STEALTH_ROCK", "ROCK_POLISH", "STONE_EDGE", "HEAD_SMASH"],
     ["ROCK_TOMB", "SANDSTORM", "EARTHQUAKE", "STRENGTH", "ROCK_SLIDE", "STONE_EDGE", "STEALTH_ROCK",
      "IRON_DEFENSE", "IRON_HEAD", "POWER_GEM", "ACCELEROCK"]),
    ("lenneon", "Lenneon", "BUG", (75, 60, 70, 130, 115, 75), ("SWARM", "SWARM", "PUNK_ROCK"), "JOLTEON", "GREEN",
     "Beat", 9, 225, "SILVER_POWDER",
     ["It hums to itself all day. Its wings buzz",
      "in time with any song it hears, and Bug",
      "Pokémon gather to listen when it sings.",
      "It always wears its round glasses."],
     "STRUGGLE_BUG", ["ECHOED_VOICE", "SING", "POLLEN_PUFF", "QUIVER_DANCE", "BUG_BUZZ", "HYPER_VOICE", "BOOMBURST"],
     ["BUG_BUZZ", "SIGNAL_BEAM", "U_TURN", "ENERGY_BALL", "GIGA_DRAIN", "PSYCHIC", "DAZZLING_GLEAM", "POLLEN_PUFF",
      "BOOMBURST", "SING", "AIR_SLASH"]),
    ("appariteon", "Appariteon", "GHOST", (65, 60, 70, 130, 95, 105), ("CURSED_BODY", "CURSED_BODY", "INFILTRATOR"), "ESPEON", "PURPLE",
     "Phantom", 9, 190, "SPELL_TAG",
     ["It fades in and out of sight on moonless",
      "nights. Those who follow it into the dark",
      "find themselves back where they started,",
      "with no memory of the way."],
     "ASTONISH", ["SHADOW_SNEAK", "CONFUSE_RAY", "HEX", "WILL_O_WISP", "SHADOW_BALL", "NASTY_PLOT", "MOONLIGHT"],
     ["SHADOW_BALL", "PSYCHIC", "TAUNT", "WILL_O_WISP", "HEX", "NASTY_PLOT", "DARK_PULSE", "ENERGY_BALL",
      "THUNDERBOLT", "PAIN_SPLIT", "SHADOW_CLAW"]),
    ("automateon", "Automateon", "STEEL", (65, 110, 130, 60, 95, 65), ("CLEAR_BODY", "CLEAR_BODY", "HEAVY_METAL"), "GLACEON", "GRAY",
     "Clockwork", 10, 480, "METAL_COAT",
     ["A brass gear turns slowly around its",
      "neck. Its fur has hardened into metal",
      "plates, and it oils its joints by",
      "rolling in the morning dew."],
     "METAL_CLAW", ["IRON_DEFENSE", "BULLET_PUNCH", "IRON_HEAD", "SHIFT_GEAR", "GYRO_BALL", "HEAVY_SLAM", "GEAR_GRIND"],
     ["STEEL_WING", "IRON_HEAD", "FLASH_CANNON", "IRON_DEFENSE", "HEAVY_SLAM", "EARTHQUAKE", "STRENGTH",
      "GYRO_BALL", "BULLET_PUNCH", "THUNDERBOLT", "STEALTH_ROCK"]),
    ("valyreon", "Valyreon", "DRAGON", (75, 65, 65, 125, 95, 100), ("MARVEL_SCALE", "MARVEL_SCALE", "DRAGONS_MAW"), "VAPOREON", "GREEN",
     "Dragonblood", 12, 300, "DRAGON_FANG",
     ["It is said to carry the blood of an",
      "ancient dragon. Its horns grow back",
      "sharper each time they break, and its",
      "breath can melt rock."],
     "DRAGON_BREATH", ["TWISTER", "SCARY_FACE", "DRAGON_PULSE", "CALM_MIND", "FLAMETHROWER", "DRACO_METEOR", "FIRE_BLAST"],
     ["DRAGON_CLAW", "DRAGON_PULSE", "DRACO_METEOR", "FLAMETHROWER", "FIRE_BLAST", "THUNDERBOLT", "SURF",
      "ICE_BEAM", "OUTRAGE", "DRAGON_TAIL", "EARTH_POWER"]),
]

EEVEE_SHARED_L1 = ["TACKLE", "TAKE_DOWN", "DOUBLE_EDGE", "TAIL_WHIP", "GROWL", "COPYCAT", "COVET", "HELPING_HAND", "CHARM",
                   "BATON_PASS", "BITE"]
TYPE_EV = {0: "HP", 1: "Attack", 2: "Defense", 3: "SpAttack", 4: "SpDefense", 5: "Speed"}


def bbox(path, h=64):
    im = Image.open(path)
    pts = [(x, y) for y in range(h) for x in range(64) if im.getpixel((x, y))]
    return (min(x for x, _ in pts), min(y for _, y in pts), max(x for x, _ in pts) + 1, max(y for _, y in pts) + 1)


def pic_size(w, h):
    r8 = lambda v: min(64, (v + 7) // 8 * 8)
    return r8(w), r8(h)


def camel(name):
    return name[0].upper() + name[1:]


def species_entry(sp, dexnum_const):
    key, name, ty, st, ab, cry, color, cat, height, weight, item, text, evo, moves, extra = sp
    d = ROOT / 'graphics/pokemon' / key
    fb = bbox(d / 'anim_front.png'); bb = bbox(d / 'back.png')
    fw, fh = pic_size(fb[2] - fb[0], fb[3] - fb[1]); bw, bh = pic_size(bb[2] - bb[0], bb[3] - bb[1])
    ev = max(range(6), key=lambda i: st[i])
    C = camel(key)
    icon_pal = icon_pal_of(d / 'icon.png')
    lines = "\n".join(f'            "{t}\\n"' if i < 3 else f'            "{t}"' for i, t in enumerate(text))
    return f"""    [SPECIES_{key.upper()}] =
    {{
        .baseHP        = {st[0]},
        .baseAttack    = {st[1]},
        .baseDefense   = {st[2]},
        .baseSpeed     = {st[5]},
        .baseSpAttack  = {st[3]},
        .baseSpDefense = {st[4]},
        .types = MON_TYPES(TYPE_{ty}),
        .catchRate = 45,
        .expYield = 184,
        .evYield_{TYPE_EV[ev]} = 2,
        .genderRatio = PERCENT_FEMALE(12.5),
        .eggCycles = 35,
        .friendship = STANDARD_FRIENDSHIP,
        .growthRate = GROWTH_MEDIUM_FAST,
        .eggGroups = MON_EGG_GROUPS(EGG_GROUP_FIELD),
        .abilities = {{ ABILITY_{ab[0]}, ABILITY_{ab[1]}, ABILITY_{ab[2]} }},
        .bodyColor = BODY_COLOR_{color},
        .speciesName = _("{name}"),
        .cryId = CRY_{cry},
        .natDexNum = {dexnum_const},
        .categoryName = _("{cat}"),
        .height = {height},
        .weight = {weight},
        .description = COMPOUND_STRING(
{lines}),
        .pokemonScale = 305,
        .pokemonOffset = 7,
        .trainerScale = 257,
        .trainerOffset = 0,
        .frontPic = gMonFrontPic_{C},
        .frontPicSize = MON_COORDS_SIZE({fw}, {fh}),
        .frontPicYOffset = {64 - fb[3]},
        .frontAnimFrames = ANIM_FRAMES(
            ANIMCMD_FRAME(0, 10),
            ANIMCMD_FRAME(1, 20),
            ANIMCMD_FRAME(0, 5),
        ),
        .frontAnimId = ANIM_V_STRETCH,
        .backPic = gMonBackPic_{C},
        .backPicSize = MON_COORDS_SIZE({bw}, {bh}),
        .backPicYOffset = {64 - bb[3]},
        .backAnimId = BACK_ANIM_CONCAVE_ARC_SMALL,
        .palette = gMonPalette_{C},
        .shinyPalette = gMonShinyPalette_{C},
        .iconSprite = gMonIcon_{C},
        .iconPalIndex = {icon_pal},
        .pokemonJumpType = PKMN_JUMP_TYPE_NONE,
        SHADOW(0, 2, SHADOW_SIZE_M)
        FOOTPRINT({C})
        .levelUpLearnset = s{C}LevelUpLearnset,
        .teachableLearnset = s{C}TeachableLearnset,
    }},
"""


ICON_PAL = {}


def icon_pal_of(path):
    return ICON_PAL[path.parent.name]


def main():
    # icon palettes: as build.py chose them
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from icons import CHOICE
    from backs import DESIGNS
    for k, (front, *_r) in DESIGNS.items():
        ICON_PAL[k] = CHOICE[front][0]

    out = ["// Pokémon Canvas: the ten new eeveelutions (tools/gym_hack/eeveelutions/gen_species.py; don't edit by hand).",
           "// Each evolves from Eevee when it levels up holding its type's power-up item.", ""]
    for sp in SPECIES:
        out.append(f"    // {sp[1]} ({sp[2].title()}): levels up holding the {sp[10].replace('_', ' ').title()}")
        out.append(species_entry(sp, f"NATIONAL_DEX_{sp[0].upper()}"))
    (ROOT / 'src/data/pokemon/species_info/canvas_families.h').write_text("\n".join(out))

    lu = ["// Pokémon Canvas: the ten new eeveelutions' level-up moves (tools/gym_hack/eeveelutions/gen_species.py)", ""]
    for sp in SPECIES:
        C = camel(sp[0]); evo, moves = sp[12], sp[13]
        lu.append(f"static const struct LevelUpMove s{C}LevelUpLearnset[] = {{")
        lu.append(f"    LEVEL_UP_MOVE( 0, MOVE_{evo}),")
        for m in EEVEE_SHARED_L1:
            lu.append(f"    LEVEL_UP_MOVE( 1, MOVE_{m}),")
        lu += ["    LEVEL_UP_MOVE( 5, MOVE_SAND_ATTACK),", "    LEVEL_UP_MOVE(10, MOVE_QUICK_ATTACK),",
               "    LEVEL_UP_MOVE(15, MOVE_BABY_DOLL_EYES),"]
        for lvl, m in zip(range(20, 55, 5), moves):
            lu.append(f"    LEVEL_UP_MOVE({lvl}, MOVE_{m}),")
        lu += ["    LEVEL_UP_MOVE(55, MOVE_LAST_RESORT),", "    LEVEL_UP_END", "};", ""]
    (ROOT / 'src/data/pokemon/level_up_learnsets/canvas.h').write_text("\n".join(lu))

    # TM/tutor moves: what every official eeveelution can learn, plus the type's own
    p = ROOT / 'src/data/pokemon/all_learnables.json'
    learn = json.loads(p.read_text())
    evs = ['VAPOREON', 'JOLTEON', 'FLAREON', 'ESPEON', 'UMBREON', 'LEAFEON', 'GLACEON', 'SYLVEON']
    common = set(learn[evs[0]])
    for e in evs[1:]:
        common &= set(learn[e])
    common -= {'MOVE_ALLURING_VOICE', 'MOVE_DISARMING_VOICE'}   # (Sylveon's Fairy moves)
    for sp in SPECIES:
        learn[sp[0].upper()] = sorted(common | {f"MOVE_{m}" for m in sp[14]} | {f"MOVE_{m}" for m in sp[13]})
    p.write_text(json.dumps(learn, indent=2) + "\n")

    # graphics
    g = ROOT / 'src/data/graphics/pokemon.h'
    text = g.read_text()
    begin, end = "// Pokémon Canvas: the new eeveelutions (tools/gym_hack/eeveelutions)\n", "// (end of the new eeveelutions)\n"
    block = begin
    for sp in SPECIES:
        k, C = sp[0], camel(sp[0])
        block += (f'    const u32 gMonFrontPic_{C}[] = INCGFX_U32("graphics/pokemon/{k}/anim_front.png", ".4bpp.smol");\n'
                  f'    const u16 gMonPalette_{C}[] = INCGFX_U16("graphics/pokemon/{k}/normal.pal", ".gbapal");\n'
                  f'    const u32 gMonBackPic_{C}[] = INCGFX_U32("graphics/pokemon/{k}/back.png", ".4bpp.smol");\n'
                  f'    const u16 gMonShinyPalette_{C}[] = INCGFX_U16("graphics/pokemon/{k}/shiny.pal", ".gbapal");\n'
                  f'    const u8 gMonIcon_{C}[] = INCGFX_U8("graphics/pokemon/{k}/icon.png", ".4bpp");\n'
                  f'#if P_FOOTPRINTS\n'
                  f'    const u8 gMonFootprint_{C}[] = INCGFX_U8("graphics/pokemon/{k}/footprint.png", ".1bpp");\n'
                  f'#endif //P_FOOTPRINTS\n')
    block += end
    if begin in text:
        text = text[:text.index(begin)] + block + text[text.index(end) + len(end):]
    else:
        anchor = "// Gym Trainer hack: Bill's hybrid"
        text = text.replace(anchor, block + "\n" + anchor, 1)
    g.write_text(text)


if __name__ == '__main__':
    main()
