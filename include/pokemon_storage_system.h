#ifndef GUARD_POKEMON_STORAGE_SYSTEM_H
#define GUARD_POKEMON_STORAGE_SYSTEM_H

#define TOTAL_BOXES_COUNT       16  // Gym Trainer hack: 14 + 2 (the 16th box's end is in flash sector 31)
#define VANILLA_BOXES_COUNT     14  // (an older save's layout: see PokemonStorage_UpgradeLayout)
#define IN_BOX_ROWS             5 // Number of rows, 6 Pokémon per row
#define IN_BOX_COLUMNS          6 // Number of columns, 5 Pokémon per column
#define IN_BOX_COUNT            (IN_BOX_ROWS * IN_BOX_COLUMNS)
#define BOX_NAME_LENGTH         8
#define MAX_FUSION_STORAGE      4

/*
            COLUMNS
ROWS        0   1   2   3   4   5
            6   7   8   9   10  11
            12  13  14  15  16  17
            18  19  20  21  22  23
            24  25  26  27  28  29
*/

// Gym Trainer hack: the boxes come last now (so there can be more of them), and `layout` says so;
// a save from before has 0 there (the padding before its boxes) and is rearranged as it loads.
#define STORAGE_LAYOUT_BOXES_LAST 2

struct PokemonStorage
{
    u8 currentBox;
    u8 layout;      // STORAGE_LAYOUT_BOXES_LAST
    u8 boxNames[TOTAL_BOXES_COUNT][BOX_NAME_LENGTH + 1];
    u8 boxWallpapers[TOTAL_BOXES_COUNT];
    struct Pokemon fusions[MAX_FUSION_STORAGE];
    struct BoxPokemon boxes[TOTAL_BOXES_COUNT][IN_BOX_COUNT];
};

// The layout before (14 boxes, the boxes first)
struct PokemonStorageV1
{
    u8 currentBox;
    struct BoxPokemon boxes[VANILLA_BOXES_COUNT][IN_BOX_COUNT];
    u8 boxNames[VANILLA_BOXES_COUNT][BOX_NAME_LENGTH + 1];
    u8 boxWallpapers[VANILLA_BOXES_COUNT];
    struct Pokemon fusions[MAX_FUSION_STORAGE];
};

void PokemonStorage_UpgradeLayout(void);

extern struct PokemonStorage *gPokemonStoragePtr;

void DrawTextWindowAndBufferTiles(const u8 *string, void *dst, u8 zero1, u8 zero2, s32 bytesToBuffer);
u8 CountMonsInBox(u8 boxId);
s16 GetFirstFreeBoxSpot(u8 boxId);
u8 CountPartyAliveNonEggMonsExcept(u8 slotToIgnore);
u16 CountPartyAliveNonEggMons_IgnoreVar0x8004Slot(void);
u8 CountPartyMons(void);
u8 *StringCopyAndFillWithSpaces(u8 *dst, const u8 *src, u16 n);
void ShowPokemonStorageSystemPC(void);
void ResetPokemonStorageSystem(void);
s16 CompactPartySlots(void);
u8 StorageGetCurrentBox(void);
u32 GetBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request);
void SetBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request, const void *value);
u32 GetCurrentBoxMonData(u8 boxPosition, s32 request);
void SetCurrentBoxMonData(u8 boxPosition, s32 request, const void *value);
u32 GetAndCopyBoxMonDataAt(u8 boxId, u8 boxPosition, s32 request, void *dst);
void SetBoxMonAt(u8 boxId, u8 boxPosition, struct BoxPokemon *src);
void ZeroBoxMonAt(u8 boxId, u8 boxPosition);
void BoxMonAtToMon(u8 boxId, u8 boxPosition, struct Pokemon *dst);
struct BoxPokemon *GetBoxedMonPtr(u8 boxId, u8 boxPosition);
u8 *GetBoxNamePtr(u8 boxId);
s16 AdvanceStorageMonIndex(struct BoxPokemon *boxMons, u8 currIndex, u8 maxIndex, u8 mode);
bool8 CheckFreePokemonStorageSpace(void);
bool32 CheckBoxMonSanityAt(u32 boxId, u32 boxPosition);
u32 CountStorageNonEggMons(void);
bool32 AnyStorageMonWithMove(enum Move move);

void ResetWaldaWallpaper(void);
void SetWaldaWallpaperLockedOrUnlocked(bool32 unlocked);
bool32 IsWaldaWallpaperUnlocked(void);
u32 GetWaldaWallpaperPatternId(void);
void SetWaldaWallpaperPatternId(u8 id);
u32 GetWaldaWallpaperIconId(void);
void SetWaldaWallpaperIconId(u8 id);
u16 *GetWaldaWallpaperColorsPtr(void);
void SetWaldaWallpaperColors(u16 color1, u16 color2);
u8 *GetWaldaPhrasePtr(void);
void SetWaldaPhrase(const u8 *src);
bool32 IsWaldaPhraseEmpty(void);

void ChooseMonFromStorage();
u32 CountPartyNonEggMons(void);
void RemoveSelectedPcMon(struct Pokemon *mon);

#endif // GUARD_POKEMON_STORAGE_SYSTEM_H
