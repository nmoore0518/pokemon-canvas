// Pokémon Canvas: the Trading Card Game duel's menu, box messages and card lists, ported from the
// Game Boy Color game's engine/duel/core.asm and home/tiles.asm (pret/poketcg) under their own names
// (Tcg_ prefixed). The include/tcg_ui.h hooks here do nothing with gTcg->headless (or make the
// stand-in's choice).
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"

#define G gTcgGbc

// ---- home/tiles.asm: the duel's graphics ----------------------------------------------------------

#define DUEL_OTHER(t)   (TCG_GFX(DuelOtherGraphics) + (t) * TCG_TILE_SIZE)
#define CGB_SYMBOLS(t)  (TCG_GFX(DuelCgbSymbolGraphics) + (t) * TCG_TILE_SIZE)

void Tcg_LoadDuelDrawCardsScreenTiles(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x74 * TCG_TILE_SIZE, DUEL_OTHER(0x29), 0x08);
}

void Tcg_LoadCardOrDuelMenuBorderTiles(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x50 * TCG_TILE_SIZE, DUEL_OTHER(0x15), 0x08);
}

void Tcg_LoadCardTypeHeaderTiles(u32 header)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x60 * TCG_TILE_SIZE, TCG_GFX(DuelCardHeaderGraphics) + header * 0x100, 0x10);
}

void Tcg_LoadDuelCardSymbolTiles(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x50 * TCG_TILE_SIZE, CGB_SYMBOLS(0), 0x30);
}

void Tcg_LoadDuelCardSymbolTiles2(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x54 * TCG_TILE_SIZE, CGB_SYMBOLS(0x4), 0xc);
}

void Tcg_LoadDuelFaceDownCardTiles(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x50 * TCG_TILE_SIZE, CGB_SYMBOLS(0x30), 0x10);
}

void Tcg_LoadDuelCheckPokemonScreenTiles(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x50 * TCG_TILE_SIZE, CGB_SYMBOLS(0x30), 0x24);
}

// (LoadPlacingThePrizesScreenTiles falls through to LoadDeckAndDiscardPileIcons)
void Tcg_LoadPlacingThePrizesScreenTiles(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE, DUEL_OTHER(0), 0xd);
    Tcg_LoadDeckAndDiscardPileIcons();
}

void Tcg_LoadDeckAndDiscardPileIcons(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x50 * TCG_TILE_SIZE, CGB_SYMBOLS(0x54), 0x30);
}

void Tcg_LoadDuelCoinTossResultTiles(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES2 + 0x30 * TCG_TILE_SIZE, DUEL_OTHER(0xd), 0x8);
}

// a 10x4 box message (BOXMSG_*) at 5, 4
void Tcg_DrawDuelBoxMessage(u32 boxMessage)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE,
                                     TCG_GFX(DuelBoxMessages) + boxMessage * 40 * TCG_TILE_SIZE, 40);
    Tcg_FillRectangle(5, 4, 10, 4, 0xa0, 1, 10);
}

void Tcg_DisplayDuelBoxMessage(u32 boxMessage)
{
    if (gTcg->headless)
        return;
    Tcg_EmptyScreen();
    Tcg_DrawDuelBoxMessage(boxMessage);
}

void Tcg_DisplayDuelistTurnScreen(void)
{
    if (gTcg->headless)
        return;
    Tcg_EmptyScreen();
    Tcg_DrawDuelBoxMessage(gTcg->whoseTurn == TCG_PLAYER ? TCG_BOXMSG_PLAYERS_TURN : TCG_BOXMSG_OPPONENTS_TURN);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_DuelistTurn);
}

// ---- the duel menu -----------------------------------------------------------------------------------

static const struct { u8 x, y; u16 text; } sDuelMenuData[] =
{
    { 3, 14, TCG_TEXT_Hand},
    { 9, 14, TCG_TEXT_Check},
    {15, 14, TCG_TEXT_Retreat},
    { 3, 16, TCG_TEXT_Attack},
    { 9, 16, TCG_TEXT_PKMNPower},
    {15, 16, TCG_TEXT_Done},
};

