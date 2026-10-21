#!/usr/bin/env python3
"""HeartGold/SoulSilver music for Johto (Gym Trainer hack), from Pokémon HnS.

HnS has HGSS's songs as MIDI files (sound/songs/midi/mus_hg_*.mid) played with an HGSS instrument
set: voicegroup229 and its sub-groups (voicegroups 229-273, plus a few DPPt ones they borrow), key
split tables 9-58, and about 200 samples (hg_*, dp_*). This copies the songs listed below and that
whole instrument set, renamed so nothing clashes with ours (voicegroup229 -> voicegroup_hg229,
KeySplitTable35 -> KeySplitTable_hg35), and adds the songs as MUS_HG_* after the FireRed ones.
(voicegroup229's last voices run on into voicegroup230, as in HnS, so the groups stay in order.)

    python3 tools/gym_hack/import_hgss_music.py [path/to/pokehns-expansion]
"""
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("/home/claude/ext/pokehns-expansion")

SONGS = [
    # towns, routes and buildings
    "goldenrod", "ecruteak", "azalea", "violet", "route34", "route38", "route42", "poke_center", "poke_mart",
    "gym", "bell_tower", "burned_tower", "dance_theater", "national_park", "game_corner", "lighthouse",
    "union_cave", "ss_aqua",
    # the convention
    "pokeathlon_lobby", "pokemon_league",
    # battles
    "vs_wild", "vs_trainer", "vs_gym_leader", "vs_gym_leader_kanto", "vs_champion",
    "victory_trainer", "victory_wild", "victory_gym_leader",
]
HG_GROUPS = [str(n) for n in range(229, 274)]     # (in this order: voices run on into the next group)
BEGIN, END = "@ BEGIN HGSS music (tools/gym_hack/import_hgss_music.py)\n", "@ END HGSS music\n"
CBEGIN, CEND = "// BEGIN HGSS music (tools/gym_hack/import_hgss_music.py)\n", "// END HGSS music\n"


def rename(text):
    text = re.sub(r"\bvoicegroup(\d+)\b", r"voicegroup_hg\1", text)
    return re.sub(r"\bKeySplitTable(\d+)\b", r"KeySplitTable_hg\1", text)


def put_block(path, block, begin=BEGIN, end=END, after=None):
    text = path.read_text()
    if begin in text:
        a, b = text.index(begin), text.index(end) + len(end)
        text = text[:a] + begin + block + end + text[b:]
    elif after:
        i = text.index(after) + len(after)
        text = text[:i] + begin + block + end + text[i:]
    else:
        text = text.rstrip("\n") + "\n\n" + begin + block + end
    path.write_text(text)


