// Pokémon Canvas: the pit under Celadon City. An underground ring where two fighters' Pokémon
// battle one on one, and the crowd bets on it.
//
// Betting on someone else's battle is against League rules (prize money between the two trainers
// is League business; a crowd wagering on the outcome is how fights get thrown), so it's down here,
// behind a door in a wall and a password (data/scripts/pit.inc; tools/gym_hack/build_pit.py made
// the maps).
//
// The card: two fighters from the roster, each with one Pokémon (a brawler of about the player's
// stage of the game), its level, ability, held item and moves - all on the card on the rim. The
// bookie's odds come from the card (a quick reckoning of who knocks out whom first, and who's
// faster), less his cut, and not quite right. Some fights are fixed: the favorite takes a dive -
// its trainer calls bad moves, or its item isn't the one on the card, or it isn't the level the
// card says. The tipster on the rim sells what he knows about the current card.
//
// The fight is a real battle, the two trainers' AI against each other (the battle engine's
// AI-vs-AI mode: IsAiVsAiBattle), the red corner's Pokémon standing in for the player's party for
// the length of it. It pays nothing and costs nothing but the bet.
#include "global.h"
#include "pit.h"
#include "battle.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "fieldmap.h"
#include "data.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "gym_challenge.h"
#include "item.h"
#include "money.h"
#include "move.h"
#include "naming_screen.h"
#include "overworld.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "script_menu.h"
#include "string_util.h"
#include "strings.h"
#include "constants/abilities.h"
#include "constants/battle_ai.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/pit.h"
#include "constants/trainers.h"
#include "malloc.h"
#include "list_menu.h"

#define FIX_CHANCE      25          // (percent of cards)
#define HOUSE_CUT       90          // (the bookie pays 90% of fair odds)
#define TIP_CUT         10          // (the tipster takes this percent of the player's bet on the bout)
#define MAX_BET         100000
#define UNDERDOG_MIN    15          // (the underdog's chance, by the card: never less than this)

enum { RED, BLUE, SIDES };
enum { PIT_FIX_NONE, PIT_FIX_DIVE, PIT_FIX_ITEM, PIT_FIX_LEVEL };

struct PitFighter
{
    const u8 *name;
    u8 trainerClass;
    u8 pic;
    u16 gfx;
    bool8 female;
};

static const struct PitFighter sFighters[] =
{
    {COMPOUND_STRING("KNUCKLES"), TRAINER_CLASS_BIKER_FRLG,      TRAINER_PIC_BIKER_FRLG,       OBJ_EVENT_GFX_BIKER,           FALSE},
    {COMPOUND_STRING("BIG LOU"),  TRAINER_CLASS_CUE_BALL_FRLG,   TRAINER_PIC_CUE_BALL_FRLG,    OBJ_EVENT_GFX_BALDING_MAN,     FALSE},
    {COMPOUND_STRING("SLIM"),     TRAINER_CLASS_BURGLAR_FRLG,    TRAINER_PIC_BURGLAR_FRLG,     OBJ_EVENT_GFX_MAN_4,           FALSE},
    {COMPOUND_STRING("DUTCH"),    TRAINER_CLASS_ROCKER_FRLG,     TRAINER_PIC_ROCKER_FRLG,      OBJ_EVENT_GFX_ROCKER,          FALSE},
    {COMPOUND_STRING("BONES"),    TRAINER_CLASS_BLACK_BELT_FRLG, TRAINER_PIC_BLACK_BELT_FRLG,  OBJ_EVENT_GFX_BLACK_BELT_FRLG,  FALSE},
    {COMPOUND_STRING("MAGDA"),    TRAINER_CLASS_CRUSH_GIRL_FRLG, TRAINER_PIC_CRUSH_GIRL_FRLG,  OBJ_EVENT_GFX_CRUSH_GIRL,      TRUE},
    {COMPOUND_STRING("SAL"),      TRAINER_CLASS_SAILOR_FRLG,     TRAINER_PIC_SAILOR_FRLG,      OBJ_EVENT_GFX_SAILOR_FRLG,     FALSE},
    {COMPOUND_STRING("TINY"),     TRAINER_CLASS_HIKER_FRLG,      TRAINER_PIC_HIKER_FRLG,       OBJ_EVENT_GFX_HIKER_FRLG,      FALSE},
    {COMPOUND_STRING("VERA"),     TRAINER_CLASS_BEAUTY_FRLG,     TRAINER_PIC_BEAUTY_FRLG,      OBJ_EVENT_GFX_BEAUTY_FRLG,     TRUE},
    {COMPOUND_STRING("RAZOR"),    TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_M_FRLG, OBJ_EVENT_GFX_COOLTRAINER_M, FALSE},
    {COMPOUND_STRING("ROXIE"),    TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_F_FRLG, OBJ_EVENT_GFX_COOLTRAINER_F, TRUE},
    {COMPOUND_STRING("MONTY"),    TRAINER_CLASS_GAMER_FRLG,      TRAINER_PIC_GAMER_FRLG,       OBJ_EVENT_GFX_MAN_2,           FALSE},
    {COMPOUND_STRING("SCRAPS"),   TRAINER_CLASS_ENGINEER_FRLG,   TRAINER_PIC_ENGINEER_FRLG,    OBJ_EVENT_GFX_WORKER_M,        FALSE},
    {COMPOUND_STRING("HOOK"),     TRAINER_CLASS_FISHERMAN_FRLG,  TRAINER_PIC_FISHERMAN_FRLG,   OBJ_EVENT_GFX_FISHER,          FALSE},
};

