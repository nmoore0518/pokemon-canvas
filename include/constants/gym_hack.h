#ifndef GUARD_CONSTANTS_GYM_HACK_H
#define GUARD_CONSTANTS_GYM_HACK_H

// Results of the team rules check (GymHack_CheckTeam)
#define GYM_HACK_TEAM_OK          0
#define GYM_HACK_TEAM_WRONG_TYPE  1 // a party Pokemon's primary type isn't the player's type
#define GYM_HACK_TEAM_DUPLICATE   2 // two party Pokemon are the same species

// checkfieldmove sets VAR_RESULT to this when no party Pokemon knows the move
// but the player has the matching key item (Hedge Clippers, etc.)
#define FIELD_MOVE_USER_ITEM 7

#endif // GUARD_CONSTANTS_GYM_HACK_H
