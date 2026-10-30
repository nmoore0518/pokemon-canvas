// Pokémon Canvas: the Dad story (data/scripts/dad_story.inc). Dad's notes as full pages, his ace
// POKéMON in GIOVANNI's story, GIOVANNI's synthesized last team and DAD's CHANSEY.
#include "global.h"
#include "dad_story.h"
#include "basement_machines.h"
#include "event_data.h"
#include "item.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "pokeball.h"
#include "pokemon.h"
#include "tv.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "synthetic_abilities.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/dad_story.h"
#include "constants/items.h"
#include "constants/pokeball.h"
#include "constants/songs.h"
#include "constants/species.h"

extern const u8 DadStory_Text_Note1[];
extern const u8 DadStory_Text_Note2[];
extern const u8 DadStory_Text_Note3[];
extern const u8 DadStory_Text_Note4[];
extern const u8 DadStory_Text_Notebook[];

// ---------------------------------------------------------------------------
// A note as a full page: a big window over the field, seven lines a page, A for the next page (the
// last one closes it), B closes it. Special (waitstate), VAR_0x8004 = NOTE_PAGE_*.
// ---------------------------------------------------------------------------
static const u8 *const sNotePages[NOTE_PAGE_COUNT] =
{
    [NOTE_PAGE_NOTE_1]   = DadStory_Text_Note1,
    [NOTE_PAGE_NOTE_2]   = DadStory_Text_Note2,
    [NOTE_PAGE_NOTE_3]   = DadStory_Text_Note3,
    [NOTE_PAGE_NOTE_4]   = DadStory_Text_Note4,
    [NOTE_PAGE_NOTEBOOK] = DadStory_Text_Notebook,
};

#define NOTE_WIDTH   26
#define NOTE_HEIGHT  14
#define NOTE_LINE_HEIGHT 14

#define tWindowId   data[0]
#define tPage       data[1]
#define tPages      data[2]
#define tNote       data[3]

static EWRAM_DATA u8 *sNoteBuffer = NULL;

// The text of page `page` (pages end at \p) copied into the buffer, its lines ending at \n
static const u8 *NotePage(const u8 *text, u32 page, u32 *count)
{
    u32 n = 0, len = 0;
    const u8 *p = text;
    *count = 1;
    for (; *p != EOS; p++)
    {
        if (*p == CHAR_PROMPT_CLEAR)
            (*count)++;
    }
    for (p = text; *p != EOS; p++)
    {
        if (*p == CHAR_PROMPT_CLEAR)
        {
            n++;
            continue;
        }
        if (n == page && len < 600)
            sNoteBuffer[len++] = *p;
        else if (n > page)
            break;
    }
    sNoteBuffer[len] = EOS;
    return sNoteBuffer;
}

static void DrawNotePage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    static const u8 sColors[] = {TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
    u32 count;
    const u8 *text = NotePage(sNotePages[tNote], tPage, &count);
    u8 pageNum[12], *end;

    tPages = count;
    FillWindowPixelBuffer(tWindowId, PIXEL_FILL(1));
    AddTextPrinterParameterized4(tWindowId, FONT_NORMAL, 6, 2, 0, NOTE_LINE_HEIGHT - 16 + 2, sColors, TEXT_SKIP_DRAW, text);
    if (count > 1)
    {
        end = ConvertIntToDecimalStringN(pageNum, tPage + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
        *end++ = CHAR_SLASH;
        ConvertIntToDecimalStringN(end, count, STR_CONV_MODE_LEFT_ALIGN, 2);
        AddTextPrinterParameterized3(tWindowId, FONT_SMALL, NOTE_WIDTH * 8 - 8 - GetStringWidth(FONT_SMALL, pageNum, 0),
                                     NOTE_HEIGHT * 8 - 13, sColors, TEXT_SKIP_DRAW, pageNum);
    }
    CopyWindowToVram(tWindowId, COPYWIN_FULL);
}

static void Task_NotePage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (JOY_NEW(A_BUTTON) && tPage + 1 < tPages)
    {
        PlaySE(SE_SELECT);
        tPage++;
        DrawNotePage(taskId);
    }
    else if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(SE_SELECT);
        ClearStdWindowAndFrameToTransparent(tWindowId, TRUE);
        RemoveWindow(tWindowId);
        Free(sNoteBuffer);
        sNoteBuffer = NULL;
        DestroyTask(taskId);
        ScriptContext_Enable();
    }
}

void ShowNotePage(void)
{
    // (field tiles 0x20-0x193: clear of the message box, like the EV editor)
    struct WindowTemplate template = CreateWindowTemplate(0, 2, 2, NOTE_WIDTH, NOTE_HEIGHT, 15, 0x20);
    u8 taskId = CreateTask(Task_NotePage, 80);
    s16 *data = gTasks[taskId].data;

    tNote = gSpecialVar_0x8004 < NOTE_PAGE_COUNT ? gSpecialVar_0x8004 : 0;
    tPage = 0;
    sNoteBuffer = Alloc(640);
    tWindowId = AddWindow(&template);
    SetStandardWindowBorderStyle(tWindowId, FALSE);
    PutWindowTilemap(tWindowId);
    DrawNotePage(taskId);
}

