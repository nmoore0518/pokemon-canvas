#!/usr/bin/env python3
"""The Gym Leader Convention's trainer sprites (Gym Trainer hack).

Imports each convention trainer's battle front pic and overworld sprite that the game doesn't
already have (tools/gym_hack/convention_roster.py) from Emerald Rogue (and Janine from Pokémon HnS):
  - front pics -> graphics/trainers/front_pics/convention/<key>.png (+ palettes/convention/<key>.pal),
    TRAINER_PIC_CONV_<KEY>;
  - overworld sprites -> graphics/object_events/pics/people/convention/<key>.png (walking frames
    made for the 3-frame ones, tools/gym_hack/make_walk_frames.py), OBJ_EVENT_GFX_CONV_<KEY>, each
    with its own palette (OBJ_EVENT_PAL_TAG_CONV_<KEY>, from 0x1300).
Then writes src/data/gym_convention_sprites.h: each trainer's pic and overworld graphics.
Reruns replace everything it made before.

    python3 tools/gym_hack/import_convention_sprites.py [rogue] [hns]
"""
import re
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from convention_roster import all_trainers, sprite_key  # noqa: E402
import make_walk_frames as walk  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ROGUE = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("/home/claude/ext/pokeemerald-rogue")
HNS = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("/home/claude/ext/pokehns-expansion")
PIC_DIR = ROOT / "graphics/trainers/front_pics/convention"
PAL_DIR = ROOT / "graphics/trainers/palettes/convention"
OW_DIR = ROOT / "graphics/object_events/pics/people/convention"
PAL_TAG_BASE = 0x1300
BEGIN, END = "// BEGIN Convention (tools/gym_hack/import_convention_sprites.py)\n", "// END Convention\n"


def camel(key):
    return "".join(p.capitalize() for p in key.split("_"))


def source(path, kind):
    if path.startswith("hns:"):
        base = HNS / ("graphics/trainers/front_pics" if kind == "pic" else "graphics/object_events/pics/people")
        return base / path[4:]
    return ROGUE / ("graphics/trainers/front_pics" if kind == "pic" else "graphics/object_events/pics") / path


def as_4bpp(im):
    """An indexed image with exactly a 16-color palette (index 0 = transparent)."""
    pal = (im.getpalette() or [])[:48]
    pal += [0] * (48 - len(pal))
    out = Image.new("P", im.size, 0)
    out.putpalette(pal + [0] * (768 - 48))
    out.putdata(list(im.getdata()))
    return out, pal


def write_jasc(path, pal):
    lines = ["JASC-PAL", "0100", "16"] + [f"{pal[i * 3]} {pal[i * 3 + 1]} {pal[i * 3 + 2]}" for i in range(16)]
    path.write_text("\r\n".join(lines) + "\r\n")


def nine_frames(im):
    if im.size[0] >= 9 * 16:
        return im
    down, up, left = (walk.frame(im, i) for i in range(3))
    frames = [down, up, left, walk.lift_foot(down, True), walk.lift_foot(down, False),
              walk.lift_foot(up, True), walk.lift_foot(up, False), walk.stride(left), walk.bob(left)]
    sheet = Image.new("P", (9 * 16, 32), 0)
    sheet.putpalette(im.getpalette())
    for i, f in enumerate(frames):
        sheet.paste(f, (i * 16, 0))
    return sheet


def replace_block(path, block, anchor=None, before=True, tag="Convention"):
    begin = BEGIN.replace("Convention", tag)
    end = END.replace("Convention", tag)
    text = path.read_text()
    if begin in text:
        a, b = text.index(begin), text.index(end) + len(end)
        text = text[:a] + begin + block + end + text[b:]
    else:
        i = text.index(anchor) if before else text.index(anchor) + len(anchor)
        text = text[:i] + begin + block + end + text[i:]
    path.write_text(text)


