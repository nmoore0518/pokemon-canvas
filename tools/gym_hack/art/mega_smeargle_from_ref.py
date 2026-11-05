"""Nano Banana's Mega Smeargle (ref.png, 1024x1024: front above, back below) -> 64x64 GBA sprites.
Usage: python3 convert.py ref.png outdir"""
import sys
from collections import Counter, deque
from PIL import Image

src, out = sys.argv[1], sys.argv[2]
im = Image.open(src).convert("RGB")
W, H = im.size
p = im.load()


def bglike(c):
    r, g, b = c
    return max(c) - min(c) < 18 and sum(c) > 450


# background: grey/white regions that are big (the eye's white is small)
bg = bytearray(W * H)
seen = bytearray(W * H)
for y in range(H):
    for x in range(W):
        i = y * W + x
        if seen[i] or not bglike(p[x, y]):
            continue
        comp, q = [], deque([(x, y)])
        seen[i] = 1
        while q:
            cx, cy = q.popleft()
            comp.append(cy * W + cx)
            for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                if 0 <= nx < W and 0 <= ny < H:
                    j = ny * W + nx
                    if not seen[j] and bglike(p[nx, ny]):
                        seen[j] = 1
                        q.append((nx, ny))
        if len(comp) > 1500:
            for j in comp:
                bg[j] = 1


def bbox(y0, y1):
    xs, ys = [], []
    for y in range(y0, y1):
        for x in range(W):
            if not bg[y * W + x]:
                xs.append(x); ys.append(y)
    return min(xs), min(ys), max(xs) + 1, max(ys) + 1


SPLIT = 515
boxes = {"front": bbox(0, SPLIT), "back": bbox(SPLIT, H)}
print(boxes)

OUTLINE = (16, 16, 16)
palette = [(0, 176, 232), OUTLINE,
           (248, 248, 248),                                  # 2 white
           (240, 224, 200), (216, 194, 168), (184, 160, 136),  # 3-5 cream: light, mid, shadow
           (160, 96, 56), (112, 64, 40),                      # 6-7 brown
           (128, 80, 136), (88, 52, 96),                      # 8-9 purple
           (128, 168, 60), (92, 128, 44),                     # 10-11 green
           (232, 144, 32), (176, 100, 24),                    # 12-13 orange
           (224, 112, 128),                                   # 14 tongue
           (200, 140, 96)]                                    # 15 brown highlight
PRIORITY = {2: 0.25, 14: 0.2, 8: 0.3, 10: 0.3, 12: 0.3}       # (small details win a cell)
DARK_FRAC = float(sys.argv[3]) if len(sys.argv) > 3 else 0.28


def nearest(c):
    return min(range(2, 16), key=lambda k: sum((a - b) ** 2 for a, b in zip(c, palette[k])))


def convert(box, size=64, mult=1.0, cx=None, top=None):
    """(the art in box, fitted into size x size; mult > 1: bigger, the frame a window onto it
    centred on source x cx, its top at source y top, what's outside cut off)"""
    x0, y0, x1, y1 = box
    s = max((x1 - x0) / size, (y1 - y0) / size) / mult
    img = Image.new("P", (size, size))
    o = img.load()
    if mult == 1.0:
        w, h = int((x1 - x0) / s + 0.999), int((y1 - y0) / s + 0.999)
        left, ttop, ox, oy = x0, y0, (size - w) // 2, size - h
    else:
        w = h = size
        left, ttop, ox, oy = (cx - size * s / 2), top, 0, 0
    for ty in range(h):
        for tx in range(w):
            sx0, sy0 = left + tx * s, ttop + ty * s
            cnt, dark, bgn, cols = 0, 0, 0, Counter()
            for yy in range(int(sy0), int(sy0 + s)):
                for xx in range(int(sx0), int(sx0 + s)):
                    if xx >= W or yy >= H or xx < 0 or yy < 0:
                        continue
                    cnt += 1
                    if bg[yy * W + xx] or not (y0 <= yy < y1):
                        bgn += 1
                        continue
                    c = p[xx, yy]
                    if sum(c) < 160:
                        dark += 1
                    else:
                        cols[nearest(c)] += 1
            if cnt == 0 or bgn > cnt * 0.55:
                continue
            if dark > cnt * DARK_FRAC:
                o[ox + tx, oy + ty] = 1
                continue
            tot = sum(cols.values())
            pick = None
            for k, f in PRIORITY.items():
                if tot and cols[k] >= tot * f and (pick is None or cols[k] > cols[pick]):
                    pick = k
            o[ox + tx, oy + ty] = pick or (cols.most_common(1)[0][0] if cols else 1)
    # cleanup: lone pixels go; the silhouette's edge is outline
    for _ in range(2):
        for y in range(size):
            for x in range(size):
                if o[x, y] and sum(1 for dx in (-1, 0, 1) for dy in (-1, 0, 1) if (dx or dy)
                                   and 0 <= x + dx < size and 0 <= y + dy < size and o[x + dx, y + dy]) <= 1:
                    o[x, y] = 0
    edge = []
    for y in range(size):
        for x in range(size):
            if o[x, y] and any(((0 <= x + dx < size and 0 <= y + dy < size) and not o[x + dx, y + dy])
                               or (mult == 1.0 and not (0 <= x + dx < size and 0 <= y + dy < size))
                               for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                edge.append((x, y))
    for q in edge:
        o[q] = 1
    flat = [v for c in palette for v in c]
    img.putpalette(flat + [0] * (768 - len(flat)))
    return img


import os
# The back sprite: drawn 1.4x as big (Smeargle's own size), the frame a window onto its top half
# (the lower body cut off, as back sprites are; the outstretched arm off the left edge). 1.4 is
# as big as it goes with the whole head and brush in the frame.
BACK_SCALE = float(os.environ.get("BACK_SCALE", "1.4"))
BACK_CX = float(os.environ.get("BACK_CX", "598"))      # (source x at the frame's centre)
convert(boxes["front"]).save(f"{out}/front.png")
bx = boxes["back"]
convert(bx, mult=BACK_SCALE, cx=BACK_CX, top=bx[1] - float(os.environ.get("BACK_TOP", "16"))).save(f"{out}/back.png")
