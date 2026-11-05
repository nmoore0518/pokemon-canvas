// Pokémon Canvas: the Trading Card Game duel's animations, ported from the Game Boy Color game's
// engine/duel/animations/ (core, commands, screen_effects), engine/gfx/sprite_animations.asm,
// home/load_animation.asm, play_animation.asm, scroll.asm and load_gfx.asm's sprite loading
// (pret/poketcg) under their own names (Tcg_ prefixed), on the emulated GBC display. The data is the
// game's (src/data/tcg/anims.h).
//
// As in the game: UpdateQueuedAnimations runs each frame (DoFrame's wDoFrameFunction) while duel
// animations are queued, plays the buffered DUEL_ANIM_*s one after another, and moves the sprites.
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "tcg_ui.h"
#include "tcg_anims.h"
#include "gpu_regs.h"
#include "main.h"
#include "constants/rgb.h"

#define G gTcgGbc

// ---- the sprite animations' buffer (wSpriteAnimBuffer) and VRAM (wSpriteVRAMBuffer) -----------------

struct TcgAnimSprite
{
    bool8 enabled;
    u8 attributes;
    u8 x, y;
    u8 tileId;
    u8 animId;
    u8 counter;
    u8 flags;
    const struct TcgSpriteAnim *anim;
    u16 pos;                // (the frame data's next entry)
    const u8 *frame;        // (NULL: none)
};

struct TcgAnimState
{
    struct TcgAnimSprite sprites[TCG_SPRITE_ANIM_BUFFER_CAPACITY];
    u8 whichSprite;                     // wWhichSprite
    u8 vramBuffer[16][4];               // wSpriteVRAMBuffer: (valid, sprite, tile offset, tiles)
    u8 vramBufferSize;
    bool8 allSpriteAnimationsDisabled;  // wAllSpriteAnimationsDisabled
    bool8 skipDelayAllowed;             // wSkipDelayAllowed
    // the duel animations
    u8 animationQueue[TCG_ANIMATION_QUEUE_LENGTH];   // wAnimationQueue
    u8 activeScreenAnim;                // wActiveScreenAnim
    u8 d4c0;                            // wd4c0
    u8 buffer[16][7];                   // wDuelAnimBuffer
    u8 bufferCurPos, bufferSize;
    u8 tempAnimation;                   // wTempAnimation
    u8 duelAnimationScreen;             // wDuelAnimationScreen
    u8 duelAnimDuelistSide;             // wDuelAnimDuelistSide (TCG_PLAYER, TCG_OPPONENT)
    u8 duelAnimLocationParam;           // wDuelAnimLocationParam
    u16 duelAnimDamage;                 // wDuelAnimDamage
    u8 duelAnimSetScreen;               // wDuelAnimSetScreen
    u8 duelAnimEffectiveness;           // wDuelAnimEffectiveness
    u8 animFlags;                       // wAnimFlags
    bool8 animationsDisabled;           // wAnimationsDisabled (the game's option: off)
    u8 damageCharIndex, damageCharAnimDelay;
    u8 decimalChars[3];
    // the attack animations
    bool8 attackAnimationIsPlaying;     // wAttackAnimationIsPlaying
    u8 loadedAttackAnimation;           // wLoadedAttackAnimation
    u8 damageAnimPlayAreaLocation;      // wDamageAnimPlayAreaLocation
    u8 damageAnimEffectiveness;         // wDamageAnimEffectiveness
    u16 damageAnimAmount;               // wDamageAnimAmount
    // the screen animations
    void (*screenAnimUpdate)(void);     // wScreenAnimUpdatePtr
    u8 screenAnimDuration;              // wScreenAnimDuration
    const s8 *screenShakeOffsets;       // wScreenShakeOffsetsPtr
    u16 tempBgPals[8][4];               // wTempBackgroundPalettesCGB
    u8 bgScrollMod;                     // wBGScrollMod
    bool8 distorting;
};

static EWRAM_DATA struct TcgAnimState sAnim = {0};
#define A (&sAnim)

// ---- sprites' graphics and palettes (load_gfx.asm) -----------------------------------------------------

// (Func_12c05) the sprite's tiles in VRAM (from $8000): its tile offset; 0xFF if there's no room
static u32 LoadSpriteToVRAM(u32 sprite)
{
    u32 i, d = 0;
    for (i = 0; i < A->vramBufferSize; i++)
    {
        if (A->vramBuffer[i][1] == sprite)
            break;
        d = A->vramBuffer[i][2] + A->vramBuffer[i][3];
    }
    if (i == A->vramBufferSize)
    {
        const struct TcgSprite *s = &gTcgSprites[sprite];
        if (A->vramBufferSize >= 16 || s->tiles == NULL)
            return 0xFF;
        A->vramBufferSize++;
        A->vramBuffer[i][0] = 0;
        A->vramBuffer[i][1] = sprite;
        A->vramBuffer[i][2] = d;
        A->vramBuffer[i][3] = s->numTiles;
        Tcg_BankswitchVRAM0();
        Tcg_WriteVram(TCG_V_TILES0 + d * TCG_TILE_SIZE, s->tiles, s->numTiles * TCG_TILE_SIZE);
    }
    A->vramBuffer[i][0]++;
    if (A->vramBuffer[i][2] + A->vramBuffer[i][3] > 0x80)
        return 0xFF;
    return A->vramBuffer[i][2];
}

