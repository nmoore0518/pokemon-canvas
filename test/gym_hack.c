// Tests for the Gym Trainer hack's custom logic (src/gym_hack.c).
// Run with: make check GAME_VERSION=FIRERED TESTS="GymHack"

#include "global.h"
#include "test/test.h"
#include "battle_main.h"
#include "event_data.h"
#include "gym_hack.h"
#include "pokemon.h"
#include "string_util.h"
#include "constants/species.h"
#include "constants/vars.h"

// Menu labels from Oak's lab script, in menu order.
extern const u8 GymHack_Text_TypeFairy[], GymHack_Text_TypeBug[], GymHack_Text_TypeDark[],
    GymHack_Text_TypeDragon[], GymHack_Text_TypeElectric[], GymHack_Text_TypeFighting[],
    GymHack_Text_TypeFire[], GymHack_Text_TypeGhost[], GymHack_Text_TypeGrass[],
    GymHack_Text_TypeGround[], GymHack_Text_TypeIce[], GymHack_Text_TypeNormal[],
    GymHack_Text_TypePoison[], GymHack_Text_TypePsychic[], GymHack_Text_TypeRock[],
    GymHack_Text_TypeSteel[], GymHack_Text_TypeWater[];

static const u8 *const sMenuLabels[GYM_HACK_STARTER_COUNT] =
{
    GymHack_Text_TypeFairy, GymHack_Text_TypeBug, GymHack_Text_TypeDark,
    GymHack_Text_TypeDragon, GymHack_Text_TypeElectric, GymHack_Text_TypeFighting,
    GymHack_Text_TypeFire, GymHack_Text_TypeGhost, GymHack_Text_TypeGrass,
    GymHack_Text_TypeGround, GymHack_Text_TypeIce, GymHack_Text_TypeNormal,
    GymHack_Text_TypePoison, GymHack_Text_TypePsychic, GymHack_Text_TypeRock,
    GymHack_Text_TypeSteel, GymHack_Text_TypeWater,
};

static u32 ChooseStarter(u32 menuIndex)
{
    gSpecialVar_Result = menuIndex;
    GymHack_PrepareStarterChoice();
    return VarGet(VAR_TEMP_2);
}

static bool32 StringEndsWith(const u8 *str, const u8 *suffix)
{
    u32 n = StringLength(str), m = StringLength(suffix);
    return n >= m && StringCompare(str + n - m, suffix) == 0;
}

TEST("GymHack: every non-Flying type has exactly one starter")
{
    u32 i, j;
    u32 seen[NUMBER_OF_MON_TYPES] = {0};

    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
        seen[gSpeciesInfo[ChooseStarter(i)].types[0]]++;

    for (j = 0; j < NUMBER_OF_MON_TYPES; j++)
    {
        if (j == TYPE_NONE || j == TYPE_FLYING || j == TYPE_MYSTERY || j == TYPE_STELLAR)
            EXPECT_EQ(seen[j], 0);
        else
            EXPECT_EQ(seen[j], 1);
    }
}

TEST("GymHack: menu labels name the species they give")
{
    u32 i;
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
        EXPECT(StringEndsWith(sMenuLabels[i], GetSpeciesName(ChooseStarter(i))));
}

TEST("GymHack: menu labels name the starter's primary type")
{
    u32 i;
    u8 prefix[32];
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
    {
        u32 species = ChooseStarter(i);
        u8 *end = StringCopy(prefix, gTypesInfo[gSpeciesInfo[species].types[0]].name);
        // Label format is "<Type> - <Species>"
        EXPECT_EQ(StringCompareN(sMenuLabels[i], prefix, end - prefix), 0);
    }
}

TEST("GymHack: rival always takes one of the three Kanto starters")
{
    u32 i;
    for (i = 0; i < GYM_HACK_STARTER_COUNT; i++)
    {
        ChooseStarter(i);
        EXPECT_LT(VarGet(VAR_TEMP_1), 3);
    }
}

TEST("GymHack: confirming locks in the starter's primary type")
{
    VarSet(VAR_PLAYER_TYPE, TYPE_NONE);
    ChooseStarter(0); // Clefairy: Fairy in modern typing
    GymHack_LockInPlayerType();
    EXPECT_EQ(VarGet(VAR_PLAYER_TYPE), TYPE_FAIRY);

    ChooseStarter(13); // Ralts: Psychic/Fairy, primary Psychic
    GymHack_LockInPlayerType();
    EXPECT_EQ(VarGet(VAR_PLAYER_TYPE), TYPE_PSYCHIC);
}
