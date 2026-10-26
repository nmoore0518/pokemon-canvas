#ifndef GUARD_ITEM_UPGRADES_H
#define GUARD_ITEM_UPGRADES_H

// Gym Trainer hack: the Lucky Egg and Amulet Coin have levels (1-10), kept for the player (not per
// item): VAR_LUCKY_EGG_LEVEL and VAR_AMULET_COIN_LEVEL. See src/item_upgrades.c.
#define ITEM_UPGRADE_MAX_LEVEL 10

u32 ItemUpgrade_GetLevel(u32 itemId);
bool32 ItemUpgrade_SetLevel(u32 itemId, u32 level);
u32 ItemUpgrade_ExpPercent(void);       // the Lucky Egg's EXP: 150 = 1.5x
u32 ItemUpgrade_MoneyPercent(void);     // the Amulet Coin's prize money: 200 = 2x
u32 ItemUpgrade_ApplyMoney(u32 money);  // (when the Amulet Coin was in the battle)
const u8 *ItemUpgrade_GetDescription(u32 itemId);   // NULL for other items

// Specials
void ItemUpgrade_Upgrade(void);
void ItemUpgrade_BufferLevel(void);

#endif // GUARD_ITEM_UPGRADES_H
