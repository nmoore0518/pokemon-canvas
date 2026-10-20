// Gym Trainer hack: the legendary Pokémon of the player's type (data/scripts/gym_legend.inc).
//
// Right after Oak hands the Pallet Town Gym over (the finale, data/scripts/gym_finale.inc), the
// lights go out and the most powerful Pokémon of the player's type appears on the battle stage.
// Talking to it starts the battle (level 70). It stays (coming back each time the gym is entered,
// even after being knocked out or run from) until it's caught.
#include "global.h"
#include "gym_legend.h"
#include "battle_setup.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "gym_decor.h"
#include "metatile_behavior.h"
#include "pokemon.h"
#include "string_util.h"
#include "constants/characters.h"
#include "battle_main.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/gym_room.h"
#include "constants/species.h"
#include "constants/vars.h"
#include "constants/map_event_ids.h"
#include "overworld.h"
#include "field_player_avatar.h"
#include "constants/event_object_movement.h"

#define GYM_LEGEND_LEVEL 70

const u16 gGymLegends[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = SPECIES_MELOETTA,     // (Aria Forme)
    [TYPE_FIGHTING] = SPECIES_KORAIDON,
    [TYPE_FLYING]   = SPECIES_TORNADUS,     // (Incarnate Forme)
    [TYPE_POISON]   = SPECIES_ETERNATUS,
    [TYPE_GROUND]   = SPECIES_GROUDON,
    [TYPE_ROCK]     = SPECIES_DIANCIE,
    [TYPE_BUG]      = SPECIES_GENESECT,
    [TYPE_GHOST]    = SPECIES_GIRATINA,     // (Altered Forme)
    [TYPE_STEEL]    = SPECIES_DIALGA,
    [TYPE_FIRE]     = SPECIES_HO_OH,
    [TYPE_WATER]    = SPECIES_PALKIA,
    [TYPE_GRASS]    = SPECIES_SHAYMIN,      // (Land Forme)
    [TYPE_ELECTRIC] = SPECIES_MIRAIDON,
    [TYPE_PSYCHIC]  = SPECIES_CRESSELIA,
    [TYPE_ICE]      = SPECIES_GLASTRIER,
    [TYPE_DRAGON]   = SPECIES_RAYQUAZA,
    [TYPE_DARK]     = SPECIES_DARKRAI,
    [TYPE_FAIRY]    = SPECIES_XERNEAS,
};

u16 GymLegend_GetSpecies(u32 type)
{
    if (type >= NUMBER_OF_MON_TYPES)
        return SPECIES_NONE;
    return gGymLegends[type];
}

static u16 PlayerLegend(void)
{
    return GymLegend_GetSpecies(VarGet(VAR_PLAYER_TYPE));
}

static void CopyUppercase(u8 *dst, const u8 *src)
{
    for (; *src != EOS; src++, dst++)
        *dst = (*src >= CHAR_a && *src <= CHAR_z) ? *src - CHAR_a + CHAR_A : *src;
    *dst = EOS;
}

// Special: VAR_RESULT = whether there's a legendary for the player's type (not caught yet);
// VAR_0x8004 = it, VAR_OBJ_GFX_ID_C = how it looks, STR_VAR_1 = its name, STR_VAR_2 = the type
void GymLegend_Setup(void)
{
    u16 species = PlayerLegend();
    gSpecialVar_Result = (species != SPECIES_NONE && !FlagGet(FLAG_GYM_LEGEND_CAUGHT));
    if (species == SPECIES_NONE)
        return;
    gSpecialVar_0x8004 = species;
    VarSet(VAR_OBJ_GFX_ID_C, species + OBJ_EVENT_MON);
    CopyUppercase(gStringVar1, GetSpeciesName(species));
    CopyUppercase(gStringVar2, gTypesInfo[VarGet(VAR_PLAYER_TYPE)].name);
}

static bool32 IsFreeSpot(s32 x, s32 y, bool32 checkObjects)
{
    s32 mx = x + MAP_OFFSET, my = y + MAP_OFFSET, tx, ty;
    u32 slot, decor, instance;
    if (x < 1 || y < 1 || x >= GYM_ROOM_WIDTH - 1 || y >= GYM_ROOM_HEIGHT - 3)
        return FALSE;
    if (MapGridGetCollisionAt(mx, my) || MapGridGetElevationAt(mx, my) != 3)
        return FALSE;
    if (MetatileBehavior_IsForcedMovementTile(MapGridGetMetatileBehaviorAt(mx, my)))
        return FALSE;
    if ((x == GYM_AIDE_X && y == GYM_AIDE_Y) || (x == GYM_AIDE_X && y == GYM_AIDE_Y + 1))
        return FALSE;
    for (slot = 0; slot < GYM_PRACTICE_SLOTS; slot++)
    {
        if (GymDecor_GetTrainerSlot(slot, &decor, &instance, &tx, &ty) && tx == x && ty == y)
            return FALSE;
    }
    return !checkObjects || GetObjectEventIdByXY(mx, my) == OBJECT_EVENTS_COUNT;
}