// Brawlers
static const u16 sPitSpecies[] =
{
    SPECIES_MACHAMP, SPECIES_PRIMEAPE, SPECIES_HITMONLEE, SPECIES_HITMONCHAN, SPECIES_HITMONTOP,
    SPECIES_POLIWRATH, SPECIES_HERACROSS, SPECIES_HARIYAMA, SPECIES_BRELOOM, SPECIES_SCYTHER,
    SPECIES_PINSIR, SPECIES_KANGASKHAN, SPECIES_TAUROS, SPECIES_URSARING, SPECIES_GRANBULL,
    SPECIES_ARCANINE, SPECIES_HOUNDOOM, SPECIES_NIDOKING, SPECIES_NIDOQUEEN, SPECIES_RHYDON,
    SPECIES_GOLEM, SPECIES_MAROWAK, SPECIES_SANDSLASH, SPECIES_DUGTRIO, SPECIES_GYARADOS,
    SPECIES_CRAWDAUNT, SPECIES_FERALIGATR, SPECIES_MIGHTYENA, SPECIES_ZANGOOSE, SPECIES_SEVIPER,
    SPECIES_ABSOL, SPECIES_SKARMORY, SPECIES_DONPHAN, SPECIES_PERSIAN, SPECIES_RATICATE,
    SPECIES_FEAROW, SPECIES_DODRIO, SPECIES_ELECTABUZZ, SPECIES_MAGMAR, SPECIES_JOLTEON,
    SPECIES_FLAREON, SPECIES_SNEASEL, SPECIES_GLIGAR, SPECIES_MUK, SPECIES_ARBOK,
    SPECIES_GRUMPIG, SPECIES_CACTURNE, SPECIES_SHARPEDO, SPECIES_LUDICOLO, SPECIES_MEDICHAM,
};

static const u16 sPitItems[] =
{
    ITEM_LEFTOVERS, ITEM_LIFE_ORB, ITEM_CHOICE_BAND, ITEM_FOCUS_SASH, ITEM_SITRUS_BERRY,
    ITEM_MUSCLE_BAND, ITEM_EXPERT_BELT, ITEM_SHELL_BELL, ITEM_QUICK_CLAW, ITEM_SCOPE_LENS,
    ITEM_ROCKY_HELMET, ITEM_BLACK_BELT, ITEM_KINGS_ROCK, ITEM_SILK_SCARF,
};

// (what a fixer swaps in: dead weight)
static const u16 sFixItems[] = {ITEM_IRON_BALL, ITEM_LAGGING_TAIL, ITEM_TOXIC_ORB, ITEM_FLAME_ORB, ITEM_STICKY_BARB};

struct PitSide
{
    u8 fighter;
    u8 level;                   // (as on the card)
    u8 abilityNum;
    u8 nature;
    u16 species;
    u16 item;                   // (as on the card)
    u16 moves[MAX_MON_MOVES];
    u16 odds;                   // what the bookie pays, x10 (14: x1.4)
};

struct PitCard
{
    struct PitSide side[SIDES];
    u8 fix;
    u8 fixSide;
    u16 fixItem;
    bool8 ready;
    bool8 tipBought;
    bool8 tipPaid;              // (his cut's been paid: on a bet placed before the tip)
    u8 betSide;                 // (SIDES: no bet)
    u32 bet;
};

static EWRAM_DATA struct PitCard sCard = {0};
static EWRAM_DATA bool8 sFightActive = FALSE;
static EWRAM_DATA struct Pokemon sSavedParty[PARTY_SIZE] = {0};   // (not the heap: a battle resets it)
static EWRAM_DATA u8 sSavedPartyCount = 0;
static EWRAM_DATA u8 sSavedName[PLAYER_NAME_LENGTH + 1] = {0};
static EWRAM_DATA struct Trainer sRedTrainer = {0};
static EWRAM_DATA struct TrainerMon sBlueMon = {0};
static EWRAM_DATA u16 sSavedPartnerId = 0;

