// Pokémon Canvas: the Drink Machine. The girl on the Celadon roof gives it (with a LIMITED EDITION
// SODA) once her stuck drinks come out (data/maps/CeladonCity_DepartmentStore_Roof_Frlg); it's a
// gym decoration, and talking to it (src/field_control_avatar.c) mixes a drink and up to two berries
// (data/scripts/drink_machine.inc). Only a recipe makes anything: otherwise nothing's used up.
//   FRESH WATER + a healing berry      SUPER POTION
//   SODA POP + a healing berry         HYPER POTION
//   LEMONADE + ORAN                    HYPER POTION
//   LEMONADE + SITRUS                  MAX POTION
//   a drink + a status berry           FULL HEAL
//   LEMONADE + SITRUS + LUM            FULL RESTORE
//   a drink + two healing berries      REVIVE (LEMONADE + two SITRUS: MAX REVIVE)
//   a drink + LEPPA                    ETHER, ELIXIR, MAX ELIXIR (by the drink)
//   LIMITED EDITION SODA + a type berry (the Tera berries: src/berry_field.c): a POKéMON in the party
//     gets that type as its second type (MON_DATA_SECOND_TYPE: GetMonType, src/pokemon.c) for good,
//     through evolution and Mega Evolution; not its first type (refused, nothing used up).
// The girl gives a soda for each type mastered after that (VAR_SODAS_FOR_MASTERY); once every type's
// been mastered and she's given them all, the tournament can give one, rarely (src/tournament.c).
#include "global.h"
#include "drink_machine.h"
#include "battle_main.h"
#include "party_menu.h"
#include "berry_field.h"
#include "event_data.h"
#include "item.h"
#include "mastery_hall.h"
#include "pokemon.h"
#include "random.h"
#include "script_menu.h"
#include "string_util.h"
#include "list_menu.h"
#include "malloc.h"
#include "gym_decor.h"
#include "constants/items.h"
#include "constants/gym_decor.h"

#define MAX_BERRIES 2
#define MAX_CHOICES 24
#define NAME_LEN    24

static const u16 sDrinks[] = {ITEM_FRESH_WATER, ITEM_SODA_POP, ITEM_LEMONADE, ITEM_LIMITED_EDITION_SODA};
static const u16 sHealingBerries[] = {ITEM_ORAN_BERRY, ITEM_SITRUS_BERRY};
static const u16 sStatusBerries[] = {ITEM_CHERI_BERRY, ITEM_CHESTO_BERRY, ITEM_PECHA_BERRY, ITEM_RAWST_BERRY,
                                     ITEM_ASPEAR_BERRY, ITEM_PERSIM_BERRY, ITEM_LUM_BERRY};

EWRAM_DATA static u16 sDrink = 0;
EWRAM_DATA static u16 sBerries[MAX_BERRIES] = {0};
EWRAM_DATA static u8 sNumBerries = 0;
EWRAM_DATA static u16 sChoices[MAX_CHOICES] = {0};     // (the list on screen: an item each; ITEM_NONE: MIX)

static bool32 IsIn(u32 item, const u16 *list, u32 n)
{
    u32 i;
    for (i = 0; i < n; i++)
        if (list[i] == item)
            return TRUE;
    return FALSE;
}

static bool32 IsHealing(u32 item) { return IsIn(item, sHealingBerries, ARRAY_COUNT(sHealingBerries)); }
static bool32 IsStatus(u32 item)  { return IsIn(item, sStatusBerries, ARRAY_COUNT(sStatusBerries)); }

// a type berry's type (TYPE_NONE: not one)
static enum Type TypeBerryType(u32 item)
{
    u32 t;
    for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES; t++)
        if (item != ITEM_NONE && GetTeraBerry(t) == item)
            return t;
    return TYPE_NONE;
}

