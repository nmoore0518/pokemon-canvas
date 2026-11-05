// Pokémon Canvas: the Trading Card Game duel's screens (include/tcg_ui.h), as its rules code calls
// them. So far only the rules are ported: these show nothing yet, and the player's choices are
// stand-ins (the first that's allowed), as they'll stay with gTcg->headless (the tests, AI-vs-AI runs).
// The game's screens, at 160x144 in the middle of the GBA's, are the next part of the port.
#include "global.h"
#include "tcg.h"
#include "tcg_ui.h"
#include "string_util.h"

void Tcg_DrawWideTextBox_PrintText(u16 textId) {}
void Tcg_DrawWideTextBox_WaitForInput(u16 textId) {}
void Tcg_WaitForWideTextBoxInput(void) {}
void Tcg_DoFrames(u32 frames) {}
void Tcg_DrawDuelMainScene(void) {}
void Tcg_DrawDuelHUDs(void) {}
void Tcg_DrawDuelHUDsIfMainScene(void) {}
void Tcg_DisplayUsePokemonPowerScreen(void) {}
void Tcg_DisplayUsedTrainerCardDetailScreen(void) {}
void Tcg_DisplayCardDetailScreen(u32 deckIndex, u16 textId) {}
void Tcg_DrawYourOrOppPlayAreaScreen(u32 duelist) {}
void TcgUi_CoinToss(u16 textId, u32 number, u32 total, bool32 heads) {}
void TcgUi_CoinTossDone(void) {}
void Tcg_ResetAttackAnimationIsPlaying(void) {}
void Tcg_PlayAttackAnimation(u32 duelist, u32 location, u32 effectiveness, u16 damage) {}
void Tcg_PlayStatusConditionQueueAnimations(void) {}
void Tcg_WaitAttackAnimation(void) {}
void Tcg_FinishQueuedAnimations(void) {}
void Tcg_DisplayDuelBoxMessage(u32 boxMessage) {}
void Tcg_PrintScrollableText_NoTextBoxLabel(u16 textId) {}
void Tcg_PlayShuffleAndDrawCardsAnimation(bool32 bothDuelists) {}
void Tcg_DisplayNoBasicPokemonInHandScreen(void) {}
void Tcg_DrawPlayAreaToPlacePrizeCards(void) {}
void Tcg_PlacePrizes(void) {}
void Tcg_RedrawTurnDuelistsMainSceneOrDuelHUD(void) {}
void Tcg_PlayBetweenTurnsAnimation(u32 duelAnim, u32 damage) {}
void Tcg_PlaySFX(u32 sfx) {}
void Tcg_PrintPlayAreaCardList(void) {}
void Tcg_DisplayDrawOneCardScreen(void) {}
void Tcg_DisplayPlayerDrawCardScreen(void) {}
void Tcg_DisplayDuelistTurnScreen(void) {}
void Tcg_DuelistIsThinking(void) {}
void Tcg_AIThinkingDelay(void) {}
void Tcg_DrawLargePictureOfCard(void) {}
void Tcg_PrintUsedTrainerCardDescription(void) {}
void Tcg_DisplayOpponentUsedAttackScreen(void) {}
void Tcg_DisplayDuelResult(u32 duelFinished, u16 textId) {}
void Tcg_DisplayEnergyDiscardScreen(u32 location)
{
    gTcg->energyDiscardPlayAreaLocation = location;
    gTcg->energyDiscardMenuNumerator = 0;
    gTcg->energyDiscardMenuDenominator = 1;
}
void Tcg_DisplayEnergyDiscardMenu(void) {}
void Tcg_OpenDuelCheckMenu(void) {}
void Tcg_EmptyScreen(void) {}
void Tcg_PlayDeckShuffleAnimation(void) {}
void Tcg_SetupPlayAreaScreen(void) {}
void Tcg_InitAndPrintPlayAreaCardInformationAndLocation_WithTextBox(void) {}
void Tcg_DisplayDrawNCardsScreen(u32 count) {}
void Tcg_OpenCardPage_FromHand(void) {}
void Tcg_DisplayCardListDetails(void) {}
void Tcg_DrawWholeScreenTextBox(u16 textId) {}
void Tcg_PlaySFX_InvalidChoice(void) {}
void Tcg_InitAndDrawCardListScreenLayout(bool32 withSelectCheckMenu) {}
void Tcg_SetCardListHeaderText(u16 headerText, u16 deckText) {}
void Tcg_SetCardListInfoBoxText(u16 textId) {}
void Tcg_DrawSymbolOnPlayAreaCursor(u32 location, u32 symbol) {}
void Tcg_EraseCursor(void) {}
void Tcg_DrawAIPeekScreen(u32 param) {}

bool32 Tcg_YesOrNoMenuWithText_SetCursorToYes(u16 textId)
{
    return FALSE;
}

u32 Tcg_DisplayCardList(void)
{
    if (gTcg->duelTempList[0] == 0xFF)
        return TCG_SELECTION_CANCEL;
    gTcg->tempCardIndex = gTcg->duelTempList[0];
    return gTcg->duelTempList[0];
}

u32 Tcg_HandlePlayAreaMenuInput(u32 item)
{
    return TCG_SELECTION_CANCEL;
}

u32 Tcg_HandlePeekSelection(void)
{
    return TCG_AI_PEEK_TARGET_DECK;
}

bool32 Tcg_HandleDefendingPokemonAttackSelection(u32 *deckIndex, u32 *attack)
{
    *deckIndex = *Tcg_NonTurnVar(TCG_DUELVARS_ARENA_CARD);
    *attack = 0;
    return FALSE;
}

// (CopyPlayerName / CopyOpponentName)
void Tcg_CopyDuelistName(u32 duelist, u8 *dest)
{
    static const u8 sPlayer[] = _("Player");
    static const u8 sRod[] = _("Rod");
    StringCopy(dest, duelist == TCG_PLAYER ? sPlayer : sRod);
}

u32 Tcg_HandleDuelMenuInput(void)
{
    return TCG_DUELMENU_DONE;
}

u32 Tcg_OpenPlayAreaScreenForSelection(bool32 excludeArena)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = excludeArena ? 1 : 0; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            return i;
    return TCG_SELECTION_CANCEL;
}

u32 Tcg_DisplayPlaceInitialPokemonCardsScreen(bool32 bench, u16 textId)
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

u32 Tcg_DisplayHandCardList(u16 textId)
{
    return TCG_SELECTION_CANCEL;
}

u32 Tcg_HandleEnergyDiscardMenuInput(void)
{
    if (gTcg->duelTempList[0] == 0xFF)
        return TCG_SELECTION_CANCEL;
    gTcg->tempCardIndex = gTcg->duelTempList[0];
    return gTcg->duelTempList[0];
}

u32 Tcg_HandleAttackMenuInput(u32 count, u32 item)
{
    return TCG_SELECTION_CANCEL;
}

u32 Tcg_SelectPlayAreaPokemonPower(u32 item)
{
    return TCG_SELECTION_CANCEL;
}

bool32 Tcg_YesOrNoMenuWithText(u16 textId)
{
    return TRUE;
}

void Tcg_SelectPrizeCards(u32 count)
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
