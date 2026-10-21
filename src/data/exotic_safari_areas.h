// The Exotic Pokémon Safari's areas (made by tools/gym_hack/build_exotic_safari.py)
struct ExoticSafariArea { u8 mapGroup, mapNum; u8 region; u8 spotCount; const u8 (*spots)[3]; };
struct ExoticSafariRoamer { u16 species, caughtFlag, hideFlag, needFlag; u8 region; };
struct ExoticSafariFind { u16 item, flag; };

static const u8 sSpots_ExoticSafari_PaldeaMeadow[][3] = {{5, 4, 3}, {7, 4, 3}, {22, 4, 3}, {23, 4, 3}, {5, 5, 3}, {8, 5, 3}, {9, 5, 3}, {21, 5, 3}, {23, 5, 3}, {5, 6, 3}, {24, 6, 3}, {25, 6, 3}, {5, 7, 3}, {7, 9, 3}, {10, 9, 3}, {19, 9, 3}, {21, 9, 3}, {10, 10, 3}, {11, 10, 3}, {18, 10, 3}, {20, 10, 3}, {22, 10, 3}, {23, 10, 3}, {7, 11, 3}, {8, 11, 3}, {20, 11, 3}, {22, 11, 3}, {24, 11, 3}, {7, 12, 3}, {8, 12, 3}, {9, 12, 3}, {10, 12, 3}, {21, 12, 3}, {22, 12, 3}, {14, 14, 3}, {16, 14, 3}, {11, 15, 3}, {18, 15, 3}, {11, 16, 3}, {13, 16, 3}, {14, 16, 3}, {15, 16, 3}, {16, 16, 3}, {18, 16, 3}, {16, 17, 3}, {18, 17, 3}, {13, 18, 3}, {17, 18, 3}};
static const u8 sSpots_ExoticSafari_PaldeaForest[][3] = {{9, 4, 3}, {7, 5, 3}, {20, 5, 3}, {24, 5, 3}, {12, 6, 3}, {14, 6, 3}, {18, 6, 3}, {3, 7, 3}, {27, 7, 3}, {7, 9, 3}, {8, 9, 3}, {6, 12, 3}, {8, 12, 3}, {13, 12, 3}, {14, 12, 3}, {26, 12, 3}, {7, 13, 3}, {10, 13, 3}, {13, 13, 3}, {22, 13, 3}, {11, 14, 3}, {14, 14, 3}, {15, 14, 3}, {17, 14, 3}, {24, 14, 3}, {29, 14, 3}, {17, 15, 3}, {28, 16, 3}, {3, 18, 3}, {10, 18, 3}, {17, 18, 3}, {20, 18, 3}, {29, 18, 3}, {3, 19, 3}, {12, 19, 3}, {18, 19, 3}, {28, 19, 3}, {4, 20, 3}, {8, 20, 3}, {10, 20, 3}, {14, 20, 3}, {26, 20, 3}, {21, 22, 3}, {19, 23, 3}, {21, 23, 3}, {26, 23, 3}, {21, 24, 3}, {25, 24, 3}};
static const u8 sSpots_ExoticSafari_PaldeaDesert[][3] = {{14, 1, 3}, {13, 2, 3}, {5, 6, 3}, {9, 7, 3}, {11, 7, 3}, {15, 9, 3}, {6, 11, 3}, {17, 11, 3}, {1, 12, 3}, {1, 13, 3}, {5, 13, 3}, {7, 14, 3}, {10, 14, 3}, {7, 15, 3}, {23, 15, 3}, {15, 17, 3}, {0, 19, 3}, {3, 19, 3}, {22, 19, 3}, {10, 20, 3}, {16, 27, 3}, {17, 27, 3}, {19, 27, 3}, {11, 30, 3}, {22, 30, 3}, {19, 32, 3}, {16, 35, 3}, {18, 36, 3}, {24, 36, 3}, {17, 37, 3}, {16, 38, 3}, {12, 39, 3}, {25, 39, 3}, {19, 40, 3}, {22, 40, 3}, {15, 41, 3}, {14, 42, 3}, {15, 42, 3}, {10, 44, 3}, {19, 44, 3}, {11, 45, 3}, {10, 46, 3}, {13, 46, 3}, {12, 47, 3}, {23, 47, 3}, {23, 48, 3}, {22, 49, 3}, {11, 50, 3}};
static const u8 sSpots_ExoticSafari_PaldeaCrater[][3] = {{8, 2, 3}, {33, 2, 3}, {35, 2, 3}, {34, 3, 3}, {1, 6, 3}, {7, 6, 3}, {11, 6, 3}, {23, 6, 4}, {23, 7, 4}, {25, 7, 4}, {2, 8, 3}, {18, 8, 4}, {28, 8, 4}, {5, 9, 3}, {28, 9, 4}, {7, 10, 3}, {24, 10, 3}, {32, 10, 4}, {5, 11, 3}, {34, 11, 4}, {23, 12, 3}, {25, 12, 3}, {28, 12, 4}, {18, 13, 3}, {21, 14, 3}, {23, 14, 3}, {26, 14, 3}, {30, 14, 4}, {33, 14, 3}, {37, 14, 3}, {16, 15, 4}, {19, 15, 3}, {24, 15, 3}, {4, 16, 3}, {15, 16, 4}, {33, 16, 3}, {2, 17, 3}, {15, 17, 4}, {26, 17, 4}, {10, 18, 3}, {6, 19, 3}, {7, 19, 3}, {9, 19, 3}, {6, 20, 3}, {10, 20, 3}, {12, 20, 3}, {33, 20, 3}, {13, 21, 3}};
static const u8 sSpots_ExoticSafari_PaldeaCraterDepths[][3] = {{5, 1, 3}, {6, 1, 3}, {9, 1, 3}, {19, 1, 3}, {38, 1, 3}, {4, 2, 3}, {9, 2, 3}, {12, 2, 3}, {2, 3, 3}, {23, 3, 3}, {2, 4, 3}, {26, 4, 3}, {5, 5, 3}, {12, 5, 3}, {15, 5, 3}, {33, 6, 3}, {6, 7, 3}, {7, 7, 3}, {12, 7, 3}, {23, 8, 4}, {25, 8, 4}, {29, 8, 4}, {33, 8, 3}, {34, 9, 3}, {17, 10, 4}, {18, 10, 4}, {19, 10, 4}, {22, 10, 4}, {33, 10, 3}, {5, 12, 3}, {22, 12, 3}, {6, 13, 3}, {19, 13, 3}, {25, 13, 3}, {28, 13, 4}, {30, 13, 4}, {5, 14, 3}, {25, 14, 3}, {27, 14, 4}, {21, 15, 4}, {27, 15, 4}, {33, 16, 3}, {20, 17, 4}, {25, 18, 3}, {23, 19, 3}, {23, 20, 3}, {24, 20, 3}, {23, 21, 3}};
static const u8 sSpots_ExoticSafari_PaldeaLake[][3] = {{4, 4, 3}, {13, 4, 3}, {20, 4, 3}, {22, 4, 3}, {33, 4, 3}, {6, 5, 3}, {15, 5, 3}, {19, 5, 3}, {22, 5, 3}, {23, 5, 3}, {32, 5, 3}, {34, 5, 3}, {3, 6, 3}, {14, 6, 3}, {4, 25, 3}, {12, 25, 3}, {13, 25, 3}, {29, 25, 3}, {39, 25, 3}, {11, 26, 3}, {14, 26, 3}, {15, 26, 3}, {16, 26, 3}, {17, 26, 3}, {18, 26, 3}, {27, 26, 3}, {28, 26, 3}, {29, 26, 3}, {39, 26, 3}, {41, 26, 3}, {3, 27, 3}, {4, 27, 3}, {5, 27, 3}, {12, 27, 3}, {13, 27, 3}, {17, 27, 3}, {18, 27, 3}, {28, 27, 3}, {31, 27, 3}, {37, 27, 3}, {4, 28, 3}, {5, 28, 3}, {6, 28, 3}, {13, 28, 3}, {15, 28, 3}, {28, 28, 3}, {29, 28, 3}, {31, 28, 3}};
static const u8 sSpots_ExoticSafari_PaldeaSnowyMountain[][3] = {{17, 7, 3}, {19, 11, 3}, {19, 13, 3}, {18, 15, 3}, {18, 17, 3}, {17, 19, 3}, {15, 22, 3}, {17, 22, 3}, {28, 22, 3}, {14, 23, 3}, {18, 23, 3}, {35, 23, 3}, {2, 24, 3}, {3, 25, 3}, {13, 25, 3}, {31, 25, 3}, {36, 25, 3}, {14, 26, 3}, {27, 26, 3}, {29, 26, 3}, {2, 27, 3}, {20, 27, 3}, {4, 28, 3}, {16, 28, 3}, {18, 28, 3}, {14, 29, 3}, {23, 29, 3}, {27, 29, 3}, {32, 29, 3}, {6, 30, 3}, {21, 30, 3}, {24, 30, 3}, {32, 30, 3}, {1, 31, 3}, {8, 31, 3}, {20, 31, 3}, {21, 31, 3}, {30, 31, 3}, {1, 32, 3}, {26, 32, 3}, {36, 32, 3}, {18, 33, 3}, {19, 33, 3}, {20, 33, 3}, {32, 33, 3}, {19, 34, 3}, {25, 34, 3}, {26, 34, 3}};
static const u8 sSpots_ExoticSafari_PaldeaAcademy[][3] = {{5, 3, 3}, {12, 3, 3}, {21, 3, 3}, {34, 3, 3}, {35, 3, 3}, {24, 4, 3}, {25, 4, 3}, {34, 4, 3}, {14, 5, 3}, {17, 6, 3}, {2, 7, 3}, {8, 7, 3}, {1, 8, 3}, {11, 8, 3}, {24, 8, 3}, {23, 9, 3}, {35, 12, 3}, {17, 14, 3}, {4, 15, 3}, {10, 15, 3}, {36, 15, 3}, {4, 16, 3}, {32, 16, 3}, {33, 16, 3}, {15, 17, 3}, {10, 19, 3}, {13, 19, 3}, {3, 20, 3}, {7, 20, 3}, {14, 20, 3}, {29, 20, 3}, {14, 22, 3}, {34, 22, 3}, {3, 23, 3}, {11, 23, 3}, {26, 23, 3}, {32, 23, 3}, {10, 24, 3}, {11, 24, 3}, {12, 24, 3}, {10, 25, 3}, {3, 26, 3}, {12, 28, 3}, {15, 28, 3}, {6, 29, 3}, {12, 30, 3}, {36, 30, 3}, {33, 32, 3}};
static const u8 sSpots_ExoticSafari_GalarMeadow[][3] = {{4, 0, 3}, {6, 0, 3}, {10, 0, 3}, {11, 0, 3}, {20, 0, 3}, {21, 0, 3}, {24, 0, 3}, {33, 0, 3}, {38, 0, 3}, {32, 3, 3}, {4, 4, 3}, {15, 4, 3}, {16, 4, 3}, {24, 4, 3}, {25, 4, 3}, {26, 4, 3}, {35, 4, 3}, {5, 5, 3}, {7, 5, 3}, {11, 5, 3}, {19, 5, 3}, {38, 5, 3}, {23, 16, 3}, {23, 18, 3}, {21, 19, 3}, {23, 19, 3}, {19, 20, 3}, {22, 21, 3}, {23, 21, 3}, {10, 22, 3}, {7, 23, 3}, {15, 23, 3}, {4, 24, 3}, {5, 24, 3}, {6, 24, 3}, {11, 24, 3}, {12, 24, 3}, {22, 24, 3}, {24, 24, 3}, {38, 24, 3}, {9, 25, 3}, {11, 25, 3}, {6, 26, 3}, {15, 26, 3}, {21, 26, 3}, {6, 27, 3}, {13, 27, 3}, {14, 27, 3}};
static const u8 sSpots_ExoticSafari_GalarLake[][3] = {{5, 9, 3}, {6, 9, 3}, {6, 11, 3}, {6, 12, 3}, {9, 12, 3}, {27, 12, 3}, {30, 12, 3}, {8, 13, 3}, {9, 13, 3}, {16, 13, 3}, {28, 13, 3}, {6, 14, 3}, {15, 14, 3}, {28, 14, 3}, {6, 15, 3}, {8, 15, 3}, {16, 15, 3}, {27, 15, 3}, {30, 15, 3}, {26, 16, 3}, {4, 20, 3}, {19, 20, 3}, {4, 22, 3}, {6, 22, 3}, {3, 23, 3}, {18, 23, 3}, {4, 24, 3}, {10, 24, 3}, {17, 24, 3}, {10, 26, 3}, {9, 27, 3}, {11, 27, 3}, {15, 27, 3}, {16, 31, 3}, {17, 31, 3}, {14, 32, 3}, {15, 32, 3}, {12, 33, 3}, {13, 33, 3}, {14, 33, 3}, {31, 34, 3}, {20, 35, 3}, {21, 35, 3}, {32, 35, 3}, {20, 36, 3}, {28, 36, 3}, {29, 36, 3}, {21, 37, 0}};
static const u8 sSpots_ExoticSafari_GalarMine[][3] = {{22, 3, 3}, {28, 3, 3}, {34, 3, 3}, {9, 4, 3}, {13, 4, 3}, {17, 4, 3}, {20, 4, 3}, {24, 4, 3}, {30, 4, 3}, {18, 5, 3}, {32, 5, 0}, {33, 5, 3}, {39, 5, 3}, {26, 6, 3}, {27, 6, 3}, {37, 6, 3}, {4, 7, 3}, {5, 7, 3}, {19, 7, 3}, {24, 7, 3}, {25, 7, 3}, {8, 8, 3}, {9, 8, 3}, {20, 8, 3}, {21, 8, 3}, {29, 8, 3}, {30, 8, 3}, {35, 8, 3}, {37, 8, 3}, {8, 9, 3}, {10, 9, 3}, {16, 9, 3}, {31, 9, 3}, {37, 9, 3}, {38, 9, 3}, {3, 10, 3}, {6, 10, 3}, {8, 10, 3}, {30, 10, 3}, {33, 10, 3}, {34, 10, 3}, {35, 10, 3}, {37, 10, 3}, {11, 11, 3}, {12, 11, 3}, {36, 11, 3}, {37, 11, 3}, {12, 12, 3}};
static const u8 sSpots_ExoticSafari_GalarMineDepths[][3] = {{3, 1, 3}, {4, 1, 3}, {5, 1, 3}, {7, 1, 3}, {13, 1, 3}, {21, 1, 3}, {13, 3, 3}, {16, 3, 3}, {2, 4, 3}, {20, 4, 3}, {1, 5, 3}, {3, 5, 3}, {6, 5, 3}, {30, 5, 3}, {2, 6, 3}, {3, 6, 3}, {20, 6, 3}, {22, 6, 3}, {23, 6, 3}, {26, 6, 3}, {7, 7, 3}, {10, 11, 3}, {6, 12, 3}, {16, 12, 3}, {26, 12, 3}, {29, 12, 3}, {7, 13, 3}, {11, 13, 3}, {24, 13, 3}, {4, 15, 3}, {12, 15, 3}, {13, 15, 3}, {22, 15, 3}, {3, 16, 3}, {22, 16, 3}, {23, 16, 3}, {28, 16, 3}, {29, 16, 3}, {22, 17, 3}, {23, 17, 3}, {29, 17, 3}, {21, 18, 3}, {22, 19, 3}, {24, 19, 3}, {12, 20, 3}, {24, 20, 3}, {13, 21, 3}, {27, 22, 3}};
static const u8 sSpots_ExoticSafari_GalarStadium[][3] = {{0, 5, 0}, {5, 5, 0}, {9, 5, 0}, {11, 5, 0}, {12, 5, 0}, {15, 5, 0}, {20, 5, 0}, {3, 6, 0}, {5, 6, 0}, {9, 6, 0}, {14, 6, 0}, {17, 6, 0}, {20, 6, 0}, {5, 7, 0}, {8, 7, 0}, {11, 7, 0}, {12, 7, 0}, {13, 7, 0}, {14, 7, 0}, {22, 7, 0}, {14, 8, 0}, {21, 8, 0}, {17, 9, 0}, {2, 11, 0}, {5, 11, 0}, {17, 11, 0}, {18, 11, 0}, {20, 11, 0}, {21, 11, 0}, {5, 12, 0}, {7, 12, 0}, {13, 12, 0}, {14, 12, 0}, {16, 12, 0}, {2, 13, 0}, {13, 13, 0}, {14, 13, 0}, {18, 13, 0}, {19, 13, 0}, {20, 13, 0}, {6, 14, 0}, {9, 14, 0}, {9, 15, 0}, {12, 15, 0}, {13, 15, 0}, {20, 15, 0}, {6, 16, 0}, {16, 16, 0}};
static const u8 sSpots_ExoticSafari_GalarStadiumPitch[][3] = {{1, 4, 0}, {10, 4, 0}, {11, 4, 0}, {18, 4, 0}, {0, 5, 0}, {1, 5, 0}, {2, 5, 0}, {11, 5, 0}, {15, 5, 0}, {18, 5, 0}, {19, 5, 0}, {1, 6, 0}, {3, 6, 0}, {5, 6, 0}, {6, 6, 0}, {7, 6, 0}, {19, 6, 0}, {1, 7, 0}, {2, 7, 0}, {4, 7, 0}, {11, 7, 0}, {12, 7, 0}, {14, 7, 0}, {19, 7, 0}, {4, 8, 0}, {5, 8, 0}, {6, 8, 0}, {10, 8, 0}, {7, 9, 0}, {12, 9, 0}, {13, 9, 0}, {1, 10, 0}, {4, 10, 0}, {6, 10, 0}, {7, 10, 0}, {6, 11, 0}, {15, 11, 0}, {17, 11, 0}, {19, 11, 0}, {0, 12, 0}, {3, 12, 0}, {5, 12, 0}, {7, 12, 0}, {11, 12, 0}, {15, 12, 0}, {16, 12, 0}, {8, 13, 0}, {9, 13, 0}};
static const u8 sSpots_ExoticSafari_GalarCity[][3] = {{1, 10, 3}, {2, 10, 3}, {3, 10, 3}, {4, 10, 3}, {5, 10, 3}, {6, 10, 3}, {7, 10, 3}, {8, 10, 3}, {9, 10, 3}, {10, 10, 3}, {11, 10, 3}, {14, 10, 3}, {1, 11, 3}, {5, 11, 3}, {6, 11, 3}, {8, 11, 3}, {10, 11, 3}, {1, 12, 3}, {2, 12, 3}, {4, 12, 3}, {5, 12, 3}, {10, 12, 3}, {0, 13, 3}, {1, 13, 3}, {2, 13, 3}, {3, 13, 3}, {4, 13, 3}, {7, 13, 3}, {8, 13, 3}, {9, 13, 3}, {10, 13, 3}, {11, 13, 3}, {0, 14, 3}, {3, 14, 3}, {6, 14, 3}, {8, 14, 3}, {9, 14, 3}, {10, 14, 3}, {11, 14, 3}, {24, 16, 3}, {16, 17, 3}, {25, 17, 3}, {37, 17, 3}, {16, 19, 3}, {24, 19, 3}, {7, 20, 3}, {8, 20, 3}, {7, 28, 3}};
static const u8 sSpots_ExoticSafari_GalarForest[][3] = {{25, 0, 3}, {38, 0, 3}, {38, 1, 3}, {38, 3, 3}, {37, 4, 3}, {11, 5, 3}, {20, 5, 3}, {4, 6, 3}, {28, 6, 3}, {4, 7, 3}, {28, 7, 3}, {6, 8, 3}, {8, 8, 3}, {37, 8, 3}, {28, 9, 3}, {31, 9, 3}, {28, 10, 3}, {18, 11, 3}, {23, 11, 3}, {16, 13, 3}, {21, 13, 3}, {22, 13, 3}, {16, 14, 3}, {14, 15, 3}, {8, 16, 3}, {2, 19, 3}, {6, 19, 3}, {0, 23, 3}, {0, 24, 3}, {14, 25, 3}, {17, 25, 3}, {13, 26, 3}, {1, 28, 3}, {4, 28, 3}, {7, 28, 3}, {25, 28, 3}, {26, 28, 3}, {32, 28, 3}, {18, 29, 3}, {24, 31, 3}, {8, 34, 3}, {13, 34, 3}, {30, 34, 3}, {33, 34, 3}, {26, 35, 3}, {5, 37, 3}, {3, 39, 3}, {35, 39, 3}};

