// Pokémon Canvas: MissingNo., found the way Red and Blue found it.
//
// Watch the old man in Viridian City show how to catch a POKéMON, go straight to Cinnabar Island
// (fly, or the balloon) and surf down its east coast: there, one wild POKéMON in five is MissingNo.
// In RB the demo copied the old man's name over the buffer the next grass area's encounters were
// read from; Cinnabar's coast, having no grass, read that name as its table. Here:
// - the demo arms the coast (MissingNo_ArmEncounter, from the old man's script); the state is in RAM
//   only, like RB's buffer.
// - a map with grass (land encounters) disarms it, as walking through one rewrote RB's buffer.
// - on the coast column (Cinnabar's x 23, y 0-15) it's armed: the encounter rate is the grass rate it
//   would have read, and a fifth of the coast's encounters are MissingNo.; once off those tiles it's
//   reset.
// - its level: from the player's name, as RB's levels came from the name's 2nd, 4th... letters, read
//   in RB's character set (capitals 128+, the terminator 80); Gen 3 has no level 0, so 1-100.
// No items are duplicated. Its moves are always Water Gun, Water Gun and Sky Attack.
#include "global.h"
#include "missingno.h"
#include "event_data.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "pokemon.h"
#include "random.h"
#include "wild_encounter.h"
#include "constants/map_groups.h"
#include "constants/characters.h"
#include "constants/moves.h"
#include "constants/species.h"

enum { MISSINGNO_NONE, MISSINGNO_ARMED, MISSINGNO_ON_COAST };

#define COAST_X     23
#define COAST_Y_MAX 15
#define COAST_RATE  20      // (out of 180, as grass rates go: a little over one step in ten)
#define COAST_ODDS  5       // (one encounter in COAST_ODDS)

static EWRAM_DATA u8 sState = MISSINGNO_NONE;

void MissingNo_ArmEncounter(void)
{
    sState = MISSINGNO_ARMED;
}

void MissingNo_OnMapLoad(void)
{
    u32 headerId = GetCurrentMapWildMonHeaderId();

    if (sState == MISSINGNO_NONE || headerId == HEADER_NONE)
        return;
    for (u32 t = 0; t < TIMES_OF_DAY_COUNT; t++)
    {
        if (gWildMonHeaders[headerId].encounterTypes[t].landMonsInfo != NULL)
        {
            sState = MISSINGNO_NONE;
            return;
        }
    }
}

static bool32 OnCoast(void)
{
    s16 x, y;

    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(MAP_CINNABAR_ISLAND)
     || gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CINNABAR_ISLAND))
        return FALSE;
    PlayerGetDestCoords(&x, &y);
    x -= MAP_OFFSET;
    y -= MAP_OFFSET;
    return x == COAST_X && y >= 0 && y <= COAST_Y_MAX;
}

void MissingNo_UpdateCoast(void)
{
    if (sState == MISSINGNO_NONE)
        return;
    if (OnCoast())
        sState = MISSINGNO_ON_COAST;
    else if (sState == MISSINGNO_ON_COAST)
        sState = MISSINGNO_NONE;
}

u32 MissingNo_WaterEncounterRate(u32 rate)
{
    return sState == MISSINGNO_ON_COAST ? COAST_RATE : rate;
}

// the player's name's letters in Red and Blue's character set
static u32 Gen1Char(u8 c)
{
    if (c >= CHAR_A && c <= CHAR_Z)
        return 0x80 + c - CHAR_A;
    if (c >= CHAR_a && c <= CHAR_z)
        return 0xA0 + c - CHAR_a;
    if (c >= CHAR_0 && c <= CHAR_9)
        return 0xF6 + c - CHAR_0;
    if (c == CHAR_SPACE)
        return 0x7F;
    return 0xE8;    // (punctuation: RB's '.')
}

static u32 NameLevel(void)
{
    u32 levels[5];
    u32 n = 0, i;
    bool32 ended = FALSE;

    for (i = 1; i < 11; i += 2)
    {
        u8 c = (i < PLAYER_NAME_LENGTH) ? gSaveBlock2Ptr->playerName[i] : EOS;
        u32 lv;

        if (ended)
            lv = 0;
        else if (c == EOS)
            lv = 0x50, ended = TRUE;
        else
            lv = Gen1Char(c);
        levels[n++] = lv;
    }
    i = levels[Random() % n];
    if (i < 1)
        i = 1;
    if (i > MAX_LEVEL)
        i = MAX_LEVEL;
    return i;
}

bool32 MissingNo_TryWaterEncounter(void)
{
    if (sState != MISSINGNO_ON_COAST || Random() % COAST_ODDS != 0)
        return FALSE;
    u32 level = NameLevel();
    if (!IsWildLevelAllowedByRepel(level))
        return FALSE;
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    ZeroEnemyPartyMons();
    CreateMonWithIVs(mon, SPECIES_MISSINGNO, level,
                     GetMonPersonality(SPECIES_MISSINGNO, MON_GENDERLESS, Random() % NUM_NATURES, 0),
                     OTID_STRUCT_PLAYER_ID, USE_RANDOM_IVS);
    SetMonMoveSlot(mon, MOVE_WATER_GUN, 0);
    SetMonMoveSlot(mon, MOVE_WATER_GUN, 1);
    SetMonMoveSlot(mon, MOVE_SKY_ATTACK, 2);
    SetMonMoveSlot(mon, MOVE_NONE, 3);
    return TRUE;
}
