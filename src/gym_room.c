// Pokémon Canvas: putting the gym's room together.
//
// The gym's tileset doesn't hold the decorations: there are far too many of them, each on any of
// the floors, for its 384 blocks and its share of video memory. Instead the room is put together
// whenever it's drawn: for every square, what's there - the floor the player painted, a
// decoration (in the way it faces), a wall or pool water that joins up with its neighbors, a
// wall's front face - becomes a block in the gym tileset (whose blocks are kept in RAM: see
// gTileset_PalletGym), and the art it uses is copied into free video memory. Identical squares
// share a block, and art already in video memory is reused.
//
// The same goes for the gym's extra rooms (GYM_ROOM_BACK...: each its own map, smaller, with its
// own floors and wall style; tools/gym_hack/build_gym_rooms.py), and the doorways to them in the
// gym's walls, open once the room is the player's.
//
// The art bank, the looks and the decoration catalog come from tools/gym_hack/build_gym_decor.py
// (src/data/gym_decor.h). If a room ever needs more blocks or art than there's room for, the
// squares that didn't fit show plain floor, and decorating refuses the piece that caused it
// (GymRoom_Overflowed).
#include "global.h"
#include "gym_room.h"
#include "gym_decor.h"
#include "bg.h"
#include "fieldmap.h"
#include "field_camera.h"
#include "malloc.h"
#include "overworld.h"
#include "event_data.h"
#include "constants/flags.h"
#include "constants/gym_room.h"
#include "constants/maps.h"
#include "constants/metatile_behaviors.h"

#include "data/gym_decor.h"

#define NUM_SLOTS        (NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY_FRLG)    // 384
#define FIRST_SLOT       NUM_METATILES_IN_PRIMARY_FRLG
#define SLOT_HASH_SIZE   512
#define ART_HASH_SIZE    1024
#define ROOM_CELLS       (GYM_ROOM_WIDTH * GYM_ROOM_HEIGHT)

#define REF_ART(r)       ((r) & 0xFFF)
#define REF_FLIPS(r)     (((r) >> 12) & 3)
#define REF_PAL(r)       (((r) >> 14) & 0xF)
#define REF_FIXED        (1 << 18)
#define HFLIP            1
#define VFLIP            2

EWRAM_DATA u16 gGymRoomMetatiles[NUM_SLOTS * NUM_TILES_PER_METATILE] = {0};
EWRAM_DATA u32 gGymRoomMetatileAttrs[NUM_SLOTS] = {0};         // (FireRed's attributes are 32 bits)

static EWRAM_DATA u16 sNumSlots = 0;
static EWRAM_DATA u16 sSlotHash[SLOT_HASH_SIZE] = {0};          // slot + 1
static EWRAM_DATA u16 sArtHashKey[ART_HASH_SIZE] = {0};         // art + 1
static EWRAM_DATA u16 sArtHashVram[ART_HASH_SIZE] = {0};
static EWRAM_DATA u16 sVramUsed = 0;
static EWRAM_DATA bool8 sOverflow = FALSE;
static EWRAM_DATA bool8 sLive = FALSE;          // video memory can be written now (not mid-load)
static EWRAM_DATA bool8 sNeedUpload = FALSE;
static EWRAM_DATA u8 sPreviewKind = 0;
static EWRAM_DATA u8 sPreviewId = 0;
static EWRAM_DATA u8 sPreviewFacing = 0;
static EWRAM_DATA s8 sPreviewX = 0;
static EWRAM_DATA s8 sPreviewY = 0;

// ---- The rooms ----
u32 GymRoom_Current(void)
{
    u16 map = (gSaveBlock1Ptr->location.mapGroup << 8) | gSaveBlock1Ptr->location.mapNum;
    u32 r;
    for (r = 0; r < GYM_ROOM_COUNT; r++)
    {
        if (sGymRooms[r].map == map)
            return r;
    }
    return GYM_ROOM_NONE;
}

static u32 Room(void)
{
    u32 r = GymRoom_Current();
    return r == GYM_ROOM_NONE ? GYM_ROOM_MAIN : r;
}

const struct GymRoomInfo *GymRoom_GetInfo(u32 room)
{
    return &sGymRooms[room < GYM_ROOM_COUNT ? room : GYM_ROOM_MAIN];
}

const struct GymRoomInfo *GymRoom_CurrentInfo(void)
{
    return &sGymRooms[Room()];
}

