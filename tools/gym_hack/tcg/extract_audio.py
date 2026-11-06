#!/usr/bin/env python3
"""Pokémon Canvas: the Trading Card Game's music and sound effects, from the pret/poketcg disassembly,
as its sound engine reads them (src/tcg/sound.c plays them as the game did, on the GBA's Game Boy
sound channels).

    python3 tools/gym_hack/tcg/extract_audio.py <path to a pret/poketcg checkout>

Writes src/data/tcg/audio.h: one blob of the game's bytes (audio/music/*, audio/sfx/*, their
headers, the wave and noise instruments and vibrato types), with every pointer in it an offset into
the blob (0 is NULL: the blob starts with a byte nobody points at), and
  gTcgSongHeaders[MUSIC_*]   each song's header (a channel mask, then each channel's pointer)
  gTcgSfxHeaders[SFX_*]      each sound effect's
  gTcgWaveInstruments, gTcgNoiseInstruments, gTcgVibratoTypes: offsets, as the engine's tables
  gTcgPitches                (Music1_Pitches) each note's frequency
The macros (macros/audio.asm) are assembled here as rgbasm would.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SRC = Path(sys.argv[1]) / "src"

NOTES = {"C_": 1, "C#": 2, "D_": 3, "D#": 4, "E_": 5, "F_": 6, "F#": 7, "G_": 8, "G#": 9, "A_": 10, "A#": 11,
         "B_": 12}
SFX_NAMES = re.findall(r"const (SFX_\w+)", "".join(p.read_text() for p in (SRC / "constants").glob("*.asm")))
CONSTS = {"TRUE": 1, "FALSE": 0, "NULL": 0, **NOTES}


def strip(line):
    return line.split(";")[0].strip()


def ev(e):
    """an rgbasm expression (numbers, the notes, + - * << | !!)"""
    e = e.strip()
    e = re.sub(r"BANK\([^)]*\)", "0", e)
    e = re.sub(r"\bNUM_SFX\b", str(len(SFX_NAMES)), e)
    e = re.sub(r"\$([0-9a-fA-F]+)", lambda m: str(int(m[1], 16)), e)
    e = re.sub(r"%([01]+)", lambda m: str(int(m[1], 2)), e)
    e = re.sub(r"[A-G][_#]", lambda m: str(NOTES[m[0]]), e)
    e = re.sub(r"\b(TRUE|FALSE|NULL)\b", lambda m: str(CONSTS[m[0]]), e)
    e = e.replace("!!", "1 if ").replace("1 if (", "(1 if (")   # (!!x: never used but with TRUE/FALSE)
    if "1 if" in e:
        e = re.sub(r"1 if\s*(\w+)", lambda m: str(1 if int(m[1]) else 0), e)
    return int(eval(e, {"__builtins__": {}}))


def args(s):
    return [a.strip() for a in s.split(",")] if s.strip() else []


class Blob:
    def __init__(self):
        self.data = bytearray([0xFF])       # (offset 0: NULL)
        self.labels = {}
        self.fixups = []                    # (offset, label)

    def label(self, name):
        assert name not in self.labels, name
        self.labels[name] = len(self.data)

    def db(self, *v):
        for x in v:
            self.data.append(x & 0xFF)

    def dw(self, ref):
        if ":" in ref:
            self.fixups.append((len(self.data), ref))
            self.data += b"\0\0"
        else:
            v = ev(ref)
            self.data += bytes([v & 0xFF, v >> 8])

    def dn(self, *nibbles):
        for i in range(0, len(nibbles), 2):
            self.db((nibbles[i] << 4) | nibbles[i + 1])

    def label_of(self, ref):
        """a label (a song bank's header can point at the other bank's: the names are shared)"""
        if ref in self.labels:
            return self.labels[ref]
        other = {"M1:": "M2:", "M2:": "M1:"}.get(ref[:3])
        return self.labels[other + ref[3:]]

    def resolve(self):
        for off, ref in self.fixups:
            v = self.label_of(ref)
            assert v < 0x10000
            self.data[off], self.data[off + 1] = v & 0xFF, v >> 8


def env(a, b):
    a, b = ev(a), ev(b)
    return (a << 4) | ((0x8 | -b) if b < 0 else b)


def assemble(blob, path, ns):
    """one file's lines into the blob, its labels prefixed with ns (local ones with the last global)"""
    glob = ""
    for line in path.read_text().splitlines():
        s = strip(line)
        if not s:
            continue
        m = re.match(r"^(\.\w+):*$|^(\w+):+$", s)
        if m:
            name = m[1] or m[2]
            if name.startswith("."):
                name = glob + name
            else:
                glob = name
            blob.label(ns + name)
            continue
        op, _, rest = s.partition(" ")
        if "\t" in op:
            op, _, rest2 = op.partition("\t")
            rest = rest2 + " " + rest
        a = args(rest)
        d = blob
        if op == "db":
            d.db(*[ev(x) for x in a])
        elif op == "dw":
            for x in a:
                ref = x if re.match(r"^\$?[0-9%]", x) or x in CONSTS else ns + (glob + x if x.startswith(".") else x)
                d.dw(ref)
        elif op == "dn":
            d.dn(*[ev(x) for x in a])
        elif op in ("table_width", "assert_table_length", "INCLUDE"):
            pass
        elif op == "note":
            d.db((ev(a[0]) << 4) | (ev(a[1]) - 1))
        elif op in ("bass", "snare1", "snare2", "snare3", "snare4", "snare5"):
            d.db(({"bass": 1, "snare1": 3, "snare2": 5, "snare3": 7, "snare4": 9, "snare5": 12}[op] << 4) | (ev(a[0]) - 1))
        elif op == "rest":
            d.db(ev(a[0]) - 1)
        elif op == "speed":
            d.db(0xd0, ev(a[0]))
        elif op == "octave":
            d.db(0xd0 | ev(a[0]))
        elif op in ("inc_octave", "dec_octave", "tie", "MainLoop", "EndMainLoop", "EndLoop", "music_ret", "music_end"):
            d.db({"inc_octave": 0xd7, "dec_octave": 0xd8, "tie": 0xd9, "MainLoop": 0xdd, "EndMainLoop": 0xde,
                  "EndLoop": 0xe0, "music_ret": 0xe3, "music_end": 0xff}[op])
        elif op == "stereo_panning":
            d.db(0xdc, (ev(a[0]) << 4) | ev(a[1]))
        elif op == "Loop":
            d.db(0xdf, ev(a[0]))
        elif op == "music_call":
            d.db(0xe2)
            d.dw(ns + a[0])
        elif op in ("frequency_offset", "wave", "cutoff", "echo", "vibrato_type", "vibrato_delay"):
            d.db({"frequency_offset": 0xe4, "wave": 0xe7, "cutoff": 0xe8, "echo": 0xe9, "vibrato_type": 0xea,
                  "vibrato_delay": 0xeb}[op], ev(a[0]))
        elif op == "duty":
            d.db(0xe5, ev(a[0]) << 6)
        elif op == "volume_envelope":
            d.db(0xe6, env(a[0], a[1]))
        elif op == "sfx_freq":
            v = ev(a[0])
            assert v < 0x800
            d.db(v >> 8, v & 0xFF)
        elif op == "sfx_env":
            d.db(0x10, env(a[0], a[1]))
        elif op == "sfx_duty":
            d.db(0x20 | (ev(a[0]) << 2))
        elif op == "sfx_loop":
            d.db(0x30, ev(a[0]))
        elif op == "sfx_endloop":
            d.db(0x40)
        elif op == "sfx_pitch_offset":
            d.db(0x50, ev(a[0]))
        elif op == "sfx_wait":
            d.db(0x60, ev(a[0]))
        elif op == "sfx_wave":
            d.db(0x70 | ev(a[0]))
        elif op == "sfx_pan":
            d.db(0x80, ((1 if ev(a[0].lstrip("!")) else 0) << 4) | (1 if ev(a[1].lstrip("!")) else 0))
        elif op == "sfx_end":
            d.db(0xf0)
        else:
            raise SystemExit(f"{path}: {s}")


