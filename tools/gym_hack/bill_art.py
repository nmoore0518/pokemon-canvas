"""Bill's hybrid (Gym Trainer hack, the Gauntlet's finale), draft 2: half man, half Clefable.
Painted from shapes (regions), shaded light-to-dark from the top left and outlined automatically,
then the face drawn by hand: Clefable's ears out of Bill's hair, spectral wings, the Clefable arm,
tail and leg, his shirt torn over a Clefable belly, a stitched seam, glowing eyes, ectoplasm feet.
    python3 tools/gym_hack/bill_art.py <out.png>
"""
from PIL import Image
import math
W=64
PAL = {"x":(222,222,222),"K":(16,12,20),
 "skin":[(246,214,222),(226,176,196),(184,130,160)],
 "hair":[(176,130,84),(136,94,56),(92,60,34)],
 "shirt":[(170,132,206),(132,94,168),(92,62,120)],
 "pants":[(196,172,114),(196,172,114),(130,112,70)],
 "ecto":[(170,250,240),(100,196,204),(56,128,150)],
}
Z={}   # region map: (x,y)->region name ; later layers overwrite
def put(x,y,r):
    if 0<=x<W and 0<=y<W: Z[(x,y)]=r
def ell(cx,cy,rx,ry,r,cond=lambda x,y:True):
    for y in range(W):
        for x in range(W):
            if ((x-cx)/rx)**2+((y-cy)/ry)**2<=1 and cond(x,y): put(x,y,r)
def poly(pts,r):
    for y in range(W):
        for x in range(W):
            # even-odd
            c=False; n=len(pts)
            for i in range(n):
                (x1,y1),(x2,y2)=pts[i],pts[(i+1)%n]
                if (y1>y+.5)!=(y2>y+.5) and x+.5 < (x2-x1)*(y+.5-y1)/(y2-y1)+x1: c=not c
            if c: put(x,y,r)
# --- back: wings, tail
poly([(22,30),(9,18),(13,26),(6,29),(14,32),(9,36),(20,36)],"wing")
poly([(42,30),(55,18),(51,26),(58,29),(50,32),(55,36),(44,36)],"wing")
# tail: thick arc
for t in range(0,300,3):
    a=math.radians(t); r=6-t/100
    cx,cy=50+r*math.cos(a)*1.0,50-r*math.sin(a)
    ell(cx,cy,1.6,1.6,"tail")
for t in range(0,10): ell(42+t*0.8,54-t*0.4,1.5,1.5,"tail")
# --- legs and wisps
poly([(23,46),(31,46),(31,58),(24,58)],"pants")
poly([(33,46),(41,46),(41,58),(34,58)],"pants")
poly([(34,50),(36,52),(38,50),(41,51),(41,58),(34,58)],"skin")   # torn: the Clefable leg showing
for x0,ln,dr in ((25,4,-1),(28,5,0),(31,3,1),(35,5,0),(38,4,1),(41,3,1)):
    for k in range(ln):
        y=58+k; w=1.5-k*0.35
        for x in range(W):
            if abs(x-(x0+dr*k*0.6))<=w: put(x,y,"ecto")
# --- torso
poly([(20,27),(44,27),(43,47),(21,47)],"shirt")
ell(32,40,10,8,"belly", lambda x,y: y>=33+((x*5)%4))
# --- arms
poly([(47,29),(44,28),(44,39),(47,44),(51,43),(52,34)],"skin")          # the Clefable arm
poly([(21,27),(16,26),(13,20),(17,15),(22,15),(23,19),(19,21),(22,26)],"shirt")  # his arm up to his head
ell(20,15,3,3,"skin")    # his hand, gripping his head
# --- head
put
for y in range(23,28):
    for x in range(29,36): put(x,y,"skin")
ell(32,17,9,10,"skin")
ell(32,11,11,6,"hair", lambda x,y: y<=12)
for i,x in enumerate(range(23,42,3)):
    for d in range(0,3+(i%2)): put(x+ (d//2),13+d,"hair"); put(x+1,13+d-1,"hair")
# ears out of the hair
poly([(23,11),(29,7),(16,0)],"ear")
poly([(41,11),(35,7),(48,0)],"ear")
ell(20,15,3,3,"skin")
REG={"wing":"ecto","tail":"skin","belly":"skin","ear":"skin"}
def base(r): return REG.get(r,r)
img=Image.new("RGB",(W,W),PAL["x"]); px=img.load()
for (x,y),r in Z.items():
    pal=PAL[base(r)]
    def dist(sx,sy):
        for k in range(1,5):
            if Z.get((x+sx*k,y+sy*k))!=r: return k
        return 9
    big=r in("belly","shirt","pants","wing","hair")
    shade=1
    if dist(-1,-1)<= (2 if big else 1) or dist(0,-1)<=1: shade=0
    if dist(1,1)<=(2 if big else 1) or dist(1,0)<=1: shade=2
    px[x,y]=pal[shade]
# ear tips: hair colors
for (x,y),r in Z.items():
    if r=="ear" and y<=5: px[x,y]=PAL["hair"][2 if Z.get((x+1,y+1))!="ear" else (0 if Z.get((x-1,y-1))!="ear" else 1)]
# outline
for y in range(W):
    for x in range(W):
        if (x,y) in Z: continue
        if any((x+dx,y+dy) in Z for dx,dy in((1,0),(-1,0),(0,1),(0,-1))): px[x,y]=PAL["K"]
# region borders: a dark line where a front region meets one behind
ORDER=["wing","tail","pants","ecto","shirt","belly","skin","hair","ear"]
for (x,y),r in list(Z.items()):
    for dx,dy in((1,0),(0,1),(-1,0),(0,-1)):
        o=Z.get((x+dx,y+dy))
        if o and o!=r and base(o)==base(r) and ORDER.index(o)>ORDER.index(r): px[x,y]=PAL[base(r)][2]
        if o and base(o)!=base(r) and ORDER.index(o)>ORDER.index(r) and r not in("ecto",): px[x,y]=PAL["K"]

F={"K":PAL["K"],"w":PAL["ecto"][0],"g":PAL["ecto"][0],"c":PAL["ecto"][1],"n":PAL["skin"][1],"k":PAL["skin"][2],"r":PAL["shirt"][2],"p":PAL["skin"][0]}
FACE=(25,15,"""
KK.........KK...
..KK.....KK.....
.KKKK...KKKK....
KpKgK...KKgKK...
.KKKK...KKKKK...
......n...KK....
.....kn.........
....KKKKKKK.....
...KwKwKwKwK....
...KrrrrrrrK....
....KwKwKwK.....
.....KKKKK......
""")
x0,y0,art=FACE
for dy,row in enumerate(art.strip("\n").split("\n")):
    for dx,ch in enumerate(row):
        if ch!=".": px[x0+dx,y0+dy]=F[ch]
# seam down the middle of the face and body
for y in range(15,47):
    if 15<=y<=27: continue
    if Z.get((33,y)) in ("skin","belly") and px[33,y]!=PAL["K"] and px[33,y] not in (F["w"],F["r"],F["g"]):
        px[33,y]=PAL["skin"][2]
        if y%3==0: px[32,y]=px[34,y]=PAL["K"]
# clefable arm fingers
for x,y in ((46,44),(48,45),(48,44),(50,44)): px[x,y]=PAL["K"]

import sys
img.save(sys.argv[1] if len(sys.argv) > 1 else 'bill_front.png')
