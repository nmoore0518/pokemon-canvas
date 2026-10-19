// Gym Trainer hack: the mentors' type items (HOLD_EFFECT_TYPE_GUARD / HOLD_EFFECT_TYPE_EDGE)
#include "global.h"
#include "test/battle.h"

ASSUMPTIONS
{
    ASSUME(GetItemHoldEffect(ITEM_HEAT_SHIELD) == HOLD_EFFECT_TYPE_GUARD);
    ASSUME(GetItemHoldEffect(ITEM_NOZZLE) == HOLD_EFFECT_TYPE_EDGE);
    ASSUME(GetItemHoldEffect(ITEM_THIRD_EYE) == HOLD_EFFECT_TYPE_EDGE);
    ASSUME(GetItemHoldEffect(ITEM_DIAMOND_PLATING) == HOLD_EFFECT_TYPE_GUARD);
}

SINGLE_BATTLE_TEST("GymHack: a defensive type item turns super effective hits into normal ones", s16 damage)
{
    enum Item item;
    enum Species species;

    PARAMETRIZE { item = ITEM_NONE;            species = SPECIES_VULPIX; }
    PARAMETRIZE { item = ITEM_HEAT_SHIELD;     species = SPECIES_VULPIX; }
    PARAMETRIZE { item = ITEM_DIAMOND_PLATING; species = SPECIES_GEODUDE; }  // 4x
    PARAMETRIZE { item = ITEM_NONE;            species = SPECIES_WOBBUFFET; } // neutral

    GIVEN {
        ASSUME(GetMoveType(MOVE_WATER_GUN) == TYPE_WATER);
        ASSUME(GetSpeciesType(SPECIES_VULPIX, 0) == TYPE_FIRE);
        ASSUME(GetSpeciesType(SPECIES_GEODUDE, 0) == TYPE_ROCK);
        ASSUME(GetSpeciesType(SPECIES_GEODUDE, 1) == TYPE_GROUND);
        PLAYER(SPECIES_WOBBUFFET) { SpAttack(100); }
        OPPONENT(species) { SpDefense(200); Item(item); }
    } WHEN {
        TURN { MOVE(player, MOVE_WATER_GUN); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WATER_GUN, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
        if (item != ITEM_NONE)
            NOT MESSAGE("It's super effective!");
    } THEN {
        EXPECT_EQ(opponent->item, item); // never used up
    } FINALLY {
        EXPECT_MUL_EQ(results[1].damage, Q_4_12(2.0), results[0].damage);
        EXPECT_EQ(results[1].damage, results[3].damage);
        EXPECT_EQ(results[2].damage, results[3].damage);
    }
}

SINGLE_BATTLE_TEST("GymHack: an offensive type item makes resisted moves hit normally", s16 damage)
{
    enum Item item;
    enum Species species;

    PARAMETRIZE { item = ITEM_NONE;   species = SPECIES_BULBASAUR; }
    PARAMETRIZE { item = ITEM_NOZZLE; species = SPECIES_BULBASAUR; }
    PARAMETRIZE { item = ITEM_NONE;   species = SPECIES_WOBBUFFET; }

    GIVEN {
        ASSUME(GetSpeciesType(SPECIES_BULBASAUR, 0) == TYPE_GRASS);
        PLAYER(SPECIES_WOBBUFFET) { SpAttack(100); Item(item); }
        OPPONENT(species) { SpDefense(200); }
    } WHEN {
        TURN { MOVE(player, MOVE_WATER_GUN); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WATER_GUN, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(player->item, item);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(2.0), results[1].damage);
        EXPECT_EQ(results[1].damage, results[2].damage);
    }
}

SINGLE_BATTLE_TEST("GymHack: an offensive type item lets moves hit types immune to them")
{
    GIVEN {
        ASSUME(GetMoveType(MOVE_TACKLE) == TYPE_NORMAL);
        ASSUME(GetSpeciesType(SPECIES_GASTLY, 0) == TYPE_GHOST);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_THIRD_EYE); }
        OPPONENT(SPECIES_GASTLY);
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("GymHack: an offensive type item doesn't get past Levitate")
{
    GIVEN {
        ASSUME(GetMoveType(MOVE_MUD_SLAP) == TYPE_GROUND);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_DIAMOND_DRILL_BIT); }
        OPPONENT(SPECIES_KOFFING) { Ability(ABILITY_LEVITATE); }
    } WHEN {
        TURN { MOVE(player, MOVE_MUD_SLAP); }
    } SCENE {
        NONE_OF { HP_BAR(opponent); }
    }
}

SINGLE_BATTLE_TEST("GymHack: type items can't be stolen")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_THIEF) == EFFECT_STEAL_ITEM);
        PLAYER(SPECIES_WOBBUFFET) { Item(ITEM_HEAT_SHIELD); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(opponent, MOVE_THIEF); }
    } THEN {
        EXPECT_EQ(player->item, ITEM_HEAT_SHIELD);
        EXPECT_EQ(opponent->item, ITEM_NONE);
    }
}
