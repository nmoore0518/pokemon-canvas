#ifndef GUARD_TCG_GBC_H
#define GUARD_TCG_GBC_H

// Pokémon Canvas: the Trading Card Game duel's screens draw as the Game Boy Color game did, onto an
// emulated GBC display (src/tcg/gbc.c): its VRAM (two banks of tiles, two 32x32 BG maps with CGB
// attributes), OAM, scroll and window registers and CGB palettes. Each frame that's shown on the GBA
// in the middle of the screen (the GBC's 160x144, in a border): the GBC tiles converted to the GBA's
// 4bpp as they're written, the BG maps to GBA screen entries, the sprites to GBA OAM.
// The game's code that draws (home/*.asm, engine/menus/*.asm...) is ported as is on top of it,
// under its own names (Tcg_ prefixed), with the GBC's VRAM addresses ($8000-$9FFF).
#include "constants/tcg.h"
#include "constants/tcg_text.h"
#include "constants/tcg_gfx.h"

#define TCG_TILEMAP_WIDTH   32
#define TCG_SCREEN_WIDTH    20      // (in tiles)
#define TCG_SCREEN_HEIGHT   18
#define TCG_TILE_SIZE       16
#define TCG_TILE_SIZE_1BPP  8
#define TCG_OAM_COUNT       40

// GBC VRAM addresses
#define TCG_V_TILES0    0x8000
#define TCG_V_TILES1    0x8800
#define TCG_V_TILES2    0x9000
#define TCG_V_BGMAP0    0x9800
#define TCG_V_BGMAP1    0x9C00

// LCDC (rLCDC's bits)
#define TCG_LCDC_ON         0x80
#define TCG_LCDC_WIN_9C00   0x40
#define TCG_LCDC_WIN_ON     0x20
#define TCG_LCDC_BLOCK01    0x10
#define TCG_LCDC_BG_9C00    0x08
#define TCG_LCDC_OBJ_16     0x04
#define TCG_LCDC_OBJ_ON     0x02
#define TCG_LCDC_BG_ON      0x01

// the joypad (hKeysHeld..., as the GBC's: the GBA's low 8 bits are the same)
#define TCG_PAD_A       0x01
#define TCG_PAD_B       0x02
#define TCG_PAD_SELECT  0x04
#define TCG_PAD_START   0x08
#define TCG_PAD_RIGHT   0x10
#define TCG_PAD_LEFT    0x20
#define TCG_PAD_UP      0x40
#define TCG_PAD_DOWN    0x80
#define TCG_PAD_CTRL_PAD 0xF0
#define TCG_PAD_BUTTONS  0x0F

#define TCG_FLUSH_ONE_PAL   0x80
#define TCG_FLUSH_ALL_PALS  0xC0

