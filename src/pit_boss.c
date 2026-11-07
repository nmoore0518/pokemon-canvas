// Pokémon Canvas: the back room under Celadon (data/scripts/pit.inc). No money changes hands
// down here: the boss puts up a Pokémon, the player puts up one of theirs, one on one, and the
// winner keeps both. Lose, and the boss keeps the player's Pokémon.
//
// His Pokémon of the day (a new one every "day" of steps: src/medical_center.c) is something
// hard to come by: one that's rare in the wild (tools/gym_hack/build_pit_boss_pool.py), the final
// form of one only its earlier forms are found wild, or (now and then) a legendary - not the gym
// legendaries (src/gym_legend.c). It has perfect IVs, a full set of EVs and a good item, and it's
// always the level of the Pokémon the player puts up. He only takes a fair fight: the two
// Pokémon's stats at that level (PitBoss_Power) within POWER_MARGIN percent of each other.
//
// The stakes are real: the game saves before the battle (with the player's Pokémon held as the
// stake: switching off mid-battle loses it) and after it. A Pokémon he's won he'll sell back for
// BUYBACK_PRICE - until he wins another.
#include "global.h"
#include "pit_boss.h"
#include "pit.h"
#include "battle.h"
#include "battle_setup.h"
#include "data.h"
#include "event_data.h"
#include "gym_challenge.h"
#include "gym_legend.h"
#include "money.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "save.h"
#include "constants/event_objects.h"
#include "string_util.h"
#include "constants/battle_ai.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/party_menu.h"
#include "constants/trainers.h"

#include "data/pit_boss_pool.h"

#define PIT_BOSS_MAGIC  0xB055
#define LEGEND_CHANCE   3           // (percent of days)
#define POWER_MARGIN    10          // (percent)
#define BUYBACK_PRICE   500000

struct PitBossSave gPitBossSave;
struct BoxPokemon gPitBossHeld;

static EWRAM_DATA struct Pokemon sSavedParty[PARTY_SIZE] = {0};   // (not the heap: a battle resets it)
static EWRAM_DATA u8 sSavedPartyCount = 0;
static EWRAM_DATA u8 sStakeSlot = 0;
static EWRAM_DATA bool8 sFightActive = FALSE;
static EWRAM_DATA struct TrainerMon sBossMon = {0};
static EWRAM_DATA u8 sBossEvs[NUM_STATS] = {0};
static EWRAM_DATA u16 sPool[1024] = {0};
static EWRAM_DATA u16 sPoolSize[2] = {0};      // (the pools, worked out once: [0] the rare ones, [1] the legendaries)
static EWRAM_DATA u16 sLegends[128] = {0};
static EWRAM_DATA u32 sSeed = 0;

// ---------------------------------------------------------------------------
// The save (flash sector 31: src/save.c)
// ---------------------------------------------------------------------------
void PitBoss_Clear(void)
{
    memset(&gPitBossSave, 0, sizeof(gPitBossSave));
    gPitBossSave.magic = PIT_BOSS_MAGIC;
    ZeroBoxMonData(&gPitBossHeld);
}

void PitBoss_Validate(void)
{
    if (gPitBossSave.magic != PIT_BOSS_MAGIC)
        PitBoss_Clear();
    if (gPitBossSave.heldValid && (GetBoxMonData(&gPitBossHeld, MON_DATA_SPECIES) == SPECIES_NONE
                                || GetBoxMonData(&gPitBossHeld, MON_DATA_SANITY_IS_BAD_EGG)))
        gPitBossSave.heldValid = FALSE;
}

static void Hold(struct BoxPokemon *mon)
{
    gPitBossHeld = *mon;
    gPitBossSave.heldValid = TRUE;
}

// ---------------------------------------------------------------------------
// His Pokémon of the day
// ---------------------------------------------------------------------------
struct BossPick
{
    u16 species;
    u16 item;
    u8 abilityNum;
    u8 nature;
    u8 evs[NUM_STATS];      // (HP, Atk, Def, Speed, SpAtk, SpDef: the STAT_* order)
};

