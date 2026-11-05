// Pokémon Canvas: the Trading Card Game's text engine, ported from the Game Boy Color game's
// home/process_text.asm, print_text.asm, text_box.asm and write_number.asm (pret/poketcg) under
// their own names (Tcg_ prefixed). The text is the game's own bytes (src/data/tcg/text.h): half-width
// letters go two to a tile, made as they're printed, kept in a most-recently-used list of tiles
// (SetupText's range) so the same pairs reuse theirs; symbols (TX_SYMBOL) are the symbol font's tiles.
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"
#include "string_util.h"
#include "constants/tcg_gfx.h"

#define G gTcgGbc

#define TCG_TEXT_HEADERS 4

static const u8 *const sFullWidthFonts = gTcgFontsGfx + TCG_GFX_OFFSET_FullWidthFonts;
static const u8 *const sHalfWidthFont = gTcgFontsGfx + TCG_GFX_OFFSET_HalfWidthFont;

// (GetTextOffsetFromTextID)
const u8 *Tcg_GetText(u16 textId)
{
    if (textId == TCG_TEXT_NONE || textId >= TCG_NUM_TEXTS)
        return NULL;
    return gTcgTexts[textId];
}

// ---- process_text.asm -------------------------------------------------------------------------------

void Tcg_InitTextFormat(void)
{
    G->fontWidth = TCG_FULL_WIDTH;
    G->textLineCurPos = 0;
    G->halfWidthPrintState = 0;
    G->japaneseSyllabary = TCG_TX_KATAKANA;
}

void Tcg_InitTextPrinting(u32 x, u32 y)
{
    G->textHorizontalAlign = x;
    G->textLineLength = 0;
    G->curTextLine = 0;
    G->textBGMap0Address = Tcg_DECoordToBGMap0Address(x, y);
    Tcg_InitTextFormat();
    G->halfWidthPrintState = 0;
}

void Tcg_InitTextPrintingInTextbox(u32 x, u32 y, u32 lineLength)
{
    Tcg_InitTextPrinting(x, y);
    G->textLineLength = lineLength;
}

// the text tiles: from firstTile to lastTile (the game's d, e)
void Tcg_SetupText(u32 firstTile, u32 lastTile)
{
    G->textNextTile = firstTile - 1;
    G->textLastTile = lastTile;
    Tcg_InitTextFormat();
    G->textFlags = 0;
    G->textTileHead = 0;
    G->tilePatternSelector = 0x88;
    G->tilePatternSelectorCorrection = 0x80;
    memset(G->textTileKey1, 0, sizeof(G->textTileKey1));
}

void Tcg_PlaceNextTextTile(u8 tile)
{
    Tcg_WriteVram(G->textBGMap0Address++, &tile, 1);
    G->textLineCurPos++;
}

static void CaseHalfWidthLetter(u8 *e)
{
    if (!G->uppercaseHalfWidthLetters)
        return;
    if (*e >= 0x60 && *e < 0x7b)
        *e -= 'a' - 'A';
}

// Func_235e: the tile for the characters (e, d), moved to the front of the list. Half-width: the first
// of a pair is held (returns 1). Returns 2 (the game's carry) found, 0 not found (the pair in e, d).
static u32 FindTextTile(u8 *e, u8 *d)
{
    u32 l, prev, next, head;
    if (G->fontWidth != TCG_FULL_WIDTH)
    {
        CaseHalfWidthLetter(e);
        *d = G->halfWidthPrintState;
        if (*d == 0)
        {
            G->halfWidthPrintState = *e;
            return 1;
        }
    }
    G->halfWidthPrintState = 0;
    for (l = G->textTileHead;; l = G->textTileNext[l])
    {
        if (G->textTileKey1[l] == 0)
            return 0;
        if (G->textTileKey1[l] == *e && G->textTileKey2[l] == *d)
            break;
    }
    head = G->textTileHead;
    if (head == l)
        return 2;
    G->textTilePrev[head] = l;
    G->textTileHead = l;
    prev = G->textTilePrev[l];
    G->textTilePrev[l] = 0;
    next = G->textTileNext[l];
    G->textTileNext[l] = head;
    G->textTileNext[prev] = next;
    if (next)
        G->textTilePrev[next] = prev;
    return 2;
}

