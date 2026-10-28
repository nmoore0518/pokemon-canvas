// Pokémon Canvas: the Exotic Pokémon Safari (Fuchsia City; built by
// tools/gym_hack/build_exotic_safari.py): the hall's doors, the roaming legendaries, the kiosk.
//
// THE HALL
// One door per region along the back wall, each in its games' two colors. The doors are closed;
// an open zone's door slides open when a player with the Exotic Pass stands in front of it (or
// comes out of it), and closes again when they walk away. Checked after every step in the hall
// (src/field_control_avatar.c) and when the hall loads.
#include "global.h"
#include "event_mons.h"
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
#include "constants/characters.h"
#include "item.h"
#include "overworld.h"
#include "random.h"
#include "shop.h"
#include "sound.h"
#include "main.h"
#include "task.h"
#include "party_menu.h"
#include "constants/moves.h"
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
// The area's name (the place in the games it's based on) for the map name pop-up, or NULL when the
// player isn't in the Exotic Safari
const u8 *ExoticSafari_GetAreaName(void)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sExoticSafariNames); i++)
    {
        if (sExoticSafariNames[i].mapGroup == gSaveBlock1Ptr->location.mapGroup
         && sExoticSafariNames[i].mapNum == gSaveBlock1Ptr->location.mapNum)
            return sExoticSafariNames[i].name;
    }
    return NULL;
}

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
enum { LEAD_LOVING, LEAD_DRAGON, LEAD_STEEL, LEAD_SHADOW, LEAD_STEAM };

static bool32 SpeciesHasType(u32 species, u32 type)
{
    return GetSpeciesType(species, 0) == type || GetSpeciesType(species, 1) == type;
}

// Whether a party member other than the lead has a type
static bool32 BehindLeadHas(u32 type)
{
    u32 i;
    for (i = 1; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG)
         && SpeciesHasType(GetMonData(mon, MON_DATA_SPECIES), type))
            return TRUE;
    }
    return FALSE;
}

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
    if (what == LEAD_STEAM)         // (Volcanion: Fire in the lead and Water behind it, or the reverse)
        return (SpeciesHasType(species, TYPE_FIRE) && BehindLeadHas(TYPE_WATER))
            || (SpeciesHasType(species, TYPE_WATER) && BehindLeadHas(TYPE_FIRE));
    if (what == LEAD_SHADOW)        // (Marshadow: a Fighting or Ghost type)
        return GetSpeciesType(species, 0) == TYPE_FIGHTING || GetSpeciesType(species, 1) == TYPE_FIGHTING
            || GetSpeciesType(species, 0) == TYPE_GHOST || GetSpeciesType(species, 1) == TYPE_GHOST;
    if (what == LEAD_STEEL)         // (Meltan)
        return GetSpeciesType(species, 0) == TYPE_STEEL || GetSpeciesType(species, 1) == TYPE_STEEL;
    return GetSpeciesType(species, 0) == TYPE_DRAGON || GetSpeciesType(species, 1) == TYPE_DRAGON;
}

// Sinnoh: what the lead POKéMON needs for the lake trio (Uxie: a level; Mesprit: a held berry;
// Azelf: HP EVs), and the party Regigigas waits for (Regirock, Regice and Registeel)
static struct Pokemon *Lead(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return NULL;
    return mon;
}

static bool32 PartyHas(u32 species)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (!GetMonData(mon, MON_DATA_IS_EGG) && GetMonData(mon, MON_DATA_SPECIES) == species)
            return TRUE;
    }
    return FALSE;
}

static const u16 sRegis[] = {SPECIES_REGIROCK, SPECIES_REGICE, SPECIES_REGISTEEL};

static u32 RegisMissing(void)
{
    u32 i, left = 0;
    for (i = 0; i < ARRAY_COUNT(sRegis); i++)
        left += !PartyHas(sRegis[i]);
    return left;
}

// STR_VAR_1: the Regis not in the party ("A", "A and B", "A, B and C")
static void BufferRegisMissing(void)
{
    u32 i, n = 0, total = RegisMissing();
    u8 *s = gStringVar1;
    *s = EOS;
    for (i = 0; i < ARRAY_COUNT(sRegis); i++)
    {
        if (PartyHas(sRegis[i]))
            continue;
        if (n > 0)
            s = StringAppend(s, n + 1 == total ? COMPOUND_STRING(" and ") : COMPOUND_STRING(", "));
        s = StringAppend(s, GetSpeciesName(sRegis[i]));
        n++;
    }
}

// Special (the Snowpoint elder): VAR_RESULT = whether every gym type has been mastered
void ExoticSafari_AllTypesMastered(void)
{
    gSpecialVar_Result = GymSwitch_AllTypesMastered();
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

// The Zygarde Cells found (the first 8 from FLAG_EXOTIC_CELL_FIRST, the rest from
// FLAG_EXOTIC_CELL_MORE_FIRST)
static u16 CellFlag(u32 i)
{
    return i < 8 ? FLAG_EXOTIC_CELL_FIRST + i : FLAG_EXOTIC_CELL_MORE_FIRST + i - 8;
}

static u32 CellsFound(void)
{
    u32 i, n = 0;
    for (i = 0; i < EXOTIC_SAFARI_CELLS; i++)
        n += FlagGet(CellFlag(i)) != 0;
    return n;
}

// Galar: the areas the player has been back to since catching Kubfu (all of them: a second Kubfu
// comes to the pitch)
static u32 GalarVisited(void)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sGalarVisitMaps); i++)
        n += FlagGet(FLAG_EXOTIC_GALAR_VISIT_FIRST + i) != 0;
    return n;
}