// the GBC's WRAM and HRAM that the screens use (wram.asm, hram.asm: the game's names)
struct TcgGbc
{
    u8 vram[2][0x2000];             // $8000-$9FFF, VRAM banks 0 and 1
    u8 oam[TCG_OAM_COUNT * 4];      // wOAM
    u8 oamOffset;                   // wOAMOffset
    u8 vBlankOAMCopyToggle;         // wVBlankOAMCopyToggle
    u8 lcdc;                        // wLCDC
    u8 scx, scy, wx, wy;            // hSCX, hSCY, hWX, hWY
    u8 bankVram;                    // hBankVRAM
    u16 bgPals[8][4];               // wBackgroundPalettesCGB
    u16 objPals[8][4];              // wObjectPalettesCGB
    u8 flushPaletteFlags;           // wFlushPaletteFlags
    u8 vBlankCounter;               // wVBlankCounter
    u8 keysHeld, keysPressed, keysReleased, dpadHeld, dpadRepeat;  // hKeysHeld...
    u8 tileMapFill;                 // wTileMapFill
    void (*doFrameFunction)(void);  // wDoFrameFunction
    void (*vBlankFunction)(void);   // wVBlankFunctionTrampoline
    bool8 debugPauseAllowed;        // wDebugPauseAllowed
    // the text engine (home/process_text.asm, print_text.asm, text_box.asm)
    u8 fontWidth;                   // wFontWidth (TCG_FULL_WIDTH, TCG_HALF_WIDTH)
    u8 textLineCurPos;              // hTextLineCurPos
    u8 halfWidthPrintState;         // wHalfWidthPrintState (the first of a pair of half-width characters)
    u8 japaneseSyllabary;           // hJapaneseSyllabary
    u8 textHorizontalAlign;         // hTextHorizontalAlign
    u8 textLineLength;              // hTextLineLength
    u8 curTextLine;                 // wCurTextLine
    u16 textBGMap0Address;          // hTextBGMap0Address
    u8 lineSeparation;              // wLineSeparation (DOUBLE_SPACED: 0)
    u8 textFlags;                   // hffb0 (bit 0: don't make tiles; bit 1: don't place them)
    u8 textLastTile;                // hffa8 (SetupText's last tile)
    u8 textTileHead;                // hffa9 (the text tiles' list: the most recently used)
    u8 textNextTile;                // wcd04 (the last tile handed out)
    u8 tilePatternSelector;         // wTilePatternSelector
    u8 tilePatternSelectorCorrection;   // wTilePatternSelectorCorrection
    u8 textTileKey1[256], textTileKey2[256];    // wc600, wc700: each text tile's characters
    u8 textTileNext[256], textTilePrev[256];    // wc800, wc900: the list, most recently used first
    u8 textTileBuffer[TCG_TILE_SIZE];   // wTextTileBuffer
    u8 uppercaseHalfWidthLetters;   // wUppercaseHalfWidthLetters
    u8 textSpeed;                   // wTextSpeed
    u8 isTextBoxLabeled;            // wIsTextBoxLabeled
    const u8 *textBoxLabel;         // wTextBoxLabel
    u8 textBoxFrameType;            // wTextBoxFrameType
    struct { u8 syllabary, fontWidth; const u8 *text; } textHeaders[4];  // wTextHeader1-4
    u8 whichTextHeader, whichTxRam2, whichTxRam3;
    u8 stringBuffer[32];            // wStringBuffer
    u8 labeledTextBoxTopBorder[48]; // wLabeledTextBoxTopBorder
    u8 textMaxLength;               // wTextMaxLength
    u16 cardPalette[4];             // wCardPalette
    // the menus (home/menus.asm)
    u8 curMenuItem;                 // wCurMenuItem
    u8 hCurMenuItem;                // hCurMenuItem
    u8 menuCursorXOffset;           // wMenuCursorXOffset
    u8 menuCursorYOffset;           // wMenuCursorYOffset
    u8 menuYSeparation;             // wMenuYSeparation
    u8 numMenuItems;                // wNumMenuItems
    u8 menuVisibleCursorTile;       // wMenuVisibleCursorTile
    u8 menuInvisibleCursorTile;     // wMenuInvisibleCursorTile
    bool32 (*menuUpdateFunc)(u32 item);     // wMenuUpdateFunc (TRUE: the game's carry)
    u8 cursorBlinkCounter;          // wCursorBlinkCounter
    u8 refreshMenuCursorSFX;        // wRefreshMenuCursorSFX
    u8 numListItems;                // wNumListItems
    u8 listScrollOffset;            // wListScrollOffset
    u8 listItemXPosition;           // wListItemXPosition
    u8 listItemNameMaxLength;       // wListItemNameMaxLength
    bool32 (*listFunction)(u32 item);   // wListFunctionPointer
    u8 cardListIndicatorYPosition;  // wCardListIndicatorYPosition
    u8 leftmostItemCursorX;         // wLeftmostItemCursorX
    u8 defaultYesOrNo;              // wDefaultYesOrNo
    // the duel's screens (engine/duel/core.asm)
    u8 currentDuelMenuItem;         // wCurrentDuelMenuItem
    u8 selectedDuelSubMenuScroll;   // wSelectedDuelSubMenuItem + 1 (+ 0 is gTcg's)
    u16 cardListInfoBoxText;        // wCardListInfoBoxText
    u16 cardListHeaderText;         // wCardListHeaderText
    u8 sortCardListByID;            // wSortCardListByID
    u8 noItemSelectionMenuKeys;     // wNoItemSelectionMenuKeys
    u8 cardListItemSelectionMenuType;   // wCardListItemSelectionMenuType (PLAY_CHECK, SELECT_CHECK)
    u8 placingInitialBenchPokemon;  // wPlacingInitialBenchPokemon
    bool8 printSortNumberInCardList;    // wPrintSortNumberInCardListPtr (PrintSortNumberInCardList or none)
    u8 cardPageExitKeys;            // wCardPageExitKeys
    u8 cardPageType;                // wCardPageType
    u8 cardPageNumber;              // wCardPageNumber
    // (the GBA side: the BG maps as GBA screen entries, uploaded at VBlank when changed)
    u16 gbaMap[2][32 * 32] ALIGNED(4);
    bool8 gbaMapDirty[2];
    bool8 gbaPalsDirty;
};