static bool32 InRoomOf(const struct GymRoomInfo *info, s32 x, s32 y)
{
    return x >= 0 && y >= 0 && x < info->width && y < info->height;
}

u32 GymRoom_GetBase(u32 room, s32 x, s32 y)
{
    const struct GymRoomInfo *info = GymRoom_GetInfo(room);
    if (!InRoomOf(info, x, y))
        return GYM_BASE_WALL_TOP;
    return info->base[y * info->width + x];
}

// The open doorway at (x, y) in a room, if there is one
static const struct GymRoomDoor *OpenDoorAt(u32 room, s32 x, s32 y)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sGymRoomDoors); i++)
    {
        const struct GymRoomDoor *d = &sGymRoomDoors[i];
        if (d->room == room && d->x == x && d->y == y && (d->flag == 0 || FlagGet(d->flag)))
            return d;
    }
    return NULL;
}

// The square in front of an open doorway (kept clear of decorations)
bool32 GymRoom_IsDoorFront(u32 room, s32 x, s32 y)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sGymRoomDoors); i++)
    {
        const struct GymRoomDoor *d = &sGymRoomDoors[i];
        if (d->room == room && d->frontX == x && d->frontY == y && (d->flag == 0 || FlagGet(d->flag)))
            return TRUE;
    }
    return FALSE;
}

// ---- The save: the floor of each square and the wall style, room by room (src/gym_decor.c) ----
static u32 FloorIn(u32 room, s32 x, s32 y)
{
    const struct GymRoomInfo *info = GymRoom_GetInfo(room);
    const u8 *floors = GymDecor_GetFloors(room);
    u32 i = y * info->width + x, f;
    if (!InRoomOf(info, x, y))
        return 0;
    f = (floors[i / 2] >> ((i % 2) * 4)) & 0xF;
    return f < GYM_FLOOR_COUNT ? f : 0;
}

u32 GymRoom_GetFloor(s32 x, s32 y)
{
    return FloorIn(Room(), x, y);
}

void GymRoom_SetFloor(s32 x, s32 y, u32 floor)
{
    u32 room = Room();
    const struct GymRoomInfo *info = GymRoom_GetInfo(room);
    u8 *floors = GymDecor_GetFloors(room);
    u32 i = y * info->width + x, shift = (i % 2) * 4;
    if (!InRoomOf(info, x, y))
        return;
    floors[i / 2] = (floors[i / 2] & ~(0xF << shift)) | ((floor & 0xF) << shift);
}

// Squares with a floor: not the walls or the way out, but under the statues, the PC and the
// doormat too (their floor shows round them)
static bool32 BaseIsPaintable(const struct GymRoomInfo *info, s32 x, s32 y)
{
    u16 base;
    if (!InRoomOf(info, x, y))
        return FALSE;
    base = info->base[y * info->width + x];
    if (base == GYM_BASE_FLOOR || base == GYM_BASE_FLOOR_SHADOW)
        return TRUE;
    return base < GYM_LOOK_COUNT && (sGymLooks[base].flags & GYM_LOOK_FLOOR_UNDER) && y < info->height - 1;
}

bool32 GymRoom_IsPaintable(s32 x, s32 y)
{
    return BaseIsPaintable(GymRoom_CurrentInfo(), x, y);
}

u32 GymRoom_GetWallStyle(void)
{
    u32 s = *GymDecor_GetWallStyle(Room());
    return s < GYM_WALL_STYLE_COUNT ? s : 0;
}

void GymRoom_SetWallStyle(u32 style)
{
    *GymDecor_GetWallStyle(Room()) = style;
}

// Fills the floor of the squares joined to (x, y) that have its floor; returns how many changed
u32 GymRoom_FillFloor(s32 x, s32 y, u32 style, bool32 (*allowed)(s32 x, s32 y))
{
    u16 *queue;
    u8 *seen;
    s32 head = 0, tail = 0;
    u32 from, n = 0;
    const struct GymRoomInfo *info = GymRoom_CurrentInfo();
    static const s8 dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
    if (!GymRoom_IsPaintable(x, y) || (allowed != NULL && !allowed(x, y)))
        return 0;
    from = GymRoom_GetFloor(x, y);
    if (from == style)
        return 0;
    queue = Alloc(ROOM_CELLS * sizeof(u16));
    seen = AllocZeroed(ROOM_CELLS);
    queue[tail++] = y * GYM_ROOM_WIDTH + x;
    seen[y * GYM_ROOM_WIDTH + x] = TRUE;
    while (head < tail)
    {
        s32 c = queue[head++], cx = c % GYM_ROOM_WIDTH, cy = c / GYM_ROOM_WIDTH, i;
        GymRoom_SetFloor(cx, cy, style);
        n++;
        for (i = 0; i < 4; i++)
        {
            s32 nx = cx + dx[i], ny = cy + dy[i];
            if (nx < 0 || ny < 0 || nx >= info->width || ny >= info->height || seen[ny * GYM_ROOM_WIDTH + nx])
                continue;
            seen[ny * GYM_ROOM_WIDTH + nx] = TRUE;
            if (GymRoom_IsPaintable(nx, ny) && GymRoom_GetFloor(nx, ny) == from && (allowed == NULL || allowed(nx, ny)))
                queue[tail++] = ny * GYM_ROOM_WIDTH + nx;
        }
    }
    Free(queue);
    Free(seen);
    return n;
}

