"""Back sprites adapted from reference pictures Nick supplied (source/<key>_back_ref.png: not
GBA-sized; some painted, some pixel art): the background is cut away, the picture shrunk to the
back sprite's size (its bottom running off the frame) and put in the design's palette, with an
outline.

REFS: crop box (None: all), background tolerance, height (the frame is 64), rows off the bottom,
extra colours (taking the palette's free slots: the front doesn't use them), colours not to use, and
whether to thicken the dark lines first (painted art, whose fine lines vanish when shrunk).
"""
from collections import deque
from pathlib import Path
from PIL import Image, ImageFilter

HERE = Path(__file__).resolve().parent
REFS = {
    'appariteon': dict(crop=None, tol=40, height=80, off=14, extra=(), avoid={(127, 127, 127)}, thicken=True),
    'automateon': dict(crop=(25, 0, 259, 294), tol=38, height=84, off=20, extra=(), avoid=set(), thicken=False,
                       frame=((40, 41, 42), (22, 23, 24, 259, 260, 261))),
    'assasseon':  dict(crop=(0, 0, 258, 235), tol=38, height=72, off=8, extra=(), avoid=set(), thicken=False),
    'hypereon':   dict(crop=(0, 0, 262, 350), tol=34, height=72, off=8, extra=(), avoid=set(), thicken=False),
    'odeon':      dict(crop=(5, 52, 265, 282), tol=38, height=72, off=8, extra=(), avoid=set(), thicken=False,
                       mane=((204, 144, 76), (152, 98, 56), (92, 56, 36))),
    'poiseon':    dict(crop=None, tol=30, height=72, off=8, extra=(), avoid=set(), thicken=False),
    'lenneon':    dict(crop=(0, 0, 220, 248), tol=40, height=72, off=8, extra=((175, 208, 108),), avoid=set(), thicken=False),
}


def cut(key):
    r = REFS[key]
    im = Image.open(HERE / 'source' / f'{key}_back_ref.png').convert('RGBA')
    if r.get('frame'):
        # a picture frame drawn across it: each frame line takes the pixels beside it
        rows, cols = r['frame']
        px = im.load()
        w, h = im.size
        for y in rows:
            for x in range(w):
                px[x, y] = px[x, min(rows) - 1] if px[x, max(rows) + 1][:3] == px[x, min(rows) - 1][:3] else px[x, max(rows) + 1]
        for x in cols:
            for y in range(h):
                px[x, y] = px[min(cols) - 1 if x < w // 2 else max(c for c in cols if c > w // 2) + 1, y]
    if r['crop']:
        im = im.crop(r['crop'])
    w, h = im.size
    px = im.load()
    tol = r['tol']
    seen, q = set(), deque()
    for x in range(w):
        q.append((x, 0)); q.append((x, h - 1))
    for y in range(h):
        q.append((0, y)); q.append((w - 1, y))
    while q:
        x, y = q.popleft()
        if (x, y) in seen or not (0 <= x < w and 0 <= y < h):
            continue
        seen.add((x, y))
        cr, cg, cb, _a = px[x, y]
        # the background: light and unsaturated (white, grey, cream)
        if min(cr, cg, cb) < 255 - 2.2 * tol or max(cr, cg, cb) - min(cr, cg, cb) > tol:
            continue
        px[x, y] = (0, 0, 0, 0)
        q.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))
    im = im.crop(im.getchannel('A').getbbox())
    if r['thicken']:
        a = im.getchannel('A')
        im = im.convert('RGB').filter(ImageFilter.MinFilter(5)).convert('RGBA')
        im.putalpha(a)
    return im


def add_colours(key, pal, front):
    """the extra colours into palette slots the front doesn't use"""
    used = {v for row in front for v in row}
    free = [i for i in range(1, 16) if i not in used]
    pal = list(pal)
    for c in REFS[key]['extra']:
        pal[free.pop(0)] = c
    return pal


