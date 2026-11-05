// Pokémon Canvas: the Trading Card Game duel's screens (include/tcg_ui.h), as its rules code calls
// them: stand-ins (weak) for the ones not ported yet. They show nothing, and the player's choices
// are the first that's allowed: as the ported screens do too with gTcg->headless (the tests, the
// AI-vs-AI runs), through TcgHeadless_* here.
#include "global.h"
#include "tcg.h"
#include "tcg_ui.h"
#include "tcg_gbc.h"
#include "string_util.h"
#include "constants/characters.h"

void __attribute__((weak)) Tcg_DrawWideTextBox_PrintText(u16 textId) {}
void __attribute__((weak)) Tcg_DrawWideTextBox_WaitForInput(u16 textId) {}
void __attribute__((weak)) Tcg_WaitForWideTextBoxInput(void) {}
void __attribute__((weak)) Tcg_DoFrames(u32 frames) {}
void __attribute__((weak)) Tcg_DrawDuelMainScene(void) {}
void __attribute__((weak)) Tcg_DrawDuelHUDs(void) {}
void __attribute__((weak)) Tcg_DrawDuelHUDsIfMainScene(void) {}
void __attribute__((weak)) Tcg_DisplayUsePokemonPowerScreen(void) {}
void __attribute__((weak)) Tcg_DisplayUsedTrainerCardDetailScreen(void) {}
void __attribute__((weak)) Tcg_DisplayCardDetailScreen(u32 deckIndex, u16 textId) {}
void __attribute__((weak)) Tcg_DrawYourOrOppPlayAreaScreen(u32 duelist) {}
void __attribute__((weak)) TcgUi_CoinToss(u16 textId, u32 number, u32 total, bool32 heads) {}
void __attribute__((weak)) TcgUi_CoinTossDone(void) {}
void __attribute__((weak)) Tcg_ResetAttackAnimationIsPlaying(void) {}
void __attribute__((weak)) Tcg_PlayAttackAnimation(u32 duelist, u32 location, u32 effectiveness, u16 damage) {}
void __attribute__((weak)) Tcg_PlayStatusConditionQueueAnimations(void) {}
void __attribute__((weak)) Tcg_WaitAttackAnimation(void) {}
void __attribute__((weak)) Tcg_FinishQueuedAnimations(void) {}
void __attribute__((weak)) Tcg_DisplayDuelBoxMessage(u32 boxMessage) {}
void __attribute__((weak)) Tcg_PrintScrollableText_NoTextBoxLabel(u16 textId) {}
void __attribute__((weak)) Tcg_PlayShuffleAndDrawCardsAnimation(bool32 bothDuelists) {}
void __attribute__((weak)) Tcg_DisplayNoBasicPokemonInHandScreen(void) {}
void __attribute__((weak)) Tcg_DrawPlayAreaToPlacePrizeCards(void) {}
void __attribute__((weak)) Tcg_PlacePrizes(void) {}
void __attribute__((weak)) Tcg_RedrawTurnDuelistsMainSceneOrDuelHUD(void) {}
void __attribute__((weak)) Tcg_PlayBetweenTurnsAnimation(u32 duelAnim, u32 damage) {}
void __attribute__((weak)) Tcg_PlaySFX(u32 sfx) {}
void __attribute__((weak)) Tcg_PrintPlayAreaCardList(void) {}
void __attribute__((weak)) Tcg_DisplayDrawOneCardScreen(void) {}
void __attribute__((weak)) Tcg_DisplayPlayerDrawCardScreen(void) {}
void __attribute__((weak)) Tcg_DisplayDuelistTurnScreen(void) {}
void __attribute__((weak)) Tcg_DuelistIsThinking(void) {}
void __attribute__((weak)) Tcg_AIThinkingDelay(void) {}
void __attribute__((weak)) Tcg_DrawLargePictureOfCard(void) {}
void __attribute__((weak)) Tcg_PrintUsedTrainerCardDescription(void) {}
void __attribute__((weak)) Tcg_DisplayOpponentUsedAttackScreen(void) {}
void __attribute__((weak)) Tcg_DisplayDuelResult(u32 duelFinished, u16 textId) {}
void __attribute__((weak)) Tcg_DisplayEnergyDiscardScreen(u32 location)
{
    gTcg->energyDiscardPlayAreaLocation = location;
    gTcg->energyDiscardMenuNumerator = 0;
    gTcg->energyDiscardMenuDenominator = 1;
}
void __attribute__((weak)) Tcg_DisplayEnergyDiscardMenu(void) {}
void __attribute__((weak)) Tcg_OpenDuelCheckMenu(void) {}
void __attribute__((weak)) Tcg_PlayDeckShuffleAnimation(void) {}
void __attribute__((weak)) Tcg_SetupPlayAreaScreen(void) {}
void __attribute__((weak)) Tcg_InitAndPrintPlayAreaCardInformationAndLocation_WithTextBox(void) {}
void __attribute__((weak)) Tcg_DisplayDrawNCardsScreen(u32 count) {}
void __attribute__((weak)) Tcg_OpenCardPage_FromHand(void) {}
void __attribute__((weak)) Tcg_DisplayCardListDetails(void) {}
void __attribute__((weak)) Tcg_DrawWholeScreenTextBox(u16 textId) {}
void __attribute__((weak)) Tcg_PlaySFX_InvalidChoice(void) {}
void __attribute__((weak)) Tcg_InitAndDrawCardListScreenLayout(bool32 withSelectCheckMenu) {}
void __attribute__((weak)) Tcg_SetCardListHeaderText(u16 headerText, u16 deckText) {}
void __attribute__((weak)) Tcg_SetCardListInfoBoxText(u16 textId) {}
void __attribute__((weak)) Tcg_DrawSymbolOnPlayAreaCursor(u32 location, u32 symbol) {}
void __attribute__((weak)) Tcg_EraseCursor(void) {}
void __attribute__((weak)) Tcg_DrawAIPeekScreen(u32 param) {}

