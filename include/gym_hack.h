#ifndef GUARD_GYM_HACK_H
#define GUARD_GYM_HACK_H

// Number of choices on the starter type menu in Oak's lab.
#define GYM_HACK_STARTER_COUNT 17

// Specials (called from event scripts)
void GymHack_PrepareStarterChoice(void);
void GymHack_LockInPlayerType(void);
void GymHack_BufferRivalChoice(void);

void GymHack_CheckTeamSpecial(void);
void GymHack_WarpInPlace(void);

u32 GymHack_CheckTeam(void);
bool32 GymHack_SpeciesCanEvolve(u32 species);
bool32 GymHack_IsRuleBattleTrainer(u32 trainerId);
const u8 *GymHack_GetBlockedBattleScript(void);
u32 GymHack_GetRivalTypeFor(u32 playerType);
u32 GymHack_GetStarterSpeciesForType(u32 type);

// HM key items
extern bool8 gGymHackFlyFromItem;
u32 GymHack_GetFieldMoveItem(u32 fieldMove);
bool32 GymHack_HasFieldMoveItem(u32 fieldMove);
void GymHack_SkipNextShowMon(void);
extern const u8 EventScript_GymHack_UseHedgeClippers[];
extern const u8 EventScript_GymHack_UseFlashlight[];
extern const u8 EventScript_GymHack_UseBoogieBoard[];
extern const u8 EventScript_GymHack_UseCanOfSpinach[];

#endif // GUARD_GYM_HACK_H
