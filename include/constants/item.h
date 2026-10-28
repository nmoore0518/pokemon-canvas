#ifndef GUARD_ITEM_CONSTANTS_H
#define GUARD_ITEM_CONSTANTS_H

// Gym Trainer hack: ten pockets, in the bag's order. The item data still names only the first five
// kinds (ITEMS, POKE_BALLS, TM_HM, BERRIES, KEY_ITEMS); GetItemPocket (src/item.c) sorts an ITEMS
// item into MEDICINE, BATTLE ITEMS, HOLD ITEMS or MEGA STONES by its sort type, and a key item
// that does something when used into TOOLS.
enum Pocket
{
    POCKET_ITEMS,
    POCKET_MEDICINE,
    POCKET_POKE_BALLS,
    POCKET_BATTLE_ITEMS,
    POCKET_HOLD_ITEMS,
    POCKET_MEGA_STONES,
    POCKET_BERRIES,
    POCKET_TM_HM,
    POCKET_TOOLS,
    POCKET_KEY_ITEMS,
    POCKETS_COUNT,
    POCKET_DUMMY = POCKETS_COUNT,
};

#define REPEL_LURE_MASK         (1 << 15)
#define IS_LAST_USED_LURE(var)  (var & REPEL_LURE_MASK)
#define REPEL_LURE_STEPS(var)   (var & (REPEL_LURE_MASK - 1))
#define LURE_STEP_COUNT         (IS_LAST_USED_LURE(VarGet(VAR_REPEL_STEP_COUNT)) ? REPEL_LURE_STEPS(VarGet(VAR_REPEL_STEP_COUNT)) : 0)
#define REPEL_STEP_COUNT        (!IS_LAST_USED_LURE(VarGet(VAR_REPEL_STEP_COUNT)) ? REPEL_LURE_STEPS(VarGet(VAR_REPEL_STEP_COUNT)) : 0)

#define ITEM_SELL_FACTOR ((I_SELL_VALUE_FRACTION >= GEN_9) ? 4 : 2)

#endif // GUARD_ITEM_CONSTANTS_H