#undef tWindowId
#undef tPage
#undef tPages
#undef tNote

// ---------------------------------------------------------------------------
// Dad's ace: a fully evolved starter of a type the player's own type is strong against (by the
// player's GYM type; the three with no such starter get a neutral one). STR_VAR_1 = its name.
// ---------------------------------------------------------------------------
static const u16 sDadsAce[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = SPECIES_CINDERACE,    // (Normal hits nothing super effectively)
    [TYPE_FIGHTING] = SPECIES_EMPOLEON,
    [TYPE_FLYING]   = SPECIES_CHESNAUGHT,
    [TYPE_POISON]   = SPECIES_PRIMARINA,
    [TYPE_GROUND]   = SPECIES_TYPHLOSION,
    [TYPE_ROCK]     = SPECIES_CHARIZARD,
    [TYPE_BUG]      = SPECIES_MEOWSCARADA,
    [TYPE_GHOST]    = SPECIES_DELPHOX,
    [TYPE_STEEL]    = SPECIES_PRIMARINA,    // (no starter is weak to Steel)
    [TYPE_FIRE]     = SPECIES_VENUSAUR,
    [TYPE_WATER]    = SPECIES_CHARIZARD,
    [TYPE_GRASS]    = SPECIES_BLASTOISE,
    [TYPE_ELECTRIC] = SPECIES_BLASTOISE,
    [TYPE_PSYCHIC]  = SPECIES_BLAZIKEN,
    [TYPE_ICE]      = SPECIES_TORTERRA,
    [TYPE_DRAGON]   = SPECIES_SCEPTILE,     // (no starter is weak to Dragon)
    [TYPE_DARK]     = SPECIES_SKELEDIRGE,
    [TYPE_FAIRY]    = SPECIES_GRENINJA,
};

u16 DadStory_GetAce(u32 type)
{
    if (type >= NUMBER_OF_MON_TYPES || sDadsAce[type] == SPECIES_NONE)
        return SPECIES_CHARIZARD;
    return sDadsAce[type];
}

void DadStory_BufferAce(void)
{
    StringCopy(gStringVar1, GetSpeciesName(DadStory_GetAce(VarGet(VAR_PLAYER_TYPE))));
}

// VAR_RESULT = the Type Ball of the player's GYM type (MOM's gift from Dad, after NUGGET BRIDGE)
void DadStory_GetGymTypeBall(void)
{
    gSpecialVar_Result = GetTypeBallItem(VarGet(VAR_PLAYER_TYPE));
    if (gSpecialVar_Result == ITEM_NONE)
        gSpecialVar_Result = ITEM_NORMAL_BALL;
}

// GIOVANNI's last team (TRAINER_GIOVANNI_DAD_FINAL): every one with its synthetic ability, in the
// Type Ball of its primary type (so its weakest stat is up by half)
void DadStory_SynthesizeParty(struct Pokemon *party)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        enum Species species = GetMonData(&party[i], MON_DATA_SPECIES);
        u32 slot = ABILITY_NUM_SYNTHETIC, ball;
        if (species == SPECIES_NONE)
            continue;
        if (GetSpeciesSyntheticAbility(species) != ABILITY_NONE)
            SetMonData(&party[i], MON_DATA_ABILITY_NUM, &slot);
        ball = ItemIdToBallId(GetTypeBallItem(GetSpeciesType(species, 0)));
        SetMonData(&party[i], MON_DATA_POKEBALL, &ball);
        CalculateMonStats(&party[i]);
    }
}

// DAD's CHANSEY: Lv. 50, perfect IVs and no EVs, its synthetic ability (Triage), in a Normal Ball,
// from "DAD" (so it counts as traded), holding DAD's NOTEBOOK. VAR_RESULT = MON_GIVEN_TO_*.
void DadStory_GiveChansey(void)
{
    struct Pokemon *mon = AllocZeroed(sizeof(struct Pokemon));
    static const u8 sDad[] = _("DAD");
    u32 i, value, otId = GetPlayerIDAsU32() ^ 0x0001DAD0;
    bool32 shiny = FALSE;

    CreateMon(mon, SPECIES_CHANSEY_DAD, DADS_CHANSEY_LEVEL, Random32(), OTID_STRUCT_PRESET(otId));
    SetMonData(mon, MON_DATA_IS_SHINY, &shiny);
    value = MAX_PER_STAT_IVS;
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_IV + i, &value);
    value = 0;
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_EV + i, &value);
    value = ABILITY_NUM_SYNTHETIC;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &value);
    value = BALL_TYPE_NORMAL;
    SetMonData(mon, MON_DATA_POKEBALL, &value);
    value = ITEM_DADS_NOTEBOOK;
    SetMonData(mon, MON_DATA_HELD_ITEM, &value);
    SetMonData(mon, MON_DATA_OT_NAME, sDad);
    value = MALE;
    SetMonData(mon, MON_DATA_OT_GENDER, &value);
    CalculateMonStats(mon);
    gSpecialVar_Result = GiveScriptedMonToPlayer(mon, PARTY_SIZE);
    Free(mon);
}