// Func_2325: FindTextTile, or a tile for the pair (a new one, or the least recently used): *tile
static u32 FindOrAllocTextTile(u8 *e, u8 *d, u8 *tile)
{
    u32 r = FindTextTile(e, d), l, head;
    if (r)
        return r;
    if (G->textLastTile == G->textNextTile)
    {
        // all used: the list's last
        for (l = G->textTileHead; G->textTileNext[l]; l = G->textTileNext[l])
            ;
        G->textTileNext[G->textTilePrev[l]] = 0;
    }
    else
    {
        if (++G->textNextTile == 0)
            G->textNextTile++;
        l = G->textNextTile;
    }
    head = G->textTileHead;
    G->textTilePrev[head] = l;
    G->textTileHead = l;
    G->textTileNext[l] = head;
    G->textTileKey1[l] = *e;
    G->textTileKey2[l] = *d;
    *tile = l;
    return 0;
}

// (ConvertTileNumberToTileDataAddress)
static u32 TileDataAddress(u8 tile)
{
    return ((tile ^ G->tilePatternSelectorCorrection) * 16) + (G->tilePatternSelector << 8);
}

// GetFullWidthFontTileOffset (from FullWidthFonts)
static u32 GetFullWidthFontTileOffset(u8 e, u8 d)
{
    u32 base = 0x50 * TCG_TILE_SIZE_1BPP;
    if (d == TCG_TX_KATAKANA)
    {
        base = 0;
        e -= 0x10;
        d = 0;
    }
    else if (d == TCG_TX_HIRAGANA)
    {
        d = 0;
    }
    return ((d << 8) | e) * 8 + base;
}

static void CopyHalfWidthCharacter(u8 c, u8 *dest)
{
    const u8 *g = sHalfWidthFont + (u8)(c - 0x20) * 8;
    u32 y;
    for (y = 0; y < TCG_TILE_SIZE_1BPP; y++)
        dest[y * 2] = g[y];
}

// GenerateTextTile: the tile for (e, d) made at its tile number
static void GenerateTextTile(u8 e, u8 d, u8 tile)
{
    u8 *buf = G->textTileBuffer;
    u32 y;
    if (G->fontWidth == TCG_FULL_WIDTH)
    {
        const u8 *g = sFullWidthFonts + GetFullWidthFontTileOffset(e, d);
        for (y = 0; y < TCG_TILE_SIZE_1BPP; y++)
            buf[y * 2] = buf[y * 2 + 1] = g[y];
    }
    else
    {
        // CreateHalfWidthFontTile: e's in the high nibble, d's in the low
        CopyHalfWidthCharacter(e, buf);
        CopyHalfWidthCharacter(d, buf + 1);
        for (y = 0; y < TCG_TILE_SIZE_1BPP; y++)
        {
            u8 a = ((buf[y * 2] << 4) | (buf[y * 2] >> 4)) | buf[y * 2 + 1];
            buf[y * 2] = buf[y * 2 + 1] = a;
        }
    }
    Tcg_WriteVram(TileDataAddress(tile), buf, TCG_TILE_SIZE);
}

// Func_22ca: a character (pair) printed
static void PrintCharacter(u8 e, u8 d)
{
    u8 tile;
    if (G->textFlags & 1)
    {
        FindTextTile(&e, &d);
        return;
    }
    switch (FindOrAllocTextTile(&e, &d, &tile))
    {
    case 1:
        return;
    case 0:
        GenerateTextTile(e, d, tile);
        break;
    }
    if (!(G->textFlags & 2))
        Tcg_PlaceNextTextTile(G->textTileHead);
}