// Whether a static legendary is caught (and so stays away). Kubfu: caught once, it comes back once
// every Galar area has been visited again
static bool32 StaticCaught(const struct ExoticSafariStatic *s)
{
    if (s->species == SPECIES_KUBFU && FlagGet(s->caughtFlag) && !FlagGet(FLAG_EXOTIC_KUBFU_TWICE))
        return GalarVisited() < ARRAY_COUNT(sGalarVisitMaps);
    return FlagGet(s->caughtFlag);
}

// A legendary's caught flag (sExoticSafariStatics or the roamers)
static u16 CaughtFlag(u16 species)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sExoticSafariStatics); i++)
        if (sExoticSafariStatics[i].species == species)
            return sExoticSafariStatics[i].caughtFlag;
    for (i = 0; i < EXOTIC_SAFARI_ROAMER_COUNT; i++)
        if (sExoticSafariRoamers[i].species == species)
            return sExoticSafariRoamers[i].caughtFlag;
    return 0;
}

// The event Pokémon's statics (src/event_mons.c): who must be in the party, and one that must be caught
struct PartySeal { u16 party[3]; u16 caught; };
static const struct PartySeal sPartySeals[] =
{
    {{SPECIES_CELEBI}, SPECIES_NONE},                                        // (Spiky-eared Pichu)
    {{SPECIES_CELEBI}, SPECIES_ZARUDE},                                      // (Zarude, Dada)
    {{SPECIES_ZACIAN, SPECIES_ZAMAZENTA, SPECIES_ETERNATUS}, SPECIES_NONE},  // (Eternamax)
};

static bool32 PartyHasBase(u32 base)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (!GetMonData(mon, MON_DATA_IS_EGG) && GET_BASE_SPECIES_ID(GetMonData(mon, MON_DATA_SPECIES)) == base)
            return TRUE;
    }
    return FALSE;
}

static u16 CaughtFlag(u16 species);

// How much of a party seal is left; STR_VAR_1 = what's missing first, VAR_0x8005 = whether that's a
// Pokémon to catch (TRUE) or one to bring (FALSE)
static u32 PartySealLeft(u32 k, bool32 buffer)
{
    const struct PartySeal *s = &sPartySeals[min(k, ARRAY_COUNT(sPartySeals) - 1)];
    u32 i, left = 0;
    if (s->caught != SPECIES_NONE && !FlagGet(CaughtFlag(s->caught)))
    {
        if (left++ == 0 && buffer)
        {
            StringCopy(gStringVar1, GetSpeciesName(s->caught));
            gSpecialVar_0x8005 = TRUE;
        }
    }
    for (i = 0; i < ARRAY_COUNT(s->party); i++)
    {
        if (s->party[i] == SPECIES_NONE || PartyHasBase(s->party[i]))
            continue;
        if (left++ == 0 && buffer)
        {
            StringCopy(gStringVar1, GetSpeciesName(s->party[i]));
            gSpecialVar_0x8005 = FALSE;
        }
    }
    return left;
}

// STR_VAR_1: the legendaries of sCaughtSeals[k] not yet caught ("A", "A and B", "A, B and C")
static void BufferUncaught(u32 k)
{
    u32 i, n = 0, total = 0;
    u8 *s = gStringVar1;
    for (i = 0; i < ARRAY_COUNT(sCaughtSeals[0]); i++)
        total += sCaughtSeals[k][i] != SPECIES_NONE && !FlagGet(CaughtFlag(sCaughtSeals[k][i]));
    *s = EOS;
    for (i = 0; i < ARRAY_COUNT(sCaughtSeals[0]); i++)
    {
        u16 sp = sCaughtSeals[k][i];
        if (sp == SPECIES_NONE || FlagGet(CaughtFlag(sp)))
            continue;
        if (n > 0)
            s = StringAppend(s, n + 1 == total ? COMPOUND_STRING(" and ") : COMPOUND_STRING(", "));
        s = StringAppend(s, GetSpeciesName(sp));
        n++;
    }
}

