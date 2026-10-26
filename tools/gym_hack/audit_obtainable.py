#!/usr/bin/env python3
"""Every Pokémon (and every form) in the game: can the player get it, and can they miss it for good?

Walks the maps the player can reach (warps, connections and script warps from Pallet Town and the
Olivine ferry: the Sevii Islands aren't reachable), then gathers every way to get a Pokémon - the
wild tables of those maps, gifts and static battles in their scripts, Game Corner prizes, in-game
trades, the Exotic Safari's legendaries (static and roaming), the gym legendaries, the starters,
the Red Gyarados, the Ruins of Alph's Unown - and follows evolutions (with the items the player can
get: marts, item balls, hidden items, gifts, the safari kiosk) and form changes (items, fusions,
Rotom's Room, the Furfrou groomer, Zygarde's cells). Lists what's left, by kind, and which
Pokémon only come from one-time sources (gifts, trades, the starter, statics that go for good).

Usage: python3 tools/gym_hack/audit_obtainable.py     (after `make firered`: it needs the headers)
"""
import glob
import json
import re
import subprocess
import tempfile
from pathlib import Path

R = Path(__file__).resolve().parents[2]


def preprocess(header, *extra):
    """the C preprocessor's view of a data header (the config's #ifs resolved)"""
    with tempfile.TemporaryDirectory() as d:
        if header is None:      # (the form-change and fusion tables)
            c = Path(d) / "fct.c"
            c.write_text('#include "global.h"\n#include "constants/form_change_types.h"\n#include "constants/items.h"\n'
                         '#include "constants/moves.h"\n#include "data/pokemon/form_change_tables.h"\n')
            header = str(c)
        out = Path(d) / "out.i"
        subprocess.run(["arm-none-eabi-gcc", "-E", "-P", "-w", "-iquote", "include", "-iquote", "src", "-I", "include",
                        "-I", "gflib", "-DMODERN=1", "-DTESTING=0", "-DFIRERED", "-std=gnu17", *extra, "-x", "c",
                        header, "-o", str(out)], cwd=R, check=True)
        return out.read_text()



groups=json.load(open(R/'data/maps/map_groups.json'))
name_of={}; maps={}
for g in groups['group_order']:
    for m in groups[g]:
        j=json.load(open(R/f'data/maps/{m}/map.json')); maps[m]=j; name_of[j['id']]=m
shared=''.join(open(p).read() for p in glob.glob(str(R/'data/scripts/*.inc')) if 'debug' not in p)
shared_warps=set(re.findall(r'\bwarp\w*\s+(MAP_\w+)',shared))
def outs(m):
    j=maps[m]; o=set()
    for w in j.get('warp_events',[]): o.add(w['dest_map'])
    for c in j.get('connections') or []: o.add(c['map'])
    p=R/f'data/maps/{m}/scripts.inc'
    if p.exists(): o|=set(re.findall(r'\bwarp\w*\s+(MAP_\w+)',p.read_text()))
    return o
start='MAP_PALLET_TOWN'
seen={start}; todo=[start]|set() if False else [start]
todo=[start]|set() if False else [start]
todo=[start,'MAP_OLIVINE_CITY_PORT_INSIDE']; seen=set(todo)
while todo:
    mid=todo.pop()
    m=name_of.get(mid)
    if not m: continue
    for d in outs(m):
        if d not in seen: seen.add(d); todo.append(d)
reach={name_of[x] for x in seen if x in name_of}

import json, re, sys
from pathlib import Path
from pathlib import Path
src = preprocess('src/data/pokemon/species_info.h', '-include', 'include/global.h')

