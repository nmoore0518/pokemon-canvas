// Pokémon Canvas: the Trading Card Game's menus and text boxes, ported from the Game Boy Color game's
// home/menus.asm (pret/poketcg) under its own names (Tcg_ prefixed). Where the game returns carry
// (a choice made), these return TRUE; the choice is in gTcgGbc's curMenuItem (e) and hCurMenuItem
// (a; $ff: B).
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"

#define G gTcgGbc

void Tcg_InitializeCardListParameters(u32 numListItems, u32 scrollOffset, u32 item, const struct TcgCardListParams *p)
{
    G->numListItems = numListItems;
    G->listScrollOffset = scrollOffset;
    G->curMenuItem = item;
    G->hCurMenuItem = item + scrollOffset;
    G->menuCursorXOffset = p->cursorX;
    G->menuCursorYOffset = p->cursorY;
    G->listItemXPosition = p->itemX;
    G->listItemNameMaxLength = p->nameMaxLength;
    G->numMenuItems = p->numItems;
    G->menuVisibleCursorTile = p->visibleCursorTile;
    G->menuInvisibleCursorTile = p->invisibleCursorTile;
    G->listFunction = p->listFunction;
    G->cursorBlinkCounter = 0;
    G->menuYSeparation = 1;
}

// (HandleCardListInput: d, e and a are the scroll offset, the item and the list item)
bool32 Tcg_HandleCardListInput(void)
{
    return Tcg_HandleMenuInput();
}

void Tcg_InitializeMenuParameters(u32 item, const struct TcgMenuParams *p)
{
    G->curMenuItem = item;
    G->hCurMenuItem = item;
    G->menuCursorXOffset = p->cursorX;
    G->menuCursorYOffset = p->cursorY;
    G->menuYSeparation = p->ySeparation;
    G->numMenuItems = p->numItems;
    G->menuVisibleCursorTile = p->visibleCursorTile;
    G->menuInvisibleCursorTile = p->invisibleCursorTile;
    G->menuUpdateFunc = p->updateFunc;
    G->cursorBlinkCounter = 0;
}

void Tcg_PlayOpenOrExitScreenSFX(void)
{
    Tcg_PlaySFX(G->hCurMenuItem == 0xFF ? TCG_SFX_CANCEL : TCG_SFX_CONFIRM);
}

static void RefreshMenuCursor_CheckPlaySFX(void)
{
    if (G->refreshMenuCursorSFX)
        Tcg_PlaySFX(G->refreshMenuCursorSFX);
    Tcg_RefreshMenuCursor();
}

// one frame of a menu: up and down (wrapping), A or B. TRUE: chosen (B: hCurMenuItem $ff)
bool32 Tcg_HandleMenuInput(void)
{
    G->refreshMenuCursorSFX = 0;
    if (G->dpadHeld)
    {
        u32 b = G->dpadHeld;
        s32 a = G->curMenuItem;
        bool32 moved = FALSE;
        if (b & TCG_PAD_UP)
        {
            a--;
            if (a < 0)
                a = G->numMenuItems - 1;
            moved = TRUE;
        }
        else if (b & TCG_PAD_DOWN)
        {
            a++;
            if (a >= G->numMenuItems)
                a = 0;
            moved = TRUE;
        }
        if (moved)
        {
            G->refreshMenuCursorSFX = TCG_SFX_CURSOR;
            Tcg_EraseCursor();
            G->curMenuItem = a;
            G->cursorBlinkCounter = 0;
        }
    }
    G->hCurMenuItem = G->curMenuItem;
    if (G->menuUpdateFunc)
    {
        if (!G->menuUpdateFunc(G->hCurMenuItem))
        {
            RefreshMenuCursor_CheckPlaySFX();
            return FALSE;
        }
        Tcg_DrawCursor2();
        Tcg_PlayOpenOrExitScreenSFX();
        return TRUE;
    }
    if (!(G->keysPressed & (TCG_PAD_A | TCG_PAD_B)))
    {
        RefreshMenuCursor_CheckPlaySFX();
        return FALSE;
    }
    if (G->keysPressed & TCG_PAD_A)
    {
        Tcg_DrawCursor2();
        Tcg_PlayOpenOrExitScreenSFX();
        return TRUE;
    }
    G->hCurMenuItem = 0xFF;
    Tcg_PlayOpenOrExitScreenSFX();
    return TRUE;
}

