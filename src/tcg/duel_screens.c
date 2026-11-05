// Pokémon Canvas: the Trading Card Game duel's screens, ported from the Game Boy Color game's
// engine/duel/core.asm (pret/poketcg) under its own names (Tcg_ prefixed), drawn on the emulated
// GBC display (src/tcg/gbc.c). These are the hooks the rules code calls (include/tcg_ui.h): with
// gTcg->headless they do nothing (or make src/tcg/ui.c's stand-in choice).
// So far: SetupDuel, the main scene (DrawDuelMainScene, DrawDuelHUDs), the card graphics and
// palettes it uses.
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"
#include "constants/rgb.h"

#define G gTcgGbc

// ---- engine/duel/core.asm: graphics and palettes -------------------------------------------------

void Tcg_ZeroObjectPositionsAndToggleOAMCopy(void)
{
    Tcg_ZeroObjectPositions();
    G->vBlankOAMCopyToggle = TRUE;
}

// (home/tiles.asm) the symbol font at v0Tiles2: the text's TX_SYMBOL tiles
void Tcg_LoadSymbolsFont(void)
{
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES2, TCG_GFX(SymbolsFont),
                                     (TCG_GFX_OFFSET_DuelCardHeaderGraphics - TCG_GFX_OFFSET_SymbolsFont) / TCG_TILE_SIZE);
}

static const u16 sCGBDefaultPalettes[5][4] =
{
    {RGB(28, 28, 24), RGB(21, 21, 16), RGB(10, 10, 8), RGB(0, 0, 0)},
    {RGB(28, 28, 24), RGB(30, 29, 0), RGB(30, 3, 0), RGB(0, 0, 0)},
    {RGB(28, 28, 24), RGB(0, 18, 0), RGB(12, 11, 20), RGB(0, 0, 0)},
    {RGB(28, 28, 24), RGB(22, 0, 22), RGB(27, 7, 3), RGB(0, 0, 0)},
    {RGB(28, 28, 24), RGB(26, 10, 0), RGB(28, 0, 0), RGB(0, 0, 0)},
};

void Tcg_SetDefaultConsolePalettes(void)
{
    G->textBoxFrameType = 0x04;
    memcpy(G->bgPals, sCGBDefaultPalettes, sizeof(sCGBDefaultPalettes));
    memcpy(G->objPals[0], sCGBDefaultPalettes[0], sizeof(G->objPals[0]));
    Tcg_FlushAllPalettes();
}

// (home/card_data.asm) a card's art ($30 tiles) to dest, its palette to wCardPalette
void Tcg_LoadCardGfx(u32 gfx, u32 dest)
{
    const u8 *g = gTcgCardGfx[gfx];
    Tcg_CopyGfxData(dest, g, 0x30, TCG_TILE_SIZE);
    memcpy(G->cardPalette, g + 0x30 * TCG_TILE_SIZE, sizeof(G->cardPalette));
}

// (LoadLoaded1CardGfx)
void Tcg_LoadLoaded1CardGfx(u32 dest)
{
    Tcg_LoadCardGfx(gTcg->loadedCard1->gfx, dest);
}

// (LoadPlayAreaCardGfx) the card at deckIndex's art (none for $ff)
void Tcg_LoadPlayAreaCardGfx(u32 deckIndex, u32 dest)
{
    if (deckIndex == 0xFF)
        return;
    Tcg_LoadCard1(deckIndex);
    Tcg_LoadLoaded1CardGfx(dest);
}

// (CopyCGBCardPalette) wCardPalette to CGB palette `pal` (0-7 BG, 8-15 OBJ)
static void CopyCGBCardPalette(u32 pal)
{
    u16 *p = pal < 8 ? G->bgPals[pal] : G->objPals[pal - 8];
    memcpy(p, G->cardPalette, sizeof(G->cardPalette));
}

void Tcg_SetBGP7OrSGB2ToCardPalette(void)
{
    CopyCGBCardPalette(7);
}

void Tcg_SetBGP6OrSGB3ToCardPalette(void)
{
    CopyCGBCardPalette(6);
}

