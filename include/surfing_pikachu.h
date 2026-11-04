#ifndef GUARD_SURFING_PIKACHU_H
#define GUARD_SURFING_PIKACHU_H

// Pokémon Canvas: Surfing Pikachu (src/surfing_pikachu.c)
#define SURFING_PIKACHU_LEVEL 50

#include "constants/surfing_pikachu.h"

bool32 SurfingPikachu_IsPikachu(u32 species);
u32 SurfingPikachu_PartyKind(void);
bool32 SurfingPikachu_InParty(void);

#endif // GUARD_SURFING_PIKACHU_H
