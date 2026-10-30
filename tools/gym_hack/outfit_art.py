#!/usr/bin/env python3
"""Pokémon Canvas: the player's outfits, as palettes for the player's own sprites.

Every outfit is the player's 16 colors with the clothes recolored, so it covers every sprite the
player has (walking, running, bike, surf, fishing, field moves, the battle back pic) at no cost in
art. Writes src/data/outfits.h (OUTFIT_* order = include/constants/outfits.h) and the wardrobe's list,
data/scripts/outfits_list.inc:
  sOutfitPalettes[OUTFIT_COUNT][2][16]           the overworld and battle back pic colors (boy, girl)
  sOutfitReflectionPalettes[OUTFIT_COUNT][2][16]  the same for reflections in water
Marks: every outfit but the casual clothes has a 4x3 mark on the chest (the Rocket uniform's red
R, a gym uniform's type emblem in the Type Ball's emblem color), drawn on the walking and running
frames that face the screen (graphics/object_events/pics/people/outfits/). The mark's color is
index 13, the bag's highlight before (the player's sheets have their bag in one shade, 14).

    python3 tools/gym_hack/outfit_art.py [preview.png]
"""
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "..")
sys.path.insert(0, HERE)
from type_ball_art import TYPES, BOTTOMS  # noqa: E402  (the gym uniforms match the Type Balls)


def read_pal(path):
    lines = open(os.path.join(ROOT, path)).read().split()
    vals = list(map(int, lines[3:3 + 48]))
    return [tuple(vals[i:i + 3]) for i in range(0, 48, 3)]


BASE = read_pal("graphics/object_events/palettes/player_frlg.pal")
REFL = read_pal("graphics/object_events/palettes/player_reflection_frlg.pal")

# The clothes' color indices
# boy (Red): cap and jacket 8 (dark) 12 11 (light); sleeves and pants 6 (dark) 5 7 (light); bag 14 13
# girl (Leaf): hat 10 (shade) 9 (white); hat band and skirt 12 11; top 6 5 7; bag strap 14 13
TYPE_ORDER = ["normal", "fighting", "flying", "poison", "ground", "rock", "bug", "ghost", "steel", "fire",
              "water", "grass", "electric", "psychic", "ice", "dragon", "dark", "fairy"]


# 4x3 chest marks: "#" the mark's color (index 13), "o" the outline black (15), "." the patch
MARKS = {
    "ROCKET":   ["###.", "##o.", "#o#."],
    "NORMAL":   [".##.", "#oo#", ".##."],
    "FIGHTING": ["####", "####", ".##."],
    "FLYING":   ["#..#", "####", ".##."],
    "POISON":   [".##.", "####", "#o#o"],
    "GROUND":   ["..#.", ".###", "####"],
    "ROCK":     [".##.", "###.", "####"],
    "BUG":      ["#oo#", ".##.", "#oo#"],
    "GHOST":    ["####", "#o#o", "#.#."],
    "STEEL":    ["####", "#oo#", "####"],
    "FIRE":     ["..#.", ".##.", "####"],
    "WATER":    ["..#.", ".###", ".###"],
    "GRASS":    ["..##", ".##.", "#..."],
    "ELECTRIC": ["..##", ".##.", "##.."],
    "PSYCHIC":  ["o##o", "#oo#", "o##o"],
    "ICE":      ["#.#.", ".#..", "#.#."],
    "DRAGON":   ["#..#", "####", ".#.#"],
    "DARK":     [".##.", "#...", ".##."],
    "FAIRY":    [".#..", "###.", ".#.."],
}
MARK_COLOR = {"ROCKET": (232, 48, 40)}
MARK_COLOR.update({t.upper(): TYPES[t][1] for t in TYPES})

