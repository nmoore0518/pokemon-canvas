// Pokémon Canvas: the Trading Card Game duel's card pages and large card pictures, ported from the
// Game Boy Color game's engine/duel/core.asm (pret/poketcg) under its own names (Tcg_ prefixed):
// OpenCardPage and its pages (a Pokémon's overview, attacks and description; an Energy's; a
// Trainer's), DrawLargePictureOfCard and DisplayCardDetailScreen, DrawWholeScreenTextBox.
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"

#define G gTcgGbc

// (wCurPlayAreaSlot, wCurPlayAreaY: the play area screen's, src/tcg/play_area.c)
EWRAM_DATA u8 gTcgCurPlayAreaSlot = 0;
EWRAM_DATA u8 gTcgCurPlayAreaY = 0;

void Tcg_SetNoLineSeparation(void)
{
    G->lineSeparation = 1;  // (SINGLE_SPACED)
}

void Tcg_SetOneLineSeparation(void)
{
    G->lineSeparation = 0;  // (DOUBLE_SPACED)
}

// (TwoByteNumberToTxSymbol_PadSpace_Bank1) n as 5 symbol tiles in wStringBuffer (no TX_SYMBOLs), up to
// 4 leading zeros as spaces; returns how many of the first four were kept (the game's b)
static u32 TwoByteNumberToTxSymbol_PadSpace_Bank1(u16 n)
{
    static const u16 sPowers[] = {10000, 1000, 100, 10, 1};
    u8 *hl = G->stringBuffer;
    u32 i, b;
    for (i = 0; i < 5; i++)
    {
        hl[i] = TCG_SYM_0 + n / sPowers[i];
        n %= sPowers[i];
    }
    hl[5] = TCG_TX_END;
    for (b = 4; b && *hl == TCG_SYM_0; b--)
        *hl++ = TCG_SYM_SPACE;
    return b;
}

void Tcg_WriteOneByteNumberInTxSymbol_PadSpace(u32 x, u32 y, u32 n)
{
    TwoByteNumberToTxSymbol_PadSpace_Bank1(n);
    Tcg_CopyDataToBGMap0(x, y, G->stringBuffer + 2, 3);
}

void Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(u32 x, u32 y, u32 n)
{
    TwoByteNumberToTxSymbol_PadSpace_Bank1(n);
    Tcg_CopyDataToBGMap0(x, y, G->stringBuffer + 3, 2);
}

// ---- the attacks' lines ----------------------------------------------------------------------------------

// (PrintAttackOrPkmnPowerInformation) the attack's energy cost (from x 2), name (x 7), the description
// icon (on page 0), damage and its +/-/x symbol on the next line
void Tcg_PrintAttackOrPkmnPowerInformation(const struct TcgAttack *attack, u32 y)
{
    u32 x, t, category;
    if (attack->name == NULL)
        return;
    Tcg_InitTextPrinting_ProcessText(7, y, attack->name);
    if (G->cardPageNumber == 0 && attack->description[0] != NULL)
        Tcg_WriteByteToBGMap0(18, y, TCG_SYM_ATK_DESCR);
    if (attack->damage)
        Tcg_WriteOneByteNumberInTxSymbol_PadSpace(15, y + 1, attack->damage);
    category = attack->category & ~TCG_RESIDUAL;
    if (category == TCG_POKEMON_POWER)
    {
        Tcg_InitTextPrinting_ProcessTextFromID(2, y, TCG_TEXT_PKMNPWR);
        return;
    }
    if (category)
        Tcg_WriteByteToBGMap0(18, y + 1, TCG_SYM_PLUS - TCG_DAMAGE_PLUS + category);
    for (x = 2, t = 0; t < TCG_NUM_ENERGY_TYPES; t++)
    {
        u32 n;
        for (n = attack->cost[t]; n; n--)
            Tcg_WriteByteToBGMap0(x++, y, TCG_SYM_FIRE + t);
    }
}

