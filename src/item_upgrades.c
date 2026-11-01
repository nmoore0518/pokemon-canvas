// Gym Trainer hack: the Lucky Egg and Amulet Coin level up (like Pokémon Unbound's), so training a
// team to level 100 for the tournament doesn't take forever.
//
// The level belongs to the player, not to one copy of the item: every Lucky Egg they hold works at
// VAR_LUCKY_EGG_LEVEL, every Amulet Coin at VAR_AMULET_COIN_LEVEL. Both start at 1 (a var of 0, so
// old saves are level 1) and go up to ITEM_UPGRADE_MAX_LEVEL. Each level adds 0.5x:
//   Lucky Egg    1.5x EXP at level 1 (as usual) ... 6x at level 10
//   Amulet Coin  2x prize money at level 1 (as usual) ... 6.5x at level 10
// Each level costs materials (sCosts):
//   the Lucky Egg   clovers, left at the Chansey statue in Pallet Town
//   the Amulet Coin old coins, which Daisy sets into it (Grandpa Oak brought them back from his trips)
// Levels 2-4 take the common one (1, 2, 3 of them), 5-7 the rare one (1, 2, 3), 8-10 the very rare
// one (1, 1, 2). The materials are hidden items (tools/gym_hack/place_upgrade_finds.py) and wild
// Pokémon hold them (TryGiveWildMonUpgradeMaterial).
#include "global.h"
#include "item_upgrades.h"
#include "battle.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/species.h"

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

// ---------------------------------------------------------------------------
// What each level costs
// ---------------------------------------------------------------------------
struct UpgradeCost
{
    u16 item;
    u8 count;
};

// (index: the level it goes up to, 2-10)
static const struct UpgradeCost sLuckyEggCosts[ITEM_UPGRADE_MAX_LEVEL + 1] =
{
    [2] = {ITEM_LUCKY_CLOVER, 1},       [3] = {ITEM_LUCKY_CLOVER, 2},       [4] = {ITEM_LUCKY_CLOVER, 3},
    [5] = {ITEM_FOUR_LEAF_CLOVER, 1},   [6] = {ITEM_FOUR_LEAF_CLOVER, 2},   [7] = {ITEM_FOUR_LEAF_CLOVER, 3},
    [8] = {ITEM_GOLDEN_CLOVER, 1},      [9] = {ITEM_GOLDEN_CLOVER, 1},      [10] = {ITEM_GOLDEN_CLOVER, 2},
};

static const struct UpgradeCost sAmuletCoinCosts[ITEM_UPGRADE_MAX_LEVEL + 1] =
{
    [2] = {ITEM_COPPER_COIN, 1},        [3] = {ITEM_COPPER_COIN, 2},        [4] = {ITEM_COPPER_COIN, 3},
    [5] = {ITEM_SILVER_COIN, 1},        [6] = {ITEM_SILVER_COIN, 2},        [7] = {ITEM_SILVER_COIN, 3},
    [8] = {ITEM_GOLD_COIN, 1},          [9] = {ITEM_GOLD_COIN, 1},          [10] = {ITEM_GOLD_COIN, 2},
};

// the cost of going up to `level` (NULL past the top)
static const struct UpgradeCost *GetCost(u32 itemId, u32 level)
{
    if (level < 2 || level > ITEM_UPGRADE_MAX_LEVEL)
        return NULL;
    if (itemId == ITEM_LUCKY_EGG)
        return &sLuckyEggCosts[level];
    if (itemId == ITEM_AMULET_COIN)
        return &sAmuletCoinCosts[level];
    return NULL;
}

u32 ItemUpgrade_GetCostItem(u32 itemId, u32 level)
{
    const struct UpgradeCost *cost = GetCost(itemId, level);
    return cost ? cost->item : ITEM_NONE;
}

u32 ItemUpgrade_GetCostCount(u32 itemId, u32 level)
{
    const struct UpgradeCost *cost = GetCost(itemId, level);
    return cost ? cost->count : 0;
}

// Special: VAR_0x8004 = the item. Is it in the bag or held by someone in the party?
// VAR_RESULT = TRUE/FALSE
void ItemUpgrade_HasItem(void)
{
    u32 i;
    gSpecialVar_Result = CheckBagHasItem(gSpecialVar_0x8004, 1);
    for (i = 0; i < gPlayerPartyCount && !gSpecialVar_Result; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM) == gSpecialVar_0x8004)
            gSpecialVar_Result = TRUE;
    }
}

