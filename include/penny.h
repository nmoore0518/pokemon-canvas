#ifndef GUARD_PENNY_H
#define GUARD_PENNY_H

// Pokémon Canvas: Penny, the Eevee fan in Viridian City (src/penny.c)
#include "constants/penny.h"

u32 Penny_GetItem(u32 type);      // (ITEM_NONE for Fairy: Sylveon needs no item)
u32 Penny_GetSpecies(u32 type);

// Specials
void Penny_HasEevee(void);
void Penny_BufferEvo(void);
void Penny_WasGiven(void);
void Penny_MarkGiven(void);

#endif // GUARD_PENNY_H