# ---- species --------------------------------------------------------------------------
blocks = re.split(r"\[(SPECIES_[A-Z0-9_]+)\]\s*=", src)
S = {}
for i in range(1, len(blocks), 2):
    sp, b = blocks[i], blocks[i+1]
    if sp in S or sp in ('SPECIES_NONE', 'SPECIES_EGG', 'SPECIES_CLEFFABILL'): continue   # (Bill's hybrid: not obtainable, by design)
    n = re.search(r'\.speciesName\s*=\s*_\("(.*?)"\)', b)
    if not n: continue
    nd = re.search(r'\.natDexNum\s*=\s*(\w+)', b)
    flags = set(re.findall(r'\.(is\w+)\s*=\s*(?:TRUE|1)\b', b))
    fst = re.search(r'\.formSpeciesIdTable\s*=\s*(\w+)', b)
    fct = re.search(r'\.formChangeTable\s*=\s*(\w+)', b)
    evos = []
    e = re.search(r"\.evolutions = \(const struct Evolution\[\]\) \{(.*?)\{ 0xFFFF", b, re.S)
    if e:
        for m in re.finditer(r"\{(EVO_\w+), ([^,]+), (SPECIES_\w+)(.*?)\}(?=\s*,\s*\{|\s*,?\s*$)", e[1], re.S):
            conds = re.findall(r"\{(IF_\w+),\s*([^,}]+)", m[4])
            evos.append((m[1], m[2].strip(), m[3], conds))
    S[sp] = dict(name=n[1], nat=nd[1] if nd else None, flags=flags, fst=fst[1] if fst else None,
                 fct=fct[1] if fct else None, evos=evos)
enum = (R/'include/constants/species.h').read_text()
num = {s: int(v) for s, v in re.findall(r"^\s+(SPECIES_[A-Z0-9_]+)\s*=\s*(\d+)", enum, re.M)}
ALIAS = dict(re.findall(r"^\s+(SPECIES_[A-Z0-9_]+)\s*=\s*(SPECIES_[A-Z0-9_]+)", enum, re.M))
def canon(sp):
    while sp in ALIAS: sp = ALIAS[sp]
    return sp

# form-change tables
fsrc = preprocess(None)
FCT = {}
for m in re.finditer(r"static const struct FormChange (\w+)\[\]\s*=\s*\{(.*?)\};", fsrc, re.S):
    FCT[m[1]] = [tuple(x.strip() for x in e.split(',')) for e in re.findall(r"\{(FORM_CHANGE_[^}]*)\}", m[2])]
FUS = {}
for m in re.finditer(r"static const struct Fusion (\w+)\[\]\s*=\s*\{(.*?)\};", fsrc, re.S):
    FUS[m[1]] = [tuple(x.strip() for x in e.split(',')) for e in re.findall(r"\{(\d+, ITEM[^}]*)\}", m[2])]

# ---- items the player can get ---------------------------------------------------------
items = set()
def grab(text, pat=r"(?:finditem|giveitem|additem|\.2byte|bufferitemname\w*|setvar VAR_0x8008,?)\s+(ITEM_\w+)"):
    items.update(re.findall(pat, text))
for m in reach:
    p = R/f'data/maps/{m}/scripts.inc'
    if p.exists(): grab(p.read_text())
    j = maps[m]
    for o in j.get('object_events', []):
        v = str(o.get('trainer_sight_or_berry_tree_id', ''))
        if v.startswith('ITEM_'): items.add(v)
    for b in j.get('bg_events', []):
        if b.get('type') == 'hidden_item': items.add(b['item'])
for f in ['gym_hack', 'gym_finale', 'gym_switch', 'gym_convention', 'gym_decor', 'gym_practice', 'gym_aides',
          'gym_candidate', 'gym_challenge', 'johto', 'exotic_safari', 'item_ball_scripts_frlg', 'players_house']:
    p = R/f'data/scripts/{f}.inc'
    if p.exists(): grab(p.read_text())
items.update(re.findall(r"ITEM_\w+", (R/'src/data/exotic_safari_areas.h').read_text()))
items.update(re.findall(r"ITEM_\w+_PLATE", (R/'src/form_items.c').read_text()))   # (Arceus's plates, in the gym)
items.discard('ITEM_NONE')

