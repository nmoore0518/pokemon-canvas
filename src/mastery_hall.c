// Gym Trainer hack (Pokémon Canvas): the Type Mastery Hall of Fame. Once the gym's type is mastered
// (src/gym_switch.c), the team that's had its longest streak with that type is kept here: after
// every win of a mastered type's streak, if the streak beats the record, the party now replaces
// it. Seen from the PC (or the Palm Pilot) as "TYPE MASTERY", in the Hall of Fame's own screen
// (src/hall_of_fame_frlg.c). Lives in RAM and goes to flash sector 30 with every save.
#include "global.h"
#include "mastery_hall.h"
#include "main.h"
#include "pokemon.h"
#include "string_util.h"
#include "constants/species.h"

#define MASTERY_HALL_MAGIC 0x4D484631    // "MHF1"

EWRAM_DATA struct ExtraSaveData gExtraSave = {0};

void MasteryHall_Clear(void)
{
    memset(&gMasteryHall, 0, sizeof(gMasteryHall));
    gMasteryHall.magic = MASTERY_HALL_MAGIC;
}

// After loading (or finding nothing there): anything that isn't ours is cleared
void MasteryHall_Validate(void)
{
    if (gMasteryHall.magic != MASTERY_HALL_MAGIC)
        MasteryHall_Clear();
}

void MasteryHall_RecordWin(u32 type, u32 streak)
{
    struct MasteryRecord *rec;
    u32 i, n = 0;

    MasteryHall_Validate();
    if (type >= NUMBER_OF_MON_TYPES || streak <= gMasteryHall.records[type].streak)
        return;
    rec = &gMasteryHall.records[type];
    memset(rec, 0, sizeof(*rec));
    rec->streak = streak;
    rec->type = type;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        struct HallofFameMon *dst;
        u8 nick[POKEMON_NAME_BUFFER_SIZE];
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        dst = &rec->mon[n++];
        dst->species = GetMonData(mon, MON_DATA_SPECIES);
        dst->tid = GetMonData(mon, MON_DATA_OT_ID);
        dst->personality = GetMonData(mon, MON_DATA_PERSONALITY);
        dst->isShiny = GetMonData(mon, MON_DATA_IS_SHINY);
        dst->lvl = GetMonData(mon, MON_DATA_LEVEL);
        GetMonData(mon, MON_DATA_NICKNAME, nick);
        memcpy(dst->nickname, nick, POKEMON_NAME_LENGTH);
    }
    rec->count = n;
}

u32 MasteryHall_Count(void)
{
    u32 type, n = 0;
    MasteryHall_Validate();
    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
        n += gMasteryHall.records[type].streak != 0;
    return n;
}
