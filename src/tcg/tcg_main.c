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
#include "tcg_sound.h"
#include "event_data.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "constants/songs.h"
#include "constants/map_groups.h"
#include "constants/rgb.h"
#include "m4a.h"

static EWRAM_DATA u8 sTcgPlayerDeck = 0;

void CB2_TcgDuel(void)
{
    u32 result;
    m4aMPlayAllStop();
    TcgSound_Stop();
    TcgSound_Start();
    Tcg_PlaySong(TCG_MUSIC_DUEL_THEME_3);    // (wDuelTheme: Rod's, as RodNPCHeader has it)
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
    TcgSound_Stop();
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

// (special PlayTcgHouseMusic: the card house's ON_TRANSITION and ON_RESUME) the Pokémon Dome's music,
// on the TCG's sound engine, unless it's on already.
void PlayTcgHouseMusic(void)
{
    if (TcgSound_IsActive() && Tcg_IsSongPlaying() && Tcg_GetPlayingSong() == TCG_MUSIC_POKEMON_DOME)
        return;
    TcgSound_Start();
    Tcg_PlaySong(TCG_MUSIC_POKEMON_DOME);
}

// (src/overworld.c, wherever a map's music is picked) the card house's, or none of the TCG's
void TcgSound_PlayMapMusic(u8 mapGroup, u8 mapNum)
{
    if (mapGroup == MAP_GROUP(MAP_SAFFRON_CITY_TCG_HOUSE) && mapNum == MAP_NUM(MAP_SAFFRON_CITY_TCG_HOUSE))
        PlayTcgHouseMusic();
    else
        TcgSound_Stop();
}