# ---- sources ---------------------------------------------------------------------------
src_of = {}          # species -> [how]
def add(sp, how):
    sp = canon(sp)
    if sp in S: src_of.setdefault(sp, []).append(how)
wj = json.loads((R/'src/data/wild_encounters.json').read_text())
reach_ids = {maps[m]['id'] for m in reach}
for g in wj['wild_encounter_groups']:
    for e in g.get('encounters', []):
        if e.get('map') not in reach_ids: continue
        for f in ('land_mons', 'water_mons', 'rock_smash_mons', 'fishing_mons'):
            for mon in e.get(f, {}).get('mons', []):
                add(mon['species'], f"wild {e['map'][4:]}")
# shared slots (src/data/exotic_safari_pools.json): every species of the slot
for pool in json.loads((R/'src/data/exotic_safari_pools.json').read_text()):
    if pool['map'] in reach_ids:
        for sp in pool['species'][1:]: add(sp, f"wild {pool['map'][4:]} (shared slot)")
# families whose wild Pokémon come in any form (src/exotic_safari.c, sRandomFormFamilies)
fam_src = (R/'src/exotic_safari.c').read_text().split('sRandomFormFamilies')[0]
FAMILIES = []
for macro, stage in re.findall(r'\{(\w+)\((\w+)\)\}', fam_src):
    body = re.search(r'#define ' + macro + r'\(stage\)(.*?)SPECIES_NONE', fam_src, re.S)[1]
    FAMILIES.append([f'SPECIES_{stage}_{x}' for x in re.findall(r'SPECIES_##stage##_(\w+)', body)])
for lst in re.findall(r'static const u16 s\w+Forms\[\] = \{(SPECIES_[^}]*?)\};', fam_src, re.S):
    FAMILIES.append([s for s in re.findall(r'SPECIES_\w+', lst) if s != 'SPECIES_NONE'])
wild_now = {sp for sp, hs in src_of.items() if any(h.startswith('wild') for h in hs)}
for fam in FAMILIES:
    if any(canon(s) in wild_now for s in fam):
        for s in fam: add(s, 'wild (any of its forms)')
# the Ruins of Alph's panels: every Unown form
for sp in S:
    if sp == 'SPECIES_UNOWN' or sp.startswith('SPECIES_UNOWN_'):
        add(sp, 'wild RUINS OF ALPH (all four panels open)')
ONE_TIME = {}        # species -> description (a one-time, missable source)
for m in sorted(reach):
    p = R/f'data/maps/{m}/scripts.inc'
    if not p.exists(): continue
    for line in p.read_text().splitlines():
        for k in ('givemon', 'giveegg', 'setwildbattle', 'createmon'):
            mm = re.search(k + r'\s+(?:\d+,\s*\d+,\s*)?(SPECIES_\w+)', line)
            if mm: add(mm[1], f'{k} {m}')
gc = (R/'data/maps/CeladonCity_GameCorner_PrizeRoom_Frlg/scripts.inc').read_text()
for sp in re.findall(r'setvar VAR_TEMP_1, (SPECIES_\w+)', gc): add(sp, 'Game Corner prize')
add('SPECIES_TYPE_NULL', 'gift Hano Beach caretaker')
# in-game trades (FRLG's, at the reachable trade houses)
trade_src = (R/'src/data/trade.h').read_text()
for t in re.findall(r'\[(INGAME_TRADE_\w+)\]', trade_src):
    pass
for m in reach:
    p = R/f'data/maps/{m}/scripts.inc'
    if p.exists():
        for t in re.findall(r'(INGAME_TRADE_\w+)', p.read_text()):
            blk = re.search(r'\[' + t + r'\]\s*=\s*\{(.*?)\n    \}', trade_src, re.S)
            if blk:
                for sp in re.findall(r'\.species = (SPECIES_\w+)', blk[1]): add(sp, f'in-game trade {m}')
