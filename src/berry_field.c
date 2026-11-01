// Pokémon Canvas: the berry field in Pallet Town, and the Tera berries.
//
// Two rows of plots below the sea, east of the player's house (tools/gym_hack/build_pallet_town.py
// makes them; include/constants/berry_field.h). Any berry can be planted in an empty plot; it grows
// by the player's steps (BERRY_STEPS_PER_STAGE a stage: seed, sprout, young, flowering, ripe) and
// a ripe plant waits until it's picked: 2-5 berries, and the plot is bare again. Each stage is a
// metatile; flowers and berries come in the berry's own color (gBerries[].info.color).
//
// The 18 type-resist berries are the Tera berries: ARCHER charges TERA_BERRY_PRICE of a type's
// berry to change a Pokémon's Tera type to that type (src/gym_tera.c); a wild Pokémon holds its
// primary type's berry WILD_TERA_BERRY_PERCENT of the time; and each is found once in Kanto
// (FLAG_BERRY_FIND_*, data/scripts/berry_field.inc).
//
// The plots are saved after the tournament in sector 31 (src/save.c), with their own checksum.
#include "global.h"
#include "berry_field.h"
#include "berry.h"
#include "event_data.h"
#include "field_camera.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "item.h"
#include "item_menu.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
#include "constants/items.h"
#include "constants/map_groups.h"

EWRAM_DATA struct BerryFieldSave gBerryFieldSave = {0};

enum PlotStage
{
    PLOT_SOIL,
    PLOT_SEED,
    PLOT_SPROUT,
    PLOT_YOUNG,
    PLOT_FLOWERING,
    PLOT_RIPE,
};

static const u16 sTeraBerries[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = ITEM_CHILAN_BERRY,
    [TYPE_FIGHTING] = ITEM_CHOPLE_BERRY,
    [TYPE_FLYING]   = ITEM_COBA_BERRY,
    [TYPE_POISON]   = ITEM_KEBIA_BERRY,
    [TYPE_GROUND]   = ITEM_SHUCA_BERRY,
    [TYPE_ROCK]     = ITEM_CHARTI_BERRY,
    [TYPE_BUG]      = ITEM_TANGA_BERRY,
    [TYPE_GHOST]    = ITEM_KASIB_BERRY,
    [TYPE_STEEL]    = ITEM_BABIRI_BERRY,
    [TYPE_FIRE]     = ITEM_OCCA_BERRY,
    [TYPE_WATER]    = ITEM_PASSHO_BERRY,
    [TYPE_GRASS]    = ITEM_RINDO_BERRY,
    [TYPE_ELECTRIC] = ITEM_WACAN_BERRY,
    [TYPE_PSYCHIC]  = ITEM_PAYAPA_BERRY,
    [TYPE_ICE]      = ITEM_YACHE_BERRY,
    [TYPE_DRAGON]   = ITEM_HABAN_BERRY,
    [TYPE_DARK]     = ITEM_COLBUR_BERRY,
    [TYPE_FAIRY]    = ITEM_ROSELI_BERRY,
};

enum Item GetTeraBerry(enum Type type)
{
    return type < NUMBER_OF_MON_TYPES ? sTeraBerries[type] : ITEM_NONE;
}

// (src/pokemon.c, SetWildMonHeldItem: before the species' own held items)
bool32 TryGiveWildMonTeraBerry(struct Pokemon *mon)
{
    enum Item berry = GetTeraBerry(GetSpeciesType(GetMonData(mon, MON_DATA_SPECIES), 0));

    if (berry == ITEM_NONE || Random() % 100 >= WILD_TERA_BERRY_PERCENT)
        return FALSE;
    SetMonData(mon, MON_DATA_HELD_ITEM, &berry);
    return TRUE;
}

// ---------------------------------------------------------------------------
// The save
// ---------------------------------------------------------------------------
static u16 Checksum(void)
{
    u32 i, sum = 0;
    const u8 *p = (const u8 *)gBerryFieldSave.plots;
    for (i = 0; i < sizeof(gBerryFieldSave.plots); i++)
        sum = sum * 31 + p[i];
    return sum ^ (sum >> 16);
}

void BerryField_Clear(void)
{
    memset(&gBerryFieldSave, 0, sizeof(gBerryFieldSave));
    gBerryFieldSave.magic = BERRY_FIELD_MAGIC;
    gBerryFieldSave.checksum = Checksum();
}

