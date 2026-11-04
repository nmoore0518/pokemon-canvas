#!/usr/bin/env python3
"""The Pokémon Canvas intro's graphics (src/intro_canvas.c).

    python3 tools/gym_hack/intro_canvas_gfx.py [preview.png]

Writes graphics/intro_canvas/:
  arena.png / arena.bin        the battle: a gym's battlefield at night (BG, 16 colours)
  rays.png / rays.bin          the badge montage's backdrop: light rays (BG, 3 colours: recoloured per badge)
  rainbow.png / rainbow.bin    the paintbrush's rainbow (BG, 16 colours)
  gym.png / gym.bin            Pallet Town's gym from the front, from the map itself (BG, 8bpp)
  badges.png                   the eight Kanto badges at 64x64, in colour (sprites; one palette each: badges_N.pal)
  badges_small.png             the same at 32x32 (the line-up: affine sprites that wide would be too many for a line)
  brush.png                    the paintbrush (sprite, 64x64)
  bolt.png                     Raichu's lightning (sprite, 64x64)
A BG's .png is its unique tiles (a 128-pixel-wide sheet); its .bin a 32x32 tilemap.
"""
import json
import math
import random
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "graphics/intro_canvas"
W, H = 240, 160


def write_pal(path, colors):
    lines = ["JASC-PAL", "0100", str(len(colors))] + [f"{r} {g} {b}" for r, g, b in colors]
    path.write_text("\n".join(lines) + "\n")


def gba(c):
    """(a colour as the GBA will show it: 5 bits a channel)"""
    return tuple((v >> 3) << 3 for v in c)