static void PrintCardPageWeaknessesOrResistances(u32 x, u32 y, u8 wr)
{
    u32 a;
    for (a = 1; a < 8; a++)
    {
        if (wr & 0x80)
            Tcg_WriteByteToBGMap0(x++, y, a);
        wr <<= 1;
    }
}

// (PrintAttackOrCardDescription) a description at x, y (one line higher if it's 7 lines or more)
void Tcg_PrintAttackOrCardDescription(u32 x, u32 y, const u8 *text)
{
    const u8 *t;
    u32 lines = 1;
    Tcg_SetNoLineSeparation();
    for (t = text; *t; t++)
    {
        if (*t >= TCG_TX_CTRL_END)
            continue;
        if (*t < TCG_TX_HALFWIDTH)
            t++;
        else if (*t == '\n')
            lines++;
    }
    if (lines >= 7)
        y--;
    Tcg_InitTextPrintingInTextbox(x, y, 19);
    Tcg_ProcessText(text);
    Tcg_SetOneLineSeparation();
}

// ---- the pages -------------------------------------------------------------------------------------------

static void DrawCardPageSurroundingBox(void)
{
    G->textBoxFrameType |= 0x80;
    Tcg_DrawRegularTextBox(0, 0, 20, 18);
    G->textBoxFrameType &= ~0x80;
    Tcg_ApplyBGP6OrSGB3ToCardImage(6, 4);
}

// (LoadCardSet2Tiles) TRUE (the game's carry): no set 2 icon
static bool32 LoadCardSet2Tiles(u32 sets)
{
    static const s8 sTileOffsets[] = {-1, 0x0, 0x4, -1, -1, -1, -1, 0x8};
    s8 t = sTileOffsets[sets & 7];
    if (t < 0)
        return TRUE;
    Tcg_CopyFontsOrDuelGraphicsTiles(TCG_V_TILES1 + 0x7c * TCG_TILE_SIZE, TCG_GFX(DuelOtherGraphics) + (0x1d + t) * TCG_TILE_SIZE, 4);
    return FALSE;
}

static void DrawCardPageSet2AndRarityIcons(void)
{
    static const u16 sCardRarityTextIDs[] = {TCG_TEXT_PromostarRarity, TCG_TEXT_CircleRarity, TCG_TEXT_DiamondRarity, TCG_TEXT_StarRarity};
    if (!LoadCardSet2Tiles(gTcg->loadedCard1->sets))
        Tcg_FillRectangle(15, 8, 2, 2, 0xfc, 1, 2);
    if (gTcg->loadedCard1->rarity != TCG_PROMOSTAR)
        Tcg_InitTextPrinting_ProcessTextFromID(18, 9, sCardRarityTextIDs[(u8)(gTcg->loadedCard1->rarity + 1)]);
}

static void PrintPokemonCardPageGenericInformation(void)
{
    u32 color;
    DrawCardPageSurroundingBox();
    Tcg_InitTextPrinting_ProcessText(5, 1, gTcg->loadedCard1->name);
    if (G->cardPageType != TCG_CARDPAGETYPE_NOT_PLAY_AREA)
        color = Tcg_GetPlayAreaCardColor(gTcgCurPlayAreaSlot);
    else
        color = gTcg->loadedCard1->type;
    Tcg_WriteByteToBGMap0(18, 1, color + 1);
    DrawCardPageSet2AndRarityIcons();
}

static const struct { u8 x, y; u16 text; } sCardPageRetreatWRTextData[] =
{
    {1, 14, TCG_TEXT_RetreatCost},
    {1, 15, TCG_TEXT_Weakness},
    {1, 16, TCG_TEXT_Resistance},
};

static void PlaceCardPageRetreatWRText(void)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sCardPageRetreatWRTextData); i++)
        Tcg_InitTextPrinting_ProcessTextFromID(sCardPageRetreatWRTextData[i].x, sCardPageRetreatWRTextData[i].y, sCardPageRetreatWRTextData[i].text);
}

