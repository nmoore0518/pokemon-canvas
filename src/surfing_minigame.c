// Pokémon Canvas: Pikachu's Beach, Pokémon Yellow's surfing minigame, on the GBA. Played in the
// beach house on Route 19 with a Surfing Pikachu in the party (src/surfing_pikachu.c).
//
// A port of pret/pokeyellow's engine/minigame/surfing_pikachu.asm (its graphics and tilemaps:
// tools/gym_hack/surfing_minigame_gfx.py), done in the Game Boy's own terms: its screen (160x144,
// sprite coordinates with the hardware's +8/+16 offsets), its tiles' numbers, its 8.8 fixed point
// speeds. The GBA's wider screen shows the Game Boy's in the middle, with more of the sea on each
// side; the course is generated a little further ahead to fill it, with its wave heights kept by
// position on the course (the Game Boy kept a screen's worth, shifting).
//
// The game: Pikachu rides the waves at the course's own pace (1.5 pixels a frame); its speed builds
// while it rides. Off a wave's crest, fast enough, it jumps: the D-pad spins it (a full turn either
// way is a flip; up to three in a row, or mixed, for more points), and it has to come down at the
// angle of the water it lands on, or it wipes out and its speed's gone. HP (time) runs down; reach
// the goal and HP left and Radness (the tricks' points) add up to the score.
#include "global.h"
#include "surfing_pikachu.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "random.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "constants/vars.h"

// ---------------------------------------------------------------------------------------------
// Graphics (the Game Boy's colour numbers as palette indices)
// ---------------------------------------------------------------------------------------------
static const u32 sBgTiles[] = INCGFX_U32("graphics/surfing_minigame/bg.png", ".4bpp");
static const u32 sObjTiles[] = INCGFX_U32("graphics/surfing_minigame/obj.png", ".4bpp");
static const u32 sIntroTiles[] = INCGFX_U32("graphics/surfing_minigame/intro.png", ".4bpp");
#include "data/surfing_minigame_tilemaps.h"

#define TILE_BYTES 32
#define NUM_BG_TILES 65
#define NUM_INTRO_TILES 144
#define WINDOW_TILES 256        // (the status bar's copy of the course tiles, white made solid)

// Yellow's own colours for Pikachu's Beach (data/sgb/sgb_palettes.asm, the Game Boy Color's)
static const u16 sPalBeach[16] = {RGB(31, 31, 31), RGB(31, 31, 0), RGB(11, 23, 31), RGB(3, 3, 3), RGB(31, 31, 31)};
static const u16 sPalTitle[16] = {RGB(31, 31, 31), RGB(9, 9, 9), RGB(31, 21, 0), RGB(3, 3, 3), RGB(31, 31, 31)};
static const u16 sPalObj0[16] = {RGB(31, 31, 31), RGB(31, 31, 0), RGB(11, 23, 31), RGB(3, 3, 3)};
static const u16 sPalObj1[16] = {RGB(31, 31, 31), RGB(31, 31, 31), RGB(11, 23, 31), RGB(3, 3, 3)};   // (OBP1: white, white, dark, black)

// The Game Boy's screen in the GBA's: sprite coordinates (with the +8/+16 hardware offsets) and tiles
#define GB_W 160
#define GB_H 144
#define OFS_X ((DISPLAY_WIDTH - GB_W) / 2)      // 40
#define OFS_Y ((DISPLAY_HEIGHT - GB_H) / 2)     // 8
#define SPRITE_X(x) ((s32)(x) - 8 + OFS_X)
#define SPRITE_Y(y) ((s32)(y) - 16 + OFS_Y)

// VRAM
#define COURSE_MAP_BLOCK 28     // BG0: 64x32 (the course, the title, the results)
#define WINDOW_MAP_BLOCK 30     // BG1: the status bar
#define COURSE_W 64
#define COURSE_H 32
#define BG0_MAP ((u16 *)BG_SCREEN_ADDR(COURSE_MAP_BLOCK))
#define BG1_MAP ((u16 *)BG_SCREEN_ADDR(WINDOW_MAP_BLOCK))

// ---------------------------------------------------------------------------------------------
// The course (surfing_pikachu.asm's tables)
// ---------------------------------------------------------------------------------------------
#define FLAT_WATER_Y 0x74
#define CENTER_X (GB_W / 2 + 8)
#define COURSE_SECTIONS 0x18

static const u8 sMetatiles[][4] =
{
    {0x00, 0x00, 0x00, 0x00}, {0x0b, 0x0b, 0x0b, 0x0b}, {0x0b, 0x02, 0x02, 0x06}, {0x03, 0x0b, 0x07, 0x03},
    {0x06, 0x06, 0x06, 0x06}, {0x07, 0x07, 0x07, 0x07}, {0x06, 0x04, 0x04, 0x08}, {0x05, 0x07, 0x08, 0x05},
    {0x0b, 0x0b, 0x11, 0x12}, {0x0b, 0x0b, 0x13, 0x03}, {0x14, 0x12, 0x04, 0x08}, {0x13, 0x07, 0x08, 0x05},
    {0x06, 0x14, 0x06, 0x14}, {0x13, 0x07, 0x13, 0x07}, {0x08, 0x08, 0x08, 0x08}, {0x14, 0x12, 0x14, 0x12},
    {0x0b, 0x11, 0x02, 0x14}, {0x06, 0x14, 0x06, 0x14}, {0x0c, 0x0c, 0x0d, 0x0d}, {0x0d, 0x0d, 0x0d, 0x0d},
    {0x0e, 0x0f, 0x10, 0x0b}, {0x12, 0x13, 0x12, 0x13},
};

enum { PAT_00, PAT_01, PAT_02, PAT_03, PAT_04, PAT_05, PAT_06, PAT_07, PAT_08, PAT_09, PAT_0A, PAT_0B, PAT_0C,
       PAT_0D, PAT_0E, PAT_0F, PAT_10, PAT_11, PAT_12, PAT_13, PAT_14, PAT_15, PAT_16, PAT_17, PAT_18, PAT_19,
       PAT_1A, PAT_1B, PAT_1C, PAT_BEACH, PAT_COUNT };

static const u8 sPatterns[PAT_COUNT][8] =
{
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01}, {0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x06},
    {0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x06, 0x0e}, {0x00, 0x00, 0x00, 0x10, 0x11, 0x06, 0x0e, 0x0e},
    {0x00, 0x00, 0x00, 0x15, 0x15, 0x0e, 0x0e, 0x0e}, {0x00, 0x00, 0x00, 0x03, 0x05, 0x07, 0x0e, 0x0e},
    {0x00, 0x00, 0x00, 0x01, 0x03, 0x05, 0x07, 0x0e}, {0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x05, 0x07},
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x06}, {0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x06, 0x0e},
    {0x00, 0x00, 0x00, 0x08, 0x0f, 0x0a, 0x0e, 0x0e}, {0x00, 0x00, 0x00, 0x09, 0x0d, 0x0b, 0x0e, 0x0e},
    {0x00, 0x00, 0x00, 0x01, 0x03, 0x05, 0x07, 0x0e}, {0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x05, 0x07},
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x06}, {0x00, 0x00, 0x00, 0x01, 0x10, 0x11, 0x06, 0x0e},
    {0x00, 0x00, 0x00, 0x01, 0x15, 0x15, 0x0e, 0x0e}, {0x00, 0x00, 0x00, 0x01, 0x03, 0x05, 0x07, 0x0e},
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x05, 0x07}, {0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x06},
    {0x00, 0x00, 0x00, 0x01, 0x08, 0x0f, 0x0a, 0x0e}, {0x00, 0x00, 0x00, 0x01, 0x09, 0x0d, 0x0b, 0x0e},
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x05, 0x07}, {0x00, 0x00, 0x00, 0x01, 0x01, 0x10, 0x11, 0x06},
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x15, 0x15, 0x0e}, {0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x05, 0x07},
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0f, 0x0a}, {0x00, 0x00, 0x00, 0x01, 0x01, 0x09, 0x0d, 0x0b},
    {0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x14, 0x14}, {0x00, 0x00, 0x00, 0x12, 0x13, 0x13, 0x13, 0x13},
};

// Each pattern's wave heights (left and right half), in tiles above the flat water
static const u8 sPatternHeights[PAT_COUNT][2] =
{
    [PAT_00] = {0, 0}, [PAT_01] = {0, 1}, [PAT_02] = {2, 3}, [PAT_03] = {4, 5}, [PAT_04] = {6, 6},
    [PAT_05] = {6, 5}, [PAT_06] = {4, 3}, [PAT_07] = {2, 1}, [PAT_08] = {0, 1}, [PAT_09] = {2, 3},
    [PAT_0A] = {4, 5}, [PAT_0B] = {5, 5}, [PAT_0C] = {4, 3}, [PAT_0D] = {2, 1}, [PAT_0E] = {0, 1},
    [PAT_0F] = {2, 3}, [PAT_10] = {4, 4}, [PAT_11] = {4, 3}, [PAT_12] = {2, 1}, [PAT_13] = {0, 1},
    [PAT_14] = {2, 3}, [PAT_15] = {3, 3}, [PAT_16] = {2, 1}, [PAT_17] = {0, 1}, [PAT_18] = {2, 2},
    [PAT_19] = {2, 1}, [PAT_1A] = {0, 1}, [PAT_1B] = {1, 1}, [PAT_1C] = {0, 0}, [PAT_BEACH] = {0, 0},
};

