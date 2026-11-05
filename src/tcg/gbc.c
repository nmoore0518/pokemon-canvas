// Pokémon Canvas: the Trading Card Game duel's emulated Game Boy Color display (include/tcg_gbc.h),
// and the game's home/ routines that drive it, ported from pret/poketcg under their own names
// (Tcg_ prefixed): lcd.asm, frames.asm, input.asm, palettes.asm, setup.asm, empty_screen.asm,
// objects.asm, bg_map.asm, copy.asm, tiles.asm (FillRectangle, the gfx copies).
//
// On the GBA (mode 0):
//   BG0  the GBC's BG map ($9800), char base 0 (the GBC's 768 tiles: bank * 384 + its tile slot),
//        screen base 28. The GBC's colours 0-3 are the GBA's 1-4 (so colour 0 stays opaque).
//   BG1  the GBC's window ($9C00), screen base 29, shown in WIN0 (the window's rectangle)
//   BG2  plain white (the GBC with its LCD off), char base 2, screen base 30
//   OBJ  the GBC's sprites: OBJ tiles bank * 256 + tile; 8x16 as the GBA's 8x16 (1D mapping)
//   WIN1 the GBC's 160x144 in the middle of the GBA's 240x160; outside it, the border (backdrop)
#include "global.h"
#include "tcg.h"
#include "tcg_gbc.h"
#include "gpu_regs.h"
#include "malloc.h"
#include "main.h"
#include "palette.h"
#include "constants/rgb.h"

#define GBC_X   ((DISPLAY_WIDTH - 160) / 2)
#define GBC_Y   ((DISPLAY_HEIGHT - 144) / 2)
#define BORDER_COLOR    RGB(4, 5, 9)

EWRAM_DATA struct TcgGbc *gTcgGbc = NULL;

static void Tcg_VBlankHandler(void);

// ---- the GBC's tiles, as the GBA's ------------------------------------------------------------

// a byte's 8 pixels (bit 7 the leftmost) as 8 nibbles (the GBA's: the low nibble the leftmost)
static u32 Spread(u32 b)
{
    u32 r = 0, x;
    for (x = 0; x < 8; x++)
        if (b & (0x80 >> x))
            r |= 1 << (x * 4);
    return r;
}

static EWRAM_DATA u32 sSpread[256] = {0};

// the GBC tile at `slot` (0-383) of `bank` to the GBA's BG tiles (and OBJ tiles: slots 0-255)
static void ConvertTile(u32 bank, u32 slot)
{
    const u8 *t = &gTcgGbc->vram[bank][slot * 16];
    vu32 *bg = (vu32 *)(BG_VRAM + (bank * 384 + slot) * 32);
    vu32 *obj = (vu32 *)(OBJ_VRAM0 + (bank * 256 + slot) * 32);
    u32 y;
    for (y = 0; y < 8; y++)
    {
        u32 row = sSpread[t[y * 2]] | (sSpread[t[y * 2 + 1]] << 1);
        bg[y] = row + 0x11111111;
        if (slot < 256)
            obj[y] = row;
    }
}

// the GBA screen entry for the GBC's BG map byte at `addr` ($9800-$9FFF: its tile, bank 1's attributes)
static void ConvertMapEntry(u32 addr)
{
    u32 off = addr - 0x8000, map = (addr - TCG_V_BGMAP0) >> 10;
    u32 tile = gTcgGbc->vram[0][off], attr = gTcgGbc->vram[1][off], slot, e;
    if (gTcgGbc->lcdc & TCG_LCDC_BLOCK01)
        slot = tile;
    else
        slot = tile < 128 ? 256 + tile : tile;
    e = ((attr >> 3) & 1) * 384 + slot;
    if (attr & 0x20)
        e |= 0x400;
    if (attr & 0x40)
        e |= 0x800;
    e |= (attr & 7) << 12;
    gTcgGbc->gbaMap[map][addr & 0x3FF] = e;
    gTcgGbc->gbaMapDirty[map] = TRUE;
}

static void ConvertAllMaps(void)
{
    u32 a;
    for (a = TCG_V_BGMAP0; a < 0xA000; a++)
        ConvertMapEntry(a);
}