// (CardPageLvHPNoTextTileData: it has no $ff of its own, so its blocks run on into CardPageNoTextTileData's)
static const u8 sCardPageLvHPNoTextTileData[] =
{
    11, 2, TCG_SYM_Lv, 0,
    15, 2, TCG_SYM_HP, 0,
    15, 16, TCG_SYM_No, 0,
    0xff
};
static const u8 sCardPageNoTextTileData[] = {15, 16, TCG_SYM_No, 0, 0xff};
static const u8 sCardPageLvHPTextTileData[] = {11, 2, TCG_SYM_Lv, 0, 15, 2, TCG_SYM_HP, 0, 0xff};

static void DisplayCardPage_PokemonOverview(void)
{
    const struct TcgCard *c = gTcg->loadedCard1;
    u8 weakness, resistance;
    u32 x, n;
    if (G->cardPageType == TCG_CARDPAGETYPE_NOT_PLAY_AREA)
    {
        PrintPokemonCardPageGenericInformation();
        PlaceCardPageRetreatWRText();
        Tcg_WriteDataBlocksToBGMap0(sCardPageLvHPNoTextTileData);
        Tcg_DrawCardSymbol(3, 2);
        if (c->stage != TCG_BASIC)
            Tcg_InitTextPrinting_ProcessText(1, 3, c->preEvoName);
        Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(12, 2, c->level);
        Tcg_WriteOneByteNumberInTxSymbol_PadSpace(16, 2, c->hp);
    }
    else
    {
        DrawCardPageSurroundingBox();
        Tcg_LoadDuelCheckPokemonScreenTiles();
        PlaceCardPageRetreatWRText();
        Tcg_WriteDataBlocksToBGMap0(sCardPageNoTextTileData);
        gTcgCurPlayAreaY = 1;
        DrawCardPageSet2AndRarityIcons();
        Tcg_PrintPlayAreaCardInformationAndLocation();
    }
    Tcg_WriteOneByteNumberInTxSymbol_PadSpace(16, 16, c->dexNumber);
    Tcg_PrintAttackOrPkmnPowerInformation(&c->attacks[0], 10);
    Tcg_PrintAttackOrPkmnPowerInformation(&c->attacks[1], 12);
    for (x = 8, n = c->retreatCost; n; n--)
        Tcg_WriteByteToBGMap0(x++, 14, TCG_SYM_COLORLESS);
    if (G->cardPageType != TCG_CARDPAGETYPE_NOT_PLAY_AREA && gTcgCurPlayAreaSlot == 0)
    {
        weakness = Tcg_GetArenaCardWeakness();
        resistance = Tcg_GetArenaCardResistance();
    }
    else
    {
        weakness = c->weakness;
        resistance = c->resistance;
    }
    PrintCardPageWeaknessesOrResistances(8, 15, weakness);
    PrintCardPageWeaknessesOrResistances(8, 16, resistance);
}

static void DisplayPokemonAttackCardPage(const struct TcgAttack *attack, const u8 *description)
{
    PrintPokemonCardPageGenericInformation();
    Tcg_PrintAttackOrPkmnPowerInformation(attack, 2);
    if (description)
        Tcg_PrintAttackOrCardDescription(1, 11, description);
}

// (PrintPokemonCardLength's .print_feet_or_inches) the number, then the text; returns the next x
static u32 PrintFeetOrInches(u32 x, u32 y, u32 n, u16 text)
{
    u32 len = TwoByteNumberToTxSymbol_PadSpace_Bank1(n) + 1;
    Tcg_CopyDataToBGMap0(x, y, G->stringBuffer + 5 - len, len);
    x += len;
    Tcg_InitTextPrinting_ProcessTextFromID(x, y, text);
    return x + 1;
}

// (PrintPokemonCardWeight) the weight in tenths, as "n.n"; returns the x after it
static u32 PrintPokemonCardWeight(u32 x, u32 y, u32 weight)
{
    u8 *hl;
    const u8 *first;
    u32 n;
    TwoByteNumberToTxSymbol_PadSpace_Bank1(weight / 10);
    hl = G->stringBuffer + 5;
    if (weight % 10)
    {
        *hl++ = TCG_SYM_DOT;
        *hl++ = TCG_SYM_0 + weight % 10;
    }
    *hl = 0;
    for (first = G->stringBuffer; *first == 0; first++)
        ;
    for (n = 0; first[n]; n++)
        ;
    Tcg_CopyDataToBGMap0(x, y, first, n);
    return x + n;
}