# safari statics and roamers, gym legends, the Red Gyarados
areas_h = (R/'src/data/exotic_safari_areas.h').read_text()
for sp in re.findall(r'MAP_NUM\(MAP_EXOTIC_SAFARI_\w+\), (SPECIES_\w+), FLAG_EXOTIC_LEGEND', areas_h):
    if sp == 'SPECIES_ZYGARDE':          # (ExoticSafari_LegendForm: 10% with 3-7 cells, 50% with 8-11, Power Construct with all 12)
        add('SPECIES_ZYGARDE_10_AURA_BREAK', 'safari legendary (static, 3-7 cells)')
        add('SPECIES_ZYGARDE_50', 'safari legendary (static, 8-11 cells)')
        add('SPECIES_ZYGARDE_50_POWER_CONSTRUCT', 'safari legendary (static, all 12 cells)')
        # (and once one is caught, wild in the Kalos zone now and then: ExoticSafari_WildSpecies)
        for f in ('10_AURA_BREAK', '50', '10_POWER_CONSTRUCT', '50_POWER_CONSTRUCT'):
            add('SPECIES_ZYGARDE_' + f, 'wild Kalos zone (rare, once Zygarde is caught; cells gate the forms)')
        continue
    if sp == 'SPECIES_KUBFU':
        add(sp, 'safari legendary (static; a second comes once every Galar area is visited again)')
        continue
    add(sp, 'safari legendary (static)')
for sp in re.findall(r'\{(SPECIES_\w+), FLAG_EXOTIC_LEGEND_\w+, FLAG_TEMP', areas_h): add(sp, 'safari legendary (roams)')
for sp in re.findall(r'\]\s*=\s*(SPECIES_\w+),', (R/'src/gym_legend.c').read_text().split('gGymLegends')[1].split('};')[0]):
    add(sp, 'gym legendary (its type)')
add('SPECIES_GYARADOS', 'Red Gyarados (Lake of Rage)')
# starters (one, for the player's type)
dex = (R/'tools/gym_hack/guide/build_dex.py').read_text()
STARTERS = re.findall(r'\{ (SPECIES_\w+),\s+TYPE_\w+\s*\}', (R/'src/gym_hack.c').read_text())
for sp in STARTERS: add(sp, 'starter (one, for the type picked)')

# the event Pokémon's side quests (src/event_mons.c): gifts
import event_mons_safari  # noqa: E402
for f in event_mons_safari.GIFT_FORMS:
    sp = 'SPECIES_' + f
    if sp in S or canon(sp) in S:
        src_of.setdefault(sp if sp in S else canon(sp), []).append('event quest (src/event_mons.c)')

# ---- closure: evolutions and form changes ------------------------------------------------
OK_IF = {"IF_MIN_FRIENDSHIP", "IF_GENDER", "IF_ATK_GT_DEF", "IF_ATK_EQ_DEF", "IF_ATK_LT_DEF", "IF_NOT_REGION",
         "IF_PID_UPPER_MODULO_10_GT", "IF_PID_UPPER_MODULO_10_EQ", "IF_PID_UPPER_MODULO_10_LT",
         "IF_PID_MODULO_100_GT", "IF_PID_MODULO_100_EQ", "IF_PID_MODULO_100_LT", "IF_BAG_ITEM_COUNT",
         "IF_KNOWS_MOVE", "IF_TYPE_IN_PARTY", "IF_SPECIES_IN_PARTY", "IF_HOLD_ITEM", "IF_MIN_BEAUTY"}
