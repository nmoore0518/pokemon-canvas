// Pokémon Canvas: Dad's basement machines (see include/basement_machines.h, GYM_HACK.md)
#include "global.h"
#include "basement_machines.h"
#include "battle_main.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "item.h"
#include "item_menu.h"
#include "mail.h"
#include "main.h"
#include "overworld.h"
#include "party_menu.h"
#include "constants/characters.h"
#include "pokeball.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "trig.h"
#include "strings.h"
#include "synthetic_abilities.h"
#include "constants/abilities.h"
#include "constants/items.h"
#include "constants/party_menu.h"
#include "constants/pokeball.h"

static const enum Type sTypeBallTypes[] =
{
    [BALL_TYPE_NORMAL - FIRST_TYPE_BALL]   = TYPE_NORMAL,
    [BALL_TYPE_FIGHTING - FIRST_TYPE_BALL] = TYPE_FIGHTING,
    [BALL_TYPE_FLYING - FIRST_TYPE_BALL]   = TYPE_FLYING,
    [BALL_TYPE_POISON - FIRST_TYPE_BALL]   = TYPE_POISON,
    [BALL_TYPE_GROUND - FIRST_TYPE_BALL]   = TYPE_GROUND,
    [BALL_TYPE_ROCK - FIRST_TYPE_BALL]     = TYPE_ROCK,
    [BALL_TYPE_BUG - FIRST_TYPE_BALL]      = TYPE_BUG,
    [BALL_TYPE_GHOST - FIRST_TYPE_BALL]    = TYPE_GHOST,
    [BALL_TYPE_STEEL - FIRST_TYPE_BALL]    = TYPE_STEEL,
    [BALL_TYPE_FIRE - FIRST_TYPE_BALL]     = TYPE_FIRE,
    [BALL_TYPE_WATER - FIRST_TYPE_BALL]    = TYPE_WATER,
    [BALL_TYPE_GRASS - FIRST_TYPE_BALL]    = TYPE_GRASS,
    [BALL_TYPE_ELECTRIC - FIRST_TYPE_BALL] = TYPE_ELECTRIC,
    [BALL_TYPE_PSYCHIC - FIRST_TYPE_BALL]  = TYPE_PSYCHIC,
    [BALL_TYPE_ICE - FIRST_TYPE_BALL]      = TYPE_ICE,
    [BALL_TYPE_DRAGON - FIRST_TYPE_BALL]   = TYPE_DRAGON,
    [BALL_TYPE_DARK - FIRST_TYPE_BALL]     = TYPE_DARK,
    [BALL_TYPE_FAIRY - FIRST_TYPE_BALL]    = TYPE_FAIRY,
};

STATIC_ASSERT(ARRAY_COUNT(sTypeBallTypes) == LAST_TYPE_BALL - FIRST_TYPE_BALL + 1, TypeBallTypes);
STATIC_ASSERT(LAST_TYPE_BALL < POKEBALL_COUNT && POKEBALL_COUNT <= 63, TypeBallsFitInPokeballBits);

enum Type GetTypeBallType(enum PokeBall ball)
{
    if (!IS_TYPE_BALL(ball))
        return TYPE_NONE;
    return sTypeBallTypes[ball - FIRST_TYPE_BALL];
}

enum Item GetTypeBallItem(enum Type type)
{
    for (u32 i = 0; i < ARRAY_COUNT(sTypeBallTypes); i++)
    {
        if (sTypeBallTypes[i] == type)
            return gPokeBalls[FIRST_TYPE_BALL + i].itemId;
    }
    return ITEM_NONE;
}

u32 GetTypeBallStatBoosts(enum Species species, enum PokeBall ball)
{
    enum Type type = GetTypeBallType(ball);
    u32 i, lowest = 0xFFFF, mask = 0;

    if (type == TYPE_NONE || (GetSpeciesType(species, 0) != type && GetSpeciesType(species, 1) != type))
        return 0;
    for (i = 0; i < NUM_STATS; i++)
    {
        u32 base = (i == STAT_HP) ? GetSpeciesBaseHP(species) : GetSpeciesBaseStat(species, i);
        if (i == STAT_HP && HasShedinjaHPHandling(species))
            continue; // its 1 HP never changes
        if (base < lowest)
        {
            lowest = base;
            mask = 0;
        }
        if (base == lowest)
            mask |= 1u << i;
    }
    return mask;
}