# Where the mark goes on the frames that face the screen: (frame, top-left x, y). Frames: the
# walking sheet's 0 (standing), 3, 4 (steps); the running frames are the surf/run sheet's 3, 4, 5.
WALK_SPOTS = {"boy": [(0, 6, 24), (3, 6, 25), (4, 6, 25)], "girl": [(0, 6, 25), (3, 5, 25), (4, 7, 25)]}
RUN_SPOTS = {"boy": [(3, 6, 24), (4, 6, 25), (5, 7, 25)], "girl": [(3, 5, 25), (4, 6, 25), (5, 7, 25)]}
# the patch round the mark: the jacket's (boy) or the top's (girl) color, which the mark stands out on
PATCH = {"boy": 12, "girl": 6}
# what a mark may be drawn over (the clothes and outlines; not skin or hair)
MARKABLE = {"boy": {8, 9, 10, 11, 12, 14, 15, 6, 5, 7}, "girl": {5, 6, 7, 11, 12, 14, 15, 10, 9}}
SHEETS = {"boy": ("red", "red"), "girl": ("leaf", "green")}
RUN_FRAMES = range(3, 14)    # the surf/run sheet's frames the walking pic table uses (9-19)
PEOPLE = "graphics/object_events/pics/people/"


def flatten_bags():
    """The player's sheets keep the bag in one shade (index 13 -> 14), so 13 is free for the marks"""
    import glob
    for f in glob.glob(os.path.join(ROOT, PEOPLE, "red", "*.png")) + glob.glob(os.path.join(ROOT, PEOPLE, "leaf", "*.png")):
        im = Image.open(f)
        data = list(im.getdata())
        if 13 in data:
            im.putdata([14 if v == 13 else v for v in data])
            im.save(f, bits=4)


def marked_sheets(name, who):
    folder, stem = SHEETS[who]
    walk = Image.open(os.path.join(ROOT, PEOPLE, folder, f"{stem}_normal.png")).copy()
    run_src = Image.open(os.path.join(ROOT, PEOPLE, folder, f"{stem}_surf_run.png"))
    run = Image.new("P", (16 * len(RUN_FRAMES), 32))
    run.putpalette(run_src.getpalette())
    for i, f in enumerate(RUN_FRAMES):
        run.paste(run_src.crop((f * 16, 0, f * 16 + 16, 32)), (i * 16, 0))
    for sheet, spots, base in ((walk, WALK_SPOTS[who], 0), (run, RUN_SPOTS[who], RUN_FRAMES[0])):
        px = sheet.load()
        for frame, x0, y0 in spots:
            fx = (frame - base) * 16
            for j, row in enumerate(MARKS[name]):
                for i, ch in enumerate(row):
                    x, y = fx + x0 + i, y0 + j
                    if px[x, y] not in MARKABLE[who]:
                        continue
                    px[x, y] = 13 if ch == "#" else 15 if ch == "o" else PATCH[who]
    return walk, run


def darker(c, f=0.6):
    return tuple(int(v * f) for v in c)


def outfit(boy, girl):
    """boy/girl: {index: color}"""
    out = []
    for changes in (boy, girl):
        pal = list(BASE)
        for i, c in changes.items():
            pal[i] = c
        out.append(pal)
    return out


def gym_uniform(t):
    top, low = TYPES[t][0], BOTTOMS[t]          # top: dark..highlight (4); low: light, mid, shadow
    boy = {8: darker(top[0], 0.7), 12: top[1], 11: top[2], 6: low[2], 5: low[1], 7: low[0]}
    girl = {10: top[2], 9: top[3], 12: low[1], 11: low[0], 6: darker(top[0], 0.8), 5: top[1], 7: top[2]}
    return outfit(boy, girl)


BLACK = [(32, 32, 40), (56, 56, 64), (88, 88, 96)]
OUTFITS = [("CASUAL", outfit({}, {})),
           ("ROCKET", outfit({8: (24, 24, 28), 12: BLACK[0], 11: BLACK[1], 6: (40, 40, 48), 5: BLACK[1], 7: BLACK[2],
                              14: (72, 72, 80), 13: (112, 112, 120)},
                             {10: BLACK[1], 9: BLACK[2], 12: (168, 32, 32), 11: (224, 64, 56), 6: (32, 32, 40),
                              5: BLACK[1], 7: BLACK[2], 14: (72, 72, 80), 13: (112, 112, 120)}))]