BATTLE = lambda meth: meth.startswith('FORM_CHANGE_BATTLE') or meth in ('FORM_CHANGE_BEGIN_BATTLE',)
have = {sp: list(h) for sp, h in src_of.items()}
blocked = {}
CUSTOM = {   # the hack's own form changers (no item in the bag)
    'SPECIES_ROTOM': 'Rotom\'s Room (Snowpoint City)',
}
def evo_ok(src, meth, param, conds):
    if meth == 'EVO_TRADE': return 'needs a trade'
    if meth == 'EVO_NONE': return 'no evolution method'
    if meth == 'EVO_ITEM' and param not in items: return f'{param} not obtainable'
    for n, a in conds:
        if n in ('IF_TIME', 'IF_NOT_TIME'): return f'time of day ({a})'
        if n == 'IF_REGION': return f'only in {a}'
        if n in ('IF_IN_MAPSEC', 'IF_IN_MAP'): return f'only at {a}'
        if n == 'IF_HOLD_ITEM' and a not in items: return f'hold {a} (not obtainable)'
        if n == 'IF_SPECIES_IN_PARTY' and a not in have: return f'{a} in party'
        if n not in OK_IF: return f'condition {n} {a}'
    return None
ALCREMIE_CREAMS = {}
for v in [s for s in S if s.startswith('SPECIES_ALCREMIE_') and s.endswith('_VANILLA_CREAM')]:
    sweet = v[len('SPECIES_ALCREMIE_'):-len('_VANILLA_CREAM')]
    ALCREMIE_CREAMS[v] = [s for s in S if s.startswith(f'SPECIES_ALCREMIE_{sweet}_') and not s.endswith('_GMAX')]
def custom_forms():
    out = []
    if 'SPECIES_FURFROU_NATURAL' in have:
        out += [(t, 'the Furfrou groomer (Lumiose City)') for t in S if t.startswith('SPECIES_FURFROU_')]
    return out
changed = True
while changed:
    changed = False
    for t, how in custom_forms():
        if t not in have: have[t] = [how]; changed = True
    for sp in list(have):
        d = S[sp]
        for meth, param, tgt, conds in d['evos']:
            tgt = canon(tgt)
            if tgt in have or tgt not in S: continue
            why = evo_ok(sp, meth, param, conds)
            if why is None:
                have[tgt] = [f"evolve {d['name']} ({meth[4:].lower()} {param})"]; changed = True
                for c in ALCREMIE_CREAMS.get(tgt, []):      # (ExoticSafari_EvolvedForm: any of the nine creams)
                    if c not in have: have[c] = [f"evolve {d['name']} (its cream: from its personality)"]
            else:
                blocked.setdefault(tgt, set()).add(f"from {d['name']}: {why}")
        for e in FCT.get(d['fct'], []):
            meth, tgt = e[0], canon(e[1]) if len(e) > 1 else None
            if not tgt or tgt in have or tgt not in S: continue
            param = e[2] if len(e) > 2 else ''
            if BATTLE(meth) or meth in ('FORM_CHANGE_FAINT', 'FORM_CHANGE_END_BATTLE'):
                continue
            if meth in ('FORM_CHANGE_ITEM_USE', 'FORM_CHANGE_ITEM_USE_MULTICHOICE', 'FORM_CHANGE_ITEM_HOLD') and param not in items and param != 'ITEM_NONE':
                if sp in CUSTOM: pass
                else:
                    blocked.setdefault(tgt, set()).add(f"form of {d['name']}: {meth[12:].lower()} {param} (not obtainable)"); continue
            have[tgt] = [f"form change from {d['name']} ({meth[12:].lower()} {param})"]; changed = True
    for tab, ents in FUS.items():
        for e in ents:
            _, item, a, b, tgt = e[:5]
            a, b, tgt = canon(a), canon(b), canon(tgt)
            if tgt in have or tgt not in S: continue
            if a in have and b in have and item in items:
                have[tgt] = [f"fusion {S[a]['name']} + {S[b]['name']} ({item})"]; changed = True
            else:
                blocked.setdefault(tgt, set()).add(f"fusion needs {item}{'' if item in items else ' (not obtainable)'}")

