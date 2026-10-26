// Gym Trainer hack (Pokémon Canvas): BILL'S ARCHIVE, the living dex.
//
// One slot for every species and form the player could keep (src/data/dex_forms.h, National Dex
// order, each form after its species). ARCHIVE in the PC's box menu (src/pokemon_storage_system.c)
// sends a Pokémon there: it's gone, and only its slot - stored, and whether it was shiny - is kept
// (a slot already full turns it away). Taking one out gives a new one at level 5: random IVs,
// nature and one of its usual abilities (shiny if the one stored was), and the slot empties. So
// there's never more than one of anything: a legendary stays one of a kind.
//
// The screen (the PC's "BILL'S ARCHIVE", or the Palm Pilot's) is laid out like a PC box: 30 icons a
// page (6x5), in dex order. Stored: in color. In a box or the party: in color, with a ball by it.
// Not owned: a dark silhouette. Shiny ones stored: a star. On the left, the one under the cursor -
// its picture, name, number and where it is - and how much of the dex is owned (stored, in a box
// or in the party). L/R turn the page, START jumps to the next generation, A takes one out.
//
// The slots are kept by species number in flash sector 30 with the Type Mastery Hall of Fame
// (struct ArchiveData, src/save.c).
#include "global.h"
#include "archive.h"
#include "bg.h"
#include "dex_forms.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "hall_of_fame.h"
#include "main.h"
#include "malloc.h"
#include "mastery_hall.h"
#include "menu.h"
#include "menu_helpers.h"
#include "script.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_pokemon_sprites.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define ARCHIVE_MAGIC 0x41524331     // "ARC1"

// ---- The slots ----
static bool32 GetBit(const u8 *bits, u32 species)
{
    return species < NUM_SPECIES && (bits[species / 8] >> (species % 8)) & 1;
}

static void SetBit(u8 *bits, u32 species, bool32 on)
{
    if (species >= NUM_SPECIES)
        return;
    if (on)
        bits[species / 8] |= 1 << (species % 8);
    else
        bits[species / 8] &= ~(1 << (species % 8));
}

void Archive_Clear(void)
{
    memset(&gExtraSave.archive, 0, sizeof(gExtraSave.archive));
    gExtraSave.archive.magic = ARCHIVE_MAGIC;
}

void Archive_Validate(void)
{
    if (gExtraSave.archive.magic != ARCHIVE_MAGIC)
        Archive_Clear();
}

bool32 Archive_IsSlot(u32 species)
{
    u32 i;
    for (i = 0; i < DEX_FORM_COUNT; i++)
    {
        if (gDexFormSpecies[i] == species)
            return TRUE;
    }
    return FALSE;
}

bool32 Archive_Has(u32 species)
{
    Archive_Validate();
    return GetBit(gExtraSave.archive.stored, species);
}

static bool32 IsStoredShiny(u32 species)
{
    return GetBit(gExtraSave.archive.shiny, species);
}

void Archive_Store(u32 species, bool32 shiny)
{
    Archive_Validate();
    SetBit(gExtraSave.archive.stored, species, TRUE);
    SetBit(gExtraSave.archive.shiny, species, shiny);
}

static void Take(u32 species)
{
    SetBit(gExtraSave.archive.stored, species, FALSE);
    SetBit(gExtraSave.archive.shiny, species, FALSE);
}

// ---- The screen ----
#define COLS        6
#define ROWS        5
#define PER_PAGE    (COLS * ROWS)
#define PAGE_COUNT  ((DEX_FORM_COUNT + PER_PAGE - 1) / PER_PAGE)
#define CELL        24
#define GRID_X      100     // (the first column's middle)
#define GRID_Y      36      // (the first row's)
#define PIC_X       40
#define PIC_Y       62
#define PIC_PAL     15      // (the picture's own OBJ palette slot)
#define LOC_PARTY   0xFF    // (sLoc: 0 nowhere, 1-16 that box, LOC_PARTY)
#define FRAME_TILE  0x1F0   // (BG0: the YES/NO window's frame)

