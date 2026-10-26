#ifndef GUARD_MASTERY_HALL_H
#define GUARD_MASTERY_HALL_H

#include "hall_of_fame.h"
#include "constants/species.h"

// Gym Trainer hack: the Type Mastery Hall of Fame (src/mastery_hall.c). For each type mastered,
// the team of its longest streak. Kept in flash sector 30 (once the Trainer Hill's), saved with
// the game (src/save.c).
struct MasteryRecord
{
    u16 streak;     // 0: not mastered yet
    u8 type;
    u8 count;
    struct HallofFameMon mon[PARTY_SIZE];
};

struct MasteryHall
{
    u32 magic;
    struct MasteryRecord records[NUMBER_OF_MON_TYPES];   // (by type)
};

// BILL'S ARCHIVE (src/archive.c): one slot per species (by species number); a bit for whether it's
// stored, and one for whether that one's shiny
#define ARCHIVE_BYTES ((NUM_SPECIES + 7) / 8)
struct ArchiveData
{
    u32 magic;
    u8 stored[ARCHIVE_BYTES];
    u8 shiny[ARCHIVE_BYTES];
};

// Flash sector 30 (src/save.c): the Type Mastery Hall of Fame, then the archive
struct ExtraSaveData
{
    struct MasteryHall mastery;
    struct ArchiveData archive;
};

extern struct ExtraSaveData gExtraSave;
#define gMasteryHall (gExtraSave.mastery)

void MasteryHall_Clear(void);
void MasteryHall_Validate(void);
void MasteryHall_RecordWin(u32 type, u32 streak);
u32 MasteryHall_Count(void);
void AccessMasteryHallPC(void);

#endif // GUARD_MASTERY_HALL_H