// The Kalos POKéMON (National Dex 650-721) the player has caught
static u32 KalosCaught(void)
{
    u32 n, count = 0;
    for (n = NATIONAL_DEX_CHESPIN; n <= NATIONAL_DEX_VOLCANION; n++)
        count += GetSetPokedexFlag(n, FLAG_GET_CAUGHT) != 0;
    return count;
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
    case SEAL_DOJO:         // (the second Kubfu: the Galar areas not visited again yet)
        if (FlagGet(s->caughtFlag))
            return ARRAY_COUNT(sGalarVisitMaps) - GalarVisited();
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
    case SEAL_CELLS:        // (Zygarde: sealArg of its Cells found)
        return CellsFound() >= s->sealArg ? 0 : s->sealArg - CellsFound();
    case SEAL_BOTTLE:       // (Hoopa: the Prison Bottle taken)
        return !FlagGet(FLAG_EXOTIC_PRISON_BOTTLE);
    case SEAL_STEAM:
        return !LeadIs(LEAD_STEAM);
    case SEAL_CAUGHT:       // (other legendaries caught first: sCaughtSeals[sealArg])
        for (i = 0; i < ARRAY_COUNT(sCaughtSeals[0]); i++)
            left += sCaughtSeals[s->sealArg][i] != SPECIES_NONE && !FlagGet(CaughtFlag(sCaughtSeals[s->sealArg][i]));
        return left;
    case SEAL_KEY:          // (a keepsake found: sUnovaKeys[sealArg])
        return !FlagGet(sUnovaKeys[s->sealArg].flag);
    case SEAL_DRAIN:        // (Yveltal: sealArg Kalos POKéMON caught)
        return KalosCaught() >= s->sealArg ? 0 : s->sealArg - KalosCaught();
    case SEAL_LEVEL:        // (Uxie: a lead of at least level sealArg)
        return !(Lead() && GetMonData(Lead(), MON_DATA_LEVEL) >= s->sealArg);
    case SEAL_BERRY:        // (Mesprit: a lead holding a berry)
        return !(Lead() && GetItemPocket(GetMonData(Lead(), MON_DATA_HELD_ITEM)) == POCKET_BERRIES);
    case SEAL_HP_EVS:       // (Azelf: a lead with sealArg HP EVs)
        return !(Lead() && GetMonData(Lead(), MON_DATA_HP_EV) >= s->sealArg);
    case SEAL_REGIS:        // (Regigigas: Regirock, Regice and Registeel in the party)
        return RegisMissing();
    case SEAL_AZURE_FLUTE:  // (Arceus: the Azure Flute, from the Snowpoint elder)
        return !FlagGet(FLAG_EXOTIC_AZURE_FLUTE);
    case SEAL_METEORITE:    // (Deoxys: the Meteorite found)
        return !FlagGet(FLAG_EXOTIC_ITEM_METEORITE);
    case SEAL_BLUE_ORB:     // (Kyogre: a lead holding the Blue Orb)
        return !(Lead() && GetMonData(Lead(), MON_DATA_HELD_ITEM) == ITEM_BLUE_ORB);
    case SEAL_GS_BALL:      // (Celebi: the GS Ball found)
        return !FlagGet(FLAG_EXOTIC_GS_BALL);
    case SEAL_PARTY:        // (event Pokémon: sPartySeals[sealArg])
        return PartySealLeft(s->sealArg, FALSE);
    case SEAL_WISH:         // (Jirachi: the zone's other legendaries caught)
        for (i = 0; i < ARRAY_COUNT(sWishSeal); i++)
            left += !FlagGet(CaughtFlag(sWishSeal[i]));
        return left;
    }
    return 0;
}

// The seal as the hint should tell it (Regidrago's: the fang first, then the lead)
static u32 SealToTell(const struct ExoticSafariStatic *s)
{
    if (s->seal == SEAL_DRAGON && FlagGet(sSealItems[s->sealArg][1]))
        return SEAL_DRAGON_LEAD;
    if (s->seal == SEAL_DOJO && FlagGet(s->caughtFlag))
        return SEAL_REVISIT;
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
    if (FlagGet(s->caughtFlag) && (s->species != SPECIES_KUBFU || FlagGet(FLAG_EXOTIC_KUBFU_TWICE)))
        gSpecialVar_Result = SEAL_RESULT_CAUGHT;     // (Kubfu, caught once: its seal tells of the second)
    else if (left == 0)
        gSpecialVar_Result = SEAL_RESULT_AWAY;
    else
        gSpecialVar_Result = SealToTell(s);
    if (s->seal == SEAL_STAKES)
        StringCopy(gStringVar1, sStakeColorNames[s->sealArg]);
    else if (s->seal == SEAL_ITEM || s->seal == SEAL_DRAGON)
        CopyItemName(sSealItems[s->sealArg][0], gStringVar1);
    else if (s->seal == SEAL_CAUGHT)
        BufferUncaught(s->sealArg);
    else if (s->seal == SEAL_KEY)
        StringCopy(gStringVar1, sUnovaKeys[s->sealArg].name);
    else if (s->seal == SEAL_FLUTE)
        StringCopy(gStringVar1, sRelics[s->sealArg].name);
    else if (s->seal == SEAL_REGIS)
        BufferRegisMissing();
    else if (s->seal == SEAL_PARTY)
        PartySealLeft(s->sealArg, TRUE);
    if (s->seal == SEAL_BLUE_ORB)            // (VAR_0x8005: the Blue Orb found yet)
        gSpecialVar_0x8005 = FlagGet(FLAG_EXOTIC_ITEM_BLUE_ORB);
    ConvertIntToDecimalStringN(gStringVar2, left, STR_CONV_MODE_LEFT_ALIGN, 2);
    if (s->seal == SEAL_LEVEL || s->seal == SEAL_HP_EVS)     // (the level or EVs it wants)
        ConvertIntToDecimalStringN(gStringVar2, s->sealArg, STR_CONV_MODE_LEFT_ALIGN, 3);
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
    if (StaticCaught(s) || SealLeft(s) != 0 || !FlagGet(s->hideFlag))
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

// ---------------------------------------------------------------------------------------------
// Johto: the beasts (free once every area has been visited), the Ruins of Alph's Unown panels
// ---------------------------------------------------------------------------------------------
static u32 JohtoVisited(void)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sJohtoVisitMaps); i++)
        n += FlagGet(FLAG_EXOTIC_JOHTO_VISIT_FIRST + i) != 0;
    return n;
}

