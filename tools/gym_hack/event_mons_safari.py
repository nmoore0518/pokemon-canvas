"""The event Pokémon side quests' pieces in the Exotic Safari (Pokémon Canvas; src/event_mons.c,
data/scripts/event_mons.inc). build_exotic_safari.py calls patch() on each region module before
building it, and extra() on each area after the region's own extras.

  - statics (SEAL_PARTY: sPartySeals in src/exotic_safari.c, by index): Spiky-eared Pichu at the Ilex
    shrine, Zarude (Dada) in Glimwood's glade, Eternamax on the stadium's pitch
  - people: the Vivillon collector (Kalos city), the Battle Bond trainer (Kalos ocean), AZ (Kalos
    badlands), Tala and the artisan (Alola resort), the boy in Glimwood, the pitch's groundskeeper
  - the artisan's four antique paint pots (one in each of four Alola areas; flags from
    FLAG_EVENT_PAINT_POT_FIRST)
"""
# map -> [(species, (x, y), its sPartySeals index)]
STATICS = {
    "ExoticSafari_JohtoIlexForest": [("PICHU_SPIKY_EARED", (33, 37), 0)],
    "ExoticSafari_GalarForest": [("ZARUDE_DADA", (30, 2), 1)],
    "ExoticSafari_GalarStadiumPitch": [("ETERNATUS_ETERNAMAX", (10, 4), 2)],
}
# map -> [(gfx, (x, y), script, movement)]
PEOPLE = {
    "ExoticSafari_KalosCity": [("OBJ_EVENT_GFX_BUG_CATCHER_FRLG", (36, 35), "EventMons_EventScript_VivillonCollector", "MOVEMENT_TYPE_FACE_DOWN")],
    "ExoticSafari_KalosOcean": [("OBJ_EVENT_GFX_YOUNGSTER_FRLG", (14, 9), "EventMons_EventScript_BondTrainer", "MOVEMENT_TYPE_FACE_DOWN")],
    "ExoticSafari_KalosBadlands": [("OBJ_EVENT_GFX_OLD_MAN_1", (33, 3), "EventMons_EventScript_Az", "MOVEMENT_TYPE_FACE_DOWN")],
    "ExoticSafari_AlolaResort": [("OBJ_EVENT_GFX_PICNICKER_FRLG", (23, 14), "EventMons_EventScript_Tala", "MOVEMENT_TYPE_FACE_DOWN"),
                                 ("OBJ_EVENT_GFX_OLD_WOMAN_FRLG", (30, 9), "EventMons_EventScript_Artisan", "MOVEMENT_TYPE_FACE_DOWN")],
    "ExoticSafari_GalarForest": [("OBJ_EVENT_GFX_LITTLE_BOY_FRLG", (37, 3), "EventMons_EventScript_GlimwoodBoy", "MOVEMENT_TYPE_FACE_LEFT")],
    "ExoticSafari_GalarStadiumPitch": [("OBJ_EVENT_GFX_WORKER_M", (3, 8), "EventMons_EventScript_Groundskeeper", "MOVEMENT_TYPE_FACE_DOWN")],
}
# the paint pots: one in each of these areas (spots picked like Zygarde's Cells: out of the way)
POT_MAPS = ["ExoticSafari_AlolaBeach", "ExoticSafari_AlolaRuins", "ExoticSafari_AlolaVolcano", "ExoticSafari_AlolaLanakila"]
POT_FLAG_FIRST = 0x4ED


def patch(mod):
    """add the statics, and what they wait for, to a region module"""
    for area in mod.AREAS.values():
        for species, pos, seal in STATICS.get(area["map"], []):
            if all(s != species for s, _ in area["statics"]):
                area["statics"].append((species, pos))
            if not hasattr(mod, "CONDITIONS"):
                mod.CONDITIONS = {}
            mod.CONDITIONS[species] = ("PARTY", seal)


def extra(ctx):
    name = ctx["name"]
    for gfx, (x, y), script, movement in PEOPLE.get(name, []):
        if (x, y) not in ctx["reach"]:
            print(f"WARNING {name}: {script} at {(x, y)} can't be reached")
        o = ctx["obj"](gfx, x, y, script, movement)
        o["elevation"] = ctx["elev"](x, y)
        ctx["objects"].append(o)
    if name in POT_MAPS:
        import kalos
        k = POT_MAPS.index(name)
        taken = {(o["x"], o["y"]) for o in ctx["objects"]}
        (x, y), = kalos._pick_spots(ctx, 1, "paint pot " + name, taken)
        label = f"{name}_EventScript_PaintPot"
        o = ctx["obj"]("OBJ_EVENT_GFX_ITEM_BALL", x, y, label, "MOVEMENT_TYPE_FACE_DOWN", f"0x{POT_FLAG_FIRST + k:03X}")
        o["elevation"] = ctx["elev"](x, y)
        ctx["objects"].append(o)
        ctx["scripts"] += [f"{label}::", "\tlock", "\tgoto EventMons_EventScript_PaintPot", ""]


# Every form the side quests give (gifts; the statics are in the safari's own table): for
# audit_obtainable.py and build_availability.py
GIFT_FORMS = (["PIKACHU_" + c for c in ("COSPLAY", "ROCK_STAR", "BELLE", "POP_STAR", "PHD", "LIBRE", "ORIGINAL", "HOENN",
                                        "SINNOH", "UNOVA", "KALOS", "ALOLA", "WORLD", "PARTNER", "STARTER")]
              + ["EEVEE_STARTER", "GRENINJA_BATTLE_BOND", "FLOETTE_ETERNAL", "ROCKRUFF_OWN_TEMPO", "MAGEARNA_ORIGINAL"]
              + [f"{s}_{p}" for s in ("SCATTERBUG", "SPEWPA", "VIVILLON") for p in ("FANCY", "POKEBALL")])
