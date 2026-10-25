#ifndef GUARD_MASTERY_HALL_H
#define GUARD_MASTERY_HALL_H

#include "hall_of_fame.h"

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

extern struct MasteryHall gMasteryHall;

void MasteryHall_Clear(void);
void MasteryHall_Validate(void);
void MasteryHall_RecordWin(u32 type, u32 streak);
u32 MasteryHall_Count(void);
void AccessMasteryHallPC(void);

#endif // GUARD_MASTERY_HALL_H
