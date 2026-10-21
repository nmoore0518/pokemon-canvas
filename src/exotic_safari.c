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
#include "gym_convention.h"
#include "pokemon.h"
#include "pokedex.h"
#include "field_weather.h"
#include "constants/weather.h"
#include "script.h"
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
#include "constants/vars.h"

#include "data/exotic_safari_doors.h"
#include "data/exotic_safari_areas.h"

#define VAR_STORM VAR_TEMP_7      // (Seafolk Village's thunderstorm, this visit)

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
enum { LEAD_LOVING, LEAD_DRAGON, LEAD_STEEL, LEAD_SHADOW };

// The lead POKéMON (the party's first): Enamorus wants one that loves its trainer as much as any
// can (the most friendship), Regidrago a Dragon type
static bool32 LeadIs(u32 what)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    u32 species;
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return FALSE;
    if (what == LEAD_LOVING)
        return GetMonData(mon, MON_DATA_FRIENDSHIP) >= MAX_FRIENDSHIP;
    species = GetMonData(mon, MON_DATA_SPECIES);
    if (what == LEAD_SHADOW)        // (Marshadow: a Fighting or Ghost type)
        return GetSpeciesType(species, 0) == TYPE_FIGHTING || GetSpeciesType(species, 1) == TYPE_FIGHTING
            || GetSpeciesType(species, 0) == TYPE_GHOST || GetSpeciesType(species, 1) == TYPE_GHOST;
    if (what == LEAD_STEEL)         // (Meltan)
        return GetSpeciesType(species, 0) == TYPE_STEEL || GetSpeciesType(species, 1) == TYPE_STEEL;
    return GetSpeciesType(species, 0) == TYPE_DRAGON || GetSpeciesType(species, 1) == TYPE_DRAGON;
}

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
    case SEAL_ROAMERS:      // (its own region's: sealArg)
    case SEAL_GUARDIANS:
        for (i = 0; i < EXOTIC_SAFARI_ROAMER_COUNT; i++)
            left += sExoticSafariRoamers[i].region == s->sealArg && !FlagGet(sExoticSafariRoamers[i].caughtFlag);
        return left;
    case SEAL_MASTERY:
        return !GymSwitch_AllTypesMastered();
    case SEAL_ITEM:
        return !FlagGet(sSealItems[s->sealArg][1]);
    case SEAL_LAMPS:
        for (i = 0; i < ARRAY_COUNT(sLamps); i++)
            left += !FlagGet(FLAG_EXOTIC_LAMP_FIRST + i);
        return left;
    case SEAL_FRIENDSHIP:
        return !LeadIs(LEAD_LOVING);
    case SEAL_DOJO:
        return !FlagGet(FLAG_EXOTIC_DOJO_WON);
    case SEAL_STONES:
        return !FlagGet(FLAG_EXOTIC_STONES_LIT);
    case SEAL_DRAGON:
        return !FlagGet(sSealItems[s->sealArg][1]) + !LeadIs(LEAD_DRAGON);
    case SEAL_FLUTE:        // (Solgaleo, Lunala: their flute found - a relic)
        return !FlagGet(sRelics[s->sealArg].flag);
    case SEAL_LIGHT:        // (Necrozma: Solgaleo and Lunala caught)
        return !FlagGet(FLAG_EXOTIC_LEGEND_SOLGALEO) + !FlagGet(FLAG_EXOTIC_LEGEND_LUNALA);
    case SEAL_PARTS:        // (Magearna: its machine parts found - relics from sealArg on)
        for (i = s->sealArg; i < ARRAY_COUNT(sRelics); i++)
            left += !FlagGet(sRelics[i].flag);
        return left;
    case SEAL_STEEL:
        return !LeadIs(LEAD_STEEL);
    case SEAL_SHADOW:
        return !LeadIs(LEAD_SHADOW);
    case SEAL_STORM:        // (Zeraora: a thunderstorm over the village, this visit)
        return VarGet(VAR_STORM) == 0;
    }
    return 0;
}

