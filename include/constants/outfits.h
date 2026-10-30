#ifndef GUARD_CONSTANTS_OUTFITS_H
#define GUARD_CONSTANTS_OUTFITS_H

// Pokémon Canvas: the player's outfits (src/outfits.c, colors from tools/gym_hack/outfit_art.py).
// Saves keep VAR_PLAYER_OUTFIT and which ones are unlocked (VAR_OUTFITS_UNLOCKED*), so new ones go
// at the end.
#define OUTFIT_CASUAL     0   // what the player starts in (always there)
#define OUTFIT_ROCKET     1   // DAD's old TEAM ROCKET uniform (the basement crates)
#define OUTFIT_NORMAL     2   // the gym uniforms, one per type (TYPE_* order, no Mystery)
#define OUTFIT_FIGHTING   3
#define OUTFIT_FLYING     4
#define OUTFIT_POISON     5
#define OUTFIT_GROUND     6
#define OUTFIT_ROCK       7
#define OUTFIT_BUG        8
#define OUTFIT_GHOST      9
#define OUTFIT_STEEL      10
#define OUTFIT_FIRE       11
#define OUTFIT_WATER      12
#define OUTFIT_GRASS      13
#define OUTFIT_ELECTRIC   14
#define OUTFIT_PSYCHIC    15
#define OUTFIT_ICE        16
#define OUTFIT_DRAGON     17
#define OUTFIT_DARK       18
#define OUTFIT_FAIRY      19
#define OUTFIT_COUNT      20

#endif // GUARD_CONSTANTS_OUTFITS_H
