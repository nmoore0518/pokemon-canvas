#!/usr/bin/env python3
"""The Gauntlet (Gym Trainer hack): its challengers' sprites and data, from gauntlet_roster.py.

  - front pics: from Emerald Rogue / HnS (already GBA: 64x64, 16 colors), or smogon/sprites and
    PokéRogue (larger, many colors: scaled to fit 64x64 and reduced to 15 colors) ->
    graphics/trainers/front_pics/gauntlet/<key>.png, TRAINER_PIC_GAUNTLET_<KEY>;
  - overworld sprites given as "rogue:<path>" -> graphics/object_events/pics/people/gauntlet/<key>.png,
    OBJ_EVENT_GFX_GAUNTLET_<KEY>;
  - src/data/gauntlet.h: each challenger's party (level 100, full IVs, EVs by what the moves are),
    name, class, pics, title card, lines; the regions.
Checks species, moves, items, and that a team has no more than two legendaries.

    python3 tools/gym_hack/build_gauntlet.py [--no-sprites]
"""
import json
import re
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
import import_convention_sprites as conv  # noqa: E402
from build_convention import const_name, known, move_categories, wrap  # noqa: E402
from gauntlet_roster import BILL, ROSTER  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
EXT = Path("/home/claude/ext")
SOURCES = {"rogue": EXT / "pokeemerald-rogue/graphics/trainers/front_pics",
           "hns": EXT / "pokehns-expansion/graphics/trainers/front_pics",
           "smogon": EXT / "smogon_sprites",
           "pr": EXT / "pokerogue_assets/images/trainer",
           "art": ROOT / "tools/gym_hack/art"}         # (gauntlet_custom.py's repaints)
PIC_DIR = ROOT / "graphics/trainers/front_pics/gauntlet"
PAL_DIR = ROOT / "graphics/trainers/palettes/gauntlet"
OW_DIR = ROOT / "graphics/object_events/pics/people/gauntlet"
PAL_TAG_BASE = 0x1380
REGIONS = ["KANTO", "JOHTO", "HOENN", "SINNOH", "UNOVA", "KALOS", "ALOLA", "GALAR", "PALDEA"]


def load_pic(src):
    kind, path = src.split(":", 1)
    if kind == "pr":
        j = json.loads((SOURCES["pr"] / f"{path}.json").read_text())
        frames = j["textures"][0]["frames"] if "textures" in j else j["frames"]
        first = frames[0] if isinstance(frames, list) else frames[sorted(frames)[0]]
        f = first["frame"]
        im = Image.open(SOURCES["pr"] / f"{path}.png").convert("RGBA")
        return im.crop((f["x"], f["y"], f["x"] + f["w"], f["y"] + f["h"]))
    im = Image.open(SOURCES[kind] / path)
    pal = SOURCES[kind].parent / "palettes" / (Path(path).stem.replace("_front_pic", "") + ".pal")
    if kind in ("rogue", "hns") and pal.exists():      # (the picture's own palette is a placeholder)
        lines = pal.read_text().split()[3:]
        im.putpalette([int(x) for x in lines[:48]])
    return im


