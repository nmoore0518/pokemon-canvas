// Gym Trainer hack: the player's own gym badge, one per type (art: tools/gym_hack/badge_art.py).
//   - Challengers who beat the player are awarded it (with its picture on screen).
//   - The BADGE PEDESTAL decoration shows it in the gym: the gym tileset holds one badge's tiles
//     (GYM_BADGE_TILE_START, palette GYM_BADGE_PALETTE), and whenever the gym's tileset is loaded
//     the player's own badge is copied over them.
#include "global.h"
#include "gym_badge.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "fieldmap.h"
#include "palette.h"
#include "sprite.h"
#include "string_util.h"
#include "constants/gym_decor.h"
#include "constants/vars.h"

extern const struct Tileset gTileset_PalletGym;

struct GymBadgeInfo
{
    const u8 *name;
    const u32 *gfx;     // 32x32, 16 tiles
    const u16 *pal;
};

static const u32 sGfx_Normal[] = INCGFX_U32("graphics/gym_badges/normal.png", ".4bpp");
static const u32 sGfx_Fighting[] = INCGFX_U32("graphics/gym_badges/fighting.png", ".4bpp");
static const u32 sGfx_Flying[] = INCGFX_U32("graphics/gym_badges/flying.png", ".4bpp");
static const u32 sGfx_Poison[] = INCGFX_U32("graphics/gym_badges/poison.png", ".4bpp");
static const u32 sGfx_Ground[] = INCGFX_U32("graphics/gym_badges/ground.png", ".4bpp");
static const u32 sGfx_Rock[] = INCGFX_U32("graphics/gym_badges/rock.png", ".4bpp");
static const u32 sGfx_Bug[] = INCGFX_U32("graphics/gym_badges/bug.png", ".4bpp");
static const u32 sGfx_Ghost[] = INCGFX_U32("graphics/gym_badges/ghost.png", ".4bpp");
static const u32 sGfx_Steel[] = INCGFX_U32("graphics/gym_badges/steel.png", ".4bpp");
static const u32 sGfx_Fire[] = INCGFX_U32("graphics/gym_badges/fire.png", ".4bpp");
static const u32 sGfx_Water[] = INCGFX_U32("graphics/gym_badges/water.png", ".4bpp");
static const u32 sGfx_Grass[] = INCGFX_U32("graphics/gym_badges/grass.png", ".4bpp");
static const u32 sGfx_Electric[] = INCGFX_U32("graphics/gym_badges/electric.png", ".4bpp");
static const u32 sGfx_Psychic[] = INCGFX_U32("graphics/gym_badges/psychic.png", ".4bpp");
static const u32 sGfx_Ice[] = INCGFX_U32("graphics/gym_badges/ice.png", ".4bpp");
static const u32 sGfx_Dragon[] = INCGFX_U32("graphics/gym_badges/dragon.png", ".4bpp");
static const u32 sGfx_Dark[] = INCGFX_U32("graphics/gym_badges/dark.png", ".4bpp");
static const u32 sGfx_Fairy[] = INCGFX_U32("graphics/gym_badges/fairy.png", ".4bpp");

static const u16 sPal_Normal[] = INCGFX_U16("graphics/gym_badges/normal.png", ".gbapal");
static const u16 sPal_Fighting[] = INCGFX_U16("graphics/gym_badges/fighting.png", ".gbapal");
static const u16 sPal_Flying[] = INCGFX_U16("graphics/gym_badges/flying.png", ".gbapal");
static const u16 sPal_Poison[] = INCGFX_U16("graphics/gym_badges/poison.png", ".gbapal");
static const u16 sPal_Ground[] = INCGFX_U16("graphics/gym_badges/ground.png", ".gbapal");
static const u16 sPal_Rock[] = INCGFX_U16("graphics/gym_badges/rock.png", ".gbapal");
static const u16 sPal_Bug[] = INCGFX_U16("graphics/gym_badges/bug.png", ".gbapal");
static const u16 sPal_Ghost[] = INCGFX_U16("graphics/gym_badges/ghost.png", ".gbapal");
static const u16 sPal_Steel[] = INCGFX_U16("graphics/gym_badges/steel.png", ".gbapal");
static const u16 sPal_Fire[] = INCGFX_U16("graphics/gym_badges/fire.png", ".gbapal");
static const u16 sPal_Water[] = INCGFX_U16("graphics/gym_badges/water.png", ".gbapal");
static const u16 sPal_Grass[] = INCGFX_U16("graphics/gym_badges/grass.png", ".gbapal");
static const u16 sPal_Electric[] = INCGFX_U16("graphics/gym_badges/electric.png", ".gbapal");
static const u16 sPal_Psychic[] = INCGFX_U16("graphics/gym_badges/psychic.png", ".gbapal");
static const u16 sPal_Ice[] = INCGFX_U16("graphics/gym_badges/ice.png", ".gbapal");
static const u16 sPal_Dragon[] = INCGFX_U16("graphics/gym_badges/dragon.png", ".gbapal");
static const u16 sPal_Dark[] = INCGFX_U16("graphics/gym_badges/dark.png", ".gbapal");
static const u16 sPal_Fairy[] = INCGFX_U16("graphics/gym_badges/fairy.png", ".gbapal");

