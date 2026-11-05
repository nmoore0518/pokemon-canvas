#ifndef GUARD_PAINTER_H
#define GUARD_PAINTER_H

// Pokémon Canvas: the painter in Pewter City and her Smeargle (src/painter.c)
struct WarpData;

bool32 Painter_IsCanvasSmeargle(u32 species);
u32 Painter_SmeargleOfType(u32 type);
u32 Painter_Offer(void);
bool32 Painting_IsActive(void);
bool32 Painting_HidesObject(const u8 *script);
bool32 Painting_BlocksCoords(s16 x, s16 y);
void Painting_RedirectWarp(struct WarpData *warp);
void Painting_TintPalettes(u32 offset, u32 count, bool32 skipFaded);
bool32 Painter_SketchAllowed(u32 move);
bool32 Painter_QuickSketchCanCopy(u32 battler, u32 move);
u32 Painter_QuickSketchMove(u32 attacker, u32 target);
extern u16 gQuickSketchChoice[];
// (specials)
void Painter_PrepareSketch(void);
void Painter_Return(void);
void Painter_GiveSmeargle(void);

#endif // GUARD_PAINTER_H
