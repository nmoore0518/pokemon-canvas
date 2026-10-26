// Pokémon Canvas: the event Pokémon's side quests (scripts: data/scripts/event_mons.inc, lines:
// tools/gym_hack/event_mons_text.py). Every form that was only ever an event in the official games
// can be had here:
//   1. The Costume Maker (Celadon Condominiums 2F, once the gym is the player's): a Cosplay Pikachu,
//      and five costumes for it, each unlocked by a show of the matching quality (CostumeUnlocked).
//   2. The Cap Collector (a boy in Pallet Town, main game): seven caps hidden around Kanto, each traded
//      for its cap Pikachu, and the Partner Cap Pikachu for all seven. The caps go in his CAP CASE.
//   3. Partners: Oak's quiz on a type the player has mastered (Electric: Partner Pikachu, Normal:
//      Partner Eevee).
//   4. The Ilex shrine (the safari's Johto zone): Spiky-eared Pichu, a static that comes for a
//      party with Celebi (SEAL_PARTY, src/exotic_safari.c).
//   5. The Vivillon collector (Kalos zone): 6 patterns shown -> a Fancy Scatterbug, all 18 -> a Poké
//      Ball one; each is then wild in the Kalos zone too, now and then (ExoticSafari's RandomForm).
//   6. Bond Phenomenon (Kalos zone): a Froakie with Battle Bond once the Gauntlet's Kalos challengers
//      are beaten. Marked (MON_DATA_BATTLE_BOND), it evolves into Greninja's Battle Bond form.
//   7. The Eternal Flower (Kalos zone): AZ, after the Gauntlet's AZ is beaten: a Floette shown, and
//      Xerneas, Yveltal and Zygarde caught -> the Eternal Flower Floette.
//   8. Own Tempo (Alola zone): a trainer's Lycanroc pack; beat her for an Own Tempo Rockruff.
//   9. The Original Color (Alola zone): an artisan; Magearna caught and her four antique paint pots
//      found -> the Original Color Magearna.
//  10. Dada (Glimwood): a static Zarude (Dada) for a party with Celebi, once Zarude is caught.
//  11. The Darkest Day (the stadium's pitch): Eternamax, a static for a party with Zacian, Zamazenta
//      and Eternatus. Eternamax is the pitch's alone: anywhere else an Eternamax in the party is an
//      Eternatus again, and on the pitch (once the Darkest Day's Eternamax is caught) every
//      Eternatus in the party is Eternamax (EventMons_OnMapLoad).
#include "global.h"
#include "event_mons.h"
#include "data.h"
#include "event_data.h"
#include "gauntlet.h"
#include "gym_challenge.h"
#include "gym_decor.h"
#include "gym_switch.h"
#include "item.h"
#include "malloc.h"
#include "party_menu.h"
#include "pokedex.h"
#include "random.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "constants/battle_ai.h"
#include "constants/gym_hack.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "constants/moves.h"
#include "constants/trainers.h"

// ---------------------------------------------------------------------------------------------
// Gifts
// ---------------------------------------------------------------------------------------------
enum { GIFT_PLAIN, GIFT_BATTLE_BOND };

// Special: VAR_0x8004 = species, VAR_0x8005 = level, VAR_0x8006 = GIFT_*. VAR_RESULT = MON_GIVEN_TO_*
// (or MON_CANT_GIVE); STR_VAR_1 = the species' name
void EventMons_GiveMon(void)
{
    struct Pokemon *mon = AllocZeroed(sizeof(struct Pokemon));
    u32 species = gSpecialVar_0x8004;
    CreateRandomMon(mon, species, gSpecialVar_0x8005);
    if (gSpecialVar_0x8006 == GIFT_BATTLE_BOND)
    {
        bool32 bond = TRUE;
        SetMonData(mon, MON_DATA_BATTLE_BOND, &bond);
    }
    gSpecialVar_Result = GiveScriptedMonToPlayer(mon, PARTY_SIZE);
    Free(mon);
    StringCopy(gStringVar1, GetSpeciesName(species));
}