static const struct ExoticSafariArea sExoticSafariAreas[] =
{
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_MEADOW), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_MEADOW), 0, ARRAY_COUNT(sSpots_ExoticSafari_PaldeaMeadow), sSpots_ExoticSafari_PaldeaMeadow},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_FOREST), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_FOREST), 0, ARRAY_COUNT(sSpots_ExoticSafari_PaldeaForest), sSpots_ExoticSafari_PaldeaForest},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_DESERT), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_DESERT), 0, ARRAY_COUNT(sSpots_ExoticSafari_PaldeaDesert), sSpots_ExoticSafari_PaldeaDesert},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_CRATER), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_CRATER), 0, ARRAY_COUNT(sSpots_ExoticSafari_PaldeaCrater), sSpots_ExoticSafari_PaldeaCrater},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_CRATER_DEPTHS), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_CRATER_DEPTHS), 0, ARRAY_COUNT(sSpots_ExoticSafari_PaldeaCraterDepths), sSpots_ExoticSafari_PaldeaCraterDepths},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_LAKE), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_LAKE), 0, ARRAY_COUNT(sSpots_ExoticSafari_PaldeaLake), sSpots_ExoticSafari_PaldeaLake},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_SNOWY_MOUNTAIN), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_SNOWY_MOUNTAIN), 0, ARRAY_COUNT(sSpots_ExoticSafari_PaldeaSnowyMountain), sSpots_ExoticSafari_PaldeaSnowyMountain},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_ACADEMY), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_ACADEMY), 0, ARRAY_COUNT(sSpots_ExoticSafari_PaldeaAcademy), sSpots_ExoticSafari_PaldeaAcademy},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_MEADOW), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_MEADOW), 1, ARRAY_COUNT(sSpots_ExoticSafari_GalarMeadow), sSpots_ExoticSafari_GalarMeadow},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_LAKE), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_LAKE), 1, ARRAY_COUNT(sSpots_ExoticSafari_GalarLake), sSpots_ExoticSafari_GalarLake},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_MINE), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_MINE), 1, ARRAY_COUNT(sSpots_ExoticSafari_GalarMine), sSpots_ExoticSafari_GalarMine},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_MINE_DEPTHS), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_MINE_DEPTHS), 1, ARRAY_COUNT(sSpots_ExoticSafari_GalarMineDepths), sSpots_ExoticSafari_GalarMineDepths},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_STADIUM), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_STADIUM), 1, ARRAY_COUNT(sSpots_ExoticSafari_GalarStadium), sSpots_ExoticSafari_GalarStadium},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_STADIUM_PITCH), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_STADIUM_PITCH), 1, ARRAY_COUNT(sSpots_ExoticSafari_GalarStadiumPitch), sSpots_ExoticSafari_GalarStadiumPitch},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_CITY), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_CITY), 1, ARRAY_COUNT(sSpots_ExoticSafari_GalarCity), sSpots_ExoticSafari_GalarCity},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_FOREST), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_FOREST), 1, ARRAY_COUNT(sSpots_ExoticSafari_GalarForest), sSpots_ExoticSafari_GalarForest},
};

