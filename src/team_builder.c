// Pokémon Canvas: the Leader Academy's TEAM BUILDER (Tansy, in the main hall; special
// Academy_OpenTeamBuilder).
//
// Up to 8 teams (kept in flash sector 30 with the Mastery Hall and the archive: struct
// TeamBuilderData, saved with the game). A team is six species and their moves; nothing is given to
// the player, it's a planner.
//   TEAMS     the 8 teams; A edits one
//   EDIT      its 6 slots; A: choose a Pokémon (a type, then a grid of the Pokémon with it as
//             their primary type - a ball
//             by those the player has, in the party, a box or the archive), then its moves;
//             SELECT: just the moves; START: the type chart
//   ANALYSIS  for each of the 18 types: how many of the team are weak to it, how many resist it
//             (or are immune), and the best hit the team's attacks have on it
#include "global.h"
#include "team_builder.h"
#include "archive.h"
#include "battle_main.h"
#include "battle_util.h"
#include "bg.h"
#include "dex_forms.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "mastery_hall.h"
#include "menu.h"
#include "menu_helpers.h"
#include "move.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_storage_system.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/moves.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define TEAMS_MAGIC 0x54424C31     // "TBL1"

STATIC_ASSERT(MOVES_COUNT <= 1024, TeamBuilderMovesFitTenBits);

static u32 GetTeamMove(const struct BuiltTeam *team, u32 slot, u32 i)
{
    return team->moveLo[slot][i] | (((team->moveHi[slot] >> (i * 2)) & 3) << 8);
}

static void SetTeamMove(struct BuiltTeam *team, u32 slot, u32 i, u32 move)
{
    team->moveLo[slot][i] = move & 0xFF;
    team->moveHi[slot] = (team->moveHi[slot] & ~(3 << (i * 2))) | (((move >> 8) & 3) << (i * 2));
}

void TeamBuilder_Clear(void)
{
    memset(&gExtraSave.teams, 0, sizeof(gExtraSave.teams));
    gExtraSave.teams.magic = TEAMS_MAGIC;
}

void TeamBuilder_Validate(void)
{
    u32 t, i, j;
    if (gExtraSave.teams.magic != TEAMS_MAGIC)
    {
        TeamBuilder_Clear();
        return;
    }
    for (t = 0; t < BUILT_TEAMS; t++)
    {
        struct BuiltTeam *team = &gExtraSave.teams.teams[t];
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (team->species[i] >= NUM_SPECIES)
                team->species[i] = SPECIES_NONE;
            for (j = 0; j < MAX_MON_MOVES; j++)
                if (GetTeamMove(team, i, j) >= MOVES_COUNT)
                    SetTeamMove(team, i, j, MOVE_NONE);
        }
    }
}

// ---- The 18 types, in a sensible order ----
static const u8 sTypes[] =
{
    TYPE_NORMAL, TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC, TYPE_ICE, TYPE_FIGHTING, TYPE_POISON,
    TYPE_GROUND, TYPE_FLYING, TYPE_PSYCHIC, TYPE_BUG, TYPE_ROCK, TYPE_GHOST, TYPE_DRAGON, TYPE_DARK,
    TYPE_STEEL, TYPE_FAIRY,
};
#define TYPE_COUNT ARRAY_COUNT(sTypes)

// ---- The screen ----
enum { WIN_TOP, WIN_LEFT, WIN_RIGHT, WIN_FOOTER, WIN_FULL, WIN_COUNT_ };
enum { MODE_TEAMS, MODE_EDIT, MODE_TYPES, MODE_GRID, MODE_MOVES, MODE_ANALYSIS, MODE_EXIT };
enum { BG_PANEL = 1, BG_EDGE, BG_WALL_A, BG_WALL_B, BG_BAR, BG_FOOT };
enum { TXT_CLEAR, TXT_WHITE, TXT_DARK, TXT_GRAY, TXT_GOLD, TXT_RED, TXT_GREEN, TXT_BLUE };

#define COLS        6
#define ROWS        5
#define PER_PAGE    (COLS * ROWS)
#define CELL        24
#define GRID_X      108
#define GRID_Y      34
#define SLOT_ROW    21
#define SLOT_Y      26      // (the first slot's icon, middle)
#define SLOT_X      14
#define LOC_ARCHIVE 0xFE
#define LOC_PARTY   0xFF
#define TAG_UI      0x5B01
#define UI_TILES    18
#define UI_TILE_BALL 16
#define NO_SPRITE   MAX_SPRITES

struct TeamBuilder
{
    u8 mode;
    u8 team;                    // (MODE_TEAMS: the cursor)
    u8 slot;
    u8 typeCursor;              // (0..TYPE_COUNT: the last is "EMPTY")
    u16 *list;                  // (MODE_GRID: that type's Pokémon, by index in gDexFormSpecies)
    u16 listCount;
    u16 page, cell;
    u8 icons[PER_PAGE];
    u8 balls[PER_PAGE];
    u8 slotIcons[PARTY_SIZE];
    u8 cursor;
    u8 *loc;                    // (by species: where the player has one)
    // choosing moves
    u8 listTask;
    u8 moveCount;               // (chosen so far)
    u16 newMoves[MAX_MON_MOVES];
    u16 newSpecies;
    struct ListMenuItem *moveItems;
    u16 *moveIds;
    u16 moveTotal;
    u8 *tilemap, *bgTiles, *uiTiles;
};

static EWRAM_DATA struct TeamBuilder *sTB = NULL;

static const struct BgTemplate sBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 1, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
    {.bg = 1, .charBaseIndex = 0, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0},
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_TOP]    = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 0,  .width = 30, .height = 2,  .paletteNum = 15, .baseBlock = 1},
    [WIN_LEFT]   = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 2,  .width = 12, .height = 16, .paletteNum = 15, .baseBlock = 61},
    [WIN_RIGHT]  = {.bg = 0, .tilemapLeft = 12, .tilemapTop = 2,  .width = 18, .height = 16, .paletteNum = 15, .baseBlock = 253},
    [WIN_FOOTER] = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 18, .width = 30, .height = 2,  .paletteNum = 15, .baseBlock = 541},
    [WIN_FULL]   = {.bg = 0, .tilemapLeft = 0,  .tilemapTop = 2,  .width = 30, .height = 16, .paletteNum = 15, .baseBlock = 61},  // (the left and right windows' tiles: never shown together)
    DUMMY_WIN_TEMPLATE,
};

