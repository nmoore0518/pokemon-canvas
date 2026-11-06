#ifndef GUARD_MISSINGNO_H
#define GUARD_MISSINGNO_H

void MissingNo_ArmEncounter(void);
void MissingNo_OnMapLoad(void);
void MissingNo_UpdateCoast(void);
u32 MissingNo_WaterEncounterRate(u32 rate);
bool32 MissingNo_TryWaterEncounter(void);

#endif // GUARD_MISSINGNO_H
