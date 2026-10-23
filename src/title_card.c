// Pokémon Canvas: a trainer's title card, shown before a special battle (the Gym Leader
// Convention's, from its script: Convention_ShowTitleCard). A full screen: the event's name at the
// top, the trainer's picture sliding in, their name large across a band in the hall's color, and
// who they are in the Pokémon world (their title, region and games) under it. A or B (or a few
// seconds) moves on; the script then starts the battle.
#include "global.h"
#include "title_card.h"
#include "bg.h"
#include "fpmath.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "trainer_pokemon_sprites.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define CARD_FRAMES     300     // (on screen this long unless A or B moves on sooner)
#define PIC_X           178
#define PIC_Y           68
#define PIC_SLIDE_FROM  320
#define BAND_TOP        11      // (tiles: the name band, rows 11-14)
#define BAND_ROWS       4

struct TitleCard
{
    u16 trainerPic;
    u16 color;
    const u8 *header;
    const u8 *name;
    const u8 *title;
    const u8 *region;
    const u8 *games;
    u8 spriteId;
    u8 matrixNum;
    u8 *tilemap;
    u32 *tiles;     // (kept until the card closes: the copy to VRAM is queued)
};

static EWRAM_DATA struct TitleCard *sCard = NULL;

enum { WIN_HEADER, WIN_NAME_SMALL, WIN_NAME, WIN_TITLE, WIN_REGION, WIN_COUNT };

static const struct BgTemplate sBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 1, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
    {.bg = 1, .charBaseIndex = 0, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_HEADER]     = {.bg = 0, .tilemapLeft = 1,  .tilemapTop = 1,  .width = 28, .height = 2, .paletteNum = 15, .baseBlock = 1},
    [WIN_NAME_SMALL] = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 18, .width = 14, .height = 2, .paletteNum = 15, .baseBlock = 57},
    [WIN_NAME]       = {.bg = 0, .tilemapLeft = 1,  .tilemapTop = BAND_TOP, .width = 28, .height = BAND_ROWS, .paletteNum = 15, .baseBlock = 85},
    [WIN_TITLE]      = {.bg = 0, .tilemapLeft = 1,  .tilemapTop = 15, .width = 28, .height = 2, .paletteNum = 15, .baseBlock = 197},
    [WIN_REGION]     = {.bg = 0, .tilemapLeft = 1,  .tilemapTop = 17, .width = 28, .height = 2, .paletteNum = 15, .baseBlock = 253},
    DUMMY_WIN_TEMPLATE,
};

// Text palette (BG palette 15): 1 white, 2 shadow, 3 soft gray, 4 the band's dark shade
enum { TXT_CLEAR, TXT_WHITE, TXT_SHADOW, TXT_GRAY, TXT_DARK };
// Background (BG palette 0): 1 the night ground, 2 its stripes, 3 the band, 4 the band's edge
enum { BG_GROUND = 1, BG_STRIPE, BG_BAND, BG_EDGE };

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

// A color, darker
static u16 Shade(u16 c, u32 num, u32 den)
{
    u32 r = (c & 0x1F) * num / den, g = ((c >> 5) & 0x1F) * num / den, b = ((c >> 10) & 0x1F) * num / den;
    return RGB(r, g, b);
}

