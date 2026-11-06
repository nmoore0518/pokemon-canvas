// Pokémon Canvas: the Trading Card Game's sound engine (pret/poketcg's audio/music1.asm, sfx.asm),
// on the GBA's own Game Boy sound channels.
// The engine is ported as the game has it, down to its RAM: its variables live in a copy of the
// game's "WRAM Audio" section (sRam, at the game's addresses from $dd80), the music and sound
// effects' bytes in gTcgAudio (src/data/tcg/audio.h, whose pointers are offsets into it; they're all
// below $dd80, so an address says which it is), and its register writes go to the GBA's PSG
// registers (the same channels, at other addresses). TcgSound_Update runs every VBlank (the game's
// timer ran it at 60.24Hz: 0.85% faster than this).
#include "global.h"
#include "tcg_sound.h"
#include "gba/io_reg.h"
#include "sound.h"
#include "constants/tcg.h"

#include "../data/tcg/audio.h"

// ---- the game's RAM -----------------------------------------------------------------------------------------

#define RAM_START 0xdd80
enum
{
    wCurSongID = 0xdd80, wCurSongBank, wCurSfxID, wSfxPriority, wMusicStereoPanning, wdd85, wMusicDuty1,
    wMusicDuty2, wMusicWave = 0xdd8a, wMusicWaveChange, wdd8c, wMusicIsPlaying, wMusicTie = 0xdd91,
    wMusicChannelPointers = 0xdd95, wMusicMainLoopStart = 0xdd9d, wMusicCh1CurPitch = 0xdda5,
    wMusicCh1CurOctave, wMusicCh2CurPitch, wMusicCh2CurOctave, wMusicCh3CurPitch, wMusicCh3CurOctave, wddab,
    wddac, wddad, wddae, wMusicOctave, wddb3 = 0xddb3, wddb7 = 0xddb7, wddb8, wddb9, wddba, wddbb,
    wMusicCutoff = 0xddbf, wddc3 = 0xddc3, wMusicEcho = 0xddc7, wMusicPitchOffset = 0xddcb,
    wMusicSpeed = 0xddcf, wMusicVibratoType = 0xddd3, wMusicVibratoType2 = 0xddd7, wdddb = 0xdddb,
    wMusicVibratoDelay = 0xdddf, wdde3 = 0xdde3, wMusicVolume = 0xdde7, wMusicFrequencyOffset = 0xddea,
    wdded = 0xdded, wddef = 0xddef, wddf0, wMusicPanning, wddf2, wMusicChannelStackPointers,
    wMusicCh1Stack = 0xddfb, wMusicCh2Stack = 0xde07, wMusicCh3Stack = 0xde13, wMusicCh4Stack = 0xde1f,
    wde2b = 0xde2b, wde2e = 0xde2e, wSFXPitchOffsets, wde32 = 0xde32, wde33, wde37 = 0xde37, wde3d = 0xde3d,
    wde3f = 0xde3f, wde43 = 0xde43, wSFXCommandPointers = 0xde4b, wSFXIsPlaying = 0xde53, wde54,
    RAM_END,
};

// the Game Boy's sound registers
enum
{
    rAUD1SWEEP = 0xff10, rAUD1LEN, rAUD1ENV, rAUD1LOW, rAUD1HIGH,
    rAUD2LEN = 0xff16, rAUD2ENV, rAUD2LOW, rAUD2HIGH,
    rAUD3ENA = 0xff1a, rAUD3LEN, rAUD3LEVEL, rAUD3LOW, rAUD3HIGH,
    rAUD4LEN = 0xff20, rAUD4ENV, rAUD4POLY, rAUD4GO,
    rAUDVOL = 0xff24, rAUDTERM, rAUDENA,
    AUD3WAVERAM = 0xff30,
};

static EWRAM_DATA u8 sRam[RAM_END - RAM_START] = {0};
static EWRAM_DATA u8 sRegs[0x30] = {0};             // (what was last written to each register)
static EWRAM_DATA bool8 sWaveDirty = FALSE;
static EWRAM_DATA bool8 sActive = FALSE;
static EWRAM_DATA bool8 sWaitingForM4a = FALSE;
static EWRAM_DATA u16 sSavedSoundCntL = 0, sSavedSoundCntH = 0;

// the GBA's register for each of the Game Boy's ($ff10-$ff26): its byte's offset from REG_BASE
static const u8 sRegMap[0x17] =
{
    0x60, 0x62, 0x63, 0x64, 0x65, 0,    0x68, 0x69, 0x6c, 0x6d,
    0x70, 0x72, 0x73, 0x74, 0x75, 0,    0x78, 0x79, 0x7c, 0x7d,
    0x80, 0x81, 0x84,
};