void Tcg_RefreshMenuCursor(void)
{
    u8 c = G->cursorBlinkCounter++;
    if (c & 0xF)
        return;
    if (G->cursorBlinkCounter & 0x10)
        Tcg_EraseCursor();
    else
        Tcg_DrawCursor(G->menuVisibleCursorTile);
}

void Tcg_EraseCursor(void)
{
    if (gTcg->headless)
        return;
    Tcg_DrawCursor(G->menuInvisibleCursorTile);
}

void Tcg_DrawCursor(u8 tile)
{
    u8 x = G->menuCursorXOffset, y = G->menuCursorYOffset + G->menuYSeparation * G->curMenuItem;
    Tcg_AdjustCoordinatesForBGScroll(&x, &y);
    Tcg_WriteByteToBGMap0(x, y, tile);
}

void Tcg_DrawCursor2(void)
{
    Tcg_DrawCursor(G->menuVisibleCursorTile);
}

void Tcg_SetMenuItem(u32 item)
{
    G->curMenuItem = item;
    G->hCurMenuItem = item;
    G->cursorBlinkCounter = 0;
}

static const u8 sDuelMenuCursorCoords[][2] =
{
    { 2, 14},   // Hand
    { 2, 16},   // Attack
    { 8, 14},   // Check
    { 8, 16},   // Pkmn Power
    {14, 14},   // Retreat
    {14, 16},   // Done
};

static void DrawDuelMenuCursor(u8 tile)
{
    Tcg_WriteByteToBGMap0(sDuelMenuCursorCoords[G->curMenuItem][0], sDuelMenuCursorCoords[G->curMenuItem][1], tile);
}

// HandleDuelMenuInput: one frame of the duel menu's 3x2 items. TRUE: A (the item in curMenuItem)
bool32 Tcg_HandleDuelMenuInputStep(void)
{
    u32 b = G->dpadHeld;
    u8 c;
    if (b)
    {
        s32 a = G->curMenuItem;
        if (b & (TCG_PAD_UP | TCG_PAD_DOWN))
            a ^= 1;
        else if (b & TCG_PAD_LEFT)
        {
            a -= 2;
            if (a < 0)
                a = (a & 1) + 4;
        }
        else if (b & TCG_PAD_RIGHT)
        {
            a += 2;
            if (a >= 6)
                a &= 1;
        }
        else
        {
            if (b & TCG_PAD_A)
            {
                Tcg_PlayOpenOrExitScreenSFX();
                return TRUE;
            }
            goto blink;
        }
        Tcg_PlaySFX(TCG_SFX_CURSOR);
        DrawDuelMenuCursor(TCG_SYM_SPACE);
        G->curMenuItem = G->hCurMenuItem = a;
        G->cursorBlinkCounter = 0;
    }
blink:
    c = G->cursorBlinkCounter++;
    if (c & 0xF)
        return FALSE;
    DrawDuelMenuCursor((G->cursorBlinkCounter & 0x10) ? TCG_SYM_SPACE : TCG_SYM_CURSOR_R);
    return FALSE;
}

// ---- card lists ----------------------------------------------------------------------------------------

static bool32 CardListMenuFunction(u32 item);

void Tcg_PrintCardListItems(u32 numListItems, u32 scrollOffset, u32 item, const struct TcgCardListParams *p)
{
    Tcg_InitializeCardListParameters(numListItems, scrollOffset, item, p);
    G->menuUpdateFunc = CardListMenuFunction;
    G->menuYSeparation = 2;
    G->cardListIndicatorYPosition = 1;
    Tcg_ReloadCardListItems();
}