// (src/pokemon.c, after an evolution is picked: a Battle Bond Froakie's line ends in Greninja's
// Battle Bond form)
u32 EventMons_EvolutionTarget(struct Pokemon *mon, u32 target)
{
    if (target == SPECIES_GRENINJA && GetMonData(mon, MON_DATA_BATTLE_BOND))
        return SPECIES_GRENINJA_BATTLE_BOND;
    return target;
}

static struct Pokemon *PartyMon(u32 slot)
{
    if (slot >= PARTY_SIZE)
        return NULL;
    if (GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_SPECIES) == SPECIES_NONE
     || GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_IS_EGG))
        return NULL;
    return &gParties[B_TRAINER_PLAYER][slot];
}

// ---------------------------------------------------------------------------------------------
// 1. The Costume Maker
// ---------------------------------------------------------------------------------------------
enum { COSTUME_NONE, COSTUME_ROCK_STAR, COSTUME_BELLE, COSTUME_POP_STAR, COSTUME_PHD, COSTUME_LIBRE, COSTUME_COUNT };

static const u16 sCostumeSpecies[COSTUME_COUNT] =
{
    SPECIES_PIKACHU_COSPLAY, SPECIES_PIKACHU_ROCK_STAR, SPECIES_PIKACHU_BELLE,
    SPECIES_PIKACHU_POP_STAR, SPECIES_PIKACHU_PHD, SPECIES_PIKACHU_LIBRE,
};
// (as in Omega Ruby/Alpha Sapphire: each costume brings its own move, the plain outfit Thunder Shock)
static const u16 sCostumeMoves[COSTUME_COUNT] =
{
    MOVE_THUNDER_SHOCK, MOVE_METEOR_MASH, MOVE_ICICLE_CRASH, MOVE_DRAINING_KISS, MOVE_ELECTRIC_TERRAIN, MOVE_FLYING_PRESS,
};

#define COSTUME_DECOR_PIECES 20
#define ACADEMY_LECTURES 8

static bool32 CostumeUnlocked(u32 costume)
{
    u32 i;
    switch (costume)
    {
    case COSTUME_NONE:      return TRUE;
    case COSTUME_ROCK_STAR: return FlagGet(FLAG_EVENT_HALL_STREAK_7);                    // (cool: a 7-win Battle Hall streak)
    case COSTUME_BELLE:     return GymDecor_NumPlaced() >= COSTUME_DECOR_PIECES;         // (beauty: a well-decorated gym)
    case COSTUME_POP_STAR:  return FlagGet(FLAG_CONVENTION_TROPHY_FAIRY);                // (cute: the Convention's Fairy hall)
    case COSTUME_PHD:                                                                    // (smart: every Academy lecture)
        for (i = 0; i < ACADEMY_LECTURES; i++)
            if (!FlagGet(FLAG_ACADEMY_LECTURE_FIRST + i))
                return FALSE;
        return TRUE;
    case COSTUME_LIBRE:     return FlagGet(FLAG_CONVENTION_TROPHY_FIGHTING);             // (tough: the Convention's Fighting hall)
    }
    return FALSE;
}

// Special: VAR_0x8005 = a costume. VAR_RESULT = whether it's ready
void EventMons_CostumeUnlocked(void)
{
    gSpecialVar_Result = CostumeUnlocked(gSpecialVar_0x8005);
}

// Special: STR_VAR_1 = how many of the five costumes are ready; VAR_RESULT = that number
void EventMons_CostumeCount(void)
{
    u32 i, n = 0;
    for (i = COSTUME_ROCK_STAR; i < COSTUME_COUNT; i++)
        n += CostumeUnlocked(i);
    gSpecialVar_Result = n;
    ConvertIntToDecimalStringN(gStringVar1, n, STR_CONV_MODE_LEFT_ALIGN, 1);
}