static const u8 sWhite[3] = {TXT_CLEAR, TXT_WHITE, TXT_DARK};
static const u8 sGray[3] = {TXT_CLEAR, TXT_GRAY, TXT_DARK};
static const u8 sGold[3] = {TXT_CLEAR, TXT_GOLD, TXT_DARK};
static const u8 sRed[3] = {TXT_CLEAR, TXT_RED, TXT_DARK};
static const u8 sGreen[3] = {TXT_CLEAR, TXT_GREEN, TXT_DARK};
static const u8 sBlue[3] = {TXT_CLEAR, TXT_BLUE, TXT_DARK};

static const u16 sUiPalette[16] = {0, RGB(31, 31, 31), RGB(4, 4, 7), RGB(30, 8, 8), RGB(31, 26, 4)};

static const struct OamData sOam32 = {.shape = SPRITE_SHAPE(32x32), .size = SPRITE_SIZE(32x32), .priority = 0};
static const struct OamData sOam8 = {.shape = SPRITE_SHAPE(8x8), .size = SPRITE_SIZE(8x8), .priority = 0};
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

static struct BuiltTeam *Team(void)
{
    return &gExtraSave.teams.teams[sTB->team];
}

static u32 TeamSize(const struct BuiltTeam *team)
{
    u32 i, n = 0;
    for (i = 0; i < PARTY_SIZE; i++)
        n += team->species[i] != SPECIES_NONE;
    return n;
}

static void SetPixel(u8 *tiles, u32 tilesWide, u32 x, u32 y, u32 color)
{
    u8 *p = &tiles[((y / 8) * tilesWide + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2];
    *p = (x & 1) ? (*p & 0x0F) | (color << 4) : (*p & 0xF0) | color;
}

static void LoadUiSprites(void)
{
    static const char *const ball[8] = {"  2222  ", " 233332 ", "23333332", "22222222", "21111112", "21111112", " 211112 ", "  2222  "};
    struct SpriteSheet sheet;
    struct SpritePalette pal = {sUiPalette, TAG_UI};
    u8 *t = sTB->uiTiles = AllocZeroed(UI_TILES * 32);
    u32 x, y;
    for (y = 2; y < 30; y++)
    {
        for (x = 2; x < 30; x++)
        {
            bool32 edge = x < 4 || x >= 28 || y < 4 || y >= 28;
            bool32 outline = x == 2 || x == 29 || y == 2 || y == 29;
            bool32 corner = (x < 9 || x > 22) && (y < 9 || y > 22);
            if (edge && corner)
                SetPixel(t, 4, x, y, outline ? 2 : 4);
        }
    }
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++)
            if (ball[y][x] != ' ')
                SetPixel(t + UI_TILE_BALL * 32, 1, x, y, ball[y][x] - '0');
    sheet.data = t;
    sheet.size = UI_TILES * 32;
    sheet.tag = TAG_UI;
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&pal);
}

static void LoadBackground(void)
{
    u8 *t = sTB->bgTiles = AllocZeroed(6 * 32);
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
    pal[BG_PANEL] = RGB(14, 6, 8);
    pal[BG_EDGE] = RGB(6, 2, 3);
    pal[BG_WALL_A] = RGB(27, 22, 22);
    pal[BG_WALL_B] = RGB(29, 24, 24);
    pal[BG_BAR] = RGB(18, 8, 11);
    pal[BG_FOOT] = RGB(8, 3, 5);
    LoadPalette(pal, BG_PLTT_ID(0), sizeof(pal));
    pal[0] = 0;
    pal[TXT_WHITE] = RGB_WHITE;
    pal[TXT_DARK] = RGB(3, 3, 6);
    pal[TXT_GRAY] = RGB(24, 20, 21);
    pal[TXT_GOLD] = RGB(31, 26, 6);
    pal[TXT_RED] = RGB(31, 10, 10);
    pal[TXT_GREEN] = RGB(12, 28, 12);
    pal[TXT_BLUE] = RGB(14, 22, 31);
    LoadPalette(pal, BG_PLTT_ID(15), sizeof(pal));
    sTB->tilemap = AllocZeroed(BG_SCREEN_SIZE);
    SetBgTilemapBuffer(1, sTB->tilemap);
}

// The background's layout: the bars, and a panel on the left (or none, for the analysis)
static void DrawBackground(bool32 panel)
{
    FillBgTilemapBufferRect_Palette0(1, 2, 0, 0, 32, 1);
    FillBgTilemapBufferRect_Palette0(1, 5, 0, 1, 32, 1);
    if (panel)
    {
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 2, 11, 16);
        FillBgTilemapBufferRect_Palette0(1, 4, 11, 2, 1, 16);
        FillBgTilemapBufferRect_Palette0(1, 1, 12, 2, 20, 16);
    }
    else
    {
        FillBgTilemapBufferRect_Palette0(1, 0, 0, 2, 32, 16);
    }
    FillBgTilemapBufferRect_Palette0(1, 3, 0, 18, 32, 2);
    CopyBgTilemapBufferToVram(1);
}

static void FindOwned(void)
{
    u32 box, pos, i;
    memset(sTB->loc, 0, NUM_SPECIES);
    for (i = 0; i < NUM_SPECIES; i++)
        if (Archive_Has(i))
            sTB->loc[i] = LOC_ARCHIVE;
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            u32 species = GetBoxMonDataAt(box, pos, MON_DATA_SPECIES);
            if (species != SPECIES_NONE && species < NUM_SPECIES && !GetBoxMonDataAt(box, pos, MON_DATA_IS_EGG))
                sTB->loc[species] = box + 1;
        }
    }
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 species = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES);
        if (species != SPECIES_NONE && species < NUM_SPECIES && !GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            sTB->loc[species] = LOC_PARTY;
    }
}

