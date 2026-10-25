// Pokémon Canvas: the Gauntlet (scripts: data/scripts/gauntlet.inc; challengers: src/data/gauntlet.h,
// made by tools/gym_hack/build_gauntlet.py from tools/gym_hack/gauntlet_roster.py).
//
// Once the player has cleared every hall of the Gym Leader Convention and mastered every gym type,
// a hooded figure turns up at the gym: an assassin, they say, hired by the angry Gym Leaders, who has
// called in the strongest trainers of the Pokémon world. The challengers come one at a time, in
// order (VAR_GAUNTLET_NEXT), each with a level-100 team; any Pokémon may battle (no type rules). A
// loss changes nothing: the same challenger comes next time. The figure's clipboard shows who's been
// beaten, region by region.
//
// After the last of them: the figure is Bill. His Cell Separation System, run backwards on himself and
// a Clefable, made him CLEFABILL (Fairy/Ghost), in pain he can't end himself; he set it all up to find
// someone strong enough. His team: five shiny legendaries and himself. Beaten, he fades away and leaves
// his Palm Pilot (the same object, VAR_OBJ_GFX_ID_D): one WITHDRAW of any Pokémon, shiny, then his
// battle simulator runs the whole Gauntlet again (Bill last) for another.
#include "global.h"
#include "gauntlet.h"
#include "data.h"
#include "event_data.h"
#include "gym_challenge.h"
#include "gym_convention.h"
#include "gym_switch.h"
#include "gym_legend.h"
#include "event_object_movement.h"
#include "overworld.h"
#include "constants/map_event_ids.h"
#include "list_menu.h"
#include "field_effect.h"
#include "menu.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "window.h"
#include "constants/songs.h"
#include "main.h"
#include "malloc.h"
#include "pokemon.h"
#include "script.h"
#include "script_menu.h"
#include "string_util.h"
#include "title_card.h"
#include "constants/characters.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/gym_hack.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/rgb.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/vars.h"

struct GauntletTrainer
{
    const u8 *name;             // (on the title card)
    const u8 *battleName;       // (in battle: 10 letters at most)
    u8 trainerClass;
    bool8 female;
    u8 region;
    u16 pic;
    u16 gfx;
    const struct TrainerMon *party;
    const u8 *title;
    const u8 *games;
    const u8 *intro;
    const u8 *defeat;
    const u8 *after;
    const u8 *win;
};

#include "data/gauntlet.h"
#include "data/gauntlet_withdraw.h"

u8 CreateMonSprite_FieldMove(enum Species species, bool8 isShiny, u32 personality, s16 x, s16 y, u8 subpriority);

// The title card's band: each region's color
static const u16 sRegionColors[GAUNTLET_REGION_COUNT] =
{
    RGB(26, 6, 6),      // Kanto: red
    RGB(24, 19, 5),     // Johto: gold
    RGB(5, 18, 13),     // Hoenn: emerald
    RGB(10, 12, 27),    // Sinnoh: indigo
    RGB(13, 13, 15),    // Unova: black and white
    RGB(6, 16, 26),     // Kalos: blue
    RGB(28, 14, 4),     // Alola: sunset
    RGB(20, 6, 18),     // Galar: purple
    RGB(28, 9, 8),      // Paldea: scarlet
};

static u32 Next(void)
{
    u32 next = VarGet(VAR_GAUNTLET_NEXT);
    return next < GAUNTLET_COUNT ? next : GAUNTLET_COUNT;
}

// The challenger up now: Bill (GAUNTLET_COUNT) once they're all beaten, and last in the simulator
static u32 Index(void)
{
    u32 state = VarGet(VAR_GAUNTLET_STATE), next = VarGet(VAR_GAUNTLET_NEXT);
    if (state == GAUNTLET_ALL_BEATEN)
        return GAUNTLET_COUNT;
    if (state == GAUNTLET_SIMULATOR)
        return next < GAUNTLET_COUNT ? next : GAUNTLET_COUNT;
    return next < GAUNTLET_COUNT ? next : GAUNTLET_COUNT - 1;
}

static bool32 Eligible(void)
{
    return FlagGet(FLAG_CONVENTION_TROPHY_GRAND) && GymSwitch_AllTypesMastered();
}

