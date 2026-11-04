// Pokémon Canvas: the opening (after the copyright and Game Freak screens), in place of FireRed's
// Nidorino and Gengar. The player sends out Raichu against a gym trainer's silhouette; the eight
// Kanto badges flash by; the player walks up to their own gym in Pallet Town; a paintbrush sweeps a
// rainbow across the screen, and on to the title screen. Timed to FireRed's intro music.
//
// Graphics: tools/gym_hack/intro_canvas_gfx.py -> graphics/intro_canvas/ (the backgrounds, the badges
// in colour, the brush); the player, Raichu, the trainer and the Poké Ball are the game's own.
// It runs on its own (its own callbacks and OAM, like the Pikachu's Beach minigame), and A, START
// or SELECT skip to the title screen, as before.
#include "global.h"
#include "gpu_regs.h"
#include "intro_canvas.h"
#include "m4a.h"
#include "main.h"
#include "malloc.h"
#include "palette.h"
#include "pokemon.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "title_screen.h"
#include "trig.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/species.h"

// ---------------------------------------------------------------------------------------------
// Graphics
// ---------------------------------------------------------------------------------------------
static const u32 sArena_Gfx[] = INCGFX_U32("graphics/intro_canvas/arena.png", ".4bpp");
static const u16 sArena_Map[] = INCBIN_U16("graphics/intro_canvas/arena.bin");
static const u16 sArena_Pal[] = INCGFX_U16("graphics/intro_canvas/arena.png", ".gbapal");
static const u32 sRays_Gfx[] = INCGFX_U32("graphics/intro_canvas/rays.png", ".4bpp");
static const u16 sRays_Map[] = INCBIN_U16("graphics/intro_canvas/rays.bin");
static const u32 sRainbow_Gfx[] = INCGFX_U32("graphics/intro_canvas/rainbow.png", ".4bpp");
static const u16 sRainbow_Map[] = INCBIN_U16("graphics/intro_canvas/rainbow.bin");
static const u16 sRainbow_Pal[] = INCGFX_U16("graphics/intro_canvas/rainbow.png", ".gbapal");
static const u32 sGym_Gfx[] = INCGFX_U32("graphics/intro_canvas/gym.png", ".8bpp");
static const u16 sGym_Map[] = INCBIN_U16("graphics/intro_canvas/gym.bin");
static const u16 sGym_Pal[] = INCGFX_U16("graphics/intro_canvas/gym.png", ".gbapal");
static const u32 sBadges_Gfx[] = INCGFX_U32("graphics/intro_canvas/badges.png", ".4bpp");
static const u32 sBadgesSmall_Gfx[] = INCGFX_U32("graphics/intro_canvas/badges_small.png", ".4bpp");
static const u16 sBadge0_Pal[] = INCGFX_U16("graphics/intro_canvas/badge_0.pal", ".gbapal");
static const u16 sBadge1_Pal[] = INCGFX_U16("graphics/intro_canvas/badge_1.pal", ".gbapal");
static const u16 sBadge2_Pal[] = INCGFX_U16("graphics/intro_canvas/badge_2.pal", ".gbapal");
static const u16 sBadge3_Pal[] = INCGFX_U16("graphics/intro_canvas/badge_3.pal", ".gbapal");
static const u16 sBadge4_Pal[] = INCGFX_U16("graphics/intro_canvas/badge_4.pal", ".gbapal");
static const u16 sBadge5_Pal[] = INCGFX_U16("graphics/intro_canvas/badge_5.pal", ".gbapal");
static const u16 sBadge6_Pal[] = INCGFX_U16("graphics/intro_canvas/badge_6.pal", ".gbapal");
static const u16 sBadge7_Pal[] = INCGFX_U16("graphics/intro_canvas/badge_7.pal", ".gbapal");
static const u16 *const sBadge_Pals[8] = {sBadge0_Pal, sBadge1_Pal, sBadge2_Pal, sBadge3_Pal, sBadge4_Pal, sBadge5_Pal, sBadge6_Pal, sBadge7_Pal};
static const u32 sBolt_Gfx[] = INCGFX_U32("graphics/intro_canvas/bolt.png", ".4bpp");
static const u16 sBolt_Pal[] = INCGFX_U16("graphics/intro_canvas/bolt.png", ".gbapal");
static const u32 sBrush_Gfx[] = INCGFX_U32("graphics/intro_canvas/brush.png", ".4bpp");
static const u16 sBrush_Pal[] = INCGFX_U16("graphics/intro_canvas/brush.png", ".gbapal");