static void Print(u32 win, u32 font, u32 x, u32 y, const u8 *color, const u8 *s)
{
    AddTextPrinterParameterized4(win, font, x, y, 0, 0, color, TEXT_SKIP_DRAW, s);
}

// The analysis uses one big window; everything else the left and right ones (same place on screen)
static void SetLayout(bool32 full)
{
    FillBgTilemapBufferRect(0, 0, 0, 2, 30, 16, 15);
    if (full)
        PutWindowTilemap(WIN_FULL);
    else
    {
        PutWindowTilemap(WIN_LEFT);
        PutWindowTilemap(WIN_RIGHT);
    }
    ScheduleBgCopyTilemapToVram(0);
}

static void PrintTop(const u8 *title)
{
    FillWindowPixelBuffer(WIN_TOP, PIXEL_FILL(0));
    Print(WIN_TOP, FONT_NORMAL, 4, 1, sWhite, title);
    CopyWindowToVram(WIN_TOP, COPYWIN_FULL);
}

static void PrintFooter(const u8 *text)
{
    FillWindowPixelBuffer(WIN_FOOTER, PIXEL_FILL(0));
    Print(WIN_FOOTER, FONT_SMALL, 4, 3, sWhite, text);
    CopyWindowToVram(WIN_FOOTER, COPYWIN_FULL);
}

static void ClearWindow(u32 win)
{
    FillWindowPixelBuffer(win, PIXEL_FILL(0));
    CopyWindowToVram(win, COPYWIN_FULL);
}

static u8 *AppendTeamName(u8 *end, u32 team)
{
    end = StringCopy(end, COMPOUND_STRING("TEAM "));
    return ConvertIntToDecimalStringN(end, team + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
}

// ---- Sprites ----
static void DestroyIcon(u8 *id)
{
    if (*id != NO_SPRITE)
        FreeAndDestroyMonIconSprite(&gSprites[*id]);
    *id = NO_SPRITE;
}

static void DestroyBall(u8 *id)
{
    if (*id != NO_SPRITE)
        DestroySprite(&gSprites[*id]);
    *id = NO_SPRITE;
}

static void DestroyAllIcons(void)
{
    u32 i;
    for (i = 0; i < PER_PAGE; i++)
    {
        DestroyIcon(&sTB->icons[i]);
        DestroyBall(&sTB->balls[i]);
    }
    for (i = 0; i < PARTY_SIZE; i++)
        DestroyIcon(&sTB->slotIcons[i]);
    gSprites[sTB->cursor].invisible = TRUE;
}

static u8 MakeIcon(u32 species, s32 x, s32 y)
{
    u8 id = CreateMonIcon(species, SpriteCB_MonIcon, x, y, 1, 0);
    if (id >= MAX_SPRITES)
        return NO_SPRITE;
    gSprites[id].oam.priority = 1;
    return id;
}

static u8 MakeBall(s32 x, s32 y)
{
    u8 id = CreateSprite(&sSmallTemplate, x, y, 0);
    if (id >= MAX_SPRITES)
        return NO_SPRITE;
    gSprites[id].oam.tileNum = GetSpriteTileStartByTag(TAG_UI) + UI_TILE_BALL;
    return id;
}

// The six icons of a team, in a column on the left (EDIT) or a 3x2 block on the right (TEAMS)
static void DrawTeamIcons(const struct BuiltTeam *team, bool32 column)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        DestroyIcon(&sTB->slotIcons[i]);
        if (team->species[i] == SPECIES_NONE)
            continue;
        if (column)
            sTB->slotIcons[i] = MakeIcon(team->species[i], SLOT_X, SLOT_Y + SLOT_ROW * i);
        else
            sTB->slotIcons[i] = MakeIcon(team->species[i], 124 + 44 * (i % 3), 44 + 40 * (i / 3));
    }
}

// ---- Types ----
static uq4_12_t Effectiveness(u32 atkType, u32 species)
{
    u32 t1 = GetSpeciesType(species, 0), t2 = GetSpeciesType(species, 1);
    uq4_12_t mod = GetTypeModifier(atkType, t1);
    if (t2 != t1)
        mod = uq4_12_multiply(mod, GetTypeModifier(atkType, t2));
    return mod;
}

static void BufferTypes(u8 *dst, u32 species)
{
    u32 t1 = GetSpeciesType(species, 0), t2 = GetSpeciesType(species, 1);
    u8 *end = StringCopy(dst, gTypesInfo[t1].name);
    if (t2 != t1)
    {
        end = StringCopy(end, COMPOUND_STRING(" / "));
        StringCopy(end, gTypesInfo[t2].name);
    }
}

// For each type: how many are weak, how many resist (or are immune), the team's best hit on it
struct TypeLine { u8 weak, resist, immune; uq4_12_t best; };

static void Analyse(const struct BuiltTeam *team, struct TypeLine *lines)
{
    u32 t, i, j;
    for (t = 0; t < TYPE_COUNT; t++)
    {
        struct TypeLine *l = &lines[t];
        memset(l, 0, sizeof(*l));
        for (i = 0; i < PARTY_SIZE; i++)
        {
            uq4_12_t mod;
            if (team->species[i] == SPECIES_NONE)
                continue;
            mod = Effectiveness(sTypes[t], team->species[i]);
            if (mod == UQ_4_12(0))
                l->immune++;
            else if (mod < UQ_4_12(1.0))
                l->resist++;
            else if (mod > UQ_4_12(1.0))
                l->weak++;
            for (j = 0; j < MAX_MON_MOVES; j++)
            {
                u32 move = GetTeamMove(team, i, j);
                uq4_12_t hit;
                if (move == MOVE_NONE || GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS || GetMovePower(move) == 0)
                    continue;
                hit = GetTypeModifier(GetMoveType(move), sTypes[t]);
                if (hit > l->best)
                    l->best = hit;
            }
        }
    }
}

