// Pokémon Canvas: the Trading Card Game duel's play area screen (each Pokémon in play: its name, level,
// stage, energies, HP, status), ported from the Game Boy Color game's engine/duel/core.asm
// (pret/poketcg) under its own names (Tcg_ prefixed): DisplayPlayAreaScreen (for viewing, for
// choosing), PrintPlayAreaCardList and the rows it prints.
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"

#define G gTcgGbc

static EWRAM_DATA u8 sExcludeArenaPokemon = 0;     // wExcludeArenaPokemon
static EWRAM_DATA u8 sNumPlayAreaItems = 0;        // wNumPlayAreaItems
static EWRAM_DATA u8 sPlayAreaScreenLoaded = 0;    // wPlayAreaScreenLoaded
EWRAM_DATA u8 gTcgPlayAreaSelectAction = 0;        // wPlayAreaSelectAction

// (process_text.asm's CopyTextData) text to dest, at most maxLength characters (half-width: twice
// that), padded with spaces
static void CopyTextData(u8 *dest, const u8 *text, u32 maxLength)
{
    bool32 half = *text == TCG_TX_HALFWIDTH;
    u32 d = half ? maxLength * 2 : maxLength;
    for (;;)
    {
        u8 a = *text;
        if (a == TCG_TX_END)
            break;
        text++;
        *dest++ = a;
        if (a >= TCG_TX_CTRL_START && a < TCG_TX_CTRL_END)
            continue;
        if (!half && a < TCG_TX_CTRL_START)
            *dest++ = *text++;      // (a TX_FULLWIDTH* and its character)
        if (--d == 0)
        {
            *dest = TCG_TX_END;
            return;
        }
    }
    while (d--)
    {
        if (half)
            *dest++ = ' ';
        else
            *dest++ = 0x70;     // (ldfw " ")
    }
    *dest = TCG_TX_END;
}

static const u8 sPlayAreaLocationTileNumbers[][3] =
{
    {0xe0, 0xe1, 0xe2},     // ACT
    {0xe3, 0xe4, 0xe5},     // BP1
    {0xe3, 0xe4, 0xe6},     // BP2
    {0xe3, 0xe4, 0xe7},     // BP3
    {0xe3, 0xe4, 0xe8},     // BP4
    {0xe3, 0xe4, 0xe9},     // BP5
};

static void PrintPlayAreaCardLocation(void)
{
    const u8 *t = sPlayAreaLocationTileNumbers[gTcgCurPlayAreaSlot];
    u8 d = gTcg->whoseTurn == TCG_PLAYER ? 0 : 0x0a, i;
    for (i = 0; i < 3; i++)
        Tcg_WriteByteToBGMap0(1, gTcgCurPlayAreaY + i, t[i] + d);
}

static const u8 sFaceDownCardTileNumbers[][2] =
{
    {0xd0, 0x02},   // basic
    {0xd4, 0x02},   // stage 1
    {0xd8, 0x01},   // stage 2
    {0xdc, 0x01},   // stage 2 special
};

static void PrintPlayAreaCardHeader(void)
{
    u32 slot = gTcgCurPlayAreaSlot, y = gTcgCurPlayAreaY;
    u8 status, n;
    const u8 *f;
    Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + slot));
    Tcg_InitTextPrinting(4, y);
    CopyTextData(gTcg->defaultText, gTcg->loadedCard1->name, 10);
    Tcg_ProcessText(gTcg->defaultText);
    Tcg_WriteByteToBGMap0(18, y, Tcg_GetPlayAreaCardColor(slot) + 1);
    Tcg_WriteByteToBGMap0(14, y, TCG_SYM_Lv);
    Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(15, y, gTcg->loadedCard1->level);
    f = sFaceDownCardTileNumbers[*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STAGE + slot) & 3];
    Tcg_FillRectangle(2, y, 2, 2, f[0], 1, 2);
    Tcg_BankswitchVRAM1();
    Tcg_FillRectangle(2, y, 2, 2, f[1], 0, 0);
    Tcg_BankswitchVRAM0();
    if (slot == 0)
    {
        status = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_STATUS);
        Tcg_WriteByteToBGMap0(2, y + 2, ((const u8[]){TCG_SYM_SPACE, TCG_SYM_CONFUSED, TCG_SYM_ASLEEP, TCG_SYM_PARALYZED})[status & 3]);
        Tcg_WriteByteToBGMap0(3, y + 2, (status & TCG_POISONED) ? TCG_SYM_POISONED : TCG_SYM_SPACE);
        Tcg_WriteByteToBGMap0(4, y + 2, (status & (TCG_DOUBLE_POISONED & ~TCG_POISONED)) ? TCG_SYM_POISONED : TCG_SYM_SPACE);
    }
    n = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_PLUSPOWER + slot);
    if (n)
    {
        Tcg_WriteByteToBGMap0(15, y + 1, TCG_SYM_PLUSPOWER);
        Tcg_WriteByteToBGMap0(16, y + 1, n + TCG_SYM_0);
    }
    n = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_ATTACHED_DEFENDER + slot);
    if (n)
    {
        Tcg_WriteByteToBGMap0(17, y + 1, TCG_SYM_DEFENDER);
        Tcg_WriteByteToBGMap0(18, y + 1, n + TCG_SYM_0);
    }
}