// The wave functions (.WaveFunctions): what each step of a wave sequence lays down next
enum { WF_CHOOSE, WF_ADVANCE, WF_FLAT_RESET, WF_FLAT, WF_BEACH_RESET };
#define WF(kind, pat) ((kind) << 8 | (pat))
static const u16 sWaveFunctions[] =
{
    WF(WF_CHOOSE, 0),
    // 01
    WF(WF_ADVANCE, PAT_13), WF(WF_ADVANCE, PAT_14), WF(WF_ADVANCE, PAT_15), WF(WF_ADVANCE, PAT_16),
    WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_17), WF(WF_ADVANCE, PAT_18), WF(WF_ADVANCE, PAT_19),
    WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00),
    WF(WF_FLAT_RESET, PAT_00),
    // 0e
    WF(WF_ADVANCE, PAT_08), WF(WF_ADVANCE, PAT_09), WF(WF_ADVANCE, PAT_0A), WF(WF_ADVANCE, PAT_0B),
    WF(WF_ADVANCE, PAT_0C), WF(WF_ADVANCE, PAT_0D), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00),
    WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_FLAT_RESET, PAT_00),
    // 1a
    WF(WF_ADVANCE, PAT_0E), WF(WF_ADVANCE, PAT_0F), WF(WF_ADVANCE, PAT_10), WF(WF_ADVANCE, PAT_11),
    WF(WF_ADVANCE, PAT_12), WF(WF_ADVANCE, PAT_0E), WF(WF_ADVANCE, PAT_0F), WF(WF_ADVANCE, PAT_10),
    WF(WF_ADVANCE, PAT_11), WF(WF_ADVANCE, PAT_12), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00),
    WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_FLAT_RESET, PAT_00),
    // 29
    WF(WF_ADVANCE, PAT_13), WF(WF_ADVANCE, PAT_14), WF(WF_ADVANCE, PAT_15), WF(WF_ADVANCE, PAT_16),
    WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00),
    WF(WF_FLAT_RESET, PAT_00),
    // 32
    WF(WF_ADVANCE, PAT_17), WF(WF_ADVANCE, PAT_18), WF(WF_ADVANCE, PAT_19), WF(WF_ADVANCE, PAT_17),
    WF(WF_ADVANCE, PAT_18), WF(WF_ADVANCE, PAT_19), WF(WF_ADVANCE, PAT_17), WF(WF_ADVANCE, PAT_18),
    WF(WF_ADVANCE, PAT_19), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00),
    WF(WF_ADVANCE, PAT_00), WF(WF_FLAT_RESET, PAT_00),
    // 40
    WF(WF_ADVANCE, PAT_1A), WF(WF_ADVANCE, PAT_1B), WF(WF_ADVANCE, PAT_0E), WF(WF_ADVANCE, PAT_0F),
    WF(WF_ADVANCE, PAT_10), WF(WF_ADVANCE, PAT_11), WF(WF_ADVANCE, PAT_12), WF(WF_ADVANCE, PAT_1A),
    WF(WF_ADVANCE, PAT_1B), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00),
    WF(WF_FLAT_RESET, PAT_00),
    // 4d
    WF(WF_ADVANCE, PAT_08), WF(WF_ADVANCE, PAT_09), WF(WF_ADVANCE, PAT_0A), WF(WF_ADVANCE, PAT_0B),
    WF(WF_ADVANCE, PAT_0C), WF(WF_ADVANCE, PAT_0D), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_1A),
    WF(WF_ADVANCE, PAT_1B), WF(WF_ADVANCE, PAT_1A), WF(WF_ADVANCE, PAT_1B), WF(WF_ADVANCE, PAT_00),
    WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_FLAT_RESET, PAT_00),
    // 5c
    WF(WF_ADVANCE, PAT_0E), WF(WF_ADVANCE, PAT_0F), WF(WF_ADVANCE, PAT_10), WF(WF_ADVANCE, PAT_11),
    WF(WF_ADVANCE, PAT_12), WF(WF_ADVANCE, PAT_13), WF(WF_ADVANCE, PAT_14), WF(WF_ADVANCE, PAT_15),
    WF(WF_ADVANCE, PAT_16), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_00),
    WF(WF_ADVANCE, PAT_00), WF(WF_FLAT_RESET, PAT_00),
    // 6a: the Big Kahuna
    WF(WF_ADVANCE, PAT_01), WF(WF_ADVANCE, PAT_02), WF(WF_ADVANCE, PAT_03), WF(WF_ADVANCE, PAT_04),
    WF(WF_ADVANCE, PAT_05), WF(WF_ADVANCE, PAT_06), WF(WF_ADVANCE, PAT_07), WF(WF_FLAT, PAT_00),
    // 72: the beach
    WF(WF_ADVANCE, PAT_00), WF(WF_ADVANCE, PAT_1C), WF(WF_ADVANCE, PAT_BEACH), WF(WF_ADVANCE, PAT_BEACH),
    WF(WF_ADVANCE, PAT_BEACH), WF(WF_ADVANCE, PAT_BEACH), WF(WF_ADVANCE, PAT_BEACH), WF(WF_ADVANCE, PAT_BEACH),
    WF(WF_ADVANCE, PAT_BEACH), WF(WF_BEACH_RESET, PAT_BEACH),
};
#define WAVE_BIG_KAHUNA 0x6a
#define WAVE_BEACH 0x72
static const u8 sWaveSequenceStarts[8] = {0x01, 0x0e, 0x1a, 0x29, 0x32, 0x40, 0x4d, 0x5c};

// Course tiles: what each lands on (SurfingMinigame_TileInteraction)
#define TILE_RISING 0x06
#define TILE_FALLING 0x07
#define TILE_FACE 0x12
#define TILE_CREST 0x14

// A sine wave with amplitude 2, for the scanlines' wobble
static const s8 sLYSine[32] = {0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, -1, -1, -1, -1, -2, -2, -2, -1, -1, -1, -1, 0, 0};
// sin(pi * x / 32) for x = 0..31, 8.8 (the disassembly's sine_table 32, with -Q8)
static const u16 sSine[32] = {0, 25, 50, 74, 98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255,
                               256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121, 98, 74, 50, 25};

// ---------------------------------------------------------------------------------------------
// The animated objects (engine/gfx/animated_objects.asm, data/sprite_anims/surfing_pikachu_*.asm)
// ---------------------------------------------------------------------------------------------
#define ATTR_PAL1 0x10
#define ATTR_XFLIP 0x20
#define ATTR_YFLIP 0x40
#define ATTR_PRIO 0x80
#define F_XFLIP (ATTR_XFLIP << 1)
#define F_YFLIP (ATTR_YFLIP << 1)
#define CMD_DELANIM 0xfc
#define CMD_DOREPEAT 0xfd
#define CMD_DORESTART 0xfe
#define CMD_ENDANIM 0xff

struct OamPart { s8 y, x; u8 tile, attr; };
struct OamSet { u8 tileBase; u8 count; const struct OamPart *parts; };

static const struct OamPart sOam1x1[] = {{-4, -4, 0x00, 0}};
static const struct OamPart sOamSurfingPikachu[] =
{
    {-12, -12, 0x00, 0}, {-12, -4, 0x01, 0}, {-12, 4, 0x02, 0},
    {-4, -12, 0x10, 0}, {-4, -4, 0x11, 0}, {-4, 4, 0x12, 0},
    {4, -12, 0x20, 0}, {4, -4, 0x21, 0}, {4, 4, 0x22, 0},
};
static const struct OamPart sOamAnnouncedText[] =
{
    {-8, -24, 0x00, 0}, {-8, -16, 0x01, 0}, {-8, -8, 0x02, 0}, {-8, 0, 0x03, 0}, {-8, 8, 0x04, 0}, {-8, 16, 0x05, 0},
    {0, -24, 0x10, 0}, {0, -16, 0x11, 0}, {0, -8, 0x12, 0}, {0, 0, 0x13, 0}, {0, 8, 0x14, 0}, {0, 16, 0x15, 0},
};
static const struct OamPart sOamWaterSpray[] = {{-4, 11, 0x00, ATTR_PAL1}, {4, 3, 0x0f, ATTR_PAL1}, {4, 11, 0x10, ATTR_PAL1}};
static const struct OamPart sOamSmallSplash[] =
{
    {-4, -16, 0x00, ATTR_PAL1 | ATTR_XFLIP}, {-4, 8, 0x00, ATTR_PAL1}, {4, -16, 0x10, ATTR_PAL1 | ATTR_XFLIP},
    {4, -8, 0x0f, ATTR_PAL1 | ATTR_XFLIP}, {4, 0, 0x0f, ATTR_PAL1}, {4, 8, 0x10, ATTR_PAL1},
};
static const struct OamPart sOamLargeSplash[] =
{
    {-12, -16, 0x00, ATTR_PAL1}, {-12, -8, 0x01, ATTR_PAL1}, {-12, 0, 0x01, ATTR_PAL1 | ATTR_XFLIP}, {-12, 8, 0x00, ATTR_PAL1 | ATTR_XFLIP},
    {-4, -16, 0x10, ATTR_PAL1}, {-4, -8, 0x11, ATTR_PAL1}, {-4, 0, 0x11, ATTR_PAL1 | ATTR_XFLIP}, {-4, 8, 0x10, ATTR_PAL1 | ATTR_XFLIP},
    {4, -16, 0x20, ATTR_PAL1}, {4, -8, 0x21, ATTR_PAL1}, {4, 0, 0x21, ATTR_PAL1 | ATTR_XFLIP}, {4, 8, 0x20, ATTR_PAL1 | ATTR_XFLIP},
};
static const struct OamPart sOamEmptySurfboard[] = {{4, -12, 0x00, 0}, {4, -4, 0x01, 0}, {4, 4, 0x02, 0}};
static const struct OamPart sOamPlus50[] = {{-4, -12, 0xbf, 0}, {-4, -4, 0xd5, 0}, {-4, 4, 0xd0, 0}};
static const struct OamPart sOamPlus150[] = {{-4, -16, 0xbf, 0}, {-4, -8, 0xd1, 0}, {-4, 0, 0xd5, 0}, {-4, 8, 0xd0, 0}};
static const struct OamPart sOamPlus350[] = {{-4, -16, 0xbf, 0}, {-4, -8, 0xd3, 0}, {-4, 0, 0xd5, 0}, {-4, 8, 0xd0, 0}};
static const struct OamPart sOamPlus750[] = {{-4, -16, 0xbf, 0}, {-4, -8, 0xd7, 0}, {-4, 0, 0xd5, 0}, {-4, 8, 0xd0, 0}};
static const struct OamPart sOamPlus180[] = {{-4, -16, 0xbf, 0}, {-4, -8, 0xd1, 0}, {-4, 0, 0xd8, 0}, {-4, 8, 0xd0, 0}};
static const struct OamPart sOamPlus500[] = {{-4, -16, 0xbf, 0}, {-4, -8, 0xd5, 0}, {-4, 0, 0xd0, 0}, {-4, 8, 0xd0, 0}};
static const struct OamPart sOamIntroPikachu[] =
{
    {-12, -16, 0x03, ATTR_XFLIP}, {-12, -8, 0x02, ATTR_XFLIP}, {-12, 0, 0x01, ATTR_XFLIP}, {-12, 8, 0x00, ATTR_XFLIP},
    {-4, -16, 0x13, ATTR_XFLIP}, {-4, -8, 0x12, ATTR_XFLIP}, {-4, 0, 0x11, ATTR_XFLIP}, {-4, 8, 0x10, ATTR_XFLIP},
    {4, -16, 0x23, ATTR_XFLIP}, {4, -8, 0x22, ATTR_XFLIP}, {4, 0, 0x21, ATTR_XFLIP}, {4, 8, 0x20, ATTR_XFLIP},
};

