// Pokémon Canvas: the Universal Pokémon Medical Center, on Vermilion City's building site.
//
// The foreman (data/maps/VermilionCity_Frlg) needs help: a MACHOP, then a MACHOKE, then a MACHAMP, then
// a shiny MACHAMP (any: one withdrawn from Bill's Palm Pilot too). Each one goes to work for him for good
// (VAR_MEDCENTER_HELPERS), and each turns something up in the dig: the Kanto fossil the player didn't
// take at Mt. Moon, then a Root or Claw Fossil, then a Skull or Armor Fossil. With the fourth the
// building's done (FLAG_MEDCENTER_OPEN): three floors (data/maps/VermilionCity_MedicalCenter_*,
// tools/gym_hack/build_medical_center.py), each with a puzzle:
//   1F  triage: six patients, each with a symptom; the right medicine for each (script only)
//   2F  runaway gurneys to push out of the way (OBJ_EVENT_GFX_GURNEY: pushed like boulders, no
//       Strength needed: src/field_player_avatar.c); the HEAL CHARM past them
//   3F  the microscopes: the head doctor describes Pokérus; the slides are pictures (below); then the
//       head nurse's physical (a battle); then Pokérus for a Pokémon, once a "day"
//
// There's no clock: a "day" is MEDCENTER_DAY_STEPS steps (VAR_STEP_DAY). Each one, Pokérus wears off
// a day (as the real games do by the clock, src/clock.c no longer does) and the head nurse can give
// it again (FLAG_POKERUS_GIVEN_TODAY).
#include "global.h"
#include "pit_boss.h"
#include "medical_center.h"
#include "decompress.h"
#include "event_data.h"
#include "malloc.h"
#include "party_menu.h"
#include "pokemon_storage_system.h"
#include "menu.h"
#include "pokemon.h"
#include "pokerus.h"
#include "random.h"
#include "script_menu.h"
#include "sprite.h"
#include "string_util.h"
#include "window.h"
#include "item.h"
#include "list_menu.h"
#include "constants/items.h"
#include "constants/species.h"

// ---- A day of steps ----
void MedicalCenter_OnStep(void)
{
    u32 steps = VarGet(VAR_STEP_DAY) + 1;
    if (steps >= MEDCENTER_DAY_STEPS)
    {
        steps = 0;
        UpdatePartyPokerusTime(1);
        FlagClear(FLAG_POKERUS_GIVEN_TODAY);
        PitBoss_NewDay();
    }
    VarSet(VAR_STEP_DAY, steps);
    PitBoss_OnStep();
}

// ---- The foreman's helpers ----
static const u16 sHelpers[MEDCENTER_NUM_HELPERS] = {SPECIES_MACHOP, SPECIES_MACHOKE, SPECIES_MACHAMP, SPECIES_MACHAMP};

// Special: STR_VAR_1 = the helper he needs next; VAR_RESULT = how many he's had (MEDCENTER_NUM_HELPERS: done)
void MedCenter_BufferHelper(void)
{
    u32 n = VarGet(VAR_MEDCENTER_HELPERS);
    gSpecialVar_Result = n;
    if (n < MEDCENTER_NUM_HELPERS)
        StringCopy(gStringVar1, GetSpeciesName(sHelpers[n]));
}

// Whether a Pokémon will do as the next helper (HELPER_*)
u32 MedCenter_HelperCheck(struct Pokemon *mon, u32 helpersSoFar, u32 partyCount)
{
    if (helpersSoFar >= MEDCENTER_NUM_HELPERS)
        return HELPER_WRONG_SPECIES;
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return HELPER_EGG;
    if (GetMonData(mon, MON_DATA_SPECIES) != sHelpers[helpersSoFar])
        return HELPER_WRONG_SPECIES;
    if (helpersSoFar == MEDCENTER_NUM_HELPERS - 1)
    {
        if (!GetMonData(mon, MON_DATA_IS_SHINY))
            return HELPER_NOT_SHINY;                // (any shiny one: Bill's Palm Pilot's too)
    }
    if (partyCount <= 1)
        return HELPER_LAST_MON;
    return HELPER_OK;
}

