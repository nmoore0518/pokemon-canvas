#include "global.h"
#include "synthetic_abilities.h"
#include "constants/abilities.h"
#include "constants/species.h"

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
    return gSpeciesSyntheticAbilities[species];
}

u16 GetSpeciesSyntheticAbility(u16 species)
{
    if (IsSpeciesSyntheticBlocked(species))
        return ABILITY_NONE;
    return GetSpeciesListedSyntheticAbility(species);
}