def ref_back(key, pal):
    r = REFS[key]
    src = cut(key)
    w, h = src.size
    s = min(r['height'] / h, 64 / w)
    W, H = round(w * s), round(h * s)
    prem = Image.new('RGBA', src.size)
    sp, pp = src.load(), prem.load()
    for y in range(h):
        for x in range(w):
            cr, cg, cb, a = sp[x, y]
            pp[x, y] = (cr * a // 255, cg * a // 255, cb * a // 255, a)
    sm = prem.resize((W, H), Image.BOX)
    ok = [i for i in range(1, 16) if pal[i] not in r['avoid']]
    def near(c):
        return min(ok, key=lambda i: sum((p - q) ** 2 for p, q in zip(c, pal[i])))
    outline = min(range(1, 16), key=lambda i: sum(pal[i]))
    g = [[0] * 64 for _ in range(64)]
    mane = set()
    ox, oy = (64 - W) // 2, 64 - H + r['off']
    for Y in range(H):
        for X in range(W):
            cr, cg, cb, a = sm.getpixel((X, Y))
            x, y = ox + X, oy + Y
            if a >= 128 and 0 <= x < 64 and 0 <= y < 64:
                c = (cr * 255 // a, cg * 255 // a, cb * 255 // a)
                g[y][x] = near(c)
                if r.get('mane') and c[0] > 190 and c[0] - c[2] > 40 and c[1] > 150:
                    mane.add((x, y))
    if mane:
        spiky_mane(g, mane, [near(m) for m in r['mane']], near((244, 240, 230)))
    drop_specks(g, 10)
    for y in range(63):
        for x in range(64):
            if g[y][x] and any(not (0 <= x + dx < 64 and 0 <= y + dy < 64) or not g[y + dy][x + dx]
                               for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                g[y][x] = outline
    return g


def drop_specks(g, n):
    """pieces of fewer than n pixels, cut off from the rest, go"""
    seen = set()
    for y in range(64):
        for x in range(64):
            if g[y][x] and (x, y) not in seen:
                part, q = [], deque([(x, y)])
                seen.add((x, y))
                while q:
                    a, b = q.popleft(); part.append((a, b))
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        c = (a + dx, b + dy)
                        if 0 <= c[0] < 64 and 0 <= c[1] < 64 and g[c[1]][c[0]] and c not in seen:
                            seen.add(c); q.append(c)
                if len(part) < n:
                    for (a, b) in part:
                        g[b][a] = 0


def spiky_mane(g, mane, ramp, cream):
    """Odeon: the neck ruffle in its front's brown, with spikes along its lower edge and ends like
    its front's mane (only the ruffle: the biggest patch; the rest of that colour is cream)"""
    light, mid, dark = ramp
    parts, seen = [], set()
    for p in mane:
        if p in seen:
            continue
        part, q = [], deque([p]); seen.add(p)
        while q:
            a, b = q.popleft(); part.append((a, b))
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                c = (a + dx, b + dy)
                if c in mane and c not in seen:
                    seen.add(c); q.append(c)
        parts.append(part)
    ruff = set(max(parts, key=len))
    top = min(y for _, y in ruff)
    ruff = {(x, y) for (x, y) in ruff if y <= top + 11}     # (a collar, not the whole back)
    for (x, y) in mane - ruff:
        g[y][x] = cream
    ys = [y for _, y in ruff]
    bot = max(ys)
    for (x, y) in ruff:
        g[y][x] = light if (y - top) / max(1, bot - top) < 0.45 else mid
    def spike(base, tip, w):
        (bx, by), (tx, ty) = base, tip
        L = max(abs(tx - bx), abs(ty - by))
        for i in range(L + 1):
            f = i / L
            cx, cy = round(bx + (tx - bx) * f), round(by + (ty - by) * f)
            half = round(w * (1 - f))
            for j in range(-half, half + 1):
                px, py = (cx + j, cy) if ty != by else (cx, cy + j)
                if 0 <= px < 64 and 0 <= py < 64:
                    edge = abs(j) == half and half > 0
                    g[py][px] = dark if edge or f >= 0.85 else (light if j < 0 and f < 0.6 else mid)
    cols = sorted({x for x, _ in ruff})
    for i, x in enumerate(range(cols[0] + 3, cols[-1] - 2, 6)):
        low = max((y for (xx, y) in ruff if xx == x), default=None)
        if low is not None:
            spike((x, low - 1), (x + 1, low + (8 if i % 2 == 0 else 5)), 3)
    for side, x in ((-1, cols[0]), (1, cols[-1])):
        rows = sorted(y for (xx, y) in ruff if xx == x)
        for y in rows[2::5]:
            spike((x - side, y), (x + side * 5, y + 2), 2)
