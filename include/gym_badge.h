#ifndef GUARD_GYM_BADGE_H
#define GUARD_GYM_BADGE_H

// Gym Trainer hack: the player's own gym badge (src/gym_badge.c)

struct MapLayout;

const u8 *GymBadge_GetName(u32 type);
bool32 GymBadge_HasArt(u32 type);
void GymBadge_PatchTilesetGfx(struct MapLayout const *mapLayout);
void GymBadge_PatchTilesetPalette(struct MapLayout const *mapLayout, bool8 skipFaded);

// Specials
void GymBadge_BufferName(void);
void GymBadge_ShowIcon(void);
void GymBadge_HideIcon(void);

#endif // GUARD_GYM_BADGE_H