void Tcg_PrintPlayAreaCardInformation(void)
{
    u32 slot = gTcgCurPlayAreaSlot, y = gTcgCurPlayAreaY, hp;
    PrintPlayAreaCardHeader();
    Tcg_PrintPlayAreaCardAttachedEnergies(7, y + 1, slot);
    Tcg_WriteByteToBGMap0(5, y + 1, TCG_SYM_E);
    Tcg_WriteByteToBGMap0(5, y + 2, TCG_SYM_HP);
    hp = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + slot);
    if (hp == 0)
    {
        Tcg_InitTextPrinting_ProcessTextFromID(7, y + 2, TCG_TEXT_KnockOut);
        return;
    }
    Tcg_DrawHPBar(gTcg->loadedCard1->hp, hp);
    Tcg_CopyDataToBGMap0(7, y + 2, gTcg->defaultText, 12);
}

void Tcg_PrintPlayAreaCardInformationAndLocation(void)
{
    if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcgCurPlayAreaSlot) == 0xFF)
        return;
    Tcg_PrintPlayAreaCardInformation();
    PrintPlayAreaCardLocation();
}

// each Pokémon in play (wDuelTempList: their deck indexes), the empty slots' labels
static void PrintPlayAreaCardList(void)
{
    u32 b, n;
    gTcg->duelDisplayedScreen = TCG_PLAY_AREA_CARD_LIST;
    n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
    for (b = 0; b < n; b++)
    {
        gTcgCurPlayAreaSlot = b;
        gTcgCurPlayAreaY = b * 3;
        gTcg->duelTempList[b] = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + b);
        Tcg_PrintPlayAreaCardInformationAndLocation();
    }
    for (; b < TCG_MAX_PLAY_AREA_POKEMON; b++)
    {
        gTcgCurPlayAreaSlot = b;
        gTcgCurPlayAreaY = b * 3;
        PrintPlayAreaCardLocation();
    }
    sNumPlayAreaItems = n;
    if (sExcludeArenaPokemon)
    {
        sNumPlayAreaItems = n - 1;
        memmove(gTcg->duelTempList, gTcg->duelTempList + 1, n - 1);
    }
}

void Tcg_PrintPlayAreaCardList(void)
{
    if (gTcg->headless)
        return;
    PrintPlayAreaCardList();
}

void Tcg_SetupPlayAreaScreen(void)
{
    if (gTcg->headless)
        return;
    sExcludeArenaPokemon = FALSE;
    if (gTcg->duelDisplayedScreen == TCG_PLAY_AREA_CARD_LIST)
        return;
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    Tcg_LoadDuelCardSymbolTiles();
    Tcg_LoadDuelCheckPokemonScreenTiles();
}

// (InitAndPrintPlayAreaCardInformationAndLocation) hTempPlayAreaLocation_ff9d's row
static void InitAndPrintPlayAreaCardInformationAndLocation(void)
{
    gTcgCurPlayAreaSlot = gTcg->tempPlayAreaLocation;
    gTcgCurPlayAreaY = gTcg->tempPlayAreaLocation * 3;
    Tcg_PrintPlayAreaCardInformationAndLocation();
}

void Tcg_InitAndPrintPlayAreaCardInformationAndLocation_WithTextBox(void)
{
    if (gTcg->headless)
        return;
    InitAndPrintPlayAreaCardInformationAndLocation();
    Tcg_SetCursorParametersForTextBox_Default(0, gTcgCurPlayAreaY);
}

static bool32 PlayAreaScreenMenuFunction(u32 item)
{
    u8 a = G->keysPressed & (TCG_PAD_A | TCG_PAD_B | TCG_PAD_START);
    if (!a)
        return FALSE;
    if (a & TCG_PAD_B)
        G->hCurMenuItem = 0xFF;
    return TRUE;
}