#define OAMSET(base, parts) {base, ARRAY_COUNT(parts), parts}
enum
{
    OS_00, OS_ANGLE_1_1, OS_ANGLE_1_2, OS_ANGLE_2_1, OS_ANGLE_2_2, OS_ANGLE_3_1, OS_ANGLE_3_2, OS_ANGLE_4_1,
    OS_ANGLE_4_2, OS_ANGLE_5_1, OS_ANGLE_5_2, OS_ANGLE_6_1, OS_ANGLE_6_2, OS_ANGLE_7_1, OS_ANGLE_7_2,
    OS_FRONT_UNUSED, OS_BACK_UNUSED, OS_SMALL_SPLASH_1, OS_SMALL_SPLASH_2, OS_BIG_SPLASH_1, OS_BIG_SPLASH_2,
    OS_BIG_SPLASH_3, OS_START, OS_GOAL, OS_OH_NO, OS_WATER_SPRAY, OS_PLUS_50, OS_PLUS_150, OS_PLUS_350,
    OS_PLUS_750, OS_PLUS_180, OS_PLUS_500, OS_INTRO_1, OS_INTRO_2, OS_INTRO_3, OS_INTRO_4,
};
static const struct OamSet sOamSets[] =
{
    OAMSET(0x00, sOam1x1),
    OAMSET(0x00, sOamSurfingPikachu), OAMSET(0x36, sOamSurfingPikachu), OAMSET(0x03, sOamSurfingPikachu),
    OAMSET(0x39, sOamSurfingPikachu), OAMSET(0x06, sOamSurfingPikachu), OAMSET(0x3c, sOamSurfingPikachu),
    OAMSET(0x09, sOamSurfingPikachu), OAMSET(0x60, sOamSurfingPikachu), OAMSET(0x0c, sOamSurfingPikachu),
    OAMSET(0x63, sOamSurfingPikachu), OAMSET(0x30, sOamSurfingPikachu), OAMSET(0x66, sOamSurfingPikachu),
    OAMSET(0x33, sOamSurfingPikachu), OAMSET(0x69, sOamSurfingPikachu), OAMSET(0x6c, sOamSurfingPikachu),
    OAMSET(0x9c, sOamSurfingPikachu), OAMSET(0xa0, sOamSurfingPikachu), OAMSET(0xa3, sOamSurfingPikachu),
    OAMSET(0xa7, sOamSmallSplash), OAMSET(0xa8, sOamLargeSplash), OAMSET(0x98, sOamEmptySurfboard),
    OAMSET(0xe0, sOamAnnouncedText), OAMSET(0xe6, sOamAnnouncedText), OAMSET(0xca, sOamAnnouncedText),
    OAMSET(0xa7, sOamWaterSpray), OAMSET(0x00, sOamPlus50), OAMSET(0x00, sOamPlus150), OAMSET(0x00, sOamPlus350),
    OAMSET(0x00, sOamPlus750), OAMSET(0x00, sOamPlus180), OAMSET(0x00, sOamPlus500),
    OAMSET(0x80, sOamIntroPikachu), OAMSET(0x84, sOamIntroPikachu), OAMSET(0x88, sOamIntroPikachu),
    OAMSET(0x8c, sOamIntroPikachu),
};

// Framesets: {oam set, duration | flip flags} pairs, or a command
#define ANGLE(a, flags) {OS_ANGLE_##a##_1, 8 | (flags), OS_ANGLE_##a##_2, 8 | (flags), CMD_DORESTART, 0}
#define POINTS(os) {os, 4, CMD_DOREPEAT, 1, os, 3, CMD_DOREPEAT, 1, os, 2, CMD_DOREPEAT, 1, os, 1, CMD_DELANIM, 0}
enum
{
    FS_00, FS_ANGLE_1, FS_ANGLE_14 = 14, FS_SMALL_SPLASH, FS_LARGE_SPLASH, FS_START, FS_GOAL, FS_OH_NO, FS_WATER_SPRAY,
    FS_PLUS_50, FS_PLUS_150, FS_PLUS_350, FS_PLUS_750, FS_PLUS_180, FS_PLUS_500, FS_INTRO_PIKACHU, FS_COUNT,
};
#define FS_FLAT 4   // (riding level)
static const u8 sFramesets[FS_COUNT][18] =
{
    [FS_00] = {OS_00, 32, CMD_ENDANIM, 0},
    [1] = ANGLE(1, 0), [2] = ANGLE(2, 0), [3] = ANGLE(3, 0), [4] = ANGLE(4, 0), [5] = ANGLE(5, 0), [6] = ANGLE(6, 0),
    [7] = ANGLE(7, 0), [8] = ANGLE(1, F_XFLIP | F_YFLIP), [9] = ANGLE(2, F_XFLIP | F_YFLIP),
    [10] = ANGLE(3, F_XFLIP | F_YFLIP), [11] = ANGLE(4, F_XFLIP | F_YFLIP), [12] = ANGLE(5, F_XFLIP | F_YFLIP),
    [13] = ANGLE(6, F_XFLIP | F_YFLIP), [14] = ANGLE(7, F_XFLIP | F_YFLIP),
    [FS_SMALL_SPLASH] = {OS_SMALL_SPLASH_1, 7, OS_SMALL_SPLASH_2, 7, CMD_DORESTART, 0},
    [FS_LARGE_SPLASH] = {OS_BIG_SPLASH_1, 2, OS_BIG_SPLASH_2, 2, CMD_DOREPEAT, 8, OS_BIG_SPLASH_3, 2, CMD_ENDANIM, 0},
    [FS_START] = {OS_START, 32, OS_START, 32, CMD_DELANIM, 0},
    [FS_GOAL] = {OS_GOAL, 32, OS_GOAL, 32, CMD_DELANIM, 0},
    [FS_OH_NO] = {OS_OH_NO, 32, CMD_ENDANIM, 0},
    [FS_WATER_SPRAY] = {OS_WATER_SPRAY, 1, CMD_DELANIM, 0},
    [FS_PLUS_50] = POINTS(OS_PLUS_50), [FS_PLUS_150] = POINTS(OS_PLUS_150), [FS_PLUS_350] = POINTS(OS_PLUS_350),
    [FS_PLUS_750] = POINTS(OS_PLUS_750), [FS_PLUS_180] = POINTS(OS_PLUS_180), [FS_PLUS_500] = POINTS(OS_PLUS_500),
    [FS_INTRO_PIKACHU] = {OS_INTRO_1, 7, OS_INTRO_2, 7, OS_INTRO_3, 7, OS_INTRO_4, 7, CMD_DORESTART, 0},
};

enum { CB_NOP, CB_PIKACHU, CB_BANNER, CB_FLIPPING_TEXT, CB_INTRO_PIKACHU };
enum { OBJ_UNUSED, OBJ_PIKACHU, OBJ_START, OBJ_GOAL, OBJ_PLUS_50, OBJ_PLUS_150, OBJ_PLUS_350, OBJ_PLUS_750,
       OBJ_PLUS_180, OBJ_PLUS_500, OBJ_WATER_SPRAY, OBJ_OH_NO, OBJ_INTRO_PIKACHU };
static const u8 sSpawnData[][2] =    // frameset, callback
{
    {FS_00, CB_NOP}, {FS_FLAT, CB_PIKACHU}, {FS_START, CB_BANNER}, {FS_GOAL, CB_BANNER}, {FS_PLUS_50, CB_NOP},
    {FS_PLUS_150, CB_NOP}, {FS_PLUS_350, CB_NOP}, {FS_PLUS_750, CB_NOP}, {FS_PLUS_180, CB_NOP},
    {FS_PLUS_500, CB_NOP}, {FS_WATER_SPRAY, CB_NOP}, {FS_OH_NO, CB_FLIPPING_TEXT}, {FS_INTRO_PIKACHU, CB_INTRO_PIKACHU},
};

