#ifndef GUARD_GAUNTLET_H
#define GUARD_GAUNTLET_H

// Pokémon Canvas: the Gauntlet (src/gauntlet.c)
void Gauntlet_OnTransition(void);
void Gauntlet_OnLoad(void);
void Gauntlet_SetupNext(void);
void Gauntlet_ShowTitleCard(void);
void Gauntlet_RecordWin(void);
void Gauntlet_BufferProgress(void);
void Gauntlet_PushRegions(void);
void Gauntlet_PushRegionNames(void);
void Gauntlet_SetFigureGfx(void);
void Gauntlet_BillDefeated(void);
void Gauntlet_RevealBill(void);
bool32 Gauntlet_IsBillBattle(void);
void Gauntlet_ChooseWithdraw(void);
void Gauntlet_UseWithdraw(void);

#endif // GUARD_GAUNTLET_H