// ---- Video memory: the art in use ----
static void CopyArt(u32 art, u32 vram)
{
    CpuCopy32(&sGymArtTiles[art * 8], (void *)(BG_VRAM + vram * TILE_SIZE_4BPP), TILE_SIZE_4BPP);
}

// The video memory tile holding art `art` (copied there now if it isn't yet), or 0 if there's no room
static u32 ArtVram(u32 art)
{
    u32 h = (art * 2654435761u) >> 22, i;
    for (i = 0; i < ART_HASH_SIZE; i++, h = (h + 1) % ART_HASH_SIZE)
    {
        if (sArtHashKey[h] == art + 1)
            return sArtHashVram[h];
        if (sArtHashKey[h] == 0)
        {
            u32 vram;
            if (sVramUsed >= GYM_VRAM_POOL_COUNT)
            {
                sOverflow = TRUE;
                return 0;
            }
            vram = sGymVramPool[sVramUsed++];
            sArtHashKey[h] = art + 1;
            sArtHashVram[h] = vram;
            if (sLive)
                CopyArt(art, vram);
            else
                sNeedUpload = TRUE;
            return vram;
        }
    }
    sOverflow = TRUE;
    return 0;
}

// A tile reference, as a map tile entry (with extra flips)
static u16 RefEntry(u32 ref, u32 flips)
{
    u32 tile;
    if (ref == 0)
        return 0;
    flips ^= REF_FLIPS(ref);
    if (ref & REF_FIXED)
        tile = ref & 0x3FF;
    else
        tile = ArtVram(REF_ART(ref));
    return tile | (flips << 10) | (REF_PAL(ref) << 12);
}

// The block with these tiles and attributes (made if there isn't one yet), or the first one
static u32 Intern(const u16 *tiles, u32 attr)
{
    u32 h = attr * 31, i, k;
    for (k = 0; k < NUM_TILES_PER_METATILE; k++)
        h = h * 131 + tiles[k];
    h = (h ^ (h >> 13)) % SLOT_HASH_SIZE;
    for (i = 0; i < SLOT_HASH_SIZE; i++, h = (h + 1) % SLOT_HASH_SIZE)
    {
        u32 slot = sSlotHash[h];
        if (slot == 0)
        {
            if (sNumSlots >= NUM_SLOTS)
                break;
            slot = sNumSlots++;
            memcpy(&gGymRoomMetatiles[slot * NUM_TILES_PER_METATILE], tiles, NUM_TILES_PER_METATILE * sizeof(u16));
            gGymRoomMetatileAttrs[slot] = attr;
            sSlotHash[h] = slot + 1;
            return FIRST_SLOT + slot;
        }
        slot--;
        if (gGymRoomMetatileAttrs[slot] == attr
         && memcmp(&gGymRoomMetatiles[slot * NUM_TILES_PER_METATILE], tiles, NUM_TILES_PER_METATILE * sizeof(u16)) == 0)
            return FIRST_SLOT + slot;
    }
    sOverflow = TRUE;
    return FIRST_SLOT;
}

static void ResetPools(void)
{
    sNumSlots = 0;
    sVramUsed = 0;
    sOverflow = FALSE;
    memset(sSlotHash, 0, sizeof(sSlotHash));
    memset(sArtHashKey, 0, sizeof(sArtHashKey));
}

// ---- The room's squares ----
enum { CELL_NONE, CELL_PIECE, CELL_WALL, CELL_WATER, CELL_DOOR, CELL_LINTEL };

struct RoomMap
{
    const struct GymRoomInfo *info;
    u8 room;
    u8 kind[ROOM_CELLS];
    u16 look[ROOM_CELLS];
    u8 floor[ROOM_CELLS];
};