// The background's four tiles: plain ground, ground with a diagonal stripe, the band, its edge
static void LoadBackground(void)
{
    u32 *tiles = sCard->tiles = AllocZeroed(4 * 32), y, x;
    u8 *px = (u8 *)tiles;
    u16 pal[16] = {0};
    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x += 2)
        {
            u8 lo, hi;
            px[0 * 32 + y * 4 + x / 2] = BG_GROUND | (BG_GROUND << 4);
            lo = ((x + y) % 8 < 2) ? BG_STRIPE : BG_GROUND;
            hi = ((x + 1 + y) % 8 < 2) ? BG_STRIPE : BG_GROUND;
            px[1 * 32 + y * 4 + x / 2] = lo | (hi << 4);
            px[2 * 32 + y * 4 + x / 2] = BG_BAND | (BG_BAND << 4);
            px[3 * 32 + y * 4 + x / 2] = (y < 2 ? BG_EDGE : BG_BAND) | ((y < 2 ? BG_EDGE : BG_BAND) << 4);
        }
    }
    LoadBgTiles(1, tiles, 4 * 32, 0);
    pal[BG_GROUND] = RGB(3, 4, 8);
    pal[BG_STRIPE] = RGB(5, 6, 11);
    pal[BG_BAND] = sCard->color;
    pal[BG_EDGE] = Shade(sCard->color, 3, 5);
    LoadPalette(pal, BG_PLTT_ID(0), sizeof(pal));
    // (the text: on the band, its dark shade is the shadow)
    pal[0] = 0;
    pal[TXT_WHITE] = RGB_WHITE;
    pal[TXT_SHADOW] = RGB(2, 2, 5);
    pal[TXT_GRAY] = RGB(20, 21, 25);
    pal[TXT_DARK] = Shade(sCard->color, 2, 5);
    LoadPalette(pal, BG_PLTT_ID(15), sizeof(pal));

    sCard->tilemap = AllocZeroed(BG_SCREEN_SIZE);
    SetBgTilemapBuffer(1, sCard->tilemap);
    for (y = 0; y < 20; y++)
        FillBgTilemapBufferRect_Palette0(1, (y >= 3 && y < 10) || y >= 15 ? 1 : 0, 0, y, 32, 1);
    FillBgTilemapBufferRect_Palette0(1, 3, 0, BAND_TOP, 32, 1);
    FillBgTilemapBufferRect_Palette0(1, 2, 0, BAND_TOP + 1, 32, BAND_ROWS - 1);
    CopyBgTilemapBufferToVram(1);
}

// One pixel of a 4bpp window's buffer
static u32 GetWinPixel(u32 windowId, u32 x, u32 y)
{
    const u8 *data = gWindows[windowId].tileData;
    u32 w = gWindows[windowId].window.width;
    u32 offset = ((y / 8) * w + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2;
    return (x & 1) ? data[offset] >> 4 : data[offset] & 0xF;
}

static void SetWinPixel(u32 windowId, u32 x, u32 y, u32 color)
{
    u8 *data = gWindows[windowId].tileData;
    u32 w = gWindows[windowId].window.width;
    u32 offset = ((y / 8) * w + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2;
    data[offset] = (x & 1) ? (data[offset] & 0x0F) | (color << 4) : (data[offset] & 0xF0) | color;
}

// The name, twice the size: printed small (off screen), then copied into the band's window with
// every pixel doubled
static void PrintBigName(void)
{
    static const u8 colors[3] = {TXT_CLEAR, TXT_WHITE, TXT_DARK};
    u32 x, y, w = gWindows[WIN_NAME_SMALL].window.width * 8, h = 16;
    u32 width = GetStringWidth(FONT_NORMAL, sCard->name, 0);
    u32 left = 4;
    FillWindowPixelBuffer(WIN_NAME_SMALL, PIXEL_FILL(0));
    AddTextPrinterParameterized4(WIN_NAME_SMALL, FONT_NORMAL, 0, 0, 0, 0, colors, TEXT_SKIP_DRAW, sCard->name);
    FillWindowPixelBuffer(WIN_NAME, PIXEL_FILL(0));
    if (width * 2 + left > gWindows[WIN_NAME].window.width * 8)
        left = 0;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w && x * 2 + left + 1 < gWindows[WIN_NAME].window.width * 8; x++)
        {
            u32 c = GetWinPixel(WIN_NAME_SMALL, x, y);
            if (c == 0)
                continue;
            SetWinPixel(WIN_NAME, left + x * 2, y * 2, c);
            SetWinPixel(WIN_NAME, left + x * 2 + 1, y * 2, c);
            SetWinPixel(WIN_NAME, left + x * 2, y * 2 + 1, c);
            SetWinPixel(WIN_NAME, left + x * 2 + 1, y * 2 + 1, c);
        }
    }
    PutWindowTilemap(WIN_NAME);
    CopyWindowToVram(WIN_NAME, COPYWIN_FULL);
}