// (LoadOBPalette) PALETTE_*'s CGB palettes as OBJ palettes from `index`
void Tcg_LoadOBPalette(u32 palette, u32 index)
{
    const u16 *p = gTcgPalettes[palette];
    u32 n;
    if (p == NULL)
        return;
    n = *p++;
    if (index + n > 8)
        n = 8 - index;
    memcpy(G->objPals[index], p, n * 8);
    Tcg_FlushAllPalettes();
}

// ---- sprite_animations.asm ------------------------------------------------------------------------------

static void ClearSpriteAnimations(void)
{
    u32 i;
    if (A->allSpriteAnimationsDisabled)
        return;
    for (i = 0; i < TCG_SPRITE_ANIM_BUFFER_CAPACITY; i++)
        A->sprites[i].enabled = FALSE;
    A->vramBufferSize = 0;
    memset(A->vramBuffer, 0, sizeof(A->vramBuffer));
    Tcg_ZeroObjectPositions();
    G->vBlankOAMCopyToggle = TRUE;
}

static void EnableAndClearSpriteAnimations(void)
{
    A->allSpriteAnimationsDisabled = FALSE;
    ClearSpriteAnimations();
}

// CreateSpriteAndAnimBufferEntry: a sprite of SPRITE_*, in wWhichSprite. TRUE (the game's carry): none free
static bool32 CreateSpriteAndAnimBufferEntry(u32 sprite)
{
    u32 tile, i;
    if (A->allSpriteAnimationsDisabled)
        return FALSE;
    tile = LoadSpriteToVRAM(sprite);
    for (i = 0; i < TCG_SPRITE_ANIM_BUFFER_CAPACITY; i++)
        if (!A->sprites[i].enabled)
            break;
    A->whichSprite = i;
    if (i == TCG_SPRITE_ANIM_BUFFER_CAPACITY)
    {
        A->whichSprite = TCG_SPRITE_ANIM_BUFFER_CAPACITY - 1;
        return TRUE;
    }
    memset(&A->sprites[i], 0, sizeof(A->sprites[i]));
    A->sprites[i].enabled = TRUE;
    A->sprites[i].tileId = tile == 0xFF ? 0 : tile;
    A->sprites[i].animId = 0xFF;
    A->sprites[i].counter = 0xFF;
    return FALSE;
}

static void DisableSpriteAnim(u32 sprite)
{
    if (A->allSpriteAnimationsDisabled)
        return;
    A->sprites[sprite % TCG_SPRITE_ANIM_BUFFER_CAPACITY].enabled = FALSE;
}

// (HandleAnimationFrame) the frame data's next entry: the frame, its count (0: back to the start),
// the move
static void HandleAnimationFrame(struct TcgAnimSprite *s)
{
    for (;;)
    {
        const s8 *fd = &s->anim->frameData[s->pos * 4];
        u8 frame = fd[0], count = fd[1];
        s->pos++;
        s->frame = frame == 0xFF ? NULL : s->anim->frameTable[frame];
        s->counter = count;
        if (count == 0)
        {
            s->pos = 0;
            continue;
        }
        s->x += (s->flags & TCG_SPRITE_ANIM_FLAG_X_INVERTED) ? -fd[2] : fd[2];
        s->y += (s->flags & TCG_SPRITE_ANIM_FLAG_Y_INVERTED) ? -fd[3] : fd[3];
        return;
    }
}

static void LoadSpriteAnimPointers(struct TcgAnimSprite *s, u32 animId)
{
    s->animId = animId;
    s->anim = &gTcgSpriteAnims[animId];
    s->pos = 0;
}

static void StartSpriteAnimation(u32 sprite, u32 animId)
{
    struct TcgAnimSprite *s = &A->sprites[sprite];
    LoadSpriteAnimPointers(s, animId);
    if (s->anim->frameData == NULL)
    {
        s->enabled = FALSE;
        return;
    }
    HandleAnimationFrame(s);
}

static void StartNewSpriteAnimation(u32 sprite, u32 animId)
{
    if (A->sprites[sprite].animId == animId)
        return;
    StartSpriteAnimation(sprite, animId);
}

// (Func_12ac9) the animation, after `delay` frames (none: at once)
static void StartSpriteAnimationAfter(u32 sprite, u32 animId, u32 delay)
{
    struct TcgAnimSprite *s = &A->sprites[sprite];
    if (delay == 0)
    {
        StartSpriteAnimation(sprite, animId);
        return;
    }
    LoadSpriteAnimPointers(s, animId);
    s->frame = NULL;
    s->counter = delay;
}

// (DrawSpriteAnimationFrame) the frame's 8x8 objects, relative to the sprite (flipped with it)
static void DrawSpriteAnimationFrame(const struct TcgAnimSprite *s)
{
    const u8 *f = s->frame;
    u32 n = *f++;
    s32 xEdge = s->x >= 0xf0 ? -1 : 0, yEdge = s->y >= 0xf0 ? -1 : 0;
    for (; n; n--, f += 4)
    {
        s32 yoff = (s8)f[0], xoff = (s8)f[1], y, x;
        u8 pal, flips;
        if (s->attributes & 0x40)
            y = (yEdge * 256 + s->y) - (yoff + 8);
        else
            y = (yEdge * 256 + s->y) + yoff;
        if (y < 0 || y > 0xFF)
            continue;
        if (s->attributes & 0x20)
            x = (xEdge * 256 + s->x) - (xoff + 8);
        else
            x = (xEdge * 256 + s->x) + xoff;
        if (x < 0 || x > 0xFF)
            continue;
        pal = (s->attributes + f[3]) & 0x17;
        flips = (s->attributes ^ f[3]) & 0xE0;
        Tcg_SetOneObjectAttributes(y, x, s->tileId + f[2], pal | flips);
    }
}