// Map script (the gym's ON_TRANSITION): the figure comes once the player has done it all (its scene
// plays from the ON_FRAME table), and waits by the door while the Gauntlet is on
void Gauntlet_OnTransition(void)
{
    u32 state = VarGet(VAR_GAUNTLET_STATE);
    if (state == GAUNTLET_NONE && Eligible())
        VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ARRIVING);
    if (state == GAUNTLET_ACTIVE || state == GAUNTLET_ALL_BEATEN || state == GAUNTLET_SIMULATOR)
        FlagClear(FLAG_HIDE_GAUNTLET_FIGURE);
    else
        FlagSet(FLAG_HIDE_GAUNTLET_FIGURE);
    Gauntlet_SetFigureGfx();
}

// Special (and on the way in): what the figure is now - the hooded figure, Bill (the hood off), or
// the Palm Pilot he left (VAR_OBJ_GFX_ID_D; the script then removes and re-adds the object)
void Gauntlet_SetFigureGfx(void)
{
    u32 gfx = OBJ_EVENT_GFX_GAUNTLET_HOODED_FIGURE;
    if (VarGet(VAR_GAUNTLET_STATE) == GAUNTLET_SIMULATOR)
        gfx = OBJ_EVENT_GFX_GAUNTLET_PALM_PILOT;
    else if (FlagGet(FLAG_GAUNTLET_BILL_REVEALED))
        gfx = OBJ_EVENT_GFX_GAUNTLET_BILL;
    VarSet(VAR_OBJ_GFX_ID_D, gfx);
}

// Map script (the gym's ON_LOAD, after the decorations are drawn): the figure stands on open floor
// near the door (the player decorates the gym, so its spot can change)
void Gauntlet_OnLoad(void)
{
    s32 r, dx, dy, cx = 9, cy = 24;
    for (r = 0; r < 12; r++)
    {
        for (dy = -r; dy <= r; dy++)
        {
            for (dx = -r; dx <= r; dx++)
            {
                s32 x = cx + dx, y = cy + dy;
                if ((abs(dx) != r && abs(dy) != r) || (x >= 11 && x <= 13 && y >= 23))
                    continue;           // (not on the way in from the door)
                if (GymLegend_IsOpenFloor(x, y))
                {
                    SetObjEventTemplateCoords(LOCALID_PALLET_GYM_FIGURE, x, y);
                    return;
                }
            }
        }
    }
}

// Special: the next challenger set up (their team, lines, walking sprite in VAR_OBJ_GFX_ID_0).
// STR_VAR_1 = their name, STR_VAR_2 = their region
void Gauntlet_SetupNext(void)
{
    const struct GauntletTrainer *t = &sGauntletTrainers[Index()];
    GymChallenge_SetupGauntlet(t->trainerClass, t->pic, t->female, t->battleName, t->party, PARTY_SIZE,
                               t->intro, t->defeat, t->after, t->win);
    VarSet(VAR_OBJ_GFX_ID_0, t->gfx);
    StringCopy(gStringVar1, t->name);
    StringCopy(gStringVar2, sGauntletRegions[t->region]);
}

// Special (the script has faded to black, then waitstates): the next challenger's title card
static EWRAM_DATA u8 sCardHeader[48] = {0};

void Gauntlet_ShowTitleCard(void)
{
    u32 index = Index();
    const struct GauntletTrainer *t = &sGauntletTrainers[index];
    bool32 sim = VarGet(VAR_GAUNTLET_STATE) == GAUNTLET_SIMULATOR;
    u8 *end;
    if (index == GAUNTLET_COUNT && !sim)
    {
        StringCopy(sCardHeader, COMPOUND_STRING("THE GAUNTLET  ·  THE LAST CHALLENGER"));
    }
    else
    {
        end = StringCopy(sCardHeader, sim ? COMPOUND_STRING("BILL'S SIMULATOR  ·  ") : COMPOUND_STRING("THE GAUNTLET  ·  CHALLENGER "));
        end = ConvertIntToDecimalStringN(end, index + 1, STR_CONV_MODE_LEFT_ALIGN, 3);
        end = StringAppend(end, COMPOUND_STRING(" OF "));
        ConvertIntToDecimalStringN(end, GAUNTLET_COUNT + sim, STR_CONV_MODE_LEFT_ALIGN, 3);
    }
    TitleCard_Show(t->pic, index == GAUNTLET_COUNT ? RGB(20, 5, 18) : sRegionColors[t->region], sCardHeader, t->name, t->title,
                   sGauntletRegions[t->region], t->games);
}

