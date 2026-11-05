// Pokémon Canvas: the Trading Card Game duel's screens (include/tcg_ui.h), as its rules code calls
// them. So far only the logic is ported: these do nothing yet (and always will with gTcg->headless).
// The game's screens, at 160x144 in the middle of the GBA's, are the next part of the port.
#include "global.h"
#include "tcg.h"
#include "tcg_ui.h"

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

// (until the play area screen's ported: the first benched Pokémon with HP)
u32 Tcg_OpenPlayAreaScreenForSelection(void)
{
    u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (i = 1; i < n; i++)
        if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
            return i;
    return TCG_PLAY_AREA_BENCH_1;
}

// (until the prize screen's ported: the first prizes left)
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