void Tcg_WriteVram(u32 addr, const u8 *src, u32 n)
{
    u32 bank = gTcgGbc->bankVram, a;
    if (n == 0)
        return;
    memcpy(&gTcgGbc->vram[bank][addr - 0x8000], src, n);
    if (addr < TCG_V_BGMAP0)
    {
        u32 first = (addr - 0x8000) / 16, last = (min(addr + n, TCG_V_BGMAP0) - 1 - 0x8000) / 16, s;
        for (s = first; s <= last; s++)
            ConvertTile(bank, s);
    }
    for (a = max(addr, TCG_V_BGMAP0); a < addr + n; a++)
        ConvertMapEntry(a);
}

void Tcg_WriteVramByte(u32 addr, u8 value)
{
    Tcg_WriteVram(addr, &value, 1);
}

void Tcg_FillVram(u32 addr, u8 value, u32 n)
{
    u8 buf[32];
    memset(buf, value, sizeof(buf));
    while (n)
    {
        u32 k = min(n, sizeof(buf));
        Tcg_WriteVram(addr, buf, k);
        addr += k;
        n -= k;
    }
}

void Tcg_BankswitchVRAM(u32 bank)
{
    gTcgGbc->bankVram = bank & 1;
}

// ---- the GBA's registers, each frame -----------------------------------------------------------

static void UploadPalettes(void)
{
    u32 p, c;
    for (p = 0; p < 8; p++)
    {
        for (c = 0; c < 4; c++)
        {
            ((vu16 *)BG_PLTT)[p * 16 + 1 + c] = gTcgGbc->bgPals[p][c];
            ((vu16 *)OBJ_PLTT)[p * 16 + c] = gTcgGbc->objPals[p][c];
        }
    }
}

static void UploadOam(void)
{
    vu16 *oam = (vu16 *)OAM;
    u32 i;
    bool32 tall = gTcgGbc->lcdc & TCG_LCDC_OBJ_16;
    for (i = 0; i < 128; i++)
    {
        const u8 *o = &gTcgGbc->oam[i * 4];
        u32 y, x, tile, attr;
        if (i >= TCG_OAM_COUNT || !(gTcgGbc->lcdc & TCG_LCDC_OBJ_ON) || o[0] == 0 || o[0] >= 160 || o[1] == 0 || o[1] >= 168)
        {
            oam[i * 4 + 0] = 0x200;     // (hidden)
            continue;
        }
        y = (GBC_Y + o[0] - 16) & 0xFF;
        x = (GBC_X + o[1] - 8) & 0x1FF;
        attr = o[3];
        tile = tall ? (o[2] & 0xFE) : o[2];
        oam[i * 4 + 0] = y | (tall ? 0x8000 : 0);
        oam[i * 4 + 1] = x | ((attr & 0x20) ? 0x1000 : 0) | ((attr & 0x40) ? 0x2000 : 0);
        oam[i * 4 + 2] = (((attr >> 3) & 1) * 256 + tile) | (0 << 10) | ((attr & 7) << 12);
    }
}

static void UpdateRegisters(void)
{
    u32 lcdc = gTcgGbc->lcdc, win = 0, wx, wy;
    bool32 on = lcdc & TCG_LCDC_ON;
    u16 inner;
    SetGpuReg(REG_OFFSET_BG0HOFS, (gTcgGbc->scx - GBC_X) & 0x1FF);
    SetGpuReg(REG_OFFSET_BG0VOFS, (gTcgGbc->scy - GBC_Y) & 0x1FF);
    wx = gTcgGbc->wx;
    wy = gTcgGbc->wy;
    SetGpuReg(REG_OFFSET_BG1HOFS, (-(GBC_X + (s32)wx - 7)) & 0x1FF);
    SetGpuReg(REG_OFFSET_BG1VOFS, (-(GBC_Y + (s32)wy)) & 0x1FF);
    if (on && (lcdc & TCG_LCDC_WIN_ON) && wx <= 166 && wy <= 143)
    {
        u32 left = GBC_X + (wx >= 7 ? wx - 7 : 0);
        win = WININ_WIN0_BG1 | WININ_WIN0_OBJ;
        SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(left, GBC_X + 160));
        SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(GBC_Y + wy, GBC_Y + 144));
    }
    else
    {
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
    }
    if (on)
        inner = ((lcdc & TCG_LCDC_BG_ON) ? WININ_WIN1_BG0 : WININ_WIN1_BG2) | WININ_WIN1_OBJ;
    else
        inner = WININ_WIN1_BG2;
    if (!on)
        win = WININ_WIN0_BG2;
    SetGpuReg(REG_OFFSET_WININ, win | inner);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
}