static u32 DayRandom(void)
{
    sSeed = sSeed * 1103515245 + 12345;
    return sSeed >> 16;
}

static bool32 InList(const u16 *list, u32 n, u32 species)
{
    u32 i;
    for (i = 0; i < n; i++)
        if (list[i] == species)
            return TRUE;
    return FALSE;
}

static bool32 IsGymLegend(u32 species)
{
    u32 type;
    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
        if (gGymLegends[type] != SPECIES_NONE && GET_BASE_SPECIES_ID(gGymLegends[type]) == GET_BASE_SPECIES_ID(species))
            return TRUE;
    return FALSE;
}

static bool32 IsLegend(u32 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    return info->isRestrictedLegendary || info->isSubLegendary || info->isMythical;
}

static bool32 IsBattleForm(u32 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    return info->isMegaEvolution || info->isPrimalReversion || info->isUltraBurst || info->isGigantamax
        || info->isTeraForm || info->isTotem;
}

static bool32 HasEvolution(u32 species)
{
    const struct Evolution *evos = GetSpeciesEvolutions(species);
    return evos != NULL && evos[0].method != EVOLUTIONS_END;
}

static void AddToPool(u32 *n, u32 species)
{
    if (*n < ARRAY_COUNT(sPool) && !InList(sPool, *n, species))
        sPool[(*n)++] = species;
}

static void AddLegend(u32 *n, u32 species)
{
    if (*n < ARRAY_COUNT(sLegends))
        sLegends[(*n)++] = species;
}

// Every final form reached from `species` that's never found wild
static void AddFinalForms(u32 *n, u32 species, u32 depth)
{
    const struct Evolution *evos = GetSpeciesEvolutions(species);
    u32 i;
    if (depth > 3 || evos == NULL)
        return;
    for (i = 0; evos[i].method != EVOLUTIONS_END; i++)
    {
        u32 target = evos[i].targetSpecies;
        if (!IsSpeciesEnabled(target) || IsBattleForm(target))
            continue;
        if (HasEvolution(target))
            AddFinalForms(n, target, depth + 1);
        else if (!InList(sWildSpecies, ARRAY_COUNT(sWildSpecies), target) && !IsLegend(target))
            AddToPool(n, target);
    }
}

// The pools (once): rare in the wild, or a final form not found wild at all; and the legendaries
static void BuildPools(void)
{
    u32 n = 0, i;
    if (sPoolSize[0] != 0)
        return;
    for (i = 0; i < ARRAY_COUNT(sRareWildSpecies); i++)
    {
        u32 s = sRareWildSpecies[i];
        if (IsSpeciesEnabled(s) && !IsLegend(s))
            AddToPool(&n, s);
    }
    for (i = 0; i < ARRAY_COUNT(sWildSpecies); i++)
        AddFinalForms(&n, sWildSpecies[i], 0);
    sPoolSize[0] = n;
    n = 0;
    for (i = 1; i < NUM_SPECIES; i++)
    {
        if (IsSpeciesEnabled(i) && IsLegend(i) && !IsBattleForm(i) && GET_BASE_SPECIES_ID(i) == i && !IsGymLegend(i))
            AddLegend(&n, i);
    }
    sPoolSize[1] = n;
}

// (the nature that raises `up` and lowers `down`)
static u32 NatureFor(u32 up, u32 down)
{
    u32 nature;
    for (nature = 0; nature < NUM_NATURES; nature++)
        if (gNaturesInfo[nature].statUp == up && gNaturesInfo[nature].statDown == down)
            return nature;
    return NATURE_HARDY;
}