bool32 BasementMachines_NiceMode(void)
{
    return FlagGet(FLAG_BASEMENT_SWITCH_ON);
}

static bool32 IsApplinLine(enum Species species)
{
    switch (species)
    {
    case SPECIES_APPLIN:
    case SPECIES_FLAPPLE:
    case SPECIES_APPLETUN:
    case SPECIES_DIPPLIN:
    case SPECIES_HYDRAPPLE:
        return TRUE;
    default:
        return FALSE;
    }
}

bool32 TryGiveWildMonLabItem(struct Pokemon *mon)
{
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    enum Item item = ITEM_NONE;
    bool32 poison = GetSpeciesType(species, 0) == TYPE_POISON || GetSpeciesType(species, 1) == TYPE_POISON;
    bool32 grass = GetSpeciesType(species, 0) == TYPE_GRASS || GetSpeciesType(species, 1) == TYPE_GRASS;

    if (poison && Random() % 100 < LAB_HELD_ITEM_PERCENT)
        item = ITEM_STRONG_ACID;
    else if (grass && Random() % 100 < (IsApplinLine(species) ? APPLIN_JUICE_PERCENT : LAB_HELD_ITEM_PERCENT))
        item = ITEM_APPLE_JUICE;
    if (item == ITEM_NONE)
        return FALSE;
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    return TRUE;
}

// ---------------------------------------------------------------------------
// The scripts' specials. VAR_0x8004 is the party slot (ChoosePartyMon); answers in VAR_RESULT.
// ---------------------------------------------------------------------------
static struct Pokemon *ChosenMon(void)
{
    return &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
}

static void UpperCase(u8 *str)
{
    for (; *str != EOS; str++)
    {
        if (*str >= CHAR_a && *str <= CHAR_z)
            *str = *str - CHAR_a + CHAR_A;
    }
}

// STR_VAR_1: its nickname, STR_VAR_2: the ability it would get
void Synthesizer_CheckMon(void)
{
    struct Pokemon *mon = ChosenMon();
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);

    GetMonNickname(mon, gStringVar1);
    if (GetMonData(mon, MON_DATA_IS_EGG))
        gSpecialVar_Result = MACHINE_EGG;
    else if (GetSpeciesSyntheticAbility(species) == ABILITY_NONE)
        gSpecialVar_Result = MACHINE_SYNTH_BLOCKED;
    else if (GetMonData(mon, MON_DATA_ABILITY_NUM) == ABILITY_NUM_SYNTHETIC)
        gSpecialVar_Result = MACHINE_SYNTH_ALREADY;
    else
        gSpecialVar_Result = MACHINE_OK;
    if (gSpecialVar_Result != MACHINE_EGG)
        StringCopy(gStringVar2, gAbilitiesInfo[GetSpeciesListedSyntheticAbility(species)].name);
}

void Synthesizer_Apply(void)
{
    struct Pokemon *mon = ChosenMon();
    u32 slot = ABILITY_NUM_SYNTHETIC;
    u32 friendship = 0;

    SetMonData(mon, MON_DATA_ABILITY_NUM, &slot);
    if (BasementMachines_NiceMode())
    {
        friendship = GetMonData(mon, MON_DATA_FRIENDSHIP) + SYNTH_NICE_FRIENDSHIP;
        if (friendship > MAX_FRIENDSHIP)
            friendship = MAX_FRIENDSHIP;
    }
    SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
}

