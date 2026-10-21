#ifndef GUARD_GYM_SWITCH_H
#define GUARD_GYM_SWITCH_H

// Gym Trainer hack: changing the gym's type after the League (src/gym_switch.c)

bool32 GymSwitch_IsOwned(u32 type);
u32 GymSwitch_GetGoal(void);
bool32 GymSwitch_IsMasteryBattle(u32 streak);
void GymSwitch_BufferTypeName(u8 *dst, u32 type);

// Specials
void GymSwitch_AfterWin(void);
void GymSwitch_BufferGoal(void);
void GymSwitch_BufferStatus(void);
void GymSwitch_CanTakeNewType(void);
void GymSwitch_HasSwitched(void);
void GymSwitch_StreakInProgress(void);
void GymSwitch_OnlyNewTypes(void);
void GymSwitch_PushTypes(void);
void GymSwitch_BufferChoice(void);
void GymSwitch_SetType(void);
void GymSwitch_AllMastered(void);
bool32 GymSwitch_AllTypesMastered(void);

#endif // GUARD_GYM_SWITCH_H