void Tcg_SetOBP1OrSGB3ToCardPalette(void)
{
    CopyCGBCardPalette(9);
}

void Tcg_FlushAllPalettesOrSendPal23Packet(void)
{
    Tcg_FlushAllPalettes();
}

// (ApplyCardCGBAttributes) the 8x6 card image at x, y in palette `pal`
static void ApplyCardCGBAttributes(u32 x, u32 y, u8 pal)
{
    Tcg_BankswitchVRAM1();
    Tcg_FillRectangle(x, y, 8, 6, pal, 0, 0);
    Tcg_BankswitchVRAM0();
}

void Tcg_ApplyBGP6OrSGB3ToCardImage(u32 x, u32 y)
{
    ApplyCardCGBAttributes(x, y, 6);
}

void Tcg_ApplyBGP7OrSGB2ToCardImage(u32 x, u32 y)
{
    ApplyCardCGBAttributes(x, y, 7);
}

// ---- SetupDuel ------------------------------------------------------------------------------------

void Tcg_SetupDuel(void)
{
    if (gTcg->headless)
        return;
    G->tileMapFill = TCG_SYM_SPACE;
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    Tcg_LoadSymbolsFont();
    Tcg_SetDefaultConsolePalettes();
    Tcg_SetupText(0x38, 0x9f);
    Tcg_EnableLCD();
}

// ---- the main scene ------------------------------------------------------------------------------------

static const u8 sDuelEAndHPTileData[] =
{
    1, 1, TCG_SYM_E, 0,
    1, 2, TCG_SYM_HP, 0,
    9, 8, TCG_SYM_E, 0,
    9, 9, TCG_SYM_HP, 0,
    0xff
};

static const u8 sDuelHorizontalSeparatorTileData[] =
{
    0, 4, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x31, 0x32, 0,
    9, 5, 0x33, 0x34, 0,
    9, 6, 0x33, 0x34, 0,
    9, 7, 0x35, 0x36, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0,
    0xff
};

static const u8 sDuelHorizontalSeparatorCGBPalData[] =
{
    0, 4, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0,
    9, 5, 0x02, 0x02, 0,
    9, 6, 0x02, 0x02, 0,
    9, 7, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0,
    0xff
};

void Tcg_DrawDuelHorizontalSeparator(void)
{
    Tcg_WriteDataBlocksToBGMap0(sDuelHorizontalSeparatorTileData);
    Tcg_BankswitchVRAM1();
    Tcg_WriteDataBlocksToBGMap0(sDuelHorizontalSeparatorCGBPalData);
    Tcg_BankswitchVRAM0();
}

// (CheckPrintCnfSlpPrz, CheckPrintPoisoned, CheckPrintDoublePoisoned)
static void CheckPrintCnfSlpPrz(u32 x, u32 y, u8 status)
{
    static const u8 sStatusSymbols[] = {TCG_SYM_SPACE, TCG_SYM_CONFUSED, TCG_SYM_ASLEEP, TCG_SYM_PARALYZED};
    Tcg_WriteByteToBGMap0(x, y, sStatusSymbols[status & TCG_CNF_SLP_PRZ & 3]);
}

static void CheckPrintPoisoned(u32 x, u32 y, u8 status)
{
    Tcg_WriteByteToBGMap0(x, y, (status & TCG_POISONED) ? TCG_SYM_POISONED : TCG_SYM_SPACE);
}

static void CheckPrintDoublePoisoned(u32 x, u32 y, u8 status)
{
    Tcg_WriteByteToBGMap0(x, y, (status & (TCG_DOUBLE_POISONED & ~TCG_POISONED)) ? TCG_SYM_POISONED : TCG_SYM_SPACE);
}

