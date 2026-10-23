// Gym Trainer hack: the Gym Leader Convention, in Goldenrod's Radio Tower.
//
// Every Gym Leader, Elite Four member and Champion (generations 1-9) is here, in one hall per type
// plus the Champions' Hall. The receptionist in the lobby sends the player to any hall; talking to
// a trainer there starts a battle with their full team (data: src/data/gym_convention.h, made by
// tools/gym_hack/build_convention.py), after which the player's party is healed. Beating everyone
// in a hall wins its trophy (a gym decoration), and clearing every hall wins the Grand Trophy.
// Whom the player has beaten is saved with the gym challenge records (struct GymChallengeSave).
// Scripts: data/scripts/gym_convention.inc. The halls: tools/gym_hack/convention_halls.py.
#include "global.h"
#include "gym_convention.h"
#include "gym_challenge.h"
#include "data.h"
#include "pokeball.h"
#include "gym_decor.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "list_menu.h"
#include "malloc.h"
#include "overworld.h"
#include "script_menu.h"
#include "string_util.h"
#include "title_card.h"
#include "constants/rgb.h"
#include "constants/characters.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/gym_convention.h"
#include "constants/gym_decor.h"
#include "constants/gym_hack.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/trainers.h"

struct ConventionTrainer
{
    const u8 *name;             // NULL: the rival (named by the player)
    u8 trainerClass;
    bool8 female;
    u8 hall;
    const struct TrainerMon *party;
    const u8 *intro;
    const u8 *defeat;
    const u8 *after;
};

struct ConventionHall
{
    s8 mapGroup;
    s8 mapNum;
    s8 warpId;
};

#include "data/gym_convention.h"
#include "data/gym_convention_sprites.h"
#include "data/gym_convention_halls.h"
#include "data/gym_convention_cards.h"

STATIC_ASSERT(CONVENTION_TRAINER_COUNT <= 8 * sizeof(((struct GymChallengeSave *)0)->conventionBeaten), ConventionBitsFit);

static const u8 *const sHallNames[CONVENTION_HALL_COUNT] =
{
    [CONVENTION_HALL_NORMAL] = COMPOUND_STRING("NORMAL"),
    [CONVENTION_HALL_FIRE] = COMPOUND_STRING("FIRE"),
    [CONVENTION_HALL_WATER] = COMPOUND_STRING("WATER"),
    [CONVENTION_HALL_GRASS] = COMPOUND_STRING("GRASS"),
    [CONVENTION_HALL_ELECTRIC] = COMPOUND_STRING("ELECTRIC"),
    [CONVENTION_HALL_ICE] = COMPOUND_STRING("ICE"),
    [CONVENTION_HALL_FIGHTING] = COMPOUND_STRING("FIGHTING"),
    [CONVENTION_HALL_POISON] = COMPOUND_STRING("POISON"),
    [CONVENTION_HALL_GROUND] = COMPOUND_STRING("GROUND"),
    [CONVENTION_HALL_FLYING] = COMPOUND_STRING("FLYING"),
    [CONVENTION_HALL_PSYCHIC] = COMPOUND_STRING("PSYCHIC"),
    [CONVENTION_HALL_BUG] = COMPOUND_STRING("BUG"),
    [CONVENTION_HALL_ROCK] = COMPOUND_STRING("ROCK"),
    [CONVENTION_HALL_GHOST] = COMPOUND_STRING("GHOST"),
    [CONVENTION_HALL_DRAGON] = COMPOUND_STRING("DRAGON"),
    [CONVENTION_HALL_DARK] = COMPOUND_STRING("DARK"),
    [CONVENTION_HALL_STEEL] = COMPOUND_STRING("STEEL"),
    [CONVENTION_HALL_FAIRY] = COMPOUND_STRING("FAIRY"),
    [CONVENTION_HALL_CHAMPIONS] = COMPOUND_STRING("CHAMPIONS"),
};

static const u16 sHallTrophies[CONVENTION_HALL_COUNT] =
{
    [CONVENTION_HALL_NORMAL] = GYM_DECOR_TROPHY_NORMAL,
    [CONVENTION_HALL_FIRE] = GYM_DECOR_TROPHY_FIRE,
    [CONVENTION_HALL_WATER] = GYM_DECOR_TROPHY_WATER,
    [CONVENTION_HALL_GRASS] = GYM_DECOR_TROPHY_GRASS,
    [CONVENTION_HALL_ELECTRIC] = GYM_DECOR_TROPHY_ELECTRIC,
    [CONVENTION_HALL_ICE] = GYM_DECOR_TROPHY_ICE,
    [CONVENTION_HALL_FIGHTING] = GYM_DECOR_TROPHY_FIGHTING,
    [CONVENTION_HALL_POISON] = GYM_DECOR_TROPHY_POISON,
    [CONVENTION_HALL_GROUND] = GYM_DECOR_TROPHY_GROUND,
    [CONVENTION_HALL_FLYING] = GYM_DECOR_TROPHY_FLYING,
    [CONVENTION_HALL_PSYCHIC] = GYM_DECOR_TROPHY_PSYCHIC,
    [CONVENTION_HALL_BUG] = GYM_DECOR_TROPHY_BUG,
    [CONVENTION_HALL_ROCK] = GYM_DECOR_TROPHY_ROCK,
    [CONVENTION_HALL_GHOST] = GYM_DECOR_TROPHY_GHOST,
    [CONVENTION_HALL_DRAGON] = GYM_DECOR_TROPHY_DRAGON,
    [CONVENTION_HALL_DARK] = GYM_DECOR_TROPHY_DARK,
    [CONVENTION_HALL_STEEL] = GYM_DECOR_TROPHY_STEEL,
    [CONVENTION_HALL_FAIRY] = GYM_DECOR_TROPHY_FAIRY,
    [CONVENTION_HALL_CHAMPIONS] = GYM_DECOR_TROPHY_CHAMPIONS,
};