#define CELL(x, y) ((y) * GYM_ROOM_WIDTH + (x))

static bool32 InRoom(const struct RoomMap *map, s32 x, s32 y)
{
    return InRoomOf(map->info, x, y);
}

static u32 Base(const struct RoomMap *map, s32 x, s32 y)
{
    return map->info->base[y * map->info->width + x];
}

static bool32 Paintable(const struct RoomMap *map, s32 x, s32 y)
{
    return BaseIsPaintable(map->info, x, y);
}

static void MarkPiece(struct RoomMap *map, u32 id, u32 facing, s32 x0, s32 y0, bool32 editing)
{
    const struct GymDecor *d = &gGymDecor[id];
    const struct GymDecorFacing *f;
    s32 x, y, k = 0;
    if (d->kind == GYM_DECOR_KIND_WALL || d->kind == GYM_DECOR_KIND_WATER)
    {
        if (InRoom(map, x0, y0))
            map->kind[CELL(x0, y0)] = d->kind == GYM_DECOR_KIND_WALL ? CELL_WALL : CELL_WATER;
        return;
    }
    if (d->kind != GYM_DECOR_KIND_PIECE)
        return;
    if (facing >= d->numFacings)
        facing = 0;
    f = &d->facings[facing];
    for (y = 0; y < f->height; y++)
    {
        for (x = 0; x < f->width; x++, k++)
        {
            s32 cx = x0 + x, cy = y0 + y;
            if (!InRoom(map, cx, cy))
                continue;
            map->kind[CELL(cx, cy)] = CELL_PIECE;
            map->look[CELL(cx, cy)] = (editing && f->editLooks) ? f->editLooks[k] : f->looks[k];
        }
    }
}

static void FillMapFor(struct RoomMap *map, bool32 editing, bool32 preview)
{
    struct GymDecorSaveData *save = GymDecor_GetSaveData();
    s32 i, x, y;
    map->room = Room();
    map->info = GymRoom_GetInfo(map->room);
    memset(map->kind, CELL_NONE, sizeof(map->kind));
    for (y = 0; y < map->info->height; y++)
        for (x = 0; x < map->info->width; x++)
            map->floor[CELL(x, y)] = FloorIn(map->room, x, y);
    for (i = 0; i < save->numPlaced; i++)
    {
        const struct GymDecorPlacement *p = &save->placed[i];
        if (GYM_PLACEMENT_ROOM(p) == map->room && p->id < GYM_DECOR_COUNT)
            MarkPiece(map, p->id, GYM_PLACEMENT_FACING(p), p->x, p->y, editing);
    }
    // The open doorways (and the wall over one in the back wall, which the player walks under)
    for (i = 0; i < (s32)ARRAY_COUNT(sGymRoomDoors); i++)
    {
        const struct GymRoomDoor *d = &sGymRoomDoors[i];
        if (d->room != map->room || !OpenDoorAt(map->room, d->x, d->y))
            continue;
        map->kind[CELL(d->x, d->y)] = CELL_DOOR;
        map->floor[CELL(d->x, d->y)] = map->floor[CELL(d->frontX, d->frontY)];
        if (Base(map, d->x, d->y) == GYM_BASE_WALL_FACE && d->y > 0)
            map->kind[CELL(d->x, d->y - 1)] = CELL_LINTEL;
    }
    if (preview && editing && sPreviewKind == GYM_PREVIEW_PIECE)
        MarkPiece(map, sPreviewId, sPreviewFacing, sPreviewX, sPreviewY, TRUE);
    else if (preview && editing && sPreviewKind == GYM_PREVIEW_FLOOR && Paintable(map, sPreviewX, sPreviewY))
        map->floor[CELL(sPreviewX, sPreviewY)] = sPreviewId;
}

static void FillMap(struct RoomMap *map, bool32 editing)
{
    FillMapFor(map, editing, TRUE);
}

// A wall square: one placed, or the back wall's top row and the side walls (not an open doorway)
static bool32 IsWall(const struct RoomMap *map, s32 x, s32 y)
{
    if (y < 0 || x < 0 || x >= map->info->width)
        return TRUE;        // (beyond the room: the back wall goes on)
    if (y >= map->info->height)
        return FALSE;
    if (map->kind[CELL(x, y)] == CELL_DOOR)
        return FALSE;
    if (Base(map, x, y) == GYM_BASE_WALL_TOP)
        return TRUE;
    return map->kind[CELL(x, y)] == CELL_WALL;
}