#define TAG_UI      0x5A01
#define TAG_DARK    0x5A02

enum { WIN_TOP, WIN_COUNT, WIN_INFO, WIN_FOOTER, WIN_COUNT_ };
enum { ST_INPUT, ST_CONFIRM, ST_MESSAGE, ST_EXIT };
// The BG's colors (palette 0) and the text's (15)
enum { BG_PANEL = 1, BG_EDGE, BG_WALL_A, BG_WALL_B, BG_BAR, BG_FOOT };
enum { TXT_CLEAR, TXT_WHITE, TXT_DARK, TXT_GRAY, TXT_GOLD };

struct Archive
{
    u8 *loc;                    // (by species: where else it is)
    u8 icons[PER_PAGE];
    u8 balls[PER_PAGE];
    u8 stars[PER_PAGE];
    u8 cursor, pic;
    u16 page, cell;
    u8 state;
    u16 formsOwned, speciesOwned, speciesTotal;
    u8 *tilemap;
    u8 *bgTiles;
    u8 *uiTiles;
};

static EWRAM_DATA struct Archive *sArc = NULL;
static EWRAM_DATA u16 sLastIndex = 0;   // (where the cursor was last time)

static const struct BgTemplate sBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 1, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
    {.bg = 1, .charBaseIndex = 0, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_TOP]    = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 0,  .width = 30, .height = 2, .paletteNum = 15, .baseBlock = 1},
    [WIN_COUNT]  = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 2,  .width = 10, .height = 3, .paletteNum = 15, .baseBlock = 61},
    [WIN_INFO]   = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 12, .width = 10, .height = 6, .paletteNum = 15, .baseBlock = 91},
    [WIN_FOOTER] = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 18, .width = 30, .height = 2, .paletteNum = 15, .baseBlock = 151},
    DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sYesNoTemplate =
{
    .bg = 0, .tilemapLeft = 24, .tilemapTop = 11, .width = 5, .height = 4, .paletteNum = 15, .baseBlock = 220,
};

static const u8 sWhite[3] = {TXT_CLEAR, TXT_WHITE, TXT_DARK};
static const u8 sGray[3] = {TXT_CLEAR, TXT_GRAY, TXT_DARK};
static const u8 sGold[3] = {TXT_CLEAR, TXT_GOLD, TXT_DARK};

static const u16 sUiPalette[16] =
{
    0, RGB(31, 31, 31), RGB(4, 4, 7), RGB(30, 8, 8), RGB(31, 26, 4), RGB(31, 31, 12),
};

static const u16 sDarkPalette[16] =
{
    0, RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8),
    RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8), RGB(5, 5, 8),
};

static const struct OamData sOam32 = {.shape = SPRITE_SHAPE(32x32), .size = SPRITE_SIZE(32x32), .priority = 0};
static const struct OamData sOam8 = {.shape = SPRITE_SHAPE(8x8), .size = SPRITE_SIZE(8x8), .priority = 0};

// The UI sprites' tiles: the cursor (32x32, 16 tiles), then the ball (1) and the star (1)
#define UI_TILES        18
#define UI_TILE_BALL    16
#define UI_TILE_STAR    17

static const struct SpriteTemplate sCursorTemplate =
{
    .tileTag = TAG_UI, .paletteTag = TAG_UI, .oam = &sOam32, .anims = gDummySpriteAnimTable,
    .images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy,
};

static const struct SpriteTemplate sSmallTemplate =
{
    .tileTag = TAG_UI, .paletteTag = TAG_UI, .oam = &sOam8, .anims = gDummySpriteAnimTable,
    .images = NULL, .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy,
};