// The seal as the hint should tell it (Regidrago's: the fang first, then the lead)
static u32 SealToTell(const struct ExoticSafariStatic *s)
{
    if (s->seal == SEAL_DRAGON && FlagGet(sSealItems[s->sealArg][1]))
        return SEAL_DRAGON_LEAD;
    return s->seal;
}

// Special (the spot where a static legendary appears, while it isn't there): VAR_0x8004 = which
// (sExoticSafariStatics). VAR_RESULT = its seal (SEAL_*), or SEAL_RESULT_CAUGHT, or
// SEAL_RESULT_AWAY if the seal is broken (ExoticSafari_TryAppear: it may come now). STR_VAR_1 = the
// stakes' color or the item it waits for, STR_VAR_2 = how many left.
void ExoticSafari_BufferSeal(void)
{
    const struct ExoticSafariStatic *s = &sExoticSafariStatics[gSpecialVar_0x8004 % ARRAY_COUNT(sExoticSafariStatics)];
    u32 left = SealLeft(s);
    if (FlagGet(s->caughtFlag))
        gSpecialVar_Result = SEAL_RESULT_CAUGHT;
    else if (left == 0)
        gSpecialVar_Result = SEAL_RESULT_AWAY;
    else
        gSpecialVar_Result = SealToTell(s);
    if (s->seal == SEAL_STAKES)
        StringCopy(gStringVar1, sStakeColorNames[s->sealArg]);
    else if (s->seal == SEAL_ITEM || s->seal == SEAL_DRAGON)
        CopyItemName(sSealItems[s->sealArg][0], gStringVar1);
    else if (s->seal == SEAL_FLUTE)
        StringCopy(gStringVar1, sRelics[s->sealArg].name);
    ConvertIntToDecimalStringN(gStringVar2, left, STR_CONV_MODE_LEFT_ALIGN, 2);
}

// The statics that ran from a battle this visit (bit per local id: they stay away until the
// player leaves the area; a temp var, cleared on the way in)
#define VAR_LEGENDS_GONE VAR_TEMP_8

// Special (a static legendary that wasn't caught): VAR_LAST_TALKED is gone for the visit
void ExoticSafari_MarkGone(void)
{
    VarSet(VAR_LEGENDS_GONE, VarGet(VAR_LEGENDS_GONE) | (1 << (gSpecialVar_LastTalked % 16)));
}

// Special: VAR_0x8004 = a static legendary (sExoticSafariStatics). If it's in this area, its seal is
// broken, it isn't caught or away for the visit, and isn't there yet: it comes (its temp flag
// cleared; the script adds its object, local id VAR_0x8005). VAR_RESULT: TRUE if it came.
void ExoticSafari_TryAppear(void)
{
    const struct ExoticSafariStatic *s = &sExoticSafariStatics[gSpecialVar_0x8004 % ARRAY_COUNT(sExoticSafariStatics)];
    gSpecialVar_Result = FALSE;
    if (s->mapGroup != gSaveBlock1Ptr->location.mapGroup || s->mapNum != gSaveBlock1Ptr->location.mapNum)
        return;
    if (FlagGet(s->caughtFlag) || SealLeft(s) != 0 || !FlagGet(s->hideFlag))
        return;
    if ((VarGet(VAR_LEGENDS_GONE) >> (s->localId % 16)) & 1)
        return;
    FlagClear(s->hideFlag);
    gSpecialVar_0x8005 = s->localId;
    gSpecialVar_0x8006 = s->species;
    gSpecialVar_Result = TRUE;
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
    // Seafolk Village: a thunderstorm on one visit in four (Zeraora comes with it)
    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(EXOTIC_SAFARI_STORM_MAP)
     && gSaveBlock1Ptr->location.mapNum == MAP_NUM(EXOTIC_SAFARI_STORM_MAP))
    {
        bool32 storm = (Random() % 4) == 0;
        VarSet(VAR_STORM, storm);
        SetSavedWeather(storm ? WEATHER_RAIN_THUNDERSTORM : WEATHER_SUNNY);
    }
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
        // (Calyrex: only once its steed, Spectrier, is caught)
        if (sExoticSafariRoamers[i].needFlag != 0 && !FlagGet(sExoticSafariRoamers[i].needFlag))
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