static void HandleAllSpriteAnimations(void)
{
    u32 i;
    if (A->allSpriteAnimationsDisabled)
        return;
    Tcg_ZeroObjectPositions();
    for (i = 0; i < TCG_SPRITE_ANIM_BUFFER_CAPACITY; i++)
    {
        struct TcgAnimSprite *s = &A->sprites[i];
        A->whichSprite = i;
        if (!s->enabled)
            continue;
        // (TryHandleSpriteAnimationFrame: twice as fast with SPRITE_ANIM_FLAG_CENTERED)
        if (s->counter != 0xFF)
        {
            u32 d = (s->flags & TCG_SPRITE_ANIM_FLAG_CENTERED) ? 2 : 1;
            if (s->counter <= d)
                HandleAnimationFrame(s);
            else
                s->counter -= d;
        }
        if (s->frame)
            DrawSpriteAnimationFrame(s);
    }
    G->vBlankOAMCopyToggle = TRUE;
}

// ---- the screen animations (screen_effects.asm) -------------------------------------------------------

static const s8 sSmallShakeOffsets[] = {21, 2, 17, -2, 13, 2, 9, -2, 5, 1, 1, -1, 0, 0};
static const s8 sBigShakeOffsets[] = {29, 4, 25, -4, 21, 4, 17, -4, 13, 3, 9, -3, 5, 2, 1, -2, 0, 0};
static const s8 sBGScrollData[64] =
{
     0,  0,  0,  1,  1,  1,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,
     4,  3,  3,  3,  3,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,
     0, -1, -1, -1, -2, -2, -2, -3, -3, -3, -4, -4, -4, -4, -4, -4,
    -5, -4, -4, -4, -4, -4, -4, -3, -3, -3, -2, -2, -2, -1, -1, -1,
};

static void DistortHBlank(void)
{
    u32 line = REG_VCOUNT, ly;
    s32 a;
    if (line < 8 - 1 || line >= 8 + 0x60 - 1)
    {
        REG_BG0HOFS = (G->scx - 40) & 0x1FF;
        return;
    }
    ly = line + 1 - 8;
    a = sBGScrollData[(ly + G->vBlankCounter) & 0x3f];
    switch (A->bgScrollMod)
    {
    case 1:
        break;
    case 2:
        a >>= 1;
        break;
    case 3:
        a >>= 2;
        break;
    default:
        a >>= 3;
        break;
    }
    REG_BG0HOFS = (G->scx + a - 40) & 0x1FF;
}

static void SetDistortion(bool32 on)
{
    if (on == A->distorting)
        return;
    A->distorting = on;
    if (on)
    {
        SetHBlankCallback(DistortHBlank);
        EnableInterrupts(INTR_FLAG_HBLANK);
    }
    else
    {
        DisableInterrupts(INTR_FLAG_HBLANK);
        SetHBlankCallback(NULL);
    }
}

static void DefaultScreenAnimationUpdate(void);

static void LoadDefaultScreenAnimationUpdateWhenFinished(void)
{
    if (A->screenAnimDuration == 0)
        DefaultScreenAnimationUpdate();
}

static void DefaultScreenAnimationUpdate(void)
{
    A->activeScreenAnim = 0xFF;
    SetDistortion(FALSE);
    G->scx = G->scy = 0;
    A->screenAnimUpdate = DefaultScreenAnimationUpdate;
}

// (UpdateShakeOffset) the offset to apply now, if the duration's reached the next one's time
static bool32 UpdateShakeOffset(s8 *offset)
{
    const s8 *p = A->screenShakeOffsets;
    if (A->screenAnimDuration >= (u8)p[0])
        return FALSE;
    *offset = p[1];
    A->screenShakeOffsets = p + 2;
    return TRUE;
}

static void ShakeScreenX_Update(void)
{
    s8 o;
    A->screenAnimDuration--;
    if (UpdateShakeOffset(&o))
        G->scx += o;
    LoadDefaultScreenAnimationUpdateWhenFinished();
}

static void ShakeScreenY_Update(void)
{
    s8 o;
    A->screenAnimDuration--;
    if (UpdateShakeOffset(&o))
        G->scy += o;
    LoadDefaultScreenAnimationUpdateWhenFinished();
}

static void WhiteFlashScreen_Update(void)
{
    if (--A->screenAnimDuration)
        return;
    memcpy(G->bgPals, A->tempBgPals, sizeof(G->bgPals));
    Tcg_FlushAllPalettes();
    DefaultScreenAnimationUpdate();
}

static void DistortScreen_Update(void)
{
    static const u8 sBGScrollModData[] = {4, 3, 2, 1, 1, 1, 1, 2};
    A->bgScrollMod = sBGScrollModData[(A->screenAnimDuration >> 3) & 7];
    A->screenAnimDuration--;
    LoadDefaultScreenAnimationUpdateWhenFinished();
}