static void SetPixel(u8 *tiles, u32 tilesWide, u32 x, u32 y, u32 color)
{
    u8 *p = &tiles[((y / 8) * tilesWide + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2];
    *p = (x & 1) ? (*p & 0x0F) | (color << 4) : (*p & 0xF0) | color;
}

// The UI sprites, drawn here: a gold frame around a cell, a little POKé BALL, a star
static void LoadUiSprites(void)
{
    static const char *const ball[8] = {"  2222  ", " 233332 ", "23333332", "22222222", "21111112", "21111112", " 211112 ", "  2222  "};
    static const char *const star[8] = {"   22   ", "  2442  ", "22244222", "24444442", " 244442 ", " 242242 ", " 22  22 ", "        "};
    struct SpriteSheet sheet;
    struct SpritePalette pal = {sUiPalette, TAG_UI};
    struct SpritePalette dark = {sDarkPalette, TAG_DARK};
    u8 *t = sArc->uiTiles = AllocZeroed(UI_TILES * 32);
    u32 x, y;
    for (y = 2; y < 30; y++)
    {
        for (x = 2; x < 30; x++)
        {
            bool32 edge = x < 4 || x >= 28 || y < 4 || y >= 28;
            bool32 outline = x == 2 || x == 29 || y == 2 || y == 29;
            bool32 corner = (x < 9 || x > 22) && (y < 9 || y > 22);     // (corners only)
            if (edge && corner)
                SetPixel(t, 4, x, y, outline ? 2 : 4);
        }
    }
    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            if (ball[y][x] != ' ')
                SetPixel(t + UI_TILE_BALL * 32, 1, x, y, ball[y][x] - '0');
            if (star[y][x] != ' ')
                SetPixel(t + UI_TILE_STAR * 32, 1, x, y, star[y][x] - '0');
        }
    }
    sheet.data = t;
    sheet.size = UI_TILES * 32;
    sheet.tag = TAG_UI;
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&pal);
    LoadSpritePalette(&dark);
}

// The background: the left panel, the wall behind the icons (a soft check), the bars
static void LoadBackground(void)
{
    u8 *t = sArc->bgTiles = AllocZeroed(6 * 32);
    u16 pal[16] = {0};
    u32 x, y;
    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            SetPixel(t + 0 * 32, 1, x, y, BG_PANEL);
            SetPixel(t + 1 * 32, 1, x, y, ((x / 4) + (y / 4)) % 2 ? BG_WALL_A : BG_WALL_B);
            SetPixel(t + 2 * 32, 1, x, y, BG_BAR);
            SetPixel(t + 3 * 32, 1, x, y, BG_FOOT);
            SetPixel(t + 4 * 32, 1, x, y, x == 7 ? BG_EDGE : BG_PANEL);
            SetPixel(t + 5 * 32, 1, x, y, y == 7 ? BG_EDGE : BG_BAR);
        }
    }
    LoadBgTiles(1, t, 6 * 32, 0);
    pal[BG_PANEL] = RGB(6, 8, 13);
    pal[BG_EDGE] = RGB(2, 3, 6);
    pal[BG_WALL_A] = RGB(17, 22, 26);
    pal[BG_WALL_B] = RGB(19, 24, 28);
    pal[BG_BAR] = RGB(9, 12, 20);
    pal[BG_FOOT] = RGB(4, 5, 9);
    LoadPalette(pal, BG_PLTT_ID(0), sizeof(pal));
    pal[0] = 0;
    pal[TXT_WHITE] = RGB_WHITE;
    pal[TXT_DARK] = RGB(3, 3, 6);
    pal[TXT_GRAY] = RGB(19, 21, 25);
    pal[TXT_GOLD] = RGB(31, 26, 6);
    LoadPalette(pal, BG_PLTT_ID(15), sizeof(pal));

    sArc->tilemap = AllocZeroed(BG_SCREEN_SIZE);
    SetBgTilemapBuffer(1, sArc->tilemap);
    FillBgTilemapBufferRect_Palette0(1, 2, 0, 0, 32, 1);
    FillBgTilemapBufferRect_Palette0(1, 5, 0, 1, 32, 1);
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 2, 9, 16);
    FillBgTilemapBufferRect_Palette0(1, 4, 9, 2, 1, 16);
    FillBgTilemapBufferRect_Palette0(1, 1, 10, 2, 22, 16);
    FillBgTilemapBufferRect_Palette0(1, 3, 0, 18, 32, 2);
    CopyBgTilemapBufferToVram(1);
}

