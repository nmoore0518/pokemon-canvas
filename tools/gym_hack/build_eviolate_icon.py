#!/usr/bin/env python3
"""The Eviolate's icon (Pokémon Canvas): Eviolite's crystal, its lavender turned to embers.

    python3 tools/gym_hack/build_eviolate_icon.py

Writes graphics/items/icons/gym/eviolate.png (Eviolite's picture) and
graphics/items/icon_palettes/gym/eviolate.pal (its palette, recoloured).
"""
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RECOLOUR = {
    (248, 200, 240): (255, 208, 168),
    (224, 176, 232): (248, 152, 112),
    (240, 224, 248): (255, 240, 216),
    (200, 144, 224): (224, 88, 64),
    (176, 120, 216): (168, 40, 48),
}


def main():
    shutil.copy(ROOT / "graphics/items/icons/eviolite.png", ROOT / "graphics/items/icons/gym/eviolate.png")
    lines = (ROOT / "graphics/items/icon_palettes/eviolite.pal").read_text().splitlines()
    out = lines[:3]
    for l in lines[3:]:
        c = tuple(int(v) for v in l.split())
        out.append("%d %d %d" % RECOLOUR.get(c, c))
    (ROOT / "graphics/items/icon_palettes/gym/eviolate.pal").write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