// (InitScreenAnimation) DUEL_ANIM_SMALL_SHAKE_X .. DUEL_ANIM_DISTORT
static void InitScreenAnimation(void)
{
    static const u8 sDurations[] = {24, 32, 24, 32, 8, 63};
    u32 which = A->tempAnimation - TCG_DUEL_SCREEN_ANIMS;
    if (A->animationsDisabled || which >= ARRAY_COUNT(sDurations))
        return;
    A->activeScreenAnim = A->tempAnimation;
    A->screenAnimDuration = sDurations[which];
    switch (which)
    {
    case 0:
    case 1:
        A->screenShakeOffsets = which == 0 ? sSmallShakeOffsets : sBigShakeOffsets;
        A->screenAnimUpdate = ShakeScreenX_Update;
        break;
    case 2:
    case 3:
        A->screenShakeOffsets = which == 2 ? sSmallShakeOffsets : sBigShakeOffsets;
        A->screenAnimUpdate = ShakeScreenY_Update;
        break;
    case 4:
    {
        u32 p, c;
        A->screenAnimUpdate = WhiteFlashScreen_Update;
        memcpy(A->tempBgPals, G->bgPals, sizeof(G->bgPals));
        for (p = 0; p < 8; p++)
            for (c = 0; c < 4; c++)
                G->bgPals[p][c] = RGB(31, 31, 31);
        Tcg_FlushAllPalettes();
        WhiteFlashScreen_Update();
        break;
    }
    default:
        A->screenAnimUpdate = DistortScreen_Update;
        A->bgScrollMod = 1;
        SetDistortion(TRUE);
        DistortScreen_Update();
        break;
    }
}

static void DoScreenAnimationUpdate(void)
{
    A->screenAnimDuration = 1;
    A->screenAnimUpdate();
    DefaultScreenAnimationUpdate();
}

// ---- the duel animations (core.asm) ---------------------------------------------------------------------------

static void UpdateQueuedAnimations(void);

static const u8 sAnimationCoordinatesIndex[] =
{
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
};

static const u8 sAnimationCoordinates[][3] =
{
    { 88, 88, TCG_SPRITE_ANIM_FLAG_3},
    { 40, 80, 0},
    {136, 48, TCG_SPRITE_ANIM_FLAG_Y_FLIP | TCG_SPRITE_ANIM_FLAG_X_FLIP | TCG_SPRITE_ANIM_FLAG_Y_INVERTED | TCG_SPRITE_ANIM_FLAG_X_INVERTED},
    { 88, 72, 0},
    { 24, 96, 0},
    { 56, 96, 0},
    { 88, 96, 0},
    {120, 96, 0},
    {152, 96, 0},
    { 88, 80, 0},
    {152, 40, 0},
    {120, 40, 0},
    { 88, 40, 0},
    { 56, 40, 0},
    { 24, 40, 0},
};

// (GetAnimCoordsAndFlags) where the animation goes: x, y, and its flags (of wAnimFlags)
static u8 GetAnimCoordsAndFlags(u8 *x, u8 *y)
{
    u32 c = 0;
    if (!(A->animFlags & TCG_SPRITE_ANIM_FLAG_CENTERED))
    {
        c = A->duelAnimationScreen * 12;
        if (A->duelAnimDuelistSide != TCG_PLAYER)
            c += 6;
        c = sAnimationCoordinatesIndex[(c + A->duelAnimLocationParam) % ARRAY_COUNT(sAnimationCoordinatesIndex)];
    }
    *x = sAnimationCoordinates[c][0];
    *y = sAnimationCoordinates[c][1];
    return A->animFlags & sAnimationCoordinates[c][2];
}

static void LoadAnimCoordsAndFlags(u32 sprite)
{
    struct TcgAnimSprite *s = &A->sprites[sprite];
    u8 x, y, f = GetAnimCoordsAndFlags(&x, &y);
    s->attributes |= f & (TCG_SPRITE_ANIM_FLAG_Y_FLIP | TCG_SPRITE_ANIM_FLAG_X_FLIP);
    s->x = x;
    s->y = y;
    s->flags |= f & (TCG_SPRITE_ANIM_FLAG_Y_INVERTED | TCG_SPRITE_ANIM_FLAG_X_INVERTED);
}

void Tcg_ResetAnimationQueue(void)
{
    if (gTcg->headless)
        return;
    Tcg_Set_OBJ_8x8();
    G->doFrameFunction = UpdateQueuedAnimations;
    memset(A->animationQueue, 0xFF, sizeof(A->animationQueue));
    A->activeScreenAnim = 0xFF;
    A->d4c0 = 0xFF;
    A->bufferCurPos = A->bufferSize = 0;
    A->duelAnimSetScreen = 0;
    DefaultScreenAnimationUpdate();
    EnableAndClearSpriteAnimations();
}

// (CheckSkipDelayAllowed) TRUE when B's held and the skip delay's allowed (the game's config's "Skip
// some" animations; off unless it's set, as a new game's is)
bool32 Tcg_CheckSkipDelayAllowed(void)
{
    return A->skipDelayAllowed && (G->keysHeld & TCG_PAD_B);
}

// (CheckAnyAnimationPlaying) TRUE (the game's carry): something's still playing
bool32 Tcg_CheckAnyAnimationPlaying(void)
{
    u8 a = A->activeScreenAnim & A->d4c0;
    u32 i;
    for (i = 0; i < TCG_ANIMATION_QUEUE_LENGTH; i++)
        a &= A->animationQueue[i];
    return a != 0xFF;
}