void BerryField_SealSave(void)
{
    gBerryFieldSave.magic = BERRY_FIELD_MAGIC;
    gBerryFieldSave.checksum = Checksum();
}

// After loading (an older save has nothing there: bare soil)
void BerryField_Validate(void)
{
    u32 i;
    if (gBerryFieldSave.magic != BERRY_FIELD_MAGIC || gBerryFieldSave.checksum != Checksum())
    {
        BerryField_Clear();
        return;
    }
    for (i = 0; i < BERRY_FIELD_PLOTS; i++)
    {
        struct BerryPlot *plot = &gBerryFieldSave.plots[i];
        if (plot->item != ITEM_NONE && (plot->item < FIRST_BERRY_INDEX || plot->item > LAST_BERRY_INDEX))
            plot->item = ITEM_NONE;
        if (plot->item == ITEM_NONE)
            plot->steps = 0;
    }
}

// ---------------------------------------------------------------------------
// The plots
// ---------------------------------------------------------------------------
static void PlotCoords(u32 index, s32 *x, s32 *y)
{
    *x = BERRY_FIELD_X0 + index % BERRY_FIELD_COLUMNS;
    *y = index < BERRY_FIELD_COLUMNS ? BERRY_FIELD_ROW_0_Y : BERRY_FIELD_ROW_1_Y;
}

static s32 PlotAt(s32 x, s32 y)
{
    s32 column = x - BERRY_FIELD_X0;
    if (column < 0 || column >= BERRY_FIELD_COLUMNS)
        return -1;
    if (y == BERRY_FIELD_ROW_0_Y)
        return column;
    if (y == BERRY_FIELD_ROW_1_Y)
        return BERRY_FIELD_COLUMNS + column;
    return -1;
}

static enum PlotStage Stage(const struct BerryPlot *plot)
{
    if (plot->item == ITEM_NONE)
        return PLOT_SOIL;
    return PLOT_SEED + min(plot->steps / BERRY_STEPS_PER_STAGE, (u32)(PLOT_RIPE - PLOT_SEED));
}

static u32 PlotMetatile(const struct BerryPlot *plot)
{
    u32 color = plot->item ? GetBerryInfo(ItemIdToBerryType(plot->item))->color : 0;
    if (color > BERRY_COLOR_PINK)
        color = BERRY_COLOR_RED;
    switch (Stage(plot))
    {
    case PLOT_SOIL:      return METATILE_BERRY_SOIL;
    case PLOT_SEED:      return METATILE_BERRY_SEED;
    case PLOT_SPROUT:    return METATILE_BERRY_SPROUT;
    case PLOT_YOUNG:     return METATILE_BERRY_YOUNG;
    case PLOT_FLOWERING: return METATILE_BERRY_FLOWER_FIRST + color * METATILE_BERRY_STEP;
    default:             return METATILE_BERRY_RIPE_FIRST + color * METATILE_BERRY_STEP;
    }
}

static bool32 InPalletTown(void)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_PALLET_TOWN)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_PALLET_TOWN);
}

static void DrawPlot(u32 index, bool32 redraw)
{
    s32 x, y;
    PlotCoords(index, &x, &y);
    MapGridSetMetatileIdAt(x + MAP_OFFSET, y + MAP_OFFSET, PlotMetatile(&gBerryFieldSave.plots[index]) | MAPGRID_COLLISION_MASK);
    if (redraw)
        CurrentMapDrawMetatileAt(x + MAP_OFFSET, y + MAP_OFFSET);
}

// Special (Pallet Town's ON_LOAD)
void BerryField_DrawPlots(void)
{
    u32 i;
    for (i = 0; i < BERRY_FIELD_PLOTS; i++)
        DrawPlot(i, FALSE);
}

// Every step (src/field_control_avatar.c): the plants grow; in Pallet Town, as you watch
void BerryField_OnStep(void)
{
    u32 i;
    bool32 here = InPalletTown();
    for (i = 0; i < BERRY_FIELD_PLOTS; i++)
    {
        struct BerryPlot *plot = &gBerryFieldSave.plots[i];
        enum PlotStage before;
        if (plot->item == ITEM_NONE || Stage(plot) == PLOT_RIPE)
            continue;
        before = Stage(plot);
        plot->steps++;
        if (here && Stage(plot) != before)
            DrawPlot(i, TRUE);
    }
}

