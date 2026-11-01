from pathlib import Path
from PIL import Image
HERE = Path(__file__).resolve().parent
import math
NEW = ['hiveon', 'kirineon', 'vitaleon', 'nimbeon', 'obliveon', 'terreon', 'lumeon', 'toxeon', 'zirconeon', 'metaleon']

def remap(im, m):
    px = im.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            p = px[x, y]
            if p[3] and p[:3] in m: px[x, y] = m[p[:3]] + (255,)

def lumeon():
    im = Image.open(HERE / 'source' / 'lumeon.png').convert('RGBA')
    remap(im, {(255, 213, 0): (204, 144, 76), (213, 164, 49): (152, 98, 56), (106, 90, 49): (92, 56, 36),
               (205, 197, 172): (214, 204, 188), (239, 228, 176): (244, 240, 230), (255, 255, 180): (255, 252, 240)})
    return im

def hiveon():
    im = Image.open(HERE / 'source' / 'hiveon.png').convert('RGBA')
    gold = (224, 184, 64, 255)
    def ring(cx, cy):
        return [(cx + dx, cy + dy) for dx in range(-2, 3) for dy in range(-2, 3) if 3 <= dx*dx + dy*dy <= 5]
    for (x, y) in ring(31, 21) + ring(39, 24) + [(34, 22), (35, 22), (36, 23)]:
        if 0 <= x < im.size[0] and 0 <= y < im.size[1]: im.putpixel((x, y), gold)
    return im

def metaleon():
    im = Image.open(HERE / 'source' / 'metaleon.png').convert('RGBA')
    px = im.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            if px[x, y][3] and px[x, y][:3] in ((76, 72, 71), (173, 173, 198)):
                px[x, y] = ((48, 216, 240) if y < 30 else (68, 72, 99)) + (255,)
            elif px[x, y][3] and px[x, y][:3] in ((132, 154, 177), (133, 153, 177), (171, 204, 219)) and not 17 <= y <= 36:
                px[x, y] = (134, 133, 167, 255)
    remap(im, {(38, 51, 67): (36, 44, 60), (68, 72, 99): (70, 82, 104), (103, 102, 134): (112, 126, 150),
               (134, 133, 167): (168, 182, 204),
               (132, 154, 177): (150, 104, 40), (133, 153, 177): (150, 104, 40), (171, 204, 219): (214, 164, 64),
               (221, 244, 252): (252, 220, 120)})
    body = {(70, 82, 104), (112, 126, 150), (168, 182, 204)}
    # panel seams across the ears
    for y in (8, 13):
        for x in range(im.size[0]):
            if px[x, y][3] and px[x, y][:3] in body: px[x, y] = (36, 44, 60, 255)
    # rivets: a bright head with a shadow under it
    for (x, y) in ((17, 7), (19, 12), (29, 7), (30, 12), (13, 44), (20, 41), (25, 47), (31, 40)):
        if px[x, y][3] and px[x, y][:3] in body and px[x, y+1][3]:
            px[x, y] = (236, 244, 255, 255); px[x, y+1] = (36, 44, 60, 255)
    return im

SRC = {'lumeon': lumeon, 'hiveon': hiveon, 'metaleon': metaleon}

def get(n):
    return SRC[n]() if n in SRC else Image.open(HERE / 'source' / (n + '.png')).convert('RGBA')

if __name__ == '__main__':
    for n in NEW:
        get(n).save(f'{n}_edited.png')
