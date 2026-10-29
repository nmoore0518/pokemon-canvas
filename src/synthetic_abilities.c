#include "global.h"
#include "synthetic_abilities.h"
#include "constants/abilities.h"
#include "constants/species.h"

#include "data/pokemon/synthetic_abilities.h"

u16 GetSpeciesSyntheticAbility(u16 species)
{
    if (species >= NUM_SPECIES)
        return ABILITY_NONE;
    return gSpeciesSyntheticAbilities[species];
}
