// Pokémon Canvas: the TCG duel's shuffling and drawing screens, the duelists' portraits and the
// duel's result screen (engine/duel/core.asm), on the emulated GBC display (src/tcg/gbc.c).
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"
#include "tcg_anims.h"
#include "tcg_sound.h"

#define G gTcgGbc


static EWRAM_DATA u8 sNumCardsTryingToDraw = 0;     // wNumCardsTryingToDraw
static EWRAM_DATA u8 sNumCardsBeingDrawn = 0;       // wNumCardsBeingDrawn

// ---- portraits (home/load_animation.asm, engine/scenes.asm's _DrawPortrait) ------------------------

// (LoadBGPalette) PALETTE_*'s CGB palettes as BG palettes from `index`
static void LoadBGPalette(u32 palette, u32 index)
{
    const u16 *p = gTcgPalettes[palette];
    u32 n;
    if (p == NULL)
        return;
    n = *p++;
    if (index + n > 8)
        n = 8 - index;
    memcpy(G->bgPals[index], p, n * 8);
    Tcg_FlushAllPalettes();
}

// a 6x6 portrait at x, y: its tiles from `firstTile` (the $8800 block's), BG palette `pal`
static void DrawPortrait(u32 x, u32 y, const u8 *gfx, u32 firstTile, u32 palette, u32 pal)
{
    u32 r, c;
    Tcg_BankswitchVRAM0();
    Tcg_WriteVram(TCG_V_TILES0 + firstTile * TCG_TILE_SIZE, gfx, 36 * TCG_TILE_SIZE);
    for (r = 0; r < 6; r++)
    {
        for (c = 0; c < 6; c++)
        {
            Tcg_WriteByteToBGMap0(x + c, y + r, firstTile + r * 6 + c);
            Tcg_BankswitchVRAM1();
            Tcg_WriteByteToBGMap0(x + c, y + r, pal);
            Tcg_BankswitchVRAM0();
        }
    }
    LoadBGPalette(palette, pal);
}

// (DrawPlayerPortrait: PLAYER_PIC, TILEMAP_PLAYER: tiles $d0, palette 7)
static void DrawPlayerPortrait(u32 x, u32 y)
{
    DrawPortrait(x, y, gTcgPlayerPortraitGfx, 0xd0, TCG_PALETTE_PLAYER_PIC, 7);
}

// (DrawOpponentPortrait: Rod's ROD_PIC, TILEMAP_OPPONENT: tiles $a0, palette 6)
static void DrawOpponentPortrait(u32 x, u32 y)
{
    DrawPortrait(x, y, gTcgRodPortraitGfx, 0xa0, TCG_PALETTE_ROD_PIC, 6);
}

// (DrawDuelistPortraitsAndNames) the two duelists' portraits and names, and the line between them
static void DrawDuelistPortraitsAndNames(void)
{
    u8 *name = gTcg->defaultText;
    s32 len;
    u32 half;
    Tcg_LoadSymbolsFont();
    Tcg_CopyDuelistName(TCG_PLAYER, name);
    Tcg_InitTextPrinting_ProcessText(0, 11, name);
    DrawPlayerPortrait(0, 5);
    // the opponent's name, aligned to the right
    Tcg_CopyDuelistName(TCG_OPPONENT, name);
    len = Tcg_GetTextLengthInTiles(name, &half);
    Tcg_InitTextPrinting_ProcessText((u8)(len + 20), 0, name);
    DrawOpponentPortrait(13, 1);
    Tcg_DrawDuelHorizontalSeparator();
}

// ---- the draw cards screen ---------------------------------------------------------------------------

static const u8 sDeckAndHandIconsTileData[] =
{
     4,  3, TCG_SYM_CROSS, 0,
    10,  3, TCG_SYM_CROSS, 0,
     8,  2, 0xf4, 0xf5, 0,
     8,  3, 0xf6, 0xf7, 0,
     2,  2, 0xf8, 0xf9, 0,
     2,  3, 0xfa, 0xfb, 0,
     9, 10, TCG_SYM_CROSS, 0,
    15, 10, TCG_SYM_CROSS, 0,
     7,  9, 0xf4, 0xf5, 0,
     7, 10, 0xf6, 0xf7, 0,
    13,  9, 0xf8, 0xf9, 0,
    13, 10, 0xfa, 0xfb, 0,
    0xff
};

static const u8 sDeckAndHandIconsCGBPalData[] =
{
     8,  2, 0x02, 0x02, 0,
     8,  3, 0x02, 0x02, 0,
     2,  2, 0x02, 0x02, 0,
     2,  3, 0x02, 0x02, 0,
     7,  9, 0x02, 0x02, 0,
     7, 10, 0x02, 0x02, 0,
    13,  9, 0x02, 0x02, 0,
    13, 10, 0x02, 0x02, 0,
    0xff
};