// The halls' colors (the title card's band)
static const u16 sHallColors[CONVENTION_HALL_COUNT] =
{
    [CONVENTION_HALL_NORMAL] = RGB(20, 19, 14),   [CONVENTION_HALL_FIRE] = RGB(29, 12, 4),
    [CONVENTION_HALL_WATER] = RGB(7, 14, 29),     [CONVENTION_HALL_GRASS] = RGB(9, 22, 8),
    [CONVENTION_HALL_ELECTRIC] = RGB(28, 22, 2),  [CONVENTION_HALL_ICE] = RGB(12, 24, 27),
    [CONVENTION_HALL_FIGHTING] = RGB(23, 6, 5),   [CONVENTION_HALL_POISON] = RGB(19, 8, 21),
    [CONVENTION_HALL_GROUND] = RGB(25, 18, 8),    [CONVENTION_HALL_FLYING] = RGB(15, 16, 29),
    [CONVENTION_HALL_PSYCHIC] = RGB(29, 9, 15),   [CONVENTION_HALL_BUG] = RGB(17, 21, 3),
    [CONVENTION_HALL_ROCK] = RGB(20, 17, 9),      [CONVENTION_HALL_GHOST] = RGB(12, 9, 19),
    [CONVENTION_HALL_DRAGON] = RGB(11, 7, 29),    [CONVENTION_HALL_DARK] = RGB(10, 8, 7),
    [CONVENTION_HALL_STEEL] = RGB(15, 17, 20),    [CONVENTION_HALL_FAIRY] = RGB(28, 14, 22),
    [CONVENTION_HALL_CHAMPIONS] = RGB(27, 21, 6),
};

#define CONVENTION_MAGIC 0x4C47   // "GL"

// A convention trainer's overworld sprite
u32 Convention_GetGraphicsId(u32 index)
{
    return sConventionGfx[index < CONVENTION_TRAINER_COUNT ? index : 0];
}

static u8 *BeatenBits(void)
{
    struct GymChallengeSave *save = GymChallenge_GetSave();
    if (save->conventionMagic != CONVENTION_MAGIC)     // (a save from before the convention)
    {
        memset(save->conventionBeaten, 0, sizeof(save->conventionBeaten));
        save->conventionMagic = CONVENTION_MAGIC;
    }
    return save->conventionBeaten;
}

bool32 Convention_IsBeaten(u32 index)
{
    return index < CONVENTION_TRAINER_COUNT && (BeatenBits()[index / 8] >> (index % 8)) & 1;
}

static u32 CountInHall(u32 hall, bool32 beatenOnly)
{
    u32 i, n = 0;
    for (i = 0; i < CONVENTION_TRAINER_COUNT; i++)
    {
        if (sConventionTrainers[i].hall == hall && (!beatenOnly || Convention_IsBeaten(i)))
            n++;
    }
    return n;
}

static bool32 IsHallCleared(u32 hall)
{
    return CountInHall(hall, TRUE) == CountInHall(hall, FALSE);
}

static const u8 *TrainerName(u32 index)
{
    return sConventionTrainers[index].name ? sConventionTrainers[index].name : gSaveBlock1Ptr->rivalName;
}

// The first convention trainer named `name`, set up as the gym challenger with their full team
// (the player's mentor, for a streak's mastery battle: src/gym_challenge.c). Their index, or -1.
s32 Convention_SetupByName(const u8 *name)
{
    u32 index, n;
    for (index = 0; index < CONVENTION_TRAINER_COUNT; index++)
    {
        const struct ConventionTrainer *t = &sConventionTrainers[index];
        if (t->name == NULL || StringCompare(t->name, name) != 0)
            continue;
        for (n = 0; n < PARTY_SIZE && t->party[n].species != SPECIES_NONE; n++)
            ;
        GymChallenge_SetupFixed(t->trainerClass, sConventionPics[index], t->female, t->name, t->party, n);
        return index;
    }
    return -1;
}