// Special (after ChoosePartyMon: VAR_0x8004): VAR_RESULT = HELPER_*; STR_VAR_2 = its name
void MedCenter_CheckHelper(void)
{
    struct Pokemon *mon;
    gSpecialVar_Result = HELPER_WRONG_SPECIES;
    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        return;
    mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    GetMonNickname(mon, gStringVar2);
    gSpecialVar_Result = MedCenter_HelperCheck(mon, VarGet(VAR_MEDCENTER_HELPERS), CalculatePlayerPartyCount());
}

// Special: the helper (VAR_0x8004) goes to work for him. VAR_RESULT = how many he's had now
void MedCenter_TakeHelper(void)
{
    u32 n = VarGet(VAR_MEDCENTER_HELPERS) + 1;
    ZeroMonData(&gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004]);
    CompactPartySlots();
    CalculatePlayerPartyCount();
    VarSet(VAR_MEDCENTER_HELPERS, n);
    if (n >= MEDCENTER_NUM_HELPERS)
        FlagSet(FLAG_MEDCENTER_OPEN);
    gSpecialVar_Result = n;
}

// ---- Pokérus ----
// What giving Pokérus to a Pokémon does (POKERUS_GIVE_*); it's given if POKERUS_GIVE_DONE
u32 MedCenter_TryGivePokerus(struct Pokemon *mon)
{
    u8 strain, days;
    if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG))
        return POKERUS_GIVE_EGG;
    if (CheckMonPokerus(mon))
        return POKERUS_GIVE_HAS_IT;
    if (CheckMonHasHadPokerus(mon))
        return POKERUS_GIVE_HAD_IT;
    strain = 1 + Random() % 15;                 // (0 is a special case in src/pokerus.c)
    days = GetDaysLeftBasedOnStrain(strain);
    SetMonData(mon, MON_DATA_POKERUS_STRAIN, &strain);
    SetMonData(mon, MON_DATA_POKERUS_DAYS_LEFT, &days);
    return POKERUS_GIVE_DONE;
}

// Special (after ChoosePartyMon: VAR_0x8004): VAR_RESULT = POKERUS_GIVE_*; STR_VAR_1 = its name
void MedCenter_GivePokerus(void)
{
    struct Pokemon *mon;
    gSpecialVar_Result = POKERUS_GIVE_EGG;
    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        return;
    mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    GetMonNickname(mon, gStringVar1);
    gSpecialVar_Result = MedCenter_TryGivePokerus(mon);
    if (gSpecialVar_Result == POKERUS_GIVE_DONE)
        FlagSet(FLAG_POKERUS_GIVEN_TODAY);
}

// ---- The microscopes' slides (3F): a picture in a framed box, like showmonpic's ----
#define SLIDE_TAG      0x7A10
#define SLIDE_SIZE     64
#define SLIDE_WIN_X    10       // (the box: like showmonpic's at 10, 3)
#define SLIDE_WIN_Y    3

static const u32 sSlidesGfx[] = INCGFX_U32("graphics/medical_center/slides.png", ".4bpp");   // (tools/gym_hack/medical_center_art.py)
static const u16 sSlidesPal[] = INCGFX_U16("graphics/medical_center/slides.png", ".gbapal");

static const struct OamData sSlideOam =
{
    .shape = SPRITE_SHAPE(64x64),
    .size = SPRITE_SIZE(64x64),
    .priority = 0,
};

static const struct SpriteTemplate sSlideTemplate =
{
    .tileTag = SLIDE_TAG,
    .paletteTag = SLIDE_TAG,
    .oam = &sSlideOam,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static EWRAM_DATA bool8 sSlideShown = FALSE;
static EWRAM_DATA u8 sSlideSprite = 0;
static EWRAM_DATA u8 sSlideWindow = 0;

// Special: slide VAR_0x8004 (0 - MEDCENTER_NUM_SLIDES - 1) in its box
void MedCenter_ShowSlide(void)
{
    struct SpriteSheet sheet = {sSlidesGfx + gSpecialVar_0x8004 * (SLIDE_SIZE * SLIDE_SIZE / 8), SLIDE_SIZE * SLIDE_SIZE / 2, SLIDE_TAG};
    struct SpritePalette pal = {sSlidesPal, SLIDE_TAG};
    if (gSpecialVar_0x8004 >= MEDCENTER_NUM_SLIDES || sSlideShown)
        return;
    sSlideShown = TRUE;
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&pal);
    sSlideSprite = CreateSprite(&sSlideTemplate, SLIDE_WIN_X * 8 + 40, SLIDE_WIN_Y * 8 + 40, 0);
    sSlideWindow = CreateWindowFromRect(SLIDE_WIN_X, SLIDE_WIN_Y, 8, 8);
    SetStandardWindowBorderStyle(sSlideWindow, TRUE);
    ScheduleBgCopyTilemapToVram(0);
}