static bool32 TeamHasAttacks(const struct BuiltTeam *team)
{
    u32 i, j;
    for (i = 0; i < PARTY_SIZE; i++)
        for (j = 0; j < MAX_MON_MOVES; j++)
            if (team->species[i] != SPECIES_NONE && GetTeamMove(team, i, j) != MOVE_NONE
             && GetMoveCategory(GetTeamMove(team, i, j)) != DAMAGE_CATEGORY_STATUS)
                return TRUE;
    return FALSE;
}

// ---- TEAMS ----
static void ShowTeams(void)
{
    u32 i;
    u8 *s = Alloc(48), *end;
    sTB->mode = MODE_TEAMS;
    DestroyAllIcons();
    DrawBackground(TRUE);
    SetLayout(FALSE);
    PrintTop(COMPOUND_STRING("TEAM BUILDER"));
    FillWindowPixelBuffer(WIN_LEFT, PIXEL_FILL(0));
    for (i = 0; i < BUILT_TEAMS; i++)
    {
        const struct BuiltTeam *team = &gExtraSave.teams.teams[i];
        u32 n = TeamSize(team);
        end = AppendTeamName(s, i);
        if (n == 0)
            StringAppend(end, COMPOUND_STRING("  -"));
        else
        {
            end = StringAppend(end, COMPOUND_STRING("  "));
            end = ConvertIntToDecimalStringN(end, n, STR_CONV_MODE_LEFT_ALIGN, 1);
            StringAppend(end, COMPOUND_STRING("/6"));
        }
        Print(WIN_LEFT, FONT_NORMAL, 10, 2 + 15 * i, i == sTB->team ? sGold : (n ? sWhite : sGray), s);
    }
    Print(WIN_LEFT, FONT_NORMAL, 1, 2 + 15 * sTB->team, sGold, COMPOUND_STRING("▶"));
    CopyWindowToVram(WIN_LEFT, COPYWIN_FULL);

    FillWindowPixelBuffer(WIN_RIGHT, PIXEL_FILL(0));
    Print(WIN_RIGHT, FONT_SMALL, 4, 100, sGray, COMPOUND_STRING("You can keep up to 8 teams."));
    Print(WIN_RIGHT, FONT_SMALL, 4, 112, sGray, COMPOUND_STRING("They're kept when you save."));
    if (TeamSize(Team()) == 0)
        Print(WIN_RIGHT, FONT_NORMAL, 30, 30, sGray, COMPOUND_STRING("(empty)"));
    CopyWindowToVram(WIN_RIGHT, COPYWIN_FULL);
    DrawTeamIcons(Team(), FALSE);
    PrintFooter(COMPOUND_STRING("{A_BUTTON} EDIT   SELECT CLEAR TEAM   {B_BUTTON} EXIT"));
    Free(s);
}

// ---- EDIT ----
static void PrintSlotDetails(void)
{
    const struct BuiltTeam *team = Team();
    u32 species = team->species[sTB->slot], i, n;
    struct TypeLine lines[TYPE_COUNT];
    u8 *s = Alloc(96), *end;

    FillWindowPixelBuffer(WIN_RIGHT, PIXEL_FILL(0));
    if (species == SPECIES_NONE)
    {
        Print(WIN_RIGHT, FONT_NORMAL, 4, 2, sGray, COMPOUND_STRING("(empty slot)"));
    }
    else
    {
        Print(WIN_RIGHT, FONT_NORMAL, 4, 0, sGold, GetSpeciesName(species));
        BufferTypes(s, species);
        Print(WIN_RIGHT, FONT_SMALL, 4, 14, sGray, s);
        for (i = 0; i < MAX_MON_MOVES; i++)
        {
            u32 move = GetTeamMove(team, sTB->slot, i);
            Print(WIN_RIGHT, FONT_SMALL, 8, 26 + 10 * i, move ? sWhite : sGray, move ? GetMoveName(move) : COMPOUND_STRING("-"));
        }
    }
    // The team at a glance: its worst weaknesses, and the types it can't hit hard
    Analyse(team, lines);
    end = StringCopy(s, COMPOUND_STRING("WEAK: "));
    n = 0;
    for (i = 0; i < TYPE_COUNT && n < 3; i++)
    {
        if (lines[i].weak >= 2 && lines[i].weak > lines[i].resist + lines[i].immune)
        {
            if (n++)
                end = StringCopy(end, COMPOUND_STRING(", "));
            end = StringCopy(end, gTypesInfo[sTypes[i]].name);
            end = StringCopy(end, COMPOUND_STRING(" x"));
            end = ConvertIntToDecimalStringN(end, lines[i].weak, STR_CONV_MODE_LEFT_ALIGN, 1);
        }
    }
    if (n == 0)
        StringCopy(end, COMPOUND_STRING("nothing major"));
    Print(WIN_RIGHT, FONT_SMALL, 4, 74, n ? sRed : sGreen, s);
    end = StringCopy(s, COMPOUND_STRING("NO 2x HIT: "));
    n = 0;
    if (TeamHasAttacks(team))
    {
        for (i = 0; i < TYPE_COUNT && n < 3; i++)
        {
            if (lines[i].best <= UQ_4_12(1.0))
            {
                if (n++)
                    end = StringCopy(end, COMPOUND_STRING(", "));
                end = StringCopy(end, gTypesInfo[sTypes[i]].name);
            }
        }
        if (n == 0)
            StringCopy(end, COMPOUND_STRING("none!"));
    }
    else
    {
        StringCopy(end, COMPOUND_STRING("(no attacks yet)"));
        n = 1;
    }
    Print(WIN_RIGHT, FONT_SMALL, 4, 86, n ? sRed : sGreen, s);
    Print(WIN_RIGHT, FONT_SMALL, 4, 112, sGray, COMPOUND_STRING("START: the full type chart"));
    CopyWindowToVram(WIN_RIGHT, COPYWIN_FULL);
    Free(s);
}