// STR_VAR_1: its nickname, STR_VAR_2: its primary type in capitals; VAR_0x8005: the Type Ball it makes
void BallMaker_CheckMon(void)
{
    struct Pokemon *mon = ChosenMon();
    enum Type type = GetSpeciesType(GetMonData(mon, MON_DATA_SPECIES), 0);

    GetMonNickname(mon, gStringVar1);
    StringCopy(gStringVar2, gTypesInfo[type].name);
    UpperCase(gStringVar2);
    gSpecialVar_0x8005 = GetTypeBallItem(type);
    if (GetMonData(mon, MON_DATA_IS_EGG))
        gSpecialVar_Result = MACHINE_EGG;
    else if (GetMonData(mon, MON_DATA_LEVEL) < BALL_MAKER_MIN_LEVEL)
        gSpecialVar_Result = MACHINE_MAKER_LOW_LEVEL;
    else if (!BasementMachines_NiceMode() && CountPartyNonEggMons() <= 1)
        gSpecialVar_Result = MACHINE_MAKER_LAST_MON;
    else if (!CheckBagHasSpace(gSpecialVar_0x8005, 1))
        gSpecialVar_Result = MACHINE_MAKER_BAG_FULL;
    else
        gSpecialVar_Result = MACHINE_OK;
}

// Evil Mode: the Pokémon is gone (its held item goes to the bag, if there's room).
// Nice Mode: it loses BALL_MAKER_NICE_LEVELS levels (STR_VAR_3: the new level).
void BallMaker_Apply(void)
{
    struct Pokemon *mon = ChosenMon();

    if (BasementMachines_NiceMode())
    {
        enum Species species = GetMonData(mon, MON_DATA_SPECIES);
        u32 level = GetMonData(mon, MON_DATA_LEVEL) - BALL_MAKER_NICE_LEVELS;
        u32 exp = gExperienceTables[gSpeciesInfo[species].growthRate][level];
        SetMonData(mon, MON_DATA_EXP, &exp);
        CalculateMonStats(mon);
        ConvertIntToDecimalStringN(gStringVar3, level, STR_CONV_MODE_LEFT_ALIGN, 3);
    }
    else
    {
        enum Item held = GetMonData(mon, MON_DATA_HELD_ITEM);
        if (held != ITEM_NONE && !ItemIsMail(held))
            AddBagItem(held, 1);
        ZeroMonData(mon);
        CompactPartySlots();
        CalculatePlayerPartyCount();
    }
}

void Bag_ChooseBall(void)
{
    SetMainCallback2(CB2_ChooseBallForChanger);
}

// STR_VAR_1: its nickname, STR_VAR_2: the ball it's in now
void BallChanger_CheckMon(void)
{
    struct Pokemon *mon = ChosenMon();

    GetMonNickname(mon, gStringVar1);
    CopyItemName(gPokeBalls[GetMonData(mon, MON_DATA_POKEBALL)].itemId, gStringVar2);
    gSpecialVar_Result = GetMonData(mon, MON_DATA_IS_EGG) ? MACHINE_EGG : MACHINE_OK;
}

// The ball picked from the bag (VAR_ITEM_ID); STR_VAR_3: its name
void BallChanger_CheckBall(void)
{
    struct Pokemon *mon = ChosenMon();
    enum PokeBall oldBall = GetMonData(mon, MON_DATA_POKEBALL);
    enum Item oldItem = gPokeBalls[oldBall].itemId;

    // (the bag used the string buffers: fill them again)
    GetMonNickname(mon, gStringVar1);
    CopyItemName(oldItem, gStringVar2);
    CopyItemName(gSpecialVar_ItemId, gStringVar3);
    if (ItemIdToBallId(gSpecialVar_ItemId) == oldBall)
        gSpecialVar_Result = MACHINE_CHANGER_SAME_BALL;
    else if (oldBall != BALL_STRANGE && oldItem != ITEM_NONE && !CheckBagHasSpace(oldItem, 1)
          && CountTotalItemQuantityInBag(gSpecialVar_ItemId) > 1)
        gSpecialVar_Result = MACHINE_CHANGER_BAG_FULL;
    else
        gSpecialVar_Result = MACHINE_OK;
}

