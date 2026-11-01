"""Hand fixes to the repainted official backs (backs.py official_back), per Nick's notes:

  odeon       no smile (Jolteon's cheek line)
  assasseon   triangular ears like its front, no tan bands
  hypereon    no back wings (just its wing ears), its tail a blue wing
  poiseon     no ribbons on the head (the body's stay)
  lenneon     both wings behind it, and the Lennon glasses (their arm and rim)
  appariteon  no ears or cheek tufts (under its wraps), a thick bandaged tail
  automateon  a gear for a ruff, an angular one-colour tail

Each takes the back grid g (palette indices) and P (a colour -> the nearest palette index) and
changes g in place. Coordinates are the back sprite's (64x64).
"""
import math
from collections import deque
from PIL import Image, ImageDraw


def mask(fn):
    im = Image.new('L', (64, 64), 0)
    fn(ImageDraw.Draw(im))
    return {(x, y) for y in range(64) for x in range(64) if im.getpixel((x, y))}


def inside(g, x, y):
    return 0 <= x < 64 and 0 <= y < 64 and g[y][x] != 0


def fill(g, cells, idx, outline=None, behind=False):
    for (x, y) in cells:
        if not (0 <= x < 64 and 0 <= y < 64) or (behind and g[y][x]):
            continue
        edge = any((x + dx, y + dy) not in cells for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        g[y][x] = outline if (outline is not None and edge) else idx


def reoutline(g, outline, keep=()):
    """every pixel next to the outside becomes outline (after something was cut away)"""
    edge = [(x, y) for y in range(64) for x in range(64)
            if g[y][x] and g[y][x] not in keep
            and any(not inside(g, x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))]
    for (x, y) in edge:
        g[y][x] = outline


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
                        if inside(g, *c) and c not in seen:
                            seen.add(c); q.append(c)
                if len(part) < n:
                    for (a, b) in part: g[b][a] = 0


def odeon(g, P):
    fur, outline = P((214, 204, 188)), P((0, 0, 0))
    # round off the cheek (its notch read as a smile)
    cheek = mask(lambda d: d.ellipse((47, 20, 58, 33), fill=255))
    for (x, y) in cheek:
        if not g[y][x] or g[y][x] != fur: g[y][x] = fur
    for (x, y) in cheek:
        if any(not inside(g, x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
            g[y][x] = outline
    dark = {P((0, 0, 0)), P((92, 56, 36)), P((204, 144, 76)), P((152, 98, 56)), P((244, 240, 230))}
    for y in range(23, 31):
        for x in range(50, 60):
            if g[y][x] in dark and all(inside(g, x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                g[y][x] = fur


def assasseon(g, P):
    body, light, dark, outline = P((45, 72, 91)), P((64, 100, 116)), P((31, 47, 72)), P((0, 0, 0))
    tan = {P((126, 102, 78)): body, P((187, 157, 123)): light, P((72, 42, 6)): dark}
    for y in range(64):
        for x in range(64):
            if g[y][x] in tan and not (30 <= x <= 36 and 30 <= y <= 38):   # (not its eye)
                g[y][x] = tan[g[y][x]]
    # the ears: wide triangles, as on its front
    for y in range(0, 26):
        for x in range(0, 40):
            g[y][x] = 0
    for tip, b0, b1 in (((7, 13), (10, 28), (23, 28)), ((29, 10), (21, 28), (36, 28))):
        m = mask(lambda d: d.polygon([tip, b0, b1], fill=255))
        fill(g, m, body, outline=outline)
        mid = ((b0[0] + b1[0]) // 2, b0[1] - 2)
        for (x, y) in mask(lambda d: d.line([tip, mid], fill=255)):
            if g[y][x] == body and y < 25: g[y][x] = dark
        for (x, y) in mask(lambda d: d.line([(tip[0] + 1, tip[1] + 3), (b0[0] + 2, b0[1] - 3)], fill=255)):
            if g[y][x] == body and y < 25: g[y][x] = light
    # where the ears meet the head, no outline across the join
    for x in range(10, 38):
        for y in (26, 27):
            if g[y][x] == outline and inside(g, x, y - 2) and inside(g, x, y + 2): g[y][x] = body


def hypereon(g, P):
    cy, light, vein, edge = P((164, 230, 230)), P((152, 238, 213)), P((98, 180, 172)), P((24, 98, 90))
    grey, white, line, black = P((205, 205, 213)), P((255, 255, 255)), P((82, 82, 90)), P((16, 16, 16))
    def tail(x, y): return (x <= 18 and y < 45) or (x <= 15 and y < 56)
    for y in range(64):
        for x in range(64):
            if g[y][x] and tail(x, y):
                g[y][x] = {grey: cy, white: light, line: edge, black: edge, P((148, 148, 172)): vein}.get(g[y][x], g[y][x])
    # a wing at the end of the tail
    import sys
    sys.path.insert(0, str(__import__('pathlib').Path(__file__).parent))
    from backs import leaf
    for r, t, w in (((14, 30), (3, 2), 7), ((16, 28), (20, 3), 5)):
        poly = leaf(r, t, w)
        m = mask(lambda d: d.polygon(poly, fill=255))
        fill(g, m, cy, outline=edge)
        for k in (0.4, 0.7):
            q = (r[0] + (t[0] - r[0]) * k + 2, r[1] + (t[1] - r[1]) * k)
            for (x, y) in mask(lambda d: d.line([r, q], fill=255)):
                if g[y][x] == cy: g[y][x] = vein


def poiseon(g, P):
    ribbon = {P((255, 255, 255)), P((130, 93, 173)), P((55, 28, 83)), P((0, 0, 0)), P((77, 54, 110))}
    head = {P((245, 231, 231)), P((198, 162, 174)), P((77, 15, 56)), P((93, 55, 66))}
    skin, outline = P((198, 162, 174)), P((77, 15, 56))
    cut = [(x, y) for y in range(0, 46) for x in range(64) if g[y][x] in ribbon]
    for (x, y) in cut:
        g[y][x] = 0
    # a cut pixel with the head on both sides (across and up/down) was ribbon over the head: skin
    def has(x, y, dx, dy):
        return any(0 <= x + dx * k < 64 and 0 <= y + dy * k < 64 and g[y + dy * k][x + dx * k] in head for k in range(1, 8))
    for (x, y) in cut:
        if has(x, y, 1, 0) and has(x, y, -1, 0) and has(x, y, 0, 1) and has(x, y, 0, -1):
            g[y][x] = skin
    for (x, y) in [(x, y) for y in range(0, 46) for x in range(64) if g[y][x] in head]:
        if any(not inside(g, x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
            g[y][x] = outline
    drop_specks(g, 16)


def lenneon(g, P):
    gold = P((224, 184, 64))
    # the arm of the glasses along the side of its head, and the rim at the edge of its face
    for (x, y) in mask(lambda d: d.line([(40, 25), (49, 29)], fill=255)):
        if g[y][x]: g[y][x] = gold
    for (x, y) in ((50, 28), (51, 28), (52, 29), (52, 30), (52, 31), (51, 32), (50, 32)):
        if g[y][x]: g[y][x] = gold


def appariteon(g, P):
    body, wrap, gap, outline = P((173, 133, 156)), P((237, 201, 224)), P((125, 95, 121)), P((0, 0, 0))
    # ears off (y < 17, right of the tail), and the cheek tufts
    for y in range(0, 17):
        for x in range(22, 64):
            g[y][x] = 0
    for y in range(28, 37):
        for x in range(21, 33):
            g[y][x] = 0
        for x in range(53, 64):
            g[y][x] = 0
    # a wrapped dome where the ears were
    dome = mask(lambda d: d.ellipse((21, 5, 56, 31), fill=255))
    dome = {(x, y) for (x, y) in dome if y <= 18}
    fill(g, dome, body, outline=outline)
    for x in range(64):     # (no line where the dome meets the head)
        if g[18][x] == outline and inside(g, x, 19) and inside(g, x, 17) and inside(g, x - 1, 18) and inside(g, x + 1, 18):
            g[18][x] = body
    for (x, y) in dome:      # (wrapped, like the rest of it)
        if g[y][x] == body and y % 8 in (0, 1, 2):
            g[y][x] = P((82, 64, 90)) if y % 8 == 1 else gap
    # the tail: the old one (and its fork) goes, a thick bandaged one rises in its place
    for y in range(0, 64):
        for x in range(0, 22 if y < 45 else 17):
            g[y][x] = 0
    path = [(16, 56), (14, 46), (12, 36), (12, 26), (14, 16), (18, 9)]
    tail = mask(lambda d: (d.line(path, fill=255, width=8), d.ellipse((13, 4, 23, 14), fill=255)))
    tail = {(x, y) for (x, y) in tail if not (g[y][x] and x > 19)}
    fill(g, tail, wrap, outline=outline, behind=False)
    for (x, y) in tail:
        if g[y][x] == wrap and (x + 2 * y) % 6 in (0, 1):
            g[y][x] = gap
    reoutline(g, outline)


def automateon(g, P):
    metal, lite, dark, outline = P((112, 126, 150)), P((168, 182, 204)), P((70, 82, 104)), P((0, 0, 0))
    brass, b_lite, b_dark = P((214, 164, 64)), P((252, 220, 120)), P((150, 104, 40))
    # the ruff becomes a gear behind its head
    for y in range(64):
        for x in range(64):
            if g[y][x] in (brass, b_lite, b_dark) and x >= 22:
                g[y][x] = metal
    cx, cy, r = 37, 48, 11
    teeth = set()
    for k in range(10):
        a = 2 * math.pi * k / 10 + 0.15
        tx, ty = cx + (r + 2) * math.cos(a), cy + (r + 2) * math.sin(a)
        teeth |= mask(lambda d: d.rectangle((tx - 2, ty - 2, tx + 2, ty + 2), fill=255))
    gear = mask(lambda d: d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)) | teeth
    gear = {(x, y) for (x, y) in gear if y >= 34 or not g[y][x]}   # (behind the head)
    fill(g, gear, brass, outline=outline)
    for (x, y) in gear:
        if g[y][x] == brass and y < cy - r + 3: g[y][x] = b_lite
        elif g[y][x] == brass and x > cx + r - 4: g[y][x] = b_dark
    hub = mask(lambda d: d.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), fill=255))
    fill(g, hub, b_dark, outline=outline)
    g[cy][cx] = b_lite
    # the tail: one colour, in angular plates
    for y in range(26, 64):
        for x in range(0, 24):
            if (x, y) not in gear: g[y][x] = 0
    plates = [(23, 49), (12, 47), (4, 42), (3, 34), (8, 27), (15, 25), (12, 31), (13, 37), (19, 40), (24, 40)]
    tail = mask(lambda d: d.polygon(plates, fill=255))
    fill(g, tail, dark, outline=outline, behind=True)
    reoutline(g, outline, keep=(brass, b_lite, b_dark))


FIXES = {'odeon': odeon, 'assasseon': assasseon, 'hypereon': hypereon, 'poiseon': poiseon,
         'lenneon': lenneon, 'appariteon': appariteon, 'automateon': automateon}
