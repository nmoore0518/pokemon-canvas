// Pokémon Canvas: the player's outfits. Each outfit is a set of colors for the player's own sprites
// (tools/gym_hack/outfit_art.py -> src/data/outfits.h), so every sprite the player has (walking,
// running, the bike, surfing, fishing, field moves, the battle back pic) wears it. The player's
// palettes in the overworld (event_object_movement.c) point at gOutfitPlayerPalettes, refreshed
// from the outfit whenever they're loaded; the battle back pic's come from Outfit_GetBackPicPalette.
// The wardrobe in the player's room (data/scripts/outfits.inc) changes it, with a live preview.
#include "global.h"
#include "outfits.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "global.fieldmap.h"
#include "palette.h"
#include "battle_main.h"
#include "pokemon.h"
#include "sprite.h"
#include "string_util.h"
#include "constants/event_objects.h"
#include "constants/gym_hack.h"
#include "constants/outfits.h"
#include "constants/pokemon.h"
#include "constants/vars.h"

#include "data/outfits.h"

EWRAM_DATA u16 gOutfitPlayerPalettes[GENDER_COUNT][16] = {0};
EWRAM_DATA u16 gOutfitPlayerReflectionPalettes[GENDER_COUNT][16] = {0};
static EWRAM_DATA u16 sBackPicPalette[16] = {0};

static const u8 sTypeOutfits[NUMBER_OF_MON_TYPES] =
{
    [TYPE_NORMAL] = OUTFIT_NORMAL, [TYPE_FIGHTING] = OUTFIT_FIGHTING, [TYPE_FLYING] = OUTFIT_FLYING,
    [TYPE_POISON] = OUTFIT_POISON, [TYPE_GROUND] = OUTFIT_GROUND, [TYPE_ROCK] = OUTFIT_ROCK,
    [TYPE_BUG] = OUTFIT_BUG, [TYPE_GHOST] = OUTFIT_GHOST, [TYPE_STEEL] = OUTFIT_STEEL,
    [TYPE_FIRE] = OUTFIT_FIRE, [TYPE_WATER] = OUTFIT_WATER, [TYPE_GRASS] = OUTFIT_GRASS,
    [TYPE_ELECTRIC] = OUTFIT_ELECTRIC, [TYPE_PSYCHIC] = OUTFIT_PSYCHIC, [TYPE_ICE] = OUTFIT_ICE,
    [TYPE_DRAGON] = OUTFIT_DRAGON, [TYPE_DARK] = OUTFIT_DARK, [TYPE_FAIRY] = OUTFIT_FAIRY,
};

static EWRAM_DATA u8 sPreview = 0;   // the outfit shown while choosing in the wardrobe, plus 1 (0: none)

u32 Outfit_Get(void)
{
    u32 outfit = VarGet(VAR_PLAYER_OUTFIT);
    return outfit < OUTFIT_COUNT ? outfit : OUTFIT_CASUAL;
}

void Outfit_UnlockAllGymUniforms(void);

static u32 ShownOutfit(void)
{
    return sPreview > 0 && sPreview <= OUTFIT_COUNT ? sPreview - 1u : Outfit_Get();
}

bool32 Outfit_IsUnlocked(u32 outfit)
{
    if (outfit == OUTFIT_CASUAL)
        return TRUE;
    if (outfit >= OUTFIT_COUNT)
        return FALSE;
    if (outfit <= 16)
        return (VarGet(VAR_OUTFITS_UNLOCKED) >> (outfit - 1)) & 1;
    return (VarGet(VAR_OUTFITS_UNLOCKED_2) >> (outfit - 17)) & 1;
}

void Outfit_Unlock(u32 outfit)
{
    if (outfit == OUTFIT_CASUAL || outfit >= OUTFIT_COUNT)
        return;
    if (outfit <= 16)
        VarSet(VAR_OUTFITS_UNLOCKED, VarGet(VAR_OUTFITS_UNLOCKED) | (1 << (outfit - 1)));
    else
        VarSet(VAR_OUTFITS_UNLOCKED_2, VarGet(VAR_OUTFITS_UNLOCKED_2) | (1 << (outfit - 17)));
}

u32 Outfit_ForType(u32 type)
{
    if (type >= NUMBER_OF_MON_TYPES || sTypeOutfits[type] == 0)
        return OUTFIT_CASUAL;
    return sTypeOutfits[type];
}

void Outfit_RefreshPalettes(void)
{
    u32 outfit = ShownOutfit();
    CpuCopy16(sOutfitPalettes[outfit], gOutfitPlayerPalettes, sizeof(gOutfitPlayerPalettes));
    CpuCopy16(sOutfitReflectionPalettes[outfit], gOutfitPlayerReflectionPalettes, sizeof(gOutfitPlayerReflectionPalettes));
}

const u16 *Outfit_GetBackPicPalette(bool32 female)
{
    CpuCopy16(sOutfitBackPicPalettes[Outfit_Get()][female ? FEMALE : MALE], sBackPicPalette, sizeof(sBackPicPalette));
    return sBackPicPalette;
}

