// Pokémon Canvas: the Gauntlet (scripts: data/scripts/gauntlet.inc; challengers: src/data/gauntlet.h,
// made by tools/gym_hack/build_gauntlet.py from tools/gym_hack/gauntlet_roster.py).
//
// Once the player has cleared every hall of the Gym Leader Convention and mastered every gym type,
// a hooded figure turns up at the gym: an assassin, they say, hired by the angry Gym Leaders, who has
// called in the strongest trainers of the Pokémon world. The challengers come one at a time, in
// order (VAR_GAUNTLET_NEXT), each with a level-100 team; any Pokémon may battle (no type rules). A
// loss changes nothing: the same challenger comes next time. The figure's clipboard shows who's been
// beaten, region by region.
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
#include "malloc.h"
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
    if (state == GAUNTLET_ACTIVE || state == GAUNTLET_ALL_BEATEN)
        FlagClear(FLAG_HIDE_GAUNTLET_FIGURE);
    else
        FlagSet(FLAG_HIDE_GAUNTLET_FIGURE);
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
    const struct GauntletTrainer *t = &sGauntletTrainers[Next() < GAUNTLET_COUNT ? Next() : GAUNTLET_COUNT - 1];
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
    const struct GauntletTrainer *t = &sGauntletTrainers[Next() < GAUNTLET_COUNT ? Next() : GAUNTLET_COUNT - 1];
    u8 *end = StringCopy(sCardHeader, COMPOUND_STRING("THE GAUNTLET  ·  CHALLENGER "));
    end = ConvertIntToDecimalStringN(end, Next() + 1, STR_CONV_MODE_LEFT_ALIGN, 3);
    end = StringAppend(end, COMPOUND_STRING(" OF "));
    ConvertIntToDecimalStringN(end, GAUNTLET_COUNT, STR_CONV_MODE_LEFT_ALIGN, 3);
    TitleCard_Show(t->pic, sRegionColors[t->region], sCardHeader, t->name, t->title,
                   sGauntletRegions[t->region], t->games);
}

// Special, after a win: on to the next challenger. VAR_RESULT = TRUE if that was the last.
// STR_VAR_1 = how many beaten, STR_VAR_2 = how many left
void Gauntlet_RecordWin(void)
{
    u32 next = Next();
    if (next < GAUNTLET_COUNT)
        VarSet(VAR_GAUNTLET_NEXT, ++next);
    gSpecialVar_Result = next >= GAUNTLET_COUNT;
    if (gSpecialVar_Result)
        VarSet(VAR_GAUNTLET_STATE, GAUNTLET_ALL_BEATEN);
    ConvertIntToDecimalStringN(gStringVar1, next, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, GAUNTLET_COUNT - next, STR_CONV_MODE_LEFT_ALIGN, 3);
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