static void PickDay(struct BossPick *p)
{
    const struct SpeciesInfo *info;
    u32 n, abilities = 0, i, attack, other, bulk;
    bool32 physical, fast;

    sSeed = gPitBossSave.day * 2654435761u ^ (gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
            | (gSaveBlock2Ptr->playerTrainerId[2] << 16) | (gSaveBlock2Ptr->playerTrainerId[3] << 24)) ^ 0x5EED;
    DayRandom();
    BuildPools();
    memset(p, 0, sizeof(*p));
    if (DayRandom() % 100 < LEGEND_CHANCE && sPoolSize[1] != 0)
        p->species = sLegends[DayRandom() % sPoolSize[1]];
    else
        p->species = (sPoolSize[0] != 0) ? sPool[DayRandom() % sPoolSize[0]] : SPECIES_GARCHOMP;
    info = &gSpeciesInfo[p->species];

    // An ability it can have
    for (i = 0; i < NUM_ABILITY_SLOTS; i++)
        abilities += (GetAbilityBySpecies(p->species, i) != ABILITY_NONE);
    p->abilityNum = DayRandom() % max(abilities, 1u);
    while (p->abilityNum > 0 && GetAbilityBySpecies(p->species, p->abilityNum) == ABILITY_NONE)
        p->abilityNum--;

    // Built the way a serious trainer would: its better attacking stat and its speed (or, if it's
    // slow, its HP) maxed, and a nature to match
    physical = info->baseAttack >= info->baseSpAttack;
    attack = physical ? STAT_ATK : STAT_SPATK;
    other = physical ? STAT_SPATK : STAT_ATK;
    fast = info->baseSpeed >= 70;
    bulk = info->baseHP + info->baseDefense + info->baseSpDefense;
    p->evs[attack] = 252;
    p->evs[fast ? STAT_SPEED : STAT_HP] = 252;
    p->evs[fast ? STAT_HP : STAT_DEF] = 6;
    p->nature = NatureFor(fast ? STAT_SPEED : attack, other);
    if (bulk >= 270 || info->baseSpeed < 50)
        p->item = ITEM_LEFTOVERS;
    else if (info->baseSpeed >= 90)
        p->item = ITEM_LIFE_ORB;
    else
        p->item = physical ? ITEM_CHOICE_BAND : ITEM_CHOICE_SPECS;
}

static void CreateBossMon(struct Pokemon *mon, const struct BossPick *p, u32 level, bool32 playersOwn)
{
    u32 i, iv = MAX_PER_STAT_IVS, item = p->item, abilityNum = p->abilityNum, nature = p->nature;
    u16 moves[MAX_MON_MOVES];
    // (the player's to keep: theirs from the start - an OT changed afterwards breaks its encryption)
    CreateMon(mon, p->species, level, sSeed, playersOwn ? OTID_STRUCT_PLAYER_ID : OTID_STRUCT_RANDOM_NO_SHINY);
    for (i = 0; i < NUM_STATS; i++)
    {
        u32 ev = p->evs[i];
        SetMonData(mon, MON_DATA_HP_IV + i, &iv);
        SetMonData(mon, MON_DATA_HP_EV + i, &ev);
    }
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilityNum);
    SetMonData(mon, MON_DATA_HIDDEN_NATURE, &nature);
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    Pit_PickMoves(p->species, level, moves);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, moves[i], i);
    CalculateMonStats(mon);
}

// A Pokémon's power: its six stats added up (at its level, with its IVs, EVs and nature)
u32 PitBoss_Power(struct Pokemon *mon)
{
    return GetMonData(mon, MON_DATA_MAX_HP) + GetMonData(mon, MON_DATA_ATK) + GetMonData(mon, MON_DATA_DEF)
         + GetMonData(mon, MON_DATA_SPEED) + GetMonData(mon, MON_DATA_SPATK) + GetMonData(mon, MON_DATA_SPDEF);
}

u32 PitBoss_GetSpecies(void)
{
    struct BossPick p;
    PickDay(&p);
    return p.species;
}

// (his Pokémon of the day's power at a level: for the tests)
u32 PitBoss_DayPower(u32 level)
{
    struct BossPick p;
    struct Pokemon mon;
    PickDay(&p);
    CreateBossMon(&mon, &p, level, FALSE);
    return PitBoss_Power(&mon);
}

// ---------------------------------------------------------------------------
// The days
// ---------------------------------------------------------------------------
// A new "day" of steps (src/medical_center.c): a new Pokémon
void PitBoss_NewDay(void)
{
    gPitBossSave.day++;
    FlagClear(FLAG_PIT_BOSS_BEATEN);
}

