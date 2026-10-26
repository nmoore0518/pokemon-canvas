#!/usr/bin/env python3
"""When can the player first get each species and form? -> src/data/dex_form_avail.h

The Leader Academy's TEAM BUILDER only offers the Pokémon the player could get by now
(src/team_builder.c, TeamBuilder_IsAvailable). This works out, for every entry of gDexFormSpecies,
the earliest point in the game it can be had, from the same sources audit_obtainable.py walks
(wild tables, gifts, statics, trades, the Game Corner, the safari's legendaries, the gym legendaries)
and the same evolutions and form changes - each one only as early as the items it needs:

  AVAIL_START     the main game (Kanto: everything up to the League)
  AVAIL_CLEAR     after the Hall of Fame (Cerulean Cave)
  AVAIL_LEADER    once the gym is the player's (Johto, the Exotic Safari, the Convention)
  AVAIL_MASTERED  every gym type mastered (Terapagos, Arceus)
  AVAIL_NEVER     nowhere in the game (event-only forms...)
  | AVAIL_GYM_LEGEND   also needs that gym legendary (it comes to the gym of its type)

Usage: python3 tools/gym_hack/build_availability.py      (after `make firered`: it needs the headers)
"""
import contextlib
import io
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
with contextlib.redirect_stdout(io.StringIO()):
    import audit_obtainable as A  # noqa: E402  (the maps the player can reach, species, form changes)

R = A.R
START, CLEAR, LEADER, MASTERED, NEVER = 0, 1, 2, 3, 15
NAMES = {START: "START", CLEAR: "CLEAR", LEADER: "LEADER", MASTERED: "MASTERED", NEVER: "NEVER"}
JOHTO = set(json.loads((R / 'data/maps/map_groups.json').read_text()).get('gMapGroup_Johto', []))


def map_tier(folder):
    if folder.startswith('ExoticSafari_') or folder.endswith('_Johto') or folder in JOHTO:
        return LEADER
    if folder.startswith('CeruleanCave_'):
        return CLEAR
    return START


# a requirement: (tier, the gym legendary's type it needs or None); later/stricter compares greater
def worse(a, b):
    return (max(a[0], b[0]), a[1] or b[1])


def better(new, old):
    return old is None or (new[0], new[1] is not None) < (old[0], old[1] is not None)


# ---- items, with the earliest point each can be had ----------------------------------------
item_req = {}


def give_item(item, req):
    if item != 'ITEM_NONE' and better(req, item_req.get(item)):
        item_req[item] = req


PAT = r"(?:finditem|giveitem|additem|\.2byte|bufferitemname\w*|setvar VAR_0x8008,?)\s+(ITEM_\w+)"
for m in A.reach:
    t = (map_tier(m), False)
    p = R / f'data/maps/{m}/scripts.inc'
    if p.exists():
        for it in re.findall(PAT, p.read_text()):
            give_item(it, t)
    j = A.maps[m]
    for o in j.get('object_events', []):
        v = str(o.get('trainer_sight_or_berry_tree_id', ''))
        if v.startswith('ITEM_'):
            give_item(v, t)
    for b in j.get('bg_events', []):
        if b.get('type') == 'hidden_item':
            give_item(b['item'], t)
SHARED = {'gym_hack': START, 'players_house': START, 'leader_academy': START,
          'gym_finale': LEADER, 'gym_switch': LEADER, 'gym_convention': LEADER, 'gym_decor': LEADER,
          'gym_practice': LEADER, 'gym_aides': LEADER, 'gym_candidate': LEADER, 'gym_challenge': LEADER,
          'johto': LEADER, 'exotic_safari': LEADER}
for f, tier in SHARED.items():
    p = R / f'data/scripts/{f}.inc'
    if p.exists():
        for it in re.findall(PAT, p.read_text()):
            give_item(it, (tier, False))
for it in re.findall(r"ITEM_\w+", (R / 'src/data/exotic_safari_areas.h').read_text()):
    give_item(it, (LEADER, False))
