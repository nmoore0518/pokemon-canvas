// Gym Trainer hack: stat training items.
//   Silver Bottle Cap  - one IV of your choice becomes 31
//   Gold Bottle Cap    - every IV becomes 31
//   Ability Capsule    - pick one of the species' regular abilities (not the hidden one: that's the
//                        Ability Patch)
//   Cilantro           - pick a nature (the stat nature, like a mint; the summary shows it too)
//   Multivitamin       - set all six EVs on an editing screen, within the game's limits
// Each item is used from the bag: it runs data/scripts/stat_items.inc, which asks for a Pokémon
// with ChoosePartyMon and then calls the specials below. The Pallet Town Mart sells them once
// they're unlocked (GymHack_PalletMart).
#include "global.h"
#include "synthetic_abilities.h"
#include "stat_items.h"
#include "event_data.h"
#include "item.h"
#include "item_menu.h"
#include "party_menu.h"
#include "list_menu.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "pokemon.h"
#include "script.h"
#include "script_menu.h"
#include "shop.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/items.h"
#include "constants/songs.h"

// Shown in this order; the values are the game's stat indexes (Speed is stored before Sp. Atk)
static const u8 sStatOrder[NUM_STATS] = {STAT_HP, STAT_ATK, STAT_DEF, STAT_SPATK, STAT_SPDEF, STAT_SPEED};

static const u8 *const sStatNames[NUM_STATS] =
{
    [STAT_HP]    = COMPOUND_STRING("HP"),
    [STAT_ATK]   = COMPOUND_STRING("ATTACK"),
    [STAT_DEF]   = COMPOUND_STRING("DEFENSE"),
    [STAT_SPEED] = COMPOUND_STRING("SPEED"),
    [STAT_SPATK] = COMPOUND_STRING("SP. ATK"),
    [STAT_SPDEF] = COMPOUND_STRING("SP. DEF"),
};

static const u8 *const sStatShortNames[NUM_STATS] =
{
    [STAT_HP]    = COMPOUND_STRING("HP"),
    [STAT_ATK]   = COMPOUND_STRING("ATK"),
    [STAT_DEF]   = COMPOUND_STRING("DEF"),
    [STAT_SPEED] = COMPOUND_STRING("SPE"),
    [STAT_SPATK] = COMPOUND_STRING("SPA"),
    [STAT_SPDEF] = COMPOUND_STRING("SPD"),
};

static struct Pokemon *ChosenMon(void)
{
    return &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
}

// ---------------------------------------------------------------------------
// The rules (also used by the tests)
// ---------------------------------------------------------------------------

bool32 StatItems_MaxIv(struct Pokemon *mon, u32 stat)
{
    u32 iv = MAX_PER_STAT_IVS;
    if (stat >= NUM_STATS || GetMonData(mon, MON_DATA_HP_IV + stat) >= MAX_PER_STAT_IVS)
        return FALSE;
    SetMonData(mon, MON_DATA_HP_IV + stat, &iv);
    CalculateMonStats(mon);
    return TRUE;
}

bool32 StatItems_MaxAllIvs(struct Pokemon *mon)
{
    bool32 changed = FALSE;
    u32 i;
    for (i = 0; i < NUM_STATS; i++)
    {
        if (GetMonData(mon, MON_DATA_HP_IV + i) < MAX_PER_STAT_IVS)
        {
            u32 iv = MAX_PER_STAT_IVS;
            SetMonData(mon, MON_DATA_HP_IV + i, &iv);
            changed = TRUE;
        }
    }
    CalculateMonStats(mon);
    return changed;
}

// The regular ability slots (0, 1) that give different abilities, in order. Returns how many.
// (The hidden ability is the Ability Patch's job.)
u32 StatItems_GetAbilityChoices(struct Pokemon *mon, u8 *slots)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Ability seen[NUM_ABILITY_SLOTS];
    u32 i, j, n = 0;
    if (GetMonData(mon, MON_DATA_ABILITY_NUM) == ABILITY_NUM_SYNTHETIC)
        return 0; // Pokémon Canvas: a synthetic ability is for good
    for (i = 0; i < NUM_NORMAL_ABILITY_SLOTS; i++)
    {
        enum Ability ability = gSpeciesInfo[species].abilities[i];
        if (ability == ABILITY_NONE)
            continue;
        for (j = 0; j < n; j++)
        {
            if (seen[j] == ability)
                break;
        }
        if (j < n)
            continue;
        seen[n] = ability;
        slots[n++] = i;
    }
    return n;
}