// Special: the slide's box closed
void MedCenter_HideSlide(void)
{
    if (!sSlideShown)
        return;
    sSlideShown = FALSE;
    if (sSlideSprite != MAX_SPRITES)
        DestroySprite(&gSprites[sSlideSprite]);
    FreeSpriteTilesByTag(SLIDE_TAG);
    FreeSpritePaletteByTag(SLIDE_TAG);
    ClearToTransparentAndRemoveWindow(sSlideWindow);
}

// ---- The Cinnabar Lab's fossils (data/maps/CinnabarIsland_PokemonLab_ExperimentRoom_Frlg) ----
// FireRed's doctor knows the Helix and Dome Fossils and the Old Amber, one of each, by flags; the
// foreman's dig turns up more, so he goes by the fossils in the bag: any of these, any number of times.
// VAR_MAP_SCENE_..._WHICH_FOSSIL keeps the one he's reviving (FireRed's 1-3, or now its item).
static const u16 sFossils[][2] =
{
    {ITEM_HELIX_FOSSIL, SPECIES_OMANYTE},
    {ITEM_DOME_FOSSIL, SPECIES_KABUTO},
    {ITEM_OLD_AMBER, SPECIES_AERODACTYL},
    {ITEM_ROOT_FOSSIL, SPECIES_LILEEP},
    {ITEM_CLAW_FOSSIL, SPECIES_ANORITH},
    {ITEM_SKULL_FOSSIL, SPECIES_CRANIDOS},
    {ITEM_ARMOR_FOSSIL, SPECIES_SHIELDON},
};

static EWRAM_DATA u16 sFossilChoices[ARRAY_COUNT(sFossils)] = {0};

// Special: the fossils in the bag, as the next menu (VAR_RESULT: how many)
void Fossil_PushList(void)
{
    u32 i, n = 0;
    for (i = 0; i < ARRAY_COUNT(sFossils); i++)
    {
        if (CheckBagHasItem(sFossils[i][0], 1))
        {
            struct ListMenuItem entry;
            u8 *name = Alloc(ITEM_NAME_LENGTH + 1);   // (the menu frees it)
            StringCopy(name, GetItemName(sFossils[i][0]));
            entry.name = name;
            entry.id = n;
            sFossilChoices[n++] = i;
            MultichoiceDynamic_PushElement(entry);
        }
    }
    gSpecialVar_Result = n;
}

// Special: VAR_RESULT = the menu's choice -> VAR_0x8005 its item; STR_VAR_1 the Pokémon, STR_VAR_2 the fossil
void Fossil_Pick(void)
{
    u32 i = gSpecialVar_Result < ARRAY_COUNT(sFossils) ? sFossilChoices[gSpecialVar_Result] : 0;
    gSpecialVar_0x8005 = sFossils[i][0];
    StringCopy(gStringVar1, GetSpeciesName(sFossils[i][1]));
    StringCopy(gStringVar2, GetItemName(sFossils[i][0]));
}

// The Pokémon in a fossil (FireRed's 1-3 too); SPECIES_NONE if it's none
u32 Fossil_Species(u32 which)
{
    u32 i;
    switch (which)
    {
    case 1: return SPECIES_OMANYTE;
    case 2: return SPECIES_KABUTO;
    case 3: return SPECIES_AERODACTYL;
    }
    for (i = 0; i < ARRAY_COUNT(sFossils); i++)
        if (sFossils[i][0] == which)
            return sFossils[i][1];
    return SPECIES_NONE;
}

// Special: VAR_0x8004 = the Pokémon he's revived (from VAR_0x8005: WHICH_FOSSIL); STR_VAR_1 its name
void Fossil_BufferRevived(void)
{
    gSpecialVar_0x8004 = Fossil_Species(gSpecialVar_0x8005);
    StringCopy(gStringVar1, GetSpeciesName(gSpecialVar_0x8004));
}
