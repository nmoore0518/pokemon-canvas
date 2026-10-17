// Gym Trainer hack: custom game logic.
//
// Starter selection: in Oak's lab the player picks one of 17 types from a
// menu. The menu order in data/maps/PalletTown_ProfessorOaksLab_Frlg/scripts.inc
// must match sStarters below.
//
// Rival: once the player's type is locked in, the rival specializes in the
// type that counters it. Every rival battle in the game is swapped for his
// team for that type (src/data/rival_teams_frlg.party).

#include "global.h"
#include "event_data.h"
#include "gym_hack.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_transition.h"
#include "trainer_pools.h"
#include "constants/abilities.h"
#include "constants/battle_ai.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "data.h"
#include "pokemon.h"
#include "string_util.h"
#include "constants/opponents.h"
#include "constants/rival_teams.h"
#include "constants/species.h"
#include "constants/vars.h"

// ---------------------------------------------------------------------------
// Starters
// ---------------------------------------------------------------------------

struct GymHackStarter
{
    u16 species;
    u8 rivalType; // the type the rival picks when the player chooses this one
};

static const struct GymHackStarter sStarters[] =
{
    { SPECIES_CLEFAIRY,   TYPE_STEEL    }, // Fairy
    { SPECIES_CATERPIE,   TYPE_FIRE     }, // Bug
    { SPECIES_POOCHYENA,  TYPE_FIGHTING }, // Dark
    { SPECIES_DRATINI,    TYPE_ICE      }, // Dragon
    { SPECIES_PIKACHU,    TYPE_GROUND   }, // Electric
    { SPECIES_TYROGUE,    TYPE_PSYCHIC  }, // Fighting
    { SPECIES_CHARMANDER, TYPE_WATER    }, // Fire
    { SPECIES_GASTLY,     TYPE_DARK     }, // Ghost
    { SPECIES_BULBASAUR,  TYPE_POISON   }, // Grass
    { SPECIES_SANDSHREW,  TYPE_GRASS    }, // Ground
    { SPECIES_SPHEAL,     TYPE_ROCK     }, // Ice
    { SPECIES_PORYGON,    TYPE_FIGHTING }, // Normal
    { SPECIES_EKANS,      TYPE_PSYCHIC  }, // Poison
    { SPECIES_RALTS,      TYPE_DARK     }, // Psychic
    { SPECIES_LARVITAR,   TYPE_WATER    }, // Rock
    { SPECIES_BELDUM,     TYPE_FIRE     }, // Steel
    { SPECIES_SQUIRTLE,   TYPE_ELECTRIC }, // Water
};

STATIC_ASSERT(ARRAY_COUNT(sStarters) == GYM_HACK_STARTER_COUNT, GymHackStarterCount);

// Script temp vars used by Oak's lab (see the .equ lines at the top of its scripts.inc)
#define PLAYER_STARTER_SPECIES  VAR_TEMP_2
#define RIVAL_STARTER_SPECIES   VAR_TEMP_3

static u32 GetPrimaryType(u32 species)
{
    return gSpeciesInfo[species].types[0];
}

static const struct GymHackStarter *GetStarterForType(u32 type)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sStarters); i++)
    {
        if (GetPrimaryType(sStarters[i].species) == type)
            return &sStarters[i];
    }
    return NULL;
}

u32 GymHack_GetRivalTypeFor(u32 playerType)
{
    const struct GymHackStarter *starter = GetStarterForType(playerType);
    return starter != NULL ? starter->rivalType : TYPE_NONE;
}

u32 GymHack_GetStarterSpeciesForType(u32 type)
{
    const struct GymHackStarter *starter = GetStarterForType(type);
    return starter != NULL ? starter->species : SPECIES_NONE;
}

// special: after the type menu. Reads the menu index from VAR_RESULT, sets
// up the starter vars, and buffers the species name (STR_VAR_1) and type
// name (STR_VAR_2) for Oak's confirmation text.
void GymHack_PrepareStarterChoice(void)
{
    u32 i = gSpecialVar_Result;

    if (i >= ARRAY_COUNT(sStarters))
        i = 0;
    VarSet(PLAYER_STARTER_SPECIES, sStarters[i].species);
    StringCopy(gStringVar1, GetSpeciesName(sStarters[i].species));
    StringCopy(gStringVar2, gTypesInfo[GetPrimaryType(sStarters[i].species)].name);
}

// special: once the player confirms. Permanently records the player's type
// and the rival's counter type, and sets up the rival's starter.
void GymHack_LockInPlayerType(void)
{
    u32 playerType = GetPrimaryType(VarGet(PLAYER_STARTER_SPECIES));
    u32 rivalType = GymHack_GetRivalTypeFor(playerType);

    VarSet(VAR_PLAYER_TYPE, playerType);
    VarSet(VAR_RIVAL_TYPE, rivalType);
    VarSet(RIVAL_STARTER_SPECIES, GymHack_GetStarterSpeciesForType(rivalType));
}

// special: buffers the rival's starter (STR_VAR_1), the player's type
// (STR_VAR_2) and the rival's type (STR_VAR_3) for his lines in the lab.
void GymHack_BufferRivalChoice(void)
{
    u32 rivalType = VarGet(VAR_RIVAL_TYPE);
    StringCopy(gStringVar1, GetSpeciesName(GymHack_GetStarterSpeciesForType(rivalType)));
    StringCopy(gStringVar2, gTypesInfo[VarGet(VAR_PLAYER_TYPE)].name);
    StringCopy(gStringVar3, gTypesInfo[rivalType].name);
}