static s32 CostumeOf(u32 species)
{
    u32 i;
    for (i = 0; i < COSTUME_COUNT; i++)
        if (sCostumeSpecies[i] == species)
            return i;
    return -1;
}

// Special: VAR_0x8004 = a party slot. VAR_RESULT = whether it's a Cosplay Pikachu (any costume);
// STR_VAR_3 = its name
void EventMons_CostumeCheckMon(void)
{
    struct Pokemon *mon = PartyMon(gSpecialVar_0x8004);
    gSpecialVar_Result = mon != NULL && CostumeOf(GetMonData(mon, MON_DATA_SPECIES)) >= 0;
    if (mon)
        GetMonNickname(mon, gStringVar3);
}

// Special: VAR_0x8004 = the party slot, VAR_0x8005 = the costume. It changes, and its costume's move
// takes the old costume's place (or a free slot, or the last move's)
void EventMons_SetCostume(void)
{
    struct Pokemon *mon = PartyMon(gSpecialVar_0x8004);
    u32 costume = min(gSpecialVar_0x8005, COSTUME_COUNT - 1), species, i, j, move;
    s32 slot = -1;
    if (mon == NULL || CostumeOf(GetMonData(mon, MON_DATA_SPECIES)) < 0)
        return;
    species = sCostumeSpecies[costume];
    SetMonData(mon, MON_DATA_SPECIES, &species);
    for (i = 0; i < MAX_MON_MOVES && slot < 0; i++)
    {
        move = GetMonData(mon, MON_DATA_MOVE1 + i);
        for (j = 0; j < COSTUME_COUNT; j++)
            if (move == sCostumeMoves[j])
                slot = i;
        if (move == MOVE_NONE)
            slot = i;
    }
    if (slot < 0)               // (no costume move yet and no free slot: the last move makes way)
        slot = MAX_MON_MOVES - 1;
    if (GetMonData(mon, MON_DATA_MOVE1 + slot) != sCostumeMoves[costume] && !MonKnowsMove(mon, sCostumeMoves[costume]))
        SetMonMoveSlot(mon, sCostumeMoves[costume], slot);
    CalculateMonStats(mon);
}

// The Battle Hall (src/leader_academy.c): a 7-win streak
void EventMons_HallStreak(u32 streak)
{
    if (streak >= 7)
        FlagSet(FLAG_EVENT_HALL_STREAK_7);
}

// ---------------------------------------------------------------------------------------------
// 2. The Cap Collector
// ---------------------------------------------------------------------------------------------
#define CAP_COUNT 7
#define CAP_GIVEN(i) (1 << (8 + (i)))       // (i = CAP_COUNT: the Partner Cap)

static const u16 sCapSpecies[CAP_COUNT + 1] =
{
    SPECIES_PIKACHU_ORIGINAL, SPECIES_PIKACHU_HOENN, SPECIES_PIKACHU_SINNOH, SPECIES_PIKACHU_UNOVA,
    SPECIES_PIKACHU_KALOS, SPECIES_PIKACHU_ALOLA, SPECIES_PIKACHU_WORLD, SPECIES_PIKACHU_PARTNER,
};
static const u8 *const sCapNames[CAP_COUNT] =
{
    COMPOUND_STRING("ORIGINAL CAP"), COMPOUND_STRING("HOENN CAP"), COMPOUND_STRING("SINNOH CAP"),
    COMPOUND_STRING("UNOVA CAP"), COMPOUND_STRING("KALOS CAP"), COMPOUND_STRING("ALOLA CAP"), COMPOUND_STRING("WORLD CAP"),
};
// (the boy's guesses at where each one went)
static const u8 *const sCapPlaces[CAP_COUNT] =
{
    COMPOUND_STRING("VIRIDIAN FOREST"), COMPOUND_STRING("MT. MOON"), COMPOUND_STRING("ROUTE 25, near BILL's"),
    COMPOUND_STRING("ROUTE 9"), COMPOUND_STRING("ROUTE 10"), COMPOUND_STRING("the UNDERGROUND PATH by SAFFRON"),
    COMPOUND_STRING("the long UNDERGROUND PATH"),
};