# ---- what's missing ----------------------------------------------------------------------
battle_tgt = set()
for ents in FCT.values():
    for e in ents:
        if len(e) > 1 and BATTLE(e[0]): battle_tgt.add(e[1])
nonbattle_tgt = set()
for ents in FCT.values():
    for e in ents:
        if len(e) > 1 and not BATTLE(e[0]) and e[0] not in ('FORM_CHANGE_FAINT', 'FORM_CHANGE_END_BATTLE'): nonbattle_tgt.add(e[1])
evo_tgt = {t for d in S.values() for _, _, t, _ in d['evos']}
missing = [s for s in S if s not in have]
def kind(s):
    f = S[s]['flags']
    if f & {'isMegaEvolution', 'isGigantamax', 'isPrimalReversion', 'isUltraBurst', 'isTeraForm'}: return 'battle'
    if 'isTotem' in f: return 'totem'
    if s in battle_tgt and s not in nonbattle_tgt and s not in evo_tgt: return 'battle'
    return 'storable'

# ---- lockouts: what's left if every one-time source (gifts, trades, the starter, the Kanto
# statics that go for good once knocked out) is missed --------------------------------------
REPEAT = ('wild', 'safari legendary', 'gym legendary', 'Red Gyarados', 'Game Corner')
def closure(seed):
    h = set(seed); ch = True
    while ch:
        ch = False
        if 'SPECIES_FURFROU_NATURAL' in h:
            for t in S:
                if t.startswith('SPECIES_FURFROU_') and t not in h: h.add(t); ch = True
        for sp in list(h):
            d = S[sp]
            for meth, param, tgt, conds in d['evos']:
                tgt = canon(tgt)
                if tgt not in h and tgt in S and evo_ok(sp, meth, param, conds) is None:
                    h.add(tgt); h.update(ALCREMIE_CREAMS.get(tgt, [])); ch = True
            for e in FCT.get(d['fct'], []):
                if len(e) < 2: continue
                meth, tgt = e[0], canon(e[1]); param = e[2] if len(e) > 2 else ''
                if tgt in h or tgt not in S or BATTLE(meth) or meth in ('FORM_CHANGE_FAINT', 'FORM_CHANGE_END_BATTLE'): continue
                if meth.startswith('FORM_CHANGE_ITEM') and param not in items and param != 'ITEM_NONE' and sp not in CUSTOM: continue
                h.add(tgt); ch = True
        for ents in FUS.values():
            for e in ents:
                _, item, a, b, tgt = e[:5]; a, b, tgt = canon(a), canon(b), canon(tgt)
                if tgt not in h and a in h and b in h and item in items: h.add(tgt); ch = True
    return h
repeatable = closure({sp for sp, hs in src_of.items() if any(x.startswith(REPEAT) for x in hs)})
lockable = sorted((s for s in have if s not in repeatable), key=lambda s: num.get(s, 0))

from collections import defaultdict
missing = [s for s in S if s not in have and kind(s) == 'storable']
EVENT = {'SPECIES_PIKACHU_COSPLAY','SPECIES_PIKACHU_ROCK_STAR','SPECIES_PIKACHU_BELLE','SPECIES_PIKACHU_POP_STAR','SPECIES_PIKACHU_PHD','SPECIES_PIKACHU_LIBRE',
 'SPECIES_PIKACHU_ORIGINAL','SPECIES_PIKACHU_HOENN','SPECIES_PIKACHU_SINNOH','SPECIES_PIKACHU_UNOVA','SPECIES_PIKACHU_KALOS','SPECIES_PIKACHU_ALOLA',
 'SPECIES_PIKACHU_PARTNER','SPECIES_PIKACHU_WORLD','SPECIES_PIKACHU_STARTER','SPECIES_EEVEE_STARTER','SPECIES_PICHU_SPIKY_EARED','SPECIES_FLOETTE_ETERNAL',
 'SPECIES_MAGEARNA_ORIGINAL','SPECIES_ZARUDE_DADA','SPECIES_GRENINJA_BATTLE_BOND','SPECIES_ROCKRUFF_OWN_TEMPO','SPECIES_ETERNATUS_ETERNAMAX',
 'SPECIES_VIVILLON_FANCY','SPECIES_VIVILLON_POKEBALL','SPECIES_SCATTERBUG_FANCY','SPECIES_SCATTERBUG_POKEBALL','SPECIES_SPEWPA_FANCY','SPECIES_SPEWPA_POKEBALL'}
