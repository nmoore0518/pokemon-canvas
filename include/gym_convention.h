#ifndef GUARD_GYM_CONVENTION_H
#define GUARD_GYM_CONVENTION_H

// Gym Trainer hack: the Gym Leader Convention (src/gym_convention.c)

bool32 Convention_IsBeaten(u32 index);
u32 Convention_GetGraphicsId(u32 index);
s32 Convention_SetupByName(const u8 *name);

// Specials
void Convention_SetupTrainer(void);
void Convention_RecordWin(void);
void Convention_PushHalls(void);
void Convention_BufferProgress(void);
void Convention_WarpToHall(void);

void Convention_ShowTitleCard(void);

#endif // GUARD_GYM_CONVENTION_H