static bool32 IsStake(struct BoxPokemon *mon)
{
    return GetBoxMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE
        && GetBoxMonData(mon, MON_DATA_PERSONALITY) == gPitBossSave.stakePersonality
        && GetBoxMonData(mon, MON_DATA_OT_ID) == gPitBossSave.stakeOtId;
}

// The stake's still held from a battle that never finished (the power went off): it's lost.
// Taken from wherever it is now.
static void ForfeitStake(void)
{
    u32 i, box, pos;
    if (!gPitBossSave.stakeHeld)
        return;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (IsStake(&gParties[B_TRAINER_PLAYER][i].box))
        {
            Hold(&gParties[B_TRAINER_PLAYER][i].box);
            ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
            CompactPartySlots();
            CalculatePlayerPartyCount();
            goto taken;
        }
    }
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            if (IsStake(GetBoxedMonPtr(box, pos)))
            {
                Hold(GetBoxedMonPtr(box, pos));
                ZeroBoxMonAt(box, pos);
                goto taken;
            }
        }
    }
taken:                                  // (or let go of elsewhere: then it's gone anyway)
    gPitBossSave.stakeHeld = FALSE;
    gPitBossSave.forfeited = TRUE;
}

void PitBoss_OnStep(void)
{
    if (gPitBossSave.stakeHeld && !sFightActive)
        ForfeitStake();
}

// ---------------------------------------------------------------------------
// Specials
// ---------------------------------------------------------------------------
// The back room's ON_TRANSITION: his Pokémon of the day, standing by him
void PitBoss_OnTransition(void)
{
    VarSet(VAR_OBJ_GFX_ID_0, PitBoss_GetSpecies() + OBJ_EVENT_MON);
}

// VAR_RESULT: whether the power went off on him last time (and he took the stake). Told once.
// STR_VAR_2 = that Pokémon
void PitBoss_CheckForfeit(void)
{
    ForfeitStake();
    gSpecialVar_Result = gPitBossSave.forfeited;
    if (gPitBossSave.forfeited)
    {
        GetBoxMonData(&gPitBossHeld, MON_DATA_NICKNAME, gStringVar2);
        StringGet_Nickname(gStringVar2);
    }
    gPitBossSave.forfeited = FALSE;
}

// STR_VAR_1 = his Pokémon of the day
void PitBoss_BufferDay(void)
{
    StringCopy(gStringVar1, GetSpeciesName(PitBoss_GetSpecies()));
}

// VAR_RESULT: whether he's holding one of the player's Pokémon (STR_VAR_2: it)
void PitBoss_HasHeld(void)
{
    gSpecialVar_Result = gPitBossSave.heldValid;
    if (gPitBossSave.heldValid)
    {
        GetBoxMonData(&gPitBossHeld, MON_DATA_NICKNAME, gStringVar2);
        StringGet_Nickname(gStringVar2);
    }
}

// Buying it back. VAR_RESULT: 0 not enough money, 1 back in the party, 2 sent to the PC, 3 no room
void PitBoss_BuyBack(void)
{
    struct Pokemon mon;
    u32 i;
    if (!gPitBossSave.heldValid)
    {
        gSpecialVar_Result = 3;
        return;
    }
    if (!IsEnoughMoney(&gSaveBlock1Ptr->money, BUYBACK_PRICE))
    {
        gSpecialVar_Result = 0;
        return;
    }
    BoxMonToMon(&gPitBossHeld, &mon);
    HealPokemon(&mon);
    for (i = 0; i < PARTY_SIZE; i++)
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == SPECIES_NONE)
            break;
    if (i < PARTY_SIZE)
    {
        gParties[B_TRAINER_PLAYER][i] = mon;
        CalculatePlayerPartyCount();
        gSpecialVar_Result = 1;
    }
    else if (CopyMonToPC(&mon) == MON_GIVEN_TO_PC)
    {
        gSpecialVar_Result = 2;
    }
    else
    {
        gSpecialVar_Result = 3;
        return;
    }
    RemoveMoney(&gSaveBlock1Ptr->money, BUYBACK_PRICE);
    gPitBossSave.heldValid = FALSE;
}

