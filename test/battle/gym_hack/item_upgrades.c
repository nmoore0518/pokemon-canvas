#include "global.h"
#include "test/battle.h"
#include "item_upgrades.h"

// Gym Trainer hack: the Lucky Egg's level (src/item_upgrades.c)
WILD_BATTLE_TEST("GymHack: a level 10 Lucky Egg gives 6x the EXP", s32 exp)
{
    enum Item item = ITEM_NONE;
    u32 level = 1;

    PARAMETRIZE { item = ITEM_NONE; level = 10; }
    PARAMETRIZE { item = ITEM_LUCKY_EGG; level = 1; }
    PARAMETRIZE { item = ITEM_LUCKY_EGG; level = 10; }

    GIVEN {
        ItemUpgrade_SetLevel(ITEM_LUCKY_EGG, level);
        PLAYER(SPECIES_WOBBUFFET) { Level(20); Item(item); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); HP(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        MESSAGE("The wild Caterpie fainted!");
        EXPERIENCE_BAR(player, captureGainedExp: &results[i].exp);
    } THEN {
        ItemUpgrade_SetLevel(ITEM_LUCKY_EGG, 1);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].exp, Q_4_12(1.5), results[1].exp);
        EXPECT_MUL_EQ(results[0].exp, Q_4_12(6), results[2].exp);
    }
}
