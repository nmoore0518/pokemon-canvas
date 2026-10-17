// Gym Trainer hack: custom game logic.
//
// Starter selection: in Oak's lab the player picks one of 17 types from a
// menu. The menu order in data/maps/PalletTown_ProfessorOaksLab_Frlg/scripts.inc
// must match sStarters below.

#include "global.h"
#include "event_data.h"
#include "gym_hack.h"
#include "battle_main.h"
#include "pokemon.h"
#include "string_util.h"
#include "constants/species.h"
#include "constants/vars.h"

// The rival still takes one of the three Kanto starters, because all of his
// later teams are keyed off that choice (VAR_STARTER_MON). This value is what
// the vanilla script calls PLAYER_STARTER_NUM, and it decides the rival's pick:
//   RIVAL_CHARMANDER: rival takes Charmander
//   RIVAL_BULBASAUR:  rival takes Bulbasaur
//   RIVAL_SQUIRTLE:   rival takes Squirtle
// Each type gets the rival whose starter has the edge on it where possible.
enum {
    RIVAL_CHARMANDER = 0,
    RIVAL_BULBASAUR = 1,
    RIVAL_SQUIRTLE = 2,
};

struct GymHackStarter
{
    u16 species;
    u8 rivalStarterNum;
};

static const struct GymHackStarter sStarters[] =
{
    { SPECIES_CLEFAIRY,   RIVAL_BULBASAUR  }, // Fairy
    { SPECIES_CATERPIE,   RIVAL_CHARMANDER }, // Bug
    { SPECIES_POOCHYENA,  RIVAL_SQUIRTLE   }, // Dark
    { SPECIES_DRATINI,    RIVAL_CHARMANDER }, // Dragon
    { SPECIES_PIKACHU,    RIVAL_BULBASAUR  }, // Electric
    { SPECIES_TYROGUE,    RIVAL_CHARMANDER }, // Fighting
    { SPECIES_CHARMANDER, RIVAL_SQUIRTLE   }, // Fire
    { SPECIES_GASTLY,     RIVAL_SQUIRTLE   }, // Ghost
    { SPECIES_BULBASAUR,  RIVAL_CHARMANDER }, // Grass
    { SPECIES_SANDSHREW,  RIVAL_SQUIRTLE   }, // Ground
    { SPECIES_SPHEAL,     RIVAL_BULBASAUR  }, // Ice
    { SPECIES_PORYGON,    RIVAL_SQUIRTLE   }, // Normal
    { SPECIES_EKANS,      RIVAL_SQUIRTLE   }, // Poison
    { SPECIES_RALTS,      RIVAL_CHARMANDER }, // Psychic
    { SPECIES_LARVITAR,   RIVAL_BULBASAUR  }, // Rock
    { SPECIES_BELDUM,     RIVAL_CHARMANDER }, // Steel
    { SPECIES_SQUIRTLE,   RIVAL_BULBASAUR  }, // Water
};

STATIC_ASSERT(ARRAY_COUNT(sStarters) == GYM_HACK_STARTER_COUNT, GymHackStarterCount);

#define PLAYER_STARTER_NUM      VAR_TEMP_1
#define PLAYER_STARTER_SPECIES  VAR_TEMP_2

static u32 GetPrimaryType(u32 species)
{
    return gSpeciesInfo[species].types[0];
}

// special: after the type menu. Reads the menu index from VAR_RESULT, sets
// up the starter vars, and buffers the species name (STR_VAR_1) and type
// name (STR_VAR_2) for Oak's confirmation text.
void GymHack_PrepareStarterChoice(void)
{
    u32 i = gSpecialVar_Result;

    if (i >= ARRAY_COUNT(sStarters))
        i = 0;
    VarSet(PLAYER_STARTER_NUM, sStarters[i].rivalStarterNum);
    VarSet(PLAYER_STARTER_SPECIES, sStarters[i].species);
    StringCopy(gStringVar1, GetSpeciesName(sStarters[i].species));
    StringCopy(gStringVar2, gTypesInfo[GetPrimaryType(sStarters[i].species)].name);
}

// special: once the player confirms. Permanently records their type.
void GymHack_LockInPlayerType(void)
{
    VarSet(VAR_PLAYER_TYPE, GetPrimaryType(VarGet(PLAYER_STARTER_SPECIES)));
}