def includes(asm, start=None):
    """the INCLUDEd audio files of an engine file, from `start` (a label) on"""
    text = (SRC / asm).read_text()
    if start:
        text = text[text.index(start):]
    return re.findall(r'INCLUDE "(audio/[^"]+)"', text)


def main():
    blob = Blob()
    # the engine's tables (one copy: music1's and music2's are the same)
    for label, f in (("WaveInstruments", "audio/wave_instruments.asm"), ("NoiseInstruments", "audio/noise_instruments.asm"),
                     ("VibratoTypes", "audio/vibrato_types.asm")):
        blob.label("T:" + label)
        assemble(blob, SRC / f, "T:" + label)
    # the songs, each engine bank's own (their headers share names)
    for bank, asm in (("M1:", "audio/music1.asm"), ("M2:", "audio/music2.asm")):
        for f in [x for x in includes(asm) if "/music" in x]:
            assemble(blob, SRC / f, bank)
    # the sound effects
    for f in [x for x in includes("audio/sfx.asm") if "sfx" in x]:
        assemble(blob, SRC / f, "S:")
    blob.resolve()

    songs = re.findall(r"const (MUSIC_\w+)", (SRC / "constants/music_constants.asm").read_text())
    sfx = re.findall(r"const (SFX_\w+)", (SRC / "constants/music_constants.asm").read_text())
    if not sfx:
        sfx = re.findall(r"const (SFX_\w+)", (SRC / "constants/sfx_constants.asm").read_text())

    def table(header_file, ns, name):
        t = (SRC / header_file).read_text()
        t = t[t.index(name):]
        t = t[:t.index("assert_table_length")]
        return [ns + x if x != "NULL" else None for x in re.findall(r"dw (\w+)", t)]
    m1 = table("audio/music1_headers.asm", "M1:", "SongHeaderPointers1")
    m2 = table("audio/music2_headers.asm", "M2:", "SongHeaderPointers2")
    song_offsets = []
    for a, b in zip(m1, m2):
        # (a song's in whichever bank has it: the other's header is a %0000 stand-in)
        oa = blob.label_of(a) if a else None
        ob = blob.label_of(b) if b else None
        pick = oa if oa is not None and blob.data[oa] != 0 else ob if ob is not None and blob.data[ob] != 0 else (oa or ob or 0)
        song_offsets.append(pick)
    sfx_offsets = [blob.labels[x] for x in table("audio/sfx_headers.asm", "S:", "SFXHeaderPointers")]
    pitches = [int(x, 16) for x in re.findall(r"dw \$([0-9a-f]+) ;", (SRC / "audio/music1.asm").read_text()
                                                [(SRC / "audio/music1.asm").read_text().index("Music1_Pitches"):])]

    out = ["// Generated by tools/gym_hack/tcg/extract_audio.py (from pret/poketcg): the Trading Card Game's",
           "// music and sound effects, as its sound engine reads them (src/tcg/sound.c). Pointers in the blob",
           "// are offsets into it.", ""]
    out.append(f"const u8 gTcgAudio[{len(blob.data)}] =")
    out.append("{")
    for k in range(0, len(blob.data), 24):
        out.append("    " + ", ".join(f"0x{b:02X}" for b in blob.data[k:k + 24]) + ",")
    out.append("};")
    out.append("")
    out.append(f"const u16 gTcgSongHeaders[{len(song_offsets)}] = {{" + ", ".join(str(o) for o in song_offsets) + "};")
    out.append(f"const u16 gTcgSfxHeaders[{len(sfx_offsets)}] = {{" + ", ".join(str(o) for o in sfx_offsets) + "};")
    for name, label in (("gTcgWaveInstruments", "T:WaveInstruments"), ("gTcgNoiseInstruments", "T:NoiseInstruments"),
                        ("gTcgVibratoTypes", "T:VibratoTypes")):
        out.append(f"const u16 {name} = {blob.labels[label]};")
    out.append(f"const u16 gTcgPitches[{len(pitches)}] = {{" + ", ".join(f"0x{p:03X}" for p in pitches) + "};")
    out.append("")
    (ROOT / "src/data/tcg/audio.h").write_text("\n".join(out))
    print(len(blob.data), "bytes;", len(song_offsets), "songs,", len(sfx_offsets), "sfx,", len(pitches), "pitches;",
          len(songs), "MUSIC_ consts")


if __name__ == "__main__":
    main()
