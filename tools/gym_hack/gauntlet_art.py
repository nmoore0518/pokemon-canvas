"""The Gauntlet's art (Gym Trainer hack): the hooded figure's walking sprite (16x32 frames: down, up,
left; the walking frames are made by make_walk_frames.py through build_gauntlet.py).
    python3 tools/gym_hack/gauntlet_art.py <out.png>
"""
import sys
from pathlib import Path

from PIL import Image

PAL = {".": (255, 0, 255), "O": (16, 12, 20), "D": (34, 26, 44), "C": (58, 46, 74), "H": (88, 72, 110),
       "K": (8, 6, 10), "E": (230, 200, 120), "S": (120, 104, 140)}
ORDER = ".ODCHKES"

DOWN = """
................
................
................
......OOOO......
.....OHHCCO.....
....OHCCCCCO....
....OHCCCCCO....
...OHCKKKKCCO...
...OHKKKKKKCO...
...OCKEKKEKCO...
...OCKKKKKKCO...
...OCCKKKKCCO...
...OHCCKKCCDO...
..OHCCCCCCCCDO..
..OHCSCCCCSCDO..
..OHCCSCCSCCDO..
.OHCCCCSSCCCCDO.
.OHCCCCDCCCCCDO.
.OHCCCCDCCCCCDO.
.OHCCCCDCCCCDDO.
.OHCCCCDCCCCCDO.
.OHCCCCDCCCCCDO.
.OHCCCCDCCCCDDO.
.OHCCCCDCCCCCDO.
.OHCCCCDCCCCCDO.
.OHCCCCDCCCCDDO.
.OCCCCCDCCCCCDO.
.ODCCCCDCCCCDDO.
..ODDDDDDDDDDO..
...OKKO..OKKO...
...OOOO..OOOO...
................
"""

UP = """
................
................
................
......OOOO......
.....OHHCCO.....
....OHCCCCCO....
....OHCCCCCO....
...OHCCCCCCCO...
...OHCCCCCCCO...
...OHCCCCCCDO...
...OHCCCCCCDO...
...OHCCCCCCDO...
...OHCCCCCCDO...
..OHCCCCCCCCDO..
..OHCCCCCCCCDO..
..OHCCCCCCCCDO..
.OHCCCCCCCCCCDO.
.OHCCCCCCCCCCDO.
.OHCCCCCCCCCCDO.
.OHCCCCCCCCCDDO.
.OHCCCCCCCCCCDO.
.OHCCCCCCCCCCDO.
.OHCCCCCCCCCDDO.
.OHCCCCCCCCCCDO.
.OHCCCCCCCCCCDO.
.OHCCCCCCCCCDDO.
.OCCCCCCCCCCCDO.
.ODCCCCCCCCCDDO.
..ODDDDDDDDDDO..
...OKKO..OKKO...
...OOOO..OOOO...
................
"""

LEFT = """
................
................
................
.......OOOO.....
......OCCHHO....
.....OCCCCCHO...
.....OCCCCCHO...
....OKKKCCCCHO..
....OKKKKCCCHO..
....OKEKKCCCHO..
....OKKKKCCCDO..
....OKKKCCCCDO..
.....OCCCCCCDO..
....OHCCCCCCCDO.
....OHCCSCCCCDO.
...OHCCCCSCCCDO.
...OHCCCCCCCCCDO
...OHCCCCCCCCCDO
...OHCCCCCCCCCDO
...OHCCCCCCCCDDO
...OHCCCCCCCCCDO
...OHCCCCCCCCCDO
...OHCCCCCCCCDDO
...OHCCCCCCCCCDO
...OHCCCCCCCCCDO
...OHCCCCCCCCDDO
...OCCCCCCCCCCDO
...ODCCCCCCCCDDO
....ODDDDDDDDDO.
.....OKKO.OKKO..
.....OOOO.OOOO..
................
"""


def frame(text):
    rows = [r for r in text.strip("\n").split("\n")]
    assert len(rows) == 32 and all(len(r) == 16 for r in rows), [len(r) for r in rows]
    im = Image.new("P", (16, 32), 0)
    pal = []
    for ch in ORDER:
        pal += list(PAL[ch])
    im.putpalette(pal + [0] * (768 - len(pal)))
    im.putdata([ORDER.index(c) for r in rows for c in r])
    return im


def sheet():
    out = Image.new("P", (48, 32), 0)
    f = [frame(DOWN), frame(UP), frame(LEFT)]
    out.putpalette(f[0].getpalette())
    for i, im in enumerate(f):
        out.paste(im, (i * 16, 0))
    return out


if __name__ == "__main__":
    sheet().save(sys.argv[1] if len(sys.argv) > 1 else "hooded_figure.png")
