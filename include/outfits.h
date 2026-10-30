#ifndef GUARD_OUTFITS_H
#define GUARD_OUTFITS_H

#include "script_menu.h"

// Pokémon Canvas: the player's outfits (constants/outfits.h)
extern u16 gOutfitPlayerPalettes[GENDER_COUNT][16];            // the player's colors (overworld and back pic)
extern u16 gOutfitPlayerReflectionPalettes[GENDER_COUNT][16];

struct ObjectEventGraphicsInfo;
const struct SpriteFrameImage *Outfit_ObjectImages(const struct ObjectEventGraphicsInfo *info);
u32 Outfit_Get(void);
bool32 Outfit_IsUnlocked(u32 outfit);
void Outfit_Unlock(u32 outfit);
u32 Outfit_ForType(u32 type);
void Outfit_RefreshPalettes(void);
const u16 *Outfit_GetBackPicPalette(bool32 female);
void Outfit_ListOnInit(struct DynamicListMenuEventArgs *eventArgs);
void Outfit_ListOnSelectionChanged(struct DynamicListMenuEventArgs *eventArgs);
void Outfit_ListOnDestroy(struct DynamicListMenuEventArgs *eventArgs);

#endif // GUARD_OUTFITS_H