#define MAX_OBJECTS 10
struct AnimObj
{
    bool8 active;
    u8 frameset, callback;
    u8 x, y;
    s8 xOffset, yOffset;
    u8 duration, frameIndex;
    u8 fieldB, fieldC, fieldD, fieldE;
};

// ---------------------------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------------------------
enum { PIKA_RIDING, PIKA_JUMPING, PIKA_LANDING, PIKA_CRASHED, PIKA_GAME_END, PIKA_INIT_RESULTS, PIKA_RESULTS };
enum
{
    R_START, R_RUN, R_WAIT_RESULTS, R_SCROLL_TO_RESULTS, R_DRAW_RESULTS, R_HP_LEFT, R_RADNESS, R_TOTAL,
    R_ADD_HP, R_ADD_RADNESS, R_WAIT_LAST, R_EXIT_ON_A, R_GAME_OVER, R_DONE,
};
#define NUM_STATIC_SPRITES 14   // (HP digits 0-3, the progress marker 4, the clouds 5-13)

struct Surfing
{
    struct AnimObj objects[MAX_OBJECTS];
    struct AnimObj *pikachu;
    u8 courseTiles[COURSE_H][COURSE_W];     // (the Game Boy's tile numbers on BG0)
    u8 waveHeight[COURSE_W];                // (by course column: the GB's wSurfingMinigameWaveHeight)
    u8 staticX[NUM_STATIC_SPRITES], staticY[NUM_STATIC_SPRITES], staticTile[NUM_STATIC_SPRITES];
    u8 lyOverrides[GB_H];
    u16 scx;                // the course's scroll, in pixels (and wSurfingMinigameSCX, its fraction)
    u8 scxFraction;
    u16 lastScx, lastScxColumn;
    u16 generatedTo;        // the course column the waves have been laid down to
    u8 routine, routineDelay, gameOverDelay;
    bool8 gameOver, introDone;
    u8 pikachuState, pikachuHeight;
    u16 speed;              // 8.8
    u8 sections;            // wSurfingMinigameDistance: sections done, and the fraction
    u16 distanceFraction;
    u16 hp, radness, total;
    u8 radnessMeter, trickFlags, crashTimer, waterSprayCounter;
    u8 boardAngleOffset, boardAngleTimer;
    bool8 boardAngleDecreasing;
    u8 jumpArcMagnitude, jumpArcFraction;
    bool8 jumpDescending;
    u8 waveFunction, waveRandom;
    u8 bgRead[2];           // the tile under Pikachu and the next (wSurfingMinigameBGMapReadBuffer)
    u8 bgPending[2];        // (read this frame: the Game Boy copied it in the next VBlank)
    u16 behindCol;          // (on the way back to the beach: the next column to lay, behind the left edge)
    u8 cloudFraction;
    u8 joy;                 // held buttons, sampled every third frame (hJoy5)
    u8 joyTimer;
    bool8 newHighScore;
    bool8 lyEffect;
};

static EWRAM_DATA struct Surfing *sSurf = NULL;

static void Intro_Init(void);
static void Game_Init(void);
static void CB2_Intro(void);
static void CB2_Game(void);
static void CB2_Exit(void);

// ---------------------------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------------------------
static void SetCourseTile(u32 col, u32 row, u8 tile)
{
    u16 entry;
    col %= COURSE_W;
    row %= COURSE_H;
    sSurf->courseTiles[row][col] = tile;
    entry = tile;
    BG0_MAP[(col >= 32 ? 0x400 : 0) + row * 32 + (col % 32)] = entry;
}

static void LoadTilesTo(void *dest, const u32 *src, u32 first, u32 count)
{
    CpuCopy32((const u8 *)src + first * TILE_BYTES, dest, count * TILE_BYTES);
}

static void DrawObject(struct AnimObj *o, u32 *oamIndex)
{
    const u8 *script = sFramesets[o->frameset];
    u8 a, flags, objAttr;
    u32 i;

    // UpdateDurationTimerAndFrameStateForCurrentAnimatedObject
    for (;;)
    {
        if (o->duration)
        {
            o->duration--;
            a = script[o->frameIndex * 2];
            flags = script[o->frameIndex * 2 + 1];
            break;
        }
        o->frameIndex++;
        a = script[o->frameIndex * 2];
        if (a == CMD_DORESTART)
        {
            o->duration = 0;
            o->frameIndex = 0xFF;
            continue;
        }
        if (a == CMD_ENDANIM)
        {
            o->duration = 0;
            o->frameIndex -= 2;
            continue;
        }
        flags = script[o->frameIndex * 2 + 1];
        o->duration = flags & 0x3f;
        break;
    }
    if (a == CMD_DOREPEAT)
        return;
    if (a == CMD_DELANIM)
    {
        o->active = FALSE;
        return;
    }
    objAttr = (flags & 0xc0) >> 1;
    for (i = 0; i < sOamSets[a].count && *oamIndex < 128; i++)
    {
        const struct OamPart *part = &sOamSets[a].parts[i];
        s8 dy = part->y, dx = part->x;
        u8 attr = ((part->attr ^ objAttr) & (ATTR_XFLIP | ATTR_YFLIP | ATTR_PRIO)) | (part->attr & ATTR_PAL1);
        struct OamData *oam = &gMain.oamBuffer[(*oamIndex)++];
        u8 y, x;
        if (objAttr & ATTR_YFLIP)
            dy = -(dy + 8);
        if (objAttr & ATTR_XFLIP)
            dx = -(dx + 8);
        y = o->y + o->yOffset + dy;
        x = o->x + o->xOffset + dx;
        *oam = (struct OamData){0};
        oam->y = (SPRITE_Y(y) + 256) & 0xFF;
        oam->x = (SPRITE_X(x) + 512) & 0x1FF;
        oam->tileNum = (u8)(sOamSets[a].tileBase + part->tile);
        oam->paletteNum = (attr & ATTR_PAL1) ? 1 : 0;
        oam->matrixNum = ((attr & ATTR_XFLIP) ? ST_OAM_HFLIP : 0) | ((attr & ATTR_YFLIP) ? ST_OAM_VFLIP : 0);
        oam->priority = (attr & ATTR_PRIO) ? 2 : 0;
        // (sprites the Game Boy would hide off its screen's top or bottom)
        if (y == 0 || y >= GB_H + 16)
            oam->affineMode = ST_OAM_AFFINE_ERASE;
    }
}

static struct AnimObj *SpawnObject(u32 id, u8 y, u8 x)
{
    u32 i;
    for (i = 0; i < MAX_OBJECTS; i++)
    {
        struct AnimObj *o = &sSurf->objects[i];
        if (o->active)
            continue;
        *o = (struct AnimObj){0};
        o->active = TRUE;
        o->frameset = sSpawnData[id][0];
        o->callback = sSpawnData[id][1];
        o->x = x;
        o->y = y;
        o->frameIndex = 0xFF;
        return o;
    }
    return NULL;
}

static void SetFrameset(struct AnimObj *o, u8 frameset)
{
    o->frameset = frameset;
    o->duration = 0;
    o->frameIndex = 0xFF;
}

// ---------------------------------------------------------------------------------------------
// The sine (SurfingPikachu_Sine): d * sin(a), a in 64 steps
// ---------------------------------------------------------------------------------------------
static s8 Sine(u8 a, u8 d)
{
    a &= 0x3f;
    if (a < 0x20)
        return (s8)((d * sSine[a]) >> 8);
    return (s8)(-(s32)((d * sSine[a & 0x1f]) >> 8));
}

// ---------------------------------------------------------------------------------------------
// The course
// ---------------------------------------------------------------------------------------------
// The next slice of waves (SurfingMinigame_GetWaveDataPointers): its pattern and heights
static u32 NextWaveSlice(u8 *left, u8 *right)
{
    u16 wf = sWaveFunctions[sSurf->waveFunction];
    u32 kind = wf >> 8, pat = wf & 0xFF;
    if (kind == WF_CHOOSE)
    {
        if (sSurf->sections >= 0x16)
        {
            if (sSurf->sections == 0x16)
                sSurf->waveFunction = WAVE_BIG_KAHUNA;
        }
        else if (sSurf->waveRandom)
        {
            sSurf->waveFunction = sWaveSequenceStarts[(sSurf->waveRandom - 1) & 7];
        }
        pat = PAT_00;
    }
    else if (kind == WF_ADVANCE)
    {
        sSurf->waveFunction++;
    }
    else if (kind == WF_FLAT_RESET || kind == WF_BEACH_RESET)
    {
        sSurf->waveFunction = 0;
    }
    *left = FLAT_WATER_Y - 8 * sPatternHeights[pat][0];
    *right = FLAT_WATER_Y - 8 * sPatternHeights[pat][1];
    return pat;
}

// Lays the next 16-pixel column of the course at `col` (in tiles)
static void LayColumn(u32 col)
{
    u8 left, right;
    u32 pat = NextWaveSlice(&left, &right), m;
    sSurf->waveHeight[col % COURSE_W] = left;
    sSurf->waveHeight[(col + 1) % COURSE_W] = right;
    for (m = 0; m < 8; m++)
    {
        const u8 *mt = sMetatiles[sPatterns[pat][m]];
        SetCourseTile(col, m * 2, mt[0]);
        SetCourseTile(col + 1, m * 2, mt[1]);
        SetCourseTile(col, m * 2 + 1, mt[2]);
        SetCourseTile(col + 1, m * 2 + 1, mt[3]);
    }
}