// (the game's own)
static const u32 sRedBack_Gfx[] = INCGFX_U32("graphics/trainers/back_pics/red.png", ".4bpp");
static const u16 sRedBack_Pal[] = INCGFX_U16("graphics/trainers/back_pics/red.png", ".gbapal");
static const u32 sLeafBack_Gfx[] = INCGFX_U32("graphics/trainers/back_pics/leaf.png", ".4bpp");
static const u16 sLeafBack_Pal[] = INCGFX_U16("graphics/trainers/back_pics/leaf.png", ".gbapal");
static const u32 sRaichuBack_Gfx[] = INCGFX_U32("graphics/pokemon/raichu/back.png", ".4bpp");
static const u16 sRaichu_Pal[] = INCGFX_U16("graphics/pokemon/raichu/normal.pal", ".gbapal");
static const u32 sTrainer_Gfx[] = INCGFX_U32("graphics/trainers/front_pics/cool_trainer_m_frlg.png", ".4bpp");
static const u32 sBall_Gfx[] = INCGFX_U32("graphics/balls/poke.png", ".4bpp");
static const u16 sBall_Pal[] = INCGFX_U16("graphics/balls/poke.png", ".gbapal");
static const u32 sRedWalk_Gfx[] = INCGFX_U32("graphics/object_events/pics/people/red/red_normal.png", ".4bpp", "-mwidth 2 -mheight 4");
static const u32 sLeafWalk_Gfx[] = INCGFX_U32("graphics/object_events/pics/people/leaf/green_normal.png", ".4bpp", "-mwidth 2 -mheight 4");
static const u16 sWalk_Pal[] = INCGFX_U16("graphics/object_events/palettes/player_frlg.pal", ".gbapal");

#define BG_SCENE    1       // the arena, the rays, the gym (char block 0, map 30)
#define BG_RAINBOW  2       // (char block 2, map 31)
#define SCENE_MAP   30
#define RAINBOW_MAP 31
#define BG_MAP(n) ((u16 *)BG_SCREEN_ADDR(n))

// OBJ VRAM (tiles) and palettes
#define TILE_BACK   0       // the player's back pic: 5 frames of 64
#define TILE_RAICHU 320
#define TILE_TRAINER 384
#define TILE_BALL   448     // 3 frames of 4
#define TILE_BOLT   460
#define TILE_BADGES 0       // (the badges' scene: 8 of 64)
#define TILE_BADGES_SMALL 512   // (and 8 of 16)
#define TILE_WALK   0       // (the gym's: the player walking, 9 frames of 8)
#define TILE_BRUSH  128
enum { P_BACK, P_RAICHU, P_TRAINER, P_BALL, P_WALK, P_BRUSH, P_BOLT, P_BADGE0 };

// ---------------------------------------------------------------------------------------------
// The timeline (frames from the music's start)
// ---------------------------------------------------------------------------------------------
#define T_THROW         40      // the player's throw (back pic frames 1-4, 6 frames each)
#define T_BALL_OUT      58
#define T_BALL_OPEN     78
#define T_ZAP1          112
#define T_ZAP2          124
#define T_ZAP3          136
#define T_KO            176     // the white-out to the badges
#define T_BADGES        192
#define BADGE_TIME      26
#define T_LINEUP        (T_BADGES + 8 * BADGE_TIME)
#define T_GYM           470
#define T_WALK_END      (T_GYM + 80)
#define T_ENTER         566
#define T_BRUSH_START   584
#define BRUSH_TIME      90
#define T_RAINBOW_FULL  (T_BRUSH_START + BRUSH_TIME)
#define T_FADE_WHITE    690
#define T_END           708

enum { SCENE_NONE, SCENE_BATTLE, SCENE_BADGES, SCENE_GYM };

struct IntroCanvas
{
    u16 t;
    u8 scene;
    u8 gender;
    s16 brightness;     // (BLDY: + lightens, - darkens)
    s16 trainerX, playerX, raichuX;
    s16 walkY;
    u16 shownBadges;
    bool8 windowOn;
    s16 windowRight;
    u8 oamCount;
};

static EWRAM_DATA struct IntroCanvas *sCanvas = NULL;

static void CB2_Canvas(void);
static void VBlankCB_Canvas(void);
static void CanvasExit(void);