static const struct TcgMenuParams sPlayAreaScreenMenuParameters_ActivePokemonIncluded = {0, 0, 3, 6, TCG_SYM_CURSOR_R, TCG_SYM_SPACE, PlayAreaScreenMenuFunction};
static const struct TcgMenuParams sPlayAreaScreenMenuParameters_ActivePokemonExcluded = {0, 3, 3, 6, TCG_SYM_CURSOR_R, TCG_SYM_SPACE, PlayAreaScreenMenuFunction};

// DisplayPlayAreaScreen: the turn holder's play area, until a Pokémon with HP is chosen (its
// PLAY_AREA_*) or B (TCG_SELECTION_CANCEL); the no-selection keys (START; viewing: A too) open the
// card's page. (wPlayAreaSelectAction's SELECT menu: not ported; none of our callers set it yet.)
static u32 DisplayPlayAreaScreen(u8 noSelectionKeys)
{
    u8 card = gTcg->tempCardIndex;
    bool32 draw = TRUE;
    G->noItemSelectionMenuKeys = noSelectionKeys;
    if (!sPlayAreaScreenLoaded)
    {
        gTcg->selectedDuelSubMenuItem = 0;
        sPlayAreaScreenLoaded = TRUE;
    }
    else
        draw = FALSE;
    for (;;)
    {
        if (draw)
        {
            Tcg_ZeroObjectPositionsAndToggleOAMCopy();
            Tcg_EmptyScreen();
            Tcg_LoadDuelCardSymbolTiles();
            Tcg_LoadDuelCheckPokemonScreenTiles();
            PrintPlayAreaCardList();
            Tcg_EnableLCD();
        }
        draw = TRUE;
        Tcg_InitializeMenuParameters(gTcg->selectedDuelSubMenuItem,
                                     sExcludeArenaPokemon ? &sPlayAreaScreenMenuParameters_ActivePokemonExcluded
                                                          : &sPlayAreaScreenMenuParameters_ActivePokemonIncluded);
        G->numMenuItems = sNumPlayAreaItems;
        for (;;)
        {
            u32 loc;
            Tcg_DoFrame();
            if (!Tcg_HandleMenuInput())
                continue;
            gTcg->selectedDuelSubMenuItem = G->curMenuItem;
            gTcgCurPlayAreaSlot = sExcludeArenaPokemon + G->curMenuItem;
            if (G->keysPressed & G->noItemSelectionMenuKeys)
            {
                u8 c = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD + gTcgCurPlayAreaSlot);
                if (c != 0xFF)
                {
                    Tcg_LoadCard1(c);
                    Tcg_OpenCardPage_FromCheckPlayArea();
                }
                break;
            }
            loc = G->hCurMenuItem + sExcludeArenaPokemon;
            gTcg->tempPlayAreaLocation = loc;
            if (G->hCurMenuItem == 0xFF)
            {
                gTcg->tempCardIndex = card;
                G->hCurMenuItem = loc;
                return TCG_SELECTION_CANCEL;
            }
            if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + loc) == 0)
                continue;
            gTcg->tempCardIndex = card;
            G->hCurMenuItem = loc;
            return loc;
        }
    }
}

// (HasAlivePokemonInPlayArea / HasAlivePokemonInBench, which the game's callers run first)
static void InitPlayAreaScreen(bool32 excludeArena)
{
    sExcludeArenaPokemon = excludeArena;
    sPlayAreaScreenLoaded = FALSE;
    gTcgPlayAreaSelectAction = 0;
}

u32 Tcg_OpenPlayAreaScreenForSelection(bool32 excludeArena)
{
    if (gTcg->headless)
    {
        u32 i, n = *Tcg_TurnVar(TCG_DUELVARS_NUMBER_OF_POKEMON_IN_PLAY_AREA);
        for (i = excludeArena ? 1 : 0; i < n; i++)
            if (*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD_HP + i))
                return i;
        return TCG_SELECTION_CANCEL;
    }
    InitPlayAreaScreen(excludeArena);
    return DisplayPlayAreaScreen(TCG_PAD_START);
}

// (OpenTurnHolderPlayAreaScreen: HasAlivePokemonInPlayArea, OpenPlayAreaScreenForViewing)
void Tcg_OpenPlayAreaScreenForViewing(void)
{
    InitPlayAreaScreen(FALSE);
    DisplayPlayAreaScreen(TCG_PAD_START | TCG_PAD_A);
}
