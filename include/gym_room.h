#ifndef GUARD_GYM_ROOM_H
#define GUARD_GYM_ROOM_H

// Pokémon Canvas: putting the gym's room together (src/gym_room.c). Every square is drawn from
// its floor and what's on it, in the gym tileset's blocks (kept in RAM) and free video memory.

// A look: one square's two layers of tiles (references into the art bank; see
// tools/gym_hack/build_gym_decor.py), its block attributes and these flags
#define GYM_LOOK_FLOOR_UNDER 1      // its bottom layer is the square's floor
#define GYM_LOOK_SOLID       2
#define GYM_LOOK_WATER       4

struct GymLook
{
    u32 tiles[8];
    u32 attr;
    u8 flags;
};

struct GymWallStyle
{
    u32 top[5];         // the top face's top left quarter, by quarter case (flipped for the others)
    u32 face[4][4];     // the front face: no ends, left end, right end, both ends
};

// sGymRoomBase codes
#define GYM_BASE_FLOOR          0xFFFF
#define GYM_BASE_WALL_TOP       0xFFFE
#define GYM_BASE_WALL_FACE      0xFFFD
#define GYM_BASE_FLOOR_SHADOW   0xFFFC

// What the room shows while decorating, on top of what's placed
#define GYM_PREVIEW_NONE   0
#define GYM_PREVIEW_PIECE  1    // a decoration (id, facing) at (x, y)
#define GYM_PREVIEW_FLOOR  2    // a floor style on one square

extern u16 gGymRoomMetatiles[];
extern u32 gGymRoomMetatileAttrs[];

void GymRoom_Load(void);
void GymRoom_Build(bool32 editing);
void GymRoom_Refresh(s32 x, s32 y, s32 w, s32 h, bool32 editing);
void GymRoom_SetPreview(u32 kind, u32 id, u32 facing, s32 x, s32 y);
bool32 GymRoom_Overflowed(void);
void GymRoom_OnTilesetLoaded(void);
void GymRoom_TilesetAnim(u16 timer);

// Square by square
u32 GymRoom_GetFloor(s32 x, s32 y);
void GymRoom_SetFloor(s32 x, s32 y, u32 style);
bool32 GymRoom_IsPaintable(s32 x, s32 y);
u32 GymRoom_FillFloor(s32 x, s32 y, u32 style, bool32 (*allowed)(s32 x, s32 y));
bool32 GymRoom_IsSolidFloor(u32 style);
u32 GymRoom_GetWallStyle(void);
void GymRoom_SetWallStyle(u32 style);
bool32 GymRoom_HasWallFace(s32 x, s32 y, s32 w, s32 h);

#endif // GUARD_GYM_ROOM_H
