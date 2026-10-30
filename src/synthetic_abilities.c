#include "global.h"
#include "synthetic_abilities.h"
#include "constants/abilities.h"
#include "constants/species.h"
#include "pokedex.h"
#include "pokemon.h"

#include "data/pokemon/synthetic_abilities.h"

bool32 IsSpeciesSyntheticBlocked(u16 species)
{
    for (u32 i = 0; i < ARRAY_COUNT(gSpeciesSyntheticBlocked); i++)
    {
        if (gSpeciesSyntheticBlocked[i] == species)
            return TRUE;
    }
    return FALSE;
}

u16 GetSpeciesListedSyntheticAbility(u16 species)
{
    if (species >= NUM_SPECIES)
        return ABILITY_NONE;
    // (the hack's own species, like DAD's CHANSEY, share their National Dex species')
    if (gSpeciesSyntheticAbilities[species] == ABILITY_NONE && gSpeciesInfo[species].natDexNum != NATIONAL_DEX_NONE)
        return gSpeciesSyntheticAbilities[NationalPokedexNumToSpecies(gSpeciesInfo[species].natDexNum)];
    return gSpeciesSyntheticAbilities[species];
}

u16 GetSpeciesSyntheticAbility(u16 species)
{
    if (IsSpeciesSyntheticBlocked(species))
        return ABILITY_NONE;
    return GetSpeciesListedSyntheticAbility(species);
}
