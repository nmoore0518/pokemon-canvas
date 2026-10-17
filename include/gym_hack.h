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
bool32 GymHack_IsRuleBattleTrainer(u32 trainerId);
const u8 *GymHack_GetBlockedBattleScript(void);
u32 GymHack_GetRivalTypeFor(u32 playerType);
u32 GymHack_GetStarterSpeciesForType(u32 type);

#endif // GUARD_GYM_HACK_H
