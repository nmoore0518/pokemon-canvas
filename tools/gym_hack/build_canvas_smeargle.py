#!/usr/bin/env python3
"""Canvas Smeargle (Pokémon Canvas): the painter in Pewter City's Smeargle (src/painter.c).

    python3 tools/gym_hack/build_canvas_smeargle.py [preview.png]

The forms, all one Smeargle (National Dex 235), 500 base stats, fast and special:
  SPECIES_SMEARGLE_CANVAS             typeless (???), its tail unpainted (the painter's: lent out)
  SPECIES_SMEARGLE_CANVAS_<TYPE>      one per type, its tail painted the type's colour (the player's
                                      gym type: given after the first painting)
  SPECIES_SMEARGLE_CANVAS_<TYPE>_MEGA Mega Smeargle, 600: a huge rainbow brush, a palette round its
                                      neck (one look for every type: canvas_mega_art.py; the
                                      SMEARGLITE, a key item: src/pokemon.c's form check)
Writes:
  include/constants/canvas_smeargle.h                     the species' numbers (included in species.h)
  src/data/pokemon/species_info/canvas_smeargle.h         their gSpeciesInfo entries
  src/data/pokemon/canvas_smeargle_forms.h                their learnset, form and form-change tables
  src/data/graphics/canvas_smeargle.h                     their palettes, icons and the Mega's pics
  graphics/items/icons/gym/smearglite.png (+ its palette)  the SMEARGLITE
  graphics/pokemon/smeargle/canvas/                       palettes (Smeargle's, its paint recoloured),
                                                          icons (its tail recoloured, in icon palette
                                                          1 or 3), the Mega's art (a draft: drawn here
                                                          from Smeargle's own frames)
"""
import math
import random
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "graphics/pokemon/smeargle"
OUT = SRC / "canvas"

TYPES = ["NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL",
         "FIRE", "WATER", "GRASS", "ELECTRIC", "PSYCHIC", "ICE", "DRAGON", "DARK", "FAIRY"]
# Each type's paint: its colour (shaded dark .. light from it)
TYPE_COLOURS = {
    "NORMAL": (184, 176, 152), "FIGHTING": (200, 56, 40), "FLYING": (152, 136, 240), "POISON": (168, 64, 168),
    "GROUND": (224, 184, 88), "ROCK": (176, 152, 64), "BUG": (152, 192, 32), "GHOST": (112, 80, 168),
    "STEEL": (168, 176, 200), "FIRE": (248, 120, 40), "WATER": (72, 136, 248), "GRASS": (88, 200, 64),
    "ELECTRIC": (248, 208, 32), "PSYCHIC": (248, 80, 136), "ICE": (128, 216, 232), "DRAGON": (104, 64, 240),
    "DARK": (88, 72, 64), "FAIRY": (240, 144, 184),
}
# The icon's tail: (icon palette, its dark and light indices). Palettes 1 and 3 share Smeargle's body
# colours (indices 2, 3, 7, 9, 11, 12, 14, 15); they differ in 4-6 and 8 and 13.
ICON_TAIL = {
    None: (1, 12, 11),
    "NORMAL": (1, 2, 3), "FIGHTING": (3, 8, 13), "FLYING": (3, 5, 6), "POISON": (1, 8, 8),
    "GROUND": (1, 9, 12), "ROCK": (1, 9, 9), "BUG": (1, 5, 6), "GHOST": (1, 15, 8),
    "STEEL": (1, 1, 2), "FIRE": (3, 8, 13), "WATER": (3, 4, 5), "GRASS": (1, 4, 5),
    "ELECTRIC": (1, 10, 11), "PSYCHIC": (1, 8, 7), "ICE": (3, 5, 6), "DRAGON": (3, 4, 4),
    "DARK": (1, 15, 14), "FAIRY": (1, 7, 8),
}
PAINT = (13, 8, 11, 9)          # (Smeargle's palette: its tail's paint, dark .. light)
FUR = (2, 5, 12)                # (light, mid, dark)
OW_PAINT = (8, 11)              # (its overworld sprite's: dark, light)
ICON_PAINT = (4, 5)             # (its icon's, in palette 1: dark, light)

