#!/usr/bin/env python3
"""The ten new eeveelutions' graphics (Pokémon Canvas).

Front sprites: the community "all-type eeveelutions" chart (source/, cut out of the chart by type;
Kirineon by Veterans, Nimbeon by PansygumPKMN - see CREDITS.md), with edits in edits.py: the Light
one recoloured as the Normal one (Eevee's colours reversed), round glasses on the Bug one, and the
Steel one made mechanical (gunmetal plating, a brass gear collar, seams and rivets). Back sprites
(backs.py make): Obsideon's and Subterraneon's are the design turned away (turned.py); Valyreon's
is a supplied back sprite (source/valyreon_back.png); the rest are adapted from reference pictures
(ref_backs.py). Icons (icons.py) are shrunk fronts on the shared icon palettes; shiny palettes
(shiny.py) are hue shifts.

    python3 tools/gym_hack/eeveelutions/build.py

Writes graphics/pokemon/<species>/{anim_front.png, back.png, normal.pal, shiny.pal, icon.png,
footprint.png} and prints each sprite's size and offsets for the species data.
"""
import shutil
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from PIL import Image
from backs import DESIGNS, make
from icons import icon, save as save_icon
from shiny import shiny

ROOT = HERE.parents[2]

def write_pal(pal, path):
    with open(path, 'w', newline='\r\n') as fh:
        fh.write('JASC-PAL\n0100\n16\n' + ''.join(f'{r} {g} {b}\n' for r, g, b in pal))

def write_pic(g, pal, path, frames):
    im = Image.new('P', (64, 64 * frames), 0)
    flat = [v for c in pal for v in c]
    im.putpalette(flat + [0] * (768 - len(flat)))
    for f in range(frames):
        for y in range(64):
            for x in range(64):
                im.putpixel((x, y + 64 * f), g[y][x])
    im.save(path, transparency=0)

def bbox(g):
    ys = [y for y in range(64) if any(g[y])]
    xs = [x for x in range(64) if any(g[y][x] for y in range(64))]
    return min(xs), max(xs), min(ys), max(ys)

def main():
    from edits import get
    for species, (front, base, _m, _f) in DESIGNS.items():
        d = ROOT / 'graphics/pokemon' / species
        d.mkdir(parents=True, exist_ok=True)
        fg, bg, pal = make(species)
        write_pic(fg, pal, d / 'anim_front.png', 2)
        write_pic(bg, pal, d / 'back.png', 1)
        write_pal(pal, d / 'normal.pal')
        write_pal(shiny(pal, front), d / 'shiny.pal')
        ig, pi = icon(get(front), front)
        save_icon(ig, pi, d / 'icon.png')
        shutil.copy(ROOT / 'graphics/pokemon/eevee/footprint.png', d / 'footprint.png')
        fx0, fx1, fy0, fy1 = bbox(fg); bx0, bx1, by0, by1 = bbox(bg)
        print(f"{species:13} front {fx1 - fx0 + 1}x{fy1 - fy0 + 1} yoff {63 - fy1}  back {bx1 - bx0 + 1}x{by1 - by0 + 1} yoff {63 - by1}  icon pal {pi}")

if __name__ == '__main__':
    main()
