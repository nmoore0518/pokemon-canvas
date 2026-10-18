"""Pixel art for Pallet Town's decorations, drawn with palette indices.

All sprites use the decoration palette (secondary palette 7 of the Pallet Town tileset):
  0 transparent, 1 white, 2 light pink, 3 pink, 8 dark pink, 9 wood, 15 warm light,
  4-7 greys (light to dark; shared with the purple roof that lives in 10-14 of the same palette).
Run this file to write a zoomed preview: python3 tools/gym_hack/pallet_art.py out.png
"""
from PIL import Image, ImageDraw

T, WHITE, LPINK, PINK, DPINK, WOOD, LIGHT = 0, 1, 2, 3, 8, 9, 15
G1, G2, G3, G4 = 4, 5, 6, 7  # greys, light -> dark

DECOR_PALETTE = {
    0: (0, 0, 255), 1: (248, 248, 248), 2: (255, 205, 222), 3: (246, 156, 189),
    4: (148, 164, 180), 5: (123, 123, 131), 6: (90, 90, 115), 7: (65, 74, 106),
    8: (205, 98, 139), 9: (172, 115, 65), 15: (238, 205, 123),
    # purple roof, same roles as the red roof's 10-14
    10: (238, 213, 255), 11: (205, 164, 238), 12: (180, 123, 222), 13: (156, 98, 205), 14: (106, 57, 156),
}


def new(w, h):
    return Image.new("P", (w, h), T)


def outline(img, color=G4):
    """Dark 1px outline around everything that isn't transparent."""
    px = img.load()
    w, h = img.size
    edge = []
    for y in range(h):
        for x in range(w):
            if px[x, y] != T:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and px[nx, ny] not in (T, color):
                    edge.append((x, y))
                    break
    for p in edge:
        px[p] = color
    return img


def chansey_statue():
    """A pink Chansey on a stone pedestal: 32x48 (2x3 blocks)."""
    img = new(32, 48)
    d = ImageDraw.Draw(img)
    px = img.load()
    # pedestal
    d.rectangle([3, 34, 28, 46], fill=G2)
    d.rectangle([3, 34, 28, 37], fill=G1)          # top face
    d.line([(3, 34), (28, 34)], fill=WHITE)
    d.rectangle([3, 44, 28, 46], fill=G3)          # shadowed base
    d.line([(5, 39), (5, 42)], fill=G3)            # carved panel edges
    d.line([(26, 39), (26, 42)], fill=G3)
    heart = [".XX.XX.", "XXXXXXX", ".XXXXX.", "..XXX..", "...X..."]
    for yy, row in enumerate(heart):
        for xx, ch in enumerate(row):
            if ch == "X":
                px[13 + xx, 39 + yy] = PINK
    px[14, 39] = LPINK

    def ell(cx, cy, rx, ry, x, y):
        return ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2

    # feet
    for y in range(29, 34):
        for x in range(32):
            if ell(10.5, 31.5, 3.2, 1.8, x, y) <= 1 or ell(20.5, 31.5, 3.2, 1.8, x, y) <= 1:
                px[x, y] = DPINK
    # side tufts
    for pts in ([(7, 9), (1, 8), (3, 11), (2, 14), (7, 15)], [(24, 9), (30, 8), (28, 11), (29, 14), (24, 15)]):
        d.polygon(pts, fill=PINK)
    px[1, 8] = px[30, 8] = DPINK
    px[2, 14] = px[29, 14] = DPINK
    # egg-shaped body, wider at the bottom, lit from the upper left
    for y in range(1, 32):
        for x in range(32):
            rx = 9.5 + 2.5 * (y - 1) / 30
            v = ell(15.5, 16.5, rx, 15, x, y)
            if v <= 1:
                nx, ny = (x - 15.5) / rx, (y - 16.5) / 15
                c = PINK
                if nx > 0.55 or ny > 0.8 or (nx > 0.35 and ny > 0.5):
                    c = DPINK
                elif ell(11, 7, 4, 3.2, x, y) <= 1:
                    c = LPINK
                px[x, y] = c
    # eyes, mouth
    for x, y in ((11, 11), (12, 11), (11, 12), (12, 12), (11, 13), (12, 13),
                 (19, 11), (20, 11), (19, 12), (20, 12), (19, 13), (20, 13)):
        px[x, y] = G4
    px[11, 11] = px[19, 11] = WHITE
    px[15, 16] = px[16, 16] = DPINK
    px[9, 14] = px[22, 14] = LPINK
    # pouch
    for y in range(23, 31):
        for x in range(32):
            if ell(15.5, 25.5, 7, 4.8, x, y) <= 1 and px[x, y] != T:
                px[x, y] = LPINK
    # egg
    for y in range(16, 29):
        for x in range(32):
            v = ell(15.5, 22, 3.6, 4.8, x, y)
            if v <= 1:
                px[x, y] = G1 if (x - 15.5) / 3.6 + (y - 22) / 4.8 > 0.9 else WHITE
            elif v <= 1.45 and px[x, y] in (PINK, LPINK, DPINK):
                px[x, y] = G2
    # little arms holding the egg
    for cx in (10.2, 20.8):
        for y in range(17, 25):
            for x in range(32):
                v = ell(cx, 20.5, 2.2, 2.6, x, y)
                if v <= 1:
                    px[x, y] = PINK if cx < 15 else DPINK
                elif v <= 1.6 and px[x, y] in (PINK, LPINK):
                    px[x, y] = DPINK
    return outline(img)