// After ChoosePartyMon (VAR_0x8004 = the slot): can it go in? VAR_RESULT:
// 0 yes (STR_VAR_1 his Pokémon, STR_VAR_2 the player's), 1 nothing chosen, 2 an Egg, 3 fainted,
// 4 it's the player's only Pokémon, 5 too strong, 6 too weak
void PitBoss_CheckStake(void)
{
    struct Pokemon *mine, boss;
    struct BossPick p;
    u32 slot = gSpecialVar_0x8004, i, others = 0, mp, bp;

    if (slot >= PARTY_SIZE || GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_SPECIES) == SPECIES_NONE)
    {
        gSpecialVar_Result = 1;
        return;
    }
    mine = &gParties[B_TRAINER_PLAYER][slot];
    GetMonData(mine, MON_DATA_NICKNAME, gStringVar2);
    StringGet_Nickname(gStringVar2);
    if (GetMonData(mine, MON_DATA_IS_EGG))
    {
        gSpecialVar_Result = 2;
        return;
    }
    if (GetMonData(mine, MON_DATA_HP) == 0)
    {
        gSpecialVar_Result = 3;
        return;
    }
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (i != slot && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) != SPECIES_NONE
         && !GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            others++;
    }
    if (others == 0)
    {
        gSpecialVar_Result = 4;
        return;
    }
    PickDay(&p);
    StringCopy(gStringVar1, GetSpeciesName(p.species));
    CreateBossMon(&boss, &p, GetMonData(mine, MON_DATA_LEVEL), FALSE);
    mp = PitBoss_Power(mine);
    bp = PitBoss_Power(&boss);
    if (mp * 100 > bp * (100 + POWER_MARGIN))
        gSpecialVar_Result = 5;
    else if (mp * 100 < bp * (100 - POWER_MARGIN))
        gSpecialVar_Result = 6;
    else
        gSpecialVar_Result = 0;
}

// The deal's made (VAR_0x8004 = the slot): the stake's held (for the save before the battle)
void PitBoss_HoldStake(void)
{
    sStakeSlot = gSpecialVar_0x8004;
    gPitBossSave.stakePersonality = GetMonData(&gParties[B_TRAINER_PLAYER][sStakeSlot], MON_DATA_PERSONALITY);
    gPitBossSave.stakeOtId = GetMonData(&gParties[B_TRAINER_PLAYER][sStakeSlot], MON_DATA_OT_ID);
    gPitBossSave.stakeHeld = TRUE;
    gPitBossSave.forfeited = FALSE;
}