// the energy symbols attached to the turn holder's Pokémon at `location`, at x, y (8 at most; a +
// in the last place for 9 or more)
void Tcg_PrintPlayAreaCardAttachedEnergies(u32 x, u32 y, u32 location)
{
    u8 *hl = gTcg->defaultText;
    u32 type, n;
    Tcg_GetPlayAreaCardAttachedEnergies(location);
    memset(hl, TCG_SYM_SPACE, 8);
    for (type = 0; type < TCG_NUM_TYPES - 1; type++)
        for (n = gTcg->attachedEnergies[type]; n && hl < gTcg->defaultText + 8; n--)
            *hl++ = TCG_SYM_FIRE + type;
    if (gTcg->totalAttachedEnergies >= 9)
        gTcg->defaultText[7] = TCG_SYM_PLUS;
    Tcg_CopyDataToBGMap0(x, y, gTcg->defaultText, 8);
}

// (DrawHPBar) the HP bar's tiles in wDefaultText: maxHP's counters, the damage's crossed out
void Tcg_DrawHPBar(u32 maxHP, u32 hp)
{
    static const u8 sFill[3] = {TCG_SYM_SPACE, TCG_SYM_HP_OK, TCG_SYM_HP_NOK};
    s32 amount[3];
    u32 i;
    amount[0] = TCG_MAX_HP;
    amount[1] = maxHP;
    amount[2] = maxHP - hp;
    for (i = 0; i < 3; i++)
    {
        s32 a = amount[i];
        u8 *hl = gTcg->defaultText;
        u32 b = TCG_HP_BAR_LENGTH;
        if (a == 0)
            continue;
        do
        {
            *hl++ = sFill[i];
            if (--b == 0)
                break;
            a -= TCG_MAX_HP / TCG_HP_BAR_LENGTH;
        } while (a != 0);
    }
}

// one duelist's HUD: the Pokémon and prize counts (at x 1 or 15), the Active Pokémon's colour and
// name and level at nameX, nameY, its energies and HP bar at barX, barY, PlusPower and Defender
static void DrawDuelHUD(u32 nameX, u32 nameY, u32 barX, u32 barY)
{
    u32 x = nameY ? 15 : 1, arena;
    s32 len;
    Tcg_WriteByteToBGMap0(x, nameY, TCG_SYM_POKEMON);
    Tcg_WriteByteToBGMap0(x + 1, nameY, *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) + TCG_SYM_0 - 1);
    Tcg_WriteByteToBGMap0(x + 2, nameY, TCG_SYM_PRIZE);
    Tcg_WriteByteToBGMap0(x + 3, nameY, Tcg_CountPrizes() + TCG_SYM_0);
    arena = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
    if (arena == 0xFF)
        return;
    Tcg_LoadCard1(arena);
    *Tcg_CopyCardNameAndLevelLength(32) = TCG_TX_END;
    if (nameY == 0)
    {
        // the opponent's: right-aligned
        len = Tcg_GetTextLengthInTiles(gTcg->defaultText, NULL);
        nameX = (u8)(len + TCG_SCREEN_WIDTH);
    }
    Tcg_InitTextPrinting(nameX, nameY);
    Tcg_ProcessText(gTcg->defaultText);
    Tcg_WriteByteToBGMap0(nameX - 1, nameY, Tcg_GetArenaCardColor() + 1);
    Tcg_PrintPlayAreaCardAttachedEnergies(barX, barY, TCG_PLAY_AREA_ARENA);
    Tcg_LoadCard1(arena);
    Tcg_DrawHPBar(gTcg->loadedCard1->hp, *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP));
    Tcg_CopyDataToBGMap0(barX, barY + 1, gTcg->defaultText, TCG_HP_BAR_LENGTH / 2);
    Tcg_CopyDataToBGMap0(barX, barY + 2, gTcg->defaultText + TCG_HP_BAR_LENGTH / 2, TCG_HP_BAR_LENGTH / 2);
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_PLUSPOWER))
    {
        Tcg_WriteByteToBGMap0(barX + 6, barY + 1, TCG_SYM_PLUSPOWER);
        Tcg_WriteByteToBGMap0(barX + 7, barY + 1, *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_PLUSPOWER) + TCG_SYM_0);
    }
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_DEFENDER))
    {
        Tcg_WriteByteToBGMap0(barX + 6, barY + 2, TCG_SYM_DEFENDER);
        Tcg_WriteByteToBGMap0(barX + 7, barY + 2, *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_DEFENDER) + TCG_SYM_0);
    }
}