void Tcg_TerminateHalfWidthText(void)
{
    if (G->fontWidth == TCG_FULL_WIDTH || G->halfWidthPrintState == 0)
        return;
    PrintCharacter(' ', 0);
}

// ClassifyTextCharacterPair: full-width, the pair's (e, d) for the font; TRUE (the game's carry): the
// two bytes were a TX_FULLWIDTH* and its character (one more byte read)
static bool32 ClassifyTextCharacterPair(u8 *e, u8 *d)
{
    if (G->fontWidth != TCG_FULL_WIDTH)
        return FALSE;
    if (*e >= TCG_TX_CTRL_END)
    {
        if (*e < 0x60 && G->japaneseSyllabary == TCG_TX_KATAKANA)
            *d = TCG_TX_KATAKANA;
        else
            *d = 0;
        return FALSE;
    }
    if (*e >= TCG_TX_CTRL_START)
    {
        *d = 0;
        return FALSE;
    }
    // TX_FULLWIDTH1..4: (the table, the character)
    {
        u8 a = *e;
        *e = *d;
        *d = a;
    }
    return TRUE;
}

static void NextLine(void)
{
    G->textLineCurPos = 0;
    G->textBGMap0Address = (G->textBGMap0Address & 0xFFE0) + G->textHorizontalAlign + TCG_TILEMAP_WIDTH;
    G->curTextLine++;
}

// ProcessSpecialTextCharacter: TRUE (the game's carry) for the ones it doesn't handle (TX_RAM*);
// *text past a TX_SYMBOL's symbol
static bool32 ProcessSpecialTextCharacter(u8 c, const u8 **text)
{
    switch (c)
    {
    case TCG_TX_END:
        break;
    case TCG_TX_HIRAGANA:
    case TCG_TX_KATAKANA:
        G->japaneseSyllabary = c;
        return FALSE;
    case '\n':
        goto end_of_line;
    case TCG_TX_SYMBOL:
    {
        u8 width = G->fontWidth;
        G->fontWidth = TCG_HALF_WIDTH;
        Tcg_TerminateHalfWidthText();
        G->fontWidth = width;
        if (G->textFlags == 0)
            Tcg_PlaceNextTextTile(**text);
        (*text)++;
        break;
    }
    case TCG_TX_HALFWIDTH:
        G->fontWidth = TCG_HALF_WIDTH;
        return FALSE;
    case TCG_TX_HALF2FULL:
        Tcg_TerminateHalfWidthText();
        G->fontWidth = TCG_FULL_WIDTH;
        G->japaneseSyllabary = TCG_TX_KATAKANA;
        return FALSE;
    default:
        return TRUE;
    }
    // .tx_end
    if (G->textLineLength == 0 || G->textLineCurPos != G->textLineLength)
        return FALSE;
end_of_line:
    Tcg_TerminateHalfWidthText();
    if (G->lineSeparation == 0)
        NextLine();     // (DOUBLE_SPACED: an extra line)
    NextLine();
    return FALSE;
}

// one character (or pair) of text at *text: TRUE (the game's carry) for a TX_RAM*, in *c
static bool32 ProcessOneCharacter(const u8 **text, u8 *c)
{
    u8 e = *(*text)++, d;
    *c = e;
    if (e >= TCG_TX_CTRL_START && e < TCG_TX_CTRL_END)
        return ProcessSpecialTextCharacter(e, text);
    d = **text;
    if (ClassifyTextCharacterPair(&e, &d))
        (*text)++;
    PrintCharacter(e, d);
    ProcessSpecialTextCharacter(TCG_TX_END, text);
    return FALSE;
}

void Tcg_ProcessText(const u8 *text)
{
    u8 c;
    Tcg_InitTextFormat();
    while (*text)
        ProcessOneCharacter(&text, &c);
    Tcg_TerminateHalfWidthText();
}