static void OpenNonTurnHolderPlayAreaScreen(void)
{
    Tcg_SwapTurn();
    Tcg_OpenPlayAreaScreenForViewing();
    Tcg_SwapTurn();
}

static bool32 OpenNonTurnHolderDiscardPileScreen(void)
{
    bool32 r;
    Tcg_SwapTurn();
    r = Tcg_OpenDiscardPileScreen();
    Tcg_SwapTurn();
    return r;
}

// PrintDuelMenuAndHandleInput: the duel menu, its shortcuts (B + a direction: a play area or discard
// pile; START, SELECT: the Active Pokémon), until an item's chosen (TCG_DUELMENU_*)
u32 Tcg_HandleDuelMenuInput(void)
{
    bool32 redrawScene = FALSE;
    if (gTcg->headless)
        return TCG_DUELMENU_DONE;
    for (;;)
    {
        u32 i;
        if (redrawScene)
            Tcg_DrawDuelMainScene();    // (DuelMainInterface)
        redrawScene = TRUE;
        Tcg_DrawWideTextBox();
        for (i = 0; i < ARRAY_COUNT(sDuelMenuData); i++)
            Tcg_InitTextPrinting_ProcessTextFromID(sDuelMenuData[i].x, sDuelMenuData[i].y, sDuelMenuData[i].text);
        Tcg_SetMenuItem(G->currentDuelMenuItem);
        for (;;)
        {
            u8 pressed;
            G->waitingFor = TCG_WAIT_DUEL_MENU;
            Tcg_DoFrame();
            pressed = G->keysPressed;
            if (G->keysHeld & TCG_PAD_B)
            {
                if (pressed & TCG_PAD_UP)
                {
                    OpenNonTurnHolderPlayAreaScreen();
                    break;
                }
                if (pressed & TCG_PAD_DOWN)
                {
                    Tcg_OpenPlayAreaScreenForViewing();
                    break;
                }
                if (pressed & TCG_PAD_LEFT)
                {
                    redrawScene = !Tcg_OpenDiscardPileScreen();
                    break;
                }
                if (pressed & TCG_PAD_RIGHT)
                {
                    redrawScene = !OpenNonTurnHolderDiscardPileScreen();
                    break;
                }
                if (pressed & TCG_PAD_START)
                {
                    Tcg_SwapTurn();
                    Tcg_OpenActivePokemonScreen();
                    Tcg_SwapTurn();
                    break;
                }
            }
            if (pressed & TCG_PAD_START)
            {
                Tcg_OpenActivePokemonScreen();
                break;
            }
            if (pressed & TCG_PAD_SELECT)
            {
                // (DuelMenuShortcut_BothActivePokemon: the play areas, the player's from the SELECT presses)
                Tcg_OpenPlayAreaScreenForViewing();
                break;
            }
            if (Tcg_HandleDuelMenuInputStep())
            {
                G->currentDuelMenuItem = G->curMenuItem;
                return G->hCurMenuItem;
            }
            G->currentDuelMenuItem = G->curMenuItem;
        }
    }
}

// ---- card lists ------------------------------------------------------------------------------------------

static void PrintSortNumberInCardList(void)
{
    const u8 *hl = gTcg->duelTempList + 10;
    u32 y = 2;
    for (; *hl != 0xFF; hl++, y += 2)
        Tcg_WriteByteToBGMap0(1, y, *hl ? *hl + TCG_SYM_0 : TCG_SYM_SPACE);
}

// DrawCardListScreenLayout: the list's box, the selected card's picture at 12, 12. TRUE: the list's empty
bool32 Tcg_DrawCardListScreenLayout(void)
{
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    Tcg_LoadSymbolsFont();
    Tcg_LoadDuelCardSymbolTiles();
    Tcg_DrawRegularTextBox(0, 0, 20, 13);
    Tcg_FillRectangle(12, 12, 8, 6, 0xa0, 6, 1);
    Tcg_ApplyBGP6OrSGB3ToCardImage(12, 12);
    if (G->printSortNumberInCardList)
        PrintSortNumberInCardList();
    return gTcg->duelTempList[0] == 0xFF;
}

