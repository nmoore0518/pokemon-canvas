#ifndef GUARD_PIT_H
#define GUARD_PIT_H

// Pokémon Canvas: the pit under Celadon (src/pit.c)
struct Trainer;

bool32 Pit_FightActive(void);
const struct Trainer *Pit_GetTrainer(u16 trainerId);
bool32 Pit_TryHiddenDoor(s32 x, s32 y, u32 direction);
u32 Pit_GetOdds(u32 side);
u32 Pit_GetSpecies(u32 side);

// Specials
void Pit_OnTransition(void);
void Pit_NextCard(void);
void Pit_PickMoves(u32 species, u32 level, u16 *moves);
void Pit_BufferCardPage(void);
void Pit_BufferCard(void);
void Pit_PushCorners(void);
void Pit_PushBets(void);
void Pit_PlaceBet(void);
void Pit_BufferBet(void);
void Pit_HasBet(void);
void Pit_BuyTip(void);
void Pit_TipBought(void);
void Pit_SetupFight(void);
void Pit_EndFight(void);
void Pit_AskPassword(void);
void Pit_CheckPassword(void);

#endif // GUARD_PIT_H