// Takes the chosen ball from the bag, puts the Pokémon in it and gives back the old one
void BallChanger_Apply(void)
{
    struct Pokemon *mon = ChosenMon();
    enum PokeBall oldBall = GetMonData(mon, MON_DATA_POKEBALL);
    enum PokeBall newBall = ItemIdToBallId(gSpecialVar_ItemId);

    RemoveBagItem(gSpecialVar_ItemId, 1);
    SetMonData(mon, MON_DATA_POKEBALL, &newBall);
    CalculateMonStats(mon); // a Type Ball's boost comes or goes
    if (oldBall != BALL_STRANGE && gPokeBalls[oldBall].itemId != ITEM_NONE)
        AddBagItem(gPokeBalls[oldBall].itemId, 1);
}

// ---- The Ball Changer's pedestals: empty until the machine runs; then the Pokémon's ball (left) and
//      the new one (right) appear on them and jump across (sprites, the real balls' pictures) ----
#define CHANGER_LEFT_X     5           // the pedestals' dishes (map blocks; build_basement.py: art.CHANGER)
#define CHANGER_RIGHT_X    8
#define CHANGER_Y          6
#define CHANGER_SWAP_FRAMES 36

static EWRAM_DATA u8 sChangerSprites[2] = {0};
static EWRAM_DATA u8 sChangerBalls[2] = {0};

static const struct OamData sChangerBallOam =
{
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 2,
};

static u8 CreateChangerBall(enum PokeBall ball, s16 blockX)
{
    struct SpriteTemplate template = gPokeBalls[ball].spriteTemplate;
    s16 x, y;
    u8 spriteId;

    template.oam = &sChangerBallOam;
    template.affineAnims = gDummySpriteAffineAnimTable;
    template.callback = SpriteCallbackDummy;
    LoadBallGfx(ball);
    SetSpritePosToMapCoords(blockX + MAP_OFFSET, CHANGER_Y + MAP_OFFSET, &x, &y);
    spriteId = CreateSprite(&template, x + 8, y - 26, 0);  // (sitting in the dish; measured in the game)
    if (spriteId != MAX_SPRITES)
        StartSpriteAnim(&gSprites[spriteId], 0);           // (closed)
    return spriteId;
}

// The Pokémon's ball on the left pedestal, the one picked from the bag (VAR_ITEM_ID) on the right
void BallChanger_ShowBalls(void)
{
    sChangerBalls[0] = GetMonData(ChosenMon(), MON_DATA_POKEBALL);
    sChangerBalls[1] = ItemIdToBallId(gSpecialVar_ItemId);
    sChangerSprites[0] = CreateChangerBall(sChangerBalls[0], CHANGER_LEFT_X);
    sChangerSprites[1] = CreateChangerBall(sChangerBalls[1], CHANGER_RIGHT_X);
}

#define tTimer data[0]

static void Task_ChangerSwap(u8 taskId)
{
    s32 t = ++gTasks[taskId].tTimer;
    s32 dx = (CHANGER_RIGHT_X - CHANGER_LEFT_X) * 16 * t / CHANGER_SWAP_FRAMES;
    s32 arc = Sin(t * 128 / CHANGER_SWAP_FRAMES, 24);

    if (sChangerSprites[0] != MAX_SPRITES)
    {
        gSprites[sChangerSprites[0]].x2 = dx;
        gSprites[sChangerSprites[0]].y2 = -arc;          // (over the top)
    }
    if (sChangerSprites[1] != MAX_SPRITES)
    {
        gSprites[sChangerSprites[1]].x2 = -dx;
        gSprites[sChangerSprites[1]].y2 = -arc / 3;      // (under it)
    }
    if (t >= CHANGER_SWAP_FRAMES)
    {
        DestroyTask(taskId);
        ScriptContext_Enable();
    }
}

// (use waitstate after it)
void BallChanger_SwapBalls(void)
{
    CreateTask(Task_ChangerSwap, 80);
}

#undef tTimer

void BallChanger_HideBalls(void)
{
    u32 i;

    for (i = 0; i < 2; i++)
    {
        if (sChangerSprites[i] != MAX_SPRITES)
            DestroySprite(&gSprites[sChangerSprites[i]]);
        sChangerSprites[i] = MAX_SPRITES;
    }
    FreeBallGfx(sChangerBalls[0]);
    FreeBallGfx(sChangerBalls[1]);
}