static void UploadWave(void)
{
    // (the GBA plays one of two wave banks and lets the other be written: write bank 0 while bank 1
    // plays, then play bank 0)
    u32 i;
    REG_SOUND3CNT_L = 0x40 | (sRegs[rAUD3ENA - 0xff10] & 0x80);
    for (i = 0; i < 4; i++)
        (&REG_WAVE_RAM0)[i] = sRegs[0x20 + i * 4] | (sRegs[0x21 + i * 4] << 8)
                            | (sRegs[0x22 + i * 4] << 16) | (sRegs[0x23 + i * 4] << 24);
    REG_SOUND3CNT_L = sRegs[rAUD3ENA - 0xff10] & 0x80;
    sWaveDirty = FALSE;
}

static void WriteReg(u16 reg, u8 v)
{
    u32 r = reg - 0xff10;
    sRegs[r] = v;
    if (reg >= AUD3WAVERAM)
    {
        sWaveDirty = TRUE;
        return;
    }
    if (reg == rAUDENA)
        return;                     // (the GBA's master enable is the Direct Sound's too: left on)
    if (reg == rAUD3ENA)
    {
        if (sWaveDirty)
            UploadWave();
        REG_SOUND3CNT_L = v & 0x80;
        return;
    }
    if (reg == rAUD3HIGH && sWaveDirty)
        UploadWave();
    if (r < ARRAY_COUNT(sRegMap) && sRegMap[r])
        *(vu8 *)(REG_BASE + sRegMap[r]) = v;
}

static u8 ReadReg(u16 reg)
{
    return sRegs[reg - 0xff10];
}

// a byte at a Game Boy address: the engine's RAM, a register, or the music's bytes
static u8 RD(u16 a)
{
    if (a >= RAM_START && a < RAM_END)
        return sRam[a - RAM_START];
    if (a >= 0xff10 && a < 0xff40)
        return ReadReg(a);
    if (a < sizeof(gTcgAudio))
        return gTcgAudio[a];
    return 0xFF;
}

static void WR(u16 a, u8 v)
{
    if (a >= RAM_START && a < RAM_END)
        sRam[a - RAM_START] = v;
    else if (a >= 0xff10 && a < 0xff40)
        WriteReg(a, v);
}

static u16 RD16(u16 a)
{
    return RD(a) | (RD(a + 1) << 8);
}

static void WR16(u16 a, u16 v)
{
    WR(a, v & 0xFF);
    WR(a + 1, v >> 8);
}

#define R(a) RD(a)
#define W(a, v) WR(a, v)

static u8 rlc(u8 a)
{
    return (a << 1) | (a >> 7);
}

static const u16 sChannelLoopStacks[4] = {wMusicCh1Stack, wMusicCh2Stack, wMusicCh3Stack, wMusicCh4Stack};
static const u8 sOctaveOffsets[8] = {0, 24, 48, 72, 96, 120, 144, 168};

// all real SFX have the same priority (SFX_STOP doesn't use this)
static u8 SfxPriority(u32 id)
{
    return id ? 0x0a : 0;
}

// ---- audio/music1.asm ---------------------------------------------------------------------------------------

static void Music1_PlayNextNote(u16 hl, u8 c);
static void SFX_Play(u8 a);
static void SFX_Update(void);

void Music1_Init(void)
{
    u32 c;
    WR(rAUDENA, 0x80);
    WR(rAUDVOL, 0x77);
    WR(rAUDTERM, 0xff);
    W(wCurSongBank, 0);
    W(wCurSongID, 0x80);
    W(wCurSfxID, 0x80);
    W(wMusicPanning, 0x77);
    W(wdd8c, 0);
    W(wSFXIsPlaying, 0);
    W(wMusicWaveChange, 0);
    W(wddef, 0);
    W(wddf0, 0);
    W(wddf2, 0);
    W(wMusicStereoPanning, 0xff);
    for (c = 0; c < 4; c++)
    {
        W(wMusicIsPlaying + c, 0);
        W(wMusicTie + c, 0);
        W(wddb3 + c, 0);
        W(wMusicPitchOffset + c, 0);
        W(wMusicCutoff + c, 0);
        WR16(wMusicChannelStackPointers + c * 2, sChannelLoopStacks[c]);
    }
}