// The battle: the stake alone against his Pokémon of the day, at its level
void PitBoss_SetupFight(void)
{
    struct BossPick p;
    u32 i, level = GetMonData(&gParties[B_TRAINER_PLAYER][sStakeSlot], MON_DATA_LEVEL);
    u16 moves[MAX_MON_MOVES];

    PickDay(&p);
    memcpy(sSavedParty, gParties[B_TRAINER_PLAYER], sizeof(sSavedParty));
    sSavedPartyCount = gPartiesCount[B_TRAINER_PLAYER];
    ZeroPlayerPartyMons();
    gParties[B_TRAINER_PLAYER][0] = sSavedParty[sStakeSlot];
    gPartiesCount[B_TRAINER_PLAYER] = 1;

    memset(&sBossMon, 0, sizeof(sBossMon));
    for (i = 0; i < NUM_STATS; i++)
        sBossEvs[i] = p.evs[i];
    // (TrainerMon's EVs: HP, Atk, Def, SpAtk, SpDef, Speed)
    sBossEvs[3] = p.evs[STAT_SPATK];
    sBossEvs[4] = p.evs[STAT_SPDEF];
    sBossEvs[5] = p.evs[STAT_SPEED];
    sBossMon.ev = sBossEvs;
    sBossMon.iv = TRAINER_PARTY_IVS(MAX_PER_STAT_IVS, MAX_PER_STAT_IVS, MAX_PER_STAT_IVS,
                                    MAX_PER_STAT_IVS, MAX_PER_STAT_IVS, MAX_PER_STAT_IVS);
    sBossMon.species = p.species;
    sBossMon.lvl = level;
    sBossMon.nature = p.nature;
    sBossMon.heldItem = p.item;
    sBossMon.ability = GetAbilityBySpecies(p.species, p.abilityNum);
    sBossMon.gender = TRAINER_MON_RANDOM_GENDER;
    sBossMon.ball = POKEBALL_COUNT;
    sBossMon.friendship = MAX_FRIENDSHIP;
    Pit_PickMoves(p.species, level, moves);
    for (i = 0; i < MAX_MON_MOVES; i++)
        sBossMon.moves[i] = moves[i];
    GymChallenge_SetupGauntlet(TRAINER_CLASS_BOSS_FRLG, TRAINER_PIC_ROCKER_FRLG, FALSE, COMPOUND_STRING("DUKE"), &sBossMon, 1,
                               COMPOUND_STRING("Let's see it."), COMPOUND_STRING("…Huh."),
                               COMPOUND_STRING("…"), COMPOUND_STRING("Mine now."));
    GymChallenge_SetPracticeAI(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_TRY_TO_FAINT | AI_FLAG_CHECK_VIABILITY
                               | AI_FLAG_SMART_SWITCHING | AI_FLAG_OMNISCIENT);
    GymChallenge_SetFormat(GYM_FORMAT_SINGLES);
    sFightActive = TRUE;
}

// After the battle. VAR_RESULT: 0 won (his Pokémon given: VAR_0x8005 = 1 if it went to the PC),
// 1 lost (he keeps the stake), 2 a draw (no deal). STR_VAR_1 his Pokémon, STR_VAR_2 the stake
void PitBoss_EndFight(void)
{
    struct Pokemon stake = gParties[B_TRAINER_PLAYER][0];
    struct BossPick p;

    memcpy(gParties[B_TRAINER_PLAYER], sSavedParty, sizeof(sSavedParty));
    gPartiesCount[B_TRAINER_PLAYER] = sSavedPartyCount;
    gParties[B_TRAINER_PLAYER][sStakeSlot] = stake;           // (as the battle left it: it may have grown)
    sFightActive = FALSE;
    gPitBossSave.stakeHeld = FALSE;

    PickDay(&p);
    StringCopy(gStringVar1, GetSpeciesName(p.species));
    GetMonData(&stake, MON_DATA_NICKNAME, gStringVar2);
    StringGet_Nickname(gStringVar2);
    gSpecialVar_0x8005 = 0;
    if (gBattleOutcome == B_OUTCOME_WON)
    {
        struct Pokemon prize;
        u32 ball = BALL_POKE;
        CreateBossMon(&prize, &p, GetMonData(&stake, MON_DATA_LEVEL), TRUE);
        SetMonData(&prize, MON_DATA_POKEBALL, &ball);
        GetSetPokedexFlag(SpeciesToNationalPokedexNum(p.species), FLAG_SET_SEEN);
        GetSetPokedexFlag(SpeciesToNationalPokedexNum(p.species), FLAG_SET_CAUGHT);
        gSpecialVar_0x8005 = (GiveCapturedMonToPlayer(&prize) == MON_GIVEN_TO_PC);
        FlagSet(FLAG_PIT_BOSS_BEATEN);
        gSpecialVar_Result = 0;
    }
    else if (gBattleOutcome == B_OUTCOME_DREW)
    {
        gSpecialVar_Result = 2;
    }
    else
    {
        Hold(&stake.box);
        ZeroMonData(&gParties[B_TRAINER_PLAYER][sStakeSlot]);
        CompactPartySlots();
        CalculatePlayerPartyCount();
        gSpecialVar_Result = 1;
    }
}

bool32 PitBoss_FightActive(void)
{
    return sFightActive;
}

// The pit's and the back room's saves (data/scripts/pit.inc Pit_EventScript_AutoSave)
void Pit_AutoSave(void)
{
    TrySavingData(SAVE_NORMAL);
}