// (the player's side at the bottom: while the AI's turn, drawn from the player's)
static void DrawDuelHUDs_Draw(void)
{
    u8 status;
    DrawDuelHUD(1, 11, 11, 8);
    status = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
    CheckPrintCnfSlpPrz(8, 5, status);
    CheckPrintPoisoned(9, 5, status);
    CheckPrintDoublePoisoned(10, 5, status);
    Tcg_SwapTurn();
    DrawDuelHUD(7, 0, 3, 1);
    status = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
    CheckPrintCnfSlpPrz(11, 6, status);
    CheckPrintPoisoned(10, 6, status);
    CheckPrintDoublePoisoned(9, 6, status);
    Tcg_SwapTurn();
}

#define AS_PLAYER(f)                                                                   \
    do {                                                                               \
        if (*Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE) == TCG_DUELIST_TYPE_PLAYER)        \
            f();                                                                       \
        else                                                                           \
        {                                                                              \
            u8 turn = gTcg->whoseTurn;                                                 \
            gTcg->whoseTurn = TCG_PLAYER;                                              \
            f();                                                                       \
            gTcg->whoseTurn = turn;                                                    \
        }                                                                              \
    } while (0)

void Tcg_DrawDuelHUDs(void)
{
    if (gTcg->headless)
        return;
    AS_PLAYER(DrawDuelHUDs_Draw);
}

void Tcg_DrawDuelHUDsIfMainScene(void)
{
    if (gTcg->headless)
        return;
    if (gTcg->duelDisplayedScreen == TCG_DUEL_MAIN_SCENE)
        Tcg_DrawDuelHUDs();
}

static void DrawDuelMainScene_Draw(void)
{
    if (gTcg->duelDisplayedScreen == TCG_DUEL_MAIN_SCENE)
        return;
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    Tcg_LoadSymbolsFont();
    gTcg->duelDisplayedScreen = TCG_DUEL_MAIN_SCENE;
    Tcg_LoadPlayAreaCardGfx(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), TCG_V_TILES1 + 0x50 * TCG_TILE_SIZE);
    Tcg_SetBGP7OrSGB2ToCardPalette();
    Tcg_SwapTurn();
    Tcg_LoadPlayAreaCardGfx(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD), TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE);
    Tcg_SetBGP6OrSGB3ToCardPalette();
    Tcg_FlushAllPalettesOrSendPal23Packet();
    Tcg_SwapTurn();
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD) != 0xFF)
    {
        Tcg_FillRectangle(0, 5, 8, 6, 0xd0, 6, 1);
        Tcg_ApplyBGP7OrSGB2ToCardImage(0, 5);
    }
    Tcg_SwapTurn();
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD) != 0xFF)
    {
        Tcg_FillRectangle(12, 1, 8, 6, 0xa0, 6, 1);
        Tcg_ApplyBGP6OrSGB3ToCardImage(12, 1);
    }
    Tcg_SwapTurn();
    Tcg_WriteDataBlocksToBGMap0(sDuelEAndHPTileData);
    Tcg_DrawDuelHorizontalSeparator();
    DrawDuelHUDs_Draw();
    Tcg_DrawWideTextBox();
    Tcg_EnableLCD();
}

void Tcg_DrawDuelMainScene(void)
{
    if (gTcg->headless)
        return;
    AS_PLAYER(DrawDuelMainScene_Draw);
}

// ---- the AI's turn ------------------------------------------------------------------------------------

// ("<Duelist> is thinking.": the AI's delays count from here)
void Tcg_DuelistIsThinking(void)
{
    if (gTcg->headless)
        return;
    G->vBlankCounter = 0;
    Tcg_DrawWideTextBox_PrintTextNoDelay(TCG_TEXT_DuelistIsThinking);
}

// (AIMakeDecision's .delay_loop: a second since the "thinking" box, at least)
void Tcg_AIThinkingDelay(void)
{
    if (gTcg->headless)
        return;
    do
        Tcg_DoFrame();
    while (G->vBlankCounter < 60);
}