def tiles_and_map(img, name, bpp=4):
    """img: a P-mode image (W x H). Writes name.png (unique tiles) and name.bin (32x32 map)."""
    w, h = img.size
    px = img.load()
    seen, tiles, tmap = {}, [], [0] * (32 * 32)
    blank = tuple([0] * 64)
    seen[blank] = 0
    tiles.append(blank)
    for ty in range(h // 8):
        for tx in range(w // 8):
            t = tuple(px[tx * 8 + x, ty * 8 + y] for y in range(8) for x in range(8))
            if t not in seen:
                seen[t] = len(tiles)
                tiles.append(t)
            tmap[ty * 32 + tx] = seen[t]
    rows = (len(tiles) + 15) // 16
    sheet = Image.new("P", (128, rows * 8))
    sheet.putpalette(img.getpalette())
    sp = sheet.load()
    for i, t in enumerate(tiles):
        for y in range(8):
            for x in range(8):
                sp[(i % 16) * 8 + x, (i // 16) * 8 + y] = t[y * 8 + x]
    if bpp == 4:
        pal = img.getpalette()[:48]
        sheet.putpalette(pal + [0] * (768 - 48))
    sheet.save(OUT / f"{name}.png")
    (OUT / f"{name}.bin").write_bytes(struct.pack("<1024H", *tmap))
    return len(tiles)


def indexed(img_rgb, colors):
    """(RGB image -> P image over exactly these colours)"""
    pal = Image.new("P", (1, 1))
    flat = [v for c in colors for v in c]
    pal.putpalette(flat + [0] * (768 - len(flat)))
    return img_rgb.convert("RGB").quantize(palette=pal, dither=Image.Dither.NONE)


# ------------------------------------------------------------------------------------------ arena
ARENA = [(255, 0, 255), (8, 8, 24), (16, 16, 48), (24, 32, 72), (32, 48, 96), (48, 64, 120), (64, 88, 144),
         (96, 120, 168), (136, 160, 200), (184, 200, 224), (232, 240, 248), (88, 72, 40), (136, 112, 64),
         (176, 152, 96), (216, 200, 144), (248, 248, 248)]


def arena():
    im = Image.new("RGB", (W, H), ARENA[1])
    d = ImageDraw.Draw(im)
    horizon = 62
    # the back wall: dark bands, and the lights' beams
    for y in range(horizon):
        k = 1 + min(3, y * 4 // horizon)
        d.line([(0, y), (W, y)], fill=ARENA[k])
    for bx in (40, 120, 200):
        for y in range(horizon):
            half = 4 + y // 6
            d.line([(bx - half, y), (bx + half, y)], fill=ARENA[4] if y < horizon // 2 else ARENA[5])
        d.ellipse([bx - 6, 0, bx + 6, 6], fill=ARENA[10])
    # the stands' rail
    d.rectangle([0, horizon - 6, W, horizon - 1], fill=ARENA[2])
    for x in range(0, W, 12):
        d.line([(x, horizon - 6), (x, horizon - 1)], fill=ARENA[4])
    # the floor: light at the back, in perspective
    for y in range(horizon, H):
        t = (y - horizon) / (H - horizon)
        k = 6 if t < 0.15 else 5 if t < 0.5 else 4
        d.line([(0, y), (W, y)], fill=ARENA[k])
    # the field's markings: its outline, centre line and circle
    def fy(z):      # (z 0 = the back, 1 = the front)
        return horizon + 4 + z * (H - horizon - 4)
    def fx(x, z):   # (x -1..1 across the field)
        half = 70 + z * 120
        return W / 2 + x * half
    edge = ARENA[8]
    for z0, z1 in ((0, 1),):
        d.line([(fx(-1, z0), fy(z0)), (fx(-1, z1), fy(z1))], fill=edge)
        d.line([(fx(1, z0), fy(z0)), (fx(1, z1), fy(z1))], fill=edge)
    d.line([(fx(-1, 0), fy(0)), (fx(1, 0), fy(0))], fill=edge)
    zc = 0.45
    d.line([(fx(-1, zc), fy(zc)), (fx(1, zc), fy(zc))], fill=edge)
    cx, cy = W / 2, fy(zc)
    d.ellipse([cx - 34, cy - 11, cx + 34, cy + 11], outline=edge)
    d.ellipse([cx - 6, cy - 2, cx + 6, cy + 2], fill=edge)
    # the spotlights where the two sides stand
    for (ex, ey, rx, ry) in ((176, 94, 40, 10), (64, 146, 56, 14)):
        d.ellipse([ex - rx, ey - ry, ex + rx, ey + ry], fill=ARENA[7])
        d.ellipse([ex - rx + 6, ey - ry + 3, ex + rx - 6, ey + ry - 3], fill=ARENA[8])
    return indexed(im, ARENA)


# ------------------------------------------------------------------------------------------- rays
def rays():
    cols = [(255, 0, 255), (40, 40, 40), (80, 80, 80), (120, 120, 120)]
    im = Image.new("RGB", (W, H), cols[1])
    d = ImageDraw.Draw(im)
    cx, cy = W / 2, H / 2
    n = 16
    for i in range(n):
        a0 = (i / n) * 2 * math.pi
        a1 = a0 + math.pi / n
        pts = [(cx, cy), (cx + 300 * math.cos(a0), cy + 300 * math.sin(a0)), (cx + 300 * math.cos(a1), cy + 300 * math.sin(a1))]
        d.polygon(pts, fill=cols[2])
    d.ellipse([cx - 40, cy - 40, cx + 40, cy + 40], fill=cols[3])
    return indexed(im, cols + [(0, 0, 0)] * 12)


# ---------------------------------------------------------------------------------------- rainbow
RAINBOW = [(232, 48, 48), (248, 136, 32), (248, 216, 48), (72, 192, 72), (56, 136, 232), (104, 72, 200), (184, 88, 200)]


def rainbow():
    cols = [(255, 0, 255)] + RAINBOW + [(255, 255, 255)] + [(0, 0, 0)] * 7
    im = Image.new("RGB", (W, H), RAINBOW[0])
    d = ImageDraw.Draw(im)
    rnd = random.Random(7)
    band = H / len(RAINBOW)
    for i, c in enumerate(RAINBOW):
        top = i * band
        # (a brush stroke's ragged edge)
        pts = [(0, top + 2)]
        for x in range(0, W + 8, 8):
            pts.append((x, top + rnd.choice((0, 1, 2, 3))))
        pts += [(W, H), (0, H)]
        d.polygon(pts, fill=c)
    # bristle streaks
    for _ in range(140):
        y = rnd.randrange(H)
        x = rnd.randrange(W)
        i = min(len(RAINBOW) - 1, int(y / band))
        lighter = tuple(min(255, v + 40) for v in RAINBOW[i])
        d.line([(x, y), (x + rnd.randrange(12, 40), y)], fill=(255, 255, 255) if rnd.random() < 0.15 else RAINBOW[max(0, i - 1)] if rnd.random() < 0.2 else RAINBOW[i])
    return indexed(im, cols)


# ------------------------------------------------------------------------------------------- gym
def gym():
    sys.path.insert(0, str(ROOT / "tools/gym_hack"))
    import render_map as r
    L = {l.get("id"): l for l in json.loads((ROOT / "data/layouts/layouts.json").read_text())["layouts"] if "id" in l}
    j = json.loads((ROOT / "data/maps/PalletTown_Frlg/map.json").read_text())
    l = L[j["layout"]]
    prim = r.Tileset(r.tileset_dir(l["primary_tileset"]))
    sec = r.Tileset(r.tileset_dir(l["secondary_tileset"]))
    w = l["width"]
    b = struct.unpack(f"<{w * l['height']}H", (ROOT / l["blockdata_filepath"]).read_bytes())
    x0, y0 = GYM_VIEW
    im = Image.new("RGB", (W, H))
    for y in range(10):
        for x in range(15):
            r.draw_metatile(im, x * 16, y * 16, b[(y0 + y) * w + x0 + x] & 0x3FF, prim, sec)
    im = im.convert("RGB")
    q = im.quantize(colors=240, dither=Image.Dither.NONE)
    pal = q.getpalette()[:240 * 3]
    # (indices 0-15 are left to the rainbow, BG palette 0, which is on screen with it)
    out = Image.new("P", (W, H))
    out.putpalette([255, 0, 255] + [0] * 45 + pal + [0] * (768 - 48 - len(pal)))
    out.putdata([v + 16 for v in q.get_flattened_data()])
    return out


GYM_VIEW = (13, 15)       # (Pallet Town's blocks: its gym's door at (20, 21) is 7 across, 6 down)


# ----------------------------------------------------------------------------------------- badges
BADGE_COLOURS = [   # (dark .. light) Boulder, Cascade, Thunder, Rainbow, Soul, Marsh, Volcano, Earth
    [(64, 64, 72), (120, 120, 128), (176, 176, 184), (232, 232, 240)],
    [(24, 72, 160), (56, 128, 224), (120, 184, 248), (216, 240, 255)],
    [(160, 72, 8), (232, 136, 24), (248, 200, 72), (255, 248, 200)],
    [(48, 136, 48), (96, 192, 80), (232, 120, 168), (255, 232, 248)],
    [(152, 40, 112), (216, 88, 168), (248, 152, 208), (255, 232, 248)],
    [(144, 104, 16), (208, 160, 32), (248, 216, 72), (255, 248, 200)],
    [(152, 24, 24), (224, 64, 40), (248, 152, 72), (255, 232, 200)],
    [(32, 104, 56), (64, 160, 88), (128, 216, 136), (224, 255, 224)],
]


def scale2x(im):
    w, h = im.size
    p = im.load()
    out = Image.new(im.mode, (w * 2, h * 2))
    if im.mode == "P":
        out.putpalette(im.getpalette())
    o = out.load()
    for y in range(h):
        for x in range(w):
            P = p[x, y]
            A = p[x, y - 1] if y > 0 else P
            B = p[x + 1, y] if x < w - 1 else P
            C = p[x - 1, y] if x > 0 else P
            D = p[x, y + 1] if y < h - 1 else P
            e0 = A if (C == A and C != D and A != B) else P
            e1 = B if (A == B and A != C and B != D) else P
            e2 = C if (D == C and D != B and C != A) else P
            e3 = D if (B == D and B != A and D != C) else P
            o[2 * x, 2 * y], o[2 * x + 1, 2 * y], o[2 * x, 2 * y + 1], o[2 * x + 1, 2 * y + 1] = e0, e1, e2, e3
    return out


def badges():
    src = Image.open(ROOT / "graphics/trainer_card/frlg/badges.png")
    sheet = Image.new("P", (64, 64 * 8))
    small = Image.new("P", (32, 32 * 8))
    for i in range(8):
        b = src.crop((i * 16, 0, i * 16 + 16, 16))
        big = scale2x(scale2x(b))
        # grey levels 1 (white) 2 3 4 (darker) 15 (outline) -> the badge's colours
        remap = {0: 0, 1: 5, 2: 4, 3: 3, 4: 2, 15: 1}
        sheet.paste(big.point(lambda v: remap.get(v, 1)), (0, i * 64))
        small.paste(scale2x(b).point(lambda v: remap.get(v, 1)), (0, i * 32))
        cols = [(255, 0, 255), (16, 16, 24)] + BADGE_COLOURS[i] + [(255, 255, 255)] + [(0, 0, 0)] * 9
        write_pal(OUT / f"badge_{i}.pal", [gba(c) for c in cols])
    cols = [(255, 0, 255), (16, 16, 24)] + BADGE_COLOURS[0] + [(255, 255, 255)] + [(0, 0, 0)] * 9
    sheet.putpalette([v for c in cols for v in c] + [0] * (768 - 48))
    sheet.save(OUT / "badges.png")
    small.putpalette(sheet.getpalette())
    small.save(OUT / "badges_small.png")


# ------------------------------------------------------------------------------------------ brush
def brush():
    """A broad paintbrush, dragged to the right: bristles (dipped in the rainbow, top to bottom) on the
    left, the ferrule, the handle out to the right."""
    cols = [(255, 0, 255), (24, 16, 16), (112, 64, 32), (160, 104, 48), (208, 160, 96), (120, 128, 144),
            (176, 184, 200), (240, 240, 248), (96, 72, 48)] + RAINBOW
    im = Image.new("RGB", (64, 64), cols[0])
    d = ImageDraw.Draw(im)
    # handle (tapering), with a highlight
    d.polygon([(24, 22), (60, 27), (62, 31), (60, 36), (24, 42)], fill=cols[3], outline=cols[1])
    d.line([(26, 26), (58, 29)], fill=cols[4])
    d.line([(26, 39), (58, 34)], fill=cols[2])
    # ferrule
    d.rectangle([14, 6, 25, 57], fill=cols[6], outline=cols[1])
    d.line([(17, 8), (17, 55)], fill=cols[7])
    d.line([(23, 8), (23, 55)], fill=cols[5])
    for y in (18, 46):
        d.line([(15, y), (24, y)], fill=cols[5])
    # bristles: the wood-brown root, then the paint, a band of the rainbow each
    d.rectangle([3, 4, 13, 59], fill=cols[8], outline=cols[1])
    n = len(RAINBOW)
    for i, c in enumerate(RAINBOW):
        y0 = 5 + i * 54 // n
        y1 = 5 + (i + 1) * 54 // n - 1
        d.rectangle([1, y0, 9, y1], fill=c)
        d.line([(0, y0 + 1), (0, y1 - 1)], fill=c)
    d.line([(10, 5), (10, 58)], fill=cols[1])
    return indexed(im, cols)


def bolt():
    """Raichu's lightning: a jagged bolt from bottom-left to top-right (shown at double size)."""
    cols = [(255, 0, 255), (40, 32, 8), (248, 200, 32), (255, 240, 120), (255, 255, 255)] + [(0, 0, 0)] * 11
    im = Image.new("RGB", (64, 64), cols[0])
    d = ImageDraw.Draw(im)
    pts = [(6, 50), (20, 38), (17, 33), (33, 26), (30, 21), (46, 16), (44, 12), (58, 8)]
    d.line(pts, fill=cols[1], width=7, joint="curve")
    d.line(pts, fill=cols[2], width=5, joint="curve")
    d.line(pts, fill=cols[3], width=3)
    d.line(pts, fill=cols[4], width=1)
    return indexed(im, cols)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    counts = {}
    for name, fn, bpp in (("arena", arena, 4), ("rays", rays, 4), ("rainbow", rainbow, 4), ("gym", gym, 8)):
        img = fn()
        counts[name] = tiles_and_map(img, name, bpp)
        globals()[f"_img_{name}"] = img
    badges()
    brush().save(OUT / "brush.png")
    bolt().save(OUT / "bolt.png")
    print("tiles:", counts)
    if len(sys.argv) > 1:
        imgs = [globals()[f"_img_{n}"].convert("RGB") for n in ("arena", "rays", "rainbow", "gym")]
        b = Image.open(OUT / "badges.png")
        prev = Image.new("RGB", (W * 2 + 72, H * 2 + 8), (0, 0, 0))
        for i, im in enumerate(imgs):
            prev.paste(im, ((i % 2) * W, (i // 2) * H))
        for i in range(8):
            pal = [tuple(map(int, l.split())) for l in (OUT / f"badge_{i}.pal").read_text().splitlines()[3:]]
            bi = b.crop((0, i * 64, 64, i * 64 + 64))
            bi.putpalette([v for c in pal for v in c] + [0] * (768 - 48))
            prev.paste(bi.convert("RGB").resize((36, 36)), (W * 2 + 2 + (i % 2) * 36, (i // 2) * 40))
        prev.paste(Image.open(OUT / "brush.png").convert("RGB"), (W * 2 + 4, 200))
        prev.resize((prev.width * 2, prev.height * 2), Image.NEAREST).save(sys.argv[1])


if __name__ == "__main__":
    main()
