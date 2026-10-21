"""The Gym Leader Convention roster (Gym Trainer hack): who's in which hall.

Each entry: (key, NAME, role, female, pic, ow)
  role: "L" Gym Leader / Kahuna / Trial Captain, "E" Elite Four, "C" Champion
  pic:  an existing TRAINER_PIC_* constant, or a path under Emerald Rogue's graphics/trainers/front_pics/
        ("hns:" = Pokémon HnS's graphics/trainers/front_pics/)
  ow:   an existing OBJ_EVENT_GFX_* constant, or a path under Emerald Rogue's graphics/object_events/pics/
        ("hns:" = Pokémon HnS's graphics/object_events/pics/people/)
Sprites: Emerald Rogue (https://github.com/Pokabbie/pokeemerald-rogue) and its artists; Janine from
Pokémon HnS. See CREDITS.md.
"""

HALLS = ["NORMAL", "FIRE", "WATER", "GRASS", "ELECTRIC", "ICE", "FIGHTING", "POISON", "GROUND",
         "FLYING", "PSYCHIC", "BUG", "ROCK", "GHOST", "DRAGON", "DARK", "STEEL", "FAIRY", "CHAMPIONS"]

R = "rogue/npc/"   # (Emerald Rogue's later-generation overworld folder)

ROSTER = {
    "NORMAL": [
        ("whitney", "WHITNEY", "L", True, "johto/whitney_front_pic.png", "people/johto/whitney.png"),
        ("norman", "NORMAN", "L", False, "TRAINER_PIC_LEADER_NORMAN", "OBJ_EVENT_GFX_NORMAN"),
        ("cheren", "CHEREN", "L", False, "rival/cheren.png", R + "rival/cheren.png"),
        ("lenora", "LENORA", "L", True, "unova/lenora.png", R + "unova/lenora.png"),
        ("ilima", "ILIMA", "L", False, "alola/ilima.png", R + "alola/ilima.png"),
        ("larry", "LARRY", "L", False, "paldea/larry.png", R + "paldea/Larry.png"),
    ],
    "FIRE": [
        ("blaine", "BLAINE", "L", False, "TRAINER_PIC_LEADER_BLAINE_FRLG", "OBJ_EVENT_GFX_BLAINE"),
        ("flannery", "FLANNERY", "L", True, "TRAINER_PIC_LEADER_FLANNERY", "OBJ_EVENT_GFX_FLANNERY"),
        ("chili", "CHILI", "L", False, "unova/chili.png", R + "unova/chili.png"),
        ("kiawe", "KIAWE", "L", False, "alola/kiawe.png", R + "alola/kiawe.png"),
        ("kabu", "KABU", "L", False, "galar/kabu.png", R + "galar/kabu.png"),
        ("flint", "FLINT", "E", False, "sinnoh/flint.png", R + "sinnoh/flint.png"),
        ("malva", "MALVA", "E", True, "kalos/malva.png", R + "kalos/malva.png"),
    ],
    "WATER": [
        ("misty", "MISTY", "L", True, "TRAINER_PIC_LEADER_MISTY_FRLG", "OBJ_EVENT_GFX_MISTY"),
        ("juan", "JUAN", "L", False, "TRAINER_PIC_LEADER_JUAN", "OBJ_EVENT_GFX_JUAN"),
        ("wake", "WAKE", "L", False, "sinnoh/crasherwake.png", R + "sinnoh/crasher_wake.png"),
        ("cress", "CRESS", "L", False, "unova/cress.png", R + "unova/cress.png"),
        ("marlon", "MARLON", "L", False, "unova/marlon.png", R + "unova/marlon.png"),
        ("lana", "LANA", "L", True, "alola/lana.png", R + "alola/lana.png"),
        ("nessa", "NESSA", "L", True, "galar/nessa.png", R + "galar/nessa.png"),
        ("kofu", "KOFU", "L", False, "paldea/kofu.png", R + "paldea/Kofu.png"),
        ("siebold", "SIEBOLD", "E", False, "kalos/siebold.png", R + "kalos/siebold.png"),
    ],
    "GRASS": [
        ("erika", "ERIKA", "L", True, "TRAINER_PIC_LEADER_ERIKA_FRLG", "OBJ_EVENT_GFX_ERIKA"),
        ("gardenia", "GARDENIA", "L", True, "sinnoh/gardenia.png", "people/sinnoh/gardenia.png"),
        ("cilan", "CILAN", "L", False, "unova/cilan.png", R + "unova/cilan.png"),
        ("ramos", "RAMOS", "L", False, "kalos/ramos.png", R + "kalos/ramos.png"),
        ("mallow", "MALLOW", "L", True, "alola/mallow.png", R + "alola/mallow.png"),
        ("milo", "MILO", "L", False, "galar/milo.png", R + "galar/milo.png"),
        ("brassius", "BRASSIUS", "L", False, "paldea/brassius.png", R + "paldea/Brassius.png"),
    ],
    "ELECTRIC": [
        ("surge", "LT. SURGE", "L", False, "TRAINER_PIC_LEADER_LT_SURGE_FRLG", "OBJ_EVENT_GFX_LT_SURGE"),
        ("wattson", "WATTSON", "L", False, "TRAINER_PIC_LEADER_WATTSON", "OBJ_EVENT_GFX_WATTSON"),
        ("volkner", "VOLKNER", "L", False, "sinnoh/volkner.png", R + "sinnoh/volkner.png"),
        ("elesa", "ELESA", "L", True, "unova/elesa.png", R + "unova/elesa.png"),
        ("clemont", "CLEMONT", "L", False, "kalos/clemont.png", R + "kalos/clement.png"),
        ("sophocles", "SOPHOCLES", "L", False, "alola/sophocles.png", R + "alola/sophocles.png"),
        ("iono", "IONO", "L", True, "paldea/iono.png", R + "paldea/Iono.png"),
    ],
    "ICE": [
        ("pryce", "PRYCE", "L", False, "johto/pryce_front_pic.png", "people/johto/pryce.png"),
        ("candice", "CANDICE", "L", True, "sinnoh/candice.png", R + "sinnoh/candice.png"),
        ("brycen", "BRYCEN", "L", False, "unova/brycen.png", R + "unova/brycen.png"),
        ("wulfric", "WULFRIC", "L", False, "kalos/wulfric.png", R + "kalos/wulfric.png"),
        ("melony", "MELONY", "L", True, "galar/melony.png", R + "galar/melony.png"),
        ("grusha", "GRUSHA", "L", False, "paldea/grusha.png", R + "paldea/Grusha.png"),
        ("lorelei", "LORELEI", "E", True, "TRAINER_PIC_ELITE_FOUR_LORELEI_FRLG", "OBJ_EVENT_GFX_LORELEI"),
        ("glacia", "GLACIA", "E", True, "TRAINER_PIC_ELITE_FOUR_GLACIA", "OBJ_EVENT_GFX_GLACIA"),
    ],
    "FIGHTING": [
        ("chuck", "CHUCK", "L", False, "johto/chuck_front_pic.png", "people/johto/chuck.png"),
        ("brawly", "BRAWLY", "L", False, "TRAINER_PIC_LEADER_BRAWLY", "OBJ_EVENT_GFX_BRAWLY"),
        ("maylene", "MAYLENE", "L", True, "sinnoh/maylene.png", "people/sinnoh/maylene.png"),
        ("korrina", "KORRINA", "L", True, "kalos/korrina.png", R + "kalos/korrina.png"),
        ("hala", "HALA", "L", False, "alola/hala.png", R + "alola/hala.png"),
        ("bea", "BEA", "L", True, "galar/bea.png", R + "galar/bea.png"),
        ("bruno", "BRUNO", "E", False, "TRAINER_PIC_ELITE_FOUR_BRUNO_FRLG", "OBJ_EVENT_GFX_BRUNO"),
        ("marshal", "MARSHAL", "E", False, "unova/marshal.png", R + "unova/marshal.png"),
    ],
    "POISON": [
        ("janine", "JANINE", "L", True, "hns:leader_janine_hns.png", "hns:gym_leaders/janine_hns.png"),
        ("roxie", "ROXIE", "L", True, "unova/roxie.png", R + "unova/roxie.png"),
        ("koga", "KOGA", "E", False, "TRAINER_PIC_LEADER_KOGA_FRLG", "OBJ_EVENT_GFX_KOGA"),
    ],
    "GROUND": [
        ("giovanni", "GIOVANNI", "L", False, "TRAINER_PIC_LEADER_GIOVANNI_FRLG", "OBJ_EVENT_GFX_GIOVANNI"),
        ("clay", "CLAY", "L", False, "unova/clay.png", R + "unova/clay.png"),
        ("hapu", "HAPU", "L", True, "alola/hapu.png", R + "alola/hapu.png"),
        ("bertha", "BERTHA", "E", True, "sinnoh/bertha.png", R + "sinnoh/bertha.png"),
        ("rika", "RIKA", "E", True, "paldea/rika.png", R + "paldea/rika.png"),
    ],
    "FLYING": [
        ("falkner", "FALKNER", "L", False, "johto/falkner_front_pic.png", "people/johto/falkner.png"),
        ("winona", "WINONA", "L", True, "TRAINER_PIC_LEADER_WINONA", "OBJ_EVENT_GFX_WINONA"),
        ("skyla", "SKYLA", "L", True, "unova/skyla.png", R + "unova/skyla.png"),
        ("kahili", "KAHILI", "E", True, "alola/kahili.png", R + "alola/kahili.png"),
        ("larry_e4", "LARRY", "E", False, "paldea/larry.png", R + "paldea/Larry.png"),
    ],
    "PSYCHIC": [
        ("sabrina", "SABRINA", "L", True, "TRAINER_PIC_LEADER_SABRINA_FRLG", "OBJ_EVENT_GFX_SABRINA"),
        ("tate_liza", "TATE&LIZA", "L", False, "TRAINER_PIC_LEADER_TATE_AND_LIZA", "OBJ_EVENT_GFX_TATE"),
        ("olympia", "OLYMPIA", "L", True, "kalos/olympia.png", R + "kalos/olympia.png"),
        ("tulip", "TULIP", "L", True, "paldea/tulip.png", R + "paldea/Tulip.png"),
        ("will", "WILL", "E", False, "johto/will_front_pic.png", "people/johto/will.png"),
        ("lucian", "LUCIAN", "E", False, "sinnoh/lucian.png", R + "sinnoh/lucian.png"),
        ("caitlin", "CAITLIN", "E", True, "unova/caitlin.png", R + "unova/caitlin.png"),
    ],
    "BUG": [
        ("bugsy", "BUGSY", "L", False, "johto/bugsy_front_pic.png", "people/johto/bugsy.png"),
        ("burgh", "BURGH", "L", False, "unova/burgh.png", R + "unova/burgh.png"),
        ("viola", "VIOLA", "L", True, "kalos/viola.png", R + "kalos/viola.png"),
        ("katy", "KATY", "L", True, "paldea/katy.png", R + "paldea/Katy.png"),
        ("aaron", "AARON", "E", False, "sinnoh/aaron.png", R + "sinnoh/aaron.png"),
    ],
    "ROCK": [
        ("brock", "BROCK", "L", False, "TRAINER_PIC_LEADER_BROCK_FRLG", "OBJ_EVENT_GFX_BROCK"),
        ("roxanne", "ROXANNE", "L", True, "TRAINER_PIC_LEADER_ROXANNE", "OBJ_EVENT_GFX_ROXANNE"),
        ("roark", "ROARK", "L", False, "sinnoh/roark.png", "people/sinnoh/roark.png"),
        ("grant", "GRANT", "L", False, "kalos/grant.png", R + "kalos/grant.png"),
        ("olivia", "OLIVIA", "L", True, "alola/olivia.png", R + "alola/olivia.png"),
        ("gordie", "GORDIE", "L", False, "galar/gordie.png", R + "galar/gordie.png"),
    ],
    "GHOST": [
        ("morty", "MORTY", "L", False, "johto/morty_front_pic.png", "people/johto/morty.png"),
        ("fantina", "FANTINA", "L", True, "sinnoh/fantina.png", "people/sinnoh/fantina.png"),
        ("acerola", "ACEROLA", "L", True, "alola/acerola.png", R + "alola/acerola.png"),
        ("allister", "ALLISTER", "L", False, "galar/allister.png", R + "galar/allister.png"),
        ("ryme", "RYME", "L", True, "paldea/ryme.png", R + "paldea/Ryme.png"),
        ("agatha", "AGATHA", "E", True, "TRAINER_PIC_ELITE_FOUR_AGATHA_FRLG", "OBJ_EVENT_GFX_AGATHA"),
        ("phoebe", "PHOEBE", "E", True, "TRAINER_PIC_ELITE_FOUR_PHOEBE", "OBJ_EVENT_GFX_PHOEBE"),
        ("shauntal", "SHAUNTAL", "E", True, "unova/shauntal.png", R + "unova/shauntal.png"),
    ],
    "DRAGON": [
        ("clair", "CLAIR", "L", True, "johto/clair_front_pic.png", "people/johto/clair.png"),
        ("drayden", "DRAYDEN", "L", False, "unova/drayden.png", R + "unova/drayden.png"),
        ("iris_l", "IRIS", "L", True, "unova/iris.png", R + "unova/iris.png"),
        ("raihan", "RAIHAN", "L", False, "galar/raihan.png", R + "galar/raihan.png"),
        ("lance", "LANCE", "E", False, "TRAINER_PIC_ELITE_FOUR_LANCE_FRLG", "OBJ_EVENT_GFX_LANCE"),
        ("drake", "DRAKE", "E", False, "TRAINER_PIC_ELITE_FOUR_DRAKE", "OBJ_EVENT_GFX_DRAKE"),
        ("drasna", "DRASNA", "E", True, "kalos/drasna.png", R + "kalos/drasna.png"),
        ("hassel", "HASSEL", "E", False, "paldea/hassel.png", R + "paldea/hassel.png"),
    ],
    "DARK": [
        ("piers", "PIERS", "L", False, "galar/piers.png", R + "galar/piers.png"),
        ("marnie", "MARNIE", "L", True, "rival/marnie.png", R + "rival/marnie.png"),
        ("nanu", "NANU", "L", False, "alola/nanu.png", R + "alola/nanu.png"),
        ("karen", "KAREN", "E", True, "johto/karen_front_pic.png", "people/johto/karen.png"),
        ("sidney", "SIDNEY", "E", False, "TRAINER_PIC_ELITE_FOUR_SIDNEY", "OBJ_EVENT_GFX_SIDNEY"),
        ("grimsley", "GRIMSLEY", "E", False, "unova/grimsley.png", R + "unova/grimsly.png"),
    ],
    "STEEL": [
        ("jasmine", "JASMINE", "L", True, "johto/jasmine_front_pic.png", "people/johto/jasmine.png"),
        ("byron", "BYRON", "L", False, "sinnoh/byron.png", R + "sinnoh/byron.png"),
        ("molayne", "MOLAYNE", "L", False, "alola/molayne.png", R + "alola/molayne.png"),
        ("wikstrom", "WIKSTROM", "E", False, "kalos/wikstrom.png", R + "kalos/wikstrom.png"),
        ("poppy", "POPPY", "E", True, "paldea/poppy.png", R + "paldea/poppy.png"),
    ],
    "FAIRY": [
        ("valerie", "VALERIE", "L", True, "kalos/valerie.png", R + "kalos/valerie.png"),
        ("mina", "MINA", "L", True, "alola/mina.png", R + "alola/mina.png"),
        ("opal", "OPAL", "L", True, "galar/opal.png", R + "galar/opal.png"),
        ("bede", "BEDE", "L", False, "rival/bede.png", R + "rival/bede.png"),
    ],
    "CHAMPIONS": [
        ("blue", "BLUE", "C", False, "TRAINER_PIC_CHAMPION_RIVAL_FRLG", "OBJ_EVENT_GFX_BLUE"),
        ("red", "RED", "C", False, "TRAINER_PIC_RED", "OBJ_EVENT_GFX_RED"),
        ("lance_c", "LANCE", "C", False, "TRAINER_PIC_ELITE_FOUR_LANCE_FRLG", "OBJ_EVENT_GFX_LANCE"),
        ("steven", "STEVEN", "C", False, "TRAINER_PIC_STEVEN", "OBJ_EVENT_GFX_STEVEN"),
        ("wallace", "WALLACE", "C", False, "TRAINER_PIC_CHAMPION_WALLACE", "OBJ_EVENT_GFX_WALLACE"),
        ("cynthia", "CYNTHIA", "C", True, "sinnoh/cynthia.png", R + "sinnoh/cynthia.png"),
        ("alder", "ALDER", "C", False, "unova/alder.png", R + "unova/alder.png"),
        ("iris_c", "IRIS", "C", True, "unova/iris.png", R + "unova/iris.png"),
        ("diantha", "DIANTHA", "C", True, "kalos/diantha.png", R + "kalos/diantha.png"),
        ("kukui", "KUKUI", "C", False, "alola/kukui.png", R + "alola/kukui.png"),
        ("leon", "LEON", "C", False, "galar/leon.png", R + "galar/leon.png"),
        ("geeta", "GEETA", "C", True, "paldea/geeta.png", R + "paldea/geeta.png"),
        ("nemona", "NEMONA", "C", True, "rival/nemona.png", R + "rival/nemona.png"),
    ],
}


def all_trainers():
    """[(index, hall, key, name, role, female, pic, ow)] in convention order."""
    out = []
    for hall in HALLS:
        for entry in ROSTER[hall]:
            out.append((len(out), hall) + entry)
    return out


def sprite_key(entry_key):
    """Trainers who appear twice share their sprites."""
    return {"larry_e4": "larry", "iris_c": "iris_l", "lance_c": "lance"}.get(entry_key, entry_key)