void Tcg_ProcessTextFromID(u16 textId)
{
    const u8 *t = Tcg_GetText(textId);
    if (t)
        Tcg_ProcessText(t);
}

void Tcg_InitTextPrinting_ProcessText(u32 x, u32 y, const u8 *text)
{
    Tcg_InitTextPrinting(x, y);
    Tcg_ProcessText(text);
}

void Tcg_InitTextPrinting_ProcessTextFromID(u32 x, u32 y, u16 textId)
{
    Tcg_InitTextPrinting(x, y);
    Tcg_ProcessTextFromID(textId);
}

// GetTextLengthInHalfTiles: the length in half tiles (*chars: the bytes, the game's c)
u32 Tcg_GetTextLengthInHalfTiles(const u8 *text, u32 *chars)
{
    u32 b = 0, c = 0;
    u8 a;
    while ((a = *text++) != TCG_TX_END)
    {
        c++;
        if (a >= TCG_TX_CTRL_START && a < TCG_TX_CTRL_END)
        {
            if (a != TCG_TX_SYMBOL)
                continue;
            b++;
        }
        else
        {
            u8 e = a, d = *text;
            b++;
            if (!ClassifyTextCharacterPair(&e, &d))
                continue;
        }
        c++;
        text++;
    }
    if (chars)
        *chars = c;
    return b;
}

// GetTextLengthInTiles: the length in tiles (the game returns its negative, for centring); half-width
// text counts half tiles, rounded up
s32 Tcg_GetTextLengthInTiles(const u8 *text, u32 *halfTiles)
{
    u32 b;
    if (*text == TCG_TX_HALFWIDTH)
    {
        b = (Tcg_GetTextLengthInHalfTiles(text, NULL) + 1) >> 1;
    }
    else
    {
        G->fontWidth = TCG_FULL_WIDTH;
        b = Tcg_GetTextLengthInHalfTiles(text, NULL);
    }
    if (halfTiles)
        *halfTiles = b;
    return -(s32)b;
}

// TwoByteNumberToTxSymbol_PadSpace: n as 5 symbol digits (TX_SYMBOL, SYM_0+d), leading zeros as
// spaces; returns the last leading space's (or the first digit's TX_SYMBOL, the game's "dec hl")
u8 *Tcg_TwoByteNumberToTxSymbol_PadSpace(u16 n, u8 *dest)
{
    static const u16 sPowers[] = {10000, 1000, 100, 10, 1};
    u32 i;
    u8 *hl;
    for (i = 0; i < 5; i++)
    {
        dest[i * 2] = TCG_TX_SYMBOL;
        dest[i * 2 + 1] = TCG_SYM_0 + n / sPowers[i];
        n %= sPowers[i];
    }
    dest[10] = TCG_TX_END;
    hl = dest;
    for (i = 5; i; i--)
    {
        hl++;
        if (*hl != TCG_SYM_0)
            return hl - 1;
        *hl++ = TCG_SYM_SPACE;
    }
    hl--;
    *hl = TCG_SYM_0;
    return hl - 1;
}

// (write_number.asm) n as 5 ASCII digits
void Tcg_TwoByteNumberToText(u16 n, u8 *dest)
{
    static const u16 sPowers[] = {10000, 1000, 100, 10, 1};
    u32 i;
    for (i = 0; i < 5; i++)
    {
        dest[i] = '0' + n / sPowers[i];
        n %= sPowers[i];
    }
    dest[5] = TCG_TX_END;
}

// ---- print_text.asm -------------------------------------------------------------------------------------

u32 Tcg_CountLinesOfTextFromID(u16 textId)
{
    const u8 *t = Tcg_GetText(textId);
    u32 c = 0;
    u8 a;
    if (t == NULL)
        return 1;
    while ((a = *t++) != TCG_TX_END)
    {
        if (a >= TCG_TX_CTRL_END)
            continue;
        if (a < TCG_TX_HALFWIDTH)
        {
            t++;
            continue;
        }
        if (a == '\n')
            c++;
    }
    return c + 1;
}

