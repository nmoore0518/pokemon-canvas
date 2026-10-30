#ifndef GUARD_BASEMENT_MACHINES_H
#define GUARD_BASEMENT_MACHINES_H

// Pokémon Canvas: Dad's basement machines (the Ability Synthesizer, the Ball Maker, the Ball Changer),
// the Type Balls they make, and the Synthesizer's fuel on wild Pokémon. Scripts:
// data/maps/PalletTown_PlayersHouse_B1F_Frlg/scripts.inc.

#include "constants/basement_machines.h"
#include "constants/pokeball.h"

enum Type GetTypeBallType(enum PokeBall ball);
enum Item GetTypeBallItem(enum Type type);   // the Type Ball item of a type (ITEM_NONE if none)
// Bit per stat (1 << STAT_HP...) whose base stat is raised by half: the species' lowest (all of them if tied),
// when the ball is a Type Ball of one of its types. 0 otherwise.
u32 GetTypeBallStatBoosts(enum Species species, enum PokeBall ball);
// SetWildMonHeldItem: Strong Acid / Apple Juice. TRUE if it gave one.
bool32 TryGiveWildMonLabItem(struct Pokemon *mon);
bool32 BasementMachines_NiceMode(void);

// The scripts' specials (VAR_0x8004: the party slot)
void Synthesizer_CheckMon(void);
void Synthesizer_Apply(void);
void BallMaker_CheckMon(void);
void BallMaker_Apply(void);
void BallChanger_CheckMon(void);
void BallChanger_CheckBall(void);
void BallChanger_Apply(void);

#endif // GUARD_BASEMENT_MACHINES_H