def lamp():
    """Street lamp: 16x32 (1x2 blocks)."""
    img = new(16, 32)
    d = ImageDraw.Draw(img)
    d.polygon([(4, 4), (11, 4), (13, 7), (2, 7)], fill=G3)       # cap
    d.rectangle([7, 1, 8, 3], fill=G3)                           # finial
    d.rectangle([4, 8, 11, 14], fill=LIGHT)                      # glass
    d.rectangle([5, 9, 6, 12], fill=WHITE)
    d.line([(4, 8), (4, 14)], fill=G3)
    d.line([(11, 8), (11, 14)], fill=G3)
    d.rectangle([3, 15, 12, 16], fill=G3)                        # bottom frame
    d.rectangle([7, 17, 8, 27], fill=G3)                         # post
    d.line([(7, 17), (7, 27)], fill=G2)
    d.rectangle([5, 27, 10, 29], fill=G2)                        # base
    d.line([(5, 27), (10, 27)], fill=G1)
    return outline(img)


def bench():
    """Wooden park bench seen from the front: 32x16 (2x1 blocks)."""
    img = new(32, 16)
    d = ImageDraw.Draw(img)
    d.rectangle([2, 1, 29, 3], fill=WOOD)                        # backrest planks
    d.rectangle([2, 5, 29, 7], fill=WOOD)
    d.line([(2, 1), (29, 1)], fill=LIGHT)
    d.line([(2, 5), (29, 5)], fill=LIGHT)
    d.rectangle([4, 4, 5, 8], fill=G3)                           # back supports
    d.rectangle([26, 4, 27, 8], fill=G3)
    d.rectangle([1, 9, 30, 11], fill=WOOD)                       # seat
    d.line([(1, 9), (30, 9)], fill=LIGHT)
    d.rectangle([3, 12, 5, 14], fill=G3)                         # legs
    d.rectangle([26, 12, 28, 14], fill=G3)
    return outline(img)


SPRITES = {"chansey": chansey_statue, "lamp": lamp, "bench": bench}


if __name__ == "__main__":
    import sys
    pal = [DECOR_PALETTE.get(i, (0, 0, 0)) for i in range(16)]
    flat = [c for rgb in pal for c in rgb]
    sheet = Image.new("P", (96, 48), T)
    x = 0
    for name in ("chansey", "lamp", "bench"):
        im = SPRITES[name]()
        sheet.paste(im, (x, 0))
        x += im.width + 8
    sheet.putpalette(flat + [0] * (768 - len(flat)))
    bg = Image.new("RGB", sheet.size, (200, 240, 220))
    rgb = sheet.convert("RGBA")
    px = sheet.load()
    mask = Image.new("L", sheet.size, 0)
    for yy in range(sheet.height):
        for xx in range(sheet.width):
            if px[xx, yy] != T:
                mask.putpixel((xx, yy), 255)
    bg.paste(rgb, (0, 0), mask)
    bg.resize((sheet.width * 8, sheet.height * 8), Image.NEAREST).save(sys.argv[1])
