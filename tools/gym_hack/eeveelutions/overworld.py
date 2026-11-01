"""Overworld sprites for the new eeveelutions (the Pokémon's own 32x32 object: Penny's plush of it).

Like the official ones' graphics/pokemon/<species>/overworld.png: six 32x32 frames, facing down (2),
up (2) and left (2; the game flips them for right). Down and left are the front sprite shrunk, up
is the back sprite shrunk; the second frame of each is a pixel lower (a step). In the species'
own palette (overworld_normal.pal / overworld_shiny.pal are its normal.pal / shiny.pal).

OVERWORLD_SPECIES lists which have one (build.py writes them; gen_species.py hooks them up).
"""
from PIL import Image
from icons import small

OVERWORLD_SPECIES = ['valyreon']


def shrink(grid, pal, size=30):
    """a 64x64 palette-index grid -> a 32x32 one (pixel art shrunk by picking the commonest colour)"""
    im = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    for y in range(64):
        for x in range(64):
            if grid[y][x]:
                im.putpixel((x, y), pal[grid[y][x]] + (255,))
    im = im.crop(im.getchannel('A').getbbox())
    sm = small(im, size)
    outline = min(range(1, 16), key=lambda i: sum(pal[i]))
    def near(c):
        return min(range(1, 16), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, pal[i])))
    W, H = sm.size
    g = [[0] * 32 for _ in range(32)]
    ox, oy = (32 - W) // 2, 31 - H
    for y in range(H):
        for x in range(W):
            p = sm.getpixel((x, y))
            if p[3]:
                g[oy + y][ox + x] = near(p[:3])
    out = [row[:] for row in g]
    for y in range(32):
        for x in range(32):
            if not g[y][x] and any(0 <= x + dx < 32 and 0 <= y + dy < 32 and g[y + dy][x + dx] and g[y + dy][x + dx] != outline
                                   for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = outline
    return out


def overworld(front, back, pal):
    """the 192x32 frame strip, as a 32-row grid of palette indices"""
    down, up = shrink(front, pal), shrink(back, pal)
    strip = [[0] * 192 for _ in range(32)]
    for f, g in enumerate((down, down, up, up, down, down)):
        lower = f % 2
        for y in range(32 - lower):
            for x in range(32):
                strip[y + lower][f * 32 + x] = g[y][x]
    return strip


def save(strip, pal, path):
    im = Image.new('P', (192, 32), 0)
    flat = [v for c in pal for v in c]
    im.putpalette(flat + [0] * (768 - len(flat)))
    for y in range(32):
        for x in range(192):
            im.putpixel((x, y), strip[y][x])
    im.save(path, transparency=0)
