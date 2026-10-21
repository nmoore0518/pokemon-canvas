// The Exotic Pokémon Safari's areas (made by tools/gym_hack/build_exotic_safari.py)
struct ExoticSafariArea { u8 mapGroup, mapNum; u8 spotCount; const u8 (*spots)[3]; };
struct ExoticSafariRoamer { u16 species, caughtFlag, hideFlag; };
struct ExoticSafariFind { u16 item, flag; };

static const u8 sSpots_ExoticSafari_PaldeaMeadow[][3] = {{5, 4, 3}, {7, 4, 3}, {22, 4, 3}, {23, 4, 3}, {5, 5, 3}, {8, 5, 3}, {9, 5, 3}, {21, 5, 3}, {23, 5, 3}, {5, 6, 3}, {24, 6, 3}, {25, 6, 3}, {5, 7, 3}, {7, 9, 3}, {10, 9, 3}, {19, 9, 3}, {21, 9, 3}, {10, 10, 3}, {11, 10, 3}, {18, 10, 3}, {20, 10, 3}, {22, 10, 3}, {23, 10, 3}, {7, 11, 3}, {8, 11, 3}, {20, 11, 3}, {22, 11, 3}, {24, 11, 3}, {7, 12, 3}, {8, 12, 3}, {9, 12, 3}, {10, 12, 3}, {21, 12, 3}, {22, 12, 3}, {14, 14, 3}, {16, 14, 3}, {11, 15, 3}, {18, 15, 3}, {11, 16, 3}, {13, 16, 3}, {14, 16, 3}, {15, 16, 3}, {16, 16, 3}, {18, 16, 3}, {16, 17, 3}, {18, 17, 3}, {13, 18, 3}, {17, 18, 3}};
static const u8 sSpots_ExoticSafari_PaldeaForest[][3] = {{9, 4, 3}, {7, 5, 3}, {20, 5, 3}, {24, 5, 3}, {12, 6, 3}, {14, 6, 3}, {18, 6, 3}, {3, 7, 3}, {27, 7, 3}, {7, 9, 3}, {8, 9, 3}, {6, 12, 3}, {8, 12, 3}, {13, 12, 3}, {14, 12, 3}, {26, 12, 3}, {7, 13, 3}, {10, 13, 3}, {13, 13, 3}, {22, 13, 3}, {11, 14, 3}, {14, 14, 3}, {15, 14, 3}, {17, 14, 3}, {24, 14, 3}, {29, 14, 3}, {17, 15, 3}, {28, 16, 3}, {3, 18, 3}, {10, 18, 3}, {17, 18, 3}, {20, 18, 3}, {29, 18, 3}, {3, 19, 3}, {12, 19, 3}, {18, 19, 3}, {28, 19, 3}, {4, 20, 3}, {8, 20, 3}, {10, 20, 3}, {14, 20, 3}, {26, 20, 3}, {21, 22, 3}, {19, 23, 3}, {21, 23, 3}, {26, 23, 3}, {21, 24, 3}, {25, 24, 3}};
static const u8 sSpots_ExoticSafari_PaldeaDesert[][3] = {{14, 1, 3}, {13, 2, 3}, {5, 6, 3}, {9, 7, 3}, {11, 7, 3}, {15, 9, 3}, {6, 11, 3}, {17, 11, 3}, {1, 12, 3}, {1, 13, 3}, {5, 13, 3}, {7, 14, 3}, {10, 14, 3}, {7, 15, 3}, {23, 15, 3}, {15, 17, 3}, {0, 19, 3}, {3, 19, 3}, {22, 19, 3}, {10, 20, 3}, {16, 27, 3}, {17, 27, 3}, {19, 27, 3}, {11, 30, 3}, {22, 30, 3}, {19, 32, 3}, {16, 35, 3}, {18, 36, 3}, {24, 36, 3}, {17, 37, 3}, {16, 38, 3}, {12, 39, 3}, {25, 39, 3}, {19, 40, 3}, {22, 40, 3}, {15, 41, 3}, {14, 42, 3}, {15, 42, 3}, {10, 44, 3}, {19, 44, 3}, {11, 45, 3}, {10, 46, 3}, {13, 46, 3}, {12, 47, 3}, {23, 47, 3}, {23, 48, 3}, {22, 49, 3}, {11, 50, 3}};
static const u8 sSpots_ExoticSafari_PaldeaCrater[][3] = {{33, 1, 3}, {8, 2, 3}, {35, 2, 3}, {33, 3, 3}, {7, 5, 3}, {1, 6, 3}, {5, 6, 3}, {11, 6, 3}, {10, 7, 3}, {2, 8, 3}, {19, 8, 4}, {24, 8, 4}, {32, 8, 4}, {33, 8, 4}, {34, 8, 4}, {5, 9, 3}, {28, 10, 4}, {5, 11, 3}, {23, 11, 3}, {31, 11, 4}, {4, 12, 3}, {7, 12, 3}, {23, 12, 3}, {25, 12, 3}, {7, 13, 3}, {21, 13, 3}, {34, 13, 3}, {20, 14, 3}, {38, 14, 3}, {28, 15, 4}, {32, 15, 3}, {4, 16, 3}, {14, 16, 4}, {30, 16, 4}, {2, 17, 3}, {14, 17, 4}, {15, 17, 4}, {28, 17, 4}, {10, 19, 3}, {20, 19, 4}, {34, 19, 3}, {12, 20, 3}, {17, 20, 4}, {20, 20, 4}, {22, 20, 4}, {29, 20, 4}, {7, 21, 3}, {34, 21, 3}};
static const u8 sSpots_ExoticSafari_PaldeaCraterDepths[][3] = {{5, 1, 3}, {6, 1, 3}, {9, 1, 3}, {19, 1, 3}, {38, 1, 3}, {4, 2, 3}, {9, 2, 3}, {12, 2, 3}, {2, 3, 3}, {23, 3, 3}, {2, 4, 3}, {26, 4, 3}, {5, 5, 3}, {12, 5, 3}, {15, 5, 3}, {33, 6, 3}, {6, 7, 3}, {7, 7, 3}, {12, 7, 3}, {23, 8, 4}, {25, 8, 4}, {29, 8, 4}, {33, 8, 3}, {34, 9, 3}, {17, 10, 4}, {18, 10, 4}, {19, 10, 4}, {22, 10, 4}, {33, 10, 3}, {5, 12, 3}, {22, 12, 3}, {6, 13, 3}, {19, 13, 3}, {25, 13, 3}, {28, 13, 4}, {30, 13, 4}, {5, 14, 3}, {25, 14, 3}, {27, 14, 4}, {21, 15, 4}, {27, 15, 4}, {33, 16, 3}, {20, 17, 4}, {25, 18, 3}, {23, 19, 3}, {23, 20, 3}, {24, 20, 3}, {23, 21, 3}};
static const u8 sSpots_ExoticSafari_PaldeaLake[][3] = {{4, 4, 3}, {13, 4, 3}, {20, 4, 3}, {22, 4, 3}, {33, 4, 3}, {6, 5, 3}, {15, 5, 3}, {19, 5, 3}, {22, 5, 3}, {23, 5, 3}, {32, 5, 3}, {34, 5, 3}, {3, 6, 3}, {14, 6, 3}, {4, 25, 3}, {12, 25, 3}, {13, 25, 3}, {29, 25, 3}, {39, 25, 3}, {11, 26, 3}, {14, 26, 3}, {15, 26, 3}, {16, 26, 3}, {17, 26, 3}, {18, 26, 3}, {27, 26, 3}, {28, 26, 3}, {29, 26, 3}, {39, 26, 3}, {41, 26, 3}, {3, 27, 3}, {4, 27, 3}, {5, 27, 3}, {12, 27, 3}, {13, 27, 3}, {17, 27, 3}, {18, 27, 3}, {28, 27, 3}, {31, 27, 3}, {37, 27, 3}, {4, 28, 3}, {5, 28, 3}, {6, 28, 3}, {13, 28, 3}, {15, 28, 3}, {28, 28, 3}, {29, 28, 3}, {31, 28, 3}};
static const u8 sSpots_ExoticSafari_PaldeaSnowyMountain[][3] = {{10, 13, 3}, {9, 16, 3}, {3, 22, 3}, {13, 22, 3}, {15, 22, 3}, {18, 22, 3}, {0, 23, 3}, {13, 23, 3}, {1, 24, 3}, {2, 24, 3}, {7, 25, 3}, {9, 25, 3}, {11, 25, 3}, {17, 25, 3}, {12, 26, 3}, {13, 26, 3}, {19, 26, 3}, {15, 27, 3}, {3, 28, 3}, {8, 28, 3}, {16, 28, 3}, {18, 28, 3}, {7, 29, 3}, {10, 29, 3}, {13, 29, 3}, {17, 29, 3}, {18, 29, 3}, {19, 29, 3}, {4, 30, 3}, {10, 30, 3}, {6, 31, 3}, {10, 31, 3}, {14, 31, 3}, {3, 32, 3}, {6, 32, 3}, {7, 32, 3}, {11, 32, 3}, {17, 32, 3}, {18, 32, 3}, {1, 33, 3}, {4, 33, 3}, {8, 33, 3}, {11, 33, 3}, {4, 34, 3}, {8, 34, 3}, {11, 34, 3}, {12, 34, 3}, {17, 34, 3}};
static const u8 sSpots_ExoticSafari_PaldeaAcademy[][3] = {{5, 3, 3}, {12, 3, 3}, {17, 4, 3}, {34, 4, 3}, {14, 5, 3}, {2, 7, 3}, {8, 7, 3}, {17, 7, 3}, {1, 8, 3}, {11, 8, 3}, {22, 8, 3}, {24, 9, 3}, {35, 12, 3}, {17, 14, 3}, {4, 15, 3}, {10, 15, 3}, {17, 15, 3}, {33, 15, 3}, {36, 15, 3}, {4, 16, 3}, {32, 16, 3}, {32, 17, 3}, {33, 17, 3}, {10, 19, 3}, {13, 19, 3}, {3, 20, 3}, {7, 20, 3}, {14, 20, 3}, {24, 21, 3}, {14, 22, 3}, {29, 22, 3}, {3, 23, 3}, {11, 23, 3}, {29, 23, 3}, {10, 24, 3}, {11, 24, 3}, {12, 24, 3}, {32, 24, 3}, {10, 25, 3}, {3, 26, 3}, {15, 26, 3}, {12, 28, 3}, {6, 29, 3}, {15, 29, 3}, {32, 29, 3}, {33, 29, 3}, {12, 30, 3}, {33, 33, 3}};

