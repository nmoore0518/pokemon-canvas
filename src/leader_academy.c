// Pokémon Canvas: the Leader Academy, north of Route 11 (maps: tools/gym_hack/build_academy.py;
// scripts: data/scripts/leader_academy.inc; battle data: src/data/academy.h, made by
// tools/gym_hack/build_academy_data.py).
//
// A school for Gym Leader candidates from every region, opened to the player after their third badge
// (their mentor walks them there). Everything is singles.
//   - Battle Points (the frontier's BP, gSaveBlock2Ptr->frontier.battlePoints) come from practice:
//     the candidates (the player's own team, levelled to them: unlimited), the instructors'
//     demonstrations and the Battle Hall's streaks (both with level-100 rental teams).
//   - The Exchange sells the competitive items for BP; its other counter, and the Supply shop,
//     sell them for money. BP is never the only way.
//   - Rental battles back up the player's party (EWRAM) and put it back afterwards: the whole run
//     happens inside one locked script, so the game can't be saved with the rentals in the party.
#include "global.h"
#include "leader_academy.h"
#include "battle.h"
#include "data.h"
#include "event_data.h"
#include "gym_candidate.h"
#include "gym_challenge.h"
#include "item.h"
#include "list_menu.h"
#include "malloc.h"
#include "pokemon.h"
#include "random.h"
#include "script_menu.h"
#include "string_util.h"
#include "strings.h"
#include "constants/battle_ai.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/trainers.h"

struct AcademyTeacher
{
    const u8 *name;
    u16 pic;
    bool8 female;
    const struct TrainerMon *party;     // (3)
};

struct AcademyCandidate
{
    const u8 *name;
    const u8 *region;
    u8 type;
    u8 skill;                           // 0 rookie .. 3 expert
    u16 pic;
    bool8 female;
    u16 species[PARTY_SIZE];            // (base forms: evolved for the level)
};

#include "data/academy.h"

#define RENTAL_TEAM   3
#define RENTAL_OFFER  6
#define BP_MAX        9999

// ---------------------------------------------------------------------------
// Battle Points
// ---------------------------------------------------------------------------
static u32 GetBP(void)
{
    return gSaveBlock2Ptr->frontier.battlePoints;
}

static void AddBP(u32 n)
{
    gSaveBlock2Ptr->frontier.battlePoints = min(GetBP() + n, (u32)BP_MAX);
}

// Special: STR_VAR_1 = the player's BP, VAR_RESULT too
void Academy_BufferBP(void)
{
    gSpecialVar_Result = GetBP();
    ConvertIntToDecimalStringN(gStringVar1, GetBP(), STR_CONV_MODE_LEFT_ALIGN, 4);
}

// Special: VAR_0x8004 BP more. STR_VAR_1 = how many, STR_VAR_2 = the total
void Academy_AddBP(void)
{
    AddBP(gSpecialVar_0x8004);
    ConvertIntToDecimalStringN(gStringVar1, gSpecialVar_0x8004, STR_CONV_MODE_LEFT_ALIGN, 4);
    ConvertIntToDecimalStringN(gStringVar2, GetBP(), STR_CONV_MODE_LEFT_ALIGN, 4);
}

// ---------------------------------------------------------------------------
// The Exchange's BP counter (its other counter is an ordinary mart: the same items for money)
// ---------------------------------------------------------------------------
struct BPItem { u16 item; u16 bp; };