static u32 CountBits(u32 v)
{
    u32 n = 0;
    for (; v; v >>= 1)
        n += v & 1;
    return n;
}

// Special (a cap's hiding place): VAR_0x8004 = which. VAR_RESULT: 0 no CAP CASE yet (it stays),
// 1 found now, 2 found before. STR_VAR_1 = its name
void EventMons_CapFound(void)
{
    u32 i = min(gSpecialVar_0x8004, CAP_COUNT - 1), caps = VarGet(VAR_EVENT_CAPS);
    StringCopy(gStringVar1, sCapNames[i]);
    if (caps & (1 << i) || caps & CAP_GIVEN(i))
        gSpecialVar_Result = 2;
    else if (!CheckBagHasItem(ITEM_CAP_CASE, 1))
        gSpecialVar_Result = 0;
    else
    {
        VarSet(VAR_EVENT_CAPS, caps | (1 << i));
        gSpecialVar_Result = 1;
    }
}

// Special (the boy): VAR_RESULT = a cap Pikachu he owes the player (SPECIES_NONE: none), VAR_0x8005
// = its cap (CAP_COUNT: the Partner Cap, for all seven); STR_VAR_1 = the cap's name
void EventMons_CapReward(void)
{
    u32 caps = VarGet(VAR_EVENT_CAPS), i;
    gSpecialVar_Result = SPECIES_NONE;
    for (i = 0; i < CAP_COUNT; i++)
    {
        if ((caps & (1 << i)) && !(caps & CAP_GIVEN(i)))
        {
            gSpecialVar_Result = sCapSpecies[i];
            gSpecialVar_0x8005 = i;
            StringCopy(gStringVar1, sCapNames[i]);
            return;
        }
    }
    if (CountBits((caps >> 8) & ((1 << CAP_COUNT) - 1)) == CAP_COUNT && !(caps & CAP_GIVEN(CAP_COUNT)))
    {
        gSpecialVar_Result = sCapSpecies[CAP_COUNT];
        gSpecialVar_0x8005 = CAP_COUNT;
        StringCopy(gStringVar1, COMPOUND_STRING("PARTNER CAP"));
    }
}

// Special: VAR_0x8005 = the cap whose Pikachu was given (EventMons_CapReward)
void EventMons_CapRewardGiven(void)
{
    u32 i = min(gSpecialVar_0x8005, CAP_COUNT);
    VarSet(VAR_EVENT_CAPS, (VarGet(VAR_EVENT_CAPS) & ~(i < CAP_COUNT ? 1 << i : 0)) | CAP_GIVEN(i));
}

// Special (the boy's hint): VAR_RESULT = how many caps are still out there; STR_VAR_1 = the next
// one's name, STR_VAR_2 = where he thinks it is
void EventMons_CapHint(void)
{
    u32 caps = VarGet(VAR_EVENT_CAPS), i, left = 0;
    s32 next = -1;
    for (i = 0; i < CAP_COUNT; i++)
    {
        if (!(caps & (1 << i)) && !(caps & CAP_GIVEN(i)))
        {
            left++;
            if (next < 0)
                next = i;
        }
    }
    gSpecialVar_Result = left;
    if (next >= 0)
    {
        StringCopy(gStringVar1, sCapNames[next]);
        StringCopy(gStringVar2, sCapPlaces[next]);
    }
}

// ---------------------------------------------------------------------------------------------
// 3. Partners
// ---------------------------------------------------------------------------------------------
// Special (Oak): VAR_RESULT = 1 for Partner Pikachu's quiz (Electric mastered), 2 for Partner Eevee's
// (Normal mastered), 0 for neither (not mastered yet, or given already)
void EventMons_PartnerReady(void)
{
    gSpecialVar_Result = 0;
    if (FlagGet(FLAG_GYM_TYPE_MASTERED + TYPE_ELECTRIC) && !FlagGet(FLAG_EVENT_PARTNER_PIKACHU))
        gSpecialVar_Result = 1;
    else if (FlagGet(FLAG_GYM_TYPE_MASTERED + TYPE_NORMAL) && !FlagGet(FLAG_EVENT_PARTNER_EEVEE))
        gSpecialVar_Result = 2;
}