// Special, after a win: on to the next challenger. VAR_RESULT = TRUE if that was the last.
// STR_VAR_1 = how many beaten, STR_VAR_2 = how many left
void Gauntlet_RecordWin(void)
{
    u32 next = Next(), total = GAUNTLET_COUNT;
    if (VarGet(VAR_GAUNTLET_STATE) == GAUNTLET_SIMULATOR)
    {
        // (the simulator: Bill comes last, and beating him earns a WITHDRAW and starts it over)
        total = GAUNTLET_COUNT + 1;
        next = Index() + 1;
        gSpecialVar_Result = next >= total;
        VarSet(VAR_GAUNTLET_NEXT, gSpecialVar_Result ? 0 : next);
        if (gSpecialVar_Result)
            VarSet(VAR_GAUNTLET_WITHDRAWS, VarGet(VAR_GAUNTLET_WITHDRAWS) + 1);
    }
    else
    {
        if (next < GAUNTLET_COUNT)
            VarSet(VAR_GAUNTLET_NEXT, ++next);
        gSpecialVar_Result = next >= GAUNTLET_COUNT;
        if (gSpecialVar_Result)
            VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ALL_BEATEN);
    }
    ConvertIntToDecimalStringN(gStringVar1, next, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, total - next, STR_CONV_MODE_LEFT_ALIGN, 3);
}

// Special: Bill's beaten (the real thing). His Palm Pilot is left: a WITHDRAW, and the simulator
void Gauntlet_BillDefeated(void)
{
    VarSet(VAR_GAUNTLET_STATE, GAUNTLET_SIMULATOR);
    VarSet(VAR_GAUNTLET_NEXT, 0);
    VarSet(VAR_GAUNTLET_WITHDRAWS, VarGet(VAR_GAUNTLET_WITHDRAWS) + 1);
    FlagClear(FLAG_GAUNTLET_BILL_REVEALED);
    Gauntlet_SetFigureGfx();
}

// Special: the reveal: the hood is off (the script re-adds the figure as Bill)
void Gauntlet_RevealBill(void)
{
    FlagSet(FLAG_GAUNTLET_BILL_REVEALED);
    Gauntlet_SetFigureGfx();
}

// For the battle music: Bill (the real thing or the simulation)
bool32 Gauntlet_IsBillBattle(void)
{
    u32 state = VarGet(VAR_GAUNTLET_STATE);
    return FlagGet(FLAG_GYM_CHALLENGE_BATTLE) && (state == GAUNTLET_ALL_BEATEN || state == GAUNTLET_SIMULATOR)
        && Index() == GAUNTLET_COUNT;
}

// ---- WITHDRAW: any Pokémon, shiny (Bill's Palm Pilot) ----
// Special (then waitstate): VAR_RESULT = the species chosen, or SPECIES_NONE (B). A list of every
// Pokémon (src/data/gauntlet_withdraw.h, National Dex order; L/R a page, START the next generation)
// and the shiny picture of the one under the cursor.
#define WD_SHOWN 6
#define WD_PIC_X 19
#define WD_PIC_Y 2

struct Withdraw
{
    struct ListMenuItem items[WITHDRAW_COUNT];
    u16 scroll, row;
    u8 listWindow, picWindow, listTask, spriteId, arrows;
};

static EWRAM_DATA struct Withdraw *sWithdraw = NULL;
static EWRAM_DATA u16 sWithdrawLastScroll = 0;
static EWRAM_DATA u16 sWithdrawLastRow = 0;

static void Withdraw_ShowPic(u32 index)
{
    struct Withdraw *w = sWithdraw;
    if (w->spriteId != MAX_SPRITES)
        FreeResourcesAndDestroySprite(&gSprites[w->spriteId], w->spriteId);
    w->spriteId = CreateMonSprite_FieldMove(sWithdrawSpecies[index], TRUE, 0, WD_PIC_X * 8 + 40, WD_PIC_Y * 8 + 40, 0);
    if (w->spriteId != MAX_SPRITES)
    {
        gSprites[w->spriteId].callback = SpriteCallbackDummy;
        gSprites[w->spriteId].oam.priority = 0;
    }
}