static void Music1_StopAllChannels(void)
{
    u8 d = R(wdd8c);
    W(wMusicIsPlaying, 0);
    if (!(d & 1))
    {
        WR(rAUD1ENV, 0x08);
        WR(rAUD1HIGH, 0x80);
    }
    W(wMusicIsPlaying + 1, 0);
    if (!(d & 2))
    {
        WR(rAUD2ENV, 0x08);
        WR(rAUD2HIGH, 0x80);
    }
    W(wMusicIsPlaying + 3, 0);
    if (!(d & 8))
    {
        WR(rAUD4ENV, 0x08);
        WR(rAUD4GO, 0x80);
    }
    W(wMusicIsPlaying + 2, 0);
    if (!(d & 4))
        WR(rAUD3LEVEL, 0);
}

static void Music1_BeginSong(u8 a)
{
    u16 hdr, bc;
    u8 e;
    u32 ch;
    hdr = gTcgSongHeaders[a];
    e = R(hdr);
    bc = hdr + 1;
    for (ch = 0; ch < 4; ch++)
    {
        if (e & 1)
        {
            u16 p = RD16(bc);
            bc += 2;
            WR16(wMusicChannelPointers + ch * 2, p);
            WR16(wMusicMainLoopStart + ch * 2, p);
            W(wddbb + ch, 1);
            W(wMusicIsPlaying + ch, 1);
            W(wMusicTie + ch, 0);
            if (ch < 3)
                W(wMusicFrequencyOffset + ch, 0);
            W(wMusicCutoff + ch, 0);
            W(wMusicVibratoDelay + ch, 0);
            W(wMusicPitchOffset + ch, 0);
            WR16(wMusicChannelStackPointers + ch * 2, sChannelLoopStacks[ch]);
            W(wMusicEcho + ch, ch < 2 ? 0x08 : 0x40);
        }
        e >>= 1;
    }
    W(wddf2, 0);
}

static void Music1_CheckForNewSound(void)
{
    if (!(R(wCurSongID) & 0x80))
    {
        Music1_StopAllChannels();
        Music1_BeginSong(R(wCurSongID));
        W(wCurSongID, R(wCurSongID) | 0x80);
    }
    if (!(R(wCurSfxID) & 0x80))
    {
        SFX_Play(R(wCurSfxID));
        W(wCurSfxID, R(wCurSfxID) | 0x80);
    }
}

// (Music1_f4967) de += the channel's frequency offset (a negative one: its complement, as the game has it)
static u16 Music1_ApplyFrequencyOffset(u16 de, u8 c)
{
    u8 a = R(wMusicFrequencyOffset + c);
    if (!(a & 0x80))
        return de + a;
    return de - (u8)~a;
}

static void Music1_note(u8 cmd, u16 hl, u8 c)
{
    u8 next = R(hl), a, d, e;
    u16 p = hl;
    if (R(wMusicTie + c) != 0x80)
    {
        W(wMusicTie + c, 1);
        W(wdddb + c, 0);
        W(wdde3 + c, 1);
        W(wMusicVibratoType + c, R(wMusicVibratoType2 + c));
    }
    // the note's length in frames: (its length) x (the speed), as a loop of adds (8 bits)
    d = R(wMusicSpeed + c);
    a = (cmd & 0xf) + 1;
    if (a < d)
    {
        u8 t = a;
        a = d;
        d = t;
    }
    e = a;
    while (--d)
        a += e;
    W(wddbb + c, a);
    if (next != 0xd9)
    {
        u8 cut = R(wMusicCutoff + c);
        if (cut != 8)
        {
            // (the cutoff: length x cutoff / 8; a cutoff of 0 loops 256 times)
            u16 sum = 0;
            e = a;
            d = cut;
            do
                sum += e;
            while (--d);
            a = (sum >> 3) & 0xFF;
        }
    }
    W(wddc3 + c, a);
    a = cmd & 0xf0;
    W(wddb7 + c, a);
    if (a != 0)
    {
        a = (a >> 4) - 1;
        if (c == 3)
        {
            // the noise channel: its instrument (the "pitch" picks the drum)
            u16 ins = RD16(gTcgNoiseInstruments + a * 2 + R(wMusicOctave + 3) * 24);
            W(wMusicStereoPanning, (R(wMusicStereoPanning) & 0x77) | R(ins));
            W(wddab, R(ins + 1));
            W(wddac, R(ins + 2));
            W(wddad, R(ins + 4));
            W(wddae, R(ins + 3));
            WR16(wdded, ins + 5);
            W(wddef, 1);
        }
        else
        {
            u8 off = a * 2 + sOctaveOffsets[R(wMusicOctave + c) & 7] + R(wMusicPitchOffset + c) * 2;
            u16 de = off / 2 < ARRAY_COUNT(gTcgPitches) ? gTcgPitches[off / 2] : 0;
            de = Music1_ApplyFrequencyOffset(de, c);
            WR16(wMusicCh1CurPitch + c * 2, de);
        }
    }
    WR16(wMusicChannelPointers + c * 2, p);
}

