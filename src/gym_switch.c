// Gym Trainer hack (Pokémon Canvas): changing the gym's type after the League.
//
// A streak of GYM_SWITCH_FIRST_STREAK wins (GYM_SWITCH_NEXT_STREAK for each later one) masters
// the gym's type: the streak's last battle is the player's mentor with their full Convention team
// (src/gym_challenge.c). Beating them earns a new type (a "ticket", saved), and when the streak
// ends (a loss, or "call it a day") Oak comes to the gym and offers it: the player picks a type
// they haven't had, and its legendary comes to the gym. Turning him down keeps the ticket: Oak
// offers it again in his lab, where the player can also switch back to any type the gym has had,
// free. The rival's type never changes. Scripts: data/scripts/gym_switch.inc.
#include "global.h"
#include "gym_switch.h"
#include "event_data.h"
#include "gym_badge.h"
#include "gym_challenge.h"
#include "gym_hack.h"
#include "gym_legend.h"
#include "battle_main.h"
#include "list_menu.h"
#include "malloc.h"
#include "overworld.h"
#include "pokemon.h"
#include "script_menu.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/gym_hack.h"
#include "constants/species.h"
#include "constants/vars.h"

#define GYM_SWITCH_MAGIC 0x5354   // "TS"

static EWRAM_DATA u8 sChosenType;

static struct GymChallengeSave *Save(void)
{
    struct GymChallengeSave *save = GymChallenge_GetSave();
    if (save->switchMagic != GYM_SWITCH_MAGIC)     // (a save from before type switching)
    {
        save->switchTickets = 0;
        save->switchesEarned = 0;
        save->switchMagic = GYM_SWITCH_MAGIC;
    }
    return save;
}

// The 18 types a gym can have (the ones with a starter)
static bool32 IsGymType(u32 type)
{
    return type < NUMBER_OF_MON_TYPES && GymHack_GetStarterSpeciesForType(type) != SPECIES_NONE;
}

static u32 CurrentType(void)
{
    return VarGet(VAR_PLAYER_TYPE);
}

bool32 GymSwitch_IsOwned(u32 type)
{
    return IsGymType(type) && (type == CurrentType() || FlagGet(FLAG_GYM_TYPE_OWNED + type));
}

static u32 CountTypes(bool32 owned)
{
    u32 type, n = 0;
    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (IsGymType(type) && GymSwitch_IsOwned(type) == owned)
            n++;
    }
    return n;
}

static bool32 CanTakeNewType(void)
{
    return Save()->switchTickets != 0 && CountTypes(FALSE) != 0;
}

static bool32 IsMastered(u32 type)
{
    return FlagGet(FLAG_GYM_TYPE_MASTERED + type);
}

// Whether every gym type's streak has been mastered (Paldea's Terapagos waits for it)
bool32 GymSwitch_AllTypesMastered(void)
{
    u32 type;
    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
    {
        if (IsGymType(type) && !IsMastered(type))
            return FALSE;
    }
    return TRUE;
}

// Special: VAR_RESULT = GymSwitch_AllTypesMastered()
void GymSwitch_AllMastered(void)
{
    gSpecialVar_Result = GymSwitch_AllTypesMastered();
}

// Whether a streak can still master something: a type the gym hasn't had is left to earn, or the
// gym's own type hasn't been mastered yet (the last type still has its mastery battle)
static bool32 MasteryAhead(void)
{
    return CountTypes(FALSE) != 0 || !IsMastered(CurrentType());
}

// The streak that masters the gym's type
u32 GymSwitch_GetGoal(void)
{
    return Save()->switchesEarned == 0 ? GYM_SWITCH_FIRST_STREAK : GYM_SWITCH_NEXT_STREAK;
}

// Whether this battle of a streak is the mastery battle (none once the gym has had every type and
// mastered them all)
bool32 GymSwitch_IsMasteryBattle(u32 streak)
{
    return FlagGet(FLAG_PALLET_GYM_AWARDED) && MasteryAhead() && streak == GymSwitch_GetGoal();
}

static void CopyUppercase(u8 *dst, const u8 *src)
{
    for (; *src != EOS; src++, dst++)
        *dst = (*src >= CHAR_a && *src <= CHAR_z) ? *src - CHAR_a + CHAR_A : *src;
    *dst = EOS;
}

void GymSwitch_BufferTypeName(u8 *dst, u32 type)
{
    CopyUppercase(dst, gTypesInfo[type < NUMBER_OF_MON_TYPES ? type : TYPE_NORMAL].name);
}

// Special, after a win: was it the mastery battle? Then a new type is earned (and Oak comes when
// the streak ends). VAR_RESULT = TRUE then; STR_VAR_1 = the gym's type.
void GymSwitch_AfterWin(void)
{
    struct GymChallengeSave *save = Save();
    gSpecialVar_Result = FALSE;
    if (!GymChallenge_WasBoss())
        return;
    FlagSet(FLAG_GYM_TYPE_MASTERED + CurrentType());
    // (and its plate hides in the gym from now on: src/form_items.c)
    if (save->switchTickets < 0xFF && CountTypes(FALSE) != 0)    // (no ticket once there's no new type)
        save->switchTickets++;
    if (save->switchesEarned < 0xFF)
        save->switchesEarned++;
    FlagSet(FLAG_GYM_RUN_MASTERED);
    GymSwitch_BufferTypeName(gStringVar1, CurrentType());
    gSpecialVar_Result = TRUE;
}

