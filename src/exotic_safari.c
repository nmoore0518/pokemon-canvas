// Pokémon Canvas: the Exotic Pokémon Safari (Fuchsia City; built by
// tools/gym_hack/build_exotic_safari.py): the hall's doors, the roaming legendaries, the kiosk.
//
// THE HALL
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
#include "overworld.h"
#include "random.h"
#include "shop.h"
#include "sound.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/maps.h"
#include "constants/songs.h"

#include "data/exotic_safari_doors.h"
#include "data/exotic_safari_areas.h"

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

// ---------------------------------------------------------------------------------------------
// THE ROAMERS: legendaries that roam the zone's areas. Each time the player goes into an area
// (through a door, a ladder, off an edge), each roamer not caught yet is there with a chance of one
// in the number of areas, somewhere in its grass (or sand, snow, cave floor: a spot from the
// area's list, data/exotic_safari_areas.h, away from the player). Each area map has an object per
// roamer (local ids 1-3, hidden by a temp flag unless it's there). Talking to it: a battle; if it
// isn't caught, it runs off (the object goes; it's somewhere else next time).
// ---------------------------------------------------------------------------------------------
static const struct ExoticSafariArea *GetArea(void)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sExoticSafariAreas); i++)
    {
        if (sExoticSafariAreas[i].mapGroup == gSaveBlock1Ptr->location.mapGroup
         && sExoticSafariAreas[i].mapNum == gSaveBlock1Ptr->location.mapNum)
            return &sExoticSafariAreas[i];
    }
    return NULL;
}

static bool32 PlaceRoamer(const struct ExoticSafariArea *area, u32 roamer)
{
    u32 tries, k, i;
    s32 px = gSaveBlock1Ptr->pos.x, py = gSaveBlock1Ptr->pos.y;
    for (tries = 0; tries < 16; tries++)
    {
        const u8 *spot = area->spots[Random() % area->spotCount];
        bool32 taken = FALSE;
        if (abs(spot[0] - px) + abs(spot[1] - py) < 6)
            continue;
        for (k = 0; k < gMapHeader.events->objectEventCount && k < OBJECT_EVENT_TEMPLATES_COUNT; k++)
        {
            const struct ObjectEventTemplate *t = &gSaveBlock1Ptr->objectEventTemplates[k];
            if (t->x == spot[0] && t->y == spot[1])
                taken = TRUE;
        }
        if (taken)
            continue;
        for (i = 0; i < OBJECT_EVENT_TEMPLATES_COUNT; i++)
        {
            struct ObjectEventTemplate *t = &gSaveBlock1Ptr->objectEventTemplates[i];
            if (t->localId == EXOTIC_SAFARI_ROAMER_LOCALID(roamer))
            {
                t->x = spot[0];
                t->y = spot[1];
                t->elevation = spot[2];
                return TRUE;
            }
        }
        return FALSE;
    }
    return FALSE;
}

// Special (each area's ON_TRANSITION)
void ExoticSafari_AreaOnTransition(void)
{
    const struct ExoticSafariArea *area = GetArea();
    u32 i;
    for (i = 0; i < EXOTIC_SAFARI_ROAMER_COUNT; i++)
    {
        FlagSet(sExoticSafariRoamers[i].hideFlag);
        if (area == NULL || area->spotCount == 0 || FlagGet(sExoticSafariRoamers[i].caughtFlag))
            continue;
        if (Random() % ARRAY_COUNT(sExoticSafariAreas) != 0)
            continue;
        if (PlaceRoamer(area, i))
            FlagClear(sExoticSafariRoamers[i].hideFlag);
    }
}

// ---------------------------------------------------------------------------------------------
// THE KIOSK: supplies, and the evolution items the player has found in the zones
// ---------------------------------------------------------------------------------------------
static EWRAM_DATA u16 sKioskItems[ARRAY_COUNT(sKioskSupplies) + ARRAY_COUNT(sKioskFinds) + 1] = {0};

void ExoticSafari_Kiosk(void)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sKioskSupplies); i++)
        sKioskItems[n++] = sKioskSupplies[i];
    for (i = 0; i < ARRAY_COUNT(sKioskFinds); i++)
    {
        if (FlagGet(sKioskFinds[i].flag))
            sKioskItems[n++] = sKioskFinds[i].item;
    }
    sKioskItems[n] = ITEM_NONE;
    CreatePokemartMenu(sKioskItems);
}
