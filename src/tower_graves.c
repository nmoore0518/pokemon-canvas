// Pokémon Canvas: the Pokémon Tower's graves and the ghost girl.
//
// 30 graves on 2F-4F (tools/gym_hack/build_tower_graves.py; src/data/tower_graves.h) show their
// Pokémon, its name and its trainer. Ten of them could have evolved and never did: each one read sets
// its bit in VAR_TOWER_GRAVES. One is DAD's ace (by the player's type), its trainer "???".
// The tenth read brings the ghost girl and her SCYTHER (on whichever floor it is): they flicker in
// near the player, half there (blended); she gives the game's one Eviolate (FLAG_GOT_EVIOLATE), and they
// flicker away. Their objects are on each floor, hidden by FLAG_TEMP_1F (data/scripts/tower_graves.inc).
#include "global.h"
#include "tower_graves.h"
#include "dad_story.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_weather.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "gpu_regs.h"
#include "gym_hack.h"
#include "overworld.h"
#include "pokemon.h"
#include "random.h"
#include "script.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "constants/event_objects.h"
#include "constants/species.h"
#include "constants/weather.h"

struct TowerGrave
{
    u16 species;        // (SPECIES_NONE: DAD's ace)
    const u8 *trainer;
};

#include "data/tower_graves.h"

#define GHOST_HIDE_FLAG FLAG_TEMP_1F
#define GHOST_ALPHA     10      // (of 16: how much of them is there, where there's no fog)
#define GHOST_STEPS     60      // (frames to flicker in or out)

u32 TowerGraves_Count(void)
{
    return ARRAY_COUNT(sTowerGraves);
}

u16 TowerGraves_GetSpecies(u32 grave)
{
    u16 species = sTowerGraves[min(grave, ARRAY_COUNT(sTowerGraves) - 1)].species;
    return species == SPECIES_NONE ? DadStory_GetAce(VarGet(VAR_PLAYER_TYPE)) : species;
}

static bool32 IsUnevolvedGrave(u32 grave)
{
    return sTowerGraves[grave].species != SPECIES_NONE && GymHack_SpeciesCanEvolve(sTowerGraves[grave].species);
}

// its bit in VAR_TOWER_GRAVES, or -1 (not one of the ten)
static s32 UnevolvedBit(u32 grave)
{
    u32 i, n = 0;
    if (!IsUnevolvedGrave(grave))
        return -1;
    for (i = 0; i < grave; i++)
        n += IsUnevolvedGrave(i);
    return n;
}

// Special: VAR_0x8004 = the grave -> VAR_0x8005 = its Pokémon, STR_VAR_1 = its name, STR_VAR_2 = its
// trainer; VAR_RESULT = whether the girl comes now (all ten read, and no Eviolate yet)
void TowerGraves_Read(void)
{
    u32 grave = min(gSpecialVar_0x8004, ARRAY_COUNT(sTowerGraves) - 1);
    s32 bit = UnevolvedBit(grave);
    u16 all = (1 << TOWER_GRAVES_UNEVOLVED) - 1;

    gSpecialVar_0x8005 = TowerGraves_GetSpecies(grave);
    StringCopy(gStringVar1, GetSpeciesName(gSpecialVar_0x8005));
    StringCopy(gStringVar2, sTowerGraves[grave].trainer);
    if (bit >= 0)
        VarSet(VAR_TOWER_GRAVES, VarGet(VAR_TOWER_GRAVES) | (1 << bit));
    gSpecialVar_Result = (VarGet(VAR_TOWER_GRAVES) & all) == all && !FlagGet(FLAG_GOT_EVIOLATE);
}

// ---------------------------------------------------------------------------
// The girl and her SCYTHER
// ---------------------------------------------------------------------------
static const struct ObjectEventTemplate *FindGhost(u16 graphicsId)
{
    u32 i;
    for (i = 0; i < gMapHeader.events->objectEventCount; i++)
    {
        const struct ObjectEventTemplate *t = &gMapHeader.events->objectEvents[i];
        if (t->graphicsId == graphicsId && t->flagId == GHOST_HIDE_FLAG)
            return t;
    }
    return NULL;
}