def main():
    # ---- Instruments: the voicegroups the HGSS set needs ------------------------------------
    groups, todo, samples, tables = [], list(HG_GROUPS), set(), set()
    while todo:
        g = todo.pop(0)
        if g in groups:
            continue
        groups.append(g)
        for line in (SRC / f"sound/voicegroups/voicegroup{g}.inc").read_text().splitlines():
            line = line.split("@")[0]
            todo += [x for x in re.findall(r"voicegroup(\d+)", line) if x != g]
            samples |= set(re.findall(r"DirectSoundWaveData_(\w+)", line))
            tables |= set(re.findall(r"KeySplitTable(\d+)", line))
    order = HG_GROUPS + sorted((g for g in groups if g not in HG_GROUPS), key=int)
    vdir = ROOT / "sound/voicegroups/hgss"
    shutil.rmtree(vdir, ignore_errors=True)
    vdir.mkdir()
    for g in order:
        (vdir / f"hg{g}.inc").write_text(rename((SRC / f"sound/voicegroups/voicegroup{g}.inc").read_text()))
    put_block(ROOT / "sound/voice_groups.inc", "".join(f'.include "sound/voicegroups/hgss/hg{g}.inc"\n' for g in order))

    # Key split tables (each a .set label and 128 bytes)
    ks = (SRC / "sound/keysplit_tables.inc").read_text()
    blocks = []
    for t in sorted(tables, key=int):
        m = re.search(rf"^\.set KeySplitTable{t},.*?(?=^\.set |\Z)", ks, re.M | re.S)
        blocks.append(rename(m.group(0)).rstrip() + "\n\n")
    put_block(ROOT / "sound/keysplit_tables.inc", "".join(blocks))

    # Samples (ours already has a few of them)
    ours = (ROOT / "sound/direct_sound_data.inc").read_text()
    if BEGIN in ours:
        ours = ours[:ours.index(BEGIN)] + ours[ours.index(END) + len(END):]
    dsd = (SRC / "sound/direct_sound_data.inc").read_text()
    entries = []
    for s in sorted(samples):
        if f"DirectSoundWaveData_{s}::" in ours:
            continue
        path = re.search(rf"DirectSoundWaveData_{s}::\s*\n\s*\.incbin \"([^\"]+)\"", dsd).group(1)
        wav = path.replace(".bin", ".wav")
        shutil.copy(SRC / wav, ROOT / wav)
        entries.append(f'\t.align 2\nDirectSoundWaveData_{s}::\n\t.incbin "{path}"\n\n')
    put_block(ROOT / "sound/direct_sound_data.inc", "".join(entries))

    # ---- Songs ------------------------------------------------------------------------------
    cfg_src = (SRC / "sound/songs/midi/midi.cfg").read_text()
    cfg_lines = []
    for s in SONGS:
        name = f"mus_hg_{s}"
        shutil.copy(SRC / f"sound/songs/midi/{name}.mid", ROOT / f"sound/songs/midi/{name}.mid")
        opts = re.search(rf"^{name}\.mid:(.*)$", cfg_src, re.M).group(1)
        cfg_lines.append(f"{name}.mid:" + re.sub(r"-G(\d+)", r"-G_hg\1", opts) + "\n")
    cfg = ROOT / "sound/songs/midi/midi.cfg"
    text = cfg.read_text()
    text = "".join(l for l in text.splitlines(True) if not l.startswith("mus_hg_"))
    cfg.write_text(text.rstrip("\n") + "\n" + "".join(cfg_lines))

    # The song table: after the last FireRed song, before the phonemes
    st = ROOT / "sound/song_table.inc"
    text = st.read_text()
    text = "".join(l for l in text.splitlines(True) if "song mus_hg_" not in l)
    anchor = "\tsong mus_rg_teachy_tv_menu, MUSIC_PLAYER_BGM, 0\n"
    text = text.replace(anchor, anchor + "".join(f"\tsong mus_hg_{s}, MUSIC_PLAYER_BGM, 0\n" for s in SONGS))
    st.write_text(text)

    # Constants: MUS_HG_* right after MUS_RG_TEACHY_TV_MENU, the phonemes moved up after them
    sh = ROOT / "include/constants/songs.h"
    text = sh.read_text()
    if CBEGIN in text:
        text = text[:text.index(CBEGIN)] + text[text.index(CEND) + len(CEND):]
    last = int(re.search(r"#define MUS_RG_TEACHY_TV_MENU\s+(\d+)", text).group(1))
    defs = "".join(f"#define MUS_HG_{s.upper():<26} {last + 1 + i}\n" for i, s in enumerate(SONGS))
    anchor = re.search(r"#define MUS_RG_TEACHY_TV_MENU.*\n", text).group(0)
    text = text.replace(anchor, anchor + CBEGIN + defs + CEND)
    text = re.sub(r"#define END_MUS\s+\S+", f"#define END_MUS                     MUS_HG_{SONGS[-1].upper()}", text)
    ph = [m for m in re.finditer(r"^(#define PH_\w+\s+)(\d+)", text, re.M)]
    first_ph = last + 1 + len(SONGS)
    out, pos, n = [], 0, 0
    for m in ph:
        out.append(text[pos:m.start()])
        out.append(m.group(1) + str(first_ph + n))
        pos, n = m.end(), n + 1
    out.append(text[pos:])
    sh.write_text("".join(out))
    print(f"HGSS music: {len(SONGS)} songs, {len(order)} voicegroups, {len(tables)} key split tables, "
          f"{len(entries)} new samples")


if __name__ == "__main__":
    main()