static void Music1_PlayNextNote(u16 hl, u8 c)
{
    for (;;)
    {
        u8 a = R(hl++);
        u16 sp, de;
        if (a < 0xd0)
        {
            Music1_note(a, hl, c);
            return;
        }
        switch (a - 0xd0)
        {
        case 0x00: // speed
            W(wMusicSpeed + c, R(hl++));
            break;
        case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: // octave
            a = (a & 7) - 1;
            if (c == 2)
                a++;
            W(wMusicOctave + c, a);
            break;
        case 0x07: // inc_octave
            W(wMusicOctave + c, R(wMusicOctave + c) + 1);
            break;
        case 0x08: // dec_octave
            W(wMusicOctave + c, R(wMusicOctave + c) - 1);
            break;
        case 0x09: // tie
            W(wMusicTie + c, 0x80);
            break;
        case 0x0c: // stereo_panning
        {
            u8 v = R(hl++), mask = 0xee, n = c;
            while (n--)
            {
                v = rlc(v);
                mask = rlc(mask);
            }
            W(wMusicStereoPanning, (R(wMusicStereoPanning) & mask) | v);
            break;
        }
        case 0x0d: // MainLoop
            WR16(wMusicMainLoopStart + c * 2, hl - 1);
            break;
        case 0x0e: // EndMainLoop
            hl = RD16(wMusicMainLoopStart + c * 2);
            break;
        case 0x0f: // Loop
            sp = RD16(wMusicChannelStackPointers + c * 2);
            a = R(hl++);
            WR16(sp, hl);
            W(sp + 2, a);
            WR16(wMusicChannelStackPointers + c * 2, sp + 3);
            break;
        case 0x10: // EndLoop
            sp = RD16(wMusicChannelStackPointers + c * 2);
            a = R(sp - 1) - 1;
            if (a == 0)
            {
                WR16(wMusicChannelStackPointers + c * 2, sp - 3);
            }
            else
            {
                W(sp - 1, a);
                hl = RD16(sp - 3);
            }
            break;
        case 0x11: // jp
            hl = RD16(hl);
            break;
        case 0x12: // call
            sp = RD16(wMusicChannelStackPointers + c * 2);
            WR16(sp, hl);
            WR16(wMusicChannelStackPointers + c * 2, sp + 2);
            hl = RD16(hl);
            break;
        case 0x13: // ret
            sp = RD16(wMusicChannelStackPointers + c * 2);
            de = RD16(sp - 2) + 2;
            WR16(wMusicChannelStackPointers + c * 2, sp - 2);
            hl = de;
            break;
        case 0x14: // frequency_offset
            W(wMusicFrequencyOffset + c, R(hl++));
            break;
        case 0x15: // duty
            W(wMusicDuty1 + c, R(hl++) & 0xc0);
            break;
        case 0x16: // volume_envelope
            W(wMusicVolume + c, R(hl++));
            break;
        case 0x17: // wave
            W(wMusicWave, R(hl++));
            W(wMusicWaveChange, 1);
            break;
        case 0x18: // cutoff
            W(wMusicCutoff + c, R(hl++));
            break;
        case 0x19: // echo
            W(wMusicEcho + c, R(hl++));
            break;
        case 0x1a: // vibrato_type
            a = R(hl++);
            W(wMusicVibratoType + c, a);
            W(wMusicVibratoType2 + c, a);
            break;
        case 0x1b: // vibrato_delay
            W(wMusicVibratoDelay + c, R(hl++));
            break;
        case 0x1c: // pitch_offset
            W(wMusicPitchOffset + c, R(hl++));
            break;
        case 0x1d: // adjust_pitch_offset
            W(wMusicPitchOffset + c, R(wMusicPitchOffset + c) + R(hl++));
            break;
        default: // end
            W(wMusicIsPlaying + c, 0);
            return;
        }
    }
}