// ---------------------------------------------------------------------------
// Rival teams
// ---------------------------------------------------------------------------

const struct Trainer gRivalTeams[DIFFICULTY_COUNT][RIVAL_TEAMS_COUNT] =
{
#include "data/rival_teams_frlg.h"
};

enum RivalStage
{
    RIVAL_STAGE_LAB,
    RIVAL_STAGE_ROUTE22_EARLY,
    RIVAL_STAGE_CERULEAN,
    RIVAL_STAGE_SS_ANNE,
    RIVAL_STAGE_POKEMON_TOWER,
    RIVAL_STAGE_SILPH,
    RIVAL_STAGE_ROUTE22_LATE,
    RIVAL_STAGE_CHAMPION_FIRST,
    RIVAL_STAGE_CHAMPION_REMATCH,
    RIVAL_STAGE_COUNT,
    RIVAL_STAGE_NONE = RIVAL_STAGE_COUNT,
};

// Types the rival can specialize in, in the same order as the teams in
// rival_teams_frlg.party (and include/constants/rival_teams.h).
static const u8 sRivalTypes[] =
{
    TYPE_FIRE, TYPE_WATER, TYPE_FIGHTING, TYPE_PSYCHIC, TYPE_DARK, TYPE_GROUND,
    TYPE_STEEL, TYPE_ICE, TYPE_ELECTRIC, TYPE_POISON, TYPE_GRASS, TYPE_ROCK,
};

#define RIVAL_TYPE_COUNT ARRAY_COUNT(sRivalTypes)
STATIC_ASSERT(RIVAL_TEAMS_COUNT == RIVAL_STAGE_COUNT * RIVAL_TYPE_COUNT, RivalTeamTableSize);

#if IS_FRLG
static u32 GetRivalStage(u32 trainerId)
{
    switch (trainerId)
    {
    case TRAINER_RIVAL_OAKS_LAB_SQUIRTLE:
    case TRAINER_RIVAL_OAKS_LAB_BULBASAUR:
    case TRAINER_RIVAL_OAKS_LAB_CHARMANDER:
        return RIVAL_STAGE_LAB;
    case TRAINER_RIVAL_ROUTE22_EARLY_SQUIRTLE:
    case TRAINER_RIVAL_ROUTE22_EARLY_BULBASAUR:
    case TRAINER_RIVAL_ROUTE22_EARLY_CHARMANDER:
        return RIVAL_STAGE_ROUTE22_EARLY;
    case TRAINER_RIVAL_CERULEAN_SQUIRTLE:
    case TRAINER_RIVAL_CERULEAN_BULBASAUR:
    case TRAINER_RIVAL_CERULEAN_CHARMANDER:
        return RIVAL_STAGE_CERULEAN;
    case TRAINER_RIVAL_SS_ANNE_SQUIRTLE:
    case TRAINER_RIVAL_SS_ANNE_BULBASAUR:
    case TRAINER_RIVAL_SS_ANNE_CHARMANDER:
        return RIVAL_STAGE_SS_ANNE;
    case TRAINER_RIVAL_POKEMON_TOWER_SQUIRTLE:
    case TRAINER_RIVAL_POKEMON_TOWER_BULBASAUR:
    case TRAINER_RIVAL_POKEMON_TOWER_CHARMANDER:
        return RIVAL_STAGE_POKEMON_TOWER;
    case TRAINER_RIVAL_SILPH_SQUIRTLE:
    case TRAINER_RIVAL_SILPH_BULBASAUR:
    case TRAINER_RIVAL_SILPH_CHARMANDER:
        return RIVAL_STAGE_SILPH;
    case TRAINER_RIVAL_ROUTE22_LATE_SQUIRTLE:
    case TRAINER_RIVAL_ROUTE22_LATE_BULBASAUR:
    case TRAINER_RIVAL_ROUTE22_LATE_CHARMANDER:
        return RIVAL_STAGE_ROUTE22_LATE;
    case TRAINER_CHAMPION_FIRST_SQUIRTLE:
    case TRAINER_CHAMPION_FIRST_BULBASAUR:
    case TRAINER_CHAMPION_FIRST_CHARMANDER:
        return RIVAL_STAGE_CHAMPION_FIRST;
    case TRAINER_CHAMPION_REMATCH_SQUIRTLE:
    case TRAINER_CHAMPION_REMATCH_BULBASAUR:
    case TRAINER_CHAMPION_REMATCH_CHARMANDER:
        return RIVAL_STAGE_CHAMPION_REMATCH;
    }
    return RIVAL_STAGE_NONE;
}
#endif

// Called by GetTrainerStructFromId (include/data.h): returns the rival's
// type-specialist team in place of an original rival battle, or NULL to use
// the trainer's normal data.
const struct Trainer *GymHack_GetRivalTrainer(u16 trainerId)
{
#if IS_FRLG
    u32 stage = GetRivalStage(trainerId);
    u32 rivalType, i;

    if (stage == RIVAL_STAGE_NONE)
        return NULL;
    rivalType = VarGet(VAR_RIVAL_TYPE);
    for (i = 0; i < RIVAL_TYPE_COUNT; i++)
    {
        if (sRivalTypes[i] == rivalType)
            return &gRivalTeams[DIFFICULTY_NORMAL][stage * RIVAL_TYPE_COUNT + i];
    }
#endif
    return NULL;
}