#define EXOTIC_SAFARI_ROAMER_COUNT 8
static const struct ExoticSafariRoamer sExoticSafariRoamers[EXOTIC_SAFARI_ROAMER_COUNT] =
{
    {SPECIES_OKIDOGI, FLAG_EXOTIC_LEGEND_OKIDOGI, FLAG_TEMP_1B, 0, 0},
    {SPECIES_MUNKIDORI, FLAG_EXOTIC_LEGEND_MUNKIDORI, FLAG_TEMP_1C, 0, 0},
    {SPECIES_FEZANDIPITI, FLAG_EXOTIC_LEGEND_FEZANDIPITI, FLAG_TEMP_1D, 0, 0},
    {SPECIES_SPECTRIER, FLAG_EXOTIC_LEGEND_SPECTRIER, FLAG_TEMP_1B, 0, 1},
    {SPECIES_CALYREX, FLAG_EXOTIC_LEGEND_CALYREX, FLAG_TEMP_1C, FLAG_EXOTIC_LEGEND_SPECTRIER, 1},
    {SPECIES_ARTICUNO_GALAR, FLAG_EXOTIC_LEGEND_ARTICUNO_GALAR, FLAG_TEMP_1D, 0, 1},
    {SPECIES_ZAPDOS_GALAR, FLAG_EXOTIC_LEGEND_ZAPDOS_GALAR, FLAG_TEMP_1E, 0, 1},
    {SPECIES_MOLTRES_GALAR, FLAG_EXOTIC_LEGEND_MOLTRES_GALAR, FLAG_TEMP_1F, 0, 1},
};
#define EXOTIC_SAFARI_ROAMER_LOCALID(hideFlag) (1 + (hideFlag) - FLAG_TEMP_1B)