// ---------------------------------------------------------------------------------------------
// 5. The Vivillon collector
// ---------------------------------------------------------------------------------------------
static const u16 sVivillonPatterns[] =
{
    SPECIES_VIVILLON_ICY_SNOW, SPECIES_VIVILLON_POLAR, SPECIES_VIVILLON_TUNDRA, SPECIES_VIVILLON_CONTINENTAL,
    SPECIES_VIVILLON_GARDEN, SPECIES_VIVILLON_ELEGANT, SPECIES_VIVILLON_MEADOW, SPECIES_VIVILLON_MODERN,
    SPECIES_VIVILLON_MARINE, SPECIES_VIVILLON_ARCHIPELAGO, SPECIES_VIVILLON_HIGH_PLAINS, SPECIES_VIVILLON_SANDSTORM,
    SPECIES_VIVILLON_RIVER, SPECIES_VIVILLON_MONSOON, SPECIES_VIVILLON_SAVANNA, SPECIES_VIVILLON_SUN,
    SPECIES_VIVILLON_OCEAN, SPECIES_VIVILLON_JUNGLE,
};
#define VIVILLON_FOR_FANCY 6

static u32 VivillonSeen(void)
{
    return VarGet(VAR_EVENT_VIVILLON_LO) | ((VarGet(VAR_EVENT_VIVILLON_HI) & 3) << 16);
}

// Special (the collector): every Vivillon in the party is noted. VAR_RESULT = how many patterns were
// new; STR_VAR_1 = how many he has now. VAR_0x8005 = the Scatterbug he owes: 1 Fancy, 2 Poké Ball, 0 none
void EventMons_RecordVivillon(void)
{
    u32 seen = VivillonSeen(), before = seen, i, k, n;
    for (i = 0; i < PARTY_SIZE; i++)
        for (k = 0; k < ARRAY_COUNT(sVivillonPatterns); k++)
            if (PartyMon(i) && GetMonData(PartyMon(i), MON_DATA_SPECIES) == sVivillonPatterns[k])
                seen |= 1 << k;
    VarSet(VAR_EVENT_VIVILLON_LO, seen & 0xFFFF);
    VarSet(VAR_EVENT_VIVILLON_HI, (VarGet(VAR_EVENT_VIVILLON_HI) & ~3) | (seen >> 16));
    n = CountBits(seen);
    gSpecialVar_Result = n - CountBits(before);
    ConvertIntToDecimalStringN(gStringVar1, n, STR_CONV_MODE_LEFT_ALIGN, 2);
    gSpecialVar_0x8005 = 0;
    if (n >= VIVILLON_FOR_FANCY && !FlagGet(FLAG_EVENT_FANCY_SCATTERBUG))
        gSpecialVar_0x8005 = 1;
    else if (n >= ARRAY_COUNT(sVivillonPatterns) && !FlagGet(FLAG_EVENT_POKEBALL_SCATTERBUG))
        gSpecialVar_0x8005 = 2;
}

// (src/exotic_safari.c: a wild Scatterbug, Spewpa or Vivillon's pattern, once the collector has given
// that pattern: 1 in 19 each)
u32 EventMons_WildVivillonPattern(u32 species)
{
    static const u16 sExtra[][3] =
    {
        {SPECIES_SCATTERBUG_FANCY, SPECIES_SPEWPA_FANCY, SPECIES_VIVILLON_FANCY},
        {SPECIES_SCATTERBUG_POKEBALL, SPECIES_SPEWPA_POKEBALL, SPECIES_VIVILLON_POKEBALL},
    };
    u32 stage;
    switch (GET_BASE_SPECIES_ID(species))
    {
    case SPECIES_SCATTERBUG: stage = 0; break;
    case SPECIES_SPEWPA:     stage = 1; break;
    case SPECIES_VIVILLON:   stage = 2; break;
    default: return species;
    }
    if (FlagGet(FLAG_EVENT_FANCY_SCATTERBUG) && Random() % 19 == 0)
        return sExtra[0][stage];
    if (FlagGet(FLAG_EVENT_POKEBALL_SCATTERBUG) && Random() % 19 == 0)
        return sExtra[1][stage];
    return species;
}

