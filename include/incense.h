#ifndef GUARD_INCENSE_H
#define GUARD_INCENSE_H

struct WildPokemon;

// Pokémon Canvas: the Incense (src/incense.c). For INCENSE_STEPS steps, every wild Pokémon that's
// 10% or less of a place's encounters (by grass, surfing, a rod or Rock Smash) is 5 times as likely.
#define INCENSE_STEPS          600     // (the Safari Zone's)
#define INCENSE_RARE_PERCENT   10
#define INCENSE_BOOST          5

bool32 Incense_IsActive(void);
void Incense_Start(void);
bool32 Incense_OnStep(void);
u32 Incense_ChooseSlot(const struct WildPokemon *mons, u32 first, const u8 *rates, u32 count);
u32 Incense_BoostedChance(const struct WildPokemon *mons, u32 first, const u8 *rates, u32 count, u32 slot, u32 scale);
void Incense_UseAnother(void);

#endif // GUARD_INCENSE_H