// the visible part of wDuelTempList: each card's symbol and name, and the up and down arrows
void Tcg_ReloadCardListItems(void)
{
    u32 c, x, y, n, i;
    u8 up = G->listScrollOffset ? TCG_SYM_CURSOR_U : TCG_SYM_SPACE, down = TCG_SYM_SPACE;
    c = G->menuCursorYOffset - 1;
    Tcg_WriteByteToBGMap0(18, c, up);
    if (G->listScrollOffset + G->numMenuItems < G->numListItems)
        down = TCG_SYM_CURSOR_D;
    Tcg_WriteByteToBGMap0(18, (u8)(G->numMenuItems * 2 + c - 1), down);
    x = G->listItemXPosition;
    y = G->menuCursorYOffset;
    n = G->numMenuItems;
    for (i = 0; ; )
    {
        u8 card = gTcg->duelTempList[G->listScrollOffset + i];
        if (card == 0xFF)
            break;
        Tcg_LoadCard1(card);
        Tcg_DrawCardSymbol(x, y);
        Tcg_InitTextPrinting(x, y);
        Tcg_CopyCardNameAndLevelLength(G->listItemNameMaxLength);
        Tcg_ProcessText(gTcg->defaultText);
        i++;
        if ((u8)(G->numListItems - 1) < i)
            break;
        y += 2;
        if (--n == 0)
            break;
    }
}

// (Func_2827: the list's text tiles made, none placed)
void Tcg_ReloadCardListItemsTilesOnly(void)
{
    G->textFlags = 1;
    Tcg_ReloadCardListItems();
    G->textFlags = 0;
}

u8 *Tcg_OneByteNumberToTxSymbol(u32 n)
{
    u8 *hl = gTcg->defaultText;
    hl[0] = TCG_SYM_0 + n / 10;
    hl[1] = TCG_SYM_0 + n % 10;
    hl[2] = TCG_SYM_SPACE;
    return hl;
}

u8 *Tcg_OneByteNumberToTxSymbol_PadSpace(u32 n)
{
    u8 *hl = Tcg_OneByteNumberToTxSymbol(n);
    if (hl[0] == TCG_SYM_0)
        hl[0] = TCG_SYM_SPACE;
    return hl;
}

u8 *Tcg_OneByteNumberToTxSymbol_TrimLeadingZeroAndAlign(u32 n)
{
    u8 *hl = Tcg_OneByteNumberToTxSymbol(n);
    if (hl[0] == TCG_SYM_0)
    {
        hl[0] = hl[1];
        hl[1] = TCG_SYM_SPACE;
    }
    return hl;
}

// one frame of a card list: up/down past the page's ends scroll it, left/right a page; the "n/N" at
// the top right. TRUE: chosen (B: hCurMenuItem $ff)
static bool32 CardListMenuFunction(u32 item)
{
    u32 b = G->dpadHeld, c = G->numMenuItems - 1;
    u8 a = G->curMenuItem;
    if (b & TCG_PAD_UP)
    {
        if (a == c)
        {
            // (it wrapped from the top)
            G->curMenuItem = 0;
            if (G->listScrollOffset == 0)
            {
                G->refreshMenuCursorSFX = 0;
                goto cont;
            }
            G->listScrollOffset--;
            Tcg_ReloadCardListItems();
        }
    }
    else if (b & TCG_PAD_DOWN)
    {
        if (a == 0)
        {
            // (it wrapped from the bottom)
            G->curMenuItem = c;
            if ((u8)(G->listScrollOffset + c + 1) == G->numListItems)
            {
                G->refreshMenuCursorSFX = 0;
                goto cont;
            }
            G->listScrollOffset++;
            Tcg_ReloadCardListItems();
        }
        else if ((u8)(a + G->listScrollOffset) >= G->numListItems)
        {
            G->curMenuItem--;
            G->refreshMenuCursorSFX = 0;
        }
    }
    else if (b & TCG_PAD_LEFT)
    {
        s32 off = G->listScrollOffset;
        if (off == 0)
            goto cont;
        off -= G->numMenuItems;
        if (off >= 0)
        {
            G->listScrollOffset = off;
            Tcg_ReloadCardListItems();
        }
        else
        {
            s32 cur;
            Tcg_EraseCursor();
            cur = G->listScrollOffset + G->curMenuItem - G->numMenuItems;
            if (cur < 0)
                cur += G->numMenuItems;
            G->curMenuItem = cur;
            G->listScrollOffset = 0;
            G->refreshMenuCursorSFX = 0;
            Tcg_ReloadCardListItems();
        }
    }
    else if (b & TCG_PAD_RIGHT)
    {
        if (G->numMenuItems < G->numListItems)
        {
            u8 next = G->listScrollOffset + G->numMenuItems;
            if ((u8)(next + G->numMenuItems - 1) < G->numListItems)
            {
                G->listScrollOffset = next;
                Tcg_ReloadCardListItems();
            }
            else
            {
                s32 cur;
                u8 abs = G->listScrollOffset + G->curMenuItem;
                Tcg_EraseCursor();
                G->listScrollOffset = G->numListItems - G->numMenuItems;
                cur = abs - G->listScrollOffset;
                if (cur < 0)
                    cur += G->numMenuItems;
                G->curMenuItem = cur;
                Tcg_ReloadCardListItems();
            }
        }
    }
cont:
    G->hCurMenuItem = G->listScrollOffset + G->curMenuItem;
    if (G->cardListIndicatorYPosition != 0xFF)
    {
        u32 y = G->cardListIndicatorYPosition;
        Tcg_CopyDataToBGMap0(13, y, Tcg_OneByteNumberToTxSymbol_PadSpace(G->hCurMenuItem + 1), 2);
        Tcg_WriteByteToBGMap0(15, y, TCG_SYM_SLASH);
        Tcg_CopyDataToBGMap0(16, y, Tcg_OneByteNumberToTxSymbol_PadSpace(G->numListItems), 2);
    }
    if (G->listFunction)
        return G->listFunction(G->hCurMenuItem);
    if (!(G->keysPressed & (TCG_PAD_A | TCG_PAD_B)))
        return FALSE;
    if (G->keysPressed & TCG_PAD_B)
        G->hCurMenuItem = 0xFF;
    return TRUE;
}

