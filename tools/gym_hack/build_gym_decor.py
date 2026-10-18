#!/usr/bin/env python3
"""Gym decorations: the catalog, and the gym's own tileset that draws them.

Usage: python3 tools/gym_hack/build_gym_decor.py [catalog.png]

Writes:
  data/tilesets/secondary/pallet_gym/   the Pallet Town Gym's tileset: the Viridian Gym tileset
                                        plus one block for every decoration piece, drawn over the
                                        gym floor (furniture comes from the indoor tileset)
  include/constants/gym_decor.h         GYM_DECOR_* ids
  src/data/gym_decor.h                  the catalog (names, prices, unlocks, blocks)
and, with an argument, a picture of every decoration with its name and price.

To add a decoration, add an entry to DECORATIONS below and run build_pallet_gym.py (which runs
this script first). Keep the order of existing entries:
saves store decorations by their position in this list, so new ones go at the end.
"""
import json
import shutil
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/gym_hack"))
import render_map as rm  # noqa: E402
import decor_art  # noqa: E402

SRC = ROOT / "data/tilesets/secondary/viridian_gym_frlg"
DST = ROOT / "data/tilesets/secondary/pallet_gym"
PRIM = ROOT / "data/tilesets/primary/building_frlg"

LAYER_NORMAL, LAYER_COVERED = 0, 1
MB_NORMAL = 0

# ---------------------------------------------------------------------------
# The catalog
#
# Each decoration is a grid of cells, top row first. A cell is:
#   "f:XX"   indoor furniture block XX (BuildingFrlg tileset), set on the gym floor
#   "f:XX~"  the same, but people can walk there (the top of a plant: you walk behind it)
#   "g:XX"   a block from the gym tileset as is (Viridian Gym: walls, statues, spinners)
#   "g:XX~"  the same, walkable
#   "rug:XX" an indoor block drawn as is, walkable (rugs)
#   "x:SET:XX"        block XX from another gym's tileset (SET, e.g. CeruleanGym), copied in
#                     (tiles and colors); its floor is swapped for this gym's floor
#   "x:SET:XX:water"  the same, as water you can SURF on
#   "wall"   an invisible wall: looks like floor, can't be walked through (shown while editing)
# price:  in Poke Dollars.   unlock: a flag (usually a badge) needed before it's sold, or None.
# keep:   keep the block's behavior (bookshelf, TV, spinner...), otherwise it's plain.
# ---------------------------------------------------------------------------
BADGE = {i: f"FLAG_BADGE0{i}_GET" for i in range(1, 9)}

DECORATIONS = [
    # --- Available from the start -----------------------------------------
    dict(key="POTTED_PLANT", name="POTTED PLANT", price=1000, unlock=None,
         desc="A leafy plant in a white pot.", cells=[["f:57~"], ["f:5F"]]),
    dict(key="TABLE", name="TABLE", price=2000, unlock=None,
         desc="A table with a checkered cloth.", cells=[["f:4C", "f:4D"], ["f:54", "f:55"]]),
    dict(key="CHAIR_LEFT", name="CHAIR (FACES RIGHT)", price=600, unlock=None,
         desc="A blue chair.", cells=[["f:4B~"]]),
    dict(key="CHAIR_RIGHT", name="CHAIR (FACES LEFT)", price=600, unlock=None,
         desc="A blue chair.", cells=[["f:4E~"]]),
    dict(key="BOOKSHELF", name="BOOKSHELF", price=2500, unlock=None, keep=True,
         desc="Packed with POKéMON books.", cells=[["f:33", "f:34"], ["f:3B", "f:3C"]]),
    dict(key="CABINET", name="CABINET", price=2500, unlock=None, keep=True,
         desc="A tall cabinet.", cells=[["f:36", "f:37"], ["f:3E", "f:3F"]]),
    dict(key="TV", name="TV", price=3000, unlock=None, keep=True,
         desc="A TV on a stand.", cells=[["f:35"], ["f:3D"]]),
    dict(key="GREEN_RUG", name="GREEN RUG", price=1500, unlock=None,
         desc="A soft green rug. 3x3.",
         cells=[["rug:43", "rug:44", "rug:46"], ["rug:53", "rug:45", "rug:56"], ["rug:5B", "rug:5C", "rug:5E"]]),
    dict(key="GYM_WALL", name="GYM WALL", price=300, unlock=None,
         desc="A section of GYM wall.", cells=[["g:29B"], ["g:2A3"]]),
    dict(key="GYM_STATUE", name="GYM STATUE", price=5000, unlock=None,
         desc="Every GYM needs one.", cells=[["g:2A1~"], ["g:2A9"]]),
    # --- Unlocked by beating Gym Leaders --------------------------------------
    dict(key="POOL_WATER", name="POOL WATER", price=800, unlock=BADGE[2], keep=True,
         desc="MISTY's pool water. SURF on it!", cells=[["x:CeruleanGym:291:water"]]),
    dict(key="TRASH_CAN", name="TRASH CAN", price=1200, unlock=BADGE[3],
         desc="Just like LT. SURGE's.", cells=[["x:VermilionGym:2B8"]]),
    dict(key="FERN", name="FERN", price=1800, unlock=BADGE[4],
         desc="A lush fern from ERIKA's GYM.", cells=[["f:102"]]),
    dict(key="INVISIBLE_WALL", name="INVISIBLE WALL", price=500, unlock=BADGE[5],
         desc="KOGA's trick. Can't be seen.", cells=[["wall"]]),
    dict(key="SPINNER_UP", name="SPIN TILE (UP)", price=1500, unlock=BADGE[8], keep=True,
         desc="Spins TRAINERS upward.", cells=[["g:28C~"]]),
    dict(key="SPINNER_DOWN", name="SPIN TILE (DOWN)", price=1500, unlock=BADGE[8], keep=True,
         desc="Spins TRAINERS downward.", cells=[["g:28B~"]]),
    dict(key="SPINNER_LEFT", name="SPIN TILE (LEFT)", price=1500, unlock=BADGE[8], keep=True,
         desc="Spins TRAINERS left.", cells=[["g:289~"]]),
    dict(key="SPINNER_RIGHT", name="SPIN TILE (RIGHT)", price=1500, unlock=BADGE[8], keep=True,
         desc="Spins TRAINERS right.", cells=[["g:28A~"]]),
    dict(key="SPINNER_STOP", name="STOP TILE", price=1000, unlock=BADGE[8], keep=True,
         desc="Stops a spinning TRAINER.", cells=[["g:28D~"]]),
    # --- Free, and always in storage: where gym challenges are fought (src/gym_challenge.c) ---
    # The leader stands on the top mark (column 1, row 1), challengers on the bottom one (row 3).
    dict(key="BATTLE_STAGE", name="BATTLE STAGE", price=0, unlock=None,
         desc="Challengers battle you here.",
         cells=[[f"art:STAGE:{x}:{y}" for x in range(3)] for y in range(5)]),
    # --- Once the gym is officially the player's --------------------------------
    dict(key="BADGE_PEDESTAL", name="BADGE PEDESTAL", price=10000, unlock="FLAG_PALLET_GYM_AWARDED",
         desc="Shows off your GYM's own BADGE.",
         cells=[["badge:0:0~", "badge:1:0~"], ["badge:0:1", "badge:1:1"],
                ["art:PEDESTAL:0:0", "art:PEDESTAL:1:0"]]),
]

