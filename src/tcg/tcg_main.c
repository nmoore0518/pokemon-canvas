// Pokémon Canvas: the Trading Card Game duel's way in from the overworld. Special StartTcgDuel
// (VAR_0x8004: the player's deck, TCG_DECK_*) fades out and runs a duel against Grandmaster Rod's
// Legendary Dragonite deck; it returns to the script with VAR_RESULT the player's result
// (TCG_DUEL_WIN, TCG_DUEL_LOSS). The script waits for it with waitstate.
// The duel runs as its own loop, as the Game Boy Color game's did (src/tcg/gbc.c's frames), from
// CB2_TcgDuel until it's over.
#include "global.h"
#include "tcg.h"
#include "tcg_ai.h"
#include "tcg_gbc.h"
#include "event_data.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "m4a.h"

static EWRAM_DATA u8 sTcgPlayerDeck = 0;

void CB2_TcgDuel(void)
{
    u32 result;
    m4aMPlayAllStop();
    ResetTasks();
    ResetSpriteData();
    FreeAllSpritePalettes();
    Tcg_GbcInit();
    memset(gTcg, 0, sizeof(*gTcg));
    if (sTcgPlayerDeck == TCG_DECK_ROD || sTcgPlayerDeck >= TCG_NUM_DUEL_DECKS)
        sTcgPlayerDeck = TCG_DECK_FIRE;
    Tcg_SetDecks(gTcgDecks[sTcgPlayerDeck], gTcgDecks[TCG_DECK_ROD]);
    gTcg->vars[TCG_PLAYER][TCG_DUELVARS_DUELIST_TYPE] = TCG_DUELIST_TYPE_PLAYER;
    gTcg->vars[TCG_OPPONENT][TCG_DUELVARS_DUELIST_TYPE] = TCG_DUELIST_TYPE_AI_OPP;
    gTcg->duelInitialPrizes = 6;
    TcgAi_SetDeck(TCG_PLAYER, 0);
    TcgAi_SetDeck(TCG_OPPONENT, TCG_DECKID_LEGENDARY_DRAGONITE);
    result = Tcg_StartDuel();
    Tcg_GbcFree();
    gSpecialVar_Result = result;
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

static void Task_StartTcgDuel(u8 taskId)
{
    if (gPaletteFade.active)
        return;
    DestroyTask(taskId);
    SetMainCallback2(CB2_TcgDuel);
}

void StartTcgDuel(void)
{
    sTcgPlayerDeck = gSpecialVar_0x8004 < TCG_NUM_DUEL_DECKS ? gSpecialVar_0x8004 : TCG_DECK_FIRE;
    if (sTcgPlayerDeck == TCG_DECK_ROD)
        sTcgPlayerDeck = TCG_DECK_FIRE;
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_StartTcgDuel, 10);
}