// the damage numbers (DUEL_ANIM_DAMAGE_HUD's): sprites of SPRITE_DUEL_DAMAGE
static void CreateDamageCharSprite(u32 animId, u8 *queueSlot)
{
    static const s8 sRelativeXPos[] = {-16, -8, 0, 8, -8, -16};
    u8 x, y;
    CreateSpriteAndAnimBufferEntry(TCG_SPRITE_DUEL_DAMAGE);
    *queueSlot = A->whichSprite;
    A->animFlags = TCG_SPRITE_ANIM_FLAG_UNSKIPPABLE;
    GetAnimCoordsAndFlags(&x, &y);
    A->sprites[A->whichSprite].x = x + sRelativeXPos[A->damageCharIndex];
    A->sprites[A->whichSprite].y = y;
    StartSpriteAnimationAfter(A->whichSprite, animId, A->damageCharAnimDelay);
}

static void DrawDamageAnimation(void)
{
    u32 n = A->duelAnimDamage, i;
    if (n >= 1000)
        return;
    A->damageCharAnimDelay = 0;
    Tcg_LoadOBPalette(TCG_PALETTE_DAMAGE, 0);
    A->decimalChars[0] = TCG_SPRITE_ANIM_79 + n / 100;
    A->decimalChars[1] = TCG_SPRITE_ANIM_79 + (n / 10) % 10;
    A->decimalChars[2] = TCG_SPRITE_ANIM_79 + n % 10;
    for (i = 0; i < 2 && A->decimalChars[i] == TCG_SPRITE_ANIM_79; i++)
        A->decimalChars[i] = 0;
    for (A->damageCharIndex = 0; A->damageCharIndex < 3; A->damageCharIndex++)
        if (A->decimalChars[A->damageCharIndex])
            CreateDamageCharSprite(A->decimalChars[A->damageCharIndex], &A->animationQueue[1 + A->damageCharIndex]);
    if (A->duelAnimEffectiveness & 1)
    {
        A->damageCharIndex = 3;
        CreateDamageCharSprite(TCG_SPRITE_ANIM_91, &A->animationQueue[4]);
    }
    A->damageCharAnimDelay = 18;
    if (A->duelAnimEffectiveness & 2)
    {
        A->damageCharIndex = 4;
        CreateDamageCharSprite(TCG_SPRITE_ANIM_90, &A->animationQueue[5]);
        A->damageCharAnimDelay += 18;
    }
    if (A->duelAnimEffectiveness & 4)
    {
        A->damageCharIndex = 5;
        CreateDamageCharSprite(TCG_SPRITE_ANIM_89, &A->animationQueue[6]);
    }
    A->duelAnimEffectiveness = 0;
}

// ---- the special animations (commands.asm): run as Func_3bb5 does, the sprites moved first -----------------

static void UpdateDuelAnimationScreen(u8 *layout, u8 *duelist);
static void SetScreenForDuelAnimation(void);
static void PrintDamageText(void);

static void RunSpecialAnimation(void (*func)(void))
{
    A->d4c0 = 0;
    HandleAllSpriteAnimations();
    func();
    A->d4c0 = 0x80;
}

static void UpdateMainSceneHUD(void)
{
    if (gTcg->duelDisplayedScreen == TCG_DUEL_MAIN_SCENE)
        Tcg_DrawDuelHUDs();
}

static void NoSpecialAnimation(void)
{
}

// (Func_1cb5e and Func_1ce03) DUEL_ANIM_DAMAGE_HUD, the screen animations, the special ones ($96+)
static void PlaySpecialDuelAnimation(void)
{
    u8 a = A->tempAnimation;
    if (a < TCG_DUEL_ANIM_SET_SCREEN)
    {
        if (a == TCG_DUEL_ANIM_DAMAGE_HUD)
            DrawDamageAnimation();
        else
            InitScreenAnimation();
        return;
    }
    switch (a)
    {
    case TCG_DUEL_ANIM_SET_SCREEN:
        RunSpecialAnimation(SetScreenForDuelAnimation);
        break;
    case TCG_DUEL_ANIM_PRINT_DAMAGE:
        RunSpecialAnimation(PrintDamageText);
        break;
    case TCG_DUEL_ANIM_UPDATE_HUD:
        RunSpecialAnimation(UpdateMainSceneHUD);
        break;
    default:
        RunSpecialAnimation(NoSpecialAnimation);
        break;
    }
}

// PlayLoadedDuelAnimation: wTempAnimation now
static void PlayLoadedDuelAnimation(void)
{
    const u8 *d;
    if (G->doFrameFunction != UpdateQueuedAnimations)
        return;
    if (A->tempAnimation >= TCG_DUEL_SPECIAL_ANIMS)
    {
        PlaySpecialDuelAnimation();
        return;
    }
    d = gTcgDuelAnimations[A->tempAnimation];
    if (A->animationsDisabled && !(d[3] & TCG_SPRITE_ANIM_FLAG_UNSKIPPABLE))
        return;
    if (d[4])
        Tcg_PlaySFX(d[4]);
    // (.handler_func: a sprite, palette and animation, when all three are set)
    if (d[0] == 0 || d[1] == 0 || d[2] == 0)
        return;
    CreateSpriteAndAnimBufferEntry(d[0]);
    A->animationQueue[0] = A->whichSprite;
    Tcg_LoadOBPalette(d[1], 0);
    A->animFlags = d[3];
    LoadAnimCoordsAndFlags(A->animationQueue[0]);
    StartNewSpriteAnimation(A->animationQueue[0], d[2]);
}