# --- Practice trainers: a person stands on the spot (src/gym_decor.c), the icon shows while decorating ---
for _k, _n, _p, _d in (("BEGINNER", "ROOKIE TRAINER", 5000, "A ROOKIE to battle for practice."),
                       ("INTERMEDIATE", "SKILLED TRAINER", 10000, "A SKILLED TRAINER for practice."),
                       ("ADVANCED", "EXPERT TRAINER", 20000, "An EXPERT for serious practice.")):
    DECORATIONS.append(dict(key=f"TRAINER_{_k}", name=_n, price=_p, unlock=None, desc=_d,
                            art=f"trainer_{_k.lower()}", trainer=True, category="GYM_DECOR_CAT_TRAINERS"))

# --- Decorations for each type (art: tools/gym_hack/decor_art.py). Unlocked by key battles
# (sTypeUnlocks in src/gym_decor.c), and all of them once the gym is the player's.
# (art, name, price, description, flat). Flat ones can be walked on; the top row of tall ones
# can be walked behind.
TYPE_DECORATIONS = {
    "NORMAL": [("couch", "COMFY COUCH", 4000, "Seats three. Naps welcome."),
               ("grandfather_clock", "TALL CLOCK", 2500, "Tick, tock, tick, tock."),
               ("vending_machine", "VENDING MACHINE", 6000, "FRESH WATER, SODA POP..."),
               ("picnic_blanket", "PICNIC BLANKET", 1500, "Lunch is served!", True),
               ("arcade_cabinet", "ARCADE GAME", 5000, "Beat the high score!")],
    "FIGHTING": [("punching_bag", "PUNCHING BAG", 2500, "Hit it as hard as you can!"),
                 ("barbell_rack", "BARBELL RACK", 3000, "For MACHOKE-level lifting."),
                 ("training_mat", "TRAINING MAT", 4500, "A soft mat for sparring.", True),
                 ("wooden_dummy", "WOODEN DUMMY", 2000, "It never hits back."),
                 ("gong", "DOJO GONG", 6000, "BONG! Let the battle begin!")],
    "FLYING": [("weather_vane", "WEATHER VANE", 1500, "Which way is the wind?"),
               ("birdbath", "BIRDBATH", 1000, "Bird POKéMON love a splash."),
               ("hot_air_balloon", "BALLOON MODEL", 8000, "Up, up and away!"),
               ("cloud_cushion", "CLOUD CUSHION", 2500, "Fluffy as an ALTARIA."),
               ("wind_turbine", "WIND TURBINE", 5000, "Powered by GUST.")],
    "POISON": [("toxic_barrel", "TOXIC BARREL", 1200, "Do not open. Seriously."),
               ("cauldron", "CAULDRON", 4000, "Something is brewing..."),
               ("sludge_puddle", "SLUDGE PUDDLE", 800, "Squelch, squelch.", True),
               ("chemistry_set", "CHEMISTRY SET", 5000, "Antidotes... probably."),
               ("poison_mushrooms", "TOADSTOOLS", 900, "Look, don't taste!")],
    "GROUND": [("sandbox", "SANDBOX", 3500, "Great for practicing DIG.", True),
               ("mine_cart", "MINE CART", 5000, "Fresh from the ROCK TUNNEL."),
               ("amphora", "CLAY AMPHORA", 2000, "An ancient clay jar."),
               ("sandcastle", "SANDCASTLE", 3000, "Don't let it get wet!"),
               ("shovel_rack", "SHOVEL RACK", 1500, "Every ground GYM needs one.")],
    "ROCK": [("boulder", "BOULDER", 2000, "Too big for STRENGTH."),
             ("stalagmite", "STALAGMITE", 1500, "Pointy, from MT. MOON."),
             ("fossil_display", "FOSSIL DISPLAY", 7000, "A fossil, framed and lit."),
             ("geode", "GEODE", 2500, "Plain outside, pretty inside."),
             ("standing_stones", "STANDING STONES", 9000, "Who set these up? A mystery.")],
    "BUG": [("honeycomb", "HONEYCOMB", 2000, "Sweet, sticky and buzzing."),
            ("ant_farm", "ANT FARM", 3500, "Watch them dig all day."),
            ("bug_net_rack", "BUG NET STAND", 1200, "Ready for VIRIDIAN FOREST."),
            ("spider_web", "SPIDER WEB", 2500, "Spun by a SPINARAK."),
            ("firefly_jar", "FIREFLY JAR", 1500, "A soft green glow.")],
    "GHOST": [("gravestone", "GRAVESTONE", 2000, "Straight from LAVENDER."),
              ("candelabra", "CANDELABRA", 3000, "Lit by WILL-O-WISP."),
              ("jack_o_lantern", "JACK-O'-LANTERN", 1200, "Grinning all year round."),
              ("haunted_wardrobe", "SPOOKY WARDROBE", 6000, "Something peeks out..."),
              ("spirit_lantern", "SPIRIT LANTERN", 2500, "Its flame never goes out.")],
    "STEEL": [("anvil", "ANVIL", 3000, "Clang! Clang! Clang!"),
              ("suit_of_armor", "SUIT OF ARMOR", 7000, "Is anyone inside?"),
              ("big_gears", "BIG GEARS", 5000, "Still turning, somehow."),
              ("toolbox", "TOOLBOX", 1000, "Wrenches, bolts and tape."),
              ("girders", "STEEL GIRDERS", 2500, "Strong as a STEELIX.")],
    "FIRE": [("fireplace", "FIREPLACE", 7000, "Warm and crackling."),
             ("brazier", "BRAZIER", 3000, "A fire on a stand."),
             ("lava_lamp", "LAVA LAMP", 2000, "Groovy, gooey, glowing."),
             ("campfire", "CAMPFIRE", 1500, "Roast a few marshmallows."),
             ("volcano_model", "VOLCANO MODEL", 10000, "Just like CINNABAR's!")],
    "WATER": [("aquarium", "AQUARIUM", 6000, "A GOLDEEN lives inside."),
              ("fountain", "FOUNTAIN", 12000, "Splishy splashy grandeur."),
              ("surfboard", "SURFBOARD", 2000, "Hang ten!"),
              ("life_ring", "LIFE RING", 1000, "Safety first at the pool."),
              ("ships_wheel", "SHIP'S WHEEL", 4000, "From the S.S. ANNE.")],
    "GRASS": [("bonsai", "BONSAI TREE", 3000, "Trimmed with great care."),
              ("sunflowers", "SUNFLOWERS", 1500, "Always facing the sun."),
              ("flower_bed", "FLOWER BED", 2500, "A row of bright flowers."),
              ("oak_tree", "OAK TREE", 6000, "Named after the PROF.?"),
              ("vegetable_patch", "VEGGIE PATCH", 3500, "Carrots, cabbage, tomatoes.")],
    "ELECTRIC": [("tesla_coil", "TESLA COIL", 8000, "Zzzzap! Keep your distance."),
                 ("generator", "GENERATOR", 6000, "Runs on THUNDERBOLT."),
                 ("giant_battery", "GIANT BATTERY", 3000, "Fully charged."),
                 ("neon_sign", "NEON SIGN", 4000, "Buzzing, blinking lights."),
                 ("plasma_ball", "PLASMA BALL", 2000, "Touch it. Feel the zap!")],
    "PSYCHIC": [("crystal_ball", "CRYSTAL BALL", 3000, "It shows your next battle."),
                ("zen_cushion", "ZEN CUSHION", 1000, "Sit. Breathe. Focus.", True),
                ("bent_spoon", "BENT SPOON", 2500, "Bent by the mind alone."),
                ("fortune_table", "FORTUNE TABLE", 5000, "The cards say you'll win."),
                ("hypno_spiral", "HYPNO SPIRAL", 4000, "You are getting sleepy...")],
    "ICE": [("ice_block", "ICE BLOCK", 800, "Cold to the touch."),
            ("snowman", "SNOWMAN", 2000, "He never melts in here."),
            ("ice_sculpture", "ICE SCULPTURE", 5000, "Carved with ICE BEAM."),
            ("igloo", "IGLOO", 9000, "Cozy, in a chilly way."),
            ("frozen_pond", "FROZEN POND", 4000, "Solid ice. Safe to walk on.", True)],
    "DRAGON": [("dragon_statue", "DRAGON STATUE", 9000, "A golden dragon, rearing."),
               ("egg_nest", "EGG NEST", 3000, "What will hatch from it?"),
               ("gold_hoard", "GOLD HOARD", 12000, "Every dragon needs one."),
               ("dragon_banner", "DRAGON BANNER", 2500, "Flown by dragon tamers."),
               ("stone_lantern", "STONE LANTERN", 3500, "From an old mountain shrine.")],
    "DARK": [("dark_throne", "DARK THRONE", 10000, "Fit for a villain."),
             ("thieves_safe", "SAFE", 3000, "What's the combination?"),
             ("crow_perch", "CROW PERCH", 2000, "A MURKROW's favorite spot."),
             ("moon_lamp", "MOON LAMP", 1500, "Moonlight, any time."),
             ("spiked_fence", "SPIKED FENCE", 2500, "Keep out. Or else.")],
    "FAIRY": [("fairy_ring", "FAIRY RING", 3000, "A circle of tiny flowers.", True),
              ("tiered_cake", "TIERED CAKE", 1500, "Too pretty to eat."),
              ("rose_arch", "ROSE ARCH", 7000, "A garden arch of roses."),
              ("teddy_bear", "TEDDY BEAR", 1200, "Soft and huggable."),
              ("star_lamp", "STAR LAMP", 2500, "Make a wish!")],
}
for _type, _items in TYPE_DECORATIONS.items():
    for _it in _items:
        DECORATIONS.append(dict(key=f"{_type}_{_it[0].upper()}", name=_it[1], price=_it[2], unlock=None,
                                desc=_it[3], art=_it[0], flat=len(_it) > 4 and _it[4],
                                category=f"GYM_DECOR_CAT_TYPE(TYPE_{_type})"))

