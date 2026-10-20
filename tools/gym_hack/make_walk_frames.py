#!/usr/bin/env python3
"""Walking frames for overworld sprites that only have standing ones (Gym Trainer hack).

FireRed's Gym Leaders and most of the Elite Four never walk, so their sheets hold just the three
standing frames (down, up, left). The mentors walk in this hack, so this adds the six walking
frames in the standard order (down x2, up x2, left x2), made from the standing ones:
  - down/up: one foot lifted (the bottom rows of that half moved up a pixel)
  - left: a stride (the feet moved apart), then a passing step (the body raised a pixel)
Run once; it skips sheets that already have 9 frames.

    python3 tools/gym_hack/make_walk_frames.py
"""
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
PEOPLE = ROOT / "graphics/object_events/pics/people"
NAMES = ["brock", "misty", "lt_surge", "erika", "koga", "sabrina", "blaine", "bruno", "agatha", "lance"]
W, H = 16, 32
FEET = 4    # how many rows at the bottom count as feet


def frame(img, i):
    return img.crop((i * W, 0, (i + 1) * W, H))


def feet_top(f):
    """The first row of the feet (the lowest FEET rows that have pixels)."""
    px = f.load()
    bottom = max((y for y in range(H) for x in range(W) if px[x, y]), default=H - 1)
    return bottom - FEET + 1


def lift_foot(f, left_half):
    out = f.copy()
    src, dst = f.load(), out.load()
    top = feet_top(f)
    xs = range(0, W // 2) if left_half else range(W // 2, W)
    for x in xs:
        for y in range(top, H):
            dst[x, y] = src[x, y + 1] if y + 1 < H else 0
    return out


def stride(f):
    out = f.copy()
    src, dst = f.load(), out.load()
    top = feet_top(f)
    for y in range(top, H):
        for x in range(W):
            dst[x, y] = 0
        for x in range(W):
            if not src[x, y]:
                continue
            nx = x - 1 if x < W // 2 else x + 1
            if 0 <= nx < W:
                dst[nx, y] = src[x, y]
    return out


def bob(f):
    out = Image.new(f.mode, f.size, 0)
    out.putpalette(f.getpalette())
    out.paste(f.crop((0, 1, W, H)), (0, 0))
    out.paste(f.crop((0, H - 1, W, H)), (0, H - 1))    # keep the feet on the ground
    return out


def main():
    for name in NAMES:
        path = PEOPLE / f"{name}.png"
        img = Image.open(path)
        assert img.mode == "P", path
        if img.size[0] >= 9 * W:
            continue
        down, up, left = (frame(img, i) for i in range(3))
        frames = [down, up, left,
                  lift_foot(down, True), lift_foot(down, False),
                  lift_foot(up, True), lift_foot(up, False),
                  stride(left), bob(left)]
        sheet = Image.new("P", (9 * W, H), 0)
        sheet.putpalette(img.getpalette())
        for i, f in enumerate(frames):
            sheet.paste(f, (i * W, 0))
        extra = {"transparency": img.info["transparency"]} if "transparency" in img.info else {}
        sheet.save(path, bits=4, **extra)
        print("walking frames:", name)


if __name__ == "__main__":
    main()
