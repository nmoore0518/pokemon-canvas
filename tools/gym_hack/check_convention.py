#!/usr/bin/env python3
"""Checks the built ROM: every Pokémon on a Gym Leader Convention team exists in it.

(A species the config leaves out has no data: base HP 0. The game would stop on it in battle.)
Reads gSpeciesInfo from pokefirered.elf.

    python3 tools/gym_hack/check_convention.py
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
NM = "arm-none-eabi-nm"


def main():
    elf = ROOT / "pokefirered.elf"
    nm = subprocess.run([NM, "-S", str(elf)], capture_output=True, text=True)
    if nm.returncode:
        nm = subprocess.run(["/opt/xpack/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/" + NM, "-S", str(elf)],
                            capture_output=True, text=True)
    addr = size = None
    for line in nm.stdout.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[3] == "gSpeciesInfo":
            addr, size = int(parts[0], 16), int(parts[1], 16)
    species, v = {}, -1          # (the enum: explicit values, aliases, and implicit + 1)
    text = (ROOT / "include/constants/species.h").read_text()
    for m in re.finditer(r"^\s+(SPECIES_\w+|NUM_SPECIES)\s*(?:=\s*([^,\n]+))?,", text, re.M):
        e = (m.group(2) or "").strip()
        if not e:
            v += 1
        elif re.fullmatch(r"\d+", e):
            v = int(e)
        elif re.fullmatch(r"\w+", e) and e in species:
            v = species[e]
        else:
            v = eval(re.sub(r"\b(SPECIES_\w+)\b", lambda k: str(species[k.group(1)]), e))
        species[m.group(1)] = v
    count = species["SPECIES_EGG"] + 1
    assert size % count == 0, (size, count)
    entry = size // count
    rom = (ROOT / "pokefirered.gba").read_bytes()
    used = sorted(set(re.findall(r"\.species = (SPECIES_\w+)", (ROOT / "src/data/gym_convention.h").read_text())))
    missing = []
    for s in used:
        off = addr - 0x08000000 + species[s] * entry
        if rom[off] == 0:             # (baseHP is the first field)
            missing.append(s)
    if missing:
        print("convention: not in the ROM:", ", ".join(missing))
        sys.exit(1)
    print(f"convention: all {len(used)} species are in the ROM")


if __name__ == "__main__":
    main()