// ---- card symbols ----------------------------------------------------------------------------------------

u32 Tcg_CardTypeToSymbolID(void)
{
    u32 type = gTcg->loadedCard1->type;
    if (type >= TCG_TYPE_TRAINER)
        return 11;
    if (type >= TCG_TYPE_ENERGY)
        return type & 7;
    return gTcg->loadedCard1->stage + 8;
}

static const u8 sCardSymbolTable[][2] =
{
    {0xe0, 0x01},   // TYPE_ENERGY_FIRE
    {0xe4, 0x02},   // TYPE_ENERGY_GRASS
    {0xe8, 0x01},   // TYPE_ENERGY_LIGHTNING
    {0xec, 0x02},   // TYPE_ENERGY_WATER
    {0xf0, 0x03},   // TYPE_ENERGY_PSYCHIC
    {0xf4, 0x03},   // TYPE_ENERGY_FIGHTING
    {0xf8, 0x00},   // TYPE_ENERGY_DOUBLE_COLORLESS
    {0xfc, 0x02},   // TYPE_ENERGY_UNUSED
    {0xd0, 0x02},   // TYPE_PKMN_*, Basic
    {0xd4, 0x02},   // TYPE_PKMN_*, Stage 1
    {0xd8, 0x01},   // TYPE_PKMN_*, Stage 2
    {0xdc, 0x02},   // TYPE_TRAINER
};

// (GetCardSymbolData: the first tile, then the CGB palette)
const u8 *Tcg_GetCardSymbolData(void)
{
    return sCardSymbolTable[Tcg_CardTypeToSymbolID()];
}

// the loaded card's 2x2 symbol at x - 2, y - 1
void Tcg_DrawCardSymbol(u32 x, u32 y)
{
    const u8 *s = Tcg_GetCardSymbolData();
    x -= 2;
    y -= 1;
    Tcg_BankswitchVRAM1();
    Tcg_FillRectangle(x, y, 2, 2, s[1], 0, 0);
    Tcg_BankswitchVRAM0();
    Tcg_FillRectangle(x, y, 2, 2, s[0], 1, 2);
}

// ---- text boxes ------------------------------------------------------------------------------------------