bool32 StatItems_SetAbilitySlot(struct Pokemon *mon, u32 slot)
{
    u8 slots[NUM_ABILITY_SLOTS];
    u32 n = StatItems_GetAbilityChoices(mon, slots), i;
    if (GetMonAbility(mon) == GetAbilityBySpecies(GetMonData(mon, MON_DATA_SPECIES), slot))
        return FALSE;
    for (i = 0; i < n; i++)
    {
        if (slots[i] == slot)
        {
            SetMonData(mon, MON_DATA_ABILITY_NUM, &slot);
            return TRUE;
        }
    }
    return FALSE;
}

bool32 StatItems_SetNature(struct Pokemon *mon, u32 nature)
{
    if (nature >= NUM_NATURES || GetMonData(mon, MON_DATA_HIDDEN_NATURE) == nature)
        return FALSE;
    SetMonData(mon, MON_DATA_HIDDEN_NATURE, &nature);
    CalculateMonStats(mon);
    return TRUE;
}

bool32 StatItems_EvsAreValid(const u16 *evs)
{
    u32 i, total = 0;
    for (i = 0; i < NUM_STATS; i++)
    {
        if (evs[i] > MAX_PER_STAT_EVS)
            return FALSE;
        total += evs[i];
    }
    return total <= MAX_TOTAL_EVS;
}

bool32 StatItems_SetEvs(struct Pokemon *mon, const u16 *evs)
{
    u32 i;
    if (!StatItems_EvsAreValid(evs))
        return FALSE;
    for (i = 0; i < NUM_STATS; i++)
    {
        u8 ev = evs[i];
        SetMonData(mon, MON_DATA_HP_EV + i, &ev);
    }
    CalculateMonStats(mon);
    return TRUE;
}

// ---------------------------------------------------------------------------
// Specials used by data/scripts/stat_items.inc. VAR_0x8004 = party slot, VAR_ITEM_ID = item.
// ---------------------------------------------------------------------------

// VAR_RESULT: 0 = can use, 1 = it's an Egg, 2 = it would have no effect. Buffers the nickname.
void StatItems_CheckMon(void)
{
    struct Pokemon *mon = ChosenMon();
    u8 slots[NUM_ABILITY_SLOTS];
    u32 i;

    GetMonNickname(mon, gStringVar1);
    if (GetMonData(mon, MON_DATA_IS_EGG))
    {
        gSpecialVar_Result = 1;
        return;
    }
    gSpecialVar_Result = 0;
    switch ((u32)gSpecialVar_ItemId)
    {
    case ITEM_BOTTLE_CAP:
    case ITEM_GOLD_BOTTLE_CAP:
        gSpecialVar_Result = 2;
        for (i = 0; i < NUM_STATS; i++)
        {
            if (GetMonData(mon, MON_DATA_HP_IV + i) < MAX_PER_STAT_IVS)
                gSpecialVar_Result = 0;
        }
        break;
    case ITEM_ABILITY_CAPSULE:
        if (StatItems_GetAbilityChoices(mon, slots) < 2)
            gSpecialVar_Result = 2;
        break;
    }
}

static void PushChoice(const u8 *text, u32 id)
{
    struct ListMenuItem item;
    u8 *name = Alloc(40);
    StringCopy(name, text);
    item.name = name;
    item.id = id;
    MultichoiceDynamic_PushElement(item);
}

// Silver Bottle Cap: "ATTACK  23", or "ATTACK  MAX"
void StatItems_PushIvList(void)
{
    struct Pokemon *mon = ChosenMon();
    u8 *str = Alloc(40);
    u32 i;
    for (i = 0; i < NUM_STATS; i++)
    {
        u32 stat = sStatOrder[i], iv = GetMonData(mon, MON_DATA_HP_IV + stat);
        u8 *end = StringCopy(str, sStatNames[stat]);
        end = StringCopy(end, COMPOUND_STRING("{CLEAR_TO 56}IV "));
        if (iv >= MAX_PER_STAT_IVS)
            StringCopy(end, COMPOUND_STRING("MAX"));
        else
            ConvertIntToDecimalStringN(end, iv, STR_CONV_MODE_LEFT_ALIGN, 2);
        PushChoice(str, stat);
    }
    Free(str);
}

