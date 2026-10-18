#ifndef GUARD_GYM_DECOR_H
#define GUARD_GYM_DECOR_H

#include "constants/gym_decor.h"

struct FieldInput;
struct DynamicListMenuEventArgs;

// A block in a decoration's footprint: a metatile id plus these flags
#define GYM_DECOR_SOLID 0x400   // can't be walked through
#define GYM_DECOR_WATER 0x800   // water at the surfing height

struct GymDecor
{
    const u8 *name;
    const u8 *description;
    u32 price;
    u16 unlockFlag;             // a flag (usually a badge) needed before it's sold; 0 = always
    u8 width;
    u8 height;
    const u16 *blocks;          // width * height, top row first
    const u16 *editBlocks;      // shown instead while decorating (invisible walls), or NULL
};

extern const struct GymDecor gGymDecor[GYM_DECOR_COUNT];

// How many decorations the save can hold
#define GYM_DECOR_MAX_TYPES  256
#define GYM_DECOR_MAX_PLACED 700

bool32 GymDecor_IsUnlocked(u32 id);
u32 GymDecor_CountOwned(u32 id);
u32 GymDecor_CountPlaced(u32 id);
u32 GymDecor_CountAvailable(u32 id);
u32 GymDecor_NumPlaced(void);
bool32 GymDecor_CanPlace(u32 id, s32 x, s32 y);
bool32 GymDecor_Place(u32 id, s32 x, s32 y);
bool32 GymDecor_RemoveAt(s32 x, s32 y);
void GymDecor_ClearAll(void);

// Specials
void GymDecor_DrawRoom(void);
void GymDecor_PushShopList(void);
void GymDecor_PushOwnedList(void);
void GymDecor_BufferDecorInfo(void);
void GymDecor_TryBuy(void);
void GymDecor_StartEditing(void);

// Edit mode hooks
bool32 GymDecor_IsEditing(void);
bool32 GymDecor_HandleEditInput(struct FieldInput *input);

// Dynamic multichoice callbacks (the shop and storage lists)
void GymDecor_ListOnInit(struct DynamicListMenuEventArgs *eventArgs);
void GymDecor_ShopListOnSelectionChanged(struct DynamicListMenuEventArgs *eventArgs);
void GymDecor_OwnedListOnSelectionChanged(struct DynamicListMenuEventArgs *eventArgs);
void GymDecor_ListOnDestroy(struct DynamicListMenuEventArgs *eventArgs);

#endif // GUARD_GYM_DECOR_H