// For a wall's top face: whether the square above counts as wall (the back wall's face does,
// so walls run up into it)
static bool32 JoinsAbove(const struct RoomMap *map, s32 x, s32 y)
{
    if (InRoom(map, x, y - 1) && Base(map, x, y - 1) == GYM_BASE_WALL_FACE && map->kind[CELL(x, y - 1)] != CELL_DOOR)
        return TRUE;
    return IsWall(map, x, y - 1);
}

// A wall's front face: on a floor square below a wall's bottom edge (a piece can stand in front
// of it: the face shows behind the piece)
static bool32 IsFace(const struct RoomMap *map, s32 x, s32 y)
{
    u32 kind;
    if (!InRoom(map, x, y))
        return x < 0 || x >= map->info->width;       // (the back wall's face runs on beyond the room)
    kind = map->kind[CELL(x, y)];
    if (kind == CELL_DOOR)
        return FALSE;
    if (Base(map, x, y) == GYM_BASE_WALL_FACE)
        return TRUE;
    if (!Paintable(map, x, y) || IsWall(map, x, y) || (kind != CELL_NONE && kind != CELL_PIECE))
        return FALSE;
    return y > 0 && IsWall(map, x, y - 1) && Base(map, x, y - 1) != GYM_BASE_WALL_TOP;
}

static bool32 IsWater(const struct RoomMap *map, s32 x, s32 y)
{
    return InRoom(map, x, y) && map->kind[CELL(x, y)] == CELL_WATER;
}

// The quarter case (tools/gym_hack/gym_room_art.py QUARTER_CASES)
enum { Q_FULL, Q_INNER, Q_VERT, Q_HORZ, Q_OUTER };

static u32 QuarterCase(bool32 u, bool32 l, bool32 diag)
{
    if (u && l)
        return diag ? Q_FULL : Q_INNER;
    if (u)
        return Q_VERT;
    if (l)
        return Q_HORZ;
    return Q_OUTER;
}

// The floor's bottom layer for square (x, y)
static void FloorTiles(const struct RoomMap *map, s32 x, s32 y, u16 *tiles)
{
    u32 style = InRoom(map, x, y) ? map->floor[CELL(x, y)] : 0, k;
    const struct GymLook *look = &sGymLooks[sGymFloorLooks[style][(y & 1) * 2 + (x & 1)]];
    for (k = 0; k < 4; k++)
        tiles[k] = RefEntry(look->tiles[k], 0);
}

static void LookTiles(const struct RoomMap *map, s32 x, s32 y, u32 lookId, u16 *tiles)
{
    const struct GymLook *look = &sGymLooks[lookId];
    u32 k;
    if (look->flags & GYM_LOOK_FLOOR_UNDER)
        FloorTiles(map, x, y, tiles);
    else
        for (k = 0; k < 4; k++)
            tiles[k] = RefEntry(look->tiles[k], 0);
    for (k = 4; k < 8; k++)
        tiles[k] = RefEntry(look->tiles[k], 0);
}

// A placed wall's front face (4 tiles), its ends where the face stops
static void FaceTiles(const struct RoomMap *map, s32 x, s32 y, const struct GymWallStyle *ws, u16 *out)
{
    u32 variant = (IsFace(map, x - 1, y) ? 0 : 1) | (IsFace(map, x + 1, y) ? 0 : 2), k;
    for (k = 0; k < 4; k++)
        out[k] = RefEntry(ws->face[variant][k], 0);
}

// Whether a piece's look can stand in front of a wall's face (the face goes in its bottom layer:
// not a rug or the like, flat on the floor)
static bool32 LookFitsFace(u32 lookId)
{
    const struct GymLook *look = &sGymLooks[lookId];
    if (!(look->flags & GYM_LOOK_FLOOR_UNDER))
        return FALSE;
    return (look->flags & GYM_LOOK_SOLID) || ((look->attr >> 29) & 3) != METATILE_LAYER_TYPE_COVERED;
}

#define ATTR_LAYER(layer) ((u32)(layer) << 29)