// ---------------------------------------------------------------------------
// The card
// ---------------------------------------------------------------------------
static u32 BadgeCount(void)
{
    u32 i, n = 0;
    for (i = 0; i < 8; i++)
        n += FlagGet(FLAG_BADGE01_GET + i) != 0;
    return n;
}

static u32 CardLevel(void)
{
    return min(25 + BadgeCount() * 4, 70u);
}

static bool32 HasMove(const u16 *moves, u32 n, u32 move)
{
    u32 i;
    for (i = 0; i < n; i++)
        if (moves[i] == move)
            return TRUE;
    return FALSE;
}

// Its four: its strongest attacks from its level-up moves (its own types first, no two of a type
// where it can help), then whatever it learned last to fill up
// (and the back room's: src/pit_boss.c)
void Pit_PickMoves(u32 species, u32 level, u16 *moves)
{
    const struct LevelUpMove *ls = GetSpeciesLevelUpLearnset(species);
    u32 n = 0, pass, i, k;
    memset(moves, 0, sizeof(u16) * MAX_MON_MOVES);
    for (pass = 0; pass < 3 && n < MAX_MON_MOVES; pass++)
    {
        for (k = 0; k < MAX_MON_MOVES && n < MAX_MON_MOVES; k++)
        {
            u32 best = MOVE_NONE, bestScore = 0;
            for (i = 0; ls[i].move != LEVEL_UP_MOVE_END; i++)
            {
                u32 move = ls[i].move, type, score;
                bool32 stab, dupType = FALSE, j;
                if (ls[i].level > level || HasMove(moves, n, move) || GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS)
                    continue;
                type = GetMoveType(move);
                stab = (type == GetSpeciesType(species, 0) || type == GetSpeciesType(species, 1));
                for (j = 0; j < n; j++)
                    dupType |= (GetMoveType(moves[j]) == type);
                if (pass == 0 && !stab)
                    continue;
                if (pass < 2 && dupType)
                    continue;
                score = GetMovePower(move) * (stab ? 3 : 2) * max(GetMoveAccuracy(move), 50u);
                if (score > bestScore)
                    best = move, bestScore = score;
            }
            if (best == MOVE_NONE)
                break;
            moves[n++] = best;
        }
    }
    // (filling up: the last moves it learned)
    for (i = 0; ls[i].move != LEVEL_UP_MOVE_END; i++)
        ;
    while (i-- > 0 && n < MAX_MON_MOVES)
    {
        if (ls[i].level <= level && !HasMove(moves, n, ls[i].move))
            moves[n++] = ls[i].move;
    }
}

static void PickMoves(struct PitSide *s)
{
    Pit_PickMoves(s->species, s->level, s->moves);
}

static void CreateSideMon(struct Pokemon *mon, const struct PitSide *s, u32 level, u32 item)
{
    u32 i, iv = 20, abilityNum = s->abilityNum, nature = s->nature;
    CreateMon(mon, s->species, level, Random32(), OTID_STRUCT_RANDOM_NO_SHINY);
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_IV + i, &iv);
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilityNum);
    SetMonData(mon, MON_DATA_HIDDEN_NATURE, &nature);
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, s->moves[i], i);
    CalculateMonStats(mon);
}

static uq4_12_t Effectiveness(u32 type, u32 species)
{
    uq4_12_t m = GetTypeModifier(type, GetSpeciesType(species, 0));
    if (GetSpeciesType(species, 1) != GetSpeciesType(species, 0))
        m = uq4_12_multiply(m, GetTypeModifier(type, GetSpeciesType(species, 1)));
    return m;
}

// How many hits `a` needs to knock `d` out with its best move (as on the card), x10
static u32 HitsToKO(struct Pokemon *a, const struct PitSide *as, struct Pokemon *d, const struct PitSide *ds)
{
    u32 i, best = 0, hp = GetMonData(d, MON_DATA_MAX_HP);
    u32 level = GetMonData(a, MON_DATA_LEVEL);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u32 move = as->moves[i], type, atk, def, dmg;
        uq4_12_t eff;
        if (move == MOVE_NONE || GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS || GetMovePower(move) <= 1)
            continue;
        type = GetMoveType(move);
        if (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL)
            atk = GetMonData(a, MON_DATA_ATK), def = GetMonData(d, MON_DATA_DEF);
        else
            atk = GetMonData(a, MON_DATA_SPATK), def = GetMonData(d, MON_DATA_SPDEF);
        dmg = ((2 * level / 5 + 2) * GetMovePower(move) * atk / max(def, 1u)) / 50 + 2;
        if (type == GetSpeciesType(as->species, 0) || type == GetSpeciesType(as->species, 1))
            dmg = dmg * 3 / 2;
        eff = Effectiveness(type, ds->species);
        dmg = dmg * eff / UQ_4_12(1.0);
        dmg = dmg * min(GetMoveAccuracy(move) ? GetMoveAccuracy(move) : 100u, 100u) / 100;
        switch (as->item)
        {
        case ITEM_CHOICE_BAND: if (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL) dmg = dmg * 3 / 2; break;
        case ITEM_LIFE_ORB:    dmg = dmg * 13 / 10; break;
        case ITEM_EXPERT_BELT: if (eff > UQ_4_12(1.0)) dmg = dmg * 12 / 10; break;
        case ITEM_MUSCLE_BAND: if (GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL) dmg = dmg * 11 / 10; break;
        default: break;
        }
        best = max(best, dmg);
    }
    if (ds->item == ITEM_LEFTOVERS || ds->item == ITEM_SITRUS_BERRY)
        hp += hp / 8;
    if (best == 0)
        return 99;
    return min((hp * 10 + best - 1) / best, 99u);
}

