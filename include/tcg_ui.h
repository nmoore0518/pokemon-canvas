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
void Tcg_PlayAttackAnimation(u32 duelist, u32 location, u32 effectiveness, u16 damage);
void Tcg_PlayStatusConditionQueueAnimations(void);
void Tcg_WaitAttackAnimation(void);
void Tcg_FinishQueuedAnimations(void);

// the player choosing (the AI's choices are in src/tcg/ai*.c)
u32 Tcg_OpenPlayAreaScreenForSelection(void);   // a PLAY_AREA_* (the game's carry: try again)
void Tcg_SelectPrizeCards(u32 count);           // (takes them: Tcg_TakePrizeCard)

#endif // GUARD_TCG_UI_H
