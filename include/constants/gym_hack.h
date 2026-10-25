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
#define GYM_CHALLENGER_BOSS       3   // the player's mentor, at full strength: the streak's mastery battle

// Changing the gym's type after the League (src/gym_switch.c): a streak this long (its last battle
// against the mentor) masters the type and earns a new one; the first takes longer
#define GYM_SWITCH_FIRST_STREAK   20
#define GYM_SWITCH_NEXT_STREAK    10

// The Gym Charm (from the mentor, after the League): Shiny odds for Pokémon of the gym's type
#define GYM_CHARM_SHINY_ODDS      250

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
#define GYM_LEGEND_PENDING       5   // the gym's new type was chosen away from it: it arrives next visit

// VAR_JOHTO_STATE (src/johto.c)
// The Gauntlet (src/gauntlet.c): VAR_GAUNTLET_STATE
#define GAUNTLET_NONE           0
#define GAUNTLET_ARRIVING       1   // (the hooded figure's scene plays as the player comes into the gym)
#define GAUNTLET_ACTIVE         2
#define GAUNTLET_ALL_BEATEN     3   // (every challenger beaten: the one who hired the figure is next)
#define GAUNTLET_SIMULATOR      4   // (Bill beaten: his Palm Pilot runs it all again, as a simulation)

#define JOHTO_NOT_INVITED       0
#define JOHTO_INVITE_WAITING    1   // (after the gym's handed over: the mentor brings it next visit)
#define JOHTO_INVITED           2   // (the ferry from Vermilion sails to Olivine)
#define JOHTO_GOT_GOODIE_BAG    3

// The Gym Leader Convention (src/gym_convention.c): Convention_RecordWin's results
#define CONVENTION_WON_HALL     1
#define CONVENTION_WON_ALL      2

#endif // GUARD_CONSTANTS_GYM_HACK_H