// The chance the red corner wins, in percent (from the card: what the bookie sees)
static u32 RedChance(void)
{
    struct Pokemon *mons = Alloc(sizeof(struct Pokemon) * SIDES);
    struct PitSide *r = &sCard.side[RED], *b = &sCard.side[BLUE];
    s32 redHits, blueHits, margin, i;
    // (each half-hit of difference, roughly, from a coin toss to a sure thing)
    static const u8 sChance[] = {50, 57, 63, 69, 74, 78, 81, 84, 87, 90, 93, 95};
    CreateSideMon(&mons[RED], r, r->level, r->item);
    CreateSideMon(&mons[BLUE], b, b->level, b->item);
    redHits = HitsToKO(&mons[RED], r, &mons[BLUE], b);       // (x10: tenths of a hit)
    blueHits = HitsToKO(&mons[BLUE], b, &mons[RED], r);
    // whoever's faster gets the first hit: half a hit's worth
    margin = blueHits - redHits;
    if (GetMonData(&mons[RED], MON_DATA_SPEED) > GetMonData(&mons[BLUE], MON_DATA_SPEED))
        margin += 5;
    else if (GetMonData(&mons[RED], MON_DATA_SPEED) < GetMonData(&mons[BLUE], MON_DATA_SPEED))
        margin -= 5;
    Free(mons);
    i = min(abs(margin) / 5, (s32)ARRAY_COUNT(sChance) - 1);
    return margin >= 0 ? sChance[i] : 100 - sChance[i];
}

static u32 OddsFor(u32 chance)
{
    // (fair odds are 100/chance; the bookie pays less, and he's a little off either way)
    s32 odds = (1000 * HOUSE_CUT / 100) / max(chance, 1u);
    odds += (s32)(Random() % 3) - 1;
    return min(max(odds, 11), 99);
}

static void NewSide(struct PitSide *s, u32 fighter, u32 level)
{
    s->fighter = fighter;
    s->species = sPitSpecies[Random() % ARRAY_COUNT(sPitSpecies)];
    s->level = level;
    s->abilityNum = Random() % 2;
    if (GetAbilityBySpecies(s->species, s->abilityNum) == ABILITY_NONE)
        s->abilityNum = 0;
    s->nature = Random() % NUM_NATURES;
    s->item = sPitItems[Random() % ARRAY_COUNT(sPitItems)];
    PickMoves(s);
}

// The best `a` can do to `d` by type: its attacks' effectiveness, the best of them (x4: 4 neutral)
static u32 BestEffect(const struct PitSide *a, const struct PitSide *d)
{
    u32 i, best = 0;
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u32 move = a->moves[i];
        if (move == MOVE_NONE || GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS || GetMovePower(move) <= 1)
            continue;
        best = max(best, (u32)(Effectiveness(GetMoveType(move), d->species) * 4 / UQ_4_12(1.0)));
    }
    return best;
}

// Whether the two make a fair bout: types roughly even (a one-sided type advantage now and then,
// when the other side makes it up some other way), both able to hurt each other, and the
// underdog with a real chance by the card. `tries` loosens it, so a card always comes.
static bool32 FairBout(u32 redChance, u32 tries)
{
    const struct PitSide *r = &sCard.side[RED], *b = &sCard.side[BLUE];
    u32 effR = BestEffect(r, b), effB = BestEffect(b, r);
    if (effR == 0 || effB == 0)
        return FALSE;
    if (tries < 40 && ((effR >= 8 && effB < 8) || (effB >= 8 && effR < 8)) && Random() % 4 != 0)
        return FALSE;
    if (tries < 60 && (redChance < UNDERDOG_MIN || redChance > 100 - UNDERDOG_MIN))
        return FALSE;
    return TRUE;
}

