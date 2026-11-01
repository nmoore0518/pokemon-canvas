#ifndef GUARD_ITEM_UPGRADES_H
#define GUARD_ITEM_UPGRADES_H

// Gym Trainer hack: the Lucky Egg and Amulet Coin have levels (1-10), kept for the player (not per
// item): VAR_LUCKY_EGG_LEVEL and VAR_AMULET_COIN_LEVEL. See src/item_upgrades.c.
#include "constants/item_upgrades.h"

#define ITEM_UPGRADE_MAX_LEVEL 10

u32 ItemUpgrade_GetLevel(u32 itemId);
bool32 ItemUpgrade_SetLevel(u32 itemId, u32 level);
u32 ItemUpgrade_ExpPercent(void);       // the Lucky Egg's EXP: 150 = 1.5x
u32 ItemUpgrade_MoneyPercent(void);     // the Amulet Coin's prize money: 200 = 2x
u32 ItemUpgrade_ApplyMoney(u32 money);  // (when the Amulet Coin was in the battle)
const u8 *ItemUpgrade_GetDescription(u32 itemId);   // NULL for other items

// What each level costs (src/item_upgrades.c sCosts): ITEM_NONE / 0 past the top
u32 ItemUpgrade_GetCostItem(u32 itemId, u32 level);
u32 ItemUpgrade_GetCostCount(u32 itemId, u32 level);

// Wild Pokémon holding the materials: Grass Pokémon a Lucky Clover, Clefairy and Chansey a
// Four-Leaf (Chansey, rarely, a Golden), Meowth and Diglett a Copper Coin, Persian and Dugtrio a Silver
// (rarely, a Gold)
#define WILD_MATERIAL_PERCENT 5
#define WILD_VERY_RARE_MATERIAL_PERCENT 1
struct Pokemon;
bool32 TryGiveWildMonUpgradeMaterial(struct Pokemon *mon, bool32 boosted);

// Specials
void ItemUpgrade_Upgrade(void);
void ItemUpgrade_BufferLevel(void);
void ItemUpgrade_HasItem(void);
void ItemUpgrade_CheckCost(void);
void ItemUpgrade_PayAndUpgrade(void);

#endif // GUARD_ITEM_UPGRADES_H