void Tcg_SetCursorParametersForTextBox(u32 x, u32 y, u8 cursorTile, u8 behindTile)
{
    G->curMenuItem = 0;
    G->menuCursorXOffset = x;
    G->menuCursorYOffset = y;
    G->menuYSeparation = 0;
    G->numMenuItems = 1;
    G->menuVisibleCursorTile = cursorTile;
    G->menuInvisibleCursorTile = behindTile;
    G->cursorBlinkCounter = 0;
}

bool32 Tcg_WaitForButtonAorB(void)
{
    for (;;)
    {
        G->waitingFor = TCG_WAIT_TEXT_BOX;
        Tcg_DoFrame();
        Tcg_RefreshMenuCursor();
        if (G->keysPressed & TCG_PAD_A)
        {
            Tcg_EraseCursor();
            return FALSE;
        }
        if (G->keysPressed & TCG_PAD_B)
        {
            Tcg_EraseCursor();
            return TRUE;
        }
    }
}

void Tcg_SetCursorParametersForTextBox_Default(u32 x, u32 y)
{
    Tcg_SetCursorParametersForTextBox(x, y, TCG_SYM_CURSOR_R, TCG_SYM_SPACE);
    Tcg_WaitForButtonAorB();
}

void Tcg_DrawWideTextBox(void)
{
    u8 x = 0, y = 12;
    Tcg_AdjustCoordinatesForBGScroll(&x, &y);
    Tcg_DrawRegularTextBox(x, y, 20, 6);
}

void Tcg_DrawNarrowTextBox(void)
{
    u8 x = 0, y = 12;
    Tcg_AdjustCoordinatesForBGScroll(&x, &y);
    Tcg_DrawRegularTextBox(x, y, 12, 6);
}

static void DrawTextBox_PrintTextNoDelay(u32 lineLength, u16 textId)
{
    u8 x = 1, y = 14;
    Tcg_AdjustCoordinatesForBGScroll(&x, &y);
    Tcg_InitTextPrintingInTextbox(x, y, lineLength);
    if (textId != TCG_TEXT_NONE)
        Tcg_PrintTextNoDelay(textId);
    else
        Tcg_ProcessText(gTcg->defaultText);
}

void Tcg_DrawWideTextBox_PrintTextNoDelay(u16 textId)
{
    Tcg_DrawWideTextBox();
    DrawTextBox_PrintTextNoDelay(19, textId);
}

void Tcg_DrawWideTextBox_PrintTextNoDelay_Wait(u16 textId)
{
    Tcg_DrawWideTextBox_PrintTextNoDelay(textId);
    Tcg_WaitForWideTextBoxInput();
}

void Tcg_DrawNarrowTextBox_PrintTextNoDelay(u16 textId)
{
    Tcg_DrawNarrowTextBox();
    DrawTextBox_PrintTextNoDelay(11, textId);
}

void Tcg_DrawWideTextBox_PrintText(u16 textId)
{
    u8 x = 1, y = 14;
    if (gTcg->headless)
        return;
    Tcg_DrawWideTextBox();
    Tcg_AdjustCoordinatesForBGScroll(&x, &y);
    Tcg_InitTextPrintingInTextbox(x, y, 19);
    Tcg_EnableLCD();
    Tcg_PrintText(textId);
}

static const struct TcgMenuParams sNarrowTextBoxMenuParameters = {10, 17, 1, 1, TCG_SYM_CURSOR_D, TCG_SYM_BOX_BOTTOM, NULL};
static const struct TcgMenuParams sWideTextBoxMenuParameters = {18, 17, 1, 1, TCG_SYM_CURSOR_D, TCG_SYM_BOX_BOTTOM, NULL};

void Tcg_DrawNarrowTextBox_WaitForInput(u16 textId)
{
    Tcg_DrawNarrowTextBox_PrintTextNoDelay(textId);
    Tcg_InitializeMenuParameters(0, &sNarrowTextBoxMenuParameters);
    Tcg_EnableLCD();
    do
    {
        Tcg_DoFrame();
        Tcg_RefreshMenuCursor();
    } while (!(G->keysPressed & (TCG_PAD_A | TCG_PAD_B)));
}