// The waves ahead are laid down to just past the GBA screen's right edge
static void GenerateAhead(void)
{
    u32 want = (sSurf->scx + DISPLAY_WIDTH - OFS_X + 16) / 8;
    while (sSurf->generatedTo < want)
    {
        LayColumn(sSurf->generatedTo);
        sSurf->generatedTo += 2;
    }
}

// The wave height under the Game Boy screen's tile column j (its wSurfingMinigameWaveHeight + j)
static u8 WaveHeight(u32 j)
{
    return sSurf->waveHeight[((sSurf->scx & ~15) / 8 + 2 + j) % COURSE_W];
}

// SurfingMinigame_ReadBGMapBuffer: the course tile 9 tiles into the screen, at Pikachu's height
static void ReadBgUnderPikachu(void)
{
    u32 col = (sSurf->scx + 9 * 8) / 8, row = (sSurf->pikachuHeight / 8) % COURSE_H;
    sSurf->bgPending[0] = sSurf->courseTiles[row][col % COURSE_W];
    sSurf->bgPending[1] = sSurf->courseTiles[row][(col + 1) % COURSE_W];
}

static void SetPikachuHeight(void)
{
    u8 h = WaveHeight((sSurf->scx & 8) ? 8 : 7), px = sSurf->scx & 7;
    switch (sSurf->bgRead[0])
    {
    case TILE_RISING:
    case TILE_CREST:
        sSurf->pikachuHeight = h - px;
        break;
    case TILE_FALLING:
        sSurf->pikachuHeight = h + px;
        break;
    default:
        sSurf->pikachuHeight = h;
        break;
    }
}

static void ScrollCourse(u32 step8_8)
{
    u32 total = (sSurf->scxFraction + step8_8);
    sSurf->scx += total >> 8;
    sSurf->scxFraction = total & 0xFF;
    GenerateAhead();
}

// The wobble: the lines below the top two tiles roll by one each frame (SurfingMinigame_UpdateLYOverrides)
static void UpdateLYOverrides(void)
{
    u32 i;
    u8 first = sSurf->lyOverrides[16];
    for (i = 16; i < GB_H - 1; i++)
        sSurf->lyOverrides[i] = sSurf->lyOverrides[i + 1];
    sSurf->lyOverrides[GB_H - 1] = first;
}

// ---------------------------------------------------------------------------------------------
// Pikachu
// ---------------------------------------------------------------------------------------------
static void SpeedUp(void)
{
    if ((sSurf->speed >> 8) < 2)
        sSurf->speed += 2;      // (1/128 a frame)
}

static void ReduceSpeed(u16 by)
{
    sSurf->speed = sSurf->speed > by ? sSurf->speed - by : 0;
}

static u8 SpeedDividedBy32(void)
{
    return (sSurf->speed << 3) >> 8;
}

static void UpdateBoardAngle(void)
{
    if (sSurf->boardAngleTimer++ & 7)
        return;
    if (sSurf->boardAngleDecreasing)
    {
        if (sSurf->boardAngleOffset == 0)
            sSurf->boardAngleDecreasing = FALSE;
        else
            sSurf->boardAngleOffset--;
    }
    else
    {
        if (sSurf->boardAngleOffset == 2)
            sSurf->boardAngleDecreasing = TRUE;
        else
            sSurf->boardAngleOffset++;
    }
}

static void UpdateSurfingFrame(struct AnimObj *o)
{
    u8 px = sSurf->scx & 7, base;
    if (px < 3 || px >= 5)
        return;
    switch (sSurf->bgRead[0])
    {
    case TILE_RISING:
    case TILE_CREST:
        base = 6;
        break;
    case TILE_FALLING:
        base = 2;
        break;
    default:
        UpdateBoardAngle();
        o->frameset = FS_FLAT;
        return;
    }
    o->frameset = base + sSurf->boardAngleOffset - 1;
}

static bool32 TryStartJump(void)
{
    u8 px = sSurf->scx & 7, a;
    if (px < 3 || px >= 5 || sSurf->bgRead[0] != TILE_CREST)
        return FALSE;
    a = SpeedDividedBy32();
    if (a < 10)
        return FALSE;
    sSurf->jumpArcMagnitude = a;
    sSurf->jumpDescending = FALSE;
    sSurf->jumpArcFraction = 0;
    return TRUE;
}

static void SpawnWaterSpray(struct AnimObj *o)
{
    u8 y, px;
    if (sSurf->waterSprayCounter++ & 3)
        return;
    y = WaveHeight((sSurf->scx & 8) ? 9 : 8);
    px = sSurf->scx & 7;
    switch (sSurf->bgRead[1])
    {
    case TILE_RISING:
    case TILE_CREST:
        y -= px;
        break;
    case TILE_FALLING:
        y += px;
        break;
    }
    SpawnObject(OBJ_WATER_SPRAY, y, o->x);
}

static void IncreaseRadnessMeter(void)
{
    if (sSurf->radnessMeter < 3)
        sSurf->radnessMeter++;
}

static void AddRadness(u32 points)
{
    sSurf->radness = min(sSurf->radness + points, 9999);
}

static void StartTrick(struct AnimObj *o)
{
    IncreaseRadnessMeter();
    o->fieldD = 0;
    o->fieldE = 0;
    PlaySE(SE_RG_CARD_FLIP);
}

// The D-pad in the air: each sampled press turns the board a step; enough in one direction is a flip
static void DPadAction(struct AnimObj *o)
{
    if (sSurf->joy & DPAD_LEFT)
    {
        o->fieldE = 0;
        if (o->fieldD++ >= 0xb)
        {
            StartTrick(o);
            sSurf->trickFlags |= 1;
        }
        o->frameset = o->frameset < 0xe ? o->frameset + 1 : 1;
    }
    else if (sSurf->joy & DPAD_RIGHT)
    {
        o->fieldD = 0;
        if (o->fieldE++ >= 0xd)
        {
            StartTrick(o);
            sSurf->trickFlags |= 2;
        }
        o->frameset = o->frameset == 1 ? 0xe : o->frameset - 1;
    }
}

// Coming down: how well the board's angle matches the water (FALSE: a wipeout)
static bool32 TileInteraction(struct AnimObj *o)
{
    enum { WIPEOUT, HARD, ROUGH, CLEAN };
    static const u8 sFlat[8] = {WIPEOUT, WIPEOUT, HARD, ROUGH, CLEAN, ROUGH, HARD, WIPEOUT};
    static const u8 sRising[8] = {WIPEOUT, WIPEOUT, WIPEOUT, WIPEOUT, HARD, ROUGH, CLEAN, ROUGH};
    static const u8 sFallingSlope[8] = {WIPEOUT, ROUGH, CLEAN, ROUGH, HARD, WIPEOUT, WIPEOUT, WIPEOUT};
    static const u8 sFace[8] = {WIPEOUT, WIPEOUT, HARD, ROUGH, CLEAN, CLEAN, ROUGH, HARD};
    const u8 *table;
    u32 landing;
    switch (sSurf->bgRead[0])
    {
    case TILE_RISING: table = sRising; break;
    case TILE_FALLING: table = sFallingSlope; break;
    case TILE_FACE:
    case TILE_CREST: table = sFace; break;
    default: table = sFlat; break;
    }
    landing = o->frameset < 8 ? table[o->frameset] : WIPEOUT;
    if (landing == WIPEOUT)
    {
        sSurf->speed = 0x40;    // (0.25)
        return FALSE;
    }
    if (landing == HARD)
        ReduceSpeed(0x80);
    else if (landing == ROUGH)
        ReduceSpeed(0x40);
    PlaySE(SE_RG_POKE_JUMP_SUCCESS);
    return TRUE;
}

// Points for the tricks of a jump: one flip 50; two the same way 150, three or more 350; mixed 180,
// three or more mixed 500
static void AddRadnessFromStunt(struct AnimObj *o)
{
    u32 meter = sSurf->radnessMeter, d, e, a;
    if (meter == 0)
        return;
    if ((sSurf->trickFlags & 3) == 3)
    {
        if (meter < 3)
        {
            AddRadness(180);
            SpawnObject(OBJ_PLUS_180, o->y - 16, o->x);
        }
        else
        {
            AddRadness(500);
            SpawnObject(OBJ_PLUS_500, o->y - 16, o->x);
        }
        return;
    }
    for (d = meter, e = 1, a = 0; d; d--, e <<= 1)
        a += e;
    AddRadness(50 * a);
    SpawnObject(OBJ_PLUS_50 + meter - 1, o->y - 16, o->x);
}

// The jump's arc: up while its magnitude falls, then down until it meets the water (TRUE)
static bool32 UpdatePikachuHeight(struct AnimObj *o)
{
    u16 arc = sSurf->jumpArcMagnitude << 8 | sSurf->jumpArcFraction;
    u16 pos = o->y << 8 | o->fieldC;
    u32 sq;
    if (!sSurf->jumpDescending)
    {
        if (arc == 0)
        {
            sSurf->jumpDescending = TRUE;
            return FALSE;
        }
        arc -= 0x80;
        sSurf->jumpArcMagnitude = arc >> 8;
        sSurf->jumpArcFraction = arc & 0xFF;
        sq = (u16)(4 * sSurf->jumpArcMagnitude * sSurf->jumpArcMagnitude);
        pos -= sq;
    }
    else
    {
        if (o->y < GB_H && o->y >= sSurf->pikachuHeight)
        {
            o->y = sSurf->pikachuHeight;
            o->fieldC = 0;
            return TRUE;
        }
        arc += 0x80;
        sSurf->jumpArcMagnitude = arc >> 8;
        sSurf->jumpArcFraction = arc & 0xFF;
        sq = (u16)(4 * sSurf->jumpArcMagnitude * sSurf->jumpArcMagnitude);
        pos += sq;
    }
    o->y = pos >> 8;
    o->fieldC = pos & 0xFF;
    return FALSE;
}