// ---------------------------------------------------------------------------------------------
// THE GALAR LEGENDARIES' PUZZLES (tools/gym_hack/galar.py)
// Zarude's lamps: Glimwood's big mushrooms are dark until the player touches each (a flag apiece);
// once all are lit, Zarude comes. Regieleki's stepping stones: a lightning bolt of stones in the
// meadow, lit one by one as they're stepped on from the top of the bolt down (a stone out of order
// puts them all out); all lit, Regieleki comes, and they stay lit.
// ---------------------------------------------------------------------------------------------
static bool32 InMap(u32 mapGroup, u32 mapNum)
{
    return gSaveBlock1Ptr->location.mapGroup == mapGroup && gSaveBlock1Ptr->location.mapNum == mapNum;
}

static void SetLamp(u32 lamp, bool32 lit, bool32 draw)
{
    u32 k, wide = sLamps[lamp][2];
    u32 first = wide == 3 ? (lit ? EXOTIC_SAFARI_LAMP_GIANT_LIT : EXOTIC_SAFARI_LAMP_GIANT_DARK)
                          : (lit ? EXOTIC_SAFARI_LAMP_BIG_LIT : EXOTIC_SAFARI_LAMP_BIG_DARK);
    for (k = 0; k < wide * 2; k++)
    {
        s32 x = sLamps[lamp][0] + k % wide + MAP_OFFSET, y = sLamps[lamp][1] + k / wide + MAP_OFFSET;
        MapGridSetMetatileIdAt(x, y, (first + k) | MAPGRID_COLLISION_MASK);
        if (draw)
            CurrentMapDrawMetatileAt(x, y);
    }
}

#define VAR_STONES_LIT VAR_TEMP_9      // (how many stones are lit, in order, this visit)

static void SetStone(u32 stone, bool32 lit, bool32 draw)
{
    s32 x = sStones[stone][0] + MAP_OFFSET, y = sStones[stone][1] + MAP_OFFSET;
    MapGridSetMetatileIdAt(x, y, (lit ? EXOTIC_SAFARI_STONE_LIT : EXOTIC_SAFARI_STONE_DARK) + sStones[stone][0] % 2);
    if (draw)
        CurrentMapDrawMetatileAt(x, y);
}

// Special (each area's ON_LOAD): the lamps and stones already lit
void ExoticSafari_AreaOnLoad(void)
{
    u32 i;
    if (InMap(MAP_GROUP(EXOTIC_SAFARI_LAMP_MAP), MAP_NUM(EXOTIC_SAFARI_LAMP_MAP)))
    {
        for (i = 0; i < ARRAY_COUNT(sLamps); i++)
        {
            if (FlagGet(FLAG_EXOTIC_LAMP_FIRST + i))
                SetLamp(i, TRUE, FALSE);
        }
    }
    if (InMap(MAP_GROUP(EXOTIC_SAFARI_STONE_MAP), MAP_NUM(EXOTIC_SAFARI_STONE_MAP)) && FlagGet(FLAG_EXOTIC_STONES_LIT))
    {
        for (i = 0; i < ARRAY_COUNT(sStones); i++)
            SetStone(i, TRUE, FALSE);
    }
}