// Where everything the player has is: the party and the boxes
static void FindOwned(void)
{
    u32 box, pos, i;
    memset(sArc->loc, 0, NUM_SPECIES);
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            u32 species = GetBoxMonDataAt(box, pos, MON_DATA_SPECIES);
            if (species != SPECIES_NONE && species < NUM_SPECIES && !GetBoxMonDataAt(box, pos, MON_DATA_IS_EGG)
             && sArc->loc[species] == 0)
                sArc->loc[species] = box + 1;
        }
    }
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 species = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES);
        if (species != SPECIES_NONE && species < NUM_SPECIES && !GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            sArc->loc[species] = LOC_PARTY;
    }
    // ...and how much of the dex that is (stored, in a box or in the party)
    sArc->formsOwned = sArc->speciesOwned = sArc->speciesTotal = 0;
    for (i = 0; i < DEX_FORM_COUNT; i++)
    {
        u32 species = gDexFormSpecies[i];
        bool32 first = i == 0 || SpeciesToNationalPokedexNum(gDexFormSpecies[i - 1]) != SpeciesToNationalPokedexNum(species);
        bool32 owned = sArc->loc[species] != 0 || Archive_Has(species);
        u32 j;
        sArc->formsOwned += owned;
        if (!first)
            continue;
        sArc->speciesTotal++;
        // (a species is owned if any of its forms is)
        for (j = i; j < DEX_FORM_COUNT && SpeciesToNationalPokedexNum(gDexFormSpecies[j]) == SpeciesToNationalPokedexNum(species); j++)
        {
            if (sArc->loc[gDexFormSpecies[j]] != 0 || Archive_Has(gDexFormSpecies[j]))
            {
                sArc->speciesOwned++;
                break;
            }
        }
    }
}

static const u8 *FormName(u32 index)
{
    return gDexFormLabels[index] ? gDexFormLabels[index] : GetSpeciesName(gDexFormSpecies[index]);
}

static const u8 *const sRegions[] = {
    COMPOUND_STRING("KANTO"), COMPOUND_STRING("JOHTO"), COMPOUND_STRING("HOENN"), COMPOUND_STRING("SINNOH"),
    COMPOUND_STRING("UNOVA"), COMPOUND_STRING("KALOS"), COMPOUND_STRING("ALOLA"), COMPOUND_STRING("GALAR"),
    COMPOUND_STRING("PALDEA"),
};

static u32 GenOf(u32 index)
{
    u32 g = 0;
    while (g + 1 < DEX_FORM_GEN_COUNT && gDexFormGenStarts[g + 1] <= index)
        g++;
    return g;
}

static u8 *AppendDexNum(u8 *end, u32 species)
{
    return ConvertIntToDecimalStringN(end, SpeciesToNationalPokedexNum(species), STR_CONV_MODE_LEADING_ZEROS, 4);
}

static u32 CurrentIndex(void);