// (PrintPlayerNumberOfHandAndDeckCards, PrintOpponentNumberOfHandAndDeckCards)
static void PrintDuelistNumberOfHandAndDeckCards(u32 duelist)
{
    u32 hand = gTcg->vars[duelist][TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND] + sNumCardsBeingDrawn;
    u32 deck = TCG_DECK_SIZE - gTcg->vars[duelist][TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK] - sNumCardsBeingDrawn;
    if (duelist == TCG_PLAYER)
    {
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(16, 10, hand);
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(10, 10, deck);
    }
    else
    {
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(5, 3, hand);
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(11, 3, deck);
    }
}

// (PrintNumberOfHandAndDeckCards) the turn duelist's
static void PrintNumberOfHandAndDeckCards(void)
{
    PrintDuelistNumberOfHandAndDeckCards(gTcg->whoseTurn);
}

// (PrintDeckAndHandIconsAndNumberOfCards; .not_cgb: just the numbers)
static void PrintBothNumbersOfHandAndDeckCards(void)
{
    PrintDuelistNumberOfHandAndDeckCards(TCG_PLAYER);
    PrintDuelistNumberOfHandAndDeckCards(TCG_OPPONENT);
}

static void PrintDeckAndHandIconsAndNumberOfCards(void)
{
    Tcg_LoadDuelDrawCardsScreenTiles();
    Tcg_WriteDataBlocksToBGMap0(sDeckAndHandIconsTileData);
    Tcg_BankswitchVRAM1();
    Tcg_WriteDataBlocksToBGMap0(sDeckAndHandIconsCGBPalData);
    Tcg_BankswitchVRAM0();
    PrintBothNumbersOfHandAndDeckCards();
}

// (PlayTurnDuelistDrawAnimation)
static void PlayTurnDuelistDrawAnimation(void)
{
    Tcg_ResetAnimationQueue();
    Tcg_PlayDuelAnimation(gTcg->whoseTurn == TCG_PLAYER ? TCG_DUEL_ANIM_PLAYER_DRAW : TCG_DUEL_ANIM_OPP_DRAW);
    do
    {
        Tcg_DoFrame();
        if (Tcg_CheckSkipDelayAllowed())
            break;
    } while (Tcg_CheckAnyAnimationPlaying());
    Tcg_FinishQueuedAnimations();
}

// (DisplayDrawNCardsScreen) the turn duelist drawing `count` cards; if the deck's run out, says so
void Tcg_DisplayDrawNCardsScreen(u32 count)
{
    u32 left, c;
    if (gTcg->headless)
        return;
    sNumCardsTryingToDraw = count;
    sNumCardsBeingDrawn = 0;
    left = TCG_DECK_SIZE - *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK);
    if (left < sNumCardsTryingToDraw)
        sNumCardsTryingToDraw = 0;      // (the game's: `ld [hl], a ; 0`)
    if (gTcg->duelDisplayedScreen != TCG_DRAW_CARDS && gTcg->duelDisplayedScreen != TCG_SHUFFLE_DECK)
    {
        Tcg_EmptyScreen();
        DrawDuelistPortraitsAndNames();
    }
    gTcg->duelDisplayedScreen = TCG_DRAW_CARDS;
    PrintDeckAndHandIconsAndNumberOfCards();
    if (sNumCardsTryingToDraw == 0)
    {
        Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_CannotDrawCardBecauseNoCardsInDeck);
        return;
    }
    Tcg_LoadTxRam3(sNumCardsTryingToDraw);
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_DrawCardsFromTheDeck);
    Tcg_EnableLCD();
    do
    {
        PlayTurnDuelistDrawAnimation();
        sNumCardsBeingDrawn++;
        PrintNumberOfHandAndDeckCards();
    } while (sNumCardsBeingDrawn < sNumCardsTryingToDraw);
    for (c = 30; c; c--)
    {
        Tcg_DoFrame();
        if (Tcg_CheckSkipDelayAllowed())
            break;
    }
}

void Tcg_DisplayDrawOneCardScreen(void)
{
    Tcg_DisplayDrawNCardsScreen(1);
}

// (DisplayPlayerDrawCardScreen) the card at hTempCardIndex_ff98
void Tcg_DisplayPlayerDrawCardScreen(void)
{
    Tcg_DisplayCardDetailScreen(gTcg->tempCardIndex, TCG_TEXT_YouDrew);
}

// ---- shuffling --------------------------------------------------------------------------------------