// Special (the old man in New Bark Town): VAR_RESULT = how many of the areas the player has been
// in (STR_VAR_1), STR_VAR_2 = how many there are
void ExoticSafari_BeastSage(void)
{
    gSpecialVar_Result = JohtoVisited();
    ConvertIntToDecimalStringN(gStringVar1, gSpecialVar_Result, STR_CONV_MODE_LEFT_ALIGN, 1);
    ConvertIntToDecimalStringN(gStringVar2, ARRAY_COUNT(sJohtoVisitMaps), STR_CONV_MODE_LEFT_ALIGN, 1);
    if (gSpecialVar_Result == ARRAY_COUNT(sJohtoVisitMaps))
        FlagSet(FLAG_EXOTIC_BEASTS_FREE);
}

#define UNOWN_PANELS 4
static u32 UnownPanelsOpen(void)
{
    u32 i, n = 0;
    for (i = 0; i < UNOWN_PANELS; i++)
        n += FlagGet(FLAG_EXOTIC_UNOWN_PANEL_FIRST + i) != 0;
    return n;
}

// Special (a chamber's stone panel, its words read): VAR_0x8004 = which (0 ESCAPE: an Escape Rope in
// the bag, 1 WATER: a Water Stone, 2 LIGHT: a lead that knows Flash, 3 HO-OH: Ho-Oh in the party).
// VAR_RESULT: 0 already open, 1 nothing answers it, 2 it opens, 3 it opens and it's the last.
// STR_VAR_1: how many are open now
void ExoticSafari_UnownPanel(void)
{
    u32 k = gSpecialVar_0x8004 % UNOWN_PANELS;
    bool32 ok;
    if (FlagGet(FLAG_EXOTIC_UNOWN_PANEL_FIRST + k))
    {
        gSpecialVar_Result = 0;
        return;
    }
    switch (k)
    {
    case 0:  ok = CheckBagHasItem(ITEM_ESCAPE_ROPE, 1); break;
    case 1:  ok = CheckBagHasItem(ITEM_WATER_STONE, 1); break;
    case 2:  ok = Lead() && MonKnowsMove(Lead(), MOVE_FLASH); break;
    default: ok = PartyHas(SPECIES_HO_OH); break;
    }
    if (!ok)
    {
        gSpecialVar_Result = 1;
        return;
    }
    FlagSet(FLAG_EXOTIC_UNOWN_PANEL_FIRST + k);
    ConvertIntToDecimalStringN(gStringVar1, UnownPanelsOpen(), STR_CONV_MODE_LEFT_ALIGN, 1);
    gSpecialVar_Result = UnownPanelsOpen() == UNOWN_PANELS ? 3 : 2;
}

// ---------------------------------------------------------------------------------------------
// WILD POKéMON: shared slots, random forms, Zygarde
// ---------------------------------------------------------------------------------------------
// Families whose wild Pokémon come in any of their forms (patterns, colors, sizes, seasons)
#define VIVILLON_PATTERNS(stage) \
    SPECIES_##stage##_ICY_SNOW, SPECIES_##stage##_POLAR, SPECIES_##stage##_TUNDRA, SPECIES_##stage##_CONTINENTAL, \
    SPECIES_##stage##_GARDEN, SPECIES_##stage##_ELEGANT, SPECIES_##stage##_MEADOW, SPECIES_##stage##_MODERN, \
    SPECIES_##stage##_MARINE, SPECIES_##stage##_ARCHIPELAGO, SPECIES_##stage##_HIGH_PLAINS, SPECIES_##stage##_SANDSTORM, \
    SPECIES_##stage##_RIVER, SPECIES_##stage##_MONSOON, SPECIES_##stage##_SAVANNA, SPECIES_##stage##_SUN, \
    SPECIES_##stage##_OCEAN, SPECIES_##stage##_JUNGLE, SPECIES_NONE