static void DisplayCardPage_PokemonDescription(void)
{
    const struct TcgCard *c = gTcg->loadedCard1;
    u32 x, y;
    PrintPokemonCardPageGenericInformation();
    Tcg_LoadDuelCardSymbolTiles2();
    Tcg_InitTextPrinting_ProcessTextFromID(1, 11, TCG_TEXT_Length);
    Tcg_InitTextPrinting_ProcessTextFromID(1, 12, TCG_TEXT_Weight);
    Tcg_WriteDataBlocksToBGMap0(sCardPageLvHPTextTileData);
    Tcg_DrawCardSymbol(3, 2);
    Tcg_WriteTwoDigitNumberInTxSymbol_PadSpace(12, 2, c->level);
    Tcg_WriteOneByteNumberInTxSymbol_PadSpace(16, 2, c->hp);
    Tcg_InitTextPrinting_ProcessText(1, 10, c->category);
    G->japaneseSyllabary = TCG_TX_KATAKANA;     // (ProcessSpecialTextCharacter(TX_KATAKANA))
    Tcg_ProcessTextFromID(TCG_TEXT_Pokemon);
    x = PrintFeetOrInches(5, 11, c->length[0], TCG_TEXT_Feet);
    PrintFeetOrInches(x, 11, c->length[1], TCG_TEXT_Inches);
    x = PrintPokemonCardWeight(5, 12, c->weight);
    Tcg_InitTextPrinting_ProcessTextFromID(x, 12, TCG_TEXT_Lbs);
    Tcg_SetNoLineSeparation();
    {
        const u8 *t;
        u32 lines = 1;
        for (t = c->description; t && *t; t++)
        {
            if (*t >= TCG_TX_CTRL_END)
                continue;
            if (*t < TCG_TX_HALFWIDTH)
                t++;
            else if (*t == '\n')
                lines++;
        }
        y = lines >= 4 ? 13 : 14;
    }
    Tcg_InitTextPrintingInTextbox(1, y, 19);
    if (c->description)
        Tcg_ProcessText(c->description);
    Tcg_SetOneLineSeparation();
}

static void DisplayEnergyOrTrainerCardPage(u32 header, const u8 *description)
{
    Tcg_LoadCardTypeHeaderTiles(header);
    Tcg_DrawRegularTextBox(0, 0, 20, 18);
    Tcg_InitTextPrinting_ProcessText(4, 3, gTcg->loadedCard1->name);
    Tcg_ApplyBGP6OrSGB3ToCardImage(6, 4);
    Tcg_FillRectangle(6, 1, 8, 2, 0xe0, 1, 8);
    DrawCardPageSet2AndRarityIcons();
    if (description)
        Tcg_PrintAttackOrCardDescription(1, 11, description);
}

