#!/usr/bin/env python3
"""Gym decorations: the catalog, and the art the game puts the gym's room together from.

Usage: python3 tools/gym_hack/build_gym_decor.py   (build_pallet_gym.py runs it)

Writes:
  data/tilesets/secondary/pallet_gym/   the Pallet Town Gym's tileset as the map tools see it (the
                                        empty room; the game puts the real one together itself,
                                        src/gym_room.c)
  include/constants/gym_decor.h         GYM_DECOR_* ids
  src/data/gym_decor.h                  the art bank, every square's possible look, the floors,
                                        wall styles and pool edges, and the catalog (names,
                                        prices, unlocks, and each piece's looks every way round)

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
    dict(key="GYM_WALL", name="GYM WALL", price=300, unlock=None, kind="WALL",
         desc="Joins up with the walls beside it.", cells=[["g:29B"]]),
    dict(key="GYM_STATUE", name="GYM STATUE", price=5000, unlock=None,
         desc="Every GYM needs one.", cells=[["g:2A1~"], ["g:2A9"]]),
    # --- Unlocked by beating Gym Leaders --------------------------------------
    dict(key="POOL_WATER", name="POOL WATER", price=800, unlock=BADGE[2], keep=True, kind="WATER",
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

# --- Trainers who join from the road (src/gym_candidate.c: gGymRecruits). Not sold: the trainer
# offers to join after the player beats them, and one goes to the gym's storage.
for _type, _name, _cls in (("NORMAL", "ANDREA", "LASS"), ("FIGHTING", "HITOSHI", "BLACK BELT"),
                           ("FLYING", "PERRY", "BIRD KEEPER"), ("POISON", "JARED", "BIKER"),
                           ("GROUND", "DREW", "CAMPER"), ("ROCK", "MARCOS", "HIKER"),
                           ("BUG", "RICK", "BUG CATCHER"), ("GHOST", "ANGELICA", "CHANNELER"),
                           ("STEEL", "BERNIE", "ENGINEER"), ("FIRE", "THOMAS", "GENTLEMAN"),
                           ("WATER", "NED", "FISHERMAN"), ("GRASS", "ALICIA", "PICNICKER"),
                           ("ELECTRIC", "LUCA", "ROCKER"), ("PSYCHIC", "NELSON", "JUGGLER"),
                           ("ICE", "NORA", "SWIMMER"), ("DRAGON", "SHIRLEY", "SWIMMER"),
                           ("DARK", "LEWIS", "BURGLAR"), ("FAIRY", "JULIA", "LASS")):
    DECORATIONS.append(dict(key=f"RECRUIT_{_type}", name=_name, price=0, unlock=None,
                            desc=f"{_cls}. Joined your GYM. Trains {_type} types.", art="trainer_recruit",
                            trainer=True, category="GYM_DECOR_CAT_TRAINERS"))

# --- Gym Leader Convention trophies (src/gym_convention.c): one for clearing each hall, one for
# clearing them all. Not sold: each goes to the gym's storage when it's won.
for _hall in ("NORMAL", "FIRE", "WATER", "GRASS", "ELECTRIC", "ICE", "FIGHTING", "POISON", "GROUND", "FLYING",
              "PSYCHIC", "BUG", "ROCK", "GHOST", "DRAGON", "DARK", "STEEL", "FAIRY", "CHAMPIONS"):
    DECORATIONS.append(dict(key=f"TROPHY_{_hall}", name=f"{_hall} TROPHY", price=0,
                            unlock=f"FLAG_CONVENTION_TROPHY_{_hall}",
                            desc=("Won in the CHAMPIONS' HALL." if _hall == "CHAMPIONS"
                                  else f"Won in the {_hall} HALL."),
                            art=f"convention_trophy_{_hall.lower()}", category="GYM_DECOR_CAT_TROPHIES"))
DECORATIONS.append(dict(key="TROPHY_GRAND", name="GRAND TROPHY", price=0, unlock="FLAG_CONVENTION_TROPHY_GRAND",
                        desc="Every hall of the CONVENTION won!", art="convention_trophy_grand",
                        category="GYM_DECOR_CAT_TROPHIES"))

# --- Floors (painted on, square by square) and wall styles (the whole gym's walls): bought once
# (tools/gym_hack/gym_room_art.py draws them). The first of each comes with the gym.
import gym_room_art as _RA  # noqa: E402
for _i, (_k, _n, _p, _u, _d, _x) in enumerate(_RA.FLOORS):
    DECORATIONS.append(dict(key=f"FLOOR_{_k}", name=_n, price=_p, unlock=_u, desc=_d, kind="FLOOR", style=_i,
                            category="GYM_DECOR_CAT_FLOORS"))
for _i, (_k, _n, _p, _u, _d) in enumerate(_RA.WALLS):
    DECORATIONS.append(dict(key=f"WALLS_{_k}", name=_n, price=_p, unlock=_u, desc=_d, kind="WALL_STYLE", style=_i,
                            category="GYM_DECOR_CAT_WALLS"))

# --- The ways each piece can be turned (START, while decorating): its sides and back
# (tools/gym_hack/decor_art_turned.py). The chairs turn into each other.
import decor_art_turned as _TA  # noqa: E402
decor_art.PIECES.update(_TA.PIECES)


def _turn_spec(t):
    if t == "mirror":
        return ("mirror", 0)
    if isinstance(t, tuple):
        return ("mirror-art", t[1])
    return t


for _d in DECORATIONS:
    _turns = _TA.TURNS.get(_d["key"]) or _TA.TURNS.get(_d.get("art"))
    if _turns and _d.get("kind", "PIECE") == "PIECE":
        _d["facings"] = {i + 1: _turn_spec(t) for i, t in enumerate(_turns)}
for _d in DECORATIONS:
    if _d["key"] == "CHAIR_LEFT":       # (faces right): then its back, facing left, its front
        _d["facings"] = {1: "chair_back", 2: [["f:4E~"]], 3: "chair_front"}
        _d["flat"] = True
    elif _d["key"] == "CHAIR_RIGHT":
        _d["facings"] = {1: "chair_front", 2: [["f:4B~"]], 3: "chair_back"}
        _d["flat"] = True

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
# The gym's looks: everything the room can show, as "looks" (a square's two layers of tiles) over
# an art bank of 8x8 tiles. The game doesn't keep any of this in the gym's tileset: when the room
# is drawn (src/gym_room.c) it puts each square together - its floor, and what's on it - in the
# gym tileset's 384 blocks (kept in RAM), and copies the art those use into free video memory.
#
# A tile reference (u32): bits 0-11 the art bank tile (0: blank), 12 h-flip, 13 v-flip,
# 14-17 the palette, 18 FIXED: bits 0-9 are a video memory tile that's always there (the indoor
# tileset's furniture, the badge).
# ---------------------------------------------------------------------------
DYN_PALETTES = (8, 9, 10)     # the shared decoration palettes (decor_art.PALETTES)
REF_HFLIP, REF_VFLIP, REF_FIXED = 1 << 12, 1 << 13, 1 << 18
LOOK_FLOOR_UNDER, LOOK_SOLID, LOOK_WATER = 1, 2, 4
BADGE_VRAM = 0x3E0            # the player's badge: 16 tiles here (src/gym_badge.c)
DOOR_VRAM = 0x3F0             # (doors animate here: kept out of the free pool)
LAYER_ATTR_SHIFT = 29


class ArtBank:
    def __init__(self):
        self.tiles = [tuple([0] * 64)]       # 0: blank
        self.index = {self.tiles[0]: 0}

    def ref(self, px, pal):
        px = tuple(px)
        if not any(px):
            return 0
        rows = [px[i * 8:(i + 1) * 8] for i in range(8)]
        variants = {
            0: px,
            REF_HFLIP: tuple(c for r in rows for c in r[::-1]),
            REF_VFLIP: tuple(c for r in rows[::-1] for c in r),
            REF_HFLIP | REF_VFLIP: tuple(c for r in rows[::-1] for c in r[::-1]),
        }
        for flip, v in variants.items():
            if v in self.index:
                return self.index[v] | flip | (pal << 14)
        self.index[px] = len(self.tiles)
        self.tiles.append(px)
        return (len(self.tiles) - 1) | (pal << 14)


def fixed_ref(entry):
    """A map tile entry (tile, flips, palette) for a tile that's always in video memory"""
    return (entry & 0x3FF) | REF_FIXED | (REF_HFLIP if entry & 0x400 else 0) | (REF_VFLIP if entry & 0x800 else 0) | ((entry >> 12) << 14)


