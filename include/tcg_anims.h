#ifndef GUARD_TCG_ANIMS_H
#define GUARD_TCG_ANIMS_H

// Pokémon Canvas: the Trading Card Game duel's animation data (src/data/tcg/anims.h, from
// tools/gym_hack/tcg/extract_anims.py), as the Game Boy Color game has it.
struct TcgSprite
{
    const u8 *tiles;            // (2bpp)
    u8 numTiles;
};

struct TcgSpriteAnim
{
    const u8 *const *frameTable;    // each frame: a count, then (y, x, tile, attributes) each
    const s8 *frameData;            // (frame index, count, dx, dy) each; a 0 count loops
};

extern const struct TcgSprite gTcgSprites[];
extern const u8 gTcgPlayerPortraitGfx[];      // (6x6 tiles each)
extern const u8 gTcgRodPortraitGfx[];
extern const struct TcgSpriteAnim gTcgSpriteAnims[];
extern const u16 *const gTcgPalettes[];         // (each: the number of palettes, then 4 colours each)
extern const u8 gTcgDuelAnimations[][6];        // (sprite, palette, sprite animation, flags, sfx, handler)
extern const u8 *const gTcgAttackAnimations[];  // (ANIMCMD_* commands)

#endif // GUARD_TCG_ANIMS_H
