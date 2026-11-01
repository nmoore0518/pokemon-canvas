from PIL import Image
from pathlib import Path
R = str(Path(__file__).resolve().parents[3] / 'graphics/pokemon/icon_palettes') + '/'
PALS = [[tuple(int(v) for v in l.split()) for l in open(R + f'pal{i}.pal').read().split('\n')[3:19]] for i in range(6)]
OUTLINE = 15   # (65, 65, 65) in every palette

def small(front, size=29):
    from collections import Counter
    im = (front if isinstance(front, Image.Image) else Image.open(front)).convert('RGBA')
    im = im.crop(im.getchannel('A').getbbox())
    w, h = im.size
    s = min(size / w, size / h)
    W, H = max(1, round(w * s)), max(1, round(h * s))
    out = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    for Y in range(H):
        for X in range(W):
            x0, x1 = int(X / s), max(int(X / s) + 1, int((X + 1) / s))
            y0, y1 = int(Y / s), max(int(Y / s) + 1, int((Y + 1) / s))
            cells = [im.getpixel((x, y)) for x in range(x0, min(x1, w)) for y in range(y0, min(y1, h))]
            solid = [c for c in cells if c[3] >= 128]
            if len(solid) * 2 < len(cells): continue
            inner = [c[:3] for c in solid if sum(c[:3]) >= 90]
            pick = Counter(inner).most_common(1)[0][0] if len(inner) * 3 >= len(solid) else (16, 16, 16)
            out.putpixel((X, Y), pick + (255,))
    return out

CHOICE = {   # icon palette, and the colours of it to use (index 1-14)
    'hiveon': (4, [1, 2, 3, 4, 5, 6, 10, 12, 14]),
    'kirineon': (0, [1, 2, 3, 13, 14, 12]),
    'vitaleon': (3, [1, 2, 3, 4, 5, 9, 12, 14, 8]),
    'nimbeon': (0, [1, 2, 3, 12, 13, 6]),
    'obliveon': (2, [1, 2, 3, 11, 12, 14]),
    'terreon': (5, [1, 2, 3, 6, 7, 10, 13, 14]),
    'lumeon': (2, [1, 2, 3, 6, 7, 14]),
    'toxeon': (2, [1, 2, 3, 11, 12, 13, 14, 10]),
    'zirconeon': (2, [1, 2, 3, 4, 5, 14]),
    'metaleon': (3, [1, 2, 3, 4, 5, 6, 9, 14]),
}

def icon(front, key, pal_index=None):
    allowed = CHOICE[key][1] if key in CHOICE else list(range(1, 15))
    if key in CHOICE: pal_index = CHOICE[key][0]
    sm = small(front)
    W, H = sm.size
    best = None
    for pi in ([pal_index] if pal_index is not None else range(6)):
        pal = PALS[pi]
        err = 0
        for y in range(H):
            for x in range(W):
                p = sm.getpixel((x, y))
                if p[3] >= 110:
                    err += min(sum((a - b) ** 2 for a, b in zip(p[:3], pal[i])) for i in range(1, 15))
        if best is None or err < best[0]: best = (err, pi)
    pi = best[1]; pal = PALS[pi]
    g = [[0] * 32 for _ in range(32)]
    ox, oy = (32 - W) // 2, 31 - H
    for y in range(H):
        for x in range(W):
            p = sm.getpixel((x, y))
            if p[3] >= 110:
                g[oy + y][ox + x] = min(allowed, key=lambda i: sum((a - b) ** 2 for a, b in zip(p[:3], pal[i])))
    # outline
    out = [row[:] for row in g]
    for y in range(32):
        for x in range(32):
            if not g[y][x] and any(0 <= x + dx < 32 and 0 <= y + dy < 32 and g[y + dy][x + dx] and g[y + dy][x + dx] != OUTLINE
                                   for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = OUTLINE
    return out, pi

def save(g, pi, path):
    im = Image.new('P', (32, 64), 0)
    flat = [v for c in PALS[pi] for v in c]
    im.putpalette(flat + [0] * (768 - len(flat)))
    for y in range(32):
        for x in range(32):
            im.putpixel((x, y), g[y][x])
            if y + 1 < 32: im.putpixel((x, y + 1 + 32), g[y][x])   # (frame 2: a pixel lower)
    im.save(path, transparency=0)

def render(g, pi, s=6):
    im = Image.new('RGB', (32, 32), (255, 255, 255))
    for y in range(32):
        for x in range(32):
            if g[y][x]: im.putpixel((x, y), PALS[pi][g[y][x]])
    return im.resize((32 * s, 32 * s), Image.NEAREST)

if __name__ == '__main__':
    from edits import get
    names = ['hiveon', 'kirineon', 'vitaleon', 'nimbeon', 'obliveon', 'terreon', 'lumeon', 'toxeon', 'zirconeon', 'metaleon']
    out = Image.new('RGB', (10 * 200, 200), (255, 255, 255))
    for i, n in enumerate(names):
        g, pi = icon(get(n), n); print(n, pi)
        out.paste(render(g, pi), (i * 200, 0))
    out.save('icons_preview.png')
