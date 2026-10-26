// Gym Trainer hack: the Lucky Egg and Amulet Coin level up (like Pokémon Unbound's), so training a
// team to level 100 for the tournament doesn't take forever.
//
// The level belongs to the player, not to one copy of the item: every Lucky Egg they hold works at
// VAR_LUCKY_EGG_LEVEL, every Amulet Coin at VAR_AMULET_COIN_LEVEL. Both start at 1 (a var of 0, so
// old saves are level 1) and go up to ITEM_UPGRADE_MAX_LEVEL. Each level adds 0.5x:
//   Lucky Egg    1.5x EXP at level 1 (as usual) ... 6x at level 10
//   Amulet Coin  2x prize money at level 1 (as usual) ... 6.5x at level 10
// How the player earns the upgrades isn't decided yet: scripts call the specials below
// (special ItemUpgrade_Upgrade with VAR_0x8004 = ITEM_LUCKY_EGG or ITEM_AMULET_COIN).
#include "global.h"
#include "item_upgrades.h"
#include "battle.h"
#include "event_data.h"
#include "item.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/items.h"

// (percent, by level; index 0 is level 1)
static const u16 sLuckyEggPercent[ITEM_UPGRADE_MAX_LEVEL] = {150, 200, 250, 300, 350, 400, 450, 500, 550, 600};
static const u16 sAmuletCoinPercent[ITEM_UPGRADE_MAX_LEVEL] = {200, 250, 300, 350, 400, 450, 500, 550, 600, 650};

static u16 VarFor(u32 itemId)
{
    switch (itemId)
    {
    case ITEM_LUCKY_EGG:   return VAR_LUCKY_EGG_LEVEL;
    case ITEM_AMULET_COIN: return VAR_AMULET_COIN_LEVEL;
    default:               return 0;
    }
}

u32 ItemUpgrade_GetLevel(u32 itemId)
{
    u16 var = VarFor(itemId);
    if (var == 0)
        return 1;
    return max(1u, min((u32)VarGet(var), (u32)ITEM_UPGRADE_MAX_LEVEL));
}

bool32 ItemUpgrade_SetLevel(u32 itemId, u32 level)
{
    u16 var = VarFor(itemId);
    if (var == 0 || level < 1 || level > ITEM_UPGRADE_MAX_LEVEL)
        return FALSE;
    VarSet(var, level);
    return TRUE;
}

u32 ItemUpgrade_ExpPercent(void)
{
    return sLuckyEggPercent[ItemUpgrade_GetLevel(ITEM_LUCKY_EGG) - 1];
}

u32 ItemUpgrade_MoneyPercent(void)
{
    return sAmuletCoinPercent[ItemUpgrade_GetLevel(ITEM_AMULET_COIN) - 1];
}

u32 ItemUpgrade_ApplyMoney(u32 money)
{
    if (gBattleStruct->moneyMultiplierItem)
        money = money * ItemUpgrade_MoneyPercent() / 100;
    return money;
}

// "2.5" from 250
static u8 *WriteMultiplier(u8 *dst, u32 percent)
{
    dst = ConvertIntToDecimalStringN(dst, percent / 100, STR_CONV_MODE_LEFT_ALIGN, 2);
    if (percent % 100)
    {
        *dst++ = CHAR_PERIOD;
        dst = ConvertIntToDecimalStringN(dst, (percent % 100) / 10, STR_CONV_MODE_LEFT_ALIGN, 1);
    }
    *dst = EOS;
    return dst;
}

// The bag's description, with the level: "Lv. 3: the holder\nearns 2.5x the\nExp. Points."
static EWRAM_DATA u8 sDescription[64] = {0};

const u8 *ItemUpgrade_GetDescription(u32 itemId)
{
    u8 *s = sDescription;
    if (VarFor(itemId) == 0)
        return NULL;
    s = StringCopy(s, COMPOUND_STRING("Lv. "));
    s = ConvertIntToDecimalStringN(s, ItemUpgrade_GetLevel(itemId), STR_CONV_MODE_LEFT_ALIGN, 2);
    if (itemId == ITEM_LUCKY_EGG)
    {
        s = StringCopy(s, COMPOUND_STRING(": the holder\nearns "));
        s = WriteMultiplier(s, ItemUpgrade_ExpPercent());
        StringCopy(s, COMPOUND_STRING("x the\nExp. Points."));
    }
    else
    {
        s = StringCopy(s, COMPOUND_STRING(": prize money\nis "));
        s = WriteMultiplier(s, ItemUpgrade_MoneyPercent());
        StringCopy(s, COMPOUND_STRING("x if the\nholder battles."));
    }
    return sDescription;
}

// Special: VAR_0x8004 = ITEM_LUCKY_EGG or ITEM_AMULET_COIN, one level up.
// VAR_RESULT = the new level, or 0 if it was at the top already
void ItemUpgrade_Upgrade(void)
{
    u32 level = ItemUpgrade_GetLevel(gSpecialVar_0x8004);
    gSpecialVar_Result = 0;
    if (level < ITEM_UPGRADE_MAX_LEVEL && ItemUpgrade_SetLevel(gSpecialVar_0x8004, level + 1))
        gSpecialVar_Result = level + 1;
}

// Special: VAR_0x8004 = the item. STR_VAR_1 = its level, STR_VAR_2 = its multiplier ("2.5");
// VAR_RESULT = the level
void ItemUpgrade_BufferLevel(void)
{
    u32 level = ItemUpgrade_GetLevel(gSpecialVar_0x8004);
    ConvertIntToDecimalStringN(gStringVar1, level, STR_CONV_MODE_LEFT_ALIGN, 2);
    WriteMultiplier(gStringVar2, gSpecialVar_0x8004 == ITEM_AMULET_COIN ? ItemUpgrade_MoneyPercent() : ItemUpgrade_ExpPercent());
    gSpecialVar_Result = level;
}
