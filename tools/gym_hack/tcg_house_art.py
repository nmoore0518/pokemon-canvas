"""The art for Saffron City's card house (tools/gym_hack/build_tcg_house.py): drawn over FireRed's
building tiles. table(): a long card table (4x2 metatiles) with a playmat and both duelists' cards;
poster(): the card back (from Nick's picture of the TCG's), 30x30, hung on the wall;
shelf(): a bookcase of card binders (1x3 metatiles). Each is the top layer, RGBA (alpha 0: clear).
"""
import collections
import struct
from PIL import Image
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
R=str(ROOT / 'data/tilesets') + '/'
P=R+'primary/building_frlg/'
def pal(path):
    l=open(path).read().split('\n')[3:19]
    return [tuple(int(x) for x in s.split()) for s in l if s.strip()]
PRI_PALS=[pal(P+'palettes/%02d.pal'%i) for i in range(16)]
PT=Image.open(P+'tiles.png'); PTW=PT.size[0]//8
PTD=PT.load()
PMT=open(P+'metatiles.bin','rb').read()
def tile_rgba(t, p, hf=False, vf=False, pals=PRI_PALS, img=None):
    img=img or PT; d=img.load(); w=img.size[0]//8
    out=Image.new('RGBA',(8,8),(0,0,0,0)); o=out.load()
    tx,ty=(t%w)*8,(t//w)*8
    for y in range(8):
        for x in range(8):
            i=d[tx+x,ty+y]
            if i==0: continue
            sx=7-x if hf else x; sy=7-y if vf else y
            o[sx,sy]=pals[p][i]+(255,)
    return out
def entry(v): return v&0x3ff,(v>>12),bool(v&0x400),bool(v&0x800)
def metatile_layers(m):
    e=struct.unpack('<8H',PMT[m*16:m*16+16])
    layers=[]
    for L in range(2):
        im=Image.new('RGBA',(16,16),(0,0,0,0))
        for k in range(4):
            t,p,hf,vf=entry(e[L*4+k])
            if t>=640: continue   # (secondary)
            im.alpha_composite(tile_rgba(t,p,hf,vf),((k%2)*8,(k//2)*8))
        layers.append(im)
    return layers
def metatile(m):
    a,b=metatile_layers(m); a=a.copy(); a.alpha_composite(b); return a

# Art for the TCG house's new metatiles (top layers): card table, Dragonite poster, binder shelf

def rect(im, x0, y0, w, h, c):
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            im.putpixel((x, y), c + (255,))

OUTLINE = (82, 82, 106)
FELT_D, FELT, FELT_L = (41, 106, 74), (65, 148, 98), (106, 189, 123)
BACK_D, BACK = (49, 74, 164), (98, 139, 222)
YEL, WHITE = (255, 214, 66), (248, 248, 248)
RED, PURP, GRN = (230, 90, 49), (156, 98, 205), (90, 180, 74)

TAN_D, TAN, GREY = (172, 148, 65), (213, 197, 106), (123, 123, 123)
COIN = (240, 150, 60)

def card_down(im, x, y):
    rect(im, x, y, 8, 11, OUTLINE)
    rect(im, x + 1, y + 1, 6, 9, BACK)
    rect(im, x + 2, y + 2, 4, 7, BACK_D)
    rect(im, x + 3, y + 4, 2, 3, BACK)

def stack(im, x, y):
    rect(im, x + 1, y + 1, 8, 11, OUTLINE)     # (the stack's depth)
    rect(im, x + 2, y + 2, 8, 11, OUTLINE)
    card_down(im, x, y)

def card_up(im, x, y, art, art2):
    rect(im, x, y, 8, 11, OUTLINE)
    rect(im, x + 1, y + 1, 6, 9, YEL)
    rect(im, x + 2, y + 2, 4, 4, art)
    im.putpixel((x + 3, y + 3), art2 + (255,)); im.putpixel((x + 4, y + 4), art2 + (255,))
    rect(im, x + 2, y + 7, 4, 1, WHITE)
    rect(im, x + 2, y + 8, 3, 1, WHITE)

def table():
    # a long card table (4x2 metatiles), its playmat and both sides' cards
    im = Image.new('RGBA', (64, 32), (0, 0, 0, 0))
    rect(im, 3, 25, 58, 4, GREY)                # (its shadow)
    for lx in (3, 56):                          # (legs)
        rect(im, lx, 23, 5, 7, OUTLINE)
        rect(im, lx + 1, 23, 3, 6, TAN_D)
    rect(im, 0, 0, 64, 25, OUTLINE)
    rect(im, 1, 1, 62, 23, TAN)
    rect(im, 1, 22, 62, 2, TAN_D)
    rect(im, 3, 3, 58, 18, FELT_D)
    rect(im, 4, 4, 56, 16, FELT)
    rect(im, 31, 4, 2, 16, FELT_L)              # (the middle line)
    for x in range(8, 57, 6):                   # (zone marks)
        if x not in (32,): im.putpixel((x, 4), FELT_L + (255,)); im.putpixel((x, 19), FELT_L + (255,))
    stack(im, 6, 6)                             # the player's deck,
    card_up(im, 19, 7, RED, WHITE)    # Active (a Fire card),
    card_up(im, 37, 7, (240, 150, 60), GRN)     # ROD's Active (Dragonite),
    stack(im, 49, 6)                            # ROD's deck
    rect(im, 31, 10, 2, 3, COIN); im.putpixel((30, 11), COIN + (255,)); im.putpixel((33, 11), COIN + (255,))
    return im

# the card back (graphics/tcg_house/card_back_source.png: Nick's picture of the TCG's), boxed down
# to 30x30 and mapped onto these colours (its border and swirl's blues, the logos' yellows, the
# Poke Ball's red, white and grey, its button's light blue)
CARD_BACK_PALETTE = [(16, 40, 160), (32, 72, 200), (64, 120, 224), (120, 168, 240), (184, 208, 248),
                     (240, 244, 252), (248, 200, 24), (200, 150, 16), (224, 40, 32), (150, 24, 24),
                     (40, 40, 48), (200, 200, 208), (140, 220, 240), (90, 96, 120)]


def poster():
    """the card back, hung on the wall (30x30, with a little shadow)"""
    from PIL import ImageEnhance
    src = Image.open(ROOT / 'graphics/tcg_house/card_back_source.png').convert('RGB')
    art = ImageEnhance.Color(src.resize((30, 30), Image.BOX)).enhance(1.4)
    im = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    rect(im, 2, 2, 30, 30, (123, 123, 123))     # (the shadow)
    for y in range(30):
        for x in range(30):
            c = art.getpixel((x, y))
            near = min(CARD_BACK_PALETTE, key=lambda p: sum((a - b) ** 2 for a, b in zip(p, c)))
            im.putpixel((1 + x, 1 + y), near + (255,))
    return im

def shelf():
    WOOD_D, WOOD, WOOD_L = (98, 57, 32), (148, 90, 49), (189, 131, 74)
    im = Image.new('RGBA', (16, 48), (0, 0, 0, 0))
    top, bottom = 6, 42
    rect(im, 0, top, 16, bottom - top, OUTLINE)
    rect(im, 1, top + 1, 14, bottom - top - 2, WOOD)
    binders = [(230, 74, 57), (57, 106, 205), (74, 172, 82), (255, 205, 57), (156, 98, 205), (240, 140, 50)]
    k = 0
    for sy in (top + 2, top + 13, top + 24):       # three shelves of binders
        rect(im, 1, sy + 9, 14, 2, WOOD_D)
        rect(im, 1, sy + 9, 14, 1, WOOD_L)
        x = 2
        while x < 14:
            c = binders[k % len(binders)]; k += 1
            w = 2 if (k % 3) else 3
            if x + w > 14: break
            rect(im, x, sy, w, 9, c)
            rect(im, x, sy + 3, w, 1, WHITE)   # (the label)
            x += w
            if k % 4 == 0: x += 1               # (a gap)
    rect(im, 0, bottom - 1, 16, 1, OUTLINE)
    return im

