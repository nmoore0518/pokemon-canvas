#include "global.h"
#include "gym_hack.h"
#include "sprite.h"
#include "constants/vars.h"
#include "bg.h"
#include "event_data.h"
#include "field_effect.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "region_map.h"
#include "sound.h"
#include "strings.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

/*
 *  This is the type of map shown when interacting with the metatiles for
 *  a wall-mounted Region Map (on the wall of the Pokemon Centers near the PC)
 *  It does not zoom, and pressing A or B closes the map
 *
 *  For the region map in the pokenav, see pokenav_region_map.c
 *  For the region map in the pokedex, see pokdex_area_screen.c/pokedex_area_region_map.c
 *  For the fly map, and utility functions all of the maps use, see region_map.c
 */

enum {
    WIN_MAPSEC_NAME,
    WIN_TITLE,
    WIN_TYPE_FINDER, // Gym Trainer hack
};

enum {
    TAG_PLAYER_ICON,
    TAG_CURSOR,
    TAG_TYPE_FINDER, // Gym Trainer hack
};

// Gym Trainer hack: type finder markers
#define MAX_TYPE_FINDER_MARKERS 64
#define MAPCURSOR_X_MIN 1 // same as region_map.c
#define MAPCURSOR_Y_MIN 2

static EWRAM_DATA struct {
    MainCallback callback;
    u32 unused;
    struct RegionMap regionMap;
    u16 state;
    bool8 typeFinderOn;
    u8 numTypeFinderMarkers;
    u8 typeFinderSpriteIds[MAX_TYPE_FINDER_MARKERS];
} *sFieldRegionMapHandler = NULL;

static void MCB2_InitRegionMapRegisters(void);
static void VBCB_FieldUpdateRegionMap(void);
static void MCB2_FieldUpdateRegionMap(void);
static void FieldUpdateRegionMap(void);
static void PrintRegionMapSecName();
static void PrintTitleWindowText();
static void PrintTypeFinderHint(void);
static void ToggleTypeFinder(void);
static void DestroyTypeFinderMarkers(void);

static const u16 sTypeFinderMarker_Pal[] = INCGFX_U16("graphics/gym_hack/type_finder_marker.png", ".gbapal");
static const u8 sTypeFinderMarker_Gfx[] = INCGFX_U8("graphics/gym_hack/type_finder_marker.png", ".4bpp");

static const struct OamData sTypeFinderMarkerOam =
{
    .shape = SPRITE_SHAPE(8x8),
    .size = SPRITE_SIZE(8x8),
    .priority = 1,
};

static const struct SpriteTemplate sTypeFinderMarkerTemplate =
{
    .tileTag = TAG_TYPE_FINDER,
    .paletteTag = TAG_TYPE_FINDER,
    .oam = &sTypeFinderMarkerOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct BgTemplate sFieldRegionMapBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }, {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 2,
        .paletteMode = 1,
        .priority = 2,
        .baseTile = 0
    }
};

static const struct WindowTemplate sFieldRegionMapWindowTemplates[] =
{
    [WIN_MAPSEC_NAME] = {
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 17,
        .width = 12,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1
    },
    [WIN_TITLE] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 1,
        .width = 7,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 25
    },
    [WIN_TYPE_FINDER] = {
        .bg = 0,
        .tilemapLeft = 1,   // top left: at the bottom it covered Cinnabar Island
        .tilemapTop = 1,
        .width = 14,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 39
    },
    DUMMY_WIN_TEMPLATE
};

void FieldInitRegionMap(MainCallback callback)
{
    SetVBlankCallback(NULL);
    sFieldRegionMapHandler = Alloc(sizeof(*sFieldRegionMapHandler));
    sFieldRegionMapHandler->state = 0;
    sFieldRegionMapHandler->callback = callback;
    sFieldRegionMapHandler->typeFinderOn = FALSE;
    sFieldRegionMapHandler->numTypeFinderMarkers = 0;
    SetMainCallback2(MCB2_InitRegionMapRegisters);
}

static void MCB2_InitRegionMapRegisters(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(1, sFieldRegionMapBgTemplates, ARRAY_COUNT(sFieldRegionMapBgTemplates));
    InitWindows(sFieldRegionMapWindowTemplates);
    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(0, 0x27, BG_PLTT_ID(13));
    ClearScheduledBgCopiesToVram();
    SetMainCallback2(MCB2_FieldUpdateRegionMap);
    SetVBlankCallback(VBCB_FieldUpdateRegionMap);
}