def canvas_refs(canvas):
    """{(tx, ty): ref} for every 8x8 tile of a decor_art canvas"""
    tiles8, _ = decor_art.to_tiles(canvas)
    return {k: BANK.ref(px, slot) for k, (slot, px) in tiles8.items()}


BANK = ArtBank()


class Looks:
    def __init__(self):
        self.list = []
        self.index = {}

    def add(self, refs, attr, flags):
        key = (tuple(refs), attr, flags)
        if key not in self.index:
            self.index[key] = len(self.list)
            self.list.append(key)
        return self.index[key]


LOOKS = Looks()


def tile_pixels(ts, tile):
    """The 64 color indices of a tile in the (virtual) gym tileset image"""
    img = ts.tiles_img[0]
    i = tile - 0x280
    x, y = (i % 16) * 8, (i // 16) * 8
    return list(img.crop((x, y, x + 8, y + 8)).getdata())


def entry_ref(ts, e):
    tile = e & 0x3FF
    if tile < 0x280:
        return fixed_ref(e) if tile else 0
    start = BADGE_TILES.get("start")
    if start is not None and start <= tile < start + 16:
        # the player's badge: in video memory at BADGE_VRAM (src/gym_badge.c)
        return fixed_ref((e & ~0x3FF) | (BADGE_VRAM + tile - start))
    pal = e >> 12
    r = BANK.ref(tile_pixels(ts, tile), pal)
    if r == 0:
        return 0
    return r ^ (REF_HFLIP if e & 0x400 else 0) ^ (REF_VFLIP if e & 0x800 else 0)


def metatile_look(ts, m, collides, water=False):
    """A look from a metatile of the (virtual) gym tileset or the indoor tileset"""
    ents, attr = (PRIM_MT[m], PRIM_ATTR[m]) if m < 0x280 else (ts.mt[m - 0x280], ts.attr[m - 0x280])
    floor_under = list(ents[:4]) == list(FLOOR_TILES)
    refs = ([0, 0, 0, 0] if floor_under else [entry_ref(ts, e) for e in ents[:4]]) + [entry_ref(ts, e) for e in ents[4:]]
    flags = (LOOK_FLOOR_UNDER if floor_under else 0) | (LOOK_SOLID if collides else 0) | (LOOK_WATER if water else 0)
    return LOOKS.add(refs, attr, flags)


def art_cell_looks(canvas, flat, trainer=False):
    """Rows of (look, edit look) for a decor_art canvas, over the floor"""
    refs = canvas_refs(canvas)
    rows = []
    for cy in range(canvas.ch):
        row = []
        for cx in range(canvas.cw):
            top = [refs[(cx * 2 + tx, cy * 2 + ty)] for ty in range(2) for tx in range(2)]
            behind = not flat and not trainer and canvas.ch > 1 and cy == 0
            layer = LAYER_NORMAL_TYPE if behind else LAYER_COVERED
            look = LOOKS.add([0, 0, 0, 0] + top, with_layer(MB_NORMAL, layer), LOOK_FLOOR_UNDER | (0 if (flat or behind) else LOOK_SOLID))
            if trainer:
                plain = LOOKS.add([0] * 8, with_layer(MB_NORMAL, LAYER_COVERED), LOOK_FLOOR_UNDER | LOOK_SOLID)
                row.append((plain, look))
            else:
                row.append((look, None))
        rows.append(row)
    return rows


LAYER_NORMAL_TYPE = LAYER_NORMAL      # (METATILE_LAYER_TYPE_NORMAL: the top layer over people)


def spec_looks(ts, d):
    """Rows of (look, edit look) for a decoration made of cell specs"""
    rows = []
    for row in d["cells"]:
        out = []
        for spec in row:
            m, collides, edit = ts.cell(spec, d.get("keep", False))
            look = metatile_look(ts, m, collides, edit == "water")
            out.append((look, metatile_look(ts, edit, collides) if isinstance(edit, int) else None))
        rows.append(out)
    return rows


def facing_looks(ts, d, facing):
    """A decoration's looks for one facing (0 down/front, 1 left, 2 up/back, 3 right), or None"""
    if facing == 0:
        if "art" in d:
            return art_cell_looks(decor_art.PIECES[d["art"]](), d.get("flat", False), d.get("trainer", False))
        return spec_looks(ts, d)
    alt = d.get("facings", {}).get(facing)
    if alt is None:
        return None
    if isinstance(alt, tuple) and alt[0] == "mirror":
        src = facing_looks(ts, d, alt[1])
        return mirror_rows(src)
    if isinstance(alt, tuple) and alt[0] == "mirror-art":
        return mirror_rows(art_cell_looks(decor_art.PIECES[alt[1]](), d.get("flat", False)))
    if isinstance(alt, str):
        return art_cell_looks(decor_art.PIECES[alt](), d.get("flat", False))
    return spec_looks(ts, dict(d, cells=alt))


def mirror_rows(rows):
    """The same looks, flipped left to right"""
    out = []
    for row in rows:
        new = []
        for look, edit in reversed(row):
            new.append((mirror_look(look), mirror_look(edit) if edit is not None else None))
        out.append(new)
    return out


def mirror_look(look):
    refs, attr, flags = LOOKS.list[look]
    def m(r):
        return r ^ REF_HFLIP if r else 0
    refs = [m(refs[1]), m(refs[0]), m(refs[3]), m(refs[2]), m(refs[5]), m(refs[4]), m(refs[7]), m(refs[6])]
    return LOOKS.add(refs, attr, flags)


def quarter_refs(canvas):
    """The top left 8x8 tile of a canvas, as a ref"""
    return canvas_refs(canvas)[(0, 0)]


def build():
    from PIL import Image
    import gym_room_art as RA
    global BANK, LOOKS
    BANK, LOOKS = ArtBank(), Looks()
    _ART_CACHE.clear()
    BADGE_TILES.clear()

    # The gym's own tileset (only what the empty room's map needs, for the map tools; the game
    # puts the room together itself)
    if DST.exists():
        shutil.rmtree(DST)
    shutil.copytree(SRC, DST)
    tiles = Image.open(SRC / "tiles.png")
    base_img = [tiles.copy(), count_used_tiles(tiles)]
    base = Tileset(base_img)
    fixed = {k: base.cell(v, False)[0] for k, v in FIXED.items()}
    from PIL import Image as _I
    badge_pal = _I.open(ROOT / "graphics/gym_badges/normal.png").getpalette()[:48]
    base.pals[BADGE_PAL] = [tuple(badge_pal[i:i + 3]) for i in range(0, 48, 3)]
    for slot in DYN_PALETTES:
        base.pals[slot] = [(255, 0, 255)] + [c for _, c in decor_art.PALETTES[slot]]
    base_img[0].save(DST / "tiles.png")
    base.write_palettes()
    (DST / "metatiles.bin").write_bytes(b"".join(struct.pack("<8H", *m) for m in base.mt))
    (DST / "metatile_attributes.bin").write_bytes(b"".join(struct.pack("<I", a) for a in base.attr))

    # Everything the room can show, from a virtual tileset with no size limit
    vimg = [tiles.copy(), count_used_tiles(tiles)]
    ts = Tileset(vimg)
    MARKER_TILES[:] = make_marker_tiles(vimg)
    catalog = []
    for d in DECORATIONS:
        kind = d.get("kind", "PIECE")
        facings = []
        if kind == "PIECE":
            for f in range(4):
                rows = facing_looks(ts, d, f)
                if rows is None:
                    break
                facings.append(rows)
        catalog.append((d, facings))

    # Floors: 4 looks each (2x2 pattern), bottom layer only
    def floor_looks(canvas, flags):
        refs = canvas_refs(canvas)
        out = []
        for cy in range(2):
            for cx in range(2):
                bottom = [refs[(cx * 2 + tx, cy * 2 + ty)] for ty in range(2) for tx in range(2)]
                out.append(LOOKS.add(bottom + [0] * 4, with_layer(MB_NORMAL, LAYER_COVERED), flags))
        return out
    floors = []
    for key, name, price, unlock, desc, extra in RA.FLOORS:
        flags = LOOK_SOLID if extra == "LAVA" else 0
        if key == "GYM":
            gym = metatile_look(ts, FLOOR, False)
            refs, attr, _ = LOOKS.list[gym]
            look = LOOKS.add([entry_ref(ts, e) for e in GYM_MT[FLOOR - 0x280][:4]] + [0] * 4, with_layer(MB_NORMAL, LAYER_COVERED), 0)
            floors.append([look] * 4)
        else:
            floors.append(floor_looks(RA.floor_canvas(key), flags))
    shadow = canvas_refs(RA.shadow_canvas())
    shadow_look = LOOKS.add([0] * 4 + [shadow[(tx, ty)] for ty in range(2) for tx in range(2)],
                            with_layer(MB_NORMAL, LAYER_COVERED), LOOK_FLOOR_UNDER)

    # Wall styles: the top face's quarter (per case, top left; flipped for the others) and the
    # front face (4 variants: no ends, left end, right end, both)
    walls = []
    for key, name, price, unlock, desc in RA.WALLS:
        quarters = [quarter_refs(RA.wall_top_quarter(key, case)) for case in RA.QUARTER_CASES]
        faces = []
        for lc, rc in ((False, False), (True, False), (False, True), (True, True)):
            refs = canvas_refs(RA.wall_face(key, lc, rc))
            faces.append([refs[(tx, ty)] for ty in range(2) for tx in range(2)])
        walls.append((quarters, faces))
    water = [quarter_refs(RA.water_quarter(case)) for case in RA.QUARTER_CASES]
    water_attr = with_layer(GYM_ATTR[0x291 - 0x280] & 0x1FF if False else 0, LAYER_COVERED)
    cer_attrs = read_attrs(rm.tileset_dir("gTileset_CeruleanGym") / "metatile_attributes.bin")
    water_behavior = cer_attrs[0x291 - 0x280] & 0x1FF

    # The empty room: a look for each fixed square, or a code (floor, the back wall)
    room = []
    from build_pallet_gym import W as RW, H as RH, MAT, MAT_TILES, BELOW_MAT, STATUES, STATUE_TOP, STATUE_BOTTOM, VOID, PC
    for y in range(RH):
        row = []
        for x in range(RW):
            row.append(None)
        room.append(row)
    def put(x, y, m, solid):
        room[y][x] = metatile_look(ts, m, solid)
    for x in range(RW):
        room[RH - 1][x] = metatile_look(ts, VOID, True)
    for x, m, below in zip(MAT, MAT_TILES, BELOW_MAT):
        put(x, RH - 2, m, False)
        put(x, RH - 1, below, True)
    for x in STATUES:
        put(x, RH - 4, STATUE_TOP, False)
        put(x, RH - 3, STATUE_BOTTOM, True)
    put(PC[0], PC[1] + 1, ts.cell(FIXED["PC_BOTTOM"], False)[0], True)   # (in this tileset: fixed[] is the map tools' one)
    for x in range(RW):
        room[0][x] = "GYM_BASE_WALL_TOP"
        room[1][x] = "GYM_BASE_WALL_FACE"
        room[2][x] = "GYM_BASE_FLOOR_SHADOW"
    for y in range(1, RH - 1):             # the side walls, down to the bottom row
        room[y][0] = room[y][RW - 1] = "GYM_BASE_WALL_TOP"
    # (the original back wall, for the gym's own wall style)
    orig_top = [metatile_look(ts, 0x2D3 if x == RW // 2 else 0x291, True) for x in range(RW)]
    orig_face = metatile_look(ts, 0x299, True)
    orig_shadow = metatile_look(ts, 0x282, False)

    # The badge pedestal's badge: fixed video memory tiles
    write_c(catalog, floors, shadow_look, walls, water, water_behavior, room, orig_top, orig_face, orig_shadow, ts)
    print(f"gym art: {len(BANK.tiles)} tiles, {len(LOOKS.list)} looks")
    return base, catalog, fixed


def free_vram(fixed_tiles):
    """Video memory tiles the room's art can use: everything but the fixed ones, the badge and
    the door animation's"""
    used = set(fixed_tiles) | {0} | set(range(BADGE_VRAM, BADGE_VRAM + 16)) | set(range(DOOR_VRAM, 0x400))
    return [t for t in range(1, 0x400) if t not in used]


def write_c(catalog, floors, shadow_look, walls, water, water_behavior, room, orig_top, orig_face, orig_shadow, ts):
    import gym_room_art as RA
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

// The player's badge: 16 tiles of video memory from this one, in this palette slot. src/gym_badge.c
// puts the player's own badge there.
#define GYM_BADGE_TILE_START {BADGE_VRAM}
#define GYM_BADGE_PALETTE    {BADGE_PAL}

#define GYM_FLOOR_COUNT {len(RA.FLOORS)}
#define GYM_WALL_STYLE_COUNT {len(RA.WALLS)}

#endif // GUARD_CONSTANTS_GYM_DECOR_H
""")
    fixed_tiles = set()
    for refs, _, _ in LOOKS.list:
        for r in refs:
            if r & REF_FIXED:
                fixed_tiles.add(r & 0x3FF)
    parts = ["// Generated by tools/gym_hack/build_gym_decor.py. Don't edit by hand.\n\n"]
    # The art bank
    words = []
    for px in BANK.tiles:
        data = bytes((px[i] | (px[i + 1] << 4)) for i in range(0, 64, 2))
        words += [struct.unpack_from("<I", data, k)[0] for k in range(0, 32, 4)]
    parts.append(f"#define GYM_ART_COUNT {len(BANK.tiles)}\n")
    parts.append("static const u32 sGymArtTiles[GYM_ART_COUNT * 8] =\n{\n")
    for k in range(0, len(words), 8):
        parts.append("    " + ", ".join(f"0x{x:08X}" for x in words[k:k + 8]) + ",\n")
    parts.append("};\n\n")
    # The looks
    parts.append(f"#define GYM_LOOK_COUNT {len(LOOKS.list)}\n")
    parts.append("static const struct GymLook sGymLooks[GYM_LOOK_COUNT] =\n{\n")
    for refs, attr, flags in LOOKS.list:
        parts.append("    {{" + ", ".join(f"0x{r:05X}" for r in refs) + f"}}, 0x{attr:08X}, {flags}}},\n")
    parts.append("};\n\n")
    # Free video memory
    pool = free_vram(fixed_tiles)
    parts.append(f"#define GYM_VRAM_POOL_COUNT {len(pool)}\n")
    parts.append("static const u16 sGymVramPool[GYM_VRAM_POOL_COUNT] =\n{\n")
    for k in range(0, len(pool), 16):
        parts.append("    " + ", ".join(str(t) for t in pool[k:k + 16]) + ",\n")
    parts.append("};\n\n")
    # The badge pedestal's badge tiles are fixed refs to BADGE_VRAM
    # Floors, walls, water
    parts.append("static const u16 sGymFloorLooks[GYM_FLOOR_COUNT][4] =\n{\n")
    for f in floors:
        parts.append("    {" + ", ".join(str(l) for l in f) + "},\n")
    parts.append("};\n")
    parts.append(f"#define GYM_LOOK_SHADOW {shadow_look}\n\n")
    parts.append("static const struct GymWallStyle sGymWallStyles[GYM_WALL_STYLE_COUNT] =\n{\n")
    for quarters, faces in walls:
        parts.append("    {\n        .top = {" + ", ".join(f"0x{r:05X}" for r in quarters) + "},\n        .face = {"
                     + ", ".join("{" + ", ".join(f"0x{r:05X}" for r in f) + "}" for f in faces) + "},\n    },\n")
    parts.append("};\n")
    parts.append("static const u32 sGymWaterQuarters[5] = {" + ", ".join(f"0x{r:05X}" for r in water) + "};\n")
    parts.append(f"#define GYM_WATER_BEHAVIOR 0x{water_behavior:X}\n\n")
    # The empty room
    parts.append("// The empty room: a look for each fixed square; GYM_BASE_FLOOR where the floor is\n")
    parts.append("static const u16 sGymRoomBase[GYM_ROOM_HEIGHT][GYM_ROOM_WIDTH] =\n{\n")
    for row in room:
        parts.append("    {" + ", ".join("GYM_BASE_FLOOR" if v is None else str(v) for v in row) + "},\n")
    parts.append("};\n")
    parts.append("static const u16 sGymOriginalWallTop[GYM_ROOM_WIDTH] = {" + ", ".join(str(v) for v in orig_top) + "};\n")
    parts.append(f"#define GYM_LOOK_ORIGINAL_WALL_FACE {orig_face}\n#define GYM_LOOK_ORIGINAL_WALL_SHADOW {orig_shadow}\n\n")
    # The decorations
    flags_kind = {"PIECE": "GYM_DECOR_KIND_PIECE", "WALL": "GYM_DECOR_KIND_WALL", "WATER": "GYM_DECOR_KIND_WATER",
                  "FLOOR": "GYM_DECOR_KIND_FLOOR", "WALL_STYLE": "GYM_DECOR_KIND_WALL_STYLE"}
    for d, facings in catalog:
        for f, rows in enumerate(facings):
            parts.append(f"static const u16 sGymDecorLooks_{d['key']}_{f}[] = {{ " + ", ".join(str(l) for row in rows for l, _ in row) + " };\n")
            if any(e is not None for row in rows for _, e in row):
                parts.append(f"static const u16 sGymDecorEditLooks_{d['key']}_{f}[] = {{ "
                             + ", ".join(str(e if e is not None else l) for row in rows for l, e in row) + " };\n")
    parts.append("\nconst struct GymDecor gGymDecor[GYM_DECOR_COUNT] =\n{\n")
    for d, facings in catalog:
        unlock = d["unlock"] or "0"
        fac = []
        for f, rows in enumerate(facings):
            edit = f"sGymDecorEditLooks_{d['key']}_{f}" if any(e is not None for row in rows for _, e in row) else "NULL"
            fac.append(f"{{{len(rows[0])}, {len(rows)}, sGymDecorLooks_{d['key']}_{f}, {edit}}}")
        if not fac:
            fac = ["{1, 1, NULL, NULL}"]
        parts.append(f"""    [GYM_DECOR_{d['key']}] =
    {{
        .name = COMPOUND_STRING("{d['name']}"),
        .description = COMPOUND_STRING("{d['desc']}"),
        .price = {d['price']},
        .unlockFlag = {unlock},
        .category = {d.get('category', 'GYM_DECOR_CAT_GENERAL')},
        .kind = {flags_kind[d.get('kind', 'PIECE')]},
        .style = {d.get('style', 0)},
        .numFacings = {len(fac)},
        .facings = {{{", ".join(fac)}}},
    }},
""")
    parts.append("};\n")
    (ROOT / "src/data/gym_decor.h").write_text("".join(parts))


if __name__ == "__main__":
    base, catalog, fixed = build()
    print(f"{len(catalog)} decorations")
    print("fixed:", {k: hex(v) for k, v in fixed.items()})