static void PikachuCallback(struct AnimObj *o)
{
    switch (sSurf->pikachuState)
    {
    case PIKA_RIDING:
        if (sSurf->gameOver)
        {
            sSurf->speed = 0;
            sSurf->pikachuState = PIKA_GAME_END;
            UpdateSurfingFrame(o);
            break;
        }
        SpawnWaterSpray(o);
        o->y = sSurf->pikachuHeight;
        if (TryStartJump())
        {
            UpdateSurfingFrame(o);
            sSurf->pikachuState = PIKA_JUMPING;
            o->fieldC = o->fieldD = o->fieldE = 0;
            sSurf->radnessMeter = 0;
            sSurf->trickFlags = 0;
            PlaySE(SE_LEDGE);
            break;
        }
        UpdateSurfingFrame(o);
        SpeedUp();
        break;
    case PIKA_JUMPING:
        DPadAction(o);
        if (!UpdatePikachuHeight(o))
            break;
        if (!TileInteraction(o))
        {
            sSurf->pikachuState = PIKA_CRASHED;
            sSurf->crashTimer = 0x60;
            SetFrameset(o, FS_LARGE_SPLASH);
            PlaySE(SE_RG_POKE_JUMP_FAILURE);
            break;
        }
        AddRadnessFromStunt(o);
        o->fieldC = 0;
        sSurf->pikachuState = PIKA_LANDING;
        break;
    case PIKA_LANDING:
        if (o->fieldC >= 0x20)
        {
            o->yOffset = 0;
            sSurf->pikachuState = PIKA_RIDING;
            break;
        }
        o->fieldC += 4;
        o->yOffset = Sine(o->fieldC, 4);
        SpawnWaterSpray(o);
        o->y = sSurf->pikachuHeight;
        break;
    case PIKA_CRASHED:
        if (sSurf->crashTimer == 0)
        {
            sSurf->pikachuState = PIKA_RIDING;
            SetFrameset(o, FS_FLAT);
            break;
        }
        sSurf->crashTimer--;
        o->y = sSurf->pikachuHeight;
        break;
    case PIKA_GAME_END:
        o->y = sSurf->pikachuHeight;
        UpdateSurfingFrame(o);
        break;
    case PIKA_INIT_RESULTS:
        SetFrameset(o, FS_SMALL_SPLASH);
        o->fieldC = 0;
        sSurf->pikachuState = PIKA_RESULTS;     // (the Game Boy waits on the results; here it's the same)
        break;
    case PIKA_RESULTS:
    {
        u8 c = o->fieldC;
        o->fieldC += 2;
        if ((c & 0x3f) < 0x20)
            o->yOffset = 0;
        else
            o->yOffset = Sine(c, 0x10);
        break;
    }
    }
}

static void RunObjects(void)
{
    u32 i, oam = NUM_STATIC_SPRITES + 1;
    for (i = 0; i < MAX_OBJECTS; i++)
    {
        struct AnimObj *o = &sSurf->objects[i];
        if (!o->active)
            continue;
        switch (o->callback)
        {
        case CB_PIKACHU:
            PikachuCallback(o);
            break;
        case CB_BANNER:
            if (o->x != CENTER_X)
                o->x += 4;
            break;
        case CB_FLIPPING_TEXT:
            if (o->fieldB)
            {
                u8 b = o->fieldB;
                s8 s;
                o->fieldB -= 2;
                s = Sine(o->fieldC++, b);
                o->yOffset = s < 0 ? -s : s;
            }
            break;
        case CB_INTRO_PIKACHU:
            if (o->fieldB++ & 1)
            {
                if (o->x == 0xc0)
                {
                    sSurf->introDone = TRUE;
                    o->active = FALSE;
                    continue;
                }
                o->x++;
            }
            break;
        }
        if (o->active)
            DrawObject(o, &oam);
    }
    for (; oam < 128; oam++)
        gMain.oamBuffer[oam] = gDummyOamData;
}

static void DrawStaticSprites(void)
{
    u32 i;
    for (i = 0; i < NUM_STATIC_SPRITES; i++)
    {
        struct OamData *oam = &gMain.oamBuffer[i];
        *oam = (struct OamData){0};
        if (sSurf->staticY[i] == 0)
        {
            *oam = gDummyOamData;
            continue;
        }
        oam->y = (SPRITE_Y(sSurf->staticY[i]) + 256) & 0xFF;
        oam->x = (SPRITE_X(sSurf->staticX[i]) + 512) & 0x1FF;
        oam->tileNum = sSurf->staticTile[i];
        oam->priority = 0;
    }
    gMain.oamBuffer[NUM_STATIC_SPRITES] = gDummyOamData;
}

static void PlaceStaticRow(u32 first, const u8 *tiles, u32 count, u8 y, u8 x)
{
    u32 i;
    for (i = 0; i < count; i++)
    {
        sSurf->staticY[first + i] = y;
        sSurf->staticX[first + i] = x + 8 * i;
        sSurf->staticTile[first + i] = tiles[i];
    }
}

static void DrawHP(void)
{
    u32 hp = sSurf->hp, i;
    for (i = 0; i < 4; i++)
    {
        sSurf->staticTile[3 - i] = 0xd0 + hp % 10;
        hp /= 10;
    }
}

// The clouds drift with Pikachu's speed (SurfingMinigame_MoveClouds)
static void MoveClouds(void)
{
    u32 sum = sSurf->cloudFraction + sSurf->speed, i;
    sSurf->cloudFraction = sum & 0xFF;
    for (i = 5; i < NUM_STATIC_SPRITES; i++)
        sSurf->staticX[i] += sum >> 8;
}

// ---------------------------------------------------------------------------------------------
// The results (SurfingMinigame_DrawResultsScreen ...)
// ---------------------------------------------------------------------------------------------
static void PutText(u32 x, u32 y, const u8 *tiles, u32 n)
{
    u32 i;
    for (i = 0; i < n; i++)
        SetCourseTile(x + i + sSurf->scx / 8, y, tiles[i]);
}

static void PutNumber(u32 x, u32 y, u32 value)
{
    u32 i;
    for (i = 0; i < 4; i++)
    {
        SetCourseTile(x + 3 - i + sSurf->scx / 8, y, 0xd0 + value % 10);
        value /= 10;
    }
}

static void PutPoints(u32 y, u32 value)
{
    static const u8 sPts[] = {0x21, 0x25, 0x26};    // "Pts"
    PutNumber(10, y, value);
    PutText(15, y, sPts, 3);
}

// Fills the GBA's extra columns either side of the Game Boy screen with its edge columns
static void ExtendScreenSides(u32 rows)
{
    u32 x, y, c = sSurf->scx / 8;
    for (y = 0; y < rows; y++)
    {
        for (x = 1; x <= 5; x++)
        {
            SetCourseTile(c + COURSE_W - x, y, sSurf->courseTiles[y][c % COURSE_W]);
            SetCourseTile(c + 19 + x, y, sSurf->courseTiles[y][(c + 19) % COURSE_W]);
        }
    }
}

static void DrawResultsScreen(void)
{
    u32 x, y, c = sSurf->scx / 8;
    for (y = 0; y < 18; y++)
        for (x = 0; x < 20; x++)
            SetCourseTile(c + x, y, 0);
    for (y = 0; y < 10; y++)
        for (x = 0; x < 20; x++)
            SetCourseTile(c + x, 6 + y, sTilemap_BeachOutro[y * 20 + x]);
    ExtendScreenSides(18);
    for (y = 1; y <= 9; y++)
    {
        u8 left = y == 1 ? 0x3b : y == 9 ? 0x3d : 0x3f, right = y == 1 ? 0x3c : y == 9 ? 0x3e : 0x3f;
        u8 fill = (y == 1 || y == 9) ? 0x40 : 0xff;
        SetCourseTile(c + 1, y, left);
        for (x = 2; x < 18; x++)
            SetCourseTile(c + x, y, fill);
        SetCourseTile(c + 18, y, right);
    }
    for (x = 5; x < NUM_STATIC_SPRITES; x++)
        sSurf->staticY[x] = 0;      // (the clouds go)
}

// ---------------------------------------------------------------------------------------------
// The routines (RunSurfingMinigameRoutine)
// ---------------------------------------------------------------------------------------------
static bool32 RunDelay(void)
{
    if (sSurf->routineDelay == 0)
        return TRUE;
    sSurf->routineDelay--;
    return FALSE;
}