static void PrintSlots(void)
{
    const struct BuiltTeam *team = Team();
    u32 i;
    FillWindowPixelBuffer(WIN_LEFT, PIXEL_FILL(0));
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 species = team->species[i];
        Print(WIN_LEFT, FONT_SMALL, 30, 3 + SLOT_ROW * i, i == sTB->slot ? sGold : (species ? sWhite : sGray),
              species ? GetSpeciesName(species) : COMPOUND_STRING("-"));
    }
    CopyWindowToVram(WIN_LEFT, COPYWIN_FULL);
    gSprites[sTB->cursor].invisible = FALSE;
    gSprites[sTB->cursor].x = SLOT_X;
    gSprites[sTB->cursor].y = SLOT_Y + 2 + SLOT_ROW * sTB->slot;
}

static void ShowEdit(void)
{
    u8 *s = Alloc(32);
    sTB->mode = MODE_EDIT;
    DestroyAllIcons();
    DrawBackground(TRUE);
    SetLayout(FALSE);
    AppendTeamName(s, sTB->team);
    PrintTop(s);
    Free(s);
    DrawTeamIcons(Team(), TRUE);
    PrintSlots();
    PrintSlotDetails();
    PrintFooter(COMPOUND_STRING("{A_BUTTON} POKéMON   SELECT MOVES   START TYPE CHART   {B_BUTTON} BACK"));
}

// ---- ANALYSIS: the full type chart ----
static void ShowAnalysis(void)
{
    struct TypeLine lines[TYPE_COUNT];
    u32 i, col;
    u8 *s = Alloc(16);
    sTB->mode = MODE_ANALYSIS;
    DestroyAllIcons();
    DrawBackground(FALSE);
    SetLayout(TRUE);
    Analyse(Team(), lines);
    PrintTop(COMPOUND_STRING("TYPE CHART"));
    FillWindowPixelBuffer(WIN_FULL, PIXEL_FILL(0));
    for (col = 0; col < 2; col++)
    {
        u32 x = 4 + 120 * col;
        Print(WIN_FULL, FONT_SMALL, x, 0, sGray, COMPOUND_STRING("TYPE"));
        Print(WIN_FULL, FONT_SMALL, x + 54, 0, sGray, COMPOUND_STRING("WEAK"));
        Print(WIN_FULL, FONT_SMALL, x + 78, 0, sGray, COMPOUND_STRING("RES"));
        Print(WIN_FULL, FONT_SMALL, x + 98, 0, sGray, COMPOUND_STRING("HIT"));
    }
    for (i = 0; i < TYPE_COUNT; i++)
    {
        const struct TypeLine *l = &lines[i];
        u32 x = 4 + 120 * (i / 9), y = 12 + 12 * (i % 9);
        const u8 *hit;
        const u8 *hitColor;
        Print(WIN_FULL, FONT_SMALL, x, y, sWhite, gTypesInfo[sTypes[i]].name);
        ConvertIntToDecimalStringN(s, l->weak, STR_CONV_MODE_LEFT_ALIGN, 1);
        Print(WIN_FULL, FONT_SMALL, x + 60, y, l->weak >= 3 ? sRed : (l->weak ? sWhite : sGray), s);
        ConvertIntToDecimalStringN(s, l->resist + l->immune, STR_CONV_MODE_LEFT_ALIGN, 1);
        Print(WIN_FULL, FONT_SMALL, x + 82, y, l->resist + l->immune ? sGreen : sGray, s);
        if (l->best >= UQ_4_12(2.0))
            hit = COMPOUND_STRING("2x"), hitColor = sGreen;
        else if (l->best >= UQ_4_12(1.0))
            hit = COMPOUND_STRING("1x"), hitColor = sWhite;
        else if (l->best > 0)
            hit = COMPOUND_STRING("1/2"), hitColor = sRed;
        else
            hit = COMPOUND_STRING("-"), hitColor = sRed;
        Print(WIN_FULL, FONT_SMALL, x + 98, y, hitColor, hit);
    }
    CopyWindowToVram(WIN_FULL, COPYWIN_FULL);
    PrintFooter(COMPOUND_STRING("WEAK 2x  RES resist/immune  HIT best attack  {B_BUTTON} BACK"));
    Free(s);
}

// ---- TYPES: choosing which type to browse ----
static void PrintTypes(void)
{
    u32 i;
    FillWindowPixelBuffer(WIN_RIGHT, PIXEL_FILL(0));
    for (i = 0; i <= TYPE_COUNT; i++)
    {
        u32 x = 6 + 46 * (i % 3), y = 4 + 16 * (i / 3);
        const u8 *name = i < TYPE_COUNT ? gTypesInfo[sTypes[i]].name : COMPOUND_STRING("EMPTY");
        Print(WIN_RIGHT, FONT_SMALL, x, y, i == sTB->typeCursor ? sGold : (i < TYPE_COUNT ? sWhite : sGray), name);
    }
    CopyWindowToVram(WIN_RIGHT, COPYWIN_FULL);
}

static void ShowTypes(void)
{
    sTB->mode = MODE_TYPES;
    gSprites[sTB->cursor].invisible = TRUE;
    FillWindowPixelBuffer(WIN_LEFT, PIXEL_FILL(0));
    Print(WIN_LEFT, FONT_NORMAL, 4, 2, sWhite, COMPOUND_STRING("Which type?"));
    Print(WIN_LEFT, FONT_SMALL, 4, 20, sGray, COMPOUND_STRING("EMPTY clears"));
    Print(WIN_LEFT, FONT_SMALL, 4, 30, sGray, COMPOUND_STRING("the slot."));
    CopyWindowToVram(WIN_LEFT, COPYWIN_FULL);
    {
        u32 i;
        for (i = 0; i < PARTY_SIZE; i++)
            DestroyIcon(&sTB->slotIcons[i]);
    }
    PrintTypes();
    PrintFooter(COMPOUND_STRING("{A_BUTTON} CHOOSE   {B_BUTTON} BACK"));
}