static const u16 sScatterbugForms[] = {VIVILLON_PATTERNS(SCATTERBUG)};
static const u16 sSpewpaForms[] = {VIVILLON_PATTERNS(SPEWPA)};
static const u16 sVivillonForms[] = {VIVILLON_PATTERNS(VIVILLON)};
#define FLOWER_COLORS(stage) SPECIES_##stage##_RED, SPECIES_##stage##_YELLOW, SPECIES_##stage##_ORANGE, SPECIES_##stage##_BLUE, SPECIES_##stage##_WHITE, SPECIES_NONE
static const u16 sFlabebeForms[] = {FLOWER_COLORS(FLABEBE)};
static const u16 sFloetteForms[] = {FLOWER_COLORS(FLOETTE)};
static const u16 sFlorgesForms[] = {FLOWER_COLORS(FLORGES)};
#define PUMPKIN_SIZES(stage) SPECIES_##stage##_AVERAGE, SPECIES_##stage##_SMALL, SPECIES_##stage##_LARGE, SPECIES_##stage##_SUPER, SPECIES_NONE
static const u16 sPumpkabooForms[] = {PUMPKIN_SIZES(PUMPKABOO)};
static const u16 sGourgeistForms[] = {PUMPKIN_SIZES(GOURGEIST)};
#define SEASONS(stage) SPECIES_##stage##_SPRING, SPECIES_##stage##_SUMMER, SPECIES_##stage##_AUTUMN, SPECIES_##stage##_WINTER, SPECIES_NONE
static const u16 sDeerlingForms[] = {SEASONS(DEERLING)};
static const u16 sSawsbuckForms[] = {SEASONS(SAWSBUCK)};
static const u16 sMiniorForms[] = {SPECIES_MINIOR_METEOR_RED, SPECIES_MINIOR_METEOR_ORANGE, SPECIES_MINIOR_METEOR_YELLOW,
                                   SPECIES_MINIOR_METEOR_GREEN, SPECIES_MINIOR_METEOR_BLUE, SPECIES_MINIOR_METEOR_INDIGO,
                                   SPECIES_MINIOR_METEOR_VIOLET, SPECIES_NONE};
static const u16 sDudunsparceForms[] = {SPECIES_DUDUNSPARCE_TWO_SEGMENT, SPECIES_DUDUNSPARCE_THREE_SEGMENT, SPECIES_NONE};

static const u16 *const sRandomFormFamilies[] =
{
    sScatterbugForms, sSpewpaForms, sVivillonForms, sFlabebeForms, sFloetteForms, sFlorgesForms,
    sPumpkabooForms, sGourgeistForms, sDeerlingForms, sSawsbuckForms, sMiniorForms, sDudunsparceForms,
};

static u16 RandomForm(u16 species)
{
    u32 f, i, n;
    for (f = 0; f < ARRAY_COUNT(sRandomFormFamilies); f++)
    {
        const u16 *forms = sRandomFormFamilies[f];
        for (i = 0; forms[i] != SPECIES_NONE; i++)
        {
            if (forms[i] == species)
            {
                for (n = 0; forms[n] != SPECIES_NONE; n++)
                    ;
                return forms[Random() % n];
            }
        }
    }
    return species;
}

// How many species share this slot here (1 for most; src/incense.c)
unsigned int ExoticSafari_PoolSize(unsigned short species)
{
    u32 i, n;
    for (i = 0; i < ARRAY_COUNT(sSlotPools); i++)
    {
        const struct ExoticSafariPool *p = &sSlotPools[i];
        if (p->mapGroup != gSaveBlock1Ptr->location.mapGroup || p->mapNum != gSaveBlock1Ptr->location.mapNum
         || p->species[0] != species)
            continue;
        for (n = 1; n < ARRAY_COUNT(p->species) && p->species[n] != SPECIES_NONE; n++)
            ;
        return n;
    }
    return 1;
}

// A slot shared by several species (the area's table names the first: sSlotPools)
static u16 PoolSpecies(u16 species)
{
    u32 i, n;
    for (i = 0; i < ARRAY_COUNT(sSlotPools); i++)
    {
        const struct ExoticSafariPool *p = &sSlotPools[i];
        if (p->mapGroup != gSaveBlock1Ptr->location.mapGroup || p->mapNum != gSaveBlock1Ptr->location.mapNum
         || p->species[0] != species)
            continue;
        for (n = 1; n < ARRAY_COUNT(p->species) && p->species[n] != SPECIES_NONE; n++)
            ;
        return p->species[Random() % n];
    }
    return species;
}

// Zygarde, once the first is caught: now and then, anywhere in the Kalos zone, another of its forms
// (10% Forme; 50% Forme once 8 Cells are found; the Power Construct ones - Complete Forme in battle -
// once all of them are)
#define ZYGARDE_WILD_ODDS 50

static bool32 InZygardeRegion(void)
{
    const struct ExoticSafariArea *area = GetArea();
    u32 i, j;
    if (area == NULL)
        return FALSE;
    for (i = 0; i < ARRAY_COUNT(sExoticSafariStatics); i++)
    {
        if (sExoticSafariStatics[i].species != SPECIES_ZYGARDE)
            continue;
        for (j = 0; j < ARRAY_COUNT(sExoticSafariAreas); j++)
        {
            if (sExoticSafariAreas[j].mapGroup == sExoticSafariStatics[i].mapGroup
             && sExoticSafariAreas[j].mapNum == sExoticSafariStatics[i].mapNum)
                return sExoticSafariAreas[j].region == area->region;
        }
    }
    return FALSE;
}

static u16 WildZygarde(void)
{
    u16 forms[4];
    u32 n = 0;
    forms[n++] = SPECIES_ZYGARDE_10_AURA_BREAK;
    if (CellsFound() >= EXOTIC_SAFARI_CELLS_FOR_50)
        forms[n++] = SPECIES_ZYGARDE_50;
    if (CellsFound() >= EXOTIC_SAFARI_CELLS)
    {
        forms[n++] = SPECIES_ZYGARDE_10_POWER_CONSTRUCT;
        forms[n++] = SPECIES_ZYGARDE_50_POWER_CONSTRUCT;
    }
    return forms[Random() % n];
}