def main():
    for d in (PIC_DIR, PAL_DIR, OW_DIR):
        d.mkdir(parents=True, exist_ok=True)
        for f in d.glob("*"):
            f.unlink()
    pics, ows = {}, {}           # sprite key -> constant
    new_pics, new_ows = [], []
    for _, hall, key, name, role, female, pic, ow in all_trainers():
        sk = sprite_key(key)
        if sk not in pics:
            if pic.startswith("TRAINER_PIC_"):
                pics[sk] = pic
            else:
                im, pal = as_4bpp(Image.open(source(pic, "pic")))
                im.save(PIC_DIR / f"{sk}.png")
                write_jasc(PAL_DIR / f"{sk}.pal", pal)
                pics[sk] = f"TRAINER_PIC_CONV_{sk.upper()}"
                new_pics.append(sk)
        if sk not in ows:
            if ow.startswith("OBJ_EVENT_GFX_"):
                ows[sk] = ow
            else:
                im, _ = as_4bpp(nine_frames(Image.open(source(ow, "ow"))))
                im.save(OW_DIR / f"{sk}.png")
                ows[sk] = f"OBJ_EVENT_GFX_CONV_{sk.upper()}"
                new_ows.append(sk)

    # Trainer pics
    replace_block(ROOT / "include/constants/trainers.h",
                  "".join(f"    TRAINER_PIC_CONV_{k.upper()},\n" for k in new_pics), "    TRAINER_PIC_COUNT,")
    gfx = "".join(f'const u32 gTrainerFrontPic_Conv{camel(k)}[] = INCGFX_U32("graphics/trainers/front_pics/convention/{k}.png", ".4bpp.smol");\n'
                  f'const u16 gTrainerPalette_Conv{camel(k)}[] = INCGFX_U16("graphics/trainers/palettes/convention/{k}.pal", ".gbapal");\n'
                  for k in new_pics)
    table = "".join(f"    [TRAINER_PIC_CONV_{k.upper()}] =\n    {{\n        .frontPic = TRAINER_FRONT_PIC(gTrainerFrontPic_Conv{camel(k)}, gTrainerPalette_Conv{camel(k)}),\n    }},\n"
                    for k in new_pics)
    th = ROOT / "src/data/graphics/trainers.h"
    text = th.read_text()
    marker_gfx = "// BEGIN Convention gfx\n"
    if marker_gfx in text:
        a = text.index(marker_gfx)
        b = text.index("// END Convention gfx\n") + len("// END Convention gfx\n")
        text = text[:a] + text[b:]
    th.write_text(text)
    # (the pics go before the table; the table entries at its end)
    text = th.read_text()
    tstart = text.index("const struct TrainerPicInfo gTrainerPicInfo[TRAINER_PIC_COUNT]")
    text = text[:tstart] + marker_gfx + gfx + "// END Convention gfx\n\n" + text[tstart:]
    th.write_text(text)
    text = th.read_text()
    tstart = text.index("const struct TrainerPicInfo gTrainerPicInfo[TRAINER_PIC_COUNT]")
    if BEGIN in text[tstart:]:
        replace_block(th, table)
    else:
        close = text.index("\n};", tstart) + 1
        th.write_text(text[:close] + BEGIN + table + END + text[close:])

    # Overworld sprites
    replace_block(ROOT / "include/constants/event_objects.h",
                  "".join(f"    OBJ_EVENT_GFX_CONV_{k.upper()},\n" for k in new_ows), "    NUM_OBJ_EVENT_GFX,")
    tags = "".join(f"#define OBJ_EVENT_PAL_TAG_CONV_{k.upper()} {PAL_TAG_BASE + i:#06x}\n" for i, k in enumerate(new_ows))
    replace_block(ROOT / "include/constants/event_objects.h", tags, "#define OBJ_EVENT_PAL_TAG_WHITE", tag="Convention palette tags")
    og = ROOT / "src/data/object_events/object_event_graphics.h"
    block = "".join(f'const u16 gObjectEventPic_Conv{camel(k)}[] = INCGFX_U16("graphics/object_events/pics/people/convention/{k}.png", ".4bpp", "-mwidth 2 -mheight 4");\n'
                    f'const u16 gObjectEventPal_Conv{camel(k)}[] = INCGFX_U16("graphics/object_events/pics/people/convention/{k}.png", ".gbapal");\n'
                    for k in new_ows)
    t = og.read_text()
    og.write_text(t[:t.index(BEGIN)] + BEGIN + block + END + t[t.index(END) + len(END):] if BEGIN in t else t.rstrip("\n") + "\n\n" + BEGIN + block + END)
    pt = ROOT / "src/data/object_events/object_event_pic_tables.h"
    block = "".join(f"static const struct SpriteFrameImage sPicTable_Conv{camel(k)}[] = {{\n"
                    f"    overworld_ascending_frames(gObjectEventPic_Conv{camel(k)}, 2, 4),\n}};\n\n" for k in new_ows)
    t = pt.read_text()
    pt.write_text(t[:t.index(BEGIN)] + BEGIN + block + END + t[t.index(END) + len(END):] if BEGIN in t else t.rstrip("\n") + "\n\n" + BEGIN + block + END)
    gi = ROOT / "src/data/object_events/object_event_graphics_info.h"
    block = "".join(f"""const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_Conv{camel(k)} = {{
    .tileTag = TAG_NONE,
    .paletteTag = OBJ_EVENT_PAL_TAG_CONV_{k.upper()},
    .reflectionPaletteTag = OBJ_EVENT_PAL_TAG_NONE,
    .size = 256,
    .width = 16,
    .height = 32,
    .paletteSlot = PALSLOT_NPC_1,
    .shadowSize = SHADOW_SIZE_M,
    .inanimate = FALSE,
    .compressed = FALSE,
    .tracks = TRACKS_FOOT,
    .oam = &gObjectEventBaseOam_16x32,
    .subspriteTables = sOamTables_16x32,
    .anims = sAnimTable_Standard,
    .images = sPicTable_Conv{camel(k)},
    .affineAnims = gDummySpriteAffineAnimTable,
}};

""" for k in new_ows)
    t = gi.read_text()
    gi.write_text(t[:t.index(BEGIN)] + BEGIN + block + END + t[t.index(END) + len(END):] if BEGIN in t else t.rstrip("\n") + "\n\n" + BEGIN + block + END)
    gp = ROOT / "src/data/object_events/object_event_graphics_info_pointers.h"
    t = gp.read_text()
    ext = "".join(f"extern const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_Conv{camel(k)};\n" for k in new_ows)
    ent = "".join(f"    [OBJ_EVENT_GFX_CONV_{k.upper()}] = &gObjectEventGraphicsInfo_Conv{camel(k)},\n" for k in new_ows)
    B2, E2 = BEGIN.replace("Convention", "Convention externs"), END.replace("Convention", "Convention externs")
    if B2 in t:
        t = t[:t.index(B2)] + B2 + ext + E2 + t[t.index(E2) + len(E2):]
    else:
        i = t.index("const struct ObjectEventGraphicsInfo *const gObjectEventGraphicsInfoPointers")
        t = t[:i] + B2 + ext + E2 + "\n" + t[i:]
    gp.write_text(t)
    t = gp.read_text()
    start = t.index("const struct ObjectEventGraphicsInfo *const gObjectEventGraphicsInfoPointers")
    if BEGIN in t[start:]:
        a = t.index(BEGIN, start)
        b = t.index(END, a) + len(END)
        t = t[:a] + BEGIN + ent + END + t[b:]
    else:
        close = t.index("\n};", start) + 1
        t = t[:close] + BEGIN + ent + END + t[close:]
    gp.write_text(t)
    em = ROOT / "src/event_object_movement.c"
    t = em.read_text()
    ent = "".join(f"    {{gObjectEventPal_Conv{camel(k)}, OBJ_EVENT_PAL_TAG_CONV_{k.upper()}}},\n" for k in new_ows)
    if BEGIN in t:
        t = t[:t.index(BEGIN)] + BEGIN + ent + END + t[t.index(END) + len(END):]
    else:
        i = t.index("    {gObjectEventPaletteNeonLight,          OBJ_EVENT_PAL_TAG_NEON_LIGHT},\n")
        i += len("    {gObjectEventPaletteNeonLight,          OBJ_EVENT_PAL_TAG_NEON_LIGHT},\n")
        t = t[:i] + BEGIN + ent + END + t[i:]
    em.write_text(t)

    # Each trainer's sprites, for src/gym_convention.c
    lines = ["// Generated by tools/gym_hack/import_convention_sprites.py: each convention trainer's sprites\n",
             "// (in convention order: tools/gym_hack/convention_roster.py)\n",
             "static const u16 sConventionPics[] = {\n"]
    lines += [f"    {pics[sprite_key(t[2])]},\n" for t in all_trainers()]
    lines += ["};\n\nstatic const u16 sConventionGfx[] = {\n"]
    lines += [f"    {ows[sprite_key(t[2])]},\n" for t in all_trainers()]
    lines += ["};\n"]
    (ROOT / "src/data/gym_convention_sprites.h").write_text("".join(lines))
    print(f"convention sprites: {len(new_pics)} front pics, {len(new_ows)} overworld sprites imported")


if __name__ == "__main__":
    main()
