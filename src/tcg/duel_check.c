// Pokémon Canvas: the Trading Card Game duel's Check menu, the "Your/Opp. Play Area" screens and the
// prizes (placing them at the start, choosing which to take), ported from the Game Boy Color game's
// engine/menus/duel.asm and engine/duel/core.asm (pret/poketcg) under their own names (Tcg_
// prefixed). The duelist variables' pages the game addresses by hWhoseTurn's value are gTcg->vars[].
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"

#define G gTcgGbc

static EWRAM_DATA u8 sCheckMenuCursorX = 0;            // wCheckMenuCursorXPosition
static EWRAM_DATA u8 sCheckMenuCursorY = 0;            // wCheckMenuCursorYPosition
static EWRAM_DATA u8 sCheckMenuCursorBlinkCounter = 0; // wCheckMenuCursorBlinkCounter
static EWRAM_DATA u8 sCheckMenuWhichDuelist = 0;       // wCheckMenuPlayAreaWhichDuelist
static EWRAM_DATA u8 sCheckMenuWhichLayout = 0;        // wCheckMenuPlayAreaWhichLayout
static EWRAM_DATA u8 sCe5e = 0;                        // wce5e (bit 7: the opponent's hand isn't on the menu)
static EWRAM_DATA u8 sLastCursorPosition = 0;          // wYourOrOppPlayAreaLastCursorPosition
static EWRAM_DATA u8 sCurPosition = 0;                 // wYourOrOppPlayAreaCurPosition
static EWRAM_DATA u8 sDuelInitialPrizesUpperBitsSet = 0;

#define VARS(d) (gTcg->vars[d])

// ---- the check menus' cursor --------------------------------------------------------------------------

static void ResetCheckMenuCursorPositionAndBlink(void)
{
    sCheckMenuCursorX = sCheckMenuCursorY = sCheckMenuCursorBlinkCounter = 0;
}

static void DrawCheckMenuCursor(u8 tile)
{
    Tcg_WriteByteToBGMap0(sCheckMenuCursorX * 10 + 1, sCheckMenuCursorY * 2 + 14, tile);
}

static void PlaySFXConfirmOrCancel(bool32 cancel)
{
    Tcg_PlaySFX(cancel ? TCG_SFX_CANCEL : TCG_SFX_CONFIRM);
}

// HandleCheckMenuInput (deck_check.asm) and HandleCheckMenuInput_YourOrOppPlayArea (yourOrOpp: its
// rules for the 3-item menus). TRUE (the game's carry): A or B (*cancel)
static bool32 HandleCheckMenuInput(bool32 yourOrOpp, bool32 *cancel)
{
    u8 d = sCheckMenuCursorX, e = sCheckMenuCursorY, pad = G->dpadHeld, sfx = 0;
    if (pad)
    {
        bool32 moved = FALSE;
        if (!yourOrOpp)
        {
            if (pad & (TCG_PAD_LEFT | TCG_PAD_RIGHT))
            {
                d ^= 1;
                moved = TRUE;
            }
            else if (pad & (TCG_PAD_UP | TCG_PAD_DOWN))
            {
                e ^= 1;
                moved = TRUE;
            }
        }
        else if (!(sCe5e & 0x80) && (pad & (TCG_PAD_LEFT | TCG_PAD_RIGHT)))
        {
            if (!(sCe5e & 0x7f))
            {
                if (e)
                    e--;
            }
            else if (!e)
            {
                e++;
            }
            d ^= 1;
            moved = TRUE;
        }
        else if (pad & (TCG_PAD_UP | TCG_PAD_DOWN))
        {
            if (d)
                d--;
            e ^= 1;
            moved = TRUE;
        }
        if (moved)
        {
            sfx = TCG_SFX_CURSOR;
            DrawCheckMenuCursor(TCG_SYM_SPACE);
            sCheckMenuCursorX = d;
            sCheckMenuCursorY = e;
            sCheckMenuCursorBlinkCounter = 0;
        }
    }
    if (G->keysPressed & (TCG_PAD_A | TCG_PAD_B))
    {
        *cancel = !(G->keysPressed & TCG_PAD_A);
        if (!*cancel)
            DrawCheckMenuCursor(TCG_SYM_CURSOR_R);
        PlaySFXConfirmOrCancel(*cancel);
        return TRUE;
    }
    if (sfx)
        Tcg_PlaySFX(sfx);
    if ((sCheckMenuCursorBlinkCounter++ & 0xF) == 0)
        DrawCheckMenuCursor((sCheckMenuCursorBlinkCounter & 0x10) ? TCG_SYM_SPACE : TCG_SYM_CURSOR_R);
    return FALSE;
}

static void PlaceTextItems(const u8 (*items)[4], u32 n)
{
    u32 i;
    for (i = 0; i < n; i++)
        Tcg_InitTextPrinting_ProcessTextFromID(items[i][0], items[i][1], items[i][2] | (items[i][3] << 8));
}

#define TEXTITEM(x, y, t) {x, y, (t) & 0xFF, (t) >> 8}

// ---- the play area screen's pieces ----------------------------------------------------------------------

static void LoadCursorTile(void)
{
    static const u8 sTileData[16] = {0xe0, 0xc0, 0x98, 0xb0, 0x84, 0x8c, 0x83, 0x82, 0x86, 0x8f, 0x9d, 0xbe, 0xf4, 0xf8, 0x50, 0x60};
    Tcg_WriteVram(TCG_V_TILES0, sTileData, sizeof(sTileData));
}

static void GetDuelInitialPrizesUpperBitsSet(void)
{
    sDuelInitialPrizesUpperBitsSet = ((1 << gTcg->duelInitialPrizes) - 1) | 0xC0;
}

// the prizes: a card or an empty slot each, at coords (y, x pairs)
static void DrawPlayArea_PrizeCards(const u8 *coords)
{
    u8 prizes = VARS(sCheckMenuWhichDuelist)[TCG_DUELVARS_PRIZES];
    u32 b;
    GetDuelInitialPrizesUpperBitsSet();
    for (b = 0; b < gTcg->duelInitialPrizes; b++, prizes >>= 1, coords += 2)
    {
        Tcg_FillRectangle(coords[1], coords[0], 2, 2, (prizes & 1) ? 0xdc : 0xe0, 1, 2);
        Tcg_BankswitchVRAM1();
        Tcg_FillRectangle(coords[1], coords[0], 2, 2, 0x02, 0, 0);
        Tcg_BankswitchVRAM0();
    }
}

static const u8 sPrizeCardsCoordinateData_YourOrOppPlayArea_Player[] = {2, 1, 2, 3, 4, 1, 4, 3, 6, 1, 6, 3};
static const u8 sPrizeCardsCoordinateData_YourOrOppPlayArea_Opponent[] = {9, 17, 9, 15, 7, 17, 7, 15, 5, 17, 5, 15};

// the bench: each Pokémon's stage card, then the empty slots, from x, y (the other layout: from the
// right, leftwards)
static void DrawPlayArea_BenchCards(s32 x, u32 y, s32 spacing)
{
    const u8 *v;
    u32 n, i;
    if (sCheckMenuWhichLayout != sCheckMenuWhichDuelist)
    {
        x += spacing * 4;
        spacing = -spacing;
    }
    v = VARS(sCheckMenuWhichDuelist);
    n = v[TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA];
    for (i = 1; i < n; i++, x += spacing)
    {
        u8 tile = (v[TCG_DUELVARS_ARENA_CARD_STAGE + i] << 2) + 0xe4;
        Tcg_FillRectangle(x, y, 2, 2, tile, 1, 2);
        Tcg_BankswitchVRAM1();
        Tcg_FillRectangle(x, y, 2, 2, (tile == 0xec || tile == 0xf0) ? 0x01 : 0x02, 0, 0);
        Tcg_BankswitchVRAM0();
    }
    for (i = n; i < TCG_MAX_PLAY_AREA_POKEMON; i++, x += spacing)
    {
        Tcg_FillRectangle(x, y, 2, 2, 0xf4, 1, 2);
        Tcg_BankswitchVRAM1();
        Tcg_FillRectangle(x, y, 2, 2, 0x02, 0, 0);
        Tcg_BankswitchVRAM0();
    }
}

