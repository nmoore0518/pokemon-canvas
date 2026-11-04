#ifndef GUARD_CONSTANTS_PAINTER_H
#define GUARD_CONSTANTS_PAINTER_H

// Pokémon Canvas: the painter in Pewter City and her Smeargle (src/painter.c, data/scripts/painter.inc)

#define CANVAS_SMEARGLE_LEVEL   15

// VAR_PAINTING_MODE: what a painting's for
#define PAINTING_MODE_NONE      0
#define PAINTING_MODE_FIRST     1   // the first: her Smeargle on loan, to find its colour
#define PAINTING_MODE_SKETCH    2   // a Sketch quest: the player's Smeargle, a move of a type
#define PAINTING_MODE_MEGA      3   // the Mega's: the SMEARGLITE

// Painter_GetOffer (VAR_RESULT): what she has for the player now
#define PAINTER_OFFER_NO_BADGE      0   // come back with a badge
#define PAINTER_OFFER_FIRST         1   // the first painting
#define PAINTER_OFFER_NEED_SMEARGLE 2   // something's ready, but the Smeargle she gave isn't in the party
#define PAINTER_OFFER_SKETCH        3   // a Sketch quest
#define PAINTER_OFFER_MEGA          4   // the Mega's painting
#define PAINTER_OFFER_BOTH          5   // either (after the League: the Mega's, or a Sketch quest)
#define PAINTER_OFFER_NOTHING       6   // nothing until the next badge

// Painter_Return (VAR_RESULT): how a painting went
#define PAINTER_RETURN_LEFT         0   // left without the battle
#define PAINTER_RETURN_FIRST_WON    1
#define PAINTER_RETURN_FIRST_LOST   2
#define PAINTER_RETURN_SKETCHED     3   // won, and kept the move (STR_VAR_1: its name)
#define PAINTER_RETURN_NO_SKETCH    4   // won, but sketched nothing
#define PAINTER_RETURN_SKETCH_LOST  5
#define PAINTER_RETURN_MEGA_WON     6
#define PAINTER_RETURN_MEGA_LOST    7

// Painter_CanEnter (VAR_RESULT)
#define PAINTER_ENTER_OK            0
#define PAINTER_ENTER_NO_ROOM       1   // (the first: no room for her Smeargle)
#define PAINTER_ENTER_NO_SMEARGLE   2   // (the player's Smeargle isn't in the party)
#define PAINTER_ENTER_TOO_FEW       3   // (a double battle: two Pokémon at least)

#endif // GUARD_CONSTANTS_PAINTER_H