// (Music1_f4714, f475a) channels 1 and 2's note: its envelope (restarted unless tied), duty and pitch
static void Music1_StartPulseNote(u8 c)
{
    u16 base = c == 0 ? rAUD1LEN : rAUD2LEN;
    u8 d = 0;
    if (R(wdd8c) & (1 << c))
        return;
    if (R(wddb7 + c) == 0)
    {
        W(wMusicTie + c, 0);
        WR(base + 1, 0x08);
        WR(base + 3, 0x80);
        return;
    }
    if (R(wMusicTie + c) != 0x80)
    {
        WR(base + 1, R(wMusicVolume + c));
        d = 0x80;
    }
    W(wMusicTie + c, 2);
    if (c == 0)
        WR(rAUD1SWEEP, 0x08);
    WR(base, R(wMusicDuty1 + c));
    WR(base + 2, R(wMusicCh1CurPitch + c * 2));
    WR(base + 3, R(wMusicCh1CurOctave + c * 2) | d);
}

static void Music1_LoadWaveInstrument(void)
{
    u16 hl = RD16(gTcgWaveInstruments + R(wMusicWave) * 2);
    u32 i;
    for (i = 0; i < 16; i++)
        WR(AUD3WAVERAM + i, R(hl + i));
    W(wMusicWaveChange, 0);
}

// (Music1_f479c) channel 3's
static void Music1_StartWaveNote(void)
{
    u8 d = 0;
    if (R(wdd8c) & 4)
        return;
    if (R(wMusicWaveChange))
    {
        WR(rAUD3ENA, 0);
        Music1_LoadWaveInstrument();
        d = 0x80;
    }
    if (R(wddb9) == 0)
    {
        W(wMusicTie, 0);        // (the game's: channel 1's tie, not 3's)
        WR(rAUD3ENA, 0);
        return;
    }
    if (R(wMusicTie + 2) != 0x80)
    {
        WR(rAUD3LEVEL, R(wMusicVolume + 2));
        WR(rAUD3ENA, 0);
        d = 0x80;
    }
    W(wMusicTie + 2, 2);
    WR(rAUD3LEN, 0);
    WR(rAUD3LOW, R(wMusicCh3CurPitch));
    WR(rAUD3ENA, 0x80);
    WR(rAUD3HIGH, R(wMusicCh3CurOctave) | d);
}

static void Music1_StopNoise(void)
{
    W(wddef, 0);
    WR(rAUD4ENV, 0x08);
    WR(rAUD4GO, 0x80);
}

// (Music1_f480a) channel 4's
static void Music1_StartNoiseNote(void)
{
    if (R(wdd8c) & 8)
        return;
    if (R(wddba) == 0)
    {
        Music1_StopNoise();
        return;
    }
    WR(rAUD4LEN, R(wddab));
    WR(rAUD4ENV, R(wddac));
    WR(rAUD4POLY, R(wddad));
    WR(rAUD4GO, R(wddae));
}

// (Music1_f4839) the noise instrument's next poly value, each frame
static void Music1_UpdateNoise(void)
{
    u16 de;
    u8 a;
    if (R(wdd8c) & 8)
    {
        W(wddef, 0);
        return;
    }
    de = RD16(wdded);
    a = R(de);
    if (a == 0xff)
    {
        Music1_StopNoise();
        return;
    }
    WR(rAUD4POLY, a);
    WR16(wdded, de + 1);
}

static u16 Music1_UpdateVibrato(u8 c)
{
    u8 a = R(wMusicVibratoDelay + c);
    u16 de = RD16(wMusicCh1CurPitch + c * 2);
    u32 guard = 0;
    if (a == 0)
        return de;
    if (R(wdde3 + c) != a)
    {
        W(wdde3 + c, R(wdde3 + c) + 1);
        return de;
    }
    for (;;)
    {
        u16 table = RD16(gTcgVibratoTypes + R(wMusicVibratoType + c) * 2);
        u8 idx = R(wdddb + c);
        W(wdddb + c, idx + 1);
        a = R(table + idx);
        if (a != 0x80)
        {
            u8 e = de & 0xFF, dd = de >> 8;
            if (!(a & 0x80))
            {
                u16 s = e + a;
                e = s;
                dd = (dd + (s >> 8)) & 7;
            }
            else
            {
                u8 n = -a;
                dd = (dd - (e < n)) & 7;
                e -= n;
            }
            return e | (dd << 8);
        }
        // (the type's end: back to its start, or on to the type after the $80)
        W(wdddb + c, 0);
        a = R(table + idx + 1);
        if (a != 0x80)
            W(wMusicVibratoType + c, a);
        if (++guard > 4)
            return de;
    }
}

