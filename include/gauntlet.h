#ifndef GUARD_GAUNTLET_H
#define GUARD_GAUNTLET_H

// Pokémon Canvas: the tournament's story, and Bill (src/gauntlet.c; the tournament: src/tournament.c)
void Gauntlet_OnTransition(void);
void Gauntlet_OnLoad(void);
void Gauntlet_SetupBill(void);
void Gauntlet_ShowBillCard(void);
void Gauntlet_ClearBillUp(void);
void Gauntlet_SetFigureGfx(void);
void Gauntlet_BillDefeated(void);
void Gauntlet_RevealBill(void);
bool32 Gauntlet_IsBillBattle(void);
void Gauntlet_ChooseWithdraw(void);
void Gauntlet_UseWithdraw(void);

#endif // GUARD_GAUNTLET_H
