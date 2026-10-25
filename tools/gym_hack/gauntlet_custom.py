"""The Gauntlet's custom battle pictures (Gym Trainer hack): challengers no sprite set has, made by
repainting a PokéRogue class picture (its colors swapped, by region if need be) into the character.
    python3 tools/gym_hack/gauntlet_custom.py        (writes tools/gym_hack/art/<key>.png)
build_gauntlet.py reads them as "art:<key>".
"""
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
import build_gauntlet as B  # noqa: E402

OUT = Path(__file__).parent / "art"

# key: (PokéRogue base, [(from, to, (y0, y1[, x0, x1]) or None)])  (in the base's own frame)
RECIPES = {
    "sycamore": ("lucian", [
        ("622039", "1c2848", None), ("9c3152", "2c4478", None), ("bd527b", "4a6ab0", None),
        ("8383c5", "3e465e", None), ("5a5a94", "232838", None), ("bdb4f6", "68708c", None),
    ]),
    "turo": ("scientist_m", [
        ("424a63", "5a6474", (0, 12)), ("292942", "343a48", (0, 12)),
        ("ad3a3a", "7a3ad0", None), ("4273ad", "30c8d8", None), ("7b4a5a", "4a3a70", (20, 40)),
    ]),
    "sada": ("scientist_f", [
        ("ffffff", "d08a4a", None), ("ced6d6", "a85a30", None), ("adadad", "7a3a22", None),
        ("6b6b7b", "4a2418", None), ("424a63", "4a2222", None), ("212942", "1a0e10", None),
        ("ad426b", "e0b050", None), ("6b3a42", "8a6020", None), ("7b84a5", "c03028", None),
    ]),
    "volo": ("backpacker_m", [
        ("52524a", "e8c858", (0, 16)), ("3a3129", "a07a28", (0, 16)),
        ("3a3163", "16161e", None), ("5a739c", "2a2a36", None), ("63b5ce", "3c3c4c", None),
        ("4a5242", "22222c", None), ("6b7b5a", "3c3c4a", None), ("de5a3a", "d8a020", None),
        ("adb5b5", "8a8a96", None), ("ffffff", "d8a020", (13, 40, 30, 99)),
    ]),
    "carmine": ("snow_ace_trainer_f", [
        ("414a52", "c8342c", (6, 22, 34, 99)), ("312931", "7a1a1c", (6, 22, 34, 99)),
        ("414a52", "2c2430", None), ("312931", "141016", None),
        ("ac6a41", "5a6a28", None), ("e68b41", "a8b838", None), ("524a41", "34342c", None),
    ]),
}


def hexc(s):
    return tuple(int(s[i:i + 2], 16) for i in (0, 2, 4))


def make(key):
    base, rules = RECIPES[key]
    im = B.load_pic("pr:" + base).convert("RGBA")
    px = im.load()
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = px[x, y]
            if a < 128:
                continue
            for src, dst, rng in rules:
                if (r, g, b) == hexc(src) and (rng is None or (rng[0] <= y <= rng[1]
                                                       and (len(rng) < 4 or rng[2] <= x <= rng[3]))):
                    px[x, y] = hexc(dst) + (255,)
                    break
    return im


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True)
    for k in (sys.argv[1:] or RECIPES):
        make(k).save(OUT / f"{k}.png")