STATS = dict(hp=70, atk=45, df=60, spe=125, spa=120, spd=80)            # 500
MEGA_STATS = dict(hp=80, atk=55, df=70, spe=150, spa=150, spd=95)       # 600


def cap(t):
    return t.capitalize()


def load_pal(path):
    return [tuple(int(v) for v in l.split()) for l in Path(path).read_text().splitlines()[3:19]]


def write_pal(path, cols):
    path.write_text("JASC-PAL\n0100\n16\n" + "".join(f"{r} {g} {b}\n" for r, g, b in cols))


def gba(c):
    return tuple((v >> 3) << 3 for v in c)


def shades(c):
    """(four shades of a colour, dark to light)"""
    dark = tuple(int(v * 0.45) for v in c)
    mid = tuple(int(v * 0.72) for v in c)
    light = tuple(int(v + (255 - v) * 0.45) for v in c)
    return [gba(dark), gba(mid), gba(c), gba(light)]


def paint_palette(base, colour):
    pal = list(base)
    if colour is None:
        # (unpainted: its tail the colour of its fur)
        fl, fm, fd = (base[i] for i in FUR)
        darker = tuple(int(v * 0.8) for v in fd)
        for idx, c in zip(PAINT, (darker, fd, fm, fl)):
            pal[idx] = c
    else:
        for idx, c in zip(PAINT, shades(colour)):
            pal[idx] = c
    return pal


# ---------------------------------------------------------------------------------------- the Mega
def megaify(img, seed, shift, paint, outline, splats=5, scale=1.6, fur=FUR):
    """A bigger brush (its paint blob grown from where it meets the handle) and paint splashed on the
    fur. `shift` moves the whole picture sideways first, to make room."""
    w, h = img.size
    moved = Image.new("P", (w, h))
    moved.paste(img.crop((max(0, -shift), 0, w - max(0, shift), h)), (max(0, shift), 0))
    p = moved.load()
    pts = {(x, y) for y in range(h) for x in range(w) if p[x, y] in paint}
    # (only the brush: the biggest patch of paint, not the marks on its back)
    best, seen = set(), set()
    for start in pts:
        if start in seen:
            continue
        comp, todo = set(), [start]
        while todo:
            q = todo.pop()
            if q in comp or q not in pts:
                continue
            comp.add(q)
            x, y = q
            todo += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1), (x + 1, y + 1), (x - 1, y - 1), (x + 1, y - 1), (x - 1, y + 1)]
        seen |= comp
        if len(comp) > len(best):
            best = comp
    blob = {q: p[q] for q in best}
    bx, by = w / 2, h * 0.6
    base = min(best, key=lambda q: (q[0] - bx) ** 2 + (q[1] - by) ** 2)
    out = moved.copy()
    o = out.load()
    new = {}
    for y in range(h):
        for x in range(w):
            k = (int(round(base[0] + (x - base[0]) / scale)), int(round(base[1] + (y - base[1]) / scale)))
            if k in blob:
                new[(x, y)] = blob[k]
    for q, v in new.items():
        if p[q] == 0 or q in blob:
            o[q] = v
    for (x, y) in list(new):
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            q = (x + dx, y + dy)
            if 0 <= q[0] < w and 0 <= q[1] < h and q not in new and o[q] == 0:
                o[q] = outline
    rnd = random.Random(seed)
    fur_px = [(x, y) for y in range(h) for x in range(w) if o[x, y] in fur[:2] and (x, y) not in new]
    for _ in range(splats):
        x, y = rnd.choice(fur_px)
        r = rnd.choice((1, 2, 2))
        for yy in range(y - r, y + r + 1):
            for xx in range(x - r, x + r + 1):
                if 0 <= xx < w and 0 <= yy < h and o[xx, yy] in fur and (xx - x) ** 2 + (yy - y) ** 2 <= r * r + 1:
                    o[xx, yy] = paint[2] if (xx + yy) % 3 else paint[3]
    return out


