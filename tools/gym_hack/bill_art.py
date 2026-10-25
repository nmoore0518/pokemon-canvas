"""Bill's hybrid, CLEFABILL (Gym Trainer hack, the Gauntlet's finale): half man, half Clefable.
Painted from shapes (regions), shaded light-to-dark from the top left and outlined automatically,
then the details drawn by hand: Clefable's ears out of Bill's hair, spectral wings, the Clefable arm,
tail and leg, his shirt torn over a Clefable belly, a stitched seam, one eye glowing and the other
black and hanging out of its socket, ectoplasm for feet.

    python3 tools/gym_hack/bill_art.py <dir>     (front.png, back.png, icon.png, normal.pal, shiny.pal)
Used by build_gauntlet.py (graphics/pokemon/clefabill/).
"""
import colorsys
import math
import sys
from pathlib import Path

from PIL import Image

PAL = {"x": (222, 222, 222), "K": (16, 12, 20),
       "skin": [(246, 214, 222), (226, 176, 196), (184, 130, 160)],
       "hair": [(176, 130, 84), (136, 94, 56), (92, 60, 34)],
       "shirt": [(170, 132, 206), (132, 94, 168), (92, 62, 120)],
       "pants": [(196, 172, 114), (196, 172, 114), (130, 112, 70)],
       "ecto": [(250, 176, 236), (214, 96, 196), (150, 52, 140)]}     # (magenta; the shiny's is teal)
SHINY_ECTO = [(170, 250, 240), (100, 196, 204), (56, 128, 150)]
# the 16 colors (0 transparent)
COLORS = [PAL["x"], PAL["K"]] + PAL["skin"] + PAL["hair"] + PAL["shirt"] + [PAL["pants"][0], PAL["pants"][2]] + PAL["ecto"]


def turn(c):
    h, l, s = colorsys.rgb_to_hls(*[v / 255 for v in c])
    return tuple(int(v * 255) for v in colorsys.hls_to_rgb((h + 0.33) % 1, l, s))


# the shiny: hues turned (sickly green, silver hair, teal shirt), teal ectoplasm
SHINY = COLORS[:2] + [turn(c) for c in COLORS[2:13]] + SHINY_ECTO

REG = {"wing": "ecto", "tail": "skin", "belly": "skin", "ear": "skin", "sleeve": "shirt", "hand": "skin",
       "neck": "skin", "patch": "skin", "carm": "skin", "leg": "skin"}
BIG = ("belly", "shirt", "pants", "wing", "hair", "sleeve")