def to_gba_pic(im):
    """A 64x64, 16-color picture (index 0 transparent), the figure at the bottom middle"""
    if im.mode == "P" and im.size == (64, 64) and len((im.getpalette() or [])) // 3 <= 16:
        return conv.as_4bpp(im)
    im = im.convert("RGBA")
    if im.mode == "RGBA":
        # (a palette image's index 0 is its transparent color: keep that as transparency)
        pass
    bbox = im.getchannel("A").point(lambda a: 255 if a > 127 else 0).getbbox()
    if bbox:
        im = im.crop(bbox)
    w, h = im.size
    s = min(64 / w, 64 / h, 1.0)
    if s < 1:
        im = im.resize((max(1, round(w * s)), max(1, round(h * s))), Image.LANCZOS)
    canvas = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    canvas.paste(im, ((64 - im.width) // 2, 64 - im.height), im)
    alpha = canvas.getchannel("A")
    rgb = Image.new("RGB", (64, 64), (0, 0, 0))
    rgb.paste(canvas.convert("RGB"), mask=alpha.point(lambda a: 255 if a > 127 else 0))
    opaque = [p for p, a in zip(rgb.get_flattened_data(), alpha.get_flattened_data()) if a > 127]
    sample = Image.new("RGB", (len(opaque), 1))
    sample.putdata(opaque)
    q = sample.quantize(colors=15, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[:45]
    pal = [255, 0, 255] + pal + [0] * (45 - len(pal))
    palimg = Image.new("P", (1, 1))
    palimg.putpalette(pal + [0] * (768 - len(pal)))
    full = rgb.quantize(palette=palimg, dither=Image.Dither.NONE)
    data = [0 if a <= 127 else (v if v != 0 else 1) for v, a in zip(full.get_flattened_data(), alpha.get_flattened_data())]
    # (an opaque pixel can't be index 0: the nearest of the others)
    out = Image.new("P", (64, 64), 0)
    out.putpalette(pal + [0] * (768 - len(pal)))
    out.putdata(data)
    return out, pal[:48]


def class_for(key, t):
    title = t["title"]
    if "ROCKET" in title:
        return "TRAINER_CLASS_TEAM_ROCKET_FRLG"
    if "MAGMA LEADER" in title:
        return "TRAINER_CLASS_MAGMA_LEADER"
    if "MAGMA" in title:
        return "TRAINER_CLASS_MAGMA_ADMIN"
    if "AQUA LEADER" in title:
        return "TRAINER_CLASS_AQUA_LEADER"
    if "AQUA" in title:
        return "TRAINER_CLASS_AQUA_ADMIN"
    if "PROFESSOR" in title:
        return "TRAINER_CLASS_PKMN_PROF_FRLG"
    return {"noland": "TRAINER_CLASS_FACTORY_HEAD", "greta": "TRAINER_CLASS_ARENA_TYCOON", "tucker": "TRAINER_CLASS_DOME_ACE",
            "lucy": "TRAINER_CLASS_PIKE_QUEEN", "brandon": "TRAINER_CLASS_PYRAMID_KING",
            "spenser": "TRAINER_CLASS_PALACE_MAVEN", "anabel": "TRAINER_CLASS_SALON_MAIDEN",
            "bill": "TRAINER_CLASS_POKEMANIAC"}.get(key, "TRAINER_CLASS_PKMN_TRAINER_1")


def legendaries():
    """species constants flagged legendary, mythical or Ultra Beast"""
    text = (ROOT / "src/data/pokemon/species_info").glob("*.h")
    out = set()
    for f in text:
        for m in re.finditer(r"\[(SPECIES_\w+)\]\s*=\s*\{(.*?)\n    \},", f.read_text(), re.S):
            if re.search(r"\.is(Legendary|Mythical|UltraBeast|SubLegendary|RestrictedLegendary|FrontierBanned)\s*=\s*TRUE", m.group(2)):
                out.add(m.group(1))
    return out


def import_sprites():
    for d in (PIC_DIR, PAL_DIR, OW_DIR):
        d.mkdir(parents=True, exist_ok=True)
        for f in d.glob("*"):
            f.unlink()
    pics, ows, new_pics, new_ows = {}, {}, [], []
    for key, t in ROSTER.items():
        if t["pic"].startswith("TRAINER_PIC_"):
            pics[key] = t["pic"]
        else:
            im, pal = to_gba_pic(load_pic(t["pic"]))
            im.save(PIC_DIR / f"{key}.png")
            conv.write_jasc(PAL_DIR / f"{key}.pal", pal)
            pics[key] = f"TRAINER_PIC_GAUNTLET_{key.upper()}"
            new_pics.append(key)
        if t["ow"].startswith("rogue:"):
            src = EXT / "pokeemerald-rogue/graphics/object_events/pics" / t["ow"][6:]
            im, _ = conv.as_4bpp(conv.nine_frames(Image.open(src)))
            im.save(OW_DIR / f"{key}.png")
            ows[key] = f"OBJ_EVENT_GFX_GAUNTLET_{key.upper()}"
            new_ows.append(key)
        else:
            ows[key] = t["ow"]
    # The hooded figure (tools/gym_hack/gauntlet_art.py); Bill, as he is now, and his Palm Pilot
    # (bill_ow.py); his picture (bill_art.py: CLEFFABILL's front)
    import bill_art
    import bill_ow
    import gauntlet_art
    for key, sheet in (("hooded_figure", gauntlet_art.sheet()), ("bill", bill_ow.sheet()),
                       ("palm_pilot", bill_ow.pilot())):
        im, _ = conv.as_4bpp(conv.nine_frames(sheet))
        im.save(OW_DIR / f"{key}.png")
        new_ows.append(key)
    pic = bill_art.indexed(bill_art.front(0))
    pic.save(PIC_DIR / "bill.png")
    conv.write_jasc(PAL_DIR / "bill.pal", [v for c in bill_art.COLORS for v in c])
    new_pics.append("bill")
    write_sprite_tables(new_pics, new_ows)
    return pics, ows


def camel(key):
    return "".join(p.capitalize() for p in key.split("_"))


def write_sprite_tables(new_pics, new_ows):
    """The same tables as the convention's sprites (import_convention_sprites.py), tagged Gauntlet"""
    B, E = "// BEGIN Gauntlet (tools/gym_hack/build_gauntlet.py)\n", "// END Gauntlet\n"

    def put(path, block, anchor, before=True, tag="Gauntlet"):
        b, e = B.replace("Gauntlet", tag, 1), E.replace("Gauntlet", tag)
        text = path.read_text()
        if b in text:
            text = text[:text.index(b)] + b + block + e + text[text.index(e) + len(e):]
        else:
            i = text.index(anchor) if before else text.index(anchor) + len(anchor)
            text = text[:i] + b + block + e + text[i:]
        path.write_text(text)

    put(ROOT / "include/constants/trainers.h", "".join(f"    TRAINER_PIC_GAUNTLET_{k.upper()},\n" for k in new_pics), "    TRAINER_PIC_COUNT,")
    th = ROOT / "src/data/graphics/trainers.h"
    put(th, "".join(f'const u32 gTrainerFrontPic_Gauntlet{camel(k)}[] = INCGFX_U32("graphics/trainers/front_pics/gauntlet/{k}.png", ".4bpp.smol");\n'
                    f'const u16 gTrainerPalette_Gauntlet{camel(k)}[] = INCGFX_U16("graphics/trainers/palettes/gauntlet/{k}.pal", ".gbapal");\n'
                    for k in new_pics), "const struct TrainerPicInfo gTrainerPicInfo[TRAINER_PIC_COUNT]", tag="Gauntlet gfx")
    text = th.read_text()
    tstart = text.index("const struct TrainerPicInfo gTrainerPicInfo[TRAINER_PIC_COUNT]")
    close = text.index("\n};", tstart) + 1
    table = "".join(f"    [TRAINER_PIC_GAUNTLET_{k.upper()}] =\n    {{\n        .frontPic = TRAINER_FRONT_PIC(gTrainerFrontPic_Gauntlet{camel(k)}, gTrainerPalette_Gauntlet{camel(k)}),\n    }},\n"
                    for k in new_pics)
    if B.replace("Gauntlet", "Gauntlet table", 1) in text:
        put(th, table, None, tag="Gauntlet table")
    else:
        th.write_text(text[:close] + B.replace("Gauntlet", "Gauntlet table", 1) + table + E.replace("Gauntlet", "Gauntlet table") + text[close:])
    # Overworld sprites
    put(ROOT / "include/constants/event_objects.h", "".join(f"    OBJ_EVENT_GFX_GAUNTLET_{k.upper()},\n" for k in new_ows), "    NUM_OBJ_EVENT_GFX,")
    put(ROOT / "include/constants/event_objects.h",
        "".join(f"#define OBJ_EVENT_PAL_TAG_GAUNTLET_{k.upper()} {PAL_TAG_BASE + i:#06x}\n" for i, k in enumerate(new_ows)),
        "#define OBJ_EVENT_PAL_TAG_WHITE", tag="Gauntlet palette tags")
    og = ROOT / "src/data/object_events/object_event_graphics.h"
    blk = "".join(f'const u16 gObjectEventPic_Gauntlet{camel(k)}[] = INCGFX_U16("graphics/object_events/pics/people/gauntlet/{k}.png", ".4bpp", "-mwidth 2 -mheight 4");\n'
                  f'const u16 gObjectEventPal_Gauntlet{camel(k)}[] = INCGFX_U16("graphics/object_events/pics/people/gauntlet/{k}.png", ".gbapal");\n'
                  for k in new_ows)
    t = og.read_text()
    og.write_text(t[:t.index(B)] + B + blk + E + t[t.index(E) + len(E):] if B in t else t.rstrip("\n") + "\n\n" + B + blk + E)
    pt = ROOT / "src/data/object_events/object_event_pic_tables.h"
    blk = "".join(f"static const struct SpriteFrameImage sPicTable_Gauntlet{camel(k)}[] = {{\n"
                  f"    overworld_ascending_frames(gObjectEventPic_Gauntlet{camel(k)}, 2, 4),\n}};\n\n" for k in new_ows)
    t = pt.read_text()
    pt.write_text(t[:t.index(B)] + B + blk + E + t[t.index(E) + len(E):] if B in t else t.rstrip("\n") + "\n\n" + B + blk + E)
    gi = ROOT / "src/data/object_events/object_event_graphics_info.h"
    blk = "".join(f"""const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_Gauntlet{camel(k)} = {{
    .tileTag = TAG_NONE,
    .paletteTag = OBJ_EVENT_PAL_TAG_GAUNTLET_{k.upper()},
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
    .images = sPicTable_Gauntlet{camel(k)},
    .affineAnims = gDummySpriteAffineAnimTable,
}};

""" for k in new_ows)
    t = gi.read_text()
    gi.write_text(t[:t.index(B)] + B + blk + E + t[t.index(E) + len(E):] if B in t else t.rstrip("\n") + "\n\n" + B + blk + E)
    gp = ROOT / "src/data/object_events/object_event_graphics_info_pointers.h"
    t = gp.read_text()
    ext = "".join(f"extern const struct ObjectEventGraphicsInfo gObjectEventGraphicsInfo_Gauntlet{camel(k)};\n" for k in new_ows)
    ent = "".join(f"    [OBJ_EVENT_GFX_GAUNTLET_{k.upper()}] = &gObjectEventGraphicsInfo_Gauntlet{camel(k)},\n" for k in new_ows)
    B2, E2 = B.replace("Gauntlet", "Gauntlet externs", 1), E.replace("Gauntlet", "Gauntlet externs")
    if B2 in t:
        t = t[:t.index(B2)] + B2 + ext + E2 + t[t.index(E2) + len(E2):]
    else:
        i = t.index("const struct ObjectEventGraphicsInfo *const gObjectEventGraphicsInfoPointers")
        t = t[:i] + B2 + ext + E2 + "\n" + t[i:]
    start = t.index("const struct ObjectEventGraphicsInfo *const gObjectEventGraphicsInfoPointers")
    if B in t[start:]:
        a = t.index(B, start)
        b = t.index(E, a) + len(E)
        t = t[:a] + B + ent + E + t[b:]
    else:
        close = t.index("\n};", start) + 1
        t = t[:close] + B + ent + E + t[close:]
    gp.write_text(t)
    em = ROOT / "src/event_object_movement.c"
    t = em.read_text()
    ent = "".join(f"    {{gObjectEventPal_Gauntlet{camel(k)}, OBJ_EVENT_PAL_TAG_GAUNTLET_{k.upper()}}},\n" for k in new_ows)
    if B in t:
        t = t[:t.index(B)] + B + ent + E + t[t.index(E) + len(E):]
    else:
        anchor = "    {gObjectEventPaletteNeonLight,          OBJ_EVENT_PAL_TAG_NEON_LIGHT},\n"
        i = t.index(anchor) + len(anchor)
        t = t[:i] + B + ent + E + t[i:]
    em.write_text(t)


def evs(kinds):
    phys, spec = kinds.count("PHYSICAL"), kinds.count("SPECIAL")
    if phys == 0 and spec == 0:
        return "TRAINER_PARTY_EVS(252, 0, 128, 0, 0, 128)", "NATURE_CALM"
    if phys >= spec:
        return "TRAINER_PARTY_EVS(4, 252, 0, 252, 0, 0)", "NATURE_JOLLY" if phys > spec + 1 else "NATURE_ADAMANT"
    return "TRAINER_PARTY_EVS(4, 0, 0, 252, 252, 0)", "NATURE_TIMID" if spec > phys + 1 else "NATURE_MODEST"


ABILITIES = None


def party(key, t, species, moves, items, cats, legends, errors):
    """A team's TrainerMons: "[Shiny ]Species @ Item {Ability}: Move / Move / Move / Move" """
    global ABILITIES
    if ABILITIES is None:
        ABILITIES = known("include/constants/abilities.h", "ABILITY")
    mons, nlegend = [], 0
    for line in t["team"]:
        m = re.match(r"^(Shiny\s+)?(.*?)(?:\s*@\s*(.*?))?(?:\s*\{(.*?)\})?:\s*(.*)$", line)
        shiny, sp, item, ability = bool(m.group(1)), m.group(2).strip(), (m.group(3) or "").strip(), m.group(4)
        mv = [x.strip() for x in m.group(5).split("/")]
        sc = const_name("SPECIES", sp)
        if sc not in species:
            errors.append(f"{key}: species {sp} ({sc})")
        nlegend += sc in legends
        ic = const_name("ITEM", item) if item else "ITEM_NONE"
        if item and ic not in items:
            errors.append(f"{key}: item {item} ({ic})")
        ac = ""
        if ability:
            ac = const_name("ABILITY", ability)
            if ac not in ABILITIES:
                errors.append(f"{key}: ability {ability} ({ac})")
            ac = f"        .ability = {ac},\n"
        mcs = []
        for x in mv:
            mc = const_name("MOVE", x)
            if mc not in moves:
                errors.append(f"{key}: move {x} ({mc})")
            mcs.append(mc)
        ev, nature = evs([cats.get(c, "STATUS") for c in mcs])
        mons.append(f"    {{\n        .species = {sc},\n        .lvl = 100,\n        .heldItem = {ic},\n{ac}"
                    f"        .moves = {{{', '.join(mcs)}}},\n        .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31),\n"
                    f"        .ev = {ev},\n        .nature = {nature},\n        .gender = TRAINER_MON_RANDOM_GENDER,\n"
                    + ("        .isShiny = TRUE,\n" if shiny else "")
                    + "        .ball = POKEBALL_COUNT,\n        .friendship = MAX_FRIENDSHIP,\n    },\n")
    return mons, nlegend


def main():
    species = known("include/constants/species.h", "SPECIES")
    moves = known("include/constants/moves.h", "MOVE")
    items = known("include/constants/items.h", "ITEM")
    cats = move_categories()
    legends = legendaries()
    errors = []
    if "--no-sprites" in sys.argv:
        pics = {k: (t["pic"] if t["pic"].startswith("TRAINER_PIC_") else f"TRAINER_PIC_GAUNTLET_{k.upper()}") for k, t in ROSTER.items()}
        ows = {k: (t["ow"] if not t["ow"].startswith("rogue:") else f"OBJ_EVENT_GFX_GAUNTLET_{k.upper()}") for k, t in ROSTER.items()}
    else:
        pics, ows = import_sprites()
    parties, entries = [], []
    order = sorted(ROSTER, key=lambda k: REGIONS.index(ROSTER[k]["region"]))   # (stable: roster order within a region)
    pics["bill"], ows["bill"] = "TRAINER_PIC_GAUNTLET_BILL", "OBJ_EVENT_GFX_GAUNTLET_BILL"
    for idx, key in enumerate(order + ["bill"]):
        t = ROSTER[key] if key != "bill" else BILL
        if t["region"] not in REGIONS:
            errors.append(f"{key}: region {t['region']}")
        if len(t["team"]) != 6:
            errors.append(f"{key}: team of {len(t['team'])}")
        mons, nlegend = party(key, t, species, moves, items, cats, legends, errors)
        if nlegend > 2 and key != "bill":
            errors.append(f"{key}: {nlegend} legendaries")
        parties.append(f"static const struct TrainerMon sGauntletParty_{key}[] = {{\n" + "".join(mons) + "};\n\n")
        intro, defeat, after, win = (wrap(x) for x in t["lines"])
        for field, val in (("title", t["title"]), ("games", t["games"])):
            if len(val) > 30:
                errors.append(f"{key}: {field} too long: {val}")
        battle = t.get("battle", t["name"])
        if len(battle) > 10:
            errors.append(f"{key}: battle name too long: {battle}")
        entries.append(f"    {{ // {idx}\n        .name = COMPOUND_STRING(\"{t['name']}\"),\n"
                       f"        .battleName = COMPOUND_STRING(\"{battle}\"),\n"
                       f"        .trainerClass = {class_for(key, t)},\n        .female = {'TRUE' if t['female'] else 'FALSE'},\n"
                       f"        .region = {REGIONS.index(t['region'])},\n        .tier = {t.get('tier', 2)},\n        .pic = {pics[key]},\n        .gfx = {ows[key]},\n"
                       f"        .party = sGauntletParty_{key},\n"
                       f"        .title = COMPOUND_STRING(\"{t['title']}\"),\n        .games = COMPOUND_STRING(\"{t['games']}\"),\n"
                       f'        .intro = COMPOUND_STRING("{intro}"),\n        .defeat = COMPOUND_STRING("{defeat}"),\n'
                       f'        .after = COMPOUND_STRING("{after}"),\n        .win = COMPOUND_STRING("{win}"),\n    }},\n')
    if errors:
        print("\n".join(errors))
        sys.exit(1)
    regions = "".join(f'    COMPOUND_STRING("{r}"),\n' for r in REGIONS)
    (ROOT / "src/data/gauntlet.h").write_text(
        "// Generated by tools/gym_hack/build_gauntlet.py from tools/gym_hack/gauntlet_roster.py: the Gauntlet's\n"
        "// challengers, in the order they come\n\n" + "".join(parties)
        + f"#define GAUNTLET_COUNT {len(order)}\n#define GAUNTLET_REGION_COUNT {len(REGIONS)}\n\n"
        + "static const u8 *const sGauntletRegions[GAUNTLET_REGION_COUNT] = {\n" + regions + "};\n\n"
        + "static const struct GauntletTrainer sGauntletTrainers[GAUNTLET_COUNT + 1] = {\n" + "".join(entries[:-1])
        + "    // (the last: Bill, who hired the hooded figure... and was it all along)\n" + entries[-1] + "};\n")
    print(f"gauntlet: {len(order)} challengers")


if __name__ == "__main__":
    main()
