// Pokémon Canvas: the tournament's board (src/tournament.c). 128 names don't fit on one screen, so
// the bracket is shown in nine pages on the Battle Dome's tree (16 names each, the left eight
// meeting the right eight in the middle): blocks A to H - each a sixteenth of the bracket, its
// first four rounds, down to the round of 16 - and the final 16, from there to the champion. L/R
// (or left/right) turns the page; A or B closes it. It opens on the page the player is on.
//
// A winner's line is lit (orange) as far as they've gone; the player's own path is blue and their
// name red; whoever's out is grayed. Once it's over, the last page names the champion.
#include "global.h"
#include "tournament.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

void DomeTree_DrawLine(u32 slot, u32 rounds, u16 glowTile);

#define PAGES           9       // (8 blocks, and the final 16)
#define PAGE_FINALS     8
#define PAL_WINNER      8       // (BG2's palettes: the lit lines' colors)
#define PAL_PLAYER      9
#define GLOW_WINNER     (1 | (PAL_WINNER << 12))
#define GLOW_PLAYER     (1 | (PAL_PLAYER << 12))
#define NAME_W          64

enum { WIN_LEFT, WIN_RIGHT, WIN_TITLE, WIN_COUNT };

static const struct BgTemplate sBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 28, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
    {.bg = 1, .charBaseIndex = 1, .mapBaseIndex = 29, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0},
    {.bg = 2, .charBaseIndex = 2, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
};

// BG2's tiles: nothing, and solid (color 1)
static const u32 sGlowTiles[16] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111,
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_LEFT]  = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 3,  .width = 8,  .height = 16, .paletteNum = 15, .baseBlock = 16},
    [WIN_RIGHT] = {.bg = 0, .tilemapLeft = 22, .tilemapTop = 3,  .width = 8,  .height = 16, .paletteNum = 15, .baseBlock = 144},
    [WIN_TITLE] = {.bg = 0, .tilemapLeft = 8,  .tilemapTop = 1,  .width = 14, .height = 2,  .paletteNum = 15, .baseBlock = 272},
    DUMMY_WIN_TEMPLATE,
};

// Text (BG palette 15)
enum { TXT_CLEAR, TXT_WHITE, TXT_DARK, TXT_SHADOW, TXT_RED, TXT_RED_SHADOW, TXT_OUT, TXT_OUT_SHADOW, TXT_TITLE_SHADOW };
static const u16 sTextPal[16] =
{
    [TXT_CLEAR] = RGB(0, 0, 0),         [TXT_WHITE] = RGB(31, 31, 31),
    [TXT_DARK] = RGB(6, 6, 7),          [TXT_SHADOW] = RGB(20, 21, 20),
    [TXT_RED] = RGB(26, 4, 3),          [TXT_RED_SHADOW] = RGB(31, 18, 16),
    [TXT_OUT] = RGB(15, 16, 16),        [TXT_OUT_SHADOW] = RGB(24, 25, 24),
    [TXT_TITLE_SHADOW] = RGB(6, 12, 9),
};
static const u8 sColorName[3] = {TXT_CLEAR, TXT_DARK, TXT_SHADOW};
static const u8 sColorPlayer[3] = {TXT_CLEAR, TXT_RED, TXT_RED_SHADOW};
static const u8 sColorOut[3] = {TXT_CLEAR, TXT_OUT, TXT_OUT_SHADOW};
static const u8 sColorTitle[3] = {TXT_CLEAR, TXT_WHITE, TXT_TITLE_SHADOW};

struct Board
{
    u8 tree[TOURNEY_ROUNDS + 1][TOURNEY_SIZE];
    u16 tilemap[0x400];     // (the tree as it comes: each page starts from it)
    u16 bgTilemap[0x400];   // (BG1's buffer)
    u16 glowTilemap[0x400]; // (BG2's: the lit lines' colors)
    void (*callback)(void);
    u8 page;
};