static const u16 sKioskSupplies[] = {ITEM_POKE_BALL, ITEM_GREAT_BALL, ITEM_ULTRA_BALL, ITEM_QUICK_BALL, ITEM_DUSK_BALL, ITEM_NET_BALL, ITEM_MAX_REPEL, ITEM_HYPER_POTION, ITEM_MAX_POTION, ITEM_FULL_HEAL, ITEM_REVIVE};
static const struct ExoticSafariFind sKioskFinds[] =
{
    {ITEM_AUSPICIOUS_ARMOR, FLAG_EXOTIC_ITEM_AUSPICIOUS_ARMOR},
    {ITEM_MALICIOUS_ARMOR, FLAG_EXOTIC_ITEM_MALICIOUS_ARMOR},
    {ITEM_ICE_STONE, FLAG_EXOTIC_ITEM_ICE_STONE},
    {ITEM_UNREMARKABLE_TEACUP, FLAG_EXOTIC_ITEM_UNREMARKABLE_TEACUP},
    {ITEM_MASTERPIECE_TEACUP, FLAG_EXOTIC_ITEM_MASTERPIECE_TEACUP},
    {ITEM_TART_APPLE, FLAG_EXOTIC_ITEM_TART_APPLE},
    {ITEM_SWEET_APPLE, FLAG_EXOTIC_ITEM_SWEET_APPLE},
    {ITEM_GALARICA_CUFF, FLAG_EXOTIC_ITEM_GALARICA_CUFF},
    {ITEM_BLACK_AUGURITE, FLAG_EXOTIC_ITEM_BLACK_AUGURITE},
    {ITEM_PEAT_BLOCK, FLAG_EXOTIC_ITEM_PEAT_BLOCK},
    {ITEM_CRACKED_POT, FLAG_EXOTIC_ITEM_CRACKED_POT},
    {ITEM_RAZOR_CLAW, FLAG_EXOTIC_ITEM_RAZOR_CLAW},
    {ITEM_CHIPPED_POT, FLAG_EXOTIC_ITEM_CHIPPED_POT},
    {ITEM_STRAWBERRY_SWEET, FLAG_EXOTIC_ITEM_STRAWBERRY_SWEET},
    {ITEM_BERRY_SWEET, FLAG_EXOTIC_ITEM_STRAWBERRY_SWEET},
    {ITEM_LOVE_SWEET, FLAG_EXOTIC_ITEM_STRAWBERRY_SWEET},
    {ITEM_STAR_SWEET, FLAG_EXOTIC_ITEM_STRAWBERRY_SWEET},
    {ITEM_CLOVER_SWEET, FLAG_EXOTIC_ITEM_STRAWBERRY_SWEET},
    {ITEM_FLOWER_SWEET, FLAG_EXOTIC_ITEM_STRAWBERRY_SWEET},
    {ITEM_RIBBON_SWEET, FLAG_EXOTIC_ITEM_STRAWBERRY_SWEET},
    {ITEM_GALARICA_WREATH, FLAG_EXOTIC_ITEM_GALARICA_WREATH},
};