OUTFITS += [(t.upper(), gym_uniform(t)) for t in TYPE_ORDER]


def overworld(name, pals):
    """The overworld colors: index 13 is the chest mark's"""
    if name not in MARK_COLOR:
        return pals
    return [[MARK_COLOR[name] if i == 13 else c for i, c in enumerate(p)] for p in pals]


def display(name):
    return {"CASUAL": "CASUAL CLOTHES", "ROCKET": "ROCKET UNIFORM"}.get(name, name + " UNIFORM")


def reflection(pal):
    """The reflection palette: each channel mapped the way the game's own reflection colors are."""
    import numpy as np
    out = []
    for ch in range(3):
        xs = np.array([BASE[i][ch] for i in range(1, 16)], float)
        ys = np.array([REFL[i][ch] for i in range(1, 16)], float)
        a, b = np.polyfit(xs, ys, 1)
        out.append((a, b))
    res = [REFL[0]]
    for i in range(1, 16):
        if pal[i] == BASE[i]:
            res.append(REFL[i])
        else:
            res.append(tuple(max(0, min(255, int(round(out[ch][0] * pal[i][ch] + out[ch][1])))) for ch in range(3)))
    return res


def gba(c):
    r, g, b = (v >> 3 for v in c)
    return r | (g << 5) | (b << 10)


def main():
    lines = ["// Generated by tools/gym_hack/outfit_art.py: the player's outfits (palettes for the player's sprites)",
             "// [outfit][MALE/FEMALE][16]", ""]
    flatten_bags()
    ow = [overworld(n, o) for n, o in OUTFITS]
    for name, arr in (("sOutfitPalettes", ow),
                      ("sOutfitBackPicPalettes", [o for _, o in OUTFITS]),
                      ("sOutfitReflectionPalettes", [[reflection(p) for p in o] for o in ow])):
        lines.append(f"static const u16 {name}[OUTFIT_COUNT][GENDER_COUNT][16] =\n{{")
        for (oname, _), pals in zip(OUTFITS, arr):
            lines.append(f"    [OUTFIT_{oname}] = {{")
            for p in pals:
                lines.append("        {" + ", ".join(f"0x{gba(c):04X}" for c in p) + "},")
            lines.append("    },")
        lines.append("};\n")
    # the marked sheets and their pic tables (the walking state's 20 frames, as the game's own)
    os.makedirs(os.path.join(ROOT, PEOPLE, "outfits"), exist_ok=True)
    tables = {}
    for name, _ in OUTFITS:
        if name not in MARKS:
            continue
        for g, who in ((0, "boy"), (1, "girl")):
            walk, run = marked_sheets(name, who)
            base = f"{name.lower()}_{who}"
            walk.save(os.path.join(ROOT, PEOPLE, "outfits", f"{base}_walk.png"), bits=4)
            run.save(os.path.join(ROOT, PEOPLE, "outfits", f"{base}_run.png"), bits=4)
            cname = "".join(w.title() for w in name.lower().split("_")) + who.title()
            lines.append(f'static const u16 sOutfitPic_{cname}Walk[] = INCGFX_U16("{PEOPLE}outfits/{base}_walk.png", ".4bpp", "-mwidth 2 -mheight 4");')
            lines.append(f'static const u16 sOutfitPic_{cname}Run[] = INCGFX_U16("{PEOPLE}outfits/{base}_run.png", ".4bpp", "-mwidth 2 -mheight 4");')
            frames = [f"    overworld_frame(sOutfitPic_{cname}Walk, 2, 4, {k})," for k in range(9)]
            frames += [f"    overworld_frame(sOutfitPic_{cname}Run, 2, 4, {k})," for k in range(len(RUN_FRAMES))]
            lines.append(f"static const struct SpriteFrameImage sOutfitPicTable_{cname}[] = {{\n" + "\n".join(frames) + "\n};")
            tables[(name, g)] = f"sOutfitPicTable_{cname}"
    lines.append("")
    lines.append("// The walking state's frames for each outfit (NULL: the player's own)")
    lines.append("static const struct SpriteFrameImage *const sOutfitPicTables[OUTFIT_COUNT][GENDER_COUNT] =\n{")
    for name, _ in OUTFITS:
        if (name, 0) in tables:
            lines.append(f"    [OUTFIT_{name}] = {{{tables[(name, 0)]}, {tables[(name, 1)]}}},")
    lines.append("};\n")
    lines.append("static const u8 *const sOutfitNames[OUTFIT_COUNT] =\n{")
    for name, _ in OUTFITS:
        lines.append(f'    [OUTFIT_{name}] = COMPOUND_STRING("{display(name)}"),')
    lines.append("};\n")
    open(os.path.join(ROOT, "src/data/outfits.h"), "w").write("\n".join(lines))
    # the wardrobe's list: every outfit the player has
    inc = ["@ Generated by tools/gym_hack/outfit_art.py: the wardrobe's list (every outfit the player has)",
           "Outfits_EventScript_PushList::"]
    for name, _ in OUTFITS:
        inc += [f"\tsetvar VAR_0x8004, OUTFIT_{name}", "\tspecial Outfit_CheckUnlocked",
                f"\tgoto_if_eq VAR_RESULT, FALSE, Outfits_EventScript_Skip_{name}",
                f"\tdynmultipush Outfits_Text_{name}, OUTFIT_{name}", f"Outfits_EventScript_Skip_{name}:"]
    inc.append("\treturn\n")
    for name, _ in OUTFITS:
        inc += [f"Outfits_Text_{name}:", f'\t.string "{display(name)}$"', ""]
    open(os.path.join(ROOT, "data/scripts/outfits_list.inc"), "w").write("\n".join(inc))
    print(f"outfits: {len(OUTFITS)}")
    if len(sys.argv) > 1:
        preview(sys.argv[1])