// (CalculateOnesAndTensDigits, then " x<tens><ones>")
static void PrintCrossNumber(u32 n)
{
    u8 *t = gTcg->defaultText;
    u8 ones = TCG_SYM_0 + n % 10, tens = n / 10 ? TCG_SYM_0 + n / 10 : TCG_SYM_SPACE;
    *t++ = TCG_TX_SYMBOL;
    *t++ = TCG_SYM_CROSS;
    *t++ = TCG_TX_SYMBOL;
    *t++ = tens;
    *t++ = TCG_TX_SYMBOL;
    *t++ = ones;
    *t = TCG_TX_END;
    Tcg_ProcessText(gTcg->defaultText);
}

static void DrawPlayArea_HandText(const u8 *coords, u32 n)
{
    Tcg_InitTextPrinting(coords[0], coords[1]);
    Tcg_ProcessTextFromID(TCG_TEXT_Hand_2);
    PrintCrossNumber(n);
}

static void DrawPlayArea_IconWithValue(const u8 *coords, u8 tile, u32 n)
{
    Tcg_FillRectangle(coords[0], coords[1], 2, 2, tile, 1, 2);
    Tcg_BankswitchVRAM1();
    Tcg_FillRectangle(coords[0], coords[1], 2, 2, 0x02, 0, 0);
    Tcg_BankswitchVRAM0();
    Tcg_InitTextPrinting(coords[0] + 2, coords[1] + 1);
    PrintCrossNumber(n);
}

// (DrawYourOrOppPlayArea_Icons.draw) the hand's count, the deck and discard pile icons and counts
static void DrawYourOrOppPlayArea_IconsAt(const u8 *coords)
{
    const u8 *v = VARS(sCheckMenuWhichDuelist);
    DrawPlayArea_HandText(coords, v[TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND]);
    DrawPlayArea_IconWithValue(coords + 2, 0xd4, TCG_DECK_SIZE - v[TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK]);
    DrawPlayArea_IconWithValue(coords + 4, 0xd8, v[TCG_DUELVARS_NUMBER_OF_CARDS_IN_DISCARD_PILE]);
}

static const u8 sPlayAreaIconCoordinates_Player1[] = {15, 7, 15, 2, 15, 4};
static const u8 sPlayAreaIconCoordinates_Opponent1[] = {1, 5, 1, 9, 1, 7};

static void DrawYourOrOppPlayArea_ActiveCardGfx(u32 x, u32 y)
{
    u8 card = VARS(sCheckMenuWhichDuelist)[TCG_DUELVARS_ARENA_CARD];
    if (card == 0xFF)
        return;
    if (gTcg->whoseTurn != sCheckMenuWhichDuelist)
    {
        Tcg_SwapTurn();
        Tcg_LoadCard1(card);
        Tcg_SwapTurn();
    }
    else
    {
        Tcg_LoadCard1(card);
    }
    Tcg_LoadLoaded1CardGfx(TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE);
    Tcg_SetBGP6OrSGB3ToCardPalette();
    Tcg_FlushAllPalettesOrSendPal23Packet();
    Tcg_FillRectangle(x, y, 8, 6, 0xa0, 6, 1);
    Tcg_ApplyBGP6OrSGB3ToCardImage(x, y);
}

// the "<Duelist>'s Play Area" screen: duelist's cards, laid out as the layout duelist's side
static void DrawYourOrOppPlayAreaScreen(u32 duelist, u32 layout)
{
    s32 len;
    u32 halfTiles;
    sCheckMenuWhichDuelist = duelist;
    sCheckMenuWhichLayout = layout;
    G->tileMapFill = 0;
    Tcg_ZeroObjectPositions();
    G->vBlankOAMCopyToggle = TRUE;
    Tcg_DoFrame();
    Tcg_EmptyScreen();
    Tcg_Set_OBJ_8x8();
    LoadCursorTile();
    Tcg_LoadSymbolsFont();
    Tcg_LoadDeckAndDiscardPileIcons();
    if (duelist == TCG_PLAYER)
        Tcg_CopyPlayerName(gTcg->defaultText);
    else
        Tcg_CopyOpponentName(gTcg->defaultText);
    len = Tcg_GetTextLengthInTiles(gTcg->defaultText, &halfTiles);
    (void)len;
    Tcg_InitTextPrinting((u8)((u8)(6 - halfTiles) >> 1) + 4, 0);
    if (gTcg->whoseTurn == TCG_PLAYER && duelist != TCG_PLAYER)
    {
        Tcg_SwapTurn();
        Tcg_PrintTextNoDelay(TCG_TEXT_DuelistsPlayArea);
        Tcg_SwapTurn();
    }
    else
    {
        Tcg_PrintTextNoDelay(TCG_TEXT_DuelistsPlayArea);
    }
    if (duelist == layout)
    {
        DrawPlayArea_PrizeCards(sPrizeCardsCoordinateData_YourOrOppPlayArea_Player);
        DrawYourOrOppPlayArea_ActiveCardGfx(6, 2);
        DrawPlayArea_BenchCards(1, 9, 4);
        DrawYourOrOppPlayArea_IconsAt(sPlayAreaIconCoordinates_Player1);
    }
    else
    {
        DrawPlayArea_PrizeCards(sPrizeCardsCoordinateData_YourOrOppPlayArea_Opponent);
        DrawYourOrOppPlayArea_ActiveCardGfx(6, 5);
        DrawPlayArea_BenchCards(1, 2, 4);
        DrawYourOrOppPlayArea_IconsAt(sPlayAreaIconCoordinates_Opponent1);
    }
    Tcg_EnableLCD();
}

// (DrawYourOrOppPlayAreaScreen_Bank0 as TurnDuelistTakePrizes's .Func_588a calls it: the duelist's
// play area in the player's layout, and a text box)
void Tcg_DrawYourOrOppPlayAreaScreen(u32 duelist)
{
    if (gTcg->headless)
        return;
    DrawYourOrOppPlayAreaScreen(duelist, TCG_PLAYER);
    Tcg_DrawWideTextBox();
}

// (Func_82b6) the prizes again (after the AI's taken some)
void Tcg_RedrawPlayAreaPrizeCards(void)
{
    if (gTcg->headless)
        return;
    DrawPlayArea_PrizeCards(sCheckMenuWhichDuelist == sCheckMenuWhichLayout
                            ? sPrizeCardsCoordinateData_YourOrOppPlayArea_Player
                            : sPrizeCardsCoordinateData_YourOrOppPlayArea_Opponent);
}

// ---- the arrows at the menu item's place on the screen -----------------------------------------------------

static const u8 sArrows_PlayerPokemon[] = {5, 5, 0, 10, 4, 10, 8, 10, 12, 10, 16, 10, 0xff};
static const u8 sArrows_PlayerHand[] = {14, 7, 0xff};
static const u8 sArrows_PlayerDiscardPile[] = {14, 5, 0xff};
static const u8 sArrows_OpponentPokemon[] = {5, 7, 0, 3, 4, 3, 8, 3, 12, 3, 16, 3, 0xff};
static const u8 sArrows_OpponentHand[] = {0, 5, 0xff};
static const u8 sArrows_OpponentDiscardPile[] = {0, 8, 0xff};
static const u8 *const sYourOrOppPlayAreaArrowPositions[] =
{
    sArrows_PlayerPokemon, sArrows_PlayerHand, sArrows_PlayerDiscardPile,
    sArrows_OpponentPokemon, sArrows_OpponentHand, sArrows_OpponentDiscardPile,
};