for it in re.findall(r"ITEM_\w+_PLATE", (R / 'src/form_items.c').read_text()):
    give_item(it, (LEADER, False))      # (Arceus's plates, hidden in the gym)

# ---- sources -------------------------------------------------------------------------------
req = {}


def add(sp, r):
    sp = A.canon(sp)
    if sp in A.S and better(r, req.get(sp)):
        req[sp] = r
        return True
    return False


wj = json.loads((R / 'src/data/wild_encounters.json').read_text())
reach_ids = {A.maps[m]['id']: m for m in A.reach}
for g in wj['wild_encounter_groups']:
    for e in g.get('encounters', []):
        m = reach_ids.get(e.get('map'))
        if not m:
            continue
        for f in ('land_mons', 'water_mons', 'rock_smash_mons', 'fishing_mons'):
            for mon in e.get(f, {}).get('mons', []):
                add(mon['species'], (map_tier(m), False))
for pool in json.loads((R / 'src/data/exotic_safari_pools.json').read_text()):
    if pool['map'] in reach_ids:
        for sp in pool['species'][1:]:
            add(sp, (LEADER, False))
for fam in A.FAMILIES:          # (wild in any of its forms)
    best = min((req[A.canon(s)] for s in fam if A.canon(s) in req), default=None)
    if best:
        for s in fam:
            add(s, best)
for sp in A.S:
    if sp == 'SPECIES_UNOWN' or sp.startswith('SPECIES_UNOWN_'):
        add(sp, (LEADER, False))     # (the Ruins of Alph, in the safari's Johto zone)
for m in A.reach:
    p = R / f'data/maps/{m}/scripts.inc'
    if not p.exists():
        continue
    text = p.read_text()
    for k in ('givemon', 'giveegg', 'setwildbattle', 'createmon'):
        for sp in re.findall(k + r'\s+(?:\d+,\s*\d+,\s*)?(SPECIES_\w+)', text):
            add(sp, (map_tier(m), False))
    trade_src = (R / 'src/data/trade.h').read_text()
    for t in re.findall(r'(INGAME_TRADE_\w+)', text):
        blk = re.search(r'\[' + t + r'\]\s*=\s*\{(.*?)\n    \}', trade_src, re.S)
        if blk:
            for sp in re.findall(r'\.species = (SPECIES_\w+)', blk[1]):
                add(sp, (map_tier(m), False))
gc = (R / 'data/maps/CeladonCity_GameCorner_PrizeRoom_Frlg/scripts.inc').read_text()
for sp in re.findall(r'setvar VAR_TEMP_1, (SPECIES_\w+)', gc):
    add(sp, (START, False))
add('SPECIES_TYPE_NULL', (LEADER, False))      # (the Hano Beach caretaker's gift)
areas_h = (R / 'src/data/exotic_safari_areas.h').read_text()
for sp, seal in re.findall(r'MAP_NUM\(MAP_EXOTIC_SAFARI_\w+\), (SPECIES_\w+), FLAG_EXOTIC_LEGEND_\w+, \w+, (SEAL_\w+)', areas_h):
    tier = MASTERED if seal in ('SEAL_MASTERY', 'SEAL_AZURE_FLUTE') else LEADER
    if sp == 'SPECIES_ZYGARDE':
        for f in ('10_AURA_BREAK', '50', '10_POWER_CONSTRUCT', '50_POWER_CONSTRUCT'):
            add('SPECIES_ZYGARDE_' + f, (tier, False))
    else:
        add(sp, (tier, False))
for sp in re.findall(r'\{(SPECIES_\w+), FLAG_EXOTIC_LEGEND_\w+, FLAG_TEMP', areas_h):
    add(sp, (LEADER, False))
for typ, sp in re.findall(r'\[(TYPE_\w+)\]\s*=\s*(SPECIES_\w+),', (R / 'src/gym_legend.c').read_text().split('gGymLegends')[1].split('};')[0]):
    add(sp, (LEADER, typ))

# ---- evolutions, form changes and fusions, as early as their items allow ----------------------
OK_IF = A.OK_IF


