"""final/<name>.png (RGBA) -> 64x64 indexed: bottom-aligned, centred, <= 15 colours"""
import sys
from PIL import Image
BG = (152, 208, 160)
def to_gba(src, bottom_pad=0):
    im = (src if isinstance(src, Image.Image) else Image.open(src)).convert('RGBA')
    im = im.crop(im.getchannel('A').getbbox())
    w, h = im.size
    assert w <= 64 and h <= 64 - bottom_pad, (src, w, h)
    canvas = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    canvas.paste(im, ((64 - w) // 2, 64 - bottom_pad - h))
    opaque = [(x, y) for y in range(64) for x in range(64) if canvas.getpixel((x, y))[3] >= 128]
    from collections import Counter
    cnt = Counter(canvas.getpixel(p)[:3] for p in opaque)
    # merge near-identical colours (the chart's soft edges) into the commoner one
    merged = {}
    reps = []
    for c, _ in cnt.most_common():
        r = next((r for r in reps if sum((a-b)**2 for a, b in zip(c, r)) < 14**2), None)
        if r is None: reps.append(c); r = c
        merged[c] = r
    for p in opaque:
        canvas.putpixel(p, merged[canvas.getpixel(p)[:3]] + (255,))
    cols = sorted(set(merged.values()), key=lambda c: sum(c))
    if len(cols) > 15:
        strip = Image.new('RGB', (len(opaque), 1))
        for i, p in enumerate(opaque): strip.putpixel((i, 0), canvas.getpixel(p)[:3])
        q = strip.quantize(15, method=Image.Quantize.MEDIANCUT)
        qp = q.getpalette()
        cols = [tuple(qp[i*3:i*3+3]) for i in range(15)]
    def nearest(c):
        return min(range(len(cols)), key=lambda i: sum((a-b)**2 for a, b in zip(c, cols[i])))
    pal = [BG] + cols + [(0, 0, 0)] * (15 - len(cols))
    g = [[0]*64 for _ in range(64)]
    for (x, y) in opaque: g[y][x] = nearest(canvas.getpixel((x, y))[:3]) + 1
    return g, pal
def save(g, pal, png, palfile, frames=2):
    idx = Image.new('P', (64, 64 * frames), 0)
    flat = [v for c in pal for v in c]
    idx.putpalette(flat + [0] * (768 - len(flat)))
    for f in range(frames):
        for y in range(64):
            for x in range(64):
                idx.putpixel((x, y + 64*f), g[y][x])
    idx.save(png, transparency=0)
    with open(palfile, 'w', newline='\r\n') as fh:
        fh.write('JASC-PAL\n0100\n16\n' + ''.join(f'{r} {gg} {b}\n' for r, gg, b in pal))