static const struct GymBadgeInfo sBadges[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL]   = {COMPOUND_STRING("VANILLA BADGE"),     sGfx_Normal,   sPal_Normal},
    [TYPE_FIGHTING] = {COMPOUND_STRING("KNOCKOUT BADGE"),    sGfx_Fighting, sPal_Fighting},
    [TYPE_FLYING]   = {COMPOUND_STRING("ROCKET BADGE"),      sGfx_Flying,   sPal_Flying},
    [TYPE_POISON]   = {COMPOUND_STRING("VENOM BADGE"),       sGfx_Poison,   sPal_Poison},
    [TYPE_GROUND]   = {COMPOUND_STRING("SHOVEL BADGE"),      sGfx_Ground,   sPal_Ground},
    [TYPE_ROCK]     = {COMPOUND_STRING("OBSIDIAN BADGE"),    sGfx_Rock,     sPal_Rock},
    [TYPE_BUG]      = {COMPOUND_STRING("INFESTATION BADGE"), sGfx_Bug,      sPal_Bug},
    [TYPE_GHOST]    = {COMPOUND_STRING("DEATH BADGE"),       sGfx_Ghost,    sPal_Ghost},
    [TYPE_STEEL]    = {COMPOUND_STRING("WRENCH BADGE"),      sGfx_Steel,    sPal_Steel},
    [TYPE_FIRE]     = {COMPOUND_STRING("LAVA BADGE"),        sGfx_Fire,     sPal_Fire},
    [TYPE_WATER]    = {COMPOUND_STRING("MONSOON BADGE"),     sGfx_Water,    sPal_Water},
    [TYPE_GRASS]    = {COMPOUND_STRING("WEED BADGE"),        sGfx_Grass,    sPal_Grass},
    [TYPE_ELECTRIC] = {COMPOUND_STRING("AMP BADGE"),         sGfx_Electric, sPal_Electric},
    [TYPE_PSYCHIC]  = {COMPOUND_STRING("ESPN BADGE"),        sGfx_Psychic,  sPal_Psychic},
    [TYPE_ICE]      = {COMPOUND_STRING("SUB-ZERO BADGE"),    sGfx_Ice,      sPal_Ice},
    [TYPE_DRAGON]   = {COMPOUND_STRING("EGG BADGE"),         sGfx_Dragon,   sPal_Dragon},
    [TYPE_DARK]     = {COMPOUND_STRING("EVIL BADGE"),        sGfx_Dark,     sPal_Dark},
    [TYPE_FAIRY]    = {COMPOUND_STRING("ELDER BADGE"),       sGfx_Fairy,    sPal_Fairy},
};

static const struct GymBadgeInfo *GetBadge(u32 type)
{
    if (type >= NUMBER_OF_MON_TYPES || sBadges[type].name == NULL)
        type = TYPE_NORMAL;
    return &sBadges[type];
}

static u32 PlayerType(void)
{
    return VarGet(VAR_PLAYER_TYPE);
}

const u8 *GymBadge_GetName(u32 type)
{
    return GetBadge(type)->name;
}

bool32 GymBadge_HasArt(u32 type)
{
    return type < NUMBER_OF_MON_TYPES && sBadges[type].name != NULL;
}

// Special: STR_VAR_1 = the player's badge
void GymBadge_BufferName(void)
{
    StringCopy(gStringVar1, GymBadge_GetName(PlayerType()));
}

// ---------------------------------------------------------------------------
// The badge on the pedestal (called by src/fieldmap.c when tilesets load)
// ---------------------------------------------------------------------------

void GymBadge_PatchTilesetGfx(struct MapLayout const *mapLayout)
{
    if (mapLayout != NULL && mapLayout->secondaryTileset == &gTileset_PalletGym)
        LoadBgTiles(2, GetBadge(PlayerType())->gfx, 16 * TILE_SIZE_4BPP, GYM_BADGE_TILE_START);
}

void GymBadge_PatchTilesetPalette(struct MapLayout const *mapLayout, bool8 skipFaded)
{
    if (mapLayout != NULL && mapLayout->secondaryTileset == &gTileset_PalletGym)
    {
        if (skipFaded)
            CpuCopy16(GetBadge(PlayerType())->pal, &gPlttBufferUnfaded[BG_PLTT_ID(GYM_BADGE_PALETTE)], PLTT_SIZE_4BPP);
        else
            LoadPalette(GetBadge(PlayerType())->pal, BG_PLTT_ID(GYM_BADGE_PALETTE), PLTT_SIZE_4BPP);
    }
}

// ---------------------------------------------------------------------------
// The badge shown on screen when it's awarded
// ---------------------------------------------------------------------------
#define TAG_GYM_BADGE 0x6B42

static const struct OamData sOam_Badge =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 0,
};

static EWRAM_DATA u8 sBadgeSpriteId;

// Special: shows the player's badge above the message box
void GymBadge_ShowIcon(void)
{
    const struct GymBadgeInfo *badge = GetBadge(PlayerType());
    struct SpriteSheet sheet = {badge->gfx, 16 * TILE_SIZE_4BPP, TAG_GYM_BADGE};
    struct SpritePalette pal = {badge->pal, TAG_GYM_BADGE};
    struct SpriteTemplate template =
    {
        .tileTag = TAG_GYM_BADGE,
        .paletteTag = TAG_GYM_BADGE,
        .oam = &sOam_Badge,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    };
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&pal);
    sBadgeSpriteId = CreateSprite(&template, DISPLAY_WIDTH - 32, 32, 0);
}

// Special
void GymBadge_HideIcon(void)
{
    if (sBadgeSpriteId < MAX_SPRITES)
        DestroySprite(&gSprites[sBadgeSpriteId]);
    sBadgeSpriteId = MAX_SPRITES;
    FreeSpriteTilesByTag(TAG_GYM_BADGE);
    FreeSpritePaletteByTag(TAG_GYM_BADGE);
}
