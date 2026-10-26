#ifndef GUARD_GYM_DECOR_H
#define GUARD_GYM_DECOR_H

#include "constants/gym_decor.h"
#include "constants/pokemon.h"

struct FieldInput;
struct MapLayout;
struct DynamicListMenuEventArgs;

// Shop and storage categories
#define GYM_DECOR_CAT_GENERAL  0
#define GYM_DECOR_CAT_TRAINERS 1
#define GYM_DECOR_CAT_TYPE(type) (2 + (type))
#define GYM_DECOR_CAT_TROPHIES GYM_DECOR_CAT_TYPE(NUMBER_OF_MON_TYPES)
#define GYM_DECOR_CAT_FLOORS   (GYM_DECOR_CAT_TROPHIES + 1)
#define GYM_DECOR_CAT_WALLS    (GYM_DECOR_CAT_TROPHIES + 2)
#define GYM_DECOR_CAT_COUNT    (GYM_DECOR_CAT_TROPHIES + 3)

// What a catalog entry is
#define GYM_DECOR_KIND_PIECE      0     // furniture and the like: its looks, in each way it can face
#define GYM_DECOR_KIND_WALL       1     // a wall square, joined up with the walls around it
#define GYM_DECOR_KIND_WATER      2     // a pool square, its edges joined up the same way
#define GYM_DECOR_KIND_FLOOR      3     // a floor style, painted square by square (bought once)
#define GYM_DECOR_KIND_WALL_STYLE 4     // a style for all the gym's walls (bought once)

// A piece facing one way (0 down, 1 left, 2 up, 3 right): its size and a look per square
// (src/gym_room.c), top row first
struct GymDecorFacing
{
    u8 width;
    u8 height;
    const u16 *looks;
    const u16 *editLooks;       // shown instead while decorating (invisible walls), or NULL
};

struct GymDecor
{
    const u8 *name;
    const u8 *description;
    u32 price;
    u16 unlockFlag;             // a flag (usually a badge) needed before it's sold; 0 = always
    u8 category;                // GYM_DECOR_CAT_*: type decorations unlock with key battles
    u8 kind;                    // GYM_DECOR_KIND_*
    u8 style;                   // (a floor or wall style's number)
    u8 numFacings;              // (1 for a piece that looks the same every way round)
    struct GymDecorFacing facings[4];
};

extern const struct GymDecor gGymDecor[GYM_DECOR_COUNT];

// The save (where Emerald keeps secret bases, unused in FireRed): what's owned, what's placed
// where, and each square's floor and the wall style. Its size can't change: the gym challenge's
// records come right after it (GymDecor_GetSpareSaveSpace).
#define GYM_DECOR_MAX_TYPES  256
#define GYM_DECOR_MAX_PLACED 560    // (the room has about 500 free squares)

struct GymDecorPlacement
{
    u8 id;
    u8 x;
    u8 y;
    u8 room;                    // the way it faces (bits 0-1), and the room (0: the gym) above
};

#define GYM_PLACEMENT_FACING(p) ((p)->room & 3)
#define GYM_PLACEMENT_ROOM(p)   ((p)->room >> 2)

struct GymRoomStyle
{
    u8 floors[350];             // each square's floor style, 4 bits each (25 x 28 squares)
    u8 wallStyle;
    u8 unused[9];
};

struct GymDecorSaveData
{
    u8 owned[GYM_DECOR_MAX_TYPES];      // how many of each the player owns (placed or stored)
    u16 numPlaced;
    u16 magic;                          // GYM_DECOR_SAVE_MAGIC once set up (older saves had other data here)
    struct GymDecorPlacement placed[GYM_DECOR_MAX_PLACED];
    struct GymRoomStyle style;          // (where placements 560-649 were: never used)
};

struct GymDecorSaveData *GymDecor_GetSaveData(void);
struct GymRoomStyle *GymDecor_GetRoomStyle(void);

bool32 GymDecor_IsUnlocked(u32 id);
void GymDecor_Give(u32 id);
bool32 GymDecor_IsTypeUnlocked(u32 type);
void GymDecor_PatchTilesetGfx(struct MapLayout const *mapLayout);
u32 GymDecor_CountOwned(u32 id);
bool32 GymDecor_FindPlaced(u32 id, s32 *x, s32 *y);
u32 GymDecor_CountPlaced(u32 id);
u32 GymDecor_CountAvailable(u32 id);
u32 GymDecor_NumPlaced(void);
bool32 GymDecor_CanPlace(u32 id, u32 facing, s32 x, s32 y);
bool32 GymDecor_Place(u32 id, u32 facing, s32 x, s32 y);
bool32 GymDecor_IsOwnedStyle(u32 id);
void GymDecor_ApplyWallStyle(void);
void GymDecor_GetKind(void);
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

bool32 GymDecor_CheckNewUnlocks(void);
void GymDecor_MarkUnlocksSeen(void);

#endif // GUARD_GYM_DECOR_H
