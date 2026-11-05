#ifndef GUARD_TCG_UI_H
#define GUARD_TCG_UI_H

// Pokémon Canvas: the Trading Card Game duel's screens, as the rules code (src/tcg/duel_*.c) calls
// them: the game's own routines (DrawWideTextBox_WaitForInput, PlayAttackAnimation, ...), under the
// same names. src/tcg/ui.c draws them; with gTcg->headless set they do nothing (no screens, no
// waiting: the unit tests, and AI-vs-AI runs).

// text boxes (the game's two-line box at the bottom of the screen)
void Tcg_DrawWideTextBox_PrintText(u16 textId);
void Tcg_DrawWideTextBox_WaitForInput(u16 textId);
void Tcg_WaitForWideTextBoxInput(void);
void Tcg_DoFrames(u32 frames);

// screens
void Tcg_DrawDuelMainScene(void);
void Tcg_DrawDuelHUDs(void);
void Tcg_DrawDuelHUDsIfMainScene(void);         // (the game's "cp DUEL_MAIN_SCENE" checks)
void Tcg_DisplayUsePokemonPowerScreen(void);
void Tcg_DisplayUsedTrainerCardDetailScreen(void);
void Tcg_DisplayCardDetailScreen(u32 deckIndex, u16 textId);
void Tcg_DrawYourOrOppPlayAreaScreen(u32 duelist);

// the coin toss screen: one coin of `total` (number: from 0), and how it landed
void TcgUi_CoinToss(u16 textId, u32 number, u32 total, bool32 heads);
void TcgUi_CoinTossDone(void);                  // (wDuelDisplayedScreen = 0)

// animations
void Tcg_ResetAttackAnimationIsPlaying(void);
void Tcg_ResetAnimationQueue(void);
void Tcg_PlayAttackAnimation(u32 duelist, u32 location, u32 effectiveness, u16 damage);
void Tcg_PlayStatusConditionQueueAnimations(void);
void Tcg_WaitAttackAnimation(void);
void Tcg_FinishQueuedAnimations(void);

// more screens
void Tcg_DisplayDuelBoxMessage(u32 boxMessage);         // (BOXMSG_*, on an empty screen)
void Tcg_PrintScrollableText_NoTextBoxLabel(u16 textId);
void Tcg_PlayShuffleAndDrawCardsAnimation(bool32 bothDuelists);
void Tcg_DisplayNoBasicPokemonInHandScreen(void);
void Tcg_DrawPlayAreaToPlacePrizeCards(void);
void Tcg_PlacePrizes(void);
void Tcg_RedrawTurnDuelistsMainSceneOrDuelHUD(void);
void Tcg_PlayBetweenTurnsAnimation(u32 duelAnim, u32 damage);
void Tcg_PlaySFX(u32 sfx);
void Tcg_PrintPlayAreaCardList(void);
void Tcg_DisplayDrawOneCardScreen(void);
void Tcg_DisplayPlayerDrawCardScreen(void);
void Tcg_DisplayDuelistTurnScreen(void);
void Tcg_DuelistIsThinking(void);                       // ("<Duelist> is thinking.")
void Tcg_AIThinkingDelay(void);                         // (AIMakeDecision's wait)
void Tcg_DrawLargePictureOfCard(void);                  // (wLoadedCard1)
void Tcg_PrintUsedTrainerCardDescription(void);
void Tcg_DisplayOpponentUsedAttackScreen(void);
void Tcg_DisplayDuelResult(u32 duelFinished, u16 textId);
void Tcg_DisplayEnergyDiscardScreen(u32 location);      // (wEnergyDiscardMenuNumerator 0, ...Denominator 1)
void Tcg_DisplayEnergyDiscardMenu(void);
void Tcg_CopyDuelistName(u32 duelist, u8 *dest);
void Tcg_EmptyScreen(void);
void Tcg_PlayDeckShuffleAnimation(void);
void Tcg_SetupPlayAreaScreen(void);
void Tcg_InitAndPrintPlayAreaCardInformationAndLocation_WithTextBox(void);   // (hTempPlayAreaLocation_ff9d's)
void Tcg_DisplayDrawNCardsScreen(u32 count);
void Tcg_OpenCardPage_FromHand(void);                   // (wLoadedCard1)
void Tcg_DisplayCardListDetails(void);                  // (wDuelTempList)
void Tcg_DrawWholeScreenTextBox(u16 textId);
void Tcg_PlaySFX_InvalidChoice(void);
void Tcg_InitAndDrawCardListScreenLayout(bool32 withSelectCheckMenu);
void Tcg_SetCardListHeaderText(u16 headerText, u16 deckText);
void Tcg_SetCardListInfoBoxText(u16 textId);
void Tcg_DrawSymbolOnPlayAreaCursor(u32 location, u32 symbol);
void Tcg_EraseCursor(void);
void Tcg_DrawAIPeekScreen(u32 param);

// the player choosing (the AI's choices are in src/tcg/ai*.c): TCG_SELECTION_CANCEL is B (the game's carry)
#define TCG_SELECTION_CANCEL 0xFF
enum { TCG_DUELMENU_HAND, TCG_DUELMENU_ATTACK, TCG_DUELMENU_CHECK, TCG_DUELMENU_PKMN_POWER, TCG_DUELMENU_RETREAT, TCG_DUELMENU_DONE };
u32 Tcg_HandleDuelMenuInput(void);                      // (TCG_DUELMENU_*; the B+direction shortcuts are its own)
void Tcg_OpenDuelCheckMenu(void);
void Tcg_RedrawPlayAreaPrizeCards(void);                // (Func_82b6: after the AI's taken its prizes)
void Tcg_OpenInPlayAreaScreen(void);                    // (the Check menu's In Play Area)
void Tcg_OpenGlossaryScreen(void);                      // (the Check menu's Glossary)
u32 Tcg_OpenPlayAreaScreenForSelection(bool32 excludeArena);   // a PLAY_AREA_* (hTempPlayAreaLocation_ff9d)
u32 Tcg_DisplayPlaceInitialPokemonCardsScreen(bool32 bench, u16 textId);   // a Basic in the hand
u32 Tcg_DisplayHandCardList(u16 textId);                // a deck index (wDuelTempList: the hand)
u32 Tcg_ReloadHandCardList(void);                       // (ReloadCardListScreen: the list again, as it was)
u32 Tcg_HandleEnergyDiscardMenuInput(void);             // a deck index of wDuelTempList (hTempCardIndex_ff98)
u32 Tcg_HandleAttackMenuInput(u32 count, u32 item);     // an item of wDuelTempList's attacks
u32 Tcg_SelectPlayAreaPokemonPower(u32 item);           // a PLAY_AREA_*
bool32 Tcg_YesOrNoMenuWithText(u16 textId);             // TRUE: no
bool32 Tcg_YesOrNoMenuWithText_SetCursorToYes(u16 textId);
u32 Tcg_DisplayCardList(void);                          // a card of wDuelTempList (hTempCardIndex_ff98)
u32 Tcg_HandlePlayAreaMenuInput(u32 item);              // a PLAY_AREA_* (the play area screen's menu)
u32 Tcg_HandlePeekSelection(void);                      // (Peek: what the player looked at, as the AI's param)
bool32 Tcg_HandleDefendingPokemonAttackSelection(u32 *deckIndex, u32 *attack);  // TRUE: B
void Tcg_SelectPrizeCards(u32 count);                   // (takes them: Tcg_TakePrizeCard)

#endif // GUARD_TCG_UI_H
