#ifndef GUARD_GYM_CHALLENGE_H
#define GUARD_GYM_CHALLENGE_H

// Gym Trainer hack: challengers at the player's gym after the League (src/gym_challenge.c)

#include "constants/gym_hack.h"

#define GYM_CHALLENGE_MAX_VICTORS 19     // challengers who beat the player, shown on the gym statue
                                         // (20 before the convention: its records took the last one's place)


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
    u16 conventionMagic;        // the Gym Leader Convention: who the player has beaten (src/gym_convention.c)
    u8 conventionBeaten[16];
    u16 switchMagic;            // changing the gym's type (src/gym_switch.c)
    u8 switchTickets;           // new types earned and not chosen yet
    u8 switchesEarned;          // types mastered (the first takes a longer streak)
};

extern u8 gGymChallengeDefeatText[];
extern u8 gGymChallengeIntroText[];
extern u8 gGymChallengeAfterText[];
extern bool8 gGymPracticeBattle;
extern u8 gGymGuestBattle;     // 0, or how many Gym Leader prizes a Guest Leader challenger pays

// A Gym Leader's prize: 4 x level x this
#define GYM_GUEST_PRIZE_RATE 25

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
void GymChallenge_GenerateRecruit(u32 type);
void GymChallenge_GenerateGuest(u32 baseLevel, u32 partySize, u32 tier, bool32 repeat);
void GymChallenge_EndPractice(void);
struct TrainerMon;
void GymChallenge_RetireStreak(void);
bool32 GymChallenge_WasBoss(void);
void GymChallenge_SetupGauntlet(u32 trainerClass, u32 pic, bool32 female, const u8 *name,
                                const struct TrainerMon *party, u32 partySize,
                                const u8 *intro, const u8 *defeat, const u8 *after, const u8 *win);
void GymChallenge_SetPracticeAI(u64 aiFlags);
void GymChallenge_SetupFixed(u32 trainerClass, u32 pic, bool32 female, const u8 *name,
                             const struct TrainerMon *party, u32 partySize);

#endif // GUARD_GYM_CHALLENGE_H