static void DisplayCardPage(void)
{
    const struct TcgCard *c = gTcg->loadedCard1;
    switch (G->cardPageNumber)
    {
    case TCG_CARDPAGE_POKEMON_OVERVIEW:
        DisplayCardPage_PokemonOverview();
        break;
    case TCG_CARDPAGE_POKEMON_ATTACK1_1:
        DisplayPokemonAttackCardPage(&c->attacks[0], c->attacks[0].description[0]);
        break;
    case TCG_CARDPAGE_POKEMON_ATTACK1_2:
        DisplayPokemonAttackCardPage(&c->attacks[0], c->attacks[0].description[1]);
        break;
    case TCG_CARDPAGE_POKEMON_ATTACK2_1:
        DisplayPokemonAttackCardPage(&c->attacks[1], c->attacks[1].description[0]);
        break;
    case TCG_CARDPAGE_POKEMON_ATTACK2_2:
        DisplayPokemonAttackCardPage(&c->attacks[1], c->attacks[1].description[1]);
        break;
    case TCG_CARDPAGE_POKEMON_DESCRIPTION:
        DisplayCardPage_PokemonDescription();
        break;
    case TCG_CARDPAGE_ENERGY:
    case TCG_CARDPAGE_ENERGY + 1:
        DisplayEnergyOrTrainerCardPage(TCG_HEADER_ENERGY, c->description);
        break;
    case TCG_CARDPAGE_TRAINER_1:
        DisplayEnergyOrTrainerCardPage(TCG_HEADER_TRAINER, c->description);
        break;
    case TCG_CARDPAGE_TRAINER_2:
        DisplayEnergyOrTrainerCardPage(TCG_HEADER_TRAINER, c->description2);
        break;
    default:
        Tcg_DrawDuelMainScene();
        break;
    }
    Tcg_EnableLCD();
}

// SwitchCardPage: whether the page exists (1: yes, 0: no); or, as the game's carry, the page to go
// to instead (*to)
static u32 SwitchCardPage(u32 page, u8 *to)
{
    const struct TcgCard *c = gTcg->loadedCard1;
    switch (page)
    {
    case 0:
        *to = TCG_CARDPAGE_POKEMON_DESCRIPTION;
        return 2;
    case TCG_CARDPAGE_POKEMON_OVERVIEW:
    case TCG_CARDPAGE_POKEMON_DESCRIPTION:
    case TCG_CARDPAGE_ENERGY:
    case TCG_CARDPAGE_TRAINER_1:
        return 1;
    case TCG_CARDPAGE_POKEMON_ATTACK1_1:
        return c->attacks[0].name != NULL;
    case TCG_CARDPAGE_POKEMON_ATTACK1_2:
        return c->attacks[0].description[1] != NULL;
    case TCG_CARDPAGE_POKEMON_ATTACK2_1:
        return c->attacks[1].name != NULL;
    case TCG_CARDPAGE_POKEMON_ATTACK2_2:
        return c->attacks[1].description[1] != NULL;
    case TCG_CARDPAGE_POKEMON_DESCRIPTION + 1:
        *to = TCG_CARDPAGE_POKEMON_OVERVIEW;
        return 2;
    case TCG_CARDPAGE_POKEMON_DESCRIPTION + 2:
        *to = TCG_CARDPAGE_ENERGY + 1;
        return 2;
    case TCG_CARDPAGE_ENERGY + 1:
    case TCG_CARDPAGE_TRAINER_2:
        return c->description2 != NULL;
    case TCG_CARDPAGE_ENERGY + 2:
        *to = TCG_CARDPAGE_ENERGY;
        return 2;
    case TCG_CARDPAGE_ENERGY + 3:
        *to = TCG_CARDPAGE_TRAINER_2;
        return 2;
    case TCG_CARDPAGE_TRAINER_2 + 1:
    default:
        *to = TCG_CARDPAGE_TRAINER_1;
        return 2;
    }
}

// GoToFirstOrNextCardPage: TRUE (the game's carry) past the last page
static bool32 GoToFirstOrNextCardPage(void)
{
    u8 to;
    if (G->cardPageNumber == 0)
    {
        u32 type = gTcg->loadedCard1->type;
        if (type & (1 << TCG_TYPE_ENERGY_F))
            G->cardPageNumber = TCG_CARDPAGE_ENERGY;
        else if (type & (1 << TCG_TYPE_TRAINER_F))
            G->cardPageNumber = TCG_CARDPAGE_TRAINER_1;
        else
            G->cardPageNumber = TCG_CARDPAGE_POKEMON_OVERVIEW;
        return FALSE;
    }
    for (;;)
    {
        u32 r = SwitchCardPage(++G->cardPageNumber, &to);
        if (r == 2)
        {
            G->cardPageNumber = to;
            return TRUE;
        }
        if (r)
            return FALSE;
    }
}