static void Tcg_VBlankHandler(void)
{
    u32 m;
    gTcgGbc->vBlankCounter++;
    if (gTcgGbc->vBlankOAMCopyToggle)
    {
        UploadOam();
        gTcgGbc->vBlankOAMCopyToggle = FALSE;
    }
    UpdateRegisters();
    for (m = 0; m < 2; m++)
    {
        if (gTcgGbc->gbaMapDirty[m])
        {
            DmaCopy32(3, (const u32 *)gTcgGbc->gbaMap[m], BG_SCREEN_ADDR(28 + m), sizeof(gTcgGbc->gbaMap[m]));
            gTcgGbc->gbaMapDirty[m] = FALSE;
        }
    }
    if (gTcgGbc->vBlankFunction)
        gTcgGbc->vBlankFunction();
    Tcg_FlushPalettesIfRequested();
}

// ---- start and stop -----------------------------------------------------------------------------

void Tcg_GbcInit(void)
{
    u32 i;
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    gTcgGbc = AllocZeroed(sizeof(*gTcgGbc));
    for (i = 0; i < 256; i++)
        sSpread[i] = Spread(i);
    DmaFill32(3, 0, (void *)VRAM, VRAM_SIZE);
    DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
    DmaFill16(3, 0, (void *)PLTT, PLTT_SIZE);
    for (i = 0; i < 128; i++)
        ((vu16 *)OAM)[i * 4] = 0x200;
    ((vu16 *)BG_PLTT)[0] = BORDER_COLOR;
    ((vu16 *)BG_PLTT)[15 * 16 + 1] = RGB_WHITE;
    // BG2: the white of the GBC's LCD off
    DmaFill32(3, 0x11111111, BG_CHAR_ADDR(2), 32);
    DmaFill16(3, 0xF000, BG_SCREEN_ADDR(30), BG_SCREEN_SIZE);
    for (i = 0; i < 2; i++)
        memset(gTcgGbc->gbaMap[i], 0, sizeof(gTcgGbc->gbaMap[i]));
    SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(2) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(28) | BGCNT_16COLOR | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(1) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(29) | BGCNT_16COLOR | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(3) | BGCNT_CHARBASE(2) | BGCNT_SCREENBASE(30) | BGCNT_16COLOR | BGCNT_TXT256x256);
    SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(GBC_X, GBC_X + 160));
    SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(GBC_Y, GBC_Y + 144));
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON | DISPCNT_BG1_ON
                                | DISPCNT_BG2_ON | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
    Tcg_SetupRegisters();
    Tcg_SetupPalettes();
    Tcg_SetupVRAM();
    ConvertAllMaps();
    UpdateRegisters();
    SetVBlankCallback(Tcg_VBlankHandler);
}

void Tcg_GbcFree(void)
{
    SetVBlankCallback(NULL);
    TRY_FREE_AND_SET_NULL(gTcgGbc);
}

// ---- home/lcd.asm, frames.asm, input.asm ---------------------------------------------------------

void Tcg_WaitForVBlank(void)
{
    if (!(gTcgGbc->lcdc & TCG_LCDC_ON))
        return;
    VBlankIntrWait();
}

void Tcg_ReadJoypad(void)
{
    u8 held = ~REG_KEYINPUT & 0xFF;
    gTcgGbc->keysReleased = gTcgGbc->keysHeld & ~held;
    gTcgGbc->keysPressed = ~gTcgGbc->keysHeld & held;
    gTcgGbc->keysHeld = held;
}

void Tcg_HandleDPadRepeat(void)
{
    gTcgGbc->dpadHeld = gTcgGbc->keysHeld;
    if (gTcgGbc->keysHeld & TCG_PAD_CTRL_PAD)
    {
        if (gTcgGbc->keysPressed & TCG_PAD_CTRL_PAD)
        {
            gTcgGbc->dpadRepeat = 24;
            return;
        }
        if (--gTcgGbc->dpadRepeat != 0)
            goto done;
        gTcgGbc->dpadRepeat = 6;
        return;
    }
done:
    gTcgGbc->dpadHeld = gTcgGbc->keysPressed & TCG_PAD_BUTTONS;
}

void Tcg_ClearJoypad(void)
{
    gTcgGbc->dpadRepeat = gTcgGbc->keysReleased = gTcgGbc->dpadHeld = gTcgGbc->keysHeld = gTcgGbc->keysPressed = 0;
}