// The walking state's frames (with the outfit's chest mark) for the player's walking sprites
const struct SpriteFrameImage *Outfit_ObjectImages(const struct ObjectEventGraphicsInfo *info)
{
    const struct SpriteFrameImage *table = NULL;
    if (info == GetObjectEventGraphicsInfo(OBJ_EVENT_GFX_RED_NORMAL))
        table = sOutfitPicTables[ShownOutfit()][MALE];
    else if (info == GetObjectEventGraphicsInfo(OBJ_EVENT_GFX_GREEN_NORMAL))
        table = sOutfitPicTables[ShownOutfit()][FEMALE];
    return table != NULL ? table : info->images;
}

// The player's sprite takes the shown outfit's colors (and chest mark) right away
static void ApplyToPlayerSprite(void)
{
    struct Sprite *sprite;
    struct ObjectEvent *player;
    Outfit_RefreshPalettes();
    if (gPlayerAvatar.spriteId >= MAX_SPRITES)
        return;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    player = &gObjectEvents[gPlayerAvatar.objectEventId];
    LoadPalette(gOutfitPlayerPalettes[gPlayerAvatar.gender], OBJ_PLTT_ID(sprite->oam.paletteNum), PLTT_SIZE_4BPP);
    sprite->images = Outfit_ObjectImages(GetObjectEventGraphicsInfo(player->graphicsId));
    StartSpriteAnim(sprite, sprite->animNum);   // (its frame is copied again)
}

// ---- specials ----

// VAR_0x8004 = an outfit -> VAR_RESULT = whether it's unlocked
void Outfit_CheckUnlocked(void)
{
    gSpecialVar_Result = Outfit_IsUnlocked(gSpecialVar_0x8004);
}

// VAR_RESULT = how many outfits the player has (the one they're in counts)
void Outfit_CountUnlocked(void)
{
    u32 i, n = 0;
    for (i = 0; i < OUTFIT_COUNT; i++)
        n += Outfit_IsUnlocked(i);
    gSpecialVar_Result = n;
}

// VAR_0x8004 = an outfit: unlock it
void Outfit_UnlockSpecial(void)
{
    Outfit_Unlock(gSpecialVar_0x8004);
}

// The gym uniform of the player's type: unlocked (VAR_RESULT = whether it's new), STR_VAR_1 = the
// type's name
void Outfit_GiveGymUniform(void)
{
    u32 outfit = Outfit_ForType(VarGet(VAR_PLAYER_TYPE));
    StringCopy(gStringVar1, gTypesInfo[VarGet(VAR_PLAYER_TYPE)].name);   // (in caps: NORMAL...)
    gSpecialVar_Result = (outfit != OUTFIT_CASUAL && !Outfit_IsUnlocked(outfit));
    Outfit_Unlock(outfit);
}

// What a save from before the outfits should already have: the gym uniform once the mentor's first
// gift (after Mt. Moon) has come
void Outfit_CatchUp(void)
{
    u32 t;
    if (VarGet(VAR_MENTOR_STAGE) & MENTOR_GAVE_GUARD)
        Outfit_Unlock(Outfit_ForType(VarGet(VAR_PLAYER_TYPE)));
    // ...and every one, once the gym's type has been changed (a type is owned only after a change)
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        if (FlagGet(FLAG_GYM_TYPE_OWNED + t))
        {
            Outfit_UnlockAllGymUniforms();
            break;
        }
    }
}

// Every type's gym uniform (changing the gym's type, after a mastery). VAR_RESULT = whether any
// were new.
void Outfit_UnlockAllGymUniforms(void)
{
    u32 t, outfit;
    gSpecialVar_Result = FALSE;
    for (t = 0; t < NUMBER_OF_MON_TYPES; t++)
    {
        outfit = Outfit_ForType(t);
        if (outfit == OUTFIT_CASUAL || Outfit_IsUnlocked(outfit))
            continue;
        Outfit_Unlock(outfit);
        gSpecialVar_Result = TRUE;
    }
}

// VAR_0x8004 = the outfit to wear (the player changes into it at once)
void Outfit_Wear(void)
{
    sPreview = 0;
    if (gSpecialVar_0x8004 < OUTFIT_COUNT && Outfit_IsUnlocked(gSpecialVar_0x8004))
        VarSet(VAR_PLAYER_OUTFIT, gSpecialVar_0x8004);
    StringCopy(gStringVar1, sOutfitNames[Outfit_Get()]);
    ApplyToPlayerSprite();
}

// ---- the wardrobe's list: the player shows each outfit as it's picked ----
void Outfit_ListOnInit(struct DynamicListMenuEventArgs *eventArgs)
{
    sPreview = 0;   // (the list opens on what the player has on)
}

void Outfit_ListOnSelectionChanged(struct DynamicListMenuEventArgs *eventArgs)
{
    sPreview = eventArgs->selectedItem + 1;
    ApplyToPlayerSprite();
}

void Outfit_ListOnDestroy(struct DynamicListMenuEventArgs *eventArgs)
{
    sPreview = 0;
    ApplyToPlayerSprite();
}