// GoToPreviousCardPage: TRUE (the game's carry) when it wrapped to the last page
static bool32 GoToPreviousCardPage(void)
{
    u8 to;
    for (;;)
    {
        u32 r = SwitchCardPage(--G->cardPageNumber, &to);
        if (r == 2)
            break;
        if (r)
            return FALSE;
    }
    G->cardPageNumber = to;
    while (SwitchCardPage(G->cardPageNumber, &to) == 0)
        G->cardPageNumber--;
    return TRUE;
}

// (DisplayFirstOrNextCardPage) TRUE: past the last page (nothing shown)
static bool32 DisplayFirstOrNextCardPage(void)
{
    if (GoToFirstOrNextCardPage())
        return TRUE;
    DisplayCardPage();
    return FALSE;
}

// (DisplayPreviousCardPage, and its caller's "call c, DisplayCardPage" when it wrapped)
static void DisplayPreviousCardPage(void)
{
    GoToPreviousCardPage();
    DisplayCardPage();
}

// the card image as 8x16 sprites (3 rows of 8 columns) at OAM x, y, from tile $a0, palette 1
void Tcg_PlaceCardImageOAM(u8 x, u8 y)
{
    u32 col, row;
    u8 tile = 0xa0;
    Tcg_Set_OBJ_8x16();
    for (col = 0; col < 8; col++, x += 8)
    {
        u8 ry = y;
        for (row = 0; row < 3; row++, ry += 16, tile += 2)
            Tcg_SetOneObjectAttributes(ry, x, tile, 1);
    }
    G->vBlankOAMCopyToggle = TRUE;
}

// OpenCardPage: wLoadedCard1's pages; A or START the next (out past the last), left and right turn
// them, the exit keys (B; from a list, up and down too) leave
void Tcg_OpenCardPage(u32 type, u8 exitKeys)
{
    G->cardPageExitKeys = exitKeys;
    G->cardPageType = type;
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    Tcg_FinishQueuedAnimations();
    Tcg_LoadDuelCardSymbolTiles();
    Tcg_LoadLoaded1CardGfx(TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE);
    Tcg_SetOBP1OrSGB3ToCardPalette();
    Tcg_SetBGP6OrSGB3ToCardPalette();
    Tcg_FlushAllPalettesOrSendPal23Packet();
    Tcg_PlaceCardImageOAM(0x38, 0x30);
    Tcg_ApplyBGP6OrSGB3ToCardImage(6, 4);
    G->cardPageNumber = 0;
    for (;;)
    {
        if (DisplayFirstOrNextCardPage())
            return;
        Tcg_EnableLCD();
        for (;;)
        {
            G->waitingFor = TCG_WAIT_CARD_PAGE;
            Tcg_DoFrame();
            if (G->dpadHeld & G->cardPageExitKeys)
                return;
            if (G->keysPressed & (TCG_PAD_START | TCG_PAD_A))
                break;
            if (G->keysPressed & TCG_PAD_LEFT)
                DisplayPreviousCardPage();
            else if (G->keysPressed & TCG_PAD_RIGHT)
            {
                if (DisplayFirstOrNextCardPage())
                    DisplayCardPage();
            }
        }
    }
}

void Tcg_OpenCardPage_FromHand(void)
{
    if (gTcg->headless)
        return;
    Tcg_OpenCardPage(TCG_CARDPAGETYPE_NOT_PLAY_AREA, TCG_PAD_B);
}

// (OpenActivePokemonScreen) the turn holder's Active Pokémon's page
void Tcg_OpenActivePokemonScreen(void)
{
    u32 arena = *Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD);
    if (arena == 0xFF)
        return;
    Tcg_LoadCard1(arena);
    gTcgCurPlayAreaSlot = 0;
    gTcgCurPlayAreaY = 0;
    Tcg_OpenCardPage_FromCheckPlayArea();
}

// ---- the large card picture ------------------------------------------------------------------------------