void Tcg_DisplayOpponentUsedAttackScreen(void)
{
    if (gTcg->headless)
        return;
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    Tcg_LoadDuelCardSymbolTiles();
    Tcg_LoadDuelFaceDownCardTiles();
    gTcg->loadedCard1 = Tcg_Card(gTcg->tempCardId);
    gTcg->loadedCard1Id = gTcg->tempCardId;
    G->cardPageNumber = TCG_CARDPAGE_POKEMON_OVERVIEW;
    Tcg_PrintAttackOrPkmnPowerInformation(&gTcg->loadedCard1->attacks[gTcg->selectedAttack ? 1 : 0], 1);
    if (gTcg->loadedAttack.description[0])
        Tcg_PrintAttackOrCardDescription(1, 4, gTcg->loadedAttack.description[0]);
}

void Tcg_PrintUsedTrainerCardDescription(void)
{
    if (gTcg->headless)
        return;
    Tcg_EmptyScreen();
    Tcg_SetNoLineSeparation();
    Tcg_InitTextPrinting_ProcessText(1, 1, gTcg->loadedCard1->name);
    Tcg_InitTextPrintingInTextbox(1, 3, 19);
    if (gTcg->loadedCard1->description)
        Tcg_ProcessText(gTcg->loadedCard1->description);
    Tcg_SetOneLineSeparation();
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_Used);
}

// ---- the energy discard screen (retreat costs, attacks' discards) --------------------------------------

static const struct TcgCardListParams sEnergyDiscardCardListParameters = {1, 5, 4, 14, 4, TCG_SYM_CURSOR_R, TCG_SYM_SPACE, NULL};

void Tcg_DisplayEnergyDiscardMenu(void)
{
    if (gTcg->headless)
        return;
    Tcg_DrawRegularTextBox(0, 3, 20, 10);
    Tcg_DrawWideTextBox_PrintTextNoDelay(TCG_TEXT_ChooseEnergyCardToDiscard);
    Tcg_EnableLCD();
    Tcg_PrintCardListItems(Tcg_CountCardsInDuelTempList(), 0, 0, &sEnergyDiscardCardListParameters);
    G->cardListIndicatorYPosition = 4;
}

// the Pokémon at `location`'s row, and its energies (wDuelTempList) to choose from
void Tcg_DisplayEnergyDiscardScreen(u32 location)
{
    gTcg->energyDiscardPlayAreaLocation = location;
    gTcg->energyDiscardMenuNumerator = 0;
    gTcg->energyDiscardMenuDenominator = 1;
    if (gTcg->headless)
        return;
    Tcg_EmptyScreen();
    Tcg_LoadDuelCardSymbolTiles();
    Tcg_LoadDuelFaceDownCardTiles();
    gTcgCurPlayAreaSlot = location;
    gTcgCurPlayAreaY = 0;
    Tcg_PrintPlayAreaCardInformation();
    Tcg_DisplayEnergyDiscardMenu();
}

// the energy chosen (its deck index; hTempCardIndex_ff98), or TCG_SELECTION_CANCEL; "n/N" chosen so far
u32 Tcg_HandleEnergyDiscardMenuInput(void)
{
    if (gTcg->headless)
    {
        if (gTcg->duelTempList[0] == 0xFF)
            return TCG_SELECTION_CANCEL;
        gTcg->tempCardIndex = gTcg->duelTempList[0];
        return gTcg->duelTempList[0];
    }
    if (gTcg->energyDiscardMenuDenominator)
    {
        Tcg_WriteByteToBGMap0(16, 16, gTcg->energyDiscardMenuNumerator + TCG_SYM_0);
        Tcg_WriteByteToBGMap0(17, 16, TCG_SYM_SLASH);
        Tcg_WriteByteToBGMap0(18, 16, gTcg->energyDiscardMenuDenominator + TCG_SYM_0);
    }
    else
    {
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(17, 16, gTcg->energyDiscardMenuNumerator);
    }
    do
        Tcg_DoFrame();
    while (!Tcg_HandleCardListInput());
    if (G->hCurMenuItem == 0xFF)
        return TCG_SELECTION_CANCEL;
    gTcg->tempCardIndex = gTcg->duelTempList[G->hCurMenuItem];
    return gTcg->tempCardIndex;
}

