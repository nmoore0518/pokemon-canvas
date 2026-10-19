#ifndef GUARD_GYM_DECOR_H
#define GUARD_GYM_DECOR_H

#include "constants/gym_decor.h"
#include "constants/pokemon.h"

struct FieldInput;
struct MapLayout;
struct DynamicListMenuEventArgs;

// A block in a decoration's footprint: a metatile id plus these flags
#define GYM_DECOR_SOLID 0x400   // can't be walked through
#define GYM_DECOR_WATER 0x800   // water at the surfing height

// Shop and storage categories
#define GYM_DECOR_CAT_GENERAL  0
#define GYM_DECOR_CAT_TRAINERS 1
#define GYM_DECOR_CAT_TYPE(type) (2 + (type))
#define GYM_DECOR_CAT_COUNT    GYM_DECOR_CAT_TYPE(NUMBER_OF_MON_TYPES)

struct GymDecor
{
    const u8 *name;
    const u8 *description;
    u32 price;
    u16 unlockFlag;             // a flag (usually a badge) needed before it's sold; 0 = always
    u8 category;                // GYM_DECOR_CAT_*: type decorations unlock with key battles
    u8 width;
    u8 height;
    const u16 *blocks;          // width * height, top row first
    const u16 *editBlocks;      // shown instead while decorating (invisible walls), or NULL
};

extern const struct GymDecor gGymDecor[GYM_DECOR_COUNT];

// How many decorations the save can hold
#define GYM_DECOR_MAX_TYPES  256
#define GYM_DECOR_MAX_PLACED 650   // the room has about 500 free squares; the rest of the space holds gym challenge records

bool32 GymDecor_IsUnlocked(u32 id);
bool32 GymDecor_IsTypeUnlocked(u32 type);
void GymDecor_PatchTilesetGfx(struct MapLayout const *mapLayout);
u32 GymDecor_CountOwned(u32 id);
bool32 GymDecor_FindPlaced(u32 id, s32 *x, s32 *y);
u32 GymDecor_CountPlaced(u32 id);
u32 GymDecor_CountAvailable(u32 id);
u32 GymDecor_NumPlaced(void);
bool32 GymDecor_CanPlace(u32 id, s32 x, s32 y);
bool32 GymDecor_Place(u32 id, s32 x, s32 y);
bool32 GymDecor_RemoveAt(s32 x, s32 y);
void GymDecor_ClearAll(void);
void *GymDecor_GetSpareSaveSpace(u32 *size);

// Specials
void GymDecor_DrawRoom(void);
void GymDecor_PushShopList(void);
void GymDecor_PushShopCategories(void);
void GymDecor_PushOwnedCategories(void);
void GymDecor_PushOwnedList(void);
void GymDecor_BufferDecorInfo(void);
void GymDecor_TryBuy(void);
void GymDecor_StartEditing(void);
void GymDecor_SetupTrainers(void);

// Practice trainers (people in the gym map for them; see src/gym_decor.c)
#define GYM_PRACTICE_SLOTS 11
u32 GymDecor_CountPlacedTrainers(void);
bool32 GymDecor_GetTrainerSlot(u32 slot, u32 *decor, u32 *instance, s32 *x, s32 *y);
void GymDecor_SetOwned(u32 id, u32 count);

// Edit mode hooks
bool32 GymDecor_IsEditing(void);
bool32 GymDecor_HandleEditInput(struct FieldInput *input);

// Dynamic multichoice callbacks (the shop and storage lists)
void GymDecor_ListOnInit(struct DynamicListMenuEventArgs *eventArgs);
void GymDecor_ShopListOnSelectionChanged(struct DynamicListMenuEventArgs *eventArgs);
void GymDecor_OwnedListOnSelectionChanged(struct DynamicListMenuEventArgs *eventArgs);
void GymDecor_ListOnDestroy(struct DynamicListMenuEventArgs *eventArgs);

#endif // GUARD_GYM_DECOR_H
