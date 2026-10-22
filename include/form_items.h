#ifndef GUARD_FORM_ITEMS_H
#define GUARD_FORM_ITEMS_H

// Pokémon Canvas: items for forms that the world gives out in its own ways (src/form_items.c)
bool32 FormItems_ObjectWaits(u16 flagId);          // (Genesect's Drives: only once it's caught)
const u8 *FormItems_GetPlateScript(s32 x, s32 y);  // (Arceus's plates, hidden in the gym)

#endif // GUARD_FORM_ITEMS_H