// Where it stands: the leader's mark on the battle stage, or (no stage, or it's taken) the free
// square nearest the middle of the room
static void FindSpot(s32 *outX, s32 *outY, bool32 checkObjects)
{
    s32 sx, sy, cx = GYM_ROOM_WIDTH / 2, cy = 18, r, dx, dy;

    if (GymDecor_FindPlaced(GYM_DECOR_BATTLE_STAGE, &sx, &sy) && IsFreeSpot(sx + 1, sy + 1, checkObjects))
    {
        *outX = sx + 1;
        *outY = sy + 1;
        return;
    }
    for (r = 0; r < GYM_ROOM_WIDTH; r++)
    {
        for (dy = -r; dy <= r; dy++)
        {
            for (dx = -r; dx <= r; dx++)
            {
                if ((abs(dx) == r || abs(dy) == r) && IsFreeSpot(cx + dx, cy + dy, checkObjects))
                {
                    *outX = cx + dx;
                    *outY = cy + dy;
                    return;
                }
            }
        }
    }
    *outX = cx;
    *outY = cy;
}

// Special (the arrival): VAR_0x8005/6 = where it appears
void GymLegend_FindSpot(void)
{
    s32 x, y;
    FindSpot(&x, &y, TRUE);
    gSpecialVar_0x8005 = x;
    gSpecialVar_0x8006 = y;
}

// Map script (ON_LOAD): after its arrival and until it's caught, it waits on the stage
void GymLegend_OnLoad(void)
{
    s32 x, y;
    u16 species = PlayerLegend();
    FlagSet(FLAG_HIDE_PALLET_GYM_LEGEND);
    if (species == SPECIES_NONE || !FlagGet(FLAG_GYM_LEGEND_APPEARED) || FlagGet(FLAG_GYM_LEGEND_CAUGHT))
        return;
    VarSet(VAR_OBJ_GFX_ID_C, species + OBJ_EVENT_MON);
    FindSpot(&x, &y, FALSE);
    SetObjEventTemplateCoords(LOCALID_PALLET_GYM_LEGEND, x, y);
    FlagClear(FLAG_HIDE_PALLET_GYM_LEGEND);
}

// Special: sets up the battle with it (then BattleSetup_StartLegendaryBattle)
void GymLegend_CreateMon(void)
{
    gSpecialVar_0x8004 = PlayerLegend();
    gSpecialVar_0x8005 = GYM_LEGEND_LEVEL;
    gSpecialVar_0x8006 = ITEM_NONE;
    CreateEnemyEventMon();
}

// Special (the arrival): the camera's trip from the player to it (VAR_0x8005/6) and back, into
// gGymLegendCameraIn/Out
#define CAMERA_PATH_MAX 48
EWRAM_DATA u8 gGymLegendCameraIn[CAMERA_PATH_MAX + 1];
EWRAM_DATA u8 gGymLegendCameraOut[CAMERA_PATH_MAX + 1];

void GymLegend_PlanCamera(void)
{
    s16 px, py;
    s32 dx, dy, n = 0, i;
    PlayerGetDestCoords(&px, &py);
    dx = gSpecialVar_0x8005 - (px - MAP_OFFSET);
    dy = gSpecialVar_0x8006 + 1 - (py - MAP_OFFSET);   // (a little below it, so it's in the middle)
    for (i = 0; i < abs(dy) && n < CAMERA_PATH_MAX / 2; i++, n++)
    {
        gGymLegendCameraIn[n] = dy < 0 ? MOVEMENT_ACTION_WALK_FAST_UP : MOVEMENT_ACTION_WALK_FAST_DOWN;
        gGymLegendCameraOut[n] = dy < 0 ? MOVEMENT_ACTION_WALK_FAST_DOWN : MOVEMENT_ACTION_WALK_FAST_UP;
    }
    for (i = 0; i < abs(dx) && n < CAMERA_PATH_MAX; i++, n++)
    {
        gGymLegendCameraIn[n] = dx < 0 ? MOVEMENT_ACTION_WALK_FAST_LEFT : MOVEMENT_ACTION_WALK_FAST_RIGHT;
        gGymLegendCameraOut[n] = dx < 0 ? MOVEMENT_ACTION_WALK_FAST_RIGHT : MOVEMENT_ACTION_WALK_FAST_LEFT;
    }
    gGymLegendCameraIn[n] = MOVEMENT_ACTION_STEP_END;
    gGymLegendCameraOut[n] = MOVEMENT_ACTION_STEP_END;
}