bool32 __attribute__((weak)) Tcg_YesOrNoMenuWithText_SetCursorToYes(u16 textId)
{
    return FALSE;
}

u32 __attribute__((weak)) Tcg_DisplayCardList(void)
{
    if (gTcg->duelTempList[0] == 0xFF)
        return TCG_SELECTION_CANCEL;
    gTcg->tempCardIndex = gTcg->duelTempList[0];
    return gTcg->duelTempList[0];
}

u32 __attribute__((weak)) Tcg_HandlePlayAreaMenuInput(u32 item)
{
    return TCG_SELECTION_CANCEL;
}

u32 __attribute__((weak)) Tcg_HandlePeekSelection(void)
{
    return TCG_AI_PEEK_TARGET_DECK;
}

bool32 __attribute__((weak)) Tcg_HandleDefendingPokemonAttackSelection(u32 *deckIndex, u32 *attack)
{
    *deckIndex = *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD);
    *attack = 0;
    return FALSE;
}

// (CopyPlayerName / CopyOpponentName) the name as the game's text: half-width (the player's, from
// the GBA's characters: its letters, digits, space, . and -; anything else a space)
void Tcg_CopyDuelistName(u32 duelist, u8 *dest)
{
    static const u8 sRod[] = "\x06Rod";
    const u8 *s;
    if (duelist != TCG_PLAYER)
    {
        memcpy(dest, sRod, sizeof(sRod));
        return;
    }
    *dest++ = TCG_TX_HALFWIDTH;
    for (s = gSaveBlock2Ptr->playerName; *s != EOS && s < gSaveBlock2Ptr->playerName + PLAYER_NAME_LENGTH; s++)
    {
        u8 c = *s;
        if (c >= CHAR_A && c <= CHAR_A + 25)
            c = 'A' + c - CHAR_A;
        else if (c >= CHAR_a && c <= CHAR_a + 25)
            c = 'a' + c - CHAR_a;
        else if (c >= CHAR_0 && c <= CHAR_0 + 9)
            c = '0' + c - CHAR_0;
        else if (c == CHAR_PERIOD)
            c = '.';
        else if (c == CHAR_HYPHEN)
            c = '-';
        else
            c = ' ';
        *dest++ = c;
    }
    *dest = TCG_TX_END;
}

u32 __attribute__((weak)) Tcg_HandleDuelMenuInput(void)
{
    return TCG_DUELMENU_DONE;
}

u32 __attribute__((weak)) Tcg_OpenPlayAreaScreenForSelection(bool32 excludeArena)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = excludeArena ? 1 : 0; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            return i;
    return TCG_SELECTION_CANCEL;
}

u32 __attribute__((weak)) Tcg_DisplayPlaceInitialPokemonCardsScreen(bool32 bench, u16 textId)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_CARDS_IN_HAND);
    if (bench)
        return TCG_SELECTION_CANCEL;
    for (i = 0; i < n; i++)
    {
        const struct TcgCard *c = Tcg_Card(Tcg_GetCardIdFromDeckIndex(*Tcg_TurnVar(TCG_DUELVARS_HAND + i)));
        if (c->type < TCG_TYPE_ENERGY && c->stage == TCG_BASIC)
            return *Tcg_TurnVar(TCG_DUELVARS_HAND + i);
    }
    return TCG_SELECTION_CANCEL;
}

u32 __attribute__((weak)) Tcg_DisplayHandCardList(u16 textId)
{
    return TCG_SELECTION_CANCEL;
}

u32 __attribute__((weak)) Tcg_ReloadHandCardList(void)
{
    return TCG_SELECTION_CANCEL;
}

u32 __attribute__((weak)) Tcg_HandleEnergyDiscardMenuInput(void)
{
    if (gTcg->duelTempList[0] == 0xFF)
        return TCG_SELECTION_CANCEL;
    gTcg->tempCardIndex = gTcg->duelTempList[0];
    return gTcg->duelTempList[0];
}

u32 __attribute__((weak)) Tcg_HandleAttackMenuInput(u32 count, u32 item)
{
    return TCG_SELECTION_CANCEL;
}

u32 __attribute__((weak)) Tcg_SelectPlayAreaPokemonPower(u32 item)
{
    return TCG_SELECTION_CANCEL;
}

bool32 __attribute__((weak)) Tcg_YesOrNoMenuWithText(u16 textId)
{
    return TRUE;
}

void __attribute__((weak)) Tcg_SelectPrizeCards(u32 count)
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
}

// (not ported yet: src/tcg/card_page.c, play_area.c)
void __attribute__((weak)) Tcg_PrintPlayAreaCardInformationAndLocation(void) {}
void __attribute__((weak)) Tcg_OpenPlayAreaScreenForViewing(void) {}
