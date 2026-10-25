"""Bill's hybrid (Gym Trainer hack, the Gauntlet's finale): a Clefable gone wrong. Painted over
Clefable's own front picture: the body gone a ghostly mauve, Bill's hair, his purple shirt in rags,
hollow glowing eyes, a stitched grimace, a seam down the body and ectoplasm where the tail was.
    python3 tools/gym_hack/bill_art.py <out_dir>
"""
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "graphics/pokemon/clefable/anim_front_gba.png"

PAL = [(222, 222, 222), (255, 255, 255), (150, 240, 230), (110, 200, 200), (60, 130, 150),
       (214, 198, 218), (176, 154, 190), (134, 110, 156), (80, 58, 100), (16, 16, 16),
       (160, 120, 210), (150, 30, 40), (176, 124, 70), (132, 90, 48), (88, 58, 28), (104, 72, 150)]
SHINY = [(222, 222, 222), (255, 255, 255), (255, 120, 200), (230, 210, 120), (170, 140, 60),
         (200, 214, 196), (158, 178, 152), (110, 134, 108), (54, 80, 58), (16, 16, 16),
         (90, 170, 120), (150, 30, 40), (220, 220, 230), (170, 170, 186), (110, 110, 128), (50, 110, 80)]
# the base's colors, in ours
REMAP = {0: 0, 1: 1, 2: 7, 3: 8, 4: 0, 5: 5, 6: 6, 7: 7, 8: 8, 9: 9, 10: 11, 11: 11, 12: 12, 13: 13, 14: 14, 15: 0}
CH = {"K": 9, "o": 8, "h": 12, "m": 13, "d": 14, "S": 10, "s": 15, "g": 2, "c": 3, "C": 4, "w": 1,
      "r": 11, "l": 5, "n": 6, "k": 7}

HAIR = (20, 9, """
         K  K           
        KhK KmK  K      
      KKmhhKKhmKKmK     
     KmhhhhmhhhmhhmK    
    KmhhhmhhhhmhhmmmK   
   KmhhmmhhhhmhhhmmdK   
   KmhmmhhhmmhhmmhmdK   
   KmmdmhmmdmhmmdmmdK   
    KddKmdKmdKmdKddK    
     K  KdK KdK Kd K    
         K   K  K       
""")

MOUTH = (24, 29, """
  KKKKKKK 
 KwKwKwKwK
 KrrrrrrrK
  KwKwKwK 
   KKKKK  
""")


def patch(px, spec):
    x0, y0, art = spec
    for dy, row in enumerate(art.strip("\n").split("\n")):
        for dx, ch in enumerate(row):
            if ch != " ":
                px[x0 + dx, y0 + dy] = CH[ch]


def front(frame=0):
    base = Image.open(BASE)
    im = Image.new("P", (64, 64), 0)
    px, bp = im.load(), base.load()
    for y in range(64):
        for x in range(64):
            px[x, y] = REMAP[bp[x, y + 64 * frame]]
    body = (5, 6, 7)
    # the eyes: hollow sockets with a glow in them, bags under
    for y in range(24, 29):
        for x in range(20, 36):
            if bp[x, y] == 9 and 25 <= y <= 27:
                px[x, y] = 2
                for nx in (x - 1, x + 1):
                    if bp[nx, y] in (5, 6, 1):
                        px[nx, y] = 9
            elif bp[x, y] == 1:
                px[x, y] = 9
    for x in range(20, 36):
        if px[x, 28] == 5 and (px[x, 27] in (2, 9)):
            px[x, 28] = 7
    # his shirt, in rags over the chest and the left arm
    for y in range(22, 48):
        for x in range(8, 31):
            ragged = 42 + (x * 7) % 5 if x >= 18 else 34 + (x * 3) % 3
            if y <= ragged and (x < 20 or y >= 33) and px[x, y] in body:
                px[x, y] = 10 if px[x, y] == 5 else 15
    # ectoplasm for a tail, and wisps for feet
    for y in range(33, 64):
        for x in range(43, 60):
            if px[x, y] in body + (8,):
                px[x, y] = 3 if px[x, y] in (5, 6) else 4
    for y in range(49, 64):
        for x in range(18, 48):
            if px[x, y] in body + (1,):
                px[x, y] = 3 if px[x, y] in (5, 1) else 4
            elif px[x, y] == 8:
                px[x, y] = 4
    # the seam down the body
    for y in range(33, 48):
        if px[37, y] in body:
            px[37, y] = 8
            if y % 3 == 0:
                px[36, y] = px[38, y] = 9
    # agonized brows (the inner ends up), and glowing tears
    for x, y in ((23, 24), (24, 24), (25, 23), (26, 23), (29, 23), (30, 23), (31, 24), (32, 24)):
        px[x, y] = 9
    for x, y in ((23, 28), (23, 29), (33, 28), (33, 29), (33, 30)):
        px[x, y] = 2
    patch(px, HAIR)
    patch(px, MOUTH)
    im.putpalette([c for rgb in PAL for c in rgb])
    return im


if __name__ == "__main__":
    out = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    front().save(out / "bill_front.png")