// ---------------------------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------------------------
static void LoadBg(const u32 *gfx, u32 gfxSize, const u16 *map, u32 charBlock, u32 mapBlock, u32 palBits)
{
    u32 i;
    u16 *dest = BG_MAP(mapBlock);
    CpuCopy32(gfx, (void *)BG_CHAR_ADDR(charBlock), gfxSize);
    for (i = 0; i < 32 * 32; i++)
        dest[i] = map[i] | palBits;
}

static void LoadObjTiles(const u32 *gfx, u32 firstTile, u32 numTiles)
{
    CpuCopy32(gfx, (void *)(OBJ_VRAM0 + firstTile * TILE_SIZE_4BPP), numTiles * TILE_SIZE_4BPP);
}

static void BeginOam(void)
{
    sCanvas->oamCount = 0;
}

// A sprite: (x, y) its top-left corner, or its centre if `scale` (8.8; 256 = 1x) isn't 0 (affine,
// double size)
static void PutSprite(s32 x, s32 y, u32 shape, u32 size, u32 tile, u32 pal, u32 scale, bool32 hflip)
{
    struct OamData *oam;
    u32 n = sCanvas->oamCount;
    if (n >= 32)
        return;
    oam = &gMain.oamBuffer[n];
    {
        // (an entry's fourth halfword belongs to matrix n / 4: keep it)
        u16 param = oam->affineParam;
        *oam = (struct OamData){0};
        oam->affineParam = param;
    }
    oam->shape = shape;
    oam->size = size;
    oam->tileNum = tile;
    oam->paletteNum = pal;
    oam->priority = 1;
    if (scale)
    {
        // (the affine matrices sit in the OAM entries' fourth halfwords: matrix n uses entries 4n-4n+3)
        s16 inv = (s16)(0x10000 / scale);
        u32 w = (shape == ST_OAM_SQUARE) ? (8 << size) : (shape == ST_OAM_H_RECTANGLE ? 16 << size : 8 << size);
        u32 h = w;
        oam->affineMode = ST_OAM_AFFINE_DOUBLE;
        oam->matrixNum = n;
        gMain.oamBuffer[4 * n + 0].affineParam = hflip ? -inv : inv;
        gMain.oamBuffer[4 * n + 1].affineParam = 0;
        gMain.oamBuffer[4 * n + 2].affineParam = 0;
        gMain.oamBuffer[4 * n + 3].affineParam = inv;
        x -= w;
        y -= h;
    }
    else if (hflip)
    {
        oam->matrixNum = ST_OAM_HFLIP;
    }
    oam->x = x & 0x1FF;
    oam->y = y & 0xFF;
    if (x <= -128 || x >= DISPLAY_WIDTH || y <= -128 || y >= DISPLAY_HEIGHT)
        oam->affineMode = ST_OAM_AFFINE_ERASE;
    sCanvas->oamCount++;
}

static void EndOam(void)
{
    u32 i;
    // (the rest hidden; their fourth halfwords keep the matrices)
    for (i = sCanvas->oamCount; i < 128; i++)
    {
        u16 param = gMain.oamBuffer[i].affineParam;
        gMain.oamBuffer[i] = (struct OamData){.affineMode = ST_OAM_AFFINE_ERASE};
        gMain.oamBuffer[i].affineParam = param;
    }
}

// The flash and fades (BLDY, on everything)
static void SetBrightness(s32 b)
{
    sCanvas->brightness = b;
}

static s32 Approach(s32 from, s32 to, s32 t, s32 duration)
{
    if (t <= 0)
        return from;
    if (t >= duration)
        return to;
    return from + (to - from) * t / duration;
}

