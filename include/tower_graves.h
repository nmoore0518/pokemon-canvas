#ifndef GUARD_TOWER_GRAVES_H
#define GUARD_TOWER_GRAVES_H

#define TOWER_GRAVES_UNEVOLVED 10

u32 TowerGraves_Count(void);
u16 TowerGraves_GetSpecies(u32 grave);
void TowerGraves_Read(void);
void TowerGraves_SummonGhosts(void);
void TowerGraves_DismissGhosts(void);

#endif // GUARD_TOWER_GRAVES_H
