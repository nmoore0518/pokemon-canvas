#ifndef GUARD_BERRY_FIELD_H
#define GUARD_BERRY_FIELD_H

// Pokémon Canvas: the berry field in Pallet Town, the Tera berries (src/berry_field.c, GYM_HACK.md)
#include "constants/berry_field.h"

#define BERRY_FIELD_MAGIC       0xBE44
#define BERRY_STEPS_PER_STAGE   100     // seed, sprout, young, flowering, then ripe: 400 steps
#define TERA_BERRY_PRICE        3       // ARCHER's price for a Tera type change, in that type's berry
#define WILD_TERA_BERRY_PERCENT 5       // a wild Pokémon holds its primary type's berry

struct BerryPlot
{
    u16 item;       // the berry growing (ITEM_NONE: bare soil)
    u16 steps;      // steps since it was planted (stops at the ripe stage)
};

// After the tournament in sector 31 (src/save.c)
struct BerryFieldSave
{
    u16 magic;
    u16 checksum;
    struct BerryPlot plots[BERRY_FIELD_PLOTS];
};

extern struct BerryFieldSave gBerryFieldSave;

void BerryField_Clear(void);
void BerryField_Validate(void);
void BerryField_SealSave(void);
void BerryField_OnStep(void);
enum Item GetTeraBerry(enum Type type);
bool32 TryGiveWildMonTeraBerry(struct Pokemon *mon);

// specials
void BerryField_DrawPlots(void);
void BerryField_GetFacingPlot(void);
void BerryField_Plant(void);
void BerryField_Harvest(void);
void BerryField_PullUp(void);

#endif // GUARD_BERRY_FIELD_H
