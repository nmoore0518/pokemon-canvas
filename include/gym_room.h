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
    u32 door[4];        // a doorway in the back wall's front face
};

// The gym and its extra rooms (each its own map: tools/gym_hack/build_gym_rooms.py). Oak gives
// the extra rooms for catching 100, 200 and 300 kinds of POKéMON (FLAG_GYM_ROOM_BACK...): their
// doorways in the gym's walls open then. Each room has its own floors, wall style and PC;
// decorations come from the gym's storage (each placement says its room).
#define GYM_ROOM_MAIN   0
#define GYM_ROOM_BACK   1
#define GYM_ROOM_LEFT   2
#define GYM_ROOM_RIGHT  3
#define GYM_ROOM_COUNT  4
#define GYM_ROOM_NONE   0xFF

#define GYM_EXTRA_ROOM_WIDTH  17
#define GYM_EXTRA_ROOM_HEIGHT 15

struct GymRoomInfo
{
    const u16 *base;            // the empty room: a look (or a GYM_BASE_* code) per square
    u16 map;
    u8 width;
    u8 height;
    u8 edit[4];                 // decorations go inside this rectangle (x0, y0, x1, y1, inclusive)...
    u8 reserved[4];             // ...but not inside this one (the way in, and the PC)
    u8 pcFrontX;
    u8 pcFrontY;
    u8 matX0;                   // the doormat (row height - 2), kept clear while decorating;
    u8 matX1;                   // 0xFF: none
};

struct GymRoomDoor
{
    u8 room;
    u8 x;                       // a doorway: in the back wall's face, or a side wall
    u8 y;
    u8 frontX;                  // the square in front, kept clear
    u8 frontY;
    u8 behavior;                // the way through (an arrow warp)
    u16 flag;                   // open while set (0: always)
};

u32 GymRoom_Current(void);                      // GYM_ROOM_* (or GYM_ROOM_NONE: not in one)
const struct GymRoomInfo *GymRoom_GetInfo(u32 room);
const struct GymRoomInfo *GymRoom_CurrentInfo(void);
bool32 GymRoom_IsDoorFront(u32 room, s32 x, s32 y);
u32 GymRoom_GetBase(u32 room, s32 x, s32 y);
bool32 GymRoom_PieceFitsWalls(u32 id, u32 facing, s32 x, s32 y);

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

#endif // GUARD_GYM_ROOM_H