static const u8 sLargeCardTileData[] =
{
     5,  0, 0xd0, 0xd4, 0xd4, 0xd4, 0xd4, 0xd4, 0xd4, 0xd4, 0xd4, 0xd1, 0,
     5,  1, 0xd6, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xd7, 0,
     5,  2, 0xd6, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xd7, 0,
     5,  3, 0xd6, 0xa0, 0xa6, 0xac, 0xb2, 0xb8, 0xbe, 0xc4, 0xca, 0xd7, 0,
     5,  4, 0xd6, 0xa1, 0xa7, 0xad, 0xb3, 0xb9, 0xbf, 0xc5, 0xcb, 0xd7, 0,
     5,  5, 0xd6, 0xa2, 0xa8, 0xae, 0xb4, 0xba, 0xc0, 0xc6, 0xcc, 0xd7, 0,
     5,  6, 0xd6, 0xa3, 0xa9, 0xaf, 0xb5, 0xbb, 0xc1, 0xc7, 0xcd, 0xd7, 0,
     5,  7, 0xd6, 0xa4, 0xaa, 0xb0, 0xb6, 0xbc, 0xc2, 0xc8, 0xce, 0xd7, 0,
     5,  8, 0xd6, 0xa5, 0xab, 0xb1, 0xb7, 0xbd, 0xc3, 0xc9, 0xcf, 0xd7, 0,
     5,  9, 0xd6, 0,
    14,  9, 0xd7, 0,
     5, 10, 0xd6, 0,
    14, 10, 0xd7, 0,
     5, 11, 0xd2, 0xd5, 0xd5, 0xd5, 0xd5, 0xd5, 0xd5, 0xd5, 0xd5, 0xd3, 0,
    0xff
};

void Tcg_DrawLargePictureOfCard(void)
{
    u32 header;
    if (gTcg->headless)
        return;
    Tcg_ZeroObjectPositionsAndToggleOAMCopy();
    Tcg_EmptyScreen();
    Tcg_LoadSymbolsFont();
    Tcg_SetDefaultConsolePalettes();
    gTcg->duelDisplayedScreen = TCG_LARGE_CARD_PICTURE;
    Tcg_LoadCardOrDuelMenuBorderTiles();
    if (gTcg->loadedCard1->type == TCG_TYPE_TRAINER)
        header = TCG_HEADER_TRAINER;
    else if (gTcg->loadedCard1->type & TCG_TYPE_ENERGY)
        header = TCG_HEADER_ENERGY;
    else
        header = TCG_HEADER_POKEMON;
    Tcg_LoadCardTypeHeaderTiles(header);
    Tcg_LoadLoaded1CardGfx(TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE);
    Tcg_SetBGP6OrSGB3ToCardPalette();
    Tcg_FlushAllPalettesOrSendPal23Packet();
    Tcg_WriteDataBlocksToBGMap0(sLargeCardTileData);
    Tcg_ApplyBGP6OrSGB3ToCardImage(6, 3);
}

// DisplayCardDetailScreen: the card's large picture, and the text (its name and level as the
// default text)
void Tcg_DisplayCardDetailScreen(u32 deckIndex, u16 textId)
{
    if (gTcg->headless)
        return;
    Tcg_LoadCard1(deckIndex);
    Tcg_DrawLargePictureOfCard();
    Tcg_CopyCardNameAndLevel();
    gTcg->txRam2[0] = NULL;
    Tcg_DrawWideTextBox_WaitForInput(textId);
}

void Tcg_DisplayUsedTrainerCardDetailScreen(void)
{
    if (gTcg->headless)
        return;
    Tcg_DisplayCardDetailScreen(gTcg->tempCardIndexFf9f, TCG_TEXT_Used);
}

void Tcg_DrawWholeScreenTextBox(u16 textId)
{
    if (gTcg->headless)
        return;
    Tcg_EmptyScreen();
    Tcg_DrawRegularTextBox(0, 0, 20, 18);
    Tcg_InitTextPrintingInTextbox(1, 1, 19);
    Tcg_SetNoLineSeparation();
    Tcg_ProcessTextFromID(textId);
    Tcg_EnableLCD();
    Tcg_SetOneLineSeparation();
    Tcg_WaitForWideTextBoxInput();
}

