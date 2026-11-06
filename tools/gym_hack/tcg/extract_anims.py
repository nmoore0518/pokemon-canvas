#!/usr/bin/env python3
"""Pokémon Canvas: the Trading Card Game duel's animations, from the pret/poketcg disassembly, as the
game has them (src/tcg/anims.c plays them as its engine did).

    python3 tools/gym_hack/tcg/extract_anims.py <path to a pret/poketcg checkout>

Writes src/data/tcg/anims.h:
  gTcgDuelAnimations[DUEL_ANIM_*]      (data/duel/animations/duel_animations.asm) sprite, palette,
                                       sprite animation, flags, sfx, handler
  gTcgAttackAnimations[ATK_ANIM_*]     (attack_animations.asm) each attack's animation commands
  gTcgSprites[SPRITE_*]                (engine/gfx/sprites.asm) the duel's sprites' tiles (2bpp)
  gTcgSpriteAnims[SPRITE_ANIM_*]       (data/sprite_animation_pointers.asm, anims1-4.asm) each
                                       animation's frame table and frame data (frame, count, dx, dy)
  gTcgPalettes[PALETTE_*]              (data/palette_pointers.asm, palettes1-2.asm) the CGB palettes
Only what the duel uses is kept (the duel's sprites and the animations they're shown with); the
rest of the tables are NULL.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import extract_gfx  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
SRC = Path(sys.argv[1]) / "src"
OAM = {"OAM_XFLIP": 0x20, "OAM_YFLIP": 0x40, "OAM_PRIO": 0x80, "OAM_PAL1": 0x10}


def strip(line):
    return line.split(";")[0].strip()


def num(tok):
    tok = tok.strip()
    if not tok:
        return 0
    expr = tok
    for k, v in OAM.items():
        expr = expr.replace(k, str(v))
    expr = re.sub(r"\$([0-9a-fA-F]+)", r"0x\1", expr)
    expr = re.sub(r"%([01]+)", r"0b\1", expr)
    if re.fullmatch(r"[\d\sxa-fA-F()+\-*|<>&]+", expr):
        return eval(expr) & 0xFF
    return "TCG_" + tok if tok != "NONE" else 0


def cval(v):
    return f"0x{v:02X}" if isinstance(v, int) else v


def parse_anim_files():
    """{global label: [(kind, data)]}, local labels as global.local"""
    blocks, cur, glob = {}, None, None
    for f in sorted((SRC / "data/duel/animations").glob("anims*.asm")) + [SRC / "gfx.asm"]:
        for line in f.read_text().splitlines():
            s = strip(line)
            if not s:
                continue
            m = re.match(r"^(\.\w+):*$", s) or re.match(r"^(\w+):+$", s)
            if m:
                lab = m[1]
                if lab.startswith("."):
                    lab = glob + lab
                else:
                    glob = lab
                cur = lab
                blocks[cur] = []
                continue
            m = re.match(r"(frame_table|frame_data|dw|db)\s+(.*)", s)
            if m and cur:
                args = [a.strip() for a in m[2].split(",")]
                if m[1] == "dw":
                    args = [glob + a if a.startswith(".") else a for a in args]
                blocks[cur].append((m[1], args))
    return blocks


def table(path, macro, nargs=1):
    out = []
    for line in (SRC / path).read_text().splitlines():
        m = re.match(r"\s*" + macro + r"\s+(.*)", strip(line))
        if m:
            out.append([a.strip() for a in m[1].split(",")])
    return out


def palettes():
    """{label: [CGB palettes, 4 colours each]}"""
    pals, cur, data = {}, None, []
    for f in ("data/palettes1.asm", "data/palettes2.asm", "gfx.asm"):
        for line in (SRC / f).read_text().splitlines():
            s = strip(line)
            m = re.match(r"^(\w+):+$", s)
            if m:
                cur = m[1]
                pals[cur] = []
                continue
            if cur is None:
                continue
            m = re.match(r"db\s+(.*)", s)
            if m:
                pals[cur].append(("db", [x.strip() for x in m[1].split(",")]))
            m = re.match(r"rgb\s+(\d+),\s*(\d+),\s*(\d+)", s)
            if m:
                r, g, b = int(m[1]), int(m[2]), int(m[3])
                pals[cur].append(("rgb", r | g << 5 | b << 10))
    out = {}
    for lab, items in pals.items():
        # (db the DMG palettes' count, then them; db the CGB palettes' count, then their colours)
        it = iter(items)
        try:
            k, v = next(it)     # (the DMG palettes' count: their gbpal lines aren't kept)
            k, v = next(it)
            ncgb = int(v[0])
            cols = [c for k, c in it if k == "rgb"][:ncgb * 4]
            out[lab] = cols
        except (StopIteration, ValueError):
            out[lab] = []
    return out


def main():
    blocks = parse_anim_files()
    duel_anims = []
    rows, cur = [], []
    for line in (SRC / "data/duel/animations/duel_animations.asm").read_text().splitlines():
        m = re.match(r"db\s+(.+)", strip(line))
        if m:
            cur.append(re.sub(r"\s+", " ", m[1].strip()))
            if len(cur) == 6:
                duel_anims.append(cur)
                cur = []
    sprites = table("engine/gfx/sprites.asm", "gfx_pointer")
    anim_ptrs = table("data/sprite_animation_pointers.asm", "anim_data_pointer")
    pal_ptrs = table("data/palette_pointers.asm", "palette_pointer")
    pals = palettes()

    # the duel's sprites (SPRITE_DUEL_*): their indexes in the Sprites table
    st = (SRC / "constants/sprite_constants.asm").read_text()
    st = st[st.index("const SPRITE_OW_PLAYER"):]  # (the file's first const_defs are SPRITE_ANIM_*'s fields)
    sprite_names = re.findall(r"const (SPRITE_\w+)", st.split("const_def")[0])
    duel_sprites = {i for i, n in enumerate(sprite_names) if n.startswith("SPRITE_DUEL")}
    # the animations to keep: the duel animations', the damage numbers' (SPRITE_ANIM_79-91), the
    # coin's and the duel's other code's (all SPRITE_ANIM_* named in engine/duel and home)
    anim_names = re.findall(r"const (SPRITE_ANIM_\w+)", (SRC / "constants/sprite_anim_constants.asm").read_text()) \
        if (SRC / "constants/sprite_anim_constants.asm").exists() else None
    used_anims = {a[2] for a in duel_anims if a[2] != "NONE"}
    code = "".join(p.read_text() for d in ("engine/duel", "home", "engine/menus") for p in (SRC / d).rglob("*.asm"))
    used_anims |= set(re.findall(r"\b(SPRITE_ANIM_\d+|SPRITE_ANIM_[A-Z_]+\d*)\b", code))
    used_pals = {a[1] for a in duel_anims if a[1] != "NONE"} | set(re.findall(r"\b(PALETTE_\w+)\b", code))
    used_pals |= {"PALETTE_PLAYER_PIC", "PALETTE_ROD_PIC"}    # (the duelists' portraits: data/duel/portraits.asm)
    pal_names = re.findall(r"const (PALETTE_\w+)", (SRC / "constants/palette_constants.asm").read_text())
    at = (SRC / "constants/animation_constants.asm").read_text()
    at = at[at.index("const SPRITE_ANIM_LIGHT_NPC_UP"):]
    anim_const_names = re.findall(r"const (SPRITE_ANIM_\w+)", at.split("const_def")[0])

    out = ["// Generated by tools/gym_hack/tcg/extract_anims.py (from pret/poketcg): the duel's animations",
           "// as the Game Boy Color game has them (src/tcg/anims.c).", ""]
    # sprites' tiles
    gfx_labels = {}
    for i, s in enumerate(sprites):
        if i in duel_sprites:
            gfx_labels[i] = (s[0], int(s[1].lstrip("$"), 16))
    sec = {}
    cur_label = None
    for line in (SRC / "gfx.asm").read_text().splitlines():
        s = strip(line)
        m = re.match(r"(\w+)::", s)
        if m:
            cur_label = m[1]
            continue
        m = re.match(r'INCBIN "([^"]+)"', s)
        if m and cur_label in {g[0] for g in gfx_labels.values()} | {"PlayerGfx", "RodGfx"}:
            sec.setdefault(cur_label, []).extend(extract_gfx.incbin(m[1]))
    for i, (lab, n) in gfx_labels.items():
        d = sec[lab][:n * 16]
        out.append(f"static const u8 sTcgSprite_{lab}[] =")
        out.append("{")
        for k in range(0, len(d), 32):
            out.append("    " + ", ".join("0x%02X" % b for b in d[k:k + 32]) + ",")
        out.append("};")
    out.append("")
    # the duelists' portraits (TILESET_PLAYER, TILESET_ROD: 6x6 tiles each)
    for lab, name in (("PlayerGfx", "gTcgPlayerPortraitGfx"), ("RodGfx", "gTcgRodPortraitGfx")):
        d = sec[lab][:36 * 16]
        out.append(f"const u8 {name}[] =")
        out.append("{")
        for k in range(0, len(d), 32):
            out.append("    " + ", ".join("0x%02X" % b for b in d[k:k + 32]) + ",")
        out.append("};")
    out.append("")
    out.append("const struct TcgSprite gTcgSprites[] =")
    out.append("{")
    for i, (lab, n) in sorted(gfx_labels.items()):
        out.append(f"    [TCG_{sprite_names[i]}] = {{sTcgSprite_{lab}, {n}}},")
    out.append("};")
    out.append("")

    # sprite animations: the kept ones' frame tables and frames
    keep = []
    for name in sorted(used_anims):
        if name not in anim_const_names:
            continue
        idx = anim_const_names.index(name)
        if idx < len(anim_ptrs):
            keep.append((name, anim_ptrs[idx][0]))
    done_tables, done_frames = set(), set()
    anim_defs = []
    for name, data_label in keep:
        items = blocks[data_label]
        ft = items[0][1][0]
        if ft not in done_tables:
            done_tables.add(ft)
            frames = [a for k, args in blocks[ft] for a in args if k == "dw"]
            for fr in frames:
                if fr in done_frames:
                    continue
                done_frames.add(fr)
                vals = [num(a) for k, args in blocks[fr] for a in args if k == "db"]
                out.append(f"static const u8 sTcgFrame_{fr.replace('.', '_')}[] = {{{', '.join(cval(v) for v in vals)}}};")
            out.append(f"static const u8 *const sTcgFrameTable_{ft}[] = {{{', '.join('sTcgFrame_' + f.replace('.', '_') for f in frames)}}};")
        fd = [num(a) for k, args in items if k == "frame_data" for a in args]
        out.append(f"static const s8 sTcg{data_label}[] = {{{', '.join(str(v if not isinstance(v, int) or v < 128 else v - 256) for v in fd)}}};")
        anim_defs.append((name, ft, data_label))
    out.append("")
    out.append("const struct TcgSpriteAnim gTcgSpriteAnims[] =")
    out.append("{")
    for name, ft, dl in anim_defs:
        out.append(f"    [TCG_{name}] = {{sTcgFrameTable_{ft}, sTcg{dl}}},")
    out.append("};")
    out.append("")

    # palettes
    out.append("const u16 *const gTcgPalettes[] =")
    out.append("{")
    pal_defs = []
    for name in sorted(used_pals):
        if name in pal_names and pal_names.index(name) < len(pal_ptrs):
            lab = pal_ptrs[pal_names.index(name)][0]
            if pals.get(lab):
                pal_defs.append((name, lab))
    out_p = []
    for name, lab in pal_defs:
        cols = pals[lab]
        out_p.append(f"static const u16 sTcgPal_{lab}[] = {{{len(cols) // 4}, {', '.join('0x%04X' % c for c in cols)}}};")
    out = out[:-2] + out_p + ["", "const u16 *const gTcgPalettes[] =", "{"]
    for name, lab in pal_defs:
        out.append(f"    [TCG_{name}] = sTcgPal_{lab},")
    out.append("};")
    out.append("")

    # duel animations
    out.append("const u8 gTcgDuelAnimations[][6] =")
    out.append("{")
    for a in duel_anims:
        out.append("    {" + ", ".join("0" if v == "NONE" else
                                   (re.sub(r"\b([A-Z][A-Z0-9_]+)\b", r"TCG_\1", v) if not re.match(r"^\$?[0-9]", v) else cval(num(v)))
                                   for v in a) + "},")
    out.append("};")
    out.append("")

    # attack animations
    cmds = {"anim_end": 0, "anim_normal": 1, "anim_player": 2, "anim_opponent": 3, "anim_screen": 4,
            "anim_play_area": 5, "anim_end2": 6}
    aa = {}
    cur = None
    ptrs = []
    for line in (SRC / "data/duel/animations/attack_animations.asm").read_text().splitlines():
        s = strip(line)
        m = re.match(r"^(\w+):$", s)
        if m:
            cur = m[1]
            aa[cur] = []
            continue
        m = re.match(r"dw\s+(\w+)", s)
        if m and cur == "PointerTable_AttackAnimation":
            ptrs.append(m[1])
            continue
        m = re.match(r"(anim_\w+)\s*(\w*)", s)
        if m and cur:
            aa[cur].append(str(cmds[m[1]]))
            if m[2]:
                aa[cur].append("TCG_" + m[2])
    for lab, c in aa.items():
        if lab != "PointerTable_AttackAnimation":
            out.append(f"static const u8 sTcg{lab}[] = {{{', '.join(c)}}};")
    out.append("")
    out.append("const u8 *const gTcgAttackAnimations[] =")
    out.append("{")
    for p in ptrs:
        out.append("    NULL," if p == "NULL" else f"    sTcg{p},")
    out.append("};")
    (ROOT / "src/data/tcg/anims.h").write_text("\n".join(out) + "\n")
    print(len(gfx_labels), "sprites,", len(anim_defs), "sprite animations,", len(pal_defs), "palettes,",
          len(duel_anims), "duel animations,", len(ptrs), "attack animations")


main()