// ---------------------------------------------------------------------------------------------
// 6, 7. The Gauntlet's Kalos challengers; AZ and the Eternal Flower
// ---------------------------------------------------------------------------------------------
#define GAUNTLET_REGION_KALOS 5

// Special: VAR_RESULT = whether the Gauntlet's Kalos challengers have all been beaten
void EventMons_KalosGauntletBeaten(void)
{
    gSpecialVar_Result = Gauntlet_RegionBeaten(GAUNTLET_REGION_KALOS);
}

// Special: VAR_RESULT = whether the Gauntlet's AZ has been beaten
void EventMons_AzBeaten(void)
{
    gSpecialVar_Result = Gauntlet_TrainerBeaten(COMPOUND_STRING("AZ"));
}

// Special: VAR_RESULT = whether a Floette (any color, not the Eternal Flower) is in the party
void EventMons_PartyHasFloette(void)
{
    u32 i;
    gSpecialVar_Result = FALSE;
    for (i = 0; i < PARTY_SIZE; i++)
        if (PartyMon(i) && GET_BASE_SPECIES_ID(GetMonData(PartyMon(i), MON_DATA_SPECIES)) == SPECIES_FLOETTE)
            gSpecialVar_Result = TRUE;
}

// Special: VAR_RESULT = how many of Xerneas, Yveltal and Zygarde aren't caught yet; STR_VAR_1 = the first
void EventMons_KalosTrioLeft(void)
{
    static const u16 sTrio[] = {SPECIES_XERNEAS, SPECIES_YVELTAL, SPECIES_ZYGARDE};
    u32 i;
    gSpecialVar_Result = 0;
    for (i = 0; i < ARRAY_COUNT(sTrio); i++)
    {
        if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(sTrio[i]), FLAG_GET_CAUGHT))
        {
            if (gSpecialVar_Result++ == 0)
                StringCopy(gStringVar1, GetSpeciesName(sTrio[i]));
        }
    }
}

// ---------------------------------------------------------------------------------------------
// 8. Own Tempo: the Alola zone's Rockruff trainer, Tala, and her Lycanroc
// ---------------------------------------------------------------------------------------------
static const struct TrainerMon sTalaParty[] =
{
    {
        .species = SPECIES_LYCANROC_MIDDAY, .lvl = 58, .heldItem = ITEM_LIFE_ORB,
        .moves = {MOVE_ACCELEROCK, MOVE_STONE_EDGE, MOVE_CLOSE_COMBAT, MOVE_PSYCHIC_FANGS},
        .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .ev = TRAINER_PARTY_EVS(4, 252, 0, 252, 0, 0),
        .nature = NATURE_JOLLY, .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT,
    },
    {
        .species = SPECIES_LYCANROC_MIDNIGHT, .lvl = 58, .heldItem = ITEM_FOCUS_SASH,
        .moves = {MOVE_STONE_EDGE, MOVE_CRUNCH, MOVE_CLOSE_COMBAT, MOVE_SUCKER_PUNCH},
        .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .ev = TRAINER_PARTY_EVS(4, 252, 0, 252, 0, 0),
        .nature = NATURE_ADAMANT, .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT,
    },
    {
        .species = SPECIES_LUCARIO, .lvl = 58, .heldItem = ITEM_EXPERT_BELT,
        .moves = {MOVE_AURA_SPHERE, MOVE_FLASH_CANNON, MOVE_DARK_PULSE, MOVE_VACUUM_WAVE},
        .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .ev = TRAINER_PARTY_EVS(4, 0, 0, 252, 252, 0),
        .nature = NATURE_TIMID, .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT,
    },
    {
        .species = SPECIES_STOUTLAND, .lvl = 58, .heldItem = ITEM_CHOICE_BAND,
        .moves = {MOVE_RETURN, MOVE_CRUNCH, MOVE_WILD_CHARGE, MOVE_PLAY_ROUGH},
        .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .ev = TRAINER_PARTY_EVS(4, 252, 0, 252, 0, 0),
        .nature = NATURE_ADAMANT, .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT,
    },
    {
        .species = SPECIES_LYCANROC_DUSK, .lvl = 60, .heldItem = ITEM_CHOICE_SCARF,
        .moves = {MOVE_ACCELEROCK, MOVE_STONE_EDGE, MOVE_CLOSE_COMBAT, MOVE_PSYCHIC_FANGS},
        .iv = TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31), .ev = TRAINER_PARTY_EVS(4, 252, 0, 252, 0, 0),
        .nature = NATURE_JOLLY, .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT,
    },
};

