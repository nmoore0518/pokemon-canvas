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
#define GYM_SWITCH_FIRST_STREAK   15
#define GYM_SWITCH_NEXT_STREAK    10
// Random gym challengers: the share of teams with a legendary (never more than one)
#define GYM_CHALLENGER_LEGEND_PERCENT 40
// Oak's battle for the gym: his level over the player's average (never below 65)
#define GYM_OAK_FINALE_MARGIN     1

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
// The tournament (src/tournament.c; its story and Bill: src/gauntlet.c): VAR_GAUNTLET_STATE
#define GAUNTLET_NONE           0
#define GAUNTLET_ARRIVING       1   // (the hooded bookie's scene plays as the player comes into the gym)
#define GAUNTLET_ACTIVE         2   // (the tournament's open: not won yet)
#define GAUNTLET_ALL_BEATEN     3   // (won: the one who put it on is next)
#define GAUNTLET_SIMULATOR      4   // (Bill beaten: his Palm Pilot runs new tournaments, as simulations)

// A tournament run's status (gTourneySave.run.status)
#define TOURNEY_NONE            0
#define TOURNEY_ACTIVE          1
#define TOURNEY_LOST            2
#define TOURNEY_WON             3
#define TOURNEY_FORFEIT         4
#define TOURNEY_BILL            5   // (a simulator title: Bill's simulation is next, for the WITHDRAW)

// The odds of a MOVE EXTRACTOR for a gym challenge win, and for a tournament match won (in Bill's
// simulator too): 1 in this many
#define GYM_CHALLENGE_EXTRACTOR_ODDS 150
#define TOURNEY_EXTRACTOR_ODDS       100
#define TOURNEY_ROUNDS          7   // (128 entrants)

// The Leader Academy (north of Route 11; src/leader_academy.c): VAR_ACADEMY_STATE
#define ACADEMY_NONE            0
#define ACADEMY_INVITE          1   // (3 badges: the mentor catches up with the player on Route 11)
#define ACADEMY_ESCORT          2   // (at the gap in the trees, with the mentor)
#define ACADEMY_WELCOME         3   // (on the campus: the headmaster's welcome plays)
#define ACADEMY_ADMITTED        4
#define ACADEMY_BRACELET_CALL   5   // (8 badges: the headmaster calls the player over as they come into the hall)
#define ACADEMY_BRACELET_GIVE   6   // (at her desk: the Mega Bracelet)
// Academy_MegaTrialCheck's VAR_RESULT, besides 0-2 stones
#define MEGA_TRIAL_HAS_ALL      3

// When the player can first get a Pokémon (gDexFormAvail, tools/gym_hack/build_availability.py)
#define AVAIL_START       0     // the main game
#define AVAIL_CLEAR       1     // after the Hall of Fame (Cerulean Cave)
#define AVAIL_LEADER      2     // once the gym is the player's (Johto, the Exotic Safari)
#define AVAIL_MASTERED    3     // every gym type mastered
#define AVAIL_NEVER       15    // nowhere in the game
#define AVAIL_TIER_MASK   0x0F
#define AVAIL_GYM_LEGEND(type) (((type) + 1) << 8)   // ... and needs the gym legendary of this type
                                                    // (it comes once the gym has had that type)
#define AVAIL_LEGEND_TYPE(a)  (((a) >> 8) - 1)          // (-1: none)
#define AVAIL_MASTERY     0x80  // (with the type above: not its legendary, but that type mastered)
#define AVAIL_TYPE_MASTERED(type) (AVAIL_GYM_LEGEND(type) | AVAIL_MASTERY)

#define JOHTO_NOT_INVITED       0
#define JOHTO_INVITE_WAITING    1   // (after the gym's handed over: the mentor brings it next visit)
#define JOHTO_INVITED           2   // (the ferry from Vermilion sails to Olivine)
#define JOHTO_GOT_GOODIE_BAG    3

// The Gym Leader Convention (src/gym_convention.c): Convention_RecordWin's results
#define CONVENTION_WON_HALL     1
#define CONVENTION_WON_ALL      2

#endif // GUARD_CONSTANTS_GYM_HACK_H