static void RunRoutine(void)
{
    static const u8 sHPLeft[] = {0x20, 0x21, 0xff, 0x22, 0x23, 0x24, 0x25};
    static const u8 sRadness[] = {0x27, 0x28, 0x29, 0x2a, 0x23, 0x26, 0x26};
    static const u8 sTotal[] = {0x2b, 0x2c, 0x25, 0x28, 0x2d};
    static const u8 sHiScore[] = {0x20, 0x2e, 0x2f, 0x30, 0x31, 0x2c, 0x32, 0x23, 0x33};
    u32 i;

    switch (sSurf->routine)
    {
    case R_START:
        SpawnObject(OBJ_START, 0x48, 0xe0);
        sSurf->routine++;
        break;
    case R_RUN:
        if (sSurf->sections >= COURSE_SECTIONS)
        {
            sSurf->routine++;
            sSurf->routineDelay = 192;
            break;
        }
        if (sSurf->hp == 0)
        {
            struct AnimObj *o;
            sSurf->gameOver = TRUE;
            sSurf->routine = R_GAME_OVER;
            sSurf->gameOverDelay = 0x80;
            o = SpawnObject(OBJ_OH_NO, 0x88, CENTER_X);
            if (o)
            {
                o->yOffset = (s8)0x80;
                o->fieldB = 0x80;
                o->fieldC = 0x30;
            }
            break;
        }
        sSurf->waveRandom = Random();
        UpdateLYOverrides();
        SetPikachuHeight();
        ReadBgUnderPikachu();
        ScrollCourse(0x180);    // (1.5 pixels a frame)
        // the distance: Pikachu's speed, a section each time it carries
        {
            u32 d = sSurf->distanceFraction + sSurf->speed;
            sSurf->distanceFraction = d & 0xFFFF;
            if (d > 0xFFFF)
            {
                sSurf->sections++;
                sSurf->staticX[4] -= 2;     // (the progress marker)
            }
        }
        sSurf->hp--;
        DrawHP();
        break;
    case R_WAIT_RESULTS:
        if (RunDelay())
        {
            // on to the beach: the Game Boy jumps its (flat, by now) sea to 0x90 pixels short of
            // where the results go and scrolls back there, laying the beach in behind. Here the
            // results go at the start of the course's next lap, and the sea there is made flat first.
            u32 x;
            sSurf->routine++;
            sSurf->scx = ((sSurf->scx / (COURSE_W * 8)) + 2) * (COURSE_W * 8) + 0x90;
            for (x = (sSurf->scx - OFS_X - 32) / 8 & ~1; x < (sSurf->scx + DISPLAY_WIDTH) / 8 + 2; x += 2)
            {
                u8 fn = sSurf->waveFunction;
                sSurf->waveFunction = WAVE_BEACH;     // (its first slice is flat water)
                LayColumn(x);
                sSurf->waveFunction = fn;
            }
            sSurf->behindCol = ((sSurf->scx - OFS_X - 32) / 8) & ~1;
            sSurf->waveFunction = WAVE_BEACH;
            sSurf->pikachuState = PIKA_GAME_END;
            // (the wobble stops: hLCDCPointer = 0)
            ScanlineEffect_Stop();
            sSurf->lyEffect = FALSE;
            sSurf->lastScx = sSurf->scx;
            sSurf->scxFraction = 0;
            break;
        }
        sSurf->waveRandom = 0;
        UpdateLYOverrides();
        SetPikachuHeight();
        ReadBgUnderPikachu();
        ScrollCourse(0x900);    // (coasting in: 9 pixels a frame)
        break;
    case R_SCROLL_TO_RESULTS:
        // (the Game Boy scrolls back 0x90 pixels, drawing the beach in behind; here the beach is
        // drawn in place and the screen eases back the same way)
        UpdateLYOverrides();
        SetPikachuHeight();
        ReadBgUnderPikachu();
        if (sSurf->lastScx - sSurf->scx >= 0x90)
        {
            sSurf->speed = 0;
            sSurf->routine++;
            sSurf->pikachuState = PIKA_INIT_RESULTS;
            break;
        }
        sSurf->scx -= 4;
        // the beach, laid in just behind the left edge as the screen comes back
        while (sSurf->behindCol * 8 + 16 > sSurf->scx - OFS_X)
        {
            sSurf->behindCol -= 2;
            LayColumn(sSurf->behindCol);
        }
        break;
    case R_DRAW_RESULTS:
        DrawResultsScreen();
        sSurf->routineDelay = 32;
        sSurf->routine++;
        break;
    case R_HP_LEFT:
        if (!RunDelay())
            break;
        PutText(2, 2, sHPLeft, ARRAY_COUNT(sHPLeft));
        PutPoints(2, sSurf->hp);
        sSurf->routineDelay = 64;
        sSurf->routine++;
        break;
    case R_RADNESS:
        if (!RunDelay())
            break;
        PutText(2, 4, sRadness, ARRAY_COUNT(sRadness));
        PutPoints(4, sSurf->radness);
        sSurf->routineDelay = 64;
        sSurf->routine++;
        break;
    case R_TOTAL:
        if (!RunDelay())
            break;
        PutText(2, 6, sTotal, ARRAY_COUNT(sTotal));
        PutPoints(4, sSurf->radness);
        PutPoints(6, sSurf->total);
        sSurf->routineDelay = 64;
        sSurf->routine++;
        break;
    case R_ADD_HP:
        if (!RunDelay())
            break;
        for (i = 0; i < 99 && sSurf->hp; i++)
        {
            sSurf->hp--;
            sSurf->total = min(sSurf->total + 1, 9999);
        }
        PutPoints(2, sSurf->hp);
        PutPoints(6, sSurf->total);
        if (sSurf->hp == 0)
        {
            sSurf->routineDelay = 64;
            sSurf->routine++;
        }
        else
        {
            PlaySE(SE_SELECT);
        }
        break;
    case R_ADD_RADNESS:
        if (!RunDelay())
            break;
        for (i = 0; i < 99 && sSurf->radness; i++)
        {
            sSurf->radness--;
            sSurf->total = min(sSurf->total + 1, 9999);
        }
        PutPoints(4, sSurf->radness);
        PutPoints(6, sSurf->total);
        if (sSurf->radness)
        {
            PlaySE(SE_SELECT);
            break;
        }
        sSurf->routineDelay = 128;
        sSurf->routine++;
        if (sSurf->total > VarGet(VAR_SURFING_HIGH_SCORE))
        {
            VarSet(VAR_SURFING_HIGH_SCORE, sSurf->total);
            sSurf->newHighScore = TRUE;
            PutText(6, 8, sHiScore, ARRAY_COUNT(sHiScore));
            sSurf->pikachuState = PIKA_RESULTS;
            PlaySE(SE_SUCCESS);
        }
        if (SurfingPikachu_InParty())
            PlayCry_Normal(SPECIES_PIKACHU, 0);
        break;
    case R_WAIT_LAST:
        if (RunDelay())
            sSurf->routine++;
        break;
    case R_EXIT_ON_A:
        UpdateLYOverrides();
        if (JOY_NEW(A_BUTTON | B_BUTTON))
            sSurf->routine = R_DONE;
        break;
    case R_GAME_OVER:
        UpdateLYOverrides();
        SetPikachuHeight();
        ReadBgUnderPikachu();
        ScrollCourse(0x180);
        if (sSurf->gameOverDelay)
            sSurf->gameOverDelay--;
        else if (JOY_NEW(A_BUTTON | B_BUTTON))
            sSurf->routine = R_DONE;
        break;
    }
}

// ---------------------------------------------------------------------------------------------
// The GBA's side
// ---------------------------------------------------------------------------------------------
static const struct BgTemplate sBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 0, .mapBaseIndex = COURSE_MAP_BLOCK, .screenSize = 1, .paletteMode = 0, .priority = 2, .baseTile = 0},
    {.bg = 1, .charBaseIndex = 0, .mapBaseIndex = WINDOW_MAP_BLOCK, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0},
};

static void VBlankCB(void)
{
    LoadOam();
    TransferPlttBuffer();
    SetGpuReg(REG_OFFSET_BG0HOFS, (sSurf->scx - OFS_X) & 0x1FF);
    if (sSurf->lyEffect)
        ScanlineEffect_InitHBlankDmaTransfer();
    else
        SetGpuReg(REG_OFFSET_BG0VOFS, (u16)-OFS_Y & 0x1FF);
}

// The wobble, for the next frame: each GBA line shows its Game Boy line's offset
static void BuildScanlines(void)
{
    u16 *buf = gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer];
    u32 y;
    for (y = 0; y < DISPLAY_HEIGHT; y++)
    {
        s32 gb = (s32)y - OFS_Y;
        s32 ofs = (gb >= 0 && gb < GB_H) ? (s8)sSurf->lyOverrides[gb] : 0;
        buf[y] = (u16)(ofs - OFS_Y) & 0x1FF;
    }
}

static void SetupDisplay(void)
{
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    ScanlineEffect_Stop();
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    ResetAllBgsCoordinates();
}

static void StartDisplay(MainCallback cb, bool32 window)
{
    SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(2) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(COURSE_MAP_BLOCK) | BGCNT_16COLOR | BGCNT_TXT512x256);
    SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(1) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(WINDOW_MAP_BLOCK) | BGCNT_16COLOR | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON | (window ? DISPCNT_BG1_ON : 0));
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK | INTR_FLAG_HBLANK);
    SetVBlankCallback(VBlankCB);
    SetMainCallback2(cb);
}

