#include "global.h"
#include "test/battle.h"

// Pokémon Canvas: the Eviolate, Eviolite's offensive twin (src/battle_util.c)
ASSUMPTIONS
{
    ASSUME(gItemsInfo[ITEM_EVIOLATE].holdEffect == HOLD_EFFECT_EVIOLATE);
}

SINGLE_BATTLE_TEST("GymHack: Eviolate boosts Attack and Sp. Atk for unevolved Pokemon", s16 damage)
{
    enum Move move;
    enum Item item;

    PARAMETRIZE { move = MOVE_SCRATCH;   item = ITEM_EVIOLATE; }
    PARAMETRIZE { move = MOVE_SCRATCH;   item = ITEM_NONE; }
    PARAMETRIZE { move = MOVE_WATER_GUN; item = ITEM_EVIOLATE; }
    PARAMETRIZE { move = MOVE_WATER_GUN; item = ITEM_NONE; }

    GIVEN {
        ASSUME(GetMoveCategory(MOVE_SCRATCH) == DAMAGE_CATEGORY_PHYSICAL);
        ASSUME(GetMoveCategory(MOVE_WATER_GUN) == DAMAGE_CATEGORY_SPECIAL);
        PLAYER(SPECIES_SQUIRTLE) { Item(item); Moves(MOVE_SCRATCH, MOVE_WATER_GUN); }
        OPPONENT(SPECIES_SNORLAX);
    } WHEN {
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[1].damage, Q_4_12(1.5), results[0].damage);
        EXPECT_MUL_EQ(results[3].damage, Q_4_12(1.5), results[2].damage);
    }
}

SINGLE_BATTLE_TEST("GymHack: Eviolate does nothing for fully evolved Pokemon", s16 damage)
{
    enum Item item;

    PARAMETRIZE { item = ITEM_EVIOLATE; }
    PARAMETRIZE { item = ITEM_NONE; }

    GIVEN {
        PLAYER(SPECIES_BLASTOISE) { Item(item); Moves(MOVE_WATER_GUN); }
        OPPONENT(SPECIES_SNORLAX);
    } WHEN {
        TURN { MOVE(player, MOVE_WATER_GUN); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}