static const struct ExoticSafariArea sExoticSafariAreas[] =
{
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_MEADOW), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_MEADOW), ARRAY_COUNT(sSpots_ExoticSafari_PaldeaMeadow), sSpots_ExoticSafari_PaldeaMeadow},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_FOREST), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_FOREST), ARRAY_COUNT(sSpots_ExoticSafari_PaldeaForest), sSpots_ExoticSafari_PaldeaForest},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_DESERT), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_DESERT), ARRAY_COUNT(sSpots_ExoticSafari_PaldeaDesert), sSpots_ExoticSafari_PaldeaDesert},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_CRATER), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_CRATER), ARRAY_COUNT(sSpots_ExoticSafari_PaldeaCrater), sSpots_ExoticSafari_PaldeaCrater},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_CRATER_DEPTHS), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_CRATER_DEPTHS), ARRAY_COUNT(sSpots_ExoticSafari_PaldeaCraterDepths), sSpots_ExoticSafari_PaldeaCraterDepths},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_LAKE), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_LAKE), ARRAY_COUNT(sSpots_ExoticSafari_PaldeaLake), sSpots_ExoticSafari_PaldeaLake},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_SNOWY_MOUNTAIN), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_SNOWY_MOUNTAIN), ARRAY_COUNT(sSpots_ExoticSafari_PaldeaSnowyMountain), sSpots_ExoticSafari_PaldeaSnowyMountain},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_ACADEMY), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_ACADEMY), ARRAY_COUNT(sSpots_ExoticSafari_PaldeaAcademy), sSpots_ExoticSafari_PaldeaAcademy},
};