static void InitAndDrawCardListScreenLayout(void)
{
    gTcg->selectedDuelSubMenuItem = 0;
    G->selectedDuelSubMenuScroll = 0;
    G->sortCardListByID = FALSE;
    G->printSortNumberInCardList = FALSE;
    G->cardListItemSelectionMenuType = 0;
    G->noItemSelectionMenuKeys = TCG_PAD_START;
    G->cardListInfoBoxText = TCG_TEXT_PleaseSelectHand;
    G->cardListHeaderText = TCG_TEXT_DuelistHand;
    Tcg_DrawCardListScreenLayout();
}

void Tcg_InitAndDrawCardListScreenLayout(bool32 withSelectCheckMenu)
{
    if (gTcg->headless)
        return;
    InitAndDrawCardListScreenLayout();
    if (withSelectCheckMenu)
        G->cardListItemSelectionMenuType = TCG_SELECT_CHECK;
}

// (SetCardListHeaderText: hl the info box's, de the header's)
void Tcg_SetCardListHeaderText(u16 infoBoxText, u16 headerText)
{
    if (gTcg->headless)
        return;
    G->cardListHeaderText = headerText;
    G->cardListInfoBoxText = infoBoxText;
}

void Tcg_SetCardListInfoBoxText(u16 textId)
{
    if (gTcg->headless)
        return;
    G->cardListInfoBoxText = textId;
}

void Tcg_PrintCardListHeaderAndInfoBoxTexts(void)
{
    u8 x = 1, y = 14;
    Tcg_AdjustCoordinatesForBGScroll(&x, &y);
    Tcg_InitTextPrinting(x, y);
    Tcg_PrintTextNoDelay(G->cardListInfoBoxText);
    Tcg_InitTextPrinting(1, 1);
    Tcg_PrintTextNoDelay(G->cardListHeaderText);
}

// (GetCardInDuelTempList: hTempCardIndex_ff98)
static u32 GetCardInDuelTempList(u32 item)
{
    gTcg->tempCardIndex = gTcg->duelTempList[item];
    return gTcg->tempCardIndex;
}

static void LoadSelectedCardGfx(void)
{
    Tcg_LoadCard1(GetCardInDuelTempList(G->hCurMenuItem));
    Tcg_LoadLoaded1CardGfx(TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE);
    Tcg_SetBGP6OrSGB3ToCardPalette();
    Tcg_FlushAllPalettesOrSendPal23Packet();
}

static bool32 CardListFunction(u32 item)
{
    if (G->keysPressed & TCG_PAD_B)
    {
        G->hCurMenuItem = 0xFF;
        return TRUE;
    }
    if (G->keysPressed & (TCG_PAD_A | TCG_PAD_SELECT | TCG_PAD_START))
        return TRUE;
    if (G->keysReleased & TCG_PAD_CTRL_PAD)
        LoadSelectedCardGfx();
    return FALSE;
}

static const struct TcgCardListParams sCardListParameters = {1, 3, 4, 14, 5, TCG_SYM_CURSOR_R, TCG_SYM_SPACE, CardListFunction};
static const struct TcgMenuParams sItemSelectionMenuParameters = {1, 14, 2, 2, TCG_SYM_CURSOR_R, TCG_SYM_SPACE, NULL};

// CardListItemSelectionMenu: Play/Check or Select/Check for the card at hTempCardIndex_ff98. TRUE (the
// game's carry): not chosen (B, or Check: the card page seen)
static bool32 CardListItemSelectionMenu(void)
{
    u16 text;
    if (G->cardListItemSelectionMenuType == 0)
        return FALSE;
    text = TCG_TEXT_SelectCheck;
    if (G->cardListItemSelectionMenuType == TCG_PLAY_CHECK)
    {
        Tcg_LoadCard1(gTcg->tempCardIndex);
        text = gTcg->loadedCard1->type == TCG_TYPE_TRAINER ? TCG_TEXT_PlayCheck1 : TCG_TEXT_PlayCheck2;
    }
    Tcg_DrawNarrowTextBox_PrintTextNoDelay(text);
    Tcg_InitializeMenuParameters(0, &sItemSelectionMenuParameters);
    do
        Tcg_DoFrame();
    while (!Tcg_HandleMenuInput());
    if (G->hCurMenuItem == 0xFF)
        return TRUE;
    if (G->hCurMenuItem == 0)
        return FALSE;
    Tcg_LoadCard1(gTcg->tempCardIndex);
    Tcg_OpenCardPage(TCG_CARDPAGETYPE_NOT_PLAY_AREA, TCG_PAD_B);
    Tcg_DrawCardListScreenLayout();
    return TRUE;
}