// (PlayShuffleAndDrawCardsAnimation_BothDuelists, _TurnDuelist, PlayShuffleAndDrawCardsAnimation)
void Tcg_PlayShuffleAndDrawCardsAnimation(bool32 bothDuelists)
{
    u32 shuffle, draw, c;
    u16 shuffleText, drawText;
    if (gTcg->headless)
        return;
    if (bothDuelists)
    {
        shuffle = TCG_DUEL_ANIM_BOTH_SHUFFLE;
        draw = TCG_DUEL_ANIM_BOTH_DRAW;
        shuffleText = TCG_TEXT_EachPlayerShuffleOpponentsDeck;
        drawText = TCG_TEXT_EachPlayerDraw7Cards;
    }
    else
    {
        shuffle = gTcg->whoseTurn == TCG_PLAYER ? TCG_DUEL_ANIM_PLAYER_SHUFFLE : TCG_DUEL_ANIM_OPP_SHUFFLE;
        draw = gTcg->whoseTurn == TCG_PLAYER ? TCG_DUEL_ANIM_PLAYER_DRAW : TCG_DUEL_ANIM_OPP_DRAW;
        shuffleText = TCG_TEXT_ShufflesTheDeck;
        drawText = TCG_TEXT_Drew7Cards;
    }
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    DrawDuelistPortraitsAndNames();
    Tcg_LoadDuelDrawCardsScreenTiles();
    gTcg->duelDisplayedScreen = TCG_SHUFFLE_DECK;
    Tcg_DrawWideTextBox_PrintText(shuffleText);
    Tcg_EnableLCD();
    // (the shuffle, three times)
    Tcg_ResetAnimationQueue();
    Tcg_PlayDuelAnimation(shuffle);
    Tcg_PlayDuelAnimation(shuffle);
    Tcg_PlayDuelAnimation(shuffle);
    do
    {
        Tcg_DoFrame();
        if (Tcg_CheckSkipDelayAllowed())
            break;
    } while (Tcg_CheckAnyAnimationPlaying());
    Tcg_FinishQueuedAnimations();
    // .print_deck_info
    sNumCardsBeingDrawn = 0;
    PrintDeckAndHandIconsAndNumberOfCards();
    Tcg_ResetAnimationQueue();
    Tcg_DrawWideTextBox_PrintText(drawText);
    do
    {
        Tcg_PlayDuelAnimation(draw);
        do
        {
            Tcg_DoFrame();
            if (Tcg_CheckSkipDelayAllowed())
                goto done;
        } while (Tcg_CheckAnyAnimationPlaying());
        sNumCardsBeingDrawn++;
        if (draw == TCG_DUEL_ANIM_BOTH_DRAW)
            PrintBothNumbersOfHandAndDeckCards();
        else
            PrintNumberOfHandAndDeckCards();
    } while (sNumCardsBeingDrawn < 7);
    for (c = 30; c; c--)
    {
        Tcg_DoFrame();
        if (Tcg_CheckSkipDelayAllowed())
            break;
    }
done:
    Tcg_FinishQueuedAnimations();
}

// (PlayDeckShuffleAnimation) the turn duelist shuffling their deck
void Tcg_PlayDeckShuffleAnimation(void)
{
    u32 left, anim;
    if (gTcg->headless)
        return;
    if (gTcg->duelDisplayedScreen != TCG_SHUFFLE_DECK)
    {
        Tcg_ZeroObjectPositionsAndToggleOAMCopy();
        Tcg_EmptyScreen();
        DrawDuelistPortraitsAndNames();
    }
    gTcg->duelDisplayedScreen = TCG_SHUFFLE_DECK;
    left = TCG_DECK_SIZE - *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK);
    if (left < 2)
    {
        // (no animation for one card)
        Tcg_LoadTxRam3(left);
        Tcg_DrawWideTextBox_PrintText(TCG_TEXT_DeckHasXCards);
        Tcg_EnableLCD();
        Tcg_DoAFrames(60);
        return;
    }
    Tcg_DrawWideTextBox_PrintText(TCG_TEXT_ShufflesTheDeck);
    Tcg_EnableLCD();
    Tcg_ResetAnimationQueue();
    anim = gTcg->whoseTurn == TCG_PLAYER ? TCG_DUEL_ANIM_PLAYER_SHUFFLE : TCG_DUEL_ANIM_OPP_SHUFFLE;
    Tcg_PlayDuelAnimation(anim);
    Tcg_PlayDuelAnimation(anim);
    Tcg_PlayDuelAnimation(anim);
    do
    {
        Tcg_DoFrame();
        if (Tcg_CheckSkipDelayAllowed())
            break;
    } while (Tcg_CheckAnyAnimationPlaying());
    Tcg_FinishQueuedAnimations();
}

// ---- the duel's result ----------------------------------------------------------------------------