// One square, put together: its block and its collision and elevation
static void ComposeCell(const struct RoomMap *map, s32 x, s32 y, u32 *metatile, u32 *collision, u32 *elevation)
{
    u16 tiles[NUM_TILES_PER_METATILE];
    u32 attr, i = CELL(x, y), wallStyle = GymRoom_GetWallStyle(), base = Base(map, x, y), k;
    const struct GymWallStyle *ws = &sGymWallStyles[wallStyle];

    *collision = 0;
    *elevation = 3;
    if (map->kind[i] == CELL_DOOR)
    {
        // An open doorway: through the back wall's face, or a gap in a side wall
        const struct GymRoomDoor *door = OpenDoorAt(map->room, x, y);
        if (base == GYM_BASE_WALL_FACE)
        {
            for (k = 0; k < 4; k++)
                tiles[k] = RefEntry(ws->door[k], 0);
        }
        else
        {
            FloorTiles(map, x, y, tiles);
        }
        tiles[4] = tiles[5] = tiles[6] = tiles[7] = 0;
        attr = (door != NULL ? door->behavior : 0) | ATTR_LAYER(METATILE_LAYER_TYPE_COVERED);
    }
    else if (base == GYM_BASE_WALL_TOP || (map->kind[i] == CELL_WALL && Paintable(map, x, y)))
    {
        // A wall's top face, joined to the walls around it
        if (base == GYM_BASE_WALL_TOP && wallStyle == 0 && y == 0)
        {
            u32 look = sGymOriginalWallTop[map->room == GYM_ROOM_MAIN && map->kind[i] != CELL_LINTEL ? x : 0];
            LookTiles(map, x, y, look, tiles);
            attr = sGymLooks[look].attr;
            if (map->kind[i] == CELL_LINTEL)
            {
                // (over a doorway: in front of the player, who walks under it)
                for (k = 0; k < 4; k++)
                    tiles[4 + k] = tiles[k];
                attr = ATTR_LAYER(METATILE_LAYER_TYPE_SPLIT);
            }
        }
        else
        {
            bool32 u = JoinsAbove(map, x, y), d = IsWall(map, x, y + 1);
            bool32 l = IsWall(map, x - 1, y), r = IsWall(map, x + 1, y);
            FloorTiles(map, x, y, tiles);
            tiles[4] = RefEntry(ws->top[QuarterCase(u, l, IsWall(map, x - 1, y - 1) || (u && l && base == GYM_BASE_WALL_TOP))], 0);
            tiles[5] = RefEntry(ws->top[QuarterCase(u, r, IsWall(map, x + 1, y - 1) || (u && r && base == GYM_BASE_WALL_TOP))], HFLIP);
            tiles[6] = RefEntry(ws->top[QuarterCase(d, l, IsWall(map, x - 1, y + 1))], VFLIP);
            tiles[7] = RefEntry(ws->top[QuarterCase(d, r, IsWall(map, x + 1, y + 1))], HFLIP | VFLIP);
            attr = ATTR_LAYER(map->kind[i] == CELL_LINTEL ? METATILE_LAYER_TYPE_SPLIT : METATILE_LAYER_TYPE_COVERED);
        }
        *collision = 1;
        *elevation = 0;
    }
    else if (IsFace(map, x, y))
    {
        if (map->kind[i] == CELL_PIECE)
        {
            // A piece against the wall: the face behind it, the piece in front
            const struct GymLook *look = &sGymLooks[map->look[i]];
            FaceTiles(map, x, y, ws, tiles);
            for (k = 4; k < 8; k++)
                tiles[k] = RefEntry(look->tiles[k], 0);
            attr = look->attr;
        }
        else if (base == GYM_BASE_WALL_FACE && wallStyle == 0)
        {
            LookTiles(map, x, y, GYM_LOOK_ORIGINAL_WALL_FACE, tiles);
            attr = sGymLooks[GYM_LOOK_ORIGINAL_WALL_FACE].attr;
        }
        else
        {
            FloorTiles(map, x, y, tiles);
            if (base == GYM_BASE_WALL_FACE)
                tiles[0] = tiles[1] = tiles[2] = tiles[3] = 0;
            FaceTiles(map, x, y, ws, &tiles[4]);
            attr = ATTR_LAYER(METATILE_LAYER_TYPE_COVERED);
        }
        *collision = 1;
        *elevation = 0;
    }
    else if (map->kind[i] == CELL_WATER)
    {
        bool32 u = IsWater(map, x, y - 1), d = IsWater(map, x, y + 1);
        bool32 l = IsWater(map, x - 1, y), r = IsWater(map, x + 1, y);
        FloorTiles(map, x, y, tiles);
        tiles[4] = RefEntry(sGymWaterQuarters[QuarterCase(u, l, IsWater(map, x - 1, y - 1))], 0);
        tiles[5] = RefEntry(sGymWaterQuarters[QuarterCase(u, r, IsWater(map, x + 1, y - 1))], HFLIP);
        tiles[6] = RefEntry(sGymWaterQuarters[QuarterCase(d, l, IsWater(map, x - 1, y + 1))], VFLIP);
        tiles[7] = RefEntry(sGymWaterQuarters[QuarterCase(d, r, IsWater(map, x + 1, y + 1))], HFLIP | VFLIP);
        attr = GYM_WATER_BEHAVIOR | ATTR_LAYER(METATILE_LAYER_TYPE_COVERED);
        *elevation = 1;
    }
    else if (map->kind[i] == CELL_PIECE)
    {
        const struct GymLook *look = &sGymLooks[map->look[i]];
        LookTiles(map, x, y, map->look[i], tiles);
        attr = look->attr;
        if (look->flags & GYM_LOOK_SOLID)
            *collision = 1;
        if (look->flags & GYM_LOOK_WATER)
            *elevation = 1;
    }
    else if (base == GYM_BASE_FLOOR || base == GYM_BASE_FLOOR_SHADOW)
    {
        if (base == GYM_BASE_FLOOR_SHADOW && wallStyle == 0 && map->floor[i] == 0)
        {
            LookTiles(map, x, y, GYM_LOOK_ORIGINAL_WALL_SHADOW, tiles);
        }
        else
        {
            const struct GymLook *shadow = &sGymLooks[GYM_LOOK_SHADOW];
            FloorTiles(map, x, y, tiles);
            for (k = 4; k < 8; k++)
                tiles[k] = base == GYM_BASE_FLOOR_SHADOW ? RefEntry(shadow->tiles[k], 0) : 0;
        }
        attr = ATTR_LAYER(METATILE_LAYER_TYPE_COVERED);
        if (sGymLooks[sGymFloorLooks[map->floor[i]][0]].flags & GYM_LOOK_SOLID)
            *collision = 1;     // (lava)
    }
    else
    {
        // The rest of the room itself: the entrance, the statues, the PC
        const struct GymLook *look = &sGymLooks[base];
        LookTiles(map, x, y, base, tiles);
        attr = look->attr;
        if (look->flags & GYM_LOOK_SOLID)
        {
            *collision = 1;
            *elevation = 0;
        }
    }
    *metatile = Intern(tiles, attr);
}