void Tcg_DoFrame(void)
{
    if (gTcgGbc->doFrameFunction)
        gTcgGbc->doFrameFunction();
    Tcg_WaitForVBlank();
    Tcg_ReadJoypad();
    Tcg_HandleDPadRepeat();
}

void Tcg_DoAFrames(u32 frames)
{
    while (frames--)
        Tcg_DoFrame();
}

void Tcg_EnableLCD(void)
{
    if (gTcgGbc->lcdc & TCG_LCDC_ON)
        return;
    gTcgGbc->lcdc |= TCG_LCDC_ON;
    gTcgGbc->flushPaletteFlags = TCG_FLUSH_ALL_PALS;
}

void Tcg_DisableLCD(void)
{
    if (!(gTcgGbc->lcdc & TCG_LCDC_ON))
        return;
    VBlankIntrWait();
    gTcgGbc->lcdc &= ~TCG_LCDC_ON;
    UpdateRegisters();
}

void Tcg_Set_OBJ_8x8(void)
{
    gTcgGbc->lcdc &= ~TCG_LCDC_OBJ_16;
}

void Tcg_Set_OBJ_8x16(void)
{
    gTcgGbc->lcdc |= TCG_LCDC_OBJ_16;
}

void Tcg_SetWindowOn(void)
{
    gTcgGbc->lcdc |= TCG_LCDC_WIN_ON;
}

void Tcg_SetWindowOff(void)
{
    gTcgGbc->lcdc &= ~TCG_LCDC_WIN_ON;
}

// ---- home/palettes.asm, setup.asm ------------------------------------------------------------------

static void FlushPalettes(u32 flags)
{
    gTcgGbc->flushPaletteFlags = flags;
    if (!(gTcgGbc->lcdc & TCG_LCDC_ON))
        Tcg_FlushPalettesIfRequested();
}

void Tcg_FlushAllPalettes(void)
{
    FlushPalettes(TCG_FLUSH_ALL_PALS);
}

void Tcg_FlushPalette(u32 which)
{
    FlushPalettes(TCG_FLUSH_ONE_PAL | which);
}

void Tcg_FlushPalette0(void)
{
    FlushPalettes(TCG_FLUSH_ONE_PAL);
}

// (the game copies one palette or all 16; here all of them, the same in the end)
void Tcg_FlushPalettesIfRequested(void)
{
    if (!gTcgGbc->flushPaletteFlags)
        return;
    UploadPalettes();
    gTcgGbc->flushPaletteFlags = 0;
}

void Tcg_SetupPalettes(void)
{
    static const u16 sInitialPalette[4] = {RGB(28, 28, 24), RGB(21, 21, 16), RGB(10, 10, 8), RGB(0, 0, 0)};
    u32 p;
    gTcgGbc->flushPaletteFlags = 0;
    for (p = 0; p < 8; p++)
    {
        memcpy(gTcgGbc->bgPals[p], sInitialPalette, sizeof(sInitialPalette));
        memcpy(gTcgGbc->objPals[p], sInitialPalette, sizeof(sInitialPalette));
    }
    UploadPalettes();
}

void Tcg_FillTileMap(void)
{
    Tcg_BankswitchVRAM0();
    Tcg_FillVram(TCG_V_BGMAP0, gTcgGbc->tileMapFill, TCG_V_BGMAP1 - TCG_V_BGMAP0);
    Tcg_BankswitchVRAM1();
    Tcg_FillVram(TCG_V_BGMAP0, 0, TCG_V_BGMAP1 - TCG_V_BGMAP0);
    Tcg_BankswitchVRAM0();
}

void Tcg_SetupVRAM(void)
{
    Tcg_FillTileMap();
    Tcg_BankswitchVRAM1();
    Tcg_FillVram(TCG_V_TILES0, 0, TCG_V_BGMAP0 - TCG_V_TILES0);
    Tcg_BankswitchVRAM0();
    Tcg_FillVram(TCG_V_TILES0, 0, TCG_V_BGMAP0 - TCG_V_TILES0);
}

void Tcg_SetupRegisters(void)
{
    gTcgGbc->scx = gTcgGbc->scy = gTcgGbc->wx = gTcgGbc->wy = 0;
    gTcgGbc->vBlankFunction = NULL;
    gTcgGbc->lcdc = TCG_LCDC_BG_ON | TCG_LCDC_OBJ_ON | TCG_LCDC_OBJ_16 | TCG_LCDC_WIN_9C00;
}