static void Withdraw_MoveCursor(s32 index, bool8 onInit, struct ListMenu *list)
{
    if (!onInit)
        PlaySE(SE_SELECT);
    Withdraw_ShowPic(index);
}

static const struct ListMenuTemplate sWithdrawListTemplate =
{
    .moveCursorFunc = Withdraw_MoveCursor,
    .totalItems = WITHDRAW_COUNT,
    .maxShowed = WD_SHOWN,
    .item_X = 8,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 0,
    .scrollMultiple = LIST_MULTIPLE_SCROLL_L_R,
    .fontId = FONT_NORMAL,
};

static void Withdraw_StartList(u32 scroll, u32 row)
{
    struct Withdraw *w = sWithdraw;
    struct ScrollArrowsTemplate arrows = {
        .firstArrowType = SCROLL_ARROW_UP, .firstX = 8 * 9, .firstY = 6,
        .secondArrowType = SCROLL_ARROW_DOWN, .secondX = 8 * 9, .secondY = 8 * (2 + 2 * WD_SHOWN) + 4,
        .fullyUpThreshold = 0, .fullyDownThreshold = WITHDRAW_COUNT - WD_SHOWN,
        .tileTag = 2000, .palTag = 100, .palNum = 0,
    };
    gMultiuseListMenuTemplate = sWithdrawListTemplate;
    gMultiuseListMenuTemplate.items = w->items;
    gMultiuseListMenuTemplate.windowId = w->listWindow;
    w->scroll = scroll;
    w->listTask = ListMenuInit(&gMultiuseListMenuTemplate, scroll, row);
    w->arrows = AddScrollIndicatorArrowPair(&arrows, &w->scroll);
}

static void Withdraw_StopList(void)
{
    struct Withdraw *w = sWithdraw;
    RemoveScrollIndicatorArrowPair(w->arrows);
    DestroyListMenuTask(w->listTask, &sWithdrawLastScroll, &sWithdrawLastRow);
}

static void Task_Withdraw(u8 taskId)
{
    struct Withdraw *w = sWithdraw;
    s32 input;
    if (JOY_NEW(START_BUTTON))
    {
        // the next generation (from the last, back to the first)
        u32 at = w->scroll + w->row, i, to = 0;
        for (i = 0; i < ARRAY_COUNT(sWithdrawGenStarts); i++)
        {
            if (sWithdrawGenStarts[i] > at)
            {
                to = sWithdrawGenStarts[i];
                break;
            }
        }
        if (to > WITHDRAW_COUNT - WD_SHOWN)
            to = WITHDRAW_COUNT - WD_SHOWN;
        PlaySE(SE_SELECT);
        Withdraw_StopList();
        Withdraw_StartList(to, 0);
        return;
    }
    input = ListMenu_ProcessInput(w->listTask);
    ListMenuGetScrollAndRow(w->listTask, &w->scroll, &w->row);
    if (input == LIST_NOTHING_CHOSEN || input == LIST_HEADER)
        return;
    gSpecialVar_Result = input == LIST_CANCEL ? SPECIES_NONE : sWithdrawSpecies[input];
    PlaySE(SE_SELECT);
    Withdraw_StopList();
    if (w->spriteId != MAX_SPRITES)
        FreeResourcesAndDestroySprite(&gSprites[w->spriteId], w->spriteId);
    ClearStdWindowAndFrame(w->listWindow, TRUE);
    ClearStdWindowAndFrame(w->picWindow, TRUE);
    RemoveWindow(w->listWindow);
    RemoveWindow(w->picWindow);
    FREE_AND_SET_NULL(sWithdraw);
    ScriptContext_Enable();
    DestroyTask(taskId);
}