def recolor(path, pal, box=None):
    im = Image.open(os.path.join(ROOT, path))
    if box:
        im = im.crop(box)
    im = im.copy()
    im.putpalette([v for c in pal for v in c])
    rgba = im.convert("RGBA")
    px = rgba.load()
    src = im.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            if src[x, y] == 0:
                px[x, y] = (0, 0, 0, 0)
    return rgba


def preview(out):
    from PIL import ImageDraw
    P = "graphics/object_events/pics/people/"
    cw, ch = 330, 100
    cols = 2
    rows = (len(OUTFITS) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cw, rows * ch), (240, 240, 232, 255))
    d = ImageDraw.Draw(sheet)
    for n, (name, (boy, girl)) in enumerate(OUTFITS):
        x0, y0 = (n % cols) * cw, (n // cols) * ch
        d.text((x0 + 4, y0 + 2), name, fill=(0, 0, 0, 255))
        x = x0 + 4
        owp = overworld(name, (boy, girl))
        for pal, bpal, ow, back in ((owp[0], boy, "red/red_normal.png", "graphics/trainers/back_pics/red.png"),
                                    (owp[1], girl, "leaf/green_normal.png", "graphics/trainers/back_pics/leaf.png")):
            if name in MARKS:
                ow = f"outfits/{name.lower()}_{'boy' if 'red' in ow else 'girl'}_walk.png"
            for k in (0, 3, 2):
                fr = recolor(P + ow, pal, (k * 16, 0, k * 16 + 16, 32)).resize((32, 64), Image.NEAREST)
                sheet.alpha_composite(fr, (x, y0 + 16)); x += 30
            bk = recolor(back, bpal, (0, 0, 64, 64)).resize((80, 80), Image.NEAREST)
            sheet.alpha_composite(bk, (x - 6, y0 + 12)); x += 72
    sheet.save(out)


if __name__ == "__main__":
    main()
