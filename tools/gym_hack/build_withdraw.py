#!/usr/bin/env python3
"""Every species and form the player could keep (Gym Trainer hack): not the battle-only ones
(megas, Gigantamax, primal, Zen Mode and the like), not totems, not CLEFFABILL - in National Dex
order, each form after its species. What Bill's Palm Pilot can WITHDRAW (the Gauntlet's finale)
and the slots of BILL'S ARCHIVE (the living dex, src/archive.c).

    python3 tools/gym_hack/build_withdraw.py
        -> include/constants/dex_forms.h (DEX_FORM_COUNT), src/data/dex_forms.h (src/dex_forms.c)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))


def preprocess(header, *extra):
    import subprocess
    import tempfile
    with tempfile.TemporaryDirectory() as d:
        if header is None:
            c = Path(d) / "fct.c"
            c.write_text('#include "global.h"\n#include "constants/form_change_types.h"\n#include "constants/items.h"\n'
                         '#include "constants/moves.h"\n#include "data/pokemon/form_change_tables.h"\n')
            header = str(c)
        out = Path(d) / "out.i"
        subprocess.run(["arm-none-eabi-gcc", "-E", "-P", "-w", "-iquote", "include", "-iquote", "src", "-I", "include",
                        "-I", "gflib", "-DMODERN=1", "-DTESTING=0", "-DFIRERED", "-std=gnu17", *extra, "-x", "c",
                        header, "-o", str(out)], cwd=ROOT, check=True)
        return out.read_text()


EXCLUDE = {"SPECIES_CLEFFABILL", "SPECIES_CHANSEY_DAD"}
LABEL = {   # forms whose constant doesn't say it well
    "SPECIES_PIKACHU_PHD": "Pikachu Ph.D.",
    "SPECIES_PIKACHU_ORIGINAL": "Pikachu Orig. Cap",
    "SPECIES_PIKACHU_HOENN": "Pikachu Hoenn Cap",
    "SPECIES_PIKACHU_SINNOH": "Pikachu Sinnoh Cap",
    "SPECIES_PIKACHU_UNOVA": "Pikachu Unova Cap",
    "SPECIES_PIKACHU_KALOS": "Pikachu Kalos Cap",
    "SPECIES_PIKACHU_ALOLA": "Pikachu Alola Cap",
    "SPECIES_PIKACHU_PARTNER": "Pikachu Partner Cap",
    "SPECIES_PIKACHU_WORLD": "Pikachu World Cap",
}


def main():
    src = preprocess("src/data/pokemon/species_info.h", "-include", "include/global.h")
    blocks = re.split(r"\[(SPECIES_[A-Z0-9_]+)\]\s*=", src)
    S = {}
    for i in range(1, len(blocks), 2):
        sp, b = blocks[i], blocks[i + 1]
        if sp in S or sp in ("SPECIES_NONE", "SPECIES_EGG"):
            continue
        n = re.search(r'\.speciesName\s*=\s*_\("(.*?)"\)', b)
        if not n:
            continue
        nd = re.search(r"\.natDexNum\s*=\s*(\w+)", b)
        flags = set(re.findall(r"\.(is\w+)\s*=\s*(?:TRUE|1)\b", b))
        fct = re.search(r"\.formChangeTable\s*=\s*(\w+)", b)
        evo_t = re.findall(r"\{EVO_\w+, [^,]+, (SPECIES_\w+)", b)
        S[sp] = dict(name=n[1], nat=nd[1] if nd else None, flags=flags, fct=fct[1] if fct else None, evo=evo_t)
    enum = (ROOT / "include/constants/species.h").read_text()
    num = {s: int(v) for s, v in re.findall(r"^\s+(SPECIES_[A-Z0-9_]+)\s*=\s*(\d+)", enum, re.M)}
    dex = (ROOT / "include/constants/pokedex.h").read_text()
    body = dex[dex.index("enum NationalDexOrder"):]
    natnum = {}
    k = 0
    for m in re.finditer(r"^\s+(NATIONAL_DEX_\w+)(?:\s*=\s*(\d+))?,", body, re.M):
        k = int(m[2]) if m[2] else k + (1 if natnum else 0)
        natnum[m[1]] = k
    fsrc = preprocess(None)
    FCT = {}
    for m in re.finditer(r"static const struct FormChange (\w+)\[\]\s*=\s*\{(.*?)\};", fsrc, re.S):
        FCT[m[1]] = [tuple(x.strip() for x in e.split(",")) for e in re.findall(r"\{(FORM_CHANGE_[^}]*)\}", m[2])]
    battle = lambda meth: meth.startswith("FORM_CHANGE_BATTLE") or meth == "FORM_CHANGE_BEGIN_BATTLE"
    battle_tgt, other_tgt = set(), set()
    for ents in FCT.values():
        for e in ents:
            if len(e) > 1:
                (battle_tgt if battle(e[0]) else other_tgt).add(e[1]) if e[0] not in ("FORM_CHANGE_FAINT", "FORM_CHANGE_END_BATTLE") or battle(e[0]) else None
    evo_tgt = {t for d in S.values() for t in d["evo"]}

    lowest = {}
    for sp in S:
        if S[sp]["nat"] and num.get(sp, 99999) < num.get(lowest.get(S[sp]["nat"]), 99999):
            lowest[S[sp]["nat"]] = sp

    def keep(s):
        f = S[s]["flags"]
        if s in EXCLUDE or not S[s]["nat"] or S[s]["nat"] == "NATIONAL_DEX_NONE":
            return False
        if f & {"isMegaEvolution", "isGigantamax", "isPrimalReversion", "isUltraBurst", "isTeraForm", "isTotem"}:
            return False
        if s in battle_tgt and s not in other_tgt and s not in evo_tgt and lowest[S[s]["nat"]] != s:
            return False
        return True

    chosen = sorted((s for s in S if keep(s)), key=lambda s: (natnum.get(S[s]["nat"], 99999), num.get(s, 99999)))
    first = {}
    for s in chosen:
        first.setdefault(S[s]["nat"], s)

    def label(s):
        if first[S[s]["nat"]] == s:
            return None
        if s in LABEL:
            return LABEL[s]
        nm = S[s]["name"]
        base = first[S[s]["nat"]][len("SPECIES_"):]
        suf = s[len("SPECIES_"):]
        suf = suf[len(base) + 1:] if suf.startswith(base + "_") else suf
        lab = f"{nm} {suf.replace('_', ' ').title()}"
        return lab if len(lab) <= 20 else lab[:20]

    gens = [1, 152, 252, 387, 494, 650, 722, 810, 906]
    gen_starts = []
    for g in gens:
        for i, s in enumerate(chosen):
            if natnum[S[s]["nat"]] >= g:
                gen_starts.append(i)
                break
    head = "// Generated by tools/gym_hack/build_withdraw.py: every species and form the player could keep,\n"
    (ROOT / "include/constants/dex_forms.h").write_text(
        "#ifndef GUARD_CONSTANTS_DEX_FORMS_H\n#define GUARD_CONSTANTS_DEX_FORMS_H\n\n" + head
        + "// in National Dex order (Bill's WITHDRAW, BILL'S ARCHIVE)\n\n"
        + f"#define DEX_FORM_COUNT {len(chosen)}\n#define DEX_FORM_GEN_COUNT {len(gen_starts)}\n\n#endif\n")
    out = [head.rstrip("\n"), "// in National Dex order (the species' own name, or a form's label)", "",
           "const u16 gDexFormSpecies[DEX_FORM_COUNT] = {"]
    out += [f"    {s}," for s in chosen]
    out += ["};", "", "const u8 *const gDexFormLabels[DEX_FORM_COUNT] = {"]
    for i, s in enumerate(chosen):
        lab = label(s)
        if lab:
            out.append(f'    [{i}] = COMPOUND_STRING("{lab}"),')
    out += ["};", "", "// where each generation starts",
            f"const u16 gDexFormGenStarts[DEX_FORM_GEN_COUNT] = {{{', '.join(map(str, gen_starts))}}};", ""]
    (ROOT / "src/data/dex_forms.h").write_text("\n".join(out))
    print(f"withdraw: {len(chosen)} Pokémon ({sum(1 for s in chosen if label(s))} forms)")


if __name__ == "__main__":
    main()