static const struct BPItem sBPItems[] =
{
    {ITEM_CHOICE_SCARF, 32}, {ITEM_CHOICE_BAND, 32}, {ITEM_CHOICE_SPECS, 32},
    {ITEM_LIFE_ORB, 24}, {ITEM_FOCUS_SASH, 24}, {ITEM_ASSAULT_VEST, 24}, {ITEM_EVIOLITE, 24},
    {ITEM_BOOSTER_ENERGY, 24}, {ITEM_LOADED_DICE, 20}, {ITEM_WEAKNESS_POLICY, 20},
    {ITEM_ROCKY_HELMET, 20}, {ITEM_HEAVY_DUTY_BOOTS, 20}, {ITEM_LEFTOVERS, 16}, {ITEM_EXPERT_BELT, 16},
    {ITEM_COVERT_CLOAK, 16}, {ITEM_CLEAR_AMULET, 16}, {ITEM_BLACK_SLUDGE, 12}, {ITEM_LIGHT_CLAY, 12},
    {ITEM_AIR_BALLOON, 12}, {ITEM_RED_CARD, 12}, {ITEM_EJECT_BUTTON, 12}, {ITEM_EJECT_PACK, 12},
    {ITEM_WHITE_HERB, 12}, {ITEM_MENTAL_HERB, 12}, {ITEM_POWER_HERB, 12}, {ITEM_TOXIC_ORB, 12},
    {ITEM_FLAME_ORB, 12}, {ITEM_DAMP_ROCK, 12}, {ITEM_HEAT_ROCK, 12}, {ITEM_SMOOTH_ROCK, 12},
    {ITEM_ICY_ROCK, 12}, {ITEM_LUM_BERRY, 4}, {ITEM_SITRUS_BERRY, 4},
    {ITEM_ABILITY_PATCH, 32}, {ITEM_GOLD_BOTTLE_CAP, 16}, {ITEM_BOTTLE_CAP, 4}, {ITEM_ABILITY_CAPSULE, 8},
    {ITEM_MULTIVITAMIN, 12}, {ITEM_CILANTRO, 16},
    {ITEM_ADAMANT_MINT, 4}, {ITEM_JOLLY_MINT, 4}, {ITEM_MODEST_MINT, 4}, {ITEM_TIMID_MINT, 4},
    {ITEM_BOLD_MINT, 4}, {ITEM_IMPISH_MINT, 4}, {ITEM_CALM_MINT, 4}, {ITEM_CAREFUL_MINT, 4},
};

// Special (before dynmultistack): the BP counter's list, "ITEM  nn BP"
void Academy_PushExchange(void)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sBPItems); i++)
    {
        struct ListMenuItem entry;
        u8 *name = Alloc(32), *end;
        end = CopyItemName(sBPItems[i].item, name);
        while (end - name < 15)
            *end++ = CHAR_SPACE;
        end = ConvertIntToDecimalStringN(end, sBPItems[i].bp, STR_CONV_MODE_RIGHT_ALIGN, 2);
        StringCopy(end, COMPOUND_STRING(" BP"));
        entry.name = name;
        entry.id = i;
        MultichoiceDynamic_PushElement(entry);
    }
}

// Special: VAR_0x8004 = the entry -> VAR_0x8005 = the item, VAR_0x8006 = its BP; STR_VAR_1/2 = them
void Academy_BufferExchangeItem(void)
{
    const struct BPItem *e = &sBPItems[min(gSpecialVar_0x8004, ARRAY_COUNT(sBPItems) - 1)];
    gSpecialVar_0x8005 = e->item;
    gSpecialVar_0x8006 = e->bp;
    CopyItemName(e->item, gStringVar1);
    ConvertIntToDecimalStringN(gStringVar2, e->bp, STR_CONV_MODE_LEFT_ALIGN, 2);
}

// Special: VAR_RESULT = whether VAR_0x8006 BP could be paid (and, if so, they are)
void Academy_SpendBP(void)
{
    gSpecialVar_Result = (GetBP() >= gSpecialVar_0x8006);
    if (gSpecialVar_Result)
        gSaveBlock2Ptr->frontier.battlePoints -= gSpecialVar_0x8006;
}

// ---------------------------------------------------------------------------
// The mentor's escort (Route 11 and the campus gate)
// ---------------------------------------------------------------------------
// Special: VAR_OBJ_GFX_ID_0 = the mentor's look, STR_VAR_1 = their name (Prof. Oak, if no mentor)
void Academy_PrepareMentor(void)
{
    GymCandidate_PrepareMentorVisit();
    if (!gSpecialVar_Result)
    {
        VarSet(VAR_OBJ_GFX_ID_0, OBJ_EVENT_GFX_PROF_OAK);
        StringCopy(gStringVar1, COMPOUND_STRING("PROF. OAK"));
    }
}

// Special: VAR_RESULT = whether the invitation is due (the third badge; not yet admitted)
bool8 Academy_InvitationDue(void)
{
    return FlagGet(FLAG_BADGE03_GET) && VarGet(VAR_ACADEMY_STATE) == ACADEMY_NONE;
}

// ---------------------------------------------------------------------------
// Candidates: the player's own team against theirs, levelled to the player
// ---------------------------------------------------------------------------
static EWRAM_DATA struct TrainerMon sAcademyParty[PARTY_SIZE] = {0};

