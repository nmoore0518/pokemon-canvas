#ifndef GUARD_CONSTANTS_BASEMENT_MACHINES_H
#define GUARD_CONSTANTS_BASEMENT_MACHINES_H

// Pokémon Canvas: Dad's basement machines (src/basement_machines.c). The wall switch's flag picks the mode:
// FLAG_BASEMENT_SWITCH_ON set = Nice Mode, clear = Evil Mode.
#define FLAG_BASEMENT_NICE_MODE FLAG_BASEMENT_SWITCH_ON

#define SYNTH_FUEL_COUNT        3   // Strong Acid (Evil Mode) or Apple Juice (Nice Mode) per use
#define SYNTH_NICE_FRIENDSHIP   20  // Nice Mode: friendship goes up this much (Evil Mode: to 0)
#define BALL_MAKER_MIN_LEVEL    50
#define BALL_MAKER_NICE_LEVELS  5   // Nice Mode: levels lost instead of the Pokémon
#define LAB_HELD_ITEM_PERCENT   25  // wild Poison Pokémon: Strong Acid; wild Grass Pokémon: Apple Juice
#define APPLIN_JUICE_PERCENT    50  // the Applin line: Apple Juice

// Results of the scripts' checks (VAR_RESULT)
#define MACHINE_OK                0
#define MACHINE_EGG               1
#define MACHINE_SYNTH_ALREADY     2  // it already has its synthetic ability
#define MACHINE_SYNTH_BLOCKED     3  // a species the Synthesizer won't take
#define MACHINE_MAKER_LOW_LEVEL   4  // under BALL_MAKER_MIN_LEVEL
#define MACHINE_MAKER_LAST_MON    5  // Evil Mode: your only Pokémon (not counting Eggs)
#define MACHINE_MAKER_BAG_FULL    6  // no room for the Type Ball
#define MACHINE_CHANGER_SAME_BALL 7  // it's already in that kind of ball
#define MACHINE_CHANGER_BAG_FULL  8  // no room to give back its old ball

#endif // GUARD_CONSTANTS_BASEMENT_MACHINES_H