// What a mix makes (ITEM_NONE: nothing). A LIMITED EDITION SODA's: the soda itself (it's then given).
u32 DrinkMachine_Recipe(u32 drink, const u16 *berries, u32 n)
{
    u32 a = n > 0 ? berries[0] : ITEM_NONE, b = n > 1 ? berries[1] : ITEM_NONE;
    if (drink == ITEM_LIMITED_EDITION_SODA)
        return (n == 1 && TypeBerryType(a) != TYPE_NONE) ? ITEM_LIMITED_EDITION_SODA : ITEM_NONE;
    if (!IsIn(drink, sDrinks, 3))
        return ITEM_NONE;
    if (n == 1)
    {
        if (IsHealing(a))
        {
            if (drink == ITEM_FRESH_WATER)
                return ITEM_SUPER_POTION;
            if (drink == ITEM_SODA_POP || a == ITEM_ORAN_BERRY)
                return ITEM_HYPER_POTION;
            return ITEM_MAX_POTION;
        }
        if (IsStatus(a))
            return ITEM_FULL_HEAL;
        if (a == ITEM_LEPPA_BERRY)
            return drink == ITEM_FRESH_WATER ? ITEM_ETHER : drink == ITEM_SODA_POP ? ITEM_ELIXIR : ITEM_MAX_ELIXIR;
        return ITEM_NONE;
    }
    if (n == 2)
    {
        if (IsHealing(a) && IsHealing(b))
            return (drink == ITEM_LEMONADE && a == ITEM_SITRUS_BERRY && b == ITEM_SITRUS_BERRY) ? ITEM_MAX_REVIVE : ITEM_REVIVE;
        if (drink == ITEM_LEMONADE && ((a == ITEM_SITRUS_BERRY && b == ITEM_LUM_BERRY) || (a == ITEM_LUM_BERRY && b == ITEM_SITRUS_BERRY)))
            return ITEM_FULL_RESTORE;
    }
    return ITEM_NONE;
}

// ---- The script's specials (data/scripts/drink_machine.inc) ----
void DrinkMachine_Begin(void)
{
    sDrink = ITEM_NONE;
    sNumBerries = 0;
    sBerries[0] = sBerries[1] = ITEM_NONE;
}

// (the menu frees each entry's name when it closes: src/script_menu.c FreeListMenuItems)
static void PushChoice(u32 n, u32 item, const u8 *label)
{
    struct ListMenuItem entry;
    u8 *name = Alloc(NAME_LEN), *end;
    sChoices[n] = item;
    if (label != NULL)
    {
        StringCopy(name, label);
    }
    else
    {
        // (its name, and how many the player has: as many as are left after what's chosen)
        u32 have = CountTotalItemQuantityInBag(item), i;
        for (i = 0; i < sNumBerries; i++)
            have -= (sBerries[i] == item);
        end = StringCopy(name, GetItemName(item));
        end = StringCopy(end, COMPOUND_STRING(" x"));
        ConvertIntToDecimalStringN(end, have, STR_CONV_MODE_LEFT_ALIGN, 3);
    }
    entry.name = name;
    entry.id = n;
    MultichoiceDynamic_PushElement(entry);
}

// Special: the drinks the player has, as the next menu (VAR_RESULT: how many)
void DrinkMachine_PushDrinks(void)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sDrinks); i++)
        if (CheckBagHasItem(sDrinks[i], 1))
            PushChoice(n++, sDrinks[i], NULL);
    gSpecialVar_Result = n;
}

// Special: VAR_RESULT = the menu's choice
void DrinkMachine_PickDrink(void)
{
    sDrink = gSpecialVar_Result < MAX_CHOICES ? sChoices[gSpecialVar_Result] : ITEM_NONE;
    StringCopy(gStringVar1, GetItemName(sDrink));
}

static bool32 HasSpare(u32 item)
{
    u32 have = CountTotalItemQuantityInBag(item), i;
    for (i = 0; i < sNumBerries; i++)
        have -= (sBerries[i] == item);
    return have > 0;
}

// Special: the berries that could go in next, as the next menu, MIX first once there's a berry in
// (VAR_RESULT: how many entries; 0: nothing more can go in, mix now. VAR_0x8005: berries in so far)
void DrinkMachine_PushBerries(void)
{
    u32 n = 0, i, t;
    gSpecialVar_0x8005 = sNumBerries;
    gSpecialVar_Result = 0;
    if (sNumBerries >= MAX_BERRIES || (sDrink == ITEM_LIMITED_EDITION_SODA && sNumBerries >= 1))
        return;
    if (sNumBerries > 0)
        PushChoice(n++, ITEM_NONE, COMPOUND_STRING("MIX!"));
    if (sDrink == ITEM_LIMITED_EDITION_SODA)
    {
        for (t = TYPE_NORMAL; t < NUMBER_OF_MON_TYPES && n < MAX_CHOICES; t++)
            if (GetTeraBerry(t) != ITEM_NONE && HasSpare(GetTeraBerry(t)))
                PushChoice(n++, GetTeraBerry(t), NULL);
    }
    else
    {
        for (i = 0; i < ARRAY_COUNT(sHealingBerries); i++)
            if (HasSpare(sHealingBerries[i]))
                PushChoice(n++, sHealingBerries[i], NULL);
        for (i = 0; i < ARRAY_COUNT(sStatusBerries); i++)
            if (HasSpare(sStatusBerries[i]))
                PushChoice(n++, sStatusBerries[i], NULL);
        if (HasSpare(ITEM_LEPPA_BERRY))
            PushChoice(n++, ITEM_LEPPA_BERRY, NULL);
    }
    gSpecialVar_Result = (sNumBerries > 0 && n == 1) ? 0 : n;   // (only MIX: just mix)
}