// (EmptyScreen: the SGB packets aside)
void Tcg_EmptyScreen(void)
{
    if (gTcg->headless)
        return;
    Tcg_DisableLCD();
    Tcg_FillTileMap();
    gTcg->duelDisplayedScreen = 0;
}

// ---- home/objects.asm ------------------------------------------------------------------------------

// (SetOneObjectAttributes: TRUE when OAM's full)
bool32 Tcg_SetOneObjectAttributes(u8 y, u8 x, u8 tile, u8 attributes)
{
    u8 *o;
    if (gTcgGbc->oamOffset >= TCG_OAM_COUNT * 4)
        return TRUE;
    o = &gTcgGbc->oam[gTcgGbc->oamOffset];
    o[0] = y;
    o[1] = x;
    o[2] = tile;
    o[3] = attributes;
    gTcgGbc->oamOffset += 4;
    return FALSE;
}

// data: a count, then (y, x, tile, attributes) each; x and y added
bool32 Tcg_SetManyObjectsAttributes(const u8 *data, u8 x, u8 y)
{
    u32 n = *data++;
    while (n--)
    {
        if (Tcg_SetOneObjectAttributes(data[0] + y, data[1] + x, data[2], data[3]))
            return TRUE;
        data += 4;
    }
    return FALSE;
}

void Tcg_ZeroObjectPositions(void)
{
    u32 i;
    gTcgGbc->oamOffset = 0;
    for (i = 0; i < TCG_OAM_COUNT; i++)
        gTcgGbc->oam[i * 4] = gTcgGbc->oam[i * 4 + 1] = 0;
}

// ---- home/bg_map.asm, text_box.asm's coordinates, tiles.asm, copy.asm -----------------------------

u32 Tcg_BCCoordToBGMap0Address(u32 x, u32 y)
{
    return TCG_V_BGMAP0 + y * TCG_TILEMAP_WIDTH + x;
}

void Tcg_AdjustCoordinatesForBGScroll(u8 *x, u8 *y)
{
    *x += (gTcgGbc->scx >> 3) & 0x1F;
    *y += (gTcgGbc->scy >> 3) & 0x1F;
}

void Tcg_CopyDataToBGMap0(u32 x, u32 y, const u8 *src, u32 n)
{
    Tcg_WriteVram(Tcg_BCCoordToBGMap0Address(x, y), src, n);
}

void Tcg_WriteByteToBGMap0(u32 x, u32 y, u8 value)
{
    Tcg_WriteVram(Tcg_BCCoordToBGMap0Address(x, y), &value, 1);
}

// x, y, data, $00: the data at x, y. Returns the next block.
const u8 *Tcg_WriteDataBlockToBGMap0(const u8 *block)
{
    u32 x = block[0], y = block[1], n = 0;
    block += 2;
    while (block[n])
        n++;
    Tcg_CopyDataToBGMap0(x, y, block, n);
    return block + n + 1;
}

// blocks until $ff
void Tcg_WriteDataBlocksToBGMap0(const u8 *blocks)
{
    do
        blocks = Tcg_WriteDataBlockToBGMap0(blocks);
    while (!(*blocks & 0x80));
}

// a w x h rectangle at x, y of tiles numbered from firstTile: +tileStep across, +rowStep down (the
// game's h and l: "lb hl, 1, 10")
void Tcg_FillRectangle(u32 x, u32 y, u32 w, u32 h, u8 firstTile, u8 tileStep, u8 rowStep)
{
    u8 row[TCG_TILEMAP_WIDTH];
    u32 addr = Tcg_DECoordToBGMap0Address(x, y), i;
    while (h--)
    {
        u8 t = firstTile;
        for (i = 0; i < w; i++, t += tileStep)
            row[i] = t;
        Tcg_WriteVram(addr, row, w);
        firstTile += rowStep;
        addr += TCG_TILEMAP_WIDTH;
    }
}

void Tcg_CopyGfxData(u32 dest, const u8 *src, u32 tiles, u32 tileSize)
{
    Tcg_WriteVram(dest, src, tiles * tileSize);
}

void Tcg_CopyFontsOrDuelGraphicsTiles(u32 dest, const u8 *src, u32 tiles)
{
    Tcg_CopyGfxData(dest, src, tiles, TCG_TILE_SIZE);
}