static void DrawArrows(u32 position, u8 tile)
{
    const u8 *p = sYourOrOppPlayAreaArrowPositions[position % 6];
    for (; *p != 0xff; p += 2)
        Tcg_WriteByteToBGMap0(p[0], p[1], tile);
}

static void EraseArrows(void)
{
    DrawArrows(sLastCursorPosition, TCG_SYM_SPACE);
}

static void RefreshArrows(u32 opponent)
{
    u32 pos = sCheckMenuCursorY * 2 + sCheckMenuCursorX + opponent * 3;
    if (pos == sLastCursorPosition)
        return;
    EraseArrows();
    sLastCursorPosition = pos;
    DrawArrows(pos, 0xf8);
}

// ---- the Check menu --------------------------------------------------------------------------------------

static const u8 sCheckMenuData[][4] =
{
    TEXTITEM(2, 14, TCG_TEXT_InPlayArea),
    TEXTITEM(2, 16, TCG_TEXT_YourPlayArea),
    TEXTITEM(12, 14, TCG_TEXT_Glossary),
    TEXTITEM(12, 16, TCG_TEXT_OppPlayArea),
};
static const u8 sYourPlayAreaMenuData[][4] =
{
    TEXTITEM(2, 14, TCG_TEXT_YourPokemon),
    TEXTITEM(12, 14, TCG_TEXT_YourHand),
    TEXTITEM(2, 16, TCG_TEXT_YourDiscardPile2),
};
static const u8 sOppPlayAreaMenuData[][4] =
{
    TEXTITEM(2, 14, TCG_TEXT_OpponentsPokemon),
    TEXTITEM(2, 16, TCG_TEXT_OpponentsDiscardPile2),
};

static void DuelCheckMenu_YourOrOppPlayArea(bool32 opponent)
{
    ResetCheckMenuCursorPositionAndBlink();
    sCe5e = opponent ? 0x80 : 0;     // (no Clairvoyance: the opponent's hand isn't on the menu)
    for (;;)
    {
        bool32 cancel;
        u32 item;
        if (opponent)
            DrawYourOrOppPlayAreaScreen(gTcg->whoseTurn ^ 1, gTcg->whoseTurn);
        else
            DrawYourOrOppPlayAreaScreen(gTcg->whoseTurn, gTcg->whoseTurn);
        sLastCursorPosition = sCheckMenuCursorY * 2 + sCheckMenuCursorX + (opponent ? 3 : 0);
        DrawArrows(sLastCursorPosition, 0xf8);
        Tcg_DrawWideTextBox();
        sCheckMenuCursorBlinkCounter = 0;
        if (opponent)
            PlaceTextItems(sOppPlayAreaMenuData, ARRAY_COUNT(sOppPlayAreaMenuData));
        else
            PlaceTextItems(sYourPlayAreaMenuData, ARRAY_COUNT(sYourPlayAreaMenuData));
        do
        {
            Tcg_DoFrame();
            RefreshArrows(opponent);
        } while (!HandleCheckMenuInput(TRUE, &cancel));
        EraseArrows();
        if (cancel)
            return;
        item = sCheckMenuCursorY * 2 + sCheckMenuCursorX;
        if (opponent)
            Tcg_SwapTurn();
        switch (item)
        {
        case 0:
            Tcg_OpenPlayAreaScreenForViewing();
            break;
        case 1:
            Tcg_OpenTurnHolderHandScreen_Simple();
            break;
        default:
            Tcg_OpenDiscardPileScreen();
            break;
        }
        if (opponent)
            Tcg_SwapTurn();
    }
}

// _OpenDuelCheckMenu: In Play Area, Glossary, Your Play Area, Opp. Play Area
void Tcg_OpenDuelCheckMenu(void)
{
    if (gTcg->headless)
        return;
    for (;;)
    {
        bool32 cancel;
        ResetCheckMenuCursorPositionAndBlink();
        sCe5e = 0;
        Tcg_DrawWideTextBox();
        sCheckMenuCursorBlinkCounter = 0;
        PlaceTextItems(sCheckMenuData, ARRAY_COUNT(sCheckMenuData));
        do
            Tcg_DoFrame();
        while (!HandleCheckMenuInput(FALSE, &cancel));
        if (cancel)
            return;
        switch (sCheckMenuCursorY * 2 + sCheckMenuCursorX)
        {
        case 0:
            Tcg_OpenInPlayAreaScreen();
            break;
        case 1:
            Tcg_OpenGlossaryScreen();
            break;
        case 2:
            DuelCheckMenu_YourOrOppPlayArea(FALSE);
            break;
        default:
            DuelCheckMenu_YourOrOppPlayArea(TRUE);
            break;
        }
    }
}

// ---- the prizes ------------------------------------------------------------------------------------------

// (cursor_transition: x, y, attributes, then the position for up, down, right, left)
static const u8 sPrizeCursorTransitionTable[6][7] =
{
    {0x08, 0x28, 0x00, 0x04, 0x02, 0x01, 0x01},
    {0x30, 0x28, 0x20, 0x05, 0x03, 0x00, 0x00},
    {0x08, 0x38, 0x00, 0x00, 0x04, 0x03, 0x03},
    {0x30, 0x38, 0x20, 0x01, 0x05, 0x02, 0x02},
    {0x08, 0x48, 0x00, 0x02, 0x00, 0x05, 0x05},
    {0x30, 0x48, 0x20, 0x03, 0x01, 0x04, 0x04},
};

static EWRAM_DATA const u8 (*sTransitionTable)[7] = NULL;   // wTransitionTablePtr

static void DrawPrizeCursor(void)
{
    const u8 *t = sTransitionTable[sCurPosition];
    Tcg_ZeroObjectPositions();
    Tcg_SetOneObjectAttributes(t[1], t[0], 0, t[2]);
}

static void ZeroObjectPositionsWithCopyToggleOn(void)
{
    Tcg_ZeroObjectPositions();
    G->vBlankOAMCopyToggle = TRUE;
}

// YourOrOppPlayAreaScreen_HandleInput with the prizes' table: TRUE (the game's carry) on A (the
// position) or B (*cancel)
static bool32 PrizeCursorHandleInput(bool32 *cancel)
{
    u8 sfx = 0;
    for (;;)
    {
        u8 temp = sCurPosition, pad = G->dpadHeld, b;
        const u8 *t = sTransitionTable[temp];
        if (!pad)
            break;
        if (pad & TCG_PAD_UP)
            sCurPosition = t[3];
        else if (pad & TCG_PAD_DOWN)
            sCurPosition = t[4];
        else if (pad & TCG_PAD_RIGHT)
            sCurPosition = t[5];
        else if (pad & TCG_PAD_LEFT)
            sCurPosition = t[6];
        else
            break;
        if (sCurPosition >= 8)
        {
            sfx = TCG_SFX_CURSOR;
            sCheckMenuCursorBlinkCounter = 0;
            break;
        }
        b = 1 << sCurPosition;
        if (sDuelInitialPrizesUpperBitsSet & b)
        {
            sfx = TCG_SFX_CURSOR;
            sCheckMenuCursorBlinkCounter = 0;
            break;
        }
        // (a position past the duel's prizes: the game moves on from it)
        if (temp != 6 || !(pad & (TCG_PAD_LEFT | TCG_PAD_RIGHT)))
            continue;
        if (gTcg->duelInitialPrizes >= 5)
        {
            sfx = TCG_SFX_CURSOR;
            sCheckMenuCursorBlinkCounter = 0;
            break;
        }
        sCurPosition = sCurPosition == 5 ? 3 : 2;
        if (gTcg->duelInitialPrizes < 3)
            sCurPosition -= 2;
        sfx = TCG_SFX_CURSOR;
        sCheckMenuCursorBlinkCounter = 0;
        break;
    }
    if (G->keysPressed & (TCG_PAD_A | TCG_PAD_B))
    {
        *cancel = !(G->keysPressed & TCG_PAD_A);
        if (!*cancel)
            DrawPrizeCursor();
        PlaySFXConfirmOrCancel(*cancel);
        return TRUE;
    }
    if (sfx)
        Tcg_PlaySFX(sfx);
    if ((sCheckMenuCursorBlinkCounter++ & 0xF) == 0)
    {
        if (sCheckMenuCursorBlinkCounter & 0x10)
            ZeroObjectPositionsWithCopyToggleOn();
        else
            DrawPrizeCursor();
    }
    return FALSE;
}