static void WriteToTextHeader(const u8 *text)
{
    G->textHeaders[G->whichTextHeader].syllabary = G->japaneseSyllabary;
    G->textHeaders[G->whichTextHeader].fontWidth = G->fontWidth;
    G->textHeaders[G->whichTextHeader].text = text;
}

static void WriteToTextHeader_MoveToNext(const u8 *text)
{
    WriteToTextHeader(text);
    if (G->whichTextHeader < TCG_TEXT_HEADERS - 1)
        G->whichTextHeader++;
}

static const u8 *ReadTextHeader(void)
{
    G->japaneseSyllabary = G->textHeaders[G->whichTextHeader].syllabary;
    G->fontWidth = G->textHeaders[G->whichTextHeader].fontWidth;
    return G->textHeaders[G->whichTextHeader].text;
}

static void ResetTxRam_WriteToTextHeader(const u8 *text)
{
    G->whichTextHeader = G->whichTxRam2 = G->whichTxRam3 = 0;
    G->japaneseSyllabary = TCG_TX_KATAKANA;
    WriteToTextHeader(text);
}

// (TwoByteNumberToText_CountLeadingZeros: the number's text, in wStringBuffer)
static const u8 *NumberText(u16 n)
{
    u8 *hl = G->stringBuffer;
    u32 c;
    if (G->fontWidth == TCG_FULL_WIDTH)
        return Tcg_TwoByteNumberToTxSymbol_PadSpace(n, G->stringBuffer);
    Tcg_TwoByteNumberToText(n, G->stringBuffer);
    for (c = 4; c && *hl == '0'; c--)
        hl++;
    return hl;
}

void Tcg_CopyPlayerName(u8 *dest)
{
    Tcg_CopyDuelistName(TCG_PLAYER, dest);
}

void Tcg_CopyOpponentName(u8 *dest)
{
    Tcg_CopyDuelistName(TCG_OPPONENT, dest);
}

// ProcessTextHeader: one character of the text in the current header; TRUE (the game's carry) at the end
static bool32 ProcessTextHeader(void)
{
    for (;;)
    {
        const u8 *text = ReadTextHeader();
        u8 c;
        if (*text == TCG_TX_END)
        {
            if (G->whichTextHeader == 0)
            {
                Tcg_TerminateHalfWidthText();
                return TRUE;
            }
            G->whichTextHeader--;
            continue;
        }
        if (!ProcessOneCharacter(&text, &c))
        {
            WriteToTextHeader(text);
            return FALSE;
        }
        switch (c)
        {
        case TCG_TX_RAM2:
        {
            const u8 *t;
            WriteToTextHeader_MoveToNext(text);
            G->japaneseSyllabary = TCG_TX_KATAKANA;
            G->fontWidth = TCG_FULL_WIDTH;
            t = gTcg->txRam2[G->whichTxRam2++ & 1];
            WriteToTextHeader(t ? t : gTcg->defaultText);
            continue;
        }
        case TCG_TX_RAM3:
            WriteToTextHeader_MoveToNext(text);
            WriteToTextHeader(NumberText(gTcg->txRam3[G->whichTxRam3++ & 1]));
            continue;
        case TCG_TX_RAM1:
            WriteToTextHeader_MoveToNext(text);
            if (gTcg->whoseTurn == TCG_OPPONENT)
                Tcg_CopyOpponentName(G->stringBuffer);
            else
                Tcg_CopyPlayerName(G->stringBuffer);
            if (G->stringBuffer[0] != TCG_TX_HALFWIDTH)
                ProcessSpecialTextCharacter(TCG_TX_HALF2FULL, &text);
            WriteToTextHeader(G->stringBuffer);
            continue;
        default:
            WriteToTextHeader(text);
            return FALSE;
        }
    }
}