static void NewCard(void)
{
    u32 level = CardLevel(), a, b, redChance, tries = 0;
    memset(&sCard, 0, sizeof(sCard));
    a = Random() % ARRAY_COUNT(sFighters);
    do
        b = Random() % ARRAY_COUNT(sFighters);
    while (b == a);
    NewSide(&sCard.side[RED], a, level + Random() % 5 - 2);
    do
    {
        // (now and then a new red corner too, so a hard one doesn't stick)
        if (tries > 0 && tries % 10 == 0)
            NewSide(&sCard.side[RED], a, level + Random() % 5 - 2);
        do
            NewSide(&sCard.side[BLUE], b, level + Random() % 5 - 2);
        while (sCard.side[BLUE].species == sCard.side[RED].species);
        redChance = RedChance();
        tries++;
    } while (!FairBout(redChance, tries) && tries < 80);
    redChance = min(max(redChance, (u32)UNDERDOG_MIN), 100u - UNDERDOG_MIN);
    sCard.side[RED].odds = OddsFor(redChance);
    sCard.side[BLUE].odds = OddsFor(100 - redChance);
    // The fix: the favorite takes a dive
    if (Random() % 100 < FIX_CHANCE)
    {
        sCard.fix = PIT_FIX_DIVE + Random() % 3;
        sCard.fixSide = redChance >= 50 ? RED : BLUE;
        sCard.fixItem = sFixItems[Random() % ARRAY_COUNT(sFixItems)];
    }
    sCard.betSide = SIDES;
    sCard.ready = TRUE;
}

static void SetGfxVars(void)
{
    u32 i;
    for (i = 0; i < SIDES; i++)
    {
        VarSet(VAR_OBJ_GFX_ID_0 + i, sCard.side[i].species + OBJ_EVENT_MON);
        VarSet(VAR_OBJ_GFX_ID_2 + i, sFighters[sCard.side[i].fighter].gfx);
    }
}

// Special (the pit's ON_TRANSITION): tonight's card (a new one each time the player comes down)
void Pit_OnTransition(void)
{
    NewCard();
    SetGfxVars();
}

// Special: the next bout (after a fight)
void Pit_NextCard(void)
{
    NewCard();
    SetGfxVars();
}

// ---------------------------------------------------------------------------
// The card on the rim, and the bookie's lines
// ---------------------------------------------------------------------------
static u8 *AppendOdds(u8 *dst, u32 odds)
{
    *dst++ = CHAR_x;
    dst = ConvertIntToDecimalStringN(dst, odds / 10, STR_CONV_MODE_LEFT_ALIGN, 2);
    *dst++ = CHAR_PERIOD;
    return ConvertIntToDecimalStringN(dst, odds % 10, STR_CONV_MODE_LEFT_ALIGN, 1);
}

static u8 *AppendSpeciesName(u8 *dst, u32 species)
{
    return StringCopy(dst, GetSpeciesName(species));
}

// Special: VAR_0x8004 = the page (0-1 the red corner, 2-3 the blue, 4 the odds) -> gStringVar4,
// VAR_0x8005 = the species (for showmonpic)
void Pit_BufferCardPage(void)
{
    u32 page = gSpecialVar_0x8004, sideId = (page >= 2) ? BLUE : RED;
    const struct PitSide *s = &sCard.side[sideId];
    u8 *str = gStringVar4;
    if (!sCard.ready)
        NewCard();
    gSpecialVar_0x8005 = s->species;
    switch (page)
    {
    case 0:
    case 2:
        str = StringCopy(str, sideId == RED ? COMPOUND_STRING("RED CORNER: ") : COMPOUND_STRING("BLUE CORNER: "));
        str = StringCopy(str, sFighters[s->fighter].name);
        str = StringCopy(str, COMPOUND_STRING("\n"));
        str = AppendSpeciesName(str, s->species);
        str = StringCopy(str, COMPOUND_STRING(" Lv"));
        str = ConvertIntToDecimalStringN(str, s->level, STR_CONV_MODE_LEFT_ALIGN, 3);
        str = StringCopy(str, COMPOUND_STRING("\pABILITY: "));
        str = StringCopy(str, gAbilitiesInfo[GetAbilityBySpecies(s->species, s->abilityNum)].name);
        str = StringCopy(str, COMPOUND_STRING("\nHOLDING: "));
        CopyItemName(s->item, str);
        break;
    case 1:
    case 3:
    {
        u32 i, n = 0;
        str = StringCopy(str, COMPOUND_STRING("MOVES: "));
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            if (s->moves[i] == MOVE_NONE)
                continue;
            if (n > 0)
                str = StringCopy(str, n == 2 ? COMPOUND_STRING(",\n") : COMPOUND_STRING(", "));
            str = StringCopy(str, GetMoveName(s->moves[i]));
            n++;
        }
        break;
    }
    default:
        str = StringCopy(str, COMPOUND_STRING("ODDS: "));
        str = AppendSpeciesName(str, sCard.side[RED].species);
        str = StringCopy(str, COMPOUND_STRING(" pays "));
        str = AppendOdds(str, sCard.side[RED].odds);
        str = StringCopy(str, COMPOUND_STRING(",\n"));
        str = AppendSpeciesName(str, sCard.side[BLUE].species);
        str = StringCopy(str, COMPOUND_STRING(" pays "));
        str = AppendOdds(str, sCard.side[BLUE].odds);
        str = StringCopy(str, COMPOUND_STRING("."));
        break;
    }
}