// GetFirstSetPrizeCard: from position `from`, the first prize still there
static u32 GetFirstSetPrizeCard(u32 from)
{
    u8 d = *Tcg_TurnVar(TCG_DUELVARS_PRIZES);
    u32 c = from, e = 6;
    for (;;)
    {
        if (d & (1 << c))
            return c;
        if (--e == 0)
            return 0;
        if (++c == 6)
            c = 0;
    }
}

// _SelectPrizeCards: the player takes `count` prizes, choosing each on their play area screen (its
// page shown); hTempPlayAreaLocation_ffa1.. the cards taken ($ff-ended), hTemp_ffa0 the prizes left
void Tcg_SelectPrizeCards(u32 count)
{
    u8 *list = &gTcg->tempPlayAreaLocationFfa1;
    if (gTcg->headless)
    {
        u32 i;
        for (i = 0; i < 6 && count; i++)
        {
            if (*Tcg_TurnVar(TCG_DUELVARS_PRIZES) & (1 << i))
            {
                Tcg_TakePrizeCard(i);
                count--;
            }
        }
        return;
    }
    sCurPosition = GetFirstSetPrizeCard(0);
    sTransitionTable = sPrizeCursorTransitionTable;
    while (count && *Tcg_TurnVar(TCG_DUELVARS_PRIZES))
    {
        u8 card, b;
        DrawYourOrOppPlayAreaScreen(gTcg->whoseTurn, gTcg->whoseTurn);
        Tcg_DrawWideTextBox();
        Tcg_InitTextPrinting(1, 14);
        Tcg_ProcessTextFromID(TCG_TEXT_PleaseChooseAPrize);
        for (;;)
        {
            bool32 cancel;
            G->vBlankOAMCopyToggle = TRUE;
            Tcg_DoFrame();
            if (!PrizeCursorHandleInput(&cancel) || cancel)
                continue;
            ZeroObjectPositionsWithCopyToggleOn();
            b = 1 << sCurPosition;
            if (*Tcg_TurnVar(TCG_DUELVARS_PRIZES) & b)
                break;
        }
        *Tcg_TurnVar(TCG_DUELVARS_PRIZES) -= b;
        card = *Tcg_TurnVar(TCG_DUELVARS_PRIZE_CARDS + sCurPosition);
        *list++ = card;
        Tcg_AddCardToHand(card);
        Tcg_LoadCard1(card);
        Tcg_Set_OBJ_8x16();
        Tcg_OpenCardPage(TCG_CARDPAGETYPE_NOT_PLAY_AREA, TCG_PAD_B);
        count--;
        sCurPosition = GetFirstSetPrizeCard(sCurPosition);
    }
    gTcg->temp = *Tcg_TurnVar(TCG_DUELVARS_PRIZES);
    *list = 0xFF;
}

// _DrawPlayAreaToPlacePrizeCards: both duelists' benches, icons and the card piles to place the
// prizes from
void Tcg_DrawPlayAreaToPlacePrizeCards(void)
{
    static const u8 sPlayerIconCoordinates[] = {15, 11, 15, 6, 15, 8};
    static const u8 sOppIconCoordinates[] = {0, 0, 0, 4, 0, 2};
    if (gTcg->headless)
        return;
    G->tileMapFill = 0;
    Tcg_ZeroObjectPositions();
    Tcg_EmptyScreen();
    Tcg_LoadSymbolsFont();
    Tcg_LoadPlacingThePrizesScreenTiles();
    sCheckMenuWhichLayout = sCheckMenuWhichDuelist = gTcg->whoseTurn;
    DrawPlayArea_BenchCards(0, 10, 3);
    DrawYourOrOppPlayArea_IconsAt(sPlayerIconCoordinates);
    Tcg_FillRectangle(8, 6, 4, 3, 0xa0, 1, 4);
    Tcg_SwapTurn();
    sCheckMenuWhichDuelist = gTcg->whoseTurn;
    DrawPlayArea_BenchCards(6, 0, 3);
    DrawYourOrOppPlayArea_IconsAt(sOppIconCoordinates);
    Tcg_FillRectangle(8, 3, 4, 3, 0xa0, 1, 4);
    Tcg_SwapTurn();
}

// (HandleDuelSetup's .PlacePrizes) the prizes dealt out one by one, the decks' counts going down
void Tcg_PlacePrizes(void)
{
    static const u8 sPrizeCardCoordinates[][4] =
    {
        {5, 6, 14, 5}, {6, 6, 13, 5}, {5, 7, 14, 4}, {6, 7, 13, 4}, {5, 8, 14, 3}, {6, 8, 13, 3},
    };
    u32 e = TCG_DECK_SIZE - 7 - 1, d;
    if (gTcg->headless)
        return;
    Tcg_EnableLCD();
    for (d = 0; d < gTcg->duelInitialPrizes && d < 6; d++, e--)
    {
        Tcg_DoAFrames(20);
        Tcg_WriteByteToBGMap0(sPrizeCardCoordinates[d][0], sPrizeCardCoordinates[d][1], 0xac);
        Tcg_WriteByteToBGMap0(sPrizeCardCoordinates[d][2], sPrizeCardCoordinates[d][3], 0xac);
        Tcg_PlaySFX(TCG_SFX_PLACE_PRIZE);
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(3, 5, e);
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(18, 7, e);
    }
}

// (DrawYourOrOppPlayAreaScreen_Bank0 as the animations call it: duelist's play area in layout's
// layout, and a text box)
void Tcg_DrawYourOrOppPlayAreaScreen_Layout(u32 duelist, u32 layout)
{
    DrawYourOrOppPlayAreaScreen(duelist, layout);
    Tcg_DrawWideTextBox();
}

// ---- Peek (engine/menus/duel.asm's _HandlePeekSelection, _DrawAIPeekScreen) -------------------------------

// (cursor_transition: x, y, attributes, then the position for up, down, right, left; 6 the hand, 7 the deck)
static const u8 sPeekYourPlayAreaTransitionTable[8][7] =
{
    {0x08, 0x28, 0x00, 0x04, 0x02, 0x01, 0x07},
    {0x30, 0x28, 0x20, 0x05, 0x03, 0x07, 0x00},
    {0x08, 0x38, 0x00, 0x00, 0x04, 0x03, 0x07},
    {0x30, 0x38, 0x20, 0x01, 0x05, 0x07, 0x02},
    {0x08, 0x48, 0x00, 0x02, 0x00, 0x05, 0x07},
    {0x30, 0x48, 0x20, 0x03, 0x01, 0x07, 0x04},
    {0x78, 0x50, 0x00, 0x07, 0x07, 0x00, 0x01},
    {0x78, 0x28, 0x00, 0x07, 0x07, 0x00, 0x01},
};

static const u8 sPeekOppPlayAreaTransitionTable[8][7] =
{
    {0xa0, 0x60, 0x20, 0x02, 0x04, 0x07, 0x01},
    {0x78, 0x60, 0x00, 0x03, 0x05, 0x00, 0x07},
    {0xa0, 0x50, 0x20, 0x04, 0x00, 0x06, 0x03},
    {0x78, 0x50, 0x00, 0x05, 0x01, 0x02, 0x06},
    {0xa0, 0x40, 0x20, 0x00, 0x02, 0x06, 0x05},
    {0x78, 0x40, 0x00, 0x01, 0x03, 0x04, 0x06},
    {0x08, 0x38, 0x00, 0x07, 0x07, 0x05, 0x04},
    {0x08, 0x60, 0x00, 0x06, 0x06, 0x01, 0x00},
};

