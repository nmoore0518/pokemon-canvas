#include "global.h"
#include "pokemon_storage_system.h"
#include "test/test.h"

// If you would like to ensure save compatibility, update the values below with those for your hack. You can find these through the debug menu.
// Please note that this simple check is not 100% foolproof, but should be able to catch most unintended shifts.
// Gym Trainer hack: FireRed's SaveBlock1 is larger than Emerald's. Pinning it here means a
// change that would break existing saves shows up as a failing test.
// Pokémon Canvas: re-pinned to the sizes in the test build (the bag's pockets fill SaveBlock1 and
// SaveBlock3; SaveBlock2 ends with the Poké Balls pocket's second part, bagPokeBalls2). Anything
// added goes at the end of a block, so older saves still load.
#if IS_FRLG
#define T_SAVEBLOCK1_SIZE 15872
#else
#define T_SAVEBLOCK1_SIZE 15568
#endif
#define T_SAVEBLOCK2_SIZE 3956
#define T_SAVEBLOCK3_SIZE 1624
#define T_POKEMONSTORAGE_SIZE 38964

TEST("SaveBlock1 is backwards compatible")
{
    EXPECT_EQ(sizeof(struct SaveBlock1), T_SAVEBLOCK1_SIZE);
}

TEST("SaveBlock2 is backwards compatible")
{
    EXPECT_EQ(sizeof(struct SaveBlock2), T_SAVEBLOCK2_SIZE);
}

TEST("SaveBlock3 is backwards compatible")
{
    EXPECT_EQ(sizeof(struct SaveBlock3), T_SAVEBLOCK3_SIZE);
}

TEST("PokemonStorage is backwards compatible")
{
    EXPECT_EQ(sizeof(struct PokemonStorage), T_POKEMONSTORAGE_SIZE);
}

#undef T_SAVEBLOCK1_SIZE
#undef T_SAVEBLOCK2_SIZE
#undef T_SAVEBLOCK3_SIZE
#undef T_POKEMONSTORAGE_SIZE