static EWRAM_DATA struct Board *sBoard = NULL;

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

// A page's sixteen: who (TOURNEY_PENDING: not decided yet), how many of the page's four rounds
// they've won, and whether they're out
static void PageEntrants(u32 page, u8 *ids, u8 *wins, bool8 *out)
{
    u8 (*tree)[TOURNEY_SIZE] = sBoard->tree;
    u32 s, k, first = page == PAGE_FINALS ? 3 : 0;
    for (s = 0; s < 16; s++)
    {
        u32 at = page == PAGE_FINALS ? s : page * 16 + s;     // (their index in round `first`)
        u32 id = tree[first][at];
        ids[s] = id;
        wins[s] = 0;
        out[s] = FALSE;
        if (id == TOURNEY_PENDING)
            continue;
        for (k = 0; k < 4; k++)
        {
            u32 winner = tree[first + k + 1][at >> (k + 1)];
            if (winner == id)
            {
                wins[s]++;
                continue;
            }
            out[s] = winner != TOURNEY_PENDING;
            break;
        }
    }
}

static void PrintCentered(u32 windowId, u32 width, u32 y, const u8 *str, const u8 *colors)
{
    u32 font = GetFontIdToFit(str, FONT_SHORT, 0, width);
    s32 x = (width - GetStringWidth(font, str, 0)) / 2;
    AddTextPrinterParameterized4(windowId, font, x < 0 ? 0 : x, y, 0, 0, colors, TEXT_SKIP_DRAW, str);
}

static void DrawPage(void)
{
    u8 ids[16], wins[16];
    bool8 out[16];
    u8 *title = Alloc(48), *end;
    u32 s, page = sBoard->page;

    CopyToBgTilemapBuffer(1, sBoard->tilemap, sizeof(sBoard->tilemap), 0);
    memset(sBoard->glowTilemap, 0, sizeof(sBoard->glowTilemap));
    FillWindowPixelBuffer(WIN_LEFT, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_RIGHT, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(0));

    PageEntrants(page, ids, wins, out);
    for (s = 0; s < 16; s++)
    {
        const u8 *name = ids[s] == TOURNEY_PENDING ? COMPOUND_STRING("- - -") : Tourney_ShortName(ids[s]);
        const u8 *colors = ids[s] == TOURNEY_PLAYER ? sColorPlayer : out[s] ? sColorOut : sColorName;
        u32 win = s < 8 ? WIN_LEFT : WIN_RIGHT, font = GetFontIdToFit(name, FONT_SHORT, 0, NAME_W - 4);
        s32 x = s < 8 ? NAME_W - 3 - GetStringWidth(font, name, 0) : 3;
        AddTextPrinterParameterized4(win, font, x < 0 ? 0 : x, (s % 8) * 16 + 1, 0, 0, colors, TEXT_SKIP_DRAW, name);
    }
    // (the lines: everyone else's first, so the player's path is drawn over theirs)
    for (s = 0; s < 16; s++)
    {
        if (ids[s] != TOURNEY_PLAYER)
            DomeTree_DrawLine(s, wins[s], GLOW_WINNER);
    }
    for (s = 0; s < 16; s++)
    {
        if (ids[s] == TOURNEY_PLAYER)
            DomeTree_DrawLine(s, wins[s], GLOW_PLAYER);
    }

    end = StringCopy(title, COMPOUND_STRING("{L_BUTTON} "));
    if (page == PAGE_FINALS)
    {
        u32 champ = sBoard->tree[TOURNEY_ROUNDS][0];
        if (champ != TOURNEY_PENDING)
        {
            end = StringAppend(end, COMPOUND_STRING("CHAMPION: "));
            end = StringAppend(end, Tourney_ShortName(champ));
        }
        else
        {
            end = StringAppend(end, COMPOUND_STRING("THE FINAL 16"));
        }
    }
    else
    {
        end = StringAppend(end, COMPOUND_STRING("BLOCK "));
        *end++ = CHAR_A + page;
        *end = EOS;
    }
    StringAppend(end, COMPOUND_STRING(" {R_BUTTON}"));
    PrintCentered(WIN_TITLE, 112, 1, title, sColorTitle);
    Free(title);

    CopyWindowToVram(WIN_LEFT, COPYWIN_GFX);
    CopyWindowToVram(WIN_RIGHT, COPYWIN_GFX);
    CopyWindowToVram(WIN_TITLE, COPYWIN_GFX);
    CopyBgTilemapBufferToVram(1);
    CopyBgTilemapBufferToVram(2);
}