// The legendaries that stand in one place, and what each waits for
struct ExoticSafariStatic { u8 mapGroup, mapNum; u16 species, caughtFlag, hideFlag; u8 seal, sealArg, localId; };
static const struct ExoticSafariStatic sExoticSafariStatics[] =
{
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_MEADOW), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_MEADOW), SPECIES_OGERPON, FLAG_EXOTIC_LEGEND_OGERPON, FLAG_TEMP_11, SEAL_MASKS, 0, 4},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_FOREST), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_FOREST), SPECIES_WO_CHIEN, FLAG_EXOTIC_LEGEND_WO_CHIEN, FLAG_TEMP_11, SEAL_STAKES, 0, 4},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_DESERT), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_DESERT), SPECIES_TING_LU, FLAG_EXOTIC_LEGEND_TING_LU, FLAG_TEMP_11, SEAL_STAKES, 2, 4},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_CRATER_DEPTHS), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_CRATER_DEPTHS), SPECIES_TERAPAGOS, FLAG_EXOTIC_LEGEND_TERAPAGOS, FLAG_TEMP_11, SEAL_MASTERY, 0, 4},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_CRATER_DEPTHS), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_CRATER_DEPTHS), SPECIES_PECHARUNT, FLAG_EXOTIC_LEGEND_PECHARUNT, FLAG_TEMP_12, SEAL_ROAMERS, 0, 5},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_LAKE), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_LAKE), SPECIES_CHI_YU, FLAG_EXOTIC_LEGEND_CHI_YU, FLAG_TEMP_11, SEAL_STAKES, 3, 4},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_PALDEA_SNOWY_MOUNTAIN), MAP_NUM(MAP_EXOTIC_SAFARI_PALDEA_SNOWY_MOUNTAIN), SPECIES_CHIEN_PAO, FLAG_EXOTIC_LEGEND_CHIEN_PAO, FLAG_TEMP_11, SEAL_STAKES, 1, 4},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_MEADOW), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_MEADOW), SPECIES_REGIELEKI, FLAG_EXOTIC_LEGEND_REGIELEKI, FLAG_TEMP_11, SEAL_STONES, 0, 6},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_MINE_DEPTHS), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_MINE_DEPTHS), SPECIES_REGIDRAGO, FLAG_EXOTIC_LEGEND_REGIDRAGO, FLAG_TEMP_11, SEAL_DRAGON, 2, 6},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_STADIUM_PITCH), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_STADIUM_PITCH), SPECIES_KUBFU, FLAG_EXOTIC_LEGEND_KUBFU, FLAG_TEMP_11, SEAL_DOJO, 0, 6},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_CITY), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_CITY), SPECIES_ZACIAN, FLAG_EXOTIC_LEGEND_ZACIAN, FLAG_TEMP_11, SEAL_ITEM, 0, 6},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_CITY), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_CITY), SPECIES_ZAMAZENTA, FLAG_EXOTIC_LEGEND_ZAMAZENTA, FLAG_TEMP_12, SEAL_ITEM, 1, 7},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_FOREST), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_FOREST), SPECIES_ZARUDE, FLAG_EXOTIC_LEGEND_ZARUDE, FLAG_TEMP_11, SEAL_LAMPS, 0, 6},
    {MAP_GROUP(MAP_EXOTIC_SAFARI_GALAR_FOREST), MAP_NUM(MAP_EXOTIC_SAFARI_GALAR_FOREST), SPECIES_ENAMORUS, FLAG_EXOTIC_LEGEND_ENAMORUS, FLAG_TEMP_12, SEAL_FRIENDSHIP, 0, 7},
};