static void LoadDuelAnimationToBuffer(void)
{
    u8 next = (A->bufferSize + 1) % ARRAY_COUNT(A->buffer);
    u8 *b;
    if (next == A->bufferCurPos)
        return;
    b = A->buffer[A->bufferSize];
    A->bufferSize = next;
    b[0] = A->tempAnimation;
    b[1] = A->duelAnimationScreen;
    b[2] = A->duelAnimDuelistSide;
    b[3] = A->duelAnimLocationParam;
    b[4] = A->duelAnimDamage & 0xFF;
    b[5] = A->duelAnimDamage >> 8;
    b[6] = A->duelAnimSetScreen;
}

static void PlayBufferedDuelAnimations(void)
{
    while (A->bufferCurPos != A->bufferSize)
    {
        const u8 *b = A->buffer[A->bufferCurPos];
        A->bufferCurPos = (A->bufferCurPos + 1) % ARRAY_COUNT(A->buffer);
        A->tempAnimation = b[0];
        A->duelAnimationScreen = b[1];
        A->duelAnimDuelistSide = b[2];
        A->duelAnimLocationParam = b[3];
        A->duelAnimDamage = b[4] | (b[5] << 8);
        A->duelAnimSetScreen = b[6];
        PlayLoadedDuelAnimation();
        if (Tcg_CheckAnyAnimationPlaying())
            break;
    }
}

// PlayDuelAnimation: DUEL_ANIM_* now if nothing's playing, else after what's queued
void Tcg_PlayDuelAnimation(u32 anim)
{
    A->tempAnimation = anim;
    if (anim < TCG_DUEL_SPECIAL_ANIMS && A->bufferCurPos == A->bufferSize && !Tcg_CheckAnyAnimationPlaying())
        PlayLoadedDuelAnimation();
    else
        LoadDuelAnimationToBuffer();
}

static void _UpdateQueuedAnimations(void)
{
    u8 a;
    if (A->activeScreenAnim != 0xFF)
    {
        A->screenAnimUpdate();
        a = A->activeScreenAnim;
    }
    else
    {
        a = A->d4c0;
        if (a == 0x80)
        {
            A->d4c0 = 0xFF;
            a = 0xFF;
        }
        else if (a != 0)
        {
            u32 i;
            for (i = 0; i < TCG_ANIMATION_QUEUE_LENGTH; i++)
            {
                u8 s = A->animationQueue[i];
                if (s != 0xFF && A->sprites[s].counter == 0xFF)
                {
                    DisableSpriteAnim(s);
                    A->animationQueue[i] = 0xFF;
                }
                a &= A->animationQueue[i];
            }
        }
    }
    if (a == 0xFF)
        PlayBufferedDuelAnimations();
}

// (UpdateQueuedAnimations: DoFrame's function while animations play)
static void UpdateQueuedAnimations(void)
{
    _UpdateQueuedAnimations();
    HandleAllSpriteAnimations();
}

// ClearAndDisableQueuedAnimations. TRUE (the game's carry): the queue wasn't running
static bool32 ClearAndDisableQueuedAnimations(void)
{
    u32 i;
    if (G->doFrameFunction != UpdateQueuedAnimations)
        return TRUE;
    A->d4c0 = 0xFF;
    if (A->activeScreenAnim != 0xFF)
        DoScreenAnimationUpdate();
    for (i = 0; i < TCG_ANIMATION_QUEUE_LENGTH; i++)
    {
        if (A->animationQueue[i] != 0xFF)
        {
            DisableSpriteAnim(A->animationQueue[i]);
            A->animationQueue[i] = 0xFF;
        }
    }
    A->bufferCurPos = A->bufferSize = 0;
    return FALSE;
}

void Tcg_FinishQueuedAnimations(void)
{
    if (gTcg->headless)
        return;
    if (!ClearAndDisableQueuedAnimations())
        G->doFrameFunction = NULL;
    Tcg_ZeroObjectPositions();
    G->vBlankOAMCopyToggle = TRUE;
}

// ---- the attack animations (commands.asm) ------------------------------------------------------------------

static void UpdateDuelAnimationScreen(u8 *layout, u8 *duelist)
{
    u8 l = A->duelAnimLocationParam;
    if (A->duelAnimSetScreen == TCG_SET_ANIM_SCREEN_MAIN)
    {
        A->duelAnimationScreen = TCG_DUEL_ANIM_SCREEN_MAIN_SCENE;
        return;
    }
    if (A->duelAnimSetScreen != TCG_SET_ANIM_SCREEN_PLAY_AREA)
        return;
    // (wDuelType isn't 0 in a duel against the AI)
    if (gTcg->whoseTurnReal == TCG_PLAYER ? !(l & 0x80) : (l & 0x80))
    {
        *layout = TCG_UNKNOWN_SCREEN_4;
        *duelist = TCG_PLAYER;
        A->duelAnimationScreen = TCG_DUEL_ANIM_SCREEN_PLAYER_PLAY_AREA;
    }
    else
    {
        *layout = TCG_UNKNOWN_SCREEN_5;
        *duelist = TCG_OPPONENT;
        A->duelAnimationScreen = TCG_DUEL_ANIM_SCREEN_OPP_PLAY_AREA;
    }
}