static const u8 sPeekPlayAreaMenuData[][4] =
{
    TEXTITEM(2, 14, TCG_TEXT_YourPlayArea),
    TEXTITEM(2, 16, TCG_TEXT_OppPlayArea),
};

static const struct TcgMenuParams sPeekPlayAreaMenuParameters = {1, 14, 2, 2, TCG_SYM_CURSOR_R, TCG_SYM_SPACE, NULL};

// (.ShowSelectedCard) the card's page; the AI's param for what was looked at (bit 7: the other side's)
static u32 PeekShowSelectedCard(u8 card, u8 param, bool32 swapPending)
{
    if (param == 0)
        param = card;
    Tcg_LoadCard1(card);
    Tcg_Set_OBJ_8x16();
    Tcg_OpenCardPage(TCG_CARDPAGETYPE_NOT_PLAY_AREA, TCG_PAD_B);
    G->vBlankOAMCopyToggle = TRUE;
    if (swapPending)
    {
        Tcg_SwapTurn();
        return param | 0x80;
    }
    return param;
}

// (_HandlePeekSelection) the player looks at a prize, a hand's random card or a deck's top card, on
// either side's play area screen
u32 Tcg_HandlePeekSelection(void)
{
    bool32 swapPending = FALSE;
    if (gTcg->headless)
        return TCG_AI_PEEK_TARGET_DECK;
    Tcg_Set_OBJ_8x8();
    LoadCursorTile();
    DrawYourOrOppPlayAreaScreen(gTcg->whoseTurn, gTcg->whoseTurn);
    for (;;)
    {
        // .check_swap
        if (swapPending)
        {
            Tcg_SwapTurn();
            swapPending = FALSE;
        }
        // which play area (B does nothing)
        Tcg_InitializeMenuParameters(0, &sPeekPlayAreaMenuParameters);
        Tcg_DrawWideTextBox();
        PlaceTextItems(sPeekPlayAreaMenuData, ARRAY_COUNT(sPeekPlayAreaMenuData));
        for (;;)
        {
            G->waitingFor = TCG_WAIT_CHECK_MENU;
            Tcg_DoFrame();
            if (Tcg_HandleMenuInput() && G->hCurMenuItem != 0xFF)
                break;
        }
        Tcg_EraseCursor();
        if (G->hCurMenuItem == 0)
        {
            if (sCheckMenuWhichDuelist != gTcg->whoseTurn)
                DrawYourOrOppPlayAreaScreen(gTcg->whoseTurn, gTcg->whoseTurn);
            swapPending = FALSE;
            sTransitionTable = sPeekYourPlayAreaTransitionTable;
        }
        else
        {
            if (sCheckMenuWhichDuelist == gTcg->whoseTurn)
                DrawYourOrOppPlayAreaScreen(gTcg->whoseTurn ^ 1, gTcg->whoseTurn);
            sTransitionTable = sPeekOppPlayAreaTransitionTable;
        }
        Tcg_DrawWideTextBox();
        Tcg_InitTextPrinting(1, 14);
        Tcg_ProcessTextFromID(TCG_TEXT_WhichCardWouldYouLikeToSee);
        sCurPosition = 0;
        if (G->hCurMenuItem != 0)
        {
            Tcg_SwapTurn();
            swapPending = TRUE;
        }
        for (;;)
        {
            bool32 cancel;
            u32 pos;
            G->vBlankOAMCopyToggle = TRUE;
            G->waitingFor = TCG_WAIT_PRIZES;
            Tcg_DoFrame();
            if (!PrizeCursorHandleInput(&cancel))
                continue;
            if (cancel)
            {
                ZeroObjectPositionsWithCopyToggleOn();
                break;
            }
            pos = sCurPosition;
            if (pos < 6)
            {
                // .SelectedPrize
                if (!(*Tcg_TurnVar(TCG_DUELVARS_PRIZES) & (1 << pos)))
                    continue;
                return PeekShowSelectedCard(*Tcg_TurnVar(TCG_DUELVARS_PRIZE_CARDS + pos), pos + 0x40, swapPending);
            }
            if (pos == 6)
            {
                // .SelectedOppsHand: a random one
                if (Tcg_CreateHandCardList())
                    continue;
                Tcg_ShuffleCards(gTcg->duelTempList, Tcg_CountCardsInDuelTempList());
                return PeekShowSelectedCard(gTcg->duelTempList[0], 0, swapPending);
            }
            // .SelectedDeck: the top card
            if (Tcg_CreateDeckCardList())
                continue;
            return PeekShowSelectedCard(gTcg->duelTempList[0], 0x7f, swapPending);
        }
    }
}

// (_DrawAIPeekScreen) the play area screen with the cursor on what the AI looked at (`param`: the
// AI's hAIPkmnPowerEffectParam, bit 7 flipped)
void Tcg_DrawAIPeekScreen(u32 param)
{
    bool32 swapPending = FALSE;
    if (gTcg->headless)
        return;
    Tcg_Set_OBJ_8x8();
    LoadCursorTile();
    sTransitionTable = sPeekYourPlayAreaTransitionTable;
    if (param & TCG_AI_PEEK_TARGET_HAND)
    {
        Tcg_SwapTurn();
        swapPending = TRUE;
        sTransitionTable = sPeekOppPlayAreaTransitionTable;
    }
    DrawYourOrOppPlayAreaScreen(gTcg->whoseTurn, swapPending ? gTcg->whoseTurn ^ 1 : gTcg->whoseTurn);
    if ((param & 0x7f) == 0x7f)
        sCurPosition = 7;
    else if (param & TCG_AI_PEEK_TARGET_PRIZE)
        sCurPosition = param & 0x3f;
    else
        sCurPosition = 6;
    DrawPrizeCursor();
    G->vBlankOAMCopyToggle = TRUE;
    if (swapPending)
        Tcg_SwapTurn();
}

// ---- the In Play Area screen (engine/menus/play_area.asm, duel.asm's DrawInPlayAreaScreen) ----------------

enum
{
    INPLAYAREA_PLAYER_BENCH_1, INPLAYAREA_PLAYER_BENCH_2, INPLAYAREA_PLAYER_BENCH_3, INPLAYAREA_PLAYER_BENCH_4,
    INPLAYAREA_PLAYER_BENCH_5, INPLAYAREA_PLAYER_ACTIVE, INPLAYAREA_PLAYER_HAND, INPLAYAREA_PLAYER_DISCARD_PILE,
    INPLAYAREA_OPP_ACTIVE, INPLAYAREA_OPP_HAND, INPLAYAREA_OPP_DISCARD_PILE, INPLAYAREA_OPP_BENCH_1,
    INPLAYAREA_OPP_BENCH_2, INPLAYAREA_OPP_BENCH_3, INPLAYAREA_OPP_BENCH_4, INPLAYAREA_OPP_BENCH_5,
    INPLAYAREA_PLAYER_PLAY_AREA, INPLAYAREA_OPP_PLAY_AREA,
};

#define IPA(x) INPLAYAREA_##x
#define XF 0x20     // (OAM_XFLIP)