void Tcg_DoFrames(u32 frames)
{
    if (gTcg->headless)
        return;
    Tcg_DoAFrames(frames);
}

// ---- the coin toss (_TossCoin) ----------------------------------------------------------------------------

static EWRAM_DATA u8 sCoinNumTossed = 0;   // wCoinTossNumTossed (the screen's count: it goes on while the screen's up)
static EWRAM_DATA u8 sCoinDuelistType = 0; // wCoinTossDuelistType

// one coin of `total` (the `number`th of this toss), its result already decided: the coin screen
// (its text box and text on the first), the coin spinning (the player presses A; the AI waits),
// landing, the result and its sound, the tally of circles and crosses
void TcgUi_CoinToss(u16 textId, u32 number, u32 total, bool32 heads)
{
    u8 e;
    if (gTcg->headless)
        return;
    if (number == 0)
    {
        if (gTcg->duelDisplayedScreen != TCG_COIN_TOSS)
        {
            sCoinNumTossed = 0;
            Tcg_EmptyScreen();
            Tcg_LoadDuelCoinTossResultTiles();
        }
        if (sCoinNumTossed == 0)
        {
            gTcg->duelDisplayedScreen = TCG_COIN_TOSS;
            Tcg_DrawLabeledTextBox(0, 12, 20, 6, NULL);
            Tcg_EnableLCD();
            Tcg_InitTextPrintingInTextbox(1, 14, 19);
            Tcg_PrintText(textId);
        }
        Tcg_EnableLCD();
        sCoinDuelistType = *Tcg_TurnVar(TCG_DUELVARS_DUELIST_TYPE);
    }
    if (total >= 2)
    {
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(15, 11, sCoinNumTossed + 1);
        Tcg_WriteByteToBGMap0(17, 11, TCG_SYM_SLASH);
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(18, 11, total);
    }
    Tcg_ResetAnimationQueue();
    Tcg_PlayDuelAnimation(TCG_DUEL_ANIM_COIN_SPIN);
    if (sCoinDuelistType != TCG_DUELIST_TYPE_PLAYER)
        Tcg_DoAFrames(30);
    else
        Tcg_WaitForWideTextBoxInput();
    Tcg_ResetAnimationQueue();
    // (the game's TAILS, its 1, is the result it counts as heads)
    Tcg_PlayDuelAnimation(heads ? TCG_DUEL_ANIM_COIN_TOSS_GOING_TAILS : TCG_DUEL_ANIM_COIN_TOSS_GOING_HEADS);
    do
        Tcg_DoFrame();
    while (Tcg_CheckAnyAnimationPlaying());
    Tcg_PlayDuelAnimation(heads ? TCG_DUEL_ANIM_COIN_TAILS : TCG_DUEL_ANIM_COIN_HEADS);
    e = heads;
    if (sCoinDuelistType != TCG_DUELIST_TYPE_PLAYER)
        e ^= 1;
    Tcg_PlaySFX(e ? TCG_SFX_COIN_TOSS_HEADS : TCG_SFX_COIN_TOSS_TAILS);
    if (total != 1)
        Tcg_FillRectangle((sCoinNumTossed % 10) * 2, (sCoinNumTossed / 10) * 2, 2, 2, heads ? 0x30 : 0x34, 1, 2);
    sCoinNumTossed++;
    if (sCoinDuelistType != TCG_DUELIST_TYPE_PLAYER)
    {
        if (sCoinNumTossed == total)
            Tcg_WaitForWideTextBoxInput();
        Tcg_DoAFrames(30);
    }
    else
    {
        Tcg_WaitForWideTextBoxInput();
    }
    Tcg_FinishQueuedAnimations();
    if (number + 1 == total)
    {
        Tcg_FinishQueuedAnimations();
        Tcg_ResetAnimationQueue();
    }
}

void TcgUi_CoinTossDone(void)
{
    gTcg->duelDisplayedScreen = 0;
}