// Special: VAR_0x8004 = the item. What the next level costs:
//   VAR_RESULT    ITEM_UPGRADE_MAXED, ITEM_UPGRADE_CAN_PAY, or ITEM_UPGRADE_SHORT
//   VAR_0x8005    the material, VAR_0x8006 how many
//   STR_VAR_1     "2 FOUR-LEAF CLOVERS" (the number and the name, plural if it's more than one)
//   STR_VAR_2     the next level, STR_VAR_3 its multiplier ("3.5")
void ItemUpgrade_CheckCost(void)
{
    u32 next = ItemUpgrade_GetLevel(gSpecialVar_0x8004) + 1;
    const struct UpgradeCost *cost = GetCost(gSpecialVar_0x8004, next);
    u8 *s;

    if (cost == NULL)
    {
        gSpecialVar_Result = ITEM_UPGRADE_MAXED;
        return;
    }
    gSpecialVar_0x8005 = cost->item;
    gSpecialVar_0x8006 = cost->count;
    s = ConvertIntToDecimalStringN(gStringVar1, cost->count, STR_CONV_MODE_LEFT_ALIGN, 1);
    *s++ = CHAR_SPACE;
    CopyItemNameHandlePlural(cost->item, s, cost->count);
    ConvertIntToDecimalStringN(gStringVar2, next, STR_CONV_MODE_LEFT_ALIGN, 2);
    WriteMultiplier(gStringVar3, ((gSpecialVar_0x8004 == ITEM_AMULET_COIN) ? sAmuletCoinPercent : sLuckyEggPercent)[next - 1]);
    gSpecialVar_Result = CheckBagHasItem(cost->item, cost->count) ? ITEM_UPGRADE_CAN_PAY : ITEM_UPGRADE_SHORT;
}

// Special: VAR_0x8004 = the item. Takes the next level's materials from the bag and goes up a level.
// VAR_RESULT = the new level, or 0 (at the top, or not enough materials: nothing changed)
void ItemUpgrade_PayAndUpgrade(void)
{
    u32 next = ItemUpgrade_GetLevel(gSpecialVar_0x8004) + 1;
    const struct UpgradeCost *cost = GetCost(gSpecialVar_0x8004, next);

    gSpecialVar_Result = 0;
    if (cost == NULL || !CheckBagHasItem(cost->item, cost->count))
        return;
    RemoveBagItem(cost->item, cost->count);
    ItemUpgrade_SetLevel(gSpecialVar_0x8004, next);
    gSpecialVar_Result = next;
}

// ---------------------------------------------------------------------------
// Wild Pokémon that hold the materials (src/pokemon.c, SetWildMonHeldItem). Doubled when the
// lead's ability raises the odds of held items (Compound Eyes, Super Luck).
// ---------------------------------------------------------------------------
static bool32 IsSpeciesIn(u32 species, const u16 *list)
{
    for (; *list != SPECIES_NONE; list++)
    {
        if (*list == species)
            return TRUE;
    }
    return FALSE;
}

static const u16 sCloverPlants[] =
{
    SPECIES_ODDISH, SPECIES_GLOOM, SPECIES_BELLSPROUT, SPECIES_WEEPINBELL, SPECIES_PARAS, SPECIES_PARASECT,
    SPECIES_EXEGGCUTE, SPECIES_TANGELA, SPECIES_HOPPIP, SPECIES_SKIPLOOM, SPECIES_SUNKERN, SPECIES_NONE,
};
static const u16 sLuckyFairies[] = {SPECIES_CLEFFA, SPECIES_CLEFAIRY, SPECIES_CLEFABLE, SPECIES_HAPPINY, SPECIES_CHANSEY, SPECIES_BLISSEY, SPECIES_NONE};
static const u16 sGoldenFairies[] = {SPECIES_HAPPINY, SPECIES_CHANSEY, SPECIES_BLISSEY, SPECIES_NONE};
static const u16 sCoinFinders[] = {SPECIES_MEOWTH, SPECIES_MEOWTH_ALOLA, SPECIES_MEOWTH_GALAR, SPECIES_DIGLETT, SPECIES_DIGLETT_ALOLA, SPECIES_NONE};
static const u16 sCoinHoarders[] = {SPECIES_PERSIAN, SPECIES_PERSIAN_ALOLA, SPECIES_PERRSERKER, SPECIES_DUGTRIO, SPECIES_DUGTRIO_ALOLA, SPECIES_NONE};

bool32 TryGiveWildMonUpgradeMaterial(struct Pokemon *mon, bool32 boosted)
{
    u32 species = GetMonData(mon, MON_DATA_SPECIES);
    u32 roll = Random() % 100, mult = boosted ? 2 : 1;
    u16 item = ITEM_NONE;

    if (IsSpeciesIn(species, sGoldenFairies) && roll < WILD_VERY_RARE_MATERIAL_PERCENT * mult)
        item = ITEM_GOLDEN_CLOVER;
    else if (IsSpeciesIn(species, sLuckyFairies) && roll < (WILD_VERY_RARE_MATERIAL_PERCENT + WILD_MATERIAL_PERCENT) * mult)
        item = ITEM_FOUR_LEAF_CLOVER;
    else if (IsSpeciesIn(species, sCloverPlants) && roll < WILD_MATERIAL_PERCENT * mult)
        item = ITEM_LUCKY_CLOVER;
    else if (IsSpeciesIn(species, sCoinHoarders) && roll < WILD_VERY_RARE_MATERIAL_PERCENT * mult)
        item = ITEM_GOLD_COIN;
    else if (IsSpeciesIn(species, sCoinHoarders) && roll < (WILD_VERY_RARE_MATERIAL_PERCENT + WILD_MATERIAL_PERCENT) * mult)
        item = ITEM_SILVER_COIN;
    else if (IsSpeciesIn(species, sCoinFinders) && roll < WILD_MATERIAL_PERCENT * mult)
        item = ITEM_COPPER_COIN;
    if (item == ITEM_NONE)
        return FALSE;
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    return TRUE;
}