// (Music1_f490b) the vibrato's pitch to the channel
static void Music1_ApplyVibrato(u8 c, u16 de)
{
    u8 e = de & 0xFF, d = de >> 8;
    if (c > 2 || R(wMusicVibratoDelay + c) == 0 || (R(wdd8c) & (1 << c)))
        return;
    if (c == 0)
    {
        WR(rAUD1LOW, e);
        WR(rAUD1LEN, ReadReg(rAUD1LEN) & 0xc0);
        WR(rAUD1HIGH, d & 0x3f);
    }
    else if (c == 1)
    {
        WR(rAUD2LOW, e);
        WR(rAUD2LEN, ReadReg(rAUD2LEN) & 0xc0);
        WR(rAUD2HIGH, d);
    }
    else
    {
        WR(rAUD3LOW, e);
        WR(rAUD3LEN, 0);
        WR(rAUD3HIGH, d);
    }
}

static void Music1_f485a(u8 c)
{
    Music1_ApplyVibrato(c, Music1_UpdateVibrato(c));
}

static void Music1_SilenceChannel(u8 c)
{
    if (R(wdd8c) & (1 << c))
        return;
    switch (c)
    {
    case 0:
        WR(rAUD1ENV, 0x08);
        WR(rAUD1HIGH, 0x80);
        break;
    case 1:
        WR(rAUD2ENV, 0x08);
        WR(rAUD2HIGH, 0x80);
        break;
    case 2:
        WR(rAUD3LEVEL, 0);
        WR(rAUD3HIGH, 0x80);
        break;
    default:
        Music1_StopNoise();
        break;
    }
}

// (Music1_UpdateChannel1-3)
static void Music1_UpdateTonalChannel(u8 c)
{
    if (!R(wMusicIsPlaying + c))
    {
        Music1_SilenceChannel(c);
        return;
    }
    if (R(wddb7 + c) != 0)
    {
        u8 n = R(wddc3 + c) - 1;
        W(wddc3 + c, n);
        if (n == 0 && R(wddbb + c) != 1 && !(R(wdd8c) & (1 << c)))
        {
            // (the cutoff: the note fades to its echo)
            if (c == 2)
            {
                WR(rAUD3LEVEL, R(wMusicEcho + 2));
            }
            else
            {
                WR(c == 0 ? rAUD1ENV : rAUD2ENV, R(wMusicEcho + c));
                WR(c == 0 ? rAUD1HIGH : rAUD2HIGH, 0x80);
            }
        }
    }
    W(wddbb + c, R(wddbb + c) - 1);
    if (R(wddbb + c) == 0)
    {
        Music1_PlayNextNote(RD16(wMusicChannelPointers + c * 2), c);
        if (!R(wMusicIsPlaying + c))
        {
            Music1_SilenceChannel(c);
            return;
        }
        if (c == 2)
            Music1_StartWaveNote();
        else
            Music1_StartPulseNote(c);
    }
    Music1_f485a(c);
}

static void Music1_UpdateChannel4(void)
{
    if (!R(wMusicIsPlaying + 3))
    {
        if (!(R(wdd8c) & 8))
            Music1_StopNoise();
        return;
    }
    W(wddbb + 3, R(wddbb + 3) - 1);
    if (R(wddbb + 3) == 0)
    {
        Music1_PlayNextNote(RD16(wMusicChannelPointers + 6), 3);
        if (!R(wMusicIsPlaying + 3))
        {
            if (!(R(wdd8c) & 8))
                Music1_StopNoise();
            return;
        }
        Music1_StartNoiseNote();
        return;
    }
    if (R(wddef))
        Music1_UpdateNoise();
}

// (Music1_f4980) every channel the sound effect isn't using: quiet
static void Music1_SilenceAll(void)
{
    u8 d = R(wdd8c);
    if (!(d & 1))
    {
        WR(rAUD1ENV, 0x08);
        WR(rAUD1HIGH, 0x80);
    }
    if (!(d & 2))
    {
        WR(rAUD2ENV, 0x08);
        WR(rAUD2HIGH, 0x80);
    }
    if (!(d & 8))
    {
        WR(rAUD4ENV, 0x08);
        WR(rAUD4GO, 0x80);
    }
    if (!(d & 4))
        WR(rAUD3LEVEL, 0);
}

// (Music1_f4866) the volume and each channel's left/right outputs (the sound effect's for its channels)
static void Music1_UpdatePanning(void)
{
    u8 a, d, e;
    WR(rAUDVOL, R(wMusicPanning));
    a = R(wMusicStereoPanning);
    if (R(wdd8c))
    {
        d = R(wdd8c) & 0xf;
        d |= d << 4;
        e = ~d;
        a = (R(wdd85) & d) | (R(wMusicStereoPanning) & e);
    }
    e = ~R(wddf0) & 0xf;
    e |= e << 4;
    WR(rAUDTERM, e & a);
}