// Special: VAR_0x8004 = trainer. Sets up their battle and lines; STR_VAR_1 = their name.
// VAR_RESULT: 0 not beaten yet, 1 beaten (no rematches until their whole hall is cleared: the
// player can see who's left), 2 beaten and the hall cleared (a rematch).
void Convention_SetupTrainer(void)
{
    u32 index = gSpecialVar_0x8004, n;
    const struct ConventionTrainer *t;
    if (index >= CONVENTION_TRAINER_COUNT)
        index = 0;
    t = &sConventionTrainers[index];
    for (n = 0; n < PARTY_SIZE && t->party[n].species != SPECIES_NONE; n++)
        ;
    GymChallenge_SetupFixed(t->trainerClass, sConventionPics[index], t->female, TrainerName(index), t->party, n);
    StringCopy(gStringVar1, TrainerName(index));
    StringExpandPlaceholders(gGymChallengeIntroText, t->intro);
    StringExpandPlaceholders(gGymChallengeDefeatText, t->defeat);
    StringExpandPlaceholders(gGymChallengeAfterText, t->after);
    if (!Convention_IsBeaten(index))
        gSpecialVar_Result = 0;
    else if (!IsHallCleared(t->hall))
        gSpecialVar_Result = 1;
    else
        gSpecialVar_Result = 2;
}

// Special: VAR_0x8004 = the trainer just beaten. VAR_RESULT: 0, or CONVENTION_WON_HALL (their
// hall is now cleared: STR_VAR_2 = the hall, its trophy is in the gym's storage), or
// CONVENTION_WON_ALL (and that was the last hall: the Grand Trophy too)
void Convention_RecordWin(void)
{
    u32 index = gSpecialVar_0x8004, hall, i;
    gSpecialVar_Result = 0;
    if (index >= CONVENTION_TRAINER_COUNT || Convention_IsBeaten(index))
        return;
    BeatenBits()[index / 8] |= 1 << (index % 8);
    hall = sConventionTrainers[index].hall;
    if (!IsHallCleared(hall))
        return;
    StringCopy(gStringVar2, sHallNames[hall]);
    FlagSet(FLAG_CONVENTION_TROPHIES + hall);
    GymDecor_Give(sHallTrophies[hall]);
    gSpecialVar_Result = CONVENTION_WON_HALL;
    for (i = 0; i < CONVENTION_HALL_COUNT; i++)
    {
        if (!IsHallCleared(i))
            return;
    }
    FlagSet(FLAG_CONVENTION_TROPHY_GRAND);
    GymDecor_Give(GYM_DECOR_TROPHY_GRAND);
    gSpecialVar_Result = CONVENTION_WON_ALL;
}

// Special: the receptionist's list of halls, each with how many there the player has beaten
void Convention_PushHalls(void)
{
    u32 hall;
    for (hall = 0; hall < CONVENTION_HALL_COUNT; hall++)
    {
        struct ListMenuItem item;
        u8 *name = Alloc(24), *end;
        end = StringCopy(name, sHallNames[hall]);
        if (IsHallCleared(hall))
        {
            end = StringAppend(end, COMPOUND_STRING(" {STAR}"));
        }
        else
        {
            *end++ = CHAR_SPACE;
            end = ConvertIntToDecimalStringN(end, CountInHall(hall, TRUE), STR_CONV_MODE_LEFT_ALIGN, 2);
            *end++ = CHAR_SLASH;
            end = ConvertIntToDecimalStringN(end, CountInHall(hall, FALSE), STR_CONV_MODE_LEFT_ALIGN, 2);
        }
        item.name = name;
        item.id = hall;
        MultichoiceDynamic_PushElement(item);
    }
}

// Special: STR_VAR_1 = how many halls are cleared; VAR_RESULT = that many
void Convention_BufferProgress(void)
{
    u32 hall, n = 0;
    for (hall = 0; hall < CONVENTION_HALL_COUNT; hall++)
        n += IsHallCleared(hall);
    ConvertIntToDecimalStringN(gStringVar1, n, STR_CONV_MODE_LEFT_ALIGN, 2);
    gSpecialVar_Result = n;
}

// Special: VAR_0x8004 = hall. Warps there (the script then waitstates)
void Convention_WarpToHall(void)
{
    const struct ConventionHall *h = &sConventionHalls[gSpecialVar_0x8004 % CONVENTION_HALL_COUNT];
    SetWarpDestination(h->mapGroup, h->mapNum, h->warpId, -1, -1);
    DoWarp();
    ResetInitialPlayerAvatarState();
}

// Special (before a convention battle; the script has faded to black, then waitstates):
// VAR_0x8004 = trainer. Their title card: who they are in the Pokémon world
static EWRAM_DATA u8 sCardHeader[48] = {0};

void Convention_ShowTitleCard(void)
{
    u32 index = gSpecialVar_0x8004 < CONVENTION_TRAINER_COUNT ? gSpecialVar_0x8004 : 0;
    u32 hall = sConventionTrainers[index].hall;
    u8 *end = StringCopy(sCardHeader, COMPOUND_STRING("GYM LEADER CONVENTION  ·  "));
    end = StringAppend(end, sHallNames[hall]);
    StringAppend(end, COMPOUND_STRING(" HALL"));
    TitleCard_Show(sConventionPics[index], sHallColors[hall], sCardHeader, TrainerName(index),
                   sConventionCards[index][0], sConventionCards[index][1], sConventionCards[index][2]);
}