class Canvas:
    """Regions painted in order (later on top), at a scale, then shaded and outlined"""

    def __init__(self, size, s=1.0, ox=0.0, oy=0.0):
        self.W, self.s, self.ox, self.oy = size, s, ox, oy
        self.Z, self.order = {}, []

    def _put(self, x, y, r):
        if 0 <= x < self.W and 0 <= y < self.W:
            self.Z[(x, y)] = r
            if r in self.order:         # (painted again: it's on top now)
                if self.order[-1] != r:
                    self.order.remove(r)
                    self.order.append(r)
            else:
                self.order.append(r)

    def ell(self, cx, cy, rx, ry, r, cond=lambda x, y: True):
        for y in range(self.W):
            for x in range(self.W):
                xx, yy = (x + .5 - self.ox) / self.s - .5, (y + .5 - self.oy) / self.s - .5
                if ((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2 <= 1 and cond(xx, yy):
                    self._put(x, y, r)

    def poly(self, pts, r):
        n = len(pts)
        for y in range(self.W):
            for x in range(self.W):
                xo, yo = (x + .5 - self.ox) / self.s, (y + .5 - self.oy) / self.s
                c = False
                for i in range(n):
                    (x1, y1), (x2, y2) = pts[i], pts[(i + 1) % n]
                    if (y1 > yo) != (y2 > yo) and xo < (x2 - x1) * (yo - y1) / (y2 - y1) + x1:
                        c = not c
                if c:
                    self._put(x, y, r)

    def render(self, outline_borders=("sleeve",)):
        Z, W = self.Z, self.W
        base = lambda r: REG.get(r, r)
        img = Image.new("RGB", (W, W), PAL["x"])
        px = img.load()
        small = self.s < 1
        for (x, y), r in Z.items():
            def dist(sx, sy):
                for k in range(1, 5):
                    if Z.get((x + sx * k, y + sy * k)) != r:
                        return k
                return 9
            big = r in BIG and not small
            shade = 1
            if dist(-1, -1) <= (2 if big else 1) or dist(0, -1) <= 1:
                shade = 0
            if dist(1, 1) <= (2 if big else 1) or dist(1, 0) <= 1:
                shade = 2
            px[x, y] = PAL[base(r)][shade]
        for (x, y), r in Z.items():     # the ears' tips: Bill's hair color
            if r == "ear" and (y - self.oy) / self.s <= 5:
                px[x, y] = PAL["hair"][2 if Z.get((x + 1, y + 1)) != "ear" else (0 if Z.get((x - 1, y - 1)) != "ear" else 1)]
        for y in range(W):
            for x in range(W):
                if (x, y) not in Z and any((x + dx, y + dy) in Z for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                    px[x, y] = PAL["K"]
        rank = {r: i for i, r in enumerate(self.order)}
        for (x, y), r in list(Z.items()):
            for dx, dy in ((1, 0), (0, 1), (-1, 0), (0, -1)):
                o = Z.get((x + dx, y + dy))
                if not o or o == r or rank[o] < rank[r]:
                    continue
                if base(o) == base(r) and o not in outline_borders:
                    px[x, y] = PAL[base(r)][2]
                elif r != "ecto":
                    px[x, y] = PAL["K"]
        return img, px

    def stamp(self, px, x0, y0, art, chars):
        for dy, row in enumerate(art.strip("\n").split("\n")):
            for dx, ch in enumerate(row):
                if ch != ".":
                    px[x0 + dx, y0 + dy] = chars[ch]


F = {"K": PAL["K"], "w": PAL["ecto"][0], "g": PAL["ecto"][0], "c": PAL["ecto"][1], "n": PAL["skin"][1],
     "k": PAL["skin"][2], "r": PAL["shirt"][2], "p": PAL["skin"][0]}


def paint_front(cv, frame=0):
    """The front, its shapes in 64x64 coordinates (frame 1: the wings up, the wisps drifting)"""
    w = -2 if frame else 0
    cv.poly([(22, 30), (9, 18 + w), (13, 26 + w), (6, 29 + w), (14, 32), (9, 36), (20, 36)], "wing")
    cv.poly([(42, 30), (55, 18 + w), (51, 26 + w), (58, 29 + w), (50, 32), (55, 36), (44, 36)], "wing")
    for t in range(0, 300, 3):
        a, r = math.radians(t), 6 - t / 100
        cv.ell(50 + r * math.cos(a), 50 - r * math.sin(a), 1.6, 1.6, "tail")
    for t in range(10):
        cv.ell(42 + t * 0.8, 54 - t * 0.4, 1.5, 1.5, "tail")
    cv.poly([(23, 46), (31, 46), (31, 58), (24, 58)], "pants")
    cv.poly([(33, 46), (41, 46), (41, 58), (34, 58)], "pants")
    cv.poly([(34, 50), (36, 52), (38, 50), (41, 51), (41, 58), (34, 58)], "leg")   # torn: the Clefable leg
    for x0, ln, dr in ((25, 4, -1), (28, 5, 0), (31, 3, 1), (35, 5, 0), (38, 4, 1), (41, 3, 1)):
        for k in range(ln):
            y, wd = 58 + k, 1.5 - k * 0.35
            cx = x0 + dr * k * 0.6 + (dr or 1) * frame * 0.8
            cv.ell(cx, y, max(wd, .5), .5, "ecto")
    cv.poly([(24, 26), (40, 26), (43, 28), (45, 32), (44, 40), (42, 47), (22, 47), (20, 40), (19, 32), (21, 28)], "shirt")
    cv.ell(32, 40, 10, 8, "belly", lambda x, y: y >= 33 + ((int(x) * 5) % 4))
    cv.poly([(47, 29), (44, 28), (44, 39), (47, 44), (51, 43), (52, 34)], "carm")       # the Clefable arm
    cv.poly([(22, 27), (17, 28), (13, 33), (12, 40), (14, 44), (18, 44), (18, 39), (20, 34)], "sleeve")
    cv.ell(17.5, 44, 2.8, 2.6, "hand")      # his hand, clutching his side
    cv.poly([(29, 23), (36, 23), (36, 28), (29, 28)], "neck")
    cv.ell(32, 17, 9, 10, "skin")
    cv.ell(32, 11, 11, 6, "hair", lambda x, y: y <= 12)
    for i, x in enumerate(range(23, 42, 3)):
        for d in range(0, 3 + (i % 2)):
            cv.poly([(x + d // 2, 13 + d), (x + d // 2 + 1, 13 + d), (x + d // 2 + 1, 14 + d), (x + d // 2, 14 + d)], "hair")
            cv.poly([(x + 1, 12 + d), (x + 2, 12 + d), (x + 2, 13 + d), (x + 1, 13 + d)], "hair")
    cv.poly([(23, 11), (29, 7), (16, 0)], "ear")
    cv.poly([(41, 11), (35, 7), (48, 0)], "ear")


FACE = """
KK.........KK...
..KK.....KK.....
.KKKK...........
KpKgK...........
.KKKK...........
......n.........
.....kn.........
...KKKKKKK......
..KwKwKwKwK.....
..KrrrrrrrK.....
...KwKwKwK......
....KKKKK.......
"""


def front(frame=0):
    cv = Canvas(64)
    paint_front(cv, frame)
    img, px = cv.render()
    cv.stamp(px, 25, 15, FACE, F)
    for y in range(28, 47):         # the seam down the body
        if cv.Z.get((33, y)) in ("skin", "belly") and px[33, y] != PAL["K"]:
            px[33, y] = PAL["skin"][2]
            if y % 3 == 0:
                px[32, y] = px[34, y] = PAL["K"]
    for y in range(15, 27):         # the other eye: black, bulging, hanging out of its socket
        for x in range(32, 43):
            if ((x - 37.2) / 3.4) ** 2 + ((y - 20.6) / 3.8) ** 2 <= 1:
                px[x, y] = PAL["K"]
    gx = 38 + (1 if frame else 0)
    px[gx, 20] = px[gx, 19] = px[gx + 1, 20] = F["g"]
    px[36, 18] = PAL["skin"][0]
    for x, y in ((35, 25), (36, 26), (38, 25)):
        px[x, y] = PAL["skin"][2]
    for x, y in ((17, 46), (19, 45), (20, 43), (46, 44), (48, 45), (48, 44), (50, 44)):
        px[x, y] = PAL["K"]
    return img


def back():
    """From behind (bigger, cut off at the bottom): the back of his head, the ears, the wings spread
    from his shoulder blades, the Clefable arm (now on the left), a hole in the shirt, the seam"""
    cv = Canvas(64)
    cv.poly([(16, 38), (48, 38), (52, 46), (52, 64), (12, 64), (12, 46)], "shirt")
    cv.ell(41, 52, 5, 4, "patch")                          # torn: Clefable skin showing
    cv.poly([(15, 39), (8, 44), (6, 54), (10, 59), (15, 55), (16, 47)], "carm")       # the Clefable arm
    cv.poly([(49, 39), (56, 44), (58, 56), (53, 59), (49, 52)], "sleeve")
    cv.ell(54.5, 59, 3, 2.6, "hand")
    cv.poly([(27, 32), (37, 32), (37, 40), (27, 40)], "neck")
    cv.ell(32, 23, 12, 12, "hair")
    cv.poly([(22, 16), (27, 12), (9, 0)], "ear")
    cv.poly([(42, 16), (37, 12), (55, 0)], "ear")
    cv.poly([(26, 44), (6, 26), (11, 36), (2, 40), (12, 45), (5, 51), (24, 50)], "wing")
    cv.poly([(38, 44), (58, 26), (53, 36), (62, 40), (52, 45), (59, 51), (40, 50)], "wing")
    for t in range(0, 300, 3):
        a, r = math.radians(t), 4.5 - t / 120
        cv.ell(32 + r * math.cos(a), 58 - r * math.sin(a), 1.7, 1.7, "tail")
    img, px = cv.render()
    for a in range(25, 160, 16):    # his hair: strands from the crown, a shine at the top left
        ex, ey = 32 + 11.5 * math.cos(math.radians(a)), 23 + 11.5 * math.sin(math.radians(a))
        for k in range(4, 24):
            t = k / 24
            x, y = round(32 + (ex - 32) * t), round(14 + (ey - 14) * t)
            if cv.Z.get((x, y)) == "hair" and px[x, y] != PAL["K"]:
                px[x, y] = PAL["hair"][2]
    for a in range(200, 260, 4):
        for rr in (8, 9):
            x, y = round(31 + rr * math.cos(math.radians(a))), round(21 + rr * math.sin(math.radians(a)))
            if cv.Z.get((x, y)) == "hair":
                px[x, y] = PAL["hair"][0]
    for y in range(33, 64):         # the seam down his back
        if px[32, y] not in (PAL["K"],) and cv.Z.get((32, y)) in ("shirt", "neck"):
            px[32, y] = PAL["shirt"][2] if cv.Z.get((32, y)) == "shirt" else PAL["skin"][2]
            if y % 3 == 0:
                px[31, y] = px[33, y] = PAL["K"]
    return img


ICON_FACE = {(14, 9): F["g"], (13, 9): PAL["K"], (17, 9): PAL["K"], (18, 9): PAL["K"], (17, 10): PAL["K"],
             (18, 10): PAL["K"], (15, 11): PAL["K"], (16, 11): PAL["K"], (17, 11): PAL["K"], (16, 12): PAL["K"]}


def icon(frame=0):
    cv = Canvas(32, 0.5, 0, 0 + frame)
    paint_front(cv, 0)
    img, px = cv.render()
    for (x, y), c in ICON_FACE.items():
        px[x, y + frame] = c
    return img


def indexed(img, colors=COLORS):
    out = Image.new("P", img.size, 0)
    out.putdata([colors.index(c) for c in img.get_flattened_data()])
    out.putpalette([v for c in colors for v in c])
    return out


def icon_sheet(icon_pal):
    """32x64 (two frames), in one of the shared icon palettes (the nearest of its colors)"""
    sheet = Image.new("P", (32, 64), 0)
    for f in range(2):
        im = icon(f)
        for y in range(32):
            for x in range(32):
                c = im.getpixel((x, y))
                if c == PAL["x"]:
                    continue
                best = min(range(1, 16), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, icon_pal[i])))
                sheet.putpixel((x, 32 * f + y), best)
    sheet.putpalette([v for c in icon_pal for v in c])
    return sheet


def write_jasc(path, colors):
    Path(path).write_text("JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{r} {g} {b}\r\n" for r, g, b in colors))


def read_jasc(path):
    v = [int(x) for x in Path(path).read_text().split()[3:]]
    return [tuple(v[i:i + 3]) for i in range(0, 48, 3)]


def write_all(out, icon_pal_path):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    sheet = Image.new("P", (64, 128), 0)
    for f in range(2):
        sheet.paste(indexed(front(f)), (0, 64 * f))
    sheet.putpalette([v for c in COLORS for v in c])
    sheet.save(out / "anim_front.png")
    indexed(back()).save(out / "back.png")
    icon_sheet(read_jasc(icon_pal_path)).save(out / "icon.png")
    write_jasc(out / "normal.pal", COLORS)
    write_jasc(out / "shiny.pal", SHINY)
    # its footprint: Clefable's (one foot of it, anyway)
    root = Path(__file__).resolve().parents[2]
    Image.open(root / "graphics/pokemon/clefable/footprint.png").save(out / "footprint.png")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    write_all(sys.argv[1] if len(sys.argv) > 1 else "clefabill", root / "graphics/pokemon/icon_palettes/pal2.pal")