static void Music1_CheckForEndOfSong(void)
{
    if ((u8)(R(wMusicIsPlaying) + R(wMusicIsPlaying + 1) + R(wMusicIsPlaying + 2) + R(wMusicIsPlaying + 3)) == 0)
        W(wCurSongID, 0x80);
}

static void Music1_Update(void)
{
    Music1_CheckForNewSound();
    SFX_Update();
    if (R(wddf2))
    {
        Music1_SilenceAll();
    }
    else
    {
        Music1_UpdateTonalChannel(0);
        Music1_UpdateTonalChannel(1);
        Music1_UpdateTonalChannel(2);
        Music1_UpdateChannel4();
    }
    Music1_UpdatePanning();
    Music1_CheckForEndOfSong();
}

// ---- audio/sfx.asm ------------------------------------------------------------------------------------------

static void SFX_Done(void)  // (Func_fc26c)
{
    W(wSFXIsPlaying, 0);
    W(wSfxPriority, 0);
    W(wCurSfxID, 0x80);
}

static void SFX_Play(u8 a)
{
    u16 hl;
    u8 mask;
    u32 c;
    if (a >= TCG_NUM_SFX)
        return;
    if (R(wSFXIsPlaying))
        W(wdd8c, 0);        // (Func_fc279: the game meant to quiet the channels, but only reads them)
    W(wSFXIsPlaying, TRUE);
    hl = gTcgSfxHeaders[a];
    mask = R(hl++);
    W(wdd8c, mask);
    W(wde54, mask);
    for (c = 0; c < 4; c++)
    {
        if (mask & 1)
        {
            WR16(wSFXCommandPointers + c * 2, RD16(hl));
            hl += 2;
            W(wSFXPitchOffsets + c, 0);
            W(wde33 + c, 1);
        }
        mask >>= 1;
    }
}

static void SFX_SetPointer(u8 c, u16 de)  // (Func_fc105)
{
    WR16(wSFXCommandPointers + c * 2, de);
}

// the channel's NRx1 (+1: NRx2, +2: NRx3, +3: NRx4)
static u16 SFX_ChannelRegs(u8 c)
{
    return rAUD1LEN + c * 5;
}

static void SFX_WriteFrequency(u8 c, u8 e, u8 d)
{
    u16 r = SFX_ChannelRegs(c);
    WR(r, ReadReg(r) & 0xc0);
    WR(r + 2, e);
    WR(r + 3, d);
}

static void SFX_ApplyPitchOffset(u8 c)
{
    u8 a = R(wSFXPitchOffsets + c), lo, hi, d;
    u16 p = wde37 + c * 2;
    if (a == 0)
        return;
    lo = R(p);
    hi = R(p + 1);
    if (!(a & 0x80))
    {
        u16 s = lo + a;
        lo = s;
        hi += s >> 8;
    }
    else
    {
        u8 n = -a;
        hi -= lo < n;
        lo -= n;
    }
    W(p, lo);
    W(p + 1, hi);
    d = R(wde2b + c);
    W(wde2b + c, 0);
    SFX_WriteFrequency(c, lo, hi | d);
}

// (Func_fc1cd) the noise channel's
static void SFX_ApplyNoisePitchOffset(void)
{
    u8 a = R(wde32), e, n, d;
    if (a == 0)
        return;
    e = R(wde3d);
    n = (a & 0x80) ? e - (u8)-a : e + a;
    W(wde3d, n);
    d = n;
    a = ((n ^ e) & 8) << 4;
    e = R(wde2e);
    W(wde2e, 0);
    WR(rAUD4LEN, 0);
    WR(rAUD4POLY, d);
    WR(rAUD4GO, a | e);
}

