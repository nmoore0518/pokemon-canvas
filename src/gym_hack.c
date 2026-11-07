// Gym Trainer hack: custom game logic.
//
// Starter selection: in Oak's lab the player picks one of 18 types from a
// menu, ordered from easiest to hardest: by the highest base stat total a
// team of six different species of that primary type can have (Gen 1-3;
// of the legendaries, only Kanto's: Johto's and Hoenn's are in the Exotic Safari). See GYM_HACK.md for the numbers. The menu order in data/maps/PalletTown_ProfessorOaksLab_Frlg/scripts.inc
// must match sStarters below.
//
// Team rules: in gym leader and rival battles (and to enter the Elite Four),
// every Pokemon in the party must have the player's type as its primary type,
// and no two may be the same species.
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
#include "battle_setup.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "fieldmap.h"
#include "overworld.h"
#include "constants/gym_hack.h"
#include "region_map.h"
#include "wild_encounter.h"
#include "constants/field_move.h"
#include "constants/items.h"
#include "item.h"
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
    { SPECIES_RALTS,      TYPE_DARK     }, // Psychic
    { SPECIES_CHARMANDER, TYPE_WATER    }, // Fire
    { SPECIES_PORYGON,    TYPE_FIGHTING }, // Normal
    { SPECIES_SQUIRTLE,   TYPE_ELECTRIC }, // Water
    { SPECIES_BULBASAUR,  TYPE_POISON   }, // Grass
    { SPECIES_PIKACHU,    TYPE_GROUND   }, // Electric
    { SPECIES_LARVITAR,   TYPE_WATER    }, // Rock
    { SPECIES_EKANS,      TYPE_PSYCHIC  }, // Poison
    { SPECIES_CATERPIE,   TYPE_FIRE     }, // Bug
    { SPECIES_BELDUM,     TYPE_FIRE     }, // Steel
    { SPECIES_SPHEAL,     TYPE_ROCK     }, // Ice
    { SPECIES_SANDSHREW,  TYPE_GRASS    }, // Ground
    { SPECIES_SPEAROW,    TYPE_ELECTRIC }, // Flying (the birds are Flying first in this hack)
    { SPECIES_DRATINI,    TYPE_ICE      }, // Dragon
    { SPECIES_TYROGUE,    TYPE_PSYCHIC  }, // Fighting
    { SPECIES_POOCHYENA,  TYPE_FIGHTING }, // Dark
    { SPECIES_GASTLY,     TYPE_DARK     }, // Ghost
    { SPECIES_CLEFAIRY,   TYPE_STEEL    }, // Fairy
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

// ---------------------------------------------------------------------------
// Team rules
// ---------------------------------------------------------------------------

// Checks the player's party against the rules (TEAM_RULE_*). On a violation, buffers text for the
// message: WRONG_TYPE -> STR_VAR_1 = the Pokemon's nickname, STR_VAR_2 = its type, STR_VAR_3 = the
// player's type; DUPLICATE -> STR_VAR_1 = the species name; DUPLICATE_ITEM -> STR_VAR_1 = the item's
// name. Eggs don't count. (With the type rule, nothing is checked before a type is chosen.)
u32 GymHack_CheckTeamRules(u32 rules)
{
    u32 playerType = VarGet(VAR_PLAYER_TYPE);
    u32 i, j;

    if ((rules & TEAM_RULE_TYPE) && playerType == TYPE_NONE)
        return GYM_HACK_TEAM_OK;    // (the gyms' rules start once there's a type: before, none apply)

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 species = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES);
        u32 item = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HELD_ITEM);
        if (species == SPECIES_NONE || GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            continue;

        if ((rules & TEAM_RULE_TYPE) && GetPrimaryType(species) != playerType)
        {
            GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_NICKNAME, gStringVar1);
            StringGet_Nickname(gStringVar1);
            if (species == SPECIES_MISSINGNO)   // (its typeless first type: Red and Blue's BIRD)
                StringCopy(gStringVar2, COMPOUND_STRING("Bird"));
            else
                StringCopy(gStringVar2, gTypesInfo[GetPrimaryType(species)].name);
            StringCopy(gStringVar3, gTypesInfo[playerType].name);
            return GYM_HACK_TEAM_WRONG_TYPE;
        }

        for (j = i + 1; j < PARTY_SIZE; j++)
        {
            u32 other = GetMonData(&gParties[B_TRAINER_PLAYER][j], MON_DATA_SPECIES);
            if (other == SPECIES_NONE || GetMonData(&gParties[B_TRAINER_PLAYER][j], MON_DATA_IS_EGG))
                continue;
            if ((rules & TEAM_RULE_SPECIES) && GET_BASE_SPECIES_ID(other) == GET_BASE_SPECIES_ID(species))
            {
                StringCopy(gStringVar1, GetSpeciesName(species));
                return GYM_HACK_TEAM_DUPLICATE;
            }
            if ((rules & TEAM_RULE_ITEM) && item != ITEM_NONE
             && GetMonData(&gParties[B_TRAINER_PLAYER][j], MON_DATA_HELD_ITEM) == item)
            {
                CopyItemName(item, gStringVar1);
                return GYM_HACK_TEAM_DUPLICATE_ITEM;
            }
        }
    }
    return GYM_HACK_TEAM_OK;
}

