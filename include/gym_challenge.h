#ifndef GUARD_GYM_CHALLENGE_H
#define GUARD_GYM_CHALLENGE_H

// Gym Trainer hack: challengers at the player's gym after the League (src/gym_challenge.c)

#include "constants/gym_hack.h"

#define GYM_CHALLENGE_MAX_VICTORS 20     // challengers who beat the player, shown on the gym statue


struct GymChallengeVictor
{
    u8 name[TRAINER_NAME_LENGTH + 1];
    u8 trainerClass;
    u16 streak;                 // the streak they ended
};

struct GymChallengeSave
{
    u16 magic;
    u16 streak;                 // current win streak (kept between sessions)
    u16 bestStreak;
    u16 wins;
    u16 losses;
    u16 numVictors;             // everyone who ever beat the player (the list keeps the latest 20)
    struct GymChallengeVictor victors[GYM_CHALLENGE_MAX_VICTORS];
};

extern u8 gGymChallengeDefeatText[];
extern bool8 gGymPracticeBattle;

const struct Trainer *GymChallenge_GetTrainer(u16 trainerId);
struct GymChallengeSave *GymChallenge_GetSave(void);
u32 GymChallenge_PlayerAverageLevel(void);
void GymChallenge_Generate(u32 kind, u32 index);
u32 GymChallenge_EvolveForLevel(u32 species, u32 level);
u32 GymChallenge_CountFamous(void);
u32 GymChallenge_CountTownsfolk(void);

// Specials
bool8 GymChallenge_IsOpen(void);
void GymChallenge_BufferRecord(void);
void GymChallenge_NextChallenger(void);
void GymChallenge_GenerateOakFinale(void);
u32 GymChallenge_OakFinaleLevel(void);
void GymChallenge_RecordWin(void);
void GymChallenge_RecordLoss(void);
void GymChallenge_BufferLastResult(void);
void GymChallenge_PlanPlayerWalk(void);
void GymChallenge_PlanChallengerWalk(void);
bool32 GymChallenge_FindPath(s32 fromX, s32 fromY, s32 toX, s32 toY, s32 blockX, s32 blockY, bool32 allowMat, u8 face, u8 *out);
void GymChallenge_BufferVictorList(void);
u32 GymChallenge_BuildPracticeTeam(u32 tier, u32 type, u32 level);
void GymChallenge_BufferPracticeTrainer(void);
void GymChallenge_GeneratePractice(void);
void GymChallenge_GeneratePracticeTier(u32 tier, u32 instance);
void GymChallenge_EndPractice(void);

#endif // GUARD_GYM_CHALLENGE_H
