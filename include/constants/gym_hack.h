#ifndef GUARD_CONSTANTS_GYM_HACK_H
#define GUARD_CONSTANTS_GYM_HACK_H

// Results of the team rules check (GymHack_CheckTeam)
#define GYM_HACK_TEAM_OK          0
#define GYM_HACK_TEAM_WRONG_TYPE  1 // a party Pokemon's primary type isn't the player's type
#define GYM_HACK_TEAM_DUPLICATE   2 // two party Pokemon are the same species

// checkfieldmove sets VAR_RESULT to this when no party Pokemon knows the move
// but the player has the matching key item (Hedge Clippers, etc.)
#define FIELD_MOVE_USER_ITEM 7

// Gym challenge: kinds of challenger (src/gym_challenge.c)
#define GYM_CHALLENGER_TRAINER    0   // a random trainer with a random team
#define GYM_CHALLENGER_FAMOUS     1   // a gym leader, Elite Four member, the rival, Giovanni or Oak
#define GYM_CHALLENGER_TOWNSFOLK  2   // someone who doesn't usually battle (Mom, Nurse Joy, Bill...)

// Oak's type menu runs from easiest to hardest; from this entry on (Fighting, Dark, Ghost,
// Fairy) Oak warns that the type is a tough road
#define GYM_HACK_HARD_STARTERS_FROM 14

// VAR_GYM_FINALE: the gym after the League
#define GYM_FINALE_NONE            0   // the League isn't won yet
#define GYM_FINALE_OAK_WAITS       1   // Oak meets the player outside their house and takes them to the gym
#define GYM_FINALE_OAK_IN_GYM      2   // the player put off Oak's challenge; he waits in the gym
#define GYM_FINALE_DONE            3   // the player beat Oak: the gym is theirs

// VAR_MENTOR_STAGE bits
#define MENTOR_MET          (1 << 0)
#define MENTOR_GAVE_GUARD   (1 << 1)    // the defensive item, after Mt. Moon (at the Cerulean Gym)
#define MENTOR_GAVE_EDGE    (1 << 2)    // the offensive item, at the Cinnabar Gym

#define MENTOR_GIFT_NONE    0
#define MENTOR_GIFT_LEADER  1   // the mentor is this gym's leader: they hand it over themselves
#define MENTOR_GIFT_VISIT   2   // the mentor drops by

// VAR_GYM_LEGEND_STATE: the legendary in the player's gym (src/gym_legend.c)
#define GYM_LEGEND_ON_STAGE      0   // waiting on the stage (once it has appeared)
#define GYM_LEGEND_ESCAPED       1   // it got away (or the player lost); the rival comes to gloat
#define GYM_LEGEND_IN_CAVE       2   // the rival hinted at Cerulean Cave: a 1% encounter there
#define GYM_LEGEND_CAUGHT_NEWS   3   // caught; the rival comes to scoff
#define GYM_LEGEND_DONE          4

// VAR_JOHTO_STATE (src/johto.c)
#define JOHTO_NOT_VISITED   0
#define JOHTO_GOT_GOODIE_BAG 1

#endif // GUARD_CONSTANTS_GYM_HACK_H
