"""Back sprites drawn from each design's own front (not the official eeveelutions' backs): the
Pokémon turned away from the camera. The front is mirrored (so it faces into the battle), its face
is painted over with the fur around it (eyes, mouth, blush, Lenneon's glasses: what you'd only see
from the front), and it's drawn a third bigger with its feet off the bottom of the frame, as the
player's Pokémon is."""
from collections import Counter

# the colours only the face shows (eyes, mouth, blush, glasses), and the head's region to clear
# (front-sprite coordinates); see FACE_FUR for colours that become fur there
FACE = {
    'lenneon':      ({(136, 0, 21), (100, 0, 15), (224, 184, 64), (255, 255, 234), (255, 255, 255)}, (36, 34, 54, 46)),
    'valyreon':     ({(169, 67, 66)}, (12, 20, 40, 40)),
    'assasseon':    ({(72, 42, 6), (133, 41, 4), (165, 82, 57)}, (20, 16, 48, 44)),
    'hypereon':     ({(180, 32, 16), (115, 16, 8), (215, 104, 94), (156, 49, 98)}, (32, 22, 54, 44)),
    'appariteon':   ({(127, 127, 127)}, (10, 10, 40, 40)),
    'subterraneon': ({(186, 78, 93), (152, 66, 65)}, (28, 22, 50, 38)),
    'odeon':        ({(16, 16, 16)}, (8, 14, 40, 44)),
    'poiseon':      ({(255, 42, 48), (140, 12, 69)}, (6, 30, 30, 56)),
    'obsideon':     ({(97, 88, 225), (58, 43, 206), (248, 248, 254)}, (16, 21, 23, 29)),
    'automateon':   ({(48, 216, 240), (236, 244, 255), (253, 255, 229), (255, 255, 230)}, (22, 22, 44, 44)),
}
# colours that are face-only within the head region (a mask or muzzle) and turn to fur there
# heads whose eyes and mouth are drawn in outline colour: every inner line in the head region goes
CLEAR_LINES = {'subterraneon': 260, 'odeon': 200, 'poiseon': 200, 'hypereon': 200, 'lenneon': 200, 'appariteon': 200,
               'obsideon': 200}   # (how dark a line is: the sum of its RGB)
FACE_FUR = {
    'assasseon': {(187, 157, 123), (126, 102, 78), (254, 234, 173), (255, 255, 255)},
    'hypereon': set(),
}

def turned(fg, pal, key, scale=4 / 3, crop=0.12):
    feats, (bx0, by0, bx1, by1) = FACE[key]
    furish = FACE_FUR.get(key, set())
    g = [row[:] for row in fg]
    outline = min(range(1, 16), key=lambda i: sum(pal[i]))
    def inside(x, y): return 0 <= x < 64 and 0 <= y < 64 and g[y][x]
    def border(x, y): return any(not inside(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    mark = set()
    for y in range(by0, by1):
        for x in range(bx0, bx1):
            if inside(x, y) and (pal[g[y][x]] in feats or pal[g[y][x]] in furish):
                mark.add((x, y))
    if key in CLEAR_LINES:
        for y in range(by0, by1):
            for x in range(bx0, bx1):
                if inside(x, y) and sum(pal[g[y][x]]) < CLEAR_LINES[key] and not border(x, y):
                    mark.add((x, y))
    # the dark lines drawn round the eyes and mouth (not the silhouette's own outline)
    for (x, y) in list(mark):
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                q = (x + dx, y + dy)
                if inside(*q) and g[q[1]][q[0]] == outline and not border(*q) and bx0 <= q[0] < bx1 and by0 <= q[1] < by1:
                    mark.add(q)
    # paint them over with the fur around them, from the outside in
    todo = set(mark)
    for _ in range(12):
        if not todo: break
        done = set()
        for (x, y) in todo:
            near = Counter(g[y + dy][x + dx] for dx in range(-2, 3) for dy in range(-2, 3)
                           if inside(x + dx, y + dy) and (x + dx, y + dy) not in todo and g[y + dy][x + dx] != outline)
            if near:
                g[y][x] = near.most_common(1)[0][0]; done.add((x, y))
        todo -= done
    # mirror, enlarge, and let the bottom run off the frame
    ys = [y for y in range(64) if any(g[y])]; xs = [x for x in range(64) if any(g[y][x] for y in range(64))]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    w, h = x1 - x0 + 1, y1 - y0 + 1
    s = min(scale, 62 / w, 1.0 / (1 - crop) * 64 / h)
    W, H = round(w * s), round(h * s)
    out = [[0] * 64 for _ in range(64)]
    ox, oy = (64 - W) // 2, 64 - H + round(H * crop)
    for Y in range(H):
        for X in range(W):
            if 0 <= ox + X < 64 and 0 <= oy + Y < 64:
                out[oy + Y][ox + X] = g[y0 + min(h - 1, int(Y / s))][x1 - min(w - 1, int(X / s))]
    return out