static void SFX_ExecuteNextCommand(u16 hl, u8 c)
{
    u32 guard;
    for (guard = 0; guard < 256; guard++)
    {
        u8 a = R(hl++), lo = a & 0xf;
        switch (a >> 4)
        {
        case 0x0: // frequency
        {
            u8 d = lo, e = R(hl++), b;
            u16 p = wde37 + c * 2;
            b = R(p);
            W(p, e);
            W(p + 1, d);
            if (c == 3)
                d = ((b ^ e) & 8) << 4;
            a = R(wde2b + c);
            W(wde2b + c, 0);
            SFX_WriteFrequency(c, e, d | a);
            SFX_SetPointer(c, hl);
            return;
        }
        case 0x1: // envelope
            W(wde2b + c, 0x80);
            WR(SFX_ChannelRegs(c) + 1, R(hl++));
            break;
        case 0x2: // duty
            WR(SFX_ChannelRegs(c), lo << 4);
            break;
        case 0x3: // loop
            a = R(hl++);
            WR16(wde43 + c * 2, hl);
            W(wde3f + c, a);
            break;
        case 0x4: // endloop
            a = R(wde3f + c) - 1;
            if (a != 0)
            {
                W(wde3f + c, a);
                hl = RD16(wde43 + c * 2);
            }
            break;
        case 0x5: // pitch_offset
            W(wSFXPitchOffsets + c, R(hl++));
            break;
        case 0x6: // wait
            if (c == 3)
                SFX_ApplyNoisePitchOffset();
            else
                SFX_ApplyPitchOffset(c);
            W(wde33 + c, R(hl++));
            SFX_SetPointer(c, hl);
            return;
        case 0x7: // wave
        {
            u16 w = RD16(gTcgWaveInstruments + lo * 2);
            u32 i;
            WR(rAUD3ENA, 0);
            for (i = 0; i < 16; i++)
                WR(AUD3WAVERAM + i, R(w + i));
            W(wMusicWaveChange, 1);
            WR(rAUD3ENA, 0x80);
            break;
        }
        case 0x8: // pan
        {
            u8 v = R(hl++), mask = 0xee, n = c;
            while (n--)
            {
                v = rlc(v);
                mask = rlc(mask);
            }
            W(wdd85, (R(wdd85) & mask) | v);
            break;
        }
        case 0xf: // end
        {
            u16 r = rAUD1ENV + c * 5;
            W(wdd8c, R(wdd8c) & ~(1 << c));
            WR(r, 0x08);
            WR(r + 2, 0x80);
            return;
        }
        default: // (unused)
            break;
        }
    }
}

static void SFX_Update(void)
{
    u32 c;
    u8 mask;
    if (!R(wdd8c))
    {
        SFX_Done();
        return;
    }
    mask = R(wdd8c);
    W(wde54, mask);
    for (c = 0; c < 4; c++, mask >>= 1)
    {
        u8 n;
        if (!(mask & 1))
            continue;
        n = R(wde33 + c) - 1;
        if (n != 0)
        {
            W(wde33 + c, n);
            SFX_ApplyPitchOffset(c);
        }
        else
        {
            SFX_ExecuteNextCommand(RD16(wSFXCommandPointers + c * 2), c);
        }
    }
}

// ---- the game's calls (home/sound.asm) ----------------------------------------------------------------------

void TcgSound_Start(void)
{
    if (sActive)
        return;
    sSavedSoundCntL = REG_SOUNDCNT_L;
    sSavedSoundCntH = REG_SOUNDCNT_H;
    REG_SOUNDCNT_H = (REG_SOUNDCNT_H & ~3) | SOUND_CGB_MIX_FULL;
    memset(sRegs, 0, sizeof(sRegs));
    Music1_Init();
    sActive = TRUE;
}

void TcgSound_Stop(void)
{
    if (!sActive)
        return;
    sActive = FALSE;
    W(wdd8c, 0);
    Music1_StopAllChannels();
    WR(rAUD3ENA, 0);
    REG_SOUNDCNT_L = sSavedSoundCntL;
    REG_SOUNDCNT_H = sSavedSoundCntH;
}

bool32 TcgSound_IsActive(void)
{
    return sActive;
}

void TcgSound_Update(void)
{
    // (a fanfare, as "giveitem"'s: the song waits, quiet, and leaves the channels to it until it's done)
    bool32 m4aMusic = !IsFanfareTaskInactive();
    if (!sActive)
        return;
    if (m4aMusic)
    {
        if (!sWaitingForM4a)
            Music1_SilenceAll();
        sWaitingForM4a = TRUE;
        return;
    }
    sWaitingForM4a = FALSE;
    Music1_Update();
}

void Tcg_PlaySong(u32 song)
{
    if (song < TCG_NUM_SONGS)
        W(wCurSongID, song);
}

u32 Tcg_GetPlayingSong(void)
{
    return R(wCurSongID) & 0x7f;
}

void Tcg_PlaySFX(u32 sfx)
{
    if (sfx && R(wSfxPriority) && R(wSfxPriority) < SfxPriority(sfx))
        return;
    W(wSfxPriority, SfxPriority(sfx));
    W(wCurSfxID, sfx);
}

void Tcg_PlaySFX_InvalidChoice(void)
{
    Tcg_PlaySFX(TCG_SFX_DENIED);
}

// (AssertSongFinished) TRUE while a song's playing
bool32 Tcg_IsSongPlaying(void)
{
    return R(wCurSongID) != 0x80;
}

bool32 Tcg_IsSFXPlaying(void)
{
    return R(wCurSfxID) != 0x80;
}