extern struct TcgGbc *gTcgGbc;

// src/data/tcg/gfx.h (tools/gym_hack/tcg/extract_gfx.py): the game's graphics, as it has them
extern const u8 gTcgFontsGfx[TCG_FONTS_GFX_SIZE];
extern const u8 gTcgCardGfx[][TCG_CARD_GFX_SIZE];
#define TCG_GFX(label) (gTcgFontsGfx + TCG_GFX_OFFSET_##label)

// src/tcg/gbc.c: start and stop the emulated display (the duel's own main loop)
void Tcg_GbcInit(void);
void Tcg_GbcFree(void);

// VRAM: the current bank's (hBankVRAM) bytes at a GBC address
static inline u8 *Tcg_Vram(u32 addr)
{
    return &gTcgGbc->vram[gTcgGbc->bankVram][addr - 0x8000];
}
void Tcg_WriteVram(u32 addr, const u8 *src, u32 n);     // (SafeCopyDataDEtoHL, HblankCopyDataHLtoDE...)
void Tcg_WriteVramByte(u32 addr, u8 value);
void Tcg_FillVram(u32 addr, u8 value, u32 n);
void Tcg_BankswitchVRAM(u32 bank);
#define Tcg_BankswitchVRAM0() Tcg_BankswitchVRAM(0)
#define Tcg_BankswitchVRAM1() Tcg_BankswitchVRAM(1)

// home/lcd.asm, frames.asm, input.asm, palettes.asm, setup.asm, objects.asm, bg_map.asm, tiles.asm
void Tcg_WaitForVBlank(void);
void Tcg_DoFrame(void);
void Tcg_DoAFrames(u32 frames);
void Tcg_ReadJoypad(void);
void Tcg_HandleDPadRepeat(void);
void Tcg_ClearJoypad(void);
void Tcg_EnableLCD(void);
void Tcg_DisableLCD(void);
void Tcg_Set_OBJ_8x8(void);
void Tcg_Set_OBJ_8x16(void);
void Tcg_SetWindowOn(void);
void Tcg_SetWindowOff(void);
void Tcg_FlushAllPalettes(void);
void Tcg_FlushPalette(u32 which);
void Tcg_FlushPalette0(void);
void Tcg_FlushPalettesIfRequested(void);
void Tcg_SetupPalettes(void);
void Tcg_SetupVRAM(void);
void Tcg_FillTileMap(void);
void Tcg_SetupRegisters(void);
bool32 Tcg_SetOneObjectAttributes(u8 y, u8 x, u8 tile, u8 attributes);
bool32 Tcg_SetManyObjectsAttributes(const u8 *data, u8 x, u8 y);
void Tcg_ZeroObjectPositions(void);
u32 Tcg_BCCoordToBGMap0Address(u32 x, u32 y);
#define Tcg_DECoordToBGMap0Address Tcg_BCCoordToBGMap0Address
void Tcg_CopyDataToBGMap0(u32 x, u32 y, const u8 *src, u32 n);
void Tcg_WriteByteToBGMap0(u32 x, u32 y, u8 value);
void Tcg_WriteDataBlocksToBGMap0(const u8 *blocks);
const u8 *Tcg_WriteDataBlockToBGMap0(const u8 *block);
void Tcg_FillRectangle(u32 x, u32 y, u32 w, u32 h, u8 firstTile, u8 tileStep, u8 rowStep);
void Tcg_CopyGfxData(u32 dest, const u8 *src, u32 tiles, u32 tileSize);
void Tcg_CopyFontsOrDuelGraphicsTiles(u32 dest, const u8 *src, u32 tiles);
void Tcg_AdjustCoordinatesForBGScroll(u8 *x, u8 *y);
void Tcg_EmptyScreen(void);

