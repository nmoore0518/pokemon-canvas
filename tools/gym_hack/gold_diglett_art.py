#!/usr/bin/env python3
"""The solid gold Diglett in Route 21's hidden cave (Pokémon Canvas): Diglett's overworld picture
(facing down, in its mound of dirt), cast in gold. One 32x32 frame.

    python3 tools/gym_hack/gold_diglett_art.py [preview.png]
        -> graphics/object_events/pics/misc/gold_diglett.png (OBJ_EVENT_GFX_GOLD_DIGLETT)
"""
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "graphics/pokemon/diglett/overworld.png"
OUT = ROOT / "graphics/object_events/pics/misc/gold_diglett.png"
# Diglett's overworld palette, slot by slot: its body and nose become gold; the dirt stays
GOLD = {
    1: (96, 64, 8),       # outline
    2: (248, 216, 88),    # body, light
    3: (168, 104, 8),     # nose, dark
    4: (208, 152, 24),    # body
    5: (255, 248, 200),   # shine
    6: (72, 40, 0),       # eyes (dark gold)
    7: (255, 224, 112),   # nose (bright: it catches the light)
    8: (255, 232, 136),   # nose, light
    9: (176, 120, 16),
}


def main(preview=None):
    src = Image.open(SRC)
    pal = src.getpalette()[:48]
    cols = [tuple(pal[i:i + 3]) for i in range(0, 48, 3)]
    for i, c in GOLD.items():
        cols[i] = c
    im = src.crop((0, 0, 32, 32))
    im.putpalette([v for c in cols for v in c] + [0] * (768 - 48))
    im.save(OUT, transparency=0)
    if preview:
        im.convert("RGB").resize((128, 128), Image.NEAREST).save(preview)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else None)