static void VBCB_FieldUpdateRegionMap(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MCB2_FieldUpdateRegionMap(void)
{
    FieldUpdateRegionMap();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    DoScheduledBgTilemapCopiesToVram();
}

static void FieldUpdateRegionMap(void)
{
    switch (sFieldRegionMapHandler->state)
    {
    case 0:
        InitRegionMap(&sFieldRegionMapHandler->regionMap, FALSE);
        CreateRegionMapPlayerIcon(TAG_PLAYER_ICON, TAG_PLAYER_ICON);
        CreateRegionMapCursor(TAG_CURSOR, TAG_CURSOR);
        sFieldRegionMapHandler->state++;
        break;
    case 1:
        DrawStdFrameWithCustomTileAndPalette(WIN_TITLE, FALSE, 0x27, 0xd);
        FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(1));
        PrintTitleWindowText();
        ScheduleBgCopyTilemapToVram(0);
        DrawStdFrameWithCustomTileAndPalette(WIN_MAPSEC_NAME, FALSE, 0x27, 0xd);
        PrintRegionMapSecName();
        if (VarGet(VAR_PLAYER_TYPE) != TYPE_NONE)
        {
            DrawStdFrameWithCustomTileAndPalette(WIN_TYPE_FINDER, FALSE, 0x27, 0xd);
            PrintTypeFinderHint();
        }
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        sFieldRegionMapHandler->state++;
        break;
    case 2:
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        ShowBg(0);
        ShowBg(2);
        sFieldRegionMapHandler->state++;
        break;
    case 3:
        if (!gPaletteFade.active)
        {
            sFieldRegionMapHandler->state++;
        }
        break;
    case 4:
        // Gym Trainer hack: SELECT shows/hides the type finder markers
        if (JOY_NEW(SELECT_BUTTON) && VarGet(VAR_PLAYER_TYPE) != TYPE_NONE)
        {
            PlaySE(SE_SELECT);
            ToggleTypeFinder();
            break;
        }
        switch (DoRegionMapInputCallback())
        {
        case MAP_INPUT_MOVE_END:
                PrintRegionMapSecName();
                PrintTitleWindowText();
                break;
        case MAP_INPUT_A_BUTTON:
        case MAP_INPUT_B_BUTTON:
                sFieldRegionMapHandler->state++;
                break;
        case MAP_INPUT_R_BUTTON:
                if (sFieldRegionMapHandler->regionMap.mapSecType == MAPSECTYPE_CITY_CANFLY
                    && FlagGet(OW_FLAG_POKE_RIDER) && Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
                {
                    PlaySE(SE_SELECT);
                    SetFlyDestination(&sFieldRegionMapHandler->regionMap);
                    gSkipShowMonAnim = TRUE;
                    ReturnToFieldFromFlyMapSelect();
                }
        }
        break;
    case 5:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sFieldRegionMapHandler->state++;
        break;
    case 6:
        if (!gPaletteFade.active)
        {
            DestroyTypeFinderMarkers();
            FreeRegionMapIconResources();
            SetMainCallback2(sFieldRegionMapHandler->callback);
            TRY_FREE_AND_SET_NULL(sFieldRegionMapHandler);
            FreeAllWindowBuffers();
        }
        break;
    }
}

static void PrintRegionMapSecName(void)
{
    if (sFieldRegionMapHandler->regionMap.mapSecType != MAPSECTYPE_NONE)
    {
        FillWindowPixelBuffer(WIN_MAPSEC_NAME, PIXEL_FILL(1));
        AddTextPrinterParameterized(WIN_MAPSEC_NAME, FONT_NORMAL, sFieldRegionMapHandler->regionMap.mapSecName, 0, 1, 0, NULL);
        ScheduleBgCopyTilemapToVram(WIN_MAPSEC_NAME);
    }
    else
    {
        FillWindowPixelBuffer(WIN_MAPSEC_NAME, PIXEL_FILL(1));
        CopyWindowToVram(WIN_MAPSEC_NAME, COPYWIN_FULL);
    }
}

static void PrintTitleWindowText(void)
{
    static const u8 FlyPromptText[] = _("{R_BUTTON} FLY");
    const u8 *region;
    if (IS_FRLG)
        region = gText_Kanto;
    else
        region = gText_Hoenn;
    u32 hoennOffset = GetStringCenterAlignXOffset(FONT_NORMAL, region, 0x38);
    u32 flyOffset = GetStringCenterAlignXOffset(FONT_NORMAL, FlyPromptText, 0x38);

    FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(1));

    if (sFieldRegionMapHandler->regionMap.mapSecType == MAPSECTYPE_CITY_CANFLY
        && FlagGet(OW_FLAG_POKE_RIDER) && Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
    {
        AddTextPrinterParameterized(WIN_TITLE, FONT_NORMAL, FlyPromptText, flyOffset, 1, 0, NULL);
        ScheduleBgCopyTilemapToVram(WIN_TITLE);
    }
    else
    {
        AddTextPrinterParameterized(WIN_TITLE, FONT_NORMAL, region, hoennOffset, 1, 0, NULL);
        CopyWindowToVram(WIN_TITLE, COPYWIN_FULL);
    }
}

// ---------------------------------------------------------------------------
// Gym Trainer hack: gym type finder. Marks every area on this map that has a
// wild Pokemon whose primary type is the player's type, without saying which.
// ---------------------------------------------------------------------------

static void PrintTypeFinderHint(void)
{
    static const u8 sText_Show[] = _("{SELECT_BUTTON} GYM TYPE FINDER");
    static const u8 sText_Hide[] = _("{SELECT_BUTTON} HIDE FINDER");

    FillWindowPixelBuffer(WIN_TYPE_FINDER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TYPE_FINDER, FONT_NORMAL,
                                sFieldRegionMapHandler->typeFinderOn ? sText_Hide : sText_Show, 0, 1, 0, NULL);
    CopyWindowToVram(WIN_TYPE_FINDER, COPYWIN_FULL);
}

static void CreateTypeFinderMarkers(void)
{
    u16 mapSecs[MAX_TYPE_FINDER_MARKERS];
    u32 i, n;
    struct SpriteSheet sheet = {sTypeFinderMarker_Gfx, sizeof(sTypeFinderMarker_Gfx), TAG_TYPE_FINDER};
    struct SpritePalette palette = {sTypeFinderMarker_Pal, TAG_TYPE_FINDER};

    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&palette);
    n = GymHack_GetTypeFinderMapSecs(mapSecs, MAX_TYPE_FINDER_MARKERS, GetRegionMapType(gMapHeader.regionMapSectionId));
    sFieldRegionMapHandler->numTypeFinderMarkers = 0;
    for (i = 0; i < n; i++)
    {
        const struct RegionMapLocation *loc = &gRegionMapEntries[mapSecs[i]];
        // Middle cell of the area, in the same 8px grid the player icon uses
        s16 x = (loc->x + loc->width / 2 + MAPCURSOR_X_MIN) * 8 + 4;
        s16 y = (loc->y + loc->height / 2 + MAPCURSOR_Y_MIN) * 8 + 4;
        u8 spriteId = CreateSprite(&sTypeFinderMarkerTemplate, x, y, 2);
        if (spriteId != MAX_SPRITES)
            sFieldRegionMapHandler->typeFinderSpriteIds[sFieldRegionMapHandler->numTypeFinderMarkers++] = spriteId;
    }
}

static void DestroyTypeFinderMarkers(void)
{
    u32 i;
    for (i = 0; i < sFieldRegionMapHandler->numTypeFinderMarkers; i++)
        DestroySprite(&gSprites[sFieldRegionMapHandler->typeFinderSpriteIds[i]]);
    if (sFieldRegionMapHandler->numTypeFinderMarkers)
    {
        FreeSpriteTilesByTag(TAG_TYPE_FINDER);
        FreeSpritePaletteByTag(TAG_TYPE_FINDER);
    }
    sFieldRegionMapHandler->numTypeFinderMarkers = 0;
}

static void ToggleTypeFinder(void)
{
    sFieldRegionMapHandler->typeFinderOn = !sFieldRegionMapHandler->typeFinderOn;
    if (sFieldRegionMapHandler->typeFinderOn)
        CreateTypeFinderMarkers();
    else
        DestroyTypeFinderMarkers();
    PrintTypeFinderHint();
}
