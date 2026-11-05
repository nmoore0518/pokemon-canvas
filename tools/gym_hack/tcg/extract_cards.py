#!/usr/bin/env python3
"""Pokémon Canvas: the Pokémon Trading Card Game duel (src/tcg/): its cards, text and decks, from the
pret/poketcg disassembly of the Game Boy Color game.

    python3 tools/gym_hack/tcg/extract_cards.py <path to a pret/poketcg checkout>

Only the cards in the decks below (Grandmaster Rod's and the player's seven) are taken. Writes:
  src/data/tcg/cards.h            gTcgCards[]: each card's data, in the game's own words
  src/data/tcg/effect_commands.h  each attack's (and Trainer's) effect commands: the routines the duel
                                  runs at each step, named as in the game (src/tcg/effects.c)
  src/data/tcg/decks.h            the decks
  include/constants/tcg_cards.h   TCG_CARD_* (our numbering) and the effect routines' list
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from tcg_charmap import Charmaps, c_bytes, text_blocks  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
SRC = Path(sys.argv[1]) / "src"

DECKS = {
    # Grandmaster Rod's
    "ROD": "LegendaryDragoniteDeck",
    # the player's: the club masters' decks, one per type, and a Colorless one of our own
    "GRASS": "FlowerPowerDeck",
    "FIRE": "FireChargeDeck",
    "WATER": "GoGoRainDanceDeck",
    "LIGHTNING": "ZappingSelfdestructDeck",
    "PSYCHIC": "StrangePsyshockDeck",
    "FIGHTING": "FirstStrikeDeck",
}
COLORLESS_DECK = [  # (60: Pidgeot's, Raticate's and Persian's lines and strong Basics; Rod's energy mix)
    ("WATER_ENERGY", 17), ("DOUBLE_COLORLESS_ENERGY", 4),
    ("PIDGEY", 4), ("PIDGEOTTO", 3), ("PIDGEOT_LV38", 2), ("RATTATA", 3), ("RATICATE", 2),
    ("MEOWTH_LV15", 3), ("PERSIAN", 2), ("SNORLAX", 2), ("CHANSEY", 2), ("KANGASKHAN", 2), ("TAUROS", 2),
    ("BILL", 3), ("PROFESSOR_OAK", 2), ("PLUSPOWER", 2), ("POTION", 2), ("SWITCH", 2), ("ENERGY_RETRIEVAL", 1),
]
TYPES = ["FIRE", "GRASS", "LIGHTNING", "WATER", "FIGHTING", "PSYCHIC", "COLORLESS"]


def deck_lists():
    d = (SRC / "data/decks.asm").read_text()
    out = {}
    for key, label in DECKS.items():
        body = d[d.index(label + ":"):]
        body = body[:body.index("deck_list_end")]
        out[key] = [(c, int(n)) for c, n in re.findall(r"card_item (\w+),\s*(\d+)", body)]
    out["COLORLESS"] = COLORLESS_DECK
    for key, cards in out.items():
        assert sum(n for _, n in cards) == 60, (key, sum(n for _, n in cards))
    return out


def texts():
    """{label: the text as a C string literal of the game's bytes} (tcg_charmap.py)"""
    cm = Charmaps(SRC)
    return {k: c_bytes(cm.encode_text_block(v)) for k, v in text_blocks(SRC).items()}


def card_blocks():
    c = (SRC / "data/cards.asm").read_text()
    out = {}
    for b in re.split(r"\n(?=\w+Card:\n)", c):
        m = re.match(r"(\w+)Card:\n", b)
        if not m:
            continue
        lines = [l.strip() for l in b.split("\n")[1:]]
        for i, l in enumerate(lines):
            if "; sets" in l:
                out[lines[i + 1].split()[1]] = (m[1], lines)
                break
    return out


def parse(label, lines):
    """the card's fields, by the data's own comments, in order"""
    it = iter([l for l in lines if l and not l.startswith(";")])
    card = {"label": label}
    first = next(it)
    card["type"] = first.split()[1]
    card["gfx"] = next(it).split()[1]
    card["name"] = next(it).split()[1]
    next(it)                                            # rarity
    next(it)                                            # sets
    card["id"] = next(it).split()[1]
    if card["type"].startswith("TYPE_PKMN"):
        card["hp"] = int(next(it).split()[1])
        card["stage"] = next(it).split()[1]
        pre = next(it).split()[1]
        card["preEvo"] = None if pre == "NONE" else pre
        card["attacks"] = []
        for _ in range(2):
            e = next(it)
            args = e.split(None, 1)[1].split(";")[0].strip()
            cost = [0] * 7
            parts = [p.strip() for p in args.split(",")] if args != "0" else []
            for k in range(0, len(parts) - 1, 2):
                cost[TYPES.index(parts[k])] += int(parts[k + 1])
            nm = next(it).split()[1]
            d1 = next(it).split()[1]
            d2 = next(it).split()[1]
            dmg = int(next(it).split()[1])
            cat = next(it).split(";")[0].split(None, 1)[1].strip()
            eff = next(it).split()[1]
            f1 = next(it).split(";")[0].split(None, 1)[1].strip()
            f2 = next(it).split(";")[0].split(None, 1)[1].strip()
            f3 = next(it).split(";")[0].split(None, 1)[1].strip()
            extra = next(it).split(";")[0].split(None, 1)[1].strip()
            anim = next(it).split()[1]
            card["attacks"].append(dict(cost=cost, name=None if nm == "NONE" else nm, desc=[d1, d2], damage=dmg,
                                        category=cat, effect=None if eff == "NONE" else eff, flags=[f1, f2, f3],
                                        extra=extra, anim=anim))
        card["retreat"] = int(next(it).split()[1])
        card["weakness"] = next(it).split(";")[0].split(None, 1)[1].strip()
        card["resistance"] = next(it).split(";")[0].split(None, 1)[1].strip()
        card["category"] = next(it).split()[1]
        card["dex"] = int(next(it).split()[1])
        next(it)
        card["level"] = int(next(it).split()[1])
        card["length"] = [int(x) for x in next(it).split(None, 1)[1].split(";")[0].split(",")]
        card["weight"] = next(it).split(None, 1)[1].split(";")[0].strip()
        card["desc"] = next(it).split()[1]
        card["aiInfo"] = next(it).split(None, 1)[1].split(";")[0].strip()
    else:
        eff = next(it).split()[1]
        card["effect"] = None if eff == "NONE" else eff
        card["desc"] = next(it).split()[1]
        card["desc2"] = next(it).split()[1]
    return card


def cstr(s):
    if s is None:
        return "NULL"
    s = s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    s = s.replace("é", "\\x80").replace("♂", "\\x81").replace("♀", "\\x82").replace("”", '\\"').replace("“", '\\"')
    return f'"{s}"'


def wr(v):
    if v in ("NONE", "0"):
        return "0"
    return " | ".join("TCG_" + p.strip() for p in v.split("|"))


def main():
    decks = deck_lists()
    used = []
    for cards in decks.values():
        for c, _ in cards:
            if c not in used:
                used.append(c)
    blocks = card_blocks()
    cards = [parse(*blocks[c]) for c in used]
    # (in the game's own order, energies first: it's the order of its card list)
    order = list(blocks)
    cards.sort(key=lambda c: order.index(c["id"]))
    tx = texts()
    need = set()
    for c in cards:
        need |= {c["name"], c["desc"]}
        need |= {c.get("preEvo"), c.get("category"), c.get("desc2")}
        for a in c.get("attacks", []):
            need |= {a["name"], *a["desc"]}
    need -= {None, "NONE"}
    missing = [n for n in need if n not in tx]
    assert not missing, missing

    # ---- constants
    h = ["// Generated by tools/gym_hack/tcg/extract_cards.py (from pret/poketcg): the Trading Card Game's",
         "// cards in Pokémon Canvas's duel (src/tcg/)", "#ifndef GUARD_CONSTANTS_TCG_CARDS_H",
         "#define GUARD_CONSTANTS_TCG_CARDS_H", "", "enum TcgCardId", "{", "    TCG_CARD_NONE,"]
    h += [f"    TCG_CARD_{c['id']}," for c in cards]
    h += ["    TCG_NUM_CARDS,", "};", ""]
    gfx = []
    for c in cards:
        g = "TCG_GFX_" + c["gfx"].replace("CardGfx", "").upper()
        if g not in gfx:
            gfx.append(g)
    h += ["enum TcgGfxId", "{"] + [f"    {g}," for g in gfx] + ["    TCG_NUM_GFX,", "};", ""]
    h += ["enum TcgDeckId", "{"] + [f"    TCG_DECK_{k}," for k in decks] + ["    TCG_NUM_DUEL_DECKS,", "};", ""]
    # effect commands: labels and the routines they name
    ec = (SRC / "engine/duel/effect_commands.asm").read_text()
    labels = []
    for c in cards:
        for a in c.get("attacks", []):
            if a["effect"] and a["effect"] not in labels:
                labels.append(a["effect"])
        if c.get("effect") and c["effect"] not in labels:
            labels.append(c["effect"])
    cmds, funcs = {}, []
    for lab in labels:
        m = re.search(r"^" + lab + r":\n((?:\s*dbw.*\n)+)", ec, re.M)
        cmds[lab] = re.findall(r"dbw (EFFECTCMDTYPE_\w+), (\w+)", m[1]) if m else []
        for _, f in cmds[lab]:
            if f not in funcs:
                funcs.append(f)
    h += ["enum TcgEffectCommandsId", "{", "    TCG_EFFECTS_NONE,"] + [f"    TCG_EFFECTS_{l}," for l in labels]
    h += ["    TCG_NUM_EFFECTS,", "};", "", "#endif // GUARD_CONSTANTS_TCG_CARDS_H", ""]
    (ROOT / "include/constants/tcg_cards.h").write_text("\n".join(h))

    # ---- cards
    out = ["// Generated by tools/gym_hack/tcg/extract_cards.py (from pret/poketcg): the cards, in the",
           "// Trading Card Game's own words, in its own encoding (tools/gym_hack/tcg/tcg_charmap.py).)", ""]
    texts_used = sorted(need)
    for n in texts_used:
        out.append(f"static const u8 sTcgText_{n}[] = {tx[n]};")
    out += ["", "const struct TcgCard gTcgCards[TCG_NUM_CARDS] =", "{"]
    T = lambda n: "NULL" if n is None or n == "NONE" else f"sTcgText_{n}"
    for c in cards:
        out.append(f"    [TCG_CARD_{c['id']}] =")
        out.append("    {")
        out.append(f"        .type = TCG_{c['type']},")
        out.append(f"        .name = {T(c['name'])},")
        out.append(f"        .description = {T(c['desc'])},")
        out.append(f"        .gfx = TCG_GFX_{c['gfx'].replace('CardGfx', '').upper()},")
        if c["type"].startswith("TYPE_PKMN"):
            out.append(f"        .hp = {c['hp']},")
            out.append(f"        .stage = TCG_{c['stage']},")
            out.append(f"        .preEvoName = {T(c['preEvo'])},")
            for k, a in enumerate(c["attacks"]):
                cost = ", ".join(str(v) for v in a["cost"])
                fl = ", ".join("0" if f == "NONE" else " | ".join("TCG_" + p.strip() for p in f.split("|")) for f in a["flags"])
                out.append(f"        .attacks[{k}] = {{")
                out.append(f"            .cost = {{{cost}}},")
                out.append(f"            .name = {T(a['name'])},")
                out.append(f"            .description = {{{T(a['desc'][0])}, {T(a['desc'][1])}}},")
                out.append(f"            .damage = {a['damage']},")
                cat = " | ".join("TCG_" + p.strip() for p in a["category"].split("|"))
                out.append(f"            .category = {cat},")
                out.append(f"            .effects = TCG_EFFECTS_{a['effect'] or 'NONE'},")
                out.append(f"            .flags = {{{fl}}},")
                ex = a["extra"]
                out.append(f"            .extra = {('TCG_' + ex) if re.match(r'[A-Z]', ex) else ex},")
                out.append(f"            .animation = TCG_{a['anim']},")
                out.append("        },")
            out.append(f"        .retreatCost = {c['retreat']},")
            out.append(f"        .weakness = {wr(c['weakness'])},")
            out.append(f"        .resistance = {wr(c['resistance'])},")
            out.append(f"        .category = {T(c['category'])},")
            out.append(f"        .dexNumber = {c['dex']},")
            out.append(f"        .level = {c['level']},")
            out.append(f"        .length = {{{c['length'][0]}, {c['length'][1]}}},")
            out.append(f"        .weight = {c['weight']},")
            out.append(f"        .aiInfo = {wr(c['aiInfo']) if c['aiInfo'] not in ('0', 'NONE') else 0},")
        else:
            out.append(f"        .effects = TCG_EFFECTS_{c['effect'] or 'NONE'},")
            out.append(f"        .description2 = {T(c['desc2'])},")
        out.append("    },")
    out += ["};", ""]
    (ROOT / "src/data/tcg").mkdir(parents=True, exist_ok=True)
    (ROOT / "src/data/tcg/cards.h").write_text("\n".join(out))

    # ---- effect commands
    e = ["// Generated by tools/gym_hack/tcg/extract_cards.py (from pret/poketcg's effect_commands.asm): what",
         "// each attack and Trainer card runs, when (src/tcg/effects.c has the routines)", ""]
    for lab in labels:
        e.append(f"static const struct TcgEffectCommand s{lab}[] =")
        e.append("{")
        for t, f in cmds[lab]:
            e.append(f"    {{TCG_{t}, Tcg_{f}}},")
        e.append("    {0},")
        e.append("};")
    e += ["", "const struct TcgEffectCommand *const gTcgEffectCommands[TCG_NUM_EFFECTS] =", "{",
          "    [TCG_EFFECTS_NONE] = NULL,"]
    e += [f"    [TCG_EFFECTS_{lab}] = s{lab}," for lab in labels]
    e += ["};", ""]
    (ROOT / "src/data/tcg/effect_commands.h").write_text("\n".join(e))
    pro = ["// Generated by tools/gym_hack/tcg/extract_cards.py: the effect routines the cards use (src/tcg/effects.c)",
           "#ifndef GUARD_TCG_EFFECT_FUNCTIONS_H", "#define GUARD_TCG_EFFECT_FUNCTIONS_H", ""]
    pro += [f"bool32 Tcg_{f}(void);" for f in funcs]
    pro += ["", "#endif", ""]
    (ROOT / "include/tcg_effect_functions.h").write_text("\n".join(pro))
    stubs = ["// Generated by tools/gym_hack/tcg/extract_cards.py: every effect routine, as a weak stub (it does",
             "// nothing) until src/tcg/effects.c ports it from the game: a ported one replaces its stub.",
             '#include "global.h"', '#include "tcg_effect_functions.h"', ""]
    stubs += [f"bool32 __attribute__((weak)) Tcg_{f}(void) {{ return FALSE; }}" for f in funcs]
    (ROOT / "src/tcg/effects_stubs.c").write_text("\n".join(stubs) + "\n")

    # ---- decks
    dk = ["// Generated by tools/gym_hack/tcg/extract_cards.py: the decks (60 cards each)", ""]
    for k, lst in decks.items():
        dk.append(f"static const u8 sTcgDeck_{k}[] =")
        dk.append("{")
        for c, n in lst:
            dk.append(f"    {', '.join(['TCG_CARD_' + c] * n)},")
        dk.append("};")
    dk += ["", "const u8 *const gTcgDecks[TCG_NUM_DUEL_DECKS] =", "{"] + [f"    [TCG_DECK_{k}] = sTcgDeck_{k}," for k in decks] + ["};", ""]
    (ROOT / "src/data/tcg/decks.h").write_text("\n".join(dk))
    print(f"{len(cards)} cards, {len(labels)} effect command lists, {len(funcs)} routines")


if __name__ == "__main__":
    main()