static void DrawTextReadyLabeledOrRegularTextBox(void)
{
    u8 x = 0, y = 12;
    Tcg_AdjustCoordinatesForBGScroll(&x, &y);
    if (G->isTextBoxLabeled)
    {
        Tcg_DrawLabeledTextBox(x, y, 20, 6, G->textBoxLabel);
    }
    else
    {
        Tcg_DrawRegularTextBox(x, y, 20, 6);
        Tcg_EnableLCD();
    }
    x = 1;
    y = 14;
    Tcg_AdjustCoordinatesForBGScroll(&x, &y);
    Tcg_InitTextPrintingInTextbox(x, y, 19);
}

static void PrintScrollableText(const u8 *text, bool32 labeled)
{
    G->isTextBoxLabeled = labeled;
    DrawTextReadyLabeledOrRegularTextBox();
    ResetTxRam_WriteToTextHeader(text);
    for (;;)
    {
        u32 c = G->textSpeed + 1;
        while (--c)
        {
            if (G->textSpeed < TCG_TEXT_SPEED_3 && (G->keysHeld & TCG_PAD_B))
                break;
            Tcg_DoFrame();
        }
        if (ProcessTextHeader())
            return;
        if (G->curTextLine >= 3)
        {
            Tcg_WaitForPlayerToAdvanceText();
            DrawTextReadyLabeledOrRegularTextBox();
        }
    }
}

void Tcg_PrintScrollableText_WithTextBoxLabel_NoWait(u16 textId, u16 labelId)
{
    G->textBoxLabel = Tcg_GetText(labelId);
    PrintScrollableText(Tcg_GetText(textId), TRUE);
}

void Tcg_PrintScrollableText_WithTextBoxLabel(u16 textId, u16 labelId)
{
    Tcg_PrintScrollableText_WithTextBoxLabel_NoWait(textId, labelId);
    Tcg_WaitForPlayerToAdvanceText();
}

void Tcg_PrintScrollableText_NoTextBoxLabel(u16 textId)
{
    if (gTcg->headless)
        return;
    PrintScrollableText(Tcg_GetText(textId), FALSE);
    Tcg_WaitForPlayerToAdvanceText();
}

void Tcg_WaitForPlayerToAdvanceText(void)
{
    Tcg_SetCursorParametersForTextBox(18, 17, TCG_SYM_CURSOR_D, TCG_SYM_BOX_BOTTOM);
    Tcg_WaitForButtonAorB();
}

static void PrintTextFrom(const u8 *text)
{
    ResetTxRam_WriteToTextHeader(text);
    do
    {
        u32 a = G->textSpeed + 1;
        if (a < TCG_TEXT_SPEED_3 + 1 && (G->keysHeld & TCG_PAD_B))
            continue;
        while (--a)
            Tcg_DoFrame();
    } while (!ProcessTextHeader());
}

// (PrintText: TEXT_NONE prints wDefaultText)
void Tcg_PrintText(u16 textId)
{
    const u8 *t = Tcg_GetText(textId);
    PrintTextFrom(t ? t : gTcg->defaultText);
}

void Tcg_PrintTextNoDelay(u16 textId)
{
    const u8 *t = Tcg_GetText(textId);
    ResetTxRam_WriteToTextHeader(t ? t : gTcg->defaultText);
    while (!ProcessTextHeader())
        ;
}

// CopyText: the text to dest (TEXT_NONE: the turn holder's name); returns its end (its TX_END)
u8 *Tcg_CopyText(u16 textId, u8 *dest)
{
    const u8 *t = Tcg_GetText(textId);
    if (t == NULL)
    {
        if (gTcg->whoseTurn == TCG_OPPONENT)
            Tcg_CopyOpponentName(dest);
        else
            Tcg_CopyPlayerName(dest);
        while (*dest)
            dest++;
        return dest;
    }
    while ((*dest = *t++) != TCG_TX_END)
        dest++;
    return dest;
}