static void SetScreenForDuelAnimation(void)
{
    u8 layout = 0, duelist = 0;
    if (A->duelAnimSetScreen == TCG_SET_ANIM_SCREEN_MAIN)
    {
        A->duelAnimationScreen = TCG_DUEL_ANIM_SCREEN_MAIN_SCENE;
        if (gTcg->duelDisplayedScreen != TCG_DUEL_MAIN_SCENE)
            Tcg_DrawDuelMainScene();
        return;
    }
    if (A->duelAnimSetScreen != TCG_SET_ANIM_SCREEN_PLAY_AREA)
        return;
    UpdateDuelAnimationScreen(&layout, &duelist);
    if (gTcg->duelDisplayedScreen != layout)
    {
        Tcg_DrawYourOrOppPlayAreaScreen_Layout(duelist, TCG_PLAYER);
        gTcg->duelDisplayedScreen = layout;
    }
    Tcg_DrawWideTextBox();
}

static void PrintDamageText(void)
{
    u16 text;
    u32 n = A->damageAnimAmount;
    if (A->loadedAttackAnimation == TCG_ATK_ANIM_HEAL || A->loadedAttackAnimation == TCG_ATK_ANIM_HEALING_WIND_PLAY_AREA)
        return;
    gTcg->loadedCard1 = Tcg_Card(gTcg->tempNonTurnDuelistCardId);
    gTcg->loadedCard1Id = gTcg->tempNonTurnDuelistCardId;
    Tcg_CopyCardNameAndLevel();
    gTcg->txRam2[0] = gTcg->txRam2[1] = NULL;
    // (GetDamageText)
    if (n)
    {
        u8 e = A->damageAnimEffectiveness & ((1 << TCG_RESISTANCE) | (1 << TCG_WEAKNESS));
        Tcg_LoadTxRam3(n);
        if (e == 0)
            text = TCG_TEXT_AttackDamage;
        else if (e == ((1 << TCG_RESISTANCE) | (1 << TCG_WEAKNESS)))
            text = TCG_TEXT_WeaknessMoreDamage2;
        else if (e & (1 << TCG_WEAKNESS))
            text = TCG_TEXT_WeaknessMoreDamage;
        else
            text = TCG_TEXT_ResistanceLessDamage;
    }
    else
    {
        if (Tcg_CheckNoDamageOrEffect())
            text = gTcg->hlText;
        else if (A->damageAnimEffectiveness & (1 << TCG_RESISTANCE))
            text = TCG_TEXT_ResistanceNoDamage;
        else
            text = TCG_TEXT_NoDamage;
    }
    if (text != TCG_TEXT_NONE)
        Tcg_DrawWideTextBox_PrintText(text);
}

// PlayAttackAnimationCommands: the loaded attack animation's commands (the first one starts the queue)
static void PlayAttackAnimationCommands(void)
{
    const u8 *d;
    if (A->loadedAttackAnimation == 0)
        return;
    d = gTcgAttackAnimations[A->loadedAttackAnimation];
    if (d == NULL)
        return;
    if (!A->attackAnimationIsPlaying)
    {
        A->attackAnimationIsPlaying = TRUE;
        Tcg_ResetAnimationQueue();
        A->duelAnimationScreen = TCG_DUEL_ANIM_SCREEN_MAIN_SCENE;
        A->duelAnimSetScreen = TCG_SET_ANIM_SCREEN_MAIN;
        A->duelAnimLocationParam = 0;
        if (d[0] != 4)  // (ANIMCMD_SET_SCREEN)
            Tcg_PlayDuelAnimation(TCG_DUEL_ANIM_SET_SCREEN);
    }
    for (;;)
    {
        u8 cmd = *d++, anim;
        switch (cmd)
        {
        case 0:     // ANIMCMD_END
        case 6:     // ANIMCMD_END_UNUSED
        default:
            return;
        case 2:     // ANIMCMD_PLAYER_SIDE
            A->duelAnimDuelistSide = gTcg->whoseTurn;
            break;
        case 3:     // ANIMCMD_OPP_SIDE
            A->duelAnimDuelistSide = gTcg->whoseTurn ^ 1;
            break;
        case 5:     // ANIMCMD_PLAY_AREA
            A->duelAnimLocationParam = A->damageAnimPlayAreaLocation & 0x7f;
            break;
        case 4:     // ANIMCMD_SET_SCREEN
        {
            u8 l, du;
            A->duelAnimSetScreen = *d++;
            A->duelAnimLocationParam = A->damageAnimPlayAreaLocation;
            UpdateDuelAnimationScreen(&l, &du);
            Tcg_PlayDuelAnimation(TCG_DUEL_ANIM_SET_SCREEN);
            continue;
        }
        case 1:     // ANIMCMD_NORMAL
            break;
        }
        // AnimationCommand_AnimNormal
        anim = *d++;
        if (anim == TCG_DUEL_ANIM_SHOW_DAMAGE)
        {
            Tcg_PlayDuelAnimation(TCG_DUEL_ANIM_PRINT_DAMAGE);
            A->duelAnimEffectiveness = A->damageAnimEffectiveness;
            A->duelAnimDamage = A->damageAnimAmount;
            Tcg_PlayDuelAnimation(TCG_DUEL_ANIM_DAMAGE_HUD);
            if (gTcg->duelDisplayedScreen == TCG_DUEL_MAIN_SCENE)
                Tcg_PlayDuelAnimation(TCG_DUEL_ANIM_UPDATE_HUD);
            continue;
        }
        if (anim == TCG_DUEL_ANIM_SHAKE1 || anim == TCG_DUEL_ANIM_SHAKE2 || anim == TCG_DUEL_ANIM_SHAKE3)
        {
            static const u8 sShakes[3][2] =
            {
                {TCG_DUEL_ANIM_SMALL_SHAKE_X, TCG_DUEL_ANIM_SMALL_SHAKE_Y},
                {TCG_DUEL_ANIM_BIG_SHAKE_X, TCG_DUEL_ANIM_BIG_SHAKE_Y},
                {TCG_DUEL_ANIM_SMALL_SHAKE_Y, TCG_DUEL_ANIM_SMALL_SHAKE_X},
            };
            anim = sShakes[anim - TCG_DUEL_ANIM_SHAKE1][gTcg->whoseTurn == TCG_PLAYER ? 0 : 1];
        }
        Tcg_PlayDuelAnimation(anim);
    }
}