void Tcg_WaitForWideTextBoxInput(void)
{
    if (gTcg->headless)
        return;
    Tcg_InitializeMenuParameters(0, &sWideTextBoxMenuParameters);
    Tcg_EnableLCD();
    do
    {
        G->waitingFor = TCG_WAIT_TEXT_BOX;
        Tcg_DoFrame();
        Tcg_RefreshMenuCursor();
    } while (!(G->keysPressed & (TCG_PAD_A | TCG_PAD_B)));
    G->waitingFor = TCG_WAIT_NONE;
    Tcg_EraseCursor();
}

void Tcg_DrawWideTextBox_WaitForInput(u16 textId)
{
    if (gTcg->headless)
        return;
    Tcg_DrawWideTextBox_PrintText(textId);
    Tcg_WaitForWideTextBoxInput();
}

// ---- yes or no ---------------------------------------------------------------------------------------------

static bool32 HandleYesOrNoMenu_Loop(bool32 refreshFirst)
{
    for (;;)
    {
        if (refreshFirst)
        {
            // .refresh_menu: the other item
            G->curMenuItem ^= 1;
            G->menuCursorXOffset = G->curMenuItem * 4 + G->leftmostItemCursorX;
            G->cursorBlinkCounter = 0;
        }
        refreshFirst = TRUE;
        for (;;)
        {
            G->waitingFor = TCG_WAIT_YES_NO;
            Tcg_DoFrame();
            Tcg_RefreshMenuCursor();
            if (G->keysPressed & TCG_PAD_A)
            {
                G->hCurMenuItem = G->curMenuItem;
                G->defaultYesOrNo = 0;
                if (G->curMenuItem == 0)
                    return FALSE;
                G->hCurMenuItem = 1;
                return TRUE;
            }
            if (G->dpadHeld & (TCG_PAD_RIGHT | TCG_PAD_LEFT))
                break;
        }
        Tcg_PlaySFX(TCG_SFX_CURSOR);
        Tcg_EraseCursor();
    }
}

// (the game's TwoItemHorizontalMenu)
bool32 Tcg_TwoItemHorizontalMenu(u16 textId)
{
    Tcg_DrawWideTextBox_PrintText(textId);
    G->leftmostItemCursorX = 6;
    Tcg_SetCursorParametersForTextBox(6, 16, TCG_SYM_CURSOR_R, TCG_SYM_SPACE);
    G->curMenuItem = 1;
    Tcg_EnableLCD();
    return HandleYesOrNoMenu_Loop(TRUE);
}

bool32 Tcg_HandleYesOrNoMenu(u32 cursorX)
{
    G->leftmostItemCursorX = cursorX;
    Tcg_SetCursorParametersForTextBox(cursorX, 16, TCG_SYM_CURSOR_R, TCG_SYM_SPACE);
    G->curMenuItem = G->defaultYesOrNo;
    Tcg_EnableLCD();
    return HandleYesOrNoMenu_Loop(TRUE);
}

void Tcg_PrintYesOrNoItems(u32 x, u32 y)
{
    u8 bx = x, by = y;
    Tcg_AdjustCoordinatesForBGScroll(&bx, &by);
    Tcg_InitTextPrinting_ProcessTextFromID(bx, by, TCG_TEXT_YesOrNo);
}

bool32 Tcg_YesOrNoMenu(void)
{
    Tcg_PrintYesOrNoItems(7, 16);
    return Tcg_HandleYesOrNoMenu(6);
}

bool32 Tcg_YesOrNoMenuWithText(u16 textId)
{
    if (gTcg->headless)
        return TRUE;
    Tcg_DrawWideTextBox_PrintText(textId);
    return Tcg_YesOrNoMenu();
}

bool32 Tcg_YesOrNoMenuWithText_SetCursorToYes(u16 textId)
{
    if (gTcg->headless)
        return TRUE;
    G->defaultYesOrNo = 1;
    return Tcg_YesOrNoMenuWithText(textId);
}

bool32 Tcg_YesOrNoMenuWithText_LeftAligned(u16 textId)
{
    Tcg_DrawNarrowTextBox_PrintTextNoDelay(textId);
    Tcg_PrintYesOrNoItems(3, 16);
    return Tcg_HandleYesOrNoMenu(2);
}