static void PrintTop(void)
{
    u8 *s = Alloc(64), *end;
    u32 first = sArc->page * PER_PAGE, last = min(first + PER_PAGE, (u32)DEX_FORM_COUNT) - 1;
    FillWindowPixelBuffer(WIN_TOP, PIXEL_FILL(0));
    AddTextPrinterParameterized4(WIN_TOP, FONT_NORMAL, 4, 1, 0, 0, sWhite, TEXT_SKIP_DRAW, COMPOUND_STRING("BILL'S ARCHIVE"));
    end = StringCopy(s, COMPOUND_STRING("No."));
    end = AppendDexNum(end, gDexFormSpecies[first]);
    end = StringAppend(end, COMPOUND_STRING("-"));
    end = AppendDexNum(end, gDexFormSpecies[last]);
    end = StringAppend(end, COMPOUND_STRING("  "));
    end = StringAppend(end, sRegions[min(GenOf(CurrentIndex()), ARRAY_COUNT(sRegions) - 1)]);
    end = StringAppend(end, COMPOUND_STRING("  "));
    end = ConvertIntToDecimalStringN(end, sArc->page + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
    end = StringAppend(end, COMPOUND_STRING("/"));
    ConvertIntToDecimalStringN(end, PAGE_COUNT, STR_CONV_MODE_LEFT_ALIGN, 2);
    AddTextPrinterParameterized4(WIN_TOP, FONT_SMALL, 236 - GetStringWidth(FONT_SMALL, s, 0), 3, 0, 0, sGray, TEXT_SKIP_DRAW, s);
    Free(s);
    CopyWindowToVram(WIN_TOP, COPYWIN_FULL);
}

static void PrintCounts(void)
{
    u8 *s = Alloc(32), *end;
    FillWindowPixelBuffer(WIN_COUNT, PIXEL_FILL(0));
    end = StringCopy(s, COMPOUND_STRING("SPECIES "));
    end = ConvertIntToDecimalStringN(end, sArc->speciesOwned, STR_CONV_MODE_LEFT_ALIGN, 4);
    end = StringAppend(end, COMPOUND_STRING("/"));
    ConvertIntToDecimalStringN(end, sArc->speciesTotal, STR_CONV_MODE_LEFT_ALIGN, 4);
    AddTextPrinterParameterized4(WIN_COUNT, FONT_SMALL, 3, 1, 0, 0, sGray, TEXT_SKIP_DRAW, s);
    end = StringCopy(s, COMPOUND_STRING("FORMS "));
    end = ConvertIntToDecimalStringN(end, sArc->formsOwned, STR_CONV_MODE_LEFT_ALIGN, 4);
    end = StringAppend(end, COMPOUND_STRING("/"));
    ConvertIntToDecimalStringN(end, DEX_FORM_COUNT, STR_CONV_MODE_LEFT_ALIGN, 4);
    AddTextPrinterParameterized4(WIN_COUNT, FONT_SMALL, 3, 12, 0, 0, sGray, TEXT_SKIP_DRAW, s);
    Free(s);
    CopyWindowToVram(WIN_COUNT, COPYWIN_FULL);
}

static void PrintFooter(const u8 *text, u32 font)
{
    FillWindowPixelBuffer(WIN_FOOTER, PIXEL_FILL(0));
    AddTextPrinterParameterized4(WIN_FOOTER, font, 4, font == FONT_SMALL ? 3 : 1, 0, 0, sWhite, TEXT_SKIP_DRAW, text);
    CopyWindowToVram(WIN_FOOTER, COPYWIN_FULL);
}

static void PrintHelp(void)
{
    PrintFooter(COMPOUND_STRING("{A_BUTTON} TAKE OUT   {L_BUTTON}{R_BUTTON} PAGE   START NEXT GENERATION   {B_BUTTON} EXIT"), FONT_SMALL);
}

static u32 CurrentIndex(void)
{
    return sArc->page * PER_PAGE + sArc->cell;
}

// The one under the cursor: its picture (a silhouette if not owned), name, number, where it is
static void ShowCurrent(void)
{
    u32 index = CurrentIndex(), species = gDexFormSpecies[index], loc = sArc->loc[species];
    bool32 stored = Archive_Has(species), owned = stored || loc != 0, shiny = stored && IsStoredShiny(species);
    u8 *s = Alloc(48), *end;

    if (sArc->pic != 0xFF)
        FreeAndDestroyMonPicSprite(sArc->pic);
    sArc->pic = CreateMonPicSprite(species, shiny, 0, TRUE, PIC_X, PIC_Y, PIC_PAL, TAG_NONE);
    if (sArc->pic != 0xFFFF && sArc->pic < MAX_SPRITES)
    {
        gSprites[sArc->pic].oam.priority = 1;
        if (!owned)
            LoadPalette(sDarkPalette, OBJ_PLTT_ID(PIC_PAL), PLTT_SIZE_4BPP);
    }
    else
    {
        sArc->pic = 0xFF;
    }

    FillWindowPixelBuffer(WIN_INFO, PIXEL_FILL(0));
    end = StringCopy(s, owned ? FormName(index) : COMPOUND_STRING("?????"));
    if (shiny)
        StringAppend(end, COMPOUND_STRING(" {STAR}"));
    AddTextPrinterParameterized4(WIN_INFO, FONT_SMALL, 3, 0, 0, 0, shiny ? sGold : sWhite, TEXT_SKIP_DRAW, s);
    end = StringCopy(s, COMPOUND_STRING("No."));
    AppendDexNum(end, species);
    AddTextPrinterParameterized4(WIN_INFO, FONT_SMALL, 3, 12, 0, 0, sGray, TEXT_SKIP_DRAW, s);
    if (stored)
        StringCopy(s, COMPOUND_STRING("IN THE ARCHIVE"));
    else if (loc == LOC_PARTY)
        StringCopy(s, COMPOUND_STRING("IN YOUR PARTY"));
    else if (loc != 0)
    {
        end = StringCopy(s, COMPOUND_STRING("IN BOX "));
        ConvertIntToDecimalStringN(end, loc, STR_CONV_MODE_LEFT_ALIGN, 2);
    }
    else
        StringCopy(s, COMPOUND_STRING("NOT OWNED"));
    AddTextPrinterParameterized4(WIN_INFO, FONT_SMALL, 3, 24, 0, 0, stored ? sGold : sWhite, TEXT_SKIP_DRAW, s);
    if (stored && loc != 0)
    {
        if (loc == LOC_PARTY)
            StringCopy(s, COMPOUND_STRING("(AND IN YOUR PARTY)"));
        else
        {
            end = StringCopy(s, COMPOUND_STRING("(AND IN BOX "));
            end = ConvertIntToDecimalStringN(end, loc, STR_CONV_MODE_LEFT_ALIGN, 2);
            StringAppend(end, COMPOUND_STRING(")"));
        }
        AddTextPrinterParameterized4(WIN_INFO, FONT_SMALL, 3, 36, 0, 0, sGray, TEXT_SKIP_DRAW, s);
    }
    Free(s);
    CopyWindowToVram(WIN_INFO, COPYWIN_FULL);
    PrintTop();

    gSprites[sArc->cursor].x = GRID_X + CELL * (sArc->cell % COLS);
    gSprites[sArc->cursor].y = GRID_Y + CELL * (sArc->cell / COLS);
}

static void DestroyPage(void)
{
    u32 i;
    for (i = 0; i < PER_PAGE; i++)
    {
        if (sArc->icons[i] != MAX_SPRITES)
            FreeAndDestroyMonIconSprite(&gSprites[sArc->icons[i]]);
        if (sArc->balls[i] != MAX_SPRITES)
            DestroySprite(&gSprites[sArc->balls[i]]);
        if (sArc->stars[i] != MAX_SPRITES)
            DestroySprite(&gSprites[sArc->stars[i]]);
        sArc->icons[i] = sArc->balls[i] = sArc->stars[i] = MAX_SPRITES;
    }
}

// The page's 30: icons (silhouettes for those not owned), balls and stars
static void DrawPage(void)
{
    u32 i, dark = IndexOfSpritePaletteTag(TAG_DARK), tile = GetSpriteTileStartByTag(TAG_UI);
    DestroyPage();
    for (i = 0; i < PER_PAGE; i++)
    {
        u32 index = sArc->page * PER_PAGE + i, species, x, y;
        bool32 stored;
        if (index >= DEX_FORM_COUNT)
            break;
        species = gDexFormSpecies[index];
        stored = Archive_Has(species);
        x = GRID_X + CELL * (i % COLS);
        y = GRID_Y + CELL * (i / COLS);
        sArc->icons[i] = CreateMonIcon(species, SpriteCB_MonIcon, x, y - 4, 1, 0);
        if (sArc->icons[i] >= MAX_SPRITES)
        {
            sArc->icons[i] = MAX_SPRITES;
            continue;
        }
        gSprites[sArc->icons[i]].oam.priority = 1;
        if (!stored && sArc->loc[species] == 0)
        {
            gSprites[sArc->icons[i]].oam.paletteNum = dark;
            gSprites[sArc->icons[i]].callback = SpriteCallbackDummy;     // (still)
        }
        if (sArc->loc[species] != 0)
        {
            sArc->balls[i] = CreateSprite(&sSmallTemplate, x + 8, y + 8, 0);
            if (sArc->balls[i] < MAX_SPRITES)
                gSprites[sArc->balls[i]].oam.tileNum = tile + UI_TILE_BALL;
            else
                sArc->balls[i] = MAX_SPRITES;
        }
        if (stored && IsStoredShiny(species))
        {
            sArc->stars[i] = CreateSprite(&sSmallTemplate, x - 8, y - 8, 0);
            if (sArc->stars[i] < MAX_SPRITES)
                gSprites[sArc->stars[i]].oam.tileNum = tile + UI_TILE_STAR;
            else
                sArc->stars[i] = MAX_SPRITES;
        }
    }
    PrintTop();
}

static void GoTo(u32 index)
{
    if (index >= DEX_FORM_COUNT)
        index = DEX_FORM_COUNT - 1;
    if (index / PER_PAGE != sArc->page)
    {
        sArc->page = index / PER_PAGE;
        sArc->cell = index % PER_PAGE;
        DrawPage();
    }
    sArc->cell = index % PER_PAGE;
    ShowCurrent();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

// Takes one out: a new one, level 5 (random IVs, nature and one of its usual abilities; shiny if
// the one stored was). VAR_RESULT-like: MON_GIVEN_TO_PARTY/PC, or MON_CANT_GIVE
static u32 TakeOut(u32 species)
{
    struct Pokemon *mon = Alloc(sizeof(struct Pokemon));
    u32 result;
    u8 shiny = IsStoredShiny(species), abilityNum = 0;
    CreateRandomMon(mon, species, 5);
    SetMonData(mon, MON_DATA_IS_SHINY, &shiny);
    if (gSpeciesInfo[species].abilities[1] != ABILITY_NONE && (Random() & 1))
        abilityNum = 1;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &abilityNum);
    CalculateMonStats(mon);
    result = GiveScriptedMonToPlayer(mon, PARTY_SIZE);
    Free(mon);
    if (result != MON_CANT_GIVE)
        Take(species);
    return result;
}

static void Task_Archive(u8 taskId)
{
    u32 index = CurrentIndex(), species = gDexFormSpecies[index];
    switch (sArc->state)
    {
    case ST_INPUT:
        if (gPaletteFade.active)
            break;
        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_PC_OFF);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sArc->state = ST_EXIT;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            if (!Archive_Has(species))
            {
                PlaySE(SE_FAILURE);
                break;
            }
            PlaySE(SE_SELECT);
            {
                u8 *s = Alloc(64), *end = StringCopy(s, COMPOUND_STRING("Take out "));
                end = StringAppend(end, FormName(index));
                StringAppend(end, COMPOUND_STRING("? It'll be Lv. 5."));
                PrintFooter(s, FONT_NORMAL);
                Free(s);
            }
            CreateYesNoMenu(&sYesNoTemplate, FRAME_TILE, 14, 0);
            sArc->state = ST_CONFIRM;
        }
        else if (JOY_NEW(START_BUTTON))
        {
            u32 g = GenOf(index) + 1;
            PlaySE(SE_SELECT);
            GoTo(g < DEX_FORM_GEN_COUNT ? gDexFormGenStarts[g] : 0);
        }
        else if (JOY_REPEAT(L_BUTTON) || JOY_REPEAT(R_BUTTON))
        {
            u32 page = JOY_REPEAT(R_BUTTON) ? (sArc->page + 1) % PAGE_COUNT : (sArc->page + PAGE_COUNT - 1) % PAGE_COUNT;
            PlaySE(SE_SELECT);
            GoTo(page * PER_PAGE + min(sArc->cell, (u32)(DEX_FORM_COUNT - 1 - page * PER_PAGE)));
        }
        else if (JOY_REPEAT(DPAD_ANY))
        {
            s32 col = sArc->cell % COLS, row = sArc->cell / COLS, page = sArc->page;
            if (JOY_REPEAT(DPAD_LEFT) && --col < 0)
            {
                col = COLS - 1;     // (off the left: the page before, its last column)
                page = (page + PAGE_COUNT - 1) % PAGE_COUNT;
            }
            else if (JOY_REPEAT(DPAD_RIGHT) && ++col >= COLS)
            {
                col = 0;
                page = (page + 1) % PAGE_COUNT;
            }
            else if (JOY_REPEAT(DPAD_UP))
                row = (row + ROWS - 1) % ROWS;
            else if (JOY_REPEAT(DPAD_DOWN))
                row = (row + 1) % ROWS;
            PlaySE(SE_SELECT);
            index = page * PER_PAGE + row * COLS + col;
            while (index >= DEX_FORM_COUNT)      // (the last page isn't full)
                index -= COLS;
            GoTo(index);
        }
        break;
    case ST_CONFIRM:
        switch (Menu_ProcessInputNoWrapClearOnChoose())
        {
        case 0:
        {
            u32 result = TakeOut(species);
            u8 *s = Alloc(64), *end = StringCopy(s, FormName(index));
            if (result == MON_CANT_GIVE)
            {
                PlaySE(SE_FAILURE);
                StringCopy(s, COMPOUND_STRING("There's no room for it!"));
            }
            else
            {
                PlaySE(SE_EXP_MAX);
                StringAppend(end, result == MON_GIVEN_TO_PARTY ? COMPOUND_STRING(" joined your party!")
                                                               : COMPOUND_STRING(" was sent to the PC."));
            }
            PrintFooter(s, FONT_NORMAL);
            Free(s);
            FindOwned();
            PrintCounts();
            DrawPage();
            ShowCurrent();
            sArc->state = ST_MESSAGE;
            break;
        }
        case 1:
        case MENU_B_PRESSED:
            PlaySE(SE_SELECT);
            PrintHelp();
            sArc->state = ST_INPUT;
            break;
        }
        break;
    case ST_MESSAGE:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            PrintHelp();
            sArc->state = ST_INPUT;
        }
        break;
    case ST_EXIT:
        if (gPaletteFade.active)
            break;
        sLastIndex = CurrentIndex();
        DestroyPage();
        if (sArc->pic != 0xFF)
            FreeAndDestroyMonPicSprite(sArc->pic);
        FreeMonIconPalettes();
        FreeAllWindowBuffers();
        Free(sArc->tilemap);
        Free(sArc->bgTiles);
        Free(sArc->uiTiles);
        Free(sArc->loc);
        FREE_AND_SET_NULL(sArc);
        DestroyTask(taskId);
        ReturnFromHallOfFamePC();      // (back to the PC's menu)
        break;
    }
}