// Special (a big mushroom touched): VAR_0x8004 = which. VAR_RESULT: 0 already lit, 1 lit now
// (STR_VAR_2: how many still dark), 2 lit now, the last one
void ExoticSafari_Lamp(void)
{
    u32 lamp = gSpecialVar_0x8004 % ARRAY_COUNT(sLamps), i, left = 0;
    gSpecialVar_Result = 0;
    if (FlagGet(FLAG_EXOTIC_LAMP_FIRST + lamp))
        return;
    FlagSet(FLAG_EXOTIC_LAMP_FIRST + lamp);
    SetLamp(lamp, TRUE, TRUE);
    for (i = 0; i < ARRAY_COUNT(sLamps); i++)
        left += !FlagGet(FLAG_EXOTIC_LAMP_FIRST + i);
    ConvertIntToDecimalStringN(gStringVar2, left, STR_CONV_MODE_LEFT_ALIGN, 2);
    gSpecialVar_Result = left ? 1 : 2;
}

// Special (a stepping stone stepped on): VAR_0x8004 = which. VAR_RESULT: 0 nothing (already lit, or
// all done), 1 lit, 2 lit and the last, 3 out of order (all put out)
void ExoticSafari_Stone(void)
{
    u32 stone = gSpecialVar_0x8004, done = VarGet(VAR_STONES_LIT), i;
    gSpecialVar_Result = 0;
    if (FlagGet(FLAG_EXOTIC_STONES_LIT) || stone >= ARRAY_COUNT(sStones) || stone < done)
        return;
    if (stone == done)
    {
        SetStone(stone, TRUE, TRUE);
        VarSet(VAR_STONES_LIT, ++done);
        gSpecialVar_Result = 1;
        if (done == ARRAY_COUNT(sStones))
        {
            FlagSet(FLAG_EXOTIC_STONES_LIT);
            gSpecialVar_Result = 2;
        }
        return;
    }
    for (i = 0; i < done; i++)
        SetStone(i, FALSE, TRUE);
    VarSet(VAR_STONES_LIT, 0);
    gSpecialVar_Result = 3;
}

// Special (Kubfu's dojo master, in the stadium's lobby): her battle, with her full team from the
// Gym Leader Convention. VAR_RESULT: FALSE if she couldn't be set up.
void ExoticSafari_SetupDojo(void)
{
    gSpecialVar_Result = Convention_SetupByName(COMPOUND_STRING("BEA")) >= 0;
}

// ---------------------------------------------------------------------------------------------
// THE ALOLA LEGENDARIES' PUZZLES (tools/gym_hack/alola.py): relics found about the zone (the Sun
// and Moon Flutes, Magearna's machine parts), and Type: Null, handed over by the resort's
// caretaker once the player has caught Alola's three first partners
// ---------------------------------------------------------------------------------------------
// Special (a relic picked up): VAR_0x8004 = which. STR_VAR_1 = its name; VAR_RESULT = how many of
// Magearna's parts are still missing (if it's one of them), else 0xFF
void ExoticSafari_TakeRelic(void)
{
    u32 relic = gSpecialVar_0x8004 % ARRAY_COUNT(sRelics), i, left = 0;
    FlagSet(sRelics[relic].flag);
    StringCopy(gStringVar1, sRelics[relic].name);
    gSpecialVar_Result = 0xFF;
    if (relic >= EXOTIC_SAFARI_FIRST_PART)
    {
        for (i = EXOTIC_SAFARI_FIRST_PART; i < ARRAY_COUNT(sRelics); i++)
            left += !FlagGet(sRelics[i].flag);
        gSpecialVar_Result = left;
        ConvertIntToDecimalStringN(gStringVar2, left, STR_CONV_MODE_LEFT_ALIGN, 2);
    }
}

// Special (the caretaker): VAR_RESULT = how many of the three first partners the player hasn't caught;
// STR_VAR_1 = the first of those
void ExoticSafari_CountStarters(void)
{
    u32 i, left = 0;
    for (i = 0; i < ARRAY_COUNT(sAlolaStarters); i++)
    {
        if (!GetSetPokedexFlag(SpeciesToNationalPokedexNum(sAlolaStarters[i]), FLAG_GET_CAUGHT))
        {
            if (left++ == 0)
                StringCopy(gStringVar1, GetSpeciesName(sAlolaStarters[i]));
        }
    }
    gSpecialVar_Result = left;
}