// Puts together the squares from (x0, y0) up to (x1, y1) (not included)
static void ComposeRect(const struct RoomMap *map, s32 x0, s32 y0, s32 x1, s32 y1)
{
    s32 x, y;
    for (y = y0; y < y1; y++)
    {
        for (x = x0; x < x1; x++)
        {
            u32 metatile, collision, elevation;
            ComposeCell(map, x, y, &metatile, &collision, &elevation);
            MapGridSetMetatileEntryAt(x + MAP_OFFSET, y + MAP_OFFSET,
                (metatile & MAPGRID_METATILE_ID_MASK) | (collision << MAPGRID_COLLISION_SHIFT) | (elevation << MAPGRID_ELEVATION_SHIFT));
        }
    }
}

static void DrawRect(s32 x0, s32 y0, s32 x1, s32 y1)
{
    s32 x, y;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++)
            CurrentMapDrawMetatileAt(x + MAP_OFFSET, y + MAP_OFFSET);
}

static bool32 PoolsLow(void)
{
    return sOverflow || sNumSlots > NUM_SLOTS - 8 || sVramUsed > GYM_VRAM_POOL_COUNT - 24;
}

// The whole room. Starts the blocks and video memory afresh if they're running out (blocks
// and art no longer in use are only let go of this way)
static bool32 BuildRoom(const struct RoomMap *map)
{
    ComposeRect(map, 0, 0, map->info->width, map->info->height);
    if (PoolsLow())
    {
        ResetPools();
        ComposeRect(map, 0, 0, map->info->width, map->info->height);
        if (!sLive)
            sNeedUpload = TRUE;
        return TRUE;
    }
    return FALSE;
}

// The gym's ON_LOAD: the room as the save has it (drawn on screen later, with the rest of the map)
void GymRoom_Load(void)
{
    struct RoomMap *map = Alloc(sizeof(struct RoomMap));
    sLive = FALSE;
    sPreviewKind = GYM_PREVIEW_NONE;
    ResetPools();
    FillMap(map, FALSE);
    BuildRoom(map);
    Free(map);
}

