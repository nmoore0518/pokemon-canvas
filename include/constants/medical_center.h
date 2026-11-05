#ifndef GUARD_CONSTANTS_MEDICAL_CENTER_H
#define GUARD_CONSTANTS_MEDICAL_CENTER_H

// Pokémon Canvas: the Universal Pokémon Medical Center (src/medical_center.c)

#define MEDCENTER_DAY_STEPS     1000    // (a "day": Pokérus wears off a day at a time, and can be given again)
#define MEDCENTER_NUM_HELPERS   4       // (MACHOP, MACHOKE, MACHAMP, a shiny MACHAMP)
#define MEDCENTER_NUM_SLIDES    5       // (the 3F microscopes)
#define MEDCENTER_POKERUS_SLIDE 2       // (the one with Pokérus on it)

// MedCenter_CheckHelper (VAR_RESULT)
#define HELPER_OK               0
#define HELPER_WRONG_SPECIES    1
#define HELPER_NOT_SHINY        2
#define HELPER_EGG              4
#define HELPER_LAST_MON         5       // (the player's only Pokémon)

// MedCenter_GivePokerus (VAR_RESULT)
#define POKERUS_GIVE_DONE       0
#define POKERUS_GIVE_HAS_IT     1
#define POKERUS_GIVE_HAD_IT     2       // (it's had it: immune now, and it keeps the effect)
#define POKERUS_GIVE_EGG        3

#endif // GUARD_CONSTANTS_MEDICAL_CENTER_H
