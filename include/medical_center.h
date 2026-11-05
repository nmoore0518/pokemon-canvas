#ifndef GUARD_MEDICAL_CENTER_H
#define GUARD_MEDICAL_CENTER_H

// Pokémon Canvas: the Universal Pokémon Medical Center (src/medical_center.c)
#include "constants/medical_center.h"

void MedicalCenter_OnStep(void);
u32 MedCenter_HelperCheck(struct Pokemon *mon, u32 helpersSoFar, u32 partyCount);
u32 MedCenter_TryGivePokerus(struct Pokemon *mon);

#endif // GUARD_MEDICAL_CENTER_H
