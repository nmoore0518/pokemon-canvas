#ifndef GUARD_DEX_FORMS_H
#define GUARD_DEX_FORMS_H

#include "constants/dex_forms.h"

// Gym Trainer hack: every species and form the player could keep, in National Dex order
// (tools/gym_hack/build_withdraw.py -> src/data/dex_forms.h)
extern const u16 gDexFormSpecies[DEX_FORM_COUNT];
extern const u8 *const gDexFormLabels[DEX_FORM_COUNT];     // (NULL: the species' own name)
extern const u16 gDexFormGenStarts[DEX_FORM_GEN_COUNT];

#endif // GUARD_DEX_FORMS_H