// (PrintDuelResultStats.PrintDuelistResultStats) the turn duelist's prizes left, whether they've
// Pokémon in play and the cards in their deck, at x, y
static void PrintDuelistResultStats(u32 x, u32 y)
{
    u32 bx = x + 7, by = y;
    Tcg_SetNoLineSeparation();
    Tcg_InitTextPrinting_ProcessTextFromID(x, y, TCG_TEXT_PrizesLeftActivePokemonCardsInDeck);
    Tcg_SetOneLineSeparation();
    x += 9;
    // prizes
    Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(bx, by, Tcg_CountPrizes());
    Tcg_InitTextPrinting_ProcessTextFromID(x, y, TCG_TEXT_Cards);
    y++;
    by++;
    // Pokémon in play
    x--;
    Tcg_InitTextPrinting_ProcessTextFromID(x, y,
        *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA) ? TCG_TEXT_Yes : TCG_TEXT_None);
    y++;
    x++;
    by++;
    // cards in the deck
    Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(bx, by, TCG_DECK_SIZE - *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_NOT_IN_DECK));
    Tcg_InitTextPrinting_ProcessTextFromID(x, y, TCG_TEXT_Cards);
}

// (PrintDuelResultStats) the player's at 8, 8, the opponent's at 1, 1
static void PrintDuelResultStats(void)
{
    PrintDuelistResultStats(8, 8);
    Tcg_SwapTurn();
    PrintDuelistResultStats(1, 1);
    Tcg_SwapTurn();
}

// (MainDuelLoop.duel_finished) the "Decision" box, then both duelists' portraits and stats, the
// result's animation and text
void Tcg_DisplayDuelResult(u32 duelFinished, u16 textId)
{
    u32 prevTurn, anim;
    if (gTcg->headless)
        return;
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    Tcg_DrawDuelBoxMessage(TCG_BOXMSG_DECISION);
    Tcg_DrawWideTextBox_WaitForInput(TCG_TEXT_Decision);
    Tcg_EmptyScreen();
    prevTurn = gTcg->whoseTurn;
    gTcg->whoseTurn = TCG_PLAYER;
    DrawDuelistPortraitsAndNames();
    PrintDuelResultStats();
    gTcg->whoseTurn = prevTurn;
    Tcg_ResetAnimationQueue();
    if (duelFinished == TCG_TURN_PLAYER_TIED)
        anim = TCG_DUEL_ANIM_DUEL_DRAW;
    else if ((duelFinished == TCG_TURN_PLAYER_WON) == (gTcg->whoseTurn == TCG_PLAYER))
        anim = TCG_DUEL_ANIM_DUEL_WIN;
    else
        anim = TCG_DUEL_ANIM_DUEL_LOSS;
    Tcg_PlayDuelAnimation(anim);
    Tcg_PlaySong(duelFinished == TCG_TURN_PLAYER_TIED ? TCG_MUSIC_MATCH_DRAW
                 : anim == TCG_DUEL_ANIM_DUEL_WIN ? TCG_MUSIC_MATCH_VICTORY : TCG_MUSIC_MATCH_LOSS);
    // (the game prints the text as the opponent's turn: its <RAMNAME> is the opponent's)
    gTcg->whoseTurn = TCG_OPPONENT;
    Tcg_DrawWideTextBox_PrintText(textId);
    Tcg_EnableLCD();
    // (.wait_song: the result's jingle; then PlayDefaultSong, the area's music: none here)
    do
        Tcg_DoFrame();
    while (Tcg_IsSongPlaying());
    Tcg_WaitForWideTextBoxInput();
    Tcg_FinishQueuedAnimations();
    gTcg->whoseTurn = prevTurn;
}

// ---- no Basic Pokémon ----------------------------------------------------------------------------------

static const struct TcgCardListParams sNoBasicPokemonCardListParameters = {1, 3, 4, 14, 7, TCG_SYM_CURSOR_R, TCG_SYM_SPACE, NULL};

// (DisplayNoBasicPokemonInHandScreen) the turn duelist's seven hand cards, with their name above
void Tcg_DisplayNoBasicPokemonInHandScreen(void)
{
    if (gTcg->headless)
        return;
    Tcg_EmptyScreen();
    Tcg_LoadDuelCardSymbolTiles();
    Tcg_DrawRegularTextBox(0, 0, 20, 18);
    Tcg_CreateHandCardList();
    Tcg_PrintCardListItems(Tcg_CountCardsInDuelTempList(), 0, 0, &sNoBasicPokemonCardListParameters);
    Tcg_InitTextPrinting(1, 1);
    Tcg_PrintTextNoDelay(TCG_TEXT_DuelistHand);
    Tcg_EnableLCD();
    Tcg_WaitForWideTextBoxInput();
}