#define EXOTIC_SAFARI_ROAMER_COUNT 3
#define EXOTIC_SAFARI_ROAMER_LOCALID(i) (1 + (i))
static const struct ExoticSafariRoamer sExoticSafariRoamers[EXOTIC_SAFARI_ROAMER_COUNT] =
{
    {SPECIES_OKIDOGI, FLAG_EXOTIC_LEGEND_OKIDOGI, FLAG_TEMP_1D},
    {SPECIES_MUNKIDORI, FLAG_EXOTIC_LEGEND_MUNKIDORI, FLAG_TEMP_1E},
    {SPECIES_FEZANDIPITI, FLAG_EXOTIC_LEGEND_FEZANDIPITI, FLAG_TEMP_1F},
};

static const u16 sKioskSupplies[] = {ITEM_POKE_BALL, ITEM_GREAT_BALL, ITEM_ULTRA_BALL, ITEM_QUICK_BALL, ITEM_DUSK_BALL, ITEM_NET_BALL, ITEM_MAX_REPEL, ITEM_HYPER_POTION, ITEM_MAX_POTION, ITEM_FULL_HEAL, ITEM_REVIVE};
static const struct ExoticSafariFind sKioskFinds[] =
{
    {ITEM_AUSPICIOUS_ARMOR, FLAG_EXOTIC_ITEM_AUSPICIOUS_ARMOR},
    {ITEM_MALICIOUS_ARMOR, FLAG_EXOTIC_ITEM_MALICIOUS_ARMOR},
    {ITEM_ICE_STONE, FLAG_EXOTIC_ITEM_ICE_STONE},
    {ITEM_UNREMARKABLE_TEACUP, FLAG_EXOTIC_ITEM_UNREMARKABLE_TEACUP},
};
