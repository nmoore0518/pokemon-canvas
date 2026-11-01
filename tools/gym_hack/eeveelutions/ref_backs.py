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
    'automateon': dict(crop=(36, 44, 250, 288), tol=38, height=72, off=8, extra=(), avoid=set(), thicken=False),
    'poiseon':    dict(crop=None, tol=30, height=72, off=8, extra=(), avoid=set(), thicken=False),
    'lenneon':    dict(crop=(0, 0, 220, 248), tol=40, height=72, off=8, extra=((175, 208, 108),), avoid=set(), thicken=False),
}


def cut(key):
    r = REFS[key]
    im = Image.open(HERE / 'source' / f'{key}_back_ref.png').convert('RGBA')
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
    ox, oy = (64 - W) // 2, 64 - H + r['off']
    for Y in range(H):
        for X in range(W):
            cr, cg, cb, a = sm.getpixel((X, Y))
            x, y = ox + X, oy + Y
            if a >= 128 and 0 <= x < 64 and 0 <= y < 64:
                g[y][x] = near((cr * 255 // a, cg * 255 // a, cb * 255 // a))
    for y in range(63):
        for x in range(64):
            if g[y][x] and any(not (0 <= x + dx < 64 and 0 <= y + dy < 64) or not g[y + dy][x + dx]
                               for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                g[y][x] = outline
    return g
