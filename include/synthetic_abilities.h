#ifndef GUARD_SYNTHETIC_ABILITIES_H
#define GUARD_SYNTHETIC_ABILITIES_H

// Pokémon Canvas: the ability each species gets from the Ability Synthesizer (Dad's basement)
// (data: tools/gym_hack/synthetic_abilities.txt -> src/data/pokemon/synthetic_abilities.h)

// MON_DATA_ABILITY_NUM's 4th value (0-1 regular, 2 hidden): the Pokémon has its species' synthetic ability.
// GetAbilityBySpecies / GetSpeciesAbility look it up, so battle, the summary screen, evolution and form
// changes all follow it. It isn't passed on by breeding, and Ability Capsules and Patches don't work on it.
#define ABILITY_NUM_SYNTHETIC 3

// The species' synthetic ability, or ABILITY_NONE if the Synthesizer won't take it (a flagged line).
u16 GetSpeciesSyntheticAbility(u16 species);
// The listed one even for flagged species (the Pokédex shows it)
u16 GetSpeciesListedSyntheticAbility(u16 species);
bool32 IsSpeciesSyntheticBlocked(u16 species);

#endif // GUARD_SYNTHETIC_ABILITIES_H
