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
#include "gym_switch.h"
#include "string_util.h"
#include "item.h"
#include "overworld.h"
#include "random.h"
#include "shop.h"
#include "sound.h"
#include "constants/exotic_safari.h"
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

static u32 CountRegionAreas(u32 region)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sExoticSafariAreas); i++)
        n += (sExoticSafariAreas[i].region == region);
    return n ? n : 1;
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
            if (t->localId == EXOTIC_SAFARI_ROAMER_LOCALID(sExoticSafariRoamers[roamer].hideFlag))
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

// ---------------------------------------------------------------------------------------------
// THE STATIC LEGENDARIES: each waits for something (its seal): the Treasures of Ruin for their 4
// stakes to be pulled, Ogerpon for its 3 masks to be found, Pecharunt for the Loyal Three (the
// roamers) to be caught, Terapagos for every gym type to be mastered. Hidden (a temp flag) until
// then, once caught, and for the rest of a visit after a battle that didn't catch it.
// ---------------------------------------------------------------------------------------------
static u32 CountFlagsLeft(const u16 *flags, u32 count)
{
    u32 i, left = 0;
    for (i = 0; i < count; i++)
        left += !FlagGet(flags[i]);
    return left;
}

static u32 StakesLeft(u32 color)
{
    u32 k, left = 0;
    for (k = 0; k < EXOTIC_SAFARI_STAKES_PER_COLOR; k++)
        left += !FlagGet(EXOTIC_SAFARI_STAKE_FLAGS + color * EXOTIC_SAFARI_STAKES_PER_COLOR + k);
    return left;
}

// How much of a static legendary's seal is left (0: it's free to appear)
static u32 SealLeft(const struct ExoticSafariStatic *s)
{
    u32 i, left = 0;
    switch (s->seal)
    {
    case SEAL_STAKES:
        return StakesLeft(s->sealArg);
    case SEAL_MASKS:
        return CountFlagsLeft(sMaskFlags, ARRAY_COUNT(sMaskFlags));
    case SEAL_ROAMERS:
        for (i = 0; i < EXOTIC_SAFARI_ROAMER_COUNT; i++)
            left += !FlagGet(sExoticSafariRoamers[i].caughtFlag);
        return left;
    case SEAL_MASTERY:
        return !GymSwitch_AllTypesMastered();
    }
    return 0;
}

// Special (the spot where a static legendary appears, while it isn't there): VAR_0x8004 = which
// (sExoticSafariStatics). VAR_RESULT = its seal (SEAL_*), or 0xFF if it's caught, 0xFE if the seal
// is broken (it's away for this visit). STR_VAR_1 = the stakes' color, STR_VAR_2 = how many left.
void ExoticSafari_BufferSeal(void)
{
    const struct ExoticSafariStatic *s = &sExoticSafariStatics[gSpecialVar_0x8004 % ARRAY_COUNT(sExoticSafariStatics)];
    u32 left = SealLeft(s);
    if (FlagGet(s->caughtFlag))
        gSpecialVar_Result = 0xFF;
    else if (left == 0)
        gSpecialVar_Result = 0xFE;
    else
        gSpecialVar_Result = s->seal;
    if (s->seal == SEAL_STAKES)
        StringCopy(gStringVar1, sStakeColorNames[s->sealArg]);
    ConvertIntToDecimalStringN(gStringVar2, left, STR_CONV_MODE_LEFT_ALIGN, 2);
}

// Special (a stake, just pulled): VAR_0x8004 = its color. VAR_RESULT = how many of that color are
// left; STR_VAR_1 = the color, STR_VAR_2 = the count.
void ExoticSafari_CountStakes(void)
{
    u32 color = gSpecialVar_0x8004 % EXOTIC_SAFARI_STAKE_COLORS;
    gSpecialVar_Result = StakesLeft(color);
    StringCopy(gStringVar1, sStakeColorNames[color]);
    ConvertIntToDecimalStringN(gStringVar2, gSpecialVar_Result, STR_CONV_MODE_LEFT_ALIGN, 2);
}

// Special (each area's ON_TRANSITION): its static legendaries shown or not, the roamers placed
void ExoticSafari_AreaOnTransition(void)
{
    const struct ExoticSafariArea *area = GetArea();
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sExoticSafariStatics); i++)
    {
        const struct ExoticSafariStatic *s = &sExoticSafariStatics[i];
        if (s->mapGroup != gSaveBlock1Ptr->location.mapGroup || s->mapNum != gSaveBlock1Ptr->location.mapNum)
            continue;
        if (FlagGet(s->caughtFlag) || SealLeft(s) != 0)
            FlagSet(s->hideFlag);
    }
    if (area == NULL)
        return;
    for (i = 0; i < EXOTIC_SAFARI_ROAMER_COUNT; i++)
    {
        // (only the area's region's roamers have objects here: one in that many areas)
        if (sExoticSafariRoamers[i].region != area->region)
            continue;
        FlagSet(sExoticSafariRoamers[i].hideFlag);
        if (area->spotCount == 0 || FlagGet(sExoticSafariRoamers[i].caughtFlag))
            continue;
        if (Random() % CountRegionAreas(area->region) != 0)
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
