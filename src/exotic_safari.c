// Pokémon Canvas: the Exotic Pokémon Safari's hall (Fuchsia City; built by
// tools/gym_hack/build_exotic_safari.py).
//
// One door per region along the back wall, each in its games' two colors. The doors are closed;
// an open zone's door slides open when a player with the Exotic Pass stands in front of it (or
// comes out of it), and closes again when they walk away. Checked after every step in the hall
// (src/field_control_avatar.c) and when the hall loads.
#include "global.h"
#include "exotic_safari.h"
#include "event_data.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "field_camera.h"
#include "item.h"
#include "sound.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/songs.h"

#include "data/exotic_safari_doors.h"

static EWRAM_DATA u8 sDoorsDrawnOpen;     // bit per door: drawn open

static bool32 InHall(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_EXOTIC_SAFARI_HALL)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_EXOTIC_SAFARI_HALL);
}

static bool32 CanEnter(void)
{
    return FlagGet(FLAG_PALLET_GYM_AWARDED) && CheckBagHasItem(ITEM_EXOTIC_PASS, 1);
}

static void SetDoor(u32 door, bool32 open, bool32 draw)
{
    u32 k;
    for (k = 0; k < 6; k++)
    {
        s32 x = sDoorX[door] - 1 + k % 3 + MAP_OFFSET;
        s32 y = EXOTIC_SAFARI_DOOR_Y - 1 + k / 3 + MAP_OFFSET;
        u16 id = open ? sDoorOpen[k] : sDoorClosed[door][k];
        // Only the doorway itself (bottom middle) can be walked into, and only when open
        if (!(open && k == 4))
            id |= MAPGRID_COLLISION_MASK;
        MapGridSetMetatileIdAt(x, y, id);
        if (draw)
            CurrentMapDrawMetatileAt(x, y);
    }
    if (open)
        sDoorsDrawnOpen |= 1 << door;
    else
        sDoorsDrawnOpen &= ~(1 << door);
}

static void UpdateDoors(bool32 draw, s32 px, s32 py)
{
    u32 door;
    bool32 enter = CanEnter();
    for (door = 0; door < EXOTIC_SAFARI_DOOR_COUNT; door++)
    {
        bool32 open = enter && sDoorHasZone[door] && px == sDoorX[door]
                   && (py == EXOTIC_SAFARI_DOOR_Y || py == EXOTIC_SAFARI_DOOR_Y + 1);
        bool32 isOpen = (sDoorsDrawnOpen >> door) & 1;
        if (!draw || open != isOpen)
        {
            SetDoor(door, open, draw);
            if (draw)
                PlaySE(SE_DOOR);
        }
    }
}

// Special (the hall's ON_LOAD): the doors as they should be (the player may come out of one)
void ExoticSafari_OnLoad(void)
{
    sDoorsDrawnOpen = 0;
    // (the player's object isn't there yet: where the warp puts them)
    UpdateDoors(FALSE, gSaveBlock1Ptr->pos.x, gSaveBlock1Ptr->pos.y);
}

// After each step
void ExoticSafari_OnStep(void)
{
    s16 px, py;
    if (!InHall())
        return;
    PlayerGetDestCoords(&px, &py);
    UpdateDoors(TRUE, px - MAP_OFFSET, py - MAP_OFFSET);
}