static struct ObjectEvent *GhostObject(const struct ObjectEventTemplate *t)
{
    u32 id = GetObjectEventIdByLocalIdAndMap(t->localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    return id < OBJECT_EVENTS_COUNT ? &gObjectEvents[id] : NULL;
}

static bool32 IsFree(s16 x, s16 y, s16 px, s16 py)
{
    return !(x == px && y == py) && MapGridGetCollisionAt(x, y) == 0
        && GetObjectEventIdByXY(x, y) == OBJECT_EVENTS_COUNT;
}

static enum Direction DirectionTo(s16 fromX, s16 fromY, s16 toX, s16 toY)
{
    s16 dx = toX - fromX, dy = toY - fromY;
    if (abs(dx) > abs(dy))
        return dx > 0 ? DIR_EAST : DIR_WEST;
    return dy > 0 ? DIR_SOUTH : DIR_NORTH;
}

// where they appear: the girl two steps from the player if there's room (else three, else one), her
// SCYTHER beside her. FALSE: nowhere at all
static bool32 PlaceGhosts(s16 px, s16 py, s16 *gx, s16 *gy, s16 *sx, s16 *sy)
{
    static const u8 sDistances[] = {2, 3, 1};
    static const s8 sBeside[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    u32 d, k;
    s32 dx, dy;

    for (d = 0; d < ARRAY_COUNT(sDistances); d++)
    {
        for (dy = -3; dy <= 3; dy++)
        {
            for (dx = -3; dx <= 3; dx++)
            {
                if ((u32)(abs(dx) + abs(dy)) != sDistances[d] || !IsFree(px + dx, py + dy, px, py))
                    continue;
                for (k = 0; k < ARRAY_COUNT(sBeside); k++)
                {
                    s16 x = px + dx + sBeside[k][0], y = py + dy + sBeside[k][1];
                    if (IsFree(x, y, px, py))
                    {
                        *gx = px + dx, *gy = py + dy, *sx = x, *sy = y;
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

// Half there: blended with what's behind them. On the foggy floors (3F, 4F) the fog's blend already
// covers them; elsewhere they get their own
static bool32 OwnBlend(void)
{
    return GetCurrentWeather() != WEATHER_FOG_HORIZONTAL;
}

static void SetGhostBlend(const struct ObjectEventTemplate *t)
{
    struct ObjectEvent *obj = t ? GhostObject(t) : NULL;
    if (obj != NULL)
        gSprites[obj->spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
}

static void SetGhostsVisible(bool32 visible)
{
    u32 i;
    for (i = 0; i < 2; i++)
    {
        const struct ObjectEventTemplate *t = FindGhost(i == 0 ? OBJ_EVENT_GFX_LITTLE_GIRL_FRLG : OBJ_EVENT_GFX_SPECIES(SCYTHER));
        struct ObjectEvent *obj = t ? GhostObject(t) : NULL;
        if (obj != NULL)
            obj->invisible = !visible;
    }
}

#define tStep   data[0]
#define tIn     data[1]

// they flicker in (or out): there more and more often, then all the time (or less and less)
static void Task_FadeGhosts(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u32 there = tIn ? tStep : GHOST_STEPS - tStep;

    SetGhostsVisible((Random() % GHOST_STEPS) < there);
    if (++tStep <= GHOST_STEPS)
        return;
    SetGhostsVisible(tIn);
    if (!tIn)
    {
        u32 i;
        for (i = 0; i < 2; i++)
        {
            const struct ObjectEventTemplate *t = FindGhost(i == 0 ? OBJ_EVENT_GFX_LITTLE_GIRL_FRLG : OBJ_EVENT_GFX_SPECIES(SCYTHER));
            if (t != NULL)
                RemoveObjectEventByLocalIdAndMap(t->localId, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
        }
        FlagSet(GHOST_HIDE_FLAG);
        if (OwnBlend())
        {
            SetGpuReg(REG_OFFSET_BLDCNT, 0);
            SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        }
    }
    DestroyTask(taskId);
    ScriptContext_Enable();
}

static void StartFade(bool32 in)
{
    u8 taskId = CreateTask(Task_FadeGhosts, 80);
    gTasks[taskId].tIn = in;
    ScriptContext_Stop();
}

#undef tStep
#undef tIn

// Special (then waitstate): the girl and her SCYTHER fade in near the player, facing them; the player
// turns to her
void TowerGraves_SummonGhosts(void)
{
    const struct ObjectEventTemplate *girl = FindGhost(OBJ_EVENT_GFX_LITTLE_GIRL_FRLG);
    const struct ObjectEventTemplate *scyther = FindGhost(OBJ_EVENT_GFX_SPECIES(SCYTHER));
    u8 mapNum = gSaveBlock1Ptr->location.mapNum, mapGroup = gSaveBlock1Ptr->location.mapGroup;
    struct ObjectEvent *g, *s, *player = &gObjectEvents[gPlayerAvatar.objectEventId];
    s16 px, py, gx, gy, sx, sy;

    PlayerGetDestCoords(&px, &py);
    if (girl == NULL || scyther == NULL || !PlaceGhosts(px, py, &gx, &gy, &sx, &sy))
        return;     // (no room: she speaks unseen)
    if (OwnBlend())
    {
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL & ~BLDCNT_TGT2_OBJ);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(GHOST_ALPHA, 16 - GHOST_ALPHA));
    }
    SetObjEventTemplateCoords(girl->localId, gx - MAP_OFFSET, gy - MAP_OFFSET);
    SetObjEventTemplateCoords(scyther->localId, sx - MAP_OFFSET, sy - MAP_OFFSET);
    FlagClear(GHOST_HIDE_FLAG);
    TrySpawnObjectEvent(girl->localId, mapNum, mapGroup);
    TrySpawnObjectEvent(scyther->localId, mapNum, mapGroup);
    SetGhostBlend(girl);
    SetGhostBlend(scyther);
    SetGhostsVisible(FALSE);
    g = GhostObject(girl);
    s = GhostObject(scyther);
    if (g != NULL)
        ObjectEventTurn(g, DirectionTo(gx, gy, px, py));
    if (s != NULL)
        ObjectEventTurn(s, DirectionTo(sx, sy, px, py));
    ObjectEventTurn(player, DirectionTo(px, py, gx, gy));
    StartFade(TRUE);
}

// Special (then waitstate): they fade away
void TowerGraves_DismissGhosts(void)
{
    if (FlagGet(GHOST_HIDE_FLAG))
        return;
    StartFade(FALSE);
}
