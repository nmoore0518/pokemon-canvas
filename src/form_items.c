// Pokémon Canvas: items for Pokémon forms that the world hands out in its own ways.
//
// GENESECT'S DRIVES: item balls in Kanto (Viridian Forest, the Safari Zone's east area, Route 25 by
// Bill's house, Route 12's grass) that are only there once Genesect has been caught: each ball's flag
// is the item's own (set when picked up); until Genesect is caught, they stay hidden
// (src/event_object_movement.c asks FormItems_ObjectWaits).
//
// ARCEUS'S PLATES: each time the player masters a gym type, that type's plate hides somewhere on
// the gym's floor, at a spot picked for the save (moved to the nearest open floor if decorations
// cover it). Nothing says so (only a hint from the
// Snowpoint elder): pressing A facing the spot, or standing on it, finds it like any hidden item.
// The Normal type has no plate.
#include "global.h"
#include "form_items.h"
#include "event_data.h"
#include "event_scripts.h"
#include "gym_legend.h"
#include "pokedex.h"
#include "random.h"
#include "constants/flags.h"
#include "constants/gym_room.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/species.h"

bool32 FormItems_ObjectWaits(u16 flagId)
{
    if (flagId < FLAG_GOT_BURN_DRIVE || flagId > FLAG_GOT_SHOCK_DRIVE)
        return FALSE;
    return !GetSetPokedexFlag(SpeciesToNationalPokedexNum(SPECIES_GENESECT), FLAG_GET_CAUGHT);
}

static const u16 sPlates[NUMBER_OF_MON_TYPES] =
{
    [TYPE_FIGHTING] = ITEM_FIST_PLATE,   [TYPE_FLYING] = ITEM_SKY_PLATE,     [TYPE_POISON] = ITEM_TOXIC_PLATE,
    [TYPE_GROUND] = ITEM_EARTH_PLATE,    [TYPE_ROCK] = ITEM_STONE_PLATE,     [TYPE_BUG] = ITEM_INSECT_PLATE,
    [TYPE_GHOST] = ITEM_SPOOKY_PLATE,    [TYPE_STEEL] = ITEM_IRON_PLATE,     [TYPE_FIRE] = ITEM_FLAME_PLATE,
    [TYPE_WATER] = ITEM_SPLASH_PLATE,    [TYPE_GRASS] = ITEM_MEADOW_PLATE,   [TYPE_ELECTRIC] = ITEM_ZAP_PLATE,
    [TYPE_PSYCHIC] = ITEM_MIND_PLATE,    [TYPE_ICE] = ITEM_ICICLE_PLATE,     [TYPE_DRAGON] = ITEM_DRACO_PLATE,
    [TYPE_DARK] = ITEM_DREAD_PLATE,      [TYPE_FAIRY] = ITEM_PIXIE_PLATE,
};

static bool32 InGym(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_PALLET_TOWN_GYM)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_PALLET_TOWN_GYM);
}

// Where a type's plate rests: a spot of the gym's floor picked for this save (from the player's
// trainer id), or if decorations cover it now, the open floor nearest it
static void PlateSpot(u32 type, s32 *outX, s32 *outY)
{
    const u8 *id = gSaveBlock2Ptr->playerTrainerId;
    u32 h = (id[0] | (id[1] << 8) | (id[2] << 16) | (id[3] << 24)) ^ (type * 0x9E3779B9);
    s32 bx, by, r, dx, dy;
    h ^= h >> 15; h *= 0x2C1B3C6D; h ^= h >> 12; h *= 0x297A2D39; h ^= h >> 15;
    bx = 1 + h % (GYM_ROOM_WIDTH - 2);
    by = 2 + (h >> 8) % (GYM_ROOM_HEIGHT - 6);
    for (r = 0; r < GYM_ROOM_WIDTH; r++)
    {
        for (dy = -r; dy <= r; dy++)
        {
            for (dx = -r; dx <= r; dx++)
            {
                if ((abs(dx) == r || abs(dy) == r) && GymLegend_IsOpenFloor(bx + dx, by + dy))
                {
                    *outX = bx + dx;
                    *outY = by + dy;
                    return;
                }
            }
        }
    }
    *outX = bx;
    *outY = by;
}

// The A button in the gym at (x, y): a plate there (its type mastered, not found yet)? Then the
// hidden-item script for it (VAR_0x8004 its flag, VAR_0x8005 the item)
static const u8 *PlateAt(s32 x, s32 y)
{
    u32 type;
    s32 px, py;
    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (sPlates[type] == ITEM_NONE || !FlagGet(FLAG_GYM_TYPE_MASTERED + type) || FlagGet(FLAG_GYM_PLATE_FOUND_FIRST + type))
            continue;
        PlateSpot(type, &px, &py);
        if (px != x || py != y)
            continue;
        gSpecialVar_0x8004 = FLAG_GYM_PLATE_FOUND_FIRST + type;
        gSpecialVar_0x8005 = sPlates[type];
        gSpecialVar_0x8009 = 1;
        return EventScript_HiddenItemScript;
    }
    return NULL;
}

const u8 *FormItems_GetPlateScript(s32 x, s32 y)
{
    s16 px, py;
    const u8 *script;
    if (!InGym())
        return NULL;
    script = PlateAt(x, y);
    if (script != NULL)
        return script;
    px = gSaveBlock1Ptr->pos.x;
    py = gSaveBlock1Ptr->pos.y;
    return PlateAt(px, py);
}