// ---- text_box.asm ----------------------------------------------------------------------------------------

// CopyLine: a line of the box at addr: left, middle x (width - 2), right
void Tcg_CopyLine(u32 addr, u32 width, u8 left, u8 middle, u8 right)
{
    u8 line[TCG_TILEMAP_WIDTH];
    u32 i;
    line[0] = left;
    for (i = 1; i < width - 1; i++)
        line[i] = middle;
    line[width - 1] = right;
    Tcg_WriteVram(addr, line, width);
}

// CopyCurrentLineAttrCGB: the line's attributes (bank 1) from wTextBoxFrameType
static void CopyCurrentLineAttrCGB(u32 addr, u32 width)
{
    u8 t = G->textBoxFrameType;
    Tcg_BankswitchVRAM1();
    Tcg_CopyLine(addr, width, t, t, t);
    Tcg_BankswitchVRAM0();
}

// ContinueDrawingTextBoxCGB: from the line at addr: the sides, the bottom
static void ContinueDrawingTextBoxCGB(u32 addr, u32 w, u32 h)
{
    for (h -= 2; h; h--)
    {
        Tcg_CopyLine(addr, w, TCG_SYM_BOX_LEFT, TCG_SYM_SPACE, TCG_SYM_BOX_RIGHT);
        Tcg_BankswitchVRAM1();
        Tcg_CopyLine(addr, w, G->textBoxFrameType, 0, G->textBoxFrameType);
        Tcg_BankswitchVRAM0();
        addr += TCG_TILEMAP_WIDTH;
    }
    Tcg_CopyLine(addr, w, TCG_SYM_BOX_BTM_L, TCG_SYM_BOX_BOTTOM, TCG_SYM_BOX_BTM_R);
    CopyCurrentLineAttrCGB(addr, w);
}

// (DrawRegularTextBoxCGB)
void Tcg_DrawRegularTextBox(u32 x, u32 y, u32 w, u32 h)
{
    u32 addr = Tcg_DECoordToBGMap0Address(x, y);
    Tcg_CopyLine(addr, w, TCG_SYM_BOX_TOP_L, TCG_SYM_BOX_TOP, TCG_SYM_BOX_TOP_R);
    CopyCurrentLineAttrCGB(addr, w);
    ContinueDrawingTextBoxCGB(addr + TCG_TILEMAP_WIDTH, w, h);
}

// a box with the label in its top line
void Tcg_DrawLabeledTextBox(u32 x, u32 y, u32 w, u32 h, const u8 *label)
{
    u8 *hl = G->labeledTextBoxTopBorder;
    u32 len, n, addr;
    const u8 *t = label ? label : (const u8 *)"";
    *hl++ = TCG_TX_SYMBOL;
    *hl++ = TCG_SYM_BOX_TOP_L;
    *hl++ = 0x70;   // (ldfw " ": the full-width space)
    while ((*hl = *t++) != TCG_TX_END)
        hl++;
    Tcg_GetTextLengthInTiles(G->labeledTextBoxTopBorder + 3, &len);
    *hl++ = TCG_TX_HALF2FULL;
    *hl++ = 0x70;
    for (n = (u8)(w - len - 4); n; n--)
    {
        *hl++ = TCG_TX_SYMBOL;
        *hl++ = TCG_SYM_BOX_TOP;
    }
    *hl++ = TCG_TX_SYMBOL;
    *hl++ = TCG_SYM_BOX_TOP_R;
    *hl = TCG_TX_END;
    Tcg_InitTextPrinting(x, y);
    Tcg_ProcessText(G->labeledTextBoxTopBorder);
    addr = Tcg_DECoordToBGMap0Address(x, y);
    CopyCurrentLineAttrCGB(addr, w);
    ContinueDrawingTextBoxCGB(addr + TCG_TILEMAP_WIDTH, w, h);
}