by_name = defaultdict(list)
for s in S: by_name[S[s]['name']].append(s)
def form(s):
    base = min(by_name[S[s]['name']], key=lambda x: num.get(x, 99999))
    if s == base: return S[s]['name']
    suf = s[len('SPECIES_'):]
    nm = S[s]['name'].upper().replace(' ', '_').replace('-', '_').replace('.', '').replace("'", '').replace('É','E').replace(':','')
    suf = suf[len(nm)+1:] if suf.startswith(nm) else suf
    return f"{S[s]['name']} ({suf.replace('_', ' ').title()})"
# whole species: no form of the name obtainable
groups = defaultdict(list)
for s in sorted(missing, key=lambda s: num.get(s, 0)):
    nm = S[s]['name']
    if s in EVENT: groups['event'].append(s); continue
    if s in evo_tgt and blocked.get(s):
        groups['evo'].append(s)
    elif not any(x in have for x in by_name[nm]):
        groups['species'].append(s)
    else:
        groups['form'].append(s)
def nat(s): return re.sub(r'NATIONAL_DEX_', '', S[s]['nat'] or '')
print('NOT IN THE GAME (no form obtainable):', len({S[s]['name'] for s in groups['species']}))
seen=set()
for s in groups['species']:
    nm=S[s]['name']
    if nm in seen: continue
    seen.add(nm)
    forms=[form(x) for x in by_name[nm] if x in groups['species'] and x!=s]
    why=sorted(blocked.get(s,[]))
    print(f"  {nm}{' + '+', '.join(forms) if forms else ''}{'  ['+'; '.join(why)+']' if why else ''}")
print('\nEVOLUTION BLOCKED (its pre-evolution is obtainable):')
for s in groups['evo']: print(f"  {form(s)}: {'; '.join(sorted(blocked[s]))}")
print('\nFORMS MISSING (the species itself is obtainable):')
fm=defaultdict(list)
for s in groups['form']: fm[S[s]['name']].append(s)
for nm, l in fm.items():
    reasons=sorted({r for s in l for r in blocked.get(s,[])})
    print(f"  {nm}: {', '.join(form(s).split('(')[-1].rstrip(')') if '(' in form(s) else form(s) for s in l)}{'  ['+'; '.join(reasons[:3])+(' ...' if len(reasons)>3 else '')+']' if reasons else ''}")
print('\nEVENT-ONLY FORMS (never in the wild in the official games either):')
print('  ' + ', '.join(form(s) for s in groups['event']))
bat=[s for s in S if s not in have and kind(s)=='battle']; tot=[s for s in S if s not in have and kind(s)=='totem']
print(f'\nBATTLE-ONLY ({len(bat)}: megas, Gigantamax, primal, Zen Mode, etc.) and TOTEM ({len(tot)}) forms: not counted')
print(f'\nTotal entries: {len(S)}; obtainable: {len(have)}; storable missing: {len(missing)}')

print('\nONE-TIME CHOICES: none (a second Kubfu comes to the pitch; Zygarde turns up wild in Kalos once caught, and the cube changes its forms)')
print('\nONLY FROM ONE-TIME SOURCES (gifts, trades, the starter, statics that go for good):')
for s in lockable: print(f"  {form(s)}: {'; '.join(src_of.get(s, have[s]))}")