// (OpenInPlayAreaScreen_TransitionTable1: no Clairvoyance, as there's no Omanyte)
static const u8 sInPlayAreaTransitionTable[16][7] =
{
    {0x18, 0x8c, 0,  IPA(PLAYER_ACTIVE), IPA(PLAYER_PLAY_AREA), IPA(PLAYER_BENCH_2), IPA(PLAYER_BENCH_5)},
    {0x30, 0x8c, 0,  IPA(PLAYER_ACTIVE), IPA(PLAYER_PLAY_AREA), IPA(PLAYER_BENCH_3), IPA(PLAYER_BENCH_1)},
    {0x48, 0x8c, 0,  IPA(PLAYER_ACTIVE), IPA(PLAYER_PLAY_AREA), IPA(PLAYER_BENCH_4), IPA(PLAYER_BENCH_2)},
    {0x60, 0x8c, 0,  IPA(PLAYER_ACTIVE), IPA(PLAYER_PLAY_AREA), IPA(PLAYER_BENCH_5), IPA(PLAYER_BENCH_3)},
    {0x78, 0x8c, 0,  IPA(PLAYER_ACTIVE), IPA(PLAYER_PLAY_AREA), IPA(PLAYER_BENCH_1), IPA(PLAYER_BENCH_4)},
    {0x30, 0x6c, 0,  IPA(OPP_ACTIVE), IPA(PLAYER_BENCH_1), IPA(PLAYER_DISCARD_PILE), IPA(PLAYER_DISCARD_PILE)},
    {0x78, 0x80, 0,  IPA(PLAYER_DISCARD_PILE), IPA(PLAYER_BENCH_1), IPA(PLAYER_ACTIVE), IPA(PLAYER_ACTIVE)},
    {0x78, 0x70, 0,  IPA(OPP_ACTIVE), IPA(PLAYER_HAND), IPA(PLAYER_ACTIVE), IPA(PLAYER_ACTIVE)},
    {0x78, 0x34, XF, IPA(OPP_BENCH_1), IPA(PLAYER_ACTIVE), IPA(OPP_DISCARD_PILE), IPA(OPP_DISCARD_PILE)},
    {0x30, 0x20, XF, IPA(OPP_BENCH_1), IPA(OPP_DISCARD_PILE), IPA(OPP_ACTIVE), IPA(OPP_ACTIVE)},
    {0x30, 0x38, XF, IPA(OPP_BENCH_1), IPA(PLAYER_ACTIVE), IPA(OPP_ACTIVE), IPA(OPP_ACTIVE)},
    {0x90, 0x14, XF, IPA(OPP_PLAY_AREA), IPA(OPP_ACTIVE), IPA(OPP_BENCH_5), IPA(OPP_BENCH_2)},
    {0x78, 0x14, XF, IPA(OPP_PLAY_AREA), IPA(OPP_ACTIVE), IPA(OPP_BENCH_1), IPA(OPP_BENCH_3)},
    {0x60, 0x14, XF, IPA(OPP_PLAY_AREA), IPA(OPP_ACTIVE), IPA(OPP_BENCH_2), IPA(OPP_BENCH_4)},
    {0x48, 0x14, XF, IPA(OPP_PLAY_AREA), IPA(OPP_ACTIVE), IPA(OPP_BENCH_3), IPA(OPP_BENCH_5)},
    {0x30, 0x14, XF, IPA(OPP_PLAY_AREA), IPA(OPP_ACTIVE), IPA(OPP_BENCH_4), IPA(OPP_BENCH_1)},
};

// (OpenInPlayAreaScreen_TextTable: 0-5 the PLAY_AREA_* whose name's printed, else a text)
static const u16 sInPlayAreaTextTable[16] =
{
    1, 2, 3, 4, 5, 0,
    TCG_TEXT_DuelistHand_2, TCG_TEXT_DuelistDiscardPile,
    0, TCG_TEXT_DuelistHand_2, TCG_TEXT_DuelistDiscardPile,
    1, 2, 3, 4, 5,
};

static const u8 sPrizeCardsCoordinateData_InPlayArea_Player[] = {9, 1, 9, 3, 11, 1, 11, 3, 13, 1, 13, 3};
static const u8 sPrizeCardsCoordinateData_InPlayArea_Opponent[] = {6, 17, 6, 15, 4, 17, 4, 15, 2, 17, 2, 15};
static const u8 sPlayAreaIconCoordinates_Player2[] = {15, 14, 15, 9, 15, 11};
static const u8 sPlayAreaIconCoordinates_Opponent2[] = {0, 2, 0, 6, 0, 4};

static EWRAM_DATA u8 sInPlayAreaFromSelectButton = 0;  // wInPlayAreaFromSelectButton
static EWRAM_DATA u8 sInPlayAreaPreservedPosition = 0; // wInPlayAreaPreservedPosition

// (DrawInPlayArea_Icons) the turn duelist's hand, deck and discard pile counts
static void DrawInPlayArea_Icons(const u8 *coords)
{
    const u8 *v = VARS(gTcg->whoseTurn);
    DrawPlayArea_HandText(coords, v[TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND]);
    DrawPlayArea_IconWithValue(coords + 2, 0xd4, TCG_DECK_SIZE - v[TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK]);
    DrawPlayArea_IconWithValue(coords + 4, 0xd8, v[TCG_DUELVARS_NUMBER_OF_CARDS_IN_DISCARD_PILE]);
}

// (DrawInPlayArea_ActiveCardGfx) both Active Pokémon's pictures
static void DrawInPlayArea_ActiveCardGfx(void)
{
    u8 player = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), opp = *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD);
    if (player != 0xFF)
    {
        Tcg_LoadCard1(player);
        Tcg_LoadLoaded1CardGfx(TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE);
        Tcg_SetBGP6OrSGB3ToCardPalette();
    }
    if (opp != 0xFF)
    {
        Tcg_SwapTurn();
        Tcg_LoadCard1(opp);
        Tcg_LoadLoaded1CardGfx(TCG_V_TILES2 + 0x50 * TCG_TILE_SIZE);
        Tcg_SetBGP7OrSGB2ToCardPalette();
        Tcg_SwapTurn();
    }
    if (player == 0xFF && opp == 0xFF)
        return;
    Tcg_FlushAllPalettesOrSendPal23Packet();
    if (player != 0xFF)
    {
        Tcg_FillRectangle(6, 9, 8, 6, 0xa0, 6, 1);
        Tcg_ApplyBGP6OrSGB3ToCardImage(6, 9);
    }
    if (opp != 0xFF)
    {
        Tcg_SwapTurn();
        Tcg_FillRectangle(6, 2, 8, 6, 0x50, 6, 1);
        Tcg_ApplyBGP7OrSGB2ToCardImage(6, 2);
        Tcg_SwapTurn();
    }
}

// (DrawInPlayAreaScreen) both sides: the opponent's above, the turn holder's below
static void DrawInPlayAreaScreen(void)
{
    G->tileMapFill = 0;
    Tcg_ZeroObjectPositions();
    G->vBlankOAMCopyToggle = TRUE;
    Tcg_DoFrame();
    Tcg_EmptyScreen();
    gTcg->duelDisplayedScreen = TCG_CHECK_PLAY_AREA;
    Tcg_Set_OBJ_8x8();
    LoadCursorTile();
    Tcg_LoadSymbolsFont();
    Tcg_LoadDeckAndDiscardPileIcons();
    Tcg_SetupText(0x80, 0x9f);
    sCheckMenuWhichDuelist = sCheckMenuWhichLayout = gTcg->whoseTurn;
    DrawPlayArea_PrizeCards(sPrizeCardsCoordinateData_InPlayArea_Player);
    DrawPlayArea_BenchCards(3, 15, 3);
    DrawInPlayArea_Icons(sPlayAreaIconCoordinates_Player2);
    sCheckMenuWhichDuelist = gTcg->whoseTurn ^ 1;
    DrawPlayArea_PrizeCards(sPrizeCardsCoordinateData_InPlayArea_Opponent);
    DrawPlayArea_BenchCards(3, 0, 3);
    Tcg_SwapTurn();
    DrawInPlayArea_Icons(sPlayAreaIconCoordinates_Opponent2);
    Tcg_SwapTurn();
    DrawInPlayArea_ActiveCardGfx();
}

static void DrawInPlayAreaCursor(void)
{
    const u8 *t = sInPlayAreaTransitionTable[sCurPosition];
    Tcg_ZeroObjectPositions();
    Tcg_SetOneObjectAttributes(t[1], t[0], 0, t[2]);
}