static const s8 sSkillOffset[] = {-4, -2, 1, 3};
static const u8 sSkillIV[] = {8, 18, 28, 31};
static const u8 sSkillBP[] = {1, 2, 3, 4};
static const u8 *const sSkillNames[] = {
    COMPOUND_STRING("ROOKIE"), COMPOUND_STRING("SKILLED"), COMPOUND_STRING("ADVANCED"), COMPOUND_STRING("EXPERT")};
static const u16 sCandidateItems[] = {ITEM_SITRUS_BERRY, ITEM_LEFTOVERS, ITEM_LIFE_ORB, ITEM_FOCUS_SASH,
                                      ITEM_LUM_BERRY, ITEM_EXPERT_BELT};

static const u8 *const sCandidateIntros[] =
{
    [0] = COMPOUND_STRING("Go easy on me, okay?\nI'm still learning!"),
    [1] = COMPOUND_STRING("A practice match?\nYou're on!"),
    [2] = COMPOUND_STRING("Let's see what KANTO's\ncandidates are made of."),
    [3] = COMPOUND_STRING("I don't do practice.\nEvery battle is the real one."),
};
static const u8 *const sCandidateDefeats[] =
{
    COMPOUND_STRING("Wow… I've got a lot\nto learn!"),
    COMPOUND_STRING("Good match!\nI'll get you next time!"),
    COMPOUND_STRING("Hmph. Well played."),
    COMPOUND_STRING("…Again. Right now."),
};

// Special: VAR_0x8004 = the candidate. Their battle is set up (TRAINER_GYM_CHALLENGER);
// STR_VAR_1 = their name, STR_VAR_2 = their region, STR_VAR_3 = their skill;
// VAR_0x8005 = the BP a win pays
void Academy_SetupCandidate(void)
{
    const struct AcademyCandidate *c = &sAcademyCandidates[min(gSpecialVar_0x8004, ARRAY_COUNT(sAcademyCandidates) - 1)];
    s32 level = (s32)GymChallenge_PlayerAverageLevel() + sSkillOffset[c->skill];
    u32 size = 3 + c->skill / 2 + (level >= 30) + (level >= 45), i;
    u64 ai = (c->skill == 0) ? AI_FLAG_BASIC_TRAINER
           : (c->skill == 3) ? (AI_FLAG_SMART_TRAINER | AI_FLAG_PREDICTION) : AI_FLAG_SMART_TRAINER;

    level = max(min(level, MAX_LEVEL), 5);
    size = min(size, (u32)PARTY_SIZE);
    for (i = 0; i < size; i++)
    {
        struct TrainerMon *m = &sAcademyParty[i];
        u32 iv = sSkillIV[c->skill];
        memset(m, 0, sizeof(*m));
        m->species = GymChallenge_EvolveForLevel(c->species[i], level);
        m->lvl = level;
        m->iv = TRAINER_PARTY_IVS(iv, iv, iv, iv, iv, iv);
        m->nature = Random() % NUM_NATURES;
        m->gender = TRAINER_MON_RANDOM_GENDER;
        m->ball = POKEBALL_COUNT;
        m->friendship = MAX_FRIENDSHIP;
        if (c->skill >= 2 || (c->skill == 1 && i == 0))
            m->heldItem = sCandidateItems[(i + c->type) % ARRAY_COUNT(sCandidateItems)];
    }
    GymChallenge_SetupGauntlet(TRAINER_CLASS_CANDIDATE, c->pic, c->female, c->name, sAcademyParty, size,
                               sCandidateIntros[c->skill], sCandidateDefeats[c->skill],
                               sCandidateDefeats[c->skill], COMPOUND_STRING("Good battle!"));
    GymChallenge_SetPracticeAI(ai);
    StringCopy(gStringVar1, c->name);
    StringCopy(gStringVar2, c->region);
    StringCopy(gStringVar3, sSkillNames[c->skill]);
    gSpecialVar_0x8005 = sSkillBP[c->skill];
}

// ---------------------------------------------------------------------------
// Rental battles (the Battle Hall's streaks; the instructors' demonstrations)
// ---------------------------------------------------------------------------
static EWRAM_DATA struct Pokemon sSavedParty[PARTY_SIZE] = {0};
static EWRAM_DATA u8 sSavedPartyCount = 0;
static EWRAM_DATA bool8 sRenting = FALSE;
static EWRAM_DATA u8 sOffer[RENTAL_OFFER] = {0};
static EWRAM_DATA u8 sTaken[RENTAL_TEAM] = {0};
static EWRAM_DATA u8 sTakenCount = 0;
static EWRAM_DATA u16 sStreak = 0;