static void LoadPalettes(void)
{
    LoadPalette(sPalBeach, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
    LoadPalette(sPalTitle, BG_PLTT_ID(1), PLTT_SIZE_4BPP);
    LoadPalette(sPalObj0, OBJ_PLTT_ID(0), PLTT_SIZE_4BPP);
    LoadPalette(sPalObj1, OBJ_PLTT_ID(1), PLTT_SIZE_4BPP);
}

// The title: Pikachu rides off to the right (SurfingPikachuMinigameIntro)
static void Intro_Init(void)
{
    u32 x, y;
    u8 *bgChars = (u8 *)BG_CHAR_ADDR(0);
    SetupDisplay();
    LoadTilesTo(bgChars, sBgTiles, 0, NUM_BG_TILES);
    LoadTilesTo(bgChars + 0x80 * TILE_BYTES, sIntroTiles, 0, 128);
    LoadTilesTo((u8 *)OBJ_VRAM0 + 0x80 * TILE_BYTES, sIntroTiles, 0, NUM_INTRO_TILES);
    LoadPalettes();
    memset(sSurf->courseTiles, 0xff, sizeof(sSurf->courseTiles));
    for (y = 0; y < 18; y++)
        for (x = 0; x < 20; x++)
            SetCourseTile(x, y, y >= 6 ? sTilemap_BeachIntro[(y - 6) * 20 + x] : 0xff);
    for (y = 0; y < 6; y++)
        for (x = 0; x < 12; x++)
            SetCourseTile(4 + x, y, sTilemap_Title[y * 12 + x]);
    for (y = 7; y < 10; y++)
        for (x = 3; x < 18; x++)
            SetCourseTile(x, y, 0xff);
    PutText(3, 7, sTilemap_UseControlPad, sizeof(sTilemap_UseControlPad));
    PutText(4, 9, sTilemap_ToSurfRad, sizeof(sTilemap_ToSurfRad));
    ExtendScreenSides(18);
    for (x = 0; x < COURSE_W; x++)
    {
        SetCourseTile(x, COURSE_H - 1, 0xff);                          // (the line above the Game Boy's screen)
        SetCourseTile(x, 18, sSurf->courseTiles[17][x]);              // (and below)
    }
    // (ids 0x00-0x0f are the intro's last 16 tiles: the Game Boy's signed tile addressing wraps
    // round to them. Here they're at 0x100.)
    LoadTilesTo(bgChars + 0x100 * TILE_BYTES, sIntroTiles, 128, NUM_INTRO_TILES - 128);
    for (y = 0; y < COURSE_H; y++)
        for (x = 0; x < COURSE_W; x++)
            if (sSurf->courseTiles[y][x] < 0x10)
                BG0_MAP[(x >= 32 ? 0x400 : 0) + y * 32 + (x % 32)] = 0x100 + sSurf->courseTiles[y][x];
    // the title's colours (its box on the Game Boy: tiles 4-15, rows 0-5)
    for (y = 0; y < 6; y++)
        for (x = 4; x < 16; x++)
            BG0_MAP[y * 32 + x] |= 1 << 12;
    sSurf->scx = 0;
    sSurf->lyEffect = FALSE;
    sSurf->introDone = FALSE;
    SpawnObject(OBJ_INTRO_PIKACHU, FLAT_WATER_Y, CENTER_X);
    PlayBGM(MUS_RG_SURF);
    StartDisplay(CB2_Intro, FALSE);
}

static void CB2_Intro(void)
{
    RunObjects();
    UpdatePaletteFade();
    if (sSurf->introDone && !gPaletteFade.active)
    {
        Game_Init();
        return;
    }
    if (JOY_NEW(START_BUTTON | A_BUTTON))
        sSurf->introDone = TRUE;
}

// The course (SurfingPikachuMinigame_LoadGFXAndLayout)
static void Game_Init(void)
{
    static const u8 sHPDigits[4] = {0xd0, 0xd0, 0xd0, 0xd0};
    static const u8 sMarker[1] = {0xfe};
    static const u8 sWideCloud[5] = {0xec, 0xed, 0xed, 0xee, 0xef};
    static const u8 sNarrowCloud[4] = {0xec, 0xed, 0xee, 0xef};
    static const u8 sStatusBar[9] = {0x17, 0x18, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19};
    u8 *bgChars = (u8 *)BG_CHAR_ADDR(0);
    u32 i, x, y;
    struct ScanlineEffectParams params;

    SetupDisplay();
    for (i = 0; i < MAX_OBJECTS; i++)
        sSurf->objects[i].active = FALSE;
    LoadTilesTo(bgChars, sBgTiles, 0, NUM_BG_TILES);
    LoadTilesTo(bgChars + 0x80 * TILE_BYTES, sObjTiles, 0x80, 0x80);
    LoadTilesTo((u8 *)OBJ_VRAM0, sObjTiles, 0, 0x100);
    // the status bar's tiles: white made solid (colour 4), so the sea doesn't show through
    LoadTilesTo(bgChars + WINDOW_TILES * TILE_BYTES, sBgTiles, 0, NUM_BG_TILES);
    for (i = 0; i < NUM_BG_TILES * TILE_BYTES; i++)
    {
        u8 *b = bgChars + WINDOW_TILES * TILE_BYTES + i;
        if ((*b & 0x0F) == 0) *b |= 0x04;
        if ((*b & 0xF0) == 0) *b |= 0x40;
    }
    LoadPalettes();

    // the sky, then the sea (rows 6-17), and the status bar
    for (y = 0; y < COURSE_H; y++)
        for (x = 0; x < COURSE_W; x++)
            SetCourseTile(x, y, (y >= 6 && y < 18) ? 0x0b : 0);
    // (the status bar: its rows down to below the screen are solid; the rest of BG1 is see-through)
    for (i = 0; i < 32 * 32; i++)
        BG1_MAP[i] = (i < 32 * 4) ? WINDOW_TILES : 0;
    for (x = 0; x < 9; x++)
        BG1_MAP[32 + 1 + 5 + x] = WINDOW_TILES + sStatusBar[x];
    BG1_MAP[1 + 5] = WINDOW_TILES + 0x15;
    BG1_MAP[2 + 5] = WINDOW_TILES + 0x16;
    BG1_MAP[32 + 12 + 5] = WINDOW_TILES + 0x1b;
    BG1_MAP[32 + 13 + 5] = WINDOW_TILES + 0x1c;
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, (u16)-(0x7e - 0 + OFS_Y) & 0x1FF);    // (the window's at line 0x7e)

    sSurf->scx = 0;
    sSurf->scxFraction = 0;
    sSurf->generatedTo = 0;
    sSurf->waveFunction = 0;
    sSurf->sections = 0;
    sSurf->distanceFraction = 0;
    sSurf->routine = R_START;
    sSurf->speed = 0x40;
    sSurf->hp = 6000;
    sSurf->radness = sSurf->total = 0;
    sSurf->gameOver = FALSE;
    sSurf->pikachuState = PIKA_RIDING;
    sSurf->pikachuHeight = FLAT_WATER_Y;
    for (i = 0; i < COURSE_W; i++)
        sSurf->waveHeight[i] = FLAT_WATER_Y;
    // (the course to the screen's right edge, all flat to begin with)
    for (x = 0; x < (DISPLAY_WIDTH - OFS_X + 16) / 8; x += 2)
    {
        LayColumn(x);
        sSurf->waveFunction = 0;
    }
    sSurf->generatedTo = x;
    // the GBA's extra sea to the left
    for (y = 0; y < COURSE_H; y++)
        for (x = COURSE_W - 6; x < COURSE_W; x++)
            SetCourseTile(x, y, sSurf->courseTiles[y][0]);
    for (i = 0; i < GB_H; i++)
        sSurf->lyOverrides[i] = sLYSine[i & 31];

    memset(sSurf->staticY, 0, sizeof(sSurf->staticY));
    PlaceStaticRow(0, sHPDigits, 4, 0x97, 0x80);
    PlaceStaticRow(4, sMarker, 1, 0x96, 0x50);
    PlaceStaticRow(5, sWideCloud, 5, 0x14, 0x20);
    PlaceStaticRow(10, sNarrowCloud, 4, 0x20, 0x80);
    DrawHP();
    sSurf->pikachu = SpawnObject(OBJ_PIKACHU, FLAT_WATER_Y, CENTER_X);

    ScanlineEffect_Clear();
    for (i = 0; i < 2; i++)
    {
        for (y = 0; y < DISPLAY_HEIGHT; y++)
            gScanlineEffectRegBuffers[i][y] = (u16)-OFS_Y & 0x1FF;
    }
    params.dmaDest = &REG_BG0VOFS;
    params.dmaControl = SCANLINE_EFFECT_DMACNT_16BIT;
    params.initState = 1;
    params.unused9 = 0;
    ScanlineEffect_SetParams(params);
    sSurf->lyEffect = TRUE;
    sSurf->joyTimer = 0;
    StartDisplay(CB2_Game, TRUE);
}

static void CB2_Game(void)
{
    // the held buttons, sampled every third frame (SurfingPikachu_GetJoypad_3FrameBuffer)
    if (sSurf->joyTimer == 0)
    {
        sSurf->joy = gMain.heldKeys;
        sSurf->joyTimer = 2;
    }
    else
    {
        sSurf->joy = 0;
        sSurf->joyTimer--;
    }
    if (JOY_NEW(SELECT_BUTTON))
        sSurf->routine = R_DONE;
    if (sSurf->routine == R_DONE)
    {
        if (!gPaletteFade.active)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            SetMainCallback2(CB2_Exit);
        }
        return;
    }
    RunRoutine();
    RunObjects();
    DrawStaticSprites();
    MoveClouds();
    BuildScanlines();
    UpdatePaletteFade();
    sSurf->bgRead[0] = sSurf->bgPending[0];
    sSurf->bgRead[1] = sSurf->bgPending[1];
}

static void CB2_Exit(void)
{
    UpdatePaletteFade();
    if (gPaletteFade.active)
        return;
    SetVBlankCallback(NULL);
    ScanlineEffect_Stop();
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    FREE_AND_SET_NULL(sSurf);
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

// Special (the beach house; the script fades to black first): Pikachu's Beach, then back
void SurfingMinigame_Start(void)
{
    sSurf = AllocZeroed(sizeof(*sSurf));
    SetMainCallback2(Intro_Init);
}