// (OpenInPlayAreaScreen_HandleInput) TRUE (the game's carry) on A (*cancel FALSE) or B
static bool32 InPlayAreaHandleInput(bool32 *cancel)
{
    u8 pad = G->dpadHeld, sfx = 0;
    const u8 *t = sInPlayAreaTransitionTable[sCurPosition];
    if (pad & (TCG_PAD_UP | TCG_PAD_DOWN | TCG_PAD_RIGHT | TCG_PAD_LEFT))
    {
        u32 a, b;
        if (pad & TCG_PAD_UP)
            a = t[3];
        else if (pad & TCG_PAD_DOWN)
            a = t[4];
        else if (pad & TCG_PAD_RIGHT)
            a = t[5];
        else
            a = t[6];
        sInPlayAreaPreservedPosition = sCurPosition;
        sCurPosition = a;
        if (a < INPLAYAREA_PLAYER_ACTIVE)
        {
            b = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) - 1;
            if (b == 0)
                sCurPosition = INPLAYAREA_PLAYER_PLAY_AREA;
            else if (sCurPosition >= b)
                sCurPosition = (pad & TCG_PAD_RIGHT) ? 0 : b - 1;
        }
        else if (a >= INPLAYAREA_OPP_BENCH_1 && a < INPLAYAREA_PLAYER_PLAY_AREA)
        {
            b = *Tcg_NonTurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) - 1;
            if (b == 0)
                sCurPosition = INPLAYAREA_OPP_PLAY_AREA;
            else if (sCurPosition - INPLAYAREA_OPP_BENCH_1 >= b)
                sCurPosition = (pad & TCG_PAD_LEFT) ? INPLAYAREA_OPP_BENCH_1 : b + INPLAYAREA_OPP_DISCARD_PILE;
        }
        sfx = TCG_SFX_CURSOR;
        sCheckMenuCursorBlinkCounter = 0;
    }
    if (G->keysPressed & (TCG_PAD_A | TCG_PAD_B))
    {
        *cancel = !(G->keysPressed & TCG_PAD_A);
        if (!*cancel)
            DrawInPlayAreaCursor();
        PlaySFXConfirmOrCancel(*cancel);
        return TRUE;
    }
    if (sfx)
        Tcg_PlaySFX(sfx);
    if ((sCheckMenuCursorBlinkCounter++ & 0xF) == 0)
    {
        if (sCheckMenuCursorBlinkCounter & 0x10)
            ZeroObjectPositionsWithCopyToggleOn();
        else if (sCurPosition < 16)
            DrawInPlayAreaCursor();
    }
    return FALSE;
}

// (.print_associated_text) the position's card name, or its hand's or discard pile's text, at 1, 17
static void PrintInPlayAreaAssociatedText(u32 pos)
{
    u16 t;
    Tcg_InitTextPrinting(1, 17);
    Tcg_ProcessTextFromID(TCG_TEXT_EmptyLine);
    G->textFlags = 1;
    Tcg_ProcessTextFromID(TCG_TEXT_Hand_2);
    G->textFlags = 0;
    Tcg_InitTextPrinting(1, 17);
    if (pos >= 16)
        return;
    t = sInPlayAreaTextTable[pos];
    if (t <= 5)
    {
        u8 card;
        if (pos < INPLAYAREA_PLAYER_HAND)
        {
            card = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + t);
            if (card == 0xFF)
                return;
            Tcg_LoadCard1(card);
        }
        else
        {
            card = *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD + t);
            if (card == 0xFF)
                return;
            Tcg_SwapTurn();
            Tcg_LoadCard1(card);
            Tcg_SwapTurn();
        }
        Tcg_CopyCardNameAndLevel();
        Tcg_ProcessText(gTcg->defaultText);
        return;
    }
    if (pos < INPLAYAREA_OPP_ACTIVE)
    {
        Tcg_PrintTextNoDelay(t);
    }
    else
    {
        Tcg_SwapTurn();
        Tcg_PrintTextNoDelay(t);
        Tcg_SwapTurn();
    }
}

// (OpenInPlayAreaScreen_TurnHolderPlayArea, _NonTurnHolderPlayArea) the Pokémon's card page
static void InPlayAreaOpenPokemonPage(u32 pos)
{
    bool32 opp = pos >= INPLAYAREA_OPP_ACTIVE;
    u32 slot;
    u8 card;
    if (!opp)
        slot = pos == INPLAYAREA_PLAYER_ACTIVE ? 0 : pos + 1;
    else
        slot = pos == INPLAYAREA_OPP_ACTIVE ? 0 : pos - INPLAYAREA_OPP_BENCH_1 + 1;
    gTcgCurPlayAreaSlot = slot;
    card = opp ? *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD + slot) : *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + slot);
    if (card == 0xFF)
        return;
    if (opp)
        Tcg_SwapTurn();
    Tcg_LoadCard1(card);
    gTcgCurPlayAreaY = 0;
    Tcg_OpenCardPage_FromCheckPlayArea();
    if (opp)
        Tcg_SwapTurn();
}

static void InPlayAreaSelection(u32 pos)
{
    u32 turn = gTcg->whoseTurn;
    switch (pos)
    {
    case INPLAYAREA_PLAYER_HAND:
        Tcg_OpenTurnHolderHandScreen_Simple();
        break;
    case INPLAYAREA_PLAYER_DISCARD_PILE:
        Tcg_OpenDiscardPileScreen();
        break;
    case INPLAYAREA_OPP_HAND:
        Tcg_OpenNonTurnHolderHandScreen_Simple();
        break;
    case INPLAYAREA_OPP_DISCARD_PILE:
        Tcg_SwapTurn();
        Tcg_OpenDiscardPileScreen();
        Tcg_SwapTurn();
        break;
    default:
        InPlayAreaOpenPokemonPage(pos);
        break;
    }
    gTcg->whoseTurn = turn;
}

// (OpenInPlayAreaScreen) TRUE (the game's carry): B; FALSE: SELECT (from the SELECT button's shortcut)
static bool32 OpenInPlayAreaScreen(void)
{
    sCurPosition = INPLAYAREA_PLAYER_ACTIVE;
    for (;;)
    {
        bool32 restart = FALSE;
        sCheckMenuCursorBlinkCounter = 0;
        DrawInPlayAreaScreen();
        Tcg_EnableLCD();
        PrintInPlayAreaAssociatedText(sCurPosition);
        while (!restart)
        {
            bool32 cancel = FALSE, pressed;
            u32 prev;
            G->vBlankOAMCopyToggle = TRUE;
            G->waitingFor = TCG_WAIT_CHECK_MENU;
            Tcg_DoFrame();
            if (G->dpadHeld & TCG_PAD_START)
                pressed = TRUE;
            else
            {
                if (sInPlayAreaFromSelectButton && (G->dpadHeld & TCG_PAD_SELECT))
                {
                    ZeroObjectPositionsWithCopyToggleOn();
                    Tcg_SetupText(0x38, 0x9f);
                    return FALSE;
                }
                prev = sCurPosition;
                pressed = InPlayAreaHandleInput(&cancel);
                if (!pressed)
                {
                    if (sCurPosition == INPLAYAREA_PLAYER_PLAY_AREA || sCurPosition == INPLAYAREA_OPP_PLAY_AREA)
                    {
                        u32 turn = gTcg->whoseTurn;
                        Tcg_SetupText(0x38, 0x9f);
                        if (sCurPosition == INPLAYAREA_PLAYER_PLAY_AREA)
                            Tcg_OpenPlayAreaScreenForViewing();
                        else
                        {
                            Tcg_SwapTurn();
                            Tcg_OpenPlayAreaScreenForViewing();
                        }
                        gTcg->whoseTurn = turn;
                        sCurPosition = sInPlayAreaPreservedPosition;
                        restart = TRUE;
                        continue;
                    }
                    if (sCurPosition != prev)
                        PrintInPlayAreaAssociatedText(sCurPosition);
                    continue;
                }
                if (cancel)
                {
                    ZeroObjectPositionsWithCopyToggleOn();
                    Tcg_SetupText(0x38, 0x9f);
                    return TRUE;
                }
            }
            // .selection: A or START
            ZeroObjectPositionsWithCopyToggleOn();
            Tcg_SetupText(0x38, 0x9f);
            sInPlayAreaPreservedPosition = sCurPosition;
            InPlayAreaSelection(sCurPosition);
            sCurPosition = sInPlayAreaPreservedPosition;
            restart = TRUE;
        }
    }
}

