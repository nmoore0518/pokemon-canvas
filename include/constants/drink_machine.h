#ifndef GUARD_CONSTANTS_DRINK_MACHINE_H
#define GUARD_CONSTANTS_DRINK_MACHINE_H

// Pokémon Canvas: the Drink Machine (src/drink_machine.c): what its specials tell the script
// DrinkMachine_Mix (VAR_RESULT)
#define DRINK_MIX_NOTHING  0   // (not a recipe: nothing's used up)
#define DRINK_MIX_MADE     1   // (STR_VAR_1: what; VAR_0x8009: its item)
#define DRINK_MIX_SODA     2   // (a LIMITED EDITION SODA mix: now to give it to a Pokémon)
#define DRINK_MIX_NO_ROOM  3   // (no room in the bag for it)

// DrinkMachine_ApplySoda (VAR_RESULT)
#define DRINK_SODA_DONE        0
#define DRINK_SODA_FIRST_TYPE  1   // (the berry's type is its first type: refused)
#define DRINK_SODA_ALREADY     2   // (it's already that type)
#define DRINK_SODA_CANT        3   // (an Egg, or no Pokémon chosen)


#endif // GUARD_CONSTANTS_DRINK_MACHINE_H
