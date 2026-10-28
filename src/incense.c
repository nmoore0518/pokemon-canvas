// Pokémon Canvas: the Incense. Lit, it makes a place's rare Pokémon 5 times as likely for 600 steps
// (VAR_INCENSE_STEPS; like a Repel, it asks to light another when it runs out:
// data/scripts/incense.inc).
//
// A Pokémon is rare if it's 10% or less of that method's encounters there (its slots added up, and
// an Exotic Safari shared slot split between its species). Each rare one's chance is multiplied by
// 5; the common ones share what's left, in the same proportions as before. If the rare ones would
// add up to more than 100% (say 25% of rare Pokémon: 125%), there's no room for the common ones:
// the rare ones get it all, still in their own proportions.
#include "global.h"
#include "incense.h"
#include "wild_encounter.h"
#include "event_data.h"
#include "exotic_safari.h"
#include "item.h"
#include "random.h"
#include "script.h"
#include "constants/items.h"

extern const u8 EventScript_IncenseWoreOff[];

bool32 Incense_IsActive(void)
{
    return VarGet(VAR_INCENSE_STEPS) != 0;
}

void Incense_Start(void)
{
    VarSet(VAR_INCENSE_STEPS, INCENSE_STEPS);
}

// A step: when it burns out, the script offers another
bool32 Incense_OnStep(void)
{
    u16 steps = VarGet(VAR_INCENSE_STEPS);
    if (steps == 0)
        return FALSE;
    VarSet(VAR_INCENSE_STEPS, --steps);
    if (steps != 0)
        return FALSE;
    ScriptContext_SetupScript(EventScript_IncenseWoreOff);
    return TRUE;
}

// Special (data/scripts/incense.inc): light another one
void Incense_UseAnother(void)
{
    if (RemoveBagItem(ITEM_INCENSE, 1))
        Incense_Start();
}

// A slot's species's share of the method's encounters, in hundredths of a percent
static u32 SpeciesShare(const struct WildPokemon *mons, u32 first, const u8 *rates, u32 count, u32 slot)
{
    u32 i, total = 0;
    for (i = 0; i < count; i++)
    {
        if (mons[first + i].species == mons[first + slot].species)
            total += rates[i];
    }
    return total * 100 / ExoticSafari_PoolSize(mons[first + slot].species);
}

// Each slot's weight with the Incense lit (out of 100 * scale)
static void Weights(const struct WildPokemon *mons, u32 first, const u8 *rates, u32 count, u32 *weights, u32 scale)
{
    u32 i, rare = 0, common = 0;
    bool8 isRare[12];

    for (i = 0; i < count; i++)
    {
        isRare[i] = SpeciesShare(mons, first, rates, count, i) <= INCENSE_RARE_PERCENT * 100;
        if (isRare[i])
            rare += rates[i];
        else
            common += rates[i];
    }
    for (i = 0; i < count; i++)
    {
        if (rare == 0 || common == 0)                      // (nothing to boost, or nothing to take from)
            weights[i] = rates[i] * scale;
        else if (rare * INCENSE_BOOST >= 100)              // (no room left for the common ones)
            weights[i] = isRare[i] ? rates[i] * 100 * scale / rare : 0;
        else if (isRare[i])
            weights[i] = rates[i] * INCENSE_BOOST * scale;
        else
            weights[i] = rates[i] * (100 - rare * INCENSE_BOOST) * scale / common;
    }
}

// The slot (0-based within these count slots) to use, with the Incense lit
u32 Incense_ChooseSlot(const struct WildPokemon *mons, u32 first, const u8 *rates, u32 count)
{
    u32 weights[12], i, total = 0, r;

    Weights(mons, first, rates, count, weights, 100);
    for (i = 0; i < count; i++)
        total += weights[i];
    if (total == 0)
        return 0;
    r = Random32() % total;
    for (i = 0; i < count; i++)
    {
        if (r < weights[i])
            return i;
        r -= weights[i];
    }
    return count - 1;
}

// (for the tests) a slot's chance with the Incense lit, out of 100 * scale
u32 Incense_BoostedChance(const struct WildPokemon *mons, u32 first, const u8 *rates, u32 count, u32 slot, u32 scale)
{
    u32 weights[12];
    Weights(mons, first, rates, count, weights, scale);
    return weights[slot];
}