// Special: STR_VAR_1/2 = the red corner's fighter and Pokémon, STR_VAR_3 = the blue corner's
// Pokémon; VAR_0x8005/6 = their odds' payouts on ¥1000 (for the lines)
void Pit_BufferCard(void)
{
    if (!sCard.ready)
        NewCard();
    StringCopy(gStringVar1, sFighters[sCard.side[RED].fighter].name);
    StringCopy(gStringVar2, GetSpeciesName(sCard.side[RED].species));
    StringCopy(gStringVar3, GetSpeciesName(sCard.side[BLUE].species));
}

// Special: the two corners, to bet on (dynmultichoice: "MACHAMP  x1.4"); VAR_RESULT = 2
void Pit_PushCorners(void)
{
    u32 i;
    for (i = 0; i < SIDES; i++)
    {
        struct ListMenuItem item;
        u8 *name = Alloc(32), *end;
        end = AppendSpeciesName(name, sCard.side[i].species);
        *end++ = CHAR_SPACE;
        AppendOdds(end, sCard.side[i].odds);
        item.name = name;
        item.id = i;
        MultichoiceDynamic_PushElement(item);
    }
    gSpecialVar_Result = SIDES;
}

static u32 TipCut(u32 bet)
{
    return sCard.tipBought ? bet * TIP_CUT / 100 : 0;
}

static const u32 sBets[] = {5000, 10000, 25000, 50000, MAX_BET};

// Special: the bets the player can cover (dynmultichoice); VAR_RESULT = how many (0: broke)
void Pit_PushBets(void)
{
    u32 i, n = 0, money = GetMoney(&gSaveBlock1Ptr->money);
    for (i = 0; i < ARRAY_COUNT(sBets); i++)
    {
        struct ListMenuItem item;
        u8 *name;
        if (sBets[i] + TipCut(sBets[i]) > money)
            break;
        name = Alloc(16);
        ConvertIntToDecimalStringN(gStringVar1, sBets[i], STR_CONV_MODE_LEFT_ALIGN, 6);
        StringExpandPlaceholders(name, gText_PokedollarVar1);
        item.name = name;
        item.id = i;
        MultichoiceDynamic_PushElement(item);
        n++;
    }
    gSpecialVar_Result = n;
}

// Special: VAR_0x8004 = the corner, VAR_0x8005 = the bet (its place in the list): taken (and the
// tipster's cut, if he's been paid in kind: VAR_RESULT = TRUE, STR_VAR_2 = his cut, for the line).
// Pit_BufferBet then has the bet's strings.
void Pit_PlaceBet(void)
{
    u32 side = min(gSpecialVar_0x8004, (u16)BLUE), bet = sBets[min(gSpecialVar_0x8005, (u16)(ARRAY_COUNT(sBets) - 1))];
    u32 cut = TipCut(bet);
    gSpecialVar_Result = FALSE;
    if (!IsEnoughMoney(&gSaveBlock1Ptr->money, bet + cut))
        return;
    RemoveMoney(&gSaveBlock1Ptr->money, bet + cut);
    if (cut > 0)
    {
        sCard.tipPaid = TRUE;
        gSpecialVar_Result = TRUE;
        ConvertIntToDecimalStringN(gStringVar2, cut, STR_CONV_MODE_LEFT_ALIGN, 6);
    }
    sCard.betSide = side;
    sCard.bet = bet;
}

// Special (after updatemoneybox, which uses STR_VAR_1): the bet's strings again
void Pit_BufferBet(void)
{
    u32 side = min(sCard.betSide, (u8)BLUE);
    StringCopy(gStringVar1, GetSpeciesName(sCard.side[side].species));
    ConvertIntToDecimalStringN(gStringVar2, sCard.bet, STR_CONV_MODE_LEFT_ALIGN, 6);
    ConvertIntToDecimalStringN(gStringVar3, sCard.bet * sCard.side[side].odds / 10, STR_CONV_MODE_LEFT_ALIGN, 7);
}

// What the bookie pays on a corner (x10), and its Pokémon (for tests)
u32 Pit_GetOdds(u32 side)
{
    return sCard.side[min(side, (u32)BLUE)].odds;
}

u32 Pit_GetSpecies(u32 side)
{
    return sCard.side[min(side, (u32)BLUE)].species;
}

// Special: VAR_RESULT = TRUE if the player has a bet on this bout
void Pit_HasBet(void)
{
    gSpecialVar_Result = (sCard.betSide < SIDES);
}