// A wild Pokémon about to be made (src/wild_encounter.c): its slot's other species, any of its
// forms; in the Ruins of Alph, once every panel is open, an Unown is any of its 28 forms
u16 ExoticSafari_WildSpecies(u16 species, u8 *level)
{
    u32 k;
    bool32 unown = species == SPECIES_UNOWN || (species >= SPECIES_UNOWN_B && species <= SPECIES_UNOWN_QUESTION);
    if (FlagGet(FLAG_EXOTIC_LEGEND_ZYGARDE) && InZygardeRegion() && Random() % ZYGARDE_WILD_ODDS == 0)
    {
        *level = EXOTIC_SAFARI_LEGEND_LEVEL;
        return WildZygarde();
    }
    species = EventMons_WildVivillonPattern(RandomForm(PoolSpecies(species)));   // (Fancy, Poké Ball: src/event_mons.c)
    if (!unown || UnownPanelsOpen() < UNOWN_PANELS
     || gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(EXOTIC_SAFARI_UNOWN_MAP)
     || gSaveBlock1Ptr->location.mapNum != MAP_NUM(EXOTIC_SAFARI_UNOWN_MAP))
        return species;
    k = Random() % 28;
    return k == 0 ? SPECIES_UNOWN : SPECIES_UNOWN_B + k - 1;
}

// An evolution about to happen (src/pokemon.c): Alcremie comes in any of its nine creams (which one
// is the Milcery's own: from its personality), its sweet as given
static const u16 sAlcremieVanilla[] = {SPECIES_ALCREMIE_STRAWBERRY_VANILLA_CREAM, SPECIES_ALCREMIE_BERRY_VANILLA_CREAM,
    SPECIES_ALCREMIE_LOVE_VANILLA_CREAM, SPECIES_ALCREMIE_STAR_VANILLA_CREAM, SPECIES_ALCREMIE_CLOVER_VANILLA_CREAM,
    SPECIES_ALCREMIE_FLOWER_VANILLA_CREAM, SPECIES_ALCREMIE_RIBBON_VANILLA_CREAM};
static const u16 sAlcremieRuby[] = {SPECIES_ALCREMIE_STRAWBERRY_RUBY_CREAM, SPECIES_ALCREMIE_BERRY_RUBY_CREAM,
    SPECIES_ALCREMIE_LOVE_RUBY_CREAM, SPECIES_ALCREMIE_STAR_RUBY_CREAM, SPECIES_ALCREMIE_CLOVER_RUBY_CREAM,
    SPECIES_ALCREMIE_FLOWER_RUBY_CREAM, SPECIES_ALCREMIE_RIBBON_RUBY_CREAM};

u16 ExoticSafari_EvolvedForm(struct Pokemon *mon, u16 target)
{
    u32 i, cream;
    for (i = 0; i < ARRAY_COUNT(sAlcremieVanilla); i++)
    {
        if (target != sAlcremieVanilla[i])
            continue;
        cream = (GetMonData(mon, MON_DATA_PERSONALITY) >> 8) % 9;    // (vanilla, then ruby ... rainbow swirl)
        return cream == 0 ? target : sAlcremieRuby[i] + cream - 1;
    }
    return target;
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
        if (StaticCaught(s) || SealLeft(s) != 0)
            FlagSet(s->hideFlag);
    }
    // Galar: once Kubfu is caught, the areas the player goes back to (all: another Kubfu)
    if (FlagGet(FLAG_EXOTIC_LEGEND_KUBFU) && !FlagGet(FLAG_EXOTIC_KUBFU_TWICE))
    {
        for (i = 0; i < ARRAY_COUNT(sGalarVisitMaps); i++)
        {
            if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(sGalarVisitMaps[i])
             && gSaveBlock1Ptr->location.mapNum == MAP_NUM(sGalarVisitMaps[i]))
                FlagSet(FLAG_EXOTIC_GALAR_VISIT_FIRST + i);
        }
    }
    // Johto: the areas the player has been in (all six free the beasts)
    for (i = 0; i < ARRAY_COUNT(sJohtoVisitMaps); i++)
    {
        if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(sJohtoVisitMaps[i])
         && gSaveBlock1Ptr->location.mapNum == MAP_NUM(sJohtoVisitMaps[i]))
            FlagSet(FLAG_EXOTIC_JOHTO_VISIT_FIRST + i);
    }
    if (JohtoVisited() == ARRAY_COUNT(sJohtoVisitMaps))
        FlagSet(FLAG_EXOTIC_BEASTS_FREE);
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

// The supply kiosk: Balls, a Max Repel, medicine and the Incense
void ExoticSafari_Kiosk(void)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sKioskSupplies); i++)
        sKioskItems[n++] = sKioskSupplies[i];
    sKioskItems[n] = ITEM_NONE;
    CreatePokemartMenu(sKioskItems);
}

