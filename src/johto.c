// Gym Trainer hack: Johto (the maps come from Pokémon HnS; see tools/gym_hack/import_johto.py)
#include "global.h"
#include "johto.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "overworld.h"
#include "gym_hack.h"
#include "region_map.h"
#include "script.h"
#include "main.h"
#include "field_player_avatar.h"
#include "constants/vars.h"

// Special (testing): warp to map group VAR_TEMP_1, number VAR_TEMP_2, at its warp VAR_TEMP_3
// (or, if that's 0xFF, at x VAR_TEMP_4, y VAR_TEMP_5)
void Johto_DebugWarp(void)
{
    if (VarGet(VAR_TEMP_3) == 0xFF)
        SetWarpDestination(VarGet(VAR_TEMP_1), VarGet(VAR_TEMP_2), WARP_ID_NONE, VarGet(VAR_TEMP_4), VarGet(VAR_TEMP_5));
    else
        SetWarpDestination(VarGet(VAR_TEMP_1), VarGet(VAR_TEMP_2), VarGet(VAR_TEMP_3), -1, -1);
    DoWarp();
    ResetInitialPlayerAvatarState();
}

// Special (testing): the Fly map, as the Hot Air Balloon opens it
void Johto_DebugFlyMap(void)
{
    gSkipShowMonAnim = TRUE;
    gGymHackFlyFromItem = TRUE;
    SetMainCallback2(CB2_OpenFlyMap);
}