// ---------------------------------------------------------------------------
// The tipster
// ---------------------------------------------------------------------------
// Special: the tip. His price is TIP_CUT percent of whatever the player bets on this bout: taken
// with the bet (Pit_PlaceBet), or now, if the bet's already down (STR_VAR_3 = his cut). VAR_RESULT:
// 0 the bet's down and the player can't pay him, 1 the fight's clean, 2 a dive (STR_VAR_1 the
// corner's Pokémon, STR_VAR_2 its fighter), 3 a swapped item (STR_VAR_3 what it's really holding),
// 4 not the level on the card. (Once a card: after that he repeats himself for free.)
void Pit_BuyTip(void)
{
    const struct PitSide *s;
    if (!sCard.tipBought && sCard.betSide < SIDES)
    {
        u32 cut = sCard.bet * TIP_CUT / 100;
        if (!IsEnoughMoney(&gSaveBlock1Ptr->money, cut))
        {
            gSpecialVar_Result = 0;
            return;
        }
        RemoveMoney(&gSaveBlock1Ptr->money, cut);
        sCard.tipPaid = TRUE;
    }
    sCard.tipBought = TRUE;
    s = &sCard.side[sCard.fixSide];
    StringCopy(gStringVar1, GetSpeciesName(s->species));
    StringCopy(gStringVar2, sFighters[s->fighter].name);
    CopyItemName(sCard.fixItem, gStringVar3);
    gSpecialVar_Result = (sCard.fix == PIT_FIX_NONE) ? 1 : sCard.fix + 1;
}

// Special: VAR_RESULT = TRUE if the tip's been bought for this card
void Pit_TipBought(void)
{
    gSpecialVar_Result = sCard.tipBought;
}

// ---------------------------------------------------------------------------
// The fight
// ---------------------------------------------------------------------------
bool32 Pit_FightActive(void)
{
    return sFightActive;
}

const struct Trainer *Pit_GetTrainer(u16 trainerId)
{
    if (trainerId == TRAINER_PIT_RED && sFightActive)
        return &sRedTrainer;
    return NULL;
}

static u64 FighterAI(u32 side)
{
    if (sCard.fix == PIT_FIX_DIVE && sCard.fixSide == side)
        return AI_FLAG_PREFER_STATUS_MOVES;     // (calling the wrong moves on purpose)
    return AI_FLAG_SMART_TRAINER;
}

// Special: the bout's set up: the red corner's Pokémon stands in for the player's party (and its
// fighter for the player), the blue corner's is the opponent (TRAINER_GYM_CHALLENGER)
void Pit_SetupFight(void)
{
    u32 i, level[SIDES], item[SIDES];
    const struct PitFighter *red = &sFighters[sCard.side[RED].fighter];
    const struct PitFighter *blue = &sFighters[sCard.side[BLUE].fighter];

    if (!sCard.ready)
        NewCard();
    for (i = 0; i < SIDES; i++)
    {
        level[i] = sCard.side[i].level;
        item[i] = sCard.side[i].item;
        if (sCard.fixSide == i && sCard.fix == PIT_FIX_ITEM)
            item[i] = sCard.fixItem;
        if (sCard.fixSide == i && sCard.fix == PIT_FIX_LEVEL)
            level[i] = max(level[i] - 6, 5u);
    }
    // The red corner, in the player's place
    memcpy(sSavedParty, gParties[B_TRAINER_PLAYER], sizeof(struct Pokemon) * PARTY_SIZE);
    sSavedPartyCount = gPartiesCount[B_TRAINER_PLAYER];
    StringCopy(sSavedName, gSaveBlock2Ptr->playerName);
    ZeroPlayerPartyMons();
    CreateSideMon(&gParties[B_TRAINER_PLAYER][0], &sCard.side[RED], level[RED], item[RED]);
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    StringCopyN(gSaveBlock2Ptr->playerName, red->name, PLAYER_NAME_LENGTH);
    gSaveBlock2Ptr->playerName[PLAYER_NAME_LENGTH] = EOS;
    memset(&sRedTrainer, 0, sizeof(sRedTrainer));
    sRedTrainer.trainerClass = red->trainerClass;
    sRedTrainer.trainerPic = red->pic;
    sRedTrainer.gender = red->female;
    sRedTrainer.aiFlags = FighterAI(RED);
    sRedTrainer.battleType = TRAINER_BATTLE_TYPE_SINGLES;
    StringCopyN(sRedTrainer.trainerName, red->name, TRAINER_NAME_LENGTH);
    sSavedPartnerId = gPartnerTrainerId;
    gPartnerTrainerId = TRAINER_PIT_RED;

    // The blue corner
    memset(&sBlueMon, 0, sizeof(sBlueMon));
    sBlueMon.species = sCard.side[BLUE].species;
    sBlueMon.lvl = level[BLUE];
    sBlueMon.iv = TRAINER_PARTY_IVS(20, 20, 20, 20, 20, 20);
    sBlueMon.nature = sCard.side[BLUE].nature;
    sBlueMon.heldItem = item[BLUE];
    sBlueMon.ability = GetAbilityBySpecies(sCard.side[BLUE].species, sCard.side[BLUE].abilityNum);
    sBlueMon.gender = TRAINER_MON_RANDOM_GENDER;
    sBlueMon.ball = POKEBALL_COUNT;
    sBlueMon.friendship = MAX_FRIENDSHIP;
    for (i = 0; i < MAX_MON_MOVES; i++)
        sBlueMon.moves[i] = sCard.side[BLUE].moves[i];
    GymChallenge_SetupGauntlet(blue->trainerClass, blue->pic, blue->female, blue->name, &sBlueMon, 1,
                               COMPOUND_STRING("Let's go!"), COMPOUND_STRING("Ugh!"),
                               COMPOUND_STRING("…"), COMPOUND_STRING("Pay up!"));
    GymChallenge_SetPracticeAI(FighterAI(BLUE));    // (practice: no prize money either way)
    GymChallenge_SetFormat(GYM_FORMAT_SINGLES);
    sFightActive = TRUE;
}