// The page the player's on: their block, or the final 16 once they've made it there
static u32 PlayerPage(void)
{
    u32 slot = Tourney_PlayerSlot((const u8 (*)[TOURNEY_SIZE])sBoard->tree);
    if (sBoard->tree[3][slot >> 3] == TOURNEY_PLAYER)
        return PAGE_FINALS;
    return slot / 16;
}

#define tState  data[0]

static void Task_Board(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:
        if (!gPaletteFade.active)
            tState = 1;
        break;
    case 1:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            PlaySE(SE_SELECT);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            tState = 2;
        }
        else if (JOY_NEW(R_BUTTON | DPAD_RIGHT))
        {
            PlaySE(SE_SELECT);
            sBoard->page = (sBoard->page + 1) % PAGES;
            DrawPage();
        }
        else if (JOY_NEW(L_BUTTON | DPAD_LEFT))
        {
            PlaySE(SE_SELECT);
            sBoard->page = (sBoard->page + PAGES - 1) % PAGES;
            DrawPage();
        }
        break;
    case 2:
        if (gPaletteFade.active)
            break;
        SetMainCallback2(sBoard->callback);
        UnsetBgTilemapBuffer(1);
        UnsetBgTilemapBuffer(2);
        FreeAllWindowBuffers();
        FREE_AND_SET_NULL(sBoard);
        DestroyTask(taskId);
        break;
    }
}

static void CB2_InitBoard(void)
{
    u16 pal[16];
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

    DecompressDataWithHeaderWram(gDomeTourneyTree_Tilemap, sBoard->tilemap);
    SetBgTilemapBuffer(1, sBoard->bgTilemap);
    DecompressAndLoadBgGfxUsingHeap(1, gDomeTourneyTree_Gfx, 0x2000, 0, 0);
    LoadPalette(gDomeTourneyTree_Pal, BG_PLTT_OFFSET, BG_PLTT_SIZE);
    SetBgTilemapBuffer(2, sBoard->glowTilemap);
    LoadBgTiles(2, sGlowTiles, sizeof(sGlowTiles), 0);
    pal[0] = 0;
    pal[1] = RGB(31, 14, 2);
    LoadPalette(pal, BG_PLTT_ID(PAL_WINNER), PLTT_SIZEOF(2));
    pal[1] = RGB(4, 11, 31);
    LoadPalette(pal, BG_PLTT_ID(PAL_PLAYER), PLTT_SIZEOF(2));
    LoadPalette(sTextPal, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    PutWindowTilemap(WIN_LEFT);
    PutWindowTilemap(WIN_RIGHT);
    PutWindowTilemap(WIN_TITLE);
    CopyBgTilemapBufferToVram(0);

    sBoard->page = PlayerPage();
    DrawPage();

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB);
    SetMainCallback2(MainCB2);
    CreateTask(Task_Board, 0);
}

// Opens the board of the run under way (or the last one); `callback` when it's closed. The screen
// should already be faded to black (the script's fadescreen).
void TourneyBoard_Open(void (*callback)(void))
{
    sBoard = AllocZeroed(sizeof(*sBoard));
    sBoard->callback = callback;
    Tourney_BuildTree(sBoard->tree);
    SetMainCallback2(CB2_InitBoard);
}