static void CB2_InitArchive(void)
{
    u32 i;
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    ResetAllBgsCoordinates();
    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(WIN_FOOTER, FRAME_TILE, BG_PLTT_ID(14));

    sArc = AllocZeroed(sizeof(*sArc));
    sArc->loc = AllocZeroed(NUM_SPECIES);
    sArc->pic = 0xFF;
    for (i = 0; i < PER_PAGE; i++)
        sArc->icons[i] = sArc->balls[i] = sArc->stars[i] = MAX_SPRITES;
    Archive_Validate();
    LoadBackground();
    LoadUiSprites();
    LoadMonIconPalettes();
    FindOwned();
    for (i = 0; i < WIN_COUNT_; i++)
        PutWindowTilemap(i);
    PrintCounts();
    PrintHelp();
    sArc->cursor = CreateSprite(&sCursorTemplate, GRID_X, GRID_Y, 0);
    if (sLastIndex >= DEX_FORM_COUNT)
        sLastIndex = 0;
    sArc->page = sLastIndex / PER_PAGE;
    sArc->cell = sLastIndex % PER_PAGE;
    DrawPage();
    ShowCurrent();

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(1);
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB);
    SetMainCallback2(MainCB2);
    CreateTask(Task_Archive, 0);
}

// Special (from the PC's menu or the Palm Pilot's; the script waitstates)
void AccessArchivePC(void)
{
    SetMainCallback2(CB2_InitArchive);
    LockPlayerFieldControls();
}