// The evolution kiosk (the other side of the counter): the evolution items found in the zones.
// VAR_RESULT: FALSE if there's nothing to sell yet
void ExoticSafari_EvoKiosk(void)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sKioskFinds); i++)
    {
        if (FlagGet(sKioskFinds[i].flag))
            sKioskItems[n++] = sKioskFinds[i].item;
    }
    sKioskItems[n] = ITEM_NONE;
    gSpecialVar_Result = n != 0;
    if (n != 0)
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
static void SetRegiWall(u32 k, bool32 open, bool32 draw);

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
    for (i = 0; i < ARRAY_COUNT(sRegiWalls); i++)
    {
        if (InMap(MAP_GROUP(sRegiWalls[i].map), MAP_NUM(sRegiWalls[i].map)) && !FlagGet(FLAG_EXOTIC_REGI_WALL_FIRST + i))
            SetRegiWall(i, FALSE, FALSE);
    }
}

// ---------------------------------------------------------------------------------------------
// Hoenn: the Regi chambers' inner walls (Emerald's). Sealed until the tablet by the Island Cave
// sees Wailord first and Relicanth last (FLAG_EXOTIC_REGIS_UNSEALED); then each opens for its own
// Braille: Regirock's (0) a lead that knows Rock Smash, Regice's (1) two minutes standing still
// before it, Registeel's (2) a lead that knows Flash
// ---------------------------------------------------------------------------------------------
static void SetRegiWall(u32 k, bool32 open, bool32 draw)
{
    const struct ExoticSafariRegiWall *w = &sRegiWalls[k];
    u32 i;
    for (i = 0; i < 6; i++)
        MapGridSetMetatileEntryAt(REGI_WALL_X + i % 3 + MAP_OFFSET, REGI_WALL_Y + i / 3 + MAP_OFFSET,
                                  open ? w->open[i] : (i < 3 ? w->cover : w->braille));
    if (draw)
        DrawWholeMapView();
}

// Special (the wall read): VAR_0x8004 = which. VAR_RESULT: 0 still sealed (the tablet), 1 its
// puzzle to do, 2 open. STR_VAR_1: the lead's name
void ExoticSafari_RegiWall(void)
{
    u32 k = gSpecialVar_0x8004 % ARRAY_COUNT(sRegiWalls);
    if (FlagGet(FLAG_EXOTIC_REGI_WALL_FIRST + k))
        gSpecialVar_Result = 2;
    else
        gSpecialVar_Result = FlagGet(FLAG_EXOTIC_REGIS_UNSEALED) ? 1 : 0;
    gStringVar1[0] = EOS;
    if (Lead())
        GetMonNickname(Lead(), gStringVar1);
}

// Special: VAR_RESULT = the lead knows the move the wall asks for (VAR_0x8004: 0 Rock Smash, else Flash)
void ExoticSafari_RegiLeadKnows(void)
{
    gSpecialVar_Result = Lead() && MonKnowsMove(Lead(), gSpecialVar_0x8004 == 0 ? MOVE_ROCK_SMASH : MOVE_FLASH);
}

// Special: VAR_0x8004's wall opens (with a bang)
void ExoticSafari_OpenRegiWall(void)
{
    u32 k = gSpecialVar_0x8004 % ARRAY_COUNT(sRegiWalls);
    FlagSet(FLAG_EXOTIC_REGI_WALL_FIRST + k);
    SetRegiWall(k, TRUE, TRUE);
    PlaySE(SE_BANG);
}

// Regice's: two minutes without a step, in front of its wall, after reading it. The task ends if
// the player moves or leaves; the wall opens when the time is up (once nothing else is going on)
#define STILLNESS_FRAMES (120 * 60)
#define tX     data[0]
#define tY     data[1]
#define tMap   data[2]
#define tTime  data[3]

static void Task_Stillness(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (gSaveBlock1Ptr->pos.x != tX || gSaveBlock1Ptr->pos.y != tY
        || (gSaveBlock1Ptr->location.mapGroup << 8 | gSaveBlock1Ptr->location.mapNum) != (u16)tMap
        || FlagGet(FLAG_EXOTIC_REGI_WALL_FIRST + 1))
    {
        DestroyTask(taskId);
        return;
    }
    if (tTime < STILLNESS_FRAMES)
        tTime++;
    else if (gMain.callback2 == CB2_Overworld && !ScriptContext_IsEnabled() && !ArePlayerFieldControlsLocked())
    {
        gSpecialVar_0x8004 = 1;
        ExoticSafari_OpenRegiWall();
        DestroyTask(taskId);
    }
}

// Special (Regice's wall read): start counting
void ExoticSafari_StartStillness(void)
{
    u8 taskId;
    if (FuncIsActiveTask(Task_Stillness))
        DestroyTask(FindTaskIdByFunc(Task_Stillness));
    taskId = CreateTask(Task_Stillness, 80);
    gTasks[taskId].tX = gSaveBlock1Ptr->pos.x;
    gTasks[taskId].tY = gSaveBlock1Ptr->pos.y;
    gTasks[taskId].tMap = gSaveBlock1Ptr->location.mapGroup << 8 | gSaveBlock1Ptr->location.mapNum;
    gTasks[taskId].tTime = 0;
}
#undef tX
#undef tY
#undef tMap
#undef tTime

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