void Tcg_OpenInPlayAreaScreen(void)
{
    if (gTcg->headless)
        return;
    sInPlayAreaFromSelectButton = FALSE;
    OpenInPlayAreaScreen();
}

// (OpenInPlayAreaScreen_FromSelectButton) TRUE: B
bool32 Tcg_OpenInPlayAreaScreen_FromSelectButton(void)
{
    if (gTcg->headless)
        return TRUE;
    sInPlayAreaFromSelectButton = TRUE;
    return OpenInPlayAreaScreen();
}

// ---- the Glossary (engine/menus/glossary.asm) ---------------------------------------------------------------

static const u8 sGlossaryTransitionTable[10][7] =
{
    {0x08, 0x28, 0x00, 0x04, 0x01, 0x05, 0x05},
    {0x08, 0x38, 0x00, 0x00, 0x02, 0x06, 0x06},
    {0x08, 0x48, 0x00, 0x01, 0x03, 0x07, 0x07},
    {0x08, 0x58, 0x00, 0x02, 0x04, 0x08, 0x08},
    {0x08, 0x68, 0x00, 0x03, 0x00, 0x09, 0x09},
    {0x58, 0x28, 0x00, 0x09, 0x06, 0x00, 0x00},
    {0x58, 0x38, 0x00, 0x05, 0x07, 0x01, 0x01},
    {0x58, 0x48, 0x00, 0x06, 0x08, 0x02, 0x02},
    {0x58, 0x58, 0x00, 0x07, 0x09, 0x03, 0x03},
    {0x58, 0x68, 0x00, 0x08, 0x05, 0x04, 0x04},
};

static const struct { u8 x; u16 title, description; } sGlossaryData[2][9] =
{
    {
        {7, TCG_TEXT_AboutTheDeck, TCG_TEXT_DeckDescription},
        {5, TCG_TEXT_AboutTheDiscardPile, TCG_TEXT_DiscardPileDescription},
        {7, TCG_TEXT_AboutTheHand, TCG_TEXT_HandDescription},
        {6, TCG_TEXT_AboutTheArena, TCG_TEXT_ArenaDescription},
        {6, TCG_TEXT_AboutTheBench, TCG_TEXT_BenchDescription},
        {4, TCG_TEXT_AboutTheActivePokemon, TCG_TEXT_ActivePokemonDescription},
        {5, TCG_TEXT_AboutBenchPokemon, TCG_TEXT_BenchPokemonDescription},
        {7, TCG_TEXT_AboutPrizes, TCG_TEXT_PrizesDescription},
        {5, TCG_TEXT_AboutDamageCounters, TCG_TEXT_DamageCountersDescription},
    },
    {
        {5, TCG_TEXT_AboutEnergyCards, TCG_TEXT_EnergyCardsDescription},
        {5, TCG_TEXT_AboutTrainerCards, TCG_TEXT_TrainerCardsDescription},
        {5, TCG_TEXT_AboutBasicPokemon, TCG_TEXT_BasicPokemonDescription},
        {5, TCG_TEXT_AboutEvolutionCards, TCG_TEXT_EvolutionCardsDescription},
        {6, TCG_TEXT_AboutAttacking, TCG_TEXT_AttackingDescription},
        {5, TCG_TEXT_AboutPokemonPower, TCG_TEXT_PokemonPowerDescription},
        {6, TCG_TEXT_AboutWeakness, TCG_TEXT_WeaknessDescription},
        {6, TCG_TEXT_AboutResistance, TCG_TEXT_ResistanceDescription},
        {6, TCG_TEXT_AboutRetreating, TCG_TEXT_RetreatingDescription},
    },
};

static EWRAM_DATA u8 sGlossaryPageNo = 0;   // wGlossaryPageNo

static void GlossaryPrintMenu(void)
{
    u8 *t = gTcg->defaultText;
    *t++ = TCG_TX_SYMBOL;
    *t++ = TCG_SYM_1 + sGlossaryPageNo;
    *t++ = TCG_TX_SYMBOL;
    *t++ = TCG_SYM_SLASH;
    *t++ = TCG_TX_SYMBOL;
    *t++ = TCG_SYM_2;
    *t = TCG_TX_END;
    Tcg_InitTextPrinting(16, 1);
    Tcg_ProcessText(gTcg->defaultText);
    Tcg_InitTextPrinting(1, 3);
    Tcg_ProcessTextFromID(sGlossaryPageNo ? TCG_TEXT_GlossaryMenuPage2 : TCG_TEXT_GlossaryMenuPage1);
}

static void GlossaryDisplayMenu(void)
{
    G->tileMapFill = 0;
    Tcg_ZeroObjectPositions();
    G->vBlankOAMCopyToggle = TRUE;
    Tcg_DoFrame();
    Tcg_EmptyScreen();
    Tcg_Set_OBJ_8x8();
    LoadCursorTile();
    Tcg_InitTextPrinting(5, 0);
    Tcg_ProcessTextFromID(TCG_TEXT_PokemonCardGlossary);
    GlossaryPrintMenu();
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_ChooseWordAndPressAButton);
}

static void GlossaryPrintDescription(u32 item)
{
    G->tileMapFill = 0;
    Tcg_EmptyScreen();
    Tcg_InitTextPrinting(5, 0);
    Tcg_ProcessTextFromID(TCG_TEXT_PokemonCardGlossary);
    Tcg_DrawRegularTextBox(0, 4, 20, 14);
    Tcg_InitTextPrinting(sGlossaryData[sGlossaryPageNo][item].x, 2);
    Tcg_ProcessTextFromID(sGlossaryData[sGlossaryPageNo][item].title);
    Tcg_InitTextPrinting(1, 5);
    Tcg_SetNoLineSeparation();
    Tcg_ProcessTextFromID(sGlossaryData[sGlossaryPageNo][item].description);
    Tcg_SetOneLineSeparation();
    Tcg_EnableLCD();
    do
    {
        G->waitingFor = TCG_WAIT_TEXT_BOX;
        Tcg_DoFrame();
    } while (!(G->keysPressed & TCG_PAD_B));
    PlaySFXConfirmOrCancel(TRUE);
}

void Tcg_OpenGlossaryScreen(void)
{
    if (gTcg->headless)
        return;
    sGlossaryPageNo = 0;
    GlossaryDisplayMenu();
    sCurPosition = 0;
    sTransitionTable = sGlossaryTransitionTable;
    sDuelInitialPrizesUpperBitsSet = 0xff;
    sCheckMenuCursorBlinkCounter = 0;
    for (;;)
    {
        bool32 cancel;
        G->vBlankOAMCopyToggle = TRUE;
        G->waitingFor = TCG_WAIT_CHECK_MENU;
        Tcg_DoFrame();
        if (G->keysPressed & TCG_PAD_SELECT)
        {
            PlaySFXConfirmOrCancel(FALSE);
        }
        else
        {
            if (!PrizeCursorHandleInput(&cancel))
                continue;
            ZeroObjectPositionsWithCopyToggleOn();
            if (cancel)
                return;
            if (sCurPosition != 9)
            {
                GlossaryPrintDescription(sCurPosition);
                GlossaryDisplayMenu();
                sCheckMenuCursorBlinkCounter = 0;
                continue;
            }
        }
        // .change_page
        sGlossaryPageNo ^= 1;
        GlossaryPrintMenu();
    }
}
