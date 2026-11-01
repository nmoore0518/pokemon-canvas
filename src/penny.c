// Pokémon Canvas: Penny, the Eevee fan in Viridian City (data/maps/ViridianCity_PennysHouse_Frlg/).
// Show her an Eevee and she says how it evolves into the gym's type, and gives the item; once the
// gym is the player's, any type (a menu), one item each.
#include "global.h"
#include "penny.h"
#include "battle.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/species.h"

struct PennyEvo
{
    u16 item;
    u16 species;
    u8 method;
};

static const struct PennyEvo sEvos[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = {ITEM_SILK_SCARF,    SPECIES_ODEON,        PENNY_EVO_HOLD},
    [TYPE_FIGHTING] = {ITEM_BLACK_BELT,    SPECIES_ASSASSEON,    PENNY_EVO_HOLD},
    [TYPE_FLYING]   = {ITEM_SHARP_BEAK,    SPECIES_HYPEREON,     PENNY_EVO_HOLD},
    [TYPE_POISON]   = {ITEM_POISON_BARB,   SPECIES_POISEON,      PENNY_EVO_HOLD},
    [TYPE_GROUND]   = {ITEM_SOFT_SAND,     SPECIES_SUBTERRANEON, PENNY_EVO_HOLD},
    [TYPE_ROCK]     = {ITEM_HARD_STONE,    SPECIES_OBSIDEON,     PENNY_EVO_HOLD},
    [TYPE_BUG]      = {ITEM_SILVER_POWDER, SPECIES_LENNEON,      PENNY_EVO_HOLD},
    [TYPE_GHOST]    = {ITEM_SPELL_TAG,     SPECIES_APPARITEON,   PENNY_EVO_HOLD},
    [TYPE_STEEL]    = {ITEM_METAL_COAT,    SPECIES_AUTOMATEON,   PENNY_EVO_HOLD},
    [TYPE_DRAGON]   = {ITEM_DRAGON_FANG,   SPECIES_VALYREON,     PENNY_EVO_HOLD},
    [TYPE_FIRE]     = {ITEM_FIRE_STONE,    SPECIES_FLAREON,      PENNY_EVO_USE},
    [TYPE_WATER]    = {ITEM_WATER_STONE,   SPECIES_VAPOREON,     PENNY_EVO_USE},
    [TYPE_ELECTRIC] = {ITEM_THUNDER_STONE, SPECIES_JOLTEON,      PENNY_EVO_USE},
    [TYPE_GRASS]    = {ITEM_LEAF_STONE,    SPECIES_LEAFEON,      PENNY_EVO_USE},
    [TYPE_ICE]      = {ITEM_ICE_STONE,     SPECIES_GLACEON,      PENNY_EVO_USE},
    [TYPE_PSYCHIC]  = {ITEM_SUN_SHARD,     SPECIES_ESPEON,       PENNY_EVO_USE},
    [TYPE_DARK]     = {ITEM_MOON_SHARD,    SPECIES_UMBREON,      PENNY_EVO_USE},
    [TYPE_FAIRY]    = {ITEM_NONE,          SPECIES_SYLVEON,      PENNY_EVO_FRIENDSHIP},
};

static const struct PennyEvo *GetEvo(u32 type)
{
    if (type >= NUMBER_OF_MON_TYPES || sEvos[type].species == SPECIES_NONE)
        type = TYPE_NORMAL;
    return &sEvos[type];
}

// for tests and the guide
u32 Penny_GetItem(u32 type)    { return GetEvo(type)->item; }
u32 Penny_GetSpecies(u32 type) { return GetEvo(type)->species; }

// VAR_RESULT: an Eevee (not an Egg) in the party
void Penny_HasEevee(void)
{
    u32 i;
    gSpecialVar_Result = FALSE;
    for (i = 0; i < gPlayerPartyCount; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES_OR_EGG) == SPECIES_EEVEE)
            gSpecialVar_Result = TRUE;
    }
}

// VAR_0x8004 = a type. STR_VAR_1 = the type, STR_VAR_2 = the item, STR_VAR_3 = what Eevee becomes;
// VAR_0x8005 = the item; VAR_RESULT = PENNY_EVO_*
void Penny_BufferEvo(void)
{
    const struct PennyEvo *evo = GetEvo(gSpecialVar_0x8004);
    StringCopy(gStringVar1, gTypesInfo[gSpecialVar_0x8004 < NUMBER_OF_MON_TYPES ? gSpecialVar_0x8004 : TYPE_NORMAL].name);
    if (evo->item != ITEM_NONE)
        CopyItemName(evo->item, gStringVar2);
    else
        gStringVar2[0] = EOS;
    StringCopy(gStringVar3, GetSpeciesName(evo->species));
    gSpecialVar_0x8005 = evo->item;
    gSpecialVar_Result = evo->method;
}

// VAR_0x8004 = a type. VAR_RESULT: she's given that type's item already
void Penny_WasGiven(void)
{
    gSpecialVar_Result = gSpecialVar_0x8004 < NUMBER_OF_MON_TYPES && FlagGet(FLAG_PENNY_GAVE_TYPE + gSpecialVar_0x8004);
}

void Penny_MarkGiven(void)
{
    if (gSpecialVar_0x8004 < NUMBER_OF_MON_TYPES)
        FlagSet(FLAG_PENNY_GAVE_TYPE + gSpecialVar_0x8004);
}