// src/tcg/text.c: home/process_text.asm, print_text.asm, text_box.asm, write_number.asm
const u8 *Tcg_GetText(u16 textId);              // (GetTextOffsetFromTextID: NONE is NULL)
void Tcg_ProcessText(const u8 *text);
void Tcg_ProcessTextFromID(u16 textId);
void Tcg_InitTextPrinting(u32 x, u32 y);
void Tcg_InitTextPrintingInTextbox(u32 x, u32 y, u32 lineLength);
void Tcg_InitTextPrinting_ProcessText(u32 x, u32 y, const u8 *text);
void Tcg_InitTextPrinting_ProcessTextFromID(u32 x, u32 y, u16 textId);
void Tcg_InitTextFormat(void);
void Tcg_SetupText(u32 firstTile, u32 lastTile);
void Tcg_PlaceNextTextTile(u8 tile);
void Tcg_TerminateHalfWidthText(void);
s32 Tcg_GetTextLengthInTiles(const u8 *text, u32 *halfTiles);
u32 Tcg_GetTextLengthInHalfTiles(const u8 *text, u32 *chars);
u8 *Tcg_TwoByteNumberToTxSymbol_PadSpace(u16 n, u8 *dest);
void Tcg_TwoByteNumberToText(u16 n, u8 *dest);
u32 Tcg_CountLinesOfTextFromID(u16 textId);
void Tcg_PrintScrollableText_WithTextBoxLabel(u16 textId, u16 labelId);
void Tcg_PrintScrollableText_WithTextBoxLabel_NoWait(u16 textId, u16 labelId);
void Tcg_PrintScrollableText_NoTextBoxLabel(u16 textId);
void Tcg_WaitForPlayerToAdvanceText(void);
void Tcg_PrintText(u16 textId);
void Tcg_PrintTextNoDelay(u16 textId);
u8 *Tcg_CopyText(u16 textId, u8 *dest);
void Tcg_DrawRegularTextBox(u32 x, u32 y, u32 w, u32 h);
void Tcg_DrawLabeledTextBox(u32 x, u32 y, u32 w, u32 h, const u8 *label);
void Tcg_CopyLine(u32 addr, u32 width, u8 left, u8 middle, u8 right);
void Tcg_CopyPlayerName(u8 *dest);

// src/tcg/menus.c: home/menus.asm
struct TcgMenuParams    // (a menu's parameters: wMenuParams)
{
    u8 cursorX, cursorY, ySeparation, numItems, visibleCursorTile, invisibleCursorTile;
    bool32 (*updateFunc)(u32 item);
};
struct TcgCardListParams    // (InitializeCardListParameters's)
{
    u8 cursorX, cursorY, itemX, nameMaxLength, numItems, visibleCursorTile, invisibleCursorTile;
    bool32 (*listFunction)(u32 item);
};
void Tcg_InitializeCardListParameters(u32 numListItems, u32 scrollOffset, u32 item, const struct TcgCardListParams *p);
bool32 Tcg_HandleCardListInput(void);
void Tcg_InitializeMenuParameters(u32 item, const struct TcgMenuParams *p);
bool32 Tcg_HandleMenuInput(void);
void Tcg_PlayOpenOrExitScreenSFX(void);
void Tcg_RefreshMenuCursor(void);
void Tcg_EraseCursor(void);
void Tcg_DrawCursor(u8 tile);
void Tcg_DrawCursor2(void);
void Tcg_SetMenuItem(u32 item);
bool32 Tcg_HandleDuelMenuInputStep(void);       // (HandleDuelMenuInput: one frame's)
void Tcg_PrintCardListItems(u32 numListItems, u32 scrollOffset, u32 item, const struct TcgCardListParams *p);
void Tcg_ReloadCardListItems(void);
u8 *Tcg_OneByteNumberToTxSymbol(u32 n);
u8 *Tcg_OneByteNumberToTxSymbol_PadSpace(u32 n);
u8 *Tcg_OneByteNumberToTxSymbol_TrimLeadingZeroAndAlign(u32 n);
u32 Tcg_CardTypeToSymbolID(void);
const u8 *Tcg_GetCardSymbolData(void);
void Tcg_DrawCardSymbol(u32 x, u32 y);
void Tcg_SetCursorParametersForTextBox(u32 x, u32 y, u8 cursorTile, u8 behindTile);
bool32 Tcg_WaitForButtonAorB(void);             // TRUE: B
void Tcg_SetCursorParametersForTextBox_Default(u32 x, u32 y);
void Tcg_DrawWideTextBox(void);
void Tcg_DrawNarrowTextBox(void);
void Tcg_DrawWideTextBox_PrintTextNoDelay(u16 textId);
void Tcg_DrawWideTextBox_PrintTextNoDelay_Wait(u16 textId);
void Tcg_DrawNarrowTextBox_PrintTextNoDelay(u16 textId);
void Tcg_DrawNarrowTextBox_WaitForInput(u16 textId);
bool32 Tcg_TwoItemHorizontalMenu(u16 textId);
bool32 Tcg_YesOrNoMenu(void);                   // TRUE: no
bool32 Tcg_YesOrNoMenuWithText_LeftAligned(u16 textId);
bool32 Tcg_HandleYesOrNoMenu(u32 cursorX);
void Tcg_PrintYesOrNoItems(u32 x, u32 y);
void Tcg_CopyOpponentName(u8 *dest);