// ---------------------------------------------------------------------------------------------
// The scenes
// ---------------------------------------------------------------------------------------------
static void StartBattle(void)
{
    u16 black[16];
    u32 i;
    LoadBg(sArena_Gfx, sizeof(sArena_Gfx), sArena_Map, 0, SCENE_MAP, 0);
    LoadPalette(sArena_Pal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
    LoadObjTiles(sCanvas->gender == FEMALE ? sLeafBack_Gfx : sRedBack_Gfx, TILE_BACK, 5 * 64);
    LoadPalette(sCanvas->gender == FEMALE ? sLeafBack_Pal : sRedBack_Pal, OBJ_PLTT_ID(P_BACK), PLTT_SIZE_4BPP);
    LoadObjTiles(sRaichuBack_Gfx, TILE_RAICHU, 64);
    LoadPalette(sRaichu_Pal, OBJ_PLTT_ID(P_RAICHU), PLTT_SIZE_4BPP);
    LoadObjTiles(sTrainer_Gfx, TILE_TRAINER, 64);
    for (i = 0; i < 16; i++)
        black[i] = RGB(1, 1, 3);    // (the silhouette)
    LoadPalette(black, OBJ_PLTT_ID(P_TRAINER), PLTT_SIZE_4BPP);
    LoadObjTiles(sBall_Gfx, TILE_BALL, 12);
    LoadPalette(sBall_Pal, OBJ_PLTT_ID(P_BALL), PLTT_SIZE_4BPP);
    LoadObjTiles(sBolt_Gfx, TILE_BOLT, 64);
    LoadPalette(sBolt_Pal, OBJ_PLTT_ID(P_BOLT), PLTT_SIZE_4BPP);
    sCanvas->scene = SCENE_BATTLE;
}

static void DrawBattle(u32 t)
{
    s32 trainerX, playerX, frame, shake = 0;
    static const u16 sZaps[3] = {T_ZAP1, T_ZAP2, T_ZAP3};
    u32 i;
    // Raichu's Thunderbolts: a bolt flickering across to the trainer
    for (i = 0; i < 3; i++)
        if (t >= sZaps[i] && t < sZaps[i] + 8 && !(t & 2))
            PutSprite(126, 84, ST_OAM_SQUARE, ST_OAM_SIZE_3, TILE_BOLT, P_BOLT, 512, FALSE);
    // the trainer slides in from the right, the player from the left
    trainerX = Approach(DISPLAY_WIDTH + 40, 176, t, 30);
    if (t >= 140 && t < T_KO)
        shake = ((t / 2) & 1) ? 3 : -3;
    PutSprite(trainerX - 32 + shake, 94 - 60, ST_OAM_SQUARE, ST_OAM_SIZE_3, TILE_TRAINER, P_TRAINER, 0, FALSE);

    playerX = Approach(-40, 64, t, 30);
    if (t >= 66)
        playerX = Approach(64, -48, t - 66, 16);
    frame = 0;
    if (t >= T_THROW)
        frame = 1 + min((t - T_THROW) / 6, 3);
    if (playerX > -40)
        PutSprite(playerX - 32, DISPLAY_HEIGHT - 64, ST_OAM_SQUARE, ST_OAM_SIZE_3, TILE_BACK + frame * 64, P_BACK, 0, FALSE);

    // the ball: up and over, then it opens
    if (t >= T_BALL_OUT && t < T_BALL_OPEN)
    {
        s32 k = t - T_BALL_OUT, n = T_BALL_OPEN - T_BALL_OUT;
        s32 x = 84 + (76 - 84) * k / n;
        s32 y = 100 + (116 - 100) * k / n - (40 * k * (n - k)) / (n * n / 4);
        PutSprite(x - 8, y - 8, ST_OAM_SQUARE, ST_OAM_SIZE_1, TILE_BALL + 4 * ((k / 4) % 3), P_BALL, 0, FALSE);
    }
    // Raichu: out of the ball, growing; then a lunge each time it zaps
    if (t >= T_BALL_OPEN)
    {
        s32 k = t - T_BALL_OPEN;
        u32 scale = Approach(48, 256, k, 10);
        s32 x = 72, y = 124;
        if ((t >= T_ZAP1 - 6 && t < T_ZAP1) || (t >= T_ZAP2 - 6 && t < T_ZAP2) || (t >= T_ZAP3 - 6 && t < T_ZAP3))
            x += 6, y -= 4;
        else if (t > T_BALL_OPEN + 12)
            y += ((t / 10) & 1);
        PutSprite(x, y, ST_OAM_SQUARE, ST_OAM_SIZE_3, TILE_RAICHU, P_RAICHU, scale, FALSE);
    }

    // the flashes: the ball opening, each zap, the knockout
    if (t >= T_BALL_OPEN && t < T_BALL_OPEN + 10)
        SetBrightness(12 - (t - T_BALL_OPEN) * 12 / 10);
    else if ((t >= T_ZAP1 && t < T_ZAP1 + 3) || (t >= T_ZAP2 && t < T_ZAP2 + 3) || (t >= T_ZAP3 && t < T_ZAP3 + 3))
        SetBrightness(9);
    else if (t >= T_KO)
        SetBrightness(Approach(0, 16, t - T_KO, 8));
    else if (t < 16)
        SetBrightness(-16 + t);
    else
        SetBrightness(0);
}

static void ShowBadgeRays(u32 badge)
{
    u16 pal[4];
    const u16 *badgePal = sBadge_Pals[badge];
    pal[0] = 0;
    pal[1] = badgePal[2];   // (its darkest, then lighter)
    pal[2] = badgePal[3];
    pal[3] = badgePal[4];
    LoadPalette(pal, BG_PLTT_ID(0), sizeof(pal));
}

static void StartBadges(void)
{
    u32 i;
    LoadBg(sRays_Gfx, sizeof(sRays_Gfx), sRays_Map, 0, SCENE_MAP, 0);
    LoadObjTiles(sBadges_Gfx, TILE_BADGES, 8 * 64);
    LoadObjTiles(sBadgesSmall_Gfx, TILE_BADGES_SMALL, 8 * 16);
    for (i = 0; i < 8; i++)
        LoadPalette(sBadge_Pals[i], OBJ_PLTT_ID(P_BADGE0 + i), PLTT_SIZE_4BPP);
    ShowBadgeRays(0);
    sCanvas->scene = SCENE_BADGES;
}

static void DrawBadges(u32 t)
{
    if (t < T_LINEUP)
    {
        // one at a time: a flash, the badge swooping in from big
        u32 badge = (t - T_BADGES) / BADGE_TIME, k = (t - T_BADGES) % BADGE_TIME;
        if (k == 0)
            ShowBadgeRays(badge);
        PutSprite(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, ST_OAM_SQUARE, ST_OAM_SIZE_3, TILE_BADGES + badge * 64,
                  P_BADGE0 + badge, Approach(512, 256, k, 7), FALSE);
        SetBrightness(Approach(badge == 0 ? 16 : 10, 0, k, 6));
    }
    else
    {
        // all eight in a row, one after another
        u32 k = t - T_LINEUP, i, shown = min(8, k / 3 + 1);
        if (k == 0)
            ShowBadgeRays(5);   // (gold)
        for (i = 0; i < shown; i++)
        {
            // (32x32, not scaled: eight affine sprites that wide would be too many for a line)
            s32 drop = Approach(-12, 0, k - i * 3, 4);
            PutSprite(22 + 28 * i - 16, DISPLAY_HEIGHT / 2 - 16 + drop, ST_OAM_SQUARE, ST_OAM_SIZE_2,
                      TILE_BADGES_SMALL + i * 16, P_BADGE0 + i, 0, FALSE);
        }
        if (k < 30)
            SetBrightness(Approach(10, 0, k % 3 == 0 ? 0 : k % 3, 3) * (k < 24));
        else if (t >= T_GYM - 6)
            SetBrightness(Approach(0, 16, t - (T_GYM - 6), 6));
        else
            SetBrightness(0);
    }
}

static void StartGym(void)
{
    // (the gym: 8bpp, its colours from 16 up; the rainbow's BG palette 0 under it)
    SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(2) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(SCENE_MAP) | BGCNT_256COLOR | BGCNT_TXT256x256);
    LoadBg(sGym_Gfx, sizeof(sGym_Gfx), sGym_Map, 0, SCENE_MAP, 0);
    LoadPalette(sGym_Pal + 16, BG_PLTT_ID(1), PLTT_SIZE_8BPP - PLTT_SIZE_4BPP);
    LoadBg(sRainbow_Gfx, sizeof(sRainbow_Gfx), sRainbow_Map, 2, RAINBOW_MAP, 0);
    LoadPalette(sRainbow_Pal, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
    LoadObjTiles(sCanvas->gender == FEMALE ? sLeafWalk_Gfx : sRedWalk_Gfx, TILE_WALK, 9 * 8);
    LoadPalette(sWalk_Pal, OBJ_PLTT_ID(P_WALK), PLTT_SIZE_4BPP);
    LoadObjTiles(sBrush_Gfx, TILE_BRUSH, 64);
    LoadPalette(sBrush_Pal, OBJ_PLTT_ID(P_BRUSH), PLTT_SIZE_4BPP);
    sCanvas->scene = SCENE_GYM;
}

static void DrawGym(u32 t)
{
    // the player walks up the plateau to the gym's door (16 pixels a step), and goes in
    static const u8 sWalkUp[4] = {5, 1, 6, 1};
    if (t < T_ENTER)
    {
        s32 y = Approach(DISPLAY_HEIGHT, 80, t - T_GYM, T_WALK_END - T_GYM);
        u32 frame = t < T_WALK_END ? sWalkUp[((t - T_GYM) / 8) % 4] : 1;
        PutSprite(112, y, ST_OAM_V_RECTANGLE, ST_OAM_SIZE_2, TILE_WALK + frame * 8, P_WALK, 0, FALSE);
    }
    // the brush: across the screen at double size, the rainbow behind it
    if (t >= T_BRUSH_START && t < T_RAINBOW_FULL + 30)
    {
        s32 x = Approach(-70, DISPLAY_WIDTH + 140, t - T_BRUSH_START, BRUSH_TIME + 20);
        PutSprite(x, DISPLAY_HEIGHT / 2, ST_OAM_SQUARE, ST_OAM_SIZE_3, TILE_BRUSH, P_BRUSH, 512, FALSE);
        sCanvas->windowOn = TRUE;
        sCanvas->windowRight = max(0, min(DISPLAY_WIDTH, x - 46));
    }
    if (t >= T_RAINBOW_FULL)
        sCanvas->windowRight = DISPLAY_WIDTH;
    if (t < T_GYM + 8)
        SetBrightness(Approach(16, 0, t - T_GYM, 8));
    else if (t >= T_FADE_WHITE)
        SetBrightness(Approach(0, 16, t - T_FADE_WHITE, 14));
    else
        SetBrightness(0);
}

// ---------------------------------------------------------------------------------------------
// Running it
// ---------------------------------------------------------------------------------------------
static void CB2_Canvas(void)
{
    u32 t = sCanvas->t;
    if (JOY_NEW(A_BUTTON | START_BUTTON | SELECT_BUTTON) || t >= T_END)
    {
        CanvasExit();
        return;
    }
    if (t == 0)
        m4aSongNumStart(MUS_RG_INTRO_FIGHT);
    if (t == T_BALL_OPEN + 2)
        PlayCry_Normal(SPECIES_RAICHU, 0);

    if (t == T_BADGES)
        StartBadges();
    else if (t == T_GYM)
        StartGym();

    BeginOam();
    switch (sCanvas->scene)
    {
    case SCENE_BATTLE: DrawBattle(t); break;
    case SCENE_BADGES: DrawBadges(t); break;
    case SCENE_GYM:    DrawGym(t); break;
    }
    EndOam();
    sCanvas->t++;
}

static void VBlankCB_Canvas(void)
{
    s32 b;
    LoadOam();
    TransferPlttBuffer();
    if (sCanvas == NULL)
        return;
    b = sCanvas->brightness;
    if (b > 0)
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_ALL | BLDCNT_EFFECT_LIGHTEN);
    else if (b < 0)
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_ALL | BLDCNT_EFFECT_DARKEN);
    else
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDY, min(16, b < 0 ? -b : b));
    if (sCanvas->windowOn)
    {
        // (the rainbow, BG2, only left of the brush; the gym, BG1, right of it)
        SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, sCanvas->windowRight));
        SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(0, DISPLAY_HEIGHT));
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG2 | WININ_WIN0_OBJ | WININ_WIN0_CLR);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG1 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_BG1_ON | DISPCNT_BG2_ON | DISPCNT_WIN0_ON);
    }
}

void CB2_IntroCanvas(void)
{
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    ScanlineEffect_Stop();
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ResetPaletteFade();
    ResetSpriteData();
    FreeAllSpritePalettes();
    FillPalette(RGB_BLACK, 0, PLTT_SIZE);
    sCanvas = AllocZeroed(sizeof(*sCanvas));
    sCanvas->gender = gSaveBlock2Ptr->playerGender == FEMALE ? FEMALE : MALE;
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(2) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(SCENE_MAP) | BGCNT_16COLOR | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2) | BGCNT_CHARBASE(2) | BGCNT_SCREENBASE(RAINBOW_MAP) | BGCNT_16COLOR | BGCNT_TXT256x256);
    StartBattle();
    sCanvas->brightness = -16;
    BeginOam();
    EndOam();
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_BG1_ON);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB_Canvas);
    SetMainCallback2(CB2_Canvas);
}

static void CanvasExit(void)
{
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    FillPalette(RGB_BLACK, 0, PLTT_SIZE);
    FREE_AND_SET_NULL(sCanvas);
    DisableInterrupts(INTR_FLAG_HBLANK);
    SetHBlankCallback(NULL);
    SetMainCallback2(CB2_InitTitleScreen);
}
