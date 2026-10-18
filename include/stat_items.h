#ifndef GUARD_STAT_ITEMS_H
#define GUARD_STAT_ITEMS_H

// Gym Trainer hack: Silver/Gold Bottle Cap, Ability Capsule, Cilantro, Multivitamin

bool32 StatItems_MaxIv(struct Pokemon *mon, u32 stat);
bool32 StatItems_MaxAllIvs(struct Pokemon *mon);
u32 StatItems_GetAbilityChoices(struct Pokemon *mon, u8 *slots);
bool32 StatItems_SetAbilitySlot(struct Pokemon *mon, u32 slot);
bool32 StatItems_SetNature(struct Pokemon *mon, u32 nature);
bool32 StatItems_EvsAreValid(const u16 *evs);
bool32 StatItems_SetEvs(struct Pokemon *mon, const u16 *evs);
const u16 *StatItems_GetPalletMartItems(void);

// Specials
void StatItems_CheckMon(void);
void StatItems_PushIvList(void);
void StatItems_PushAbilityList(void);
void StatItems_PushNatureList(void);
void StatItems_Apply(void);
void StatItems_OpenEvEditor(void);
void GymHack_PalletMart(void);

// Bag
void ItemUseOutOfBattle_StatItem(u8 taskId);

#endif // GUARD_STAT_ITEMS_H