// Special (the aide): VAR_RESULT = whether there's a mastery goal ahead in this streak;
// STR_VAR_1 = the streak it takes
void GymSwitch_BufferGoal(void)
{
    gSpecialVar_Result = MasteryAhead() && GymChallenge_GetSave()->streak < GymSwitch_GetGoal();
    ConvertIntToDecimalStringN(gStringVar1, GymSwitch_GetGoal(), STR_CONV_MODE_LEFT_ALIGN, 3);
}

// Special (Oak): VAR_RESULT = what he can offer: 0 nothing yet (STR_VAR_1 = the streak it takes,
// STR_VAR_2 = the gym's type), 1 a new type or a switch back. STR_VAR_3 = the gym's type too.
void GymSwitch_BufferStatus(void)
{
    ConvertIntToDecimalStringN(gStringVar1, GymSwitch_GetGoal(), STR_CONV_MODE_LEFT_ALIGN, 3);
    GymSwitch_BufferTypeName(gStringVar2, CurrentType());
    GymSwitch_BufferTypeName(gStringVar3, CurrentType());
    gSpecialVar_Result = CanTakeNewType() || CountTypes(TRUE) > 1;
}

// Special (Oak's lab): VAR_RESULT = whether a streak is going (no changing type until it ends:
// a loss, or calling it a day after mastering); STR_VAR_1 = it
void GymSwitch_StreakInProgress(void)
{
    u32 streak = GymChallenge_GetSave()->streak;
    gSpecialVar_Result = streak != 0;
    ConvertIntToDecimalStringN(gStringVar1, streak, STR_CONV_MODE_LEFT_ALIGN, 5);
}

// Special: VAR_RESULT = whether the gym has ever changed type
void GymSwitch_HasSwitched(void)
{
    gSpecialVar_Result = FlagGet(FLAG_GYM_TYPE_OWNED + CurrentType());
}

// Special: VAR_0x8004 = TRUE if the type menu would only have new types (no other type the gym
// has had: no {STAR}s to explain)
void GymSwitch_OnlyNewTypes(void)
{
    gSpecialVar_0x8004 = CountTypes(TRUE) <= 1;
}

// Special: VAR_RESULT = whether a new type can be chosen (a ticket, and a type left)
void GymSwitch_CanTakeNewType(void)
{
    gSpecialVar_Result = CanTakeNewType();
}

// Special: the type menu (dynmultistack). VAR_0x8004 = TRUE: only types the gym hasn't had (Oak's
// offer in the gym); FALSE: also the ones it has had ({STAR}, free), which come first.
void GymSwitch_PushTypes(void)
{
    u32 pass, type;
    bool32 newTypes = CanTakeNewType();
    for (pass = 0; pass < 2; pass++)
    {
        bool32 owned = (pass == 0);
        if (owned ? gSpecialVar_0x8004 : !newTypes)
            continue;
        for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
        {
            struct ListMenuItem item;
            u8 *name;
            if (!IsGymType(type) || type == CurrentType() || GymSwitch_IsOwned(type) != owned)
                continue;
            name = Alloc(24);
            GymSwitch_BufferTypeName(name, type);
            if (owned)
                StringAppend(name, COMPOUND_STRING(" {STAR}"));
            item.name = name;
            item.id = type;
            MultichoiceDynamic_PushElement(item);
        }
    }
}

// Special: VAR_RESULT = the type picked from the menu. STR_VAR_1 = it, STR_VAR_2 = its badge;
// VAR_RESULT = whether it's a new type (it uses the mastery up)
void GymSwitch_BufferChoice(void)
{
    sChosenType = IsGymType(gSpecialVar_Result) ? gSpecialVar_Result : TYPE_NORMAL;
    GymSwitch_BufferTypeName(gStringVar1, sChosenType);
    StringCopy(gStringVar2, GymBadge_GetName(sChosenType));
    gSpecialVar_Result = !GymSwitch_IsOwned(sChosenType);
}

// Special: the gym becomes the chosen type. A new one uses up the mastery. VAR_RESULT = whether its
// legendary is on its way (VAR_GYM_LEGEND_STATE is GYM_LEGEND_PENDING); VAR_0x8007 = whether
// the old type's legendary was waiting on the stage (it leaves for Cerulean Cave: STR_VAR_3).
// STR_VAR_1 = the type, STR_VAR_2 = its badge.
void GymSwitch_SetType(void)
{
    struct GymChallengeSave *save = Save();
    u32 from = CurrentType(), to = sChosenType;

    if (!GymSwitch_IsOwned(to))
    {
        if (save->switchTickets != 0)
            save->switchTickets--;
        FlagClear(FLAG_GYM_RUN_MASTERED);   // (Oak's offer is taken: nothing to come for)
    }
    if (IsGymType(from))
        FlagSet(FLAG_GYM_TYPE_OWNED + from);
    FlagSet(FLAG_GYM_TYPE_OWNED + to);
    StringCopy(gStringVar3, GetSpeciesName(GymLegend_GetSpecies(from)));
    CopyUppercase(gStringVar3, gStringVar3);
    gSpecialVar_Result = GymLegend_ChangeType(from, to);
    VarSet(VAR_PLAYER_TYPE, to);
    GymSwitch_BufferTypeName(gStringVar1, to);
    StringCopy(gStringVar2, GymBadge_GetName(to));
    // The badge on the gym's pedestal
    GymBadge_PatchTilesetGfx(gMapHeader.mapLayout);
    GymBadge_PatchTilesetPalette(gMapHeader.mapLayout, FALSE);
}