# Fixed pieces of the gym itself (not for sale)
FIXED = {
    "PC_TOP": "f:63",
    "PC_BOTTOM": "f:62",
}


# ---------------------------------------------------------------------------
def read_metatiles(path):
    data = path.read_bytes()
    return [list(struct.unpack_from("<8H", data, i)) for i in range(0, len(data), 16)]


def read_attrs(path):
    data = path.read_bytes()
    return [struct.unpack_from("<I", data, i)[0] for i in range(0, len(data), 4)]


PRIM_MT = read_metatiles(PRIM / "metatiles.bin")
PRIM_ATTR = read_attrs(PRIM / "metatile_attributes.bin")
GYM_MT = read_metatiles(SRC / "metatiles.bin")
GYM_ATTR = read_attrs(SRC / "metatile_attributes.bin")
FLOOR = 0x281
FLOOR_TILES = GYM_MT[FLOOR - 0x280][:4]


def layer_of(attr):
    return (attr >> 29) & 3


def with_layer(attr, layer):
    return (attr & ~0x60000000) | (layer << 29)


class Tileset:
    def __init__(self, tiles_img):
        self.mt = [list(m) for m in GYM_MT]
        self.attr = list(GYM_ATTR)
        self.cache = {}
        self.tiles_img = tiles_img          # [PIL image, number of tiles used]
        self.pals = [rm.load_pal(p) for p in sorted((DST / "palettes").glob("*.pal"))]
        self.free_pals = []                 # 8-10: the shared decoration palettes, 11: the badge
        for slot in DYN_PALETTES:
            self.pals[slot] = [(255, 0, 255)] + [c for _, c in decor_art.PALETTES[slot]]
        self.pal_map = {}
        self.tile_map = {}
        self.sources = {}

    def source(self, name):
        if name not in self.sources:
            d = rm.tileset_dir(f"gTileset_{name}")
            self.sources[name] = (rm.Tileset(d), read_attrs(d / "metatile_attributes.bin"))
        return self.sources[name]

    def import_entry(self, name, e):
        src = self.source(name)[0]
        t, flips, p = e & 0x3FF, e & 0xC00, e >> 12
        if p >= 7:
            colors = src.pals[p]
            same = [i for i in range(7, 13) if [tuple(c[:3]) for c in self.pals[i]] == [tuple(c[:3]) for c in colors]]
            if not same:
                # Redraw the tile in the closest of the shared decoration palettes
                key = (name, t, p)
                if key not in self.tile_map:
                    px = list(src.tile(t - 0x280).getdata()) if t >= 0x280 else [0] * 64
                    rgb = [tuple(colors[i][:3]) for i in px]
                    best = None
                    for slot in DYN_PALETTES:
                        pal = [c for _, c in decor_art.PALETTES[slot]]
                        out, err = [], 0
                        for i, c in zip(px, rgb):
                            if i == 0:
                                out.append(0)
                                continue
                            k = min(range(len(pal)), key=lambda j: sum((a - b) ** 2 for a, b in zip(c, pal[j])))
                            err += sum((a - b) ** 2 for a, b in zip(c, pal[k]))
                            out.append(k + 1)
                        if best is None or err < best[0]:
                            best = (err, slot, out)
                    self.tile_map[key] = append_tile(self.tiles_img, best[2]) | (best[1] << 12)
                return self.tile_map[key] | flips
            p = same[0]
        if t >= 0x280:
            key = (name, t)
            if key not in self.tile_map:
                px = list(src.tile(t - 0x280).getdata())
                self.tile_map[key] = append_tile(self.tiles_img, px)
            t = self.tile_map[key]
        return t | flips | (p << 12)

    def write_palettes(self):
        for i, colors in enumerate(self.pals):
            text = "JASC-PAL\r\n0100\r\n16\r\n" + "".join(f"{c[0]} {c[1]} {c[2]}\r\n" for c in colors)
            (DST / f"palettes/{i:02}.pal").write_text(text, newline="")

    def add(self, entries, attr):
        key = (tuple(entries), attr)
        if key not in self.cache:
            self.mt.append(list(entries))
            self.attr.append(attr)
            self.cache[key] = 0x280 + len(self.mt) - 1
        return self.cache[key]

    def cell(self, spec, keep):
        """Returns (metatile id, collides, edit-mode metatile id or None)."""
        walkable = spec.endswith("~")
        spec = spec.rstrip("~")
        if spec == "wall":
            marker = self.add(FLOOR_TILES + MARKER_TILES, with_layer(MB_NORMAL, LAYER_NORMAL))
            return FLOOR, True, marker
        if spec.startswith("badge:"):
            _, cx, cy = spec.split(":")
            return self.add(FLOOR_TILES + badge_tiles(self.tiles_img, int(cx), int(cy)),
                            with_layer(MB_NORMAL, LAYER_NORMAL)), not walkable, None
        if spec.startswith("art:"):
            _, name, cx, cy = spec.split(":")
            return self.add(art_tiles(self.tiles_img, name, int(cx), int(cy)) + GYM_MT[FLOOR - 0x280][4:],
                            with_layer(MB_NORMAL, LAYER_NORMAL)), False, None
        if spec.startswith("x:"):
            parts = spec.split(":")
            name, mid = parts[1], int(parts[2], 16)
            water = len(parts) > 3 and parts[3] == "water"
            src, attrs = self.source(name)
            ents = src.metatiles[mid - 0x280]
            floor = src.metatiles[0x281 - 0x280][:4]
            if list(ents[:4]) == list(floor):
                imported = FLOOR_TILES + [self.import_entry(name, e) for e in ents[4:]]
            else:
                imported = [self.import_entry(name, e) for e in ents]
            attr = attrs[mid - 0x280] if keep else attrs[mid - 0x280] & ~0x1FF
            return self.add(imported, attr), not (walkable or water), "water" if water else None
        kind, num = spec.split(":")
        mid = int(num, 16)
        if kind == "g":
            attr = GYM_ATTR[mid - 0x280]
            if not keep:
                attr &= ~0x1FF
            if attr == GYM_ATTR[mid - 0x280]:
                return mid, not walkable, None
            return self.add(GYM_MT[mid - 0x280], attr), not walkable, None
        src, attr = PRIM_MT[mid], PRIM_ATTR[mid]
        if not keep:
            attr &= ~0x1FF
        if kind == "rug":
            return self.add(src, attr), False, None
        # furniture: the gym floor underneath, the furniture's own top layer above
        return self.add(FLOOR_TILES + src[4:], attr), not walkable, None