// Special: Tala's battle
void EventMons_SetupTala(void)
{
    GymChallenge_SetupGauntlet(TRAINER_CLASS_COOLTRAINER_FRLG, TRAINER_PIC_COOLTRAINER_F_FRLG, TRUE, COMPOUND_STRING("TALA"),
                               sTalaParty, ARRAY_COUNT(sTalaParty),
                               COMPOUND_STRING("My pack runs together.\nLet's see if yours can keep up!"),
                               COMPOUND_STRING("Ha! You howl with the best\nof them."),
                               COMPOUND_STRING("Ha! You howl with the best\nof them."),
                               COMPOUND_STRING("The pack wins this time!"));
}

// ---------------------------------------------------------------------------------------------
// 9. The Original Color
// ---------------------------------------------------------------------------------------------
#define PAINT_POTS 4

// Special: VAR_RESULT = how many of the antique paint pots are still out there
void EventMons_PaintPotsLeft(void)
{
    u32 i;
    gSpecialVar_Result = 0;
    for (i = 0; i < PAINT_POTS; i++)
        gSpecialVar_Result += !FlagGet(FLAG_EVENT_PAINT_POT_FIRST + i);
    ConvertIntToDecimalStringN(gStringVar2, gSpecialVar_Result, STR_CONV_MODE_LEFT_ALIGN, 1);
}

// ---------------------------------------------------------------------------------------------
// 11. Eternamax belongs to the pitch
// ---------------------------------------------------------------------------------------------
// (the overworld, as each map loads: before its ON_TRANSITION script)
void EventMons_OnMapLoad(void)
{
    u32 i, species, onPitch = gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_STADIUM_PITCH)
                           && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_STADIUM_PITCH);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = PartyMon(i);
        u32 to = SPECIES_NONE;
        if (mon == NULL)
            continue;
        species = GetMonData(mon, MON_DATA_SPECIES);
        if (species == SPECIES_ETERNATUS_ETERNAMAX && !onPitch)
            to = SPECIES_ETERNATUS;
        else if (species == SPECIES_ETERNATUS && onPitch && FlagGet(FLAG_EXOTIC_LEGEND_ETERNATUS_ETERNAMAX))
            to = SPECIES_ETERNATUS_ETERNAMAX;
        if (to != SPECIES_NONE)
        {
            u32 hp = GetMonData(mon, MON_DATA_HP), maxHp = GetMonData(mon, MON_DATA_MAX_HP);
            SetMonData(mon, MON_DATA_SPECIES, &to);
            CalculateMonStats(mon);
            // (the same share of its HP)
            if (hp != 0 && maxHp != 0)
            {
                hp = max(1, hp * GetMonData(mon, MON_DATA_MAX_HP) / maxHp);
                SetMonData(mon, MON_DATA_HP, &hp);
            }
        }
    }
}