// Ability Capsule: one entry per different ability; ids are ability slots. VAR_0x8005 = current one.
void StatItems_PushAbilityList(void)
{
    struct Pokemon *mon = ChosenMon();
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Ability current = GetMonAbility(mon);
    u8 slots[NUM_ABILITY_SLOTS];
    u8 *str = Alloc(40);
    u32 n = StatItems_GetAbilityChoices(mon, slots), i;
    gSpecialVar_0x8005 = 0;
    for (i = 0; i < n; i++)
    {
        enum Ability ability = GetAbilityBySpecies(species, slots[i]);
        u8 *end = StringCopy(str, gAbilitiesInfo[ability].name);
        if (ability == current)
        {
            StringCopy(end, COMPOUND_STRING(" (now)"));
            gSpecialVar_0x8005 = i;
        }
        PushChoice(str, slots[i]);
    }
    Free(str);
}

// Cilantro: "Adamant +ATK -SPA"; ids are natures. VAR_0x8005 = current one.
void StatItems_PushNatureList(void)
{
    u32 current = GetMonData(ChosenMon(), MON_DATA_HIDDEN_NATURE), i;
    u8 *str = Alloc(40);
    for (i = 0; i < NUM_NATURES; i++)
    {
        const struct NatureInfo *info = &gNaturesInfo[i];
        u8 *end = StringCopy(str, info->name);
        end = StringCopy(end, COMPOUND_STRING("{CLEAR_TO 52}"));
        if (info->statUp == info->statDown)
        {
            end = StringCopy(end, COMPOUND_STRING("-"));
        }
        else
        {
            end = StringCopy(end, COMPOUND_STRING("+"));
            end = StringCopy(end, sStatShortNames[info->statUp]);
            end = StringCopy(end, COMPOUND_STRING(" -"));
            end = StringCopy(end, sStatShortNames[info->statDown]);
        }
        if (i == current)
            StringCopy(end, COMPOUND_STRING(" (now)"));
        PushChoice(str, i);
    }
    gSpecialVar_0x8005 = current;
    Free(str);
}

// Uses the item on the chosen Pokémon (VAR_0x8005 = the stat, ability slot or nature picked).
// VAR_RESULT: TRUE if it worked. The message to show is put in gStringVar4.
void StatItems_Apply(void)
{
    struct Pokemon *mon = ChosenMon();
    u32 choice = gSpecialVar_0x8005;
    bool32 ok = FALSE;

    GetMonNickname(mon, gStringVar1);
    switch ((u32)gSpecialVar_ItemId)
    {
    case ITEM_BOTTLE_CAP:
        StringCopy(gStringVar2, sStatNames[choice < NUM_STATS ? choice : 0]);
        ok = StatItems_MaxIv(mon, choice);
        StringExpandPlaceholders(gStringVar4, ok
            ? COMPOUND_STRING("{STR_VAR_1}'s {STR_VAR_2} reached\nits full potential!")
            : COMPOUND_STRING("{STR_VAR_1}'s {STR_VAR_2} is already\nat its full potential."));
        break;
    case ITEM_GOLD_BOTTLE_CAP:
        ok = StatItems_MaxAllIvs(mon);
        StringExpandPlaceholders(gStringVar4,
            COMPOUND_STRING("Every one of {STR_VAR_1}'s stats\nreached its full potential!"));
        break;
    case ITEM_ABILITY_CAPSULE:
        ok = StatItems_SetAbilitySlot(mon, choice);
        StringCopy(gStringVar2, gAbilitiesInfo[GetMonAbility(mon)].name);
        StringExpandPlaceholders(gStringVar4, ok
            ? COMPOUND_STRING("{STR_VAR_1}'s ABILITY became\n{STR_VAR_2}!")
            : COMPOUND_STRING("{STR_VAR_1} already has\n{STR_VAR_2}."));
        break;
    case ITEM_CILANTRO:
        ok = StatItems_SetNature(mon, choice);
        StringCopy(gStringVar2, gNaturesInfo[GetMonData(mon, MON_DATA_HIDDEN_NATURE)].name);
        StringExpandPlaceholders(gStringVar4, ok
            ? COMPOUND_STRING("{STR_VAR_1} took a sniff…\nIt now has a {STR_VAR_2} nature!")
            : COMPOUND_STRING("{STR_VAR_1} already has a\n{STR_VAR_2} nature."));
        break;
    }
    gSpecialVar_Result = ok;
}

