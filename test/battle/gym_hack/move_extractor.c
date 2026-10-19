// Gym Trainer hack: the Move Extractor (src/move_extractor.c)
#include "global.h"
#include "test/battle.h"
#include "move_extractor.h"

SINGLE_BATTLE_TEST("GymHack: Move Extractor teaches the foe's last move as the user's own type")
{
    GIVEN {
        ASSUME(GetMoveType(MOVE_WATER_GUN) == TYPE_WATER);
        PLAYER(SPECIES_BULBASAUR) { Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_SQUIRTLE) { Moves(MOVE_WATER_GUN, MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); MOVE(opponent, MOVE_WATER_GUN); }
        TURN { USE_ITEM(player, ITEM_MOVE_EXTRACTOR); MOVE(opponent, MOVE_TACKLE); }
        TURN { MOVE(player, moveSlot: 1); MOVE(opponent, MOVE_TACKLE); }
    } SCENE {
        MESSAGE("The opposing Squirtle used Water Gun!");
        MESSAGE("Water Gun is now a Grass-type move!");
        MESSAGE("The opposing Squirtle lost Water Gun for this battle!");
        MESSAGE("Bulbasaur used Water Gun!");
        MESSAGE("It's super effective!");
    } THEN {
        EXPECT(IsStolenMove(&gParties[B_TRAINER_PLAYER][0], MOVE_WATER_GUN));
        EXPECT_EQ(GetStolenMoveType(&gParties[B_TRAINER_PLAYER][0]), TYPE_GRASS);
        EXPECT_EQ(gBattleMons[B_POSITION_OPPONENT_LEFT].pp[0], 0);
    }
}

SINGLE_BATTLE_TEST("GymHack: Move Extractor can't be used before the foe has used a move")
{
    GIVEN {
        PLAYER(SPECIES_BULBASAUR) { Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_SQUIRTLE) { Moves(MOVE_WATER_GUN); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_MOVE_EXTRACTOR); MOVE(opponent, MOVE_WATER_GUN); }
    } SCENE {
        MESSAGE("There's no move to extract!");
    } THEN {
        EXPECT(!IsStolenMove(&gParties[B_TRAINER_PLAYER][0], MOVE_WATER_GUN));
    }
}