// ---- the attack menu (DuelMenu_Attack's) ----------------------------------------------------------------

static EWRAM_DATA u8 sAttackPageNumber = 0;    // wAttackPageNumber (ATTACKPAGE_*)

// DisplayAttackPage: the page, and the switch to its other one (a second page only if it has one)
static void DisplayAttackPage(void)
{
    const struct TcgCard *c = gTcg->loadedCard1;
    switch (sAttackPageNumber)
    {
    case 0:
        DisplayPokemonAttackCardPage(&c->attacks[0], c->attacks[0].description[0]);
        break;
    case 1:
        if (c->attacks[0].description[1] == NULL)
            return;
        DisplayPokemonAttackCardPage(&c->attacks[0], c->attacks[0].description[1]);
        break;
    case 2:
        DisplayPokemonAttackCardPage(&c->attacks[1], c->attacks[1].description[0]);
        break;
    default:
        if (c->attacks[1].description[1] == NULL)
            return;
        DisplayPokemonAttackCardPage(&c->attacks[1], c->attacks[1].description[1]);
        break;
    }
    sAttackPageNumber ^= 1;
}

// OpenAttackPage: the selected attack's page (left/right: its other page), until A or B
static void OpenAttackPage(void)
{
    G->cardPageNumber = TCG_CARDPAGE_POKEMON_OVERVIEW;
    gTcgCurPlayAreaSlot = 0;
    Tcg_EmptyScreen();
    Tcg_FinishQueuedAnimations();
    Tcg_LoadLoaded1CardGfx(TCG_V_TILES1 + 0x20 * TCG_TILE_SIZE);
    Tcg_SetOBP1OrSGB3ToCardPalette();
    Tcg_SetBGP6OrSGB3ToCardPalette();
    Tcg_FlushAllPalettesOrSendPal23Packet();
    Tcg_PlaceCardImageOAM(0x38, 0x30);
    Tcg_ApplyBGP6OrSGB3ToCardImage(6, 4);
    gTcg->selectedDuelSubMenuItem = G->hCurMenuItem;
    sAttackPageNumber = gTcg->duelTempList[G->hCurMenuItem * 2 + 1] ? 2 : 0;
    for (;;)
    {
        DisplayAttackPage();
        Tcg_EnableLCD();
        for (;;)
        {
            Tcg_DoFrame();
            if (G->dpadHeld & (TCG_PAD_RIGHT | TCG_PAD_LEFT))
                break;
            if (G->keysPressed & (TCG_PAD_A | TCG_PAD_B))
                return;
        }
    }
}

static const struct TcgMenuParams sAttackMenuParameters = {1, 13, 2, 2, TCG_SYM_CURSOR_R, TCG_SYM_SPACE, NULL};

// the attack menu (PrintAndLoadAttacksToDuelTempList has printed it): an attack (its item), or
// TCG_SELECTION_CANCEL. START opens the attack's page (then the menu again, from the main scene).
u32 Tcg_HandleAttackMenuInput(u32 count, u32 item)
{
    if (gTcg->headless)
        return TCG_SELECTION_CANCEL;
    for (;;)
    {
        Tcg_InitializeMenuParameters(item, &sAttackMenuParameters);
        G->numMenuItems = count;
        Tcg_LoadCard1(*Tcg_TurnVar(TCG_DUELVARS_ARENA_CARD));
        for (;;)
        {
            G->waitingFor = TCG_WAIT_ATTACK_MENU;
            Tcg_DoFrame();
            if (G->keysPressed & TCG_PAD_START)
            {
                OpenAttackPage();
                Tcg_DrawDuelMainScene();
                count = Tcg_PrintAndLoadAttacksToDuelTempList();
                item = gTcg->selectedDuelSubMenuItem;
                break;
            }
            if (Tcg_HandleMenuInput())
                return G->hCurMenuItem == 0xFF ? TCG_SELECTION_CANCEL : G->hCurMenuItem;
        }
    }
}