// Special (after the battle): everything's put back. VAR_RESULT: the winner (0 red, 1 blue);
// VAR_0x8004: 0 no bet, 1 the bet won (STR_VAR_2 what it paid), 2 it lost (STR_VAR_2 the stake).
// STR_VAR_1 = the winner's Pokémon, STR_VAR_3 = its fighter
void Pit_EndFight(void)
{
    u32 winner = (gBattleOutcome == B_OUTCOME_WON) ? RED : BLUE;
    if (sFightActive)
    {
        memcpy(gParties[B_TRAINER_PLAYER], sSavedParty, sizeof(struct Pokemon) * PARTY_SIZE);
        gPartiesCount[B_TRAINER_PLAYER] = sSavedPartyCount;
        StringCopy(gSaveBlock2Ptr->playerName, sSavedName);
    }
    gPartnerTrainerId = sSavedPartnerId;
    sFightActive = FALSE;
    gSpecialVar_Result = winner;
    StringCopy(gStringVar1, GetSpeciesName(sCard.side[winner].species));
    StringCopy(gStringVar3, sFighters[sCard.side[winner].fighter].name);
    if (sCard.betSide >= SIDES)
    {
        gSpecialVar_0x8004 = 0;
    }
    else if (sCard.betSide == winner)
    {
        u32 pay = sCard.bet * sCard.side[winner].odds / 10;
        AddMoney(&gSaveBlock1Ptr->money, pay);
        ConvertIntToDecimalStringN(gStringVar2, pay, STR_CONV_MODE_LEFT_ALIGN, 7);
        gSpecialVar_0x8004 = 1;
    }
    else
    {
        ConvertIntToDecimalStringN(gStringVar2, sCard.bet, STR_CONV_MODE_LEFT_ALIGN, 7);
        gSpecialVar_0x8004 = 2;
    }
    sCard.ready = FALSE;
}

// ---------------------------------------------------------------------------
// The way in
// ---------------------------------------------------------------------------
extern const u8 Pit_EventScript_HiddenDoor[];

// Celadon: walking east into the wall from (PIT_DOOR_X, PIT_DOOR_Y) (src/field_control_avatar.c)
bool32 Pit_TryHiddenDoor(s32 x, s32 y, u32 direction)
{
    if (direction != DIR_EAST
     || gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(MAP_CELADON_CITY)
     || gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CELADON_CITY)
     || x != PIT_DOOR_X + MAP_OFFSET || y != PIT_DOOR_Y + MAP_OFFSET)
        return FALSE;
    ScriptContext_SetupScript(Pit_EventScript_HiddenDoor);
    return TRUE;
}

// Special (then waitstate): the doorman asks for the password (the naming screen)
void Pit_AskPassword(void)
{
    gStringVar2[0] = EOS;
    DoNamingScreen(NAMING_SCREEN_CODE, gStringVar2, 0, 0, 0, CB2_ReturnToFieldContinueScript);
}

// Special: VAR_RESULT = TRUE if what the player said (STR_VAR_2) is the password (any case)
void Pit_CheckPassword(void)
{
    static const u8 sPassword[] = _("GRAVEL");
    u32 i;
    u8 said[CODE_NAME_LENGTH + 1];
    for (i = 0; i < CODE_NAME_LENGTH && gStringVar2[i] != EOS; i++)
    {
        u8 c = gStringVar2[i];
        said[i] = (c >= CHAR_a && c <= CHAR_z) ? c - CHAR_a + CHAR_A : c;
    }
    said[i] = EOS;
    gSpecialVar_Result = (StringCompare(said, sPassword) == 0);
}
