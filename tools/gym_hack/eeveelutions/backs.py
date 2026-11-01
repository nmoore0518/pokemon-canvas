import math
from PIL import Image, ImageDraw
from to_gba import to_gba
from pathlib import Path
from edits import get
R = str(Path(__file__).resolve().parents[3] / 'graphics/pokemon') + '/'
BLACK = (0, 0, 0)

def base(name):
    im = Image.open(R + name + '/back.png')
    return [[im.getpixel((x, y)) for x in range(64)] for y in range(64)]

def nearest(pal, c):
    return min(range(1, 16), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, pal[i])))

def mask(fn):
    im = Image.new('L', (64, 64), 0); fn(ImageDraw.Draw(im))
    return {(x, y) for y in range(64) for x in range(64) if im.getpixel((x, y))}

def fill(g, cells, idx, outline=None, behind=False):
    for (x, y) in cells:
        if not (0 <= x < 64 and 0 <= y < 64) or (behind and g[y][x]): continue
        edge = any((x + dx, y + dy) not in cells for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        g[y][x] = outline if (outline is not None and edge) else idx

def leaf(r, t, w, k=12):
    (rx, ry), (tx, ty) = r, t
    L = math.hypot(tx - rx, ty - ry); ux, uy = (tx - rx) / L, (ty - ry) / L; px, py = -uy, ux
    side1, side2 = [], []
    for i in range(k + 1):
        f = i / k; h = w * math.sin(math.pi * f) ** 0.8
        cx, cy = rx + ux * L * f, ry + uy * L * f
        side1.append((cx + px * h, cy + py * h)); side2.append((cx - px * h, cy - py * h))
    return side1 + side2[::-1]

# design: front name, base back, {base index: target colour}, features(g, P) where P(colour) -> index
DESIGNS = {}

def lenneon_feat(g, P):
    for r, t, w in (((22, 44), (2, 10), 7), ((26, 46), (8, 22), 5), ((42, 44), (62, 8), 7), ((40, 46), (58, 22), 5)):
        poly = leaf(r, t, w)
        m = mask(lambda d: d.polygon(poly, fill=255))
        fill(g, m, P((190, 185, 182)), outline=P((71, 80, 93)), behind=True)
        vein = mask(lambda d: d.line([r, t], fill=255))
        for (x, y) in vein:
            if g[y][x] == P((190, 185, 182)): g[y][x] = P((123, 124, 129))
DESIGNS['lenneon'] = ('hiveon', 'glaceon', {3: (137, 171, 157), 2: (93, 134, 126), 1: (49, 103, 103), 8: (35, 77, 71), 5: (35, 77, 71),
    6: (49, 103, 103), 7: (93, 134, 126), 9: (255, 255, 255), 4: BLACK, 10: (136, 0, 21), 11: (136, 0, 21)}, lenneon_feat)

DESIGNS['valyreon'] = ('kirineon', 'vaporeon', {10: (104, 174, 160), 11: (152, 200, 190), 12: (70, 130, 122), 2: (52, 86, 85), 1: (18, 46, 38),
    7: (121, 118, 141), 9: (90, 88, 84), 8: (90, 88, 84), 5: (71, 71, 71), 3: (90, 88, 84), 4: (71, 71, 71), 6: BLACK,
    13: (169, 67, 66), 14: (169, 67, 66), 15: (169, 67, 66)}, None)

DESIGNS['assasseon'] = ('vitaleon', 'umbreon', {7: (64, 100, 116), 2: (91, 127, 141), 1: (45, 72, 91), 11: (31, 47, 72), 3: BLACK,
    5: (187, 157, 123), 6: (126, 102, 78), 4: (72, 42, 6), 10: (133, 41, 4), 12: (165, 82, 57), 8: (72, 42, 6), 9: (255, 255, 255)}, None)

def wings_cyan(g, P):
    for r, t, w in (((24, 46), (0, 6), 9), ((44, 46), (63, 4), 9)):
        poly = leaf(r, t, w)
        m = mask(lambda d: d.polygon(poly, fill=255))
        fill(g, m, P((164, 230, 230)), outline=P((24, 98, 90)), behind=True)
        for k in (0.35, 0.6, 0.85):
            q = (r[0] + (t[0] - r[0]) * k, r[1] + (t[1] - r[1]) * k)
            vein = mask(lambda d: d.line([r, q], fill=255))
            for (x, y) in vein:
                if g[y][x] == P((164, 230, 230)): g[y][x] = P((98, 180, 172))
DESIGNS['hypereon'] = ('nimbeon', 'espeon', {2: (255, 255, 255), 5: (255, 255, 255), 4: (205, 205, 213), 13: (148, 148, 172), 1: (82, 82, 90),
    10: (164, 230, 230), 8: (180, 32, 16), 11: (115, 16, 8), 6: (156, 49, 98), 7: (215, 104, 94), 12: (205, 205, 213), 3: BLACK,
    14: (180, 32, 16), 15: (215, 104, 94), 9: (255, 255, 255)}, wings_cyan)

def bands(g, P):
    dark, mid = P((125, 95, 121)), P((173, 133, 156))
    light = {P((219, 172, 206)), P((237, 201, 224))}
    for y in range(64):
        if y % 8 in (0, 1, 2):
            for x in range(64):
                if g[y][x] in light or (g[y][x] == mid and y % 8 == 1): g[y][x] = P((82, 64, 90)) if y % 8 == 1 else dark
DESIGNS['appariteon'] = ('obliveon', 'espeon', {5: (219, 172, 206), 2: (237, 201, 224), 4: (173, 133, 156), 13: (125, 95, 121), 1: (82, 64, 90),
    10: (156, 137, 173), 8: (173, 133, 156), 11: (125, 95, 121), 6: (82, 64, 90), 7: (156, 137, 173), 12: (237, 201, 224), 3: BLACK,
    14: (125, 95, 121), 15: (173, 133, 156), 9: (249, 230, 248)}, bands)

DESIGNS['subterraneon'] = ('terreon', 'eevee', {3: (186, 163, 109), 1: (140, 116, 68), 2: (93, 69, 23), 5: (93, 69, 23), 12: (93, 69, 23), 4: BLACK,
    6: BLACK, 8: (236, 136, 87), 10: (236, 136, 87), 11: (189, 97, 60), 7: (139, 74, 32), 9: (236, 211, 155)}, None)

DESIGNS['odeon'] = ('lumeon', 'jolteon', {7: (244, 240, 230), 6: (255, 255, 255), 3: (214, 204, 188), 4: (214, 204, 188), 5: (152, 98, 56), 2: (92, 56, 36),
    1: BLACK, 9: (204, 144, 76), 11: (152, 98, 56), 12: (92, 56, 36), 10: (92, 56, 36), 8: (16, 16, 16)}, None)

def spots(g, P):
    for (x, y) in ((24, 40), (30, 46), (36, 38), (20, 50), (40, 48)):
        for (dx, dy) in ((0, 0), (1, 0), (0, 1), (1, 1)):
            if g[y + dy][x + dx] in (P((245, 231, 231)), P((198, 162, 174))): g[y + dy][x + dx] = P((181, 90, 165))
DESIGNS['poiseon'] = ('toxeon', 'sylveon', {11: (130, 93, 173), 12: (77, 54, 110), 8: (55, 28, 83), 7: (245, 231, 231), 4: (198, 162, 174),
    10: (93, 55, 66), 1: (77, 15, 56), 13: (181, 90, 165), 5: (130, 93, 173), 6: (77, 54, 110), 2: (55, 28, 83), 3: BLACK, 9: (255, 255, 255)}, spots)

DESIGNS['obsideon'] = ('zirconeon', 'leafeon', {10: (94, 92, 104), 11: (84, 81, 96), 13: (73, 71, 85), 12: (56, 56, 62), 8: (56, 56, 62), 6: (84, 81, 96),
    5: (27, 27, 29), 7: (27, 27, 29), 15: (27, 27, 29), 4: (27, 27, 29), 2: (211, 220, 245), 9: (183, 189, 236), 3: (123, 135, 199),
    1: (136, 125, 186), 14: (248, 248, 254)}, None)

def gearback(g, P):
    body = {P((70, 82, 104)), P((112, 126, 150)), P((168, 182, 204))}
    for y in (14, 20):
        for x in range(64):
            if g[y][x] in body: g[y][x] = P((36, 44, 60))
DESIGNS['automateon'] = ('metaleon', 'eevee', {3: (112, 126, 150), 1: (70, 82, 104), 2: (36, 44, 60), 5: (36, 44, 60), 12: (36, 44, 60), 4: BLACK, 6: BLACK,
    8: (214, 164, 64), 10: (252, 220, 120), 7: (150, 104, 40), 11: (214, 164, 64), 9: (236, 244, 255)}, gearback)

def make(key):
    """front, back, palette. Which back each has:
      TURNED      the design turned away (turned.py)
      DRAWN       a back sprite supplied for it (source/<key>_back.png, 1x)
      otherwise   an official eeveelution back repainted in its colours (official_back), with
                  Hypereon's ears made into wings"""
    from turned import turned
    front = DESIGNS[key][0]
    fg, pal = to_gba(get(front))
    if key in TURNED:
        bg = turned(fg, pal, key)
    elif key in DRAWN:
        bg = drawn_back(key, pal)
    else:
        bg = official_back(key)
        if key == 'hypereon':
            wing_ears(bg, lambda c: nearest(pal, c))
    return fg, bg, pal

TURNED = {'obsideon', 'subterraneon'}
DRAWN = {'valyreon'}

def drawn_back(key, pal):
    """source/<key>_back.png: 64x64 at most once the bottom runs off the frame"""
    im = Image.open(Path(__file__).resolve().parent / 'source' / f'{key}_back.png').convert('RGBA')
    im = im.crop(im.getchannel('A').getbbox())
    w, h = im.size
    g = [[0] * 64 for _ in range(64)]
    ox = (64 - w) // 2
    for y in range(min(h, 64)):
        for x in range(w):
            p = im.getpixel((x, y))
            if p[3] >= 128 and 0 <= ox + x < 64:
                g[y + max(0, 64 - h)][ox + x] = nearest(pal, p[:3])
    return g

def wing_ears(g, P):
    """Hypereon: its ears (the official back's) turned into small cyan wings, feathered"""
    cy, dcy, edge, light = P((164, 230, 230)), P((98, 180, 172)), P((24, 98, 90)), P((152, 238, 213))
    white, grey, line, black = P((255, 255, 255)), P((205, 205, 213)), P((82, 82, 90)), P((16, 16, 16))
    def ear(x, y):
        return (17 <= x <= 44 and y < 17 + (40 - x) * 5 / 21) or (x >= 46 and y < 17 + (x - 46) * 15 / 12)
    for y in range(64):
        for x in range(64):
            v = g[y][x]
            if v and ear(x, y):
                g[y][x] = {white: light, grey: cy, line: edge, black: edge}.get(v, v)
    for (x0, y0, x1, y1) in ((20, 12, 34, 16), (24, 11, 36, 15), (48, 12, 58, 24), (50, 10, 60, 18)):
        for (x, y) in mask(lambda d: d.line([(x0, y0), (x1, y1)], fill=255)):
            if g[y][x] in (cy, light): g[y][x] = dcy

def official_back(key):
    front, b, m, feat = DESIGNS[key]
    fg, pal = to_gba(get(front))
    src = base(b)
    lut = {i: nearest(pal, c) for i, c in m.items()}
    g = [[(lut.get(v, nearest(pal, BLACK)) if v else 0) for v in row] for row in src]
    if feat: feat(g, lambda c: nearest(pal, c))
    return g

def render(g, pal, s):
    im = Image.new('RGB', (64, 64), (255, 255, 255))
    for y in range(64):
        for x in range(64):
            if g[y][x]: im.putpixel((x, y), pal[g[y][x]])
    return im.resize((64 * s, 64 * s), Image.NEAREST)

if __name__ == '__main__':
    out = Image.new('RGB', (10 * 200, 400), (255, 255, 255))
    for i, k in enumerate(DESIGNS):
        fg, bg, pal = make(k)
        out.paste(render(fg, pal, 3), (i * 200, 0)); out.paste(render(bg, pal, 3), (i * 200, 200))
    out.save('backs_preview.png')