// The gyms' rules: one type, no two alike, no two holding the same item
u32 GymHack_CheckTeam(void)
{
    return GymHack_CheckTeamRules(TEAM_RULES_GYM);
}

// Battles where the rules are checked right before the battle starts. The
// Elite Four and Champion are checked once, at the door to the League
// (IndigoPlateau_PokemonCenter_1F), so a player can never get stuck inside.
bool32 GymHack_IsRuleBattleTrainer(u32 trainerId)
{
#if IS_FRLG
    switch (trainerId)
    {
    case TRAINER_LEADER_BROCK:
    case TRAINER_LEADER_MISTY:
    case TRAINER_LEADER_LT_SURGE:
    case TRAINER_LEADER_ERIKA:
    case TRAINER_LEADER_KOGA:
    case TRAINER_LEADER_SABRINA:
    case TRAINER_LEADER_BLAINE:
    case TRAINER_LEADER_GIOVANNI:
        return TRUE;
    }
    {
        u32 stage = GetRivalStage(trainerId);
        return stage != RIVAL_STAGE_NONE
            && stage != RIVAL_STAGE_CHAMPION_FIRST
            && stage != RIVAL_STAGE_CHAMPION_REMATCH;
    }
#else
    return FALSE;
#endif
}

extern const u8 EventScript_GymHack_TeamNotAllowed[];

// Called by the dotrainerbattle script command (src/scrcmd.c) just before a
// trainer battle starts. Returns the script to run instead when the battle
// isn't allowed, or NULL to battle normally.
const u8 *GymHack_GetBlockedBattleScript(void)
{
    u32 result;

    if (!GymHack_IsRuleBattleTrainer(TRAINER_BATTLE_PARAM.opponentA)
     && !GymHack_IsRuleBattleTrainer(TRAINER_BATTLE_PARAM.opponentB))
        return NULL;
    result = GymHack_CheckTeam();
    if (result == GYM_HACK_TEAM_OK)
        return NULL;
    gSpecialVar_Result = result;
    return EventScript_GymHack_TeamNotAllowed;
}

// special: VAR_RESULT = GymHack_CheckTeam()
void GymHack_CheckTeamSpecial(void)
{
    gSpecialVar_Result = GymHack_CheckTeam();
}

// special: reloads the current map with the player where they stand. Used
// after a blocked battle so that a rival who walked up for a scripted battle
// is put back, ready to try again once the team is fixed.
void GymHack_WarpInPlace(void)
{
    s16 x, y;
    PlayerGetDestCoords(&x, &y);
    SetWarpDestination(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum,
                       WARP_ID_NONE, x - MAP_OFFSET, y - MAP_OFFSET);
    DoDiveWarp();
    ResetInitialPlayerAvatarState();
}

// ---------------------------------------------------------------------------
// Evolution toggle (party menu: STOP EVOLVING / ALLOW EVOLVING)
// ---------------------------------------------------------------------------

bool32 GymHack_SpeciesCanEvolve(u32 species)
{
    const struct Evolution *evolutions = GetSpeciesEvolutions(species);
    u32 i;

    if (evolutions == NULL)
        return FALSE;
    for (i = 0; evolutions[i].method != EVOLUTIONS_END; i++)
    {
        if (SanitizeSpeciesId(evolutions[i].targetSpecies) != SPECIES_NONE)
            return TRUE;
    }
    return FALSE;
}

// ---------------------------------------------------------------------------
// HM key items: each works like its HM outside of battle (src/item_use.c for
// using them from the bag; checkfieldmove and the field move scripts for
// walking up to a tree, boulder or water).
// ---------------------------------------------------------------------------

EWRAM_DATA bool8 gGymHackFlyFromItem = FALSE;