// ---- GRID: that type's Pokémon ----
static void BuildList(u32 type)
{
    u32 i;
    sTB->listCount = 0;
    for (i = 0; i < DEX_FORM_COUNT; i++)
    {
        u32 species = gDexFormSpecies[i];
        if (GetSpeciesType(species, 0) == type)      // (primary type only: the gym's rule)
            sTB->list[sTB->listCount++] = i;
    }
}

static u32 PageCount(void)
{
    return max((sTB->listCount + PER_PAGE - 1) / PER_PAGE, 1);
}

static u32 GridIndex(void)
{
    return sTB->page * PER_PAGE + sTB->cell;
}

static void PrintGridCurrent(void)
{
    u32 index = GridIndex(), species, loc;
    u8 *s = Alloc(48), *end;
    FillWindowPixelBuffer(WIN_LEFT, PIXEL_FILL(0));
    if (index < sTB->listCount)
    {
        u32 form = sTB->list[index];
        species = gDexFormSpecies[form];
        loc = sTB->loc[species];
        Print(WIN_LEFT, FONT_SMALL, 4, 2, sGold, gDexFormLabels[form] ? gDexFormLabels[form] : GetSpeciesName(species));
        BufferTypes(s, species);
        Print(WIN_LEFT, FONT_SMALL, 4, 14, sGray, s);
        if (loc == LOC_PARTY)
            StringCopy(s, COMPOUND_STRING("IN YOUR PARTY"));
        else if (loc == LOC_ARCHIVE)
            StringCopy(s, COMPOUND_STRING("IN THE ARCHIVE"));
        else if (loc != 0)
        {
            end = StringCopy(s, COMPOUND_STRING("IN BOX "));
            ConvertIntToDecimalStringN(end, loc, STR_CONV_MODE_LEFT_ALIGN, 2);
        }
        else
            StringCopy(s, COMPOUND_STRING("NOT OWNED"));
        Print(WIN_LEFT, FONT_SMALL, 4, 26, loc ? sGreen : sGray, s);
    }
    end = StringCopy(s, gTypesInfo[sTypes[sTB->typeCursor]].name);
    end = StringCopy(end, COMPOUND_STRING("  "));
    end = ConvertIntToDecimalStringN(end, sTB->page + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
    end = StringCopy(end, COMPOUND_STRING("/"));
    ConvertIntToDecimalStringN(end, PageCount(), STR_CONV_MODE_LEFT_ALIGN, 2);
    Print(WIN_LEFT, FONT_SMALL, 4, 116, sGray, s);
    CopyWindowToVram(WIN_LEFT, COPYWIN_FULL);
    gSprites[sTB->cursor].invisible = FALSE;
    gSprites[sTB->cursor].x = GRID_X + CELL * (sTB->cell % COLS);
    gSprites[sTB->cursor].y = GRID_Y + CELL * (sTB->cell / COLS);
    Free(s);
}

static void DrawGridPage(void)
{
    u32 i;
    for (i = 0; i < PER_PAGE; i++)
    {
        u32 index = sTB->page * PER_PAGE + i, species, x, y;
        DestroyIcon(&sTB->icons[i]);
        DestroyBall(&sTB->balls[i]);
        if (index >= sTB->listCount)
            continue;
        species = gDexFormSpecies[sTB->list[index]];
        x = GRID_X + CELL * (i % COLS);
        y = GRID_Y + CELL * (i / COLS);
        sTB->icons[i] = MakeIcon(species, x, y - 4);
        if (sTB->loc[species] != 0)
            sTB->balls[i] = MakeBall(x + 8, y + 8);
    }
}

static void ShowGrid(void)
{
    sTB->mode = MODE_GRID;
    ClearWindow(WIN_RIGHT);
    BuildList(sTypes[sTB->typeCursor]);
    sTB->page = sTB->cell = 0;
    DrawGridPage();
    PrintGridCurrent();
    PrintFooter(COMPOUND_STRING("{A_BUTTON} CHOOSE   {L_BUTTON}{R_BUTTON} PAGE   {B_BUTTON} BACK   BALL: YOU HAVE ONE"));
}

static void GridGoTo(u32 index)
{
    if (index >= sTB->listCount)
        index = sTB->listCount ? sTB->listCount - 1 : 0;
    if (index / PER_PAGE != sTB->page)
    {
        sTB->page = index / PER_PAGE;
        sTB->cell = index % PER_PAGE;
        DrawGridPage();
    }
    sTB->cell = index % PER_PAGE;
    PrintGridCurrent();
}

// ---- MOVES ----
static bool32 InMoves(const u16 *moves, u32 n, u32 move)
{
    u32 i;
    for (i = 0; i < n; i++)
        if (moves[i] == move)
            return TRUE;
    return FALSE;
}

static void AddMove(u32 move)
{
    if (move == MOVE_NONE || move >= MOVES_COUNT || sTB->moveTotal >= 400 || InMoves(sTB->moveIds, sTB->moveTotal, move))
        return;
    sTB->moveIds[sTB->moveTotal++] = move;
}

// Everything it can learn: by level, by TM/tutor, as an egg move (its earlier forms' too, as far
// as the learnsets go)
static void BuildMoveList(u32 species)
{
    const struct LevelUpMove *lvl = GetSpeciesLevelUpLearnset(species);
    const u16 *teach = GetSpeciesTeachableLearnset(species);
    const u16 *egg = GetSpeciesEggMoves(species);
    u32 i;
    sTB->moveTotal = 0;
    for (i = 0; lvl != NULL && lvl[i].move != LEVEL_UP_MOVE_END && i < 200; i++)
        AddMove(lvl[i].move);
    for (i = 0; teach != NULL && teach[i] != MOVE_UNAVAILABLE && i < 300; i++)
        AddMove(teach[i]);
    for (i = 0; egg != NULL && egg[i] != MOVE_UNAVAILABLE && i < 100; i++)
        AddMove(egg[i]);
}

static void PrintMovesChosen(void)
{
    u32 i;
    u8 *s = Alloc(32), *end;
    FillWindowPixelBuffer(WIN_LEFT, PIXEL_FILL(0));
    Print(WIN_LEFT, FONT_SMALL, 4, 2, sGold, GetSpeciesName(sTB->newSpecies));
    end = StringCopy(s, COMPOUND_STRING("MOVE "));
    end = ConvertIntToDecimalStringN(end, min(sTB->moveCount + 1, MAX_MON_MOVES), STR_CONV_MODE_LEFT_ALIGN, 1);
    StringCopy(end, COMPOUND_STRING(" OF 4"));
    Print(WIN_LEFT, FONT_SMALL, 4, 14, sWhite, s);
    for (i = 0; i < MAX_MON_MOVES; i++)
        Print(WIN_LEFT, FONT_SMALL, 6, 30 + 11 * i, i < sTB->moveCount ? sWhite : sGray,
              i < sTB->moveCount ? GetMoveName(sTB->newMoves[i]) : COMPOUND_STRING("-"));
    CopyWindowToVram(WIN_LEFT, COPYWIN_FULL);
    Free(s);
}

static void StartMoveList(void)
{
    struct ListMenuTemplate t = {0};
    u32 i, n = 0;
    if (sTB->moveItems == NULL)
        sTB->moveItems = AllocZeroed(sizeof(struct ListMenuItem) * 401);
    sTB->moveItems[n].name = COMPOUND_STRING("DONE");
    sTB->moveItems[n++].id = 0;
    for (i = 0; i < sTB->moveTotal; i++)
    {
        if (InMoves(sTB->newMoves, sTB->moveCount, sTB->moveIds[i]))
            continue;
        sTB->moveItems[n].name = GetMoveName(sTB->moveIds[i]);
        sTB->moveItems[n++].id = sTB->moveIds[i];
    }
    t.items = sTB->moveItems;
    t.totalItems = n;
    t.maxShowed = 8;
    t.windowId = WIN_RIGHT;
    t.item_X = 12;
    t.cursor_X = 2;
    t.upText_Y = 1;
    t.cursorPal = TXT_GOLD;
    t.fillValue = 0;
    t.cursorShadowPal = TXT_DARK;
    t.fontId = FONT_NORMAL;
    FillWindowPixelBuffer(WIN_RIGHT, PIXEL_FILL(0));
    sTB->listTask = ListMenuInit(&t, 0, 0);
    CopyWindowToVram(WIN_RIGHT, COPYWIN_FULL);
}

static void EndMoveList(void)
{
    if (sTB->listTask != TASK_NONE)
        DestroyListMenuTask(sTB->listTask, NULL, NULL);
    sTB->listTask = TASK_NONE;
}

// newSpecies, newMoves[0..moveCount) -> the slot
static void CommitSlot(void)
{
    struct BuiltTeam *team = Team();
    u32 i;
    team->species[sTB->slot] = sTB->newSpecies;
    for (i = 0; i < MAX_MON_MOVES; i++)
        SetTeamMove(team, sTB->slot, i, i < sTB->moveCount ? sTB->newMoves[i] : MOVE_NONE);
}

static void ShowMoves(u32 species)
{
    u32 i;
    sTB->mode = MODE_MOVES;
    sTB->newSpecies = species;
    sTB->moveCount = 0;
    memset(sTB->newMoves, 0, sizeof(sTB->newMoves));
    for (i = 0; i < PER_PAGE; i++)
    {
        DestroyIcon(&sTB->icons[i]);
        DestroyBall(&sTB->balls[i]);
    }
    gSprites[sTB->cursor].invisible = TRUE;
    BuildMoveList(species);
    PrintMovesChosen();
    StartMoveList();
    PrintFooter(COMPOUND_STRING("{A_BUTTON} ADD MOVE   {B_BUTTON} DONE (KEEPS THOSE CHOSEN)"));
}

// ---- Running it ----
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

static void Task_TeamBuilder(u8 taskId)
{
    struct BuiltTeam *team;
    if (gPaletteFade.active)
        return;
    team = Team();
    switch (sTB->mode)
    {
    case MODE_TEAMS:
        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_PC_OFF);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sTB->mode = MODE_EXIT;
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            sTB->slot = 0;
            ShowEdit();
        }
        else if (JOY_NEW(SELECT_BUTTON) && TeamSize(team) != 0)
        {
            PlaySE(SE_SELECT);
            memset(team, 0, sizeof(*team));
            ShowTeams();
        }
        else if (JOY_REPEAT(DPAD_UP) || JOY_REPEAT(DPAD_DOWN))
        {
            PlaySE(SE_SELECT);
            sTB->team = (sTB->team + (JOY_REPEAT(DPAD_UP) ? BUILT_TEAMS - 1 : 1)) % BUILT_TEAMS;
            ShowTeams();
        }
        break;
    case MODE_EDIT:
        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            ShowTeams();
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            ShowTypes();
        }
        else if (JOY_NEW(SELECT_BUTTON) && team->species[sTB->slot] != SPECIES_NONE)
        {
            PlaySE(SE_SELECT);
            ShowMoves(team->species[sTB->slot]);
        }
        else if (JOY_NEW(START_BUTTON))
        {
            PlaySE(SE_SELECT);
            ShowAnalysis();
        }
        else if (JOY_REPEAT(DPAD_UP) || JOY_REPEAT(DPAD_DOWN))
        {
            PlaySE(SE_SELECT);
            sTB->slot = (sTB->slot + (JOY_REPEAT(DPAD_UP) ? PARTY_SIZE - 1 : 1)) % PARTY_SIZE;
            PrintSlots();
            PrintSlotDetails();
        }
        break;
    case MODE_ANALYSIS:
        if (JOY_NEW(B_BUTTON | START_BUTTON | A_BUTTON))
        {
            PlaySE(SE_SELECT);
            ShowEdit();
        }
        break;
    case MODE_TYPES:
        if (JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_SELECT);
            ShowEdit();
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            if (sTB->typeCursor == TYPE_COUNT)
            {
                sTB->newSpecies = SPECIES_NONE;
                sTB->moveCount = 0;
                CommitSlot();
                ShowEdit();
            }
            else
            {
                ShowGrid();
            }
        }
        else if (JOY_REPEAT(DPAD_ANY))
        {
            s32 c = sTB->typeCursor;
            if (JOY_REPEAT(DPAD_LEFT))
                c--;
            else if (JOY_REPEAT(DPAD_RIGHT))
                c++;
            else if (JOY_REPEAT(DPAD_UP))
                c -= 3;
            else if (JOY_REPEAT(DPAD_DOWN))
                c += 3;
            c = (c + TYPE_COUNT + 1) % (TYPE_COUNT + 1);
            PlaySE(SE_SELECT);
            sTB->typeCursor = c;
            PrintTypes();
        }
        break;
    case MODE_GRID:
        if (JOY_NEW(B_BUTTON))
        {
            u32 i;
            PlaySE(SE_SELECT);
            for (i = 0; i < PER_PAGE; i++)
            {
                DestroyIcon(&sTB->icons[i]);
                DestroyBall(&sTB->balls[i]);
            }
            ShowTypes();
        }
        else if (JOY_NEW(A_BUTTON) && GridIndex() < sTB->listCount)
        {
            PlaySE(SE_SELECT);
            ShowMoves(gDexFormSpecies[sTB->list[GridIndex()]]);
        }
        else if ((JOY_REPEAT(L_BUTTON) || JOY_REPEAT(R_BUTTON)) && PageCount() > 1)
        {
            u32 pages = PageCount();
            u32 page = JOY_REPEAT(R_BUTTON) ? (sTB->page + 1) % pages : (sTB->page + pages - 1) % pages;
            PlaySE(SE_SELECT);
            GridGoTo(page * PER_PAGE + sTB->cell);
        }
        else if (JOY_REPEAT(DPAD_ANY))
        {
            s32 col = sTB->cell % COLS, row = sTB->cell / COLS, page = sTB->page, pages = PageCount();
            u32 index;
            if (JOY_REPEAT(DPAD_LEFT) && --col < 0)
            {
                col = COLS - 1;
                page = (page + pages - 1) % pages;
            }
            else if (JOY_REPEAT(DPAD_RIGHT) && ++col >= COLS)
            {
                col = 0;
                page = (page + 1) % pages;
            }
            else if (JOY_REPEAT(DPAD_UP))
                row = (row + ROWS - 1) % ROWS;
            else if (JOY_REPEAT(DPAD_DOWN))
                row = (row + 1) % ROWS;
            PlaySE(SE_SELECT);
            index = page * PER_PAGE + row * COLS + col;
            while (index >= sTB->listCount && index >= COLS)
                index -= COLS;
            GridGoTo(index);
        }
        break;
    case MODE_MOVES:
    {
        s32 input = ListMenu_ProcessInput(sTB->listTask);
        if (input == LIST_NOTHING_CHOSEN)
            break;
        if (input == LIST_CANCEL || input == 0)
        {
            // B (or DONE) before any is chosen on a Pokémon already in the slot: its moves stay
            PlaySE(SE_SELECT);
            EndMoveList();
            if (sTB->moveCount != 0 || team->species[sTB->slot] != sTB->newSpecies)
                CommitSlot();
            ShowEdit();
            break;
        }
        PlaySE(SE_SELECT);
        sTB->newMoves[sTB->moveCount++] = input;
        EndMoveList();
        if (sTB->moveCount >= MAX_MON_MOVES)
        {
            CommitSlot();
            ShowEdit();
            break;
        }
        PrintMovesChosen();
        StartMoveList();
        break;
    }
    case MODE_EXIT:
        DestroyAllIcons();
        EndMoveList();
        FreeMonIconPalettes();
        FreeAllWindowBuffers();
        Free(sTB->tilemap);
        Free(sTB->bgTiles);
        Free(sTB->uiTiles);
        Free(sTB->loc);
        Free(sTB->list);
        Free(sTB->moveIds);
        if (sTB->moveItems != NULL)
            Free(sTB->moveItems);
        FREE_AND_SET_NULL(sTB);
        DestroyTask(taskId);
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        break;
    }
}