void Tcg_ResetAttackAnimationIsPlaying(void)
{
    A->attackAnimationIsPlaying = FALSE;
}

// PlayAttackAnimation: the loaded attack's animation on the duelist's Pokémon at `location`, with its
// damage and effectiveness
void Tcg_PlayAttackAnimation(u32 duelist, u32 location, u32 effectiveness, u16 damage)
{
    u8 turn = gTcg->whoseTurn;
    if (gTcg->headless)
        return;
    gTcg->whoseTurn = gTcg->whoseTurnReal;
    A->damageAnimEffectiveness = effectiveness;
    if (gTcg->whoseTurn != duelist)
        location |= 0x80;
    A->damageAnimPlayAreaLocation = location;
    A->damageAnimAmount = damage;
    A->loadedAttackAnimation = gTcg->loadedAttack.animation;
    if (A->loadedAttackAnimation == TCG_ATK_ANIM_HIT && damage >= 70)
        A->loadedAttackAnimation = TCG_ATK_ANIM_BIG_HIT;
    PlayAttackAnimationCommands();
    gTcg->whoseTurn = turn;
}

void Tcg_WaitAttackAnimation(void)
{
    if (gTcg->headless || A->loadedAttackAnimation == 0)
        return;
    do
        Tcg_DoFrame();
    while (Tcg_CheckAnyAnimationPlaying());
}

// PlayStatusConditionQueueAnimations: each condition inflicted, its animation
void Tcg_PlayStatusConditionQueueAnimations(void)
{
    u32 i;
    if (gTcg->headless || gTcg->statusConditionQueueIndex == 0)
        return;
    gTcg->statusConditionQueue[gTcg->statusConditionQueueIndex] = 0;
    for (i = 0; gTcg->statusConditionQueue[i]; i += 3)
    {
        u8 who = gTcg->statusConditionQueue[i], cond = gTcg->statusConditionQueue[i + 2], anim;
        if (cond == TCG_ASLEEP)
            anim = TCG_ATK_ANIM_SLEEP;
        else if (cond == TCG_PARALYZED)
            anim = TCG_ATK_ANIM_PARALYSIS;
        else if (cond == TCG_POISONED || cond == TCG_DOUBLE_POISONED)
            anim = TCG_ATK_ANIM_POISON;
        else if (cond == TCG_CONFUSED)
            anim = TCG_QUEUE_DUELIST(gTcg->whoseTurn) == who ? TCG_ATK_ANIM_OWN_CONFUSION : TCG_ATK_ANIM_CONFUSION;
        else
            continue;
        A->loadedAttackAnimation = anim;
        A->duelAnimLocationParam = 0;
        PlayAttackAnimationCommands();
    }
}

// (RedrawTurnDuelistsDuelHUD)
static void RedrawTurnDuelistsDuelHUD(void)
{
    if (gTcg->whoseTurn == gTcg->whoseTurnReal)
    {
        Tcg_DrawDuelHUDs();
        return;
    }
    Tcg_SwapTurn();
    Tcg_DrawDuelHUDs();
    Tcg_SwapTurn();
}

void Tcg_RedrawTurnDuelistsMainSceneOrDuelHUD(void)
{
    if (gTcg->headless)
        return;
    if (gTcg->duelDisplayedScreen == TCG_DUEL_MAIN_SCENE)
    {
        RedrawTurnDuelistsDuelHUD();
        return;
    }
    if (gTcg->whoseTurn == gTcg->whoseTurnReal)
    {
        Tcg_DrawDuelMainScene();
        return;
    }
    Tcg_SwapTurn();
    Tcg_DrawDuelMainScene();
    Tcg_SwapTurn();
}

// PlayBetweenTurnsAnimation: DUEL_ANIM_* on the turn holder's Active Pokémon (the damage: for the
// damage numbers), to its end
void Tcg_PlayBetweenTurnsAnimation(u32 anim, u32 damage)
{
    if (gTcg->headless)
        return;
    A->duelAnimDuelistSide = gTcg->whoseTurn;
    A->duelAnimLocationParam = 0;
    A->duelAnimationScreen = TCG_DUEL_ANIM_SCREEN_MAIN_SCENE;
    A->duelAnimDamage = damage;
    Tcg_PlayDuelAnimation(anim);
    do
        Tcg_DoFrame();
    while (Tcg_CheckAnyAnimationPlaying());
    RedrawTurnDuelistsDuelHUD();
}