static bool32 InList(const u8 *list, u32 n, u32 value)
{
    u32 i;
    for (i = 0; i < n; i++)
        if (list[i] == value)
            return TRUE;
    return FALSE;
}

static void CreateRentalMon(struct Pokemon *mon, const struct TrainerMon *set)
{
    static const u8 sEvStat[] = {MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPATK_EV,
                                 MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV};
    u32 i, item = set->heldItem, nature = set->nature;
    u8 abilityNum = 0, iv = MAX_PER_STAT_IVS;

    CreateMon(mon, set->species, set->lvl, Random32(), OTID_STRUCT_PLAYER_ID);
    for (i = 0; i < NUM_STATS; i++)
    {
        u8 ev = set->ev != NULL ? set->ev[i] : 0;
        SetMonData(mon, MON_DATA_HP_IV + i, &iv);
        SetMonData(mon, sEvStat[i], &ev);
    }
    SetMonData(mon, MON_DATA_HIDDEN_NATURE, &nature);
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    for (i = 0; i < NUM_ABILITY_SLOTS; i++)
    {
        if (set->ability != ABILITY_NONE && GetAbilityBySpecies(set->species, i) == set->ability)
        {
            abilityNum = i;
            break;
        }
    }
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilityNum);
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(mon, set->moves[i], i);
    CalculateMonStats(mon);
}

// Special: the player's party is put aside (and the rental team starts empty)
void Academy_RentalBegin(void)
{
    u32 i, j;
    if (!sRenting)
    {
        memcpy(sSavedParty, gParties[B_TRAINER_PLAYER], sizeof(sSavedParty));
        sSavedPartyCount = gPartiesCount[B_TRAINER_PLAYER];
        sRenting = TRUE;
    }
    ZeroPlayerPartyMons();
    gPartiesCount[B_TRAINER_PLAYER] = 0;
    sTakenCount = 0;
    sStreak = 0;
    for (i = 0; i < RENTAL_OFFER; i++)
    {
        do
            j = Random() % ACADEMY_RENTAL_COUNT;
        while (InList(sOffer, i, j));
        sOffer[i] = j;
    }
}

// Special: the player's own party is back
void Academy_RentalEnd(void)
{
    if (!sRenting)
        return;
    memcpy(gParties[B_TRAINER_PLAYER], sSavedParty, sizeof(sSavedParty));
    gPartiesCount[B_TRAINER_PLAYER] = sSavedPartyCount;
    sRenting = FALSE;
}

// Special (before dynmultistack): the offered rentals not taken yet, "GARCHOMP @ CHOICE SCARF"
void Academy_PushRentals(void)
{
    u32 i;
    for (i = 0; i < RENTAL_OFFER; i++)
    {
        const struct TrainerMon *set = &sAcademyRentals[sOffer[i]];
        struct ListMenuItem entry;
        u8 *name, *end;
        if (InList(sTaken, sTakenCount, sOffer[i]))
            continue;
        name = Alloc(40);
        end = StringCopy(name, GetSpeciesName(set->species));
        if (set->heldItem != ITEM_NONE)
        {
            end = StringCopy(end, COMPOUND_STRING(" · "));
            CopyItemName(set->heldItem, end);
        }
        entry.name = name;
        entry.id = i;
        MultichoiceDynamic_PushElement(entry);
    }
}

// Special: VAR_0x8004 = an offer -> STR_VAR_1 = "SPECIES @ ITEM", STR_VAR_2/3 = its moves, two by two
void Academy_BufferRental(void)
{
    const struct TrainerMon *set = &sAcademyRentals[sOffer[min(gSpecialVar_0x8004, RENTAL_OFFER - 1)]];
    u8 *end = StringCopy(gStringVar1, GetSpeciesName(set->species));
    u32 i;
    if (set->heldItem != ITEM_NONE)
    {
        end = StringCopy(end, COMPOUND_STRING(" · "));
        CopyItemName(set->heldItem, end);
    }
    for (i = 0; i < 2; i++)
    {
        u8 *dst = i == 0 ? gStringVar2 : gStringVar3;
        end = StringCopy(dst, GetMoveName(set->moves[i * 2]));
        end = StringCopy(end, COMPOUND_STRING(" / "));
        StringCopy(end, GetMoveName(set->moves[i * 2 + 1]));
    }
}