// Special: VAR_RESULT = the menu's choice. VAR_RESULT then TRUE: mix now; FALSE: a berry went in.
void DrinkMachine_PickBerry(void)
{
    u32 item = gSpecialVar_Result < MAX_CHOICES ? sChoices[gSpecialVar_Result] : ITEM_NONE;
    gSpecialVar_Result = TRUE;
    if (item == ITEM_NONE || sNumBerries >= MAX_BERRIES)
        return;
    sBerries[sNumBerries++] = item;
    gSpecialVar_Result = FALSE;
}

// Special: the mix. VAR_RESULT = DRINK_MIX_*; STR_VAR_1 the drink made (or the soda's type)
void DrinkMachine_Mix(void)
{
    u32 made = DrinkMachine_Recipe(sDrink, sBerries, sNumBerries), i;
    gSpecialVar_Result = DRINK_MIX_NOTHING;
    if (made == ITEM_NONE)
        return;
    if (made == ITEM_LIMITED_EDITION_SODA)
    {
        StringCopy(gStringVar1, gTypesInfo[TypeBerryType(sBerries[0])].name);
        gSpecialVar_Result = DRINK_MIX_SODA;
        return;
    }
    if (!CheckBagHasSpace(made, 1))
    {
        StringCopy(gStringVar1, GetItemName(made));
        gSpecialVar_Result = DRINK_MIX_NO_ROOM;
        return;
    }
    RemoveBagItem(sDrink, 1);
    for (i = 0; i < sNumBerries; i++)
        RemoveBagItem(sBerries[i], 1);
    AddBagItem(made, 1);
    gSpecialVar_0x8009 = made;
    StringCopy(gStringVar1, GetItemName(made));
    gSpecialVar_Result = DRINK_MIX_MADE;
}

// The soda's type, given to a Pokémon (DRINK_SODA_*); nothing's used up unless it worked
u32 DrinkMachine_GiveSecondType(struct Pokemon *mon, enum Type type)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    u8 value = type;
    if (species == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return DRINK_SODA_CANT;
    if (GetMonType(mon, 0) == type)
        return DRINK_SODA_FIRST_TYPE;
    if (GetMonType(mon, 1) == type)
        return DRINK_SODA_ALREADY;
    SetMonData(mon, MON_DATA_SECOND_TYPE, &value);
    return DRINK_SODA_DONE;
}

// Special (after ChoosePartyMon: VAR_0x8004 the Pokémon): the soda's given. VAR_RESULT = DRINK_SODA_*;
// STR_VAR_1 the Pokémon, STR_VAR_2 the type
void DrinkMachine_ApplySoda(void)
{
    struct Pokemon *mon;
    enum Type type = TypeBerryType(sBerries[0]);
    gSpecialVar_Result = DRINK_SODA_CANT;
    if (gSpecialVar_0x8004 >= PARTY_SIZE || sDrink != ITEM_LIMITED_EDITION_SODA || type == TYPE_NONE)
        return;
    mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, gTypesInfo[type].name);
    gSpecialVar_Result = DrinkMachine_GiveSecondType(mon, type);
    if (gSpecialVar_Result == DRINK_SODA_DONE)
    {
        RemoveBagItem(ITEM_LIMITED_EDITION_SODA, 1);
        RemoveBagItem(sBerries[0], 1);
    }
}

// ---- The girl on the Celadon roof ----
// Special: the machine and the first soda, once her drinks are out
void DrinkGirl_GiveMachine(void)
{
    FlagSet(FLAG_GOT_DRINK_MACHINE);
    GymDecor_Give(GYM_DECOR_DRINK_MACHINE);
}

// Special: VAR_RESULT = the sodas she owes the player now (one for each type mastered since)
void DrinkGirl_SodasOwed(void)
{
    u32 mastered = MasteryHall_Count(), given = VarGet(VAR_SODAS_FOR_MASTERY);
    gSpecialVar_Result = mastered > given ? mastered - given : 0;
}

// Special: one of them given
void DrinkGirl_GaveMasterySoda(void)
{
    VarSet(VAR_SODAS_FOR_MASTERY, VarGet(VAR_SODAS_FOR_MASTERY) + 1);
}

// (src/tournament.c) whether a tournament win can give a soda: once she's given one for every type
bool32 DrinkMachine_TournamentSodaUnlocked(void)
{
    return FlagGet(FLAG_GOT_DRINK_MACHINE) && VarGet(VAR_SODAS_FOR_MASTERY) >= DRINK_NUM_TYPES;
}