// ---------------------------------------------------------------------------
// The scripts (data/scripts/berry_field.inc)
// ---------------------------------------------------------------------------

// The plot the player faces: VAR_0x8004 = it (BERRY_FIELD_PLOTS: none), VAR_RESULT = 0 bare,
// 1 growing (STR_VAR_2 = about how many steps to go), 2 ripe; STR_VAR_1 = its berry's name
void BerryField_GetFacingPlot(void)
{
    s16 x, y;
    s32 index;
    struct BerryPlot *plot;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    index = PlotAt(x - MAP_OFFSET, y - MAP_OFFSET);
    gSpecialVar_0x8004 = index < 0 ? BERRY_FIELD_PLOTS : index;
    gSpecialVar_Result = 0;
    if (index < 0)
        return;
    plot = &gBerryFieldSave.plots[index];
    if (plot->item == ITEM_NONE)
        return;
    CopyItemName(plot->item, gStringVar1);
    if (Stage(plot) == PLOT_RIPE)
    {
        gSpecialVar_Result = 2;
        return;
    }
    ConvertIntToDecimalStringN(gStringVar2, (PLOT_RIPE - PLOT_SEED) * BERRY_STEPS_PER_STAGE - plot->steps,
                               STR_CONV_MODE_LEFT_ALIGN, 4);
    gSpecialVar_Result = 1;
}

// VAR_0x8004 = the plot, VAR_ITEM_ID = the berry (from the bag): planted; STR_VAR_1 = its name
void BerryField_Plant(void)
{
    struct BerryPlot *plot;

    if (gSpecialVar_0x8004 >= BERRY_FIELD_PLOTS || gSpecialVar_ItemId < FIRST_BERRY_INDEX || gSpecialVar_ItemId > LAST_BERRY_INDEX)
        return;
    plot = &gBerryFieldSave.plots[gSpecialVar_0x8004];
    if (!RemoveBagItem(gSpecialVar_ItemId, 1))
        return;
    plot->item = gSpecialVar_ItemId;
    plot->steps = 0;
    CopyItemName(plot->item, gStringVar1);
    DrawPlot(gSpecialVar_0x8004, TRUE);
}

// VAR_0x8004 = a ripe plot: its berries go to the bag (STR_VAR_1 = the name, STR_VAR_2 = how many;
// VAR_RESULT = FALSE if the bag had no room), and the plot is bare
void BerryField_Harvest(void)
{
    struct BerryPlot *plot;
    const struct BerryInfo *info;
    u32 lo, hi, count;

    gSpecialVar_Result = FALSE;
    if (gSpecialVar_0x8004 >= BERRY_FIELD_PLOTS)
        return;
    plot = &gBerryFieldSave.plots[gSpecialVar_0x8004];
    if (plot->item == ITEM_NONE || Stage(plot) != PLOT_RIPE)
        return;
    info = GetBerryInfo(ItemIdToBerryType(plot->item));
    lo = max(2u, min((u32)info->minYield, 5u));
    hi = max(lo, min((u32)info->maxYield, 5u));
    count = lo + Random() % (hi - lo + 1);
    CopyItemNameHandlePlural(plot->item, gStringVar1, count);
    ConvertIntToDecimalStringN(gStringVar2, count, STR_CONV_MODE_LEFT_ALIGN, 1);
    if (!AddBagItem(plot->item, count))
        return;
    plot->item = ITEM_NONE;
    plot->steps = 0;
    gSpecialVar_Result = TRUE;
    DrawPlot(gSpecialVar_0x8004, TRUE);
}

// VAR_0x8004 = a growing plot: the plant's pulled up, its berry back in the bag (STR_VAR_1)
void BerryField_PullUp(void)
{
    struct BerryPlot *plot;

    if (gSpecialVar_0x8004 >= BERRY_FIELD_PLOTS)
        return;
    plot = &gBerryFieldSave.plots[gSpecialVar_0x8004];
    if (plot->item == ITEM_NONE)
        return;
    CopyItemName(plot->item, gStringVar1);
    AddBagItem(plot->item, 1);
    plot->item = ITEM_NONE;
    plot->steps = 0;
    DrawPlot(gSpecialVar_0x8004, TRUE);
}