static void CB2_InitTeamBuilder(void)
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

    sTB = AllocZeroed(sizeof(*sTB));
    sTB->loc = AllocZeroed(NUM_SPECIES);
    sTB->list = AllocZeroed(sizeof(u16) * DEX_FORM_COUNT);
    sTB->moveIds = AllocZeroed(sizeof(u16) * 400);
    sTB->listTask = TASK_NONE;
    for (i = 0; i < PER_PAGE; i++)
        sTB->icons[i] = sTB->balls[i] = NO_SPRITE;
    for (i = 0; i < PARTY_SIZE; i++)
        sTB->slotIcons[i] = NO_SPRITE;
    TeamBuilder_Validate();
    LoadBackground();
    LoadUiSprites();
    LoadMonIconPalettes();
    FindOwned();
    PutWindowTilemap(WIN_TOP);
    PutWindowTilemap(WIN_FOOTER);
    sTB->cursor = CreateSprite(&sCursorTemplate, 0, 0, 0);
    ShowTeams();

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    ShowBg(0);
    ShowBg(1);
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB);
    SetMainCallback2(MainCB2);
    CreateTask(Task_TeamBuilder, 0);
}

static void Task_WaitFadeThenOpen(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_InitTeamBuilder);
    }
}

// Special (Tansy; the script waitstates)
void Academy_OpenTeamBuilder(void)
{
    LockPlayerFieldControls();
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_WaitFadeThenOpen, 0);
}