// src/tcg/duel_screens.c: engine/duel/core.asm's screens
void Tcg_SetupDuel(void);
void Tcg_ZeroObjectPositionsAndToggleOAMCopy(void);
void Tcg_LoadSymbolsFont(void);
void Tcg_SetDefaultConsolePalettes(void);
void Tcg_LoadCardGfx(u32 gfx, u32 dest);
void Tcg_LoadLoaded1CardGfx(u32 dest);
void Tcg_LoadPlayAreaCardGfx(u32 deckIndex, u32 dest);
void Tcg_SetBGP7OrSGB2ToCardPalette(void);
void Tcg_SetBGP6OrSGB3ToCardPalette(void);
void Tcg_SetOBP1OrSGB3ToCardPalette(void);
void Tcg_FlushAllPalettesOrSendPal23Packet(void);
void Tcg_ApplyBGP6OrSGB3ToCardImage(u32 x, u32 y);
void Tcg_ApplyBGP7OrSGB2ToCardImage(u32 x, u32 y);
void Tcg_PrintPlayAreaCardAttachedEnergies(u32 x, u32 y, u32 location);
void Tcg_DrawHPBar(u32 maxHP, u32 hp);


// src/tcg/duel_lists.c: the duel menu, box messages, card lists
void Tcg_DrawDuelBoxMessage(u32 boxMessage);
void Tcg_LoadDuelCardSymbolTiles(void);
void Tcg_LoadDuelCardSymbolTiles2(void);
void Tcg_LoadDuelFaceDownCardTiles(void);
void Tcg_LoadDuelCheckPokemonScreenTiles(void);
void Tcg_LoadCardOrDuelMenuBorderTiles(void);
void Tcg_LoadCardTypeHeaderTiles(u32 header);
void Tcg_LoadPlacingThePrizesScreenTiles(void);
void Tcg_LoadDeckAndDiscardPileIcons(void);
void Tcg_LoadDuelCoinTossResultTiles(void);
void Tcg_LoadDuelDrawCardsScreenTiles(void);
bool32 Tcg_DrawCardListScreenLayout(void);
void Tcg_PrintCardListHeaderAndInfoBoxTexts(void);
bool32 Tcg_OpenDiscardPileScreen(void);
void Tcg_OpenTurnHolderHandScreen_Simple(void);
void Tcg_OpenNonTurnHolderHandScreen_Simple(void);

// src/tcg/card_page.c: the card pages
void Tcg_OpenCardPage(u32 type, u8 exitKeys);
#define Tcg_OpenCardPage_FromCheckHandOrDiscardPile() Tcg_OpenCardPage(TCG_CARDPAGETYPE_NOT_PLAY_AREA, TCG_PAD_B | TCG_PAD_UP | TCG_PAD_DOWN)
#define Tcg_OpenCardPage_FromCheckPlayArea() Tcg_OpenCardPage(TCG_CARDPAGETYPE_PLAY_AREA, TCG_PAD_B)
void Tcg_OpenActivePokemonScreen(void);
void Tcg_OpenPlayAreaScreenForViewing(void);

#endif // GUARD_TCG_GBC_H