// DisplayCardList: wDuelTempList's cards, until one's chosen (its deck index, hTempCardIndex_ff98)
// or B (TCG_SELECTION_CANCEL). SELECT sorts it; START (and wNoItemSelectionMenuKeys) a card's page.
static u32 DisplayCardList(void)
{
    for (;;)
    {
        bool32 reload = TRUE;
        Tcg_DrawNarrowTextBox();
        Tcg_PrintCardListHeaderAndInfoBoxTexts();
        for (;;)
        {
            u8 b;
            G->waitingFor = TCG_WAIT_CARD_LIST;
            if (reload)
            {
                Tcg_PrintCardListItems(Tcg_CountCardsInDuelTempList(), G->selectedDuelSubMenuScroll,
                                       gTcg->selectedDuelSubMenuItem, &sCardListParameters);
                LoadSelectedCardGfx();
                Tcg_EnableLCD();
                reload = FALSE;
            }
            Tcg_DoFrame();
            if (G->dpadHeld & TCG_PAD_CTRL_PAD)
            {
                // (.UpdateListOnDPadInput: the texts' tiles kept in the list)
                G->textFlags = 1;
                Tcg_PrintCardListHeaderAndInfoBoxTexts();
                G->textFlags = 0;
            }
            if (!Tcg_HandleCardListInput())
                continue;
            gTcg->selectedDuelSubMenuItem = G->curMenuItem;
            G->selectedDuelSubMenuScroll = G->listScrollOffset;
            b = G->keysPressed;
            if (b & TCG_PAD_SELECT)
            {
                if (G->sortCardListByID)
                    continue;
                Tcg_SortCardsInDuelTempListByID();
                gTcg->selectedDuelSubMenuItem = 0;
                G->selectedDuelSubMenuScroll = 0;
                G->sortCardListByID = TRUE;
                Tcg_EraseCursor();
                reload = TRUE;
                continue;
            }
            if (b & TCG_PAD_B)
                return TCG_SELECTION_CANCEL;
            if (G->noItemSelectionMenuKeys & b)
            {
                // .open_card_page: up and down move through the list
                for (;;)
                {
                    Tcg_LoadCard1(GetCardInDuelTempList(G->hCurMenuItem));
                    Tcg_OpenCardPage_FromCheckHandOrDiscardPile();
                    if (G->dpadHeld & TCG_PAD_UP)
                    {
                        if (G->hCurMenuItem == 0)
                            continue;
                        G->hCurMenuItem--;
                    }
                    else if (G->dpadHeld & TCG_PAD_DOWN)
                    {
                        if (G->hCurMenuItem + 1 >= Tcg_CountCardsInDuelTempList())
                            continue;
                        G->hCurMenuItem++;
                    }
                    else
                        break;
                    gTcg->selectedDuelSubMenuItem = 0;
                    G->selectedDuelSubMenuScroll = G->hCurMenuItem;
                }
                Tcg_DrawCardListScreenLayout();
                break;
            }
            GetCardInDuelTempList(G->hCurMenuItem);
            if (CardListItemSelectionMenu())
                break;
            return gTcg->tempCardIndex;
        }
    }
}

u32 Tcg_DisplayCardList(void)
{
    if (gTcg->headless)
        return gTcg->duelTempList[0] == 0xFF ? TCG_SELECTION_CANCEL : gTcg->duelTempList[0];
    return DisplayCardList();
}

