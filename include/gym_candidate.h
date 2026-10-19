#ifndef GUARD_GYM_CANDIDATE_H
#define GUARD_GYM_CANDIDATE_H

// Gym Trainer hack: the player as a Gym Leader candidate (src/gym_candidate.c)

#include "constants/gym_decor.h"
#include "constants/pokemon.h"

#define GYM_CANDIDATE_LEADERS    8
#define GYM_CANDIDATE_ELITE_FOUR 4

struct GymCandidateLeader
{
    const u8 *name;
    u16 approvalFlag;
    u8 guests;          // challengers the player holds the gym against
    u8 level;           // their level (at least; more if the player is ahead)
    u8 partySize;
    u8 tier;            // how evolved their Pokémon are (practice tiers: 0 rookie, 1 skilled, 2 expert)
    const u8 *offer;
    const u8 *approve;
};

struct GymCandidateMentor
{
    u8 who;             // Gym Leader 0-7, Elite Four 8-11
    const u8 *first;    // on learning the player's type
    const u8 *approve;  // coming around
};

struct GymRecruit
{
    u16 decor;          // their trainer decoration
    u16 trainerId;      // their battle on the road (name, class, picture)
    u16 gfx;
    bool8 female;
    const u8 *pitch;
};

extern const struct GymCandidateLeader gGymCandidateLeaders[GYM_CANDIDATE_LEADERS];
extern const struct GymRecruit gGymRecruits[NUMBER_OF_MON_TYPES];

u32 GymCandidate_CountApprovals(void);
const struct GymCandidateMentor *GymCandidate_GetMentorForType(u32 type);
const u8 *GymCandidate_GetEliteFourApproval(u32 i);
const struct GymRecruit *GymCandidate_GetRecruit(u32 type);
const struct GymRecruit *GymCandidate_GetRecruitByDecor(u32 decor);

// Specials
void GymCandidate_BufferApprovals(void);
void GymCandidate_IsApproved(void);
void GymCandidate_BufferOffer(void);
void GymCandidate_MoreGuests(void);
void GymCandidate_GenerateGuest(void);
void GymCandidate_BufferMentorFirst(void);
void GymCandidate_Approve(void);
void GymCandidate_BufferEliteFourApproval(void);
void GymCandidate_PlaceGuest(void);
void GymCandidate_FaceGuest(void);
void GymCandidate_BufferRecruit(void);
void GymCandidate_Recruit(void);

#endif // GUARD_GYM_CANDIDATE_H