static void PrintText(void)
{
    static const u8 header[3] = {TXT_CLEAR, TXT_GRAY, TXT_SHADOW};
    static const u8 white[3] = {TXT_CLEAR, TXT_WHITE, TXT_SHADOW};
    static const u8 gray[3] = {TXT_CLEAR, TXT_GRAY, TXT_SHADOW};
    u8 *s = Alloc(64);
    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(0));
    AddTextPrinterParameterized4(WIN_HEADER, FONT_SMALL, 0, 2, 1, 0, header, TEXT_SKIP_DRAW, sCard->header);
    FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(0));
    AddTextPrinterParameterized4(WIN_TITLE, FONT_NORMAL, 4, 1, 0, 0, white, TEXT_SKIP_DRAW, sCard->title);
    StringCopy(s, sCard->region);
    StringAppend(s, COMPOUND_STRING("  ·  "));
    StringAppend(s, sCard->games);
    FillWindowPixelBuffer(WIN_REGION, PIXEL_FILL(0));
    AddTextPrinterParameterized4(WIN_REGION, FONT_SMALL, 4, 2, 0, 0, gray, TEXT_SKIP_DRAW, s);
    Free(s);
    PutWindowTilemap(WIN_HEADER);
    PutWindowTilemap(WIN_TITLE);
    PutWindowTilemap(WIN_REGION);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
    CopyWindowToVram(WIN_TITLE, COPYWIN_FULL);
    CopyWindowToVram(WIN_REGION, COPYWIN_FULL);
    PrintBigName();
}

#define tState  data[0]
#define tTimer  data[1]

static void Task_TitleCard(u8 taskId)
{
    struct Sprite *pic = &gSprites[sCard->spriteId];
    s16 *data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:     // (fading in, the picture sliding in from the right)
        if (pic->x > PIC_X)
            pic->x -= (pic->x - PIC_X + 3) / 4;
        if (!gPaletteFade.active && pic->x <= PIC_X)
            tState = 1;
        break;
    case 1:
        if (++tTimer >= CARD_FRAMES || JOY_NEW(A_BUTTON | B_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            tState = 2;
        }
        break;
    case 2:
        if (gPaletteFade.active)
            break;
        FreeOamMatrix(sCard->matrixNum);
        FreeAndDestroyTrainerPicSprite(sCard->spriteId);
        FreeAllWindowBuffers();
        Free(sCard->tilemap);
        Free(sCard->tiles);
        FREE_AND_SET_NULL(sCard);
        DestroyTask(taskId);
        SetMainCallback2(CB2_ReturnToFieldContinueScript);
        break;
    }
}

static void CB2_InitTitleCard(void)
{
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    ResetAllBgsCoordinates();
    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    LoadBackground();
    PrintText();

    // The picture: twice its size (an affine sprite), sliding in
    sCard->spriteId = CreateTrainerPicSprite(sCard->trainerPic, TRUE, PIC_SLIDE_FROM, PIC_Y, 0, TAG_NONE);
    sCard->matrixNum = AllocOamMatrix();
    gSprites[sCard->spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    gSprites[sCard->spriteId].oam.matrixNum = sCard->matrixNum;
    gSprites[sCard->spriteId].oam.priority = 1;
    gSprites[sCard->spriteId].affineAnimPaused = TRUE;
    CalcCenterToCornerVec(&gSprites[sCard->spriteId], gSprites[sCard->spriteId].oam.shape,
                          gSprites[sCard->spriteId].oam.size, ST_OAM_AFFINE_DOUBLE);
    SetOamMatrix(sCard->matrixNum, Q_8_8(0.625), 0, 0, Q_8_8(0.625));   // (1.6 times its size)

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(1);
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB);
    SetMainCallback2(MainCB2);
    CreateTask(Task_TitleCard, 0);
}

// Shows a title card (the screen should already be faded to black: the script's fadescreen); the
// script waitstates until it's done. The strings must last until then (ROM or saved buffers).
void TitleCard_Show(u16 trainerPic, u16 color, const u8 *header, const u8 *name, const u8 *title,
                    const u8 *region, const u8 *games)
{
    sCard = AllocZeroed(sizeof(*sCard));
    sCard->trainerPic = trainerPic;
    sCard->color = color;
    sCard->header = header;
    sCard->name = name;
    sCard->title = title;
    sCard->region = region;
    sCard->games = games;
    PlaySE(SE_M_SWAGGER);
    SetMainCallback2(CB2_InitTitleCard);
}