def mega_art():
    """The Mega's sprites are hand art (mega_anim_front.png, mega_back.png: converted from Nick's
    reference by tools/gym_hack/art/mega_smeargle_from_ref.py, then touched up): only its palettes
    and its icon are made here. One look for every type."""
    import canvas_mega_art as M
    icon = Image.open(SRC / "icon.png")
    isheet = Image.new("P", icon.size)
    isheet.putpalette(icon.getpalette())
    for k in range(icon.size[1] // 32):
        isheet.paste(M.icon(icon.crop((0, 32 * k, 32, 32 * k + 32))), (0, 32 * k))
    isheet.save(OUT / "mega_icon.png")
    write_pal(OUT / "mega.pal", M.MEGA_PALETTE)
    write_pal(OUT / "mega_shiny.pal", M.MEGA_SHINY_PALETTE)


def tail_icon(src, tail, name):
    """(the icon, its tail's two shades moved to the type's indices)"""
    _, dark, light = tail
    out = src.point(lambda v: dark if v == ICON_PAINT[0] else light if v == ICON_PAINT[1] else v)
    out.putpalette(src.getpalette())
    out.save(OUT / name)


def smearglite_icon():
    """The SMEARGLITE: a Mega Stone's sphere (Ampharosite's shape) with a rainbow swirled in it."""
    src = Image.open(ROOT / "graphics/items/icons/ampharosite.png")
    p = src.load()
    out = Image.new("P", src.size)
    o = out.load()
    rainbow = [(232, 56, 56), (248, 144, 40), (248, 216, 48), (80, 192, 72), (64, 136, 232), (152, 88, 208)]
    slots = [2, 3, 4, 6, 7, 8]
    for y in range(src.size[1]):
        for x in range(src.size[0]):
            v = p[x, y]
            if v in (2, 3, 4, 6, 7, 8, 10, 11):
                a = (math.degrees(math.atan2(y - 11.5, x - 11.5)) + 360 + (x + y) * 9) % 360
                v = slots[int(a / 60) % 6]
            o[x, y] = v
    pal = load_pal(ROOT / "graphics/items/icon_palettes/ampharosite.pal")
    for slot, c in zip(slots, rainbow):
        pal[slot] = c
    flat = [v for c in pal for v in c]
    out.putpalette(flat + [0] * (768 - len(flat)))
    (ROOT / "graphics/items/icons/gym").mkdir(exist_ok=True)
    out.save(ROOT / "graphics/items/icons/gym/smearglite.png")
    write_pal(ROOT / "graphics/items/icon_palettes/gym/smearglite.pal", [gba(c) for c in pal])


# -------------------------------------------------------------------------------------------- C
def species_names():
    names = ["SPECIES_SMEARGLE_CANVAS"]
    names += [f"SPECIES_SMEARGLE_CANVAS_{t}" for t in TYPES]
    names += [f"SPECIES_SMEARGLE_CANVAS_{t}_MEGA" for t in TYPES]
    return names


def write_constants():
    lines = ["// Generated by tools/gym_hack/build_canvas_smeargle.py: Canvas Smeargle's forms (in the",
             "// species enum, after SPECIES_PIKACHU_SURFING). Typeless, then one per type, then their Megas.",
             "#ifndef GUARD_CONSTANTS_CANVAS_SMEARGLE_SPECIES",
             "#define GUARD_CONSTANTS_CANVAS_SMEARGLE_SPECIES",
             "#define CANVAS_SMEARGLE_SPECIES \\"]
    lines += [f"    {n}, \\" for n in species_names()]
    lines += ["", "#endif", ""]
    (ROOT / "include/constants/canvas_smeargle.h").write_text("\n".join(lines))


def write_graphics():
    lines = ["// Generated by tools/gym_hack/build_canvas_smeargle.py: Canvas Smeargle's palettes, icons and",
             "// the Mega's pics (its other pics are Smeargle's own)", ""]
    for key in [None] + TYPES:
        n = "Typeless" if key is None else cap(key)
        f = "typeless" if key is None else key.lower()
        lines.append(f'const u16 gMonPalette_SmeargleCanvas{n}[] = INCGFX_U16("graphics/pokemon/smeargle/canvas/{f}.pal", ".gbapal");')
        lines.append(f'const u16 gMonShinyPalette_SmeargleCanvas{n}[] = INCGFX_U16("graphics/pokemon/smeargle/canvas/{f}_shiny.pal", ".gbapal");')
        lines.append(f'const u8 gMonIcon_SmeargleCanvas{n}[] = INCGFX_U8("graphics/pokemon/smeargle/canvas/icon_{f}.png", ".4bpp");')
    lines.append('const u16 gOverworldPalette_SmeargleCanvasTypeless[] = INCGFX_U16("graphics/pokemon/smeargle/canvas/overworld_typeless.pal", ".gbapal");')
    lines.append('const u16 gMonPalette_SmeargleCanvasMega[] = INCGFX_U16("graphics/pokemon/smeargle/canvas/mega.pal", ".gbapal");')
    lines.append('const u16 gMonShinyPalette_SmeargleCanvasMega[] = INCGFX_U16("graphics/pokemon/smeargle/canvas/mega_shiny.pal", ".gbapal");')
    lines.append('const u8 gMonIcon_SmeargleCanvasMega[] = INCGFX_U8("graphics/pokemon/smeargle/canvas/mega_icon.png", ".4bpp");')
    lines.append('const u32 gMonFrontPic_SmeargleCanvasMega[] = INCGFX_U32("graphics/pokemon/smeargle/canvas/mega_anim_front.png", ".4bpp.smol");')
    lines.append('const u32 gMonBackPic_SmeargleCanvasMega[] = INCGFX_U32("graphics/pokemon/smeargle/canvas/mega_back.png", ".4bpp.smol");')
    lines.append("")
    (ROOT / "src/data/graphics/canvas_smeargle.h").write_text("\n".join(lines))


def entry(species, key, mega):
    n = "Typeless" if key is None else cap(key)
    st = MEGA_STATS if mega else STATS
    types = "MON_TYPES(TYPE_MYSTERY)" if key is None else f"MON_TYPES(TYPE_{key})"
    abilities = "{ ABILITY_PROTEAN, ABILITY_PROTEAN, ABILITY_PROTEAN }" if mega else "{ ABILITY_SERENE_GRACE, ABILITY_NONE, ABILITY_TECHNICIAN }"
    if key is None:
        desc = ('"A Smeargle that has never painted with\\n"\n            "its own colour. It watches trainers\\n"\n'
                '            "battle, waiting for a style to call its\\n"\n            "own. Its tail is still bare."')
    elif mega:
        desc = ('"Its brush has grown to twice its size\\n"\n            "and holds every colour at once. It\\n"\n'
                '            "changes type to match whatever move\\n"\n            "it paints with next."')
    else:
        desc = ('"It has found its colour, its trainer\'s\\n"\n            "own, and paints with nothing else.\\n"\n'
                '            "It learns by watching: a technique seen\\n"\n            "once is a technique copied."')
    tail = ICON_TAIL[key]
    icon = "gMonIcon_SmeargleCanvasMega" if mega else f"gMonIcon_SmeargleCanvas{n}"
    iconPal = 1 if mega else tail[0]
    pal = "gMonPalette_SmeargleCanvasMega" if mega else f"gMonPalette_SmeargleCanvas{n}"
    shiny = "gMonShinyPalette_SmeargleCanvasMega" if mega else f"gMonShinyPalette_SmeargleCanvas{n}"
    front = "gMonFrontPic_SmeargleCanvasMega" if mega else "gMonFrontPic_Smeargle"
    back = "gMonBackPic_SmeargleCanvasMega" if mega else "gMonBackPic_Smeargle"
    forms = f"sSmeargleCanvas{n}{'Mega' if mega else ''}FormChangeTable"
    ow_pal = "gOverworldPalette_SmeargleCanvasTypeless" if key is None else "gOverworldPalette_Smeargle"
    return f"""    [{species}] =
    {{
        .baseHP        = {st['hp']},
        .baseAttack    = {st['atk']},
        .baseDefense   = {st['df']},
        .baseSpeed     = {st['spe']},
        .baseSpAttack  = {st['spa']},
        .baseSpDefense = {st['spd']},
        .types = {types},
        .catchRate = 45,
        .expYield = {270 if mega else 175},
        .evYield_SpAttack = 1,
        .evYield_Speed = 2,
        .genderRatio = PERCENT_FEMALE(50),
        .eggCycles = 20,
        .friendship = STANDARD_FRIENDSHIP,
        .growthRate = GROWTH_FAST,
        .eggGroups = MON_EGG_GROUPS(EGG_GROUP_NO_EGGS_DISCOVERED),
        .abilities = {abilities},
        .bodyColor = BODY_COLOR_WHITE,
        .noFlip = TRUE,
        .speciesName = _("Smeargle"),
        .cryId = CRY_SMEARGLE,
        .natDexNum = NATIONAL_DEX_SMEARGLE,
        .categoryName = _("Painter"),
        .height = {16 if mega else 12},
        .weight = {640 if mega else 580},
        .description = COMPOUND_STRING(
            {desc}),
        .pokemonScale = 287,
        .pokemonOffset = 5,
        .trainerScale = 256,
        .trainerOffset = 0,
        .frontPic = {front},
        .frontPicSize = MON_COORDS_SIZE(64, {64 if mega else 56}),
        .frontPicYOffset = {2 if mega else 6},
        .frontAnimFrames = ANIM_FRAMES(
            ANIMCMD_FRAME(1, 12),
            ANIMCMD_FRAME(0, 12),
            ANIMCMD_FRAME(1, 12),
            ANIMCMD_FRAME(0, 10),
        ),
        .frontAnimId = ANIM_H_JUMPS_V_STRETCH,
        .backPic = {back},
        .backPicSize = MON_COORDS_SIZE({64 if mega else 56}, {56 if mega else 48}),
        .backPicYOffset = {7 if mega else 9},
        .backAnimId = BACK_ANIM_H_SLIDE,
        .palette = {pal},
        .shinyPalette = {shiny},
        .iconSprite = {icon},
        .iconPalIndex = {iconPal},
        .pokemonJumpType = PKMN_JUMP_TYPE_NONE,
        SHADOW(6, 7, SHADOW_SIZE_S)
        FOOTPRINT(Smeargle)
        OVERWORLD(
            sPicTable_Smeargle,
            SIZE_32x32,
            SHADOW_SIZE_M,
            TRACKS_FOOT,
            sAnimTable_Following,
            {ow_pal},
            gShinyOverworldPalette_Smeargle
        )
        .teachingType = TM_ILLITERATE,
        .levelUpLearnset = sSmeargleCanvasLevelUpLearnset,
        .teachableLearnset = sSmeargleTeachableLearnset,
        .formSpeciesIdTable = sSmeargleCanvasFormSpeciesIdTable,
        .formChangeTable = {"NULL" if key is None else forms},
        .isFrontierBanned = TRUE,
        .cannotBeTraded = TRUE,{chr(10) + "        .isMegaEvolution = TRUE," if mega else ""}
    }},
"""


def write_species():
    lines = ["// Generated by tools/gym_hack/build_canvas_smeargle.py: Canvas Smeargle (src/painter.c)", ""]
    lines.append(entry("SPECIES_SMEARGLE_CANVAS", None, False))
    for t in TYPES:
        lines.append(entry(f"SPECIES_SMEARGLE_CANVAS_{t}", t, False))
    for t in TYPES:
        lines.append(entry(f"SPECIES_SMEARGLE_CANVAS_{t}_MEGA", t, True))
    (ROOT / "src/data/pokemon/species_info/canvas_smeargle.h").write_text("\n".join(lines))


def write_forms():
    lines = ["// Generated by tools/gym_hack/build_canvas_smeargle.py: Canvas Smeargle's forms. A form Mega",
             "// Evolves with the SMEARGLITE in the bag (a key item, not held: param1 MOVE_NONE, param2 the",
             "// item; src/pokemon.c), into its type's Mega.", "",
             "static const struct LevelUpMove sSmeargleCanvasLevelUpLearnset[] = {",
             "    LEVEL_UP_MOVE(1, MOVE_MASTERSTROKE),",
             "    LEVEL_UP_END",
             "};", "",
             "static const u16 sSmeargleCanvasFormSpeciesIdTable[] = {"]
    lines += [f"    {n}," for n in species_names()]
    lines += ["    FORM_SPECIES_END,", "};", ""]
    for t in TYPES:
        n = cap(t)
        lines += [f"static const struct FormChange sSmeargleCanvas{n}FormChangeTable[] = {{",
                  f"    {{FORM_CHANGE_BATTLE_MEGA_EVOLUTION_MOVE, SPECIES_SMEARGLE_CANVAS_{t}_MEGA, MOVE_NONE, ITEM_SMEARGLITE}},",
                  "    {FORM_CHANGE_TERMINATOR},", "};",
                  f"static const struct FormChange sSmeargleCanvas{n}MegaFormChangeTable[] = {{",
                  f"    {{FORM_CHANGE_FAINT, SPECIES_SMEARGLE_CANVAS_{t}}},",
                  f"    {{FORM_CHANGE_END_BATTLE, SPECIES_SMEARGLE_CANVAS_{t}}},",
                  "    {FORM_CHANGE_TERMINATOR},", "};", ""]
    (ROOT / "src/data/pokemon/canvas_smeargle_forms.h").write_text("\n".join(lines))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    normal, shiny = load_pal(SRC / "normal.pal"), load_pal(SRC / "shiny.pal")
    for key in [None] + TYPES:
        f = "typeless" if key is None else key.lower()
        colour = None if key is None else TYPE_COLOURS[key]
        write_pal(OUT / f"{f}.pal", paint_palette(normal, colour))
        write_pal(OUT / f"{f}_shiny.pal", paint_palette(shiny, colour))
        tail_icon(Image.open(SRC / "icon.png"), ICON_TAIL[key], f"icon_{f}.png")
    ow = load_pal(SRC / "overworld_normal.pal")
    ow[OW_PAINT[0]], ow[OW_PAINT[1]] = ow[4], ow[5]
    write_pal(OUT / "overworld_typeless.pal", ow)
    mega_art()
    smearglite_icon()
    write_constants()
    write_graphics()
    write_species()
    write_forms()
    if len(sys.argv) > 1:
        preview(sys.argv[1])


def rgba(img, pal):
    out = Image.new("RGBA", img.size)
    px, o = img.load(), out.load()
    for y in range(img.size[1]):
        for x in range(img.size[0]):
            v = px[x, y]
            o[x, y] = (*pal[v], 0 if v == 0 else 255)
    return out


def preview(path):
    front = Image.open(SRC / "anim_front.png").crop((0, 0, 64, 64))
    mega = [Image.open(OUT / "mega_anim_front.png").crop((0, 64 * i, 64, 64 * i + 64)) for i in range(2)]
    mback = Image.open(OUT / "mega_back.png")
    keys = [None] + TYPES
    sheet = Image.new("RGBA", (64 * 10, 64 * 4), (96, 112, 128, 255))
    for i, key in enumerate(keys):
        f = "typeless" if key is None else key.lower()
        sheet.alpha_composite(rgba(front, load_pal(OUT / f"{f}.pal")), ((i % 10) * 64, (i // 10) * 64))
    for i, key in enumerate(["", "_shiny"]):
        pal = load_pal(OUT / f"mega{key}.pal")
        sheet.alpha_composite(rgba(mega[0], pal), (i * 128, 128))
        sheet.alpha_composite(rgba(mega[1], pal), (i * 128 + 64, 128))
        sheet.alpha_composite(rgba(mback, pal), (i * 128, 192))
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(path)


if __name__ == "__main__":
    main()