def evo_req(src_r, meth, param, conds):
    r = src_r
    if meth in ('EVO_TRADE', 'EVO_NONE'):
        return None
    if meth == 'EVO_ITEM':
        if param not in item_req:
            return None
        r = worse(r, item_req[param])
    for n, a in conds:
        if n in ('IF_TIME', 'IF_NOT_TIME', 'IF_REGION', 'IF_IN_MAPSEC', 'IF_IN_MAP'):
            return None
        if n == 'IF_HOLD_ITEM':
            if a not in item_req:
                return None
            r = worse(r, item_req[a])
        elif n == 'IF_SPECIES_IN_PARTY':
            if A.canon(a) not in req:
                return None
            r = worse(r, req[A.canon(a)])
        elif n not in OK_IF:
            return None
    return r


changed = True
while changed:
    changed = False
    if 'SPECIES_FURFROU_NATURAL' in req:        # (the groomer, in the safari's Kalos zone)
        for t in A.S:
            if t.startswith('SPECIES_FURFROU_'):
                changed |= add(t, worse(req['SPECIES_FURFROU_NATURAL'], (LEADER, False)))
    for sp, r in list(req.items()):
        d = A.S[sp]
        for meth, param, tgt, conds in d['evos']:
            tgt = A.canon(tgt)
            if tgt not in A.S:
                continue
            nr = evo_req(r, meth, param, conds)
            if nr is not None:
                changed |= add(tgt, nr)
                for c in A.ALCREMIE_CREAMS.get(tgt, []):
                    changed |= add(c, nr)
        for e in A.FCT.get(d['fct'], []):
            if len(e) < 2:
                continue
            meth, tgt = e[0], A.canon(e[1])
            param = e[2] if len(e) > 2 else ''
            if tgt not in A.S or A.BATTLE(meth) or meth in ('FORM_CHANGE_FAINT', 'FORM_CHANGE_END_BATTLE'):
                continue
            nr = r
            if sp in A.CUSTOM:
                nr = worse(r, (LEADER, False))          # (Rotom's Room, in the safari)
            elif meth.startswith('FORM_CHANGE_ITEM') and param != 'ITEM_NONE':
                if param not in item_req:
                    continue
                nr = worse(r, item_req[param])
            changed |= add(tgt, nr)
    for ents in A.FUS.values():
        for e in ents:
            _, item, a, b, tgt = e[:5]
            a, b, tgt = A.canon(a), A.canon(b), A.canon(tgt)
            if a in req and b in req and item in item_req and tgt in A.S:
                changed |= add(tgt, worse(worse(req[a], req[b]), item_req[item]))

# ---- write --------------------------------------------------------------------------------
forms = re.findall(r'^\s+(SPECIES_\w+),', (R / 'src/data/dex_forms.h').read_text().split('gDexFormSpecies')[1].split('};')[0], re.M)
out = ["// Generated by tools/gym_hack/build_availability.py: for each entry of gDexFormSpecies, the",
       "// earliest point in the game the player can get it (AVAIL_*, include/constants/gym_hack.h)", "",
       "const u16 gDexFormAvail[DEX_FORM_COUNT] = {"]
count = {}
for sp in forms:
    r = req.get(A.canon(sp))
    v = NEVER if r is None else r[0]
    key = NAMES[v] + ('+LEGEND' if r and r[1] else '')
    count[key] = count.get(key, 0) + 1
    code = f"AVAIL_{NAMES[v]}" + (f" | AVAIL_GYM_LEGEND({r[1]})" if r and r[1] else "")
    out.append(f"    {code},  // {sp[8:]}")
out.append("};\n")
(R / 'src/data/dex_form_avail.h').write_text("\n".join(out))
print(f"{len(forms)} forms:", ", ".join(f"{k} {v}" for k, v in sorted(count.items())))
if '-v' in sys.argv:
    for sp in forms:
        r = req.get(A.canon(sp))
        print(sp[8:], NAMES[NEVER if r is None else r[0]] + (f'+{r[1]}' if r and r[1] else ''))