# ---------------------------------------------------------------------------
# Pixel art drawn in the gym floor's colors (palette 12)
ART_PAL = 12
WHITE, LIGHT, DARK, CLAY, CLAY_LIGHT = 5, 4, 1, 6, 7


def stage_art():
    """The battle stage: a clay court with white lines, a center circle and two start marks."""
    w, h = 48, 80
    px = [[CLAY_LIGHT] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            if x in (0, w - 1) or y in (0, h - 1):
                px[y][x] = DARK
            elif x in (2, 3, w - 4, w - 3) or y in (2, 3, h - 4, h - 3):
                px[y][x] = WHITE
            elif y in (h // 2 - 1, h // 2):
                px[y][x] = WHITE
            elif (x + y) % 6 == 0:
                px[y][x] = CLAY
    def ring(cx, cy, r0, r1, color):
        for y in range(h):
            for x in range(w):
                d = (x - cx + 0.5) ** 2 + (y - cy + 0.5) ** 2
                if r0 * r0 <= d <= r1 * r1:
                    px[y][x] = color
    ring(24, 40, 5.5, 7, WHITE)
    for cy in (24, 56):                  # the two start marks, on rows 1 and 3
        ring(24, cy, 0, 5, LIGHT)
        ring(24, cy, 5, 6.5, WHITE)
    return px


def pedestal_art():
    """A stone plinth (32x16) for the badge to sit on."""
    w, h = 32, 16
    px = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            if 2 <= x <= 29 and 1 <= y <= 4:                  # the top slab
                px[y][x] = 13 if y > 1 else 5
            elif 4 <= x <= 27 and 5 <= y <= 13:               # the column
                px[y][x] = 4 if x < 9 else (3 if x < 24 else 2)
            elif 2 <= x <= 29 and y == 14:                    # the base
                px[y][x] = 3
            if px[y][x] and (y == 4 or y == 13) and 4 <= x <= 27:
                px[y][x] = 14                                 # gold trim
    # outline
    out = [row[:] for row in px]
    for y in range(h):
        for x in range(w):
            if px[y][x] == 0 and any(0 <= x + dx < w and 0 <= y + dy < h and px[y + dy][x + dx]
                                     for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = DARK
    return out


ART = {"STAGE": stage_art, "PEDESTAL": pedestal_art}

# The player's badge (graphics/gym_badges, made by badge_art.py): 16 tiles in a row in the tileset,
# drawn in palette BADGE_PAL. The game swaps in the player's own badge tiles and colors when the
# gym loads (src/gym_badge.c); the tileset holds the Normal badge.
BADGE_PAL = 11
BADGE_TILES = {}


def badge_tiles(tiles_img, cx, cy):
    """The 4 tile entries for block (cx, cy) of the 32x32 badge; adds all 16 tiles the first time."""
    from PIL import Image
    if not BADGE_TILES:
        img = Image.open(ROOT / "graphics/gym_badges/normal.png")
        px = list(img.getdata())
        for ty in range(4):
            for tx in range(4):
                tile = [px[(ty * 8 + y) * 32 + tx * 8 + x] for y in range(8) for x in range(8)]
                BADGE_TILES[(tx, ty)] = append_tile(tiles_img, tile) | (BADGE_PAL << 12)
        BADGE_TILES["start"] = BADGE_TILES[(0, 0)] & 0x3FF
    return [BADGE_TILES[(cx * 2 + tx, cy * 2 + ty)] for ty in range(2) for tx in range(2)]
_ART_CACHE = {}


def art_tiles(tiles_img, name, cx, cy):
    """The 4 tile entries for block (cx, cy) of a piece of art, adding its tiles once."""
    if name not in _ART_CACHE:
        _ART_CACHE[name] = (ART[name](), {})
    px, done = _ART_CACHE[name]
    entries = []
    for ty in range(2):
        for tx in range(2):
            key = (cx * 2 + tx, cy * 2 + ty)
            if key not in done:
                x0, y0 = key[0] * 8, key[1] * 8
                tile = [px[y0 + y][x0 + x] for y in range(8) for x in range(8)]
                done[key] = append_tile(tiles_img, tile) | (ART_PAL << 12)
            entries.append(done[key])
    return entries


# A dotted outline, shown on invisible walls while decorating (gym palette 12, index 7: dark)
MARKER_TILES = []


def make_marker_tiles(tiles_img):
    """Adds the marker's 8x8 tiles to the tileset image; returns the 4 tile entries."""
    dark = 7
    tl = [[0] * 8 for _ in range(8)]
    for i in range(0, 8, 2):
        tl[1][i] = dark
        tl[i][1] = dark
    import copy
    tr = [row[::-1] for row in tl]
    bl = tl[::-1]
    br = [row[::-1] for row in bl]
    entries = []
    for t in (tl, tr, bl, br):
        entries.append(append_tile(tiles_img, [c for row in t for c in row]) | (12 << 12))
    return entries


def append_tile(tiles_img, pixels):
    from PIL import Image
    n_tiles = (tiles_img[0].height // 8) * 16
    used = tiles_img[1]
    idx = used
    if idx >= n_tiles:
        img = Image.new("P", (128, tiles_img[0].height + 8), 0)
        img.putpalette(tiles_img[0].getpalette())
        img.paste(tiles_img[0], (0, 0))
        tiles_img[0] = img
    t = Image.new("P", (8, 8))
    t.putdata(pixels)
    tiles_img[0].paste(t, ((idx % 16) * 8, (idx // 16) * 8))
    tiles_img[1] += 1
    return 0x280 + idx


def count_used_tiles(img):
    """Number of tiles up to the last non-empty one."""
    rows = img.height // 8
    last = 0
    for i in range(rows * 16):
        x, y = (i % 16) * 8, (i // 16) * 8
        if any(img.crop((x, y, x + 8, y + 8)).getdata()):
            last = i + 1
    return last


# ---------------------------------------------------------------------------
# Decorations drawn in code (decor_art.py). There's no room left for their tiles in the gym's
# tileset, so they go in video memory the gym never uses: tiles of the indoor tileset that no gym
# block uses, and unused gym tiles. The game copies them there whenever the gym's tileset loads
# (GymDecor_PatchTilesetGfx in src/gym_decor.c). Their blocks go in unused gym tileset blocks.
# ---------------------------------------------------------------------------
DYN_PALETTES = (8, 9, 10)     # the shared decoration palettes (decor_art.PALETTES)


class DynArt:
    """Hands out the free video memory tiles and blocks to the code-drawn decorations."""
    def __init__(self, ts, free_tiles, free_mts):
        self.ts, self.free_tiles, self.free_mts = ts, list(free_tiles), list(free_mts)
        self.tiles = []          # (vram tile, 64 color indices), in the order they were given out
        self.index = {}

    def tile_entry(self, slot, px):
        if not any(px):
            return 0                         # tile 0 is blank
        rows = [px[i * 8:(i + 1) * 8] for i in range(8)]
        variants = {
            0: px,
            0x400: [c for r in rows for c in r[::-1]],
            0x800: [c for r in rows[::-1] for c in r],
            0xC00: [c for r in rows[::-1] for c in r[::-1]],
        }
        for flip, v in variants.items():
            if tuple(v) in self.index:
                return self.index[tuple(v)] | flip | (slot << 12)
        if not self.free_tiles:
            raise SystemExit("out of free video memory tiles for code-drawn decorations")
        t = self.free_tiles.pop(0)
        self.tiles.append((t, px))
        self.index[tuple(px)] = t
        return t | (slot << 12)

    def block(self, entries, attr):
        if not self.free_mts:
            raise SystemExit("out of free gym tileset blocks for code-drawn decorations")
        m = self.free_mts.pop(0)
        while len(self.ts.mt) <= m - 0x280:
            self.ts.mt.append([0] * 8)
            self.ts.attr.append(0)
        self.ts.mt[m - 0x280] = list(entries)
        self.ts.attr[m - 0x280] = attr
        return m

    def decoration(self, d):
        """Returns the decoration's rows of cells (metatile, collides, edit-mode metatile)."""
        canvas = decor_art.PIECES[d["art"]]()
        tiles8, _ = decor_art.to_tiles(canvas)
        flat, trainer = d.get("flat", False), d.get("trainer", False)
        rows = []
        for cy in range(canvas.ch):
            row = []
            for cx in range(canvas.cw):
                ents = [self.tile_entry(*tiles8[(cx * 2 + tx, cy * 2 + ty)]) for ty in range(2) for tx in range(2)]
                behind = not flat and not trainer and canvas.ch > 1 and cy == 0
                m = self.block(FLOOR_TILES + ents, with_layer(MB_NORMAL, LAYER_NORMAL if behind else LAYER_COVERED))
                if trainer:
                    # Plain floor (a person stands there); the icon only while decorating
                    row.append((FLOOR, True, m))
                else:
                    row.append((m, not (flat or behind), None))
            rows.append(row)
        return rows

    def uploads(self):
        """(first vram tile, count) runs, in the order of self.tiles."""
        runs = []
        for t, _ in self.tiles:
            if runs and runs[-1][0] + runs[-1][1] == t:
                runs[-1][1] += 1
            else:
                runs.append([t, 1])
        return runs


def free_space(ts, catalog, keep):
    """Video memory tiles and tileset blocks the gym never uses."""
    used_mt = set(keep)
    for d, rows in catalog:
        for row in rows:
            for m, _, e in row:
                used_mt.add(m)
                if isinstance(e, int):
                    used_mt.add(e)
    used_tiles = {0}
    for m in used_mt:
        ents = PRIM_MT[m] if m < 0x280 else ts.mt[m - 0x280]
        used_tiles.update(e & 0x3FF for e in ents)
    used_tiles.update(range(BADGE_TILES["start"], BADGE_TILES["start"] + 16))
    free_tiles = [t for t in range(1, 1024) if t not in used_tiles]
    free_mts = [m for m in range(0x281, 0x280 + 384) if m not in used_mt]
    return free_tiles, free_mts


# Gym blocks build_pallet_gym.py puts on the map (keep them out of the free pool)
MAP_METATILES = [0x281, 0x282, 0x291, 0x2D3, 0x299, 0x2AA, 0x2AB, 0x2AC, 0x2B2, 0x2B3, 0x2B4, 0x2A1, 0x2A9, 0x008]


def build():
    from PIL import Image
    if DST.exists():
        shutil.rmtree(DST)
    shutil.copytree(SRC, DST)
    tiles = Image.open(SRC / "tiles.png")
    tiles_img = [tiles, count_used_tiles(tiles)]
    MARKER_TILES[:] = make_marker_tiles(tiles_img)
    _ART_CACHE.clear()
    BADGE_TILES.clear()

    ts = Tileset(tiles_img)
    catalog = []
    for d in DECORATIONS:
        if "art" not in d:
            catalog.append((d, [[ts.cell(c, d.get("keep", False)) for c in row] for row in d["cells"]]))
    fixed = {k: ts.cell(v, False)[0] for k, v in FIXED.items()}
    keep = set(MAP_METATILES) | set(fixed.values())
    layout = ROOT / "data/layouts/PalletTown_Gym_Frlg/map.bin"
    if layout.exists():
        data = layout.read_bytes()
        keep |= {struct.unpack_from("<H", data, i)[0] & 0x3FF for i in range(0, len(data), 2)}
    dyn = DynArt(ts, *free_space(ts, catalog, keep))
    static = dict((d["key"], rows) for d, rows in catalog)
    catalog = [(d, static[d["key"]] if "art" not in d else dyn.decoration(d)) for d in DECORATIONS]
    from PIL import Image
    badge_pal = Image.open(ROOT / "graphics/gym_badges/normal.png").getpalette()[:48]
    ts.pals[BADGE_PAL] = [tuple(badge_pal[i:i + 3]) for i in range(0, 48, 3)]
    for slot in DYN_PALETTES:
        ts.pals[slot] = [(255, 0, 255)] + [c for _, c in decor_art.PALETTES[slot]]
    tiles_img[0].save(DST / "tiles.png")
    ts.write_palettes()
    if tiles_img[1] > 384:
        raise SystemExit(f"the gym tileset has {tiles_img[1]} tiles; the limit is 384")
    if len(ts.mt) > 384:
        raise SystemExit(f"the gym tileset has {len(ts.mt)} blocks; the limit is 384")
    (DST / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *m) for m in ts.mt))
    (DST / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in ts.attr))
    write_c(catalog, BADGE_TILES["start"], dyn)
    print(f"code-drawn decorations: {len(dyn.tiles)} tiles in {len(dyn.uploads())} runs; "
          f"{len(dyn.free_tiles)} tiles and {len(dyn.free_mts)} blocks still free")
    return ts, catalog, fixed


def write_c(catalog, badge_tile_start, dyn):
    ids = "\n".join(f"    GYM_DECOR_{d['key']}," for d, _ in catalog)
    (ROOT / "include/constants/gym_decor.h").write_text(f"""// Generated by tools/gym_hack/build_gym_decor.py. Don't edit by hand.
#ifndef GUARD_CONSTANTS_GYM_DECOR_H
#define GUARD_CONSTANTS_GYM_DECOR_H

// Saves store decorations by these numbers: only ever add new ones at the end.
enum GymDecorId
{{
{ids}
    GYM_DECOR_COUNT
}};

// The player's badge in the gym tileset: 16 tiles from this one (in the secondary tileset), in
// this palette slot. src/gym_badge.c swaps in the player's own badge.
#define GYM_BADGE_TILE_START {badge_tile_start}
#define GYM_BADGE_PALETTE    {BADGE_PAL}

#endif // GUARD_CONSTANTS_GYM_DECOR_H
""")

    def flags(c, e):
        return (" | GYM_DECOR_SOLID" if c else "") + (" | GYM_DECOR_WATER" if e == "water" else "")

    parts = ["// Generated by tools/gym_hack/build_gym_decor.py from its DECORATIONS list. Don't edit by hand.\n"]
    for d, rows in catalog:
        blocks = ", ".join(f"0x{m:03X}{flags(c, e)}" for row in rows for m, c, e in row)
        parts.append(f"static const u16 sGymDecorBlocks_{d['key']}[] = {{ {blocks} }};\n")
        if any(isinstance(e, int) for row in rows for _, _, e in row):
            edits = ", ".join(f"0x{(e if isinstance(e, int) else m):03X}" for row in rows for m, _, e in row)
            parts.append(f"static const u16 sGymDecorEditBlocks_{d['key']}[] = {{ {edits} }};\n")
    parts.append("\nconst struct GymDecor gGymDecor[GYM_DECOR_COUNT] =\n{\n")
    for d, rows in catalog:
        has_edit = any(isinstance(e, int) for row in rows for _, _, e in row)
        unlock = d["unlock"] or "0"
        parts.append(f"""    [GYM_DECOR_{d['key']}] =
    {{
        .name = COMPOUND_STRING("{d['name']}"),
        .description = COMPOUND_STRING("{d['desc']}"),
        .price = {d['price']},
        .unlockFlag = {unlock},
        .category = {d.get('category', 'GYM_DECOR_CAT_GENERAL')},
        .width = {len(rows[0])},
        .height = {len(rows)},
        .blocks = sGymDecorBlocks_{d['key']},
        .editBlocks = {f"sGymDecorEditBlocks_{d['key']}" if has_edit else "NULL"},
    }},
""")
    parts.append("};\n")
    # The code-drawn decorations' tiles, copied to video memory in these runs
    words = []
    for _, px in dyn.tiles:
        data = bytes((px[i] | (px[i + 1] << 4)) for i in range(0, 64, 2))
        words += [struct.unpack_from("<I", data, k)[0] for k in range(0, 32, 4)]
    parts.append("\n// Tiles of the decorations drawn in code (tools/gym_hack/decor_art.py). They're copied to\n"
                 "// video memory the gym doesn't otherwise use: {first tile, number of tiles}, in order.\n")
    parts.append("static const u32 sGymDecorArtTiles[] =\n{\n")
    for k in range(0, len(words), 8):
        parts.append("    " + ", ".join(f"0x{x:08X}" for x in words[k:k + 8]) + ",\n")
    parts.append("};\n")
    parts.append("static const u16 sGymDecorArtRuns[][2] =\n{\n" + "".join(f"    {{{a}, {n}}},\n" for a, n in dyn.uploads()) + "};\n")
    (ROOT / "src/data/gym_decor.h").write_text("".join(parts))


def preview(catalog, out):
    from PIL import Image, ImageDraw
    prim = rm.Tileset(rm.tileset_dir("gTileset_BuildingFrlg"))
    sec = rm.Tileset(DST)
    cell_w, cell_h = 120, 144
    cols = 6
    img = Image.new("RGB", (cols * cell_w, ((len(catalog) + cols - 1) // cols) * cell_h), (40, 40, 48))
    d = ImageDraw.Draw(img)
    for k, (dec, rows) in enumerate(catalog):
        ox, oy = (k % cols) * cell_w, (k // cols) * cell_h
        w, h = len(rows[0]), len(rows)
        tile = Image.new("RGB", ((w + 2) * 16, (h + 2) * 16))
        for y in range(h + 2):
            for x in range(w + 2):
                rm.draw_metatile(tile, x * 16, y * 16, FLOOR, prim, sec)
        if "art" in dec:
            # Code-drawn: its tiles live in video memory, not the tileset
            art = decor_art.render(decor_art.PIECES[dec["art"]]())
            floor = tile.crop((0, 0, art.width, art.height))
            mask = Image.new("L", art.size, 0)
            canvas = decor_art.PIECES[dec["art"]]()
            for yy in range(canvas.h):
                for xx in range(canvas.w):
                    if canvas.px[yy][xx] is not None:
                        mask.putpixel((xx, yy), 255)
            floor.paste(art, (0, 0), mask)
            tile.paste(floor, (16, 16))
        else:
            for y, row in enumerate(rows):
                for x, (m, c, e) in enumerate(row):
                    rm.draw_metatile(tile, (x + 1) * 16, (y + 1) * 16, e if isinstance(e, int) else m, prim, sec)
        img.paste(tile, (ox + 4, oy + 4))
        d.text((ox + 4, oy + cell_h - 26), dec["name"][:19], fill=(255, 255, 255))
        cat = dec.get("category", "")
        lock = f"  {cat.split('TYPE_')[-1][:-1]}" if "TYPE(" in cat else ("" if not dec["unlock"] else f"  badge {dec['unlock'][11]}")
        d.text((ox + 4, oy + cell_h - 14), f"${dec['price']}{lock}", fill=(255, 220, 120))
    img = img.resize((img.width * 2, img.height * 2), Image.NEAREST)
    img.save(out)


if __name__ == "__main__":
    ts, catalog, fixed = build()
    print(f"{len(catalog)} decorations, {len(ts.mt)} of 384 gym blocks used")
    print("fixed:", {k: hex(v) for k, v in fixed.items()})
    if len(sys.argv) > 1:
        preview(catalog, sys.argv[1])