// Special: VAR_0x8004 = an offer, taken into the rental team. VAR_RESULT = how many there are now
void Academy_TakeRental(void)
{
    u32 offer = sOffer[min(gSpecialVar_0x8004, RENTAL_OFFER - 1)];
    if (sTakenCount < RENTAL_TEAM && !InList(sTaken, sTakenCount, offer))
    {
        CreateRentalMon(&gParties[B_TRAINER_PLAYER][sTakenCount], &sAcademyRentals[offer]);
        sTaken[sTakenCount++] = offer;
        gPartiesCount[B_TRAINER_PLAYER] = sTakenCount;
    }
    gSpecialVar_Result = sTakenCount;
}

static void SetupRentalOpponent(u32 trainerClass, u32 pic, bool32 female, const u8 *name,
                                const struct TrainerMon *party, u64 ai, const u8 *intro, const u8 *defeat)
{
    GymChallenge_SetupGauntlet(trainerClass, pic, female, name, party, RENTAL_TEAM, intro, defeat, defeat,
                               COMPOUND_STRING("Good battle!"));
    GymChallenge_SetPracticeAI(ai);
}

// Special: the Battle Hall's next battle: a candidate (at random) with 3 rentals the player doesn't
// have. STR_VAR_1 = their name, STR_VAR_2 = which battle of the streak it is
void Academy_SetupHallBattle(void)
{
    const struct AcademyCandidate *c = &sAcademyCandidates[Random() % ARRAY_COUNT(sAcademyCandidates)];
    u8 picked[RENTAL_TEAM];
    u32 i, j;
    u64 ai = sStreak >= 6 ? (AI_FLAG_SMART_TRAINER | AI_FLAG_PREDICTION)
           : sStreak >= 2 ? AI_FLAG_SMART_TRAINER : AI_FLAG_BASIC_TRAINER;
    for (i = 0; i < RENTAL_TEAM; i++)
    {
        do
            j = Random() % ACADEMY_RENTAL_COUNT;
        while (InList(picked, i, j) || InList(sTaken, sTakenCount, j));
        picked[i] = j;
        sAcademyParty[i] = sAcademyRentals[j];
    }
    SetupRentalOpponent(TRAINER_CLASS_CANDIDATE, c->pic, c->female, c->name, sAcademyParty, ai,
                        COMPOUND_STRING("Rental rules: let's see\nwho reads the match better!"),
                        COMPOUND_STRING("Your rentals, my rentals…\nand you still won."));
    StringCopy(gStringVar1, c->name);
    ConvertIntToDecimalStringN(gStringVar2, sStreak + 1, STR_CONV_MODE_LEFT_ALIGN, 3);
}

// Special: a Battle Hall win. VAR_0x8004 = the BP it pays (1, +1 every 3 in a row, +5 at 7),
// VAR_RESULT = the streak now; STR_VAR_2 = the streak
void Academy_HallWin(void)
{
    sStreak++;
    gSpecialVar_0x8004 = 1 + sStreak / 3 + (sStreak == 7 ? 5 : 0);
    gSpecialVar_Result = sStreak;
    ConvertIntToDecimalStringN(gStringVar2, sStreak, STR_CONV_MODE_LEFT_ALIGN, 3);
}

// Special: VAR_0x8004 = an instructor (0..7): their demonstration, with their 3 sets.
// STR_VAR_1 = their name
void Academy_SetupTeacher(void)
{
    const struct AcademyTeacher *t = &sAcademyTeachers[min(gSpecialVar_0x8004, ARRAY_COUNT(sAcademyTeachers) - 1)];
    memcpy(sAcademyParty, t->party, sizeof(struct TrainerMon) * RENTAL_TEAM);
    SetupRentalOpponent(TRAINER_CLASS_INSTRUCTOR, t->pic, t->female, t->name, sAcademyParty,
                        AI_FLAG_SMART_TRAINER | AI_FLAG_PREDICTION,
                        COMPOUND_STRING("Class is in session!"),
                        COMPOUND_STRING("Excellent! You were\nlistening after all."));
    StringCopy(gStringVar1, t->name);
}