u32 GymHack_GetFieldMoveItem(u32 fieldMove)
{
    switch (fieldMove)
    {
    case FIELD_MOVE_CUT:      return ITEM_HEDGE_CLIPPERS;
    case FIELD_MOVE_FLASH:    return ITEM_FLASHLIGHT;
    case FIELD_MOVE_SURF:     return ITEM_BOOGIE_BOARD;
    case FIELD_MOVE_FLY:      return ITEM_HOT_AIR_BALLOON;
    case FIELD_MOVE_STRENGTH: return ITEM_CAN_OF_SPINACH;
    case FIELD_MOVE_ROCK_SMASH: return ITEM_SLEDGEHAMMER;
    case FIELD_MOVE_WATERFALL:  return ITEM_ROCKET_BOOTS;
    }
    return ITEM_NONE;
}

bool32 GymHack_HasFieldMoveItem(u32 fieldMove)
{
    u32 item = GymHack_GetFieldMoveItem(fieldMove);
    return item != ITEM_NONE && CheckBagHasItem(item, 1);
}

// special: the next field move animation skips the "Pokemon pops out" part
void GymHack_SkipNextShowMon(void)
{
    gSkipShowMonAnim = TRUE;
}

// ---------------------------------------------------------------------------
// Gym type finder (Town Map, SELECT): which areas have a wild Pokemon whose
// primary type is the player's type. Only says *that* one is there, not which.
// ---------------------------------------------------------------------------

static bool32 MonListHasType(const struct WildPokemonInfo *info, u32 count, u32 type)
{
    u32 i;
    if (info == NULL || info->wildPokemon == NULL)
        return FALSE;
    for (i = 0; i < count; i++)
    {
        if (GetPrimaryType(info->wildPokemon[i].species) == type)
            return TRUE;
    }
    return FALSE;
}

bool32 GymHack_WildHeaderHasType(const struct WildPokemonHeader *header, u32 type)
{
    u32 t;
    for (t = 0; t < TIMES_OF_DAY_COUNT; t++)
    {
        const struct WildEncounterTypes *e = &header->encounterTypes[t];
        if (MonListHasType(e->landMonsInfo, NUM_LAND_MONS_ENCOUNTER_SLOTS, type)
         || MonListHasType(e->waterMonsInfo, NUM_WATER_MONS_ENCOUNTER_SLOTS, type)
         || MonListHasType(e->rockSmashMonsInfo, NUM_ROCK_SMASH_MONS_ENCOUNTER_SLOTS, type)
         || MonListHasType(e->fishingMonsInfo, NUM_FISHING_MONS_ENCOUNTER_SLOTS, type))
            return TRUE;
    }
    return FALSE;
}

// Fills `out` with the map sections on the given region map (Kanto or a Sevii
// map) that have at least one wild Pokemon of the player's type. Returns how many.
u32 GymHack_GetTypeFinderMapSecs(u16 *out, u32 max, u32 regionMapType)
{
    u32 type = VarGet(VAR_PLAYER_TYPE);
    u32 i, j, count = 0;

    if (type == TYPE_NONE)
        return 0;
    for (i = 0; gWildMonHeaders[i].mapGroup != MAP_GROUP(MAP_UNDEFINED) && count < max; i++)
    {
        u32 mapSec = Overworld_GetMapHeaderByGroupAndId(gWildMonHeaders[i].mapGroup, gWildMonHeaders[i].mapNum)->regionMapSectionId;
        if (mapSec >= MAPSEC_NONE || GetRegionMapType(mapSec) != regionMapType)
            continue;
        for (j = 0; j < count; j++)
        {
            if (out[j] == mapSec)
                break;
        }
        if (j < count)
            continue;
        if (GymHack_WildHeaderHasType(&gWildMonHeaders[i], type))
            out[count++] = mapSec;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Gen 4+ Pokémon: the gym's reward. Until the League makes the player a Gym Leader
// (FLAG_PALLET_GYM_AWARDED), none can be obtained: the places they're found wild (the Exotic
// Safari, Johto) open after it, and the pit's back room (src/pit_boss.c) leaves them out of its
// prizes till then. (Evolving and breeding into one still work.) By species: an Alolan or
// Galarian form of an older Pokémon isn't one; the hack's own species aren't either.
// ---------------------------------------------------------------------------
bool32 GymHack_IsModernSpecies(u32 species)
{
    u32 dex;
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return FALSE;
    dex = gSpeciesInfo[species].natDexNum;
    return dex > NATIONAL_DEX_DEOXYS && dex <= NATIONAL_DEX_PECHARUNT;
}

bool32 GymHack_SpeciesLocked(u32 species)
{
    return !FlagGet(FLAG_PALLET_GYM_AWARDED) && GymHack_IsModernSpecies(species);
}