#define EXOTIC_SAFARI_STAKE_COLORS 4
#define EXOTIC_SAFARI_STAKES_PER_COLOR 4
#define EXOTIC_SAFARI_STAKE_FLAGS 0x0EF
static const u8 *const sStakeColorNames[] = {COMPOUND_STRING("PURPLE"), COMPOUND_STRING("YELLOW"), COMPOUND_STRING("GREEN"), COMPOUND_STRING("BLUE")};
static const u16 sMaskFlags[] = {FLAG_EXOTIC_ITEM_WELLSPRING_MASK, FLAG_EXOTIC_ITEM_HEARTHFLAME_MASK, FLAG_EXOTIC_ITEM_CORNERSTONE_MASK};

// Zarude's lamps: Glimwood's big mushrooms (top-left corners), dark until touched
#define EXOTIC_SAFARI_LAMP_MAP MAP_EXOTIC_SAFARI_GALAR_FOREST
#define EXOTIC_SAFARI_LAMP_BIG_LIT 0x2BB     // (2x2)
#define EXOTIC_SAFARI_LAMP_BIG_DARK 0x2C7
#define EXOTIC_SAFARI_LAMP_GIANT_LIT 0x2C1   // (3x2, in front of a tree)
#define EXOTIC_SAFARI_LAMP_GIANT_DARK 0x2CB
static const u8 sLamps[][3] = {{3, 37, 2}, {20, 10, 2}, {6, 4, 2}, {35, 26, 2}, {14, 5, 2}, {33, 38, 2}, {24, 38, 2}, {9, 18, 3}};   // x, y, width
// Regieleki's stepping stones, in the order they're to be stepped on
#define EXOTIC_SAFARI_STONE_MAP MAP_EXOTIC_SAFARI_GALAR_MEADOW
#define EXOTIC_SAFARI_STONE_DARK 0x3D4   // (+ x % 2: the grass under it)
#define EXOTIC_SAFARI_STONE_LIT 0x3D6
static const u8 sStones[][2] = {{23, 8}, {21, 9}, {19, 10}, {22, 10}, {20, 11}, {18, 12}, {16, 13}};
// The items the Galar legendaries wait for (SEAL_ITEM, SEAL_DRAGON: their index)
static const u16 sSealItems[][2] = {{ITEM_RUSTED_SWORD, FLAG_EXOTIC_ITEM_RUSTED_SWORD}, {ITEM_RUSTED_SHIELD, FLAG_EXOTIC_ITEM_RUSTED_SHIELD}, {ITEM_DRAGON_FANG, FLAG_EXOTIC_ITEM_DRAGON_FANG}};