// Special (a Zygarde Cell picked up): VAR_0x8004 = which. VAR_RESULT = how many are still out there;
// STR_VAR_1 = how many found, STR_VAR_2 = how many left
void ExoticSafari_TakeCell(void)
{
    FlagSet(CellFlag(gSpecialVar_0x8004 % EXOTIC_SAFARI_CELLS));
    gSpecialVar_Result = EXOTIC_SAFARI_CELLS - CellsFound();
    ConvertIntToDecimalStringN(gStringVar1, CellsFound(), STR_CONV_MODE_LEFT_ALIGN, 2);
    ConvertIntToDecimalStringN(gStringVar2, gSpecialVar_Result, STR_CONV_MODE_LEFT_ALIGN, 2);
}

// Special (Zygarde, before its battle): VAR_RESULT = its Cells still out there; STR_VAR_1 = the
// Forme it's in ("10%", "50%"), STR_VAR_2 = the Cells left
void ExoticSafari_CountCells(void)
{
    gSpecialVar_Result = EXOTIC_SAFARI_CELLS - CellsFound();
    StringCopy(gStringVar1, CellsFound() >= EXOTIC_SAFARI_CELLS_FOR_50 ? COMPOUND_STRING("50%") : COMPOUND_STRING("10%"));
    ConvertIntToDecimalStringN(gStringVar2, gSpecialVar_Result, STR_CONV_MODE_LEFT_ALIGN, 2);
}

// Special (a legendary's battle, VAR_0x8004 its species): the form it battles in. Zygarde: its 10%
// Forme; its 50% Forme with 8 of its Cells found; with all of them, whole: Power Construct (its 100%
// Complete Forme in battle)
// Special (after the level is set, VAR_0x8005): the event Pokémon's own levels (Spiky-eared Pichu: a
// first-stage Pokémon, at a starter's level)
void ExoticSafari_LegendLevel(void)
{
    if (gSpecialVar_0x8004 == SPECIES_PICHU_SPIKY_EARED)
        gSpecialVar_0x8005 = 5;
}

void ExoticSafari_LegendForm(void)
{
    if (gSpecialVar_0x8004 != SPECIES_ZYGARDE)
        return;
    if (CellsFound() >= EXOTIC_SAFARI_CELLS)
        gSpecialVar_0x8004 = SPECIES_ZYGARDE_50_POWER_CONSTRUCT;
    else if (CellsFound() >= EXOTIC_SAFARI_CELLS_FOR_50)
        gSpecialVar_0x8004 = SPECIES_ZYGARDE_50;
    else
        gSpecialVar_0x8004 = SPECIES_ZYGARDE_10;
}

// The Furfrou groomer's trims, in her menu's order
static const u16 sFurfrouTrims[] =
{
    SPECIES_FURFROU_HEART, SPECIES_FURFROU_STAR, SPECIES_FURFROU_DIAMOND, SPECIES_FURFROU_DEBUTANTE,
    SPECIES_FURFROU_MATRON, SPECIES_FURFROU_DANDY, SPECIES_FURFROU_LA_REINE, SPECIES_FURFROU_KABUKI,
    SPECIES_FURFROU_PHARAOH, SPECIES_FURFROU_NATURAL,
};

// Special: VAR_RESULT = whether party slot VAR_0x8004 is a Furfrou (STR_VAR_1: its name)
void ExoticSafari_IsFurfrou(void)
{
    struct Pokemon *mon;
    gSpecialVar_Result = FALSE;
    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        return;
    mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    if (GetMonData(mon, MON_DATA_IS_EGG) || GET_BASE_SPECIES_ID(GetMonData(mon, MON_DATA_SPECIES)) != SPECIES_FURFROU)
        return;
    GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
    StringGet_Nickname(gStringVar1);
    gSpecialVar_Result = TRUE;
}

// Special: party slot VAR_0x8004 (a Furfrou) given trim VAR_0x8005 (sFurfrouTrims). VAR_RESULT: FALSE
// if it already has that trim
void ExoticSafari_TrimFurfrou(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004 % PARTY_SIZE];
    u16 species = sFurfrouTrims[gSpecialVar_0x8005 % ARRAY_COUNT(sFurfrouTrims)];
    gSpecialVar_Result = GetMonData(mon, MON_DATA_SPECIES) != species;
    if (!gSpecialVar_Result)
        return;
    SetMonData(mon, MON_DATA_SPECIES, &species);
    CalculateMonStats(mon);
}

// Special (one of Unova's keepsakes picked up): VAR_0x8004 = which (sUnovaKeys). STR_VAR_1 = its name
void ExoticSafari_TakeKey(void)
{
    const struct ExoticSafariKey *key = &sUnovaKeys[gSpecialVar_0x8004 % ARRAY_COUNT(sUnovaKeys)];
    FlagSet(key->flag);
    StringCopy(gStringVar1, key->name);
}

// Special: VAR_RESULT = whether the legendary VAR_0x8004 (a species) has been caught
void ExoticSafari_LegendCaught(void)
{
    u16 flag = CaughtFlag(gSpecialVar_0x8004);
    gSpecialVar_Result = flag != 0 && FlagGet(flag);
}
