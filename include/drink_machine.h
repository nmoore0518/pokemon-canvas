#ifndef GUARD_DRINK_MACHINE_H
#define GUARD_DRINK_MACHINE_H

// Pokémon Canvas: the Drink Machine (src/drink_machine.c)

#include "constants/drink_machine.h"

u32 DrinkMachine_Recipe(u32 drink, const u16 *berries, u32 n);
u32 DrinkMachine_GiveSecondType(struct Pokemon *mon, enum Type type);
bool32 DrinkMachine_TournamentSodaUnlocked(void);

#endif // GUARD_DRINK_MACHINE_H
