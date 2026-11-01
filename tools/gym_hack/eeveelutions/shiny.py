import colorsys
SHINY = {'hiveon': -150, 'kirineon': -120, 'vitaleon': 140, 'nimbeon': -125, 'obliveon': 180, 'terreon': 180,
         'lumeon': None, 'toxeon': 180, 'zirconeon': 120, 'metaleon': 180}
KEEP = {'hiveon': {(224, 184, 64)}}
def shiny(pal, key):
    D = SHINY[key]
    out = [pal[0]]
    for (r, g, b) in pal[1:]:
        if (r, g, b) in KEEP.get(key, ()):
            out.append((r, g, b)); continue
        h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
        if s < 0.12 or l < 0.08 or l > 0.95:
            out.append((r, g, b)); continue
        if D is None:      # Odeon: the brown mane goes silver-grey
            s *= 0.15
        else:
            h = (h + D / 360) % 1
        rr, gg, bb = colorsys.hls_to_rgb(h, l, s)
        out.append((round(rr * 255), round(gg * 255), round(bb * 255)))
    return out