// the hand's list (the duel menu's Hand), with Play/Check
u32 Tcg_DisplayHandCardList(u16 textId)
{
    u32 card;
    if (gTcg->headless)
        return TCG_SELECTION_CANCEL;
    InitAndDrawCardListScreenLayout();
    G->cardListInfoBoxText = textId;
    G->cardListItemSelectionMenuType = TCG_PLAY_CHECK;
    card = DisplayCardList();
    if (G->sortCardListByID)
        Tcg_SortHandCardsByID();
    return card;
}

u32 Tcg_ReloadHandCardList(void)
{
    u32 card;
    if (gTcg->headless)
        return TCG_SELECTION_CANCEL;
    Tcg_DrawCardListScreenLayout();
    card = DisplayCardList();
    if (G->sortCardListByID)
        Tcg_SortHandCardsByID();
    return card;
}

// the Basic Pokémon to place at the start: the Active (bench FALSE), or Benched (B: none)
u32 Tcg_DisplayPlaceInitialPokemonCardsScreen(bool32 bench, u16 textId)
{
    u32 card;
    if (gTcg->headless)
    {
        u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);
        if (bench)
            return TCG_SELECTION_CANCEL;
        for (i = 0; i < n; i++)
        {
            Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_HAND + i));
            if (gTcg->loadedCard1->type < TCG_TYPE_ENERGY && gTcg->loadedCard1->stage == TCG_BASIC)
                return *Tcg_TurnVar(TCG_DUELVARS_HAND + i);
        }
        return TCG_SELECTION_CANCEL;
    }
    G->placingInitialBenchPokemon = bench;
    Tcg_CreateHandCardList();
    InitAndDrawCardListScreenLayout();
    G->cardListInfoBoxText = textId;
    G->cardListItemSelectionMenuType = TCG_PLAY_CHECK;
    for (;;)
    {
        card = DisplayCardList();
        if (card == TCG_SELECTION_CANCEL)
        {
            if (!bench)
                continue;
            break;
        }
        Tcg_LoadCard1(card);
        if (gTcg->loadedCard1->type < TCG_TYPE_ENERGY && gTcg->loadedCard1->stage == TCG_BASIC)
            break;
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_YouCannotSelectThisCard);
        Tcg_DrawCardListScreenLayout();
    }
    if (G->sortCardListByID)
        Tcg_SortHandCardsByID();
    return card;
}

// OpenDiscardPileScreen: TRUE (the game's carry) when it's empty
bool32 Tcg_OpenDiscardPileScreen(void)
{
    if (Tcg_CreateDiscardPileCardList())
    {
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_TheDiscardPileHasNoCards);
        return TRUE;
    }
    InitAndDrawCardListScreenLayout();
    G->cardListHeaderText = gTcg->whoseTurn == TCG_PLAYER ? TCG_TEXT_YourDiscardPile : TCG_TEXT_OpponentsDiscardPile;
    G->cardListInfoBoxText = TCG_TEXT_ChooseTheCardYouWishToExamine;
    G->noItemSelectionMenuKeys = TCG_PAD_START | TCG_PAD_A;
    DisplayCardList();
    return FALSE;
}

void Tcg_OpenTurnHolderHandScreen_Simple(void)
{
    if (Tcg_CreateHandCardList())
    {
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_NoCardsInHand);
        return;
    }
    InitAndDrawCardListScreenLayout();
    G->noItemSelectionMenuKeys = TCG_PAD_START | TCG_PAD_A;
    DisplayCardList();
}

void Tcg_OpenNonTurnHolderHandScreen_Simple(void)
{
    Tcg_SwapTurn();
    Tcg_OpenTurnHolderHandScreen_Simple();
    Tcg_SwapTurn();
}

// (DisplayCardListDetails) the cards in wDuelTempList: "The card you received" / "You received
// these cards"
void Tcg_DisplayCardListDetails(void)
{
    if (gTcg->headless || gTcg->duelTempList[0] == 0xff)
        return;
    InitAndDrawCardListScreenLayout();
    Tcg_PrintCardListItems(Tcg_CountCardsInDuelTempList(), 0, 0, &sCardListParameters);
    Tcg_InitTextPrinting(1, 1);
    Tcg_PrintTextNoDelay(TCG_TEXT_TheCardYouReceived);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_YouReceivedTheseCards);
}