// The whole room put together again, and redrawn
void GymRoom_Build(bool32 editing)
{
    struct RoomMap *map = Alloc(sizeof(struct RoomMap));
    FillMap(map, editing);
    BuildRoom(map);
    DrawRect(0, 0, map->info->width, map->info->height);
    Free(map);
}

// After a change to the squares (x, y, w, h): those, and the ones around them (walls, water and
// faces join up with their neighbors), put together again and redrawn
void GymRoom_Refresh(s32 x, s32 y, s32 w, s32 h, bool32 editing)
{
    struct RoomMap *map = Alloc(sizeof(struct RoomMap));
    s32 x0, y0, x1, y1;
    FillMap(map, editing);
    x0 = max(x - 1, 0);
    y0 = max(y - 1, 0);
    x1 = min(x + w + 1, (s32)map->info->width);
    y1 = min(y + h + 1, (s32)map->info->height);
    if (x0 >= x1 || y0 >= y1)
    {
        Free(map);
        return;
    }
    if (PoolsLow())
    {
        BuildRoom(map);
        DrawRect(0, 0, map->info->width, map->info->height);
    }
    else
    {
        ComposeRect(map, x0, y0, x1, y1);
        if (PoolsLow() && BuildRoom(map))
            DrawRect(0, 0, map->info->width, map->info->height);
        else
            DrawRect(x0, y0, x1, y1);
    }
    Free(map);
}

void GymRoom_SetPreview(u32 kind, u32 id, u32 facing, s32 x, s32 y)
{
    sPreviewKind = kind;
    sPreviewId = id;
    sPreviewFacing = facing;
    sPreviewX = x;
    sPreviewY = y;
}

bool32 GymRoom_IsSolidFloor(u32 style)
{
    return style < GYM_FLOOR_COUNT && (sGymLooks[sGymFloorLooks[style][0]].flags & GYM_LOOK_SOLID);
}

bool32 GymRoom_Overflowed(void)
{
    return sOverflow;
}

// Whether decoration `id` (facing that way) at (x, y) would leave every wall face whole: a piece
// in front of a wall's face has to be one the face can show behind (not a rug), and no pool can
// go there. (Checked with the piece there, without the preview: walls make faces below them.)
bool32 GymRoom_PieceFitsWalls(u32 id, u32 facing, s32 x, s32 y)
{
    struct RoomMap *map = Alloc(sizeof(struct RoomMap));
    const struct GymDecor *d = &gGymDecor[id];
    bool32 ok = TRUE;
    s32 cx, cy, w = 1, h = 1;
    FillMapFor(map, FALSE, FALSE);
    MarkPiece(map, id, facing, x, y, FALSE);
    if (d->kind == GYM_DECOR_KIND_PIECE)
    {
        w = d->facings[facing < d->numFacings ? facing : 0].width;
        h = d->facings[facing < d->numFacings ? facing : 0].height;
    }
    for (cy = y; cy <= y + h && ok; cy++)
    {
        for (cx = x; cx < x + w && ok; cx++)
        {
            u32 kind;
            if (!InRoom(map, cx, cy) || Base(map, cx, cy) == GYM_BASE_WALL_FACE)
                continue;
            kind = map->kind[CELL(cx, cy)];
            if (kind == CELL_PIECE && IsFace(map, cx, cy) && !LookFitsFace(map->look[CELL(cx, cy)]))
                ok = FALSE;
            else if (kind == CELL_WATER && cy > 0 && IsWall(map, cx, cy - 1) && Base(map, cx, cy - 1) != GYM_BASE_WALL_TOP)
                ok = FALSE;
        }
    }
    Free(map);
    return ok;
}

// (src/fieldmap.c, after the gym's tileset is copied to video memory: that copy is queued, so the
// room's art goes in once it's done: see TilesetAnim_PalletGym)
void GymRoom_OnTilesetLoaded(void)
{
    sLive = FALSE;
    sNeedUpload = TRUE;
}

static void UploadAll(void)
{
    u32 h;
    for (h = 0; h < ART_HASH_SIZE; h++)
        if (sArtHashKey[h])
            CopyArt(sArtHashKey[h] - 1, sArtHashVram[h]);
}

// Every frame in the gym (its tileset's animation callback: src/tileset_anims.c)
void GymRoom_TilesetAnim(u16 timer)
{
    if (sNeedUpload && !IsDma3ManagerBusyWithBgCopy())
    {
        UploadAll();
        sNeedUpload = FALSE;
        sLive = TRUE;
    }
}