// ---------------------------------------------------------------------------
// Multivitamin: the EV editing screen
// ---------------------------------------------------------------------------
#define EV_STEP 4               // a stat point every 4 EVs at level 100

struct EvEditor
{
    struct Pokemon preview;     // a copy, to show what the stats would be
    u16 evs[NUM_STATS];
    u16 startEvs[NUM_STATS];
    u8 cursor;                  // index into sStatOrder
    u8 windowId;
    u8 helpWindowId;
};

static EWRAM_DATA struct EvEditor *sEvEditor;

static u32 EvTotal(const u16 *evs)
{
    u32 i, total = 0;
    for (i = 0; i < NUM_STATS; i++)
        total += evs[i];
    return total;
}

// The most this stat can have, given the others
static u32 EvRoom(const u16 *evs, u32 stat)
{
    u32 room = MAX_TOTAL_EVS - (EvTotal(evs) - evs[stat]);
    return room < MAX_PER_STAT_EVS ? room : MAX_PER_STAT_EVS;
}

static void DrawEvEditor(void)
{
    struct EvEditor *ed = sEvEditor;
    u8 *str = Alloc(64);
    u8 *end;
    u32 i;
    static const u8 sGray[] = {TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
    static const u8 sRed[] = {TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_GRAY};

    StatItems_SetEvs(&ed->preview, ed->evs);
    FillWindowPixelBuffer(ed->windowId, PIXEL_FILL(1));

    GetMonNickname(&ed->preview, gStringVar1);
    AddTextPrinterParameterized3(ed->windowId, FONT_NORMAL, 8, 1, sGray, TEXT_SKIP_DRAW, gStringVar1);
    end = StringCopy(str, COMPOUND_STRING("TOTAL "));
    end = ConvertIntToDecimalStringN(end, EvTotal(ed->evs), STR_CONV_MODE_LEFT_ALIGN, 3);
    end = StringCopy(end, COMPOUND_STRING("/"));
    ConvertIntToDecimalStringN(end, MAX_TOTAL_EVS, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized3(ed->windowId, FONT_NORMAL, 120, 1,
                                 EvTotal(ed->evs) >= MAX_TOTAL_EVS ? sRed : sGray, TEXT_SKIP_DRAW, str);
    AddTextPrinterParameterized3(ed->windowId, FONT_SMALL, 100, 12, sGray, TEXT_SKIP_DRAW, COMPOUND_STRING("EVs"));
    AddTextPrinterParameterized3(ed->windowId, FONT_SMALL, 160, 12, sGray, TEXT_SKIP_DRAW, COMPOUND_STRING("STAT"));
    for (i = 0; i < NUM_STATS; i++)
    {
        u32 stat = sStatOrder[i], y = 23 + i * 12;
        bool32 full = ed->evs[stat] >= EvRoom(ed->evs, stat);
        if (i == ed->cursor)
            AddTextPrinterParameterized3(ed->windowId, FONT_NORMAL, 0, y, sGray, TEXT_SKIP_DRAW, gText_SelectorArrow2);
        AddTextPrinterParameterized3(ed->windowId, FONT_NORMAL, 8, y, sGray, TEXT_SKIP_DRAW, sStatNames[stat]);
        end = StringCopy(str, i == ed->cursor ? COMPOUND_STRING("{LEFT_ARROW} ") : COMPOUND_STRING("  "));
        end = ConvertIntToDecimalStringN(end, ed->evs[stat], STR_CONV_MODE_RIGHT_ALIGN, 3);
        if (i == ed->cursor)
            StringCopy(end, COMPOUND_STRING(" {RIGHT_ARROW}"));
        AddTextPrinterParameterized3(ed->windowId, FONT_NORMAL, 84, y, full ? sRed : sGray, TEXT_SKIP_DRAW, str);
        ConvertIntToDecimalStringN(str, GetMonData(&ed->preview, MON_DATA_MAX_HP + stat), STR_CONV_MODE_RIGHT_ALIGN, 3);
        AddTextPrinterParameterized3(ed->windowId, FONT_NORMAL, 160, y, sGray, TEXT_SKIP_DRAW, str);
    }
    CopyWindowToVram(ed->windowId, COPYWIN_FULL);
    Free(str);
}

static void DrawEvEditorHelp(void)
{
    static const u8 sGray[] = {TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
    u8 *str = Alloc(80);
    FillWindowPixelBuffer(sEvEditor->helpWindowId, PIXEL_FILL(1));
    AddTextPrinterParameterized3(sEvEditor->helpWindowId, FONT_SMALL, 2, 1, sGray, TEXT_SKIP_DRAW,
        COMPOUND_STRING("{LEFT_ARROW}{RIGHT_ARROW}: 4 at a time    L/R: none or max"));
    StringCopy(str, COMPOUND_STRING("A: use   B: cancel   Max "));
    ConvertIntToDecimalStringN(str + StringLength(str), MAX_PER_STAT_EVS, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringAppend(str, COMPOUND_STRING(" each, "));
    ConvertIntToDecimalStringN(str + StringLength(str), MAX_TOTAL_EVS, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringAppend(str, COMPOUND_STRING(" total"));
    AddTextPrinterParameterized3(sEvEditor->helpWindowId, FONT_SMALL, 2, 16, sGray, TEXT_SKIP_DRAW, str);
    CopyWindowToVram(sEvEditor->helpWindowId, COPYWIN_FULL);
    Free(str);
}

static void CloseEvEditor(u8 taskId, u32 result)
{
    ClearStdWindowAndFrameToTransparent(sEvEditor->windowId, TRUE);
    RemoveWindow(sEvEditor->windowId);
    ClearStdWindowAndFrameToTransparent(sEvEditor->helpWindowId, TRUE);
    RemoveWindow(sEvEditor->helpWindowId);
    FREE_AND_SET_NULL(sEvEditor);
    gSpecialVar_Result = result;
    DestroyTask(taskId);
    ScriptContext_Enable();
}

static void Task_EvEditor(u8 taskId)
{
    struct EvEditor *ed = sEvEditor;
    u32 stat = sStatOrder[ed->cursor];
    s32 value = ed->evs[stat];
    s32 room = EvRoom(ed->evs, stat);

    if (JOY_NEW(A_BUTTON))
    {
        u32 i;
        bool32 changed = FALSE;
        for (i = 0; i < NUM_STATS; i++)
        {
            if (ed->evs[i] != ed->startEvs[i])
                changed = TRUE;
        }
        if (!changed)
        {
            PlaySE(SE_SELECT);
            CloseEvEditor(taskId, 2);
            return;
        }
        PlaySE(SE_SELECT);
        StatItems_SetEvs(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004], ed->evs);
        CloseEvEditor(taskId, 1);
        return;
    }
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        CloseEvEditor(taskId, 0);
        return;
    }
    if (JOY_REPEAT(DPAD_UP))
    {
        ed->cursor = (ed->cursor + NUM_STATS - 1) % NUM_STATS;
        PlaySE(SE_SELECT);
        DrawEvEditor();
    }
    else if (JOY_REPEAT(DPAD_DOWN))
    {
        ed->cursor = (ed->cursor + 1) % NUM_STATS;
        PlaySE(SE_SELECT);
        DrawEvEditor();
    }
    else if (JOY_REPEAT(DPAD_RIGHT | DPAD_LEFT | L_BUTTON | R_BUTTON))
    {
        s32 target = value;
        if (JOY_REPEAT(DPAD_RIGHT))
            target = value + EV_STEP - (value % EV_STEP);   // up to the next multiple of 4
        else if (JOY_REPEAT(DPAD_LEFT))
            target = (value % EV_STEP) ? value - (value % EV_STEP) : value - EV_STEP;
        else if (JOY_NEW(R_BUTTON))
            target = room;
        else if (JOY_NEW(L_BUTTON))
            target = 0;
        if (target > room)
            target = room;
        if (target < 0)
            target = 0;
        if (target == value)
        {
            if (JOY_NEW(DPAD_RIGHT | DPAD_LEFT | L_BUTTON | R_BUTTON))
                PlaySE(SE_FAILURE);   // at a limit
            return;
        }
        ed->evs[stat] = target;
        PlaySE(SE_BALL_TRAY_EXIT);
        DrawEvEditor();
    }
}

// Special (waitstate): VAR_RESULT 1 = EVs set, 2 = nothing changed, 0 = cancelled
void StatItems_OpenEvEditor(void)
{
    // Tiles: the editor uses 0x20-0x157 and the help 0x21E-0x285, clear of the message box (0x194-0x21D)
    struct WindowTemplate template = CreateWindowTemplate(0, 2, 1, 26, 12, 15, 0x20);
    struct WindowTemplate helpTemplate = CreateWindowTemplate(0, 2, 15, 26, 4, 15, 0x21E);
    struct Pokemon *mon = ChosenMon();
    u32 i;

    sEvEditor = AllocZeroed(sizeof(*sEvEditor));
    sEvEditor->preview = *mon;
    for (i = 0; i < NUM_STATS; i++)
        sEvEditor->evs[i] = sEvEditor->startEvs[i] = GetMonData(mon, MON_DATA_HP_EV + i);
    sEvEditor->windowId = AddWindow(&template);
    SetStandardWindowBorderStyle(sEvEditor->windowId, FALSE);
    PutWindowTilemap(sEvEditor->windowId);
    sEvEditor->helpWindowId = AddWindow(&helpTemplate);
    SetStandardWindowBorderStyle(sEvEditor->helpWindowId, FALSE);
    PutWindowTilemap(sEvEditor->helpWindowId);
    DrawEvEditor();
    DrawEvEditorHelp();
    CreateTask(Task_EvEditor, 80);
}

// ---------------------------------------------------------------------------
// The Pallet Town Mart: its usual stock plus whichever stat items are unlocked
// ---------------------------------------------------------------------------
static const u16 sPalletMartBaseItems[] =
{
    ITEM_POKE_BALL, ITEM_POTION, ITEM_ANTIDOTE, ITEM_PARALYZE_HEAL,
    ITEM_AWAKENING, ITEM_BURN_HEAL, ITEM_ICE_HEAL, ITEM_REPEL,
};

static const struct { u16 item; u16 flag; } sStatItemUnlocks[] =
{
    {ITEM_BOTTLE_CAP,      FLAG_BADGE04_GET},     // Erika
    {ITEM_ABILITY_CAPSULE, FLAG_BADGE06_GET},     // Sabrina
    {ITEM_ABILITY_PATCH,   FLAG_BADGE06_GET},
    {ITEM_CILANTRO,        FLAG_BADGE07_GET},     // Blaine
    {ITEM_GOLD_BOTTLE_CAP, FLAG_SYS_GAME_CLEAR},  // the Elite Four
    {ITEM_MULTIVITAMIN,    FLAG_SYS_GAME_CLEAR},
};

static EWRAM_DATA u16 sPalletMartItems[ARRAY_COUNT(sPalletMartBaseItems) + ARRAY_COUNT(sStatItemUnlocks) + 1];

const u16 *StatItems_GetPalletMartItems(void)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sPalletMartBaseItems); i++)
        sPalletMartItems[n++] = sPalletMartBaseItems[i];
    for (i = 0; i < ARRAY_COUNT(sStatItemUnlocks); i++)
    {
        if (FlagGet(sStatItemUnlocks[i].flag))
            sPalletMartItems[n++] = sStatItemUnlocks[i].item;
    }
    sPalletMartItems[n] = ITEM_NONE;
    return sPalletMartItems;
}

// Special (waitstate), in place of the pokemart command
void GymHack_PalletMart(void)
{
    CreatePokemartMenu(StatItems_GetPalletMartItems());
}