void Gauntlet_ChooseWithdraw(void)
{
    struct Withdraw *w;
    struct WindowTemplate list = {.bg = 0, .tilemapLeft = 1, .tilemapTop = 1, .width = 15, .height = 2 * WD_SHOWN,
                                  .paletteNum = 15, .baseBlock = 0x008};
    struct WindowTemplate pic = {.bg = 0, .tilemapLeft = WD_PIC_X + 1, .tilemapTop = WD_PIC_Y + 1, .width = 8, .height = 8,
                                 .paletteNum = 15, .baseBlock = 0x008 + 15 * 2 * WD_SHOWN};
    u32 i;
    sWithdraw = w = AllocZeroed(sizeof(*w));
    for (i = 0; i < WITHDRAW_COUNT; i++)
    {
        w->items[i].name = sWithdrawLabels[i] ? sWithdrawLabels[i] : GetSpeciesName(sWithdrawSpecies[i]);
        w->items[i].id = i;
    }
    w->spriteId = MAX_SPRITES;
    LoadMessageBoxAndBorderGfx();
    w->listWindow = AddWindow(&list);
    w->picWindow = AddWindow(&pic);
    SetStandardWindowBorderStyle(w->listWindow, FALSE);
    SetStandardWindowBorderStyle(w->picWindow, FALSE);
    PutWindowTilemap(w->listWindow);
    PutWindowTilemap(w->picWindow);
    FillWindowPixelBuffer(w->picWindow, PIXEL_FILL(1));
    CopyWindowToVram(w->picWindow, COPYWIN_FULL);
    if (sWithdrawLastScroll + sWithdrawLastRow >= WITHDRAW_COUNT)
        sWithdrawLastScroll = sWithdrawLastRow = 0;
    Withdraw_StartList(sWithdrawLastScroll, sWithdrawLastRow);
    CopyWindowToVram(w->listWindow, COPYWIN_FULL);
    CreateTask(Task_Withdraw, 80);
}

// Special: VAR_GAUNTLET_WITHDRAWS - 1 (after a WITHDRAW)
void Gauntlet_UseWithdraw(void)
{
    u32 n = VarGet(VAR_GAUNTLET_WITHDRAWS);
    if (n)
        VarSet(VAR_GAUNTLET_WITHDRAWS, n - 1);
}

// Special: the clipboard's summary. STR_VAR_1 = beaten, STR_VAR_2 = how many in all, STR_VAR_3 = the
// next challenger's region. VAR_RESULT = TRUE if they're all beaten
void Gauntlet_BufferProgress(void)
{
    ConvertIntToDecimalStringN(gStringVar1, Next(), STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, GAUNTLET_COUNT, STR_CONV_MODE_LEFT_ALIGN, 3);
    gSpecialVar_Result = Next() >= GAUNTLET_COUNT;
    if (!gSpecialVar_Result)
        StringCopy(gStringVar3, sGauntletRegions[sGauntletTrainers[Next()].region]);
}

static u32 CountInRegion(u32 region, bool32 beatenOnly)
{
    u32 i, n = 0;
    for (i = 0; i < GAUNTLET_COUNT; i++)
        n += sGauntletTrainers[i].region == region && (!beatenOnly || i < Next());
    return n;
}

// Special: the clipboard's pages, one per region: "KANTO 3/10" (a star once all are beaten)
void Gauntlet_PushRegions(void)
{
    u32 region;
    for (region = 0; region < GAUNTLET_REGION_COUNT; region++)
    {
        struct ListMenuItem item;
        u8 *name = Alloc(24), *end;
        end = StringCopy(name, sGauntletRegions[region]);
        *end++ = CHAR_SPACE;
        end = ConvertIntToDecimalStringN(end, CountInRegion(region, TRUE), STR_CONV_MODE_LEFT_ALIGN, 2);
        *end++ = CHAR_SLASH;
        end = ConvertIntToDecimalStringN(end, CountInRegion(region, FALSE), STR_CONV_MODE_LEFT_ALIGN, 2);
        if (CountInRegion(region, TRUE) == CountInRegion(region, FALSE))
            StringAppend(end, COMPOUND_STRING(" {STAR}"));
        item.name = name;
        item.id = region;
        MultichoiceDynamic_PushElement(item);
    }
}

// Special: VAR_0x8004 = a region. Its page: the names of those beaten, "???" for the rest
void Gauntlet_PushRegionNames(void)
{
    u32 i;
    for (i = 0; i < GAUNTLET_COUNT; i++)
    {
        struct ListMenuItem item;
        if (sGauntletTrainers[i].region != gSpecialVar_0x8004)
            continue;
        item.name = i < Next() ? sGauntletTrainers[i].name : COMPOUND_STRING("???");
        item.id = i;
        MultichoiceDynamic_PushElement(item);
    }
}
